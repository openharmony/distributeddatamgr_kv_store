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
#include "sync_able_engine.h"

#include "db_dump_helper.h"
#include "db_errno.h"
#include "log_print.h"
#include "parcel.h"
#include "ref_object.h"
#include "relational_sync_able_storage.h"
#include "runtime_context.h"
#include "user_change_monitor.h"

namespace DistributedDB {
SyncAbleEngine::SyncAbleEngine(ISyncInterface *store)
    : syncer_(),
      started_(false),
      closed_(false),
      isSyncModuleActiveCheck_(false),
      isSyncNeedActive_(true),
      store_(store),
      userChangeListener_(nullptr)
{}

SyncAbleEngine::~SyncAbleEngine()
{
    if (userChangeListener_ != nullptr) {
        userChangeListener_->Drop(true);
        userChangeListener_ = nullptr;
    }
}

// Start a sync action.
int SyncAbleEngine::Sync(const ISyncer::SyncParma &parm, uint64_t connectionId)
{
    if (!started_) {
        int errCode = StartSyncer();
        if (!started_) {
            return errCode;
        }
    }
    return syncer_.Sync(parm, connectionId);
}

void SyncAbleEngine::WakeUpSyncer()
{
    StartSyncer();
}

void SyncAbleEngine::Close()
{
    StopSyncer();
}

// Get The current virtual timestamp
uint64_t SyncAbleEngine::GetTimestamp()
{
    if (NeedStartSyncer()) {
        StartSyncer();
    }
    return syncer_.GetTimestamp();
}

int SyncAbleEngine::EraseDeviceWaterMark(const std::string &deviceId, bool isNeedHash, const std::string &tableName)
{
    if (NeedStartSyncer()) {
        int errCode = StartSyncer();
        if (errCode != E_OK && errCode != -E_NO_NEED_ACTIVE) {
            return errCode;
        }
    }
    return syncer_.EraseDeviceWaterMark(deviceId, isNeedHash, tableName);
}

// Start syncer
int SyncAbleEngine::StartSyncer(bool isCheckSyncActive, bool isNeedActive)
{
    int errCode = E_OK;
    {
        std::unique_lock<std::mutex> lock(syncerOperateLock_);
        errCode = StartSyncerWithNoLock(isCheckSyncActive, isNeedActive);
        closed_ = false;
    }
    UserChangeHandle();
    return errCode;
}

int SyncAbleEngine::StartSyncerWithNoLock(bool isCheckSyncActive, bool isNeedActive)
{
    if (store_ == nullptr) {
        LOGF("RDB got null sync interface.");
        return -E_INVALID_ARGS;
    }
    if (!isCheckSyncActive) {
        SetSyncModuleActive();
        isNeedActive = GetSyncModuleActive();
    }

    int errCode = syncer_.Initialize(store_, isNeedActive);
    if (errCode == E_OK) {
        started_ = true;
    } else {
        LOGE("RDB start syncer failed, err:'%d'.", errCode);
    }

    bool isSyncDualTupleMode = store_->GetDbProperties().GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false);
    std::string label = store_->GetDbProperties().GetStringProp(DBProperties::IDENTIFIER_DATA, "");
    if (isSyncDualTupleMode && isCheckSyncActive && !isNeedActive && (userChangeListener_ == nullptr)) {
        // active to non_active
        userChangeListener_ = RuntimeContext::GetInstance()->RegisterUserChangedListener(
            std::bind(&SyncAbleEngine::ChangeUserListener, this), UserChangeMonitor::USER_ACTIVE_TO_NON_ACTIVE_EVENT);
        LOGI("[StartSyncerWithNoLock] [%.3s] After RegisterUserChangedListener", label.c_str());
    } else if (isSyncDualTupleMode && (userChangeListener_ == nullptr)) {
        EventType event = isNeedActive ?
            UserChangeMonitor::USER_ACTIVE_EVENT : UserChangeMonitor::USER_NON_ACTIVE_EVENT;
        userChangeListener_ = RuntimeContext::GetInstance()->RegisterUserChangedListener(
            std::bind(&SyncAbleEngine::UserChangeHandle, this), event);
        LOGI("[StartSyncerWithNoLock] [%.3s] After RegisterUserChangedListener event=%d", label.c_str(), event);
    }
    return errCode;
}

// Stop syncer
void SyncAbleEngine::StopSyncer()
{
    NotificationChain::Listener *userChangeListener = nullptr;
    {
        std::unique_lock<std::mutex> lock(syncerOperateLock_);
        StopSyncerWithNoLock(true);
        userChangeListener = userChangeListener_;
        userChangeListener_ = nullptr;
    }
    if (userChangeListener != nullptr) {
        userChangeListener->Drop(true);
        userChangeListener = nullptr;
    }
}

