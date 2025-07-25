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

#include "kv_store_nb_delegate_impl.h"

#include <functional>
#include <string>

#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "db_types.h"
#include "kv_store_changed_data_impl.h"
#include "kv_store_errno.h"
#include "kv_store_nb_conflict_data_impl.h"
#include "kv_store_observer.h"
#include "kv_store_result_set_impl.h"
#include "kvdb_manager.h"
#include "kvdb_pragma.h"
#include "log_print.h"
#include "param_check_utils.h"
#include "performance_analysis.h"
#include "platform_specific.h"
#include "store_types.h"
#include "sync_operation.h"

namespace DistributedDB {
namespace {
    struct PragmaCmdPair {
        int externCmd = 0;
        int innerCmd = 0;
    };

    const PragmaCmdPair g_pragmaMap[] = {
        {GET_DEVICE_IDENTIFIER_OF_ENTRY, PRAGMA_GET_DEVICE_IDENTIFIER_OF_ENTRY},
        {AUTO_SYNC, PRAGMA_AUTO_SYNC},
        {PERFORMANCE_ANALYSIS_GET_REPORT, PRAGMA_PERFORMANCE_ANALYSIS_GET_REPORT},
        {PERFORMANCE_ANALYSIS_OPEN, PRAGMA_PERFORMANCE_ANALYSIS_OPEN},
        {PERFORMANCE_ANALYSIS_CLOSE, PRAGMA_PERFORMANCE_ANALYSIS_CLOSE},
        {PERFORMANCE_ANALYSIS_SET_REPORTFILENAME, PRAGMA_PERFORMANCE_ANALYSIS_SET_REPORTFILENAME},
        {GET_IDENTIFIER_OF_DEVICE, PRAGMA_GET_IDENTIFIER_OF_DEVICE},
        {GET_QUEUED_SYNC_SIZE, PRAGMA_GET_QUEUED_SYNC_SIZE},
        {SET_QUEUED_SYNC_LIMIT, PRAGMA_SET_QUEUED_SYNC_LIMIT},
        {GET_QUEUED_SYNC_LIMIT, PRAGMA_GET_QUEUED_SYNC_LIMIT},
        {SET_WIPE_POLICY, PRAGMA_SET_WIPE_POLICY},
        {RESULT_SET_CACHE_MODE, PRAGMA_RESULT_SET_CACHE_MODE},
        {RESULT_SET_CACHE_MAX_SIZE, PRAGMA_RESULT_SET_CACHE_MAX_SIZE},
        {SET_SYNC_RETRY, PRAGMA_SET_SYNC_RETRY},
        {SET_MAX_LOG_LIMIT, PRAGMA_SET_MAX_LOG_LIMIT},
        {EXEC_CHECKPOINT, PRAGMA_EXEC_CHECKPOINT},
        {SET_MAX_VALUE_SIZE, PRAGMA_SET_MAX_VALUE_SIZE},
    };

    constexpr const char *INVALID_CONNECTION = "[KvStoreNbDelegate] Invalid connection for operation";
}

KvStoreNbDelegateImpl::KvStoreNbDelegateImpl(IKvDBConnection *conn, const std::string &storeId)
    : conn_(conn),
      storeId_(storeId),
      releaseFlag_(false)
{}

KvStoreNbDelegateImpl::~KvStoreNbDelegateImpl()
{
    if (!releaseFlag_) {
        LOGF("[KvStoreNbDelegate] Can't release directly");
        return;
    }
    conn_ = nullptr;
#ifndef _WIN32
    std::lock_guard<std::mutex> lock(libMutex_);
    DBCommon::UnLoadGrdLib(dlHandle_);
    dlHandle_ = nullptr;
#endif
}

DBStatus KvStoreNbDelegateImpl::Get(const Key &key, Value &value) const
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    return GetInner(option, key, value);
}

DBStatus KvStoreNbDelegateImpl::GetEntries(const Key &keyPrefix, std::vector<Entry> &entries) const
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    return GetEntriesInner(option, keyPrefix, entries);
}

DBStatus KvStoreNbDelegateImpl::GetEntries(const Key &keyPrefix, KvStoreResultSet *&resultSet) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    IOption option;
    option.dataType = IOption::SYNC_DATA;
    IKvDBResultSet *kvDbResultSet = nullptr;
    int errCode = conn_->GetResultSet(option, keyPrefix, kvDbResultSet);
    if (errCode == E_OK) {
        resultSet = new (std::nothrow) KvStoreResultSetImpl(kvDbResultSet);
        if (resultSet != nullptr) {
            return OK;
        }

        LOGE("[KvStoreNbDelegate] Alloc result set failed.");
        conn_->ReleaseResultSet(kvDbResultSet);
        kvDbResultSet = nullptr;
        return DB_ERROR;
    }

    LOGE("[KvStoreNbDelegate] Get result set failed: %d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::GetEntries(const Query &query, std::vector<Entry> &entries) const
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    if (conn_ != nullptr) {
        int errCode = conn_->GetEntries(option, query, entries);
        if (errCode == E_OK) {
            return OK;
        } else if (errCode == -E_NOT_FOUND) {
            LOGD("[KvStoreNbDelegate] Not found the data by query");
            return NOT_FOUND;
        }

        LOGE("[KvStoreNbDelegate] Get the batch data by query err:%d", errCode);
        return TransferDBErrno(errCode);
    }

    LOGE("%s", INVALID_CONNECTION);
    return DB_ERROR;
}

