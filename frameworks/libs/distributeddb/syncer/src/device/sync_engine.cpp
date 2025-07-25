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

#include "sync_engine.h"

#include <algorithm>
#include <deque>
#include <functional>

#include "ability_sync.h"
#include "db_common.h"
#include "db_dump_helper.h"
#include "db_errno.h"
#include "device_manager.h"
#include "hash.h"
#include "isync_state_machine.h"
#include "log_print.h"
#include "runtime_context.h"
#include "single_ver_serialize_manager.h"
#include "subscribe_manager.h"
#include "time_sync.h"

#ifndef OMIT_MULTI_VER
#include "commit_history_sync.h"
#include "multi_ver_data_sync.h"
#include "value_slice_sync.h"
#endif

namespace DistributedDB {
int SyncEngine::queueCacheSize_ = 0;
int SyncEngine::maxQueueCacheSize_ = DEFAULT_CACHE_SIZE;
unsigned int SyncEngine::discardMsgNum_ = 0;
std::mutex SyncEngine::queueLock_;

SyncEngine::SyncEngine()
    : syncInterface_(nullptr),
      communicator_(nullptr),
      deviceManager_(nullptr),
      metadata_(nullptr),
      execTaskCount_(0),
      isSyncRetry_(false),
      communicatorProxy_(nullptr),
      isActive_(false),
      remoteExecutor_(nullptr)
{
}

SyncEngine::~SyncEngine()
{
    LOGD("[SyncEngine] ~SyncEngine!");
    ClearInnerResource();
    equalIdentifierMap_.clear();
    subManager_ = nullptr;
    LOGD("[SyncEngine] ~SyncEngine ok!");
}

int SyncEngine::Initialize(ISyncInterface *syncInterface, const std::shared_ptr<Metadata> &metadata,
    const InitCallbackParam &callbackParam)
{
    if ((syncInterface == nullptr) || (metadata == nullptr)) {
        LOGE("[SyncEngine] [Initialize] syncInterface or metadata is nullptr.");
        return -E_INVALID_ARGS;
    }
    int errCode = StartAutoSubscribeTimer(*syncInterface);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = InitComunicator(syncInterface);
    if (errCode != E_OK) {
        LOGE("[SyncEngine] Init Communicator failed");
        // There need to set nullptr. other wise, syncInterface will be
        // DecRef in th destroy-method.
        StopAutoSubscribeTimer();
        return errCode;
    }
    onRemoteDataChanged_ = callbackParam.onRemoteDataChanged;
    offlineChanged_ = callbackParam.offlineChanged;
    queryAutoSyncCallback_ = callbackParam.queryAutoSyncCallback;
    errCode = InitInnerSource(callbackParam.onRemoteDataChanged, callbackParam.offlineChanged, syncInterface);
    if (errCode != E_OK) {
        // reset ptr if initialize device manager failed
        StopAutoSubscribeTimer();
        return errCode;
    }
    SetSyncInterface(syncInterface);
    if (subManager_ == nullptr) {
        subManager_ = std::make_shared<SubscribeManager>();
    }
    metadata_ = metadata;
    isActive_ = true;
    LOGI("[SyncEngine] Engine [%.3s] init ok", label_.c_str());
    return E_OK;
}

int SyncEngine::Close()
{
    LOGI("[SyncEngine] [%.3s] close enter!", label_.c_str());
    isActive_ = false;
    UnRegCommunicatorsCallback();
    StopAutoSubscribeTimer();
    std::vector<ISyncTaskContext *> decContext;
    // Clear SyncContexts
    {
        std::unique_lock<std::mutex> lock(contextMapLock_);
        for (auto &iter : syncTaskContextMap_) {
            decContext.push_back(iter.second);
            iter.second = nullptr;
        }
        syncTaskContextMap_.clear();
    }
    for (auto &iter : decContext) {
        RefObject::KillAndDecObjRef(iter);
        iter = nullptr;
    }
    WaitingExecTaskExist();
    ReleaseCommunicators();
    {
        std::lock_guard<std::mutex> msgLock(queueLock_);
        while (!msgQueue_.empty()) {
            Message *inMsg = msgQueue_.front();
            msgQueue_.pop_front();
            if (inMsg != nullptr) { // LCOV_EXCL_BR_LINE
                queueCacheSize_ -= GetMsgSize(inMsg);
                delete inMsg;
                inMsg = nullptr;
            }
        }
    }
    // close db, rekey or import scene, need clear all remote query info
    // local query info will destroy with syncEngine destruct
    if (subManager_ != nullptr) {
        subManager_->ClearAllRemoteQuery();
    }

    RemoteExecutor *executor = GetAndIncRemoteExector();
    if (executor != nullptr) {
        executor->Close();
        RefObject::DecObjRef(executor);
        executor = nullptr;
    }
    ClearInnerResource();
    LOGI("[SyncEngine] [%.3s] closed!", label_.c_str());
    return E_OK;
}

int SyncEngine::AddSyncOperation(SyncOperation *operation)
{
    if (operation == nullptr) {
        LOGE("[SyncEngine] operation is nullptr");
        return -E_INVALID_ARGS;
    }

    std::vector<std::string> devices = operation->GetDevices();
    std::string localDeviceId;
    int errCode = GetLocalDeviceId(localDeviceId);
    for (const auto &deviceId : devices) {
        if (errCode != E_OK) {
            operation->SetStatus(deviceId, errCode == -E_BUSY ?
                SyncOperation::OP_BUSY_FAILURE : SyncOperation::OP_FAILED);
            continue;
        }
        if (!CheckDeviceIdValid(deviceId, localDeviceId)) {
            operation->SetStatus(deviceId, SyncOperation::OP_INVALID_ARGS);
            continue;
        }
        operation->SetStatus(deviceId, SyncOperation::OP_WAITING);
        if (AddSyncOperForContext(deviceId, operation) != E_OK) {
            operation->SetStatus(deviceId, SyncOperation::OP_FAILED);
        }
    }
    return E_OK;
}

void SyncEngine::RemoveSyncOperation(int syncId)
{
    std::lock_guard<std::mutex> lock(contextMapLock_);
    for (auto &iter : syncTaskContextMap_) {
        ISyncTaskContext *context = iter.second;
        if (context != nullptr) {
            context->RemoveSyncOperation(syncId);
        }
    }
}

#ifndef OMIT_MULTI_VER
void SyncEngine::BroadCastDataChanged() const
{
    if (deviceManager_ != nullptr) {
        (void)deviceManager_->SendBroadCast(LOCAL_DATA_CHANGED);
    }
}
#endif // OMIT_MULTI_VER

void SyncEngine::StartCommunicator()
{
    if (communicator_ == nullptr) {
        LOGE("[SyncEngine][StartCommunicator] communicator is not set!");
        return;
    }
    LOGD("[SyncEngine][StartCommunicator] RegOnConnectCallback");
    int errCode = communicator_->RegOnConnectCallback(
        [this, deviceManager = deviceManager_](const std::string &targetDev, bool isConnect) {
            deviceManager->OnDeviceConnectCallback(targetDev, isConnect);
        }, nullptr);
    if (errCode != E_OK) {
        LOGE("[SyncEngine][StartCommunicator] register failed, auto sync can not use! err %d", errCode);
        return;
    }
    communicator_->Activate(GetUserId());
}

void SyncEngine::GetOnlineDevices(std::vector<std::string> &devices) const
{
    devices.clear();
    if (deviceManager_ != nullptr) {
        deviceManager_->GetOnlineDevices(devices);
    }
}

int SyncEngine::InitInnerSource(const std::function<void(std::string)> &onRemoteDataChanged,
    const std::function<void(std::string)> &offlineChanged, ISyncInterface *syncInterface)
{
    deviceManager_ = new (std::nothrow) DeviceManager();
    if (deviceManager_ == nullptr) {
        LOGE("[SyncEngine] deviceManager alloc failed!");
        return -E_OUT_OF_MEMORY;
    }
    auto executor = new (std::nothrow) RemoteExecutor();
    if (executor == nullptr) {
        LOGE("[SyncEngine] remoteExecutor alloc failed!");
        delete deviceManager_;
        deviceManager_ = nullptr;
        return -E_OUT_OF_MEMORY;
    }

    int errCode = E_OK;
    do {
        CommunicatorProxy *comProxy = nullptr;
        {
            std::lock_guard<std::mutex> lock(communicatorProxyLock_);
            comProxy = communicatorProxy_;
            RefObject::IncObjRef(comProxy);
        }
        errCode = deviceManager_->Initialize(comProxy, onRemoteDataChanged, offlineChanged);
        RefObject::DecObjRef(comProxy);
        if (errCode != E_OK) {
            LOGE("[SyncEngine] deviceManager init failed! err %d", errCode);
            break;
        }
        errCode = executor->Initialize(syncInterface, communicator_);
    } while (false);
    if (errCode != E_OK) {
        delete deviceManager_;
        deviceManager_ = nullptr;
        delete executor;
        executor = nullptr;
    } else {
        SetRemoteExector(executor);
    }
    return errCode;
}

int SyncEngine::InitComunicator(const ISyncInterface *syncInterface)
{
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    int errCode = RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    if (communicatorAggregator == nullptr) {
        LOGE("[SyncEngine] Get ICommunicatorAggregator error when init the sync engine err = %d", errCode);
        return errCode;
    }
    std::vector<uint8_t> label = syncInterface->GetIdentifier();
    bool isSyncDualTupleMode = syncInterface->GetDbProperties().GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false);
    if (isSyncDualTupleMode) {
        std::vector<uint8_t> dualTuplelabel = syncInterface->GetDualTupleIdentifier();
        LOGI("[SyncEngine] dual tuple mode, original identifier=%.3s, target identifier=%.3s", VEC_TO_STR(label),
            VEC_TO_STR(dualTuplelabel));
        communicator_ = communicatorAggregator->AllocCommunicator(dualTuplelabel, errCode, GetUserId(syncInterface));
    } else {
        communicator_ = communicatorAggregator->AllocCommunicator(label, errCode, GetUserId(syncInterface));
    }
    if (communicator_ == nullptr) {
        LOGE("[SyncEngine] AllocCommunicator error when init the sync engine! err = %d", errCode);
        return errCode;
    }

