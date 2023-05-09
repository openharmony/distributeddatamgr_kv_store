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

#include "remote_executor.h"

#include "db_constant.h"
#include "db_common.h"
#include "hash.h"
#include "prepared_stmt.h"
#include "semaphore_utils.h"
#include "sync_generic_interface.h"
#include "sync_types.h"
#include "time_helper.h"
#include "version.h"

namespace DistributedDB {
namespace {
    constexpr uint32_t MAX_SEARCH_TASK_EXECUTE = 2;
    constexpr uint32_t MAX_SEARCH_TASK_PER_DEVICE = 5;
    constexpr uint32_t MAX_QUEUE_COUNT = 10;
    constexpr uint32_t REMOTE_EXECUTOR_SEND_TIME_OUT = 3000; // 3S

    void ReleaseMessageAndPacket(Message *message, ISyncPacket *packet)
    {
        delete message;
        message = nullptr;
        delete packet;
        packet = nullptr;
    }
}

RemoteExecutor::RemoteExecutor()
    : workingThreadsCount_(0),
      syncInterface_(nullptr),
      communicator_(nullptr),
      lastSessionId_(0),
      lastTaskId_(0),
      closed_(false)
{
}

int RemoteExecutor::Initialize(ISyncInterface *syncInterface, ICommunicator *communicator)
{
    if (syncInterface == nullptr || communicator == nullptr) {
        return -E_INVALID_ARGS;
    }
    closed_ = false;
    std::lock_guard<std::mutex> autoLock(innerSourceLock_);
    syncInterface_ = syncInterface;
    communicator_ = communicator;
    return E_OK;
}

int RemoteExecutor::RemoteQuery(const std::string device, const RemoteCondition &condition,
    uint64_t timeout, uint64_t connectionId, std::shared_ptr<ResultSet> &result)
{
    if (closed_) {
        return -E_BUSY;
    }
    if (!CheckParamValid(device, timeout)) {
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    SemaphoreUtils semaphore(0);
    Task task;
    task.result = std::make_shared<RelationalResultSetImpl>();
    task.target = device;
    task.timeout = timeout;
    task.condition = condition;
    task.onFinished = [&semaphore, &errCode, &result](int32_t retCode, std::shared_ptr<ResultSet> taskResult) {
        errCode = retCode;
        result = taskResult;
        semaphore.SendSemaphore();
    };
    task.connectionId = connectionId;
    errCode = RemoteQueryInner(task);
    if (errCode != E_OK) {
        return errCode;
    }
    semaphore.WaitSemaphore();
    return errCode;
}

int RemoteExecutor::ReceiveMessage(const std::string &targetDev, Message *inMsg)
{
    if (inMsg == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (closed_) {
        LOGD("[RemoteExecutor][ReceiveMessageInner] db is closing ignore msg");
        delete inMsg;
        inMsg = nullptr;
        return -E_BUSY;
    }
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, targetDev, inMsg]() {
        ReceiveMessageInner(targetDev, inMsg);
        RefObject::DecObjRef(this);
    });
    if (errCode != E_OK) {
        RefObject::DecObjRef(this);
    }
    return errCode;
}

void RemoteExecutor::NotifyDeviceOffline(const std::string &device)
{
    if (closed_) {
        return;
    }
    LOGD("[RemoteExecutor][NotifyDeviceOffline] device=%s{private} offline", device.c_str());
    std::vector<uint32_t> removeList;
    RemoveTaskByDevice(device, removeList);
    for (const auto &sessionId : removeList) {
        DoFinished(sessionId, -E_PERIPHERAL_INTERFACE_FAIL);
    }
}

void RemoteExecutor::NotifyUserChange()
{
    if (closed_) {
        return;
    }
    LOGD("[RemoteExecutor][NotifyUserChange] userchange enter");
    RemoveAllTask(-E_USER_CHANGE);
    LOGD("[RemoteExecutor][NotifyUserChange] userchange exist");
}

void RemoteExecutor::Close()
{
    closed_ = true;
    LOGD("[RemoteExecutor][Close] close enter");
    RemoveAllTask(-E_BUSY);
    ClearInnerSource();
    {
        std::unique_lock<std::mutex> lock(msgQueueLock_);
        clearCV_.wait(lock, [this] { return workingThreadsCount_ == 0; });
    }
    LOGD("[RemoteExecutor][Close] close exist");
}

void RemoteExecutor::NotifyConnectionClosed(uint64_t connectionId)
{
    if (closed_) {
        return;
    }
    std::vector<uint32_t> removeList;
    RemoveTaskByConnection(connectionId, removeList);
    for (const auto &sessionId : removeList) {
        DoFinished(sessionId, -E_BUSY);
    }
}

int RemoteExecutor::ReceiveRemoteExecutorRequest(const std::string &targetDev, Message *inMsg)
{
    LOGD("[RemoteExecutor][ReceiveRemoteExecutorRequest] receive request");
    {
        std::lock_guard<std::mutex> autoLock(msgQueueLock_);
        searchMessageQueue_.push(std::make_pair(targetDev, inMsg));
        if (workingThreadsCount_ + 1 > MAX_SEARCH_TASK_EXECUTE) {
            // message deal in work thread, exist here
            return -E_NOT_NEED_DELETE_MSG;
        }
        workingThreadsCount_++;
    }
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this]() {
        bool empty = true;
        do {
            std::pair<std::string, Message *> entry;
            {
                std::lock_guard<std::mutex> autoLock(msgQueueLock_);
                empty = searchMessageQueue_.empty();
                if (empty) {
                    workingThreadsCount_--;
                    continue;
                }
                entry = searchMessageQueue_.front();
                searchMessageQueue_.pop();
            }
            ParseOneRequestMessage(entry.first, entry.second);
            delete entry.second;
            entry.second = nullptr;
        } while (!empty);
        clearCV_.notify_one();
        RefObject::DecObjRef(this);
    });
    if (errCode != E_OK) {
        workingThreadsCount_--;
        clearCV_.notify_one();
        RefObject::DecObjRef(this);
    } else {
        errCode = -E_NOT_NEED_DELETE_MSG;
    }
    return errCode;
}