DBStatus KvStoreNbDelegateImpl::GetEntries(const Query &query, KvStoreResultSet *&resultSet) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    IOption option;
    option.dataType = IOption::SYNC_DATA;
    IKvDBResultSet *kvDbResultSet = nullptr;
    int errCode = conn_->GetResultSet(option, query, kvDbResultSet);
    if (errCode == E_OK) {
        resultSet = new (std::nothrow) KvStoreResultSetImpl(kvDbResultSet);
        if (resultSet != nullptr) {
            return OK;
        }

        LOGE("[KvStoreNbDelegate] Alloc result set failed.");
        conn_->ReleaseResultSet(kvDbResultSet);
        kvDbResultSet = nullptr;
        return DB_ERROR;
    }

    LOGE("[KvStoreNbDelegate] Get result set for query failed: %d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::GetCount(const Query &query, int &count) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    IOption option;
    option.dataType = IOption::SYNC_DATA;
    int errCode = conn_->GetCount(option, query, count);
    if (errCode == E_OK) {
        if (count == 0) {
            return NOT_FOUND;
        }
        return OK;
    }

    LOGE("[KvStoreNbDelegate] Get count for query failed: %d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::CloseResultSet(KvStoreResultSet *&resultSet)
{
    if (resultSet == nullptr) {
        return INVALID_ARGS;
    }

    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    // release inner result set
    IKvDBResultSet *kvDbResultSet = nullptr;
    (static_cast<KvStoreResultSetImpl *>(resultSet))->GetResultSet(kvDbResultSet);
    conn_->ReleaseResultSet(kvDbResultSet);
    // release external result set
    delete resultSet;
    resultSet = nullptr;
    return OK;
}

DBStatus KvStoreNbDelegateImpl::Put(const Key &key, const Value &value)
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    return PutInner(option, key, value);
}

DBStatus KvStoreNbDelegateImpl::PutBatch(const std::vector<Entry> &entries)
{
    if (conn_ != nullptr) {
        IOption option;
        option.dataType = IOption::SYNC_DATA;
        int errCode = conn_->PutBatch(option, entries);
        if (errCode == E_OK) {
            return OK;
        }

        LOGE("[KvStoreNbDelegate] Put batch data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }

    LOGE("%s", INVALID_CONNECTION);
    return DB_ERROR;
}

DBStatus KvStoreNbDelegateImpl::DeleteBatch(const std::vector<Key> &keys)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    IOption option;
    option.dataType = IOption::SYNC_DATA;
    int errCode = conn_->DeleteBatch(option, keys);
    if (errCode == E_OK || errCode == -E_NOT_FOUND) {
        return OK;
    }

    LOGE("[KvStoreNbDelegate] Delete batch data failed:%d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::Delete(const Key &key)
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    return DeleteInner(option, key);
}

DBStatus KvStoreNbDelegateImpl::GetLocal(const Key &key, Value &value) const
{
    IOption option;
    option.dataType = IOption::LOCAL_DATA;
    return GetInner(option, key, value);
}

DBStatus KvStoreNbDelegateImpl::GetLocalEntries(const Key &keyPrefix, std::vector<Entry> &entries) const
{
    IOption option;
    option.dataType = IOption::LOCAL_DATA;
    return GetEntriesInner(option, keyPrefix, entries);
}

DBStatus KvStoreNbDelegateImpl::PutLocal(const Key &key, const Value &value)
{
    IOption option;
    option.dataType = IOption::LOCAL_DATA;
    return PutInner(option, key, value);
}

DBStatus KvStoreNbDelegateImpl::DeleteLocal(const Key &key)
{
    IOption option;
    option.dataType = IOption::LOCAL_DATA;
    return DeleteInner(option, key);
}

DBStatus KvStoreNbDelegateImpl::PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
    const KvStoreNbPublishOnConflict &onConflict)
{
    if (key.empty() || key.size() > DBConstant::MAX_KEY_SIZE) {
        LOGW("[KvStoreNbDelegate][Publish] Invalid para");
        return INVALID_ARGS;
    }

    if (conn_ != nullptr) {
        PragmaPublishInfo publishInfo{ key, deleteLocal, updateTimestamp, onConflict };
        int errCode = conn_->Pragma(PRAGMA_PUBLISH_LOCAL, static_cast<PragmaData>(&publishInfo));
        if (errCode != E_OK) {
            LOGE("[KvStoreNbDelegate] Publish local err:%d", errCode);
        }
        return TransferDBErrno(errCode);
    }

    LOGE("%s", INVALID_CONNECTION);
    return DB_ERROR;
}

DBStatus KvStoreNbDelegateImpl::UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp)
{
    if (key.empty() || key.size() > DBConstant::MAX_KEY_SIZE) {
        LOGW("[KvStoreNbDelegate][Unpublish] Invalid para");
        return INVALID_ARGS;
    }

    if (conn_ != nullptr) {
        PragmaUnpublishInfo unpublishInfo{ key, deletePublic, updateTimestamp };
        int errCode = conn_->Pragma(PRAGMA_UNPUBLISH_SYNC, static_cast<PragmaData>(&unpublishInfo));
        if (errCode != E_OK) {
            LOGE("[KvStoreNbDelegate] Unpublish result:%d", errCode);
        }
        return TransferDBErrno(errCode);
    }

    LOGE("%s", INVALID_CONNECTION);
    return DB_ERROR;
}

DBStatus KvStoreNbDelegateImpl::PutLocalBatch(const std::vector<Entry> &entries)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    IOption option;
    option.dataType = IOption::LOCAL_DATA;
    int errCode = conn_->PutBatch(option, entries);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Put local batch data failed:%d", errCode);
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::DeleteLocalBatch(const std::vector<Key> &keys)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    IOption option;
    option.dataType = IOption::LOCAL_DATA;
    int errCode = conn_->DeleteBatch(option, keys);
    if (errCode == E_OK || errCode == -E_NOT_FOUND) {
        return OK;
    }

    LOGE("[KvStoreNbDelegate] Delete local batch data failed:%d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::RegisterObserver(const Key &key, unsigned int mode, KvStoreObserver *observer)
{
    if (key.size() > DBConstant::MAX_KEY_SIZE) {
        return INVALID_ARGS;
    }
    if (observer == nullptr) {
        LOGE("[KvStoreNbDelegate][RegisterObserver] Observer is null");
        return INVALID_ARGS;
    }
    if (conn_ == nullptr) {
        LOGE("[RegisterObserver]%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    uint64_t rawMode = DBCommon::EraseBit(mode, DBConstant::OBSERVER_CHANGES_MASK);
    if (rawMode == static_cast<uint64_t>(ObserverMode::OBSERVER_CHANGES_CLOUD)) {
        return RegisterCloudObserver(key, mode, observer);
    }
    return RegisterDeviceObserver(key, static_cast<unsigned int>(rawMode), observer);
}

DBStatus KvStoreNbDelegateImpl::CheckDeviceObserver(const Key &key, unsigned int mode, KvStoreObserver *observer)
{
    if (!ParamCheckUtils::CheckObserver(key, mode)) {
        LOGE("[KvStoreNbDelegate][CheckDeviceObserver] Register nb observer by illegal mode or key size!");
        return INVALID_ARGS;
    }

    std::lock_guard<std::mutex> lockGuard(observerMapLock_);
    if (observerMap_.size() >= DBConstant::MAX_OBSERVER_COUNT) {
        LOGE("[KvStoreNbDelegate][CheckDeviceObserver] The number of kv observers has been over limit, storeId[%.3s]",
            storeId_.c_str());
        return OVER_MAX_LIMITS;
    }
    if (observerMap_.find(observer) != observerMap_.end()) {
        LOGE("[KvStoreNbDelegate][CheckDeviceObserver] Observer has been already registered!");
        return ALREADY_SET;
    }
    return OK;
}

DBStatus KvStoreNbDelegateImpl::RegisterDeviceObserver(const Key &key, unsigned int mode, KvStoreObserver *observer)
{
    if (conn_->IsTransactionStarted()) {
        LOGE("[KvStoreNbDelegate][RegisterDeviceObserver] Transaction unfinished");
        return BUSY;
    }
    DBStatus status = CheckDeviceObserver(key, mode, observer);
    if (status != OK) {
        LOGE("[KvStoreNbDelegate][RegisterDeviceObserver] Observer map cannot be registered, status:%d", status);
        return status;
    }

    int errCode = E_OK;
    auto storeId = storeId_;
    KvDBObserverHandle *observerHandle = conn_->RegisterObserver(
        mode, key,
        [observer, storeId](const KvDBCommitNotifyData &notifyData) {
            KvStoreChangedDataImpl data(&notifyData);
            LOGD("[KvStoreNbDelegate][RegisterDeviceObserver] Trigger [%s] on change", storeId.c_str());
            observer->OnChange(data);
        },
        errCode);

    if (errCode != E_OK || observerHandle == nullptr) {
        LOGE("[KvStoreNbDelegate][RegisterDeviceObserver] Register device observer failed:%d!", errCode);
        return DB_ERROR;
    }

    observerMap_.insert(std::pair<const KvStoreObserver *, const KvDBObserverHandle *>(observer, observerHandle));
    LOGI("[KvStoreNbDelegate][RegisterDeviceObserver] Register device observer ok mode:%u", mode);
    return OK;
}

DBStatus KvStoreNbDelegateImpl::CheckCloudObserver(KvStoreObserver *observer)
{
    std::lock_guard<std::mutex> lockGuard(observerMapLock_);
    if (cloudObserverMap_.size() >= DBConstant::MAX_OBSERVER_COUNT) {
        LOGE("[KvStoreNbDelegate][CheckCloudObserver] The number of kv cloud observers over limit, storeId[%.3s]",
            storeId_.c_str());
        return OVER_MAX_LIMITS;
    }
    if (cloudObserverMap_.find(observer) != cloudObserverMap_.end()) {
        LOGE("[KvStoreNbDelegate][CheckCloudObserver] Cloud observer has been already registered!");
        return ALREADY_SET;
    }
    return OK;
}

DBStatus KvStoreNbDelegateImpl::RegisterCloudObserver(const Key &key, unsigned int mode, KvStoreObserver *observer)
{
    DBStatus status = CheckCloudObserver(observer);
    if (status != OK) {
        LOGE("[KvStoreNbDelegate][RegisterCloudObserver] Cloud observer map cannot be registered, status:%d", status);
        return status;
    }

    auto storeId = storeId_;
    ObserverAction action = [observer, storeId](
                                const std::string &device, ChangedData &&changedData, bool isChangedData) {
        if (isChangedData) {
            LOGD("[KvStoreNbDelegate][RegisterCloudObserver] Trigger [%s] on change", storeId.c_str());
            observer->OnChange(Origin::ORIGIN_CLOUD, device, std::move(changedData));
        }
    };
    int errCode = conn_->RegisterObserverAction(observer, action);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate][RegisterCloudObserver] Register cloud observer failed:%d!", errCode);
        return DB_ERROR;
    }
    cloudObserverMap_[observer] = mode;
    LOGI("[KvStoreNbDelegate][RegisterCloudObserver] Register cloud observer ok mode:%u", mode);
    return OK;
}

DBStatus KvStoreNbDelegateImpl::UnRegisterObserver(const KvStoreObserver *observer)
{
    if (observer == nullptr) {
        return INVALID_ARGS;
    }

    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    DBStatus cloudRet = UnRegisterCloudObserver(observer);
    DBStatus devRet = UnRegisterDeviceObserver(observer);
    if (cloudRet == OK || devRet == OK) {
        return OK;
    }
    return devRet;
}

DBStatus KvStoreNbDelegateImpl::UnRegisterDeviceObserver(const KvStoreObserver *observer)
{
    std::lock_guard<std::mutex> lockGuard(observerMapLock_);
    auto iter = observerMap_.find(observer);
    if (iter == observerMap_.end()) {
        LOGE("[KvStoreNbDelegate] [%s] Observer has not been registered!", storeId_.c_str());
        return NOT_FOUND;
    }

    const KvDBObserverHandle *observerHandle = iter->second;
    int errCode = conn_->UnRegisterObserver(observerHandle);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] UnRegistObserver failed:%d!", errCode);
        return DB_ERROR;
    }
    observerMap_.erase(iter);
    return OK;
}

