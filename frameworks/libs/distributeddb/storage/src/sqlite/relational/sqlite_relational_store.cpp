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
#include "sqlite_relational_store.h"

#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "db_constant.h"
#include "db_dump_helper.h"
#include "db_errno.h"
#include "log_print.h"
#include "db_types.h"
#include "sqlite_log_table_manager.h"
#include "sqlite_relational_store_connection.h"
#include "storage_engine_manager.h"
#include "cloud_sync_utils.h"

namespace DistributedDB {
namespace {
    constexpr const char *DISTRIBUTED_TABLE_MODE = "distributed_table_mode";
}

SQLiteRelationalStore::~SQLiteRelationalStore()
{
    sqliteStorageEngine_ = nullptr;
}

// Called when a new connection created.
void SQLiteRelationalStore::IncreaseConnectionCounter()
{
    connectionCount_.fetch_add(1, std::memory_order_seq_cst);
    if (connectionCount_.load() > 0) {
        sqliteStorageEngine_->SetConnectionFlag(true);
    }
}

RelationalStoreConnection *SQLiteRelationalStore::GetDBConnection(int &errCode)
{
    std::lock_guard<std::mutex> lock(connectMutex_);
    RelationalStoreConnection* connection = new (std::nothrow) SQLiteRelationalStoreConnection(this);
    if (connection == nullptr) {
        errCode = -E_OUT_OF_MEMORY;
        return nullptr;
    }
    IncObjRef(this);
    IncreaseConnectionCounter();
    return connection;
}

static void InitDataBaseOption(const RelationalDBProperties &properties, OpenDbProperties &option)
{
    option.uri = properties.GetStringProp(DBProperties::DATA_DIR, "");
    option.createIfNecessary = properties.GetBoolProp(DBProperties::CREATE_IF_NECESSARY, false);
    if (properties.IsEncrypted()) {
        option.cipherType = properties.GetCipherType();
        option.passwd = properties.GetPasswd();
        option.iterTimes = properties.GetIterTimes();
    }
}

int SQLiteRelationalStore::InitStorageEngine(const RelationalDBProperties &properties)
{
    OpenDbProperties option;
    InitDataBaseOption(properties, option);
    std::string identifier = properties.GetStringProp(DBProperties::IDENTIFIER_DATA, "");

    StorageEngineAttr poolSize = {1, 1, 0, 16}; // at most 1 write 16 read.
    int errCode = sqliteStorageEngine_->InitSQLiteStorageEngine(poolSize, option, identifier);
    if (errCode != E_OK) {
        LOGE("Init the sqlite storage engine failed:%d", errCode);
    }
    return errCode;
}

void SQLiteRelationalStore::ReleaseResources()
{
    if (sqliteStorageEngine_ != nullptr) {
        sqliteStorageEngine_->ClearEnginePasswd();
        sqliteStorageEngine_ = nullptr;
    }
    if (cloudSyncer_ != nullptr) {
        cloudSyncer_->Close();
        RefObject::KillAndDecObjRef(cloudSyncer_);
        cloudSyncer_ = nullptr;
    }
    RefObject::DecObjRef(storageEngine_);
}

int SQLiteRelationalStore::CheckDBMode()
{
    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->CheckDBModeForRelational();
    if (errCode != E_OK) {
        LOGE("check relational DB mode failed. %d", errCode);
    }

    ReleaseHandle(handle);
    return errCode;
}

int SQLiteRelationalStore::GetSchemaFromMeta(RelationalSchemaObject &schema)
{
    Key schemaKey;
    DBCommon::StringToVector(DBConstant::RELATIONAL_SCHEMA_KEY, schemaKey);
    Value schemaVal;
    int errCode = storageEngine_->GetMetaData(schemaKey, schemaVal);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("Get relational schema from meta table failed. %d", errCode);
        return errCode;
    } else if (errCode == -E_NOT_FOUND || schemaVal.empty()) {
        LOGW("No relational schema info was found.");
        return -E_NOT_FOUND;
    }

    std::string schemaStr;
    DBCommon::VectorToString(schemaVal, schemaStr);
    errCode = schema.ParseFromSchemaString(schemaStr);
    if (errCode != E_OK) {
        LOGE("Parse schema string from meta table failed.");
        return errCode;
    }

    sqliteStorageEngine_->SetSchema(schema);
    return E_OK;
}

int SQLiteRelationalStore::CheckTableModeFromMeta(DistributedTableMode mode, bool isUnSet)
{
    const Key modeKey(DISTRIBUTED_TABLE_MODE, DISTRIBUTED_TABLE_MODE + strlen(DISTRIBUTED_TABLE_MODE));
    Value modeVal;
    int errCode = storageEngine_->GetMetaData(modeKey, modeVal);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("Get distributed table mode from meta table failed. errCode=%d", errCode);
        return errCode;
    }

    DistributedTableMode orgMode = DistributedTableMode::SPLIT_BY_DEVICE;
    if (!modeVal.empty()) {
        std::string value(modeVal.begin(), modeVal.end());
        orgMode = static_cast<DistributedTableMode>(strtoll(value.c_str(), nullptr, 10)); // 10: decimal
    } else if (isUnSet) {
        return E_OK; // First set table mode.
    }

    if (orgMode != mode) {
        LOGE("Check distributed table mode mismatch, orgMode=%d, openMode=%d", orgMode, mode);
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int SQLiteRelationalStore::CheckProperties(RelationalDBProperties properties)
{
    RelationalSchemaObject schema;
    int errCode = GetSchemaFromMeta(schema);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("Get relational schema from meta failed. errcode=%d", errCode);
        return errCode;
    }
    int ret = InitTrackerSchemaFromMeta();
    if (ret != E_OK) {
        LOGE("Init tracker schema from meta failed. errcode=%d", ret);
        return ret;
    }
    properties.SetSchema(schema);

    // Empty schema means no distributed table has been used, we may set DB to any table mode
    // If there is a schema but no table mode, it is the 'SPLIT_BY_DEVICE' mode of old version
    bool isSchemaEmpty = (errCode == -E_NOT_FOUND);
    auto mode = static_cast<DistributedTableMode>(properties.GetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE,
        DistributedTableMode::SPLIT_BY_DEVICE));
    errCode = CheckTableModeFromMeta(mode, isSchemaEmpty);
    if (errCode != E_OK) {
        LOGE("Get distributed table mode from meta failed. errcode=%d", errCode);
        return errCode;
    }

    errCode = SaveTableModeToMeta(mode);
    if (errCode != E_OK) {
        LOGE("Save table mode to meta failed. errCode=%d", errCode);
        return errCode;
    }

    return E_OK;
}

