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

#include "time_sync.h"

#include "parcel.h"
#include "log_print.h"
#include "sync_types.h"
#include "message_transform.h"
#include "version.h"
#include "isync_task_context.h"

namespace DistributedDB {
std::mutex TimeSync::timeSyncSetLock_;
std::set<TimeSync *> TimeSync::timeSyncSet_;
namespace {
    constexpr uint64_t TIME_SYNC_INTERVAL = 24 * 60 * 60 * 1000; // 24h
    constexpr int TRIP_DIV_HALF = 2;
    constexpr int64_t MAX_TIME_OFFSET_NOISE = 1 * 1000 * 10000; // 1s for 100ns
    constexpr int64_t MAX_TIME_RTT_NOISE = 1 * 1000 * 10000; // 1s for 100ns
    constexpr uint64_t RTT_NOISE_CHECK_INTERVAL = 30 * 60 * 1000 * 10000u; // 30min for 100ns
}

// Class TimeSyncPacket
TimeSyncPacket::TimeSyncPacket()
    : sourceTimeBegin_(0),
      sourceTimeEnd_(0),
      targetTimeBegin_(0),
      targetTimeEnd_(0),
      version_(TIME_SYNC_VERSION_V1),
      requestLocalOffset_(0),
      responseLocalOffset_(0)
{
}

TimeSyncPacket::~TimeSyncPacket()
{
}

void TimeSyncPacket::SetSourceTimeBegin(Timestamp sourceTimeBegin)
{
    sourceTimeBegin_ = sourceTimeBegin;
}

Timestamp TimeSyncPacket::GetSourceTimeBegin() const
{
    return sourceTimeBegin_;
}

void TimeSyncPacket::SetSourceTimeEnd(Timestamp sourceTimeEnd)
{
    sourceTimeEnd_ = sourceTimeEnd;
}

Timestamp TimeSyncPacket::GetSourceTimeEnd() const
{
    return sourceTimeEnd_;
}

void TimeSyncPacket::SetTargetTimeBegin(Timestamp targetTimeBegin)
{
    targetTimeBegin_ = targetTimeBegin;
}

Timestamp TimeSyncPacket::GetTargetTimeBegin() const
{
    return targetTimeBegin_;
}

void TimeSyncPacket::SetTargetTimeEnd(Timestamp targetTimeEnd)
{
    targetTimeEnd_ = targetTimeEnd;
}

Timestamp TimeSyncPacket::GetTargetTimeEnd() const
{
    return targetTimeEnd_;
}

void TimeSyncPacket::SetVersion(uint32_t version)
{
    version_ = version;
}

uint32_t TimeSyncPacket::GetVersion() const
{
    return version_;
}

TimeOffset TimeSyncPacket::GetRequestLocalOffset() const
{
    return requestLocalOffset_;
}

void TimeSyncPacket::SetRequestLocalOffset(TimeOffset offset)
{
    requestLocalOffset_ = offset;
}

TimeOffset TimeSyncPacket::GetResponseLocalOffset() const
{
    return responseLocalOffset_;
}

void TimeSyncPacket::SetResponseLocalOffset(TimeOffset offset)
{
    responseLocalOffset_ = offset;
}

uint32_t TimeSyncPacket::CalculateLen()
{
    uint32_t len = Parcel::GetUInt32Len(); // version_
    len += Parcel::GetUInt64Len(); // sourceTimeBegin_
    len += Parcel::GetUInt64Len(); // sourceTimeEnd_
    len += Parcel::GetUInt64Len(); // targetTimeBegin_
    len += Parcel::GetUInt64Len(); // targetTimeEnd_
    len = Parcel::GetEightByteAlign(len);
    len += Parcel::GetInt64Len(); // requestLocalOffset_
    len += Parcel::GetInt64Len(); // responseLocalOffset_
    return len;
}

// Class TimeSync
TimeSync::TimeSync()
    : communicateHandle_(nullptr),
      metadata_(nullptr),
      timeHelper_(nullptr),
      retryTime_(0),
      driverTimerId_(0),
      isSynced_(false),
      isAckReceived_(false),
      timeChangedListener_(nullptr),
      timeDriverLockCount_(0),
      isOnline_(true),
      closed_(false)
{
}

TimeSync::~TimeSync()
{
    Finalize();
    driverTimerId_ = 0;

    if (timeChangedListener_ != nullptr) {
        timeChangedListener_->Drop(true);
        timeChangedListener_ = nullptr;
    }
    timeHelper_ = nullptr;
    communicateHandle_ = nullptr;
    metadata_ = nullptr;

    std::lock_guard<std::mutex> lock(timeSyncSetLock_);
    timeSyncSet_.erase(this);
}

int TimeSync::RegisterTransformFunc()
{
    TransformFunc func;
    func.computeFunc = std::bind(&TimeSync::CalculateLen, std::placeholders::_1);
    func.serializeFunc = std::bind(&TimeSync::Serialization, std::placeholders::_1,
                                   std::placeholders::_2, std::placeholders::_3);
    func.deserializeFunc = std::bind(&TimeSync::DeSerialization, std::placeholders::_1,
                                     std::placeholders::_2, std::placeholders::_3);
    return MessageTransform::RegTransformFunction(TIME_SYNC_MESSAGE, func);
}

int TimeSync::Initialize(ICommunicator *communicator, const std::shared_ptr<Metadata> &metadata,
    const ISyncInterface *storage, const DeviceID &deviceId)
{
    if ((communicator == nullptr) || (storage == nullptr) || (metadata == nullptr)) {
        return -E_INVALID_ARGS;
    }
    {
        std::lock_guard<std::mutex> lock(timeSyncSetLock_);
        timeSyncSet_.insert(this);
    }
    communicateHandle_ = communicator;
    metadata_ = metadata;
    deviceId_ = deviceId;
    timeHelper_ = std::make_unique<TimeHelper>();

    int errCode = timeHelper_->Initialize(storage, metadata_);
    if (errCode != E_OK) {
        timeHelper_ = nullptr;
        LOGE("[TimeSync] timeHelper Init failed, err %d.", errCode);
        return errCode;
    }
    dbId_ = storage->GetIdentifier();
    driverCallback_ = std::bind(&TimeSync::TimeSyncDriver, this, std::placeholders::_1);
    errCode = RuntimeContext::GetInstance()->SetTimer(TIME_SYNC_INTERVAL, driverCallback_, nullptr, driverTimerId_);
    if (errCode != E_OK) {
        return errCode;
    }
    isSynced_ = metadata_->IsTimeSyncFinish(deviceId_);
    return errCode;
}

void TimeSync::Finalize()
{
    // Stop the timer
    LOGD("[TimeSync] Finalize enter!");
    RuntimeContext *runtimeContext = RuntimeContext::GetInstance();
    TimerId timerId;
    {
        std::unique_lock<std::mutex> lock(timeDriverLock_);
        timerId = driverTimerId_;
    }
    runtimeContext->RemoveTimer(timerId, true);
    std::unique_lock<std::mutex> lock(timeDriverLock_);
    timeDriverCond_.wait(lock, [this](){ return this->timeDriverLockCount_ == 0; });
    LOGD("[TimeSync] Finalized!");
}

int TimeSync::SyncStart(const CommErrHandler &handler,  uint32_t sessionId)
{
    isOnline_ = true;
    TimeSyncPacket packet;
    Timestamp startTime = timeHelper_->GetTime();
    packet.SetSourceTimeBegin(startTime);
    TimeOffset timeOffset = metadata_->GetLocalTimeOffset();
    packet.SetRequestLocalOffset(timeOffset);
    // send timeSync request
    LOGD("[TimeSync] startTime = %" PRIu64 ", offset = % " PRId64 " , dev = %s{private}", startTime, timeOffset,
        deviceId_.c_str());

    Message *message = new (std::nothrow) Message(TIME_SYNC_MESSAGE);
    if (message == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    message->SetSessionId(sessionId);
    message->SetMessageType(TYPE_REQUEST);
    message->SetPriority(Priority::NORMAL);
    int errCode = message->SetCopiedObject<>(packet);
    if (errCode != E_OK) {
        delete message;
        message = nullptr;
        return errCode;
    }
    errCode = SendMessageWithSendEnd(message, handler);
    if (errCode != E_OK) {
        delete message;
        message = nullptr;
    }
    return errCode;
}

uint32_t TimeSync::CalculateLen(const Message *inMsg)
{
    if (!(IsPacketValid(inMsg, TYPE_RESPONSE) || IsPacketValid(inMsg, TYPE_REQUEST))) {
        return 0;
    }

    const TimeSyncPacket *packet = const_cast<TimeSyncPacket *>(inMsg->GetObject<TimeSyncPacket>());
    if (packet == nullptr) {
        return 0;
    }

    return TimeSyncPacket::CalculateLen();
}

int TimeSync::Serialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    if ((buffer == nullptr) || !(IsPacketValid(inMsg, TYPE_RESPONSE) || IsPacketValid(inMsg, TYPE_REQUEST))) {
        return -E_INVALID_ARGS;
    }
    const TimeSyncPacket *packet = inMsg->GetObject<TimeSyncPacket>();
    if ((packet == nullptr) || (length != TimeSyncPacket::CalculateLen())) {
        return -E_INVALID_ARGS;
    }

    Parcel parcel(buffer, length);
    Timestamp srcBegin = packet->GetSourceTimeBegin();
    Timestamp srcEnd = packet->GetSourceTimeEnd();
    Timestamp targetBegin = packet->GetTargetTimeBegin();
    Timestamp targetEnd = packet->GetTargetTimeEnd();

    int errCode = parcel.WriteUInt32(TIME_SYNC_VERSION_V1);
    if (errCode != E_OK) {
        return -E_SECUREC_ERROR;
    }
    errCode = parcel.WriteUInt64(srcBegin);
    if (errCode != E_OK) {
        return -E_SECUREC_ERROR;
    }
    errCode = parcel.WriteUInt64(srcEnd);
    if (errCode != E_OK) {
        return -E_SECUREC_ERROR;
    }
    errCode = parcel.WriteUInt64(targetBegin);
    if (errCode != E_OK) {
        return -E_SECUREC_ERROR;
    }
    errCode = parcel.WriteUInt64(targetEnd);
    if (errCode != E_OK) {
        return -E_SECUREC_ERROR;
    }
    parcel.EightByteAlign();
    parcel.WriteInt64(packet->GetRequestLocalOffset());
    parcel.WriteInt64(packet->GetResponseLocalOffset());
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    return errCode;
}

int TimeSync::DeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    if ((buffer == nullptr) || !(IsPacketValid(inMsg, TYPE_RESPONSE) || IsPacketValid(inMsg, TYPE_REQUEST))) {
        return -E_INVALID_ARGS;
    }
    TimeSyncPacket packet;
    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    Timestamp srcBegin;
    Timestamp srcEnd;
    Timestamp targetBegin;
    Timestamp targetEnd;

