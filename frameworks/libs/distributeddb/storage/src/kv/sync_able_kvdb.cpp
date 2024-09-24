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

#include "sync_able_kvdb.h"

#include "db_dump_helper.h"
#include "db_errno.h"
#include "log_print.h"
#include "parcel.h"
#include "runtime_context.h"
#include "user_change_monitor.h"

namespace DistributedDB {
const EventType SyncAbleKvDB::REMOTE_PUSH_FINISHED = 1;

SyncAbleKvDB::SyncAbleKvDB()
    : started_(false),
      closed_(false),
      isSyncModuleActiveCheck_(false),
      isSyncNeedActive_(true),
      notifyChain_(nullptr),
      userChangeListener_(nullptr),
      cloudSyncer_(nullptr)
{}

SyncAbleKvDB::~SyncAbleKvDB()
{
    if (notifyChain_ != nullptr) {
        (void)notifyChain_->UnRegisterEventType(REMOTE_PUSH_FINISHED);
        KillAndDecObjRef(notifyChain_);
        notifyChain_ = nullptr;
    }
    if (userChangeListener_ != nullptr) {
        userChangeListener_->Drop(true);
        userChangeListener_ = nullptr;
    }
    std::lock_guard<std::mutex> autoLock(cloudSyncerLock_);
    KillAndDecObjRef(cloudSyncer_);
    cloudSyncer_ = nullptr;
}

void SyncAbleKvDB::DelConnection(GenericKvDBConnection *connection)
{
    auto realConnection = static_cast<SyncAbleKvDBConnection *>(connection);
    if (realConnection != nullptr) {
        KillAndDecObjRef(realConnection);
        realConnection = nullptr;
    }
}

void SyncAbleKvDB::TriggerSync(int notifyEvent)
{
    if (!started_) {
        StartSyncer();
    }
    if (started_) {
        syncer_.LocalDataChanged(notifyEvent);
    }
}

void SyncAbleKvDB::CommitNotify(int notifyEvent, KvDBCommitNotifyFilterAbleData *data)
{
    SyncAbleKvDB::TriggerSync(notifyEvent);

    GenericKvDB::CommitNotify(notifyEvent, data);
}

void SyncAbleKvDB::Close()
{
    StopSyncer(true);
}

// Start a sync action.
int SyncAbleKvDB::Sync(const ISyncer::SyncParma &parma, uint64_t connectionId)
{
    if (!started_) {
        int errCode = StartSyncer();
        if (!started_) {
            return errCode;
        }
    }
    return syncer_.Sync(parma, connectionId);
}

// Cancel a sync action.
int SyncAbleKvDB::CancelSync(uint32_t syncId)
{
    if (!started_) {
        return -E_NOT_INIT;
    }
    return syncer_.CancelSync(syncId);
}

void SyncAbleKvDB::EnableAutoSync(bool enable)
{
    if (NeedStartSyncer()) {
        StartSyncer();
    }
    return syncer_.EnableAutoSync(enable);
}

void SyncAbleKvDB::WakeUpSyncer()
{
    if (NeedStartSyncer()) {
        StartSyncer();
    }
}

// Stop a sync action in progress.
void SyncAbleKvDB::StopSync(uint64_t connectionId)
{
    if (started_) {
        syncer_.StopSync(connectionId);
    }
}

void SyncAbleKvDB::SetSyncModuleActive()
{
    if (isSyncModuleActiveCheck_) {
        return;
    }
    IKvDBSyncInterface *syncInterface = GetSyncInterface();
    if (syncInterface == nullptr) {
        LOGD("KvDB got null sync interface.");
        return;
    }
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    if (!isSyncDualTupleMode) {
        isSyncNeedActive_ = true;
        isSyncModuleActiveCheck_ = true;
        return;
    }
    isSyncNeedActive_ = RuntimeContext::GetInstance()->IsSyncerNeedActive(syncInterface->GetDbProperties());
    if (!isSyncNeedActive_) {
        LOGI("syncer no need to active");
    }
    isSyncModuleActiveCheck_ = true;
}

bool SyncAbleKvDB::GetSyncModuleActive()
{
    return isSyncNeedActive_;
}

void SyncAbleKvDB::ReSetSyncModuleActive()
{
    isSyncModuleActiveCheck_ = false;
    isSyncNeedActive_ = true;
}

// Start syncer
int SyncAbleKvDB::StartSyncer(bool isCheckSyncActive, bool isNeedActive)
{
    StartCloudSyncer();
    int errCode = E_OK;
    {
        std::unique_lock<std::mutex> lock(syncerOperateLock_);
        errCode = StartSyncerWithNoLock(isCheckSyncActive, isNeedActive);
        closed_ = false;
    }
    UserChangeHandle();
    return errCode;
}

int SyncAbleKvDB::StartSyncerWithNoLock(bool isCheckSyncActive, bool isNeedActive)
{
    IKvDBSyncInterface *syncInterface = GetSyncInterface();
    if (syncInterface == nullptr) {
        LOGD("KvDB got null sync interface.");
        return -E_INVALID_ARGS;
    }
    if (!isCheckSyncActive) {
        SetSyncModuleActive();
        isNeedActive = GetSyncModuleActive();
    }
    int errCode = syncer_.Initialize(syncInterface, isNeedActive);
    if (errCode == E_OK) {
        started_ = true;
    }
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    std::string label = syncInterface->GetDbProperties().GetStringProp(DBProperties::IDENTIFIER_DATA, "");
    if (isSyncDualTupleMode && isCheckSyncActive && !isNeedActive && (userChangeListener_ == nullptr)) {
        // active to non_active
        userChangeListener_ = RuntimeContext::GetInstance()->RegisterUserChangedListener(
            [this](void *) { ChangeUserListener(); }, UserChangeMonitor::USER_ACTIVE_TO_NON_ACTIVE_EVENT);
        LOGI("[KVDB] [StartSyncerWithNoLock] [%.3s] After RegisterUserChangedListener", label.c_str());
    } else if (isSyncDualTupleMode && (userChangeListener_ == nullptr)) {
        EventType event = isNeedActive ?
            UserChangeMonitor::USER_ACTIVE_EVENT : UserChangeMonitor::USER_NON_ACTIVE_EVENT;
        userChangeListener_ = RuntimeContext::GetInstance()->RegisterUserChangedListener(
            [this](void *) { UserChangeHandle(); }, event);
        LOGI("[KVDB] [StartSyncerWithNoLock] [%.3s] After RegisterUserChangedListener event=%d", label.c_str(), event);
    }
    return errCode;
}

// Stop syncer
void SyncAbleKvDB::StopSyncer(bool isClosedOperation, bool isStopTaskOnly)
{
    {
        std::unique_lock<std::mutex> lock(cloudSyncerLock_);
        if (cloudSyncer_ != nullptr) {
            if (isStopTaskOnly) {
                cloudSyncer_->StopAllTasks();
            } else if (isClosedOperation) {
                cloudSyncer_->Close();
                RefObject::KillAndDecObjRef(cloudSyncer_);
                cloudSyncer_ = nullptr;
            }
        }
    }
    NotificationChain::Listener *userChangeListener = nullptr;
    {
        std::unique_lock<std::mutex> lock(syncerOperateLock_);
        StopSyncerWithNoLock(isClosedOperation);
        userChangeListener = userChangeListener_;
        userChangeListener_ = nullptr;
    }
    if (userChangeListener != nullptr) {
        userChangeListener->Drop(true);
        userChangeListener = nullptr;
    }
}

void SyncAbleKvDB::StopSyncerWithNoLock(bool isClosedOperation)
{
    if (!isClosedOperation && userChangeListener_ != nullptr) {
        std::unique_lock<std::mutex> lock(cloudSyncerLock_);
        if (cloudSyncer_ != nullptr) {
            cloudSyncer_->StopAllTasks();
        }
    }
    ReSetSyncModuleActive();
    syncer_.Close(isClosedOperation);
    if (started_) {
        started_ = false;
    }
    closed_ = isClosedOperation;
    if (!isClosedOperation && userChangeListener_ != nullptr) {
        userChangeListener_->Drop(false);
        userChangeListener_ = nullptr;
    }
}

void SyncAbleKvDB::UserChangeHandle()
{
    bool isNeedChange;
    bool isNeedActive = true;
    IKvDBSyncInterface *syncInterface = GetSyncInterface();
    if (syncInterface == nullptr) {
        LOGD("KvDB got null sync interface.");
        return;
    }
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().
        GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE, false);
    if (!isSyncDualTupleMode) {
        LOGD("[SyncAbleKvDB] no use syncDualTupleMode, abort userChange");
        return;
    }
    std::unique_lock<std::mutex> lock(syncerOperateLock_);
    if (closed_) {
        LOGI("kvDB is already closed");
        return;
    }
    isNeedActive = RuntimeContext::GetInstance()->IsSyncerNeedActive(syncInterface->GetDbProperties());
    isNeedChange = (isNeedActive != isSyncNeedActive_);
    // non_active to active or active to non_active
    if (isNeedChange) {
        StopSyncerWithNoLock(); // will drop userChangeListener
        isSyncModuleActiveCheck_ = true;
        isSyncNeedActive_ = isNeedActive;
        StartSyncerWithNoLock(true, isNeedActive);
    }
}

