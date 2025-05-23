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

#include "sync_task_context.h"

#include <algorithm>
#include <cmath>

#include "db_constant.h"
#include "db_dump_helper.h"
#include "db_dfx_adapter.h"
#include "db_errno.h"
#include "hash.h"
#include "isync_state_machine.h"
#include "log_print.h"
#include "time_helper.h"
#include "version.h"

namespace DistributedDB {
std::mutex SyncTaskContext::synTaskContextSetLock_;
std::set<ISyncTaskContext *> SyncTaskContext::synTaskContextSet_;

namespace {
    constexpr int NEGOTIATION_LIMIT = 2;
    constexpr uint32_t SESSION_ID_MAX_VALUE = 0x8fffffffu;
}

SyncTaskContext::SyncTaskContext()
    : syncOperation_(nullptr),
      syncId_(0),
      mode_(0),
      isAutoSync_(false),
      status_(0),
      taskExecStatus_(0),
      syncInterface_(nullptr),
      communicator_(nullptr),
      stateMachine_(nullptr),
      requestSessionId_(0),
      lastRequestSessionId_(0),
      timeHelper_(nullptr),
      remoteSoftwareVersion_(0),
      remoteSoftwareVersionId_(0),
      isCommNormal_(true),
      taskErrCode_(E_OK),
      commErrCode_(E_OK),
      syncTaskRetryStatus_(false),
      isSyncRetry_(false),
      negotiationCount_(0),
      isAutoSubscribe_(false)
{
}

SyncTaskContext::~SyncTaskContext()
{
    if (stateMachine_ != nullptr) {
        delete stateMachine_;
        stateMachine_ = nullptr;
    }
    SyncTaskContext::ClearSyncOperation();
    ClearSyncTarget();
    syncInterface_ = nullptr;
    communicator_ = nullptr;
}

int SyncTaskContext::AddSyncTarget(ISyncTarget *target)
{
    if (target == nullptr) {
        return -E_INVALID_ARGS;
    }
    int targetMode = target->GetMode();
    auto syncId = static_cast<uint32_t>(target->GetSyncId());
    {
        std::lock_guard<std::mutex> lock(targetQueueLock_);
        if (target->GetTaskType() == ISyncTarget::REQUEST) {
            requestTargetQueue_.push_back(target);
        } else if (target->GetTaskType() == ISyncTarget::RESPONSE) {
            responseTargetQueue_.push_back(target);
        } else {
            return -E_INVALID_ARGS;
        }
    }
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, targetMode, syncId]() {
        CancelCurrentSyncRetryIfNeed(targetMode, syncId);
        RefObject::DecObjRef(this);
    });
    if (errCode != E_OK) {
        RefObject::DecObjRef(this);
    }
    if (onSyncTaskAdd_) {
        RefObject::IncObjRef(this);
        errCode = RuntimeContext::GetInstance()->ScheduleTask([this]() {
            onSyncTaskAdd_();
            RefObject::DecObjRef(this);
        });
        if (errCode != E_OK) {
            RefObject::DecObjRef(this);
        }
    }
    return E_OK;
}

void SyncTaskContext::SetOperationStatus(int status)
{
    std::lock_guard<std::mutex> lock(operationLock_);
    if (syncOperation_ == nullptr) {
        LOGD("[SyncTaskContext][SetStatus] syncOperation is null");
        return;
    }
    int finalStatus = status;

    int operationStatus = syncOperation_->GetStatus(deviceId_);
    if (status == SyncOperation::OP_SEND_FINISHED && operationStatus == SyncOperation::OP_RECV_FINISHED) {
        if (GetTaskErrCode() == -E_EKEYREVOKED) { // LCOV_EXCL_BR_LINE
            finalStatus = SyncOperation::OP_EKEYREVOKED_FAILURE;
        } else {
            finalStatus = SyncOperation::OP_FINISHED_ALL;
        }
    } else if (status == SyncOperation::OP_RECV_FINISHED && operationStatus == SyncOperation::OP_SEND_FINISHED) {
        if (GetTaskErrCode() == -E_EKEYREVOKED) {
            finalStatus = SyncOperation::OP_EKEYREVOKED_FAILURE;
        } else {
            finalStatus = SyncOperation::OP_FINISHED_ALL;
        }
    }
    syncOperation_->SetStatus(deviceId_, finalStatus);
    if (finalStatus >= SyncOperation::OP_FINISHED_ALL) {
        SaveLastPushTaskExecStatus(finalStatus);
    }
    if (syncOperation_->CheckIsAllFinished()) {
        syncOperation_->Finished();
    }
}

