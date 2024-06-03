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
#define LOG_TAG "SingleStoreImpl"
#include "single_store_impl.h"

#include "backup_manager.h"
#include "data_change_notifier.h"
#include "dds_trace.h"
#include "dev_manager.h"
#include "kvdb_service_client.h"
#include "log_print.h"
#include "store_result_set.h"
#include "store_util.h"
#include "task_executor.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedDataDfx;
using namespace std::chrono;
SingleStoreImpl::SingleStoreImpl(
    std::shared_ptr<DBStore> dbStore, const AppId &appId, const Options &options, const Convertor &cvt)
    : convertor_(cvt), dbStore_(std::move(dbStore))
{
    std::string path = options.GetDatabaseDir();
    appId_ = appId.appId;
    storeId_ = dbStore_->GetStoreId();
    autoSync_ = options.autoSync;
    isClientSync_ = options.isClientSync;
    syncObserver_ = std::make_shared<SyncObserver>();
    roleType_ = options.role;
    dataType_ = options.dataType;
    if (options.backup) {
        BackupManager::GetInstance().Prepare(path, storeId_);
    }
}

SingleStoreImpl::~SingleStoreImpl()
{
    if (taskId_ > 0) {
        TaskExecutor::GetInstance().Remove(taskId_);
    }
}

StoreId SingleStoreImpl::GetStoreId() const
{
    return { storeId_ };
}

Status SingleStoreImpl::RetryWithCheckPoint(std::function<DistributedDB::DBStatus()> lambda)
{
    auto dbStatus = lambda();
    if (dbStatus != DistributedDB::LOG_OVER_LIMITS) {
        return StoreUtil::ConvertStatus(dbStatus);
    }
    DistributedDB::PragmaData data;
    dbStore_->Pragma(DistributedDB::EXEC_CHECKPOINT, data);
    dbStatus = lambda();
    return StoreUtil::ConvertStatus(dbStatus);
}

Status SingleStoreImpl::Put(const Key &key, const Value &value)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = convertor_.ToLocalDBKey(key);
    if (dbKey.empty() || value.Size() > MAX_VALUE_LENGTH) {
        ZLOGE("invalid key:%{public}s size:[k:%{public}zu v:%{public}zu]",
            StoreUtil::Anonymous(key.ToString()).c_str(), key.Size(), value.Size());
        return INVALID_ARGUMENT;
    }

    auto status = RetryWithCheckPoint([this, &dbKey, &value]() { return dbStore_->Put(dbKey, value); });
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x key:%{public}s, value size:%{public}zu", status,
            StoreUtil::Anonymous(key.ToString()).c_str(), value.Size());
    }
    DoNotifyChange();
    return status;
}

Status SingleStoreImpl::PutBatch(const std::vector<Entry> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBEntry> dbEntries;
    for (const auto &entry : entries) {
        DBEntry dbEntry;
        dbEntry.key = convertor_.ToLocalDBKey(entry.key);
        if (dbEntry.key.empty() || entry.value.Size() > MAX_VALUE_LENGTH) {
            ZLOGE("invalid key:%{public}s size:[k:%{public}zu v:%{public}zu]",
                StoreUtil::Anonymous(entry.key.ToString()).c_str(), entry.key.Size(), entry.value.Size());
            return INVALID_ARGUMENT;
        }
        dbEntry.value = entry.value;
        dbEntries.push_back(std::move(dbEntry));
    }

    auto status = RetryWithCheckPoint([this, &dbEntries]() { return dbStore_->PutBatch(dbEntries); });
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x entries size:%{public}zu", status, entries.size());
    }
    DoNotifyChange();
    return status;
}

Status SingleStoreImpl::Delete(const Key &key)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = convertor_.ToLocalDBKey(key);
    if (dbKey.empty()) {
        ZLOGE("invalid key:%{public}s size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
        return INVALID_ARGUMENT;
    }

    auto status = RetryWithCheckPoint([this, &dbKey]() { return dbStore_->Delete(dbKey); });
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x key:%{public}s", status, StoreUtil::Anonymous(key.ToString()).c_str());
    }
    DoNotifyChange();
    return status;
}