    uint32_t version = 0;
    parcel.ReadUInt32(version);
    if (parcel.IsError()) {
        return -E_INVALID_ARGS;
    }
    if (version > TIME_SYNC_VERSION_V1) {
        packet.SetVersion(version);
        return inMsg->SetCopiedObject<>(packet);
    }
    parcel.ReadUInt64(srcBegin);
    parcel.ReadUInt64(srcEnd);
    parcel.ReadUInt64(targetBegin);
    parcel.ReadUInt64(targetEnd);
    if (parcel.IsError()) {
        return -E_INVALID_ARGS;
    }
    packet.SetSourceTimeBegin(srcBegin);
    packet.SetSourceTimeEnd(srcEnd);
    packet.SetTargetTimeBegin(targetBegin);
    packet.SetTargetTimeEnd(targetEnd);
    parcel.EightByteAlign();
    if (parcel.IsContinueRead()) {
        TimeOffset requestLocalOffset;
        TimeOffset responseLocalOffset;
        parcel.ReadInt64(requestLocalOffset);
        parcel.ReadInt64(responseLocalOffset);
        if (parcel.IsError()) {
            LOGE("[TimeSync] Parse packet failed, message type %u", inMsg->GetMessageType());
            return -E_PARSE_FAIL;
        }
        packet.SetRequestLocalOffset(requestLocalOffset);
        packet.SetResponseLocalOffset(responseLocalOffset);
    }

