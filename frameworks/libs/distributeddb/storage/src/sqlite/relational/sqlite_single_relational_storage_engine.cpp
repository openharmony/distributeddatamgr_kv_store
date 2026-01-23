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
#include "log_table_manager_factory.h"
#include "res_finalizer.h"
#include "runtime_context.h"
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
    auto mode = GetRelationalProperties().GetDistributedTableMode();
    return new (std::nothrow) SQLiteSingleVerRelationalStorageExecutor(dbHandle, isWrite, mode);
}

int SQLiteSingleRelationalStorageEngine::Upgrade(sqlite3 *db)
{
    int errCode = SQLiteRelationalUtils::CreateRelationalMetaTable(db);
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

    errCode = SQLiteUtils::RegisterRegexpMatchFunction(db);
    if (errCode != E_OK) {
        LOGE("[engine] register regexp match failed!");
    }

    errCode = SQLiteUtils::RegisterIsEntityDuplicateFunction(db);
    if (errCode != E_OK) {
        LOGE("[engine] register is_entity_duplicate match failed!");
    }
    
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::CreateNewExecutor(bool isWrite, StorageExecutor *&handle)
{
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(GetOption(), db, false);
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

void SQLiteSingleRelationalStorageEngine::ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&handle,
    bool isExternal)
{
    if (handle == nullptr) {
        return;
    }
    StorageExecutor *databaseHandle = handle;
    Recycle(databaseHandle, isExternal);
    handle = nullptr;
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
constexpr const char *ASYNC_GEN_LOG_TASK_PREFIX = "async_generate_log_task_";

int SaveSchemaToMetaTable(SQLiteSingleVerRelationalStorageExecutor *handle, const RelationalSchemaObject &schema)
{
    const Key schemaKey(DBConstant::RELATIONAL_SCHEMA_KEY,
        DBConstant::RELATIONAL_SCHEMA_KEY + strlen(DBConstant::RELATIONAL_SCHEMA_KEY));
    Value schemaVal;
    auto schemaStr = schema.ToSchemaString();
    if (schemaStr.size() > SchemaConstant::SCHEMA_STRING_SIZE_LIMIT) {
        LOGE("schema is too large %zu", schemaStr.size());
        return -E_MAX_LIMITS;
    }
    DBCommon::StringToVector(schemaStr, schemaVal);
    int errCode = handle->PutKvData(schemaKey, schemaVal); // save schema to meta_data
    if (errCode != E_OK) {
        LOGE("Save schema to meta table failed. %d", errCode);
    }
    return errCode;
}

int SaveTrackerSchemaToMetaTable(SQLiteSingleVerRelationalStorageExecutor *handle,
    const RelationalSchemaObject &schema)
{
    const Key schemaKey(DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY,
        DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY + strlen(DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY));
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

int SQLiteSingleRelationalStorageEngine::CreateDistributedTable(const CreateDistributedTableParam &param,
    bool &schemaChanged)
{
    const auto &tableName = param.tableName;
    const auto &syncType = param.syncType;
    std::lock_guard<std::mutex> autoLock(createDistributedTableMutex_);
    RelationalSchemaObject schema = GetSchema();
    auto tableInfo = schema.GetTable(tableName);
    tableInfo.SetCloudTable(param.cloudTable);
    bool isUpgraded = false;
    if (DBCommon::CaseInsensitiveCompare(tableInfo.GetTableName(), tableName)) {
        LOGI("distributed table bas been created.");
        if (schema.GetTable(tableName).GetTableSyncType() != syncType) {
            LOGE("table sync type mismatch.");
            return -E_TYPE_MISMATCH;
        }
        isUpgraded = true;
        int errCode = UpgradeDistributedTable(tableInfo, schemaChanged, syncType);
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

    int errCode = CreateDistributedTable(param, isUpgraded, schema);
    if (errCode != E_OK) {
        LOGE("CreateDistributedTable failed. %d", errCode);
        return errCode;
    }
    if (param.isAsync) {
        errCode = TriggerGenLogTask(param.identity);
        if (errCode != E_OK) {
            LOGE("Start async generate cloud log info task failed. %d", errCode);
            RefObject::DecObjRef(this);
            return errCode;
        }
    } else {
        errCode = GenCloudLogInfoIfNeeded(param.tableName, param.identity);
        if (errCode != E_OK) {
            LOGE("Generate cloud log info failed. %d", errCode);
            return errCode;
        }
    }
    if (isUpgraded && (schemaChanged || param.isTrackerSchemaChanged)) {
        // Used for upgrading the stock data of the trackerTable
        errCode = GenLogInfoForUpgrade(tableName, schemaChanged);
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::CreateDistributedSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
    const std::string &tableName, const std::string &sharedTableName, TableSyncType tableSyncType,
    RelationalSchemaObject &schema)
{
    TableInfo table;
    table.SetTableName(sharedTableName);
    table.SetOriginTableName(tableName);
    table.SetSharedTableMark(true);
    table.SetTableSyncType(tableSyncType);
    table.SetTrackerTable(GetTrackerSchema().GetTrackerTable(sharedTableName));
    if (!table.GetTrackerTable().IsEmpty() && tableSyncType == TableSyncType::DEVICE_COOPERATION) { // LCOV_EXCL_BR_LINE
        LOGE("current is trackerTable, not support creating device distributed table.");
        return -E_NOT_SUPPORT;
    }
    bool isUpgraded = schema.GetTable(sharedTableName).GetTableName() == sharedTableName;
    CreateDistributedTableParam param;
    int errCode = CreateDistributedTable(handle, isUpgraded, param, table, schema);
    if (errCode != E_OK) {
        LOGE("create distributed table failed. %d", errCode);
        return errCode;
    }
    std::lock_guard lock(schemaMutex_);
    schema_ = schema;
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::CreateDistributedTable(const CreateDistributedTableParam &param,
    bool isUpgraded, RelationalSchemaObject &schema)
{
    const auto &tableName = param.tableName;
    const auto &tableSyncType = param.syncType;
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

    TableInfo table;
    table.SetTableName(tableName);
    table.SetTableSyncType(tableSyncType);
    table.SetTrackerTable(GetTrackerSchema().GetTrackerTable(tableName));
    table.SetDistributedTable(schema.GetDistributedTable(tableName));
    table.SetCloudTable(param.cloudTable);
    if (isUpgraded) {
        table.SetSourceTableReference(schema.GetTable(tableName).GetTableReference());
    }
    if (param.isAsync && !table.GetTrackerTable().GetTableName().empty()) {
        LOGE("[CreateDistributedTable] not support async create distributed table on tracker table");
        return -E_NOT_SUPPORT;
    }
    if (!table.GetTrackerTable().IsEmpty() && param.syncType == TableSyncType::DEVICE_COOPERATION &&
        GetRelationalProperties().GetDistributedTableMode() == DistributedTableMode::SPLIT_BY_DEVICE) {
        LOGE("not support create distributed table with split mode on tracker table.");
        (void)handle->Rollback();
        return -E_NOT_SUPPORT;
    }
    errCode = CreateDistributedTable(handle, isUpgraded, param, table, schema);
    if (errCode != E_OK) {
        LOGE("create distributed table failed. %d", errCode);
        (void)handle->Rollback();
        return errCode;
    }
    errCode = handle->Commit();
    if (errCode == E_OK) {
        SetSchema(schema);
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::CreateDistributedTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
    bool isUpgraded, const CreateDistributedTableParam &param, TableInfo &table, RelationalSchemaObject &schema)
{
    auto mode = GetRelationalProperties().GetDistributedTableMode();
    TableSyncType tableSyncType = table.GetTableSyncType();
    std::string tableName = table.GetTableName();
    int errCode = handle->InitCursorToMeta(tableName);
    if (errCode != E_OK) {
        LOGE("init cursor to meta failed. %d", errCode);
        return errCode;
    }
    errCode = handle->CreateDistributedTable(mode, isUpgraded, param.identity, table, param.isAsync);
    if (errCode != E_OK) {
        LOGE("create distributed table failed. %d", errCode);
        return errCode;
    }

    schema.SetTableMode(mode);
    // update table if tableName changed
    schema.RemoveRelationalTable(tableName);
    schema.AddRelationalTable(table);
    errCode = SaveSchemaToMetaTable(handle, schema);
    if (errCode != E_OK) {
        LOGE("Save schema to meta table for create distributed table failed. %d", errCode);
        return errCode;
    }

    errCode = SaveSyncTableTypeAndDropFlagToMeta(handle, tableName, tableSyncType);
    if (errCode != E_OK) {
        LOGE("Save sync table type or drop flag to meta table failed. %d", errCode);
        return errCode;
    }
    if (param.isAsync) {
        return AddAsyncGenLogTask(handle, tableName);
    }
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::UpgradeDistributedTable(const TableInfo &tableInfo, bool &schemaChanged,
    TableSyncType syncType)
{
    LOGD("Upgrade distributed table.");
    RelationalSchemaObject schema = GetSchema();
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

    auto mode = GetRelationalProperties().GetDistributedTableMode();
    errCode = handle->UpgradeDistributedTable(tableInfo, mode, schemaChanged, schema, syncType);
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
        SetSchema(schema);
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

    // go fast to check missing tables without transaction
    errCode = handle->CheckAndCleanDistributedTable(schema_.GetTableNames(), missingTables);
    if (errCode == E_OK) {
        if (missingTables.empty()) {
            LOGI("Missing table is empty.");
            ReleaseExecutor(handle);
            return errCode;
        }
    } else {
        LOGE("Get missing distributed table failed. %d", errCode);
        ReleaseExecutor(handle);
        return errCode;
    }
    missingTables.clear();

    std::lock_guard lock(schemaMutex_);
    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        return errCode;
    }

    errCode = handle->CheckAndCleanDistributedTable(schema_.GetTableNames(), missingTables);
    if (errCode == E_OK) {
        // Remove non-existent tables from the schema
        for (const auto &tableName : missingTables) {
            schema_.RemoveRelationalTable(tableName);
        }
        errCode = SaveSchemaToMetaTable(handle, schema_); // save schema to meta_data
        if (errCode != E_OK) {
            LOGE("Save schema to metaTable failed. %d", errCode);
            (void)handle->Rollback();
        } else {
            errCode = handle->Commit();
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

const RelationalDBProperties SQLiteSingleRelationalStorageEngine::GetRelationalProperties() const
{
    std::lock_guard lock(propertiesMutex_);
    return properties_;
}

void SQLiteSingleRelationalStorageEngine::SetProperties(const RelationalDBProperties &properties)
{
    std::lock_guard lock(propertiesMutex_);
    properties_ = properties;
}

int SQLiteSingleRelationalStorageEngine::SetTrackerTable(const TrackerSchema &schema, const TableInfo &tableInfo,
    bool isFirstCreate)
{
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
    RelationalSchemaObject tracker = GetTrackerSchema();
    tracker.InsertTrackerSchema(schema);
    int ret = handle->CreateTrackerTable(tracker.GetTrackerTable(schema.tableName), tableInfo, isFirstCreate);
    if (ret != E_OK && ret != -E_WITH_INVENTORY_DATA) {
        (void)handle->Rollback();
        return ret;
    }
    errCode = SaveSyncTableTypeAndDropFlagToMeta(handle, schema.tableName, tableInfo.GetTableSyncType());
    if (errCode != E_OK) {
        LOGE("[SetTrackerTable] Save sync type to meta table failed: %d", errCode);
        (void)handle->Rollback();
        return errCode;
    }

    if (schema.trackerColNames.empty() && !schema.isTrackAction) {
        tracker.RemoveTrackerSchema(schema);
    }
    errCode = SaveTrackerSchemaToMetaTable(handle, tracker);
    if (errCode != E_OK) {
        (void)handle->Rollback();
        return errCode;
    }

    errCode = handle->Commit();
    if (errCode != E_OK) {
        return errCode;
    }

    SetTrackerSchema(tracker);
    return ret;
}

void SQLiteSingleRelationalStorageEngine::CacheTrackerSchema(const TrackerSchema &schema)
{
    std::lock_guard lock(trackerSchemaMutex_);
    trackerSchema_.InsertTrackerSchema(schema);
    if (!schema.isTrackAction && schema.trackerColNames.empty()) {
        // if isTrackAction be false and trackerColNames is empty, will remove the tracker schema.
        trackerSchema_.RemoveTrackerSchema(schema);
    }
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
    const TableInfoMap tableInfoMap = trackerSchema.GetTrackerTables();
    for (const auto &iter: tableInfoMap) {
        TableInfo tableInfo;
        errCode = handle->AnalysisTrackerTable(iter.second.GetTrackerTable(), tableInfo);
        if (errCode == -E_NOT_FOUND) { // LCOV_EXCL_BR_LINE
            const std::string trackerTableName = iter.second.GetTrackerTable().GetTableName();
            errCode = CleanTrackerDeviceTable({ trackerTableName }, trackerSchema, handle);
            if (errCode != E_OK) {
                LOGE("cancel tracker table failed during db opening. %d", errCode);
                ReleaseExecutor(handle);
                return errCode;
            }
        } else if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
            LOGE("the tracker schema does not match the tracker schema. %d", errCode);
            ReleaseExecutor(handle);
            return errCode;
        }
    }
    SetTrackerSchema(trackerSchema);
    ReleaseExecutor(handle);
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::SaveTrackerSchema(const std::string &tableName, bool isFirstCreate)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    RelationalSchemaObject tracker = GetTrackerSchema();
    errCode = SaveTrackerSchemaToMetaTable(handle, tracker);
    if (errCode != E_OK || !isFirstCreate) {
        ReleaseExecutor(handle);
        return errCode;
    }
    errCode = handle->CheckInventoryData(DBCommon::GetLogTableName(tableName));
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(!condition.readOnly,
        OperatePerm::NORMAL_PERM, errCode, true));
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->ExecuteSql(condition, records);
    ReleaseExecutor(handle, true);
    return errCode;
}

static int CheckReference(const std::vector<TableReferenceProperty> &tableReferenceProperty,
    const RelationalSchemaObject &schema)
{
    for (const auto &reference : tableReferenceProperty) {
        TableInfo sourceTableInfo = schema.GetTable(reference.sourceTableName);
        TableInfo targetTableInfo = schema.GetTable(reference.targetTableName);
        if (strcasecmp(sourceTableInfo.GetTableName().c_str(), reference.sourceTableName.c_str()) != 0 ||
            strcasecmp(targetTableInfo.GetTableName().c_str(), reference.targetTableName.c_str()) != 0) {
            LOGE("can't set reference for table which doesn't create distributed table.");
            return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
        }
        if (sourceTableInfo.GetTableSyncType() != CLOUD_COOPERATION ||
            targetTableInfo.GetTableSyncType() != CLOUD_COOPERATION) {
            LOGE("can't set reference for table which doesn't create distributed table with cloud mode.");
            return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
        }
        if (sourceTableInfo.GetSharedTableMark() || targetTableInfo.GetSharedTableMark()) {
            LOGE("can't set reference for shared table.");
            return -E_NOT_SUPPORT;
        }

        FieldInfoMap sourceFieldMap = sourceTableInfo.GetFields();
        FieldInfoMap targetFieldMap = targetTableInfo.GetFields();
        for (const auto &[sourceFieldName, targetFieldName] : reference.columns) {
            if (sourceFieldMap.find(sourceFieldName) == sourceFieldMap.end() ||
                targetFieldMap.find(targetFieldName) == targetFieldMap.end()) {
                LOGE("reference field doesn't exists in table.");
                return -E_INVALID_ARGS;
            }
        }
    }
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty,
    SQLiteSingleVerRelationalStorageExecutor *handle, std::set<std::string> &clearWaterMarkTables,
    RelationalSchemaObject &schema)
{
    std::lock_guard lock(schemaMutex_);
    schema = schema_;
    int errCode = CheckReference(tableReferenceProperty, schema);
    if (errCode != E_OK) {
        LOGE("check reference failed, errCode = %d.", errCode);
        return errCode;
    }
    int res = handle->GetClearWaterMarkTables(tableReferenceProperty, schema, clearWaterMarkTables);
    if (res != E_OK && res != -E_TABLE_REFERENCE_CHANGED) {
        return res;
    }
    schema.SetReferenceProperty(tableReferenceProperty);
    errCode = SaveSchemaToMetaTable(handle, schema);
    if (errCode != E_OK) {
        LOGE("Save schema to meta table for reference failed. %d", errCode);
        return errCode;
    }
    auto mode = GetRelationalProperties().GetDistributedTableMode();
    for (auto &table : schema.GetTables()) {
        if (table.second.GetTableSyncType() == TableSyncType::DEVICE_COOPERATION) {
            continue;
        }
        TableInfo tableInfo = table.second;
        errCode = handle->RenewTableTrigger(mode, tableInfo, TableSyncType::CLOUD_COOPERATION);
        if (errCode != E_OK) {
            LOGE("renew table trigger for reference failed. %d", errCode);
            return errCode;
        }
    }
    return res;
}

RelationalSchemaObject SQLiteSingleRelationalStorageEngine::GetTrackerSchema() const
{
    std::lock_guard lock(trackerSchemaMutex_);
    return trackerSchema_;
}

void SQLiteSingleRelationalStorageEngine::SetTrackerSchema(const RelationalSchemaObject &trackerSchema)
{
    std::lock_guard lock(trackerSchemaMutex_);
    trackerSchema_ = trackerSchema;
}

int SQLiteSingleRelationalStorageEngine::CleanTrackerData(const std::string &tableName, int64_t cursor)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) { // LCOV_EXCL_BR_LINE
        return errCode;
    }
    TrackerTable trackerTable = GetTrackerSchema().GetTrackerTable(tableName);
    bool isOnlyTrackTable = false;
    RelationalSchemaObject schema = GetSchema();
    if (!trackerTable.IsTableNameEmpty() &&
        !DBCommon::CaseInsensitiveCompare(schema.GetTable(tableName).GetTableName(), tableName)) {
        isOnlyTrackTable = true;
    }
    errCode = handle->CleanTrackerData(tableName, cursor, isOnlyTrackTable);
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::UpgradeSharedTable(const DataBaseSchema &cloudSchema,
    const std::vector<std::string> &deleteTableNames, const std::map<std::string, std::vector<Field>> &updateTableNames,
    const std::map<std::string, std::string> &alterTableNames)
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
    RelationalSchemaObject schema = GetSchema();
    errCode = UpgradeSharedTableInner(handle, cloudSchema, deleteTableNames, updateTableNames, alterTableNames);
    if (errCode != E_OK) {
        handle->Rollback();
        ReleaseExecutor(handle);
        return errCode;
    }
    errCode = handle->Commit();
    if (errCode != E_OK) {
        std::lock_guard lock(schemaMutex_);
        schema_ = schema; // revert schema to the initial state
    }
    ReleaseExecutor(handle);
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::UpgradeSharedTableInner(SQLiteSingleVerRelationalStorageExecutor *&handle,
    const DataBaseSchema &cloudSchema, const std::vector<std::string> &deleteTableNames,
    const std::map<std::string, std::vector<Field>> &updateTableNames,
    const std::map<std::string, std::string> &alterTableNames)
{
    RelationalSchemaObject schema = GetSchema();
    int errCode = DoDeleteSharedTable(handle, deleteTableNames, schema);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] delete shared table or distributed table failed. %d", errCode);
        return errCode;
    }
    errCode = DoUpdateSharedTable(handle, updateTableNames, cloudSchema, schema);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] update shared table or distributed table failed. %d", errCode);
        return errCode;
    }
    errCode = CheckIfExistUserTable(handle, cloudSchema, alterTableNames, schema);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] check local user table failed. %d", errCode);
        return errCode;
    }
    errCode = DoAlterSharedTableName(handle, alterTableNames, schema);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] alter shared table or distributed table failed. %d", errCode);
        return errCode;
    }
    errCode = DoCreateSharedTable(handle, cloudSchema, updateTableNames, alterTableNames, schema);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] create shared table or distributed table failed. %d", errCode);
        return errCode;
    }
    std::lock_guard lock(schemaMutex_);
    schema_ = schema;
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::DoDeleteSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
    const std::vector<std::string> &deleteTableNames, RelationalSchemaObject &schema)
{
    if (deleteTableNames.empty()) {
        return E_OK;
    }
    int errCode = handle->DeleteTable(deleteTableNames);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] delete shared table failed. %d", errCode);
        return errCode;
    }
    std::vector<Key> keys;
    for (const auto &tableName : deleteTableNames) {
        errCode = handle->CleanResourceForDroppedTable(tableName);
        if (errCode != E_OK) {
            LOGE("[RelationalStorageEngine] delete shared distributed table failed. %d", errCode);
            return errCode;
        }
        Key sharedTableKey = DBCommon::GetPrefixTableName(tableName);
        if (sharedTableKey.empty() || sharedTableKey.size() > DBConstant::MAX_KEY_SIZE) {
            LOGE("[RelationalStorageEngine] shared table key is invalid.");
            return -E_INVALID_ARGS;
        }
        keys.push_back(sharedTableKey);
        schema.RemoveRelationalTable(tableName);
    }
    errCode = handle->DeleteMetaData(keys);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] delete meta data failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::DoUpdateSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
    const std::map<std::string, std::vector<Field>> &updateTableNames, const DataBaseSchema &cloudSchema,
    RelationalSchemaObject &localSchema)
{
    if (updateTableNames.empty()) {
        return E_OK;
    }
    int errCode = handle->UpdateSharedTable(updateTableNames);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] update shared table failed. %d", errCode);
        return errCode;
    }
    for (const auto &tableSchema : cloudSchema.tables) {
        if (updateTableNames.find(tableSchema.sharedTableName) != updateTableNames.end()) {
            errCode = CreateDistributedSharedTable(handle, tableSchema.name, tableSchema.sharedTableName,
                TableSyncType::CLOUD_COOPERATION, localSchema);
            if (errCode != E_OK) {
                LOGE("[RelationalStorageEngine] update shared distributed table failed. %d", errCode);
                return errCode;
            }
        }
    }
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::DoAlterSharedTableName(SQLiteSingleVerRelationalStorageExecutor *&handle,
    const std::map<std::string, std::string> &alterTableNames, RelationalSchemaObject &schema)
{
    if (alterTableNames.empty()) {
        return E_OK;
    }
    int errCode = handle->AlterTableName(alterTableNames);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] alter shared table failed. %d", errCode);
        return errCode;
    }
    std::map<std::string, std::string> distributedSharedTableNames;
    for (const auto &tableName : alterTableNames) {
        errCode = handle->DeleteTableTrigger(tableName.first);
        if (errCode != E_OK) {
            LOGE("[RelationalStorageEngine] delete shared table trigger failed. %d", errCode);
            return errCode;
        }
        std::string oldDistributedName = DBCommon::GetLogTableName(tableName.first);
        std::string newDistributedName = DBCommon::GetLogTableName(tableName.second);
        distributedSharedTableNames[oldDistributedName] = newDistributedName;
    }
    errCode = handle->AlterTableName(distributedSharedTableNames);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] alter distributed shared table failed. %d", errCode);
        return errCode;
    }
    for (const auto &[oldTableName, newTableName] : alterTableNames) {
        TableInfo tableInfo = schema.GetTable(oldTableName);
        tableInfo.SetTableName(newTableName);
        schema.AddRelationalTable(tableInfo);
        schema.RemoveRelationalTable(oldTableName);
    }
    errCode = UpdateKvData(handle, alterTableNames);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] update kv data failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::DoCreateSharedTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
    const DataBaseSchema &cloudSchema, const std::map<std::string, std::vector<Field>> &updateTableNames,
    const std::map<std::string, std::string> &alterTableNames, RelationalSchemaObject &schema)
{
    for (auto const &tableSchema : cloudSchema.tables) {
        if (tableSchema.sharedTableName.empty()) {
            continue;
        }
        if (updateTableNames.find(tableSchema.sharedTableName) != updateTableNames.end()) {
            continue;
        }
        bool isUpdated = false;
        for (const auto &alterTableName : alterTableNames) {
            if (alterTableName.second == tableSchema.sharedTableName) {
                isUpdated = true;
                break;
            }
        }
        if (isUpdated) {
            continue;
        }
        int errCode = handle->CreateSharedTable(tableSchema);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = CreateDistributedSharedTable(handle, tableSchema.name, tableSchema.sharedTableName,
            TableSyncType::CLOUD_COOPERATION, schema);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::UpdateKvData(SQLiteSingleVerRelationalStorageExecutor *&handle,
    const std::map<std::string, std::string> &alterTableNames)
{
    std::vector<Key> keys;
    for (const auto &tableName : alterTableNames) {
        Key oldKey = DBCommon::GetPrefixTableName(tableName.first);
        Value value;
        int ret = handle->GetKvData(oldKey, value);
        if (ret == -E_NOT_FOUND) {
            continue;
        }
        if (ret != E_OK) {
            LOGE("[RelationalStorageEngine] get meta data failed. %d", ret);
            return ret;
        }
        keys.push_back(oldKey);
        Key newKey = DBCommon::GetPrefixTableName(tableName.second);
        ret = handle->PutKvData(newKey, value);
        if (ret != E_OK) {
            LOGE("[RelationalStorageEngine] put meta data failed. %d", ret);
            return ret;
        }
    }
    int errCode = handle->DeleteMetaData(keys);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageEngine] delete meta data failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::CheckIfExistUserTable(SQLiteSingleVerRelationalStorageExecutor *&handle,
    const DataBaseSchema &cloudSchema, const std::map<std::string, std::string> &alterTableNames,
    const RelationalSchemaObject &schema)
{
    for (const auto &tableSchema : cloudSchema.tables) {
        if (alterTableNames.find(tableSchema.sharedTableName) != alterTableNames.end()) {
            continue;
        }
        TableInfo tableInfo = schema.GetTable(tableSchema.sharedTableName);
        if (tableInfo.GetSharedTableMark()) {
            continue;
        }
        int errCode = handle->CheckIfExistUserTable(tableSchema.sharedTableName);
        if (errCode != E_OK) {
            LOGE("[RelationalStorageEngine] local exists table. %d", errCode);
            return errCode;
        }
    }
    return E_OK;
}

std::pair<std::vector<std::string>, int> SQLiteSingleRelationalStorageEngine::CalTableRef(
    const std::vector<std::string> &tableNames, const std::map<std::string, std::string> &sharedTableOriginNames)
{
    std::pair<std::vector<std::string>, int> res = { tableNames, E_OK };
    std::map<std::string, std::map<std::string, bool>> reachableReference;
    std::map<std::string, int> tableWeight;
    {
        std::lock_guard lock(schemaMutex_);
        reachableReference = schema_.GetReachableRef();
        tableWeight = schema_.GetTableWeight();
    }
    if (reachableReference.empty()) {
        return res;
    }
    auto reachableWithShared = GetReachableWithShared(reachableReference, sharedTableOriginNames);
    // check dependency conflict
    for (size_t i = 0; i < tableNames.size(); ++i) {
        for (size_t j = i + 1; j < tableNames.size(); ++j) {
            // such as table A B, if dependency is A->B
            // sync should not be A->B, it should be B->A
            // so if A can reach B, it's wrong
            if (reachableWithShared[tableNames[i]][tableNames[j]]) {
                LOGE("[RDBStorageEngine] table %zu reach table %zu", i, j);
                res.second = -E_INVALID_ARGS;
                return res;
            }
        }
    }
    tableWeight = GetTableWeightWithShared(tableWeight, sharedTableOriginNames);
    auto actualTable = DBCommon::GenerateNodesByNodeWeight(tableNames, reachableWithShared, tableWeight);
    res.first.assign(actualTable.begin(), actualTable.end());
    return res;
}

int SQLiteSingleRelationalStorageEngine::CleanTrackerDeviceTable(const std::vector<std::string> &tableNames,
    RelationalSchemaObject &trackerSchemaObj, SQLiteSingleVerRelationalStorageExecutor *&handle)
{
    std::vector<std::string> missingTrackerTables;
    int errCode = handle->CheckAndCleanDistributedTable(tableNames, missingTrackerTables);
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        LOGE("Check tracker table failed. %d", errCode);
        return errCode;
    }
    if (missingTrackerTables.empty()) { // LCOV_EXCL_BR_LINE
        return E_OK;
    }
    for (const auto &tableName : missingTrackerTables) {
        TrackerSchema schema;
        schema.tableName = tableName;
        trackerSchemaObj.RemoveTrackerSchema(schema);
    }
    errCode = SaveTrackerSchemaToMetaTable(handle, trackerSchemaObj); // save schema to meta_data
    if (errCode != E_OK) { // LCOV_EXCL_BR_LINE
        LOGE("Save tracker schema to metaTable failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::GetAsyncGenLogTasks(std::vector<std::pair<int, std::string>> &asyncGenLogTasks)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(false, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    ResFinalizer finalizer([&handle, this] { this->ReleaseExecutor(handle); });
    return GetAsyncGenLogTasks(handle, asyncGenLogTasks);
}

int SQLiteSingleRelationalStorageEngine::GetAsyncGenLogTasks(const SQLiteSingleVerRelationalStorageExecutor *handle,
    std::vector<std::pair<int, std::string>> &asyncGenLogTasks)
{
    std::map<Key, Value> asyncTaskMap;
    Key keyPrefix;
    DBCommon::StringToVector(ASYNC_GEN_LOG_TASK_PREFIX, keyPrefix);
    int errCode = handle->GetKvDataByPrefixKey(keyPrefix, asyncTaskMap);
    if (errCode != E_OK) {
        LOGE("[GetAsyncGenLogTasks] Get async gen log task failed: %d", errCode);
        return errCode;
    }

    uint32_t taskPrefixLen = std::string(ASYNC_GEN_LOG_TASK_PREFIX).size();
    for (const auto &task : std::as_const(asyncTaskMap)) {
        std::string taskTableName;
        DBCommon::VectorToString(task.second, taskTableName);
        std::string taskName;
        DBCommon::VectorToString(task.first, taskName);
        if (taskName.size() < taskPrefixLen) {
            LOGW("[GetAsyncGenLogTasks] Invalid async gen log task name: %s", taskName.c_str());
            continue;
        }
        std::string taskId = taskName.substr(taskPrefixLen);
        if (!DBCommon::IsStringAllDigit(taskId)) {
            LOGW("[GetAsyncGenLogTasks] Invalid async gen log task ID: %s", taskName.c_str());
            continue;
        }
        int curTaskId = std::strtol(taskId.c_str(), nullptr, DBConstant::STR_TO_LL_BY_DEVALUE);
        asyncGenLogTasks.push_back(std::make_pair(curTaskId, std::string(taskTableName)));
    }
    std::sort(asyncGenLogTasks.begin(), asyncGenLogTasks.end());
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::AddAsyncGenLogTask(const SQLiteSingleVerRelationalStorageExecutor *handle,
    const std::string &tableName)
{
    std::vector<std::pair<int, std::string>> asyncGenLogTasks;
    int errCode = GetAsyncGenLogTasks(handle, asyncGenLogTasks);
    if (errCode != E_OK) {
        LOGE("[AddAsyncGenLogTask] Get async gen log task failed: %d", errCode);
        return errCode;
    }
    int curMaxTaskId = 0;
    for (const auto &task : asyncGenLogTasks) {
        curMaxTaskId = std::max(curMaxTaskId, task.first);
        if (tableName == task.second) {
            return E_OK;
        }
    }
    curMaxTaskId++;

    Key curTaskKey;
    DBCommon::StringToVector(std::string(ASYNC_GEN_LOG_TASK_PREFIX) + std::to_string(curMaxTaskId), curTaskKey);
    Value curTaskVal;
    DBCommon::StringToVector(tableName, curTaskVal);
    errCode = handle->PutKvData(curTaskKey, curTaskVal);
    if (errCode != E_OK) {
        LOGE("[AddAsyncGenLogTask] Put async gen log task of table %s failed: %d",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
        return errCode;
    }
    std::lock_guard<std::mutex> lock(genLogTaskStatusMutex_);
    if (genLogTaskStatus_ == GenLogTaskStatus::RUNNING) {
        genLogTaskStatus_ = GenLogTaskStatus::RUNNING_APPENDED;
    }
    return errCode;
}

int SQLiteSingleRelationalStorageEngine::GenCloudLogInfo(const std::string &identity)
{
    std::vector<std::pair<int, std::string>> asyncGenLogTasks;
    bool isTaskAppend = false;
    do {
        asyncGenLogTasks.clear();
        int errCode = GetAsyncGenLogTasks(asyncGenLogTasks);
        if (errCode != E_OK) {
            LOGE("[GenCloudLogInfo] Get async gen log task failed: %d", errCode);
            return errCode;
        }
        if (asyncGenLogTasks.empty()) {
            break;
        }
        errCode = GenCloudLogInfoWithTables(identity, asyncGenLogTasks);
        if (errCode != E_OK) {
            LOGE("[GenCloudLogInfo] Generate cloud log failed: %d", errCode);
            return errCode;
        }
        std::lock_guard<std::mutex> lock(genLogTaskStatusMutex_);
        if (genLogTaskStatus_ == GenLogTaskStatus::RUNNING_APPENDED) {
            genLogTaskStatus_ = GenLogTaskStatus::RUNNING;
            isTaskAppend = true;
        } else if (genLogTaskStatus_ == GenLogTaskStatus::DB_CLOSED) {
            return E_OK;
        } else {
            genLogTaskStatus_ = GenLogTaskStatus::IDLE;
        }
    } while (isTaskAppend);
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::GenCloudLogInfoWithTables(const std::string &identity,
    const std::vector<std::pair<int, std::string>> &taskTables)
{
    int errCode = E_OK;
    auto schema = GetSchema();
    for (const auto &[taskId, tableName] : taskTables) {
        LOGI("[GenCloudLogInfoWithTables] start gen log of table %s",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
        TableInfo table = schema.GetTable(tableName);
        if (table.GetTableName().empty()) {
            LOGW("[GenCloudLogInfoWithTables] gen log for no exist table, skip");
            continue;
        }
        auto mode = GetRelationalProperties().GetDistributedTableMode();
        std::unique_ptr<SqliteLogTableManager> tableManager =
            LogTableManagerFactory::GetTableManager(table, mode, table.GetTableSyncType());
        errCode = GeneLogInfoForExistedDataInBatch(identity, table, tableManager);
        if (errCode != E_OK) {
            LOGE("[GenCloudLogInfoWithTables] Generate log of table %s for exist data failed: %d",
                DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
            return errCode;
        }
        errCode = RemoveAsyncGenLogTask(taskId);
        if (errCode != E_OK) {
            LOGW("[GenCloudLogInfoWithTables] Remove task of table %s for exist data failed: %d",
                DBCommon::StringMiddleMaskingWithLen(tableName).c_str(), errCode);
        }
        {
            std::unique_lock<std::mutex> lock(genLogTaskCvMutex_);
            genLogTaskCv_.notify_all();
        }
        LOGI("[GenCloudLogInfoWithTables] finish gen log of table %s",
            DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
    }
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::GenLogInfoForUpgrade(const std::string &tableName, bool schemaChanged)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true,
        OperatePerm::NORMAL_PERM, errCode));
    if (handle == nullptr) {
        return errCode;
    }
    ResFinalizer finalizer([&handle, this] { this->ReleaseExecutor(handle); });

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }

    TableInfo table = GetSchema().GetTable(tableName);
    errCode = handle->UpgradedLogForExistedData(table, schemaChanged);
    if (errCode != E_OK) {
        LOGE("Upgrade tracker table log failed. %d", errCode);
        (void)handle->Rollback();
        return errCode;
    }
    return handle->Commit();
}

int SQLiteSingleRelationalStorageEngine::GeneLogInfoForExistedDataInBatch(const std::string &identity,
    const TableInfo &tableInfo, std::unique_ptr<SqliteLogTableManager> &logMgrPtr)
{
    int changedCount = 0;
    int batchNum = 0;
    int recordBatchNum = 10; // print log when gen 10 batch
    do {
        if (IsNeedStopGenLogTask()) {
            LOGI("gen log task interrupted.");
            return -E_TASK_INTERRUPTED;
        }
        changedCount = 0;
        int errCode = GeneLogInfoForExistedData(identity, tableInfo, logMgrPtr,
            SQLiteRelationalUtils::BATCH_GEN_LOG_SIZE, changedCount);
        if (errCode != E_OK) {
            LOGE("[GeneLogInfoForExistedDataInBatch] Generate one batch log failed: %d", errCode);
            return errCode;
        }
        batchNum++;
        if (batchNum == recordBatchNum) {
            LOGI("[GeneLogInfoForExistedDataInBatch] Generate 10 batch log finished");
            batchNum = 0;
        }
        std::this_thread::sleep_for(CloudDbConstant::ASYNC_GEN_LOG_INTERVAL);
    } while (changedCount != 0);
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::GeneLogInfoForExistedData(const std::string &identity,
    const TableInfo &tableInfo, std::unique_ptr<SqliteLogTableManager> &logMgrPtr, uint32_t limitNum, int &changedCount)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true,
        OperatePerm::NORMAL_PERM, errCode));
    if (handle == nullptr) {
        return errCode;
    }
    ResFinalizer finalizer([handle, this] {
        SQLiteSingleVerRelationalStorageExecutor *releaseHandle = handle;
        this->ReleaseExecutor(releaseHandle);
    });
    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }
    auto transactionStart = std::chrono::steady_clock::now();
    sqlite3 *db = nullptr;
    if (handle->GetDbHandle(db) != E_OK) {
        LOGE("[GeneOneBatchLogInfoForExistedData] invalid db");
        (void)handle->Rollback();
        return -E_INVALID_DB;
    }
    SQLiteRelationalUtils::GenLogParam param = {
        db, handle->IsMemory(), false, limitNum
    };
    errCode = SQLiteRelationalUtils::GeneLogInfoForExistedData(identity, tableInfo, logMgrPtr, param);
    if (errCode != E_OK) {
        (void)handle->Rollback();
        LOGE("[GeneOneBatchLogInfoForExistedData] Generate one batch log failed: %d", errCode);
        return errCode;
    }
    changedCount = sqlite3_changes(db);
    errCode = handle->SetLogTriggerStatus(true);
    if (errCode != E_OK) {
        (void)handle->Rollback();
        return errCode;
    }
    auto duration = std::chrono::steady_clock::now() - transactionStart;
    if (duration > CloudDbConstant::LONG_TIME_TRANSACTION) {
        LOGI("[GeneOneBatchLogInfoForExistedData] Generate one batch log have cost %" PRId64 "ms.", duration.count());
    }
    return handle->Commit();
}

int SQLiteSingleRelationalStorageEngine::GenCloudLogInfoIfNeeded(const std::string &tableName,
    const std::string &identity)
{
    std::vector<std::pair<int, std::string>> asyncGenLogTasks;
    int errCode = GetAsyncGenLogTasks(asyncGenLogTasks);
    if (errCode != E_OK) {
        LOGE("[GenCloudLogInfoIfNeeded] get async gen log tasks failed: %d", errCode);
        return -E_INVALID_DB;
    }

    for (const auto &[taskId, taskTableName] : asyncGenLogTasks) {
        if (tableName != taskTableName) {
            continue;
        }
        TableInfo tableInfo = GetSchema().GetTable(tableName);
        auto mode = GetRelationalProperties().GetDistributedTableMode();
        std::unique_ptr<SqliteLogTableManager> tableManager =
            LogTableManagerFactory::GetTableManager(tableInfo, mode, tableInfo.GetTableSyncType());
        int changedCount = 0;
        errCode = GeneLogInfoForExistedData(identity, tableInfo, tableManager, 0u, changedCount);
        if (errCode != E_OK) {
            LOGE("[GenCloudLogInfoIfNeeded] gen log failed: %d, changedCount: %d", errCode, changedCount);
            return errCode;
        }
        return RemoveAsyncGenLogTask(taskId);
    }
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::RemoveAsyncGenLogTask(int taskId)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true,
        OperatePerm::NORMAL_PERM, errCode));
    if (handle == nullptr) {
        LOGE("[RemoveAsyncGenLogTask] Get handle failed to remove async gen log task: %d", errCode);
        return errCode;
    }

    Key curTaskKey;
    DBCommon::StringToVector(std::string(ASYNC_GEN_LOG_TASK_PREFIX) + std::to_string(taskId), curTaskKey);
    errCode = handle->DeleteMetaData({curTaskKey});
    ReleaseExecutor(handle);
    if (errCode != E_OK) {
        LOGE("[RemoveAsyncGenLogTask] Remove async gen log task %d failed: %d", taskId, errCode);
    }
    return errCode;
}

bool SQLiteSingleRelationalStorageEngine::IsNeedStopGenLogTask()
{
    std::lock_guard<std::mutex> lock(genLogTaskStatusMutex_);
    return (genLogTaskStatus_ == GenLogTaskStatus::DB_CLOSED) || (genLogTaskStatus_ == GenLogTaskStatus::INTERRUPTED);
}

std::map<std::string, std::map<std::string, bool>> SQLiteSingleRelationalStorageEngine::GetReachableWithShared(
    const std::map<std::string, std::map<std::string, bool>> &reachableReference,
    const std::map<std::string, std::string> &tableToShared)
{
    // we translate all origin table to shared table
    std::map<std::string, std::map<std::string, bool>> reachableWithShared;
    for (const auto &[source, reach] : reachableReference) {
        bool sourceHasNoShared = tableToShared.find(source) == tableToShared.end();
        for (const auto &[target, isReach] : reach) {
            // merge two reachable reference
            reachableWithShared[source][target] = isReach;
            if (sourceHasNoShared || tableToShared.find(target) == tableToShared.end()) {
                continue;
            }
            // record shared reachable reference
            reachableWithShared[tableToShared.at(source)][tableToShared.at(target)] = isReach;
        }
    }
    return reachableWithShared;
}

std::map<std::string, int> SQLiteSingleRelationalStorageEngine::GetTableWeightWithShared(
    const std::map<std::string, int> &tableWeight, const std::map<std::string, std::string> &tableToShared)
{
    std::map<std::string, int> res;
    for (const auto &[table, weight] : tableWeight) {
        res[table] = weight;
        if (tableToShared.find(table) == tableToShared.end()) {
            continue;
        }
        res[tableToShared.at(table)] = weight;
    }
    return res;
}

int SQLiteSingleRelationalStorageEngine::UpdateExtendField(const DistributedDB::TrackerSchema &schema)
{
    if (schema.extendColNames.empty()) {
        return E_OK;
    }
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true,
        OperatePerm::NORMAL_PERM, errCode));
    if (handle == nullptr) {
        return errCode;
    }
    ResFinalizer finalizer([&handle, this] { this->ReleaseExecutor(handle); });

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = handle->UpdateExtendField(schema.tableName, schema.extendColNames);
    if (errCode != E_OK) {
        LOGE("[%s [%zu]] Update extend field failed. %d",
            DBCommon::StringMiddleMasking(schema.tableName).c_str(), schema.tableName.size(), errCode);
        (void)handle->Rollback();
        return errCode;
    }

    RelationalSchemaObject tracker = GetTrackerSchema();
    TrackerTable oldTrackerTable = tracker.GetTrackerTable(schema.tableName);
    const std::set<std::string>& oldExtendColNames = oldTrackerTable.GetExtendNames();
    const std::string lowVersionExtendColName = oldTrackerTable.GetExtendName();
    if (!oldExtendColNames.empty()) {
        errCode = handle->UpdateDeleteDataExtendField(schema.tableName, lowVersionExtendColName,
            oldExtendColNames, schema.extendColNames);
        if (errCode != E_OK) {
            LOGE("[%s [%zu]] Update extend field for delete data failed. %d",
                DBCommon::StringMiddleMasking(schema.tableName).c_str(), schema.tableName.size(), errCode);
            (void)handle->Rollback();
            return errCode;
        }
    }

    // Try clear historical mismatched log, which usually do not occur and apply to tracker table only.
    if (GetSchema().GetTable(schema.tableName).Empty()) {
        handle->ClearLogOfMismatchedData(schema.tableName);
    }
    return handle->Commit();
}