int SQLiteRelationalStore::SaveSchemaToMeta()
{
    Key schemaKey;
    DBCommon::StringToVector(DBConstant::RELATIONAL_SCHEMA_KEY, schemaKey);
    Value schemaVal;
    DBCommon::StringToVector(sqliteStorageEngine_->GetSchema().ToSchemaString(), schemaVal);
    int errCode = storageEngine_->PutMetaData(schemaKey, schemaVal);
    if (errCode != E_OK) {
        LOGE("Save relational schema to meta table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStore::SaveTableModeToMeta(DistributedTableMode mode)
{
    const Key modeKey(DISTRIBUTED_TABLE_MODE, DISTRIBUTED_TABLE_MODE + strlen(DISTRIBUTED_TABLE_MODE));
    Value modeVal;
    DBCommon::StringToVector(std::to_string(mode), modeVal);
    int errCode = storageEngine_->PutMetaData(modeKey, modeVal);
    if (errCode != E_OK) {
        LOGE("Save relational schema to meta table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStore::SaveLogTableVersionToMeta()
{
    LOGD("save log table version to meta table, version: %s", DBConstant::LOG_TABLE_VERSION_CURRENT.c_str());
    const Key logVersionKey(DBConstant::LOG_TABLE_VERSION_KEY.begin(), DBConstant::LOG_TABLE_VERSION_KEY.end());
    Value logVersionVal(DBConstant::LOG_TABLE_VERSION_CURRENT.begin(), DBConstant::LOG_TABLE_VERSION_CURRENT.end());
    int errCode = storageEngine_->PutMetaData(logVersionKey, logVersionVal);
    if (errCode != E_OK) {
        LOGE("save log table version to meta table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStore::CleanDistributedDeviceTable()
{
    std::vector<std::string> missingTables;
    int errCode = sqliteStorageEngine_->CleanDistributedDeviceTable(missingTables);
    if (errCode != E_OK) {
        LOGE("Clean distributed device table failed. %d", errCode);
    }
    for (const auto &deviceTableName : missingTables) {
        std::string deviceHash;
        std::string tableName;
        DBCommon::GetDeviceFromName(deviceTableName, deviceHash, tableName);
        syncAbleEngine_->EraseDeviceWaterMark(deviceHash, false, tableName);
        if (errCode != E_OK) {
            LOGE("Erase water mark failed:%d", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SQLiteRelationalStore::Open(const RelationalDBProperties &properties)
{
    std::lock_guard<std::mutex> lock(initalMutex_);
    if (isInitialized_) {
        LOGD("[RelationalStore][Open] relational db was already initialized.");
        return E_OK;
    }
    int errCode = InitSQLiteStorageEngine(properties);
    if (errCode != E_OK) {
        return errCode;
    }

    do {
        errCode = InitStorageEngine(properties);
        if (errCode != E_OK) {
            LOGE("[RelationalStore][Open] Init database context fail! errCode = [%d]", errCode);
            break;
        }

        storageEngine_ = new (std::nothrow) RelationalSyncAbleStorage(sqliteStorageEngine_);
        if (storageEngine_ == nullptr) {
            LOGE("[RelationalStore][Open] Create syncable storage failed");
            errCode = -E_OUT_OF_MEMORY;
            break;
        }

        syncAbleEngine_ = std::make_shared<SyncAbleEngine>(storageEngine_);
        // to guarantee the life cycle of sync module and syncAbleEngine_ are the same, then the sync module will not
        // be destructed when close store
        storageEngine_->SetSyncAbleEngine(syncAbleEngine_);
        cloudSyncer_ = new(std::nothrow) CloudSyncer(StorageProxy::GetCloudDb(storageEngine_));

        errCode = CheckDBMode();
        if (errCode != E_OK) {
            break;
        }

        errCode = CheckProperties(properties);
        if (errCode != E_OK) {
            break;
        }

        errCode = SaveLogTableVersionToMeta();
        if (errCode != E_OK) {
            break;
        }

        errCode = CleanDistributedDeviceTable();
        if (errCode != E_OK) {
            break;
        }

        isInitialized_ = true;
        return E_OK;
    } while (false);

    ReleaseResources();
    return errCode;
}

void SQLiteRelationalStore::OnClose(const std::function<void(void)> &notifier)
{
    AutoLock lockGuard(this);
    if (notifier) {
        closeNotifiers_.push_back(notifier);
    } else {
        LOGW("Register 'Close()' notifier failed, notifier is null.");
    }
}

SQLiteSingleVerRelationalStorageExecutor *SQLiteRelationalStore::GetHandle(bool isWrite, int &errCode) const
{
    if (sqliteStorageEngine_ == nullptr) {
        errCode = -E_INVALID_DB;
        return nullptr;
    }

    return static_cast<SQLiteSingleVerRelationalStorageExecutor *>(sqliteStorageEngine_->FindExecutor(isWrite,
        OperatePerm::NORMAL_PERM, errCode));
}
void SQLiteRelationalStore::ReleaseHandle(SQLiteSingleVerRelationalStorageExecutor *&handle) const
{
    if (handle == nullptr) {
        return;
    }

    if (sqliteStorageEngine_ != nullptr) {
        StorageExecutor *databaseHandle = handle;
        sqliteStorageEngine_->Recycle(databaseHandle);
        handle = nullptr;
    }
}

int SQLiteRelationalStore::Sync(const ISyncer::SyncParma &syncParam, uint64_t connectionId)
{
    return syncAbleEngine_->Sync(syncParam, connectionId);
}

// Called when a connection released.
void SQLiteRelationalStore::DecreaseConnectionCounter(uint64_t connectionId)
{
    int count = connectionCount_.fetch_sub(1, std::memory_order_seq_cst);
    if (count <= 0) {
        LOGF("Decrease db connection counter failed, count <= 0.");
        return;
    }
    if (storageEngine_ != nullptr) {
        storageEngine_->EraseDataChangeCallback(connectionId);
    }
    if (count != 1) {
        return;
    }

    LockObj();
    auto notifiers = std::move(closeNotifiers_);
    UnlockObj();
    for (const auto &notifier : notifiers) {
        if (notifier) {
            notifier();
        }
    }

    // Sync Close
    syncAbleEngine_->Close();

    if (cloudSyncer_ != nullptr) {
        cloudSyncer_->Close();
        RefObject::KillAndDecObjRef(cloudSyncer_);
        cloudSyncer_ = nullptr;
    }

    if (sqliteStorageEngine_ != nullptr) {
        sqliteStorageEngine_ = nullptr;
    }
    {
        if (storageEngine_ != nullptr) {
            storageEngine_->RegisterHeartBeatListener(nullptr);
        }
        std::lock_guard<std::mutex> lock(lifeCycleMutex_);
        StopLifeCycleTimer();
        lifeCycleNotifier_ = nullptr;
    }
    // close will dec sync ref of storageEngine_
    DecObjRef(storageEngine_);
}

void SQLiteRelationalStore::ReleaseDBConnection(uint64_t connectionId, RelationalStoreConnection *connection)
{
    if (connectionCount_.load() == 1) {
        sqliteStorageEngine_->SetConnectionFlag(false);
    }

    connectMutex_.lock();
    if (connection != nullptr) {
        KillAndDecObjRef(connection);
        DecreaseConnectionCounter(connectionId);
        connectMutex_.unlock();
        KillAndDecObjRef(this);
    } else {
        connectMutex_.unlock();
    }
}

void SQLiteRelationalStore::WakeUpSyncer()
{
    syncAbleEngine_->WakeUpSyncer();
}

int SQLiteRelationalStore::CreateDistributedTable(const std::string &tableName, TableSyncType syncType,
    bool trackerSchemaChanged)
{
    RelationalSchemaObject localSchema = sqliteStorageEngine_->GetSchema();
    TableInfo tableInfo = localSchema.GetTable(tableName);
    if (!tableInfo.Empty()) {
        bool isSharedTable = tableInfo.GetSharedTableMark();
        if (isSharedTable) {
            return E_OK; // shared table will create distributed table when use SetCloudDbSchema
        }
    }

    auto mode = static_cast<DistributedTableMode>(sqliteStorageEngine_->GetProperties().GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));

    std::string localIdentity; // collaboration mode need local identify
    if (mode == DistributedTableMode::COLLABORATION) {
        int errCode = syncAbleEngine_->GetLocalIdentity(localIdentity);
        if (errCode != E_OK || localIdentity.empty()) {
            LOGD("Get local identity failed, can not create.");
            return -E_NOT_SUPPORT;
        }
    }

    bool schemaChanged = false;
    int errCode = sqliteStorageEngine_->CreateDistributedTable(tableName, DBCommon::TransferStringToHex(localIdentity),
        schemaChanged, syncType, trackerSchemaChanged);
    if (errCode != E_OK) {
        LOGE("Create distributed table failed. %d", errCode);
    }
    if (schemaChanged) {
        LOGD("Notify schema changed.");
        storageEngine_->NotifySchemaChanged();
    }
    return errCode;
}

int32_t SQLiteRelationalStore::GetCloudSyncTaskCount()
{
    if (cloudSyncer_ == nullptr) {
        LOGE("[RelationalStore] cloudSyncer was not initialized when get cloud sync task count.");
        return -1;
    }
    return cloudSyncer_->GetCloudSyncTaskCount();
}

int SQLiteRelationalStore::CleanCloudData(ClearMode mode)
{
    auto tableMode = static_cast<DistributedTableMode>(sqliteStorageEngine_->GetProperties().GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));
    if (tableMode == DistributedTableMode::COLLABORATION) {
        LOGE("Not support remove device data in collaboration mode.");
        return -E_NOT_SUPPORT;
    }
    RelationalSchemaObject localSchema = sqliteStorageEngine_->GetSchema();
    TableInfoMap tables = localSchema.GetTables();
    std::vector<std::string> cloudTableNameList;
    for (const auto &tableInfo : tables) {
        bool isSharedTable = tableInfo.second.GetSharedTableMark();
        if ((mode == CLEAR_SHARED_TABLE && !isSharedTable) || (mode != CLEAR_SHARED_TABLE && isSharedTable)) {
            continue;
        }
        if (tableInfo.second.GetTableSyncType() == CLOUD_COOPERATION) {
            cloudTableNameList.push_back(tableInfo.first);
        }
    }
    if (cloudTableNameList.empty()) {
        LOGI("[RelationalStore] device doesn't has cloud table, clean cloud data finished.");
        return E_OK;
    }
    if (cloudSyncer_ == nullptr) {
        LOGE("[RelationalStore] cloudSyncer was not initialized when clean cloud data");
        return -E_INVALID_DB;
    }
    int errCode = cloudSyncer_->CleanCloudData(mode, cloudTableNameList, localSchema);
    if (errCode != E_OK) {
        LOGE("[RelationalStore] failed to clean cloud data, %d.", errCode);
    }

    return errCode;
}

int SQLiteRelationalStore::RemoveDeviceData()
{
    auto mode = static_cast<DistributedTableMode>(sqliteStorageEngine_->GetProperties().GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));
    if (mode == DistributedTableMode::COLLABORATION) {
        LOGE("Not support remove device data in collaboration mode.");
        return -E_NOT_SUPPORT;
    }

    std::vector<std::string> tableNameList = GetAllDistributedTableName();
    if (tableNameList.empty()) {
        return E_OK;
    }
    // erase watermark first
    int errCode = EraseAllDeviceWatermark(tableNameList);
    if (errCode != E_OK) {
        LOGE("remove watermark failed %d", errCode);
        return errCode;
    }
    SQLiteSingleVerRelationalStorageExecutor *handle = nullptr;
    errCode = GetHandleAndStartTransaction(handle);
    if (handle == nullptr) {
        return errCode;
    }

    for (const auto &table: tableNameList) {
        errCode = handle->DeleteDistributedDeviceTable("", table);
        if (errCode != E_OK) {
            LOGE("delete device data failed. %d", errCode);
            break;
        }

        errCode = handle->DeleteDistributedAllDeviceTableLog(table);
        if (errCode != E_OK) {
            LOGE("delete device data failed. %d", errCode);
            break;
        }
    }

    if (errCode != E_OK) {
        (void)handle->Rollback();
        ReleaseHandle(handle);
        return errCode;
    }

    errCode = handle->Commit();
    ReleaseHandle(handle);
    storageEngine_->NotifySchemaChanged();
    return errCode;
}

int SQLiteRelationalStore::RemoveDeviceData(const std::string &device, const std::string &tableName)
{
    auto mode = static_cast<DistributedTableMode>(sqliteStorageEngine_->GetProperties().GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));
    if (mode == DistributedTableMode::COLLABORATION) {
        LOGE("Not support remove device data in collaboration mode.");
        return -E_NOT_SUPPORT;
    }

    TableInfoMap tables = sqliteStorageEngine_->GetSchema().GetTables(); // TableInfoMap
    auto iter = tables.find(tableName);
    if (tables.empty() || (!tableName.empty() && iter == tables.end())) {
        LOGE("Remove device data with table name which is not a distributed table or no distributed table found.");
        return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
    }
    // cloud mode is not permit
    if (iter != tables.end() && iter->second.GetTableSyncType() == CLOUD_COOPERATION) {
        LOGE("Remove device data with cloud sync table name.");
        return -E_NOT_SUPPORT;
    }
    bool isNeedHash = false;
    std::string hashDeviceId;
    int errCode = syncAbleEngine_->GetHashDeviceId(device, hashDeviceId);
    if (errCode == -E_NOT_SUPPORT) {
        isNeedHash = true;
        hashDeviceId = device;
        errCode = E_OK;
    }
    if (errCode != E_OK) {
        return errCode;
    }
    if (isNeedHash) {
        // check device is uuid in meta
        std::set<std::string> hashDevices;
        errCode = GetExistDevices(hashDevices);
        if (errCode != E_OK) {
            return errCode;
        }
        if (hashDevices.find(DBCommon::TransferHashString(device)) == hashDevices.end()) {
            LOGD("[SQLiteRelationalStore] not match device, just return");
            return E_OK;
        }
    }
    return RemoveDeviceDataInner(hashDeviceId, device, tableName, isNeedHash);
}

int SQLiteRelationalStore::RegisterObserverAction(uint64_t connectionId, const StoreObserver *observer,
    const RelationalObserverAction &action)
{
    return storageEngine_->RegisterObserverAction(connectionId, observer, action);
}

int SQLiteRelationalStore::UnRegisterObserverAction(uint64_t connectionId, const StoreObserver *observer)
{
    return storageEngine_->UnRegisterObserverAction(connectionId, observer);
}

int SQLiteRelationalStore::StopLifeCycleTimer()
{
    auto runtimeCxt = RuntimeContext::GetInstance();
    if (runtimeCxt == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (lifeTimerId_ != 0) {
        TimerId timerId = lifeTimerId_;
        lifeTimerId_ = 0;
        runtimeCxt->RemoveTimer(timerId, false);
    }
    return E_OK;
}

int SQLiteRelationalStore::StartLifeCycleTimer(const DatabaseLifeCycleNotifier &notifier)
{
    auto runtimeCxt = RuntimeContext::GetInstance();
    if (runtimeCxt == nullptr) {
        return -E_INVALID_ARGS;
    }
    RefObject::IncObjRef(this);
    TimerId timerId = 0;
    int errCode = runtimeCxt->SetTimer(DBConstant::DEF_LIFE_CYCLE_TIME,
        [this](TimerId id) -> int {
            std::lock_guard<std::mutex> lock(lifeCycleMutex_);
            if (lifeCycleNotifier_) {
                // normal identifier mode
                std::string identifier;
                if (sqliteStorageEngine_->GetProperties().GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false)) {
                    identifier = sqliteStorageEngine_->GetProperties().GetStringProp(
                        DBProperties::DUAL_TUPLE_IDENTIFIER_DATA, "");
                } else {
                    identifier = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::IDENTIFIER_DATA, "");
                }
                auto userId = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::USER_ID, "");
                lifeCycleNotifier_(identifier, userId);
            }
            return 0;
        },
        [this]() {
            int ret = RuntimeContext::GetInstance()->ScheduleTask([this]() {
                RefObject::DecObjRef(this);
            });
            if (ret != E_OK) {
                LOGE("SQLiteSingleVerNaturalStore timer finalizer ScheduleTask, errCode %d", ret);
            }
        },
        timerId);
    if (errCode != E_OK) {
        lifeTimerId_ = 0;
        LOGE("SetTimer failed:%d", errCode);
        RefObject::DecObjRef(this);
        return errCode;
    }

    lifeCycleNotifier_ = notifier;
    lifeTimerId_ = timerId;
    return E_OK;
}

int SQLiteRelationalStore::RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier)
{
    int errCode;
    {
        std::lock_guard<std::mutex> lock(lifeCycleMutex_);
        if (lifeTimerId_ != 0) {
            errCode = StopLifeCycleTimer();
            if (errCode != E_OK) {
                LOGE("Stop the life cycle timer failed:%d", errCode);
                return errCode;
            }
        }

        if (!notifier) {
            return E_OK;
        }
        errCode = StartLifeCycleTimer(notifier);
        if (errCode != E_OK) {
            LOGE("Register life cycle timer failed:%d", errCode);
            return errCode;
        }
    }
    auto listener = std::bind(&SQLiteRelationalStore::HeartBeat, this);
    storageEngine_->RegisterHeartBeatListener(listener);
    return errCode;
}

void SQLiteRelationalStore::HeartBeat()
{
    std::lock_guard<std::mutex> lock(lifeCycleMutex_);
    int errCode = ResetLifeCycleTimer();
    if (errCode != E_OK) {
        LOGE("Heart beat for life cycle failed:%d", errCode);
    }
}

int SQLiteRelationalStore::ResetLifeCycleTimer()
{
    if (lifeTimerId_ == 0) {
        return E_OK;
    }
    auto lifeNotifier = lifeCycleNotifier_;
    lifeCycleNotifier_ = nullptr;
    int errCode = StopLifeCycleTimer();
    if (errCode != E_OK) {
        LOGE("[Reset timer]Stop the life cycle timer failed:%d", errCode);
    }
    return StartLifeCycleTimer(lifeNotifier);
}

std::string SQLiteRelationalStore::GetStorePath() const
{
    return sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::DATA_DIR, "");
}

RelationalDBProperties SQLiteRelationalStore::GetProperties() const
{
    return sqliteStorageEngine_->GetProperties();
}

void SQLiteRelationalStore::StopSync(uint64_t connectionId)
{
    return syncAbleEngine_->StopSync(connectionId);
}

void SQLiteRelationalStore::Dump(int fd)
{
    std::string userId = "";
    std::string appId = "";
    std::string storeId = "";
    std::string label = "";
    if (sqliteStorageEngine_ != nullptr) {
        userId = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::USER_ID, "");
        appId = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::APP_ID, "");
        storeId = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::STORE_ID, "");
        label = sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::IDENTIFIER_DATA, "");
    }
    label = DBCommon::TransferStringToHex(label);
    DBDumpHelper::Dump(fd, "\tdb userId = %s, appId = %s, storeId = %s, label = %s\n",
        userId.c_str(), appId.c_str(), storeId.c_str(), label.c_str());
    if (syncAbleEngine_ != nullptr) {
        syncAbleEngine_->Dump(fd);
    }
}

int SQLiteRelationalStore::RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
    uint64_t connectionId, std::shared_ptr<ResultSet> &result)
{
    if (sqliteStorageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (condition.sql.size() > DBConstant::REMOTE_QUERY_MAX_SQL_LEN) {
        LOGE("remote query sql len is larger than %" PRIu32, DBConstant::REMOTE_QUERY_MAX_SQL_LEN);
        return -E_MAX_LIMITS;
    }

    if (!sqliteStorageEngine_->GetSchema().IsSchemaValid()) {
        LOGW("not a distributed relational store.");
        return -E_NOT_SUPPORT;
    }
    const auto &properties = sqliteStorageEngine_->GetProperties();
    int tableMode = properties.GetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE,
        DistributedTableMode::SPLIT_BY_DEVICE);
    if (tableMode != DistributedTableMode::SPLIT_BY_DEVICE) {
        LOGW("only support split mode.");
        return -E_NOT_SUPPORT;
    }

    // Check whether to be able to operate the db.
    int errCode = E_OK;
    auto *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->CheckEncryptedOrCorrupted();
    ReleaseHandle(handle);
    if (errCode != E_OK) {
        return errCode;
    }

    return syncAbleEngine_->RemoteQuery(device, condition, timeout, connectionId, result);
}

