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
#include "relational_sync_able_storage.h"

#include <utility>

#include "cloud/cloud_db_constant.h"
#include "data_compression.h"
#include "db_common.h"
#include "db_dfx_adapter.h"
#include "generic_single_ver_kv_entry.h"
#include "platform_specific.h"
#include "relational_remote_query_continue_token.h"
#include "relational_sync_data_inserter.h"
#include "res_finalizer.h"
#include "runtime_context.h"

namespace DistributedDB {
namespace {
void TriggerCloseAutoLaunchConn(const RelationalDBProperties &properties)
{
    static constexpr const char *CLOSE_CONN_TASK = "auto launch close relational connection";
    (void)RuntimeContext::GetInstance()->ScheduleQueuedTask(
        std::string(CLOSE_CONN_TASK),
        [properties] { RuntimeContext::GetInstance()->CloseAutoLaunchConnection(DBTypeInner::DB_RELATION, properties); }
    );
}
}

#define CHECK_STORAGE_ENGINE do { \
    if (storageEngine_ == nullptr) { \
        return -E_INVALID_DB; \
    } \
} while (0)

RelationalSyncAbleStorage::RelationalSyncAbleStorage(std::shared_ptr<SQLiteSingleRelationalStorageEngine> engine)
    : storageEngine_(std::move(engine)),
      isCachedOption_(false)
{}

RelationalSyncAbleStorage::~RelationalSyncAbleStorage()
{
    syncAbleEngine_ = nullptr;
}

// Get interface type of this relational db.
int RelationalSyncAbleStorage::GetInterfaceType() const
{
    return SYNC_RELATION;
}

// Get the interface ref-count, in order to access asynchronously.
void RelationalSyncAbleStorage::IncRefCount()
{
    LOGD("RelationalSyncAbleStorage ref +1");
    IncObjRef(this);
}

// Drop the interface ref-count.
void RelationalSyncAbleStorage::DecRefCount()
{
    LOGD("RelationalSyncAbleStorage ref -1");
    DecObjRef(this);
}

// Get the identifier of this rdb.
std::vector<uint8_t> RelationalSyncAbleStorage::GetIdentifier() const
{
    std::string identifier = storageEngine_->GetIdentifier();
    return std::vector<uint8_t>(identifier.begin(), identifier.end());
}

std::vector<uint8_t> RelationalSyncAbleStorage::GetDualTupleIdentifier() const
{
    std::string identifier = storageEngine_->GetProperties().GetStringProp(
        DBProperties::DUAL_TUPLE_IDENTIFIER_DATA, "");
    std::vector<uint8_t> identifierVect(identifier.begin(), identifier.end());
    return identifierVect;
}

// Get the max timestamp of all entries in database.
void RelationalSyncAbleStorage::GetMaxTimestamp(Timestamp &timestamp) const
{
    int errCode = E_OK;
    auto handle = GetHandle(false, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return;
    }
    timestamp = 0;
    errCode = handle->GetMaxTimestamp(storageEngine_->GetSchema().GetTableNames(), timestamp);
    if (errCode != E_OK) {
        LOGE("GetMaxTimestamp failed, errCode:%d", errCode);
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }
    ReleaseHandle(handle);
    return;
}

int RelationalSyncAbleStorage::GetMaxTimestamp(const std::string &tableName, Timestamp &timestamp) const
{
    int errCode = E_OK;
    auto handle = GetHandle(false, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }
    timestamp = 0;
    errCode = handle->GetMaxTimestamp({ tableName }, timestamp);
    if (errCode != E_OK) {
        LOGE("GetMaxTimestamp failed, errCode:%d", errCode);
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }
    ReleaseHandle(handle);
    return errCode;
}

SQLiteSingleVerRelationalStorageExecutor *RelationalSyncAbleStorage::GetHandle(bool isWrite, int &errCode,
    OperatePerm perm) const
{
    if (storageEngine_ == nullptr) {
        errCode = -E_INVALID_DB;
        return nullptr;
    }
    auto handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
        storageEngine_->FindExecutor(isWrite, perm, errCode));
    if (handle == nullptr) {
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }
    return handle;
}

SQLiteSingleVerRelationalStorageExecutor *RelationalSyncAbleStorage::GetHandleExpectTransaction(bool isWrite,
    int &errCode, OperatePerm perm) const
{
    if (storageEngine_ == nullptr) {
        errCode = -E_INVALID_DB;
        return nullptr;
    }
    if (transactionHandle_ != nullptr) {
        return transactionHandle_;
    }
    auto handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
        storageEngine_->FindExecutor(isWrite, perm, errCode));
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        handle = nullptr;
    }
    return handle;
}

void RelationalSyncAbleStorage::ReleaseHandle(SQLiteSingleVerRelationalStorageExecutor *&handle) const
{
    if (storageEngine_ == nullptr) {
        return;
    }
    StorageExecutor *databaseHandle = handle;
    storageEngine_->Recycle(databaseHandle);
    std::function<void()> listener = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(heartBeatMutex_);
        listener = heartBeatListener_;
    }
    if (listener) {
        listener();
    }
}