void SyncTaskContext::SaveLastPushTaskExecStatus(int finalStatus)
{
    (void)finalStatus;
}

void SyncTaskContext::Clear()
{
    StopTimer();
    retryTime_ = 0;
    sequenceId_ = 1;
    syncId_ = 0;
    isAutoSync_ = false;
    requestSessionId_ = 0;
    isNeedRetry_ = NO_NEED_RETRY;
    mode_ = SyncModeType::INVALID_MODE;
    status_ = SyncOperation::OP_WAITING;
    taskErrCode_ = E_OK;
    packetId_ = 0;
    isAutoSubscribe_ = false;
}

int SyncTaskContext::RemoveSyncOperation(int syncId)
{
    std::lock_guard<std::mutex> lock(targetQueueLock_);
    auto iter = std::find_if(requestTargetQueue_.begin(), requestTargetQueue_.end(),
        [syncId](const ISyncTarget *target) {
            if (target == nullptr) {
                return false;
            }
            return target->GetSyncId() == syncId;
        });
    if (iter != requestTargetQueue_.end()) {
        if (*iter != nullptr) {
            delete *iter;
        }
        requestTargetQueue_.erase(iter);
        return E_OK;
    }
    return -E_INVALID_ARGS;
}

void SyncTaskContext::ClearSyncTarget()
{
    std::lock_guard<std::mutex> lock(targetQueueLock_);
    for (auto &requestTarget : requestTargetQueue_) {
        if (requestTarget != nullptr) {
            delete requestTarget;
        }
    }
    requestTargetQueue_.clear();

    for (auto &responseTarget : responseTargetQueue_) {
        if (responseTarget != nullptr) { // LCOV_EXCL_BR_LINE
            delete responseTarget;
        }
    }
    responseTargetQueue_.clear();
}

bool SyncTaskContext::IsTargetQueueEmpty() const
{
    std::lock_guard<std::mutex> lock(targetQueueLock_);
    return requestTargetQueue_.empty() && responseTargetQueue_.empty();
}

int SyncTaskContext::GetOperationStatus() const
{
    std::lock_guard<std::mutex> lock(operationLock_);
    if (syncOperation_ == nullptr) {
        return SyncOperation::OP_FINISHED_ALL;
    }
    return syncOperation_->GetStatus(deviceId_);
}

void SyncTaskContext::SetMode(int mode)
{
    mode_ = mode;
}

int SyncTaskContext::GetMode() const
{
    return mode_;
}

void SyncTaskContext::MoveToNextTarget(uint32_t timeout)
{
    ClearSyncOperation();
    TaskParam param;
    // call other system api without lock
    param.timeout = timeout;
    std::lock_guard<std::mutex> lock(targetQueueLock_);
    while (!requestTargetQueue_.empty() || !responseTargetQueue_.empty()) {
        ISyncTarget *tmpTarget = nullptr;
        if (!requestTargetQueue_.empty()) {
            tmpTarget = requestTargetQueue_.front();
            requestTargetQueue_.pop_front();
        } else {
            tmpTarget = responseTargetQueue_.front();
            responseTargetQueue_.pop_front();
        }
        if (tmpTarget == nullptr) {
            LOGE("[SyncTaskContext][MoveToNextTarget] currentTarget is null skip!");
            continue;
        }
        SyncOperation *tmpOperation = nullptr;
        tmpTarget->GetSyncOperation(tmpOperation);
        if ((tmpOperation != nullptr) && tmpOperation->IsKilled()) {
            // if killed skip this syncOperation_.
            delete tmpTarget;
            tmpTarget = nullptr;
            continue;
        }
        CopyTargetData(tmpTarget, param);
        delete tmpTarget;
        tmpTarget = nullptr;
        break;
    }
}