int SQLiteRelationalStore::EraseAllDeviceWatermark(const std::vector<std::string> &tableNameList)
{
    std::set<std::string> devices;
    int errCode = GetExistDevices(devices);
    if (errCode != E_OK) {
        return errCode;
    }
    for (const auto &tableName: tableNameList) {
        for (const auto &device: devices) {
            errCode = syncAbleEngine_->EraseDeviceWaterMark(device, false, tableName);
            if (errCode != E_OK) {
                return errCode;
            }
        }
    }
    return E_OK;
}

std::string SQLiteRelationalStore::GetDevTableName(const std::string &device, const std::string &hashDev) const
{
    std::string devTableName;
    StoreInfo info = {
        sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::USER_ID, ""),
        sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::APP_ID, ""),
        sqliteStorageEngine_->GetProperties().GetStringProp(DBProperties::STORE_ID, "")
    };
    if (RuntimeContext::GetInstance()->TranslateDeviceId(device, info, devTableName) != E_OK) {
        devTableName = hashDev;
    }
    return devTableName;
}

int SQLiteRelationalStore::GetHandleAndStartTransaction(SQLiteSingleVerRelationalStorageExecutor *&handle) const
{
    int errCode = E_OK;
    handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("get handle failed %d", errCode);
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        LOGE("start transaction failed %d", errCode);
        ReleaseHandle(handle);
    }
    return errCode;
}