DBStatus KvStoreNbDelegateImpl::UnRegisterCloudObserver(const KvStoreObserver *observer)
{
    std::lock_guard<std::mutex> lockGuard(observerMapLock_);
    auto iter = cloudObserverMap_.find(observer);
    if (iter == cloudObserverMap_.end()) {
        LOGW("[KvStoreNbDelegate] [%s] CloudObserver has not been registered!", storeId_.c_str());
        return NOT_FOUND;
    }
    int errCode = conn_->UnRegisterObserverAction(observer);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] UnRegisterCloudObserver failed:%d!", errCode);
        return DB_ERROR;
    }
    cloudObserverMap_.erase(iter);
    return OK;
}

DBStatus KvStoreNbDelegateImpl::RemoveDeviceData(const std::string &device)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    if (device.empty() || device.length() > DBConstant::MAX_DEV_LENGTH) {
        return INVALID_ARGS;
    }
    int errCode = conn_->Pragma(PRAGMA_RM_DEVICE_DATA,
        const_cast<void *>(static_cast<const void *>(&device)));
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate][%.3s] Remove specific device data failed:%d", storeId_.c_str(), errCode);
    } else {
        LOGI("[KvStoreNbDelegate][%.3s] Remove specific device data OK", storeId_.c_str());
    }
    return TransferDBErrno(errCode);
}