int SyncTaskContext::GetNextTarget(uint32_t timeout)
{
    MoveToNextTarget(timeout);
    int checkErrCode = RunPermissionCheck(GetPermissionCheckFlag(IsAutoSync(), GetMode()));
    if (checkErrCode != E_OK) {
        SetOperationStatus(SyncOperation::OP_PERMISSION_CHECK_FAILED);
        return checkErrCode;
    }
    return E_OK;
}

uint32_t SyncTaskContext::GetSyncId() const
{
    return syncId_;
}

// Get the current task deviceId.
std::string SyncTaskContext::GetDeviceId() const
{
    return deviceId_;
}

std::string SyncTaskContext::GetTargetUserId() const
{
    return targetUserId_;
}

void SyncTaskContext::SetTargetUserId(const std::string &userId)
{
    targetUserId_ = userId;
}

void SyncTaskContext::SetTaskExecStatus(int status)
{
    taskExecStatus_ = status;
}

int SyncTaskContext::GetTaskExecStatus() const
{
    return taskExecStatus_;
}

bool SyncTaskContext::IsAutoSync() const
{
    return isAutoSync_;
}

int SyncTaskContext::StartTimer()
{
    std::lock_guard<std::mutex> lockGuard(timerLock_);
    if (timerId_ > 0) {
        return -E_UNEXPECTED_DATA;
    }
    TimerId timerId = 0;
    RefObject::IncObjRef(this);
    TimerAction timeOutCallback = [this](TimerId id) { return TimeOut(id); };
    int errCode = RuntimeContext::GetInstance()->SetTimer(timeout_, timeOutCallback,
        [this]() {
            int ret = RuntimeContext::GetInstance()->ScheduleTask([this]() { RefObject::DecObjRef(this); });
            if (ret != E_OK) {
                LOGE("[SyncTaskContext] timer finalizer ScheduleTask, errCode %d", ret);
            }
        }, timerId);
    if (errCode != E_OK) {
        RefObject::DecObjRef(this);
        return errCode;
    }
    timerId_ = timerId;
    return errCode;
}

void SyncTaskContext::StopTimer()
{
    TimerId timerId;
    {
        std::lock_guard<std::mutex> lockGuard(timerLock_);
        timerId = timerId_;
        if (timerId_ == 0) {
            return;
        }
        timerId_ = 0;
    }
    RuntimeContext::GetInstance()->RemoveTimer(timerId);
}

int SyncTaskContext::ModifyTimer(int milliSeconds)
{
    std::lock_guard<std::mutex> lockGuard(timerLock_);
    if (timerId_ == 0) {
        return -E_UNEXPECTED_DATA;
    }
    return RuntimeContext::GetInstance()->ModifyTimer(timerId_, milliSeconds);
}

void SyncTaskContext::SetRetryTime(int retryTime)
{
    retryTime_ = retryTime;
}

int SyncTaskContext::GetRetryTime() const
{
    return retryTime_;
}

void SyncTaskContext::SetRetryStatus(int isNeedRetry)
{
    isNeedRetry_ = isNeedRetry;
}

int SyncTaskContext::GetRetryStatus() const
{
    return isNeedRetry_;
}

TimerId SyncTaskContext::GetTimerId() const
{
    return timerId_;
}

uint32_t SyncTaskContext::GetRequestSessionId() const
{
    return requestSessionId_;
}

void SyncTaskContext::IncSequenceId()
{
    sequenceId_++;
}

uint32_t SyncTaskContext::GetSequenceId() const
{
    return sequenceId_;
}

void SyncTaskContext::ReSetSequenceId()
{
    sequenceId_ = 1;
}

void SyncTaskContext::IncPacketId()
{
    packetId_++;
}

uint64_t SyncTaskContext::GetPacketId() const
{
    return packetId_;
}

int SyncTaskContext::GetTimeoutTime() const
{
    return timeout_;
}