void RemoteExecutor::ParseOneRequestMessage(const std::string &device, Message *inMsg)
{
    if (closed_) {
        LOGW("[RemoteExecutor][ParseOneRequestMessage] closed");
        return;
    }
    int errCode = CheckPermissions(device, inMsg);
    if (errCode != E_OK) {
        ResponseFailed(errCode, inMsg->GetSessionId(), inMsg->GetSequenceId(), device);
        return;
    }
    errCode = SendRemoteExecutorData(device, inMsg);
    if (errCode != E_OK) {
        ResponseFailed(errCode, inMsg->GetSessionId(), inMsg->GetSequenceId(), device);
    }
}

int RemoteExecutor::CheckPermissions(const std::string &device, Message *inMsg)
{
    SyncGenericInterface *storage = static_cast<SyncGenericInterface *>(GetAndIncSyncInterface());
    if (storage == nullptr) {
        LOGE("[RemoteExecutor][CheckPermissions] storage is nullptr.");
        return -E_BUSY;
    }
    // permission check
    std::string appId = storage->GetDbProperties().GetStringProp(DBProperties::APP_ID, "");
    std::string userId = storage->GetDbProperties().GetStringProp(DBProperties::USER_ID, "");
    std::string storeId = storage->GetDbProperties().GetStringProp(DBProperties::STORE_ID, "");
    int32_t instanceId = syncInterface_->GetDbProperties().GetIntProp(DBProperties::INSTANCE_ID, 0);
    int errCode = RuntimeContext::GetInstance()->RunPermissionCheck(
        { userId, appId, storeId, device, instanceId }, CHECK_FLAG_SEND);
    if (errCode != E_OK) {
        LOGE("[RemoteExecutor][CheckPermissions] check permission errCode = %d.", errCode);
        storage->DecRefCount();
        return errCode;
    }
    const auto *requestPacket = inMsg->GetObject<RemoteExecutorRequestPacket>();
    if (requestPacket == nullptr) {
        LOGE("[RemoteExecutor] get packet object failed");
        storage->DecRefCount();
        return -E_INVALID_ARGS;
    }
    errCode = CheckRemoteRecvData(device, storage, requestPacket->GetSecLabel());
    storage->DecRefCount();
    return errCode;
}

int RemoteExecutor::SendRemoteExecutorData(const std::string &device, const Message *inMsg)
{
    auto *syncInterface = GetAndIncSyncInterface();
    if (syncInterface == nullptr) {
        LOGE("[RemoteExecutor][ParseOneRequestMessage] storage is nullptr.");
        return -E_INVALID_ARGS;
    }
    if (syncInterface->GetInterfaceType() != ISyncInterface::SYNC_RELATION) {
        LOGE("[RemoteExecutor][ParseOneRequestMessage] storage is not relation.");
        syncInterface->DecRefCount();
        return -E_NOT_SUPPORT;
    }
    RelationalDBSyncInterface *storage = static_cast<RelationalDBSyncInterface *>(syncInterface);

    const RemoteExecutorRequestPacket *requestPacket = inMsg->GetObject<RemoteExecutorRequestPacket>();
    if (requestPacket == nullptr) {
        LOGE("[RemoteExecutor] get packet object failed");
        storage->DecRefCount();
        return -E_INVALID_ARGS;
    }

    int errCode = ResponseRemoteQueryRequest(storage, requestPacket->GetPreparedStmt(), device, inMsg->GetSessionId());
    storage->DecRefCount();
    return errCode;
}