// Get meta data associated with the given key.
int RelationalSyncAbleStorage::GetMetaData(const Key &key, Value &value) const
{
    CHECK_STORAGE_ENGINE;
    if (key.size() > DBConstant::MAX_KEY_SIZE) {
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->GetKvData(key, value);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }
    ReleaseHandle(handle);
    return errCode;
}

// Put meta data as a key-value entry.
int RelationalSyncAbleStorage::PutMetaData(const Key &key, const Value &value)
{
    CHECK_STORAGE_ENGINE;
    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->PutKvData(key, value); // meta doesn't need time.
    if (errCode != E_OK) {
        LOGE("Put kv data err:%d", errCode);
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }
    ReleaseHandle(handle);
    return errCode;
}

// Delete multiple meta data records in a transaction.
int RelationalSyncAbleStorage::DeleteMetaData(const std::vector<Key> &keys)
{
    CHECK_STORAGE_ENGINE;
    for (const auto &key : keys) {
        if (key.empty() || key.size() > DBConstant::MAX_KEY_SIZE) {
            return -E_INVALID_ARGS;
        }
    }
    int errCode = E_OK;
    auto handle = GetHandle(true, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }

    handle->StartTransaction(TransactType::IMMEDIATE);
    errCode = handle->DeleteMetaData(keys);
    if (errCode != E_OK) {
        handle->Rollback();
        LOGE("[SinStore] DeleteMetaData failed, errCode = %d", errCode);
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    } else {
        handle->Commit();
    }
    ReleaseHandle(handle);
    return errCode;
}

