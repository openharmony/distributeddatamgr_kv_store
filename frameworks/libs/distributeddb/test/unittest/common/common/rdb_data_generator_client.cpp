/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "rdb_data_generator.h"

#include "distributeddb_tools_unit_test.h"
#include "log_print.h"

namespace DistributedDBUnitTest {
using namespace DistributedDB;
std::string RDBDataGenerator::GetTypeText(int type)
{
    switch (type) {
        case DistributedDB::TYPE_INDEX<int64_t>:
            return "INTEGER";
        case DistributedDB::TYPE_INDEX<std::string>:
            return "TEXT";
        case DistributedDB::TYPE_INDEX<DistributedDB::Assets>:
            return "ASSETS";
        case DistributedDB::TYPE_INDEX<DistributedDB::Asset>:
            return "ASSET";
        case DistributedDB::TYPE_INDEX<double>:
            return "DOUBLE";
        case DistributedDB::TYPE_INDEX<Bytes>:
            return "BLOB";
        default:
            return "";
    }
}

int RDBDataGenerator::InitDatabaseWithSchemaInfo(const UtDateBaseSchemaInfo &schemaInfo, sqlite3 &db)
{
    int errCode = RelationalTestUtils::ExecSql(&db, "PRAGMA journal_mode=WAL;");
    if (errCode != SQLITE_OK) {
        LOGE("[RDBDataGenerator] Execute sql failed %d", errCode);
        return errCode;
    }
    for (const auto &tableInfo : schemaInfo.tablesInfo) {
        errCode = InitTableWithSchemaInfo(tableInfo, db);
        if (errCode != SQLITE_OK) {
            LOGE("[RDBDataGenerator] Init table failed %d, %s", errCode, tableInfo.name.c_str());
            break;
        }
    }
    return errCode;
}

int RDBDataGenerator::InitTableWithSchemaInfo(const UtTableSchemaInfo &tableInfo, sqlite3 &db)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + tableInfo.name + "(";
    for (const auto &fieldInfo : tableInfo.fieldInfo) {
        sql += "'" + fieldInfo.field.colName + "' " + GetTypeText(fieldInfo.field.type);
        if (fieldInfo.field.primary) {
            sql += " PRIMARY KEY";
            if (fieldInfo.isAutoIncrement) {
                sql += " AUTOINCREMENT";
            }
        }
        if (!fieldInfo.field.nullable) {
            sql += " NOT NULL ON CONFLICT IGNORE";
        }
        sql += ",";
    }
    sql.pop_back();
    sql += ");";
    int errCode = RelationalTestUtils::ExecSql(&db, sql);
    if (errCode != SQLITE_OK) {
        LOGE("[RDBDataGenerator] Execute sql failed %d, sql is %s", errCode, sql.c_str());
    }
    return errCode;
}
}