std::pair<int, bool> SQLiteSingleRelationalStorageEngine::SetDistributedSchema(const DistributedSchema &schema,
    const std::string &localIdentity, bool isForceUpgrade)
{
    std::lock_guard<std::mutex> autoLock(createDistributedTableMutex_);
    auto schemaObj = GetSchema();
    std::pair<int, bool> res = {E_OK, schemaObj.CheckDistributedSchemaChange(schema)};
    auto &[errCode, isSchemaChange] = res;
    if (GetRelationalProperties().GetDistributedTableMode() == DistributedTableMode::SPLIT_BY_DEVICE) {
        LOGE("tableMode SPLIT_BY_DEVICE not support set distributed schema");
        errCode = -E_NOT_SUPPORT;
        return res;
    }
    if (!isSchemaChange) {
        return res;
    }
    auto localSchema = schemaObj.GetDistributedSchema();
    if (localSchema.version != 0 && localSchema.version >= schema.version) {
        LOGE("new schema version no upgrade old:%" PRIu32 " new:%" PRIu32, localSchema.version, schema.version);
        errCode = -E_INVALID_ARGS;
    } else {
        errCode = SetDistributedSchemaInner(schemaObj, schema, localIdentity, isForceUpgrade);
    }
    if (errCode == E_OK) {
        SetSchema(schemaObj);
    }
    return res;
}