    return inMsg->SetCopiedObject<>(packet);
}

int TimeSync::AckRecv(const Message *message, uint32_t targetSessionId)
{
    // only check when sessionId is not 0, because old version timesync sessionId is 0.
    if (message != nullptr && message->GetSessionId() != 0 &&
        message->GetErrorNo() == E_FEEDBACK_COMMUNICATOR_NOT_FOUND && message->GetSessionId() == targetSessionId) {
        LOGE("[AbilitySync][AckMsgCheck] Remote db is closed");
        return -E_FEEDBACK_COMMUNICATOR_NOT_FOUND;
    }
    if (!IsPacketValid(message, TYPE_RESPONSE)) {
        return -E_INVALID_ARGS;
    }
    const TimeSyncPacket *packet = message->GetObject<TimeSyncPacket>();
    if (packet == nullptr) {
        LOGE("[TimeSync] AckRecv packet is null");
        return -E_INVALID_ARGS;
    }

    TimeSyncPacket packetData = TimeSyncPacket(*packet);
    Timestamp sourceTimeEnd = timeHelper_->GetTime();
    packetData.SetSourceTimeBegin(GetSourceBeginTime(packetData.GetSourceTimeBegin(), targetSessionId));
    packetData.SetSourceTimeEnd(sourceTimeEnd);
    if (packetData.GetSourceTimeBegin() > packetData.GetSourceTimeEnd() ||
        packetData.GetTargetTimeBegin() > packetData.GetTargetTimeEnd() ||
        packetData.GetSourceTimeEnd() > TimeHelper::MAX_VALID_TIME ||
        packetData.GetTargetTimeEnd() > TimeHelper::MAX_VALID_TIME) {
        LOGD("[TimeSync][AckRecv] Time valid check failed.");
        return -E_INVALID_TIME;
    }
    int errCode = SaveOffsetWithAck(packetData);
    {
        std::lock_guard<std::mutex> lock(cvLock_);
        isAckReceived_ = true;
    }
    conditionVar_.notify_all();
    ResetTimer();
    return errCode;
}

