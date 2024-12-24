/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cloud/cloud_storage_utils.h"
#include "device_tracker_log_table_manager.h"

namespace DistributedDB {

std::string DeviceTrackerLogTableManager::CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
    const std::string &identity)
{
    (void)identity;
    std::string sql;
    if (table.GetPrimaryKey().size() == 1) {
        sql = "calc_hash(" + references + "'" + table.GetPrimaryKey().at(0)  + "', 0)";
    }  else {
        sql = "calc_hash(";
        for (const auto &it : table.GetPrimaryKey()) {
            sql += "calc_hash(" + references + "'" + it.second + "', 0)||";
        }
        sql.pop_back();
        sql.pop_back();
        sql += ", 0)";
    }
    return sql;
}

void DeviceTrackerLogTableManager::GetIndexSql(const TableInfo &table, std::vector<std::string> &schema)
{
    const std::string tableName = GetLogTableName(table);

    std::string indexCursor = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_cursor_index ON " + tableName + "(cursor);";
    std::string indexDataKey = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_data_key_index ON " + tableName + "(data_key);";
    schema.emplace_back(indexCursor);
    schema.emplace_back(indexDataKey);
}

std::string DeviceTrackerLogTableManager::GetPrimaryKeySql(const TableInfo &table)
{
    return "PRIMARY KEY(hash_key)";
}

// The parameter "identity" is a hash string that identifies a device. The same for the next two functions.
std::string DeviceTrackerLogTableManager::GetInsertTrigger(const TableInfo &table, const std::string &identity)
{
    if (table.GetTrackerTable().IsEmpty()) {
        return "";
    }
    std::string logTblName = GetLogTableName(table);
    std::string tableName = table.GetTableName();
    std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + tableName + "_ON_INSERT AFTER INSERT \n";
    insertTrigger += "ON '" + tableName + "'\n";
    insertTrigger += "WHEN (SELECT count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata ";
    insertTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    insertTrigger += "BEGIN\n";
    insertTrigger += CloudStorageUtils::GetCursorIncSql(tableName) + "\n";
    insertTrigger += "\t INSERT OR REPLACE INTO " + logTblName;
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, cloud_gid";
    insertTrigger += ", extend_field, cursor, version, sharing_resource, status)";
    insertTrigger += " VALUES (new." + std::string(DBConstant::SQLITE_INNER_ROWID) + ", '', '',";
    insertTrigger += " get_sys_time(0), get_last_time(),";
    insertTrigger += " CASE WHEN (SELECT count(*)<>0 FROM " + logTblName + " WHERE hash_key=" +
        CalcPrimaryKeyHash("NEW.", table, identity) + " AND flag&0x02=0x02) THEN 0x22 ELSE 0x02 END,";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ", '', ";
    insertTrigger += table.GetTrackerTable().GetAssignValSql();
    insertTrigger += ", " + CloudStorageUtils::GetSelectIncCursorSql(tableName) + ", '', '', 0);\n";
    insertTrigger += "SELECT client_observer('" + tableName + "', NEW._rowid_, 0, 3";
    insertTrigger += ");\n";
    insertTrigger += "END;";
    return insertTrigger;
}

std::string DeviceTrackerLogTableManager::GetUpdateTrigger(const TableInfo &table, const std::string &identity)
{
    if (table.GetTrackerTable().IsEmpty()) {
        return "";
    }
    (void)identity;
    std::string logTblName = GetLogTableName(table);
    std::string tableName = table.GetTableName();
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + tableName + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON '" + tableName + "'\n";
    updateTrigger += "WHEN (SELECT count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata ";
    updateTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    updateTrigger += "BEGIN\n";
    updateTrigger += CloudStorageUtils::GetCursorIncSql(tableName);
    updateTrigger.pop_back();
    updateTrigger += " AND " + table.GetTrackerTable().GetDiffTrackerValSql() + ";";
    updateTrigger += "\t UPDATE " + logTblName;
    updateTrigger += " SET timestamp=" + GetUpdateTimestamp(table, false) + ", device='', flag=0x22";
    updateTrigger += table.GetTrackerTable().GetExtendAssignValSql();
    updateTrigger += table.GetTrackerTable().GetDiffIncCursorSql(tableName);
    updateTrigger += " WHERE data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    updateTrigger += "SELECT client_observer('" + tableName + "', OLD." + std::string(DBConstant::SQLITE_INNER_ROWID);
    updateTrigger += ", 1, (";
    updateTrigger += table.GetTrackerTable().GetDiffTrackerValSql();
    updateTrigger += ") | (";
    updateTrigger += GetChangeDataStatus(table);
    updateTrigger += "));";
    updateTrigger += "END;";
    return updateTrigger;
}

std::string DeviceTrackerLogTableManager::GetDeleteTrigger(const TableInfo &table, const std::string &identity)
{
    if (table.GetTrackerTable().IsEmpty()) {
        return "";
    }
    (void)identity;
    std::string tableName = table.GetTableName();
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + tableName + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON '" + tableName + "'\n";
    deleteTrigger += "WHEN (SELECT count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata ";
    deleteTrigger += "WHERE key = 'log_trigger_switch' AND VALUE = 'true')\n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += CloudStorageUtils::GetCursorIncSql(tableName) + "\n";
    deleteTrigger += "\t UPDATE " + GetLogTableName(table);
    deleteTrigger += " SET data_key=-1,flag=0x03,timestamp=get_sys_time(0)";
    deleteTrigger += table.GetTrackerTable().GetExtendAssignValSql(true);
    deleteTrigger += ", cursor=" + CloudStorageUtils::GetSelectIncCursorSql(tableName) + "";
    deleteTrigger += " WHERE data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";";
    deleteTrigger += "SELECT client_observer('" + tableName + "', -1, 2, ";
    deleteTrigger += table.GetTrackerTable().IsEmpty() ? "2" : "3";
    deleteTrigger += ");\n";
    deleteTrigger += "END;";
    return deleteTrigger;
}

std::vector<std::string> DeviceTrackerLogTableManager::GetDropTriggers(const TableInfo &table)
{
    std::vector<std::string> dropTriggers;
    std::string tableName = table.GetTableName();
    std::string insertTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_INSERT; ";
    std::string updateTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_UPDATE; ";
    std::string deleteTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_DELETE; ";
    dropTriggers.emplace_back(insertTrigger);
    dropTriggers.emplace_back(updateTrigger);
    dropTriggers.emplace_back(deleteTrigger);
    if (table.GetTrackerTable().IsEmpty()) {
        std::string deleteLogTable = "DROP TABLE IF EXISTS " + GetLogTableName(table) + ";";
        dropTriggers.emplace_back(deleteLogTable);
    }
    return dropTriggers;
}

std::string DeviceTrackerLogTableManager::GetChangeDataStatus(const TableInfo &table)
{
    // first 2 is p2p change when sync field empty, second 2 is p2p change when sync field update
    return GetUpdateWithAssignSql(table, "2", "2", "0");
}
}