int SQLiteRelationalStore::RemoveDeviceDataInner(const std::string &mappingDev, const std::string &device,
    const std::string &tableName, bool isNeedHash)
{
    std::string hashHexDev;
    std::string hashDev;
    std::string devTableName;
    if (!isNeedHash) {
        // if is not need hash mappingDev mean hash(uuid) device is param device
        hashHexDev = DBCommon::TransferStringToHex(mappingDev);
        hashDev = mappingDev;
        devTableName = device;
    } else {
        // if is need hash mappingDev mean uuid
        hashDev = DBCommon::TransferHashString(mappingDev);
        hashHexDev = DBCommon::TransferStringToHex(hashDev);
        devTableName = GetDevTableName(mappingDev, hashHexDev);
    }
    // erase watermark first
    int errCode = syncAbleEngine_->EraseDeviceWaterMark(hashDev, false, tableName);
    if (errCode != E_OK) {
        LOGE("erase watermark failed %d", errCode);
        return errCode;
    }
    SQLiteSingleVerRelationalStorageExecutor *handle = nullptr;
    errCode = GetHandleAndStartTransaction(handle);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->DeleteDistributedDeviceTable(devTableName, tableName);
    TableInfoMap tables = sqliteStorageEngine_->GetSchema().GetTables(); // TableInfoMap
    if (errCode != E_OK) {
        LOGE("delete device data failed. %d", errCode);
        tables.clear();
    }

    for (const auto &it : tables) {
        if (tableName.empty() || it.second.GetTableName() == tableName) {
            errCode = handle->DeleteDistributedDeviceTableLog(hashHexDev, it.second.GetTableName());
            if (errCode != E_OK) {
                LOGE("delete device data failed. %d", errCode);
                break;
            }
        }
    }

    if (errCode != E_OK) {
        (void)handle->Rollback();
        ReleaseHandle(handle);
        return errCode;
    }
    errCode = handle->Commit();
    ReleaseHandle(handle);
    storageEngine_->NotifySchemaChanged();
    return errCode;
}