std::string KvStoreNbDelegateImpl::GetStoreId() const
{
    return storeId_;
}

DBStatus KvStoreNbDelegateImpl::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    bool wait = false)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    if (mode > SYNC_MODE_PUSH_PULL) {
        LOGE("not support other mode");
        return NOT_SUPPORT;
    }

    PragmaSync pragmaData(
        devices, mode, [this, onComplete](const std::map<std::string, int> &statuses) {
            OnSyncComplete(statuses, onComplete);
        }, wait);
    int errCode = conn_->Pragma(PRAGMA_SYNC_DEVICES, &pragmaData);
    if (errCode < E_OK) {
        LOGE("[KvStoreNbDelegate] Sync data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus KvStoreNbDelegateImpl::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    if (mode > SYNC_MODE_PUSH_PULL) {
        LOGE("not support other mode");
        return NOT_SUPPORT;
    }

    QuerySyncObject querySyncObj(query);
    if (!querySyncObj.GetRelationTableNames().empty()) {
        LOGE("check query table names from tables failed!");
        return NOT_SUPPORT;
    }

    if (!DBCommon::CheckQueryWithoutMultiTable(query)) {
        LOGE("not support for invalid query");
        return NOT_SUPPORT;
    }
    if (querySyncObj.GetSortType() != SortType::NONE || querySyncObj.IsQueryByRange()) {
        LOGE("not support order by timestamp and query by range");
        return NOT_SUPPORT;
    }
    PragmaSync pragmaData(
        devices, mode, querySyncObj, [this, onComplete](const std::map<std::string, int> &statuses) {
            OnSyncComplete(statuses, onComplete);
        }, wait);
    int errCode = conn_->Pragma(PRAGMA_SYNC_DEVICES, &pragmaData);
    if (errCode < E_OK) {
        LOGE("[KvStoreNbDelegate] QuerySync data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

void KvStoreNbDelegateImpl::OnDeviceSyncProcess(const std::map<std::string, DeviceSyncProcess> &processMap,
    const DeviceSyncProcessCallback &onProcess) const
{
    std::map<std::string, DeviceSyncProcess> result;
    for (const auto &pair : processMap) {
        DeviceSyncProcess info = pair.second;
        int status = info.errCode;
        info.errCode = SyncOperation::DBStatusTrans(status);
        info.process = SyncOperation::DBStatusTransProcess(status);
        result.insert(std::pair<std::string, DeviceSyncProcess>(pair.first, info));
    }
    if (onProcess) {
        onProcess(result);
    }
}

DBStatus KvStoreNbDelegateImpl::Sync(const DeviceSyncOption &option, const DeviceSyncProcessCallback &onProcess)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    if (option.mode != SYNC_MODE_PULL_ONLY) {
        LOGE("Not support other mode");
        return NOT_SUPPORT;
    }
    DeviceSyncProcessCallback onSyncProcess = [this, onProcess](const std::map<std::string, DeviceSyncProcess> &map) {
        OnDeviceSyncProcess(map, onProcess);
    };
    int errCode = E_OK;
    if (option.isQuery) {
        QuerySyncObject querySyncObj(option.query);
        if (!querySyncObj.GetRelationTableNames().empty()) {
            LOGE("Sync with option and check query table names from tables failed!");
            return NOT_SUPPORT;
        }
        if (!DBCommon::CheckQueryWithoutMultiTable(option.query)) {
            LOGE("Not support for invalid query");
            return NOT_SUPPORT;
        }

        if (querySyncObj.GetSortType() != SortType::NONE || querySyncObj.IsQueryByRange()) {
            LOGE("Not support order by timestamp and query by range");
            return NOT_SUPPORT;
        }
        PragmaSync pragmaData(option, querySyncObj, onSyncProcess);
        errCode = conn_->Pragma(PRAGMA_SYNC_DEVICES, &pragmaData);
    } else {
        PragmaSync pragmaData(option, onSyncProcess);
        errCode = conn_->Pragma(PRAGMA_SYNC_DEVICES, &pragmaData);
    }

    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] DeviceSync data failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus KvStoreNbDelegateImpl::CancelSync(uint32_t syncId)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    uint32_t tempSyncId = syncId;
    int errCode = conn_->Pragma(PRAGMA_CANCEL_SYNC_DEVICES, &tempSyncId);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] CancelSync failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus KvStoreNbDelegateImpl::Pragma(PragmaCmd cmd, PragmaData &paramData)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    int errCode = -E_NOT_SUPPORT;
    for (const auto &item : g_pragmaMap) {
        if (item.externCmd == cmd) {
            errCode = conn_->Pragma(item.innerCmd, paramData);
            break;
        }
    }

    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Pragma failed:%d", errCode);
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::SetConflictNotifier(int conflictType, const KvStoreNbConflictNotifier &notifier)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    if (!ParamCheckUtils::CheckConflictNotifierType(conflictType)) {
        LOGE("%s", INVALID_CONNECTION);
        return INVALID_ARGS;
    }

    int errCode;
    if (!notifier) {
        errCode = conn_->SetConflictNotifier(conflictType, nullptr);
        goto END;
    }

    errCode = conn_->SetConflictNotifier(conflictType,
        [conflictType, notifier](const KvDBCommitNotifyData &data) {
            int resultCode;
            const std::list<KvDBConflictEntry> entries = data.GetCommitConflicts(resultCode);
            if (resultCode != E_OK) {
                LOGE("Get commit conflicted entries failed:%d!", resultCode);
                return;
            }

            for (const auto &entry : entries) {
                // Prohibit signed numbers to perform bit operations
                uint32_t entryType = static_cast<uint32_t>(entry.type);
                uint32_t type = static_cast<uint32_t>(conflictType);
                if (entryType & type) {
                    KvStoreNbConflictDataImpl dataImpl;
                    dataImpl.SetConflictData(entry);
                    notifier(dataImpl);
                }
            }
        });

END:
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Register conflict failed:%d!", errCode);
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::Rekey(const CipherPassword &password)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    int errCode = conn_->Rekey(password);
    if (errCode == E_OK) {
        return OK;
    }

    LOGE("[KvStoreNbDelegate] Rekey failed:%d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::Export(const std::string &filePath, const CipherPassword &passwd, bool force)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    std::string fileDir;
    std::string fileName;
    OS::SplitFilePath(filePath, fileDir, fileName);

    std::string canonicalUrl;
    if (!ParamCheckUtils::CheckDataDir(fileDir, canonicalUrl)) {
        return INVALID_ARGS;
    }

    if (!OS::CheckPathExistence(canonicalUrl)) {
        return NO_PERMISSION;
    }

    canonicalUrl = canonicalUrl + "/" + fileName;
    if (!force && OS::CheckPathExistence(canonicalUrl)) {
        return FILE_ALREADY_EXISTED;
    }

    int errCode = conn_->Export(canonicalUrl, passwd);
    if (errCode == E_OK) {
        return OK;
    }
    LOGE("[KvStoreNbDelegate] Export failed:%d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::Import(const std::string &filePath, const CipherPassword &passwd,
    bool isNeedIntegrityCheck)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    std::string fileDir;
    std::string fileName;
    OS::SplitFilePath(filePath, fileDir, fileName);

    std::string canonicalUrl;
    if (!ParamCheckUtils::CheckDataDir(fileDir, canonicalUrl)) {
        return INVALID_ARGS;
    }

    canonicalUrl = canonicalUrl + "/" + fileName;
    if (!OS::CheckPathExistence(canonicalUrl)) {
        LOGE("Import file path err, DBStatus = INVALID_FILE errno = [%d]", errno);
        return INVALID_FILE;
    }

    int errCode = conn_->Import(canonicalUrl, passwd, isNeedIntegrityCheck);
    if (errCode == E_OK) {
        LOGI("[KvStoreNbDelegate] Import ok");
        return OK;
    }

    LOGE("[KvStoreNbDelegate] Import failed:%d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::StartTransaction()
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    int errCode = conn_->StartTransaction();
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] StartTransaction failed:%d", errCode);
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::Commit()
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    int errCode = conn_->Commit();
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Commit failed:%d", errCode);
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::Rollback()
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    int errCode = conn_->RollBack();
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Rollback failed:%d", errCode);
    }
    return TransferDBErrno(errCode);
}