Status SingleStoreImpl::DeleteBatch(const std::vector<Key> &keys)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBKey> dbKeys;
    for (const auto &key : keys) {
        DBKey dbKey = convertor_.ToLocalDBKey(key);
        if (dbKey.empty()) {
            ZLOGE("invalid key:%{public}s size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
            return INVALID_ARGUMENT;
        }
        dbKeys.push_back(std::move(dbKey));
    }

    auto status = RetryWithCheckPoint([this, &dbKeys]() { return dbStore_->DeleteBatch(dbKeys); });
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x keys size:%{public}zu", status, keys.size());
    }
    DoNotifyChange();
    return status;
}

Status SingleStoreImpl::StartTransaction()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    auto status = RetryWithCheckPoint([this]() { return dbStore_->StartTransaction(); });
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, StoreUtil::Anonymous(storeId_).c_str());
    }
    return status;
}

Status SingleStoreImpl::Commit()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->Commit();
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, dbStore_->GetStoreId().c_str());
    }
    return status;
}

Status SingleStoreImpl::Rollback()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->Rollback();
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, StoreUtil::Anonymous(storeId_).c_str());
    }
    return status;
}

Status SingleStoreImpl::SubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    if (observer == nullptr) {
        ZLOGE("invalid observer is null");
        return INVALID_ARGUMENT;
    }

    uint32_t realType = type;
    std::shared_ptr<ObserverBridge> bridge = PutIn(realType, observer);
    if (bridge == nullptr) {
        return (realType == type) ? OVER_MAX_LIMITS : STORE_ALREADY_SUBSCRIBE;
    }

    Status status = SUCCESS;
    unsigned int mode = DistributedDB::OBSERVER_CHANGES_NATIVE;
    if (isClientSync_) {
        mode |= DistributedDB::OBSERVER_CHANGES_FOREIGN;
    }

    if ((realType & SUBSCRIBE_TYPE_LOCAL) == SUBSCRIBE_TYPE_LOCAL) {
        auto dbStatus = dbStore_->RegisterObserver({}, mode, bridge.get());
        status = StoreUtil::ConvertStatus(dbStatus);
    }

    if ((((realType & SUBSCRIBE_TYPE_REMOTE) == SUBSCRIBE_TYPE_REMOTE) ||
            ((realType & SUBSCRIBE_TYPE_CLOUD) == SUBSCRIBE_TYPE_CLOUD)) &&
        status == SUCCESS) {
        realType &= ~SUBSCRIBE_TYPE_LOCAL;
        status = bridge->RegisterRemoteObserver();
    }

    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x type:%{public}d->%{public}d observer:0x%{public}x", status, type, realType,
            StoreUtil::Anonymous(bridge.get()));
        TakeOut(realType, observer);
    }
    return status;
}

Status SingleStoreImpl::UnSubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    if (observer == nullptr) {
        ZLOGE("invalid observer is null");
        return INVALID_ARGUMENT;
    }

    uint32_t realType = type;
    std::shared_ptr<ObserverBridge> bridge = TakeOut(realType, observer);
    if (bridge == nullptr) {
        return STORE_NOT_SUBSCRIBE;
    }

    Status status = SUCCESS;

    if ((realType & SUBSCRIBE_TYPE_LOCAL) == SUBSCRIBE_TYPE_LOCAL) {
        auto dbStatus = dbStore_->UnRegisterObserver(bridge.get());
        status = StoreUtil::ConvertStatus(dbStatus);
    }

    if (((realType & SUBSCRIBE_TYPE_REMOTE) == SUBSCRIBE_TYPE_REMOTE) && status == SUCCESS) {
        realType &= ~SUBSCRIBE_TYPE_LOCAL;
        status = bridge->UnregisterRemoteObserver();
    }

    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x type:%{public}d->%{public}d observer:0x%{public}x", status, type, realType,
            StoreUtil::Anonymous(bridge.get()));
    }
    return status;
}