int RemoteExecutor::ReceiveRemoteExecutorAck(const std::string &targetDev, Message *inMsg)
{
    auto *packet = inMsg->GetObject<RemoteExecutorAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = packet->GetAckCode();
    uint32_t sessionId = inMsg->GetSessionId();
    uint32_t sequenceId = inMsg->GetSequenceId();
    if (!IsPacketValid(sessionId)) {
        LOGD("[RemoteExecutor][ReceiveRemoteExecutorAck] receive unknown ack");
        return -E_INVALID_ARGS;
    }
    if (errCode == E_OK) {
        auto storage = GetAndIncSyncInterface();
        auto communicator = GetAndIncCommunicator();
        errCode = CheckSecurityOption(storage, communicator, packet->GetSecurityOption());
        if (storage != nullptr) {
            storage->DecRefCount();
        }
        RefObject::DecObjRef(communicator);
    }
    if (errCode != E_OK) {
        DoFinished(sessionId, errCode);
    } else {
        ReceiveDataWithValidSession(targetDev, sessionId, sequenceId, packet);
    }
    return E_OK;
}

bool RemoteExecutor::CheckParamValid(const std::string &device, uint64_t timeout) const
{
    if (timeout < DBConstant::MIN_TIMEOUT || timeout > DBConstant::MAX_TIMEOUT) {
        LOGD("[RemoteExecutor][CheckParamValid] timeout=invalid %" PRIu64, timeout);
        return false;
    }
    if (device.empty()) {
        LOGD("[RemoteExecutor][CheckParamValid] device is empty");
        return false;
    }
    ICommunicator *communicator = GetAndIncCommunicator();
    if (communicator == nullptr) {
        return false;
    }
    std::string localId;
    int errCode = communicator->GetLocalIdentity(localId);
    RefObject::DecObjRef(communicator);
    if (errCode != E_OK) {
        return false;
    }
    if (localId == device) {
        LOGD("[RemoteExecutor][CheckParamValid] cannot sync to self");
        return false;
    }
    return true;
}

bool RemoteExecutor::CheckTaskExeStatus(const std::string &device)
{
    uint32_t queueCount = 0u;
    uint32_t exeTaskCount = 0u;
    uint32_t totalCount = 0u; // waiting task count in all queue
    if (searchTaskQueue_.find(device) != searchTaskQueue_.end()) {
        queueCount = searchTaskQueue_.at(device).size();
    }
    if (deviceWorkingSet_.find(device) != deviceWorkingSet_.end()) {
        exeTaskCount = deviceWorkingSet_.at(device).size();
    }
    for (auto &entry : searchTaskQueue_) {
        int currentExeCount = static_cast<int>(deviceWorkingSet_[device].size());
        int currentQueueCount = static_cast<int>(entry.second.size());
        if ((currentQueueCount + currentExeCount) < static_cast<int>(MAX_SEARCH_TASK_PER_DEVICE)) {
            // all task in this queue can execute, no need calculate as waiting task count
            continue;
        }
        totalCount += static_cast<uint32_t>(currentQueueCount + currentExeCount -
            static_cast<int>(MAX_SEARCH_TASK_PER_DEVICE));
    }
    return (queueCount + exeTaskCount + 1 <= MAX_SEARCH_TASK_PER_DEVICE + MAX_SEARCH_TASK_EXECUTE) &&
        (totalCount + 1 <= MAX_QUEUE_COUNT);
}

uint32_t RemoteExecutor::GenerateSessionId()
{
    uint32_t sessionId = Hash::Hash32Func(std::to_string(TimeHelper::GetSysCurrentTime()));
    while (taskMap_.find(sessionId) != taskMap_.end()) {
        sessionId++;
        if (sessionId == 0) { // if over flow start with 1
            sessionId++;
        }
    }
    lastSessionId_ = sessionId;
    return sessionId;
}

uint32_t RemoteExecutor::GenerateTaskId()
{
    lastTaskId_++;
    if (lastTaskId_ == 0) { // if over flow start with 1
        lastTaskId_++;
    }
    return lastTaskId_;
}

int RemoteExecutor::RemoteQueryInner(const Task &task)
{
    uint32_t sessionId = 0u;
    {
        // check task count and push into queue in lock
        std::lock_guard<std::mutex> autoLock(taskLock_);
        if (!CheckTaskExeStatus(task.target)) {
            LOGE("[RemoteExecutor][RemoteQueryInner] queue size is over limit");
            return -E_MAX_LIMITS;
        }
        sessionId = GenerateSessionId();
        searchTaskQueue_[task.target].push_back(sessionId);
        if (taskMap_.find(sessionId) != taskMap_.end()) {
            LOGE("[RemoteExecutor][RemoteQueryInner] task already exist");
            return -E_INTERNAL_ERROR; // should not happen
        }
        taskMap_[sessionId] = task;
        taskMap_[sessionId].taskId = GenerateTaskId();
        LOGD("[RemoteExecutor][RemoteQuery] RemoteQuery create task taskId=%" PRIu32 " target is %s",
            taskMap_[sessionId].taskId, task.target.c_str());
    }
    std::string device = task.target;
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, device]() {
        TryExecuteTaskInLock(device);
        RefObject::DecObjRef(this);
    });
    if (errCode != E_OK) {
        LOGD("[RemoteExecutor][RemoteQueryInner] Schedule task failed try to roll back");
        DoRollBack(sessionId);
        RefObject::DecObjRef(this);
    }
    return errCode;
}