// Delete multiple meta data records with key prefix in a transaction.
int RelationalSyncAbleStorage::DeleteMetaDataByPrefixKey(const Key &keyPrefix) const
{
    CHECK_STORAGE_ENGINE;
    if (keyPrefix.empty() || keyPrefix.size() > DBConstant::MAX_KEY_SIZE) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    auto handle = GetHandle(true, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->DeleteMetaDataByPrefixKey(keyPrefix);
    if (errCode != E_OK) {
        LOGE("[SinStore] DeleteMetaData by prefix key failed, errCode = %d", errCode);
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }
    ReleaseHandle(handle);
    return errCode;
}

// Get all meta data keys.
int RelationalSyncAbleStorage::GetAllMetaKeys(std::vector<Key> &keys) const
{
    CHECK_STORAGE_ENGINE;
    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->GetAllMetaKeys(keys);
    if (errCode != E_OK) {
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }
    ReleaseHandle(handle);
    return errCode;
}

const RelationalDBProperties &RelationalSyncAbleStorage::GetDbProperties() const
{
    return storageEngine_->GetProperties();
}

static int GetKvEntriesByDataItems(std::vector<SingleVerKvEntry *> &entries, std::vector<DataItem> &dataItems)
{
    int errCode = E_OK;
    for (auto &item : dataItems) {
        auto entry = new (std::nothrow) GenericSingleVerKvEntry();
        if (entry == nullptr) {
            errCode = -E_OUT_OF_MEMORY;
            LOGE("GetKvEntries failed, errCode:%d", errCode);
            SingleVerKvEntry::Release(entries);
            break;
        }
        entry->SetEntryData(std::move(item));
        entries.push_back(entry);
    }
    return errCode;
}

static size_t GetDataItemSerialSize(const DataItem &item, size_t appendLen)
{
    // timestamp and local flag: 3 * uint64_t, version(uint32_t), key, value, origin dev and the padding size.
    // the size would not be very large.
    static const size_t maxOrigDevLength = 40;
    size_t devLength = std::max(maxOrigDevLength, item.origDev.size());
    size_t dataSize = (Parcel::GetUInt64Len() * 3 + Parcel::GetUInt32Len() + Parcel::GetVectorCharLen(item.key) +
                       Parcel::GetVectorCharLen(item.value) + devLength + appendLen);
    return dataSize;
}

static bool CanHoldDeletedData(const std::vector<DataItem> &dataItems, const DataSizeSpecInfo &dataSizeInfo,
    size_t appendLen)
{
    bool reachThreshold = (dataItems.size() >= dataSizeInfo.packetSize);
    for (size_t i = 0, blockSize = 0; !reachThreshold && i < dataItems.size(); i++) {
        blockSize += GetDataItemSerialSize(dataItems[i], appendLen);
        reachThreshold = (blockSize >= dataSizeInfo.blockSize * DBConstant::QUERY_SYNC_THRESHOLD);
    }
    return !reachThreshold;
}

static void ProcessContinueTokenForQuerySync(const std::vector<DataItem> &dataItems, int &errCode,
    SQLiteSingleVerRelationalContinueToken *&token)
{
    if (errCode != -E_UNFINISHED) { // Error happened or get data finished. Token should be cleared.
        delete token;
        token = nullptr;
        return;
    }

    if (dataItems.empty()) {
        errCode = -E_INTERNAL_ERROR;
        LOGE("Get data unfinished but data items is empty.");
        delete token;
        token = nullptr;
        return;
    }
    token->SetNextBeginTime(dataItems.back());
    token->UpdateNextSyncOffset(dataItems.size());
}

/**
 * Caller must ensure that parameter token is valid.
 * If error happened, token will be deleted here.
 */
int RelationalSyncAbleStorage::GetSyncDataForQuerySync(std::vector<DataItem> &dataItems,
    SQLiteSingleVerRelationalContinueToken *&token, const DataSizeSpecInfo &dataSizeInfo) const
{
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = E_OK;
    auto handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(storageEngine_->FindExecutor(false,
        OperatePerm::NORMAL_PERM, errCode));
    if (handle == nullptr) {
        goto ERROR;
    }

    do {
        errCode = handle->GetSyncDataByQuery(dataItems,
            Parcel::GetAppendedLen(),
            dataSizeInfo,
            std::bind(&SQLiteSingleVerRelationalContinueToken::GetStatement, *token,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
            storageEngine_->GetSchema().GetTable(token->GetQuery().GetTableName()));
        if (errCode == -E_FINISHED) {
            token->FinishGetData();
            errCode = token->IsGetAllDataFinished() ? E_OK : -E_UNFINISHED;
        }
    } while (errCode == -E_UNFINISHED && CanHoldDeletedData(dataItems, dataSizeInfo, Parcel::GetAppendedLen()));

ERROR:
    if (errCode != -E_UNFINISHED && errCode != E_OK) { // Error happened.
        dataItems.clear();
    }
    ProcessContinueTokenForQuerySync(dataItems, errCode, token);
    ReleaseHandle(handle);
    return errCode;
}

// use kv struct data to sync
// Get the data which would be synced with query condition
int RelationalSyncAbleStorage::GetSyncData(QueryObject &query, const SyncTimeRange &timeRange,
    const DataSizeSpecInfo &dataSizeInfo, ContinueToken &continueStmtToken,
    std::vector<SingleVerKvEntry *> &entries) const
{
    if (!timeRange.IsValid()) {
        return -E_INVALID_ARGS;
    }
    query.SetSchema(storageEngine_->GetSchema());
    auto token = new (std::nothrow) SQLiteSingleVerRelationalContinueToken(timeRange, query);
    if (token == nullptr) {
        LOGE("[SingleVerNStore] Allocate continue token failed.");
        return -E_OUT_OF_MEMORY;
    }

    continueStmtToken = static_cast<ContinueToken>(token);
    return GetSyncDataNext(entries, continueStmtToken, dataSizeInfo);
}

int RelationalSyncAbleStorage::GetSyncDataNext(std::vector<SingleVerKvEntry *> &entries,
    ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
{
    auto token = static_cast<SQLiteSingleVerRelationalContinueToken *>(continueStmtToken);
    if (!token->CheckValid()) {
        return -E_INVALID_ARGS;
    }
    RelationalSchemaObject schema = storageEngine_->GetSchema();
    const auto fieldInfos = schema.GetTable(token->GetQuery().GetTableName()).GetFieldInfos();
    std::vector<std::string> fieldNames;
    fieldNames.reserve(fieldInfos.size());
    for (const auto &fieldInfo : fieldInfos) { // order by cid
        fieldNames.push_back(fieldInfo.GetFieldName());
    }
    token->SetFieldNames(fieldNames);

    std::vector<DataItem> dataItems;
    int errCode = GetSyncDataForQuerySync(dataItems, token, dataSizeInfo);
    if (errCode != E_OK && errCode != -E_UNFINISHED) { // The code need be sent to outside except new error happened.
        continueStmtToken = static_cast<ContinueToken>(token);
        return errCode;
    }

    int innerCode = GetKvEntriesByDataItems(entries, dataItems);
    if (innerCode != E_OK) {
        errCode = innerCode;
        delete token;
        token = nullptr;
    }
    continueStmtToken = static_cast<ContinueToken>(token);
    return errCode;
}

namespace {
std::vector<DataItem> ConvertEntries(std::vector<SingleVerKvEntry *> entries)
{
    std::vector<DataItem> dataItems;
    for (const auto &itemEntry : entries) {
        GenericSingleVerKvEntry *entry = static_cast<GenericSingleVerKvEntry *>(itemEntry);
        if (entry != nullptr) {
            DataItem item;
            item.origDev = entry->GetOrigDevice();
            item.flag = entry->GetFlag();
            item.timestamp = entry->GetTimestamp();
            item.writeTimestamp = entry->GetWriteTimestamp();
            entry->GetKey(item.key);
            entry->GetValue(item.value);
            entry->GetHashKey(item.hashKey);
            dataItems.push_back(item);
        }
    }
    return dataItems;
}
}

int RelationalSyncAbleStorage::PutSyncDataWithQuery(const QueryObject &object,
    const std::vector<SingleVerKvEntry *> &entries, const DeviceID &deviceName)
{
    std::vector<DataItem> dataItems = ConvertEntries(entries);
    return PutSyncData(object, dataItems, deviceName);
}

namespace {
inline DistributedTableMode GetCollaborationMode(const std::shared_ptr<SQLiteSingleRelationalStorageEngine> &engine)
{
    return static_cast<DistributedTableMode>(engine->GetProperties().GetIntProp(
        RelationalDBProperties::DISTRIBUTED_TABLE_MODE, DistributedTableMode::SPLIT_BY_DEVICE));
}

inline bool IsCollaborationMode(const std::shared_ptr<SQLiteSingleRelationalStorageEngine> &engine)
{
    return GetCollaborationMode(engine) == DistributedTableMode::COLLABORATION;
}
}

int RelationalSyncAbleStorage::SaveSyncDataItems(const QueryObject &object, std::vector<DataItem> &dataItems,
    const std::string &deviceName)
{
    int errCode = E_OK;
    LOGD("[RelationalSyncAbleStorage::SaveSyncDataItems] Get write handle.");
    QueryObject query = object;
    query.SetSchema(storageEngine_->GetSchema());

    RelationalSchemaObject remoteSchema;
    errCode = GetRemoteDeviceSchema(deviceName, remoteSchema);
    if (errCode != E_OK) {
        LOGE("Find remote schema failed. err=%d", errCode);
        return errCode;
    }

    StoreInfo info = {
        storageEngine_->GetProperties().GetStringProp(DBProperties::USER_ID, ""),
        storageEngine_->GetProperties().GetStringProp(DBProperties::APP_ID, ""),
        storageEngine_->GetProperties().GetStringProp(DBProperties::STORE_ID, "")
    };
    auto inserter = RelationalSyncDataInserter::CreateInserter(deviceName, query, storageEngine_->GetSchema(),
        remoteSchema.GetTable(query.GetTableName()).GetFieldInfos(), info);
    inserter.SetEntries(dataItems);

    auto *handle = GetHandle(true, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }

    DBDfxAdapter::StartTraceSQL();

    errCode = handle->SaveSyncItems(inserter);

    DBDfxAdapter::FinishTraceSQL();
    if (errCode == E_OK) {
        // dataItems size > 0 now because already check before
        // all dataItems will write into db now, so need to observer notify here
        // if some dataItems will not write into db in the future, observer notify here need change
        ChangedData data;
        TriggerObserverAction(deviceName, std::move(data), false);
    }

    ReleaseHandle(handle);
    return errCode;
}

int RelationalSyncAbleStorage::PutSyncData(const QueryObject &query, std::vector<DataItem> &dataItems,
    const std::string &deviceName)
{
    if (deviceName.length() > DBConstant::MAX_DEV_LENGTH) {
        LOGW("Device length is invalid for sync put");
        return -E_INVALID_ARGS;
    }

    int errCode = SaveSyncDataItems(query, dataItems, deviceName); // Currently true to check value content
    if (errCode != E_OK) {
        LOGE("[Relational] PutSyncData errCode:%d", errCode);
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }
    return errCode;
}

int RelationalSyncAbleStorage::RemoveDeviceData(const std::string &deviceName, bool isNeedNotify)
{
    (void) deviceName;
    (void) isNeedNotify;
    return -E_NOT_SUPPORT;
}

RelationalSchemaObject RelationalSyncAbleStorage::GetSchemaInfo() const
{
    return storageEngine_->GetSchema();
}

int RelationalSyncAbleStorage::GetSecurityOption(SecurityOption &option) const
{
    std::lock_guard<std::mutex> autoLock(securityOptionMutex_);
    if (isCachedOption_) {
        option = securityOption_;
        return E_OK;
    }
    std::string dbPath = storageEngine_->GetProperties().GetStringProp(DBProperties::DATA_DIR, "");
    int errCode = RuntimeContext::GetInstance()->GetSecurityOption(dbPath, securityOption_);
    if (errCode == E_OK) {
        option = securityOption_;
        isCachedOption_ = true;
    }
    return errCode;
}

void RelationalSyncAbleStorage::NotifyRemotePushFinished(const std::string &deviceId) const
{
    return;
}

// Get the timestamp when database created or imported
int RelationalSyncAbleStorage::GetDatabaseCreateTimestamp(Timestamp &outTime) const
{
    return OS::GetCurrentSysTimeInMicrosecond(outTime);
}

std::vector<QuerySyncObject> RelationalSyncAbleStorage::GetTablesQuery()
{
    auto tableNames = storageEngine_->GetSchema().GetTableNames();
    std::vector<QuerySyncObject> queries;
    queries.reserve(tableNames.size());
    for (const auto &it : tableNames) {
        queries.emplace_back(Query::Select(it));
    }
    return queries;
}

int RelationalSyncAbleStorage::LocalDataChanged(int notifyEvent, std::vector<QuerySyncObject> &queryObj)
{
    (void) queryObj;
    return -E_NOT_SUPPORT;
}

int RelationalSyncAbleStorage::InterceptData(std::vector<SingleVerKvEntry *> &entries, const std::string &sourceID,
    const std::string &targetID) const
{
    return E_OK;
}

int RelationalSyncAbleStorage::CreateDistributedDeviceTable(const std::string &device,
    const RelationalSyncStrategy &syncStrategy)
{
    auto mode = storageEngine_->GetProperties().GetIntProp(RelationalDBProperties::DISTRIBUTED_TABLE_MODE,
        DistributedTableMode::SPLIT_BY_DEVICE);
    if (mode != DistributedTableMode::SPLIT_BY_DEVICE) {
        LOGD("No need create device table in COLLABORATION mode.");
        return E_OK;
    }

    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        LOGE("Start transaction failed:%d", errCode);
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
        ReleaseHandle(handle);
        return errCode;
    }

    StoreInfo info = {
        storageEngine_->GetProperties().GetStringProp(DBProperties::USER_ID, ""),
        storageEngine_->GetProperties().GetStringProp(DBProperties::APP_ID, ""),
        storageEngine_->GetProperties().GetStringProp(DBProperties::STORE_ID, "")
    };
    for (const auto &[table, strategy] : syncStrategy) {
        if (!strategy.permitSync) {
            continue;
        }

        errCode = handle->CreateDistributedDeviceTable(device, storageEngine_->GetSchema().GetTable(table), info);
        if (errCode != E_OK) {
            LOGE("Create distributed device table failed. %d", errCode);
            break;
        }
    }

    if (errCode == E_OK) {
        errCode = handle->Commit();
    } else {
        (void)handle->Rollback();
    }

    ReleaseHandle(handle);
    return errCode;
}

int RelationalSyncAbleStorage::RegisterSchemaChangedCallback(const std::function<void()> &callback)
{
    std::lock_guard lock(onSchemaChangedMutex_);
    onSchemaChanged_ = callback;
    return E_OK;
}

void RelationalSyncAbleStorage::NotifySchemaChanged()
{
    std::lock_guard lock(onSchemaChangedMutex_);
    if (onSchemaChanged_) {
        LOGD("Notify relational schema was changed");
        onSchemaChanged_();
    }
}
int RelationalSyncAbleStorage::GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const
{
    algorithmSet.clear();
    DataCompression::GetCompressionAlgo(algorithmSet);
    return E_OK;
}

void RelationalSyncAbleStorage::RegisterObserverAction(uint64_t connectionId, const RelationalObserverAction &action)
{
    std::lock_guard<std::mutex> lock(dataChangeDeviceMutex_);
    dataChangeCallbackMap_[connectionId] = action;
}

void RelationalSyncAbleStorage::TriggerObserverAction(const std::string &deviceName,
    ChangedData &&changedData, bool isChangedData)
{
    IncObjRef(this);
    int taskErrCode =
        RuntimeContext::GetInstance()->ScheduleTask([this, deviceName, changedData, isChangedData] () mutable {
        std::lock_guard<std::mutex> lock(dataChangeDeviceMutex_);
        if (!dataChangeCallbackMap_.empty()) {
            auto it = dataChangeCallbackMap_.rbegin(); // call the last valid observer
            if (it->second != nullptr) {
                it->second(deviceName, std::move(changedData), isChangedData);
            }
        }
        DecObjRef(this);
    });
    if (taskErrCode != E_OK) {
        LOGE("TriggerObserverAction scheduletask retCode=%d", taskErrCode);
        DecObjRef(this);
    }
}

void RelationalSyncAbleStorage::RegisterHeartBeatListener(const std::function<void()> &listener)
{
    std::lock_guard<std::mutex> autoLock(heartBeatMutex_);
    heartBeatListener_ = listener;
}

int RelationalSyncAbleStorage::CheckAndInitQueryCondition(QueryObject &query) const
{
    RelationalSchemaObject schema = storageEngine_->GetSchema();
    TableInfo table = schema.GetTable(query.GetTableName());
    if (!table.IsValid()) {
        LOGE("Query table is not a distributed table.");
        return -E_DISTRIBUTED_SCHEMA_NOT_FOUND;
    }
    if (table.GetTableSyncType() == CLOUD_COOPERATION) {
        LOGE("cloud table mode is not support");
        return -E_NOT_SUPPORT;
    }
    query.SetSchema(schema);

    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->CheckQueryObjectLegal(table, query, schema.GetSchemaVersion());
    if (errCode != E_OK) {
        LOGE("Check relational query condition failed. %d", errCode);
        TriggerCloseAutoLaunchConn(storageEngine_->GetProperties());
    }

    ReleaseHandle(handle);
    return errCode;
}

bool RelationalSyncAbleStorage::CheckCompatible(const std::string &schema, uint8_t type) const
{
    // return true if is relational schema.
    return !schema.empty() && ReadSchemaType(type) == SchemaType::RELATIVE;
}

int RelationalSyncAbleStorage::GetRemoteQueryData(const PreparedStmt &prepStmt, size_t packetSize,
    std::vector<std::string> &colNames, std::vector<RelationalRowData *> &data) const
{
    if (IsCollaborationMode(storageEngine_) || !storageEngine_->GetSchema().IsSchemaValid()) {
        return -E_NOT_SUPPORT;
    }
    if (prepStmt.GetOpCode() != PreparedStmt::ExecutorOperation::QUERY || !prepStmt.IsValid()) {
        LOGE("[ExecuteQuery] invalid args");
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    auto handle = GetHandle(false, errCode, OperatePerm::NORMAL_PERM);
    if (handle == nullptr) {
        LOGE("[ExecuteQuery] get handle fail:%d", errCode);
        return errCode;
    }
    errCode = handle->ExecuteQueryBySqlStmt(prepStmt.GetSql(), prepStmt.GetBindArgs(), packetSize, colNames, data);
    if (errCode != E_OK) {
        LOGE("[ExecuteQuery] ExecuteQueryBySqlStmt failed:%d", errCode);
    }
    ReleaseHandle(handle);
    return errCode;
}

int RelationalSyncAbleStorage::ExecuteQuery(const PreparedStmt &prepStmt, size_t packetSize,
    RelationalRowDataSet &dataSet, ContinueToken &token) const
{
    dataSet.Clear();
    if (token == nullptr) {
        // start query
        std::vector<std::string> colNames;
        std::vector<RelationalRowData *> data;
        ResFinalizer finalizer([&data] { RelationalRowData::Release(data); });

        int errCode = GetRemoteQueryData(prepStmt, packetSize, colNames, data);
        if (errCode != E_OK) {
            return errCode;
        }

        // create one token
        token = static_cast<ContinueToken>(
            new (std::nothrow) RelationalRemoteQueryContinueToken(std::move(colNames), std::move(data)));
        if (token == nullptr) {
            LOGE("ExecuteQuery OOM");
            return -E_OUT_OF_MEMORY;
        }
    }

    auto remoteToken = static_cast<RelationalRemoteQueryContinueToken *>(token);
    if (!remoteToken->CheckValid()) {
        LOGE("ExecuteQuery invalid token");
        return -E_INVALID_ARGS;
    }

    int errCode = remoteToken->GetData(packetSize, dataSet);
    if (errCode == -E_UNFINISHED) {
        errCode = E_OK;
    } else {
        if (errCode != E_OK) {
            dataSet.Clear();
        }
        delete remoteToken;
        remoteToken = nullptr;
        token = nullptr;
    }
    LOGI("ExecuteQuery finished, errCode:%d, size:%d", errCode, dataSet.GetSize());
    return errCode;
}

int RelationalSyncAbleStorage::SaveRemoteDeviceSchema(const std::string &deviceId, const std::string &remoteSchema,
    uint8_t type)
{
    if (ReadSchemaType(type) != SchemaType::RELATIVE) {
        return -E_INVALID_ARGS;
    }

    RelationalSchemaObject schemaObj;
    int errCode = schemaObj.ParseFromSchemaString(remoteSchema);
    if (errCode != E_OK) {
        LOGE("Parse remote schema failed. err=%d", errCode);
        return errCode;
    }

    std::string keyStr = DBConstant::REMOTE_DEVICE_SCHEMA_KEY_PREFIX + DBCommon::TransferHashString(deviceId);
    Key remoteSchemaKey(keyStr.begin(), keyStr.end());
    Value remoteSchemaBuff(remoteSchema.begin(), remoteSchema.end());
    errCode = PutMetaData(remoteSchemaKey, remoteSchemaBuff);
    if (errCode != E_OK) {
        LOGE("Save remote schema failed. err=%d", errCode);
        return errCode;
    }

    return remoteDeviceSchema_.Put(deviceId, remoteSchema);
}

int RelationalSyncAbleStorage::GetRemoteDeviceSchema(const std::string &deviceId, RelationalSchemaObject &schemaObj)
{
    if (schemaObj.IsSchemaValid()) {
        return -E_INVALID_ARGS;
    }

    std::string remoteSchema;
    int errCode = remoteDeviceSchema_.Get(deviceId, remoteSchema);
    if (errCode == -E_NOT_FOUND) {
        LOGW("Get remote device schema miss cached.");
        std::string keyStr = DBConstant::REMOTE_DEVICE_SCHEMA_KEY_PREFIX + DBCommon::TransferHashString(deviceId);
        Key remoteSchemaKey(keyStr.begin(), keyStr.end());
        Value remoteSchemaBuff;
        errCode = GetMetaData(remoteSchemaKey, remoteSchemaBuff);
        if (errCode != E_OK) {
            LOGE("Get remote device schema from meta failed. err=%d", errCode);
            return errCode;
        }
        remoteSchema = std::string(remoteSchemaBuff.begin(), remoteSchemaBuff.end());
        errCode = remoteDeviceSchema_.Put(deviceId, remoteSchema);
    }

    if (errCode != E_OK) {
        LOGE("Get remote device schema failed. err=%d", errCode);
        return errCode;
    }

    errCode = schemaObj.ParseFromSchemaString(remoteSchema);
    if (errCode != E_OK) {
        LOGE("Parse remote schema failed. err=%d", errCode);
    }
    return errCode;
}

void RelationalSyncAbleStorage::ReleaseRemoteQueryContinueToken(ContinueToken &token) const
{
    auto remoteToken = static_cast<RelationalRemoteQueryContinueToken *>(token);
    delete remoteToken;
    remoteToken = nullptr;
    token = nullptr;
}

int RelationalSyncAbleStorage::StartTransaction(TransactType type)
{
    CHECK_STORAGE_ENGINE;
    std::unique_lock<std::shared_mutex> lock(transactionMutex_);
    if (transactionHandle_ != nullptr) {
        LOGD("Transaction started already.");
        return -E_TRANSACT_STATE;
    }
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
        storageEngine_->FindExecutor(type == TransactType::IMMEDIATE, OperatePerm::NORMAL_PERM, errCode));
    if (handle == nullptr) {
        ReleaseHandle(handle);
        return errCode;
    }
    errCode = handle->StartTransaction(type);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        return errCode;
    }
    transactionHandle_ = handle;
    return errCode;
}

int RelationalSyncAbleStorage::Commit()
{
    std::unique_lock<std::shared_mutex> lock(transactionMutex_);
    if (transactionHandle_ == nullptr) {
        LOGE("relation database is null or the transaction has not been started");
        return -E_INVALID_DB;
    }
    int errCode = transactionHandle_->Commit();
    ReleaseHandle(transactionHandle_);
    transactionHandle_ = nullptr;
    LOGD("connection commit transaction!");
    return errCode;
}

int RelationalSyncAbleStorage::Rollback()
{
    std::unique_lock<std::shared_mutex> lock(transactionMutex_);
    if (transactionHandle_ == nullptr) {
        LOGE("Invalid handle for rollback or the transaction has not been started.");
        return -E_INVALID_DB;
    }

    int errCode = transactionHandle_->Rollback();
    ReleaseHandle(transactionHandle_);
    transactionHandle_ = nullptr;
    LOGI("connection rollback transaction!");
    return errCode;
}

int RelationalSyncAbleStorage::GetUploadCount(const std::string &tableName, const Timestamp &timestamp,
    const bool isCloudForcePush, int64_t &count)
{
    int errCode = E_OK;
    auto *handle = GetHandleExpectTransaction(false, errCode);
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->GetUploadCount(tableName, timestamp, isCloudForcePush, count);
    if (transactionHandle_ == nullptr) {
        ReleaseHandle(handle);
    }
    return errCode;
}

int RelationalSyncAbleStorage::FillCloudGid(const CloudSyncData &data)
{
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = E_OK;
    auto writeHandle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
        storageEngine_->FindExecutor(true, OperatePerm::NORMAL_PERM, errCode));
    if (writeHandle == nullptr) {
        return errCode;
    }
    errCode = writeHandle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(writeHandle);
        return errCode;
    }
    errCode = writeHandle->UpdateCloudLogGid(data);
    if (errCode != E_OK) {
        writeHandle->Rollback();
        ReleaseHandle(writeHandle);
        return errCode;
    }
    errCode = writeHandle->Commit();
    ReleaseHandle(writeHandle);
    return errCode;
}

