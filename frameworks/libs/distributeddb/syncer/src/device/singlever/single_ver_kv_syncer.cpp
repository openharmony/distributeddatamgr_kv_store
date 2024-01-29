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

#include "single_ver_kv_syncer.h"

#include <functional>
#include <map>
#include <mutex>

#include "db_common.h"
#include "ikvdb_sync_interface.h"
#include "log_print.h"
#include "meta_data.h"
#include "single_ver_sync_engine.h"
#include "sqlite_single_ver_natural_store.h"

namespace DistributedDB {
SingleVerKVSyncer::SingleVerKVSyncer()
    : autoSyncEnable_(false), triggerSyncTask_(true)
{
}

SingleVerKVSyncer::~SingleVerKVSyncer()
{
}

void SingleVerKVSyncer::EnableAutoSync(bool enable)
{
    LOGI("[Syncer] EnableAutoSync enable = %d, Label=%s", enable, label_.c_str());
    if (autoSyncEnable_ == enable) {
        return;
    }

    autoSyncEnable_ = enable;
    if (!enable) {
        return;
    }

    if (!initialized_) {
        LOGI("[Syncer] Syncer has not Init");
        return;
    }

    std::vector<std::string> devices;
    GetOnlineDevices(devices);
    if (devices.empty()) {
        LOGI("[Syncer] EnableAutoSync no online devices");
        return;
    }
    int errCode = Sync(devices, SyncModeType::AUTO_PUSH, nullptr, nullptr, false);
    if (errCode != E_OK) {
        LOGE("[Syncer] sync start by EnableAutoSync failed err %d", errCode);
    }
}

// Local data changed callback
void SingleVerKVSyncer::LocalDataChanged(int notifyEvent)
{
    if (!initialized_) {
        LOGE("[Syncer] Syncer has not Init");
        return;
    }

    if (notifyEvent != static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_FINISH_MIGRATE_EVENT) &&
        notifyEvent != static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT)) {
        LOGD("[Syncer] ignore event:%d", notifyEvent);
        return;
    }
    if (!triggerSyncTask_) {
        LOGI("[Syncer] some sync task is scheduling");
        return;
    }
    triggerSyncTask_ = false;
    std::vector<std::string> devices;
    GetOnlineDevices(devices);
    if (devices.empty()) {
        LOGI("[Syncer] LocalDataChanged no online standard devices, Label=%s", label_.c_str());
        triggerSyncTask_ = true;
        return;
    }
    ISyncEngine *engine = syncEngine_;
    ISyncInterface *storage = syncInterface_;
    RefObject::IncObjRef(engine);
    storage->IncRefCount();
    // To avoid many task were produced and waiting in the queue. For example, put value in a loop.
    // It will consume thread pool resources, so other task will delay until these task finish.
    // In extreme situation, 10 thread run the localDataChanged task and 1 task waiting in queue.
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, devices, engine, storage] {
        triggerSyncTask_ = true;
        if (!TryFullSync(devices)) {
            TriggerSubQuerySync(devices);
        }
        RefObject::DecObjRef(engine);
        storage->DecRefCount();
    });
    // if task schedule failed, but triggerSyncTask_ is not set to true, other thread may skip the schedule time
    // when task schedule failed, it means unormal status, it is unable to schedule next time probably
    // so it is ok if other thread skip the schedule if last task schedule failed
    if (errCode != E_OK) {
        triggerSyncTask_ = true;
        LOGE("[TriggerSync] LocalDataChanged retCode:%d", errCode);
        RefObject::DecObjRef(engine);
        storage->DecRefCount();
    }
}