int SQLiteRelationalStore::GetExistDevices(std::set<std::string> &hashDevices) const
{
    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("[SingleVerRDBStore] GetExistsDeviceList get handle failed:%d", errCode);
        return errCode;
    }
    errCode = handle->GetExistsDeviceList(hashDevices);
    if (errCode != E_OK) {
        LOGE("[SingleVerRDBStore] Get remove device list from meta failed. err=%d", errCode);
    }
    ReleaseHandle(handle);
    return errCode;
}

std::vector<std::string> SQLiteRelationalStore::GetAllDistributedTableName()
{
    TableInfoMap tables = sqliteStorageEngine_->GetSchema().GetTables(); // TableInfoMap
    std::vector<std::string> tableNames;
    for (const auto &table: tables) {
        if (table.second.GetTableSyncType() == TableSyncType::CLOUD_COOPERATION) {
            continue;
        }
        tableNames.push_back(table.second.GetTableName());
    }
    return tableNames;
}

int SQLiteRelationalStore::SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb)
{
    if (cloudSyncer_ == nullptr) {
        LOGE("[RelationalStore][SetCloudDB] cloudSyncer was not initialized");
        return -E_INVALID_DB;
    }
    cloudSyncer_->SetCloudDB(cloudDb);
    return E_OK;
}

void SQLiteRelationalStore::AddFields(const std::vector<Field> &newFields, const std::set<std::string> &equalFields,
    std::vector<Field> &addFields)
{
    for (const auto &newField : newFields) {
        if (equalFields.find(newField.colName) == equalFields.end()) {
            addFields.push_back(newField);
        }
    }
}

bool SQLiteRelationalStore::CheckFields(const std::vector<Field> &newFields, const TableInfo &tableInfo,
    std::vector<Field> &addFields)
{
    std::vector<FieldInfo> oldFields = tableInfo.GetFieldInfos();
    if (newFields.size() < oldFields.size()) {
        return false;
    }
    std::set<std::string> equalFields;
    for (const auto &oldField : oldFields) {
        bool isFieldExist = false;
        for (const auto &newField : newFields) {
            if (newField.colName != oldField.GetFieldName()) {
                continue;
            }
            isFieldExist = true;
            int32_t type = newField.type;
            // Field type need to match storage type
            // Field type : Nil, int64_t, double, std::string, bool, Bytes, Asset, Assets
            // Storage type : NONE, NULL, INTEGER, REAL, TEXT, BLOB
            if (type >= TYPE_INDEX<Nil> && type <= TYPE_INDEX<std::string>) {
                type++; // storage type - field type = 1
            } else if (type == TYPE_INDEX<bool>) {
                type = static_cast<int32_t>(StorageType::STORAGE_TYPE_NULL);
            } else if (type >= TYPE_INDEX<Asset> && type <= TYPE_INDEX<Assets>) {
                type = static_cast<int32_t>(StorageType::STORAGE_TYPE_BLOB);
            }
            auto primaryKeyMap = tableInfo.GetPrimaryKey();
            auto it = std::find_if(primaryKeyMap.begin(), primaryKeyMap.end(),
                [&newField](const std::map<int, std::string>::value_type &pair) {
                    return pair.second == newField.colName;
                });
            if (type != static_cast<int32_t>(oldField.GetStorageType()) ||
                newField.primary != (it != primaryKeyMap.end()) || newField.nullable == oldField.IsNotNull()) {
                return false;
            }
            equalFields.insert(newField.colName);
        }
        if (!isFieldExist) {
            return false;
        }
    }
    AddFields(newFields, equalFields, addFields);
    return true;
}