void RemoteExecutor::TryExecuteTaskInLock(const std::string &device)
{
    uint32_t sessionId = 0u;
    {
        std::lock_guard<std::mutex> autoLock(taskLock_);
        if (deviceWorkingSet_[device].size() >= MAX_SEARCH_TASK_EXECUTE) {
            return;
        }
        if (searchTaskQueue_[device].empty()) {
            LOGD("[RemoteExecutor][TryExecuteTaskInLock] no task to execute");
            return;
        }
        sessionId = searchTaskQueue_[device].front();
        if (taskMap_.find(sessionId) == taskMap_.end()) {
            searchTaskQueue_[device].pop_front();
            LOGD("[RemoteExecutor][TryExecuteTaskInLock] task was removed no need execute");
            return;
        }
        taskMap_[sessionId].status = Status::WORKING;
        searchTaskQueue_[device].pop_front();
        deviceWorkingSet_[device].insert(sessionId);
        LOGD("[RemoteExecutor][RemoteQuery] RemoteQuery execute taskId=%" PRIu32, taskMap_[sessionId].taskId);
        StartTimer(taskMap_[sessionId].timeout, sessionId);
    }
    int errCode = RequestStart(sessionId);
    if (errCode != E_OK) {
        DoFinished(sessionId, errCode);
    }
}

void RemoteExecutor::DoRollBack(uint32_t sessionId)
{
    Task task;
    std::lock_guard<std::mutex> autoLock(taskLock_);
    if (taskMap_.find(sessionId) == taskMap_.end()) {
        return;
    }
    task = taskMap_[sessionId];
    if (task.status != Status::WAITING) {
        // task is execute, abort roll back
        return;
    }
    taskMap_.erase(sessionId);

    auto iter = searchTaskQueue_[task.target].begin();
    while (iter != searchTaskQueue_[task.target].end()) {
        if ((*iter) == sessionId) {
            break;
        }
        iter++;
    }
    if (iter != searchTaskQueue_[task.target].end()) {
        searchTaskQueue_[task.target].erase(iter);
    }
    // this task should not in workingSet
    deviceWorkingSet_[task.target].erase(sessionId);
}

int RemoteExecutor::RequestStart(uint32_t sessionId)
{
    Message *message = new (std::nothrow) Message(REMOTE_EXECUTE_MESSAGE);
    if (message == nullptr) {
        LOGE("[RemoteExecutor][RequestStart] new message error");
        return -E_OUT_OF_MEMORY;
    }
    message->SetSessionId(sessionId);
    message->SetMessageType(TYPE_REQUEST);
    auto *packet = new (std::nothrow) RemoteExecutorRequestPacket();
    if (packet == nullptr) {
        LOGE("[RemoteExecutor][RequestStart] new packet error");
        ReleaseMessageAndPacket(message, nullptr);
        return -E_OUT_OF_MEMORY;
    }
    std::string target;
    int errCode = FillRequestPacket(packet, sessionId, target);
    if (errCode != E_OK) {
        ReleaseMessageAndPacket(message, packet);
        return errCode;
    }
    errCode = message->SetExternalObject(packet);
    if (errCode != E_OK) {
        ReleaseMessageAndPacket(message, packet);
        LOGE("[RemoteExecutor][RequestStart] set external object failed errCode=%d", errCode);
    }
    auto communicator = GetAndIncCommunicator();
    auto syncInterface = GetAndIncSyncInterface();
    if (communicator == nullptr || syncInterface == nullptr) {
        ReleaseMessageAndPacket(message, nullptr);
        if (syncInterface != nullptr) {
            syncInterface->DecRefCount();
        }
        RefObject::DecObjRef(communicator);
        return -E_BUSY;
    }
    SendConfig sendConfig;
    SetSendConfigParam(syncInterface->GetDbProperties(), target, false, REMOTE_EXECUTOR_SEND_TIME_OUT, sendConfig);
    RefObject::IncObjRef(this);
    errCode = communicator->SendMessage(target, message, sendConfig, [this, sessionId](int errCode) {
        if (errCode != E_OK) {
            DoSendFailed(sessionId, errCode);
        }
        RefObject::DecObjRef(this);
    });
    RefObject::DecObjRef(communicator);
    syncInterface->DecRefCount();
    return errCode;
}