    errCode = communicator_->RegOnMessageCallback(
        [this](const std::string &targetDev, Message *inMsg) { MessageReciveCallback(targetDev, inMsg); }, []() {});
    if (errCode != E_OK) {
        LOGE("[SyncEngine] SyncRequestCallback register failed! err = %d", errCode);
        communicatorAggregator->ReleaseCommunicator(communicator_, GetUserId(syncInterface));
        communicator_ = nullptr;
        return errCode;
    }
    {
        std::lock_guard<std::mutex> lock(communicatorProxyLock_);
        communicatorProxy_ = new (std::nothrow) CommunicatorProxy();
        if (communicatorProxy_ == nullptr) {
            communicatorAggregator->ReleaseCommunicator(communicator_, GetUserId(syncInterface));
            communicator_ = nullptr;
            return -E_OUT_OF_MEMORY;
        }
        communicatorProxy_->SetMainCommunicator(communicator_);
    }
    label.resize(3); // only show 3 Bytes enough
    label_ = DBCommon::VectorToHexString(label);
    LOGD("[SyncEngine] RegOnConnectCallback");
    return errCode;
}

int SyncEngine::AddSyncOperForContext(const std::string &deviceId, SyncOperation *operation)
{
    int errCode = E_OK;
    bool isSyncDualTupleMode = syncInterface_->GetDbProperties().GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, false);
    std::string targetUserId = DBConstant::DEFAULT_USER;
    if (isSyncDualTupleMode) {
        targetUserId = GetTargetUserId(deviceId);
    }
    ISyncTaskContext *context = nullptr;
    {
        std::lock_guard<std::mutex> lock(contextMapLock_);
        context = FindSyncTaskContext({deviceId, targetUserId});
        if (context == nullptr) {
            if (!IsKilled()) {
                context = GetSyncTaskContext({deviceId, targetUserId}, errCode);
            }
            if (context == nullptr) {
                return errCode;
            }
        }
        if (context->IsKilled()) { // LCOV_EXCL_BR_LINE
            return -E_OBJ_IS_KILLED;
        }
        // IncRef for SyncEngine to make sure context is valid, to avoid a big lock
        RefObject::IncObjRef(context);
    }

    errCode = context->AddSyncOperation(operation);
    if (operation != nullptr) {
        operation->SetSyncContext(context); // make the life cycle of context and operation are same
    }
    RefObject::DecObjRef(context);
    return errCode;
}

