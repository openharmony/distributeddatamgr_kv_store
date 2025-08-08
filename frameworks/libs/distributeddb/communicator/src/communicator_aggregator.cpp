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

#include "communicator_aggregator.h"

#include <sstream>
#include "communicator.h"
#include "communicator_linker.h"
#include "db_common.h"
#include "endian_convert.h"
#include "hash.h"
#include "log_print.h"
#include "protocol_proto.h"

namespace DistributedDB {
namespace {
constexpr int MAX_SEND_RETRY = 2;
constexpr int RETRY_TIME_SPLIT = 4;
inline std::string GetThreadId()
{
    std::stringstream stream;
    stream << std::this_thread::get_id();
    return stream.str();
}
}

std::atomic<bool> CommunicatorAggregator::isCommunicatorNotFoundFeedbackEnable_{true};

CommunicatorAggregator::CommunicatorAggregator()
    : shutdown_(false),
      incFrameId_(0),
      localSourceId_(0)
{
}

CommunicatorAggregator::~CommunicatorAggregator()
{
    scheduler_.Finalize(); // Clear residual frame dumped by linker after CommunicatorAggregator finalize
    adapterHandle_ = nullptr;
    commLinker_ = nullptr;
}

int CommunicatorAggregator::Initialize(IAdapter *inAdapter, const std::shared_ptr<DBStatusAdapter> &statusAdapter)
{
    if (inAdapter == nullptr) {
        return -E_INVALID_ARGS;
    }
    adapterHandle_ = inAdapter;

    combiner_.Initialize();
    retainer_.Initialize();
    scheduler_.Initialize();

    int errCode;
    commLinker_ = new (std::nothrow) CommunicatorLinker(this, statusAdapter);
    if (commLinker_ == nullptr) {
        errCode = -E_OUT_OF_MEMORY;
        goto ROLL_BACK;
    }
    commLinker_->Initialize();

    errCode = RegCallbackToAdapter();
    if (errCode != E_OK) {
        goto ROLL_BACK;
    }

    errCode = adapterHandle_->StartAdapter();
    if (errCode != E_OK) {
        LOGE("[CommAggr][Init] Start Adapter Fail, errCode=%d.", errCode);
        goto ROLL_BACK;
    }
    GenerateLocalSourceId();

    shutdown_ = false;
    InitSendThread();
    dbStatusAdapter_ = statusAdapter;
    RegDBChangeCallback();
    return E_OK;
ROLL_BACK:
    UnRegCallbackFromAdapter();
    if (commLinker_ != nullptr) {
        RefObject::DecObjRef(commLinker_); // Refcount of linker is 1 when created, here to unref linker
        commLinker_ = nullptr;
    }
    // Scheduler do not need to do finalize in this roll_back
    retainer_.Finalize();
    combiner_.Finalize();
    return errCode;
}

void CommunicatorAggregator::Finalize()
{
    shutdown_ = true;
    retryCv_.notify_all();
    {
        std::lock_guard<std::mutex> wakingLockGuard(wakingMutex_);
        wakingSignal_ = true;
        wakingCv_.notify_one();
    }
    if (useExclusiveThread_) {
        exclusiveThread_.join(); // Waiting thread to thoroughly quit
        LOGI("[CommAggr][Final] Sub Thread Exit.");
    } else {
        LOGI("[CommAggr][Final] Begin wait send task exit.");
        std::unique_lock<std::mutex> scheduleSendTaskLock(scheduleSendTaskMutex_);
        finalizeCv_.wait(scheduleSendTaskLock, [this]() {
            return !sendTaskStart_;
        });
        LOGI("[CommAggr][Final] End wait send task exit.");
    }
    scheduler_.Finalize(); // scheduler_ must finalize here to make space for linker to dump residual frame

    adapterHandle_->StopAdapter();
    UnRegCallbackFromAdapter();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait 100 ms to make sure all callback thread quit

    // No callback now and later, so combiner, retainer and linker can finalize or delete safely
    RefObject::DecObjRef(commLinker_); // Refcount of linker is 1 when created, here to unref linker
    commLinker_ = nullptr;
    retainer_.Finalize();
    combiner_.Finalize();
    dbStatusAdapter_ = nullptr;
}

ICommunicator *CommunicatorAggregator::AllocCommunicator(uint64_t commLabel, int &outErrorNo, const std::string &userId)
{
    uint64_t netOrderLabel = HostToNet(commLabel);
    uint8_t *eachByte = reinterpret_cast<uint8_t *>(&netOrderLabel);
    std::vector<uint8_t> realLabel(COMM_LABEL_LENGTH, 0);
    for (int i = 0; i < static_cast<int>(sizeof(uint64_t)); i++) {
        realLabel[i] = eachByte[i];
    }
    return AllocCommunicator(realLabel, outErrorNo, userId);
}

ICommunicator *CommunicatorAggregator::AllocCommunicator(const std::vector<uint8_t> &commLabel, int &outErrorNo,
    const std::string &userId)
{
    std::lock_guard<std::mutex> commMapLockGuard(commMapMutex_);
    LOGI("[CommAggr][Alloc] Label=%.3s.", VEC_TO_STR(commLabel));
    if (commLabel.size() != COMM_LABEL_LENGTH) {
        outErrorNo = -E_INVALID_ARGS;
        return nullptr;
    }

    if (commMap_.count(userId) != 0 && commMap_[userId].count(commLabel) != 0) {
        outErrorNo = -E_ALREADY_ALLOC;
        return nullptr;
    }

    Communicator *commPtr = new(std::nothrow) Communicator(this, commLabel);
    if (commPtr == nullptr) {
        LOGE("[CommAggr][Alloc] Communicator create failed, may be no available memory.");
        outErrorNo = -E_OUT_OF_MEMORY;
        return nullptr;
    }
    commMap_[userId][commLabel] = {commPtr, false}; // Communicator is not activated when allocated
    return commPtr;
}

void CommunicatorAggregator::ReleaseCommunicator(ICommunicator *inCommunicator, const std::string &userId)
{
    if (inCommunicator == nullptr) {
        return;
    }
    Communicator *commPtr = static_cast<Communicator *>(inCommunicator);
    LabelType commLabel = commPtr->GetCommunicatorLabel();
    LOGI("[CommAggr][Release] Label=%.3s.", VEC_TO_STR(commLabel));

    std::lock_guard<std::mutex> commMapLockGuard(commMapMutex_);
    if (commMap_.count(userId) == 0 || commMap_[userId].count(commLabel) == 0) {
        LOGE("[CommAggr][Release] Not Found.");
        return;
    }
    commMap_[userId].erase(commLabel);
    if (commMap_[userId].empty()) {
        commMap_.erase(userId);
    }
    RefObject::DecObjRef(commPtr); // Refcount of Communicator is 1 when created, here to unref Communicator

    int errCode = commLinker_->DecreaseLocalLabel(commLabel);
    if (errCode != E_OK) {
        LOGE("[CommAggr][Release] DecreaseLocalLabel Fail, Just Log, errCode=%d.", errCode);
    }
}

int CommunicatorAggregator::RegCommunicatorLackCallback(const CommunicatorLackCallback &onCommLack,
    const Finalizer &inOper)
{
    std::lock_guard<std::mutex> onCommLackLockGuard(onCommLackMutex_);
    return RegCallBack(onCommLack, onCommLackHandle_, inOper, onCommLackFinalizer_);
}

int CommunicatorAggregator::RegOnConnectCallback(const OnConnectCallback &onConnect, const Finalizer &inOper)
{
    std::lock_guard<std::mutex> onConnectLockGuard(onConnectMutex_);
    int errCode = RegCallBack(onConnect, onConnectHandle_, inOper, onConnectFinalizer_);
    if (onConnect && errCode == E_OK) {
        // Register action and success
        std::set<std::string> onlineTargets = commLinker_->GetOnlineRemoteTarget();
        for (auto &entry : onlineTargets) {
            LOGI("[CommAggr][RegConnect] Online target=%s{private}.", entry.c_str());
            onConnectHandle_(entry, true);
        }
    }
    return errCode;
}

uint32_t CommunicatorAggregator::GetCommunicatorAggregatorMtuSize() const
{
    return adapterHandle_->GetMtuSize() - ProtocolProto::GetLengthBeforeSerializedData();
}

uint32_t CommunicatorAggregator::GetCommunicatorAggregatorMtuSize(const std::string &target) const
{
    return adapterHandle_->GetMtuSize(target) - ProtocolProto::GetLengthBeforeSerializedData();
}

uint32_t CommunicatorAggregator::GetCommunicatorAggregatorTimeout() const
{
    return adapterHandle_->GetTimeout();
}

uint32_t CommunicatorAggregator::GetCommunicatorAggregatorTimeout(const std::string &target) const
{
    return adapterHandle_->GetTimeout(target);
}

bool CommunicatorAggregator::IsDeviceOnline(const std::string &device) const
{
    return adapterHandle_->IsDeviceOnline(device);
}

int CommunicatorAggregator::GetLocalIdentity(std::string &outTarget) const
{
    return adapterHandle_->GetLocalIdentity(outTarget);
}

void CommunicatorAggregator::ActivateCommunicator(const LabelType &commLabel, const std::string &userId)
{
    std::lock_guard<std::mutex> commMapLockGuard(commMapMutex_);
    LOGI("[CommAggr][Activate] Label=%.3s.", VEC_TO_STR(commLabel));
    if (commMap_[userId].count(commLabel) == 0) {
        LOGW("[CommAggr][Activate] Communicator of this label not allocated.");
        return;
    }
    if (commMap_[userId].at(commLabel).second) {
        return;
    }
    commMap_[userId].at(commLabel).second = true; // Mark this communicator as activated

    // IncreaseLocalLabel below and DecreaseLocalLabel in ReleaseCommunicator should all be protected by commMapMutex_
    // To avoid disordering probably caused by concurrent call to ActivateCommunicator and ReleaseCommunicator
    std::set<std::string> onlineTargets;
    int errCode = commLinker_->IncreaseLocalLabel(commLabel, onlineTargets);
    if (errCode != E_OK) {
        LOGE("[CommAggr][Activate] IncreaseLocalLabel Fail, Just Log, errCode=%d.", errCode);
        // Do not return here
    }
    for (auto &entry : onlineTargets) {
        LOGI("[CommAggr][Activate] Already Online Target=%s{private}.", entry.c_str());
        commMap_[userId].at(commLabel).first->OnConnectChange(entry, true);
    }
    // Do Redeliver, the communicator is responsible to deal with the frame
    std::list<FrameInfo> framesToRedeliver = retainer_.FetchFramesForSpecificCommunicator(commLabel);
    for (auto &entry : framesToRedeliver) {
        commMap_[userId].at(commLabel).first->OnBufferReceive(entry.srcTarget, entry.buffer, entry.sendUser);
    }
}

namespace {
void DoOnSendEndByTaskIfNeed(const OnSendEnd &onEnd, int result)
{
    if (onEnd) {
        TaskAction onSendEndTask = [onEnd, result]() {
            LOGD("[CommAggr][SendEndTask] Before On Send End.");
            onEnd(result, true);
            LOGD("[CommAggr][SendEndTask] After On Send End.");
        };
        int errCode = RuntimeContext::GetInstance()->ScheduleTask(onSendEndTask);
        if (errCode != E_OK) {
            LOGE("[CommAggr][SendEndTask] ScheduleTask failed, errCode = %d.", errCode);
        }
    }
}
}

int CommunicatorAggregator::ScheduleSendTask(const std::string &dstTarget, SerialBuffer *inBuff,
    FrameType inType, const TaskConfig &inConfig, const OnSendEnd &onEnd)
{
    if (inBuff == nullptr) {
        return -E_INVALID_ARGS;
    }

    if (!ReGenerateLocalSourceIdIfNeed()) {
        delete inBuff;
        inBuff = nullptr;
        DoOnSendEndByTaskIfNeed(onEnd, -E_PERIPHERAL_INTERFACE_FAIL);
        LOGE("[CommAggr][Create] Exit ok but discard since localSourceId zero, thread=%s.", GetThreadId().c_str());
        return E_OK; // Returns E_OK here to indicate this buffer was accepted though discard immediately
    }
    bool sendLabelExchange = true;
    if (dbStatusAdapter_ != nullptr) {
        sendLabelExchange = dbStatusAdapter_->IsSendLabelExchange();
    }
    PhyHeaderInfo info{localSourceId_, incFrameId_.fetch_add(1, std::memory_order_seq_cst), inType,
        sendLabelExchange};
    int errCode = ProtocolProto::SetPhyHeader(inBuff, info);
    if (errCode != E_OK) {
        LOGE("[CommAggr][Create] Set phyHeader fail, thread=%s, errCode=%d", GetThreadId().c_str(), errCode);
        return errCode;
    }
    {
        std::lock_guard<std::mutex> autoLock(sendRecordMutex_);
        sendRecord_[info.frameId] = {};
    }
    SendTask task{inBuff, dstTarget, onEnd, info.frameId, true, inConfig.isRetryTask, inConfig.infos};
    if (inConfig.nonBlock) {
        errCode = scheduler_.AddSendTaskIntoSchedule(task, inConfig.prio);
    } else {
        errCode = RetryUntilTimeout(task, inConfig.timeout, inConfig.prio);
    }
    if (errCode != E_OK) {
        LOGW("[CommAggr][Create] Exit failed, thread=%s, errCode=%d", GetThreadId().c_str(), errCode);
        return errCode;
    }
    TriggerSendData();
    LOGI("[CommAggr][Create] Exit ok, dev=%.3s, frameId=%u, isRetry=%d", dstTarget.c_str(), info.frameId,
        task.isRetryTask);
    return E_OK;
}

void CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(bool isEnable)
{
    isCommunicatorNotFoundFeedbackEnable_ = isEnable;
}

int CommunicatorAggregator::GetRemoteCommunicatorVersion(const std::string &target, uint16_t &outVersion) const
{
    std::lock_guard<std::mutex> versionMapLockGuard(versionMapMutex_);
    auto pair = versionMap_.find(target);
    if (pair == versionMap_.end()) {
        return -E_NOT_FOUND;
    }
    outVersion = pair->second;
    return E_OK;
}

void CommunicatorAggregator::SendDataRoutine()
{
    while (!shutdown_) {
        if (scheduler_.GetNoDelayTaskCount() == 0) {
            std::unique_lock<std::mutex> wakingUniqueLock(wakingMutex_);
            LOGI("[CommAggr][Routine] Send done and sleep.");
            wakingCv_.wait(wakingUniqueLock, [this] { return this->wakingSignal_; });
            LOGI("[CommAggr][Routine] Send continue.");
            wakingSignal_ = false;
            continue;
        }
        SendOnceData();
    }
}

void CommunicatorAggregator::SendPacketsAndDisposeTask(const SendTask &inTask, uint32_t mtu,
    const std::vector<std::pair<const uint8_t *, std::pair<uint32_t, uint32_t>>> &eachPacket, uint32_t totalLength)
{
    bool taskNeedFinalize = true;
    int errCode = E_OK;
    ResetFrameRecordIfNeed(inTask.frameId, mtu);
    uint32_t startIndex;
    {
        std::lock_guard<std::mutex> autoLock(sendRecordMutex_);
        startIndex = sendRecord_[inTask.frameId].sendIndex;
    }
    uint64_t currentSendSequenceId = IncreaseSendSequenceId(inTask.dstTarget);
    DeviceInfos deviceInfos = {inTask.dstTarget, inTask.infos, inTask.isRetryTask};
    for (uint32_t index = startIndex; index < static_cast<uint32_t>(eachPacket.size()) && inTask.isValid; ++index) {
        auto &entry = eachPacket[index];
        LOGI("[CommAggr][SendPackets] DoSendBytes, dstTarget=%s{private}, extendHeadLength=%" PRIu32
            ", packetLength=%" PRIu32 ".", inTask.dstTarget.c_str(), entry.second.first, entry.second.second);
        ProtocolProto::DisplayPacketInformation(entry.first + entry.second.first, entry.second.second);
        errCode = adapterHandle_->SendBytes(deviceInfos, entry.first, entry.second.second, totalLength);
        {
            std::lock_guard<std::mutex> autoLock(sendRecordMutex_);
            sendRecord_[inTask.frameId].sendIndex = index;
        }
        if (errCode == -E_WAIT_RETRY) {
            LOGE("[CommAggr][SendPackets] SendBytes temporally fail.");
            taskNeedFinalize = false;
            break;
        } else if (errCode != E_OK) {
            LOGE("[CommAggr][SendPackets] SendBytes totally fail, errCode=%d.", errCode);
            break;
        } else {
            std::lock_guard<std::mutex> autoLock(retryCountMutex_);
            retryCount_[inTask.dstTarget] = 0;
        }
    }
    if (errCode == -E_WAIT_RETRY) {
        RetrySendTaskIfNeed(inTask.dstTarget, currentSendSequenceId);
    }
    if (taskNeedFinalize) {
        TaskFinalizer(inTask, errCode);
        std::lock_guard<std::mutex> autoLock(sendRecordMutex_);
        sendRecord_.erase(inTask.frameId);
    }
}

int CommunicatorAggregator::RetryUntilTimeout(SendTask &inTask, uint32_t timeout, Priority inPrio)
{
    int errCode = scheduler_.AddSendTaskIntoSchedule(inTask, inPrio);
    if (errCode != E_OK) {
        bool notTimeout = true;
        auto retryFunc = [this, inPrio, &inTask]()->bool {
            if (this->shutdown_) {
                delete inTask.buffer;
                inTask.buffer = nullptr;
                return true;
            }
            int retCode = scheduler_.AddSendTaskIntoSchedule(inTask, inPrio);
            if (retCode != E_OK) {
                return false;
            }
            return true;
        };

        if (timeout == 0) { // Unlimited retry
            std::unique_lock<std::mutex> retryUniqueLock(retryMutex_);
            retryCv_.wait(retryUniqueLock, retryFunc);
        } else {
            std::unique_lock<std::mutex> retryUniqueLock(retryMutex_);
            notTimeout = retryCv_.wait_for(retryUniqueLock, std::chrono::milliseconds(timeout), retryFunc);
        }

        if (shutdown_) {
            return E_OK;
        }
        if (!notTimeout) {
            return -E_TIMEOUT;
        }
    }
    return E_OK;
}

void CommunicatorAggregator::TaskFinalizer(const SendTask &inTask, int result)
{
    // Call the OnSendEnd if need
    if (inTask.onEnd) {
        LOGD("[CommAggr][TaskFinal] On Send End.");
        inTask.onEnd(result, true);
    }
    // Finalize the task that just scheduled
    int errCode = scheduler_.FinalizeLastScheduleTask();
    // Notify Sendable To All Communicator If Need
    if (errCode == -E_CONTAINER_FULL_TO_NOTFULL) {
        retryCv_.notify_all();
    }
    if (errCode == -E_CONTAINER_NOTEMPTY_TO_EMPTY) {
        NotifySendableToAllCommunicator();
    }
}

void CommunicatorAggregator::NotifySendableToAllCommunicator()
{
    std::lock_guard<std::mutex> commMapLockGuard(commMapMutex_);
    for (auto &userCommMap : commMap_) {
        for (auto &entry : userCommMap.second) {
            // Ignore nonactivated communicator
            if (entry.second.second) {
                entry.second.first->OnSendAvailable();
            }
        }
    }
}

void CommunicatorAggregator::OnBytesReceive(const ReceiveBytesInfo &receiveBytesInfo,
    const DataUserInfoProc &userInfoProc)
{
    ProtocolProto::DisplayPacketInformation(receiveBytesInfo.bytes, receiveBytesInfo.length);
    ParseResult packetResult;
    int errCode = ProtocolProto::CheckAndParsePacket(receiveBytesInfo.srcTarget, receiveBytesInfo.bytes,
        receiveBytesInfo.length, packetResult);
    if (errCode != E_OK) {
        LOGE("[CommAggr][Receive] Parse packet fail, errCode=%d.", errCode);
        if (errCode == -E_VERSION_NOT_SUPPORT) {
            TriggerVersionNegotiation(receiveBytesInfo.srcTarget);
        }
        return;
    }

    // Update version of remote target
    SetRemoteCommunicatorVersion(receiveBytesInfo.srcTarget, packetResult.GetDbVersion());
    if (dbStatusAdapter_ != nullptr) {
        dbStatusAdapter_->SetRemoteOptimizeCommunication(receiveBytesInfo.srcTarget,
            !packetResult.IsSendLabelExchange());
    }
    if (packetResult.GetFrameTypeInfo() == FrameType::EMPTY) { // Empty frame will never be fragmented
        LOGI("[CommAggr][Receive] Empty frame, just ignore in this version of distributeddb.");
        return;
    }

    if (packetResult.IsFragment()) {
        OnFragmentReceive(receiveBytesInfo, packetResult, userInfoProc);
    } else if (packetResult.GetFrameTypeInfo() != FrameType::APPLICATION_MESSAGE) {
        errCode = OnCommLayerFrameReceive(receiveBytesInfo.srcTarget, packetResult);
        if (errCode != E_OK) {
            LOGE("[CommAggr][Receive] CommLayer receive fail, errCode=%d.", errCode);
        }
    } else {
        errCode = OnAppLayerFrameReceive(receiveBytesInfo, packetResult, userInfoProc);
        if (errCode != E_OK) {
            LOGE("[CommAggr][Receive] AppLayer receive fail, errCode=%d.", errCode);
        }
    }
}

void CommunicatorAggregator::OnTargetChange(const std::string &target, bool isConnect)
{
    if (target.empty()) {
        LOGE("[CommAggr][OnTarget] Target empty string.");
        return;
    }
    // For process level target change
    {
        std::lock_guard<std::mutex> onConnectLockGuard(onConnectMutex_);
        if (onConnectHandle_) {
            onConnectHandle_(target, isConnect);
            LOGI("[CommAggr][OnTarget] On Connect End."); // Log in case callback block this thread
        } else {
            LOGI("[CommAggr][OnTarget] ConnectHandle invalid currently.");
        }
    }
    std::set<LabelType> relatedLabels;
    // For communicator level target change
    if (isConnect) {
        int errCode = commLinker_->TargetOnline(target, relatedLabels);
        if (errCode != E_OK) {
            LOGE("[CommAggr][OnTarget] TargetOnline fail, target=%s{private}, errCode=%d.", target.c_str(), errCode);
        }
    } else {
        commLinker_->TargetOffline(target, relatedLabels);
    }
    // All related communicator online or offline this target, no matter TargetOnline or TargetOffline fail or not
    std::lock_guard<std::mutex> commMapLockGuard(commMapMutex_);
    for (auto &userCommMap : commMap_) {
        for (auto &entry: userCommMap.second) {
            // Ignore nonactivated communicator
            if (entry.second.second && (!isConnect || (relatedLabels.count(entry.first) != 0))) {
                entry.second.first->OnConnectChange(target, isConnect);
            }
        }
    }
}

void CommunicatorAggregator::OnSendable(const std::string &target)
{
    int errCode = scheduler_.NoDelayTaskByTarget(target);
    if (errCode != E_OK) {
        LOGE("[CommAggr][Sendable] NoDelay target=%s{private} fail, errCode=%d.", target.c_str(), errCode);
        return;
    }
    TriggerSendData();
}

void CommunicatorAggregator::OnFragmentReceive(const ReceiveBytesInfo &receiveBytesInfo,
    const ParseResult &inResult, const DataUserInfoProc &userInfoProc)
{
    int errorNo = E_OK;
    ParseResult frameResult;
    SerialBuffer *frameBuffer = combiner_.AssembleFrameFragment(receiveBytesInfo.bytes, receiveBytesInfo.length,
        inResult, frameResult, errorNo);
    if (errorNo != E_OK) {
        LOGE("[CommAggr][Receive] Combine fail, errCode=%d.", errorNo);
        return;
    }
    if (frameBuffer == nullptr) {
        LOGW("[CommAggr][Receive] Combine undone.");
        return;
    }

    int errCode = ProtocolProto::CheckAndParseFrame(frameBuffer, frameResult);
    if (errCode != E_OK) {
        LOGE("[CommAggr][Receive] Parse frame fail, errCode=%d.", errCode);
        delete frameBuffer;
        frameBuffer = nullptr;
        if (errCode == -E_VERSION_NOT_SUPPORT) {
            TriggerVersionNegotiation(receiveBytesInfo.srcTarget);
        }
        return;
    }

    if (frameResult.GetFrameTypeInfo() != FrameType::APPLICATION_MESSAGE) {
        errCode = OnCommLayerFrameReceive(receiveBytesInfo.srcTarget, frameResult);
        if (errCode != E_OK) {
            LOGE("[CommAggr][Receive] CommLayer receive fail after combination, errCode=%d.", errCode);
        }
        delete frameBuffer;
        frameBuffer = nullptr;
    } else {
        errCode = OnAppLayerFrameReceive(receiveBytesInfo, frameBuffer, frameResult, userInfoProc);
        if (errCode != E_OK) {
            LOGE("[CommAggr][Receive] AppLayer receive fail after combination, errCode=%d.", errCode);
        }
    }
}

int CommunicatorAggregator::OnCommLayerFrameReceive(const std::string &srcTarget, const ParseResult &inResult)
{
    if (inResult.GetFrameTypeInfo() == FrameType::COMMUNICATION_LABEL_EXCHANGE_ACK) {
        int errCode = commLinker_->ReceiveLabelExchangeAck(srcTarget, inResult.GetLabelExchangeDistinctValue(),
            inResult.GetLabelExchangeSequenceId());
        if (errCode != E_OK) {
            LOGE("[CommAggr][CommReceive] Receive LabelExchangeAck Fail.");
            return errCode;
        }
    } else {
        std::map<LabelType, bool> changedLabels;
        int errCode = commLinker_->ReceiveLabelExchange(srcTarget, inResult.GetLatestCommLabels(),
            inResult.GetLabelExchangeDistinctValue(), inResult.GetLabelExchangeSequenceId(), changedLabels);
        if (errCode != E_OK) {
            LOGE("[CommAggr][CommReceive] Receive LabelExchange Fail.");
            return errCode;
        }
        NotifyConnectChange(srcTarget, changedLabels);
    }
    return E_OK;
}

int CommunicatorAggregator::OnAppLayerFrameReceive(const ReceiveBytesInfo &receiveBytesInfo,
    const ParseResult &inResult, const DataUserInfoProc &userInfoProc)
{
    SerialBuffer *buffer = new (std::nothrow) SerialBuffer();
    if (buffer == nullptr) {
        LOGE("[CommAggr][AppReceive] New SerialBuffer fail.");
        return -E_OUT_OF_MEMORY;
    }
    int errCode = buffer->SetExternalBuff(receiveBytesInfo.bytes, receiveBytesInfo.length - inResult.GetPaddingLen(),
        ProtocolProto::GetAppLayerFrameHeaderLength());
    if (errCode != E_OK) {
        LOGE("[CommAggr][AppReceive] SetExternalBuff fail, errCode=%d.", errCode);
        delete buffer;
        buffer = nullptr;
        return -E_INTERNAL_ERROR;
    }
    return OnAppLayerFrameReceive(receiveBytesInfo, buffer, inResult, userInfoProc);
}

// In early time, we cover "OnAppLayerFrameReceive" totally by commMapMutex_, then search communicator, if not found,
// we call onCommLackHandle_ if exist to ask whether to retain this frame or not, if the answer is yes we retain this
// frame, otherwise we discard this frame and send out CommunicatorNotFound feedback.
// We design so(especially cover this function totally by commMapMutex_) to avoid current situation described below
// 1:This func find that target communicator not allocated or activated, so decide to retain this frame.
// 2:Thread switch out, the target communicator is allocated and activated, previous retained frame is fetched out.
// 3:Thread switch back, this frame is then retained into the retainer, no chance to be fetched out.
// In conclusion: the decision to retain a frame and the action to retain a frame should not be separated.
// Otherwise, at the action time, the retain decision may be obsolete and wrong.
// #### BUT #### since onCommLackHandle_ callback is go beyond DistributedDB and there is the risk that the final upper
// user may do something such as GetKvStore(we can prevent them to so) which could result in calling AllocCommunicator
// in the same callback thread finally causing DeadLock on commMapMutex_.
// #### SO #### we have to make a change described below
// 1:Search communicator under commMapMutex_, if found then deliver frame to that communicator and end.
// 2:Call onCommLackHandle_ if exist to ask whether to retain this frame or not, without commMapMutex_.
// Note: during this period, commMap_ maybe changed, and communicator not found before may exist now.
// 3:Search communicator under commMapMutex_ again, if found then deliver frame to that communicator and end.
// 4:If still not found, retain this frame if need or otherwise send CommunicatorNotFound feedback.
int CommunicatorAggregator::OnAppLayerFrameReceive(const ReceiveBytesInfo &receiveBytesInfo,
    SerialBuffer *&inFrameBuffer, const ParseResult &inResult, const DataUserInfoProc &userInfoProc)
{
    LabelType toLabel = inResult.GetCommLabel();
    UserInfo userInfo = { .sendUser = DBConstant::DEFAULT_USER };
    if (receiveBytesInfo.isNeedGetUserInfo) {
        int ret = GetDataUserId(inResult, toLabel, userInfoProc, receiveBytesInfo.srcTarget, userInfo);
        if (ret == NEED_CORRECT_TARGET_USER) {
            TryToFeedBackWithErr(receiveBytesInfo.srcTarget, toLabel, inFrameBuffer,
                E_NEED_CORRECT_TARGET_USER);
            delete inFrameBuffer;
            inFrameBuffer = nullptr;
            return -E_NEED_CORRECT_TARGET_USER;
        }
        if (ret != E_OK || userInfo.sendUser.empty()) {
            LOGE("[CommAggr][AppReceive] get data user id err, ret=%d, empty receiveUser=%d, empty sendUser=%d", ret,
                userInfo.receiveUser.empty(), userInfo.sendUser.empty());
            delete inFrameBuffer;
            inFrameBuffer = nullptr;
            return ret != E_OK ? ret : -E_NO_TRUSTED_USER;
        }
    }
    {
        std::lock_guard<std::mutex> commMapLockGuard(commMapMutex_);
        int errCode = TryDeliverAppLayerFrameToCommunicatorNoMutex(userInfoProc, receiveBytesInfo.srcTarget,
            inFrameBuffer, toLabel, userInfo);
        if (errCode == E_OK) { // Attention: Here is equal to E_OK
            return E_OK;
        } else if (errCode == -E_FEEDBACK_DB_CLOSING) {
            TryToFeedbackWhenCommunicatorNotFound(receiveBytesInfo.srcTarget, toLabel, inFrameBuffer,
                E_FEEDBACK_DB_CLOSING);
            delete inFrameBuffer;
            inFrameBuffer = nullptr;
            return errCode; // The caller will display errCode in log
        }
    }
    LOGI("[CommAggr][AppReceive] Communicator of %.3s not found or nonactivated.", VEC_TO_STR(toLabel));
    return ReTryDeliverAppLayerFrameOnCommunicatorNotFound(receiveBytesInfo, inFrameBuffer, inResult, userInfoProc,
        userInfo);
}

int CommunicatorAggregator::GetDataUserId(const ParseResult &inResult, const LabelType &toLabel,
    const DataUserInfoProc &userInfoProc, const std::string &device, UserInfo &userInfo)
{
    if (userInfoProc.processCommunicator == nullptr) {
        LOGE("[CommAggr][GetDataUserId] processCommunicator is nullptr");
        return E_INVALID_ARGS;
    }
    std::string label(toLabel.begin(), toLabel.end());
    std::vector<UserInfo> userInfos;
    DataUserInfo dataUserInfo = {userInfoProc.data, userInfoProc.length, label, device};
    DBStatus ret = userInfoProc.processCommunicator->GetDataUserInfo(dataUserInfo, userInfos);
    LOGI("[CommAggr][GetDataUserId] get data user info, ret=%d", ret);
    if (ret == NO_PERMISSION) {
        LOGE("[CommAggr][GetDataUserId] userId dismatched, drop packet");
        return ret;
    } else if (ret == NEED_CORRECT_TARGET_USER) {
        LOGW("[CommAggr][GetDataUserId] the target user is incorrect and needs to be corrected");
        return ret;
    }
    if (!userInfos.empty()) {
        userInfo = userInfos[0];
    } else {
        LOGW("[CommAggr][GetDataUserId] userInfos is empty");
    }
    return E_OK;
}

int CommunicatorAggregator::TryDeliverAppLayerFrameToCommunicatorNoMutex(const DataUserInfoProc &userInfoProc,
    const std::string &srcTarget, SerialBuffer *&inFrameBuffer, const LabelType &toLabel, const UserInfo &userInfo)
{
    // Ignore nonactivated communicator, which is regarded as inexistent
    const std::string &sendUser = userInfo.sendUser;
    const std::string &receiveUser = userInfo.receiveUser;
    if (commMap_[receiveUser].count(toLabel) != 0 && commMap_[receiveUser].at(toLabel).second) {
        int ret = commMap_[receiveUser].at(toLabel).first->OnBufferReceive(srcTarget, inFrameBuffer, sendUser);
        // Frame handed over to communicator who is responsible to delete it. The frame is deleted here after return.
        if (ret == E_OK) {
            inFrameBuffer = nullptr;
        }
        return ret;
    }
    Communicator *communicator = nullptr;
    bool isEmpty = false;
    for (auto &userCommMap : commMap_) {
        for (auto &entry : userCommMap.second) {
            if (entry.first == toLabel && entry.second.second) {
                communicator = entry.second.first;
                isEmpty = userCommMap.first.empty();
                LOGW("[CommAggr][TryDeliver] Found communicator of %s, but required user is %s",
                     userCommMap.first.c_str(), receiveUser.c_str());
                break;
            }
        }
        if (communicator != nullptr) {
            break;
        }
    }
    if (communicator != nullptr && (receiveUser.empty() || isEmpty)) {
        int ret = communicator->OnBufferReceive(srcTarget, inFrameBuffer, sendUser);
        if (ret == E_OK) {
            inFrameBuffer = nullptr;
        }
        return ret;
    }
    LOGE("[CommAggr][TryDeliver] Communicator not found");
    return -E_NOT_FOUND;
}

int CommunicatorAggregator::RegCallbackToAdapter()
{
    RefObject::IncObjRef(this); // Reference to be hold by adapter
    int errCode = adapterHandle_->RegBytesReceiveCallback(
        [this](const ReceiveBytesInfo &receiveBytesInfo, const DataUserInfoProc &userInfoProc) {
            OnBytesReceive(receiveBytesInfo, userInfoProc);
        }, [this]() { RefObject::DecObjRef(this); });
    if (errCode != E_OK) {
        RefObject::DecObjRef(this); // Rollback in case reg failed
        return errCode;
    }

    RefObject::IncObjRef(this); // Reference to be hold by adapter
    errCode = adapterHandle_->RegTargetChangeCallback(
        [this](const std::string &target, bool isConnect) { OnTargetChange(target, isConnect); },
        [this]() { RefObject::DecObjRef(this); });
    if (errCode != E_OK) {
        RefObject::DecObjRef(this); // Rollback in case reg failed
        return errCode;
    }

    RefObject::IncObjRef(this); // Reference to be hold by adapter
    errCode = adapterHandle_->RegSendableCallback([this](const std::string &target, int deviceCommErrCode) {
            LOGI("[CommAggr] Send able dev=%.3s, deviceCommErrCode=%d", target.c_str(), deviceCommErrCode);
            (void)IncreaseSendSequenceId(target);
            scheduler_.SetDeviceCommErrCode(target, deviceCommErrCode);
            OnSendable(target);
        },
        [this]() { RefObject::DecObjRef(this); });
    if (errCode != E_OK) {
        RefObject::DecObjRef(this); // Rollback in case reg failed
        return errCode;
    }

    return E_OK;
}

void CommunicatorAggregator::UnRegCallbackFromAdapter()
{
    adapterHandle_->RegBytesReceiveCallback(nullptr, nullptr);
    adapterHandle_->RegTargetChangeCallback(nullptr, nullptr);
    adapterHandle_->RegSendableCallback(nullptr, nullptr);
    if (dbStatusAdapter_ != nullptr) {
        dbStatusAdapter_->SetDBStatusChangeCallback(nullptr, nullptr, nullptr);
    }
}

void CommunicatorAggregator::GenerateLocalSourceId()
{
    std::string identity;
    adapterHandle_->GetLocalIdentity(identity);
    // When GetLocalIdentity fail, the identity be an empty string, the localSourceId be zero, need regenerate
    // The localSourceId is std::atomic<uint64_t>, so there is no concurrency risk
    uint64_t identityHash = Hash::HashFunc(identity);
    if (identityHash != localSourceId_) {
        LOGI("[CommAggr][GenSrcId] identity=%s{private}, localSourceId=%" PRIu64, identity.c_str(), ULL(identityHash));
    }
    localSourceId_ = identityHash;
}

bool CommunicatorAggregator::ReGenerateLocalSourceIdIfNeed()
{
    // The deviceId will change when switch user from A to B
    // We can't listen to the user change, because it's hard to ensure the timing is correct.
    // So we regenerate to make sure the deviceId and localSourceId is correct when we create send task.
    // The localSourceId is std::atomic<uint64_t>, so there is no concurrency risk, no need lockguard here.
    GenerateLocalSourceId();
    return (localSourceId_ != 0);
}

void CommunicatorAggregator::TriggerVersionNegotiation(const std::string &dstTarget)
{
    LOGI("[CommAggr][TrigVer] Do version negotiate with target=%s{private}.", dstTarget.c_str());
    int errCode = E_OK;
    SerialBuffer *buffer = ProtocolProto::BuildEmptyFrameForVersionNegotiate(errCode);
    if (errCode != E_OK) {
        LOGE("[CommAggr][TrigVer] Build empty frame fail, errCode=%d", errCode);
        return;
    }

    TaskConfig config{true, true, 0, Priority::HIGH};
    errCode = ScheduleSendTask(dstTarget, buffer, FrameType::EMPTY, config);
    if (errCode != E_OK) {
        LOGE("[CommAggr][TrigVer] Send empty frame fail, errCode=%d", errCode);
        // if send fails, free buffer, otherwise buffer will be taked over by SendTaskScheduler
        delete buffer;
        buffer = nullptr;
    }
}

void CommunicatorAggregator::TryToFeedbackWhenCommunicatorNotFound(const std::string &dstTarget,
    const LabelType &dstLabel, const SerialBuffer *inOriFrame, int inErrCode)
{
    if (!isCommunicatorNotFoundFeedbackEnable_) {
        return;
    }
    TryToFeedBackWithErr(dstTarget, dstLabel, inOriFrame, inErrCode);
}

void CommunicatorAggregator::TryToFeedBackWithErr(const std::string &dstTarget,
    const DistributedDB::LabelType &dstLabel, const DistributedDB::SerialBuffer *inOriFrame, int inErrCode)
{
    if (dstTarget.empty() || inOriFrame == nullptr) {
        return;
    }
    int errCode = E_OK;
    Message *message = ProtocolProto::ToMessage(inOriFrame, errCode, true);
    if (message == nullptr) {
        if (errCode == -E_VERSION_NOT_SUPPORT) {
            TriggerVersionNegotiation(dstTarget);
        }
        return;
    }
    // Message is release in TriggerCommunicatorNotFoundFeedback
    TriggerCommunicatorFeedback(dstTarget, dstLabel, message, inErrCode);
}

void CommunicatorAggregator::TriggerCommunicatorFeedback(const std::string &dstTarget,
    const LabelType &dstLabel, Message* &oriMsg, int sendErrNo)
{
    if (oriMsg == nullptr || oriMsg->GetMessageType() != TYPE_REQUEST) {
        LOGI("[CommAggr][TrigNotFound] Do nothing for message with type not request.");
        // Do not have to do feedback if the message is not a request type message
        delete oriMsg;
        oriMsg = nullptr;
        return;
    }

    LOGI("[CommAggr][TrigNotFound] Do communicator feedback with target=%s{private}, send error code=%d.",
        dstTarget.c_str(), sendErrNo);
    oriMsg->SetMessageType(TYPE_RESPONSE);
    oriMsg->SetErrorNo(sendErrNo);

    int errCode = E_OK;
    SerialBuffer *buffer = ProtocolProto::BuildFeedbackMessageFrame(oriMsg, dstLabel, errCode);
    delete oriMsg;
    oriMsg = nullptr;
    if (errCode != E_OK) {
        LOGE("[CommAggr][TrigNotFound] Build communicator feedback frame fail, errCode=%d", errCode);
        return;
    }

    TaskConfig config{true, true, 0, Priority::HIGH};
    errCode = ScheduleSendTask(dstTarget, buffer, FrameType::APPLICATION_MESSAGE, config);
    if (errCode != E_OK) {
        LOGE("[CommAggr][TrigNotFound] Send communicator feedback frame fail, errCode=%d", errCode);
        // if send fails, free buffer, otherwise buffer will be taked over by ScheduleSendTask
        delete buffer;
        buffer = nullptr;
    }
}

void CommunicatorAggregator::SetRemoteCommunicatorVersion(const std::string &target, uint16_t version)
{
    std::lock_guard<std::mutex> versionMapLockGuard(versionMapMutex_);
    versionMap_[target] = version;
}

std::shared_ptr<ExtendHeaderHandle> CommunicatorAggregator::GetExtendHeaderHandle(const ExtendInfo &paramInfo)
{
    if (adapterHandle_ == nullptr) {
        return nullptr;
    }
    return adapterHandle_->GetExtendHeaderHandle(paramInfo);
}

void CommunicatorAggregator::OnRemoteDBStatusChange(const std::string &devInfo, const std::vector<DBInfo> &dbInfos)
{
    std::map<LabelType, bool> changedLabels;
    for (const auto &dbInfo: dbInfos) {
        std::string label = DBCommon::GenerateHashLabel(dbInfo);
        LabelType labelType(label.begin(), label.end());
        changedLabels[labelType] = dbInfo.isNeedSync;
    }
    if (commLinker_ != nullptr) {
        commLinker_->UpdateOnlineLabels(devInfo, changedLabels);
    }
    NotifyConnectChange(devInfo, changedLabels);
}

void CommunicatorAggregator::NotifyConnectChange(const std::string &srcTarget,
    const std::map<LabelType, bool> &changedLabels)
{
    if (commLinker_ != nullptr && !commLinker_->IsRemoteTargetOnline(srcTarget)) {
        LOGW("[CommAggr][NotifyConnectChange] from offline target=%s{private}.", srcTarget.c_str());
        for (const auto &entry : changedLabels) {
            LOGW("[CommAggr] REMEMBER: label=%s, inOnline=%d.", VEC_TO_STR(entry.first), entry.second);
        }
        return;
    }
    // Do target change notify
    std::lock_guard<std::mutex> commMapLockGuard(commMapMutex_);
    for (auto &entry : changedLabels) {
        for (auto &userCommMap : commMap_) {
            // Ignore nonactivated communicator
            if (userCommMap.second.count(entry.first) != 0 && userCommMap.second.at(entry.first).second) {
                LOGI("[CommAggr][NotifyConnectChange] label=%s, srcTarget=%s{private}, isOnline=%d.",
                     VEC_TO_STR(entry.first), srcTarget.c_str(), entry.second);
                userCommMap.second.at(entry.first).first->OnConnectChange(srcTarget, entry.second);
            }
        }
    }
}

void CommunicatorAggregator::RegDBChangeCallback()
{
    if (dbStatusAdapter_ != nullptr) {
        dbStatusAdapter_->SetDBStatusChangeCallback(
            [this](const std::string &devInfo, const std::vector<DBInfo> &dbInfos) {
                OnRemoteDBStatusChange(devInfo, dbInfos);
            },
            [this]() {
                if (commLinker_ != nullptr) {
                    (void)commLinker_->TriggerLabelExchangeEvent(false);
                }
            },
            [this](const std::string &dev) {
                if (commLinker_ != nullptr) {
                    std::set<LabelType> relatedLabels;
                    (void)commLinker_->TargetOnline(dev, relatedLabels);
                }
            });
    }
}
void CommunicatorAggregator::InitSendThread()
{
    if (RuntimeContext::GetInstance()->GetThreadPool() != nullptr) {
        return;
    }
    exclusiveThread_ = std::thread([this] { SendDataRoutine(); });
    useExclusiveThread_ = true;
}

void CommunicatorAggregator::SendOnceData()
{
    SendTask taskToSend;
    uint32_t totalLength = 0;
    int errCode = scheduler_.ScheduleOutSendTask(taskToSend, totalLength);
    if (errCode != E_OK) {
        return; // Not possible to happen
    }
    // <vector, extendHeadSize>
    std::vector<std::pair<std::vector<uint8_t>, uint32_t>> piecePackets;
    uint32_t mtu = adapterHandle_->GetMtuSize(taskToSend.dstTarget);
    if (taskToSend.buffer == nullptr) {
        LOGE("[CommAggr] buffer of taskToSend is nullptr.");
        return;
    }
    errCode = ProtocolProto::SplitFrameIntoPacketsIfNeed(taskToSend.buffer, mtu, piecePackets);
    if (errCode != E_OK) {
        LOGE("[CommAggr] Split frame fail, errCode=%d.", errCode);
        TaskFinalizer(taskToSend, errCode);
        return;
    }
    // <addr, <extendHeadSize, totalLen>>
    std::vector<std::pair<const uint8_t *, std::pair<uint32_t, uint32_t>>> eachPacket;
    if (piecePackets.empty()) {
        // Case that no need to split a frame, just use original buffer as a packet
        std::pair<const uint8_t *, uint32_t> tmpEntry = taskToSend.buffer->GetReadOnlyBytesForEntireBuffer();
        std::pair<const uint8_t *, std::pair<uint32_t, uint32_t>> entry;
        entry.first = tmpEntry.first - taskToSend.buffer->GetExtendHeadLength();
        entry.second.first = taskToSend.buffer->GetExtendHeadLength();
        entry.second.second = tmpEntry.second + entry.second.first;
        eachPacket.push_back(entry);
    } else {
        for (auto &entry : piecePackets) {
            std::pair<const uint8_t *, std::pair<uint32_t, uint32_t>> tmpEntry = {&(entry.first[0]),
                {entry.second, entry.first.size()}};
            eachPacket.push_back(tmpEntry);
        }
    }

    SendPacketsAndDisposeTask(taskToSend, mtu, eachPacket, totalLength);
}

void CommunicatorAggregator::TriggerSendData()
{
    if (useExclusiveThread_) {
        std::lock_guard<std::mutex> wakingLockGuard(wakingMutex_);
        wakingSignal_ = true;
        wakingCv_.notify_one();
        return;
    }
    {
        std::lock_guard<std::mutex> autoLock(scheduleSendTaskMutex_);
        if (sendTaskStart_) {
            return;
        }
        sendTaskStart_ = true;
    }
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this]() {
        LOGI("[CommAggr] Send thread start.");
        while (!shutdown_ && scheduler_.GetNoDelayTaskCount() != 0) {
            SendOnceData();
        }
        {
            std::lock_guard<std::mutex> autoLock(scheduleSendTaskMutex_);
            sendTaskStart_ = false;
        }
        if (!shutdown_ && scheduler_.GetNoDelayTaskCount() != 0) {
            TriggerSendData(); // avoid sendTaskStart_ was mark false after trigger thread check it
        }
        finalizeCv_.notify_one();
        RefObject::DecObjRef(this);
        LOGI("[CommAggr] Send thread end.");
    });
    if (errCode != E_OK) {
        LOGW("[CommAggr] Trigger send data failed %d", errCode);
        RefObject::DecObjRef(this);
    }
}