void RemoteExecutor::ResponseFailed(int errCode, uint32_t sessionId, uint32_t sequenceId,
    const std::string &device)
{
    RemoteExecutorAckPacket *packet = new (std::nothrow) RemoteExecutorAckPacket();
    if (packet == nullptr) {
        LOGE("[RemoteExecutor][ResponseFailed] new RemoteExecutorAckPacket error");
        return;
    }
    packet->SetAckCode(errCode);
    packet->SetLastAck();
    (void)ResponseStart(packet, sessionId, sequenceId, device);
}

int RemoteExecutor::ResponseData(RelationalRowDataSet &&dataSet, const SendMessage &sendMessage,
    const std::string &device)
{
    RemoteExecutorAckPacket *packet = new (std::nothrow) RemoteExecutorAckPacket();
    if (packet == nullptr) {
        LOGE("[RemoteExecutor][ResponseFailed] new RemoteExecutorAckPacket error");
        return -E_OUT_OF_MEMORY;
    }
    packet->SetAckCode(E_OK);
    if (sendMessage.isLast) {
        packet->SetLastAck();
    }
    packet->SetSecurityOption(sendMessage.option);
    packet->MoveInRowDataSet(std::move(dataSet));
    return ResponseStart(packet, sendMessage.sessionId, sendMessage.sequenceId, device);
}

int RemoteExecutor::ResponseStart(RemoteExecutorAckPacket *packet, uint32_t sessionId, uint32_t sequenceId,
    const std::string &device)
{
    SyncGenericInterface *storage = static_cast<SyncGenericInterface *>(GetAndIncSyncInterface());
    if (storage == nullptr) {
        ReleaseMessageAndPacket(nullptr, packet);
        LOGE("[RemoteExecutor][ResponseStart] storage is nullptr.");
        return -E_BUSY;
    }
    Message *message = new (std::nothrow) Message(REMOTE_EXECUTE_MESSAGE);
    if (message == nullptr) {
        LOGE("[RemoteExecutor][ResponseStart] new message error");
        storage->DecRefCount();
        ReleaseMessageAndPacket(nullptr, packet);
        return -E_OUT_OF_MEMORY;
    }
    packet->SetVersion(RemoteExecutorAckPacket::RESPONSE_PACKET_VERSION_CURRENT);

    int errCode = message->SetExternalObject(packet);
    if (errCode != E_OK) {
        ReleaseMessageAndPacket(message, packet);
        storage->DecRefCount();
        LOGE("[RemoteExecutor][ResponseStart] set external object failed errCode: %d", errCode);
        return errCode;
    }
    auto *communicator = GetAndIncCommunicator();
    if (communicator == nullptr) {
        ReleaseMessageAndPacket(message, nullptr);
        storage->DecRefCount();
        LOGD("[RemoteExecutor][ResponseStart] communicator is nullptr");
        return -E_BUSY;
    }

    message->SetTarget(device);
    message->SetSessionId(sessionId);
    message->SetSequenceId(sequenceId);
    message->SetMessageType(TYPE_RESPONSE);
    SendConfig sendConfig;
    SetSendConfigParam(storage->GetDbProperties(), device, false, SEND_TIME_OUT, sendConfig);
    errCode = communicator->SendMessage(device, message, sendConfig, nullptr);
    RefObject::DecObjRef(communicator);
    if (errCode != E_OK) {
        ReleaseMessageAndPacket(message, nullptr);
        LOGE("[RemoteExecutor][ParseOneRequestMessage] send message failed, errCode: %d", errCode);
    }
    storage->DecRefCount();
    return errCode;
}

void RemoteExecutor::StartTimer(uint64_t timeout, uint32_t sessionId)
{
    TimerId timerId = 0u;
    RefObject::IncObjRef(this);
    TimerAction timeoutCallBack = std::bind(&RemoteExecutor::TimeoutCallBack, this, std::placeholders::_1);
    int errCode = RuntimeContext::GetInstance()->SetTimer(timeout, timeoutCallBack, [this]() {
        RefObject::DecObjRef(this);
    }, timerId);
    if (errCode != E_OK) {
        RefObject::DecObjRef(this);
        LOGW("[RemoteExecutor][StartTimer] errCode=%d", errCode);
    }
    LOGD("[RemoteExecutor][StartTimer] timerId=%" PRIu64, timerId);
    std::lock_guard<std::mutex> autoLock(timeoutLock_);
    timeoutMap_[timerId] = sessionId;
    taskFinishMap_[sessionId] = timerId;
}

void RemoteExecutor::RemoveTimer(uint32_t sessionId)
{
    TimerId timerId = 0u;
    {
        std::lock_guard<std::mutex> autoLock(timeoutLock_);
        if (taskFinishMap_.find(sessionId) == taskFinishMap_.end()) {
            return;
        }
        timerId = taskFinishMap_[sessionId];
        LOGD("[RemoteExecutor][RemoveTimer] timerId=%" PRIu32, timerId);
        timeoutMap_.erase(timerId);
        taskFinishMap_.erase(sessionId);
    }
    RuntimeContext::GetInstance()->RemoveTimer(timerId);
}