Status SingleStoreImpl::Get(const Key &key, Value &value)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = convertor_.ToWholeDBKey(key);
    if (dbKey.empty()) {
        ZLOGE("invalid key:%{public}s size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
        return INVALID_ARGUMENT;
    }

    DBValue dbValue;
    auto dbStatus = dbStore_->Get(dbKey, dbValue);
    value = std::move(dbValue);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x key:%{public}s", status, StoreUtil::Anonymous(key.ToString()).c_str());
    }
    return status;
}

void SingleStoreImpl::Get(const Key &key, const std::string &networkId,
    const std::function<void(Status, Value &&)> &onResult)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (dataType_ != DataType::TYPE_DYNAMICAL) {
        onResult(NOT_SUPPORT, Value());
        return;
    }
    if (networkId == DevManager::GetInstance().GetLocalDevice().networkId || !IsRemoteChanged(networkId)) {
        Value value;
        auto status = Get(key, value);
        onResult(status, std::move(value));
        return;
    }
    uint64_t sequenceId = StoreUtil::GenSequenceId();
    asyncFuncs_.Insert(sequenceId, { .key = key, .toGet = onResult });
    auto result = SyncExt(networkId, sequenceId);
    if (result != SUCCESS) {
        asyncFuncs_.Erase(sequenceId);
        onResult(result, Value());
    }
}

void SingleStoreImpl::GetEntries(const Key &prefix, const std::string &networkId,
    const std::function<void(Status, std::vector<Entry> &&)> &onResult)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (dataType_ != DataType::TYPE_DYNAMICAL) {
        onResult(NOT_SUPPORT, {});
        return;
    }
    if (networkId == DevManager::GetInstance().GetLocalDevice().networkId || !IsRemoteChanged(networkId)) {
        std::vector<Entry> entries;
        auto status = GetEntries(prefix, entries);
        onResult(status, std::move(entries));
        return;
    }
    uint64_t sequenceId = StoreUtil::GenSequenceId();
    asyncFuncs_.Insert(sequenceId, { .key = prefix, .toGetEntries = onResult });
    auto result = SyncExt(networkId, sequenceId);
    if (result != SUCCESS) {
        asyncFuncs_.Erase(sequenceId);
        onResult(result, {});
    }
}

Status SingleStoreImpl::SyncExt(const std::string &networkId, uint64_t sequenceId)
{
    auto clientUuid = DevManager::GetInstance().ToUUID(networkId);
    if (clientUuid.empty()) {
        ZLOGE("clientUuid is empty");
        return INVALID_ARGUMENT;
    }
    KVDBService::SyncInfo syncInfo;
    syncInfo.mode = SyncMode::PULL;
    syncInfo.seqId = sequenceId;
    syncInfo.devices = { networkId };
    auto status = DoSyncExt(syncInfo, shared_from_this());
    if (status != SUCCESS) {
        ZLOGE("sync ext failed, status:%{public}d networkId:%{public}s app:%{public}s store:%{public}s", status,
            StoreUtil::Anonymous(networkId).c_str(), appId_.c_str(), StoreUtil::Anonymous(storeId_).c_str());
        return status;
    }
    return SUCCESS;
}

void SingleStoreImpl::SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId)
{
    AsyncFunc asyncFunc;
    auto exist = asyncFuncs_.ComputeIfPresent(sequenceId, [&asyncFunc](const auto &key, auto &value) -> bool {
        asyncFunc = value;
        return false;
    });
    if (!exist) {
        ZLOGD("not exist, sequenceId:%{public}" PRIu64, sequenceId);
        return ;
    }
    if (asyncFunc.toGet) {
        Value value;
        auto status = Get(asyncFunc.key, value);
        asyncFunc.toGet(status, std::move(value));
        return;
    }
    if (asyncFunc.toGetEntries) {
        std::vector<Entry> entries;
        auto status = GetEntries(asyncFunc.key, entries);
        asyncFunc.toGetEntries(status, std::move(entries));
        return;
    }
    ZLOGW("sync ext completed, but do nothing, key:%{public}s, sequenceId:%{public}" PRIu64,
        StoreUtil::Anonymous(asyncFunc.key.ToString()).c_str(), sequenceId);
}

