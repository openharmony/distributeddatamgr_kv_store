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

#include "single_ver_data_sync.h"

#include "db_common.h"
#include "db_types.h"
#include "generic_single_ver_kv_entry.h"
#include "intercepted_data_impl.h"
#include "log_print.h"
#include "message_transform.h"
#include "performance_analysis.h"
#include "single_ver_data_sync_utils.h"
#include "single_ver_sync_state_machine.h"
#include "subscribe_manager.h"
#ifdef RELATIONAL_STORE
#include "relational_db_sync_interface.h"
#endif

namespace DistributedDB {
SingleVerDataSync::SingleVerDataSync()
    : mtuSize_(0),
      storage_(nullptr),
      communicateHandle_(nullptr),
      metadata_(nullptr)
{
}

SingleVerDataSync::~SingleVerDataSync()
{
    storage_ = nullptr;
    communicateHandle_ = nullptr;
    metadata_ = nullptr;
}

int SingleVerDataSync::Initialize(ISyncInterface *inStorage, ICommunicator *inCommunicateHandle,
    const std::shared_ptr<Metadata> &inMetadata, const std::string &deviceId)
{
    if ((inStorage == nullptr) || (inCommunicateHandle == nullptr) || (inMetadata == nullptr)) {
        return -E_INVALID_ARGS;
    }
    storage_ = static_cast<SyncGenericInterface *>(inStorage);
    communicateHandle_ = inCommunicateHandle;
    metadata_ = inMetadata;
    mtuSize_ = DBConstant::MIN_MTU_SIZE; // default size is 1K, it will update when need sync data.
    std::vector<uint8_t> label = inStorage->GetIdentifier();
    label.resize(3); // only show 3 Bytes enough
    label_ = DBCommon::VectorToHexString(label);
    deviceId_ = deviceId;
    msgSchedule_.Initialize(label_, deviceId_);
    return E_OK;
}

int SingleVerDataSync::SyncStart(int mode, SingleVerSyncTaskContext *context)
{
    std::lock_guard<std::mutex> lock(lock_);
    int errCode = CheckPermitSendData(mode, context);
    if (errCode != E_OK) {
        return errCode;
    }
    if (sessionId_ != 0) { // auto sync timeout resend
        return ReSendData(context);
    }
    ResetSyncStatus(mode, context);
    LOGI("[DataSync] SendStart,mode=%d,label=%s,device=%s", mode_, label_.c_str(), STR_MASK(deviceId_));
    int tmpMode = SyncOperation::TransferSyncMode(mode);
    if (tmpMode == SyncModeType::PUSH) {
        errCode = PushStart(context);
    } else if (tmpMode == SyncModeType::PUSH_AND_PULL) {
        errCode = PushPullStart(context);
    } else if (tmpMode == SyncModeType::PULL) {
        errCode = PullRequestStart(context);
    } else {
        SingleVerDataSyncUtils::CacheInitWaterMark(context, this);
        errCode = PullResponseStart(context);
    }
    if (context->IsSkipTimeoutError(errCode)) {
        // if E_TIMEOUT occurred, means send message pressure is high, put into resend map and wait for resend.
        // just return to avoid higher pressure for send.
        return E_OK;
    }
    if (errCode != E_OK) {
        LOGE("[DataSync] SendStart errCode=%d", errCode);
        return errCode;
    }
    if (tmpMode == SyncModeType::PUSH_AND_PULL && context->GetTaskErrCode() == -E_EKEYREVOKED) {
        LOGE("wait for recv finished for push and pull mode");
        return -E_EKEYREVOKED;
    }
    return InnerSyncStart(context);
}

int SingleVerDataSync::InnerSyncStart(SingleVerSyncTaskContext *context)
{
    int errCode = CheckPermitSendData(mode_, context);
    if (errCode != E_OK) {
        return errCode;
    }
    while (true) {
        if (windowSize_ <= 0 || isAllDataHasSent_) {
            LOGD("[DataSync] InnerDataSync winSize=%d,isAllSent=%d,label=%s,device=%s", windowSize_, isAllDataHasSent_,
                label_.c_str(), STR_MASK(deviceId_));
            return E_OK;
        }
        int mode = SyncOperation::TransferSyncMode(mode_);
        if (mode == SyncModeType::PULL) {
            LOGE("[DataSync] unexpected error");
            return -E_INVALID_ARGS;
        }
        context->IncSequenceId();
        if (mode == SyncModeType::PUSH || mode == SyncModeType::PUSH_AND_PULL) {
            errCode = PushStart(context);
        } else {
            errCode = PullResponseStart(context);
        }
        if ((mode == SyncModeType::PUSH_AND_PULL) && errCode == -E_EKEYREVOKED) {
            LOGE("[DataSync] wait for recv finished,label=%s,device=%s", label_.c_str(), STR_MASK(deviceId_));
            isAllDataHasSent_ = true;
            return -E_EKEYREVOKED;
        }
        if (context->IsSkipTimeoutError(errCode)) {
            // if E_TIMEOUT occurred, means send message pressure is high, put into resend map and wait for resend.
            // just return to avoid higher pressure for send.
            return E_OK;
        }
        if (errCode != E_OK) {
            LOGE("[DataSync] InnerSend errCode=%d", errCode);
            return errCode;
        }
    }
    return E_OK;
}

void SingleVerDataSync::InnerClearSyncStatus()
{
    sessionId_ = 0;
    reSendMap_.clear();
    windowSize_ = 0;
    maxSequenceIdHasSent_ = 0;
    isAllDataHasSent_ = false;
}

int SingleVerDataSync::TryContinueSync(SingleVerSyncTaskContext *context, const Message *message)
{
    if (message == nullptr) {
        LOGE("[DataSync] AckRecv message nullptr");
        return -E_INVALID_ARGS;
    }
    const DataAckPacket *packet = message->GetObject<DataAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    uint64_t packetId = packet->GetPacketId(); // above 102 version data request reserve[0] store packetId value
    uint32_t sessionId = message->GetSessionId();
    uint32_t sequenceId = message->GetSequenceId();

    std::lock_guard<std::mutex> lock(lock_);
    LOGI("[DataSync] recv ack seqId=%" PRIu32 ",packetId=%" PRIu64 ",winSize=%d,label=%s,dev=%s", sequenceId, packetId,
        windowSize_, label_.c_str(), STR_MASK(deviceId_));
    if (sessionId != sessionId_) {
        LOGI("[DataSync] ignore ack,sessionId is different");
        return E_OK;
    }
    Timestamp lastQueryTime = 0;
    if (reSendMap_.count(sequenceId) != 0) {
        lastQueryTime = reSendMap_[sequenceId].end;
        reSendMap_.erase(sequenceId);
        windowSize_++;
    } else {
        LOGI("[DataSync] ack seqId not in map");
        return E_OK;
    }
    if (context->IsQuerySync() && storage_->GetInterfaceType() == ISyncInterface::SYNC_RELATION) {
        Timestamp dbLastQueryTime = 0;
        int errCode = metadata_->GetLastQueryTime(context->GetQuerySyncId(), context->GetDeviceId(), dbLastQueryTime);
        if (errCode == E_OK && dbLastQueryTime < lastQueryTime) {
            errCode = metadata_->SetLastQueryTime(context->GetQuerySyncId(), context->GetDeviceId(), lastQueryTime);
        }
        if (errCode != E_OK) {
            return errCode;
        }
    }
    if (!isAllDataHasSent_) {
        return InnerSyncStart(context);
    } else if (reSendMap_.empty()) {
        context->SetOperationStatus(SyncOperation::OP_SEND_FINISHED);
        InnerClearSyncStatus();
        return -E_FINISHED;
    }
    return E_OK;
}

void SingleVerDataSync::ClearSyncStatus()
{
    std::lock_guard<std::mutex> lock(lock_);
    InnerClearSyncStatus();
}

int SingleVerDataSync::ReSendData(SingleVerSyncTaskContext *context)
{
    if (reSendMap_.empty()) {
        LOGI("[DataSync] ReSend map empty");
        return -E_INTERNAL_ERROR;
    }
    uint32_t sequenceId = reSendMap_.begin()->first;
    ReSendInfo reSendInfo = reSendMap_.begin()->second;
    LOGI("[DataSync] ReSend mode=%d,start=%" PRIu64 ",end=%" PRIu64 ",delStart=%" PRIu64 ",delEnd=%" PRIu64 ","
        "seqId=%" PRIu32 ",packetId=%" PRIu64 ",windowsize=%d,label=%s,deviceId=%s", mode_, reSendInfo.start,
        reSendInfo.end, reSendInfo.deleteBeginTime, reSendInfo.deleteEndTime, sequenceId, reSendInfo.packetId,
        windowSize_, label_.c_str(), STR_MASK(deviceId_));
    DataSyncReSendInfo dataReSendInfo = {sessionId_, sequenceId, reSendInfo.start, reSendInfo.end,
        reSendInfo.deleteBeginTime, reSendInfo.deleteEndTime, reSendInfo.packetId};
    return ReSend(context, dataReSendInfo);
}

std::string SingleVerDataSync::GetLocalDeviceName()
{
    std::string deviceInfo;
    if (communicateHandle_ != nullptr) {
        communicateHandle_->GetLocalIdentity(deviceInfo);
    }
    return deviceInfo;
}

int SingleVerDataSync::Send(SingleVerSyncTaskContext *context, const Message *message, const CommErrHandler &handler,
    uint32_t packetLen)
{
    bool startFeedDogRet = false;
    if (packetLen > mtuSize_ && mtuSize_ > NOTIFY_MIN_MTU_SIZE) {
        uint32_t time = static_cast<uint32_t>(static_cast<uint64_t>(packetLen) *
            static_cast<uint64_t>(context->GetTimeoutTime()) / mtuSize_); // no overflow
        startFeedDogRet = context->StartFeedDogForSync(time, SyncDirectionFlag::SEND);
    }
    SendConfig sendConfig;
    SetSendConfigParam(storage_->GetDbProperties(), context->GetDeviceId(), false, SEND_TIME_OUT, sendConfig);
    int errCode = communicateHandle_->SendMessage(context->GetDeviceId(), message, sendConfig, handler);
    if (errCode != E_OK) {
        LOGE("[DataSync][Send] send message failed, errCode=%d", errCode);
        if (startFeedDogRet) {
            context->StopFeedDogForSync(SyncDirectionFlag::SEND);
        }
    }
    return errCode;
}

int SingleVerDataSync::GetDataWithPerformanceRecord(SingleVerSyncTaskContext *context,
    std::vector<SendDataItem> &outData, size_t packetSize)
{
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_READ_DATA);
    }
    // start a watch dog before get data
    // it will send ack util get data finished
    context->StartFeedDogForGetData(context->GetResponseSessionId());
    int errCode = GetData(context, packetSize, outData);
    context->StopFeedDogForGetData();
    if (performance != nullptr) {
        performance->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_READ_DATA);
    }
    if (!outData.empty()) {
        SingleVerDataSyncUtils::RecordClientId(*context, *storage_, metadata_);
    }
    return errCode;
}