int TimeSync::RequestRecv(const Message *message)
{
    if (!IsPacketValid(message, TYPE_REQUEST)) {
        return -E_INVALID_ARGS;
    }

    const TimeSyncPacket *packet = message->GetObject<TimeSyncPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }

    // build timeSync ack packet
    TimeSyncPacket ackPacket = BuildAckPacket(*packet);
    if (ackPacket.GetSourceTimeBegin() > TimeHelper::MAX_VALID_TIME) {
        LOGD("[TimeSync][RequestRecv] Time valid check failed.");
        return -E_INVALID_TIME;
    }
    ReTimeSyncIfNeed(ackPacket);

    Message *ackMessage = new (std::nothrow) Message(TIME_SYNC_MESSAGE);
    if (ackMessage == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    ackMessage->SetSessionId(message->GetSessionId());
    ackMessage->SetPriority(Priority::NORMAL);
    ackMessage->SetMessageType(TYPE_RESPONSE);
    ackMessage->SetTarget(deviceId_);
    int errCode = ackMessage->SetCopiedObject<>(ackPacket);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
        return errCode;
    }

    errCode = SendPacket(deviceId_, ackMessage);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
    }
    return errCode;
}

int TimeSync::SaveTimeOffset(const DeviceID &deviceID, TimeOffset timeOffset)
{
    return metadata_->SaveTimeOffset(deviceID, timeOffset);
}

std::pair<TimeOffset, TimeOffset> TimeSync::CalculateTimeOffset(const TimeSyncPacket &timeSyncInfo)
{
    TimeOffset roundTrip = static_cast<TimeOffset>((timeSyncInfo.GetSourceTimeEnd() -
        timeSyncInfo.GetSourceTimeBegin()) - (timeSyncInfo.GetTargetTimeEnd() - timeSyncInfo.GetTargetTimeBegin()));
    TimeOffset offset1 = static_cast<TimeOffset>(timeSyncInfo.GetTargetTimeBegin() -
        timeSyncInfo.GetSourceTimeBegin() - (roundTrip / TRIP_DIV_HALF));
    TimeOffset offset2 = static_cast<TimeOffset>(timeSyncInfo.GetTargetTimeEnd() + (roundTrip / TRIP_DIV_HALF) -
        timeSyncInfo.GetSourceTimeEnd());
    TimeOffset offset = (offset1 / TRIP_DIV_HALF) + (offset2 / TRIP_DIV_HALF);
    LOGD("TimeSync::CalculateTimeOffset roundTrip= %" PRId64 ", offset1 = %" PRId64 ", offset2 = %" PRId64
        ", offset = %" PRId64, roundTrip, offset1, offset2, offset);
    return {offset, roundTrip};
}

bool TimeSync::IsPacketValid(const Message *inMsg, uint16_t messageType)
{
    if (inMsg == nullptr) {
        return false;
    }
    if (inMsg->GetMessageId() != TIME_SYNC_MESSAGE) {
        LOGD("message Id = %d", inMsg->GetMessageId());
        return false;
    }
    if (messageType != inMsg->GetMessageType()) {
        LOGD("input Type = %" PRIu16 ", inMsg type = %" PRIu16, messageType, inMsg->GetMessageType());
        return false;
    }
    return true;
}