bool SingleStoreImpl::IsRemoteChanged(const std::string &deviceId)
{
    auto clientUuid = DevManager::GetInstance().ToUUID(deviceId);
    if (clientUuid.empty()) {
        return true;
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return true;
    }
    auto serviceAgent = service->GetServiceAgent({ appId_ });
    if (serviceAgent == nullptr) {
        return true;
    }
    return serviceAgent->IsChanged(clientUuid);
}

void SingleStoreImpl::SyncCompleted(const std::map<std::string, Status> &results)
{}

Status SingleStoreImpl::GetEntries(const Key &prefix, std::vector<Entry> &entries) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    DBKey dbPrefix = convertor_.GetPrefix(prefix);
    if (dbPrefix.empty() && !prefix.Empty()) {
        ZLOGE("invalid prefix:%{public}s size:%{public}zu", StoreUtil::Anonymous(prefix.ToString()).c_str(),
            prefix.Size());
        return INVALID_ARGUMENT;
    }

    DBQuery dbQuery = DBQuery::Select();
    dbQuery.PrefixKey(dbPrefix);
    auto status = GetEntries(dbQuery, entries);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x prefix:%{public}s", status, StoreUtil::Anonymous(prefix.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetEntries(const DataQuery &query, std::vector<Entry> &entries) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    DBQuery dbQuery = convertor_.GetDBQuery(query);
    auto status = GetEntries(dbQuery, entries);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x query:%{public}s", status, StoreUtil::Anonymous(query.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetResultSet(const Key &prefix, std::shared_ptr<ResultSet> &resultSet) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    DBKey dbPrefix = convertor_.GetPrefix(prefix);
    if (dbPrefix.empty() && !prefix.Empty()) {
        ZLOGE("invalid prefix:%{public}s size:%{public}zu", StoreUtil::Anonymous(prefix.ToString()).c_str(),
            prefix.Size());
        return INVALID_ARGUMENT;
    }

    DBQuery dbQuery = DistributedDB::Query::Select();
    dbQuery.PrefixKey(dbPrefix);
    auto status = GetResultSet(dbQuery, resultSet);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x prefix:%{public}s", status, StoreUtil::Anonymous(prefix.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetResultSet(const DataQuery &query, std::shared_ptr<ResultSet> &resultSet) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    DBQuery dbQuery = convertor_.GetDBQuery(query);
    auto status = GetResultSet(dbQuery, resultSet);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x query:%{public}s", status, StoreUtil::Anonymous(query.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::CloseResultSet(std::shared_ptr<ResultSet> &resultSet)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (resultSet == nullptr) {
        ZLOGE("input is nullptr");
        return INVALID_ARGUMENT;
    }

    auto status = resultSet->Close();
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, StoreUtil::Anonymous(storeId_).c_str());
    }
    resultSet = nullptr;
    return status;
}

Status SingleStoreImpl::GetCount(const DataQuery &query, int &result) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DBQuery dbQuery = convertor_.GetDBQuery(query);
    auto dbStatus = dbStore_->GetCount(dbQuery, result);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x query:%{public}s", status, StoreUtil::Anonymous(query.ToString()).c_str());
    }
    return status;
}

Status SingleStoreImpl::GetSecurityLevel(SecurityLevel &secLevel) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DistributedDB::SecurityOption option;
    auto dbStatus = dbStore_->GetSecurityOption(option);
    secLevel = static_cast<SecurityLevel>(StoreUtil::GetSecLevel(option));
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x security:[%{public}d]", status, option.securityLabel);
    }
    return status;
}

