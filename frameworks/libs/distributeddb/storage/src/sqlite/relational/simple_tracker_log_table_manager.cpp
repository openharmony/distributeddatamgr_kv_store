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

#include "simple_tracker_log_table_manager.h"

namespace DistributedDB {

std::string SimpleTrackerLogTableManager::CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
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

void SimpleTrackerLogTableManager::GetIndexSql(const TableInfo &table, std::vector<std::string> &schema)
{
    const std::string tableName = GetLogTableName(table);

    std::string indexCursor = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_cursor_index ON " + tableName + "(cursor);";
    std::string indexDataKey = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_data_key_index ON " + tableName + "(data_key);";
    schema.emplace_back(indexCursor);
    schema.emplace_back(indexDataKey);
}

std::string SimpleTrackerLogTableManager::GetPrimaryKeySql(const TableInfo &table)
{
    return "PRIMARY KEY(hash_key)";
}

// The parameter "identity" is a hash string that identifies a device. The same for the next two functions.
std::string SimpleTrackerLogTableManager::GetInsertTrigger(const TableInfo &table, const std::string &identity)
{
    if (table.GetTrackerTable().GetTrackerColNames().empty()) {
        return "";
    }
    std::string logTblName = GetLogTableName(table);
    std::string tableName = table.GetTableName();
    std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + tableName + "_ON_INSERT AFTER INSERT \n";
    insertTrigger += "ON '" + tableName + "'\n";
    insertTrigger += " FOR EACH ROW \n";
    insertTrigger += "BEGIN\n";
    insertTrigger += "\t INSERT OR REPLACE INTO " + logTblName;
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, cloud_gid";
    insertTrigger += ", extend_field, cursor, version, sharing_resource, status)";
    insertTrigger += " VALUES (new." + std::string(DBConstant::SQLITE_INNER_ROWID) + ", '', '',";
    insertTrigger += " get_raw_sys_time(), get_raw_sys_time(), 0x02, ";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ", '', ";
    insertTrigger += table.GetTrackerTable().GetAssignValSql();
    insertTrigger += ", case when (SELECT MIN(_rowid_) IS NOT NULL FROM " + logTblName + ") then ";
    insertTrigger += " (SELECT case when (MAX(cursor) is null) then 1 else MAX(cursor) + 1 END";
    insertTrigger += " FROM " +  logTblName + ")";
    insertTrigger += " ELSE new." + std::string(DBConstant::SQLITE_INNER_ROWID) + " end, '', '', 0);\n";
    insertTrigger += "SELECT client_observer('" + tableName + "', NEW._rowid_, 0, ";
    insertTrigger += table.GetTrackerTable().GetTrackerColNames().empty() ? "0" : "1";
    insertTrigger += ");\n";
    insertTrigger += "END;";
    return insertTrigger;
}

std::string SimpleTrackerLogTableManager::GetUpdateTrigger(const TableInfo &table, const std::string &identity)
{
    if (table.GetTrackerTable().GetTrackerColNames().empty()) {
        return "";
    }
    (void)identity;
    std::string logTblName = GetLogTableName(table);
    std::string tableName = table.GetTableName();
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + tableName + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON '" + tableName + "'\n";
    updateTrigger += " FOR EACH ROW ";
    updateTrigger += "BEGIN\n"; // if user change the primary key, we can still use gid to identify which one is updated
    updateTrigger += "\t UPDATE " + logTblName;
    updateTrigger += " SET timestamp=get_raw_sys_time(), device='', flag=0x02";
    updateTrigger += table.GetTrackerTable().GetExtendAssignValSql();
    updateTrigger += table.GetTrackerTable().GetDiffIncCursorSql(logTblName);
    updateTrigger += " where data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    updateTrigger += "select client_observer('" + tableName + "', OLD." + std::string(DBConstant::SQLITE_INNER_ROWID);
    updateTrigger += ", 1, ";
    updateTrigger += table.GetTrackerTable().GetDiffTrackerValSql();
    updateTrigger += ");";
    updateTrigger += "END;";
    return updateTrigger;
}

std::string SimpleTrackerLogTableManager::GetDeleteTrigger(const TableInfo &table, const std::string &identity)
{
    if (table.GetTrackerTable().GetTrackerColNames().empty()) {
        return "";
    }
    (void)identity;
    std::string tableName = table.GetTableName();
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + tableName + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON '" + tableName + "'\n";
    deleteTrigger += " FOR EACH ROW \n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += "\t UPDATE " + GetLogTableName(table);
    deleteTrigger += " SET data_key=-1,flag=0x03,timestamp=get_raw_sys_time()";
    deleteTrigger += table.GetTrackerTable().GetExtendAssignValSql(true);
    deleteTrigger += ", cursor = (SELECT case when (MAX(cursor) is null) then 1 else MAX(cursor) + 1 END ";
    deleteTrigger += " FROM " + GetLogTableName(table) + ")";
    deleteTrigger += " WHERE data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID) + ";";
    // -1 is rowid when data is deleted, 2 means change type is delete(ClientChangeType)
    deleteTrigger += "SELECT client_observer('" + tableName + "', -1, 2, ";
    deleteTrigger += table.GetTrackerTable().GetTrackerColNames().empty() ? "0" : "1";
    deleteTrigger += ");\n";
    deleteTrigger += "END;";
    return deleteTrigger;
}

std::vector<std::string> SimpleTrackerLogTableManager::GetDropTriggers(const TableInfo &table)
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
        std::string deleteLogTable = "DROP TABLE IF EXISTS " + GetLogTableName(table) + ";";
        dropTriggers.emplace_back(deleteLogTable);
    }
    return dropTriggers;
}
}