void SyncTaskContext::SetTimeoutCallback(const TimerAction &timeOutCallback)
{
    timeOutCallback_ = timeOutCallback;
}

void SyncTaskContext::SetTimeOffset(TimeOffset offset)
{
    timeOffset_ = offset;
}

TimeOffset SyncTaskContext::GetTimeOffset() const
{
    return timeOffset_;
}

int SyncTaskContext::StartStateMachine()
{
    return stateMachine_->StartSync();
}

int SyncTaskContext::ReceiveMessageCallback(Message *inMsg)
{
    if (inMsg->GetMessageId() != ABILITY_SYNC_MESSAGE) {
        uint16_t remoteVersion = 0;
        (void)communicator_->GetRemoteCommunicatorVersion(deviceId_, remoteVersion);
        SetRemoteSoftwareVersion(SOFTWARE_VERSION_EARLIEST + remoteVersion);
    }
    int errCode = E_OK;
    if (IncUsedCount() == E_OK) {
        errCode = stateMachine_->ReceiveMessageCallback(inMsg);
        SafeExit();
    }
    return errCode;
}

void SyncTaskContext::RegOnSyncTask(const std::function<int(void)> &callback)
{
    onSyncTaskAdd_ = callback;
}

int SyncTaskContext::IncUsedCount()
{
    AutoLock lock(this);
    if (IsKilled()) {
        LOGI("[SyncTaskContext] IncUsedCount isKilled");
        return -E_OBJ_IS_KILLED;
    }
    usedCount_++;
    return E_OK;
}

void SyncTaskContext::SafeExit()
{
    AutoLock lock(this);
    usedCount_--;
    if (usedCount_ < 1) {
        safeKill_.notify_one();
    }
}

Timestamp SyncTaskContext::GetCurrentLocalTime() const
{
    if (timeHelper_ == nullptr) {
        return TimeHelper::INVALID_TIMESTAMP;
    }
    return timeHelper_->GetTime();
}

void SyncTaskContext::Abort(int status)
{
    (void)status;
    Clear();
}

void SyncTaskContext::CommErrHandlerFunc(int errCode, ISyncTaskContext *context, int32_t sessionId, bool isDirectEnd)
{
    {
        std::lock_guard<std::mutex> lock(synTaskContextSetLock_);
        if (synTaskContextSet_.count(context) == 0) {
            LOGI("[SyncTaskContext][CommErrHandle] context has been killed");
            return;
        }
        // IncObjRef to maker sure context not been killed. after the lock_guard
        RefObject::IncObjRef(context);
    }

    static_cast<SyncTaskContext *>(context)->CommErrHandlerFuncInner(errCode, static_cast<uint32_t>(sessionId),
        isDirectEnd);
    RefObject::DecObjRef(context);
}

void SyncTaskContext::SetRemoteSoftwareVersion(uint32_t version)
{
    std::lock_guard<std::mutex> lock(remoteSoftwareVersionLock_);
    if (remoteSoftwareVersion_ == version) {
        return;
    }
    remoteSoftwareVersion_ = version;
    remoteSoftwareVersionId_++;
}

uint32_t SyncTaskContext::GetRemoteSoftwareVersion() const
{
    std::lock_guard<std::mutex> lock(remoteSoftwareVersionLock_);
    return remoteSoftwareVersion_;
}

uint64_t SyncTaskContext::GetRemoteSoftwareVersionId() const
{
    std::lock_guard<std::mutex> lock(remoteSoftwareVersionLock_);
    return remoteSoftwareVersionId_;
}

bool SyncTaskContext::IsCommNormal() const
{
    return isCommNormal_;
}