int TimeSync::SendPacket(const DeviceID &deviceId, const Message *message, const CommErrHandler &handler)
{
    SendConfig conf;
    timeHelper_->SetSendConfig(deviceId, false, SEND_TIME_OUT, conf);
    int errCode = communicateHandle_->SendMessage(deviceId, message, conf, handler);
    if (errCode != E_OK) {
        LOGE("[TimeSync] SendPacket failed, err %d", errCode);
    }
    return errCode;
}

int TimeSync::TimeSyncDriver(TimerId timerId)
{
    if (timerId != driverTimerId_) {
        return -E_INTERNAL_ERROR;
    }
    if (!isOnline_) {
        return E_OK;
    }
    std::lock_guard<std::mutex> lock(timeDriverLock_);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this]() {
        CommErrHandler handler = std::bind(&TimeSync::CommErrHandlerFunc, std::placeholders::_1, this);
        (void)this->SyncStart(handler);
        std::lock_guard<std::mutex> innerLock(this->timeDriverLock_);
        this->timeDriverLockCount_--;
        this->timeDriverCond_.notify_all();
    });
    if (errCode != E_OK) {
        LOGE("[TimeSync][TimerSyncDriver] ScheduleTask failed err %d", errCode);
        return errCode;
    }
    timeDriverLockCount_++;
    return E_OK;
}

int TimeSync::GetTimeOffset(TimeOffset &outOffset, uint32_t timeout, uint32_t sessionId)
{
    if (!isSynced_) {
        {
            std::lock_guard<std::mutex> lock(cvLock_);
            isAckReceived_ = false;
        }
        CommErrHandler handler = std::bind(&TimeSync::CommErrHandlerFunc, std::placeholders::_1, this);
        int errCode = SyncStart(handler, sessionId);
        LOGD("TimeSync::GetTimeOffset start, current time = %" PRIu64 ", errCode = %d, timeout = %" PRIu32 " ms",
            TimeHelper::GetSysCurrentTime(), errCode, timeout);
        std::unique_lock<std::mutex> lock(cvLock_);
        if (errCode != E_OK || !conditionVar_.wait_for(lock, std::chrono::milliseconds(timeout), [this]() {
            return this->isAckReceived_ || this->closed_;
            })) {
            LOGD("TimeSync::GetTimeOffset, retryTime_ = %d", retryTime_);
            retryTime_++;
            if (retryTime_ < MAX_RETRY_TIME) {
                lock.unlock();
                LOGI("TimeSync::GetTimeOffset timeout, try again");
                return GetTimeOffset(outOffset, timeout);
            }
            retryTime_ = 0;
            return -E_TIMEOUT;
        }
    }
    if (IsClosed()) {
        return -E_BUSY;
    }
    retryTime_ = 0;
    metadata_->GetTimeOffset(deviceId_, outOffset);
    return E_OK;
}

bool TimeSync::IsNeedSync() const
{
    return !isSynced_;
}

void TimeSync::SetOnline(bool isOnline)
{
    isOnline_ = isOnline;
}

void TimeSync::CommErrHandlerFunc(int errCode, TimeSync *timeSync)
{
    LOGD("[TimeSync][CommErrHandle] errCode:%d", errCode);
    std::lock_guard<std::mutex> lock(timeSyncSetLock_);
    if (timeSyncSet_.count(timeSync) == 0) {
        LOGI("[TimeSync][CommErrHandle] timeSync has been killed");
        return;
    }
    if (timeSync == nullptr) {
        LOGI("[TimeSync][CommErrHandle] timeSync is nullptr");
        return;
    }
    if (errCode != E_OK) {
        timeSync->SetOnline(false);
    } else {
        timeSync->SetOnline(true);
    }
}

void TimeSync::ResetTimer()
{
    TimerId timerId;
    {
        std::lock_guard<std::mutex> lock(timeDriverLock_);
        timerId = driverTimerId_;
        driverTimerId_ = 0u;
    }
    if (timerId == 0u) {
        return;
    }
    RuntimeContext::GetInstance()->RemoveTimer(timerId, true);
    int errCode = RuntimeContext::GetInstance()->SetTimer(
        TIME_SYNC_INTERVAL, driverCallback_, nullptr, timerId);
    if (errCode != E_OK) {
        LOGW("[TimeSync] Reset TimeSync timer failed err :%d", errCode);
    } else {
        std::lock_guard<std::mutex> lock(timeDriverLock_);
        driverTimerId_ = timerId;
    }
}

