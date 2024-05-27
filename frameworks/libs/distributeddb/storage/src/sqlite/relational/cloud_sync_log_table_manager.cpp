/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cloud_sync_log_table_manager.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"

namespace DistributedDB {
std::string CloudSyncLogTableManager::CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
    const std::string &identity)
{
    (void)identity;
    std::string sql;
    FieldInfoMap fieldInfos = table.GetFields();
    if (table.GetPrimaryKey().size() == 1) {
        std::string pkName = table.GetPrimaryKey().at(0);
        if (pkName == "rowid") {
            std::string collateStr = std::to_string(static_cast<uint32_t>(CollateType::COLLATE_NONE));
            // we use _rowid_ to reference sqlite inner rowid to avoid rowid is a user defined column,
            // user can't create distributed table with column named "_rowid_"
            sql = "calc_hash(" + references + "'" + std::string(DBConstant::SQLITE_INNER_ROWID) + "', " +
                collateStr + ")";
        } else {
            if (fieldInfos.find(pkName) == fieldInfos.end()) {
                return sql;
            }
            std::string collateStr = std::to_string(static_cast<uint32_t>(fieldInfos.at(pkName).GetCollateType()));
            sql = "calc_hash(" + references + "'" + pkName  + "', " + collateStr + ")";
        }
    }  else {
        std::set<std::string> primaryKeySet; // we need sort primary key by upper name
        for (const auto &it : table.GetPrimaryKey()) {
            primaryKeySet.emplace(DBCommon::ToUpperCase(it.second));
        }
        sql = "calc_hash(";
        for (const auto &it : primaryKeySet) {
            if (fieldInfos.find(it) == fieldInfos.end()) {
                return sql;
            }
            std::string collateStr = std::to_string(static_cast<uint32_t>(fieldInfos.at(it).GetCollateType()));
            sql += "calc_hash(" + references + "'" + it + "', " + collateStr + ")||";
        }
        sql.pop_back();
        sql.pop_back();
        sql += ", 0)";
    }
    return sql;
}

void CloudSyncLogTableManager::GetIndexSql(const TableInfo &table, std::vector<std::string> &schema)
{
    const std::string tableName = GetLogTableName(table);

    std::string indexTimestampFlagGid = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_cloud_time_flag_gid_index ON " + tableName + "(timestamp, flag, cloud_gid);";
    std::string indexGid = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_cloud_gid_index ON " + tableName + "(cloud_gid);";
    std::string indexDataKey = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_data_key_index ON " + tableName + "(data_key);";
    std::string indexCursor = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_cursor_index ON " + tableName + "(cursor);";
    schema.emplace_back(indexTimestampFlagGid);
    schema.emplace_back(indexGid);
    schema.emplace_back(indexDataKey);
    schema.emplace_back(indexCursor);
}

std::string CloudSyncLogTableManager::GetPrimaryKeySql(const TableInfo &table)
{
    auto primaryKey = table.GetPrimaryKey();
    if (primaryKey[0] == CloudDbConstant::ROW_ID_FIELD_NAME) {
        return "PRIMARY KEY(hash_key, cloud_gid)";
    }
    return "PRIMARY KEY(hash_key)";
}

// The parameter "identity" is a hash string that identifies a device. The same for the next two functions.
std::string CloudSyncLogTableManager::GetInsertTrigger(const TableInfo &table, const std::string &identity)
{
    std::string logTblName = GetLogTableName(table);
    std::string tableName = table.GetTableName();
    std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + tableName + "_ON_INSERT AFTER INSERT \n";
    insertTrigger += "ON '" + tableName + "'\n";
    insertTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    insertTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    insertTrigger += "BEGIN\n";
    insertTrigger += "\t INSERT OR REPLACE INTO " + logTblName;
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, cloud_gid, ";
    insertTrigger += " extend_field, cursor, version, sharing_resource, status)";
    insertTrigger += " VALUES (new." + std::string(DBConstant::SQLITE_INNER_ROWID) + ", '', '',";
    insertTrigger += " get_raw_sys_time(), get_raw_sys_time(), 0x02|0x20, ";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ", CASE WHEN (SELECT count(*)<>0 FROM ";
    insertTrigger += logTblName + " WHERE hash_key = " + CalcPrimaryKeyHash("NEW.", table, identity);
    insertTrigger += ") THEN (SELECT cloud_gid FROM " + logTblName + " WHERE hash_key = ";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ") ELSE '' END, ";
    insertTrigger += table.GetTrackerTable().GetAssignValSql();
    insertTrigger += ", case when (SELECT MIN(_rowid_) IS NOT NULL FROM " + logTblName + ") then ";
    insertTrigger += " (SELECT case when (MAX(cursor) is null) then 1 else MAX(cursor) + 1 END";
    insertTrigger += " FROM " +  logTblName + ")";
    insertTrigger += " ELSE new." + std::string(DBConstant::SQLITE_INNER_ROWID) + " end, '', '', 0);\n";
    insertTrigger += CloudStorageUtils::GetTableRefUpdateSql(table, OpType::INSERT);
    insertTrigger += "SELECT client_observer('" + tableName + "', NEW." + std::string(DBConstant::SQLITE_INNER_ROWID);
    insertTrigger += ", 0, ";
    insertTrigger += (table.GetTrackerTable().GetTrackerColNames().empty() ? "0" : "1");
    insertTrigger += ");\n";
    insertTrigger += "END;";
    return insertTrigger;
}

