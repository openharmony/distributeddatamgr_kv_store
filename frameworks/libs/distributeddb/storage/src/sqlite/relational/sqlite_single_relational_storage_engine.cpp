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
#ifdef RELATIONAL_STORE
#include "sqlite_single_relational_storage_engine.h"

#include "db_common.h"
#include "db_errno.h"
#include "res_finalizer.h"
#include "sqlite_relational_database_upgrader.h"
#include "sqlite_single_ver_relational_storage_executor.h"


namespace DistributedDB {
SQLiteSingleRelationalStorageEngine::SQLiteSingleRelationalStorageEngine(RelationalDBProperties properties)
    : properties_(properties)
{}

SQLiteSingleRelationalStorageEngine::~SQLiteSingleRelationalStorageEngine()
{}

StorageExecutor *SQLiteSingleRelationalStorageEngine::NewSQLiteStorageExecutor(sqlite3 *dbHandle, bool isWrite,
    bool isMemDb)
{
    auto mode = static_cast<DistributedTableMode>(properties_.GetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE,
        DistributedTableMode::SPLIT_BY_DEVICE));
    return new (std::nothrow) SQLiteSingleVerRelationalStorageExecutor(dbHandle, isWrite, mode);
}

int SQLiteSingleRelationalStorageEngine::Upgrade(sqlite3 *db)
{
    int errCode = CreateRelationalMetaTable(db);
    if (errCode != E_OK) {
        LOGE("Create relational store meta table failed. err=%d", errCode);
        return errCode;
    }
    LOGD("[RelationalEngine][Upgrade] upgrade relational store.");
    auto upgrader = std::make_unique<SqliteRelationalDatabaseUpgrader>(db);
    return upgrader->Upgrade();
}

int SQLiteSingleRelationalStorageEngine::RegisterFunction(sqlite3 *db) const
{
    int errCode = SQLiteUtils::RegisterCalcHash(db);
    if (errCode != E_OK) {
        LOGE("[engine] register calculate hash failed!");
        return errCode;
    }

    errCode = SQLiteUtils::RegisterGetLastTime(db);
    if (errCode != E_OK) {
        LOGE("[engine] register get last time failed!");
        return errCode;
    }

    errCode = SQLiteUtils::RegisterGetSysTime(db);
    if (errCode != E_OK) {
        LOGE("[engine] register get sys time failed!");
        return errCode;
    }

    errCode = SQLiteUtils::RegisterGetRawSysTime(db);
    if (errCode != E_OK) {
        LOGE("[engine] register get raw sys time failed!");
        return errCode;
    }

    errCode = SQLiteUtils::RegisterCloudDataChangeObserver(db);
    if (errCode != E_OK) {
        LOGE("[engine] register cloud observer failed!");
    }

    errCode = SQLiteUtils::RegisterCloudDataChangeServerObserver(db);
    if (errCode != E_OK) {
        LOGE("[engine] register cloud server observer failed!");
    }

    return errCode;
}

int SQLiteSingleRelationalStorageEngine::CreateNewExecutor(bool isWrite, StorageExecutor *&handle)
{
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(option_, db, false);
    if (errCode != E_OK) {
        return errCode;
    }
    do {
        errCode = Upgrade(db); // create meta_data table.
        if (errCode != E_OK) {
            break;
        }

        errCode = RegisterFunction(db);
        if (errCode != E_OK) {
            break;
        }

        handle = NewSQLiteStorageExecutor(db, isWrite, false);
        if (handle == nullptr) {
            LOGE("[Relational] New SQLiteStorageExecutor[%d] for the pool failed.", isWrite);
            errCode = -E_OUT_OF_MEMORY;
            break;
        }
        return E_OK;
    } while (false);

    (void)sqlite3_close_v2(db);
    db = nullptr;
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&handle)
{
    if (handle == nullptr) {
        return E_OK;
    }
    StorageExecutor *databaseHandle = handle;
    Recycle(databaseHandle);
    handle = nullptr;
    return E_OK;
}

void SQLiteSingleRelationalStorageEngine::SetSchema(const RelationalSchemaObject &schema)
{
    std::lock_guard lock(schemaMutex_);
    schema_ = schema;
}

RelationalSchemaObject SQLiteSingleRelationalStorageEngine::GetSchema() const
{
    std::lock_guard lock(schemaMutex_);
    return schema_;
}