int RemoteExecutor::TimeoutCallBack(TimerId timerId)
{
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, timerId]() {
        DoTimeout(timerId);
        RefObject::DecObjRef(this);
    });
    if (errCode != E_OK) {
        LOGW("[RemoteExecutor][TimeoutCallBack] Schedule task failed");
        RefObject::DecObjRef(this);
    }
    return -E_NO_NEED_TIMER;
}

void RemoteExecutor::DoTimeout(TimerId timerId)
{
    LOGD("[RemoteExecutor][DoTimeout] timerId=%" PRIu64, timerId);
    uint32_t sessionId = 0u;
    {
        std::lock_guard<std::mutex> autoLock(timeoutLock_);
        if (timeoutMap_.find(timerId) == timeoutMap_.end()) {
            return;
        }
        sessionId = timeoutMap_[timerId];
    }
    DoFinished(sessionId, -E_TIMEOUT);
}

void RemoteExecutor::DoSendFailed(uint32_t sessionId, int errCode)
{
    LOGD("[RemoteExecutor][DoSendFailed] send failed errCode=%d", errCode);
    DoFinished(sessionId, -E_PERIPHERAL_INTERFACE_FAIL);
}

void RemoteExecutor::DoFinished(uint32_t sessionId, int errCode)
{
    Task task;
    if (ClearTaskInfo(sessionId, task) == E_OK) {
        LOGD("[RemoteExecutor][DoFinished] taskId=%" PRIu32 " errCode=%d", task.taskId, errCode);
    } else {
        return;
    }
    RefObject::IncObjRef(this);
    if (task.onFinished != nullptr) {
        task.onFinished(errCode, task.result);
        LOGD("[RemoteExecutor][DoFinished] onFinished");
    }
    std::string device = task.target;
    int retCode = RuntimeContext::GetInstance()->ScheduleTask([this, device]() {
        TryExecuteTaskInLock(device);
        RefObject::DecObjRef(this);
    });
    if (retCode != E_OK) {
        LOGD("[RemoteExecutor][RemoteQueryInner] Schedule task failed");
        RefObject::DecObjRef(this);
    }
}

int RemoteExecutor::ClearTaskInfo(uint32_t sessionId, Task &task)
{
    {
        std::lock_guard<std::mutex> autoLock(taskLock_);
        if (taskMap_.find(sessionId) == taskMap_.end()) {
            return -E_NOT_FOUND;
        }
        task = taskMap_[sessionId];
        taskMap_.erase(sessionId);
        deviceWorkingSet_[task.target].erase(sessionId);
    }
    RemoveTimer(sessionId);
    return E_OK;
}

void RemoteExecutor::ClearInnerSource()
{
    {
        std::lock_guard<std::mutex> autoLock(innerSourceLock_);
        syncInterface_ = nullptr;
        communicator_ = nullptr;
    }
    std::lock_guard<std::mutex> autoLock(msgQueueLock_);
    LOGD("[RemoteExecutor][ClearInnerSource] clear message cache now");
    while (!searchMessageQueue_.empty()) {
        auto entry = searchMessageQueue_.front();
        searchMessageQueue_.pop();
        delete entry.second;
        entry.second = nullptr;
    }
}

int RemoteExecutor::FillRequestPacket(RemoteExecutorRequestPacket *packet, uint32_t sessionId, std::string &target)
{
    ISyncInterface *storage = GetAndIncSyncInterface();
    if (storage == nullptr) {
        return -E_BUSY;
    }
    SecurityOption localOption;
    int errCode = storage->GetSecurityOption(localOption);
    storage->DecRefCount();
    storage = nullptr;
    if (errCode != E_OK && errCode != -E_NOT_SUPPORT) {
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    Task task;
    {
        std::lock_guard<std::mutex> autoLock(taskLock_);
        if (taskMap_.find(sessionId) == taskMap_.end()) {
            LOGD("[RemoteExecutor][FillRequestPacket] this task has finished");
            return -E_FINISHED;
        }
        task = taskMap_[sessionId];
    }
    PreparedStmt stmt;
    stmt.SetOpCode(PreparedStmt::ExecutorOperation::QUERY);
    stmt.SetSql(task.condition.sql);
    stmt.SetBindArgs(task.condition.bindArgs);
    packet->SetVersion(RemoteExecutorRequestPacket::REQUEST_PACKET_VERSION_CURRENT);
    packet->SetPreparedStmt(stmt);
    packet->SetNeedResponse();
    packet->SetSecLabel(errCode == E_NOT_SUPPORT ? NOT_SURPPORT_SEC_CLASSIFICATION : localOption.securityLabel);
    target = task.target;
    return E_OK;
}

void RemoteExecutor::ReceiveMessageInner(const std::string &targetDev, Message *inMsg)
{
    int errCode = E_OK;
    if (inMsg->IsFeedbackError() && IsPacketValid(inMsg->GetSessionId())) {
        DoFinished(inMsg->GetSessionId(), -inMsg->GetErrorNo());
        delete inMsg;
        inMsg = nullptr;
        return;
    }
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            errCode = ReceiveRemoteExecutorRequest(targetDev, inMsg);
            break;
        case TYPE_RESPONSE:
            errCode = ReceiveRemoteExecutorAck(targetDev, inMsg);
            break;
        default:
            LOGD("[RemoteExecutor][ReceiveMessageInner] Receive unknown message");
            break;
    }
    if (errCode != -E_NOT_NEED_DELETE_MSG) {
        delete inMsg;
        inMsg = nullptr;
    }
}