std::string CloudSyncLogTableManager::GetUpdateTrigger(const TableInfo &table, const std::string &identity)
{
    (void)identity;
    std::string logTblName = GetLogTableName(table);
    std::string tableName = table.GetTableName();
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + tableName + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON '" + tableName + "'\n";
    updateTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    updateTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    updateTrigger += "BEGIN\n"; // if user change the primary key, we can still use gid to identify which one is updated
    updateTrigger += "\t UPDATE " + logTblName;
    updateTrigger += " SET timestamp=get_raw_sys_time(), device='', flag=0x02|0x20";
    if (!table.GetTrackerTable().GetTrackerColNames().empty()) {
        updateTrigger += table.GetTrackerTable().GetExtendAssignValSql();
    }
    updateTrigger += ", cursor = (SELECT case when (MAX(cursor) is null) then 1 else (MAX(cursor) + 1) END ";
    updateTrigger += " from " + logTblName + "), ";
    updateTrigger += CloudStorageUtils::GetUpdateLockChangedSql();
    updateTrigger += " WHERE data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    updateTrigger += CloudStorageUtils::GetTableRefUpdateSql(table, OpType::UPDATE);
    updateTrigger += "select client_observer('" + tableName + "', OLD.";
    updateTrigger += std::string(DBConstant::SQLITE_INNER_ROWID);
    updateTrigger += ", 1, ";
    updateTrigger += table.GetTrackerTable().GetDiffTrackerValSql();
    updateTrigger += ");";
    updateTrigger += "END;";
    return updateTrigger;
}

std::string CloudSyncLogTableManager::GetDeleteTrigger(const TableInfo &table, const std::string &identity)
{
    (void)identity;
    std::string logTblName = GetLogTableName(table);
    std::string tableName = table.GetTableName();
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + tableName + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON '" + tableName + "'\n";
    deleteTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    deleteTrigger += "WHERE key = 'log_trigger_switch' AND VALUE = 'true')\n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += "\t UPDATE " + GetLogTableName(table);
    deleteTrigger += " SET data_key=-1,";
    deleteTrigger += "flag=(CASE WHEN cloud_gid='' then 0x03 else 0x03|0x20 END),";
    deleteTrigger += "timestamp=get_raw_sys_time()";
    if (!table.GetTrackerTable().GetTrackerColNames().empty()) {
        deleteTrigger += table.GetTrackerTable().GetExtendAssignValSql(true);
    }
    deleteTrigger += ", cursor = (SELECT case when (MAX(cursor) is null) then 1 else MAX(cursor) + 1 END ";
    deleteTrigger += " FROM " +  logTblName + "), ";
    deleteTrigger += CloudStorageUtils::GetDeleteLockChangedSql();
    deleteTrigger += " WHERE data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    deleteTrigger += CloudStorageUtils::GetTableRefUpdateSql(table, OpType::DELETE);
    // -1 is rowid when data is deleted, 2 means change type is delete(ClientChangeType)
    deleteTrigger += "SELECT client_observer('" + tableName + "', -1, 2, ";
    deleteTrigger += table.GetTrackerTable().GetTrackerColNames().empty() ? "0" : "1";
    deleteTrigger += ");\n";
    deleteTrigger += "END;";
    return deleteTrigger;
}

std::vector<std::string> CloudSyncLogTableManager::GetDropTriggers(const TableInfo &table)
{
    std::vector<std::string> dropTriggers;
    std::string tableName = table.GetTableName();
    std::string insertTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_INSERT; ";
    std::string updateTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_UPDATE; ";
    std::string deleteTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_DELETE; ";
    dropTriggers.emplace_back(insertTrigger);
    dropTriggers.emplace_back(updateTrigger);
    dropTriggers.emplace_back(deleteTrigger);
    if (table.GetTrackerTable().GetTrackerColNames().empty()) {
        std::string clearExtendSql = "UPDATE " + GetLogTableName(table) + " SET extend_field = '';";
        dropTriggers.emplace_back(clearExtendSql);
    }
    return dropTriggers;
}
} // DistributedDB