bool SQLiteRelationalStore::PrepareSharedTable(const DataBaseSchema &schema, std::vector<std::string> &deleteTableNames,
    std::map<std::string, std::vector<Field>> &updateTableNames, std::map<std::string, std::string> &alterTableNames)
{
    std::set<std::string> tableNames;
    std::map<std::string, std::string> sharedTableNamesMap;
    std::map<std::string, std::vector<Field>> fieldsMap;
    for (const auto &table : schema.tables) {
        tableNames.insert(table.name);
        sharedTableNamesMap[table.name] = table.sharedTableName;
        std::vector<Field> fields = table.fields;
        bool hasPrimaryKey = DBCommon::HasPrimaryKey(fields);
        Field ownerField = {CloudDbConstant::CLOUD_OWNER, TYPE_INDEX<std::string>, hasPrimaryKey};
        Field privilegeField = {CloudDbConstant::CLOUD_PRIVILEGE, TYPE_INDEX<std::string>};
        fields.insert(fields.begin(), privilegeField);
        fields.insert(fields.begin(), ownerField);
        fieldsMap[table.name] = fields;
    }

    RelationalSchemaObject localSchema = sqliteStorageEngine_->GetSchema();
    TableInfoMap tableList = localSchema.GetTables();
    for (const auto &tableInfo : tableList) {
        if (!tableInfo.second.GetSharedTableMark()) {
            continue;
        }
        std::string oldSharedTableName = tableInfo.second.GetTableName();
        std::string oldOriginTableName = tableInfo.second.GetOriginTableName();
        std::vector<Field> addFields;
        if (tableNames.find(oldOriginTableName) == tableNames.end()) {
            deleteTableNames.push_back(oldSharedTableName);
        } else if (sharedTableNamesMap[oldOriginTableName].empty()) {
            deleteTableNames.push_back(oldSharedTableName);
        } else if (CheckFields(fieldsMap[oldOriginTableName], tableInfo.second, addFields)) {
            if (!addFields.empty()) {
                updateTableNames[oldSharedTableName] = addFields;
            }
            if (oldSharedTableName != sharedTableNamesMap[oldOriginTableName]) {
                alterTableNames[oldSharedTableName] = sharedTableNamesMap[oldOriginTableName];
            }
        } else {
            return false;
        }
    }
    return true;
}

int SQLiteRelationalStore::PrepareAndSetCloudDbSchema(const DataBaseSchema &schema)
{
    if (storageEngine_ == nullptr) {
        LOGE("[RelationalStore][PrepareAndSetCloudDbSchema] storageEngine was not initialized");
        return -E_INVALID_DB;
    }
    int errCode = CheckCloudSchema(schema);
    if (errCode != E_OK) {
        return errCode;
    }
    // delete, update and create shared table and its distributed table
    errCode = ExecuteCreateSharedTable(schema);
    if (errCode != E_OK) {
        LOGE("[RelationalStore] prepare shared table failed:%d", errCode);
        return errCode;
    }
    return storageEngine_->SetCloudDbSchema(schema);
}

int SQLiteRelationalStore::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    if (cloudSyncer_ == nullptr) {
        LOGE("[RelationalStore][SetIAssetLoader] cloudSyncer was not initialized");
        return -E_INVALID_DB;
    }
    cloudSyncer_->SetIAssetLoader(loader);
    return E_OK;
}

int SQLiteRelationalStore::ChkSchema(const TableName &tableName)
{
    if (storageEngine_ == nullptr) {
        LOGE("[RelationalStore][ChkSchema] storageEngine was not initialized");
        return -E_INVALID_DB;
    }
    return storageEngine_->ChkSchema(tableName);
}

int SQLiteRelationalStore::Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess)
{
    if (storageEngine_ == nullptr) {
        LOGE("[RelationalStore][Sync] storageEngine was not initialized");
        return -E_INVALID_DB;
    }
    int errCode = CheckBeforeSync(option);
    if (errCode != E_OK) {
        return errCode;
    }
    LOGI("sync mode:%d, pri:%d, comp:%d", option.mode, option.priorityTask, option.compensatedSyncOnly);
    if (option.compensatedSyncOnly) {
        CloudSyncer::CloudTaskInfo info = CloudSyncUtils::InitCompensatedSyncTaskInfo();
        info.callback = onProcess;
        cloudSyncer_->GenerateCompensatedSync(info);
        return E_OK;
    }
    CloudSyncer::CloudTaskInfo info;
    FillSyncInfo(option, onProcess, info);
    auto [table, ret] = sqliteStorageEngine_->CalTableRef(info.table, storageEngine_->GetSharedTableOriginNames());
    if (ret != E_OK) {
        return ret;
    }
    ret = ReFillSyncInfoTable(table, info);
    if (ret != E_OK) {
        return ret;
    }
    return cloudSyncer_->Sync(info);
}

int SQLiteRelationalStore::CheckBeforeSync(const CloudSyncOption &option)
{
    if (cloudSyncer_ == nullptr) {
        LOGE("[RelationalStore] cloudSyncer was not initialized when sync");
        return -E_INVALID_DB;
    }
    if (option.waitTime > DBConstant::MAX_SYNC_TIMEOUT || option.waitTime < DBConstant::INFINITE_WAIT) {
        return -E_INVALID_ARGS;
    }
    int errCode = CheckQueryValid(option);
    if (errCode != E_OK) {
        return errCode;
    }
    SecurityOption securityOption;
    errCode = storageEngine_->GetSecurityOption(securityOption);
    if (errCode != E_OK && errCode != -E_NOT_SUPPORT) {
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    if (errCode == E_OK && securityOption.securityLabel == S4) {
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    return E_OK;
}

int SQLiteRelationalStore::CheckQueryValid(const CloudSyncOption &option)
{
    if (option.compensatedSyncOnly) {
        return E_OK;
    }
    QuerySyncObject syncObject(option.query);
    int errCode = syncObject.GetValidStatus();
    if (errCode != E_OK) {
        LOGE("[RelationalStore] query is invalid or not support %d", errCode);
        return errCode;
    }
    std::vector<QuerySyncObject> object = QuerySyncObject::GetQuerySyncObject(option.query);
    bool isFromTable = object.empty();
    if (!option.priorityTask && !isFromTable) {
        LOGE("[RelationalStore] not support normal sync with query");
        return -E_NOT_SUPPORT;
    }
    const auto tableNames = syncObject.GetRelationTableNames();
    for (const auto &tableName : tableNames) {
        QuerySyncObject querySyncObject;
        querySyncObject.SetTableName(tableName);
        object.push_back(querySyncObject);
    }
    std::vector<std::string> syncTableNames;
    for (const auto &item : object) {
        std::string tableName = item.GetRelationTableName();
        syncTableNames.emplace_back(tableName);
    }
    errCode = CheckTableName(syncTableNames);
    if (errCode != E_OK) {
        return errCode;
    }
    return CheckObjectValid(option.priorityTask, object, isFromTable);
}

int SQLiteRelationalStore::CheckObjectValid(bool priorityTask, const std::vector<QuerySyncObject> &object,
    bool isFromTable)
{
    RelationalSchemaObject localSchema = sqliteStorageEngine_->GetSchema();
    for (const auto &item : object) {
        if (priorityTask && !item.IsContainQueryNodes() && !isFromTable) {
            LOGE("[RelationalStore] not support priority sync with full table");
            return -E_INVALID_ARGS;
        }
        int errCode = storageEngine_->CheckQueryValid(item);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!priorityTask || isFromTable) {
            continue;
        }
        if (!item.IsInValueOutOfLimit()) {
            LOGE("[RelationalStore] not support priority sync in count out of limit");
            return -E_MAX_LIMITS;
        }
        std::string tableName = item.GetRelationTableName();
        TableInfo tableInfo = localSchema.GetTable(tableName);
        if (!tableInfo.Empty()) {
            const std::map<int, FieldName>& primaryKeyMap = tableInfo.GetPrimaryKey();
            errCode = item.CheckPrimaryKey(primaryKeyMap);
            if (errCode != E_OK) {
                return errCode;
            }
        }
    }
    return E_OK;
}

int SQLiteRelationalStore::CheckTableName(const std::vector<std::string> &tableNames)
{
    if (tableNames.empty()) {
        LOGE("[RelationalStore] sync with empty table");
        return -E_INVALID_ARGS;
    }
    for (const auto &table: tableNames) {
        int errCode = ChkSchema(table);
        if (errCode != E_OK) {
            LOGE("[RelationalStore] schema check failed when sync");
            return errCode;
        }
    }
    return E_OK;
}

void SQLiteRelationalStore::FillSyncInfo(const CloudSyncOption &option, const SyncProcessCallback &onProcess,
    CloudSyncer::CloudTaskInfo &info)
{
    auto syncObject = QuerySyncObject::GetQuerySyncObject(option.query);
    if (syncObject.empty()) {
        QuerySyncObject querySyncObject(option.query);
        info.table = querySyncObject.GetRelationTableNames();
        for (const auto &item: info.table) {
            QuerySyncObject object(Query::Select());
            object.SetTableName(item);
            info.queryList.push_back(object);
        }
    } else {
        for (auto &item: syncObject) {
            info.table.push_back(item.GetRelationTableName());
            info.queryList.push_back(std::move(item));
        }
    }
    info.devices = option.devices;
    info.mode = option.mode;
    info.callback = onProcess;
    info.timeout = option.waitTime;
    info.priorityTask = option.priorityTask;
    info.compensatedTask = option.compensatedSyncOnly;
    info.users.push_back("");
    info.lockAction = option.lockAction;
    info.merge = option.merge;
}

int SQLiteRelationalStore::SetTrackerTable(const TrackerSchema &trackerSchema)
{
    RelationalSchemaObject localSchema = sqliteStorageEngine_->GetSchema();
    TableInfo tableInfo = localSchema.GetTable(trackerSchema.tableName);
    if (tableInfo.Empty()) {
        return sqliteStorageEngine_->SetTrackerTable(trackerSchema);
    }
    bool isFirstCreate = false;
    int errCode = sqliteStorageEngine_->CheckAndCacheTrackerSchema(trackerSchema, tableInfo, isFirstCreate);
    if (errCode != E_OK) {
        return errCode == -E_IGNORE_DATA ? E_OK : errCode;
    }
    errCode = CreateDistributedTable(trackerSchema.tableName, tableInfo.GetTableSyncType(), true);
    if (errCode != E_OK) {
        return errCode;
    }
    return sqliteStorageEngine_->SaveTrackerSchema(trackerSchema.tableName, isFirstCreate);
}

int SQLiteRelationalStore::ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records)
{
    if (condition.sql.empty()) {
        LOGE("[RelationalStore] execute sql is empty.");
        return -E_INVALID_ARGS;
    }
    return sqliteStorageEngine_->ExecuteSql(condition, records);
}