void SyncAbleKvDB::ChangeUserListener()
{
    // only active to non_active call, put into USER_NON_ACTIVE_EVENT listener from USER_ACTIVE_TO_NON_ACTIVE_EVENT
    if (userChangeListener_ != nullptr) {
        userChangeListener_->Drop(false);
        userChangeListener_ = nullptr;
    }
    if (userChangeListener_ == nullptr) {
        userChangeListener_ = RuntimeContext::GetInstance()->RegisterUserChangedListener(
            [this](void *) { UserChangeHandle(); }, UserChangeMonitor::USER_NON_ACTIVE_EVENT);
        IKvDBSyncInterface *syncInterface = GetSyncInterface();
        std::string label = syncInterface->GetDbProperties().GetStringProp(DBProperties::IDENTIFIER_DATA, "");
        LOGI("[KVDB] [ChangeUserListener] [%.3s] After RegisterUserChangedListener", label.c_str());
    }
}

uint64_t SyncAbleKvDB::GetTimestampFromDB()
{
    return 0; //default is 0
}

// Get The current virtual timestamp
uint64_t SyncAbleKvDB::GetTimestamp(bool needStartSync)
{
    if (NeedStartSyncer()) {
        if (needStartSync) {
            StartSyncer();
        } else {
            // if syncer not start, get offset time from database
            return GetTimestampFromDB();
        }
    }
    return syncer_.GetTimestamp();
}