void SyncEngine::MessageReciveCallbackTask(ISyncTaskContext *context, const ICommunicator *communicator,
    Message *inMsg)
{
    std::string deviceId = context->GetDeviceId();

    if (inMsg->GetMessageId() != LOCAL_DATA_CHANGED) {
        int errCode = context->ReceiveMessageCallback(inMsg);
        if (errCode == -E_NOT_NEED_DELETE_MSG) {
            goto MSG_CALLBACK_OUT_NOT_DEL;
        }
        // add auto sync here while recv subscribe request
        QuerySyncObject syncObject;
        if (errCode == E_OK && context->IsNeedTriggerQueryAutoSync(inMsg, syncObject)) {
            InternalSyncParma param;
            GetQueryAutoSyncParam(deviceId, syncObject, param);
            queryAutoSyncCallback_(param);
        }
    }

    delete inMsg;
    inMsg = nullptr;
MSG_CALLBACK_OUT_NOT_DEL:
    ScheduleTaskOut(context, communicator);
}

void SyncEngine::RemoteDataChangedTask(ISyncTaskContext *context, const ICommunicator *communicator, Message *inMsg)
{
    std::string deviceId = context->GetDeviceId();
    if (onRemoteDataChanged_ && deviceManager_->IsDeviceOnline(deviceId)) {
        onRemoteDataChanged_(deviceId);
    } else {
        LOGE("[SyncEngine] onRemoteDataChanged is null!");
    }
    delete inMsg;
    inMsg = nullptr;
    ScheduleTaskOut(context, communicator);
}

void SyncEngine::ScheduleTaskOut(ISyncTaskContext *context, const ICommunicator *communicator)
{
    (void)DealMsgUtilQueueEmpty();
    DecExecTaskCount();
    RefObject::DecObjRef(communicator);
    RefObject::DecObjRef(context);
}

int SyncEngine::DealMsgUtilQueueEmpty()
{
    if (!isActive_) {
        return -E_BUSY; // db is closing just return
    }
    int errCode = E_OK;
    Message *inMsg = nullptr;
    {
        std::lock_guard<std::mutex> lock(queueLock_);
        if (msgQueue_.empty()) {
            return errCode;
        }
        inMsg = msgQueue_.front();
        msgQueue_.pop_front();
        queueCacheSize_ -= GetMsgSize(inMsg);
    }

    IncExecTaskCount();
    // it will deal with the first message in queue, we should increase object reference counts and sure that resources
    // could be prevented from destroying by other threads.
    do {
        ISyncTaskContext *nextContext = GetContextForMsg({inMsg->GetTarget(), inMsg->GetSenderUserId()}, errCode);
        if (errCode != E_OK) {
            break;
        }
        errCode = ScheduleDealMsg(nextContext, inMsg);
        if (errCode != E_OK) {
            RefObject::DecObjRef(nextContext);
        }
    } while (false);
    if (errCode != E_OK) {
        delete inMsg;
        inMsg = nullptr;
        DecExecTaskCount();
    }
    return errCode;
}

ISyncTaskContext *SyncEngine::GetContextForMsg(const DeviceSyncTarget &target, int &errCode)
{
    ISyncTaskContext *context = nullptr;
    {
        std::lock_guard<std::mutex> lock(contextMapLock_);
        context = FindSyncTaskContext(target);
        if (context != nullptr) { // LCOV_EXCL_BR_LINE
            if (context->IsKilled()) {
                errCode = -E_OBJ_IS_KILLED;
                return nullptr;
            }
        } else {
            if (IsKilled()) {
                errCode = -E_OBJ_IS_KILLED;
                return nullptr;
            }
            context = GetSyncTaskContext(target, errCode);
            if (context == nullptr) {
                return nullptr;
            }
        }
        // IncRef for context to make sure context is valid, when task run another thread
        RefObject::IncObjRef(context);
    }
    return context;
}

int SyncEngine::ScheduleDealMsg(ISyncTaskContext *context, Message *inMsg)
{
    if (inMsg == nullptr) {
        LOGE("[SyncEngine] MessageReciveCallback inMsg is null!");
        DecExecTaskCount();
        return E_OK;
    }
    CommunicatorProxy *comProxy = nullptr;
    {
        std::lock_guard<std::mutex> lock(communicatorProxyLock_);
        comProxy = communicatorProxy_;
        RefObject::IncObjRef(comProxy);
    }
    int errCode = E_OK;
    // deal remote local data changed message
    if (inMsg->GetMessageId() == LOCAL_DATA_CHANGED) {
        RemoteDataChangedTask(context, comProxy, inMsg);
    } else {
        errCode = RuntimeContext::GetInstance()->ScheduleTask(
            [this, context, comProxy, inMsg] { MessageReciveCallbackTask(context, comProxy, inMsg); });
    }

    if (errCode != E_OK) {
        LOGE("[SyncEngine] MessageReciveCallbackTask Schedule failed err %d", errCode);
        RefObject::DecObjRef(comProxy);
    }
    return errCode;
}

void SyncEngine::MessageReciveCallback(const std::string &targetDev, Message *inMsg)
{
    IncExecTaskCount();
    int errCode = MessageReciveCallbackInner(targetDev, inMsg);
    if (errCode != E_OK) {
        if (inMsg != nullptr) {
            delete inMsg;
            inMsg = nullptr;
        }
        DecExecTaskCount();
        LOGE("[SyncEngine] MessageReciveCallback failed!");
    }
}