int SingleVerDataSync::GetData(SingleVerSyncTaskContext *context, size_t packetSize, std::vector<SendDataItem> &outData)
{
    int errCode;
    UpdateMtuSize();
    if (context->GetRetryStatus() == SyncTaskContext::NEED_RETRY) {
        context->SetRetryStatus(SyncTaskContext::NO_NEED_RETRY);
        LOGI("[DataSync][GetData] resend data");
        errCode = GetUnsyncData(context, outData, packetSize);
    } else {
        ContinueToken token;
        context->GetContinueToken(token);
        if (token == nullptr) {
            errCode = GetUnsyncData(context, outData, packetSize);
        } else {
            LOGD("[DataSync][GetData] get data from token");
            // if there is data to send, read out data, and update local watermark, send data
            errCode = GetNextUnsyncData(context, outData, packetSize);
        }
    }
    if (errCode == -E_UNFINISHED) {
        LOGD("[DataSync][GetData] not finished.");
    }
    if (SingleVerDataSyncUtils::IsGetDataSuccessfully(errCode)) {
        std::string localHashName = DBCommon::TransferHashString(GetLocalDeviceName());
        SingleVerDataSyncUtils::TransDbDataItemToSendDataItem(localHashName, outData);
    }
    return errCode;
}

int SingleVerDataSync::GetMatchData(SingleVerSyncTaskContext *context, SyncEntry &syncOutData)
{
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    size_t packetSize = (version > SOFTWARE_VERSION_RELEASE_2_0) ?
        DBConstant::MAX_HPMODE_PACK_ITEM_SIZE : DBConstant::MAX_NORMAL_PACK_ITEM_SIZE;
    bool needCompressOnSync = false;
    uint8_t compressionRate = DBConstant::DEFAULT_COMPTRESS_RATE;
    (void)storage_->GetCompressionOption(needCompressOnSync, compressionRate);
    int errCode = GetDataWithPerformanceRecord(context, syncOutData.entries, packetSize);
    if (!SingleVerDataSyncUtils::IsGetDataSuccessfully(errCode)) {
        context->SetTaskErrCode(errCode);
        return errCode;
    }

    int innerCode = InterceptData(syncOutData);
    if (innerCode != E_OK) {
        context->SetTaskErrCode(innerCode);
        return innerCode;
    }

    CompressAlgorithm remoteAlgo = context->ChooseCompressAlgo();
    if (needCompressOnSync && remoteAlgo != CompressAlgorithm::NONE) {
        int compressCode = GenericSingleVerKvEntry::Compress(syncOutData.entries, syncOutData.compressedEntries,
            { remoteAlgo, version });
        if (compressCode != E_OK) {
            return compressCode;
        }
    }
    return errCode;
}

int SingleVerDataSync::GetUnsyncData(SingleVerSyncTaskContext *context, std::vector<SendDataItem> &outData,
    size_t packetSize)
{
    SyncTimeRange waterRange;
    DataSizeSpecInfo syncDataSizeInfo = GetDataSizeSpecInfo(packetSize);
    WaterMark startMark = 0;
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    GetLocalWaterMark(curType, context->GetQuerySyncId(), context, startMark);
    if ((waterRange.endTime == 0) || (startMark > waterRange.endTime)) {
        return E_OK;
    }
    waterRange.beginTime = startMark;
    if (curType == SyncType::QUERY_SYNC_TYPE) {
        WaterMark deletedStartMark = 0;
        GetLocalDeleteSyncWaterMark(context, deletedStartMark);
        Timestamp lastQueryTimestamp = 0;
        int errCode = metadata_->GetLastQueryTime(context->GetQuerySyncId(), context->GetDeviceId(),
            lastQueryTimestamp);
        if (errCode != E_OK) {
            return errCode;
        }
        waterRange.deleteBeginTime = deletedStartMark;
        waterRange.lastQueryTime = lastQueryTimestamp;
    }
    return GetUnsyncData(context, outData, syncDataSizeInfo, waterRange);
}

int SingleVerDataSync::GetUnsyncData(SingleVerSyncTaskContext *context, std::vector<SendDataItem> &outData,
    DataSizeSpecInfo syncDataSizeInfo, SyncTimeRange &waterMarkInfo)
{
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    ContinueToken token = nullptr;
    context->GetContinueToken(token);
    if (token != nullptr) {
        storage_->ReleaseContinueToken(token);
    }
    int errCode;
    if (curType != SyncType::QUERY_SYNC_TYPE) {
        errCode = storage_->GetSyncData(waterMarkInfo.beginTime, waterMarkInfo.endTime, outData, token,
            syncDataSizeInfo);
    } else {
        QuerySyncObject queryObj = context->GetQuery();
        errCode = storage_->GetSyncData(queryObj, waterMarkInfo, syncDataSizeInfo, token, outData);
    }
    context->SetContinueToken(token);
    if (!SingleVerDataSyncUtils::IsGetDataSuccessfully(errCode)) {
        LOGE("[DataSync][GetUnsyncData] get unsync data failed,errCode=%d", errCode);
    }
    return errCode;
}

int SingleVerDataSync::GetNextUnsyncData(SingleVerSyncTaskContext *context, std::vector<SendDataItem> &outData,
    size_t packetSize)
{
    ContinueToken token;
    context->GetContinueToken(token);
    DataSizeSpecInfo syncDataSizeInfo = GetDataSizeSpecInfo(packetSize);
    int errCode = storage_->GetSyncDataNext(outData, token, syncDataSizeInfo);
    context->SetContinueToken(token);
    if (!SingleVerDataSyncUtils::IsGetDataSuccessfully(errCode)) {
        LOGE("[DataSync][GetNextUnsyncData] get next unsync data failed, errCode=%d", errCode);
    }
    return errCode;
}

int SingleVerDataSync::SaveData(const SingleVerSyncTaskContext *context, const std::vector<SendDataItem> &inData,
    SyncType curType, const QuerySyncObject &query)
{
    if (inData.empty()) {
        return E_OK;
    }
    SingleVerDataSyncUtils::RecordClientId(*context, *storage_, metadata_);
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_SAVE_DATA);
    }
    const auto localDeviceName = GetLocalDeviceName();
    const std::string localHashName = DBCommon::TransferHashString(localDeviceName);
    SingleVerDataSyncUtils::TransSendDataItemToLocal(context, localHashName, inData);
    std::vector<SendDataItem> copyData = inData;
    int errCode = storage_->InterceptData(copyData, GetDeviceId(), localDeviceName, false);
    if (errCode != E_OK) {
        LOGE("[DataSync][SaveData] intercept data failed, errCode=%d", errCode);
        return errCode;
    }
    // query only support prefix key and don't have query in packet in 104 version
    errCode = storage_->PutSyncDataWithQuery(query, copyData, context->GetDeviceId());
    if (performance != nullptr) {
        performance->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_SAVE_DATA);
    }
    if (errCode != E_OK) {
        LOGE("[DataSync][SaveData] save sync data failed, errCode=%d", errCode);
    }
    return errCode;
}

void SingleVerDataSync::ResetSyncStatus(int inMode, SingleVerSyncTaskContext *context)
{
    mode_ = inMode;
    maxSequenceIdHasSent_ = 0;
    isAllDataHasSent_ = false;
    context->ReSetSequenceId();
    reSendMap_.clear();
    if (context->GetRemoteSoftwareVersion() < SOFTWARE_VERSION_RELEASE_3_0) {
        windowSize_ = LOW_VERSION_WINDOW_SIZE;
    } else {
        windowSize_ = HIGH_VERSION_WINDOW_SIZE;
    }
    int mode = SyncOperation::TransferSyncMode(inMode);
    if (mode == SyncModeType::PUSH || mode == SyncModeType::PUSH_AND_PULL || mode == SyncModeType::PULL) {
        sessionId_ = context->GetRequestSessionId();
    } else {
        sessionId_ = context->GetResponseSessionId();
    }
}

SyncTimeRange SingleVerDataSync::GetSyncDataTimeRange(SyncType syncType, SingleVerSyncTaskContext *context,
    const std::vector<SendDataItem> &inData, UpdateWaterMark &isUpdate)
{
    WaterMark localMark = 0;
    WaterMark deleteMark = 0;
    GetLocalWaterMark(syncType, context->GetQuerySyncId(), context, localMark);
    GetLocalDeleteSyncWaterMark(context, deleteMark);
    return SingleVerDataSyncUtils::GetSyncDataTimeRange(syncType, localMark, deleteMark, inData, isUpdate);
}

int SingleVerDataSync::SaveLocalWaterMark(SyncType syncType, const SingleVerSyncTaskContext *context,
    SyncTimeRange dataTimeRange, bool isCheckBeforUpdate) const
{
    WaterMark localMark = 0;
    int errCode = E_OK;
    const std::string &deviceId = context->GetDeviceId();
    std::string queryId = context->GetQuerySyncId();
    if (syncType != SyncType::QUERY_SYNC_TYPE) {
        if (isCheckBeforUpdate) {
            GetLocalWaterMark(syncType, queryId, context, localMark);
            if (localMark >= dataTimeRange.endTime) {
                return E_OK;
            }
        }
        errCode = metadata_->SaveLocalWaterMark(deviceId, dataTimeRange.endTime);
    } else {
        bool isNeedUpdateMark = true;
        bool isNeedUpdateDeleteMark = true;
        if (isCheckBeforUpdate) {
            WaterMark deleteDataWaterMark = 0;
            GetLocalWaterMark(syncType, queryId, context, localMark);
            GetLocalDeleteSyncWaterMark(context, deleteDataWaterMark);
            if (localMark >= dataTimeRange.endTime) {
                isNeedUpdateMark = false;
            }
            if (deleteDataWaterMark >= dataTimeRange.deleteEndTime) {
                isNeedUpdateDeleteMark = false;
            }
        }
        if (isNeedUpdateMark) {
            LOGD("label=%s,dev=%s,endTime=%" PRIu64, label_.c_str(), STR_MASK(GetDeviceId()), dataTimeRange.endTime);
            errCode = metadata_->SetSendQueryWaterMark(queryId, deviceId, dataTimeRange.endTime);
            if (errCode != E_OK) {
                LOGE("[DataSync][SaveLocalWaterMark] save query metadata watermark failed,errCode=%d", errCode);
                return errCode;
            }
        }
        if (isNeedUpdateDeleteMark) {
            LOGD("label=%s,dev=%s,deleteEndTime=%" PRIu64, label_.c_str(), STR_MASK(GetDeviceId()),
                dataTimeRange.deleteEndTime);
            errCode = metadata_->SetSendDeleteSyncWaterMark(context->GetDeleteSyncId(), dataTimeRange.deleteEndTime);
        }
    }
    if (errCode != E_OK) {
        LOGE("[DataSync][SaveLocalWaterMark] save metadata local watermark failed,errCode=%d", errCode);
    }
    return errCode;
}