Status SingleStoreImpl::RemoveDeviceData(const std::string &device)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    if (device.empty()) {
        auto dbStatus = dbStore_->RemoveDeviceData();
        auto status = StoreUtil::ConvertStatus(dbStatus);
        if (status != SUCCESS) {
            ZLOGE("status:0x%{public}x device:all others", status);
        }
        return status;
    }

    auto dbStatus = dbStore_->RemoveDeviceData(DevManager::GetInstance().ToUUID(device));
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x device:%{public}s", status, StoreUtil::Anonymous(device).c_str());
    }
    return status;
}

Status SingleStoreImpl::Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t delay)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    KVDBService::SyncInfo syncInfo;
    syncInfo.seqId = StoreUtil::GenSequenceId();
    syncInfo.mode = mode;
    syncInfo.delay = delay;
    syncInfo.devices = devices;
    return DoSync(syncInfo, syncObserver_);
}

Status SingleStoreImpl::Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
    std::shared_ptr<SyncCallback> syncCallback, uint32_t delay)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    KVDBService::SyncInfo syncInfo;
    syncInfo.seqId = StoreUtil::GenSequenceId();
    syncInfo.mode = mode;
    syncInfo.devices = devices;
    syncInfo.query = query.ToString();
    syncInfo.delay = delay;
    return DoSync(syncInfo, syncCallback);
}

Status SingleStoreImpl::CloudSync(const AsyncDetail &async)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    auto serviceAgent = service->GetServiceAgent({ appId_ });
    if (serviceAgent == nullptr) {
        ZLOGE("failed! invalid agent app:%{public}s store:%{public}s!", appId_.c_str(),
              StoreUtil::Anonymous(storeId_).c_str());
        return ILLEGAL_STATE;
    }
    KVDBService::SyncInfo syncInfo;
    syncInfo.seqId = StoreUtil::GenSequenceId();
    serviceAgent->AddCloudSyncCallback(syncInfo.seqId, async);
    auto status = service->CloudSync({ appId_ }, { storeId_ }, syncInfo);
    if (status != SUCCESS) {
        ZLOGE("sync failed!: %{public}d", status);
        serviceAgent->DeleteCloudSyncCallback(syncInfo.seqId);
    }
    return status;
}

Status SingleStoreImpl::RegisterSyncCallback(std::shared_ptr<SyncCallback> callback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    if (callback == nullptr) {
        ZLOGW("INVALID_ARGUMENT.");
        return INVALID_ARGUMENT;
    }
    syncObserver_->Add(callback);
    return SUCCESS;
}

Status SingleStoreImpl::UnRegisterSyncCallback()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    syncObserver_->Clean();
    return SUCCESS;
}

Status SingleStoreImpl::SetSyncParam(const KvSyncParam &syncParam)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->SetSyncParam({ appId_ }, { storeId_ }, syncParam);
}

Status SingleStoreImpl::GetSyncParam(KvSyncParam &syncParam)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->GetSyncParam({ appId_ }, { storeId_ }, syncParam);
}

Status SingleStoreImpl::SetCapabilityEnabled(bool enabled) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    if (enabled) {
        return service->EnableCapability({ appId_ }, { storeId_ });
    }
    return service->DisableCapability({ appId_ }, { storeId_ });
}

Status SingleStoreImpl::SetCapabilityRange(
    const std::vector<std::string> &localLabels, const std::vector<std::string> &remoteLabels) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->SetCapability({ appId_ }, { storeId_ }, localLabels, remoteLabels);
}

