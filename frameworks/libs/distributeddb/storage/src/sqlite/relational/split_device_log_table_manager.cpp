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

#include "split_device_log_table_manager.h"

namespace DistributedDB {
std::string SplitDeviceLogTableManager::CalcPrimaryKeyHash(const std::string &references, const TableInfo &table,
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

std::string SplitDeviceLogTableManager::GetInsertTrigger(const TableInfo &table, const std::string &identity)
{
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_INSERT AFTER INSERT \n";
    insertTrigger += "ON '" + table.GetTableName() + "'\n";
    insertTrigger += "BEGIN\n";
    insertTrigger += "\t INSERT OR REPLACE INTO " + logTblName;
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key)";
    insertTrigger += " VALUES (new." + std::string(DBConstant::SQLITE_INNER_ROWID) + ", '', '',";
    insertTrigger += " get_sys_time(0), get_last_time(),";
    insertTrigger += " CASE WHEN (SELECT count(*)<>0 FROM " + logTblName + " WHERE hash_key=" +
        CalcPrimaryKeyHash("NEW.", table, identity) + " AND flag&0x02=0x02) THEN 0x22 ELSE 0x02 END,";
    insertTrigger += CalcPrimaryKeyHash("NEW.", table, identity) + ");\n";
    insertTrigger += "END;";
    return insertTrigger;
}

std::string SplitDeviceLogTableManager::GetUpdateTrigger(const TableInfo &table, const std::string &identity)
{
    std::string logTblName = DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON '" + table.GetTableName() + "'\n";
    updateTrigger += "BEGIN\n";
    if (table.GetPrimaryKey().size() == 1 && table.GetPrimaryKey().at(0) == "rowid") {
        // primary key is rowid, it can't be changed
        updateTrigger += "\t UPDATE " + DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
        updateTrigger += " SET timestamp=get_sys_time(0), device='', flag=0x22";
        updateTrigger += " WHERE hash_key=" + CalcPrimaryKeyHash("OLD.", table, identity) +
            " AND flag&0x02=0x02;\n";
    } else {
        // primary key may be changed, so we need to set the old log record deleted, then insert or replace a new
        // log record(if primary key not change, insert or replace will modify the log record we set deleted in previous
        // step)
        updateTrigger += "\t UPDATE " + logTblName;
        updateTrigger += " SET data_key=-1,timestamp=get_sys_time(0), device='', flag=0x03";
        updateTrigger += " WHERE hash_key=" + CalcPrimaryKeyHash("OLD.", table, identity) + " AND flag&0x02=0x02;\n";
        updateTrigger += "\t INSERT OR REPLACE INTO " + logTblName + " VALUES (NEW." +
            std::string(DBConstant::SQLITE_INNER_ROWID) + ", '', '', get_sys_time(0), "
            "get_last_time(), CASE WHEN (" + CalcPrimaryKeyHash("NEW.", table, identity) + " != " +
            CalcPrimaryKeyHash("NEW.", table, identity) + ") THEN 0x02 ELSE 0x22 END, " +
            CalcPrimaryKeyHash("NEW.", table, identity) + ", '', '', '', '', '', 0);\n";
    }
    updateTrigger += "END;";
    return updateTrigger;
}

std::string SplitDeviceLogTableManager::GetDeleteTrigger(const TableInfo &table, const std::string &identity)
{
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON '" + table.GetTableName() + "'\n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += "\t UPDATE " + DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    deleteTrigger += " SET data_key=-1,flag=0x03,timestamp=get_sys_time(0)";
    deleteTrigger += " WHERE hash_key=" + CalcPrimaryKeyHash("OLD.", table, identity) +
            " AND flag&0x02=0x02;\n";
    deleteTrigger += "END;";
    return deleteTrigger;
}

std::string SplitDeviceLogTableManager::GetPrimaryKeySql(const TableInfo &table)
{
    return "PRIMARY KEY(device, hash_key)";
}

std::vector<std::string> SplitDeviceLogTableManager::GetDropTriggers(const TableInfo &table)
{
    std::vector<std::string> dropTriggers;
    std::string tableName = table.GetTableName();
    std::string insertTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_INSERT; ";
    std::string updateTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_UPDATE; ";
    std::string deleteTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + tableName + "_ON_DELETE; ";
    dropTriggers.emplace_back(insertTrigger);
    dropTriggers.emplace_back(updateTrigger);
    dropTriggers.emplace_back(deleteTrigger);
    return dropTriggers;
}
}