int SQLiteRelationalStore::CleanWaterMark(std::set<std::string> &clearWaterMarkTable)
{
    int errCode = E_OK;
    for (const auto &tableName : clearWaterMarkTable) {
        std::string cloudWaterMark;
        Value blobMetaVal;
        errCode = DBCommon::SerializeWaterMark(0, cloudWaterMark, blobMetaVal);
        if (errCode != E_OK) {
            LOGE("[SQLiteRelationalStore] SerializeWaterMark failed, errCode = %d", errCode);
            return errCode;
        }
        errCode = storageEngine_->PutMetaData(DBCommon::GetPrefixTableName(tableName), blobMetaVal, true);
        if (errCode != E_OK) {
            LOGE("[SQLiteRelationalStore] put meta data failed, errCode = %d", errCode);
            return errCode;
        }
    }
    errCode = cloudSyncer_->CleanWaterMarkInMemory(clearWaterMarkTable);
    if (errCode != E_OK) {
        LOGE("[SQLiteRelationalStore] CleanWaterMarkInMemory failed, errCode = %d", errCode);
    }
    return errCode;
}

int SQLiteRelationalStore::SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty)
{
    SQLiteSingleVerRelationalStorageExecutor *handle = nullptr;
    int errCode = GetHandleAndStartTransaction(handle);
    if (errCode != E_OK) {
        LOGE("[SQLiteRelationalStore] SetReference start transaction failed, errCode = %d", errCode);
        return errCode;
    }
    std::set<std::string> clearWaterMarkTables;
    RelationalSchemaObject schema;
    errCode = sqliteStorageEngine_->SetReference(tableReferenceProperty, handle, clearWaterMarkTables, schema);
    if (errCode != E_OK && errCode != -E_TABLE_REFERENCE_CHANGED) {
        LOGE("[SQLiteRelationalStore] SetReference failed, errCode = %d", errCode);
        (void)handle->Rollback();
        ReleaseHandle(handle);
        return errCode;
    }

    if (!clearWaterMarkTables.empty()) {
        storageEngine_->SetReusedHandle(handle);
        int ret = CleanWaterMark(clearWaterMarkTables);
        if (ret != E_OK) {
            LOGE("[SQLiteRelationalStore] SetReference failed, errCode = %d", ret);
            storageEngine_->SetReusedHandle(nullptr);
            (void)handle->Rollback();
            ReleaseHandle(handle);
            return ret;
        }
        storageEngine_->SetReusedHandle(nullptr);
        LOGI("[SQLiteRelationalStore] SetReference clear water mark success");
    }

    int ret = handle->Commit();
    ReleaseHandle(handle);
    if (ret == E_OK) {
        sqliteStorageEngine_->SetSchema(schema);
        return errCode;
    }
    LOGE("[SQLiteRelationalStore] SetReference commit transaction failed, errCode = %d", ret);
    return ret;
}

int SQLiteRelationalStore::InitTrackerSchemaFromMeta()
{
    int errCode = sqliteStorageEngine_->GetOrInitTrackerSchemaFromMeta();
    return errCode == -E_NOT_FOUND ? E_OK : errCode;
}

int SQLiteRelationalStore::CleanTrackerData(const std::string &tableName, int64_t cursor)
{
    if (tableName.empty()) {
        return -E_INVALID_ARGS;
    }
    return sqliteStorageEngine_->CleanTrackerData(tableName, cursor);
}

int SQLiteRelationalStore::ExecuteCreateSharedTable(const DataBaseSchema &schema)
{
    if (sqliteStorageEngine_ == nullptr) {
        LOGE("[RelationalStore][ExecuteCreateSharedTable] sqliteStorageEngine was not initialized");
        return -E_INVALID_DB;
    }
    std::vector<std::string> deleteTableNames;
    std::map<std::string, std::vector<Field>> updateTableNames;
    std::map<std::string, std::string> alterTableNames;
    if (!PrepareSharedTable(schema, deleteTableNames, updateTableNames, alterTableNames)) {
        LOGE("[RelationalStore][ExecuteCreateSharedTable] table fields are invalid.");
        return -E_INVALID_ARGS;
    }
    LOGI("[RelationalStore][ExecuteCreateSharedTable] upgrade shared table start");
    // upgrade contains delete, alter, update and create
    int errCode = sqliteStorageEngine_->UpgradeSharedTable(schema, deleteTableNames, updateTableNames,
        alterTableNames);
    if (errCode != E_OK) {
        LOGE("[RelationalStore][ExecuteCreateSharedTable] upgrade shared table failed. %d", errCode);
    } else {
        LOGI("[RelationalStore][ExecuteCreateSharedTable] upgrade shared table end");
    }
    return errCode;
}