// remote device online callback
void SingleVerKVSyncer::RemoteDataChanged(const std::string &device)
{
    LOGI("[SingleVerKVSyncer] device online dev %s", STR_MASK(device));
    if (!initialized_) {
        LOGE("[Syncer] Syncer has not Init");
        return;
    }
    std::string userId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::USER_ID, "");
    std::string appId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::APP_ID, "");
    std::string storeId = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::STORE_ID, "");
    RuntimeContext::GetInstance()->NotifyDatabaseStatusChange(userId, appId, storeId, device, true);
    SingleVerSyncer::RemoteDataChanged(device);
    if (autoSyncEnable_) {
        RefObject::IncObjRef(syncEngine_);
        syncInterface_->IncRefCount();
        int retCode = RuntimeContext::GetInstance()->ScheduleTask([this, userId, appId, storeId, device] {
            std::vector<std::string> devices;
            devices.push_back(device);
            int errCode = E_OK;
            if (RuntimeContext::GetInstance()->IsNeedAutoSync(userId, appId, storeId, device)) {
                errCode = Sync(devices, SyncModeType::AUTO_PUSH, nullptr, nullptr, false);
            }
            if (errCode != E_OK) {
                LOGE("[SingleVerKVSyncer] sync start by RemoteDataChanged failed err %d", errCode);
            }
            RefObject::DecObjRef(syncEngine_);
            syncInterface_->DecRefCount();
        });
        if (retCode != E_OK) {
            LOGE("[AutoLaunch] RemoteDataChanged triggler sync retCode:%d", retCode);
            RefObject::DecObjRef(syncEngine_);
            syncInterface_->DecRefCount();
        }
    }
    // db online again ,trigger subscribe
    // if remote device online, subscribequery num is 0
    std::vector<QuerySyncObject> syncQueries;
    static_cast<SingleVerSyncEngine *>(syncEngine_)->GetLocalSubscribeQueries(device, syncQueries);
    if (syncQueries.empty()) {
        LOGI("no need to trigger auto subscribe");
        return;
    }
    LOGI("[SingleVerKVSyncer] trigger local subscribe sync, queryNums=%zu", syncQueries.size());
    for (const auto &query : syncQueries) {
        TriggerSubscribe(device, query);
    }
    static_cast<SingleVerSyncEngine *>(syncEngine_)->PutUnfinishedSubQueries(device, syncQueries);
}

int SingleVerKVSyncer::SyncConditionCheck(const SyncParma &param, const ISyncEngine *engine,
    ISyncInterface *storage) const
{
    if (!param.isQuerySync) {
        return E_OK;
    }
    QuerySyncObject query = param.syncQuery;
    int errCode = static_cast<SingleVerKvDBSyncInterface *>(storage)->CheckAndInitQueryCondition(query);
    if (errCode != E_OK) {
        LOGE("[SingleVerKVSyncer] QuerySyncObject check failed");
        return errCode;
    }
    if (param.mode != SUBSCRIBE_QUERY) {
        return E_OK;
    }
    if (query.HasLimit() || query.HasOrderBy()) {
        LOGE("[SingleVerKVSyncer] subscribe query not support limit,offset or orderby");
        return -E_NOT_SUPPORT;
    }
    if (param.devices.size() > MAX_DEVICES_NUM) {
        LOGE("[SingleVerKVSyncer] devices is overlimit");
        return -E_MAX_LIMITS;
    }
    return engine->SubscribeLimitCheck(param.devices, query);
}

void SingleVerKVSyncer::TriggerSubscribe(const std::string &device, const QuerySyncObject &query)
{
    if (!initialized_) {
        LOGE("[Syncer] Syncer has not Init");
        return;
    }
    RefObject::IncObjRef(syncEngine_);
    int retCode = RuntimeContext::GetInstance()->ScheduleTask([this, device, query] {
        std::vector<std::string> devices;
        devices.push_back(device);
        SyncParma param;
        param.devices = devices;
        param.mode = SyncModeType::AUTO_SUBSCRIBE_QUERY;
        param.onComplete = nullptr;
        param.onFinalize = nullptr;
        param.wait = false;
        param.isQuerySync = true;
        param.syncQuery = query;
        int errCode = Sync(param);
        if (errCode != E_OK) {
            LOGE("[SingleVerKVSyncer] subscribe start by RemoteDataChanged failed err %d", errCode);
        }
        RefObject::DecObjRef(syncEngine_);
    });
    if (retCode != E_OK) {
        LOGE("[Syncer] triggler query subscribe start failed err %d", retCode);
        RefObject::DecObjRef(syncEngine_);
    }
}

bool SingleVerKVSyncer::TryFullSync(const std::vector<std::string> &devices)
{
    if (!initialized_) {
        LOGE("[Syncer] Syncer has not Init");
        return true;
    }
    if (!autoSyncEnable_) {
        LOGD("[Syncer] autoSync no enable");
        return false;
    }
    int errCode = Sync(devices, SyncModeType::AUTO_PUSH, nullptr, nullptr, false);
    if (errCode != E_OK) {
        LOGE("[Syncer] sync start by RemoteDataChanged failed err %d", errCode);
        return false;
    }
    return true;
}