Status SingleStoreImpl::SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    SyncInfo syncInfo;
    syncInfo.seqId = StoreUtil::GenSequenceId();
    syncInfo.devices = devices;
    syncInfo.query = query.ToString();
    auto serviceAgent = service->GetServiceAgent({ appId_ });
    if (serviceAgent == nullptr) {
        ZLOGE("failed! invalid agent app:%{public}s, store:%{public}s!", appId_.c_str(),
            StoreUtil::Anonymous(storeId_).c_str());
        return ILLEGAL_STATE;
    }

    serviceAgent->AddSyncCallback(syncObserver_, syncInfo.seqId);
    return service->AddSubscribeInfo({ appId_ }, { storeId_ }, syncInfo);
}

Status SingleStoreImpl::UnsubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    SyncInfo syncInfo;
    syncInfo.seqId = StoreUtil::GenSequenceId();
    syncInfo.devices = devices;
    syncInfo.query = query.ToString();
    auto serviceAgent = service->GetServiceAgent({ appId_ });
    if (serviceAgent == nullptr) {
        ZLOGE("failed! invalid agent app:%{public}s, store:%{public}s!", appId_.c_str(),
            StoreUtil::Anonymous(storeId_).c_str());
        return ILLEGAL_STATE;
    }

    serviceAgent->AddSyncCallback(syncObserver_, syncInfo.seqId);
    return service->RmvSubscribeInfo({ appId_ }, { storeId_ }, syncInfo);
}

int32_t SingleStoreImpl::AddRef()
{
    ref_++;
    return ref_;
}

int32_t SingleStoreImpl::Close(bool isForce)
{
    if (isForce) {
        ref_ = 1;
    }
    ref_--;
    if (ref_ != 0) {
        return ref_;
    }

    observers_.Clear();
    asyncFuncs_.Clear();
    syncObserver_->Clean();
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    dbStore_ = nullptr;
    return ref_;
}

Status SingleStoreImpl::Backup(const std::string &file, const std::string &baseDir)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    auto status = BackupManager::GetInstance().Backup(file, baseDir, storeId_, dbStore_);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s backup:%{public}s ", status,
            StoreUtil::Anonymous(storeId_).c_str(), file.c_str());
    }
    return status;
}

Status SingleStoreImpl::Restore(const std::string &file, const std::string &baseDir)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    auto service = KVDBServiceClient::GetInstance();
    if (service != nullptr) {
        service->Close({ appId_ }, { storeId_ });
    }
    auto status = BackupManager::GetInstance().Restore(file, baseDir, appId_, storeId_, dbStore_);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s backup:%{public}s ", status,
            StoreUtil::Anonymous(storeId_).c_str(), file.c_str());
    }
    return status;
}

Status SingleStoreImpl::DeleteBackup(const std::vector<std::string> &files, const std::string &baseDir,
    std::map<std::string, DistributedKv::Status> &results)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    for (auto &file : files) {
        results.emplace(file, DEVICE_NOT_FOUND);
    }
    auto status = BackupManager::GetInstance().DeleteBackup(results, baseDir, storeId_);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, StoreUtil::Anonymous(storeId_).c_str());
    }
    return status;
}

std::function<void(ObserverBridge *)> SingleStoreImpl::BridgeReleaser()
{
    return [this](ObserverBridge *obj) {
        if (obj == nullptr) {
            return;
        }
        Status status = ALREADY_CLOSED;
        {
            std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
            if (dbStore_ != nullptr) {
                auto dbStatus = dbStore_->UnRegisterObserver(obj);
                status = StoreUtil::ConvertStatus(dbStatus);
            }
        }

        Status remote = obj->UnregisterRemoteObserver();
        if (status != SUCCESS || remote != SUCCESS) {
            ZLOGE("status:0x%{public}x remote:0x%{public}x observer:0x%{public}x", status, remote,
                StoreUtil::Anonymous(obj));
        }

        delete obj;
    };
}