void SyncTaskContext::CommErrHandlerFuncInner(int errCode, uint32_t sessionId, bool isDirectEnd)
{
    {
        RefObject::AutoLock lock(this);
        if ((sessionId != requestSessionId_) || (requestSessionId_ == 0)) {
            return;
        }

        if (errCode == E_OK) {
            SetCommFailErrCode(errCode);
            // when communicator sent message failed, the state machine will get the error and exit this sync task
            // it seems unnecessary to change isCommNormal_ value, so just return here
            return;
        }
    }
    LOGE("[SyncTaskContext][CommErr] errCode %d, isDirectEnd %d", errCode, static_cast<int>(isDirectEnd));
    if (!isDirectEnd) {
        SetErrCodeWhenWaitTimeOut(errCode);
        return;
    }
    if (errCode > 0) {
        SetCommFailErrCode(static_cast<int>(SyncOperation::OP_COMM_ABNORMAL));
    } else {
        SetCommFailErrCode(errCode);
    }
    stateMachine_->CommErrAbort(sessionId);
}

int SyncTaskContext::TimeOut(TimerId id)
{
    if (!timeOutCallback_) {
        return E_OK;
    }
    IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, id]() {
        timeOutCallback_(id);
        DecObjRef(this);
    });
    if (errCode != E_OK) {
        LOGW("[SyncTaskContext][TimeOut] Trigger TimeOut Async Failed! TimerId=" PRIu64 " errCode=%d", id, errCode);
        DecObjRef(this);
    }
    return E_OK;
}

void SyncTaskContext::CopyTargetData(const ISyncTarget *target, const TaskParam &taskParam)
{
    retryTime_ = 0;
    mode_ = target->GetMode();
    status_ = SyncOperation::OP_SYNCING;
    isNeedRetry_ = SyncTaskContext::NO_NEED_RETRY;
    taskErrCode_ = E_OK;
    packetId_ = 0;
    isCommNormal_ = true; // reset comm status here
    commErrCode_ = E_OK;
    syncTaskRetryStatus_ = isSyncRetry_;
    timeout_ = static_cast<int>(taskParam.timeout);
    negotiationCount_ = 0;
    target->GetSyncOperation(syncOperation_);
    ReSetSequenceId();

    if (syncOperation_ != nullptr) {
        // IncRef for syncOperation_ to make sure syncOperation_ is valid, when setStatus
        RefObject::IncObjRef(syncOperation_);
        syncId_ = syncOperation_->GetSyncId();
        isAutoSync_ = syncOperation_->IsAutoSync();
        isAutoSubscribe_ = syncOperation_->IsAutoControlCmd();
        if (isAutoSync_ || mode_ == SUBSCRIBE_QUERY || mode_ == UNSUBSCRIBE_QUERY) {
            syncTaskRetryStatus_ = true;
        }
        requestSessionId_ = GenerateRequestSessionId();
        LOGI("[SyncTaskContext][copyTarget] mode=%d,syncId=%d,isAutoSync=%d,isRetry=%d,dev=%s{private}",
            mode_, syncId_, isAutoSync_, syncTaskRetryStatus_, deviceId_.c_str());
        DBDfxAdapter::StartAsyncTrace(syncActionName_, static_cast<int>(syncId_));
    } else {
        isAutoSync_ = false;
        LOGI("[SyncTaskContext][copyTarget] for response data dev %s{private},isRetry=%d", deviceId_.c_str(),
            syncTaskRetryStatus_);
    }
}

void SyncTaskContext::KillWait()
{
    StopTimer();
    UnlockObj();
    stateMachine_->NotifyClosing();
    stateMachine_->AbortImmediately();
    LockObj();
    LOGW("[SyncTaskContext] Try to kill a context, now wait.");
    bool noDeadLock = WaitLockedUntil(
        safeKill_,
        [this]() {
            if (usedCount_ < 1) {
                return true;
            }
            return false;
        },
        KILL_WAIT_SECONDS);
    if (!noDeadLock) { // LCOV_EXCL_BR_LINE
        LOGE("[SyncTaskContext] Dead lock may happen, we stop waiting the task exit.");
    } else {
        LOGW("[SyncTaskContext] Wait the task exit ok.");
    }
    std::lock_guard<std::mutex> lock(synTaskContextSetLock_);
    synTaskContextSet_.erase(this);
}