void SingleVerKVSyncer::TriggerSubQuerySync(const std::vector<std::string> &devices)
{
    if (!initialized_) {
        LOGE("[Syncer] Syncer has not Init");
        return;
    }
    std::shared_ptr<Metadata> metadata = nullptr;
    ISyncInterface *syncInterface = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (metadata_ == nullptr || syncInterface_ == nullptr) {
            return;
        }
        metadata = metadata_;
        syncInterface = syncInterface_;
        syncInterface->IncRefCount();
    }
    int errCode;
    for (auto &device : devices) {
        std::vector<QuerySyncObject> queries;
        static_cast<SingleVerSyncEngine *>(syncEngine_)->GetRemoteSubscribeQueries(device, queries);
        for (auto &query : queries) {
            std::string queryId = query.GetIdentify();
            WaterMark queryWaterMark = 0;
            uint64_t lastTimestamp = metadata->GetQueryLastTimestamp(device, queryId);
            errCode = metadata->GetSendQueryWaterMark(queryId, device, queryWaterMark, false);
            if (errCode != E_OK) {
                LOGE("[Syncer] get queryId=%s,dev=%s watermark failed", STR_MASK(queryId), STR_MASK(device));
                continue;
            }
            if (lastTimestamp < queryWaterMark || lastTimestamp == 0) {
                continue;
            }
            LOGD("[Syncer] lastTime=%" PRIu64 " vs WaterMark=%" PRIu64 ",trigger queryId=%s,dev=%s", lastTimestamp,
                queryWaterMark, STR_MASK(queryId), STR_MASK(device));
            InternalSyncParma param;
            std::vector<std::string> targetDevices;
            targetDevices.push_back(device);
            param.devices = targetDevices;
            param.mode = SyncModeType::AUTO_PUSH;
            param.isQuerySync = true;
            param.syncQuery = query;
            QueryAutoSync(param);
        }
    }
    syncInterface->DecRefCount();
}

SyncerBasicInfo SingleVerKVSyncer::DumpSyncerBasicInfo()
{
    SyncerBasicInfo basicInfo = GenericSyncer::DumpSyncerBasicInfo();
    basicInfo.isAutoSync = autoSyncEnable_;
    return basicInfo;
}

int SingleVerKVSyncer::InitSyncEngine(DistributedDB::ISyncInterface *syncInterface)
{
    int errCode = GenericSyncer::InitSyncEngine(syncInterface);
    if (errCode != E_OK) {
        return errCode;
    }
    TriggerAddSubscribeAsync(syncInterface);
    return E_OK;
}

void SingleVerKVSyncer::TriggerAddSubscribeAsync(ISyncInterface *syncInterface)
{
    if (syncInterface == nullptr || syncEngine_ == nullptr) {
        return;
    }
    if (syncInterface->GetInterfaceType() != ISyncInterface::SYNC_SVD) {
        return;
    }
    DBInfo dbInfo;
    auto storage = static_cast<SyncGenericInterface *>(syncInterface);
    storage->GetDBInfo(dbInfo);
    std::map<std::string, std::vector<QuerySyncObject>> subscribeQuery;
    RuntimeContext::GetInstance()->GetSubscribeQuery(dbInfo, subscribeQuery);
    if (subscribeQuery.empty()) {
        LOGD("[SingleVerKVSyncer][TriggerAddSubscribeAsync] Subscribe cache is empty");
        return;
    }
    storage->IncRefCount();
    ISyncEngine *engine = syncEngine_;
    RefObject::IncObjRef(engine);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, engine, storage, subscribeQuery]() {
        engine->AddSubscribe(storage, subscribeQuery);
        // try to trigger query sync after add trigger
        LocalDataChanged(static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
        RefObject::DecObjRef(engine);
        storage->DecRefCount();
    });
    if (errCode != E_OK) {
        LOGW("[SingleVerKVSyncer] TriggerAddSubscribeAsync failed errCode = %d", errCode);
        syncInterface->DecRefCount();
        RefObject::DecObjRef(engine);
    }
}
} // namespace DistributedDB