int SQLiteSingleRelationalStorageEngine::SetDistributedSchemaInner(RelationalSchemaObject &schemaObj,
    const DistributedSchema &schema, const std::string &localIdentity, bool isForceUpgrade)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    ResFinalizer resFinalizer([this, handle]() {
        auto rdbHandle = handle;
        ReleaseExecutor(rdbHandle);
    });

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SetDistributedSchemaInTraction(schemaObj, schema, localIdentity, isForceUpgrade, *handle);
    if (errCode != E_OK) {
        (void)handle->Rollback();
        return errCode;
    }
    return handle->Commit();
}

int SQLiteSingleRelationalStorageEngine::SetDistributedSchemaInTraction(RelationalSchemaObject &schemaObj,
    const DistributedSchema &schema, const std::string &localIdentity, bool isForceUpgrade,
    SQLiteSingleVerRelationalStorageExecutor &handle)
{
    int errCode = SQLiteRelationalUtils::CheckDistributedSchemaValid(schemaObj, schema, isForceUpgrade, &handle);
    if (errCode != E_OK) {
        return errCode;
    }
    auto changeStatus = schemaObj.GetTableChangeStatus(schema);
    schemaObj.SetDistributedSchema(schema);
    for (const auto &table : schema.tables) {
        if (!changeStatus[table.tableName]) {
            continue;
        }
        TableInfo tableInfo = schemaObj.GetTable(table.tableName);
        tableInfo.SetTrackerTable(GetTrackerSchema().GetTrackerTable(table.tableName));
        if (tableInfo.Empty()) {
            continue;
        }
        tableInfo.SetDistributedTable(schemaObj.GetDistributedTable(table.tableName));
        errCode = handle.RenewTableTrigger(schemaObj.GetTableMode(), tableInfo, tableInfo.GetTableSyncType(),
            localIdentity);
        if (errCode != E_OK) {
            LOGE("Failed to refresh %s:%zu trigger while setting up distributed schema: %d",
                DBCommon::StringMiddleMasking(table.tableName).c_str(), table.tableName.size(), errCode);
            return errCode;
        }
        errCode = handle.UpdateHashKey(schemaObj.GetTableMode(), tableInfo, tableInfo.GetTableSyncType(),
            localIdentity);
        if (errCode != E_OK) {
            LOGE("Failed to update %s:%zu hash_key while setting up distributed schema: %d",
                DBCommon::StringMiddleMasking(table.tableName).c_str(), table.tableName.size(), errCode);
            return errCode;
        }
    }
    errCode = SaveSchemaToMetaTable(&handle, schemaObj);
    if (errCode != E_OK) {
        LOGE("Save schema to meta table for set distributed schema failed. %d", errCode);
    }
    return errCode;
}