void SyncTaskContext::ClearSyncOperation()
{
    std::lock_guard<std::mutex> lock(operationLock_);
    if (syncOperation_ != nullptr) {
        DBDfxAdapter::FinishAsyncTrace(syncActionName_, static_cast<int>(syncId_));
        RefObject::DecObjRef(syncOperation_);
        syncOperation_ = nullptr;
    }
}

void SyncTaskContext::CancelCurrentSyncRetryIfNeed(int newTargetMode, uint32_t syncId)
{
    AutoLock lock(this);
    if (!isAutoSync_) {
        return;
    }
    if (syncId_ >= syncId) {
        return;
    }
    int mode = SyncOperation::TransferSyncMode(newTargetMode);
    if (newTargetMode == mode_ || mode == SyncModeType::PUSH_AND_PULL) {
        SetRetryTime(AUTO_RETRY_TIMES);
        ModifyTimer(timeout_);
    }
}

int SyncTaskContext::GetTaskErrCode() const
{
    return taskErrCode_;
}

void SyncTaskContext::SetTaskErrCode(int errCode)
{
    taskErrCode_ = errCode;
}

bool SyncTaskContext::IsSyncTaskNeedRetry() const
{
    return syncTaskRetryStatus_;
}

void SyncTaskContext::SetSyncRetry(bool isRetry)
{
    isSyncRetry_ = isRetry;
}

int SyncTaskContext::GetSyncRetryTimes() const
{
    if (IsAutoSync() || mode_ == SUBSCRIBE_QUERY || mode_ == UNSUBSCRIBE_QUERY) {
        return AUTO_RETRY_TIMES;
    }
    return MANUAL_RETRY_TIMES;
}

int SyncTaskContext::GetSyncRetryTimeout(int retryTime) const
{
    int timeoutTime = GetTimeoutTime();
    if (IsAutoSync()) {
        // set the new timeout value with 2 raised to the power of retryTime.
        return timeoutTime * (1u << retryTime);
    }
    return timeoutTime;
}

void SyncTaskContext::ClearAllSyncTask()
{
}

bool SyncTaskContext::IsAutoLiftWaterMark() const
{
    return negotiationCount_ < NEGOTIATION_LIMIT;
}

void SyncTaskContext::IncNegotiationCount()
{
    negotiationCount_++;
}

bool SyncTaskContext::IsNeedTriggerQueryAutoSync(Message *inMsg, QuerySyncObject &query)
{
    return stateMachine_->IsNeedTriggerQueryAutoSync(inMsg, query);
}

bool SyncTaskContext::IsAutoSubscribe() const
{
    return isAutoSubscribe_;
}

bool SyncTaskContext::IsCurrentSyncTaskCanBeSkipped() const
{
    return false;
}

void SyncTaskContext::ResetLastPushTaskStatus()
{
}

void SyncTaskContext::SchemaChange()
{
    if (stateMachine_ != nullptr) {
        stateMachine_->SchemaChange();
    }
}

void SyncTaskContext::Dump(int fd)
{
    size_t totalSyncTaskCount = 0u;
    size_t autoSyncTaskCount = 0u;
    size_t reponseTaskCount = 0u;
    {
        std::lock_guard<std::mutex> lock(targetQueueLock_);
        totalSyncTaskCount = requestTargetQueue_.size() + responseTargetQueue_.size();
        for (const auto &target : requestTargetQueue_) {
            if (target->IsAutoSync()) { // LCOV_EXCL_BR_LINE
                autoSyncTaskCount++;
            }
        }
        reponseTaskCount = responseTargetQueue_.size();
    }
    DBDumpHelper::Dump(fd, "\t\ttarget = %s, total sync task count = %zu, auto sync task count = %zu,"
        " response task count = %zu\n",
        deviceId_.c_str(), totalSyncTaskCount, autoSyncTaskCount, reponseTaskCount);
}

