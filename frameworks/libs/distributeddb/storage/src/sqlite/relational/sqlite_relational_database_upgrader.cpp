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
#include "simple_tracker_log_table_manager.h"
#include "sqlite_relational_utils.h"

namespace DistributedDB {
SqliteRelationalDatabaseUpgrader::SqliteRelationalDatabaseUpgrader(sqlite3 *db)
    : db_(db)
{}

SqliteRelationalDatabaseUpgrader::~SqliteRelationalDatabaseUpgrader() {}

int SqliteRelationalDatabaseUpgrader::Upgrade()
{
    // read version first, if not newest, start transaction
    std::string logTableVersion;
    int errCode = SQLiteUtils::GetLogTableVersion(db_, logTableVersion);
    if (errCode != E_OK) {
        LOGW("[Relational][Upgrade] Get log table version return. %d", errCode);
        return (errCode == -E_NOT_FOUND) ? E_OK : errCode;
    }
    if (IsNewestVersion(logTableVersion)) {
        return E_OK;
    }

    errCode = BeginUpgrade();
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

    if (IsNewestVersion(logTableVersion)) {
        LOGD("[Relational][UpgradeTrigger] No need upgrade.");
        return E_OK;
    }

    RelationalSchemaObject schemaObj;
    RelationalSchemaObject trackerSchemaObj;
    errCode = GetParseSchema(schemaObj, trackerSchemaObj);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = UpgradeLogTable(logTableVersion, schemaObj, trackerSchemaObj);
    if (errCode != E_OK) {
        LOGE("[Relational][Upgrade] Upgrade log table failed, err = %d.", errCode);
        return errCode;
    }

    return UpgradeTrigger(logTableVersion, schemaObj, trackerSchemaObj);
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

int SqliteRelationalDatabaseUpgrader::UpgradeTrigger(const std::string &logTableVersion,
    const RelationalSchemaObject &schemaObj, const RelationalSchemaObject &trackerSchemaObj)
{
    DistributedTableMode mode = schemaObj.GetTableMode();
    TableInfoMap trackerTables = trackerSchemaObj.GetTrackerTables();
    for (const auto &table : schemaObj.GetTables()) {
        bool isExists = false;
        int errCode = SQLiteUtils::CheckTableExists(db_, table.first, isExists);
        if (errCode == E_OK && !isExists) {
            LOGI("[Relational][UpgradeLogTable] table may has been deleted, skip upgrade distributed trigger.");
            continue;
        }
        TableInfo tableInfo = table.second;
        tableInfo.SetTrackerTable(trackerSchemaObj.GetTrackerTable(table.first));
        auto manager = LogTableManagerFactory::GetTableManager(mode, tableInfo.GetTableSyncType());
        errCode = manager->AddRelationalLogTableTrigger(db_, tableInfo, "");
        if (errCode != E_OK) {
            LOGE("[Relational][Upgrade] recreate distributed trigger failed. err:%d", errCode);
            return errCode;
        }
        trackerTables.erase(table.first);
    }
    // Need to upgrade non-distributed trigger
    auto manager = std::make_unique<SimpleTrackerLogTableManager>();
    for (const auto &table: trackerTables) {
        TableInfo tableInfo;
        int ret = SQLiteRelationalUtils::AnalysisTrackerTable(db_, table.second.GetTrackerTable(), tableInfo);
        if (ret == -E_NOT_FOUND) {
            LOGI("[Relational][Upgrade] table may has been deleted, skip upgrade tracker trigger.");
            continue;
        }
        if (ret != E_OK) {
            LOGE("[Relational][Upgrade] analysis tracker table schema failed %d.", ret);
            return ret;
        }
        ret = manager->AddRelationalLogTableTrigger(db_, tableInfo, "");
        if (ret != E_OK) {
            LOGE("[Relational][Upgrade] recreate trigger failed. err:%d", ret);
            return ret;
        }
    }
    LOGI("[Relational][UpgradeLogTable] recreate trigger success, ver:%s to ver:%s", logTableVersion.c_str(),
        DBConstant::LOG_TABLE_VERSION_CURRENT.c_str());
    return E_OK;
}

int SqliteRelationalDatabaseUpgrader::UpgradeLogTable(const std::string &logTableVersion,
    const RelationalSchemaObject &schemaObj, const RelationalSchemaObject &trackerSchemaObj)
{
    TableInfoMap trackerTables = trackerSchemaObj.GetTrackerTables();
    for (const auto &table : schemaObj.GetTables()) {
        std::string logName = DBCommon::GetLogTableName(table.first);
        int errCode = UpgradeLogBaseOnVersion(logTableVersion, logName);
        if (errCode != E_OK) {
            LOGE("[Relational][UpgradeLogTable] upgrade distributed log table failed. err:%d", errCode);
            return errCode;
        }
        trackerTables.erase(table.first);
    }
    // Need to upgrade non-distributed log table
    for (const auto &table: trackerTables) {
        std::string logName = DBCommon::GetLogTableName(table.first);
        int errCode = UpgradeLogBaseOnVersion(logTableVersion, logName);
        if (errCode != E_OK) {
            LOGE("[Relational][UpgradeLogTable] upgrade tracker log table failed. err:%d", errCode);
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
    tableInfo.SetTableName(logName);
    int errCode = SQLiteUtils::AnalysisSchemaFieldDefine(db_, logName, tableInfo);
    if (errCode == E_OK && tableInfo.Empty()) {
        LOGI("[Relational][UpgradeLogTable] table may has been deleted, skip upgrade log table.");
        return E_OK;
    }
    if (errCode != E_OK) {
        return errCode;
    }
    std::vector<std::string> addColSqlVec;
    if (oldVersion < DBConstant::LOG_TABLE_VERSION_3) {
        SQLiteRelationalUtils::AddUpgradeSqlToList(tableInfo, { { "cloud_gid", "text" } }, addColSqlVec);
    }
    if (oldVersion < DBConstant::LOG_TABLE_VERSION_5_1) {
        SQLiteRelationalUtils::AddUpgradeSqlToList(tableInfo, { { "extend_field", "blob" },
            { "cursor", "int" }, { "version", "text" } }, addColSqlVec);
    }
    if (oldVersion < DBConstant::LOG_TABLE_VERSION_5_3) {
        SQLiteRelationalUtils::AddUpgradeSqlToList(tableInfo, { { "sharing_resource", "text" } }, addColSqlVec);
    }
    if (oldVersion < DBConstant::LOG_TABLE_VERSION_5_5) {
        SQLiteRelationalUtils::AddUpgradeSqlToList(tableInfo, { { "status", "int default 0" } }, addColSqlVec);
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

int SqliteRelationalDatabaseUpgrader::GetParseSchema(RelationalSchemaObject &schemaObj,
    RelationalSchemaObject &trackerSchemaObj)
{
    // get schema from meta
    std::string schemaDefine;
    int errCode = SQLiteUtils::GetRelationalSchema(db_, schemaDefine);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[Relational][UpgradeTrigger] Get relational schema from meta return. err:%d", errCode);
        return errCode;
    }
    if (errCode == -E_NOT_FOUND || schemaDefine.empty()) {
        LOGI("[Relational][UpgradeTrigger] relational schema is empty:%d.", errCode);
    } else {
        errCode = schemaObj.ParseFromSchemaString(schemaDefine);
        if (errCode != E_OK) {
            LOGE("[Relational][UpgradeTrigger] Parse to relational schema failed. err:%d", errCode);
            return errCode;
        }
    }

    std::string trackerSchemaDefine;
    errCode = SQLiteUtils::GetRelationalSchema(db_, trackerSchemaDefine, DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[Relational][UpgradeTrigger] Get tracker schema from meta return. err:%d", errCode);
        return errCode;
    }
    if (errCode == -E_NOT_FOUND || trackerSchemaDefine.empty()) {
        LOGI("[Relational][UpgradeTrigger] tracker schema is empty:%d", errCode);
    } else {
        errCode = trackerSchemaObj.ParseFromTrackerSchemaString(trackerSchemaDefine);
        if (errCode != E_OK) {
            LOGE("[Relational][UpgradeTrigger] Parse to tracker schema failed. err:%d", errCode);
            return errCode;
        }
    }
    return E_OK;
}
} // namespace DistributedDB
