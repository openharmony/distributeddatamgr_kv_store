/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "sqlite_single_ver_natural_store.h"

#include <algorithm>
#include <thread>
#include <chrono>

#include "data_compression.h"
#include "db_common.h"
#include "db_constant.h"
#include "db_dump_helper.h"
#include "db_dfx_adapter.h"
#include "db_errno.h"
#include "generic_single_ver_kv_entry.h"
#include "intercepted_data_impl.h"
#include "kvdb_utils.h"
#include "log_print.h"
#include "platform_specific.h"
#include "schema_object.h"
#include "single_ver_database_oper.h"
#include "single_ver_utils.h"
#include "storage_engine_manager.h"
#include "sqlite_single_ver_natural_store_connection.h"
#include "value_hash_calc.h"

namespace DistributedDB {
KvDBProperties &SQLiteSingleVerNaturalStore::GetDbPropertyForUpdate()
{
    return MyProp();
}

void SQLiteSingleVerNaturalStore::HeartBeatForLifeCycle() const
{
    std::lock_guard<std::mutex> lock(lifeCycleMutex_);
    int errCode = ResetLifeCycleTimer();
    if (errCode != E_OK) {
        LOGE("Heart beat for life cycle failed:%d", errCode);
    }
}

int SQLiteSingleVerNaturalStore::StartLifeCycleTimer(const DatabaseLifeCycleNotifier &notifier) const
{
    auto runtimeCxt = RuntimeContext::GetInstance();
    if (runtimeCxt == nullptr) {
        return -E_INVALID_ARGS;
    }
    RefObject::IncObjRef(this);
    TimerId timerId = 0;
    int errCode = runtimeCxt->SetTimer(autoLifeTime_,
        [this](TimerId id) -> int {
            std::lock_guard<std::mutex> lock(lifeCycleMutex_);
            if (lifeCycleNotifier_) {
                std::string identifier;
                if (GetMyProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE, false)) {
                    identifier = GetMyProperties().GetStringProp(KvDBProperties::DUAL_TUPLE_IDENTIFIER_DATA, "");
                } else {
                    identifier = GetMyProperties().GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
                }
                auto userId = GetMyProperties().GetStringProp(DBProperties::USER_ID, "");
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

int SQLiteSingleVerNaturalStore::ResetLifeCycleTimer() const
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

int SQLiteSingleVerNaturalStore::StopLifeCycleTimer() const
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

bool SQLiteSingleVerNaturalStore::IsDataMigrating() const
{
    if (storageEngine_ == nullptr) {
        return false;
    }

    if (storageEngine_->IsMigrating()) {
        LOGD("Migrating now.");
        return true;
    }
    return false;
}

void SQLiteSingleVerNaturalStore::SetConnectionFlag(bool isExisted) const
{
    if (storageEngine_ != nullptr) {
        storageEngine_->SetConnectionFlag(isExisted);
    }
}

int SQLiteSingleVerNaturalStore::TriggerToMigrateData() const
{
    SQLiteSingleVerStorageEngine *storageEngine = nullptr;
    {
        std::lock_guard<std::shared_mutex> autoLock(engineMutex_);
        if (storageEngine_ == nullptr) {
            return E_OK;
        }
        storageEngine = storageEngine_;
        RefObject::IncObjRef(storageEngine);
    }
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, storageEngine]() {
        AsyncDataMigration(storageEngine);
    });
    if (errCode != E_OK) {
        RefObject::DecObjRef(this);
        RefObject::DecObjRef(storageEngine);
        LOGE("[SingleVerNStore] Trigger to migrate data failed : %d.", errCode);
    }
    return errCode;
}

bool SQLiteSingleVerNaturalStore::IsCacheDBMode() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] IsCacheDBMode storage engine is invalid.");
        return false;
    }
    EngineState engineState = storageEngine_->GetEngineState();
    return (engineState == EngineState::CACHEDB);
}

bool SQLiteSingleVerNaturalStore::IsExtendedCacheDBMode() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] storage engine is invalid.");
        return false;
    }
    EngineState engineState = storageEngine_->GetEngineState();
    return (engineState == EngineState::CACHEDB || engineState == EngineState::MIGRATING ||
        engineState == EngineState::ATTACHING);
}

int SQLiteSingleVerNaturalStore::CheckReadDataControlled() const
{
    if (IsExtendedCacheDBMode()) {
        int err = IsCacheDBMode() ? -E_EKEYREVOKED : -E_BUSY;
        LOGE("Existed cache database can not read data, errCode = [%d]!", err);
        return err;
    }
    return E_OK;
}