int SyncEngine::MessageReciveCallbackInner(const std::string &targetDev, Message *inMsg)
{
    if (targetDev.empty() || inMsg == nullptr) {
        LOGE("[SyncEngine][MessageReciveCallback] from a invalid device or inMsg is null ");
        return -E_INVALID_ARGS;
    }
    if (!isActive_) {
        LOGE("[SyncEngine] engine is closing, ignore msg");
        return -E_BUSY;
    }
    if (inMsg->GetMessageId() == REMOTE_EXECUTE_MESSAGE) {
        return HandleRemoteExecutorMsg(targetDev, inMsg);
    }

    int msgSize = 0;
    if (!IsSkipCalculateLen(inMsg)) {
        msgSize = GetMsgSize(inMsg);
        if (msgSize <= 0) {
            LOGE("[SyncEngine] GetMsgSize makes a mistake");
            return -E_NOT_SUPPORT;
        }
    }

    {
        std::lock_guard<std::mutex> lock(queueLock_);
        if ((queueCacheSize_ + msgSize) > maxQueueCacheSize_) {
            LOGE("[SyncEngine] The size of message queue is beyond maximum");
            discardMsgNum_++;
            return -E_BUSY;
        }

        if (execTaskCount_ > MAX_EXEC_NUM) {
            PutMsgIntoQueue(targetDev, inMsg, msgSize);
            // task dont exec here
            DecExecTaskCount();
            return E_OK;
        }
    }

    int errCode = E_OK;
    ISyncTaskContext *nextContext = GetContextForMsg({targetDev, inMsg->GetSenderUserId()}, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    LOGD("[SyncEngine] MessageReciveCallback MSG ID = %d", inMsg->GetMessageId());
    return ScheduleDealMsg(nextContext, inMsg);
}

void SyncEngine::PutMsgIntoQueue(const std::string &targetDev, Message *inMsg, int msgSize)
{
    if (inMsg->GetMessageId() == LOCAL_DATA_CHANGED) {
        auto iter = std::find_if(msgQueue_.begin(), msgQueue_.end(),
            [&targetDev](const Message *msg) {
                return targetDev == msg->GetTarget() && msg->GetMessageId() == LOCAL_DATA_CHANGED;
            });
        if (iter != msgQueue_.end()) { // LCOV_EXCL_BR_LINE
            delete inMsg;
            inMsg = nullptr;
            return;
        }
    }
    inMsg->SetTarget(targetDev);
    msgQueue_.push_back(inMsg);
    queueCacheSize_ += msgSize;
    LOGW("[SyncEngine] The quantity of executing threads is beyond maximum. msgQueueSize = %zu", msgQueue_.size());
}

int SyncEngine::GetMsgSize(const Message *inMsg) const
{
    switch (inMsg->GetMessageId()) {
        case TIME_SYNC_MESSAGE:
            return TimeSync::CalculateLen(inMsg);
        case ABILITY_SYNC_MESSAGE:
            return AbilitySync::CalculateLen(inMsg);
        case DATA_SYNC_MESSAGE:
        case QUERY_SYNC_MESSAGE:
        case CONTROL_SYNC_MESSAGE:
            return SingleVerSerializeManager::CalculateLen(inMsg);
#ifndef OMIT_MULTI_VER
        case COMMIT_HISTORY_SYNC_MESSAGE:
            return CommitHistorySync::CalculateLen(inMsg);
        case MULTI_VER_DATA_SYNC_MESSAGE:
            return MultiVerDataSync::CalculateLen(inMsg);
        case VALUE_SLICE_SYNC_MESSAGE:
            return ValueSliceSync::CalculateLen(inMsg);
#endif
        case LOCAL_DATA_CHANGED:
            return DeviceManager::CalculateLen();
        default:
            LOGE("[SyncEngine] GetMsgSize not support msgId:%u", inMsg->GetMessageId());
            return -E_NOT_SUPPORT;
    }
}

ISyncTaskContext *SyncEngine::FindSyncTaskContext(const DeviceSyncTarget &target)
{
    if (target.userId == DBConstant::DEFAULT_USER) {
        for (auto &[key, value] : syncTaskContextMap_) {
            if (key.device == target.device) {
                return value;
            }
        }
    }
    auto iter = syncTaskContextMap_.find(target);
    if (iter != syncTaskContextMap_.end()) {
        ISyncTaskContext *context = iter->second;
        return context;
    }
    return nullptr;
}

std::vector<ISyncTaskContext *> SyncEngine::GetSyncTaskContextAndInc(const std::string &deviceId)
{
    std::vector<ISyncTaskContext *> contexts;
    std::lock_guard<std::mutex> lock(contextMapLock_);
    for (const auto &iter : syncTaskContextMap_) {
        if (iter.first.device != deviceId) {
            continue;
        }
        if (iter.second == nullptr) {
            LOGI("[SyncEngine] dev=%s, context is null, no need to clear sync operation", STR_MASK(deviceId));
            return {};
        }
        if (iter.second->IsKilled()) { // LCOV_EXCL_BR_LINE
            LOGI("[SyncEngine] context is killing");
            return {};
        }
        RefObject::IncObjRef(iter.second);
        contexts.push_back(iter.second);
    }
    return contexts;
}

ISyncTaskContext *SyncEngine::GetSyncTaskContext(const DeviceSyncTarget &target, int &errCode)
{
    auto storage = GetAndIncSyncInterface();
    if (storage == nullptr) {
        errCode = -E_INVALID_DB;
        LOGE("[SyncEngine] SyncTaskContext alloc failed with null db");
        return nullptr;
    }
    ISyncTaskContext *context = CreateSyncTaskContext(*storage);
    if (context == nullptr) {
        errCode = -E_OUT_OF_MEMORY;
        LOGE("[SyncEngine] SyncTaskContext alloc failed, may be no memory available!");
        return nullptr;
    }
    errCode = context->Initialize(target, storage, metadata_, communicatorProxy_);
    if (errCode != E_OK) {
        LOGE("[SyncEngine] context init failed err %d, dev %s", errCode, STR_MASK(target.device));
        RefObject::DecObjRef(context);
        storage->DecRefCount();
        context = nullptr;
        return nullptr;
    }
    syncTaskContextMap_.insert(std::pair<DeviceSyncTarget, ISyncTaskContext *>(target, context));
    // IncRef for SyncEngine to make sure SyncEngine is valid when context access
    RefObject::IncObjRef(this);
    context->OnLastRef([this, target, storage]() {
        LOGD("[SyncEngine] SyncTaskContext for id %s finalized", STR_MASK(target.device));
        RefObject::DecObjRef(this);
        storage->DecRefCount();
    });
    context->RegOnSyncTask([this, context] { return ExecSyncTask(context); });
    return context;
}

int SyncEngine::ExecSyncTask(ISyncTaskContext *context)
{
    if (IsKilled()) {
        return -E_OBJ_IS_KILLED;
    }
    auto timeout = GetTimeout(context->GetDeviceId());
    AutoLock lockGuard(context);
    int status = context->GetTaskExecStatus();
    if ((status == SyncTaskContext::RUNNING) || context->IsKilled()) {
        return -E_NOT_SUPPORT;
    }
    context->SetTaskExecStatus(ISyncTaskContext::RUNNING);
    while (!context->IsTargetQueueEmpty()) {
        int errCode = context->GetNextTarget(timeout);
        if (errCode != E_OK) {
            // current task execute failed, try next task
            context->ClearSyncOperation();
            continue;
        }
        if (context->IsCurrentSyncTaskCanBeSkipped()) { // LCOV_EXCL_BR_LINE
            context->SetOperationStatus(SyncOperation::OP_FINISHED_ALL);
            context->ClearSyncOperation();
            continue;
        }
        context->UnlockObj();
        errCode = context->StartStateMachine();
        context->LockObj();
        if (errCode != E_OK) {
            // machine start failed because timer start failed, try to execute next task
            LOGW("[SyncEngine] machine StartSync failed");
            context->SetOperationStatus(SyncOperation::OP_FAILED);
            context->ClearSyncOperation();
            continue;
        }
        // now task is running just return here
        return errCode;
    }
    LOGD("[SyncEngine] ExecSyncTask finished");
    context->SetTaskExecStatus(ISyncTaskContext::FINISHED);
    return E_OK;
}

int SyncEngine::GetQueueCacheSize() const
{
    std::lock_guard<std::mutex> lock(queueLock_);
    return queueCacheSize_;
}

void SyncEngine::SetQueueCacheSize(int size)
{
    std::lock_guard<std::mutex> lock(queueLock_);
    queueCacheSize_ = size;
}

unsigned int SyncEngine::GetDiscardMsgNum() const
{
    std::lock_guard<std::mutex> lock(queueLock_);
    return discardMsgNum_;
}

void SyncEngine::SetDiscardMsgNum(unsigned int num)
{
    std::lock_guard<std::mutex> lock(queueLock_);
    discardMsgNum_ = num;
}

unsigned int SyncEngine::GetMaxExecNum() const
{
    return MAX_EXEC_NUM;
}

int SyncEngine::GetMaxQueueCacheSize() const
{
    return maxQueueCacheSize_;
}

void SyncEngine::SetMaxQueueCacheSize(int value)
{
    maxQueueCacheSize_ = value;
}

std::string SyncEngine::GetLabel() const
{
    return label_;
}

bool SyncEngine::GetSyncRetry() const
{
    return isSyncRetry_;
}

void SyncEngine::SetSyncRetry(bool isRetry)
{
    if (isSyncRetry_ == isRetry) {
        LOGI("sync retry is equal, syncTry=%d, no need to set.", isRetry);
        return;
    }
    isSyncRetry_ = isRetry;
    LOGI("[SyncEngine] SetSyncRetry:%d ok", isRetry);
    std::lock_guard<std::mutex> lock(contextMapLock_);
    for (auto &iter : syncTaskContextMap_) {
        ISyncTaskContext *context = iter.second;
        if (context != nullptr) { // LCOV_EXCL_BR_LINE
            context->SetSyncRetry(isRetry);
        }
    }
}

int SyncEngine::SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets)
{
    if (!isActive_) {
        LOGI("[SyncEngine] engine is closed, just put into map");
        return E_OK;
    }
    ICommunicator *communicator = nullptr;
    {
        std::lock_guard<std::mutex> lock(equalCommunicatorsLock_);
        if (equalCommunicators_.count(identifier) != 0) {
            communicator = equalCommunicators_[identifier];
        } else {
            int errCode = E_OK;
            communicator = AllocCommunicator(identifier, errCode, GetUserId());
            if (communicator == nullptr) {
                return errCode;
            }
            equalCommunicators_[identifier] = communicator;
        }
    }
    std::string targetDevices;
    for (const auto &dev : targets) {
        targetDevices += DBCommon::StringMasking(dev) + ",";
    }
    LOGI("[SyncEngine] set equal identifier=%.3s, original=%.3s, targetDevices=%s",
        DBCommon::TransferStringToHex(identifier).c_str(), label_.c_str(),
        targetDevices.substr(0, (targetDevices.size() > 0 ? targetDevices.size() - 1 : 0)).c_str());
    {
        std::lock_guard<std::mutex> lock(communicatorProxyLock_);
        if (communicatorProxy_ == nullptr) {
            return -E_INTERNAL_ERROR;
        }
        communicatorProxy_->SetEqualCommunicator(communicator, identifier, targets);
    }
    communicator->Activate(GetUserId());
    return E_OK;
}