void SQLiteSingleRelationalStorageEngine::StopGenLogTask(bool isCloseDb)
{
    if (isCloseDb) {
        SetGenLogTaskStatus(GenLogTaskStatus::DB_CLOSED);
    } else {
        SetGenLogTaskStatus(GenLogTaskStatus::INTERRUPTED);
    }
}

int SQLiteSingleRelationalStorageEngine::StopGenLogTaskWithTables(const std::vector<std::string> &tables)
{
    if (tables.empty()) {
        StopGenLogTask();
        return E_OK;
    }
    std::vector<std::pair<int, std::string>> asyncGenLogTasks;
    int errCode = GetAsyncGenLogTasks(asyncGenLogTasks);
    if (errCode != E_OK) {
        LOGE("[StopGenLogTaskWithTables] get async gen log tasks failed: %d", errCode);
        return errCode;
    }
    std::set<std::string> tableSet(tables.begin(), tables.end());
    for (const auto &[taskId, tableName] : asyncGenLogTasks) {
        if (tableSet.count(tableName) > 0) {
            LOGI("[StopGenLogTaskWithTables] exist table async gen log when stop task");
            StopGenLogTask();
            break;
        }
    }
    return E_OK;
}

void SQLiteSingleRelationalStorageEngine::ResetGenLogTaskStatus()
{
    std::lock_guard<std::mutex> lock(genLogTaskStatusMutex_);
    if (genLogTaskStatus_ == GenLogTaskStatus::INTERRUPTED) {
        genLogTaskStatus_ = GenLogTaskStatus::IDLE;
    }
}