void SingleVerDataSync::GetPeerWaterMark(SyncType syncType, const std::string &queryIdentify,
    const DeviceID &deviceId, WaterMark &waterMark) const
{
    if (syncType != SyncType::QUERY_SYNC_TYPE) {
        metadata_->GetPeerWaterMark(deviceId, waterMark);
        return;
    }
    metadata_->GetRecvQueryWaterMark(queryIdentify, deviceId, waterMark);
}

void SingleVerDataSync::GetPeerDeleteSyncWaterMark(const DeviceID &deviceId, WaterMark &waterMark)
{
    metadata_->GetRecvDeleteSyncWaterMark(deviceId, waterMark);
}

void SingleVerDataSync::GetLocalDeleteSyncWaterMark(const SingleVerSyncTaskContext *context,
    WaterMark &waterMark) const
{
    metadata_->GetSendDeleteSyncWaterMark(context->GetDeleteSyncId(), waterMark, context->IsAutoLiftWaterMark());
}

void SingleVerDataSync::GetLocalWaterMark(SyncType syncType, const std::string &queryIdentify,
    const SingleVerSyncTaskContext *context, WaterMark &waterMark) const
{
    if (syncType != SyncType::QUERY_SYNC_TYPE) {
        metadata_->GetLocalWaterMark(context->GetDeviceId(), waterMark);
        return;
    }
    metadata_->GetSendQueryWaterMark(queryIdentify, context->GetDeviceId(),
        waterMark, context->IsAutoLiftWaterMark());
}

int SingleVerDataSync::RemoveDeviceDataHandle(SingleVerSyncTaskContext *context, const Message *message,
    WaterMark maxSendDataTime)
{
    bool isNeedClearRemoteData = false;
    std::lock_guard<std::mutex> autoLock(removeDeviceDataLock_);
    if (context->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_3_0) {
        uint64_t clearDeviceDataMark = 0;
        metadata_->GetRemoveDataMark(context->GetDeviceId(), clearDeviceDataMark);
        isNeedClearRemoteData = (clearDeviceDataMark == REMOVE_DEVICE_DATA_MARK);
    } else {
        const DataRequestPacket *packet = message->GetObject<DataRequestPacket>();
        if (packet == nullptr) {
            LOGE("[RemoveDeviceDataHandle] get packet object failed");
            return -E_INVALID_ARGS;
        }
        SyncType curType = SyncOperation::GetSyncType(packet->GetMode());
        WaterMark packetLocalMark = packet->GetLocalWaterMark();
        WaterMark peerMark = 0;
        GetPeerWaterMark(curType, context->GetQuerySyncId(), context->GetDeviceId(), peerMark);
        isNeedClearRemoteData = ((packetLocalMark == 0) && (peerMark != 0));
    }
    if (!isNeedClearRemoteData) {
        return E_OK;
    }
    int errCode = E_OK;
    if (context->IsNeedClearRemoteStaleData()) {
        // need to clear remote device history data
        errCode = storage_->RemoveDeviceData(context->GetDeviceId(), true);
        if (errCode != E_OK) {
            (void)SendDataAck(context, message, errCode, maxSendDataTime);
            return errCode;
        }
        if (context->GetRemoteSoftwareVersion() == SOFTWARE_VERSION_EARLIEST) {
            // avoid repeat clear in ack
            metadata_->SaveLocalWaterMark(context->GetDeviceId(), 0);
        }
    }
    if (context->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_3_0) {
        errCode = metadata_->ResetMetaDataAfterRemoveData(context->GetDeviceId());
        if (errCode != E_OK) {
            (void)SendDataAck(context, message, errCode, maxSendDataTime);
            return errCode;
        }
    }
    return E_OK;
}

int SingleVerDataSync::DealRemoveDeviceDataByAck(SingleVerSyncTaskContext *context, WaterMark ackWaterMark,
    const std::vector<uint64_t> &reserved)
{
    bool isNeedClearRemoteData = false;
    std::lock_guard<std::mutex> autoLock(removeDeviceDataLock_);
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    if (context->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_3_0) {
        uint64_t clearDeviceDataMark = 0;
        metadata_->GetRemoveDataMark(context->GetDeviceId(), clearDeviceDataMark);
        isNeedClearRemoteData = (clearDeviceDataMark != 0);
    } else if (reserved.empty()) {
        WaterMark localMark = 0;
        GetLocalWaterMark(curType, context->GetQuery().GetIdentify(), context, localMark);
        isNeedClearRemoteData = ((localMark != 0) && (ackWaterMark == 0));
    } else {
        WaterMark peerMark = 0;
        GetPeerWaterMark(curType, context->GetQuerySyncId(),
            context->GetDeviceId(), peerMark);
        isNeedClearRemoteData = ((reserved[ACK_PACKET_RESERVED_INDEX_LOCAL_WATER_MARK] == 0) && (peerMark != 0));
    }
    if (!isNeedClearRemoteData) {
        return E_OK;
    }
    // need to clear remote historydata
    LOGI("[DataSync][WaterMarkException] AckRecv reserved not empty,rebuilted,clear historydata,label=%s,dev=%s",
        label_.c_str(), STR_MASK(GetDeviceId()));
    int errCode = storage_->RemoveDeviceData(context->GetDeviceId(), true);
    if (errCode != E_OK) {
        return errCode;
    }
    if (context->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_3_0) {
        errCode = metadata_->ResetMetaDataAfterRemoveData(context->GetDeviceId());
    }
    return errCode;
}

void SingleVerDataSync::SetSessionEndTimestamp(Timestamp end)
{
    sessionEndTimestamp_ = end;
}

Timestamp SingleVerDataSync::GetSessionEndTimestamp() const
{
    return sessionEndTimestamp_;
}

void SingleVerDataSync::UpdateSendInfo(SyncTimeRange dataTimeRange, SingleVerSyncTaskContext *context)
{
    ReSendInfo reSendInfo;
    reSendInfo.start = dataTimeRange.beginTime;
    reSendInfo.end = dataTimeRange.endTime;
    reSendInfo.deleteBeginTime = dataTimeRange.deleteBeginTime;
    reSendInfo.deleteEndTime = dataTimeRange.deleteEndTime;
    reSendInfo.packetId = context->GetPacketId();
    maxSequenceIdHasSent_++;
    reSendMap_[maxSequenceIdHasSent_] = reSendInfo;
    windowSize_--;
    ContinueToken token;
    context->GetContinueToken(token);
    if (token == nullptr) {
        isAllDataHasSent_ = true;
    }
    LOGI("[DataSync] mode=%d,start=%" PRIu64 ",end=%" PRIu64 ",deleteStart=%" PRIu64 ",deleteEnd=%" PRIu64 ","
        "seqId=%" PRIu32 ",packetId=%" PRIu64 ",window_size=%d,isAllSend=%d,label=%s,device=%s", mode_,
        reSendInfo.start, reSendInfo.end, reSendInfo.deleteBeginTime, reSendInfo.deleteEndTime, maxSequenceIdHasSent_,
        reSendInfo.packetId, windowSize_, isAllDataHasSent_, label_.c_str(), STR_MASK(deviceId_));
}

void SingleVerDataSync::FillDataRequestPacket(DataRequestPacket *packet, SingleVerSyncTaskContext *context,
    SyncEntry &syncData, int sendCode, int mode)
{
    SingleVerDataSyncUtils::SetDataRequestCommonInfo(*context, *storage_, *packet, metadata_);
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    WaterMark localMark = 0;
    WaterMark peerMark = 0;
    WaterMark deleteMark = 0;
    bool needCompressOnSync = false;
    uint8_t compressionRate = DBConstant::DEFAULT_COMPTRESS_RATE;
    (void)storage_->GetCompressionOption(needCompressOnSync, compressionRate);
    std::string id = context->GetQuerySyncId();
    GetLocalWaterMark(curType, id, context, localMark);
    GetPeerWaterMark(curType, id, context->GetDeviceId(), peerMark);
    GetLocalDeleteSyncWaterMark(context, deleteMark);
    if (((mode != SyncModeType::RESPONSE_PULL && sendCode == E_OK)) ||
        (mode == SyncModeType::RESPONSE_PULL && sendCode == SEND_FINISHED)) {
        packet->SetLastSequence();
    }
    int tmpMode = mode;
    if (mode == SyncModeType::RESPONSE_PULL) {
        tmpMode = (curType == SyncType::QUERY_SYNC_TYPE) ? SyncModeType::QUERY_PUSH : SyncModeType::PUSH;
    }
    packet->SetData(syncData.entries);
    packet->SetCompressData(syncData.compressedEntries);
    packet->SetBasicInfo(sendCode, version, tmpMode);
    packet->SetExtraConditions(RuntimeContext::GetInstance()->GetPermissionCheckParam(storage_->GetDbProperties()));
    packet->SetWaterMark(localMark, peerMark, deleteMark);
    if (SyncOperation::TransferSyncMode(mode) == SyncModeType::PUSH_AND_PULL) {
        packet->SetEndWaterMark(context->GetEndMark());
        packet->SetSessionId(context->GetRequestSessionId());
    }
    packet->SetQuery(context->GetQuery());
    packet->SetQueryId(context->GetQuerySyncId());
    CompressAlgorithm curAlgo = context->ChooseCompressAlgo();
    // empty compress data should not mark compress
    if (!syncData.compressedEntries.empty() && needCompressOnSync && curAlgo != CompressAlgorithm::NONE) {
        packet->SetCompressDataMark();
        packet->SetCompressAlgo(curAlgo);
    }
    SingleVerDataSyncUtils::SetPacketId(packet, context, version);
    if (curType == SyncType::QUERY_SYNC_TYPE && (context->GetQuery().HasLimit() ||
        context->GetQuery().HasOrderBy())) {
        packet->SetUpdateWaterMark();
    }
    LOGD("[DataSync] curType=%d,local=%" PRIu64 ",del=%" PRIu64 ",end=%" PRIu64 ",label=%s,dev=%s,queryId=%s,"
        "isCompress=%d", static_cast<int>(curType), localMark, deleteMark, context->GetEndMark(), label_.c_str(),
        STR_MASK(GetDeviceId()), STR_MASK(context->GetQuery().GetIdentify()), packet->IsCompressData());
}