int RelationalSyncAbleStorage::GetCloudData(const TableSchema &tableSchema, const Timestamp &beginTime,
    ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult)
{
    if (transactionHandle_ == nullptr) {
        LOGE(" the transaction has not been started");
        return -E_INVALID_DB;
    }
    SyncTimeRange syncTimeRange = { .beginTime = beginTime };
    QueryObject queryObject;
    queryObject.SetTableName(tableSchema.name);
    auto token = new (std::nothrow) SQLiteSingleVerRelationalContinueToken(syncTimeRange, queryObject);
    if (token == nullptr) {
        LOGE("[SingleVerNStore] Allocate continue token failed.");
        return -E_OUT_OF_MEMORY;
    }
    token->SetCloudTableSchema(tableSchema);
    continueStmtToken = static_cast<ContinueToken>(token);
    return GetCloudDataNext(continueStmtToken, cloudDataResult);
}

int RelationalSyncAbleStorage::GetCloudDataNext(ContinueToken &continueStmtToken,
    CloudSyncData &cloudDataResult)
{
    if (continueStmtToken == nullptr) {
        return -E_INVALID_ARGS;
    }
    auto token = static_cast<SQLiteSingleVerRelationalContinueToken *>(continueStmtToken);
    if (!token->CheckValid()) {
        return -E_INVALID_ARGS;
    }
    if (transactionHandle_ == nullptr) {
        LOGE("the transaction has not been started, release the token");
        ReleaseCloudDataToken(continueStmtToken);
        return -E_INVALID_DB;
    }
    int errCode = transactionHandle_->GetSyncCloudData(cloudDataResult, CloudDbConstant::MAX_UPLOAD_SIZE, *token);
    if (errCode != -E_UNFINISHED) {
        delete token;
        token = nullptr;
    }
    continueStmtToken = static_cast<ContinueToken>(token);
    return errCode;
}