void SQLiteSingleRelationalStorageEngine::SetGenLogTaskStatus(GenLogTaskStatus status)
{
    std::lock_guard<std::mutex> lock(genLogTaskStatusMutex_);
    if (genLogTaskStatus_ == GenLogTaskStatus::DB_CLOSED) {
        return;
    }
    genLogTaskStatus_ = status;
}

int SQLiteSingleRelationalStorageEngine::WaitAsyncGenLogTaskFinished(const std::vector<std::string> &tables,
    const std::string &identity)
{
    int errCode = E_OK;
    auto waitStart = std::chrono::steady_clock::now();
    std::chrono::microseconds duration(0);
    do {
        std::vector<std::pair<int, std::string>> asyncGenLogTasks;
        if (IsNeedStopWaitGenLogTask(tables, asyncGenLogTasks, errCode)) {
            return errCode;
        }
        bool isNeedStartGenLogTask = false;
        {
            std::lock_guard<std::mutex> lock(genLogTaskStatusMutex_);
            isNeedStartGenLogTask = (genLogTaskStatus_ == GenLogTaskStatus::IDLE);
            if (isNeedStartGenLogTask) {
                genLogTaskStatus_ = GenLogTaskStatus::RUNNING_BEFORE_SYNC;
            }
        }
        if (isNeedStartGenLogTask) {
            errCode = GenCloudLogInfoWithTables(identity, asyncGenLogTasks);
            {
                std::lock_guard<std::mutex> lock(genLogTaskStatusMutex_);
                if (genLogTaskStatus_ == GenLogTaskStatus::RUNNING_BEFORE_SYNC) {
                    genLogTaskStatus_ = GenLogTaskStatus::IDLE;
                }
            }
            if (errCode != E_OK) {
                LOGE("[WaitAsyncGenLogTaskFinished] Trigger async gen log task failed: %d", errCode);
                return errCode;
            }
            std::unique_lock<std::mutex> lock(genLogTaskCvMutex_);
            genLogTaskCv_.notify_all();
            break;
        } else {
            std::unique_lock<std::mutex> lock(genLogTaskCvMutex_);
            genLogTaskCv_.wait_for(lock, SYNC_WAIT_GEN_LOG_ONCE_TIME);
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                waitStart);
            LOGI("[WaitAsyncGenLogTaskFinished] Waiting for the log generation task to finished before cloud sync.");
        }
    } while (duration < SYNC_WAIT_GEN_LOG_MAX_TIME);
    if (duration >= SYNC_WAIT_GEN_LOG_MAX_TIME) {
        LOGE("[WaitAsyncGenLogTaskFinished] Waiting for the log generation task timeout");
        return -E_TIMEOUT;
    }
    return E_OK;
}

