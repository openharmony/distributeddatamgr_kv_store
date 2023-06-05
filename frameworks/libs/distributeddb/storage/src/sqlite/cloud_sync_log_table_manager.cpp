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

namespace DistributedDB {
std::string CloudSyncLogTableManager::CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
    const std::string &identity)
{
    (void)identity;
    std::string sql;
    if (table.GetPrimaryKey().size() == 1) {
        sql = "calc_hash(" + references + "'" + table.GetPrimaryKey().at(0)  + "')";
    }  else {
        std::set<std::string> primaryKeySet; // we need sort primary key by name
        for (const auto &it : table.GetPrimaryKey()) {
            primaryKeySet.emplace(it.second);
        }
        sql = "calc_hash(";
        for (const auto &it : primaryKeySet) {
            sql += "calc_hash(" + references + "'" + it + "')||";
        }
        sql.pop_back();
        sql.pop_back();
        sql += ")";
    }
    return sql;
}

void CloudSyncLogTableManager::GetIndexSql(const TableInfo &table, std::vector<std::string> &schema)
{
    const std::string tableName = GetLogTableName(table);

    std::string indexTimestampFlagGid = "CREATE INDEX IF NOT EXISTS " + tableName +
        "_cloud_time_flag_gid_index ON " + tableName + "(timestamp, flag, cloud_gid);";
    schema.emplace_back(indexTimestampFlagGid);
}

std::string CloudSyncLogTableManager::GetPrimaryKeySql(const TableInfo &table)
{
    return "PRIMARY KEY(hash_key)";
}

// The parameter "identity" is a hash string that identifies a device. The same for the next two functions.
std::string CloudSyncLogTableManager::GetInsertTrigger(const TableInfo &table, const std::string &identity)
{
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_INSERT AFTER INSERT \n";
    insertTrigger += "ON '" + table.GetTableName() + "'\n";
    insertTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    insertTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    insertTrigger += "BEGIN\n";
    insertTrigger += "\t INSERT OR REPLACE INTO " + logTblName;
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key, cloud_gid)";
    insertTrigger += " VALUES (new.rowid, '', '',";
    insertTrigger += " get_raw_sys_time(), get_raw_sys_time(), 0x02, ";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ", '');\n";
    insertTrigger += "END;";
    return insertTrigger;
}

std::string CloudSyncLogTableManager::GetUpdateTrigger(const TableInfo &table, const std::string &identity)
{
    (void)identity;
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON '" + table.GetTableName() + "'\n";
    updateTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    updateTrigger += "WHERE key = 'log_trigger_switch' AND value = 'true')\n";
    updateTrigger += "BEGIN\n"; // if user change the primary key, we can still use gid to identify which one is updated
    updateTrigger += "\t UPDATE " + DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    updateTrigger += " SET timestamp=get_raw_sys_time(), device='', flag=0x02";
    updateTrigger += " WHERE data_key = OLD.rowid;\n";
    updateTrigger += "END;";
    return updateTrigger;
}

std::string CloudSyncLogTableManager::GetDeleteTrigger(const TableInfo &table, const std::string &identity)
{
    (void)identity;
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON '" + table.GetTableName() + "'\n";
    deleteTrigger += "WHEN (SELECT count(*) from " + DBConstant::RELATIONAL_PREFIX + "metadata ";
    deleteTrigger += "WHERE key = 'log_trigger_switch' AND VALUE = 'true')\n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += "\t UPDATE " + DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    deleteTrigger += " SET data_key=-1,flag=0x03,timestamp=get_raw_sys_time()";
    deleteTrigger += " WHERE data_key = OLD.rowid;";
    deleteTrigger += "END;";
    return deleteTrigger;
}
} // DistributedDB