int RelationalSyncAbleStorage::ReleaseCloudDataToken(ContinueToken &continueStmtToken)
{
    if (continueStmtToken == nullptr) {
        return E_OK;
    }
    auto token = static_cast<SQLiteSingleVerRelationalContinueToken *>(continueStmtToken);
    if (!token->CheckValid()) {
        return E_OK;
    }
    int errCode = token->ReleaseCloudStatement();
    delete token;
    token = nullptr;
    return errCode;
}

int RelationalSyncAbleStorage::ChkSchema(const TableName &tableName)
{
    std::shared_lock<std::shared_mutex> readLock(schemaMgrMutex_);
    RelationalSchemaObject localSchema = GetSchemaInfo();
    return schemaMgr_.ChkSchema(tableName, localSchema);
}

int RelationalSyncAbleStorage::SetCloudDbSchema(const DataBaseSchema &schema)
{
    std::unique_lock<std::shared_mutex> writeLock(schemaMgrMutex_);
    schemaMgr_.SetCloudDbSchema(schema);
    return E_OK;
}

int RelationalSyncAbleStorage::GetCloudDbSchema(DataBaseSchema &cloudSchema)
{
    std::shared_lock<std::shared_mutex> readLock(schemaMgrMutex_);
    cloudSchema = *(schemaMgr_.GetCloudDbSchema());
    return E_OK;
}