std::shared_ptr<ObserverBridge> SingleStoreImpl::PutIn(uint32_t &realType, std::shared_ptr<Observer> observer)
{
    std::shared_ptr<ObserverBridge> bridge = nullptr;
    observers_.Compute(uintptr_t(observer.get()),
        [this, &realType, observer, &bridge](const auto &, std::pair<uint32_t, std::shared_ptr<ObserverBridge>> &pair) {
            if ((pair.first & realType) == realType) {
                realType = (realType & (~pair.first));
                return (pair.first != 0);
            }

            if (observers_.Size() > MAX_OBSERVER_SIZE) {
                return false;
            }

            if (pair.first == 0) {
                auto release = BridgeReleaser();
                StoreId storeId{ storeId_ };
                AppId appId{ appId_ };
                pair.second = { new ObserverBridge(appId, storeId, observer, convertor_), release };
            }
            bridge = pair.second;
            realType = (realType & (~pair.first));
            pair.first = pair.first | realType;
            return (pair.first != 0);
        });
    return bridge;
}

std::shared_ptr<ObserverBridge> SingleStoreImpl::TakeOut(uint32_t &realType, std::shared_ptr<Observer> observer)
{
    std::shared_ptr<ObserverBridge> bridge = nullptr;
    observers_.ComputeIfPresent(uintptr_t(observer.get()),
        [&realType, observer, &bridge](const auto &, std::pair<uint32_t, std::shared_ptr<ObserverBridge>> &pair) {
            if ((pair.first & realType) == 0) {
                return (pair.first != 0);
            }
            realType = (realType & pair.first);
            pair.first = (pair.first & (~realType));
            bridge = pair.second;
            return (pair.first != 0);
        });
    return bridge;
}

Status SingleStoreImpl::GetResultSet(const DBQuery &query, std::shared_ptr<ResultSet> &resultSet) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    DistributedDB::KvStoreResultSet *dbResultSet = nullptr;
    auto status = dbStore_->GetEntries(query, dbResultSet);
    if (dbResultSet == nullptr) {
        return StoreUtil::ConvertStatus(status);
    }
    resultSet = std::make_shared<StoreResultSet>(dbResultSet, dbStore_, convertor_);
    return SUCCESS;
}

Status SingleStoreImpl::GetEntries(const DBQuery &query, std::vector<Entry> &entries) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", StoreUtil::Anonymous(storeId_).c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBEntry> dbEntries;
    std::string deviceId;
    auto dbStatus = dbStore_->GetEntries(query, dbEntries);
    entries.resize(dbEntries.size());
    auto it = entries.begin();
    for (auto &dbEntry : dbEntries) {
        auto &entry = *it;
        entry.key = convertor_.ToKey(std::move(dbEntry.key), deviceId);
        entry.value = std::move(dbEntry.value);
        ++it;
    }

    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status == NOT_FOUND) {
        status = SUCCESS;
    }
    return status;
}

Status SingleStoreImpl::DoClientSync(const SyncInfo &syncInfo, std::shared_ptr<SyncCallback> observer)
{
    auto complete = [observer](const std::map<std::string, DistributedDB::DBStatus> &devicesMap) {
        if (observer == nullptr) {
            return;
        }
        std::map<std::string, Status> result;
        for (auto &[key, dbStatus] : devicesMap) {
            result[key] = StoreUtil::ConvertStatus(dbStatus);
        }
        observer->SyncCompleted(result);
    };
        
    auto dbStatus = dbStore_->Sync(syncInfo.devices, StoreUtil::GetDBMode(SyncMode(syncInfo.mode)), complete);
    Status status = StoreUtil::ConvertStatus(dbStatus);
    if (status != Status::SUCCESS) {
        ZLOGE("client Sync failed: %{public}d", status);
    }
    return status;
}

