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

#include "generic_syncer.h"

#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "ref_object.h"
#include "sqlite_single_ver_natural_store.h"
#include "time_sync.h"
#include "single_ver_data_sync.h"
#ifndef OMIT_MULTI_VER
#include "commit_history_sync.h"
#include "multi_ver_data_sync.h"
#include "value_slice_sync.h"
#endif
#include "device_manager.h"
#include "db_constant.h"
#include "ability_sync.h"
#include "generic_single_ver_kv_entry.h"
#include "single_ver_serialize_manager.h"

namespace DistributedDB {
namespace {
    constexpr uint32_t DEFAULT_MTU_SIZE = 1024u * 1024u; // 1M
}
const int GenericSyncer::MIN_VALID_SYNC_ID = 1;
std::mutex GenericSyncer::moduleInitLock_;
int GenericSyncer::currentSyncId_ = 0;
std::mutex GenericSyncer::syncIdLock_;
GenericSyncer::GenericSyncer()
    : syncEngine_(nullptr),
      syncInterface_(nullptr),
      timeHelper_(nullptr),
      metadata_(nullptr),
      initialized_(false),
      queuedManualSyncSize_(0),
      queuedManualSyncLimit_(DBConstant::QUEUED_SYNC_LIMIT_DEFAULT),
      manualSyncEnable_(true),
      closing_(false),
      engineFinalize_(false),
      timeChangeListenerFinalize_(true),
      timeChangedListener_(nullptr)
{
}

GenericSyncer::~GenericSyncer()
{
    LOGD("[GenericSyncer] ~GenericSyncer!");
    if (syncEngine_ != nullptr) {
        syncEngine_->OnKill([this]() { this->syncEngine_->Close(); });
        RefObject::KillAndDecObjRef(syncEngine_);
        // waiting all thread exist
        std::unique_lock<std::mutex> cvLock(engineMutex_);
        bool engineFinalize = engineFinalizeCv_.wait_for(cvLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT),
            [this]() { return engineFinalize_; });
        if (!engineFinalize) {
            LOGW("syncer finalize before engine finalize!");
        }
        syncEngine_ = nullptr;
    }
    ReleaseInnerResource();
    std::lock_guard<std::mutex> lock(syncerLock_);
    syncInterface_ = nullptr;
}

int GenericSyncer::Initialize(ISyncInterface *syncInterface, bool isNeedActive)
{
    if (syncInterface == nullptr) {
        LOGE("[Syncer] Init failed, the syncInterface is null!");
        return -E_INVALID_ARGS;
    }

    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (initialized_) {
            return E_OK;
        }
        if (closing_) {
            LOGE("[Syncer] Syncer is closing, return!");
            return -E_BUSY;
        }
        std::vector<uint8_t> label = syncInterface->GetIdentifier();
        label_ = DBCommon::StringMasking(DBCommon::VectorToHexString(label));

        int errCode = InitStorageResource(syncInterface);
        if (errCode != E_OK) {
            return errCode;
        }
        // As timeChangedListener_ will record time change, it should not be clear even if engine init failed.
        // It will be clear in destructor.
        int errCodeTimeChangedListener = InitTimeChangedListener();
        if (errCodeTimeChangedListener != E_OK) {
            return -E_INTERNAL_ERROR;
        }
        errCode = CheckSyncActive(syncInterface, isNeedActive);
        if (errCode != E_OK) {
            return errCode;
        }

        if (!RuntimeContext::GetInstance()->IsCommunicatorAggregatorValid()) {
            LOGW("[Syncer] Communicator component not ready!");
            return -E_NOT_INIT;
        }

        errCode = SyncModuleInit();
        if (errCode != E_OK) {
            LOGE("[Syncer] Sync ModuleInit ERR!");
            return -E_INTERNAL_ERROR;
        }

        errCode = InitSyncEngine(syncInterface);
        if (errCode != E_OK) {
            return errCode;
        }
        syncEngine_->SetEqualIdentifier();
        initialized_ = true;
    }

    // StartCommunicator may start an auto sync, this function can not in syncerLock_
    syncEngine_->StartCommunicator();
    if (RuntimeContext::GetInstance()->CheckDBTimeChange(syncInterface_->GetIdentifier())) {
        ResetTimeSyncMarkByTimeChange(metadata_, *syncInterface_);
    }
    return E_OK;
}

int GenericSyncer::Close(bool isClosedOperation)
{
    int errCode = CloseInner(isClosedOperation);
    if (errCode != -E_BUSY && isClosedOperation) {
        ReleaseInnerResource();
    }
    return errCode;
}

int GenericSyncer::Sync(const std::vector<std::string> &devices, int mode,
    const std::function<void(const std::map<std::string, int> &)> &onComplete,
    const std::function<void(void)> &onFinalize, bool wait = false)
{
    SyncParma param;
    param.devices = devices;
    param.mode = mode;
    param.onComplete = onComplete;
    param.onFinalize = onFinalize;
    param.wait = wait;
    return Sync(param);
}