void TimeSync::Close()
{
    Finalize();
    {
        std::lock_guard<std::mutex> lock(cvLock_);
        closed_ = true;
    }
    conditionVar_.notify_all();
}

bool TimeSync::IsClosed() const
{
    std::lock_guard<std::mutex> lock(cvLock_);
    return closed_ ;
}

int TimeSync::SendMessageWithSendEnd(const Message *message, const CommErrHandler &handler)
{
    std::shared_ptr<TimeSync> timeSyncPtr = shared_from_this();
    auto sessionId = message->GetSessionId();
    return SendPacket(deviceId_, message, [handler, timeSyncPtr, sessionId, this](int errCode) {
        if (closed_) {
            LOGW("[TimeSync] DB closed, ignore send end! dev=%.3s", deviceId_.c_str());
            return;
        }
        {
            std::lock_guard<std::mutex> autoLock(beginTimeMutex_);
            sessionBeginTime_.clear();
            sessionBeginTime_[sessionId] = timeHelper_->GetTime();
        }
        if (handler != nullptr) {
            handler(errCode);
        }
    });
}

Timestamp TimeSync::GetSourceBeginTime(Timestamp packetBeginTime, uint32_t sessionId)
{
    std::lock_guard<std::mutex> autoLock(beginTimeMutex_);
    if (sessionBeginTime_.find(sessionId) == sessionBeginTime_.end()) {
        LOGW("[TimeSync] Current cache not exist packet send time");
        return packetBeginTime;
    }
    auto sendTime = sessionBeginTime_[sessionId];
    LOGD("[TimeSync] Use packet send time %" PRIu64 " rather than %" PRIu64, sendTime, packetBeginTime);
    return sendTime;
}

TimeSyncPacket TimeSync::BuildAckPacket(const TimeSyncPacket &request)
{
    TimeSyncPacket ackPacket = TimeSyncPacket(request);
    Timestamp targetTimeBegin = timeHelper_->GetTime();
    ackPacket.SetTargetTimeBegin(targetTimeBegin);
    Timestamp targetTimeEnd = timeHelper_->GetTime();
    ackPacket.SetTargetTimeEnd(targetTimeEnd);
    TimeOffset requestOffset = request.GetRequestLocalOffset();
    TimeOffset responseOffset = metadata_->GetLocalTimeOffset();
    ackPacket.SetRequestLocalOffset(requestOffset);
    ackPacket.SetResponseLocalOffset(responseOffset);
    LOGD("TimeSync::RequestRecv, dev = %s{private}, sTimeEnd = %" PRIu64 ", tTimeEnd = %" PRIu64 ", sbegin = %" PRIu64
        ", tbegin = %" PRIu64 ", request offset = %" PRId64 ", response offset = %" PRId64, deviceId_.c_str(),
        ackPacket.GetSourceTimeEnd(), ackPacket.GetTargetTimeEnd(), ackPacket.GetSourceTimeBegin(),
        ackPacket.GetTargetTimeBegin(), requestOffset, responseOffset);
    return ackPacket;
}

bool TimeSync::IsRemoteLowVersion(uint32_t checkVersion)
{
    uint16_t version = 0;
    int errCode = communicateHandle_->GetRemoteCommunicatorVersion(deviceId_, version);
    return errCode == -E_NOT_FOUND || (version < checkVersion - SOFTWARE_VERSION_EARLIEST);
}

void TimeSync::ReTimeSyncIfNeed(const TimeSyncPacket &ackPacket)
{
    TimeOffset timeOffsetIgnoreRtt =
        static_cast<TimeOffset>(ackPacket.GetSourceTimeBegin() - ackPacket.GetTargetTimeBegin());
    bool reTimeSync = false;
    if (IsRemoteLowVersion(SOFTWARE_VERSION_RELEASE_9_0)) {
        reTimeSync = CheckReTimeSyncIfNeedWithLowVersion(timeOffsetIgnoreRtt);
    } else {
        reTimeSync = CheckReTimeSyncIfNeedWithHighVersion(timeOffsetIgnoreRtt, ackPacket);
    }

    if ((std::abs(timeOffsetIgnoreRtt) >= INT64_MAX / 2) || reTimeSync) { // 2 is half of INT64_MAX
        LOGI("[TimeSync][RequestRecv] timeOffSet invalid, should do time sync");
        SetTimeSyncFinishInner(false);
        RuntimeContext::GetInstance()->ClearDeviceTimeInfo(deviceId_);
    }

    // reset time change by time sync
    int errCode = metadata_->SetTimeChangeMark(deviceId_, false);
    if (errCode != E_OK) {
        LOGW("[TimeSync] Mark dev %.3s no time change failed %d", deviceId_.c_str(), errCode);
    }
}

