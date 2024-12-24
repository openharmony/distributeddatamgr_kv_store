/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "collaboration_log_table_manager.h"
#include "cloud/cloud_storage_utils.h"
namespace DistributedDB {
bool CollaborationLogTableManager::IsCollaborationWithoutKey(const TableInfo &table)
{
    return ((table.GetIdentifyKey().size() == 1u && table.GetIdentifyKey().at(0) == "rowid") ||
        table.GetAutoIncrement());
}

std::string CollaborationLogTableManager::CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
    const std::string &identity)
{
    std::string sql;
    if (IsCollaborationWithoutKey(table)) {
        sql = "calc_hash('" + identity + "'||calc_hash(" + references + std::string(DBConstant::SQLITE_INNER_ROWID) +
            ", 0), 0)";
    } else {
        if (table.GetIdentifyKey().size() == 1u) {
            sql = "calc_hash(" + references + "'" + table.GetIdentifyKey().at(0) + "', 0)";
        } else {
            sql = "calc_hash(";
            for (const auto &it : table.GetIdentifyKey()) {
                sql += "calc_hash(" + references + "'" + it + "', 0)||";
            }
            sql.pop_back();
            sql.pop_back();
            sql += ", 0)";
        }
    }
    return sql;
}

std::string CollaborationLogTableManager::GetInsertTrigger(const TableInfo &table, const std::string &identity)
{
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_INSERT AFTER INSERT \n";
    insertTrigger += "ON '" + table.GetTableName() + "'\n";
    insertTrigger += "WHEN (SELECT count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata ";
    insertTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    insertTrigger += "BEGIN\n";
    insertTrigger += CloudStorageUtils::GetCursorIncSql(table.GetTableName()) + "\n";
    insertTrigger += "\t INSERT OR REPLACE INTO " + logTblName;
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, cursor)";
    insertTrigger += " VALUES (new." +  std::string(DBConstant::SQLITE_INNER_ROWID) + ", '', '',";
    insertTrigger += " get_sys_time(0), get_last_time(),";
    insertTrigger += " CASE WHEN (SELECT count(*)<>0 FROM " + logTblName + " WHERE hash_key=" +
        CalcPrimaryKeyHash("NEW.", table, identity) + " AND flag&0x02=0x02) THEN 0x22 ELSE 0x02 END,";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ",";
    insertTrigger += CloudStorageUtils::GetSelectIncCursorSql(table.GetTableName()) + ");\n";
    insertTrigger += "SELECT client_observer('" + table.GetTableName() + "', NEW.";
    insertTrigger += std::string(DBConstant::SQLITE_INNER_ROWID) + ", 0, 2);\n"; // 0 is insert, 2 is p2p change
    insertTrigger += "END;";
    return insertTrigger;
}

std::string CollaborationLogTableManager::GetUpdateTrigger(const TableInfo &table, const std::string &identity)
{
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON '" + table.GetTableName() + "'\n";
    updateTrigger += "WHEN (SELECT count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata ";
    updateTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    updateTrigger += "BEGIN\n";
    updateTrigger += CloudStorageUtils::GetCursorIncSql(table.GetTableName()) + "\n";
    if (table.GetIdentifyKey().size() == 1u && table.GetIdentifyKey().at(0) == "rowid") {
        // primary key is rowid, it can't be changed
        updateTrigger += "\t UPDATE " + std::string(DBConstant::RELATIONAL_PREFIX) + table.GetTableName() + "_log";
        updateTrigger += " SET timestamp=" + GetUpdateTimestamp(table, false) + ", device='', flag=0x22, ";
        updateTrigger += "cursor=" + CloudStorageUtils::GetSelectIncCursorSql(table.GetTableName());
        updateTrigger += " WHERE data_key = OLD." +  std::string(DBConstant::SQLITE_INNER_ROWID) + ";";
    } else {
        // insert or replace a new
        // log record(if primary key not change, insert or replace will modify the log record we set deleted in previous
        // step)
        updateTrigger += "\t INSERT INTO " + logTblName + " VALUES (NEW." +
            std::string(DBConstant::SQLITE_INNER_ROWID) + ", '', '', get_sys_time(0), "
            "get_last_time(), CASE WHEN (" + CalcPrimaryKeyHash("NEW.", table, identity) + " != " +
            CalcPrimaryKeyHash("NEW.", table, identity) + ") THEN 0x02 ELSE 0x22 END, " +
            // status not used, default value 0 is UNLOCK.
            CalcPrimaryKeyHash("NEW.", table, identity) + ", '', '', " +
            CloudStorageUtils::GetSelectIncCursorSql(table.GetTableName()) + ", '', '', 0) " +
            "ON CONFLICT(hash_key) DO UPDATE SET timestamp=" + GetUpdateTimestamp(table, false) +
            ", device='', flag=0x22, cursor=" + CloudStorageUtils::GetSelectIncCursorSql(table.GetTableName()) +
            ";\n";
    }
    updateTrigger += "SELECT client_observer('" + table.GetTableName() + "', OLD.";
    updateTrigger += std::string(DBConstant::SQLITE_INNER_ROWID);
    updateTrigger += ", 1, " + GetChangeDataStatus(table) + ");\n"; // 1 is updated
    updateTrigger += "END;";
    return updateTrigger;
}