int GenericSyncer::Sync(const InternalSyncParma &param)
{
    SyncParma syncParam;
    syncParam.devices = param.devices;
    syncParam.mode = param.mode;
    syncParam.isQuerySync = param.isQuerySync;
    syncParam.syncQuery = param.syncQuery;
    return Sync(syncParam);
}

int GenericSyncer::Sync(const SyncParma &param)
{
    return Sync(param, DBConstant::IGNORE_CONNECTION_ID);
}

int GenericSyncer::Sync(const SyncParma &param, uint64_t connectionId)
{
    int errCode = SyncPreCheck(param);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = AddQueuedManualSyncSize(param.mode, param.wait);
    if (errCode != E_OK) {
        return errCode;
    }

    uint32_t syncId = GenerateSyncId();
    errCode = PrepareSync(param, syncId, connectionId);
    if (errCode != E_OK) {
        LOGE("[Syncer] PrepareSync failed when sync called, err %d", errCode);
        return errCode;
    }
    PerformanceAnalysis::GetInstance()->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_SYNC_TOTAL);
    return E_OK;
}

int GenericSyncer::PrepareSync(const SyncParma &param, uint32_t syncId, uint64_t connectionId)
{
    auto *operation =
        new (std::nothrow) SyncOperation(syncId, param.devices, param.mode, param.onComplete, param.wait);
    if (operation == nullptr) {
        SubQueuedSyncSize();
        return -E_OUT_OF_MEMORY;
    }
    ISyncEngine *engine = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(syncerLock_);
        PerformanceAnalysis::GetInstance()->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_SYNC_TOTAL);
        InitSyncOperation(operation, param);
        LOGI("[Syncer] GenerateSyncId %" PRIu32 ", mode = %d, wait = %d, label = %s, devices = %s", syncId, param.mode,
            param.wait, label_.c_str(), GetSyncDevicesStr(param.devices).c_str());
        engine = syncEngine_;
        RefObject::IncObjRef(engine);
    }
    AddSyncOperation(engine, operation);
    RefObject::DecObjRef(engine);
    PerformanceAnalysis::GetInstance()->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_SYNC_TOTAL);
    if (connectionId != DBConstant::IGNORE_CONNECTION_ID) {
        std::lock_guard<std::mutex> lockGuard(syncIdLock_);
        connectionIdMap_[connectionId].push_back(static_cast<int>(syncId));
        syncIdMap_[static_cast<int>(syncId)] = connectionId;
    }
    if (operation->CheckIsAllFinished()) {
        operation->Finished();
        RefObject::KillAndDecObjRef(operation);
    } else {
        operation->WaitIfNeed();
        RefObject::DecObjRef(operation);
    }
    return E_OK;
}

int GenericSyncer::RemoveSyncOperation(int syncId)
{
    SyncOperation *operation = nullptr;
    std::unique_lock<std::mutex> lock(operationMapLock_);
    auto iter = syncOperationMap_.find(syncId);
    if (iter != syncOperationMap_.end()) {
        LOGD("[Syncer] RemoveSyncOperation id:%d.", syncId);
        operation = iter->second;
        syncOperationMap_.erase(syncId);
        lock.unlock();
        if ((!operation->IsAutoSync()) && (!operation->IsBlockSync()) && (!operation->IsAutoControlCmd())) {
            SubQueuedSyncSize();
        }
        operation->NotifyIfNeed();
        RefObject::KillAndDecObjRef(operation);
        operation = nullptr;
        std::lock_guard<std::mutex> lockGuard(syncIdLock_);
        if (syncIdMap_.find(syncId) == syncIdMap_.end()) {
            return E_OK;
        }
        uint64_t connectionId = syncIdMap_[syncId];
        if (connectionIdMap_.find(connectionId) != connectionIdMap_.end()) {
            connectionIdMap_[connectionId].remove(syncId);
        }
        syncIdMap_.erase(syncId);
        return E_OK;
    }
    return -E_INVALID_ARGS;
}

int GenericSyncer::StopSync(uint64_t connectionId)
{
    std::list<int> syncIdList;
    {
        std::lock_guard<std::mutex> lockGuard(syncIdLock_);
        if (connectionIdMap_.find(connectionId) == connectionIdMap_.end()) {
            return E_OK;
        }
        syncIdList = connectionIdMap_[connectionId];
        connectionIdMap_.erase(connectionId);
    }
    for (auto syncId : syncIdList) {
        RemoveSyncOperation(syncId);
        if (syncEngine_ != nullptr) {
            syncEngine_->AbortMachineIfNeed(syncId);
        }
    }
    if (syncEngine_ != nullptr) {
        syncEngine_->NotifyConnectionClosed(connectionId);
    }
    return E_OK;
}

uint64_t GenericSyncer::GetTimestamp()
{
    std::shared_ptr<TimeHelper> timeHelper = nullptr;
    ISyncInterface *storage = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        timeHelper = timeHelper_;
        if (syncInterface_ != nullptr) {
            storage = syncInterface_;
            storage->IncRefCount();
        }
    }
    if (storage == nullptr) {
        return TimeHelper::GetSysCurrentTime();
    }
    if (timeHelper == nullptr) {
        storage->DecRefCount();
        return TimeHelper::GetSysCurrentTime();
    }
    uint64_t timestamp = timeHelper->GetTime();
    storage->DecRefCount();
    return timestamp;
}