void SQLiteSingleVerNaturalStore::IncreaseCacheRecordVersion() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] Increase cache version storage engine is invalid.");
        return;
    }
    storageEngine_->IncreaseCacheRecordVersion();
}

uint64_t SQLiteSingleVerNaturalStore::GetCacheRecordVersion() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] Get cache version storage engine is invalid.");
        return 0;
    }
    return storageEngine_->GetCacheRecordVersion();
}

uint64_t SQLiteSingleVerNaturalStore::GetAndIncreaseCacheRecordVersion() const
{
    if (storageEngine_ == nullptr) {
        LOGE("[SingleVerNStore] Get cache version storage engine is invalid.");
        return 0;
    }
    return storageEngine_->GetAndIncreaseCacheRecordVersion();
}

void SQLiteSingleVerNaturalStore::CheckAmendValueContentForSyncProcedure(std::vector<DataItem> &dataItems) const
{
    const SchemaObject &schemaObjRef = MyProp().GetSchemaConstRef();
    if (!schemaObjRef.IsSchemaValid()) {
        // Not a schema database, do not need to check more
        return;
    }
    uint32_t deleteCount = 0;
    uint32_t amendCount = 0;
    uint32_t neglectCount = 0;
    for (auto &eachItem : dataItems) {
        if ((eachItem.flag & DataItem::DELETE_FLAG) == DataItem::DELETE_FLAG ||
            (eachItem.flag & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) == DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) {
            // Delete record not concerned
            deleteCount++;
            continue;
        }
        bool useAmendValue = false;
        int errCode = CheckValueAndAmendIfNeed(ValueSource::FROM_SYNC, eachItem.value, eachItem.value, useAmendValue);
        if (errCode != E_OK) {
            eachItem.neglect = true;
            neglectCount++;
            continue;
        }
        if (useAmendValue) {
            amendCount++;
        }
    }
    LOGI("[SqlSinStore][CheckAmendForSync] OriCount=%zu, DeleteCount=%u, AmendCount=%u, NeglectCount=%u",
        dataItems.size(), deleteCount, amendCount, neglectCount);
}

void SQLiteSingleVerNaturalStore::NotifyRemotePushFinished(const std::string &targetId) const
{
    std::string identifier = DBCommon::VectorToHexString(GetIdentifier());
    LOGI("label:%.6s sourceTarget: %s{private} push finished", identifier.c_str(), targetId.c_str());
    NotifyRemotePushFinishedInner(targetId);
}

int SQLiteSingleVerNaturalStore::CheckIntegrity() const
{
    int errCode = E_OK;
    auto handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->CheckIntegrity();
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::SaveCreateDBTimeIfNotExisted()
{
    Timestamp createDBTime = 0;
    int errCode = GetDatabaseCreateTimestamp(createDBTime);
    if (errCode == -E_NOT_FOUND) {
        errCode = SaveCreateDBTime();
    }
    if (errCode != E_OK) {
        LOGE("SaveCreateDBTimeIfNotExisted failed, errCode=%d.", errCode);
    }
    return errCode;
}

int SQLiteSingleVerNaturalStore::DeleteMetaDataByPrefixKey(const Key &keyPrefix) const
{
    if (keyPrefix.empty() || keyPrefix.size() > DBConstant::MAX_KEY_SIZE) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    auto handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->DeleteMetaDataByPrefixKey(keyPrefix);
    if (errCode != E_OK) {
        LOGE("[SinStore] DeleteMetaData by prefix key failed, errCode = %d", errCode);
    }

    ReleaseHandle(handle);
    HeartBeatForLifeCycle();
    return errCode;
}

int SQLiteSingleVerNaturalStore::GetCompressionOption(bool &needCompressOnSync, uint8_t &compressionRate) const
{
    needCompressOnSync = GetDbProperties().GetBoolProp(KvDBProperties::COMPRESS_ON_SYNC, false);
    compressionRate = GetDbProperties().GetIntProp(KvDBProperties::COMPRESSION_RATE,
        DBConstant::DEFAULT_COMPTRESS_RATE);
    return E_OK;
}

int SQLiteSingleVerNaturalStore::GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const
{
    algorithmSet.clear();
    DataCompression::GetCompressionAlgo(algorithmSet);
    return E_OK;
}

int SQLiteSingleVerNaturalStore::CheckAndInitQueryCondition(QueryObject &query) const
{
    const SchemaObject &localSchema = MyProp().GetSchemaConstRef();
    if (localSchema.GetSchemaType() != SchemaType::NONE && localSchema.GetSchemaType() != SchemaType::JSON) {
        // Flatbuffer schema is not support subscribe
        return -E_NOT_SUPPORT;
    }
    query.SetSchema(localSchema);

    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->CheckQueryObjectLegal(query);
    if (errCode != E_OK) {
        LOGE("Check query condition failed [%d]!", errCode);
    }
    ReleaseHandle(handle);
    return errCode;
}

void SQLiteSingleVerNaturalStore::SetDataInterceptor(const PushDataInterceptor &interceptor)
{
    std::unique_lock<std::shared_mutex> lock(dataInterceptorMutex_);
    dataInterceptor_ = interceptor;
}

int SQLiteSingleVerNaturalStore::InterceptData(std::vector<SingleVerKvEntry *> &entries, const std::string &sourceID,
    const std::string &targetID) const
{
    PushDataInterceptor interceptor = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(dataInterceptorMutex_);
        if (dataInterceptor_ == nullptr) {
            return E_OK;
        }
        interceptor = dataInterceptor_;
    }

    InterceptedDataImpl data(entries, [this](const Value &newValue) -> int {
            bool useAmendValue = false;
            Value amendValue = newValue;
            return this->CheckValueAndAmendIfNeed(ValueSource::FROM_LOCAL, newValue, amendValue, useAmendValue);
        }
    );

    int errCode = interceptor(data, sourceID, targetID);
    if (data.IsError()) {
        SingleVerKvEntry::Release(entries);
        LOGE("Intercept data failed:%d.", errCode);
        return -E_INTERCEPT_DATA_FAIL;
    }
    return E_OK;
}