void CommunicatorAggregator::ResetFrameRecordIfNeed(const uint32_t frameId, const uint32_t mtu)
{
    std::lock_guard<std::mutex> autoLock(sendRecordMutex_);
    if (sendRecord_[frameId].splitMtu == 0u || sendRecord_[frameId].splitMtu != mtu) {
        sendRecord_[frameId].splitMtu = mtu;
        sendRecord_[frameId].sendIndex = 0u;
    }
}

void CommunicatorAggregator::RetrySendTaskIfNeed(const std::string &target, uint64_t sendSequenceId)
{
    if (IsRetryOutOfLimit(target)) {
        LOGD("[CommAggr] Retry send task is out of limit! target is %s{private}", target.c_str());
        scheduler_.InvalidSendTask(target);
        std::lock_guard<std::mutex> autoLock(retryCountMutex_);
        retryCount_[target] = 0;
    } else {
        RetrySendTask(target, sendSequenceId);
        if (sendSequenceId != GetSendSequenceId(target)) {
            LOGD("[CommAggr] %.3s Send sequence id has changed", target.c_str());
            return;
        }
        scheduler_.DelayTaskByTarget(target);
    }
}

void CommunicatorAggregator::RetrySendTask(const std::string &target, uint64_t sendSequenceId)
{
    int32_t currentRetryCount = 0;
    {
        std::lock_guard<std::mutex> autoLock(retryCountMutex_);
        retryCount_[target]++;
        currentRetryCount = retryCount_[target];
        LOGD("[CommAggr] Target %s{private} retry count is %" PRId32, target.c_str(), currentRetryCount);
    }
    TimerId timerId = 0u;
    RefObject::IncObjRef(this);
    (void)RuntimeContext::GetInstance()->SetTimer(GetNextRetryInterval(target, currentRetryCount),
        [this, target, sendSequenceId](TimerId id) {
        if (sendSequenceId == GetSendSequenceId(target)) {
            OnSendable(target);
        } else {
            LOGD("[CommAggr] %.3s Send sequence id has changed in timer", target.c_str());
        }
        RefObject::DecObjRef(this);
        return -E_END_TIMER;
    }, nullptr, timerId);
}