void GenericSyncer::QueryAutoSync(const InternalSyncParma &param)
{
    if (!initialized_) {
        LOGW("[Syncer] Syncer has not Init");
        return;
    }
    LOGI("[GenericSyncer] trigger query syncmode=%u,dev=%s", param.mode, GetSyncDevicesStr(param.devices).c_str());
    ISyncInterface *syncInterface = nullptr;
    ISyncEngine *engine = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (syncInterface_ == nullptr || syncEngine_ == nullptr) {
            LOGW("[Syncer] Syncer has not Init");
            return;
        }
        syncInterface = syncInterface_;
        engine = syncEngine_;
        syncInterface->IncRefCount();
        RefObject::IncObjRef(engine);
    }
    int retCode = RuntimeContext::GetInstance()->ScheduleTask([this, param, engine, syncInterface] {
        int errCode = Sync(param);
        if (errCode != E_OK) {
            LOGE("[GenericSyncer] sync start by QueryAutoSync failed err %d", errCode);
        }
        RefObject::DecObjRef(engine);
        syncInterface->DecRefCount();
    });
    if (retCode != E_OK) {
        LOGE("[GenericSyncer] QueryAutoSync triggler sync retCode:%d", retCode);
        RefObject::DecObjRef(engine);
        syncInterface->DecRefCount();
    }
}

void GenericSyncer::AddSyncOperation(ISyncEngine *engine, SyncOperation *operation)
{
    if (operation == nullptr) {
        return;
    }

    LOGD("[Syncer] AddSyncOperation.");
    engine->AddSyncOperation(operation);

    if (operation->CheckIsAllFinished()) {
        return;
    }

    std::lock_guard<std::mutex> lock(operationMapLock_);
    syncOperationMap_.insert(std::pair<int, SyncOperation *>(operation->GetSyncId(), operation));
    // To make sure operation alive before WaitIfNeed out
    RefObject::IncObjRef(operation);
}

void GenericSyncer::SyncOperationKillCallbackInner(int syncId)
{
    if (syncEngine_ != nullptr) {
        LOGI("[Syncer] Operation on kill id = %d", syncId);
        syncEngine_->RemoveSyncOperation(syncId);
    }
}

void GenericSyncer::SyncOperationKillCallback(int syncId)
{
    SyncOperationKillCallbackInner(syncId);
}

int GenericSyncer::InitMetaData(ISyncInterface *syncInterface)
{
    if (metadata_ != nullptr) {
        return E_OK;
    }

    metadata_ = std::make_shared<Metadata>();
    if (metadata_ == nullptr) {
        LOGE("[Syncer] metadata make shared failed");
        return -E_OUT_OF_MEMORY;
    }
    int errCode = metadata_->Initialize(syncInterface);
    if (errCode != E_OK) {
        LOGE("[Syncer] metadata Initializeate failed! err %d.", errCode);
        metadata_ = nullptr;
    }
    syncInterface_ = syncInterface;
    return errCode;
}