bool SQLiteSingleRelationalStorageEngine::IsNeedStopWaitGenLogTask(const std::vector<std::string> &tables,
    std::vector<std::pair<int, std::string>> &asyncGenLogTasks, int &errCode)
{
    if (IsNeedStopGenLogTask()) {
        errCode = -E_TASK_INTERRUPTED;
        return true;
    }
    std::set<std::string> tableSet(tables.begin(), tables.end());
    errCode = GetAsyncGenLogTasksWithTables(tableSet, asyncGenLogTasks);
    if (errCode != E_OK) {
        LOGE("[WaitAsyncGenLogTaskFinished] Check async gen log task failed: %d", errCode);
        return true;
    }
    return asyncGenLogTasks.empty();
}

int SQLiteSingleRelationalStorageEngine::GetAsyncGenLogTasksWithTables(const std::set<std::string> &tables,
    std::vector<std::pair<int, std::string>> &tasks)
{
    std::vector<std::pair<int, std::string>> asyncGenLogTasks;
    int errCode = GetAsyncGenLogTasks(asyncGenLogTasks);
    if (errCode != E_OK) {
        LOGE("[CheckAsyncGenLogTasks] Get async gen log task failed: %d", errCode);
        return errCode;
    }
    for (const auto &task : asyncGenLogTasks) {
        if (tables.find(task.second) != tables.end()) {
            tasks.push_back(task);
        }
    }
    return E_OK;
}