int SQLiteSingleVerNaturalStore::AddSubscribe(const std::string &subscribeId, const QueryObject &query,
    bool needCacheSubscribe)
{
    if (IsSupportSubscribe() != E_OK) {
        return -E_NOT_SUPPORT;
    }
    const SchemaObject &localSchema = MyProp().GetSchemaConstRef();
    QueryObject queryInner = query;
    queryInner.SetSchema(localSchema);
    if (IsExtendedCacheDBMode() && needCacheSubscribe) { // cache auto subscribe when engine state is in CACHEDB mode
        LOGI("Cache subscribe query and return ok when in cacheDB.");
        storageEngine_->CacheSubscribe(subscribeId, queryInner);
        return E_OK;
    }

    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        return errCode;
    }

    errCode = handle->AddSubscribeTrigger(queryInner, subscribeId);
    if (errCode != E_OK) {
        LOGE("Add subscribe trigger failed: %d", errCode);
        (void)handle->Rollback();
    } else {
        errCode = handle->Commit();
    }
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::SetMaxLogSize(uint64_t limit)
{
    LOGI("Set the max log size to %" PRIu64, limit);
    maxLogSize_.store(limit);
    return E_OK;
}
uint64_t SQLiteSingleVerNaturalStore::GetMaxLogSize() const
{
    return maxLogSize_.load();
}

void SQLiteSingleVerNaturalStore::Dump(int fd)
{
    std::string userId = MyProp().GetStringProp(DBProperties::USER_ID, "");
    std::string appId = MyProp().GetStringProp(DBProperties::APP_ID, "");
    std::string storeId = MyProp().GetStringProp(DBProperties::STORE_ID, "");
    std::string label = MyProp().GetStringProp(DBProperties::IDENTIFIER_DATA, "");
    label = DBCommon::TransferStringToHex(label);
    DBDumpHelper::Dump(fd, "\tdb userId = %s, appId = %s, storeId = %s, label = %s\n",
        userId.c_str(), appId.c_str(), storeId.c_str(), label.c_str());
    SyncAbleKvDB::Dump(fd);
}