void SyncEngine::SetEqualIdentifier()
{
    std::map<std::string, std::vector<std::string>> equalIdentifier; // key: equalIdentifier value: devices
    for (auto &item : equalIdentifierMap_) {
        if (equalIdentifier.find(item.second) == equalIdentifier.end()) { // LCOV_EXCL_BR_LINE
            equalIdentifier[item.second] = {item.first};
        } else {
            equalIdentifier[item.second].push_back(item.first);
        }
    }
    for (const auto &item : equalIdentifier) {
        SetEqualIdentifier(item.first, item.second);
    }
}

void SyncEngine::SetEqualIdentifierMap(const std::string &identifier, const std::vector<std::string> &targets)
{
    for (auto iter = equalIdentifierMap_.begin(); iter != equalIdentifierMap_.end();) {
        if (identifier == iter->second) {
            iter = equalIdentifierMap_.erase(iter);
            continue;
        }
        iter++;
    }
    for (const auto &device : targets) {
        equalIdentifierMap_[device] = identifier;
    }
}

void SyncEngine::OfflineHandleByDevice(const std::string &deviceId, ISyncInterface *storage)
{
    if (!isActive_) {
        LOGD("[SyncEngine][OfflineHandleByDevice] ignore offline because not init");
        return;
    }
    RemoteExecutor *executor = GetAndIncRemoteExector();
    if (executor != nullptr) {
        executor->NotifyDeviceOffline(deviceId);
        RefObject::DecObjRef(executor);
        executor = nullptr;
    }
    // db closed or device is offline
    // clear remote subscribe and trigger
    std::vector<std::string> remoteQueryId;
    subManager_->GetRemoteSubscribeQueryIds(deviceId, remoteQueryId);
    subManager_->ClearRemoteSubscribeQuery(deviceId);
    for (const auto &queryId: remoteQueryId) {
        if (!subManager_->IsQueryExistSubscribe(queryId)) {
            static_cast<SingleVerKvDBSyncInterface *>(storage)->RemoveSubscribe(queryId);
        }
    }
    DBInfo dbInfo;
    static_cast<SyncGenericInterface *>(storage)->GetDBInfo(dbInfo);
    RuntimeContext::GetInstance()->RemoveRemoteSubscribe(dbInfo, deviceId);
    // get context and Inc context if context is not nullptr
    std::vector<ISyncTaskContext *> contexts = GetSyncTaskContextAndInc(deviceId);
    for (const auto &context : contexts) {
        {
            std::lock_guard<std::mutex> lock(communicatorProxyLock_);
            if (communicatorProxy_ == nullptr) {
                return;
            }
            if (communicatorProxy_->IsDeviceOnline(deviceId)) { // LCOV_EXCL_BR_LINE
                LOGI("[SyncEngine] target dev=%s is online, no need to clear task.", STR_MASK(deviceId));
                RefObject::DecObjRef(context);
                return;
            }
        }
        // means device is offline, clear local subscribe
        subManager_->ClearLocalSubscribeQuery(deviceId);
        // clear sync task
        if (context != nullptr) {
            context->ClearAllSyncTask();
            RefObject::DecObjRef(context);
        }
    }
}