void KvStoreNbDelegateImpl::SetReleaseFlag(bool flag)
{
    releaseFlag_ = flag;
}

DBStatus KvStoreNbDelegateImpl::Close()
{
    if (conn_ != nullptr) {
        int errCode = KvDBManager::ReleaseDatabaseConnection(conn_);
        if (errCode == -E_BUSY) {
            LOGI("[KvStoreNbDelegate] Busy for close");
            return BUSY;
        }
        conn_ = nullptr;
    }
    return OK;
}

DBStatus KvStoreNbDelegateImpl::CheckIntegrity() const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    return TransferDBErrno(conn_->CheckIntegrity());
}

DBStatus KvStoreNbDelegateImpl::GetSecurityOption(SecurityOption &option) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    return TransferDBErrno(conn_->GetSecurityOption(option.securityLabel, option.securityFlag));
}

DBStatus KvStoreNbDelegateImpl::SetRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    PragmaRemotePushNotify notify(notifier);
    int errCode = conn_->Pragma(PRAGMA_REMOTE_PUSH_FINISHED_NOTIFY, reinterpret_cast<void *>(&notify));
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Set remote push finished notify failed : %d", errCode);
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::GetInner(const IOption &option, const Key &key, Value &value) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    int errCode = conn_->Get(option, key, value);
    if (errCode == E_OK) {
        return OK;
    }

    if (errCode != -E_NOT_FOUND) {
        LOGE("[KvStoreNbDelegate] [%s] Get the data failed:%d", storeId_.c_str(), errCode);
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::GetEntriesInner(const IOption &option,
    const Key &keyPrefix, std::vector<Entry> &entries) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    int errCode = conn_->GetEntries(option, keyPrefix, entries);
    if (errCode == E_OK) {
        return OK;
    }
    LOGE("[KvStoreNbDelegate] Get the batch data failed:%d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::PutInner(const IOption &option, const Key &key, const Value &value)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_PUT_DATA);
    }

    int errCode = conn_->Put(option, key, value);
    if (performance != nullptr) {
        performance->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_PUT_DATA);
    }

    if (errCode == E_OK) {
        return OK;
    }
    LOGE("[KvStoreNbDelegate] Put the data failed:%d", errCode);
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::DeleteInner(const IOption &option, const Key &key)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    int errCode = conn_->Delete(option, key);
    if (errCode == E_OK || errCode == -E_NOT_FOUND) {
        return OK;
    }

    LOGE("[KvStoreNbDelegate] Delete the data failed:%d", errCode);
    return TransferDBErrno(errCode);
}