std::string CollaborationLogTableManager::GetDeleteTrigger(const TableInfo &table, const std::string &identity)
{
    (void)identity;
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON '" + table.GetTableName() + "'\n";
    deleteTrigger += "WHEN (SELECT count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata ";
    deleteTrigger += "WHERE key = 'log_trigger_switch' AND VALUE = 'true')\n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += CloudStorageUtils::GetCursorIncSql(table.GetTableName()) + "\n";
    deleteTrigger += "\t UPDATE " + std::string(DBConstant::RELATIONAL_PREFIX) + table.GetTableName() + "_log";
    deleteTrigger += " SET data_key=-1,flag=0x03,timestamp=get_sys_time(0),";
    deleteTrigger += "cursor=" + CloudStorageUtils::GetSelectIncCursorSql(table.GetTableName());
    deleteTrigger += " WHERE data_key = OLD." +  std::string(DBConstant::SQLITE_INNER_ROWID) + ";\n";
    // first 2 is deleted, second 2 is p2p change
    deleteTrigger += "SELECT client_observer('" + table.GetTableName() + "', -1, 2, 2);\n";
    deleteTrigger += "END;";
    return deleteTrigger;
}

std::string CollaborationLogTableManager::GetPrimaryKeySql(const TableInfo &table)
{
    return "PRIMARY KEY(hash_key)";
}

void CollaborationLogTableManager::GetIndexSql(const TableInfo &table, std::vector<std::string> &schema)
{
    SqliteLogTableManager::GetIndexSql(table, schema);
    std::string dataKeyIndex = "CREATE INDEX IF NOT EXISTS " + std::string(DBConstant::RELATIONAL_PREFIX) +
        "datakey_index ON " + GetLogTableName(table) + "(data_key);";
    schema.emplace_back(dataKeyIndex);
}

std::vector<std::string> CollaborationLogTableManager::GetDropTriggers(const TableInfo &table)
{
    std::vector<std::string> dropTriggers;
    const std::string &tableName = table.GetTableName();
    std::string insertTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_INSERT; ";
    std::string updateTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_UPDATE; ";
    std::string deleteTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_DELETE; ";
    std::string updatePkTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_UPDATE_PK; ";
    dropTriggers.emplace_back(insertTrigger);
    dropTriggers.emplace_back(updateTrigger);
    dropTriggers.emplace_back(deleteTrigger);
    dropTriggers.emplace_back(updatePkTrigger);
    return dropTriggers;
}

std::string CollaborationLogTableManager::GetUpdatePkTrigger(const TableInfo &table, const std::string &identity)
{
    if (table.GetIdentifyKey().size() == 1u && table.GetIdentifyKey().at(0) == "rowid") {
        return "";
    }
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string updatePkTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updatePkTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_UPDATE_PK AFTER UPDATE \n";
    updatePkTrigger += "ON '" + table.GetTableName() + "'\n";
    updatePkTrigger += "WHEN (SELECT count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata ";
    updatePkTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true' AND ";
    updatePkTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + " != " +
        CalcPrimaryKeyHash("OLD.", table, identity);
    updatePkTrigger += ")\n";
    updatePkTrigger += "BEGIN\n";
    // primary key was changed, so we need to set the old log record deleted,
    updatePkTrigger += "\t UPDATE " + logTblName;
    updatePkTrigger += " SET data_key=-1, timestamp=get_sys_time(0), device='', flag=0x03";
    updatePkTrigger += " WHERE data_key = OLD." +  std::string(DBConstant::SQLITE_INNER_ROWID)+ ";\n";
    updatePkTrigger += "END;";
    return updatePkTrigger;
}

std::string CollaborationLogTableManager::GetChangeDataStatus(const TableInfo &table)
{
    // first 2 is p2p change when sync field empty, second 2 is p2p change when sync field update
    return GetUpdateWithAssignSql(table, "2", "2", "0");
}
}