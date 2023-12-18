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

#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "log_table_manager_factory.h"
#include "relational_schema_object.h"
#include "sqlite_relational_utils.h"

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
        LOGE("[Relational][Upgrade] Parse to relational schema failed. err:%d", errCode);
        return errCode;
    }

    std::string trackerSchemaDefine;
    RelationalSchemaObject trackerSchemaObject;
    errCode = SQLiteUtils::GetRelationalSchema(db_, trackerSchemaDefine, DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY);
    if (errCode == E_OK) {
        errCode = trackerSchemaObject.ParseFromTrackerSchemaString(trackerSchemaDefine);
        if (errCode != E_OK) {
            LOGE("[Relational][Upgrade] Parse to tracker schema failed. err:%d", errCode);
            return errCode;
        }
    } else if (errCode != -E_NOT_FOUND) {
        LOGW("[Relational][Upgrade] Get tracker schema from meta return. err:%d", errCode);
        return errCode;
    }

    return UpgradeTriggerBaseOnSchema(schemaObject, trackerSchemaObject);
}

static bool inline NeedUpdateLogTable(const std::string &logTableVersion)
{
    return logTableVersion < DBConstant::LOG_TABLE_VERSION_CURRENT;
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
        LOGE("[Relational][UpgradeLogTable] Parse to relational schema failed. %d", errCode);
        return errCode;
    }

    for (const auto &item : schemaObject.GetTables()) {
        std::string logName = DBCommon::GetLogTableName(item.first);
        errCode = UpgradeLogBaseOnVersion(logTableVersion, logName);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    LOGI("[Relational][UpgradeLogTable] success, ver:%s to ver:%s", logTableVersion.c_str(),
        DBConstant::LOG_TABLE_VERSION_CURRENT.c_str());
    return E_OK;
}

int SqliteRelationalDatabaseUpgrader::UpgradeLogBaseOnVersion(const std::string &oldVersion,
    const std::string &logName)
{
    TableInfo tableInfo;
    int errCode = SQLiteUtils::AnalysisSchemaFieldDefine(db_, logName, tableInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    tableInfo.SetTableName(logName);
    std::vector<std::string> addColSqlVec;
    if (oldVersion < DBConstant::LOG_TABLE_VERSION_3) {
        SQLiteRelationalUtils::AddUpgradeSqlToList(tableInfo, { { "cloud_gid", "text" } }, addColSqlVec);
    }
    if (oldVersion < DBConstant::LOG_TABLE_VERSION_5_1) {
        SQLiteRelationalUtils::AddUpgradeSqlToList(tableInfo, { { "extend_field", "blob" },
            { "cursor", "int" }, { "version", "text" } }, addColSqlVec);
    }
    for (size_t i = 0; i < addColSqlVec.size(); ++i) {
        errCode = SQLiteUtils::ExecuteRawSQL(db_, addColSqlVec[i]);
        if (errCode != E_OK) {
            LOGE("[Relational][UpgradeLogTable] add column failed. err:%d, index:%zu, curVer:%s, maxVer:%s", errCode,
                 i, oldVersion.c_str(), DBConstant::LOG_TABLE_VERSION_CURRENT.c_str());
            return errCode;
        }
    }
    return errCode;
}

int SqliteRelationalDatabaseUpgrader::UpgradeTriggerBaseOnSchema(const RelationalSchemaObject &relationalSchema,
    const RelationalSchemaObject &trackerSchema)
{
    DistributedTableMode mode = relationalSchema.GetTableMode();
    int errCode = E_OK;
    for (const auto &table : relationalSchema.GetTables()) {
        std::vector<std::string> dropSqlVec;
        std::string prefixName = DBConstant::SYSTEM_TABLE_PREFIX + table.first;
        dropSqlVec.push_back("DROP TRIGGER IF EXISTS " + prefixName + "_ON_UPDATE;");
        dropSqlVec.push_back("DROP TRIGGER IF EXISTS " + prefixName + "_ON_INSERT;");
        dropSqlVec.push_back("DROP TRIGGER IF EXISTS " + prefixName + "_ON_DELETE;");
        for (size_t i = 0; i < dropSqlVec.size(); ++i) {
            errCode = SQLiteUtils::ExecuteRawSQL(db_, dropSqlVec[i]);
            if (errCode != E_OK) {
                LOGE("[Relational][Upgrade] drop trigger failed. err:%d, index:%zu", errCode, i);
                return errCode;
            }
        }
        TableInfo tableInfo = table.second;
        tableInfo.SetTrackerTable(trackerSchema.GetTrackerTable(table.first));
        auto manager = LogTableManagerFactory::GetTableManager(mode, tableInfo.GetTableSyncType());
        errCode = manager->AddRelationalLogTableTrigger(db_, tableInfo, "");
        if (errCode != E_OK) {
            LOGE("[Relational][Upgrade] recreate trigger failed. err:%d", errCode);
            return errCode;
        }
    }
    LOGI("[Relational][Upgrade] recreate trigger finish.");
    return E_OK;
}
} // namespace DistributedDB