int GenericSyncer::InitTimeHelper(ISyncInterface *syncInterface)
{
    if (timeHelper_ != nullptr) {
        return E_OK;
    }

    timeHelper_ = std::make_shared<TimeHelper>();
    if (timeHelper_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    int errCode = timeHelper_->Initialize(syncInterface, metadata_);
    if (errCode != E_OK) {
        LOGE("[Syncer] TimeHelper init failed! err:%d.", errCode);
        timeHelper_ = nullptr;
    }
    return errCode;
}

int GenericSyncer::InitSyncEngine(ISyncInterface *syncInterface)
{
    if (syncEngine_ != nullptr && syncEngine_->IsEngineActive()) {
        LOGI("[Syncer] syncEngine is active");
        return E_OK;
    }
    int errCode = BuildSyncEngine();
    if (errCode != E_OK) {
        return errCode;
    }
    const std::function<void(std::string)> onlineFunc = std::bind(&GenericSyncer::RemoteDataChanged,
        this, std::placeholders::_1);
    const std::function<void(std::string)> offlineFunc = std::bind(&GenericSyncer::RemoteDeviceOffline,
        this, std::placeholders::_1);
    const std::function<void(const InternalSyncParma &param)> queryAutoSyncFunc =
        std::bind(&GenericSyncer::QueryAutoSync, this, std::placeholders::_1);
    const ISyncEngine::InitCallbackParam param = { onlineFunc, offlineFunc, queryAutoSyncFunc };
    errCode = syncEngine_->Initialize(syncInterface, metadata_, param);
    if (errCode == E_OK) {
        syncInterface->IncRefCount();
        label_ = syncEngine_->GetLabel();
        return E_OK;
    } else {
        LOGE("[Syncer] SyncEngine init failed! err:%d.", errCode);
        RefObject::KillAndDecObjRef(syncEngine_);
        syncEngine_ = nullptr;
        return errCode;
    }
}

int GenericSyncer::CheckSyncActive(ISyncInterface *syncInterface, bool isNeedActive)
{
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    if (!isSyncDualTupleMode || isNeedActive) {
        return E_OK;
    }
    LOGI("[Syncer] syncer no need to active");
    int errCode = BuildSyncEngine();
    if (errCode != E_OK) {
        return errCode;
    }
    return -E_NO_NEED_ACTIVE;
}

uint32_t GenericSyncer::GenerateSyncId()
{
    std::lock_guard<std::mutex> lock(syncIdLock_);
    currentSyncId_++;
    // if overflow, reset to 1
    if (currentSyncId_ <= 0) {
        currentSyncId_ = MIN_VALID_SYNC_ID;
    }
    return currentSyncId_;
}

bool GenericSyncer::IsValidMode(int mode) const
{
    if ((mode >= SyncModeType::INVALID_MODE) || (mode < SyncModeType::PUSH)) {
        LOGE("[Syncer] Sync mode is not valid!");
        return false;
    }
    return true;
}

int GenericSyncer::SyncConditionCheck(const SyncParma &param, const ISyncEngine *engine, ISyncInterface *storage) const
{
    (void)param;
    (void)engine;
    (void)storage;
    return E_OK;
}

bool GenericSyncer::IsValidDevices(const std::vector<std::string> &devices) const
{
    if (devices.empty()) {
        LOGE("[Syncer] devices is empty!");
        return false;
    }
    return true;
}

void GenericSyncer::ClearSyncOperations(bool isClosedOperation)
{
    std::vector<SyncOperation *> syncOperation;
    {
        std::lock_guard<std::mutex> lock(operationMapLock_);
        for (auto &item : syncOperationMap_) {
            bool isBlockSync = item.second->IsBlockSync();
            if (isBlockSync || !isClosedOperation) {
                int status = (!isClosedOperation) ? SyncOperation::OP_USER_CHANGED : SyncOperation::OP_FAILED;
                item.second->SetUnfinishedDevStatus(status);
                RefObject::IncObjRef(item.second);
                syncOperation.push_back(item.second);
            }
        }
    }

    if (!isClosedOperation) { // means user changed
        syncEngine_->NotifyUserChange();
    }

    for (auto &operation : syncOperation) {
        // block sync operation or userChange will trigger remove sync operation
        // caller won't blocked for block sync
        // caller won't blocked for userChange operation no mater it is block or non-block sync
        TriggerSyncFinished(operation);
        RefObject::DecObjRef(operation);
    }
    ClearInnerResource(isClosedOperation);
}

void GenericSyncer::ClearInnerResource(bool isClosedOperation)
{
    {
        std::lock_guard<std::mutex> lock(operationMapLock_);
        for (auto &iter : syncOperationMap_) {
            RefObject::KillAndDecObjRef(iter.second);
            iter.second = nullptr;
        }
        syncOperationMap_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(syncIdLock_);
        if (isClosedOperation) {
            connectionIdMap_.clear();
        } else { // only need to clear syncid when user change
            for (auto &item : connectionIdMap_) {
                item.second.clear();
            }
        }
        syncIdMap_.clear();
    }
}

void GenericSyncer::TriggerSyncFinished(SyncOperation *operation)
{
    if (operation != nullptr && operation->CheckIsAllFinished()) {
        operation->Finished();
    }
}

void GenericSyncer::OnSyncFinished(int syncId)
{
    (void)(RemoveSyncOperation(syncId));
}

int GenericSyncer::SyncModuleInit()
{
    static bool isInit = false;
    std::lock_guard<std::mutex> lock(moduleInitLock_);
    if (!isInit) {
        int errCode = SyncResourceInit();
        if (errCode != E_OK) {
            return errCode;
        }
        isInit = true;
        return E_OK;
    }
    return E_OK;
}

int GenericSyncer::SyncResourceInit()
{
    int errCode = TimeSync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register timesync message transform func ERR!");
        return errCode;
    }
    errCode = SingleVerSerializeManager::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register SingleVerDataSync message transform func ERR!");
        return errCode;
    }
#ifndef OMIT_MULTI_VER
    errCode = CommitHistorySync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register CommitHistorySync message transform func ERR!");
        return errCode;
    }
    errCode = MultiVerDataSync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register MultiVerDataSync message transform func ERR!");
        return errCode;
    }
    errCode = ValueSliceSync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register ValueSliceSync message transform func ERR!");
        return errCode;
    }
#endif
    errCode = DeviceManager::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register DeviceManager message transform func ERR!");
        return errCode;
    }
    errCode = AbilitySync::RegisterTransformFunc();
    if (errCode != E_OK) {
        LOGE("Register AbilitySync message transform func ERR!");
        return errCode;
    }
    return E_OK;
}