void SyncEngine::ClearAllSyncTaskByDevice(const std::string &deviceId)
{
    std::vector<ISyncTaskContext *> contexts = GetSyncTaskContextAndInc(deviceId);
    for (const auto &context : contexts) {
        if (context != nullptr) {
            context->ClearAllSyncTask();
            RefObject::DecObjRef(context);
        }
    }
}

void SyncEngine::GetLocalSubscribeQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries)
{
    subManager_->GetLocalSubscribeQueries(device, subscribeQueries);
}

void SyncEngine::GetRemoteSubscribeQueryIds(const std::string &device, std::vector<std::string> &subscribeQueryIds)
{
    subManager_->GetRemoteSubscribeQueryIds(device, subscribeQueryIds);
}

void SyncEngine::GetRemoteSubscribeQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries)
{
    subManager_->GetRemoteSubscribeQueries(device, subscribeQueries);
}

void SyncEngine::PutUnfinishedSubQueries(const std::string &device,
    const std::vector<QuerySyncObject> &subscribeQueries)
{
    subManager_->PutLocalUnFinishedSubQueries(device, subscribeQueries);
}

void SyncEngine::GetAllUnFinishSubQueries(std::map<std::string, std::vector<QuerySyncObject>> &allSyncQueries)
{
    subManager_->GetAllUnFinishSubQueries(allSyncQueries);
}

ICommunicator *SyncEngine::AllocCommunicator(const std::string &identifier, int &errCode, std::string userId)
{
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    errCode = RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    if (communicatorAggregator == nullptr) {
        LOGE("[SyncEngine] Get ICommunicatorAggregator error when SetEqualIdentifier err = %d", errCode);
        return nullptr;
    }
    std::vector<uint8_t> identifierVect(identifier.begin(), identifier.end());
    auto communicator = communicatorAggregator->AllocCommunicator(identifierVect, errCode, userId);
    if (communicator == nullptr) {
        LOGE("[SyncEngine] AllocCommunicator error when SetEqualIdentifier! err = %d", errCode);
        return communicator;
    }

    errCode = communicator->RegOnMessageCallback(
        [this](const std::string &targetDev, Message *inMsg) { MessageReciveCallback(targetDev, inMsg); }, []() {});
    if (errCode != E_OK) {
        LOGE("[SyncEngine] SyncRequestCallback register failed in SetEqualIdentifier! err = %d", errCode);
        communicatorAggregator->ReleaseCommunicator(communicator, userId);
        return nullptr;
    }

    errCode = communicator->RegOnConnectCallback(
        [this, deviceManager = deviceManager_](const std::string &targetDev, bool isConnect) {
            deviceManager->OnDeviceConnectCallback(targetDev, isConnect);
        }, nullptr);
    if (errCode != E_OK) {
        LOGE("[SyncEngine][RegConnCB] register failed in SetEqualIdentifier! err %d", errCode);
        communicator->RegOnMessageCallback(nullptr, nullptr);
        communicatorAggregator->ReleaseCommunicator(communicator, userId);
        return nullptr;
    }

    return communicator;
}

void SyncEngine::UnRegCommunicatorsCallback()
{
    if (communicator_ != nullptr) {
        communicator_->RegOnMessageCallback(nullptr, nullptr);
        communicator_->RegOnConnectCallback(nullptr, nullptr);
        communicator_->RegOnSendableCallback(nullptr, nullptr);
    }
    std::lock_guard<std::mutex> lock(equalCommunicatorsLock_);
    for (const auto &iter : equalCommunicators_) {
        iter.second->RegOnMessageCallback(nullptr, nullptr);
        iter.second->RegOnConnectCallback(nullptr, nullptr);
        iter.second->RegOnSendableCallback(nullptr, nullptr);
    }
}

void SyncEngine::ReleaseCommunicators()
{
    {
        std::lock_guard<std::mutex> lock(communicatorProxyLock_);
        RefObject::KillAndDecObjRef(communicatorProxy_);
        communicatorProxy_ = nullptr;
    }
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    int errCode = RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    if (communicatorAggregator == nullptr) {
        LOGF("[SyncEngine] ICommunicatorAggregator get failed when fialize SyncEngine err %d", errCode);
        return;
    }

    if (communicator_ != nullptr) {
        communicatorAggregator->ReleaseCommunicator(communicator_, GetUserId());
        communicator_ = nullptr;
    }

    std::lock_guard<std::mutex> lock(equalCommunicatorsLock_);
    for (auto &iter : equalCommunicators_) {
        communicatorAggregator->ReleaseCommunicator(iter.second, GetUserId());
    }
    equalCommunicators_.clear();
}

bool SyncEngine::IsSkipCalculateLen(const Message *inMsg)
{
    if (inMsg->IsFeedbackError()) {
        LOGE("[SyncEngine] Feedback Message with errorNo=%u.", inMsg->GetErrorNo());
        return true;
    }
    return false;
}

void SyncEngine::GetSubscribeSyncParam(const std::string &device, const QuerySyncObject &query,
    InternalSyncParma &outParam)
{
    outParam.devices = { device };
    outParam.mode = SyncModeType::AUTO_SUBSCRIBE_QUERY;
    outParam.isQuerySync = true;
    outParam.syncQuery = query;
}

void SyncEngine::GetQueryAutoSyncParam(const std::string &device, const QuerySyncObject &query,
    InternalSyncParma &outParam)
{
    outParam.devices = { device };
    outParam.mode = SyncModeType::AUTO_PUSH;
    outParam.isQuerySync = true;
    outParam.syncQuery = query;
}

int SyncEngine::StartAutoSubscribeTimer([[gnu::unused]] const ISyncInterface &syncInterface)
{
    return E_OK;
}

void SyncEngine::StopAutoSubscribeTimer()
{
}

int SyncEngine::SubscribeLimitCheck(const std::vector<std::string> &devices, QuerySyncObject &query) const
{
    return subManager_->LocalSubscribeLimitCheck(devices, query);
}


