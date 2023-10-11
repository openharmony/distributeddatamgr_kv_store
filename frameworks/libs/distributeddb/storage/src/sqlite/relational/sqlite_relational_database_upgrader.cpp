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

#include "sqlite_relational_database_upgrader.h"

#include "db_constant.h"
#include "db_errno.h"
#include "relational_schema_object.h"
#include "log_table_manager_factory.h"

namespace DistributedDB {
SqliteRelationalDatabaseUpgrader::SqliteRelationalDatabaseUpgrader(sqlite3 *db)
    : db_(db)
{}

SqliteRelationalDatabaseUpgrader::~SqliteRelationalDatabaseUpgrader() {}

int SqliteRelationalDatabaseUpgrader::Upgrade()
{
    int errCode = BeginUpgrade();
    if (errCode != E_OK) {
        LOGE("[Relational][Upgrade] Begin upgrade failed. err=%d", errCode);
        return errCode;
    }

    errCode = ExecuteUpgrade();
    if (errCode != E_OK) {
        LOGE("[Relational][Upgrade] Execute upgrade failed. err=%d", errCode);
        (void)EndUpgrade(false);
        return errCode;
    }

    errCode = EndUpgrade(true);
    if (errCode != E_OK) {
        LOGE("[Relational][Upgrade] End upgrade failed. err=%d", errCode);
    }
    return errCode;
}

int SqliteRelationalDatabaseUpgrader::BeginUpgrade()
{
    return SQLiteUtils::BeginTransaction(db_, TransactType::IMMEDIATE);
}

int SqliteRelationalDatabaseUpgrader::ExecuteUpgrade()
{
    std::string logTableVersion;
    int errCode = SQLiteUtils::GetLogTableVersion(db_, logTableVersion);
    if (errCode != E_OK) {
        LOGW("[Relational][Upgrade] Get log table version return %d", errCode);
        return (errCode == -E_NOT_FOUND) ? E_OK : errCode;
    }

    errCode = UpgradeLogTable(logTableVersion);
    if (errCode != E_OK) {
        LOGE("[Relational][Upgrade] Upgrade log table failed, err = %d.", errCode);
        return errCode;
    }

    return UpgradeTrigger(logTableVersion);
}

int SqliteRelationalDatabaseUpgrader::EndUpgrade(bool isSuccess)
{
    if (isSuccess) {
        return SQLiteUtils::CommitTransaction(db_);
    }

    return SQLiteUtils::RollbackTransaction(db_);
}

bool SqliteRelationalDatabaseUpgrader::IsNewestVersion(const std::string &logTableVersion)
{
    return logTableVersion == DBConstant::LOG_TABLE_VERSION_CURRENT;
}

int SqliteRelationalDatabaseUpgrader::UpgradeTrigger(const std::string &logTableVersion)
{
    if (IsNewestVersion(logTableVersion)) {
        LOGD("[Relational][Upgrade] No need upgrade trigger.");
        return E_OK;
    }

    // get schema from meta
    std::string schemaDefine;
    int errCode = SQLiteUtils::GetRelationalSchema(db_, schemaDefine);
    if (errCode != E_OK) {
        LOGW("[Relational][Upgrade] Get relational schema from meta return %d.", errCode);
        return (errCode == -E_NOT_FOUND) ? E_OK : errCode;
    }

    RelationalSchemaObject schemaObject;
    errCode = schemaObject.ParseFromSchemaString(schemaDefine);
    if (errCode != E_OK) {
        LOGE("[Relational][Upgrade] Parse to relational schema failed.", errCode);
        return errCode;
    }

    DistributedTableMode mode = schemaObject.GetTableMode();
    for (const auto &[tableName, tableInfo] : schemaObject.GetTables()) {
        std::string dropTriggerSql = "DROP TRIGGER IF EXISTS " + DBConstant::SYSTEM_TABLE_PREFIX + tableName +
            "_ON_UPDATE;";
        dropTriggerSql += "DROP TRIGGER IF EXISTS " + DBConstant::SYSTEM_TABLE_PREFIX + tableName +
            "_ON_INSERT;";
        dropTriggerSql += "DROP TRIGGER IF EXISTS " + DBConstant::SYSTEM_TABLE_PREFIX + tableName +
            "_ON_DELETE;";
        errCode = SQLiteUtils::ExecuteRawSQL(db_, dropTriggerSql);
        if (errCode != E_OK) {
            LOGE("[Relational][Upgrade] drop trigger failed.", errCode);
            return errCode;
        }
        auto manager = LogTableManagerFactory::GetTableManager(mode, tableInfo.GetTableSyncType());
        errCode = manager->AddRelationalLogTableTrigger(db_, tableInfo, "");
        if (errCode != E_OK) {
            LOGE("[Relational][Upgrade] recreate trigger failed.", errCode);
            return errCode;
        }
    }
    return E_OK;
}

static bool inline NeedUpdateLogTable(const std::string &logTableVersion)
{
    return logTableVersion < DBConstant::LOG_TABLE_VERSION_3;
}

int SqliteRelationalDatabaseUpgrader::UpgradeLogTable(const std::string &logTableVersion)
{
    if (!NeedUpdateLogTable(logTableVersion)) {
        LOGD("[Relational][Upgrade] No need upgrade log table.");
        return E_OK;
    }

    // get schema from meta
    std::string schemaDefine;
    int errCode = SQLiteUtils::GetRelationalSchema(db_, schemaDefine);
    if (errCode != E_OK) {
        LOGW("[Relational][UpgradeLogTable] Get relational schema from meta return %d.", errCode);
        return (errCode == -E_NOT_FOUND) ? E_OK : errCode;
    }

    RelationalSchemaObject schemaObject;
    errCode = schemaObject.ParseFromSchemaString(schemaDefine);
    if (errCode != E_OK) {
        LOGE("[Relational][UpgradeLogTable] Parse to relational schema failed.", errCode);
        return errCode;
    }

    for (const auto &item : schemaObject.GetTables()) {
        std::string addColumnSql = "alter table " + DBConstant::RELATIONAL_PREFIX + item.first +
            "_log add cloud_gid text after hash_key;";
        errCode = SQLiteUtils::ExecuteRawSQL(db_, addColumnSql);
        if (errCode != E_OK) {
            LOGE("[Relational][UpgradeLogTable] add column failed.", errCode);
            return errCode;
        }
    }
    return E_OK;
}
} // namespace DistributedDB