int GenericSyncer::GetQueuedSyncSize(int *queuedSyncSize) const
{
    if (queuedSyncSize == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    *queuedSyncSize = queuedManualSyncSize_;
    LOGI("[GenericSyncer] GetQueuedSyncSize:%d", queuedManualSyncSize_);
    return E_OK;
}

int GenericSyncer::SetQueuedSyncLimit(const int *queuedSyncLimit)
{
    if (queuedSyncLimit == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    queuedManualSyncLimit_ = *queuedSyncLimit;
    LOGI("[GenericSyncer] SetQueuedSyncLimit:%d", queuedManualSyncLimit_);
    return E_OK;
}

int GenericSyncer::GetQueuedSyncLimit(int *queuedSyncLimit) const
{
    if (queuedSyncLimit == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    *queuedSyncLimit = queuedManualSyncLimit_;
    LOGI("[GenericSyncer] GetQueuedSyncLimit:%d", queuedManualSyncLimit_);
    return E_OK;
}

bool GenericSyncer::IsManualSync(int inMode) const
{
    int mode = SyncOperation::TransferSyncMode(inMode);
    if ((mode == SyncModeType::PULL) || (mode == SyncModeType::PUSH) || (mode == SyncModeType::PUSH_AND_PULL) ||
        (mode == SyncModeType::SUBSCRIBE_QUERY) || (mode == SyncModeType::UNSUBSCRIBE_QUERY)) {
        return true;
    }
    return false;
}

int GenericSyncer::AddQueuedManualSyncSize(int mode, bool wait)
{
    if (IsManualSync(mode) && (!wait)) {
        std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
        if (!manualSyncEnable_) {
            LOGI("[GenericSyncer] manualSyncEnable is Disable");
            return -E_BUSY;
        }
        queuedManualSyncSize_++;
    }
    return E_OK;
}

bool GenericSyncer::IsQueuedManualSyncFull(int mode, bool wait) const
{
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    if (IsManualSync(mode) && (!manualSyncEnable_)) {
        LOGI("[GenericSyncer] manualSyncEnable_:false");
        return true;
    }
    if (IsManualSync(mode) && (!wait)) {
        if (queuedManualSyncSize_ < queuedManualSyncLimit_) {
            return false;
        } else {
            LOGD("[GenericSyncer] queuedManualSyncSize_:%d < queuedManualSyncLimit_:%d", queuedManualSyncSize_,
                queuedManualSyncLimit_);
            return true;
        }
    } else {
        return false;
    }
}

void GenericSyncer::SubQueuedSyncSize(void)
{
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    queuedManualSyncSize_--;
    if (queuedManualSyncSize_ < 0) {
        LOGE("[GenericSyncer] queuedManualSyncSize_ < 0!");
        queuedManualSyncSize_ = 0;
    }
}

int GenericSyncer::DisableManualSync(void)
{
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    if (queuedManualSyncSize_ > 0) {
        LOGD("[GenericSyncer] DisableManualSync fail, queuedManualSyncSize_:%d", queuedManualSyncSize_);
        return -E_BUSY;
    }
    manualSyncEnable_ = false;
    LOGD("[GenericSyncer] DisableManualSync ok");
    return E_OK;
}

int GenericSyncer::EnableManualSync(void)
{
    std::lock_guard<std::mutex> lock(queuedManualSyncLock_);
    manualSyncEnable_ = true;
    LOGD("[GenericSyncer] EnableManualSync ok");
    return E_OK;
}

int GenericSyncer::GetLocalIdentity(std::string &outTarget) const
{
    std::string deviceId;
    int errCode =  RuntimeContext::GetInstance()->GetLocalIdentity(deviceId);
    if (errCode != E_OK) {
        LOGE("[GenericSyncer] GetLocalIdentity fail errCode:%d", errCode);
        return errCode;
    }
    outTarget = DBCommon::TransferHashString(deviceId);
    return E_OK;
}

void GenericSyncer::GetOnlineDevices(std::vector<std::string> &devices) const
{
    std::string identifier;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        // Get devices from AutoLaunch first.
        if (syncInterface_ == nullptr) {
            LOGI("[Syncer] GetOnlineDevices syncInterface_ is nullptr");
            return;
        }
        bool isSyncDualTupleMode = syncInterface_->GetDbProperties().GetBoolProp(KvDBProperties::SYNC_DUAL_TUPLE_MODE,
            false);
        if (isSyncDualTupleMode) {
            identifier = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::DUAL_TUPLE_IDENTIFIER_DATA,
                "");
        } else {
            identifier = syncInterface_->GetDbProperties().GetStringProp(KvDBProperties::IDENTIFIER_DATA, "");
        }
    }
    RuntimeContext::GetInstance()->GetAutoLaunchSyncDevices(identifier, devices);
    if (!devices.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(syncerLock_);
    if (closing_) {
        LOGW("[Syncer] Syncer is closing, return!");
        return;
    }
    if (syncEngine_ != nullptr) {
        syncEngine_->GetOnlineDevices(devices);
    }
}

int GenericSyncer::SetSyncRetry(bool isRetry)
{
    if (syncEngine_ == nullptr) {
        return -E_NOT_INIT;
    }
    syncEngine_->SetSyncRetry(isRetry);
    return E_OK;
}

int GenericSyncer::SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets)
{
    std::lock_guard<std::mutex> lock(syncerLock_);
    if (syncEngine_ == nullptr) {
        return -E_NOT_INIT;
    }
    int errCode = syncEngine_->SetEqualIdentifier(identifier, targets);
    if (errCode == E_OK) {
        syncEngine_->SetEqualIdentifierMap(identifier, targets);
    }
    return errCode;
}

std::string GenericSyncer::GetSyncDevicesStr(const std::vector<std::string> &devices) const
{
    std::string syncDevices;
    for (const auto &dev:devices) {
        syncDevices += DBCommon::StringMasking(dev);
        syncDevices += ",";
    }
    return syncDevices.substr(0, syncDevices.size() - 1);
}

int GenericSyncer::StatusCheck() const
{
    if (!initialized_) {
        LOGE("[Syncer] Syncer is not initialized, return!");
        return -E_BUSY;
    }
    if (closing_) {
        LOGW("[Syncer] Syncer is closing, return!");
        return -E_BUSY;
    }
    return E_OK;
}

int GenericSyncer::SyncPreCheck(const SyncParma &param) const
{
    ISyncEngine *engine = nullptr;
    ISyncInterface *storage = nullptr;
    int errCode = E_OK;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        errCode = StatusCheck();
        if (errCode != E_OK) {
            return errCode;
        }
        if (!IsValidDevices(param.devices) || !IsValidMode(param.mode)) {
            return -E_INVALID_ARGS;
        }
        if (IsQueuedManualSyncFull(param.mode, param.wait)) {
            LOGE("[Syncer] -E_BUSY");
            return -E_BUSY;
        }
        storage = syncInterface_;
        engine = syncEngine_;
        if (storage == nullptr || engine == nullptr) {
            return -E_BUSY;
        }
        storage->IncRefCount();
        RefObject::IncObjRef(engine);
    }
    errCode = SyncConditionCheck(param, engine, storage);
    storage->DecRefCount();
    RefObject::DecObjRef(engine);
    return errCode;
}