bool TimeSync::CheckReTimeSyncIfNeedWithLowVersion(TimeOffset timeOffsetIgnoreRtt)
{
    TimeOffset metadataTimeOffset;
    metadata_->GetTimeOffset(deviceId_, metadataTimeOffset);
    return (std::abs(metadataTimeOffset) >= INT64_MAX / 2) || // 2 is half of INT64_MAX
        (std::abs(metadataTimeOffset - timeOffsetIgnoreRtt) > MAX_TIME_OFFSET_NOISE);
}

bool TimeSync::CheckReTimeSyncIfNeedWithHighVersion(TimeOffset timeOffsetIgnoreRtt, const TimeSyncPacket &ackPacket)
{
    TimeOffset rawTimeOffset = timeOffsetIgnoreRtt - ackPacket.GetRequestLocalOffset() +
        ackPacket.GetResponseLocalOffset();
    auto [errCode, info] = RuntimeContext::GetInstance()->GetDeviceTimeInfo(deviceId_);
    return errCode == -E_NOT_FOUND || (std::abs(info.systemTimeOffset - rawTimeOffset) > MAX_TIME_OFFSET_NOISE);
}

int TimeSync::SaveOffsetWithAck(const TimeSyncPacket &ackPacket)
{
    // calculate timeoffset of two devices
    auto [offset, rtt] = CalculateTimeOffset(ackPacket);
    TimeOffset rawOffset = CalculateRawTimeOffset(ackPacket, offset);
    LOGD("TimeSync::AckRecv, dev = %s{private}, sEnd = %" PRIu64 ", tEnd = %" PRIu64 ", sBegin = %" PRIu64
        ", tBegin = %" PRIu64 ", offset = %" PRId64 ", rawOffset = %" PRId64 ", requestLocalOffset = %" PRId64
        ", responseLocalOffset = %" PRId64,
        deviceId_.c_str(),
        ackPacket.GetSourceTimeEnd(),
        ackPacket.GetTargetTimeEnd(),
        ackPacket.GetSourceTimeBegin(),
        ackPacket.GetTargetTimeBegin(),
        offset,
        rawOffset,
        ackPacket.GetRequestLocalOffset(),
        ackPacket.GetResponseLocalOffset());

    // save timeoffset into metadata, maybe a block action
    int errCode = SaveTimeOffset(deviceId_, offset);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = metadata_->SetSystemTimeOffset(deviceId_, rawOffset);
    if (errCode != E_OK) {
        return errCode;
    }
    DeviceTimeInfo info;
    info.systemTimeOffset = rawOffset;
    info.recordTime = timeHelper_->GetSysCurrentTime();
    info.rtt = rtt;
    RuntimeContext::GetInstance()->SetDeviceTimeInfo(deviceId_, info);
    SetTimeSyncFinishInner(true);
    // save finish next time after save failed
    return E_OK;
}

TimeOffset TimeSync::CalculateRawTimeOffset(const TimeSyncPacket &timeSyncInfo, TimeOffset deltaTime)
{
    // deltaTime = (t1' + response - t1 - request + t2' + response - t2 - request)/2
    // rawTimeOffset =  request - response + (t1' - t1 + t2' - t2)/2
    // rawTimeOffset = deltaTime + requestLocalOffset - responseLocalOffset
    return deltaTime + timeSyncInfo.GetRequestLocalOffset() - timeSyncInfo.GetResponseLocalOffset();
}

bool TimeSync::CheckSkipTimeSync(const DeviceTimeInfo &info)
{
    uint64_t currentRawTime = timeHelper_->GetSysCurrentTime();
    if (currentRawTime < info.recordTime) {
        LOGW("[TimeSync] current time %" PRIu64 " less than record time %" PRIu64, currentRawTime, info.recordTime);
        return false;
    }
    uint64_t interval = timeHelper_->GetSysCurrentTime() - info.recordTime;
    if (info.rtt < MAX_TIME_RTT_NOISE) {
        return true;
    }
    if (interval > RTT_NOISE_CHECK_INTERVAL) {
        LOGI("[TimeSync] rtt %" PRId64 " is greater than noise should re time sync, interval is %" PRIu64, info.rtt,
            interval);
        return false;
    }
#ifdef DE_DEBUG_ENV
#ifdef TEST_RTT_NOISE_CHECK_INTERVAL
    if (interval > TEST_RTT_NOISE_CHECK_INTERVAL) {
        LOGI("[TimeSync][TEST] rtt %" PRId64 " is greater than noise should re time sync, interval is %" PRIu64,
            info.rtt, interval);
        return false;
    }
#endif
#endif
    return true;
}