int SyncTaskContext::RunPermissionCheck(uint8_t flag) const
{
    std::string appId = syncInterface_->GetDbProperties().GetStringProp(DBProperties::APP_ID, "");
    std::string userId = syncInterface_->GetDbProperties().GetStringProp(DBProperties::USER_ID, "");
    std::string storeId = syncInterface_->GetDbProperties().GetStringProp(DBProperties::STORE_ID, "");
    std::string subUserId = syncInterface_->GetDbProperties().GetStringProp(DBProperties::SUB_USER, "");
    int32_t instanceId = syncInterface_->GetDbProperties().GetIntProp(DBProperties::INSTANCE_ID, 0);
    int errCode = RuntimeContext::GetInstance()->RunPermissionCheck(
        { userId, appId, storeId, deviceId_, subUserId, instanceId }, flag);
    if (errCode != E_OK) {
        LOGE("[SyncTaskContext] RunPermissionCheck not pass errCode:%d, flag:%d, %s{private}",
            errCode, flag, deviceId_.c_str());
    }
    return errCode;
}

uint8_t SyncTaskContext::GetPermissionCheckFlag(bool isAutoSync, int syncMode)
{
    uint8_t flag = 0;
    int mode = SyncOperation::TransferSyncMode(syncMode);
    if (mode == SyncModeType::PUSH || mode == SyncModeType::RESPONSE_PULL) {
        flag = CHECK_FLAG_SEND;
    } else if (mode == SyncModeType::PULL) {
        flag = CHECK_FLAG_RECEIVE;
    } else if (mode == SyncModeType::PUSH_AND_PULL) {
        flag = CHECK_FLAG_SEND | CHECK_FLAG_RECEIVE;
    }
    if (isAutoSync) {
        flag = flag | CHECK_FLAG_AUTOSYNC;
    }
    if (mode != SyncModeType::RESPONSE_PULL) {
        // it means this sync is started by local
        flag = flag | CHECK_FLAG_SPONSOR;
    }
    return flag;
}

void SyncTaskContext::AbortMachineIfNeed(uint32_t syncId)
{
    uint32_t sessionId = 0u;
    {
        RefObject::AutoLock autoLock(this);
        if (syncId_ != syncId) {
            return;
        }
        sessionId = requestSessionId_;
    }
    stateMachine_->InnerErrorAbort(sessionId);
}

SyncOperation *SyncTaskContext::GetAndIncSyncOperation() const
{
    std::lock_guard<std::mutex> lock(operationLock_);
    if (syncOperation_ == nullptr) {
        return nullptr;
    }
    RefObject::IncObjRef(syncOperation_);
    return syncOperation_;
}

uint32_t SyncTaskContext::GenerateRequestSessionId()
{
    uint32_t sessionId = lastRequestSessionId_ != 0 ? lastRequestSessionId_ + 1 : 0;
    // make sure sessionId is between 0x01 and 0x8fffffff
    if (sessionId > SESSION_ID_MAX_VALUE || sessionId == 0) {
        sessionId = Hash::Hash32Func(deviceId_ + std::to_string(syncId_) +
            std::to_string(TimeHelper::GetSysCurrentTime()));
    }
    lastRequestSessionId_ = sessionId;
    return sessionId;
}

bool SyncTaskContext::IsSchemaCompatible() const
{
    return true;
}

void SyncTaskContext::SetDbAbility([[gnu::unused]] DbAbility &remoteDbAbility)
{
}

void SyncTaskContext::TimeChange()
{
    if (stateMachine_ == nullptr) {
        LOGW("[SyncTaskContext] machine is null when time change");
        return;
    }
    stateMachine_->TimeChange();
}

int32_t SyncTaskContext::GetResponseTaskCount()
{
    std::lock_guard<std::mutex> autoLock(targetQueueLock_);
    return static_cast<int32_t>(responseTargetQueue_.size());
}

int SyncTaskContext::GetCommErrCode() const
{
    return commErrCode_;
}

void SyncTaskContext::SetCommFailErrCode(int errCode)
{
    commErrCode_ = errCode;
}

void SyncTaskContext::SetErrCodeWhenWaitTimeOut(int errCode)
{
    if (errCode > 0) {
        SetCommFailErrCode(static_cast<int>(SyncOperation::OP_TIMEOUT));
    } else {
        SetCommFailErrCode(errCode);
    }
}
} // namespace DistributedDB