int RelationalSyncAbleStorage::GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket,
    DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo)
{
    if (transactionHandle_ == nullptr) {
        LOGE(" the transaction has not been started");
        return -E_INVALID_DB;
    }

    TableSchema tableSchema;
    int errCode = GetCloudTableSchema(tableName, tableSchema);
    if (errCode != E_OK) {
        LOGE("Get cloud schema failed when query log for cloud sync, %d", errCode);
        return errCode;
    }
    return transactionHandle_->GetInfoByPrimaryKeyOrGid(tableSchema, vBucket, dataInfoWithLog, assetInfo);
}

int RelationalSyncAbleStorage::PutCloudSyncData(const std::string &tableName, DownloadData &downloadData)
{
    if (transactionHandle_ == nullptr) {
        LOGE(" the transaction has not been started");
        return -E_INVALID_DB;
    }

    TableSchema tableSchema;
    int errCode = GetCloudTableSchema(tableName, tableSchema);
    if (errCode != E_OK) {
        LOGE("Get cloud schema failed when save cloud data, %d", errCode);
        return errCode;
    }
    return transactionHandle_->PutCloudSyncData(tableName, tableSchema, downloadData);
}

int RelationalSyncAbleStorage::CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
    const RelationalSchemaObject &localSchema, std::vector<Asset> &assets)
{
    if (transactionHandle_ == nullptr) {
        LOGE("the transaction has not been started");
        return -E_INVALID_DB;
    }

    return transactionHandle_->DoCleanInner(mode, tableNameList, localSchema, assets);
}