int SQLiteSingleVerNaturalStore::IsSupportSubscribe() const
{
    const SchemaObject &localSchema = MyProp().GetSchemaConstRef();
    if (localSchema.GetSchemaType() != SchemaType::NONE && localSchema.GetSchemaType() != SchemaType::JSON) {
        // Flatbuffer schema is not support subscribe
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

int SQLiteSingleVerNaturalStore::RemoveDeviceDataInner(const std::string &hashDev, bool isNeedNotify)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("[SingleVerNStore] RemoveDeviceData get handle failed:%d", errCode);
        return errCode;
    }
    uint64_t logFileSize = handle->GetLogFileSize();
    ReleaseHandle(handle);
    if (logFileSize > GetMaxLogSize()) {
        LOGW("[SingleVerNStore] RmDevData log size[%" PRIu64 "] over the limit", logFileSize);
        return -E_LOG_OVER_LIMITS;
    }

    std::set<std::string> removeDevices;
    if (hashDev.empty()) {
        errCode = GetExistsDeviceList(removeDevices);
        if (errCode != E_OK) {
            LOGE("[SingleVerNStore] get remove device list failed:%d", errCode);
            return errCode;
        }
    } else {
        removeDevices.insert(hashDev);
    }

    LOGD("[SingleVerNStore] remove device data, size=%zu", removeDevices.size());
    for (const auto &iterDevice : removeDevices) {
        // Call the syncer module to erase the water mark.
        errCode = EraseDeviceWaterMark(iterDevice, false);
        if (errCode != E_OK) {
            LOGE("[SingleVerNStore] erase water mark failed:%d", errCode);
            return errCode;
        }
    }

    if (IsExtendedCacheDBMode()) {
        errCode = RemoveDeviceDataInCacheMode(hashDev, isNeedNotify);
    } else {
        errCode = RemoveDeviceDataNormally(hashDev, isNeedNotify);
    }
    if (errCode != E_OK) {
        LOGE("[SingleVerNStore] RemoveDeviceData failed:%d", errCode);
    }

    return errCode;
}

int SQLiteSingleVerNaturalStore::RemoveDeviceDataInner(const std::string &hashDev, ClearMode mode)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("[SingleVerNStore] RemoveDeviceData get handle failed:%d", errCode);
        return errCode;
    }
    errCode = handle->RemoveDeviceData(hashDev, mode);
    ReleaseHandle(handle);
    return errCode;
}

int SQLiteSingleVerNaturalStore::RemoveDeviceDataInner(const std::string &hashDev, const std::string &user,
    ClearMode mode)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("[SingleVerNStore] RemoveDeviceData get handle failed:%d", errCode);
        return errCode;
    }
    errCode = handle->RemoveDeviceData(hashDev, user, mode);
    ReleaseHandle(handle);
    return errCode;
}

void SQLiteSingleVerNaturalStore::AbortHandle()
{
    std::unique_lock<std::shared_mutex> lock(abortHandleMutex_);
    abortPerm_ = OperatePerm::RESTART_SYNC_PERM;
}

void SQLiteSingleVerNaturalStore::EnableHandle()
{
    std::unique_lock<std::shared_mutex> lock(abortHandleMutex_);
    abortPerm_ = OperatePerm::NORMAL_PERM;
}

int SQLiteSingleVerNaturalStore::TryHandle() const
{
    std::unique_lock<std::shared_mutex> lock(abortHandleMutex_);
    if (abortPerm_ == OperatePerm::RESTART_SYNC_PERM) {
        LOGW("[SingleVerNStore] Restarting sync, handle id[%s] is busy",
            DBCommon::TransferStringToHex(storageEngine_->GetIdentifier()).c_str());
        return -E_BUSY;
    }
    return E_OK;
}

std::pair<int, SQLiteSingleVerStorageExecutor*> SQLiteSingleVerNaturalStore::GetStorageExecutor(bool isWrite)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageExecutor *handle = GetHandle(isWrite, errCode);
    return {errCode, handle};
}

void SQLiteSingleVerNaturalStore::RecycleStorageExecutor(SQLiteSingleVerStorageExecutor *executor)
{
    ReleaseHandle(executor);
}

TimeOffset SQLiteSingleVerNaturalStore::GetLocalTimeOffsetForCloud()
{
    return GetLocalTimeOffset();
}

int SQLiteSingleVerNaturalStore::RegisterObserverAction(const KvStoreObserver *observer, const ObserverAction &action)
{
    std::lock_guard<std::mutex> autoLock(cloudStoreMutex_);
    if (sqliteCloudKvStore_ == nullptr) {
        return -E_INTERNAL_ERROR;
    }
    sqliteCloudKvStore_->RegisterObserverAction(observer, action);
    return E_OK;
}

int SQLiteSingleVerNaturalStore::UnRegisterObserverAction(const KvStoreObserver *observer)
{
    std::lock_guard<std::mutex> autoLock(cloudStoreMutex_);
    if (sqliteCloudKvStore_ == nullptr) {
        return -E_INTERNAL_ERROR;
    }
    sqliteCloudKvStore_->UnRegisterObserverAction(observer);
    return E_OK;
}

int SQLiteSingleVerNaturalStore::GetCloudVersion(const std::string &device,
    std::map<std::string, std::string> &versionMap)
{
    std::lock_guard<std::mutex> autoLock(cloudStoreMutex_);
    if (sqliteCloudKvStore_ == nullptr) {
        return -E_INTERNAL_ERROR;
    }
    return sqliteCloudKvStore_->GetCloudVersion(device, versionMap);
}
}