bool RemoteExecutor::IsPacketValid(uint32_t sessionId)
{
    std::lock_guard<std::mutex> autoLock(taskLock_);
    return taskMap_.find(sessionId) != taskMap_.end() && taskMap_[sessionId].status == Status::WORKING;
}

void RemoteExecutor::ReceiveDataWithValidSession(const std::string &targetDev, uint32_t sessionId, uint32_t sequenceId,
    const RemoteExecutorAckPacket *packet)
{
    bool isReceiveFinished = false;
    {
        std::lock_guard<std::mutex> autoLock(taskLock_);
        if (taskMap_.find(sessionId) == taskMap_.end() || taskMap_[sessionId].status != Status::WORKING) {
            LOGD("[RemoteExecutor][ReceiveRemoteExecutorAck] receive unknown ack");
            return;
        }
        LOGD("[RemoteExecutor][ReceiveRemoteExecutorAck] taskId=%" PRIu32 " sequenceId=%" PRIu32,
            taskMap_[sessionId].taskId, sequenceId);
        taskMap_[sessionId].currentCount++;
        if (packet->IsLastAck()) {
            taskMap_[sessionId].targetCount = sequenceId;
        }
        taskMap_[sessionId].result->Put(targetDev, sequenceId, packet->MoveOutRowDataSet());
        if (taskMap_[sessionId].currentCount == taskMap_[sessionId].targetCount) {
            isReceiveFinished = true;
        }
    }
    if (isReceiveFinished) {
        DoFinished(sessionId, E_OK);
    }
}

ICommunicator *RemoteExecutor::GetAndIncCommunicator() const
{
    std::lock_guard<std::mutex> autoLock(innerSourceLock_);
    ICommunicator *communicator = communicator_;
    RefObject::IncObjRef(communicator);
    return communicator;
}

ISyncInterface *RemoteExecutor::GetAndIncSyncInterface() const
{
    std::lock_guard<std::mutex> autoLock(innerSourceLock_);
    ISyncInterface *syncInterface = syncInterface_;
    if (syncInterface != nullptr) {
        syncInterface->IncRefCount();
    }
    return syncInterface;
}

void RemoteExecutor::RemoveTaskByDevice(const std::string &device, std::vector<uint32_t> &removeList)
{
    std::lock_guard<std::mutex> autoLock(taskLock_);
    if (deviceWorkingSet_.find(device) != deviceWorkingSet_.end()) {
        for (auto &sessionId : deviceWorkingSet_[device]) {
            removeList.push_back(sessionId);
        }
    }
    if (searchTaskQueue_.find(device) != searchTaskQueue_.end()) {
        for (auto &sessionId : searchTaskQueue_[device]) {
            removeList.push_back(sessionId);
        }
    }
}

void RemoteExecutor::RemoveAllTask(int errCode)
{
    std::vector<OnFinished> waitToNotify;
    std::vector<uint32_t> removeTimerList;
    {
        std::lock_guard<std::mutex> autoLock(taskLock_);
        for (auto &taskEntry : taskMap_) {
            if (taskEntry.second.onFinished != nullptr) {
                waitToNotify.push_back(taskEntry.second.onFinished);
                LOGD("[RemoteExecutor][RemoveAllTask] taskId=%" PRIu32 " result is %d",
                    taskEntry.second.taskId, errCode);
            }
            if (taskEntry.second.status == Status::WORKING) {
                removeTimerList.push_back(taskEntry.first);
            }
        }
        taskMap_.clear();
        deviceWorkingSet_.clear();
        searchTaskQueue_.clear();
    }
    for (const auto &callBack : waitToNotify) {
        callBack(errCode, nullptr);
    }
    for (const auto &sessionId : removeTimerList) {
        RemoveTimer(sessionId);
    }
    std::lock_guard<std::mutex> autoLock(timeoutLock_);
    timeoutMap_.clear();
    taskFinishMap_.clear();
}

