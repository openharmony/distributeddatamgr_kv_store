/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "knowledge_log_table_manager.h"

#include "cloud/cloud_storage_utils.h"
#include "res_finalizer.h"

namespace DistributedDB {

std::string KnowledgeLogTableManager::GetUpdateTrigger(const TableInfo &table, const std::string &identity)
{
    (void)identity;
    std::string logTblName = GetLogTableName(table);
    std::string tableName = table.GetTableName();
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + tableName + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON '" + tableName + "'\n";
    updateTrigger += " FOR EACH ROW ";
    updateTrigger += "BEGIN\n";
    updateTrigger += CloudStorageUtils::GetCursorIncSql(tableName);
    updateTrigger.pop_back();
    updateTrigger += " AND " + table.GetTrackerTable().GetDiffTrackerValSql() + ";";
    updateTrigger += "\t UPDATE " + logTblName;
    updateTrigger += " SET timestamp=get_raw_sys_time(), device='', flag=0x02";
    updateTrigger += ", cursor=" + CloudStorageUtils::GetSelectIncCursorSql(tableName);
    updateTrigger += table.GetTrackerTable().GetExtendAssignValSql();
    updateTrigger += " WHERE data_key = OLD." + std::string(DBConstant::SQLITE_INNER_ROWID);
    updateTrigger += GetDiffIncSql(table) + ";\n";
    updateTrigger += "SELECT client_observer('" + tableName + "', OLD." +
        std::string(DBConstant::SQLITE_INNER_ROWID);
    updateTrigger += ", 1, ";
    updateTrigger += table.GetTrackerTable().GetDiffTrackerValSql();
    updateTrigger += ");";
    updateTrigger += "END;";
    return updateTrigger;
}

std::string KnowledgeLogTableManager::GetDiffIncSql(const TableInfo &tableInfo)
{
    const auto &table = tableInfo.GetTrackerTable();
    if (table.IsEmpty()) {
        return "";
    }
    std::string sql = " AND (";
    size_t index = 0;
    for (const auto &colName: table.GetTrackerColNames()) {
        sql += "(NEW." + colName + " IS NOT OLD." + colName + ")";
        if (index < table.GetTrackerColNames().size() - 1) {
            sql += " OR ";
        }
        index++;
    }
    sql += ") ";
    return sql;
}
}