void SyncEngine::ClearInnerResource()
{
    ClearSyncInterface();
    if (deviceManager_ != nullptr) {
        delete deviceManager_;
        deviceManager_ = nullptr;
    }
    communicator_ = nullptr;
    metadata_ = nullptr;
    onRemoteDataChanged_ = nullptr;
    offlineChanged_ = nullptr;
    queryAutoSyncCallback_ = nullptr;
    std::lock_guard<std::mutex> autoLock(remoteExecutorLock_);
    if (remoteExecutor_ != nullptr) {
        RefObject::KillAndDecObjRef(remoteExecutor_);
        remoteExecutor_ = nullptr;
    }
}

bool SyncEngine::IsEngineActive() const
{
    return isActive_;
}

void SyncEngine::SchemaChange()
{
    std::vector<ISyncTaskContext *> tmpContextVec;
    {
        std::lock_guard<std::mutex> lock(contextMapLock_);
        for (const auto &entry : syncTaskContextMap_) { // LCOV_EXCL_BR_LINE
            auto context = entry.second;
            if (context == nullptr || context->IsKilled()) {
                continue;
            }
            RefObject::IncObjRef(context);
            tmpContextVec.push_back(context);
        }
    }
    for (const auto &entryContext : tmpContextVec) {
        entryContext->SchemaChange();
        RefObject::DecObjRef(entryContext);
    }
}

void SyncEngine::IncExecTaskCount()
{
    std::lock_guard<std::mutex> incLock(execTaskCountLock_);
    execTaskCount_++;
}

void SyncEngine::DecExecTaskCount()
{
    {
        std::lock_guard<std::mutex> decLock(execTaskCountLock_);
        execTaskCount_--;
    }
    execTaskCv_.notify_all();
}

void SyncEngine::Dump(int fd)
{
    {
        std::lock_guard<std::mutex> lock(communicatorProxyLock_);
        std::string communicatorLabel;
        if (communicatorProxy_ != nullptr) {
            communicatorProxy_->GetLocalIdentity(communicatorLabel);
        }
        DBDumpHelper::Dump(fd, "\tcommunicator label = %s, equalIdentify Info [\n", communicatorLabel.c_str());
        if (communicatorProxy_ != nullptr) {
            communicatorProxy_->GetLocalIdentity(communicatorLabel);
            communicatorProxy_->Dump(fd);
        }
    }
    DBDumpHelper::Dump(fd, "\t]\n\tcontext info [\n");
    // dump context info
    std::lock_guard<std::mutex> autoLock(contextMapLock_);
    for (const auto &entry : syncTaskContextMap_) {
        if (entry.second != nullptr) {
            entry.second->Dump(fd);
        }
    }
    DBDumpHelper::Dump(fd, "\t]\n\n");
}

int SyncEngine::RemoteQuery(const std::string &device, const RemoteCondition &condition,
    uint64_t timeout, uint64_t connectionId, std::shared_ptr<ResultSet> &result)
{
    RemoteExecutor *executor = GetAndIncRemoteExector();
    if (!isActive_ || executor == nullptr) {
        RefObject::DecObjRef(executor);
        return -E_BUSY; // db is closing just return
    }
    int errCode = executor->RemoteQuery(device, condition, timeout, connectionId, result);
    RefObject::DecObjRef(executor);
    return errCode;
}

void SyncEngine::NotifyConnectionClosed(uint64_t connectionId)
{
    RemoteExecutor *executor = GetAndIncRemoteExector();
    if (!isActive_ || executor == nullptr) {
        RefObject::DecObjRef(executor);
        return; // db is closing just return
    }
    executor->NotifyConnectionClosed(connectionId);
    RefObject::DecObjRef(executor);
}

void SyncEngine::NotifyUserChange()
{
    RemoteExecutor *executor = GetAndIncRemoteExector();
    if (!isActive_ || executor == nullptr) {
        RefObject::DecObjRef(executor);
        return; // db is closing just return
    }
    executor->NotifyUserChange();
    RefObject::DecObjRef(executor);
}

RemoteExecutor *SyncEngine::GetAndIncRemoteExector()
{
    std::lock_guard<std::mutex> autoLock(remoteExecutorLock_);
    RefObject::IncObjRef(remoteExecutor_);
    return remoteExecutor_;
}

void SyncEngine::SetRemoteExector(RemoteExecutor *executor)
{
    std::lock_guard<std::mutex> autoLock(remoteExecutorLock_);
    remoteExecutor_ = executor;
}

bool SyncEngine::CheckDeviceIdValid(const std::string &checkDeviceId, const std::string &localDeviceId)
{
    if (checkDeviceId.empty()) {
        return false;
    }
    if (checkDeviceId.length() > DBConstant::MAX_DEV_LENGTH) {
        LOGE("[SyncEngine] dev is too long len=%zu", checkDeviceId.length());
        return false;
    }
    return localDeviceId != checkDeviceId;
}

int SyncEngine::GetLocalDeviceId(std::string &deviceId)
{
    if (!isActive_ || communicator_ == nullptr) {
        // db is closing
        return -E_BUSY;
    }
    auto communicator = communicator_;
    RefObject::IncObjRef(communicator);
    int errCode = communicator->GetLocalIdentity(deviceId);
    RefObject::DecObjRef(communicator);
    return errCode;
}

void SyncEngine::AbortMachineIfNeed(uint32_t syncId)
{
    std::vector<ISyncTaskContext *> abortContexts;
    {
        std::lock_guard<std::mutex> lock(contextMapLock_);
        for (const auto &entry : syncTaskContextMap_) {
            auto context = entry.second;
            if (context == nullptr || context->IsKilled()) { // LCOV_EXCL_BR_LINE
                continue;
            }
            RefObject::IncObjRef(context);
            if (context->GetSyncId() == syncId) {
                RefObject::IncObjRef(context);
                abortContexts.push_back(context);
            }
            RefObject::DecObjRef(context);
        }
    }
    for (const auto &abortContext : abortContexts) {
        abortContext->AbortMachineIfNeed(static_cast<uint32_t>(syncId));
        RefObject::DecObjRef(abortContext);
    }
}

void SyncEngine::WaitingExecTaskExist()
{
    std::unique_lock<std::mutex> closeLock(execTaskCountLock_);
    bool isTimeout = execTaskCv_.wait_for(closeLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT),
        [this]() { return execTaskCount_ == 0; });
    if (!isTimeout) { // LCOV_EXCL_BR_LINE
        LOGD("SyncEngine Close with executing task!");
    }
}