void GenericSyncer::InitSyncOperation(SyncOperation *operation, const SyncParma &param)
{
    operation->SetIdentifier(syncInterface_->GetIdentifier());
    operation->Initialize();
    operation->OnKill(std::bind(&GenericSyncer::SyncOperationKillCallback, this, operation->GetSyncId()));
    std::function<void(int)> onFinished = std::bind(&GenericSyncer::OnSyncFinished, this, std::placeholders::_1);
    operation->SetOnSyncFinished(onFinished);
    operation->SetOnSyncFinalize(param.onFinalize);
    if (param.isQuerySync) {
        operation->SetQuery(param.syncQuery);
    }
}

int GenericSyncer::BuildSyncEngine()
{
    if (syncEngine_ != nullptr) {
        return E_OK;
    }
    syncEngine_ = CreateSyncEngine();
    if (syncEngine_ == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    syncEngine_->OnLastRef([this]() {
        LOGD("[Syncer] SyncEngine finalized");
        {
            std::lock_guard<std::mutex> cvLock(engineMutex_);
            engineFinalize_ = true;
        }
        engineFinalizeCv_.notify_all();
    });
    return E_OK;
}

void GenericSyncer::Dump(int fd)
{
    if (syncEngine_ == nullptr) {
        return;
    }
    RefObject::IncObjRef(syncEngine_);
    syncEngine_->Dump(fd);
    RefObject::DecObjRef(syncEngine_);
}

SyncerBasicInfo GenericSyncer::DumpSyncerBasicInfo()
{
    SyncerBasicInfo baseInfo;
    if (syncEngine_ == nullptr) {
        return baseInfo;
    }
    RefObject::IncObjRef(syncEngine_);
    baseInfo.isSyncActive = syncEngine_->IsEngineActive();
    RefObject::DecObjRef(syncEngine_);
    return baseInfo;
}

int GenericSyncer::RemoteQuery(const std::string &device, const RemoteCondition &condition,
    uint64_t timeout, uint64_t connectionId, std::shared_ptr<ResultSet> &result)
{
    ISyncEngine *syncEngine = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        int errCode = StatusCheck();
        if (errCode != E_OK) {
            return errCode;
        }
        syncEngine = syncEngine_;
        RefObject::IncObjRef(syncEngine);
    }
    if (syncEngine == nullptr) {
        return -E_NOT_INIT;
    }
    int errCode = syncEngine->RemoteQuery(device, condition, timeout, connectionId, result);
    RefObject::DecObjRef(syncEngine);
    return errCode;
}

int GenericSyncer::InitTimeChangedListener()
{
    int errCode = E_OK;
    if (timeChangedListener_ != nullptr) {
        return errCode;
    }
    timeChangedListener_ = RuntimeContext::GetInstance()->RegisterTimeChangedLister(
        [this](void *changedOffset) {
            RecordTimeChangeOffset(changedOffset);
        },
        [this]() {
            {
                std::lock_guard<std::mutex> autoLock(timeChangeListenerMutex_);
                timeChangeListenerFinalize_ = true;
            }
            timeChangeCv_.notify_all();
        }, errCode);
    if (timeChangedListener_ == nullptr) {
        LOGE("[GenericSyncer] Init RegisterTimeChangedLister failed");
        return errCode;
    }
    {
        std::lock_guard<std::mutex> autoLock(timeChangeListenerMutex_);
        timeChangeListenerFinalize_ = false;
    }
    return E_OK;
}