bool CommunicatorAggregator::IsRetryOutOfLimit(const std::string &target)
{
    std::lock_guard<std::mutex> autoLock(retryCountMutex_);
    return retryCount_[target] >= MAX_SEND_RETRY;
}

int32_t CommunicatorAggregator::GetNextRetryInterval(const std::string &target, int32_t currentRetryCount)
{
    uint32_t timeout = DBConstant::MIN_TIMEOUT;
    if (adapterHandle_ != nullptr) {
        timeout = adapterHandle_->GetTimeout(target);
    }
    return static_cast<int32_t>(timeout) * currentRetryCount / RETRY_TIME_SPLIT;
}

uint64_t CommunicatorAggregator::GetSendSequenceId(const std::string &target)
{
    std::lock_guard<std::mutex> autoLock(sendSequenceMutex_);
    return sendSequence_[target];
}

uint64_t CommunicatorAggregator::IncreaseSendSequenceId(const std::string &target)
{
    std::lock_guard<std::mutex> autoLock(sendSequenceMutex_);
    return ++sendSequence_[target];
}

void CommunicatorAggregator::ClearOnlineLabel()
{
    std::lock_guard<std::mutex> autoLock(commMapMutex_);
    if (commLinker_ == nullptr) {
        LOGE("[CommAggr] clear online label with null linker");
        return;
    }
    commLinker_->ClearOnlineLabel();
}