int SyncEngine::HandleRemoteExecutorMsg(const std::string &targetDev, Message *inMsg)
{
    RemoteExecutor *executor = GetAndIncRemoteExector();
    int errCode = E_OK;
    if (executor != nullptr) {
        errCode = executor->ReceiveMessage(targetDev, inMsg);
    } else {
        errCode = -E_BUSY;
    }
    DecExecTaskCount();
    RefObject::DecObjRef(executor);
    return errCode;
}

void SyncEngine::AddSubscribe(SyncGenericInterface *storage,
    const std::map<std::string, std::vector<QuerySyncObject>> &subscribeQuery)
{
    for (const auto &[device, queryList]: subscribeQuery) {
        for (const auto &query: queryList) {
            AddQuerySubscribe(storage, device, query);
        }
    }
}

void SyncEngine::AddQuerySubscribe(SyncGenericInterface *storage, const std::string &device,
    const QuerySyncObject &query)
{
    int errCode = storage->AddSubscribe(query.GetIdentify(), query, true);
    if (errCode != E_OK) {
        LOGW("[SyncEngine][AddSubscribe] Add trigger failed dev = %s queryId = %s",
            STR_MASK(device), STR_MASK(query.GetIdentify()));
        return;
    }
    errCode = subManager_->ReserveRemoteSubscribeQuery(device, query);
    if (errCode != E_OK) {
        if (!subManager_->IsQueryExistSubscribe(query.GetIdentify())) { // LCOV_EXCL_BR_LINE
            (void)storage->RemoveSubscribe(query.GetIdentify());
        }
        return;
    }
    subManager_->ActiveRemoteSubscribeQuery(device, query);
}

void SyncEngine::TimeChange()
{
    std::vector<ISyncTaskContext *> decContext;
    {
        // copy context
        std::lock_guard<std::mutex> lock(contextMapLock_);
        for (const auto &iter : syncTaskContextMap_) {
            RefObject::IncObjRef(iter.second);
            decContext.push_back(iter.second);
        }
    }
    for (auto &iter : decContext) {
        iter->TimeChange();
        RefObject::DecObjRef(iter);
    }
}

int32_t SyncEngine::GetResponseTaskCount()
{
    std::vector<ISyncTaskContext *> decContext;
    {
        // copy context
        std::lock_guard<std::mutex> lock(contextMapLock_);
        for (const auto &iter : syncTaskContextMap_) {
            RefObject::IncObjRef(iter.second);
            decContext.push_back(iter.second);
        }
    }
    int32_t taskCount = 0;
    for (auto &iter : decContext) {
        taskCount += iter->GetResponseTaskCount();
        RefObject::DecObjRef(iter);
    }
    {
        std::lock_guard<std::mutex> decLock(execTaskCountLock_);
        taskCount += static_cast<int32_t>(execTaskCount_);
    }
    return taskCount;
}

void SyncEngine::ClearSyncInterface()
{
    ISyncInterface *syncInterface = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(storageMutex_);
        if (syncInterface_ == nullptr) {
            return;
        }
        syncInterface = syncInterface_;
        syncInterface_ = nullptr;
    }
    syncInterface->DecRefCount();
}

ISyncInterface *SyncEngine::GetAndIncSyncInterface()
{
    std::lock_guard<std::mutex> autoLock(storageMutex_);
    if (syncInterface_ == nullptr) {
        return nullptr;
    }
    syncInterface_->IncRefCount();
    return syncInterface_;
}

void SyncEngine::SetSyncInterface(ISyncInterface *syncInterface)
{
    std::lock_guard<std::mutex> autoLock(storageMutex_);
    syncInterface_ = syncInterface;
}

std::string SyncEngine::GetUserId(const ISyncInterface *syncInterface)
{
    if (syncInterface == nullptr) {
        LOGW("[SyncEngine][GetUserId] sync interface has not initialized");
        return "";
    }
    std::string userId = syncInterface->GetDbProperties().GetStringProp(DBProperties::USER_ID, "");
    std::string subUserId = syncInterface->GetDbProperties().GetStringProp(DBProperties::SUB_USER, "");
    if (!subUserId.empty()) {
        userId += "-" + subUserId;
    }
    return userId;
}

std::string SyncEngine::GetUserId()
{
    std::lock_guard<std::mutex> autoLock(storageMutex_);
    return GetUserId(syncInterface_);
}

uint32_t SyncEngine::GetTimeout(const std::string &dev)
{
    ICommunicator *communicator = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(communicatorProxyLock_);
        if (communicatorProxy_ == nullptr) {
            LOGW("[SyncEngine] Communicator is null when get %.3s timeout", dev.c_str());
            return DBConstant::MIN_TIMEOUT;
        }
        communicator = communicatorProxy_;
        RefObject::IncObjRef(communicator);
    }
    uint32_t timeout = communicator->GetTimeout(dev);
    RefObject::DecObjRef(communicator);
    return timeout;
}

std::string SyncEngine::GetTargetUserId(const std::string &dev)
{
    std::string targetUserId;
    ICommunicator *communicator = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(communicatorProxyLock_);
        if (communicatorProxy_ == nullptr) {
            LOGW("[SyncEngine] Communicator is null when get target user");
            return targetUserId;
        }
        communicator = communicatorProxy_;
        RefObject::IncObjRef(communicator);
    }
    DBProperties properties = syncInterface_->GetDbProperties();
    ExtendInfo extendInfo;
    extendInfo.appId = properties.GetStringProp(DBProperties::APP_ID, "");
    extendInfo.userId = properties.GetStringProp(DBProperties::USER_ID, "");
    extendInfo.storeId = properties.GetStringProp(DBProperties::STORE_ID, "");
    extendInfo.dstTarget = dev;
    extendInfo.subUserId = properties.GetStringProp(DBProperties::SUB_USER, "");
    targetUserId = communicator->GetTargetUserId(extendInfo);
    RefObject::DecObjRef(communicator);
    return targetUserId;
}

int32_t SyncEngine::GetRemoteQueryTaskCount()
{
    auto executor = GetAndIncRemoteExector();
    if (executor == nullptr) {
        LOGW("[SyncEngine] RemoteExecutor is null when get remote query task count");
        RefObject::DecObjRef(executor);
        return 0;
    }
    auto count = executor->GetTaskCount();
    RefObject::DecObjRef(executor);
    return count;
}
} // namespace DistributedDB