// Get the dataItem's append length
uint32_t SyncAbleKvDB::GetAppendedLen() const
{
    return Parcel::GetAppendedLen();
}

int SyncAbleKvDB::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash)
{
    if (NeedStartSyncer()) {
        int errCode = StartSyncer();
        if (errCode != E_OK && errCode != -E_NO_NEED_ACTIVE) {
            return errCode;
        }
    }
    return syncer_.EraseDeviceWaterMark(deviceId, isNeedHash);
}

int SyncAbleKvDB::GetQueuedSyncSize(int *queuedSyncSize) const
{
    return syncer_.GetQueuedSyncSize(queuedSyncSize);
}

int SyncAbleKvDB::SetQueuedSyncLimit(const int *queuedSyncLimit)
{
    return syncer_.SetQueuedSyncLimit(queuedSyncLimit);
}

int SyncAbleKvDB::GetQueuedSyncLimit(int *queuedSyncLimit) const
{
    return syncer_.GetQueuedSyncLimit(queuedSyncLimit);
}

int SyncAbleKvDB::DisableManualSync(void)
{
    return syncer_.DisableManualSync();
}

int SyncAbleKvDB::EnableManualSync(void)
{
    return syncer_.EnableManualSync();
}

int SyncAbleKvDB::GetLocalIdentity(std::string &outTarget) const
{
    return syncer_.GetLocalIdentity(outTarget);
}