int CommunicatorAggregator::ReTryDeliverAppLayerFrameOnCommunicatorNotFound(const ReceiveBytesInfo &receiveBytesInfo,
    SerialBuffer *&inFrameBuffer, const ParseResult &inResult, const DataUserInfoProc &userInfoProc,
    const UserInfo &userInfo)
{
    LabelType toLabel = inResult.GetCommLabel();
    int errCode = -E_NOT_FOUND;
    {
        std::lock_guard<std::mutex> onCommLackLockGuard(onCommLackMutex_);
        if (onCommLackHandle_) {
            errCode = onCommLackHandle_(toLabel, userInfo.receiveUser);
            LOGI("[CommAggr][AppReceive] On CommLack End."); // Log in case callback block this thread
        } else {
            LOGI("[CommAggr][AppReceive] CommLackHandle invalid currently.");
        }
    }
    // Here we have to lock commMapMutex_ and search communicator again.
    std::lock_guard<std::mutex> commMapLockGuard(commMapMutex_);
    int errCodeAgain = TryDeliverAppLayerFrameToCommunicatorNoMutex(userInfoProc, receiveBytesInfo.srcTarget,
        inFrameBuffer, toLabel, userInfo);
    if (errCodeAgain == E_OK) { // Attention: Here is equal to E_OK.
        LOGI("[CommAggr][AppReceive] Communicator of %.3s found after try again(rare case).", VEC_TO_STR(toLabel));
        return E_OK;
    }
    // Here, communicator is still not found, retain or discard according to the result of onCommLackHandle_
    if (errCode != E_OK || errCodeAgain == -E_FEEDBACK_DB_CLOSING) {
        TryToFeedbackWhenCommunicatorNotFound(receiveBytesInfo.srcTarget, toLabel, inFrameBuffer,
            errCodeAgain == -E_FEEDBACK_DB_CLOSING ? E_FEEDBACK_DB_CLOSING : E_FEEDBACK_COMMUNICATOR_NOT_FOUND);
        if (inFrameBuffer != nullptr) {
            delete inFrameBuffer;
            inFrameBuffer = nullptr;
        }
        return errCode == E_OK ? errCodeAgain : errCode; // The caller will display errCode in log
    }
    // Do Retention, the retainer is responsible to deal with the frame
    retainer_.RetainFrame(FrameInfo{inFrameBuffer, receiveBytesInfo.srcTarget, userInfo.sendUser, toLabel,
        inResult.GetFrameId()});
    inFrameBuffer = nullptr;
    return E_OK;
}

void CommunicatorAggregator::ResetRetryCount()
{
    std::lock_guard<std::mutex> autoLock(retryCountMutex_);
    retryCount_.clear();
}
DEFINE_OBJECT_TAG_FACILITIES(CommunicatorAggregator)
} // namespace DistributedDB