void GenericSyncer::ReleaseInnerResource()
{
    NotificationChain::Listener *timeChangedListener = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (timeChangedListener_ != nullptr) {
            timeChangedListener = timeChangedListener_;
            timeChangedListener_ = nullptr;
        }
        timeHelper_ = nullptr;
        metadata_ = nullptr;
    }
    if (timeChangedListener != nullptr) {
        timeChangedListener->Drop(true);
        RuntimeContext::GetInstance()->StopTimeTickMonitorIfNeed();
    }
    std::unique_lock<std::mutex> uniqueLock(timeChangeListenerMutex_);
    LOGD("[GenericSyncer] Begin wait time change listener finalize");
    timeChangeCv_.wait(uniqueLock, [this]() {
        return timeChangeListenerFinalize_;
    });
    LOGD("[GenericSyncer] End wait time change listener finalize");
}

void GenericSyncer::RecordTimeChangeOffset(void *changedOffset)
{
    std::shared_ptr<Metadata> metadata = nullptr;
    ISyncInterface *storage = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (changedOffset == nullptr || metadata_ == nullptr || syncInterface_ == nullptr) {
            return;
        }
        storage = syncInterface_;
        metadata = metadata_;
        storage->IncRefCount();
    }
    TimeOffset changedTimeOffset = *(reinterpret_cast<TimeOffset *>(changedOffset)) *
        static_cast<TimeOffset>(TimeHelper::TO_100_NS);
    TimeOffset orgOffset = metadata->GetLocalTimeOffset() - changedTimeOffset;
    TimeOffset currentSysTime = static_cast<TimeOffset>(TimeHelper::GetSysCurrentTime());
    Timestamp maxItemTime = 0;
    storage->GetMaxTimestamp(maxItemTime);
    if ((orgOffset + currentSysTime) > TimeHelper::BUFFER_VALID_TIME) {
        orgOffset = TimeHelper::BUFFER_VALID_TIME -
            currentSysTime + static_cast<TimeOffset>(TimeHelper::MS_TO_100_NS);
    }
    if ((currentSysTime + orgOffset) <= static_cast<TimeOffset>(maxItemTime)) {
        orgOffset = static_cast<TimeOffset>(maxItemTime) - currentSysTime +
            static_cast<TimeOffset>(TimeHelper::MS_TO_100_NS); // 1ms
    }
    metadata->SaveLocalTimeOffset(orgOffset);
    ResetTimeSyncMarkByTimeChange(metadata, *storage);
    storage->DecRefCount();
}

int GenericSyncer::CloseInner(bool isClosedOperation)
{
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (!initialized_) {
            LOGW("[Syncer] Syncer[%s] don't need to close, because it has not been init", label_.c_str());
            return -E_NOT_INIT;
        }
        initialized_ = false;
        if (closing_) {
            LOGE("[Syncer] Syncer is closing, return!");
            return -E_BUSY;
        }
        closing_ = true;
    }
    ClearSyncOperations(isClosedOperation);
    if (syncEngine_ != nullptr) {
        syncEngine_->Close();
        LOGD("[Syncer] Close SyncEngine!");
        std::lock_guard<std::mutex> lock(syncerLock_);
        closing_ = false;
    }
    return E_OK;
}

int GenericSyncer::GetSyncDataSize(const std::string &device, size_t &size) const
{
    uint64_t localWaterMark = 0;
    std::shared_ptr<Metadata> metadata = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (metadata_ == nullptr || syncInterface_ == nullptr) {
            return -E_INTERNAL_ERROR;
        }
        if (closing_) {
            LOGE("[Syncer] Syncer is closing, return!");
            return -E_BUSY;
        }
        int errCode = static_cast<SyncGenericInterface *>(syncInterface_)->TryHandle();
        if (errCode != E_OK) {
            LOGE("[Syncer] syncer is restarting, return!");
            return errCode;
        }
        syncInterface_->IncRefCount();
        metadata = metadata_;
    }
    metadata->GetLocalWaterMark(device, localWaterMark);
    uint32_t expectedMtuSize = DEFAULT_MTU_SIZE;
    DataSizeSpecInfo syncDataSizeInfo = {expectedMtuSize, static_cast<size_t>(MAX_TIMESTAMP)};
    std::vector<SendDataItem> outData;
    ContinueToken token = nullptr;
    int errCode = static_cast<SyncGenericInterface *>(syncInterface_)->GetSyncData(localWaterMark, MAX_TIMESTAMP,
        outData, token, syncDataSizeInfo);
    if (token != nullptr) {
        static_cast<SyncGenericInterface *>(syncInterface_)->ReleaseContinueToken(token);
        token = nullptr;
    }
    if ((errCode != E_OK) && (errCode != -E_UNFINISHED)) {
        LOGE("calculate sync data size failed %d", errCode);
        syncInterface_->DecRefCount();
        return errCode;
    }
    uint32_t totalLen = 0;
    if (errCode == -E_UNFINISHED) {
        totalLen = expectedMtuSize;
    } else {
        totalLen = GenericSingleVerKvEntry::CalculateLens(outData, SOFTWARE_VERSION_CURRENT);
    }
    for (auto &entry : outData) {
        delete entry;
        entry = nullptr;
    }
    syncInterface_->DecRefCount();
    // if larger than 1M, return 1M
    size = (totalLen >= expectedMtuSize) ? expectedMtuSize : totalLen;
    return E_OK;
}

