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
            sql = "calc_hash(" + references + "'" + table.GetPrimaryKey().at(0)  + "', " + collateStr + ")";
        } else {
            if (fieldInfos.find(pkName) == fieldInfos.end()) {
                return sql;
            }
            std::string collateStr = std::to_string(static_cast<uint32_t>(fieldInfos.at(pkName).GetCollateType()));
            sql = "calc_hash(" + references + "'" + table.GetPrimaryKey().at(0)  + "', " + collateStr + ")";
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
    schema.emplace_back(indexTimestampFlagGid);
    schema.emplace_back(indexGid);
}

std::string CloudSyncLogTableManager::GetPrimaryKeySql(const TableInfo &table)
{
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
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, cloud_gid)";
    insertTrigger += " VALUES (new.rowid, '', '',";
    insertTrigger += " get_raw_sys_time(), get_raw_sys_time(), 0x02, ";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ", CASE WHEN (SELECT count(*)<>0 FROM ";
    insertTrigger += logTblName + " where hash_key = " + CalcPrimaryKeyHash("NEW.", table, identity);
    insertTrigger += ") THEN (select cloud_gid from " + logTblName + " where hash_key = ";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ") ELSE '' END);\n";
    insertTrigger += "select client_observer('" + tableName + "', NEW.rowid, 0);\n";
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
    updateTrigger += " SET timestamp=get_raw_sys_time(), device='', flag=0x02";
    updateTrigger += " WHERE data_key = OLD.rowid;\n";
    updateTrigger += "select client_observer('" + tableName + "', OLD.rowid, 1);\n";
    updateTrigger += "END;";
    return updateTrigger;
}

std::string CloudSyncLogTableManager::GetDeleteTrigger(const TableInfo &table, const std::string &identity)
{
    (void)identity;
    std::string tableName = table.GetTableName();
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + tableName + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON '" + tableName + "'\n";
    deleteTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    deleteTrigger += "WHERE key = 'log_trigger_switch' AND VALUE = 'true')\n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += "\t UPDATE " + GetLogTableName(table);
    deleteTrigger += " SET data_key=-1,flag=0x03,timestamp=get_raw_sys_time()";
    deleteTrigger += " WHERE data_key = OLD.rowid;";
    // -1 is rowid when data is deleted, 2 means change type is delete(ClientChangeType)
    deleteTrigger += "select client_observer('" + tableName + "', -1, 2);\n";
    deleteTrigger += "END;";
    return deleteTrigger;
}
} // DistributedDB