int SingleVerDataSync::RequestStart(SingleVerSyncTaskContext *context, int mode)
{
    int errCode = QuerySyncCheck(context);
    if (errCode != E_OK) {
        LOGE("[DataSync][PushStart] check query failed, errCode=%d", errCode);
        return errCode;
    }
    errCode = RemoveDeviceDataIfNeed(context);
    if (errCode != E_OK) {
        context->SetTaskErrCode(errCode);
        return errCode;
    }
    SyncEntry syncData;
    // get data
    errCode = GetMatchData(context, syncData);
    SingleVerDataSyncUtils::TranslateErrCodeIfNeed(mode, context->GetRemoteSoftwareVersion(), errCode);

    if (!SingleVerDataSyncUtils::IsGetDataSuccessfully(errCode)) {
        LOGE("[DataSync][PushStart] get data failed, errCode=%d", errCode);
        return errCode;
    }

    DataRequestPacket *packet = new (std::nothrow) DataRequestPacket;
    if (packet == nullptr) {
        LOGE("[DataSync][PushStart] new DataRequestPacket error");
        return -E_OUT_OF_MEMORY;
    }
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    UpdateWaterMark isUpdateWaterMark;
    SyncTimeRange dataTime = GetSyncDataTimeRange(curType, context, syncData.entries, isUpdateWaterMark);
    if (errCode == E_OK) {
        SetSessionEndTimestamp(std::max(dataTime.endTime, dataTime.deleteEndTime));
    }
    FillDataRequestPacket(packet, context, syncData, errCode, mode);
    errCode = SendDataPacket(curType, packet, context);
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordEnd(PT_TEST_RECORDS::RECORD_MACHINE_START_TO_PUSH_SEND);
    }
    if (errCode == E_OK || errCode == -E_TIMEOUT) {
        UpdateSendInfo(dataTime, context);
    }
    if (errCode == E_OK) {
        if (curType == SyncType::QUERY_SYNC_TYPE && (context->GetQuery().HasLimit() ||
            context->GetQuery().HasOrderBy())) {
            LOGI("[DataSync][RequestStart] query contain limit/offset/orderby, no need to update watermark.");
            return E_OK;
        }
        SyncTimeRange tmpDataTime = SingleVerDataSyncUtils::ReviseLocalMark(curType, dataTime, isUpdateWaterMark);
        SaveLocalWaterMark(curType, context, tmpDataTime);
    }
    return errCode;
}

int SingleVerDataSync::PushStart(SingleVerSyncTaskContext *context)
{
    if (context == nullptr) {
        return -E_INVALID_ARGS;
    }
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    return RequestStart(context,
        (curType == SyncType::QUERY_SYNC_TYPE) ? SyncModeType::QUERY_PUSH : SyncModeType::PUSH);
}

int SingleVerDataSync::PushPullStart(SingleVerSyncTaskContext *context)
{
    if (context == nullptr) {
        return -E_INVALID_ARGS;
    }
    return RequestStart(context, context->GetMode());
}

int SingleVerDataSync::PullRequestStart(SingleVerSyncTaskContext *context)
{
    if (context == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = QuerySyncCheck(context);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = RemoveDeviceDataIfNeed(context);
    if (errCode != E_OK) {
        context->SetTaskErrCode(errCode);
        return errCode;
    }
    DataRequestPacket *packet = new (std::nothrow) DataRequestPacket;
    if (packet == nullptr) {
        LOGE("[DataSync][PullRequest]new DataRequestPacket error");
        return -E_OUT_OF_MEMORY;
    }
    SyncType syncType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    WaterMark peerMark = 0;
    WaterMark localMark = 0;
    WaterMark deleteMark = 0;
    GetPeerWaterMark(syncType, context->GetQuerySyncId(),
        context->GetDeviceId(), peerMark);
    GetLocalWaterMark(syncType, context->GetQuerySyncId(), context, localMark);
    GetLocalDeleteSyncWaterMark(context, deleteMark);
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    WaterMark endMark = context->GetEndMark();
    SyncTimeRange dataTime = {localMark, deleteMark, localMark, deleteMark};
    SingleVerDataSyncUtils::SetDataRequestCommonInfo(*context, *storage_, *packet, metadata_);
    packet->SetBasicInfo(E_OK, version, context->GetMode());
    packet->SetExtraConditions(RuntimeContext::GetInstance()->GetPermissionCheckParam(storage_->GetDbProperties()));
    packet->SetWaterMark(localMark, peerMark, deleteMark);
    packet->SetEndWaterMark(endMark);
    packet->SetSessionId(context->GetRequestSessionId());
    packet->SetQuery(context->GetQuery());
    packet->SetQueryId(context->GetQuerySyncId());
    packet->SetLastSequence();
    SingleVerDataSyncUtils::SetPacketId(packet, context, version);
    context->SetRetryStatus(SyncTaskContext::NO_NEED_RETRY);
    LOGD("[DataSync][Pull] curType=%d,local=%" PRIu64 ",del=%" PRIu64 ",end=%" PRIu64 ",peer=%" PRIu64 ",label=%s,"
        "dev=%s", static_cast<int>(syncType), localMark, deleteMark, endMark, peerMark,
        label_.c_str(), STR_MASK(GetDeviceId()));
    UpdateSendInfo(dataTime, context);
    return SendDataPacket(syncType, packet, context);
}

int SingleVerDataSync::PullResponseStart(SingleVerSyncTaskContext *context)
{
    if (context == nullptr) {
        return -E_INVALID_ARGS;
    }
    SyncEntry syncData;
    // get data
    int errCode = GetMatchData(context, syncData);
    if (!SingleVerDataSyncUtils::IsGetDataSuccessfully(errCode)) {
        if (context->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_2_0) {
            (void)SendPullResponseDataPkt(errCode, syncData, context);
        }
        return errCode;
    }
    // if send finished
    int ackCode = E_OK;
    ContinueToken token = nullptr;
    context->GetContinueToken(token);
    if (errCode == E_OK && token == nullptr) {
        LOGD("[DataSync][PullResponse] send last frame end");
        ackCode = SEND_FINISHED;
    }
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    UpdateWaterMark isUpdateWaterMark;
    SyncTimeRange dataTime = GetSyncDataTimeRange(curType, context, syncData.entries, isUpdateWaterMark);
    if (errCode == E_OK) {
        SetSessionEndTimestamp(std::max(dataTime.endTime, dataTime.deleteEndTime));
    }
    errCode = SendPullResponseDataPkt(ackCode, syncData, context);
    if (errCode == E_OK || errCode == -E_TIMEOUT) {
        UpdateSendInfo(dataTime, context);
    }
    if (errCode == E_OK) {
        if (curType == SyncType::QUERY_SYNC_TYPE && (context->GetQuery().HasLimit() ||
            context->GetQuery().HasOrderBy())) {
            LOGI("[DataSync][PullResponseStart] query contain limit/offset/orderby, no need to update watermark.");
            return E_OK;
        }
        SyncTimeRange tmpDataTime = SingleVerDataSyncUtils::ReviseLocalMark(curType, dataTime, isUpdateWaterMark);
        SaveLocalWaterMark(curType, context, tmpDataTime);
    }
    return errCode;
}

void SingleVerDataSync::UpdateQueryPeerWaterMark(SyncType syncType, const std::string &queryId,
    const SyncTimeRange &dataTime, const SingleVerSyncTaskContext *context, UpdateWaterMark isUpdateWaterMark)
{
    WaterMark tmpPeerWatermark = dataTime.endTime;
    WaterMark tmpPeerDeletedWatermark = dataTime.deleteEndTime;
    if (isUpdateWaterMark.normalUpdateMark) {
        tmpPeerWatermark++;
    }
    if (isUpdateWaterMark.deleteUpdateMark) {
        tmpPeerDeletedWatermark++;
    }
    UpdatePeerWaterMark(syncType, queryId, context, tmpPeerWatermark, tmpPeerDeletedWatermark);
}

void SingleVerDataSync::UpdatePeerWaterMark(SyncType syncType, const std::string &queryId,
    const SingleVerSyncTaskContext *context, WaterMark peerWatermark, WaterMark peerDeletedWatermark)
{
    if (peerWatermark == 0 && peerDeletedWatermark == 0) {
        return;
    }
    int errCode = E_OK;
    if (syncType != SyncType::QUERY_SYNC_TYPE) {
        errCode = metadata_->SavePeerWaterMark(context->GetDeviceId(), peerWatermark, true);
    } else {
        if (peerWatermark != 0) {
            LOGD("label=%s,dev=%s,endTime=%" PRIu64, label_.c_str(), STR_MASK(GetDeviceId()), peerWatermark);
            errCode = metadata_->SetRecvQueryWaterMark(queryId, context->GetDeviceId(), peerWatermark);
            if (errCode != E_OK) {
                LOGE("[DataSync][UpdatePeerWaterMark] save query peer water mark failed,errCode=%d", errCode);
            }
        }
        if (peerDeletedWatermark != 0) {
            LOGD("label=%s,dev=%s,peerDeletedTime=%" PRIu64,
                label_.c_str(), STR_MASK(GetDeviceId()), peerDeletedWatermark);
            errCode = metadata_->SetRecvDeleteSyncWaterMark(context->GetDeleteSyncId(), peerDeletedWatermark);
        }
    }
    if (errCode != E_OK) {
        LOGE("[DataSync][UpdatePeerWaterMark] save peer water mark failed,errCode=%d", errCode);
    }
}

int SingleVerDataSync::DoAbilitySyncIfNeed(SingleVerSyncTaskContext *context, const Message *message, bool isControlMsg)
{
    uint16_t remoteCommunicatorVersion = 0;
    if (communicateHandle_->GetRemoteCommunicatorVersion(context->GetDeviceId(), remoteCommunicatorVersion) ==
        -E_NOT_FOUND) {
        LOGE("[DataSync][DoAbilitySyncIfNeed] get remote communicator version failed");
        return -E_VERSION_NOT_SUPPORT;
    }
    // If remote is not the first version, we need check SOFTWARE_VERSION_BASE
    if (remoteCommunicatorVersion == 0) {
        LOGI("[DataSync] set remote version 0");
        context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_EARLIEST);
        return E_OK;
    } else {
        LOGI("[DataSync][DoAbilitySyncIfNeed] need do ability sync");
        if (isControlMsg) {
            SendControlAck(context, message, -E_NEED_ABILITY_SYNC, 0);
        } else {
            (void)SendDataAck(context, message, -E_NEED_ABILITY_SYNC, 0);
        }
        return -E_NEED_ABILITY_SYNC;
    }
}