int RelationalSyncAbleStorage::GetCloudTableSchema(const TableName &tableName, TableSchema &tableSchema)
{
    std::shared_lock<std::shared_mutex> readLock(schemaMgrMutex_);
    return schemaMgr_.GetCloudTableSchema(tableName, tableSchema);
}

int RelationalSyncAbleStorage::FillCloudAssetForDownload(const std::string &tableName, VBucket &asset,
    bool isDownloadSuccess)
{
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    TableSchema tableSchema;
    int errCode = GetCloudTableSchema(tableName, tableSchema);
    if (errCode != E_OK) {
        LOGE("Get cloud schema failed when fill cloud asset, %d", errCode);
        return errCode;
    }
    auto writeHandle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
        storageEngine_->FindExecutor(true, OperatePerm::NORMAL_PERM, errCode));
    if (writeHandle == nullptr) {
        return errCode;
    }
    errCode = writeHandle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(writeHandle);
        return errCode;
    }
    errCode = writeHandle->FillCloudAssetForDownload(tableSchema, asset, isDownloadSuccess);
    if (errCode != E_OK) {
        writeHandle->Rollback();
        ReleaseHandle(writeHandle);
        return errCode;
    }
    errCode = writeHandle->Commit();
    ReleaseHandle(writeHandle);
    return errCode;
}