void KvStoreNbDelegateImpl::OnSyncComplete(const std::map<std::string, int> &statuses,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete) const
{
    std::map<std::string, DBStatus> result;
    for (const auto &pair : statuses) {
        DBStatus status = SyncOperation::DBStatusTrans(pair.second);
        result.insert(std::pair<std::string, DBStatus>(pair.first, status));
    }
    if (onComplete) {
        onComplete(result);
    }
}

DBStatus KvStoreNbDelegateImpl::SetEqualIdentifier(const std::string &identifier,
    const std::vector<std::string> &targets)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    PragmaSetEqualIdentifier pragma(identifier, targets);
    int errCode = conn_->Pragma(PRAGMA_ADD_EQUAL_IDENTIFIER, reinterpret_cast<void *>(&pragma));
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Set store equal identifier failed : %d", errCode);
    }

    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::SetPushDataInterceptor(const PushDataInterceptor &interceptor)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    PushDataInterceptor notify = interceptor;
    int errCode = conn_->Pragma(PRAGMA_INTERCEPT_SYNC_DATA, static_cast<void *>(&notify));
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Set data interceptor notify failed : %d", errCode);
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::SubscribeRemoteQuery(const std::vector<std::string> &devices,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    QuerySyncObject querySyncObj(query);
    if (querySyncObj.GetSortType() != SortType::NONE || querySyncObj.IsQueryByRange()) {
        LOGE("not support order by timestamp and query by range");
        return NOT_SUPPORT;
    }
    PragmaSync pragmaData(devices, SyncModeType::SUBSCRIBE_QUERY, querySyncObj,
        [this, onComplete](const std::map<std::string, int> &statuses) { OnSyncComplete(statuses, onComplete); }, wait);
    int errCode = conn_->Pragma(PRAGMA_SUBSCRIBE_QUERY, &pragmaData);
    if (errCode < E_OK) {
        LOGE("[KvStoreNbDelegate] Subscribe remote data with query failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus KvStoreNbDelegateImpl::UnSubscribeRemoteQuery(const std::vector<std::string> &devices,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    QuerySyncObject querySyncObj(query);
    if (querySyncObj.GetSortType() != SortType::NONE || querySyncObj.IsQueryByRange()) {
        LOGE("not support order by timestamp and query by range");
        return NOT_SUPPORT;
    }
    PragmaSync pragmaData(devices, SyncModeType::UNSUBSCRIBE_QUERY, querySyncObj,
        [this, onComplete](const std::map<std::string, int> &statuses) { OnSyncComplete(statuses, onComplete); }, wait);
    int errCode = conn_->Pragma(PRAGMA_SUBSCRIBE_QUERY, &pragmaData);
    if (errCode < E_OK) {
        LOGE("[KvStoreNbDelegate] Unsubscribe remote data with query failed:%d", errCode);
        return TransferDBErrno(errCode);
    }
    return OK;
}

DBStatus KvStoreNbDelegateImpl::RemoveDeviceData()
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }

    std::string device; // Empty device for remove all device data
    int errCode = conn_->Pragma(PRAGMA_RM_DEVICE_DATA,
        const_cast<void *>(static_cast<const void *>(&device)));
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate][%.3s] Remove device data failed:%d", storeId_.c_str(), errCode);
    } else {
        LOGI("[KvStoreNbDelegate][%.3s] Remove device data OK", storeId_.c_str());
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::GetKeys(const Key &keyPrefix, std::vector<Key> &keys) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    int errCode = conn_->GetKeys(option, keyPrefix, keys);
    if (errCode == E_OK) {
        return OK;
    }
    LOGE("[KvStoreNbDelegate] Get the keys failed:%d", errCode);
    return TransferDBErrno(errCode);
}