int SingleVerDataSync::DataRequestRecvPre(SingleVerSyncTaskContext *context, const Message *message)
{
    if (context == nullptr || message == nullptr) {
        return -E_INVALID_ARGS;
    }
    auto *packet = message->GetObject<DataRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (context->GetRemoteSoftwareVersion() <= SOFTWARE_VERSION_BASE) {
        return DoAbilitySyncIfNeed(context, message);
    }
    int32_t sendCode = packet->GetSendCode();
    if (sendCode == -E_VERSION_NOT_SUPPORT) {
        LOGE("[DataSync] Version mismatch: ver=%u, current=%u", packet->GetVersion(), SOFTWARE_VERSION_CURRENT);
        (void)SendDataAck(context, message, -E_VERSION_NOT_SUPPORT, 0);
        return -E_WAIT_NEXT_MESSAGE;
    }
    // only deal with pull response packet errCode
    if (sendCode != E_OK && sendCode != SEND_FINISHED && sendCode != -E_UNFINISHED &&
        message->GetSessionId() == context->GetRequestSessionId()) {
        LOGE("[DataSync][DataRequestRecvPre] remote pullResponse getData sendCode=%d", sendCode);
        return sendCode;
    }
    int errCode = RunPermissionCheck(context, message, packet);
    if (errCode != E_OK) {
        return errCode;
    }
    if (std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT) > SOFTWARE_VERSION_RELEASE_2_0) {
        errCode = CheckSchemaStrategy(context, message);
    }
    if (errCode == E_OK) {
        errCode = SingleVerDataSyncUtils::RequestQueryCheck(packet, storage_);
    }
    if (errCode != E_OK) {
        (void)SendDataAck(context, message, errCode, 0);
        return errCode;
    }
    errCode = SingleVerDataSyncUtils::SchemaVersionMatchCheck(*context, *packet, metadata_);
    if (errCode != E_OK) {
        (void)SendDataAck(context, message, errCode, 0);
    }
    return errCode;
}

int SingleVerDataSync::DataRequestRecv(SingleVerSyncTaskContext *context, const Message *message,
    WaterMark &pullEndWatermark)
{
    int errCode = DataRequestRecvPre(context, message);
    if (errCode != E_OK) {
        return errCode;
    }
    const DataRequestPacket *packet = message->GetObject<DataRequestPacket>();
    const std::vector<SendDataItem> &data = packet->GetData();
    SyncType curType = SyncOperation::GetSyncType(packet->GetMode());
    LOGI("[DataSync][DataRequestRecv] curType=%d, remote ver=%" PRIu32 ", size=%zu, errCode=%d, queryId=%s,"
        " Label=%s, dev=%s", static_cast<int>(curType), packet->GetVersion(), data.size(), packet->GetSendCode(),
        STR_MASK(packet->GetQueryId()), label_.c_str(), STR_MASK(GetDeviceId()));
    context->SetReceiveWaterMarkErr(false);
    UpdateWaterMark isUpdateWaterMark;
    SyncTimeRange dataTime = SingleVerDataSyncUtils::GetRecvDataTimeRange(curType, data, isUpdateWaterMark);
    errCode = RemoveDeviceDataHandle(context, message, dataTime.endTime);
    if (errCode != E_OK) {
        return errCode;
    }
    Metadata::MetaWaterMarkAutoLock autoLock(metadata_);
    if (WaterMarkErrHandle(curType, context, message)) {
        return E_OK;
    }
    GetPullEndWatermark(context, packet, pullEndWatermark);
    // save data first
    errCode = SaveData(context, data, curType, packet->GetQuery());
    if (errCode != E_OK) {
        (void)SendDataAck(context, message, errCode, dataTime.endTime);
        return errCode;
    }
    SingleVerDataSyncUtils::UpdateSyncProcess(context, packet);

    if (pullEndWatermark > 0 && !storage_->IsReadable()) { // pull mode
        pullEndWatermark = 0;
        errCode = SendDataAck(context, message, -E_EKEYREVOKED, dataTime.endTime);
    } else {
        // if data is empty, we don't know the max timestap of this packet.
        errCode = SendDataAck(context, message, !data.empty() ? E_OK : WATER_MARK_INVALID, dataTime.endTime);
    }
    RemotePushFinished(packet->GetSendCode(), packet->GetMode(), message->GetSessionId(),
        context->GetRequestSessionId());
    if (curType != SyncType::QUERY_SYNC_TYPE && isUpdateWaterMark.normalUpdateMark) {
        UpdatePeerWaterMark(curType, "", context, dataTime.endTime + 1, 0);
    } else if (curType == SyncType::QUERY_SYNC_TYPE && packet->IsNeedUpdateWaterMark()) {
        UpdateQueryPeerWaterMark(curType, packet->GetQueryId(), dataTime, context, isUpdateWaterMark);
    }
    if (errCode != E_OK) {
        return errCode;
    }
    if (packet->GetSendCode() == SEND_FINISHED) {
        return -E_RECV_FINISHED;
    }
    return errCode;
}

int SingleVerDataSync::SendDataPacket(SyncType syncType, DataRequestPacket *packet,
    SingleVerSyncTaskContext *context)
{
    Message *message = new (std::nothrow) Message(SingleVerDataSyncUtils::GetMessageId(syncType));
    if (message == nullptr) {
        LOGE("[DataSync][SendDataPacket] new message error");
        delete packet;
        packet = nullptr;
        return -E_OUT_OF_MEMORY;
    }
    uint32_t packetLen = packet->CalculateLen(SingleVerDataSyncUtils::GetMessageId(syncType));
    int errCode = message->SetExternalObject(packet);
    if (errCode != E_OK) {
        delete packet;
        packet = nullptr;
        delete message;
        message = nullptr;
        LOGE("[DataSync][SendDataPacket] set external object failed errCode=%d", errCode);
        return errCode;
    }
    SingleVerDataSyncUtils::SetMessageHeadInfo(*message, TYPE_REQUEST, context->GetDeviceId(),
        context->GetSequenceId(), context->GetRequestSessionId());
    PerformanceAnalysis *performance = PerformanceAnalysis::GetInstance();
    if (performance != nullptr) {
        performance->StepTimeRecordStart(PT_TEST_RECORDS::RECORD_DATA_SEND_REQUEST_TO_ACK_RECV);
    }
    CommErrHandler handler = [this, context, sessionId = message->GetSessionId()](int ret) {
        SyncTaskContext::CommErrHandlerFunc(ret, context, sessionId);
    };
    errCode = Send(context, message, handler, packetLen);
    if (errCode != E_OK) {
        delete message;
        message = nullptr;
    }

    return errCode;
}

int SingleVerDataSync::SendPullResponseDataPkt(int ackCode, SyncEntry &syncOutData,
    SingleVerSyncTaskContext *context)
{
    DataRequestPacket *packet = new (std::nothrow) DataRequestPacket;
    if (packet == nullptr) {
        LOGE("[DataSync][SendPullResponseDataPkt] new data request packet error");
        return -E_OUT_OF_MEMORY;
    }
    SyncType syncType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    FillDataRequestPacket(packet, context, syncOutData, ackCode, SyncModeType::RESPONSE_PULL);

    if ((ackCode == E_OK || ackCode == SEND_FINISHED) &&
        SingleVerDataSyncUtils::IsSupportRequestTotal(packet->GetVersion())) {
        uint32_t total = 0u;
        (void)SingleVerDataSyncUtils::GetUnsyncTotal(context, storage_, total);
        LOGD("[SendPullResponseDataPkt] GetUnsyncTotal total=%u", total);
        packet->SetTotalDataCount(total);
    }
    uint32_t packetLen = packet->CalculateLen(SingleVerDataSyncUtils::GetMessageId(syncType));
    Message *message = new (std::nothrow) Message(SingleVerDataSyncUtils::GetMessageId(syncType));
    if (message == nullptr) {
        LOGE("[DataSync][SendPullResponseDataPkt] new message error");
        delete packet;
        packet = nullptr;
        return -E_OUT_OF_MEMORY;
    }
    int errCode = message->SetExternalObject(packet);
    if (errCode != E_OK) {
        delete packet;
        packet = nullptr;
        delete message;
        message = nullptr;
        LOGE("[SendPullResponseDataPkt] set external object failed, errCode=%d", errCode);
        return errCode;
    }
    SingleVerDataSyncUtils::SetMessageHeadInfo(*message, TYPE_REQUEST, context->GetDeviceId(),
        context->GetSequenceId(), context->GetResponseSessionId());
    SendResetWatchDogPacket(context, packetLen);
    errCode = Send(context, message, nullptr, packetLen);
    if (errCode != E_OK) {
        delete message;
        message = nullptr;
    }
    return errCode;
}

void SingleVerDataSync::SendFinishedDataAck(SingleVerSyncTaskContext *context, const Message *message)
{
    (void)SendDataAck(context, message, E_OK, 0);
}

int SingleVerDataSync::SendDataAck(SingleVerSyncTaskContext *context, const Message *message, int32_t recvCode,
    WaterMark maxSendDataTime)
{
    const DataRequestPacket *packet = message->GetObject<DataRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    Message *ackMessage = new (std::nothrow) Message(message->GetMessageId());
    if (ackMessage == nullptr) {
        LOGE("[DataSync][SendDataAck] new message error");
        return -E_OUT_OF_MEMORY;
    }
    DataAckPacket ack;
    SetAckPacket(ack, context, packet, recvCode, maxSendDataTime);
    int errCode = ackMessage->SetCopiedObject(ack);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
        LOGE("[DataSync][SendDataAck] set copied object failed, errcode=%d", errCode);
        return errCode;
    }
    SingleVerDataSyncUtils::SetMessageHeadInfo(*ackMessage, TYPE_RESPONSE, context->GetDeviceId(),
        message->GetSequenceId(), message->GetSessionId());

    errCode = Send(context, ackMessage, nullptr, 0);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
    }
    return errCode;
}

bool SingleVerDataSync::AckPacketIdCheck(const Message *message)
{
    if (message == nullptr) {
        LOGE("[DataSync] AckRecv message nullptr");
        return false;
    }
    if (message->GetMessageType() == TYPE_NOTIFY || message->IsFeedbackError()) {
        return true;
    }
    const DataAckPacket *packet = message->GetObject<DataAckPacket>();
    if (packet == nullptr) {
        return false;
    }
    uint64_t packetId = packet->GetPacketId(); // above 102 version data request reserve[0] store packetId value
    std::lock_guard<std::mutex> lock(lock_);
    uint32_t sequenceId = message->GetSequenceId();
    if (reSendMap_.count(sequenceId) != 0) {
        uint64_t originalPacketId = reSendMap_[sequenceId].packetId;
        if (DataAckPacket::IsPacketIdValid(packetId) && packetId != originalPacketId) {
            LOGE("[DataSync] packetId[%" PRIu64 "] is not match with original[%" PRIu64 "]", packetId,
                originalPacketId);
            return false;
        }
    }
    return true;
}