void SyncAbleEngine::StopSyncerWithNoLock(bool isClosedOperation)
{
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

void SyncAbleEngine::UserChangeHandle()
{
    if (store_ == nullptr) {
        LOGD("[SyncAbleEngine] RDB got null sync interface in userChange.");
        return;
    }
    bool isSyncDualTupleMode = store_->GetDbProperties().GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false);
    if (!isSyncDualTupleMode) {
        LOGD("[SyncAbleEngine] no use syncDualTupleMode, abort userChange");
        return;
    }
    std::unique_lock<std::mutex> lock(syncerOperateLock_);
    if (closed_) {
        LOGI("RDB is already closed");
        return;
    }
    bool isNeedActive = RuntimeContext::GetInstance()->IsSyncerNeedActive(store_->GetDbProperties());
    bool isNeedChange = (isNeedActive != isSyncNeedActive_);
    // non_active to active or active to non_active
    if (isNeedChange) {
        StopSyncerWithNoLock(); // will drop userChangeListener
        isSyncModuleActiveCheck_ = true;
        isSyncNeedActive_ = isNeedActive;
        StartSyncerWithNoLock(true, isNeedActive);
    }
}

void SyncAbleEngine::ChangeUserListener()
{
    // only active to non_active call, put into USER_NON_ACTIVE_EVENT listener from USER_ACTIVE_TO_NON_ACTIVE_EVENT
    if (userChangeListener_ != nullptr) {
        userChangeListener_->Drop(false);
        userChangeListener_ = nullptr;
    }
    userChangeListener_ = RuntimeContext::GetInstance()->RegisterUserChangedListener(
        std::bind(&SyncAbleEngine::UserChangeHandle, this), UserChangeMonitor::USER_NON_ACTIVE_EVENT);
    std::string label = store_->GetDbProperties().GetStringProp(DBProperties::IDENTIFIER_DATA, "");
    LOGI("[ChangeUserListener] [%.3s] After RegisterUserChangedListener", label.c_str());
}

void SyncAbleEngine::SetSyncModuleActive()
{
    if (isSyncModuleActiveCheck_) {
        return;
    }

    bool isSyncDualTupleMode = store_->GetDbProperties().GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false);
    if (!isSyncDualTupleMode) {
        isSyncNeedActive_ = true;
        isSyncModuleActiveCheck_ = true;
        return;
    }
    isSyncNeedActive_ = RuntimeContext::GetInstance()->IsSyncerNeedActive(store_->GetDbProperties());
    if (!isSyncNeedActive_) {
        LOGI("syncer no need to active");
    }
    isSyncModuleActiveCheck_ = true;
}

bool SyncAbleEngine::GetSyncModuleActive()
{
    return isSyncNeedActive_;
}

void SyncAbleEngine::ReSetSyncModuleActive()
{
    isSyncModuleActiveCheck_ = false;
    isSyncNeedActive_ = true;
}

int SyncAbleEngine::GetLocalIdentity(std::string &outTarget)
{
    if (!started_) {
        StartSyncer();
    }
    return syncer_.GetLocalIdentity(outTarget);
}

void SyncAbleEngine::StopSync(uint64_t connectionId)
{
    if (started_) {
        syncer_.StopSync(connectionId);
    }
}

void SyncAbleEngine::Dump(int fd)
{
    SyncerBasicInfo basicInfo = syncer_.DumpSyncerBasicInfo();
    DBDumpHelper::Dump(fd, "\tisSyncActive = %d, isAutoSync = %d\n\n", basicInfo.isSyncActive,
        basicInfo.isAutoSync);
    if (basicInfo.isSyncActive) {
        DBDumpHelper::Dump(fd, "\tDistributedDB Database Sync Module Message Info:\n");
        syncer_.Dump(fd);
    }
}

int SyncAbleEngine::RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
    uint64_t connectionId, std::shared_ptr<ResultSet> &result)
{
    if (!started_) {
        int errCode = StartSyncer();
        if (!started_) {
            return errCode;
        }
    }
    return syncer_.RemoteQuery(device, condition, timeout, connectionId, result);
}

bool SyncAbleEngine::NeedStartSyncer() const
{
    if (!RuntimeContext::GetInstance()->IsCommunicatorAggregatorValid()) {
        LOGW("Engine communicator not ready!");
        return false;
    }
    // don't start when check callback got not active
    // equivalent to !(!isSyncNeedActive_ && isSyncModuleActiveCheck_)
    return !started_ && (isSyncNeedActive_ || !isSyncModuleActiveCheck_);
}

int SyncAbleEngine::GetHashDeviceId(const std::string &clientId, std::string &hashDevId)
{
    if (NeedStartSyncer()) {
        int errCode = StartSyncer();
        if (errCode != E_OK && errCode != -E_NO_NEED_ACTIVE) {
            return errCode;
        }
    }
    return syncer_.GetHashDeviceId(clientId, hashDevId);
}
}