int RelationalSyncAbleStorage::FillCloudGidAndAsset(const OpType opType, const CloudSyncData &data)
{
    CHECK_STORAGE_ENGINE;
    if (opType == OpType::UPDATE && data.updData.assets.empty()) {
        return E_OK;
    }
    int errCode = E_OK;
    auto writeHandle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
        storageEngine_->FindExecutor(true, OperatePerm::NORMAL_PERM, errCode));
    if (writeHandle == nullptr) {
        return errCode;
    }
    errCode = writeHandle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(writeHandle);
        return errCode;
    }
    if (opType == OpType::INSERT) {
        errCode = writeHandle->UpdateCloudLogGid(data);
        if (errCode != E_OK) {
            LOGE("Failed to fill cloud log gid, %d.", errCode);
            writeHandle->Rollback();
            ReleaseHandle(writeHandle);
            return errCode;
        }
        if (!data.insData.assets.empty()) {
            errCode = writeHandle->FillCloudAssetForUpload(data.tableName, data.insData);
        }
    } else {
        errCode = writeHandle->FillCloudAssetForUpload(data.tableName, data.updData);
    }
    if (errCode != E_OK) {
        LOGE("Failed to fill cloud asset, %d.", errCode);
        writeHandle->Rollback();
        ReleaseHandle(writeHandle);
        return errCode;
    }
    errCode = writeHandle->Commit();
    ReleaseHandle(writeHandle);
    return errCode;
}

void RelationalSyncAbleStorage::SetSyncAbleEngine(std::shared_ptr<SyncAbleEngine> syncAbleEngine)
{
    syncAbleEngine_ = syncAbleEngine;
}

std::string RelationalSyncAbleStorage::GetIdentify() const
{
    if (storageEngine_ == nullptr) {
        LOGW("[RelationalSyncAbleStorage] engine is nullptr return default");
        return "";
    }
    return storageEngine_->GetIdentifier();
}

void RelationalSyncAbleStorage::EraseDataChangeCallback(uint64_t connectionId)
{
    std::lock_guard<std::mutex> lock(dataChangeDeviceMutex_);
    auto it = dataChangeCallbackMap_.find(connectionId);
    if (it != dataChangeCallbackMap_.end()) {
        dataChangeCallbackMap_.erase(it);
    }
}

void RelationalSyncAbleStorage::ReleaseContinueToken(ContinueToken &continueStmtToken) const
{
    auto token = static_cast<SQLiteSingleVerRelationalContinueToken *>(continueStmtToken);
    if (token == nullptr || !(token->CheckValid())) {
        LOGW("[RelationalSyncAbleStorage][ReleaseContinueToken] Input is not a continue token.");
        return;
    }
    delete token;
    continueStmtToken = nullptr;
}
}
#endif