int SyncAbleKvDB::SetStaleDataWipePolicy(WipePolicy policy)
{
    return syncer_.SetStaleDataWipePolicy(policy);
}

int SyncAbleKvDB::RegisterEventType(EventType type)
{
    if (notifyChain_ == nullptr) {
        notifyChain_ = new (std::nothrow) NotificationChain;
        if (notifyChain_ == nullptr) {
            return -E_OUT_OF_MEMORY;
        }
    }

    int errCode = notifyChain_->RegisterEventType(type);
    if (errCode == -E_ALREADY_REGISTER) {
        return E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[SyncAbleKvDB] Register event type %u failed! err %d", type, errCode);
        KillAndDecObjRef(notifyChain_);
        notifyChain_ = nullptr;
    }
    return errCode;
}

NotificationChain::Listener *SyncAbleKvDB::AddRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier,
    int &errCode)
{
    std::unique_lock<std::shared_mutex> lock(notifyChainLock_);
    errCode = RegisterEventType(REMOTE_PUSH_FINISHED);
    if (errCode != E_OK) {
        return nullptr;
    }

    auto listener = notifyChain_->RegisterListener(REMOTE_PUSH_FINISHED,
        [notifier](void *arg) {
            if (arg == nullptr) {
                LOGE("PragmaRemotePushNotify is null.");
                return;
            }
            notifier(*static_cast<RemotePushNotifyInfo *>(arg));
        }, nullptr, errCode);
    if (errCode != E_OK) {
        LOGE("[SyncAbleKvDB] Add remote push finished notifier failed! err %d", errCode);
    }
    return listener;
}

void SyncAbleKvDB::NotifyRemotePushFinishedInner(const std::string &targetId) const
{
    NotificationChain *notify = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock(notifyChainLock_);
        if (notifyChain_ == nullptr) {
            return;
        }
        notify = notifyChain_;
        RefObject::IncObjRef(notify);
    }
    RemotePushNotifyInfo info;
    info.deviceId = targetId;
    notify->NotifyEvent(REMOTE_PUSH_FINISHED, static_cast<void *>(&info));
    RefObject::DecObjRef(notify);
}

int SyncAbleKvDB::SetSyncRetry(bool isRetry)
{
    IKvDBSyncInterface *syncInterface = GetSyncInterface();
    if (syncInterface == nullptr) {
        LOGD("KvDB got null sync interface.");
        return -E_INVALID_DB;
    }
    bool localOnly = syncInterface->GetDbProperties().GetBoolProp(KvDBProperties::LOCAL_ONLY, false);
    if (localOnly) {
        return -E_NOT_SUPPORT;
    }
    if (NeedStartSyncer()) {
        StartSyncer();
    }
    return syncer_.SetSyncRetry(isRetry);
}

int SyncAbleKvDB::SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets)
{
    if (NeedStartSyncer()) {
        StartSyncer();
    }
    return syncer_.SetEqualIdentifier(identifier, targets);
}

void SyncAbleKvDB::Dump(int fd)
{
    SyncerBasicInfo basicInfo = syncer_.DumpSyncerBasicInfo();
    DBDumpHelper::Dump(fd, "\tisSyncActive = %d, isAutoSync = %d\n\n", basicInfo.isSyncActive,
        basicInfo.isAutoSync);
    if (basicInfo.isSyncActive) {
        DBDumpHelper::Dump(fd, "\tDistributedDB Database Sync Module Message Info:\n");
        syncer_.Dump(fd);
    }
}

int SyncAbleKvDB::GetSyncDataSize(const std::string &device, size_t &size) const
{
    return syncer_.GetSyncDataSize(device, size);
}