int SQLiteSingleRelationalStorageEngine::TriggerGenLogTask(const std::string &identity)
{
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, identity]() {
        bool isNeedStartTask = false;
        {
            std::lock_guard<std::mutex> lock(genLogTaskStatusMutex_);
            if (genLogTaskStatus_ == GenLogTaskStatus::IDLE ||
                genLogTaskStatus_ == GenLogTaskStatus::RUNNING_BEFORE_SYNC) {
                genLogTaskStatus_ = GenLogTaskStatus::RUNNING;
                isNeedStartTask = true;
            }
        }
        if (isNeedStartTask) {
            int ret = GenCloudLogInfo(identity);
            if (ret != E_OK && ret != -E_TASK_INTERRUPTED) {
                LOGE("[TriggerGenLogTask] Gen cloud log info failed: %d", ret);
                SetGenLogTaskStatus(GenLogTaskStatus::IDLE);
            }
        }
        RefObject::DecObjRef(this);
    });
    return errCode;
}

std::pair<int, TableInfo> SQLiteSingleRelationalStorageEngine::AnalyzeTable(const std::string &tableName)
{
    std::pair<int, TableInfo> res;
    auto &[errCode, _] = res;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(false, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return res;
    }
    ResFinalizer resFinalizer([this, handle]() {
        auto rdbHandle = handle;
        ReleaseExecutor(rdbHandle);
    });
    return handle->AnalyzeTable(tableName);
}

#ifdef USE_DISTRIBUTEDDB_CLOUD
int SQLiteSingleRelationalStorageEngine::PutCloudGid(const std::string &tableName, std::vector<VBucket> &data)
{
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(FindExecutor(true, OperatePerm::NORMAL_PERM,
        errCode));
    if (handle == nullptr) {
        return errCode;
    }
    ResFinalizer finalizer([this, handle]() {
        auto releaseHandle = handle;
        ReleaseExecutor(releaseHandle);
    });
    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = handle->PutCloudGid(tableName, data);
    if (errCode == E_OK) {
        errCode = handle->Commit();
        if (errCode != E_OK) {
            LOGE("[RDBEngine] Commit transaction failed[%d] when put cloud gid", errCode);
        }
    } else {
        int ret = handle->Rollback();
        if (ret != E_OK) {
            LOGW("[RDBEngine] Rollback transaction failed[%d] when put cloud gid", ret);
        }
    }
    return errCode;
}
#endif
}
#endif