Status SingleStoreImpl::DoSync(const SyncInfo &syncInfo, std::shared_ptr<SyncCallback> observer)
{
    Status cStatus = Status::SUCCESS;
    if (isClientSync_) {
        cStatus = DoClientSync(syncInfo, observer);
    }

    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }

    auto serviceAgent = service->GetServiceAgent({ appId_ });
    if (serviceAgent == nullptr) {
        ZLOGE("failed! invalid agent app:%{public}s store:%{public}s!", appId_.c_str(),
            StoreUtil::Anonymous(storeId_).c_str());
        return ILLEGAL_STATE;
    }

    serviceAgent->AddSyncCallback(observer, syncInfo.seqId);
    auto status = service->Sync({ appId_ }, { storeId_ }, syncInfo);
    if (status != Status::SUCCESS) {
        serviceAgent->DeleteSyncCallback(syncInfo.seqId);
    }

    if (!isClientSync_) {
        return status;
    }
    if (cStatus == SUCCESS || status == SUCCESS) {
        return SUCCESS;
    } else {
        ZLOGE("sync failed!: %{public}d, %{public}d", cStatus, status);
        return ERROR;
    }
}

Status SingleStoreImpl::DoSyncExt(const SyncInfo &syncInfo, std::shared_ptr<SyncCallback> observer)
{
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    auto serviceAgent = service->GetServiceAgent({ appId_ });
    if (serviceAgent == nullptr) {
        ZLOGE("failed! invalid agent app:%{public}s store:%{public}s!",
            appId_.c_str(), StoreUtil::Anonymous(storeId_).c_str());
        return ILLEGAL_STATE;
    }
    serviceAgent->AddSyncCallback(observer, syncInfo.seqId);
    auto status = service->SyncExt({ appId_ }, { storeId_ }, syncInfo);
    if (status != Status::SUCCESS) {
        serviceAgent->DeleteSyncCallback(syncInfo.seqId);
    }
    return status;
}

void SingleStoreImpl::DoNotifyChange()
{
    if (!autoSync_ && dataType_ == DataType::TYPE_DYNAMICAL) {
        return;
    }
    ZLOGD("app:%{public}s store:%{public}s dataType:%{public}d!",
        appId_.c_str(), StoreUtil::Anonymous(storeId_).c_str(), dataType_);
    if (dataType_ == DataType::TYPE_STATICS) {
        DataChangeNotifier::GetInstance().DoNotifyChange(appId_, { { storeId_ } }, true);
        return;
    }
    DataChangeNotifier::GetInstance().DoNotifyChange(appId_, { { storeId_ } });
}

void SingleStoreImpl::OnRemoteDied()
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (taskId_ > 0) {
        return;
    }
    observers_.ForEach([](const auto &, std::pair<uint32_t, std::shared_ptr<ObserverBridge>> &pair) {
        if ((pair.first & SUBSCRIBE_TYPE_REMOTE) == SUBSCRIBE_TYPE_REMOTE) {
            pair.second->OnServiceDeath();
        }
        return false;
    });
    taskId_ = TaskExecutor::GetInstance().Schedule(std::chrono::milliseconds(INTERVAL), [this]() {
        Register();
    });
}

void SingleStoreImpl::Register()
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    Status status = SUCCESS;
    observers_.ForEach([&status](const auto &, std::pair<uint32_t, std::shared_ptr<ObserverBridge>> &pair) {
        if ((pair.first & SUBSCRIBE_TYPE_REMOTE) == SUBSCRIBE_TYPE_REMOTE) {
            status = pair.second->RegisterRemoteObserver();
            if (status != SUCCESS) {
                return true;
            }
        }
        return false;
    });
    if (status != SUCCESS) {
        taskId_ = TaskExecutor::GetInstance().Schedule(std::chrono::milliseconds(INTERVAL), [this]() {
            Register();
        });
    } else {
        taskId_ = 0;
    }
}

Status SingleStoreImpl::SetIdentifier(const std::string &accountId, const std::string &appId,
    const std::string &storeId, const std::vector<std::string> &tagretDev)
{
    auto syncIdentifier = DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(accountId, appId, storeId);
    auto dbStatus = dbStore_->SetEqualIdentifier(syncIdentifier, tagretDev);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("SetIdentifier failed, status:0x%{public}x", status);
    }
    return status;
}
} // namespace OHOS::DistributedKv