bool SyncAbleKvDB::NeedStartSyncer() const
{
    if (!RuntimeContext::GetInstance()->IsCommunicatorAggregatorValid()) {
        return false;
    }
    // don't start when check callback got not active
    // equivalent to !(!isSyncNeedActive_ && isSyncModuleActiveCheck_)
    return !started_ && (isSyncNeedActive_ || !isSyncModuleActiveCheck_);
}

int SyncAbleKvDB::GetHashDeviceId(const std::string &clientId, std::string &hashDevId)
{
    if (!NeedStartSyncer()) {
        return syncer_.GetHashDeviceId(clientId, hashDevId);
    }
    int errCode = StartSyncer();
    if (errCode != E_OK && errCode != -E_NO_NEED_ACTIVE) {
        return errCode;
    }
    return syncer_.GetHashDeviceId(clientId, hashDevId);
}

int SyncAbleKvDB::GetWatermarkInfo(const std::string &device, WatermarkInfo &info)
{
    if (NeedStartSyncer()) {
        StartSyncer();
    }
    return syncer_.GetWatermarkInfo(device, info);
}

int SyncAbleKvDB::UpgradeSchemaVerInMeta()
{
    return syncer_.UpgradeSchemaVerInMeta();
}

void SyncAbleKvDB::ResetSyncStatus()
{
    syncer_.ResetSyncStatus();
}

ICloudSyncStorageInterface *SyncAbleKvDB::GetICloudSyncInterface() const
{
    return nullptr;
}

void SyncAbleKvDB::StartCloudSyncer()
{
    auto cloudStorage = GetICloudSyncInterface();
    if (cloudStorage == nullptr) {
        return;
    }
    int conflictType = MyProp().GetIntProp(KvDBProperties::CONFLICT_RESOLVE_POLICY,
        static_cast<int>(SingleVerConflictResolvePolicy::DEFAULT_LAST_WIN));
    {
        std::lock_guard<std::mutex> autoLock(cloudSyncerLock_);
        if (cloudSyncer_ != nullptr) {
            return;
        }
        cloudSyncer_ = new(std::nothrow) CloudSyncer(StorageProxy::GetCloudDb(cloudStorage),
            static_cast<SingleVerConflictResolvePolicy>(conflictType));
        if (cloudSyncer_ == nullptr) {
            LOGW("[SyncAbleKvDB][StartCloudSyncer] start cloud syncer and cloud syncer was not initialized");
        }
    }
}

TimeOffset SyncAbleKvDB::GetLocalTimeOffset()
{
    if (NeedStartSyncer()) {
        StartSyncer();
    }
    return syncer_.GetLocalTimeOffset();
}

void SyncAbleKvDB::FillSyncInfo(const CloudSyncOption &option, const SyncProcessCallback &onProcess,
    CloudSyncer::CloudTaskInfo &info)
{
    QuerySyncObject query(option.query);
    query.SetTableName(CloudDbConstant::CLOUD_KV_TABLE_NAME);
    info.queryList.push_back(query);
    info.table.push_back(CloudDbConstant::CLOUD_KV_TABLE_NAME);
    info.callback = onProcess;
    info.devices = option.devices;
    info.mode = option.mode;
    std::set<std::string> userSet(option.users.begin(), option.users.end());
    info.users = std::vector<std::string>(userSet.begin(), userSet.end());
    info.lockAction = option.lockAction;
    info.storeId = MyProp().GetStringProp(DBProperties::STORE_ID, "");
    info.merge = option.merge;
    info.prepareTraceId = option.prepareTraceId;
}