int SingleVerDataSync::AckRecv(SingleVerSyncTaskContext *context, const Message *message)
{
    int errCode = SingleVerDataSyncUtils::AckMsgErrnoCheck(context, message);
    if (errCode != E_OK) {
        return errCode;
    }
    const DataAckPacket *packet = message->GetObject<DataAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    int32_t recvCode = packet->GetRecvCode();
    LOGD("[DataSync][AckRecv] ver=%u,recvCode=%d,myversion=%u,label=%s,dev=%s", packet->GetVersion(), recvCode,
        SOFTWARE_VERSION_CURRENT, label_.c_str(), STR_MASK(GetDeviceId()));
    if (recvCode == -E_VERSION_NOT_SUPPORT) {
        LOGE("[DataSync][AckRecv] Version mismatch");
        return -E_VERSION_NOT_SUPPORT;
    }

    if (recvCode == -E_NEED_ABILITY_SYNC || recvCode == -E_NOT_PERMIT || recvCode == -E_NEED_TIME_SYNC) {
        // we should ReleaseContinueToken, avoid crash
        LOGI("[DataSync][AckRecv] Data sync abort,recvCode =%d,label =%s,dev=%s", recvCode, label_.c_str(),
            STR_MASK(GetDeviceId()));
        context->ReleaseContinueToken();
        return recvCode;
    }
    uint64_t data = packet->GetData();
    if (recvCode == LOCAL_WATER_MARK_NOT_INIT) {
        return DealWaterMarkException(context, data, packet->GetReserved());
    }

    if (recvCode == -E_SAVE_DATA_NOTIFY && data != 0) {
        // data only use low 32bit
        context->StartFeedDogForSync(static_cast<uint32_t>(data), SyncDirectionFlag::RECEIVE);
        LOGI("[DataSync][AckRecv] notify ResetWatchDog=%" PRIu64 ",label=%s,dev=%s", data, label_.c_str(),
            STR_MASK(GetDeviceId()));
    }

    if (recvCode != E_OK && recvCode != WATER_MARK_INVALID) {
        LOGW("[DataSync][AckRecv] Received a uncatched recvCode=%d,label=%s,dev=%s", recvCode,
            label_.c_str(), STR_MASK(GetDeviceId()));
        return recvCode;
    }

    // Judge if send finished
    ContinueToken token;
    context->GetContinueToken(token);
    if (((message->GetSessionId() == context->GetResponseSessionId()) ||
        (message->GetSessionId() == context->GetRequestSessionId())) && (token == nullptr)) {
        return -E_NO_DATA_SEND;
    }

    // send next data
    return -E_SEND_DATA;
}

void SingleVerDataSync::SendSaveDataNotifyPacket(SingleVerSyncTaskContext *context, uint32_t pktVersion,
    uint32_t sessionId, uint32_t sequenceId, uint32_t inMsgId)
{
    if (inMsgId != DATA_SYNC_MESSAGE && inMsgId != QUERY_SYNC_MESSAGE) {
        LOGE("[SingleVerDataSync] messageId not available.");
        return;
    }
    Message *ackMessage = new (std::nothrow) Message(inMsgId);
    if (ackMessage == nullptr) {
        LOGE("[DataSync][SaveDataNotify] new message failed");
        return;
    }

    DataAckPacket ack;
    ack.SetRecvCode(-E_SAVE_DATA_NOTIFY);
    ack.SetVersion(pktVersion);
    int errCode = ackMessage->SetCopiedObject(ack);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
        LOGE("[DataSync][SendSaveDataNotifyPacket] set copied object failed,errcode=%d", errCode);
        return;
    }
    SingleVerDataSyncUtils::SetMessageHeadInfo(*ackMessage, TYPE_NOTIFY, context->GetDeviceId(), sequenceId, sessionId);

    errCode = Send(context, ackMessage, nullptr, 0);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
    }
    LOGD("[DataSync][SaveDataNotify] Send SaveDataNotify packet Finished,errcode=%d,label=%s,dev=%s",
        errCode, label_.c_str(), STR_MASK(GetDeviceId()));
}

void SingleVerDataSync::GetPullEndWatermark(const SingleVerSyncTaskContext *context, const DataRequestPacket *packet,
    WaterMark &pullEndWatermark) const
{
    if (packet == nullptr) {
        return;
    }
    int mode = SyncOperation::TransferSyncMode(packet->GetMode());
    if ((mode == SyncModeType::PULL) || (mode == SyncModeType::PUSH_AND_PULL)) {
        WaterMark endMark = packet->GetEndWaterMark();
        TimeOffset offset;
        metadata_->GetTimeOffset(context->GetDeviceId(), offset);
        pullEndWatermark = endMark - static_cast<WaterMark>(offset);
        LOGD("[DataSync][PullEndWatermark] packetEndMark=%" PRIu64 ",offset=%" PRId64 ",endWaterMark=%" PRIu64 ","
            "label=%s,dev=%s", endMark, offset, pullEndWatermark, label_.c_str(), STR_MASK(GetDeviceId()));
    }
}

int SingleVerDataSync::DealWaterMarkException(SingleVerSyncTaskContext *context, WaterMark ackWaterMark,
    const std::vector<uint64_t> &reserved)
{
    WaterMark deletedWaterMark = 0;
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    if (curType == SyncType::QUERY_SYNC_TYPE) {
        if (reserved.size() <= ACK_PACKET_RESERVED_INDEX_DELETE_WATER_MARK) {
            LOGE("[DataSync] get packet reserve size failed");
            return -E_INVALID_ARGS;
        }
        deletedWaterMark = reserved[ACK_PACKET_RESERVED_INDEX_DELETE_WATER_MARK];
    }
    LOGI("[DataSync][WaterMarkException] AckRecv water error, mark=%" PRIu64 ",deleteMark=%" PRIu64 ","
        "label=%s,dev=%s", ackWaterMark, deletedWaterMark, label_.c_str(), STR_MASK(GetDeviceId()));
    int errCode = SaveLocalWaterMark(curType, context,
        {0, 0, ackWaterMark, deletedWaterMark});
    if (errCode != E_OK) {
        return errCode;
    }
    context->SetRetryStatus(SyncTaskContext::NEED_RETRY);
    context->IncNegotiationCount();
    SingleVerDataSyncUtils::PushAndPullKeyRevokHandle(context);
    if (!context->IsNeedClearRemoteStaleData()) {
        return -E_RE_SEND_DATA;
    }
    errCode = DealRemoveDeviceDataByAck(context, ackWaterMark, reserved);
    if (errCode != E_OK) {
        return errCode;
    }
    return -E_RE_SEND_DATA;
}

int SingleVerDataSync::RunPermissionCheck(SingleVerSyncTaskContext *context, const Message *message,
    const DataRequestPacket *packet)
{
    int mode = SyncOperation::TransferSyncMode(packet->GetMode());
    int errCode = SingleVerDataSyncUtils::RunPermissionCheck(context, storage_, label_, packet);
    if (errCode != E_OK) {
        if (context->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_EARLIEST) { // ver 101 can't handle this errCode
            (void)SendDataAck(context, message, -E_NOT_PERMIT, 0);
        }
        return -E_NOT_PERMIT;
    }
    const std::vector<SendDataItem> &data = packet->GetData();
    WaterMark maxSendDataTime = SingleVerDataSyncUtils::GetMaxSendDataTime(data);
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    auto securityOption = packet->GetSecurityOption();
    if (context->GetRemoteSeccurityOption().securityLabel == SecurityLabel::NOT_SET &&
        securityOption.securityLabel != SecurityLabel::NOT_SET) {
        context->SetRemoteSeccurityOption(packet->GetSecurityOption());
    }
    if (version > SOFTWARE_VERSION_RELEASE_2_0 && (mode != SyncModeType::PULL) &&
        !context->GetReceivcPermitCheck()) {
        bool permitReceive = SingleVerDataSyncUtils::CheckPermitReceiveData(context, communicateHandle_, storage_);
        if (permitReceive) {
            context->SetReceivcPermitCheck(true);
        } else {
            (void)SendDataAck(context, message, -E_SECURITY_OPTION_CHECK_ERROR, maxSendDataTime);
            return -E_SECURITY_OPTION_CHECK_ERROR;
        }
    }
    return errCode;
}

// used in pull response
void SingleVerDataSync::SendResetWatchDogPacket(SingleVerSyncTaskContext *context, uint32_t packetLen)
{
    // When mtu less than 30k, we send data with bluetooth
    // In order not to block the bluetooth channel, we don't send notify packet here
    if (mtuSize_ >= packetLen || mtuSize_ < NOTIFY_MIN_MTU_SIZE) {
        return;
    }
    uint64_t data = static_cast<uint64_t>(packetLen) * static_cast<uint64_t>(context->GetTimeoutTime()) / mtuSize_;

    Message *ackMessage = new (std::nothrow) Message(DATA_SYNC_MESSAGE);
    if (ackMessage == nullptr) {
        LOGE("[DataSync][ResetWatchDog] new message failed");
        return;
    }

    DataAckPacket ack;
    ack.SetData(data);
    ack.SetRecvCode(-E_SAVE_DATA_NOTIFY);
    ack.SetVersion(std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT));
    int errCode = ackMessage->SetCopiedObject(ack);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
        LOGE("[DataSync][ResetWatchDog] set copied object failed, errcode=%d", errCode);
        return;
    }
    SingleVerDataSyncUtils::SetMessageHeadInfo(*ackMessage, TYPE_NOTIFY, context->GetDeviceId(),
        context->GetSequenceId(), context->GetResponseSessionId());

    errCode = Send(context, ackMessage, nullptr, 0);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
        LOGE("[DataSync][ResetWatchDog] Send packet failed,errcode=%d,label=%s,dev=%s", errCode, label_.c_str(),
            STR_MASK(GetDeviceId()));
    } else {
        LOGI("[DataSync][ResetWatchDog] data = %" PRIu64 ",label=%s,dev=%s", data, label_.c_str(),
            STR_MASK(GetDeviceId()));
    }
}