void TimeSync::SetTimeSyncFinishIfNeed()
{
    auto [errCode, info] = RuntimeContext::GetInstance()->GetDeviceTimeInfo(deviceId_);
    if (errCode != E_OK) {
        return;
    }
    int64_t systemTimeOffset = metadata_->GetSystemTimeOffset(deviceId_);
    LOGD("[TimeSync] Check db offset %" PRId64 " cache offset %" PRId64, systemTimeOffset, info.systemTimeOffset);
    if (!CheckSkipTimeSync(info) || (IsNeedSync() &&
        std::abs(systemTimeOffset - info.systemTimeOffset) >= MAX_TIME_OFFSET_NOISE)) {
        SetTimeSyncFinishInner(false);
        return;
    }
    if (IsNeedSync()) {
        SetTimeSyncFinishInner(true);
    }
    if (systemTimeOffset != info.systemTimeOffset) {
        errCode = metadata_->SetSystemTimeOffset(deviceId_, info.systemTimeOffset);
        if (errCode != E_OK) {
            return;
        }
    }
    LOGI("[TimeSync] Mark time sync finish success");
}

void TimeSync::ClearTimeSyncFinish()
{
    RuntimeContext::GetInstance()->ClearDeviceTimeInfo(deviceId_);
    SetTimeSyncFinishInner(false);
}

int TimeSync::GenerateTimeOffsetIfNeed(TimeOffset systemOffset, TimeOffset senderLocalOffset)
{
    if (IsRemoteLowVersion(SOFTWARE_VERSION_RELEASE_9_0)) {
        return E_OK;
    }
    auto [errCode, info] = RuntimeContext::GetInstance()->GetDeviceTimeInfo(deviceId_);
    bool syncFinish = !IsNeedSync();
    bool timeChange = metadata_->IsTimeChange(deviceId_);
    // avoid local time change but remote record time sync finish
    // should return re time sync, after receive time sync request, reset time change mark
    // we think offset is ok when local time sync to remote
    if ((timeChange && !syncFinish) ||
        (errCode == E_OK && (std::abs(info.systemTimeOffset + systemOffset) > MAX_TIME_OFFSET_NOISE))) {
        LOGI("[TimeSync] time offset is invalid should do time sync again! packet %" PRId64 " cache %" PRId64
            " time change %d sync finish %d", -systemOffset, info.systemTimeOffset, static_cast<int>(timeChange),
            static_cast<int>(syncFinish));
        ClearTimeSyncFinish();
        RuntimeContext::GetInstance()->ClearDeviceTimeInfo(deviceId_);
        return -E_NEED_TIME_SYNC;
    }
    // Sender's systemOffset = Sender's deltaTime + requestLocalOffset - responseLocalOffset
    // Sender's deltaTime = Sender's systemOffset - requestLocalOffset + responseLocalOffset
    // Receiver's deltaTime = -Sender's deltaTime = -Sender's systemOffset + requestLocalOffset - responseLocalOffset
    TimeOffset offset = -systemOffset + senderLocalOffset - metadata_->GetLocalTimeOffset();
    errCode = metadata_->SetSystemTimeOffset(deviceId_, -systemOffset);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = metadata_->SaveTimeOffset(deviceId_, offset);
    if (errCode != E_OK) {
        return errCode;
    }
    SetTimeSyncFinishInner(true);
    info.systemTimeOffset = -systemOffset;
    info.recordTime = timeHelper_->GetSysCurrentTime();
    RuntimeContext::GetInstance()->SetDeviceTimeInfo(deviceId_, info);
    return E_OK;
}

void TimeSync::SetTimeSyncFinishInner(bool finish)
{
    isSynced_ = finish;
    if (IsRemoteLowVersion(SOFTWARE_VERSION_RELEASE_9_0)) {
        return;
    }
    int errCode = metadata_->SetTimeSyncFinishMark(deviceId_, finish);
    if (errCode != E_OK) {
        LOGW("[TimeSync] Set %.3s time sync finish %d mark failed %d", deviceId_.c_str(), static_cast<int>(finish),
            errCode);
    }
}
} // namespace DistributedDB