size_t KvStoreNbDelegateImpl::GetSyncDataSize(const std::string &device) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return 0;
    }
    if (device.empty()) {
        LOGE("device len is invalid");
        return 0;
    }
    size_t size = 0;
    int errCode = conn_->GetSyncDataSize(device, size);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] calculate sync data size failed : %d", errCode);
        return 0;
    }
    return size;
}

DBStatus KvStoreNbDelegateImpl::UpdateKey(const UpdateKeyCallback &callback)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    if (callback == nullptr) {
        LOGE("[KvStoreNbDelegate] Invalid callback for operation");
        return INVALID_ARGS;
    }
    int errCode = conn_->UpdateKey(callback);
    if (errCode == E_OK) {
        LOGI("[KvStoreNbDelegate] update keys success");
        return OK;
    }
    LOGE("[KvStoreNbDelegate] update keys failed:%d", errCode);
    return TransferDBErrno(errCode);
}

std::pair<DBStatus, WatermarkInfo> KvStoreNbDelegateImpl::GetWatermarkInfo(const std::string &device)
{
    std::pair<DBStatus, WatermarkInfo> res;
    if (device.empty() || device.size() > DBConstant::MAX_DEV_LENGTH) {
        LOGE("[KvStoreNbDelegate] device invalid length %zu", device.size());
        res.first = INVALID_ARGS;
        return res;
    }
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        res.first = DB_ERROR;
        return res;
    }
    int errCode = conn_->GetWatermarkInfo(device, res.second);
    if (errCode == E_OK) {
        LOGI("[KvStoreNbDelegate] get watermark info success");
    } else {
        LOGE("[KvStoreNbDelegate] get watermark info failed:%d", errCode);
    }
    res.first = TransferDBErrno(errCode);
    return res;
}

DBStatus KvStoreNbDelegateImpl::RemoveDeviceData(const std::string &device, ClearMode mode)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    int errCode = conn_->RemoveDeviceData(device, mode);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate][%.3s] Remove device data res %d", storeId_.c_str(), errCode);
    } else {
        LOGI("[KvStoreNbDelegate][%.3s] Remove device data OK, mode[%d]", storeId_.c_str(), static_cast<int>(mode));
    }
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::RemoveDeviceData(const std::string &device, const std::string &user,
    ClearMode mode)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    if (user.empty() && mode != ClearMode::DEFAULT) {
        LOGE("[KvStoreNbDelegate] Remove device data with empty user!");
        return INVALID_ARGS;
    }
    int errCode = conn_->RemoveDeviceData(device, user, mode);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate][%.3s] Remove device data with user res %d", storeId_.c_str(), errCode);
    } else {
        LOGI("[KvStoreNbDelegate][%.3s] Remove device data with user OK, mode[%d]",
            storeId_.c_str(), static_cast<int>(mode));
    }
    return TransferDBErrno(errCode);
}