void RemoteExecutor::RemoveTaskByConnection(uint64_t connectionId, std::vector<uint32_t> &removeList)
{
    std::lock_guard<std::mutex> autoLock(taskLock_);
    for (auto &entry : taskMap_) {
        if (entry.second.connectionId == connectionId) {
            removeList.push_back(entry.first);
        }
    }
}

int RemoteExecutor::GetPacketSize(const std::string &device, size_t &packetSize)
{
    auto *communicator = GetAndIncCommunicator();
    if (communicator == nullptr) {
        LOGD("communicator is nullptr");
        return -E_BUSY;
    }

    packetSize = communicator->GetCommunicatorMtuSize(device) * 9 / 10; // get the 9/10 of the size
    RefObject::DecObjRef(communicator);
    return E_OK;
}

bool RemoteExecutor::CheckRemoteSecurityOption(const std::string &device, const SecurityOption &remoteOption,
    const SecurityOption &localOption)
{
    bool res = false;
    if (remoteOption.securityLabel == localOption.securityLabel ||
        (remoteOption.securityLabel == SecurityLabel::NOT_SET ||
        localOption.securityLabel == SecurityLabel::NOT_SET)) {
        res = RuntimeContext::GetInstance()->CheckDeviceSecurityAbility(device, remoteOption);
    }
    if (!res) {
        LOGE("[RemoteExecutor][CheckRemoteSecurityOption] check error remote:%d, %d local:%d, %d",
            remoteOption.securityLabel, remoteOption.securityFlag,
            localOption.securityLabel, localOption.securityFlag);
    }
    return res;
}

int RemoteExecutor::ResponseRemoteQueryRequest(RelationalDBSyncInterface *storage, const PreparedStmt &stmt,
    const std::string &device, uint32_t sessionId)
{
    size_t packetSize = 0u;
    int errCode = GetPacketSize(device, packetSize);
    if (errCode != E_OK) {
        return errCode;
    }
    SecurityOption option;
    errCode = storage->GetSecurityOption(option);
    if (errCode == -E_NOT_SUPPORT) {
        option.securityLabel = NOT_SURPPORT_SEC_CLASSIFICATION;
        errCode = E_OK;
    }
    if (errCode != E_OK) {
        LOGD("GetSecurityOption errCode:%d", errCode);
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    ContinueToken token = nullptr;
    uint32_t sequenceId = 1u;
    do {
        RelationalRowDataSet dataSet;
        errCode = storage->ExecuteQuery(stmt, packetSize, dataSet, token);
        if (errCode != E_OK) {
            LOGE("[RemoteExecutor] call ExecuteQuery failed: %d", errCode);
            break;
        }
        SendMessage sendMessage = { sessionId, sequenceId, token == nullptr, option };
        errCode = ResponseData(std::move(dataSet), sendMessage, device);
        if (errCode != E_OK) {
            break;
        }
        sequenceId++;
    } while (token != nullptr);
    if (token != nullptr) {
        storage->ReleaseRemoteQueryContinueToken(token);
    }
    return errCode;
}

int RemoteExecutor::CheckSecurityOption(ISyncInterface *storage, ICommunicator *communicator,
    const SecurityOption &remoteOption)
{
    if (storage == nullptr || communicator == nullptr) {
        return -E_BUSY;
    }
    if (storage->GetInterfaceType() != ISyncInterface::SYNC_RELATION) {
        return -E_NOT_SUPPORT;
    }
    std::string device;
    communicator->GetLocalIdentity(device);
    SecurityOption localOption;
    int errCode = storage->GetSecurityOption(localOption);
    if (errCode != E_OK && errCode != -E_NOT_SUPPORT) {
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    if (remoteOption.securityLabel == NOT_SURPPORT_SEC_CLASSIFICATION || errCode == -E_NOT_SUPPORT) {
        return E_OK;
    }
    if (!CheckRemoteSecurityOption(device, remoteOption, localOption)) {
        errCode = -E_SECURITY_OPTION_CHECK_ERROR;
    } else {
        errCode = E_OK;
    }
    return errCode;
}

int RemoteExecutor::CheckRemoteRecvData(const std::string &device, SyncGenericInterface *storage,
    int32_t remoteSecLabel)
{
    SecurityOption localOption;
    int errCode = storage->GetSecurityOption(localOption);
    if (errCode == -E_NOT_SUPPORT) {
        return E_OK;
    }
    if (errCode != E_OK) {
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    if (remoteSecLabel == UNKNOWN_SECURITY_LABEL || remoteSecLabel == NOT_SURPPORT_SEC_CLASSIFICATION) {
        return E_OK;
    }
    if (RuntimeContext::GetInstance()->CheckDeviceSecurityAbility(device, localOption)) {
        return E_OK;
    }
    return -E_SECURITY_OPTION_CHECK_ERROR;
}
}