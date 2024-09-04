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
    int SingleVerDataSync::ControlCmdAckRecv(SingleVerSyncTaskContext *context, const Message *message)
{
    std::shared_ptr<SubscribeManager> subManager = context->GetSubscribeManager();
    if (subManager == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = SingleVerDataSyncUtils::AckMsgErrnoCheck(context, message);
    if (errCode != E_OK) {
        SingleVerDataSyncUtils::ControlAckErrorHandle(context, subManager);
        return errCode;
    }
    const ControlAckPacket *packet = message->GetObject<ControlAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    int32_t recvCode = packet->GetRecvCode();
    uint32_t cmdType = packet->GetcontrolCmdType();
    if (recvCode != E_OK) {
        LOGE("[DataSync][AckRecv] control sync abort,recvCode=%d,label=%s,dev=%s,type=%u", recvCode, label_.c_str(),
            STR_MASK(GetDeviceId()), cmdType);
        // for unsubscribe no need to do something
        SingleVerDataSyncUtils::ControlAckErrorHandle(context, subManager);
        return recvCode;
    }
    if (cmdType == ControlCmdType::SUBSCRIBE_QUERY_CMD) {
        errCode = subManager->ActiveLocalSubscribeQuery(context->GetDeviceId(), context->GetQuery());
    } else if (cmdType == ControlCmdType::UNSUBSCRIBE_QUERY_CMD) {
        subManager->RemoveLocalSubscribeQuery(context->GetDeviceId(), context->GetQuery());
    }
    if (errCode != E_OK) {
        LOGE("[DataSync] ack handle failed,label =%s,dev=%s,type=%u", label_.c_str(), STR_MASK(GetDeviceId()), cmdType);
        return errCode;
    }
    return -E_NO_DATA_SEND; // means control msg send finished
}

int SingleVerDataSync::ControlCmdStartCheck(SingleVerSyncTaskContext *context)
{
    if ((context->GetMode() != SyncModeType::SUBSCRIBE_QUERY) &&
        (context->GetMode() != SyncModeType::UNSUBSCRIBE_QUERY)) {
        LOGE("[ControlCmdStartCheck] not support controlCmd");
        return -E_INVALID_ARGS;
    }
    if (context->GetMode() == SyncModeType::SUBSCRIBE_QUERY &&
        context->GetQuery().HasInKeys() &&
        context->IsNotSupportAbility(SyncConfig::INKEYS_QUERY)) {
        return -E_NOT_SUPPORT;
    }
    if ((context->GetMode() != SyncModeType::SUBSCRIBE_QUERY) || context->GetReceivcPermitCheck()) {
        return E_OK;
    }
    bool permitReceive = SingleVerDataSyncUtils::CheckPermitReceiveData(context, communicateHandle_, storage_);
    if (permitReceive) {
        context->SetReceivcPermitCheck(true);
    } else {
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    return E_OK;
}

int SingleVerDataSync::SendControlPacket(ControlRequestPacket *packet, SingleVerSyncTaskContext *context)
{
    Message *message = new (std::nothrow) Message(CONTROL_SYNC_MESSAGE);
    if (message == nullptr) {
        LOGE("[DataSync][SendControlPacket] new message error");
        delete packet;
        packet = nullptr;
        return -E_OUT_OF_MEMORY;
    }
    uint32_t packetLen = packet->CalculateLen();
    int errCode = message->SetExternalObject(packet);
    if (errCode != E_OK) {
        delete packet;
        packet = nullptr;
        delete message;
        message = nullptr;
        LOGE("[DataSync][SendControlPacket] set external object failed errCode=%d", errCode);
        return errCode;
    }
    SingleVerDataSyncUtils::SetMessageHeadInfo(*message, TYPE_REQUEST, context->GetDeviceId(),
        context->GetSequenceId(), context->GetRequestSessionId());
    CommErrHandler handler = [this, context, sessionId = message->GetSessionId()](int ret, bool isDirectEnd) {
        SyncTaskContext::CommErrHandlerFunc(ret, context, sessionId, isDirectEnd);
    };
    errCode = Send(context, message, handler, packetLen);
    if (errCode != E_OK) {
        delete message;
        message = nullptr;
    }
    return errCode;
}

int SingleVerDataSync::SendControlAck(SingleVerSyncTaskContext *context, const Message *message, int32_t recvCode,
    uint32_t controlCmdType, const CommErrHandler &handler)
{
    Message *ackMessage = new (std::nothrow) Message(message->GetMessageId());
    if (ackMessage == nullptr) {
        LOGE("[DataSync][SendControlAck] new message error");
        return -E_OUT_OF_MEMORY;
    }
    uint32_t version = std::min(context->GetRemoteSoftwareVersion(), SOFTWARE_VERSION_CURRENT);
    ControlAckPacket ack;
    ack.SetPacketHead(recvCode, version, static_cast<int32_t>(controlCmdType), 0);
    int errCode = ackMessage->SetCopiedObject(ack);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
        LOGE("[DataSync][SendControlAck] set copied object failed, errcode=%d", errCode);
        return errCode;
    }
    SingleVerDataSyncUtils::SetMessageHeadInfo(*ackMessage, TYPE_RESPONSE, context->GetDeviceId(),
        message->GetSequenceId(), message->GetSessionId());
    errCode = Send(context, ackMessage, handler, 0);
    if (errCode != E_OK) {
        delete ackMessage;
        ackMessage = nullptr;
    }
    return errCode;
}

int SingleVerDataSync::ControlCmdRequestRecvPre(SingleVerSyncTaskContext *context, const Message *message)
{
    if (context == nullptr || message == nullptr) {
        return -E_INVALID_ARGS;
    }
    const ControlRequestPacket *packet = message->GetObject<ControlRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    uint32_t controlCmdType = packet->GetcontrolCmdType();
    if (context->GetRemoteSoftwareVersion() <= SOFTWARE_VERSION_BASE) {
        return DoAbilitySyncIfNeed(context, message, true);
    }
    if (controlCmdType >= ControlCmdType::INVALID_CONTROL_CMD) {
        SendControlAck(context, message, -E_NOT_SUPPORT, controlCmdType);
        return -E_WAIT_NEXT_MESSAGE;
    }
    return E_OK;
}

int SingleVerDataSync::SubscribeRequestRecvPre(SingleVerSyncTaskContext *context, const SubscribeRequest *packet,
    const Message *message)
{
    uint32_t controlCmdType = packet->GetcontrolCmdType();
    if (controlCmdType != ControlCmdType::SUBSCRIBE_QUERY_CMD) {
        return E_OK;
    }
    QuerySyncObject syncQuery = packet->GetQuery();
    int errCode;
    if (!packet->IsAutoSubscribe()) {
        errCode = storage_->CheckAndInitQueryCondition(syncQuery);
        if (errCode != E_OK) {
            LOGE("[SingleVerDataSync] check sync query failed,errCode=%d", errCode);
            SendControlAck(context, message, errCode, controlCmdType);
            return -E_WAIT_NEXT_MESSAGE;
        }
    }
    int mode = SingleVerDataSyncUtils::GetModeByControlCmdType(
        static_cast<ControlCmdType>(packet->GetcontrolCmdType()));
    if (mode >= SyncModeType::INVALID_MODE) {
        LOGE("[SingleVerDataSync] invalid mode");
        SendControlAck(context, message, -E_INVALID_ARGS, controlCmdType);
        return -E_WAIT_NEXT_MESSAGE;
    }
    errCode = CheckPermitSendData(mode, context);
    if (errCode != E_OK) {
        LOGE("[SingleVerDataSync] check sync query failed,errCode=%d", errCode);
        SendControlAck(context, message, errCode, controlCmdType);
    }
    return errCode;
}

int SingleVerDataSync::SubscribeRequestRecv(SingleVerSyncTaskContext *context, const Message *message)
{
    const SubscribeRequest *packet = message->GetObject<SubscribeRequest>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = SubscribeRequestRecvPre(context, packet, message);
    if (errCode != E_OK) {
        return errCode;
    }
    uint32_t controlCmdType = packet->GetcontrolCmdType();
    std::shared_ptr<SubscribeManager> subscribeManager = context->GetSubscribeManager();
    if (subscribeManager == nullptr) {
        LOGE("[SingleVerDataSync] subscribeManager check failed");
        SendControlAck(context, message, -E_NOT_REGISTER, controlCmdType);
        return -E_INVALID_ARGS;
    }
    errCode = storage_->AddSubscribe(packet->GetQuery().GetIdentify(), packet->GetQuery(), packet->IsAutoSubscribe());
    if (errCode != E_OK) {
        LOGE("[SingleVerDataSync] add trigger failed,err=%d,label=%s,dev=%s", errCode, label_.c_str(),
            STR_MASK(GetDeviceId()));
        SendControlAck(context, message, errCode, controlCmdType);
        return errCode;
    }
    errCode = subscribeManager->ReserveRemoteSubscribeQuery(context->GetDeviceId(), packet->GetQuery());
    if (errCode != E_OK) {
        LOGE("[SingleVerDataSync] add remote subscribe query failed,err=%d,label=%s,dev=%s", errCode, label_.c_str(),
            STR_MASK(GetDeviceId()));
        RemoveSubscribeIfNeed(packet->GetQuery().GetIdentify(), subscribeManager);
        SendControlAck(context, message, errCode, controlCmdType);
        return errCode;
    }
    errCode = SendControlAck(context, message, E_OK, controlCmdType);
    if (errCode != E_OK) {
        subscribeManager->DeleteRemoteSubscribeQuery(context->GetDeviceId(), packet->GetQuery());
        RemoveSubscribeIfNeed(packet->GetQuery().GetIdentify(), subscribeManager);
        LOGE("[SingleVerDataSync] send control msg failed,err=%d,label=%s,dev=%s", errCode, label_.c_str(),
            STR_MASK(GetDeviceId()));
        return errCode;
    }
    subscribeManager->ActiveRemoteSubscribeQuery(context->GetDeviceId(), packet->GetQuery());
    DBInfo dbInfo;
    storage_->GetDBInfo(dbInfo);
    RuntimeContext::GetInstance()->RecordRemoteSubscribe(dbInfo, context->GetDeviceId(), packet->GetQuery());
    return errCode;
}

int SingleVerDataSync::UnsubscribeRequestRecv(SingleVerSyncTaskContext *context, const Message *message)
{
    const SubscribeRequest *packet = message->GetObject<SubscribeRequest>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    uint32_t controlCmdType = packet->GetcontrolCmdType();
    std::shared_ptr<SubscribeManager> subscribeManager = context->GetSubscribeManager();
    if (subscribeManager == nullptr) {
        LOGE("[SingleVerDataSync] subscribeManager check failed");
        SendControlAck(context, message, -E_NOT_REGISTER, controlCmdType);
        return -E_INVALID_ARGS;
    }
    int errCode;
    std::lock_guard<std::mutex> autoLock(unsubscribeLock_);
    if (subscribeManager->IsLastRemoteContainSubscribe(context->GetDeviceId(), packet->GetQuery().GetIdentify())) {
        errCode = storage_->RemoveSubscribe(packet->GetQuery().GetIdentify());
        if (errCode != E_OK) {
            LOGE("[SingleVerDataSync] remove trigger failed,err=%d,label=%s,dev=%s", errCode, label_.c_str(),
                STR_MASK(GetDeviceId()));
            SendControlAck(context, message, errCode, controlCmdType);
            return errCode;
        }
    }
    errCode = SendControlAck(context, message, E_OK, controlCmdType);
    if (errCode != E_OK) {
        LOGE("[SingleVerDataSync] send control msg failed,err=%d,label=%s,dev=%s", errCode, label_.c_str(),
            STR_MASK(GetDeviceId()));
        return errCode;
    }
    subscribeManager->RemoveRemoteSubscribeQuery(context->GetDeviceId(), packet->GetQuery());
    DBInfo dbInfo;
    storage_->GetDBInfo(dbInfo);
    RuntimeContext::GetInstance()->RemoveRemoteSubscribe(dbInfo, context->GetDeviceId(), packet->GetQuery());
    metadata_->RemoveQueryFromRecordSet(context->GetDeviceId(), packet->GetQuery().GetIdentify());
    return errCode;
}

void SingleVerDataSync::PutDataMsg(Message *message)
{
    return msgSchedule_.PutMsg(message);
}

Message *SingleVerDataSync::MoveNextDataMsg(SingleVerSyncTaskContext *context, bool &isNeedHandle,
    bool &isNeedContinue)
{
    return msgSchedule_.MoveNextMsg(context, isNeedHandle, isNeedContinue);
}

bool SingleVerDataSync::IsNeedReloadQueue()
{
    return msgSchedule_.IsNeedReloadQueue();
}

void SingleVerDataSync::ScheduleInfoHandle(bool isNeedHandleStatus, bool isNeedClearMap, const Message *message)
{
    msgSchedule_.ScheduleInfoHandle(isNeedHandleStatus, isNeedClearMap, message);
}

void SingleVerDataSync::ClearDataMsg()
{
    msgSchedule_.ClearMsg();
}

int SingleVerDataSync::QuerySyncCheck(SingleVerSyncTaskContext *context)
{
    if (context == nullptr) {
        return -E_INVALID_ARGS;
    }
    bool isCheckStatus = false;
    int errCode = SingleVerDataSyncUtils::QuerySyncCheck(context, isCheckStatus);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCheckStatus) {
        context->SetTaskErrCode(-E_NOT_SUPPORT);
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

void SingleVerDataSync::RemoveSubscribeIfNeed(const std::string &queryId,
    const std::shared_ptr<SubscribeManager> &subscribeManager)
{
    if (!subscribeManager->IsQueryExistSubscribe(queryId)) {
        storage_->RemoveSubscribe(queryId);
    }
}
} // namespace DistributedDB