int32_t KvStoreNbDelegateImpl::GetTaskCount()
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    return conn_->GetTaskCount();
}

DBStatus KvStoreNbDelegateImpl::SetReceiveDataInterceptor(const DataInterceptor &interceptor)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    int errCode = conn_->SetReceiveDataInterceptor(interceptor);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Set receive data interceptor errCode:%d", errCode);
    }
    LOGI("[KvStoreNbDelegate] Set receive data interceptor");
    return TransferDBErrno(errCode);
}

DBStatus KvStoreNbDelegateImpl::GetDeviceEntries(const std::string &device, std::vector<Entry> &entries) const
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    int errCode = conn_->GetEntries(device, entries);
    if (errCode == E_OK) {
        return OK;
    }
    LOGE("[KvStoreNbDelegate] Get the entries failed:%d", errCode);
    return TransferDBErrno(errCode);
}

KvStoreNbDelegate::DatabaseStatus KvStoreNbDelegateImpl::GetDatabaseStatus() const
{
    KvStoreNbDelegate::DatabaseStatus status;
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return status;
    }
    status.isRebuild = conn_->IsRebuild();
    LOGI("[KvStoreNbDelegate] rebuild %d", static_cast<int>(status.isRebuild));
    return status;
}

#ifdef USE_DISTRIBUTEDDB_CLOUD
DBStatus KvStoreNbDelegateImpl::Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    return TransferDBErrno(conn_->Sync(option, onProcess));
}

DBStatus KvStoreNbDelegateImpl::SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    if (cloudDBs.empty()) {
        LOGE("[KvStoreNbDelegate] no cloud db");
        return INVALID_ARGS;
    }
    return TransferDBErrno(conn_->SetCloudDB(cloudDBs));
}

DBStatus KvStoreNbDelegateImpl::SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    return TransferDBErrno(conn_->SetCloudDbSchema(schema));
}

DBStatus KvStoreNbDelegateImpl::SetCloudSyncConfig(const CloudSyncConfig &config)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    if (!DBCommon::CheckCloudSyncConfigValid(config)) {
        return INVALID_ARGS;
    }
    int errCode = conn_->SetCloudSyncConfig(config);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Set cloud sync config errCode:%d", errCode);
    }
    LOGI("[KvStoreNbDelegate] Set cloud sync config");
    return TransferDBErrno(errCode);
}

void KvStoreNbDelegateImpl::SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback)
{
    if (conn_ == nullptr || callback == nullptr) {
        LOGD("[KvStoreNbDelegate] Invalid connection or callback for operation");
        return;
    }
    conn_->SetGenCloudVersionCallback(callback);
}

std::pair<DBStatus, std::map<std::string, std::string>> KvStoreNbDelegateImpl::GetCloudVersion(
    const std::string &device)
{
    std::pair<DBStatus, std::map<std::string, std::string>> res;
    if (device.size() > DBConstant::MAX_DEV_LENGTH) {
        LOGE("[KvStoreNbDelegate] device invalid length %zu", device.size());
        res.first = INVALID_ARGS;
        return res;
    }
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        res.first = DB_ERROR;
        return res;
    }
    int errCode = conn_->GetCloudVersion(device, res.second);
    if (errCode == E_OK) {
        LOGI("[KvStoreNbDelegate] get cloudVersion success");
    } else {
        LOGE("[KvStoreNbDelegate] get cloudVersion failed:%d", errCode);
    }
    if (errCode == E_OK && res.second.empty()) {
        errCode = -E_NOT_FOUND;
    }
    res.first = TransferDBErrno(errCode);
    return res;
}

DBStatus KvStoreNbDelegateImpl::ClearMetaData(ClearKvMetaDataOption option)
{
    if (option.type != ClearKvMetaOpType::CLEAN_CLOUD_WATERMARK) {
        return NOT_SUPPORT;
    }
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    int errCode = conn_->ClearCloudWatermark();
    if (errCode == E_OK) {
        LOGI("[KvStoreNbDelegate][%.3s] clear kv cloud watermark success", storeId_.c_str());
    } else {
        LOGE("[KvStoreNbDelegate][%.3s] clear kv cloud watermark failed:%d", storeId_.c_str(), errCode);
    }
    return TransferDBErrno(errCode);
}
#endif

DBStatus KvStoreNbDelegateImpl::OperateDataStatus(uint32_t dataOperator)
{
    if (conn_ == nullptr) {
        LOGE("%s", INVALID_CONNECTION);
        return DB_ERROR;
    }
    int errCode = conn_->OperateDataStatus(dataOperator);
    if (errCode != E_OK) {
        LOGE("[KvStoreNbDelegate] Operate data status errCode:%d", errCode);
    } else {
        LOGI("[KvStoreNbDelegate] Operate data status success");
    }
    return TransferDBErrno(errCode);
}

void KvStoreNbDelegateImpl::SetHandle(void *handle)
{
#ifndef _WIN32
    std::lock_guard<std::mutex> lock(libMutex_);
    dlHandle_ = handle;
#endif
}
} // namespace DistributedDB
