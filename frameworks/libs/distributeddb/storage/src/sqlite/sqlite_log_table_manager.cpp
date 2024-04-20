/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "sqlite_log_table_manager.h"

namespace DistributedDB {
int SqliteLogTableManager::AddRelationalLogTableTrigger(sqlite3 *db, const TableInfo &table,
    const std::string &identity)
{
    std::vector<std::string> sqls = GetDropTriggers(table);
    std::string insertTrigger = GetInsertTrigger(table, identity);
    if (!insertTrigger.empty()) {
        sqls.emplace_back(insertTrigger);
    }
    std::string updateTrigger = GetUpdateTrigger(table, identity);
    if (!updateTrigger.empty()) {
        sqls.emplace_back(updateTrigger);
    }
    std::string deleteTrigger = GetDeleteTrigger(table, identity);
    if (!deleteTrigger.empty()) {
        sqls.emplace_back(deleteTrigger);
    }
    // add insert,update,delete trigger
    for (const auto &sql : sqls) {
        int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (errCode != E_OK) {
            LOGE("[LogTableManager] execute create log trigger sql failed, errCode=%d", errCode);
            return errCode;
        }
    }
    return E_OK;
}

int SqliteLogTableManager::CreateRelationalLogTable(sqlite3 *db, const TableInfo &table)
{
    const std::string tableName = GetLogTableName(table);
    std::string primaryKey = GetPrimaryKeySql(table);

    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + tableName + "(" \
        "data_key    INT NOT NULL," \
        "device      BLOB," \
        "ori_device  BLOB," \
        "timestamp   INT  NOT NULL," \
        "wtimestamp  INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "hash_key    BLOB NOT NULL," \
        "cloud_gid   TEXT," + \
        "extend_field BLOB," + \
        "cursor INT DEFAULT 0," + \
        "version TEXT DEFAULT ''," + \
        "sharing_resource TEXT DEFAULT ''," + \
        "status INT DEFAULT 0," + \
        primaryKey + ");";
    std::vector<std::string> logTableSchema;
    logTableSchema.emplace_back(createTableSql);
    GetIndexSql(table, logTableSchema);

    for (const auto &sql : logTableSchema) {
        int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (errCode != E_OK) {
            LOGE("[LogTableManager] execute create log table schema failed, errCode=%d", errCode);
            return errCode;
        }
    }
    return E_OK;
}

int SqliteLogTableManager::CreateKvSyncLogTable(sqlite3 *db)
{
    const std::string tableName = "naturalbase_kv_aux_sync_data_log";
    const std::string primaryKey = "PRIMARY KEY(userid, hash_key)";
    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + tableName + "(" \
        "userid    TEXT NOT NULL," + \
        "hash_key  BLOB NOT NULL," + \
        "cloud_gid TEXT," + \
        "version   TEXT," + \
        primaryKey + ");";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, createTableSql);
    if (errCode != E_OK) {
        LOGE("[LogTableManager] execute create cloud log table schema failed, errCode=%d", errCode);
        return errCode;
    }
    return E_OK;
}

void SqliteLogTableManager::GetIndexSql(const TableInfo &table, std::vector<std::string> &schema)
{
    const std::string tableName = GetLogTableName(table);

    std::string indexTimestampFlag = "CREATE INDEX IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX +
        "time_flag_index ON " + tableName + "(timestamp, flag);";
    schema.emplace_back(indexTimestampFlag);

    std::string indexHashkey = "CREATE INDEX IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX +
        "hashkey_index ON " + tableName + "(hash_key);";
    schema.emplace_back(indexHashkey);
}

std::string SqliteLogTableManager::GetLogTableName(const TableInfo &table) const
{
    return DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
}
}