int SQLiteRelationalStore::ReFillSyncInfoTable(const std::vector<std::string> &actualTable,
    CloudSyncer::CloudTaskInfo &info)
{
    if (info.priorityTask && actualTable.size() != info.table.size()) {
        LOGE("[RelationalStore] Not support regenerate table with priority task");
        return -E_NOT_SUPPORT;
    }
    if (actualTable.size() == info.table.size()) {
        return E_OK;
    }
    LOGD("[RelationalStore] Fill tables from %zu to %zu", info.table.size(), actualTable.size());
    info.table = actualTable;
    info.queryList.clear();
    for (const auto &item: info.table) {
        QuerySyncObject object(Query::Select());
        object.SetTableName(item);
        info.queryList.push_back(object);
    }
    return E_OK;
}

int SQLiteRelationalStore::Pragma(PragmaCmd cmd, PragmaData &pragmaData)
{
    if (cmd != LOGIC_DELETE_SYNC_DATA) {
        return -E_NOT_SUPPORT;
    }
    if (pragmaData == nullptr) {
        return -E_INVALID_ARGS;
    }
    auto logicDelete = *(static_cast<bool *>(pragmaData));
    if (storageEngine_ == nullptr) {
        LOGE("[RelationalStore][ChkSchema] storageEngine was not initialized");
        return -E_INVALID_DB;
    }
    storageEngine_->SetLogicDelete(logicDelete);
    return E_OK;
}

int SQLiteRelationalStore::UpsertData(RecordStatus status, const std::string &tableName,
    const std::vector<VBucket> &records)
{
    if (storageEngine_ == nullptr) {
        LOGE("[RelationalStore][UpsertData] sqliteStorageEngine was not initialized");
        return -E_INVALID_DB;
    }
    int errCode = CheckParamForUpsertData(status, tableName, records);
    if (errCode != E_OK) {
        return errCode;
    }
    return storageEngine_->UpsertData(status, tableName, records);
}

int SQLiteRelationalStore::CheckParamForUpsertData(RecordStatus status, const std::string &tableName,
    const std::vector<VBucket> &records)
{
    if (status != RecordStatus::WAIT_COMPENSATED_SYNC) {
        LOGE("[RelationalStore][CheckParamForUpsertData] invalid status %" PRId64, static_cast<int64_t>(status));
        return -E_INVALID_ARGS;
    }
    if (records.empty()) {
        LOGE("[RelationalStore][CheckParamForUpsertData] records is empty");
        return -E_INVALID_ARGS;
    }
    size_t recordSize = records.size();
    if (recordSize > DBConstant::MAX_BATCH_SIZE) {
        LOGE("[RelationalStore][CheckParamForUpsertData] records size over limit, size %zu", recordSize);
        return -E_MAX_LIMITS;
    }
    return CheckSchemaForUpsertData(tableName, records);
}

static int ChkTable(const TableInfo &table)
{
    if (table.IsNoPkTable() || table.GetSharedTableMark()) {
        LOGE("[RelationalStore][ChkTable] not support table without pk or with tablemark");
        return -E_NOT_SUPPORT;
    }
    if (table.GetTableName().empty() || (table.GetTableSyncType() != TableSyncType::CLOUD_COOPERATION)) {
        return -E_NOT_FOUND;
    }
    return E_OK;
}

int SQLiteRelationalStore::CheckSchemaForUpsertData(const std::string &tableName, const std::vector<VBucket> &records)
{
    if (tableName.empty()) {
        return -E_INVALID_ARGS;
    }
    auto schema = storageEngine_->GetSchemaInfo();
    auto table = schema.GetTable(tableName);
    int errCode = ChkTable(table);
    if (errCode != E_OK) {
        return errCode;
    }
    TableSchema cloudTableSchema;
    errCode = storageEngine_->GetCloudTableSchema(tableName, cloudTableSchema);
    if (errCode != E_OK) {
        LOGE("Get cloud schema failed when check upsert data, %d", errCode);
        return errCode;
    }
    errCode = ChkSchema(tableName);
    if (errCode != E_OK) {
        return errCode;
    }
    std::set<std::string> dbPkFields;
    for (auto &field : table.GetIdentifyKey()) {
        dbPkFields.insert(field);
    }
    for (const auto &record : records) {
        std::set<std::string> recordPkFields;
        for (const auto &item : record) {
            if (dbPkFields.find(item.first) == dbPkFields.end()) {
                continue;
            }
            recordPkFields.insert(item.first);
        }
        if (recordPkFields.size() != dbPkFields.size()) {
            LOGE("[RelationalStore][CheckSchemaForUpsertData] pk size not equal param %zu schema %zu",
                recordPkFields.size(), dbPkFields.size());
            return -E_INVALID_ARGS;
        }
    }
    return errCode;
}

int SQLiteRelationalStore::InitSQLiteStorageEngine(const RelationalDBProperties &properties)
{
    auto engine = new(std::nothrow) SQLiteSingleRelationalStorageEngine(properties);
    if (engine == nullptr) {
        LOGE("[RelationalStore][Open] Create storage engine failed");
        return -E_OUT_OF_MEMORY;
    }
    sqliteStorageEngine_ = std::shared_ptr<SQLiteSingleRelationalStorageEngine>(engine,
        [](SQLiteSingleRelationalStorageEngine *releaseEngine) {
        RefObject::KillAndDecObjRef(releaseEngine);
    });
    return E_OK;
}

int SQLiteRelationalStore::CheckCloudSchema(const DataBaseSchema &schema)
{
    if (storageEngine_ == nullptr) {
        LOGE("[RelationalStore][CheckCloudSchema] storageEngine was not initialized");
        return -E_INVALID_DB;
    }
    std::shared_ptr<DataBaseSchema> cloudSchema;
    (void) storageEngine_->GetCloudDbSchema(cloudSchema);
    RelationalSchemaObject localSchema = sqliteStorageEngine_->GetSchema();
    for (const auto &tableSchema : schema.tables) {
        TableInfo tableInfo = localSchema.GetTable(tableSchema.name);
        if (tableInfo.Empty()) {
            continue;
        }
        if (tableInfo.GetSharedTableMark()) {
            LOGE("[RelationalStore][CheckCloudSchema] Table name is existent shared table's name.");
            return -E_INVALID_ARGS;
        }
    }
    for (const auto &tableSchema : schema.tables) {
        if (cloudSchema == nullptr) {
            continue;
        }
        for (const auto &oldSchema : cloudSchema->tables) {
            if (!CloudStorageUtils::CheckCloudSchemaFields(tableSchema, oldSchema)) {
                LOGE("[RelationalStore][CheckCloudSchema] Schema fields are invalid.");
                return -E_INVALID_ARGS;
            }
        }
    }
    return E_OK;
}
} //namespace DistributedDB
#endif