namespace {
const std::string DEVICE_TYPE = "device";
const std::string CLOUD_TYPE = "cloud";
const std::string SYNC_TABLE_TYPE = "sync_table_type_";

int SaveSchemaToMetaTable(SQLiteSingleVerRelationalStorageExecutor *handle, const RelationalSchemaObject &schema)
{
    const Key schemaKey(DBConstant::RELATIONAL_SCHEMA_KEY.begin(), DBConstant::RELATIONAL_SCHEMA_KEY.end());
    Value schemaVal;
    DBCommon::StringToVector(schema.ToSchemaString(), schemaVal);
    int errCode = handle->PutKvData(schemaKey, schemaVal); // save schema to meta_data
    if (errCode != E_OK) {
        LOGE("Save schema to meta table failed. %d", errCode);
    }
    return errCode;
}

int SaveTrackerSchemaToMetaTable(SQLiteSingleVerRelationalStorageExecutor *handle,
    const RelationalSchemaObject &schema)
{
    const Key schemaKey(DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY.begin(),
        DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY.end());
    Value schemaVal;
    DBCommon::StringToVector(schema.ToSchemaString(), schemaVal);
    int errCode = handle->PutKvData(schemaKey, schemaVal); // save schema to meta_data
    if (errCode != E_OK) {
        LOGE("Save schema to meta table failed. %d", errCode);
    }
    return errCode;
}

int SaveSyncTableTypeAndDropFlagToMeta(SQLiteSingleVerRelationalStorageExecutor *handle, const std::string &tableName,
    TableSyncType syncType)
{
    Key key;
    DBCommon::StringToVector(SYNC_TABLE_TYPE + tableName, key);
    Value value;
    DBCommon::StringToVector(syncType == DEVICE_COOPERATION ? DEVICE_TYPE : CLOUD_TYPE, value);
    int errCode = handle->PutKvData(key, value);
    if (errCode != E_OK) {
        LOGE("Save sync table type to meta table failed. %d", errCode);
        return errCode;
    }
    DBCommon::StringToVector(DBConstant::TABLE_IS_DROPPED + tableName, key);
    errCode = handle->DeleteMetaData({ key });
    if (errCode != E_OK) {
        LOGE("Save table drop flag to meta table failed. %d", errCode);
    }
    return errCode;
}
}

int SQLiteSingleRelationalStorageEngine::CreateDistributedTable(const std::string &tableName,
    const std::string &identity, bool &schemaChanged, TableSyncType syncType)
{
    std::lock_guard lock(schemaMutex_);
    RelationalSchemaObject schema = schema_;
    bool isUpgraded = false;
    if (schema.GetTable(tableName).GetTableName() == tableName) {
        LOGI("distributed table bas been created.");
        if (schema.GetTable(tableName).GetTableSyncType() != syncType) {
            LOGE("table sync type mismatch.");
            return -E_TYPE_MISMATCH;
        }
        isUpgraded = true;
        int errCode = UpgradeDistributedTable(tableName, schemaChanged, syncType);
        if (errCode != E_OK) {
            LOGE("Upgrade distributed table failed. %d", errCode);
            return errCode;
        }
        // Triggers may need to be rebuilt, no return directly
    } else if (schema.GetTables().size() >= DBConstant::MAX_DISTRIBUTED_TABLE_COUNT) {
        LOGE("The number of distributed tables is exceeds limit.");
        return -E_MAX_LIMITS;
    } else {
        schemaChanged = true;
    }

    return CreateDistributedTable(tableName, isUpgraded, identity, schema, syncType);
}

int SQLiteSingleRelationalStorageEngine::CreateDistributedTable(const std::string &tableName, bool isUpgraded,
    const std::string &identity, RelationalSchemaObject &schema, TableSyncType tableSyncType)
{
    LOGD("Create distributed table.");
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    ResFinalizer finalizer([&handle, this] { this->ReleaseExecutor(handle); });

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }

     errCode = handle->GetOrInitTrackerSchemaFromMeta(trackerSchema_);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        ReleaseExecutor(handle);
        return errCode;
    }

    auto mode = static_cast<DistributedTableMode>(properties_.GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));
    TableInfo table;
    table.SetTableName(tableName);
    table.SetTableSyncType(tableSyncType);
    table.SetTrackerTable(trackerSchema_.GetTrackerTable(tableName));
    errCode = handle->CreateDistributedTable(mode, isUpgraded, identity, table, tableSyncType);
    if (errCode != E_OK) {
        LOGE("create distributed table failed. %d", errCode);
        (void)handle->Rollback();
        return errCode;
    }

    schema.SetTableMode(mode);
    schema.AddRelationalTable(table);
    errCode = SaveSchemaToMetaTable(handle, schema);
    if (errCode != E_OK) {
        LOGE("Save schema to meta table for create distributed table failed. %d", errCode);
        (void)handle->Rollback();
        return errCode;
    }

    errCode = SaveSyncTableTypeAndDropFlagToMeta(handle, tableName, tableSyncType);
    if (errCode != E_OK) {
        (void)handle->Rollback();
        return errCode;
    }

    errCode = handle->Commit();
    if (errCode == E_OK) {
        schema_ = schema;
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::UpgradeDistributedTable(const std::string &tableName, bool &schemaChanged,
    TableSyncType syncType)
{
    LOGD("Upgrade distributed table.");
    RelationalSchemaObject schema = schema_;
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }

    auto mode = static_cast<DistributedTableMode>(properties_.GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));
    errCode = handle->UpgradeDistributedTable(tableName, mode, schemaChanged, schema, syncType);
    if (errCode != E_OK) {
        LOGE("Upgrade distributed table failed. %d", errCode);
        (void)handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }

    errCode = SaveSchemaToMetaTable(handle, schema);
        if (errCode != E_OK) {
        LOGE("Save schema to meta table for upgrade distributed table failed. %d", errCode);
        (void)handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }

    errCode = handle->Commit();
    if (errCode == E_OK) {
        schema_ = schema;
    }
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::CleanDistributedDeviceTable(std::vector<std::string> &missingTables)
{
    int errCode = E_OK;
    auto handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }

    std::lock_guard lock(schemaMutex_);
    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }

    errCode = handle->CheckAndCleanDistributedTable(schema_.GetTableNames(), missingTables);
    if (errCode == E_OK) {
        errCode = handle->Commit();
        if (errCode == E_OK) {
            // Remove non-existent tables from the schema
            for (const auto &tableName : missingTables) {
                schema_.RemoveRelationalTable(tableName);
            }
            SaveSchemaToMetaTable(handle, schema_); // save schema to meta_data
        }
    } else {
        LOGE("Check distributed table failed. %d", errCode);
        (void)handle->Rollback();
    }

    ReleaseExecutor(handle);
    return errCode;
}