bool GenericSyncer::IsNeedActive(ISyncInterface *syncInterface)
{
    bool localOnly = syncInterface->GetDbProperties().GetBoolProp(KvDBProperties::LOCAL_ONLY, false);
    if (localOnly) {
        LOGD("[Syncer] Local only db, don't need active syncer");
        return false;
    }
    return true;
}

int GenericSyncer::GetHashDeviceId(const std::string &clientId, std::string &hashDevId) const
{
    (void)clientId;
    (void)hashDevId;
    return -E_NOT_SUPPORT;
}

int GenericSyncer::InitStorageResource(ISyncInterface *syncInterface)
{
    // As metadata_ will be used in EraseDeviceWaterMark, it should not be clear even if engine init failed.
    // It will be clear in destructor.
    int errCode = InitMetaData(syncInterface);
    if (errCode != E_OK) {
        return errCode;
    }

    // As timeHelper_ will be used in GetTimestamp, it should not be clear even if engine init failed.
    // It will be clear in destructor.
    errCode = InitTimeHelper(syncInterface);
    if (errCode != E_OK) {
        return errCode;
    }

    if (!IsNeedActive(syncInterface)) {
        return -E_NO_NEED_ACTIVE;
    }
    return errCode;
}

int GenericSyncer::GetWatermarkInfo(const std::string &device, WatermarkInfo &info)
{
    std::shared_ptr<Metadata> metadata = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(syncerLock_);
        metadata = metadata_;
    }
    if (metadata == nullptr) {
        LOGE("[Syncer] Metadata is not init");
        return -E_NOT_INIT;
    }
    std::string dev;
    bool devNeedHash = true;
    int errCode = metadata->GetHashDeviceId(device, dev);
    if (errCode != E_OK && errCode != -E_NOT_SUPPORT) {
        LOGE("[Syncer] Get watermark info failed %d", errCode);
        return errCode;
    } else if (errCode == E_OK) {
        devNeedHash = false;
    } else {
        dev = device;
    }
    return metadata->GetWaterMarkInfoFromDB(dev, devNeedHash, info);
}

int GenericSyncer::UpgradeSchemaVerInMeta()
{
    std::shared_ptr<Metadata> metadata = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(syncerLock_);
        metadata = metadata_;
    }
    if (metadata == nullptr) {
        LOGE("[Syncer] metadata is not init");
        return -E_NOT_INIT;
    }
    int errCode = metadata->ClearAllAbilitySyncFinishMark();
    if (errCode != E_OK) {
        LOGE("[Syncer] clear ability mark failed:%d", errCode);
        return errCode;
    }
    auto [err, localSchemaVer] = metadata->GetLocalSchemaVersion();
    if (err != E_OK) {
        LOGE("[Syncer] get local schema version failed:%d", err);
        return err;
    }
    errCode = metadata->SetLocalSchemaVersion(localSchemaVer + 1);
    if (errCode != E_OK) {
        LOGE("[Syncer] increase local schema version failed:%d", errCode);
    }
    return errCode;
}

void GenericSyncer::ResetTimeSyncMarkByTimeChange(std::shared_ptr<Metadata> &metadata, ISyncInterface &storage)
{
    if (syncEngine_ != nullptr) {
        syncEngine_->TimeChange();
    }
    int errCode = metadata->ClearAllTimeSyncFinishMark();
    if (errCode != E_OK) {
        LOGW("[GenericSyncer] %s clear time sync finish mark failed %d", label_.c_str(), errCode);
    } else {
        LOGD("[GenericSyncer] ClearAllTimeSyncFinishMark finish");
        RuntimeContext::GetInstance()->ResetDBTimeChangeStatus(storage.GetIdentifier());
    }
}

void GenericSyncer::ResetSyncStatus()
{
    std::shared_ptr <Metadata> metadata = nullptr;
    {
        std::lock_guard <std::mutex> lock(syncerLock_);
        if (metadata_ == nullptr) {
            return;
        }
        metadata = metadata_;
    }
    metadata->ClearAllAbilitySyncFinishMark();
}

int64_t GenericSyncer::GetLocalTimeOffset()
{
    std::shared_ptr<Metadata> metadata = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (metadata_ == nullptr) {
            return 0;
        }
        metadata = metadata_;
    }
    return metadata->GetLocalTimeOffset();
}

int32_t GenericSyncer::GetTaskCount()
{
    int32_t count = 0;
    {
        std::lock_guard<std::mutex> autoLock(operationMapLock_);
        count += static_cast<int32_t>(syncOperationMap_.size());
    }
    ISyncEngine *syncEngine = nullptr;
    {
        std::lock_guard<std::mutex> lock(syncerLock_);
        if (syncEngine_ == nullptr) {
            return count;
        }
        syncEngine = syncEngine_;
        RefObject::IncObjRef(syncEngine);
    }
    count += syncEngine->GetResponseTaskCount();
    RefObject::DecObjRef(syncEngine);
    return count;
}
} // namespace DistributedDB