int SyncAbleKvDB::CheckSyncOption(const CloudSyncOption &option, const CloudSyncer &syncer)
{
    if (option.users.empty()) {
        LOGE("[SyncAbleKvDB][Sync] no user in sync option");
        return -E_INVALID_ARGS;
    }
    const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs = syncer.GetCloudDB();
    if (cloudDBs.empty()) {
        LOGE("[SyncAbleKvDB][Sync] not set cloud db");
        return -E_CLOUD_ERROR;
    }
    auto schemas = GetDataBaseSchemas();
    if (schemas.empty()) {
        LOGE("[SyncAbleKvDB][Sync] not set cloud schema");
        return -E_SCHEMA_MISMATCH;
    }
    for (const auto &user : option.users) {
        if (cloudDBs.find(user) == cloudDBs.end()) {
            LOGE("[SyncAbleKvDB][Sync] cloud db with invalid user: %s", user.c_str());
            return -E_INVALID_ARGS;
        }
        if (schemas.find(user) == schemas.end()) {
            LOGE("[SyncAbleKvDB][Sync] cloud schema with invalid user: %s", user.c_str());
            return -E_SCHEMA_MISMATCH;
        }
    }
    if (option.waitTime > DBConstant::MAX_SYNC_TIMEOUT || option.waitTime < DBConstant::INFINITE_WAIT) {
        LOGE("[SyncAbleKvDB][Sync] invalid wait time of sync option: %lld", option.waitTime);
        return -E_INVALID_ARGS;
    }
    if (!CheckSchemaSupportForCloudSync()) {
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

int SyncAbleKvDB::Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess)
{
    auto syncer = GetAndIncCloudSyncer();
    if (syncer == nullptr) {
        LOGE("[SyncAbleKvDB][Sync] cloud syncer was not initialized");
        return -E_INVALID_DB;
    }
    int errCode = CheckSyncOption(option, *syncer);
    if (errCode != E_OK) {
        RefObject::DecObjRef(syncer);
        return errCode;
    }
    CloudSyncer::CloudTaskInfo info;
    FillSyncInfo(option, onProcess, info);
    errCode = syncer->Sync(info);
    RefObject::DecObjRef(syncer);
    return errCode;
}

int SyncAbleKvDB::SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs)
{
    auto syncer = GetAndIncCloudSyncer();
    if (syncer == nullptr) {
        LOGE("[SyncAbleKvDB][SetCloudDB] cloud syncer was not initialized");
        return -E_INVALID_DB;
    }
    int errCode = syncer->SetCloudDB(cloudDBs);
    RefObject::DecObjRef(syncer);
    return errCode;
}

int SyncAbleKvDB::CleanAllWaterMark()
{
    auto syncer = GetAndIncCloudSyncer();
    if (syncer == nullptr) {
        LOGE("[SyncAbleKvDB][CleanAllWaterMark] cloud syncer was not initialized");
        return -E_INVALID_DB;
    }
    syncer->CleanAllWaterMark();
    RefObject::DecObjRef(syncer);
    return E_OK;
}

int32_t SyncAbleKvDB::GetTaskCount()
{
    int32_t taskCount = 0;
    auto cloudSyncer = GetAndIncCloudSyncer();
    if (cloudSyncer != nullptr) {
        taskCount += cloudSyncer->GetCloudSyncTaskCount();
        RefObject::DecObjRef(cloudSyncer);
    }
    if (NeedStartSyncer()) {
        return taskCount;
    }
    taskCount += syncer_.GetTaskCount();
    return taskCount;
}

CloudSyncer *SyncAbleKvDB::GetAndIncCloudSyncer()
{
    std::lock_guard<std::mutex> autoLock(cloudSyncerLock_);
    if (cloudSyncer_ == nullptr) {
        return nullptr;
    }
    RefObject::IncObjRef(cloudSyncer_);
    return cloudSyncer_;
}

void SyncAbleKvDB::SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback)
{
    auto cloudSyncer = GetAndIncCloudSyncer();
    if (cloudSyncer == nullptr) {
        LOGE("[SyncAbleKvDB][SetGenCloudVersionCallback] cloud syncer was not initialized");
        return;
    }
    cloudSyncer->SetGenCloudVersionCallback(callback);
    RefObject::DecObjRef(cloudSyncer);
}

std::map<std::string, DataBaseSchema> SyncAbleKvDB::GetDataBaseSchemas()
{
    return {};
}

bool SyncAbleKvDB::CheckSchemaSupportForCloudSync() const
{
    return true; // default is valid
}
}