const RelationalDBProperties &SQLiteSingleRelationalStorageEngine::GetProperties() const
{
    return properties_;
}

void SQLiteSingleRelationalStorageEngine::SetProperties(const RelationalDBProperties &properties)
{
    properties_ = properties;
}

int SQLiteSingleRelationalStorageEngine::CreateRelationalMetaTable(sqlite3 *db)
{
    std::string sql =
        "CREATE TABLE IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX + "metadata(" \
        "key    BLOB PRIMARY KEY NOT NULL," \
        "value  BLOB);";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("[SQLite] execute create table sql failed, err=%d", errCode);
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::SetTrackerTable(const TrackerSchema &schema)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }
    RelationalSchemaObject tracker = trackerSchema_;
    errCode = handle->GetOrInitTrackerSchemaFromMeta(tracker);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        ReleaseExecutor(handle);
        return errCode;
    }

    tracker.InsertTrackerSchema(schema);
    errCode = handle->CreateTrackerTable(tracker.GetTrackerTable(schema.tableName));
    if (errCode != E_OK) {
        (void)handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }

    if (schema.trackerColNames.empty()) {
        tracker.RemoveTrackerSchema(schema);
    }
    errCode = SaveTrackerSchemaToMetaTable(handle, tracker);
    if (errCode != E_OK) {
        (void)handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }

    errCode = handle->Commit();
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }

    trackerSchema_ = tracker;
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::InitTrackerSchemaFromMeta(const TrackerSchema &schema)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    RelationalSchemaObject tracker = trackerSchema_;
    errCode = handle->GetOrInitTrackerSchemaFromMeta(tracker);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        ReleaseExecutor(handle);
        return errCode;
    }

    tracker.InsertTrackerSchema(schema);
    TableInfo tableInfo;
    errCode = handle->AnalysisTrackerTable(tracker.GetTrackerTable(schema.tableName), tableInfo);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }

    if (schema.trackerColNames.empty()) {
        tracker.RemoveTrackerSchema(schema);
    }

    trackerSchema_ = tracker;
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::GetOrInitTrackerSchemaFromMeta()
{
    RelationalSchemaObject trackerSchema;
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->GetOrInitTrackerSchemaFromMeta(trackerSchema);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }
    for (const auto &iter: trackerSchema.GetTrackerTables()) {
        TableInfo tableInfo;
        errCode = handle->AnalysisTrackerTable(iter.second.GetTrackerTable(), tableInfo);
        if (errCode != E_OK) {
            ReleaseExecutor(handle);
            return errCode;
        }
    }
    trackerSchema_ = trackerSchema;
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::SaveTrackerSchema()
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    RelationalSchemaObject tracker = trackerSchema_;
    errCode = SaveTrackerSchemaToMetaTable(handle, tracker);
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }
    errCode = handle->ExecuteSql(condition, records);
    if (errCode != E_OK) {
        (void)handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }
    errCode = handle->Commit();
    ReleaseExecutor(handle);
    return errCode;
}

RelationalSchemaObject SQLiteSingleRelationalStorageEngine::GetTrackerSchema() const
{
    return trackerSchema_;
}
}
#endif