int32_t SingleVerDataSync::ReSend(SingleVerSyncTaskContext *context, DataSyncReSendInfo reSendInfo)
{
    if (context == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = RemoveDeviceDataIfNeed(context);
    if (errCode != E_OK) {
        context->SetTaskErrCode(errCode);
        return errCode;
    }
    SyncEntry syncData;
    errCode = GetReSendData(syncData, context, reSendInfo);
    if (!SingleVerDataSyncUtils::IsGetDataSuccessfully(errCode)) {
        return errCode;
    }
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    DataRequestPacket *packet = new (std::nothrow) DataRequestPacket;
    if (packet == nullptr) {
        LOGE("[DataSync][ReSend] new DataRequestPacket error");
        return -E_OUT_OF_MEMORY;
    }
    FillRequestReSendPacket(context, packet, reSendInfo, syncData, errCode);
    errCode = SendReSendPacket(packet, context, reSendInfo.sessionId, reSendInfo.sequenceId);
    if (curType == SyncType::QUERY_SYNC_TYPE && (context->GetQuery().HasLimit() || context->GetQuery().HasOrderBy())) {
        LOGI("[DataSync][ReSend] query contain limit/offset/orderby, no need to update watermark.");
        return errCode;
    }
    if (errCode == E_OK && SyncOperation::TransferSyncMode(context->GetMode()) != SyncModeType::PULL) {
        // resend.end may not update in localwatermark while E_TIMEOUT occurred in send message last time.
        SyncTimeRange dataTime {reSendInfo.start, reSendInfo.deleteDataStart, reSendInfo.end, reSendInfo.deleteDataEnd};
        if (reSendInfo.deleteDataEnd > reSendInfo.deleteDataStart && curType == SyncType::QUERY_SYNC_TYPE) {
            dataTime.deleteEndTime += 1;
        }
        if (reSendInfo.end > reSendInfo.start) {
            dataTime.endTime += 1;
        }
        errCode = SaveLocalWaterMark(curType, context, dataTime, true);
        if (errCode != E_OK) {
            LOGE("[DataSync][ReSend] SaveLocalWaterMark failed.");
        }
    }
    return errCode;
}

int SingleVerDataSync::SendReSendPacket(DataRequestPacket *packet, SingleVerSyncTaskContext *context,
    uint32_t sessionId, uint32_t sequenceId)
{
    SyncType syncType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    Message *message = new (std::nothrow) Message(SingleVerDataSyncUtils::GetMessageId(syncType));
    if (message == nullptr) {
        LOGE("[DataSync][SendDataPacket] new message error");
        delete packet;
        packet = nullptr;
        return -E_OUT_OF_MEMORY;
    }
    uint32_t packetLen = packet->CalculateLen(SingleVerDataSyncUtils::GetMessageId(syncType));
    int errCode = message->SetExternalObject(packet);
    if (errCode != E_OK) {
        delete packet;
        packet = nullptr;
        delete message;
        message = nullptr;
        LOGE("[DataSync][SendReSendPacket] SetExternalObject failed errCode=%d", errCode);
        return errCode;
    }
    SingleVerDataSyncUtils::SetMessageHeadInfo(*message, TYPE_REQUEST, context->GetDeviceId(), sequenceId, sessionId);
    CommErrHandler handler = [this, context, sessionId = message->GetSessionId()](int ret) {
        SyncTaskContext::CommErrHandlerFunc(ret, context, sessionId);
    };
    errCode = Send(context, message, handler, packetLen);
    if (errCode != E_OK) {
        delete message;
        message = nullptr;
    }
    return errCode;
}

int SingleVerDataSync::CheckPermitSendData(int inMode, SingleVerSyncTaskContext *context)
{
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    int mode = SyncOperation::TransferSyncMode(inMode);
    // for pull mode it just need to get data, no need to send data.
    if (version <= SOFTWARE_VERSION_RELEASE_2_0 || mode == SyncModeType::PULL) {
        return E_OK;
    }
    if (context->GetSendPermitCheck()) {
        return E_OK;
    }
    bool isPermitSync = true;
    std::string deviceId = context->GetDeviceId();
    SecurityOption remoteSecOption = context->GetRemoteSeccurityOption();
    if (mode == SyncModeType::PUSH || mode == SyncModeType::PUSH_AND_PULL || mode == SyncModeType::RESPONSE_PULL) {
        isPermitSync = SingleVerDataSyncUtils::IsPermitRemoteDeviceRecvData(deviceId, remoteSecOption, storage_);
    }
    LOGI("[DataSync][PermitSendData] mode=%d,dev=%s,label=%d,flag=%d,PermitSync=%d", mode, STR_MASK(deviceId_),
        remoteSecOption.securityLabel, remoteSecOption.securityFlag, isPermitSync);
    if (isPermitSync) {
        context->SetSendPermitCheck(true);
        return E_OK;
    }
    if (mode == SyncModeType::PUSH || mode == SyncModeType::PUSH_AND_PULL) {
        context->SetTaskErrCode(-E_SECURITY_OPTION_CHECK_ERROR);
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    if (mode == SyncModeType::RESPONSE_PULL) {
        SyncEntry syncData;
        (void)SendPullResponseDataPkt(-E_SECURITY_OPTION_CHECK_ERROR, syncData, context);
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    if (mode == SyncModeType::SUBSCRIBE_QUERY) {
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    return E_OK;
}

std::string SingleVerDataSync::GetLabel() const
{
    return label_;
}

std::string SingleVerDataSync::GetDeviceId() const
{
    return deviceId_;
}

bool SingleVerDataSync::WaterMarkErrHandle(SyncType syncType, SingleVerSyncTaskContext *context, const Message *message)
{
    const DataRequestPacket *packet = message->GetObject<DataRequestPacket>();
    if (packet == nullptr) {
        LOGE("[WaterMarkErrHandle] get packet object failed");
        return -E_INVALID_ARGS;
    }
    WaterMark packetLocalMark = packet->GetLocalWaterMark();
    WaterMark packetDeletedMark = packet->GetDeletedWaterMark();
    WaterMark peerMark = 0;
    WaterMark deletedMark = 0;
    GetPeerWaterMark(syncType, packet->GetQueryId(), context->GetDeviceId(), peerMark);
    if (syncType == SyncType::QUERY_SYNC_TYPE) {
        GetPeerDeleteSyncWaterMark(context->GetDeleteSyncId(), deletedMark);
    }
    if (syncType != SyncType::QUERY_SYNC_TYPE && packetLocalMark > peerMark) {
        LOGI("[DataSync][DataRequestRecv] packetLocalMark=%" PRIu64 ",current=%" PRIu64, packetLocalMark, peerMark);
        context->SetReceiveWaterMarkErr(true);
        (void)SendDataAck(context, message, LOCAL_WATER_MARK_NOT_INIT, 0);
        return true;
    }
    if (syncType == SyncType::QUERY_SYNC_TYPE && (packetLocalMark > peerMark || packetDeletedMark > deletedMark)) {
        LOGI("[DataSync][DataRequestRecv] packetDeletedMark=%" PRIu64 ",deletedMark=%" PRIu64 ","
            "packetLocalMark=%" PRIu64 ",peerMark=%" PRIu64, packetDeletedMark, deletedMark, packetLocalMark,
            peerMark);
        context->SetReceiveWaterMarkErr(true);
        (void)SendDataAck(context, message, LOCAL_WATER_MARK_NOT_INIT, 0);
        return true;
    }
    return false;
}

int SingleVerDataSync::CheckSchemaStrategy(SingleVerSyncTaskContext *context, const Message *message)
{
    auto *packet = message->GetObject<DataRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (metadata_->IsAbilitySyncFinish(deviceId_)) {
        return E_OK;
    }
    auto query = packet->GetQuery();
    std::pair<bool, bool> schemaSyncStatus = context->GetSchemaSyncStatus(query);
    if (!schemaSyncStatus.second) {
        LOGE("[DataSync][CheckSchemaStrategy] isSchemaSync=%d check failed", schemaSyncStatus.second);
        (void)SendDataAck(context, message, -E_NEED_ABILITY_SYNC, 0);
        return -E_NEED_ABILITY_SYNC;
    }
    if (!schemaSyncStatus.first) {
        LOGE("[DataSync][CheckSchemaStrategy] Strategy permitSync=%d check failed", schemaSyncStatus.first);
        (void)SendDataAck(context, message, -E_SCHEMA_MISMATCH, 0);
        return -E_SCHEMA_MISMATCH;
    }
    return E_OK;
}

void SingleVerDataSync::RemotePushFinished(int sendCode, int inMode, uint32_t msgSessionId, uint32_t contextSessionId)
{
    int mode = SyncOperation::TransferSyncMode(inMode);
    if ((mode != SyncModeType::PUSH) && (mode != SyncModeType::PUSH_AND_PULL) && (mode != SyncModeType::QUERY_PUSH) &&
        (mode != SyncModeType::QUERY_PUSH_PULL)) {
        return;
    }

    if ((sendCode == E_OK) && (msgSessionId != 0) && (msgSessionId != contextSessionId))  {
        storage_->NotifyRemotePushFinished(deviceId_);
    }
}

void SingleVerDataSync::SetAckPacket(DataAckPacket &ackPacket, SingleVerSyncTaskContext *context,
    const DataRequestPacket *packet, int32_t recvCode, WaterMark maxSendDataTime)
{
    SyncType curType = SyncOperation::GetSyncType(packet->GetMode());
    WaterMark localMark = 0;
    GetLocalWaterMark(curType, packet->GetQueryId(), context, localMark);
    ackPacket.SetRecvCode(recvCode);
    // send ack packet
    if ((recvCode == E_OK) && (maxSendDataTime != 0)) {
        ackPacket.SetData(maxSendDataTime + 1); // + 1 to next start
    } else if (recvCode != WATER_MARK_INVALID) {
        WaterMark mark = 0;
        GetPeerWaterMark(curType, packet->GetQueryId(), context->GetDeviceId(), mark);
        ackPacket.SetData(mark);
    }
    std::vector<uint64_t> reserved {localMark};
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    uint64_t packetId = 0;
    if (version > SOFTWARE_VERSION_RELEASE_2_0) {
        packetId = packet->GetPacketId(); // above 102 version data request reserve[0] store packetId value
    }
    if (version > SOFTWARE_VERSION_RELEASE_2_0 && packetId > 0) {
        reserved.push_back(packetId);
    }
    // while recv is not E_OK, data is peerMark, reserve[2] is deletedPeerMark value
    if (curType == SyncType::QUERY_SYNC_TYPE && recvCode != WATER_MARK_INVALID) {
        WaterMark deletedPeerMark;
        GetPeerDeleteSyncWaterMark(context->GetDeleteSyncId(), deletedPeerMark);
        reserved.push_back(deletedPeerMark); // query sync mode, reserve[2] store deletedPeerMark value
    }
    ackPacket.SetReserved(reserved);
    ackPacket.SetVersion(version);
}

int SingleVerDataSync::GetReSendData(SyncEntry &syncData, SingleVerSyncTaskContext *context,
    DataSyncReSendInfo reSendInfo)
{
    int mode = SyncOperation::TransferSyncMode(context->GetMode());
    if (mode == SyncModeType::PULL) {
        return E_OK;
    }
    ContinueToken token = nullptr;
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    size_t packetSize = (version > SOFTWARE_VERSION_RELEASE_2_0) ?
        DBConstant::MAX_HPMODE_PACK_ITEM_SIZE : DBConstant::MAX_NORMAL_PACK_ITEM_SIZE;
    DataSizeSpecInfo reSendDataSizeInfo = GetDataSizeSpecInfo(packetSize);
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    int errCode;
    if (curType != SyncType::QUERY_SYNC_TYPE) {
        errCode = storage_->GetSyncData(reSendInfo.start, reSendInfo.end + 1, syncData.entries, token,
            reSendDataSizeInfo);
    } else {
        QuerySyncObject queryObj = context->GetQuery();
        errCode = storage_->GetSyncData(queryObj, SyncTimeRange { reSendInfo.start, reSendInfo.deleteDataStart,
            reSendInfo.end + 1, reSendInfo.deleteDataEnd + 1 }, reSendDataSizeInfo, token, syncData.entries);
    }
    if (token != nullptr) {
        storage_->ReleaseContinueToken(token);
    }
    if (errCode == -E_BUSY || errCode == -E_EKEYREVOKED) {
        context->SetTaskErrCode(errCode);
        return errCode;
    }
    if (!SingleVerDataSyncUtils::IsGetDataSuccessfully(errCode)) {
        return errCode;
    }
    SingleVerDataSyncUtils::TransDbDataItemToSendDataItem(
        DBCommon::TransferHashString(GetLocalDeviceName()), syncData.entries);

    int innerCode = InterceptData(syncData);
    if (innerCode != E_OK) {
        context->SetTaskErrCode(innerCode);
        return innerCode;
    }

    bool needCompressOnSync = false;
    uint8_t compressionRate = DBConstant::DEFAULT_COMPTRESS_RATE;
    (void)storage_->GetCompressionOption(needCompressOnSync, compressionRate);
    CompressAlgorithm remoteAlgo = context->ChooseCompressAlgo();
    if (needCompressOnSync && remoteAlgo != CompressAlgorithm::NONE) {
        int compressCode = GenericSingleVerKvEntry::Compress(syncData.entries, syncData.compressedEntries,
            { remoteAlgo, version });
        if (compressCode != E_OK) {
            return compressCode;
        }
    }
    return errCode;
}

int SingleVerDataSync::RemoveDeviceDataIfNeed(SingleVerSyncTaskContext *context)
{
    if (context->GetRemoteSoftwareVersion() <= SOFTWARE_VERSION_RELEASE_3_0) {
        return E_OK;
    }
    uint64_t clearRemoteDataMark = 0;
    std::lock_guard<std::mutex> autoLock(removeDeviceDataLock_);
    metadata_->GetRemoveDataMark(context->GetDeviceId(), clearRemoteDataMark);
    if (clearRemoteDataMark == 0) {
        return E_OK;
    }
    int errCode = E_OK;
    if (context->IsNeedClearRemoteStaleData() && clearRemoteDataMark == REMOVE_DEVICE_DATA_MARK) {
        errCode = storage_->RemoveDeviceData(context->GetDeviceId(), true);
        if (errCode != E_OK) {
            LOGE("clear remote %s data failed,errCode=%d", STR_MASK(GetDeviceId()), errCode);
            return errCode;
        }
    }
    if (clearRemoteDataMark == REMOVE_DEVICE_DATA_MARK) {
        errCode = metadata_->ResetMetaDataAfterRemoveData(context->GetDeviceId());
        if (errCode != E_OK) {
            LOGE("set %s removeDataWaterMark to false failed,errCode=%d", STR_MASK(GetDeviceId()), errCode);
            return errCode;
        }
    }
    return E_OK;
}

void SingleVerDataSync::UpdateMtuSize()
{
    mtuSize_ = communicateHandle_->GetCommunicatorMtuSize(deviceId_) * 9 / 10; // get the 9/10 of the size
}

void SingleVerDataSync::FillRequestReSendPacket(SingleVerSyncTaskContext *context, DataRequestPacket *packet,
    DataSyncReSendInfo reSendInfo, SyncEntry &syncData, int sendCode)
{
    SingleVerDataSyncUtils::SetDataRequestCommonInfo(*context, *storage_, *packet, metadata_);
    SyncType curType = (context->IsQuerySync()) ? SyncType::QUERY_SYNC_TYPE : SyncType::MANUAL_FULL_SYNC_TYPE;
    WaterMark peerMark = 0;
    GetPeerWaterMark(curType, context->GetQuerySyncId(), context->GetDeviceId(),
        peerMark);
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    // transfer reSend mode, RESPONSE_PULL transfer to push or query push
    // PUSH_AND_PULL mode which sequenceId lager than first transfer to push or query push
    int reSendMode = SingleVerDataSyncUtils::GetReSendMode(context->GetMode(), reSendInfo.sequenceId, curType);
    if (GetSessionEndTimestamp() == std::max(reSendInfo.end, reSendInfo.deleteDataEnd) ||
        SyncOperation::TransferSyncMode(context->GetMode()) == SyncModeType::PULL) {
        LOGI("[DataSync][ReSend] set lastid,label=%s,dev=%s", label_.c_str(), STR_MASK(GetDeviceId()));
        packet->SetLastSequence();
    }
    if (sendCode == E_OK && GetSessionEndTimestamp() == std::max(reSendInfo.end, reSendInfo.deleteDataEnd) &&
        context->GetMode() == SyncModeType::RESPONSE_PULL) {
        sendCode = SEND_FINISHED;
    }
    packet->SetData(syncData.entries);
    packet->SetCompressData(syncData.compressedEntries);
    packet->SetBasicInfo(sendCode, version, reSendMode);
    packet->SetExtraConditions(RuntimeContext::GetInstance()->GetPermissionCheckParam(storage_->GetDbProperties()));
    packet->SetWaterMark(reSendInfo.start, peerMark, reSendInfo.deleteDataStart);
    if (SyncOperation::TransferSyncMode(reSendMode) != SyncModeType::PUSH || context->IsQuerySync()) {
        packet->SetEndWaterMark(context->GetEndMark());
        packet->SetQuery(context->GetQuery());
    }
    packet->SetQueryId(context->GetQuerySyncId());
    if (version > SOFTWARE_VERSION_RELEASE_2_0) {
        std::vector<uint64_t> reserved {reSendInfo.packetId};
        packet->SetReserved(reserved);
    }
    if (reSendMode == SyncModeType::PULL || reSendMode == SyncModeType::QUERY_PULL) {
        // resend pull packet dont set compress type
        return;
    }
    bool needCompressOnSync = false;
    uint8_t compressionRate = DBConstant::DEFAULT_COMPTRESS_RATE;
    (void)storage_->GetCompressionOption(needCompressOnSync, compressionRate);
    CompressAlgorithm curAlgo = context->ChooseCompressAlgo();
    if (needCompressOnSync && curAlgo != CompressAlgorithm::NONE) {
        packet->SetCompressDataMark();
        packet->SetCompressAlgo(curAlgo);
    }
    FillRequestReSendPacketV2(context, packet);
}

void SingleVerDataSync::FillRequestReSendPacketV2(const SingleVerSyncTaskContext *context, DataRequestPacket *packet)
{
    if (context->GetMode() == SyncModeType::RESPONSE_PULL &&
        SingleVerDataSyncUtils::IsSupportRequestTotal(packet->GetVersion())) {
        uint32_t total = 0u;
        (void)SingleVerDataSyncUtils::GetUnsyncTotal(context, storage_, total);
        packet->SetTotalDataCount(total);
    }
}

DataSizeSpecInfo SingleVerDataSync::GetDataSizeSpecInfo(size_t packetSize)
{
    bool needCompressOnSync = false;
    uint8_t compressionRate = DBConstant::DEFAULT_COMPTRESS_RATE;
    (void)storage_->GetCompressionOption(needCompressOnSync, compressionRate);
    uint32_t blockSize = std::min(static_cast<uint32_t>(DBConstant::MAX_SYNC_BLOCK_SIZE),
        mtuSize_ * 100 / compressionRate);  // compressionRate max is 100
    return {blockSize, packetSize};
}

int SingleVerDataSync::InterceptData(SyncEntry &syncEntry)
{
    if (storage_ == nullptr) {
        LOGE("Invalid DB. Can not intercept data.");
        return -E_INVALID_DB;
    }

    // GetLocalDeviceName get local device ID.
    // GetDeviceId get remote device ID.
    // If intercept data fail, entries will be released.
    return storage_->InterceptData(syncEntry.entries, GetLocalDeviceName(), GetDeviceId(), true);
}

int SingleVerDataSync::ControlCmdStart(SingleVerSyncTaskContext *context)
{
    if (context == nullptr) {
        return -E_INVALID_ARGS;
    }
    std::shared_ptr<SubscribeManager> subManager = context->GetSubscribeManager();
    if (subManager == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = ControlCmdStartCheck(context);
    if (errCode != E_OK) {
        return errCode;
    }
    ControlRequestPacket* packet = new (std::nothrow) SubscribeRequest();
    if (packet == nullptr) {
        LOGE("[DataSync][ControlCmdStart] new SubscribeRequest error");
        return -E_OUT_OF_MEMORY;
    }
    if (context->GetMode() == SyncModeType::SUBSCRIBE_QUERY) {
        errCode = subManager->ReserveLocalSubscribeQuery(context->GetDeviceId(), context->GetQuery());
        if (errCode != E_OK) {
            LOGE("[DataSync][ControlCmdStart] reserve local subscribe query failed,err=%d", errCode);
            delete packet;
            packet = nullptr;
            return errCode;
        }
    }
    SingleVerDataSyncUtils::FillControlRequestPacket(packet, context);
    errCode = SendControlPacket(packet, context);
    if (errCode != E_OK && context->GetMode() == SyncModeType::SUBSCRIBE_QUERY) {
        subManager->DeleteLocalSubscribeQuery(context->GetDeviceId(), context->GetQuery());
    }
    return errCode;
}

int SingleVerDataSync::ControlCmdRequestRecv(SingleVerSyncTaskContext *context, const Message *message)
{
    const ControlRequestPacket *packet = message->GetObject<ControlRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    LOGI("[SingleVerDataSync] recv control cmd message,label=%s,dev=%s,controlType=%u", label_.c_str(),
        STR_MASK(GetDeviceId()), packet->GetcontrolCmdType());
    int errCode = ControlCmdRequestRecvPre(context, message);
    if (errCode != E_OK) {
        return errCode;
    }
    if (packet->GetcontrolCmdType() == ControlCmdType::SUBSCRIBE_QUERY_CMD) {
        errCode = SubscribeRequestRecv(context, message);
    } else if (packet->GetcontrolCmdType() == ControlCmdType::UNSUBSCRIBE_QUERY_CMD) {
        errCode = UnsubscribeRequestRecv(context, message);
    }
    return errCode;
}
} // namespace DistributedDB
