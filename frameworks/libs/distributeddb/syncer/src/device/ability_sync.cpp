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

#include "ability_sync.h"

#include "message_transform.h"
#include "version.h"
#include "db_errno.h"
#include "log_print.h"
#include "sync_types.h"
#include "db_common.h"
#include "single_ver_kvdb_sync_interface.h"
#include "single_ver_sync_task_context.h"
#include "single_ver_kv_sync_task_context.h"
#ifdef RELATIONAL_STORE
#include "relational_db_sync_interface.h"
#include "single_ver_relational_sync_task_context.h"
#endif

namespace DistributedDB {
AbilitySyncRequestPacket::AbilitySyncRequestPacket()
    : protocolVersion_(ABILITY_SYNC_VERSION_V1),
      sendCode_(E_OK),
      softwareVersion_(SOFTWARE_VERSION_CURRENT),
      secLabel_(0),
      secFlag_(0),
      schemaType_(0),
      dbCreateTime_(0),
      schemaVersion_(0)
{
}

AbilitySyncRequestPacket::~AbilitySyncRequestPacket()
{
}

void AbilitySyncRequestPacket::SetProtocolVersion(uint32_t protocolVersion)
{
    protocolVersion_ = protocolVersion;
}

uint32_t AbilitySyncRequestPacket::GetProtocolVersion() const
{
    return protocolVersion_;
}

void AbilitySyncRequestPacket::SetSendCode(int32_t sendCode)
{
    sendCode_ = sendCode;
}

int32_t AbilitySyncRequestPacket::GetSendCode() const
{
    return sendCode_;
}

void AbilitySyncRequestPacket::SetSoftwareVersion(uint32_t swVersion)
{
    softwareVersion_ = swVersion;
}

uint32_t AbilitySyncRequestPacket::GetSoftwareVersion() const
{
    return softwareVersion_;
}

void AbilitySyncRequestPacket::SetSchema(const std::string &schema)
{
    schema_ = schema;
}

std::string AbilitySyncRequestPacket::GetSchema() const
{
    return schema_;
}

void AbilitySyncRequestPacket::SetSchemaType(uint32_t schemaType)
{
    schemaType_ = schemaType;
}

uint32_t AbilitySyncRequestPacket::GetSchemaType() const
{
    return schemaType_;
}

void AbilitySyncRequestPacket::SetSecLabel(int32_t secLabel)
{
    secLabel_ = secLabel;
}

int32_t AbilitySyncRequestPacket::GetSecLabel() const
{
    return secLabel_;
}

void AbilitySyncRequestPacket::SetSecFlag(int32_t secFlag)
{
    secFlag_ = secFlag;
}

int32_t AbilitySyncRequestPacket::GetSecFlag() const
{
    return secFlag_;
}

void AbilitySyncRequestPacket::SetDbCreateTime(uint64_t dbCreateTime)
{
    dbCreateTime_ = dbCreateTime;
}

uint64_t AbilitySyncRequestPacket::GetDbCreateTime() const
{
    return dbCreateTime_;
}

uint32_t AbilitySyncRequestPacket::CalculateLen() const
{
    uint64_t len = 0;
    len += Parcel::GetUInt32Len(); // protocolVersion_
    len += Parcel::GetIntLen(); // sendCode_
    len += Parcel::GetUInt32Len(); // softwareVersion_
    uint32_t schemaLen = Parcel::GetStringLen(schema_);
    if (schemaLen == 0) {
        LOGE("[AbilitySyncRequestPacket][CalculateLen] schemaLen err!");
        return 0;
    }
    len += schemaLen;
    len += Parcel::GetIntLen(); // secLabel_
    len += Parcel::GetIntLen(); // secFlag_
    len += Parcel::GetUInt32Len(); // schemaType_
    len += Parcel::GetUInt64Len(); // dbCreateTime_
    len += DbAbility::CalculateLen(dbAbility_); // dbAbility_
    len += Parcel::GetUInt64Len(); // schema version add in 109
    // the reason why not 8-byte align is that old version is not 8-byte align
    // so it is not possible to set 8-byte align for high version.
    if (len > INT32_MAX) {
        LOGE("[AbilitySyncRequestPacket][CalculateLen] err len:%" PRIu64, len);
        return 0;
    }
    return len;
}

DbAbility AbilitySyncRequestPacket::GetDbAbility() const
{
    return dbAbility_;
}

void AbilitySyncRequestPacket::SetDbAbility(const DbAbility &dbAbility)
{
    dbAbility_ = dbAbility;
}

void AbilitySyncRequestPacket::SetSchemaVersion(uint64_t schemaVersion)
{
    schemaVersion_ = schemaVersion;
}

uint64_t AbilitySyncRequestPacket::GetSchemaVersion() const
{
    return schemaVersion_;
}

AbilitySyncAckPacket::AbilitySyncAckPacket()
    : protocolVersion_(ABILITY_SYNC_VERSION_V1),
      softwareVersion_(SOFTWARE_VERSION_CURRENT),
      ackCode_(E_OK),
      secLabel_(0),
      secFlag_(0),
      schemaType_(0),
      permitSync_(0),
      requirePeerConvert_(0),
      dbCreateTime_(0),
      schemaVersion_(0)
{
}

AbilitySyncAckPacket::~AbilitySyncAckPacket()
{
}

void AbilitySyncAckPacket::SetProtocolVersion(uint32_t protocolVersion)
{
    protocolVersion_ = protocolVersion;
}

void AbilitySyncAckPacket::SetSoftwareVersion(uint32_t swVersion)
{
    softwareVersion_ = swVersion;
}

uint32_t AbilitySyncAckPacket::GetSoftwareVersion() const
{
    return softwareVersion_;
}

uint32_t AbilitySyncAckPacket::GetProtocolVersion() const
{
    return protocolVersion_;
}

void AbilitySyncAckPacket::SetAckCode(int32_t ackCode)
{
    ackCode_ = ackCode;
}

int32_t AbilitySyncAckPacket::GetAckCode() const
{
    return ackCode_;
}

void AbilitySyncAckPacket::SetSchema(const std::string &schema)
{
    schema_ = schema;
}

std::string AbilitySyncAckPacket::GetSchema() const
{
    return schema_;
}

void AbilitySyncAckPacket::SetSchemaType(uint32_t schemaType)
{
    schemaType_ = schemaType;
}

uint32_t AbilitySyncAckPacket::GetSchemaType() const
{
    return schemaType_;
}

void AbilitySyncAckPacket::SetSecLabel(int32_t secLabel)
{
    secLabel_ = secLabel;
}

int32_t AbilitySyncAckPacket::GetSecLabel() const
{
    return secLabel_;
}

void AbilitySyncAckPacket::SetSecFlag(int32_t secFlag)
{
    secFlag_ = secFlag;
}

int32_t AbilitySyncAckPacket::GetSecFlag() const
{
    return secFlag_;
}

void AbilitySyncAckPacket::SetPermitSync(uint32_t permitSync)
{
    permitSync_ = permitSync;
}

uint32_t AbilitySyncAckPacket::GetPermitSync() const
{
    return permitSync_;
}

void AbilitySyncAckPacket::SetRequirePeerConvert(uint32_t requirePeerConvert)
{
    requirePeerConvert_ = requirePeerConvert;
}

uint32_t AbilitySyncAckPacket::GetRequirePeerConvert() const
{
    return requirePeerConvert_;
}

void AbilitySyncAckPacket::SetDbCreateTime(uint64_t dbCreateTime)
{
    dbCreateTime_ = dbCreateTime;
}

uint64_t AbilitySyncAckPacket::GetDbCreateTime() const
{
    return dbCreateTime_;
}

uint64_t AbilitySyncAckPacket::GetSchemaVersion() const
{
    return schemaVersion_;
}

void AbilitySyncAckPacket::SetSchemaVersion(uint64_t schemaVersion)
{
    schemaVersion_ = schemaVersion;
}

uint32_t AbilitySyncAckPacket::CalculateLen() const
{
    uint64_t len = 0;
    len += Parcel::GetUInt32Len();
    len += Parcel::GetUInt32Len();
    len += Parcel::GetIntLen();
    uint32_t schemaLen = Parcel::GetStringLen(schema_);
    if (schemaLen == 0) {
        LOGE("[AbilitySyncAckPacket][CalculateLen] schemaLen err!");
        return 0;
    }
    len += schemaLen;
    len += Parcel::GetIntLen(); // secLabel_
    len += Parcel::GetIntLen(); // secFlag_
    len += Parcel::GetUInt32Len(); // schemaType_
    len += Parcel::GetUInt32Len(); // permitSync_
    len += Parcel::GetUInt32Len(); // requirePeerConvert_
    len += Parcel::GetUInt64Len(); // dbCreateTime_
    len += DbAbility::CalculateLen(dbAbility_); // dbAbility_
    len += SchemaNegotiate::CalculateParcelLen(relationalSyncOpinion_);
    len += Parcel::GetUInt64Len(); // schemaVersion_
    if (len > INT32_MAX) {
        LOGE("[AbilitySyncAckPacket][CalculateLen] err len:%" PRIu64, len);
        return 0;
    }
    return len;
}

DbAbility AbilitySyncAckPacket::GetDbAbility() const
{
    return dbAbility_;
}

void AbilitySyncAckPacket::SetDbAbility(const DbAbility &dbAbility)
{
    dbAbility_ = dbAbility;
}

void AbilitySyncAckPacket::SetRelationalSyncOpinion(const RelationalSyncOpinion &relationalSyncOpinion)
{
    relationalSyncOpinion_ = relationalSyncOpinion;
}

RelationalSyncOpinion AbilitySyncAckPacket::GetRelationalSyncOpinion() const
{
    return relationalSyncOpinion_;
}

AbilitySync::AbilitySync()
    : communicator_(nullptr),
      storageInterface_(nullptr),
      metadata_(nullptr),
      syncFinished_(false)
{
}

AbilitySync::~AbilitySync()
{
    communicator_ = nullptr;
    storageInterface_ = nullptr;
    metadata_ = nullptr;
}

int AbilitySync::Initialize(ICommunicator *inCommunicator, ISyncInterface *inStorage,
    const std::shared_ptr<Metadata> &inMetadata, const std::string &deviceId)
{
    if (inCommunicator == nullptr || inStorage == nullptr || deviceId.empty() || inMetadata == nullptr) {
        return -E_INVALID_ARGS;
    }
    communicator_ = inCommunicator;
    storageInterface_ = inStorage;
    metadata_ = inMetadata;
    deviceId_ = deviceId;
    return E_OK;
}

int AbilitySync::SyncStart(uint32_t sessionId, uint32_t sequenceId, uint16_t remoteCommunicatorVersion,
    const CommErrHandler &handler, const ISyncTaskContext *context)
{
    AbilitySyncRequestPacket packet;
    int errCode = SetAbilityRequestBodyInfo(remoteCommunicatorVersion, context, packet);
    if (errCode != E_OK) {
        return errCode;
    }
    Message *message = new (std::nothrow) Message(ABILITY_SYNC_MESSAGE);
    if (message == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    message->SetMessageType(TYPE_REQUEST);
    errCode = message->SetCopiedObject<>(packet);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][SyncStart] SetCopiedObject failed, err %d", errCode);
        delete message;
        message = nullptr;
        return errCode;
    }
    message->SetVersion(MSG_VERSION_EXT);
    message->SetSessionId(sessionId);
    message->SetSequenceId(sequenceId);
    SendConfig conf;
    SetSendConfigParam(storageInterface_->GetDbProperties(), deviceId_, false, SEND_TIME_OUT, conf);
    errCode = communicator_->SendMessage(deviceId_, message, conf, handler);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][SyncStart] SendPacket failed, err %d", errCode);
        delete message;
        message = nullptr;
    }
    return errCode;
}

int AbilitySync::AckRecv(const Message *message, ISyncTaskContext *context)
{
    int errCode = AckMsgCheck(message, context);
    if (errCode != E_OK) {
        return errCode;
    }
    const AbilitySyncAckPacket *packet = message->GetObject<AbilitySyncAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    context->SetRemoteSoftwareVersion(remoteSoftwareVersion);
    metadata_->SetRemoteSoftwareVersion(context->GetDeviceId(), context->GetTargetUserId(), remoteSoftwareVersion);
    if (remoteSoftwareVersion > SOFTWARE_VERSION_RELEASE_2_0) {
        errCode = AckRecvWithHighVersion(message, context, packet);
    } else {
        std::string schema = packet->GetSchema();
        uint8_t schemaType = packet->GetSchemaType();
        bool isCompatible = static_cast<SyncGenericInterface *>(storageInterface_)->CheckCompatible(schema, schemaType);
        if (!isCompatible) { // LCOV_EXCL_BR_LINE
            (static_cast<SingleVerSyncTaskContext *>(context))->SetTaskErrCode(-E_SCHEMA_MISMATCH);
            LOGE("[AbilitySync][AckRecv] scheme check failed");
            return -E_SCHEMA_MISMATCH;
        }
        LOGI("[AbilitySync][AckRecv]remoteSoftwareVersion = %u, isCompatible = %d,", remoteSoftwareVersion,
            isCompatible);
    }
    return errCode;
}

int AbilitySync::RequestRecv(const Message *message, ISyncTaskContext *context)
{
    if (message == nullptr || context == nullptr) {
        return -E_INVALID_ARGS;
    }
    const AbilitySyncRequestPacket *packet = message->GetObject<AbilitySyncRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (packet->GetSendCode() == -E_VERSION_NOT_SUPPORT) {
        AbilitySyncAckPacket ackPacket;
        (void)SendAck(context, message, -E_VERSION_NOT_SUPPORT, false, ackPacket);
        LOGI("[AbilitySync][RequestRecv] version can not support, remote version is %u", packet->GetProtocolVersion());
        return -E_VERSION_NOT_SUPPORT;
    }

    std::string schema = packet->GetSchema();
    uint8_t schemaType = packet->GetSchemaType();
    bool isCompatible = static_cast<SyncGenericInterface *>(storageInterface_)->CheckCompatible(schema, schemaType);
    if (!isCompatible) {
        (static_cast<SingleVerSyncTaskContext *>(context))->SetTaskErrCode(-E_SCHEMA_MISMATCH);
    }
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    context->SetRemoteSoftwareVersion(remoteSoftwareVersion);
    return HandleRequestRecv(message, context, isCompatible);
}

int AbilitySync::AckNotifyRecv(const Message *message, ISyncTaskContext *context)
{
    if (message == nullptr || context == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (message->GetErrorNo() == E_FEEDBACK_UNKNOWN_MESSAGE) {
        LOGE("[AbilitySync][AckNotifyRecv] Remote device dose not support this message id");
        context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_EARLIEST);
        return -E_FEEDBACK_UNKNOWN_MESSAGE;
    }
    const AbilitySyncAckPacket *packet = message->GetObject<AbilitySyncAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    int errCode = packet->GetAckCode();
    if (errCode != E_OK) {
        LOGE("[AbilitySync][AckNotifyRecv] received an errCode %d", errCode);
        return errCode;
    }
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    context->SetRemoteSoftwareVersion(remoteSoftwareVersion);
    AbilitySyncAckPacket sendPacket;
    std::pair<bool, bool> schemaSyncStatus;
    errCode = HandleVersionV3AckSchemaParam(packet, sendPacket, context, false, schemaSyncStatus);
    errCode = errCode == -E_ABILITY_SYNC_FINISHED ? E_OK : errCode;
    int ackCode = errCode;
    LOGI("[AckNotifyRecv] receive dev = %s ack notify, remoteSoftwareVersion = %u, ackCode = %d",
        STR_MASK(deviceId_), remoteSoftwareVersion, errCode);
    if (errCode == E_OK) {
        ackCode = AbilitySync::LAST_NOTIFY;
    }
    (void)SendAckWithEmptySchema(context, message, ackCode, true);
    return errCode;
}

bool AbilitySync::GetAbilitySyncFinishedStatus() const
{
    return syncFinished_;
}

void AbilitySync::SetAbilitySyncFinishedStatus(bool syncFinished, ISyncTaskContext &context)
{
    syncFinished_ = syncFinished;
    if (context.GetRemoteSoftwareVersion() < SOFTWARE_VERSION_RELEASE_9_0) {
        return;
    }
    // record finished with all schema compatible
    if (syncFinished && !context.IsSchemaCompatible()) { // LCOV_EXCL_BR_LINE
        return;
    }
    int errCode = metadata_->SetAbilitySyncFinishMark(deviceId_, context.GetTargetUserId(), syncFinished);
    if (errCode != E_OK) {
        LOGW("[AbilitySync] Set ability sync finish mark failed %d", errCode);
    }
}

bool AbilitySync::SecLabelCheckInner(int32_t remoteSecLabel, int errCode, SecurityOption option,
    const AbilitySyncRequestPacket *packet) const
{
    if (remoteSecLabel == NOT_SUPPORT_SEC_CLASSIFICATION || remoteSecLabel == SecurityLabel::NOT_SET) {
        return true;
    }
    if (errCode == -E_NOT_SUPPORT || (errCode == E_OK && option.securityLabel == SecurityLabel::NOT_SET)) {
        return true;
    }
    if (remoteSecLabel == FAILED_GET_SEC_CLASSIFICATION || errCode != E_OK) {
        LOGE("[AbilitySync][RequestRecv] check error remoteL:%d, errCode:%d", remoteSecLabel, errCode);
        return false;
    }
    if (remoteSecLabel == option.securityLabel) {
        return true;
    }
    LOGE("[AbilitySync][RequestRecv] check error remote:%d , %d local:%d , %d",
         remoteSecLabel, packet->GetSecFlag(), option.securityLabel, option.securityFlag);
    return false;
}

bool AbilitySync::SecLabelCheck(const AbilitySyncRequestPacket *packet) const
{
    SecurityOption option;
    auto *syncInterface = static_cast<SyncGenericInterface *>(storageInterface_);
    if (syncInterface == nullptr) {
        LOGE("[AbilitySync][RequestRecv] get sync interface wrong");
        return false;
    }
    int errCode = syncInterface->GetSecurityOption(option);
    int32_t remoteSecLabel = TransformSecLabelIfNeed(packet->GetSecLabel(), option.securityLabel);
    LOGI("[AbilitySync][RequestRecv] remote label:%d local l:%d, f:%d, errCode:%d", remoteSecLabel,
        option.securityLabel, option.securityFlag, errCode);
    if (remoteSecLabel == NOT_SUPPORT_SEC_CLASSIFICATION && errCode == -E_NOT_SUPPORT) {
        return true;
    }
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    if (errCode != -E_NOT_SUPPORT && option.securityLabel == SecurityLabel::NOT_SET) {
        LOGE("[AbilitySync][RequestRecv] local security label not set!");
        return false;
    }
    if (remoteSoftwareVersion >= SOFTWARE_VERSION_RELEASE_7_0 && remoteSecLabel == SecurityLabel::NOT_SET) {
        LOGE("[AbilitySync][RequestRecv] remote security label not set!");
        return false;
    }
    return SecLabelCheckInner(remoteSecLabel, errCode, option, packet);
}

void AbilitySync::HandleVersionV3RequestParam(const AbilitySyncRequestPacket *packet, ISyncTaskContext *context)
{
    int32_t remoteSecLabel = packet->GetSecLabel();
    int32_t remoteSecFlag = packet->GetSecFlag();
    DbAbility remoteDbAbility = packet->GetDbAbility();
    (static_cast<SingleVerSyncTaskContext *>(context))->SetDbAbility(remoteDbAbility);
    (static_cast<SingleVerSyncTaskContext *>(context))->SetRemoteSeccurityOption({remoteSecLabel, remoteSecFlag});
    (static_cast<SingleVerSyncTaskContext *>(context))->SetReceivcPermitCheck(false);
    LOGI("[AbilitySync][HandleVersionV3RequestParam] remoteSecLabel = %d, remoteSecFlag = %d, remoteSchemaType = %u",
        remoteSecLabel, remoteSecFlag, packet->GetSchemaType());
}

void AbilitySync::HandleVersionV3AckSecOptionParam(const AbilitySyncAckPacket *packet,
    ISyncTaskContext *context)
{
    int32_t remoteSecLabel = packet->GetSecLabel();
    int32_t remoteSecFlag = packet->GetSecFlag();
    SecurityOption secOption = {remoteSecLabel, remoteSecFlag};
    (static_cast<SingleVerSyncTaskContext *>(context))->SetRemoteSeccurityOption(secOption);
    (static_cast<SingleVerSyncTaskContext *>(context))->SetSendPermitCheck(false);
    LOGI("[AbilitySync][AckRecv] remoteSecLabel = %d, remoteSecFlag = %d", remoteSecLabel, remoteSecFlag);
}

int AbilitySync::HandleVersionV3AckSchemaParam(const AbilitySyncAckPacket *recvPacket,
    AbilitySyncAckPacket &sendPacket,  ISyncTaskContext *context, bool sendOpinion,
    std::pair<bool, bool> &schemaSyncStatus)
{
    if (IsSingleRelationalVer()) {
        return HandleRelationAckSchemaParam(recvPacket, sendPacket, context, sendOpinion, schemaSyncStatus);
    }
    return HandleKvAckSchemaParam(recvPacket, context, sendPacket, schemaSyncStatus);
}

void AbilitySync::GetPacketSecOption(const ISyncTaskContext *context, SecurityOption &option) const
{
    int errCode =
        (static_cast<SyncGenericInterface *>(storageInterface_))->GetSecurityOption(option);
    if (errCode == -E_NOT_SUPPORT) {
        LOGE("[AbilitySync][SyncStart] GetSecOpt not surpport sec classification");
        option.securityLabel = NOT_SUPPORT_SEC_CLASSIFICATION;
    } else if (errCode != E_OK) {
        LOGE("[AbilitySync][SyncStart] GetSecOpt errCode:%d", errCode);
        option.securityLabel = FAILED_GET_SEC_CLASSIFICATION;
    }
    if (context == nullptr) {
        return;
    }
    auto remoteSecOption = (static_cast<const SingleVerSyncTaskContext *>(context))->GetRemoteSeccurityOption();
    option.securityLabel = TransformSecLabelIfNeed(option.securityLabel, remoteSecOption.securityLabel);
}

int AbilitySync::RegisterTransformFunc()
{
    TransformFunc func;
    func.computeFunc = [](const Message *inMsg) { return CalculateLen(inMsg); };
    func.serializeFunc = [](uint8_t *buffer, uint32_t length, const Message *inMsg) {
        return Serialization(buffer, length, inMsg);
    };
    func.deserializeFunc = [](const uint8_t *buffer, uint32_t length, Message *inMsg) {
        return DeSerialization(buffer, length, inMsg);
    };
    return MessageTransform::RegTransformFunction(ABILITY_SYNC_MESSAGE, func);
}

uint32_t AbilitySync::CalculateLen(const Message *inMsg)
{
    if ((inMsg == nullptr) || (inMsg->GetMessageId() != ABILITY_SYNC_MESSAGE)) {
        return 0;
    }
    int errCode;
    uint32_t len = 0;
    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            errCode = RequestPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[AbilitySync][CalculateLen] request packet calc length err %d", errCode);
            }
            break;
        case TYPE_RESPONSE:
            errCode = AckPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[AbilitySync][CalculateLen] ack packet calc length err %d", errCode);
            }
            break;
        case TYPE_NOTIFY:
            errCode = AckPacketCalculateLen(inMsg, len);
            if (errCode != E_OK) {
                LOGE("[AbilitySync][CalculateLen] ack packet calc length err %d", errCode);
            }
            break;
        default:
            LOGE("[AbilitySync][CalculateLen] message type not support, type %d", inMsg->GetMessageType());
            break;
    }
    return len;
}

int AbilitySync::Serialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    if ((buffer == nullptr) || (inMsg == nullptr)) {
        return -E_INVALID_ARGS;
    }

    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            return RequestPacketSerialization(buffer, length, inMsg);
        case TYPE_RESPONSE:
        case TYPE_NOTIFY:
            return AckPacketSerialization(buffer, length, inMsg);
        default:
            return -E_MESSAGE_TYPE_ERROR;
    }
}

int AbilitySync::DeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    if ((buffer == nullptr) || (inMsg == nullptr)) {
        return -E_INVALID_ARGS;
    }

    switch (inMsg->GetMessageType()) {
        case TYPE_REQUEST:
            return RequestPacketDeSerialization(buffer, length, inMsg);
        case TYPE_RESPONSE:
        case TYPE_NOTIFY:
            return AckPacketDeSerialization(buffer, length, inMsg);
        default:
            return -E_MESSAGE_TYPE_ERROR;
    }
}

int AbilitySync::RequestPacketCalculateLen(const Message *inMsg, uint32_t &len)
{
    const AbilitySyncRequestPacket *packet = inMsg->GetObject<AbilitySyncRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }

    len = packet->CalculateLen();
    return E_OK;
}

int AbilitySync::AckPacketCalculateLen(const Message *inMsg, uint32_t &len)
{
    const AbilitySyncAckPacket *packet = inMsg->GetObject<AbilitySyncAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }

    len = packet->CalculateLen();
    return E_OK;
}

int AbilitySync::RequestPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    const AbilitySyncRequestPacket *packet = inMsg->GetObject<AbilitySyncRequestPacket>();
    if ((packet == nullptr) || (length != packet->CalculateLen())) {
        return -E_INVALID_ARGS;
    }

    Parcel parcel(buffer, length);
    parcel.WriteUInt32(packet->GetProtocolVersion());
    parcel.WriteInt(packet->GetSendCode());
    parcel.WriteUInt32(packet->GetSoftwareVersion());
    parcel.WriteString(packet->GetSchema());
    parcel.WriteInt(packet->GetSecLabel());
    parcel.WriteInt(packet->GetSecFlag());
    parcel.WriteUInt32(packet->GetSchemaType());
    parcel.WriteUInt64(packet->GetDbCreateTime());
    int errCode = DbAbility::Serialize(parcel, packet->GetDbAbility());
    parcel.WriteUInt64(packet->GetSchemaVersion());
    if (parcel.IsError() || errCode != E_OK) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int AbilitySync::AckPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg)
{
    const AbilitySyncAckPacket *packet = inMsg->GetObject<AbilitySyncAckPacket>();
    if ((packet == nullptr) || (length != packet->CalculateLen())) {
        return -E_INVALID_ARGS;
    }

    Parcel parcel(buffer, length);
    parcel.WriteUInt32(ABILITY_SYNC_VERSION_V1);
    parcel.WriteUInt32(SOFTWARE_VERSION_CURRENT);
    parcel.WriteInt(packet->GetAckCode());
    parcel.WriteString(packet->GetSchema());
    parcel.WriteInt(packet->GetSecLabel());
    parcel.WriteInt(packet->GetSecFlag());
    parcel.WriteUInt32(packet->GetSchemaType());
    parcel.WriteUInt32(packet->GetPermitSync());
    parcel.WriteUInt32(packet->GetRequirePeerConvert());
    parcel.WriteUInt64(packet->GetDbCreateTime());
    int errCode = DbAbility::Serialize(parcel, packet->GetDbAbility());
    if (parcel.IsError() || errCode != E_OK) {
        return -E_PARSE_FAIL;
    }
    errCode = SchemaNegotiate::SerializeData(packet->GetRelationalSyncOpinion(), parcel);
    if (parcel.IsError() || errCode != E_OK) {
        return -E_PARSE_FAIL;
    }
    parcel.WriteUInt64(packet->GetSchemaVersion());
    if (parcel.IsError()) {
        LOGE("[AbilitySync] Serialize schema version failed");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int AbilitySync::RequestPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    auto *packet = new (std::nothrow) AbilitySyncRequestPacket();
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    uint32_t version = 0;
    uint32_t softwareVersion = 0;
    std::string schema;
    int32_t sendCode = 0;
    int errCode = -E_PARSE_FAIL;

    parcel.ReadUInt32(version);
    if (parcel.IsError()) {
        goto ERROR_OUT;
    }
    packet->SetProtocolVersion(version);
    if (version > ABILITY_SYNC_VERSION_V1) {
        packet->SetSendCode(-E_VERSION_NOT_SUPPORT);
        errCode = inMsg->SetExternalObject<>(packet);
        if (errCode != E_OK) {
            goto ERROR_OUT;
        }
        return errCode;
    }
    parcel.ReadInt(sendCode);
    parcel.ReadUInt32(softwareVersion);
    parcel.ReadString(schema);
    errCode = RequestPacketDeSerializationTailPart(parcel, packet, softwareVersion);
    if (parcel.IsError() || errCode != E_OK) {
        goto ERROR_OUT;
    }
    packet->SetSendCode(sendCode);
    packet->SetSoftwareVersion(softwareVersion);
    packet->SetSchema(schema);

    errCode = inMsg->SetExternalObject<>(packet);
    if (errCode == E_OK) {
        return E_OK;
    }

ERROR_OUT:
    delete packet;
    return errCode;
}

int AbilitySync::RequestPacketDeSerializationTailPart(Parcel &parcel, AbilitySyncRequestPacket *packet,
    uint32_t version)
{
    if (!parcel.IsError() && version > SOFTWARE_VERSION_RELEASE_2_0) {
        int32_t secLabel = 0;
        int32_t secFlag = 0;
        uint32_t schemaType = 0;
        parcel.ReadInt(secLabel);
        parcel.ReadInt(secFlag);
        parcel.ReadUInt32(schemaType);
        packet->SetSecLabel(secLabel);
        packet->SetSecFlag(secFlag);
        packet->SetSchemaType(schemaType);
    }
    if (!parcel.IsError() && version > SOFTWARE_VERSION_RELEASE_3_0) {
        uint64_t dbCreateTime = 0;
        parcel.ReadUInt64(dbCreateTime);
        packet->SetDbCreateTime(dbCreateTime);
    }
    DbAbility remoteDbAbility;
    int errCode = DbAbility::DeSerialize(parcel, remoteDbAbility);
    if (errCode != E_OK) {
        LOGE("[AbilitySync] request packet DeSerializ failed.");
        return errCode;
    }
    packet->SetDbAbility(remoteDbAbility);
    if (version >= SOFTWARE_VERSION_RELEASE_9_0) {
        uint64_t schemaVersion = 0u;
        parcel.ReadUInt64(schemaVersion);
        if (parcel.IsError()) {
            LOGW("[AbilitySync] request packet read schema version failed");
            return -E_PARSE_FAIL;
        }
        packet->SetSchemaVersion(schemaVersion);
    }
    return E_OK;
}

int AbilitySync::AckPacketDeSerializationTailPart(Parcel &parcel, AbilitySyncAckPacket *packet, uint32_t version)
{
    if (!parcel.IsError() && version > SOFTWARE_VERSION_RELEASE_2_0) {
        int32_t secLabel = 0;
        int32_t secFlag = 0;
        uint32_t schemaType = 0;
        uint32_t permitSync = 0;
        uint32_t requirePeerConvert = 0;
        parcel.ReadInt(secLabel);
        parcel.ReadInt(secFlag);
        parcel.ReadUInt32(schemaType);
        parcel.ReadUInt32(permitSync);
        parcel.ReadUInt32(requirePeerConvert);
        packet->SetSecLabel(secLabel);
        packet->SetSecFlag(secFlag);
        packet->SetSchemaType(schemaType);
        packet->SetPermitSync(permitSync);
        packet->SetRequirePeerConvert(requirePeerConvert);
    }
    if (!parcel.IsError() && version > SOFTWARE_VERSION_RELEASE_3_0) {
        uint64_t dbCreateTime = 0;
        parcel.ReadUInt64(dbCreateTime);
        packet->SetDbCreateTime(dbCreateTime);
    }
    DbAbility remoteDbAbility;
    int errCode = DbAbility::DeSerialize(parcel, remoteDbAbility);
    if (errCode != E_OK) {
        LOGE("[AbilitySync] ack packet DeSerializ failed.");
        return errCode;
    }
    packet->SetDbAbility(remoteDbAbility);
    RelationalSyncOpinion relationalSyncOpinion;
    errCode = SchemaNegotiate::DeserializeData(parcel, relationalSyncOpinion);
    if (errCode != E_OK) {
        LOGE("[AbilitySync] ack packet DeSerializ RelationalSyncOpinion failed.");
        return errCode;
    }
    packet->SetRelationalSyncOpinion(relationalSyncOpinion);
    if (version >= SOFTWARE_VERSION_RELEASE_9_0) {
        uint64_t schemaVersion = 0;
        parcel.ReadUInt64(schemaVersion);
        if (parcel.IsError()) {
            LOGW("[AbilitySync] ack packet read schema version failed.");
            return -E_PARSE_FAIL;
        }
        packet->SetSchemaVersion(schemaVersion);
    }
    return E_OK;
}

int AbilitySync::AckPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg)
{
    auto *packet = new (std::nothrow) AbilitySyncAckPacket();
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }

    Parcel parcel(const_cast<uint8_t *>(buffer), length);
    uint32_t version = 0;
    uint32_t softwareVersion = 0;
    int32_t ackCode = E_OK;
    std::string schema;
    int errCode;
    parcel.ReadUInt32(version);
    if (parcel.IsError()) {
        LOGE("[AbilitySync][RequestDeSerialization] read version failed!");
        errCode = -E_PARSE_FAIL;
        goto ERROR_OUT;
    }
    packet->SetProtocolVersion(version);
    if (version > ABILITY_SYNC_VERSION_V1) {
        packet->SetAckCode(-E_VERSION_NOT_SUPPORT);
        errCode = inMsg->SetExternalObject<>(packet);
        if (errCode != E_OK) {
            goto ERROR_OUT;
        }
        return errCode;
    }
    parcel.ReadUInt32(softwareVersion);
    parcel.ReadInt(ackCode);
    parcel.ReadString(schema);
    errCode = AckPacketDeSerializationTailPart(parcel, packet, softwareVersion);
    if (parcel.IsError() || errCode != E_OK) {
        LOGE("[AbilitySync][RequestDeSerialization] DeSerialization failed!");
        errCode = -E_PARSE_FAIL;
        goto ERROR_OUT;
    }
    packet->SetSoftwareVersion(softwareVersion);
    packet->SetAckCode(ackCode);
    packet->SetSchema(schema);
    errCode = inMsg->SetExternalObject<>(packet);
    if (errCode == E_OK) {
        return E_OK;
    }

ERROR_OUT:
    delete packet;
    return errCode;
}

void AbilitySync::SetAbilityRequestBodyInfoInner(uint16_t remoteCommunicatorVersion, AbilitySyncRequestPacket &packet,
    std::string &schemaStr, uint32_t schemaType) const
{
    // 102 version is forbidden to sync with 103 json-schema or flatbuffer-schema
    // so schema should put null string while remote is 102 version to avoid this bug.
    if (remoteCommunicatorVersion == 1) {
        packet.SetSchema("");
        packet.SetSchemaType(0);
    } else {
        packet.SetSchema(schemaStr);
        packet.SetSchemaType(schemaType);
    }
}

int AbilitySync::SetAbilityRequestBodyInfo(uint16_t remoteCommunicatorVersion, const ISyncTaskContext *context,
    AbilitySyncRequestPacket &packet) const
{
    if (storageInterface_ == nullptr) {
        LOGE("[AbilitySync][FillAbilityRequest] storageInterface is nullptr");
        return -E_INVALID_ARGS;
    }
    uint64_t dbCreateTime;
    int errCode = (static_cast<SyncGenericInterface *>(storageInterface_))->GetDatabaseCreateTimestamp(dbCreateTime);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][FillAbilityRequest] GetDatabaseCreateTimestamp failed, err %d", errCode);
        return errCode;
    }
    SecurityOption option;
    GetPacketSecOption(context, option);
    std::string schemaStr;
    uint32_t schemaType = 0;
    if (IsSingleKvVer()) {
        SchemaObject schemaObj = (static_cast<SingleVerKvDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
        schemaStr = schemaObj.ToSchemaString();
        schemaType = static_cast<uint32_t>(schemaObj.GetSchemaType());
    } else if (IsSingleRelationalVer()) {
        auto schemaObj = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
        schemaStr = schemaObj.ToSchemaString();
        schemaType = static_cast<uint32_t>(schemaObj.GetSchemaType());
    }
    DbAbility dbAbility;
    errCode = GetDbAbilityInfo(dbAbility);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][FillAbilityRequest] GetDbAbility failed, err %d", errCode);
        return errCode;
    }
    auto [err, localSchemaVer] = metadata_->GetLocalSchemaVersion();
    if (err != E_OK) {
        LOGE("[AbilitySync][FillAbilityRequest] GetLocalSchemaVersion failed, err %d", err);
        return err;
    }
    SetAbilityRequestBodyInfoInner(remoteCommunicatorVersion, packet, schemaStr, schemaType);
    packet.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    packet.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    packet.SetSecLabel(option.securityLabel);
    packet.SetSecFlag(option.securityFlag);
    packet.SetDbCreateTime(dbCreateTime);
    packet.SetDbAbility(dbAbility);
    packet.SetSchemaVersion(localSchemaVer);
    LOGI("[AbilitySync][FillRequest] ver=%u,Lab=%d,Flag=%d,dbCreateTime=%" PRId64 ",schemaVer=%" PRId64,
        SOFTWARE_VERSION_CURRENT, option.securityLabel, option.securityFlag, dbCreateTime, localSchemaVer);
    return E_OK;
}

int AbilitySync::SetAbilityAckBodyInfo(const ISyncTaskContext *context, int ackCode, bool isAckNotify,
    AbilitySyncAckPacket &ackPacket) const
{
    ackPacket.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    ackPacket.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    if (!isAckNotify) {
        SecurityOption option;
        GetPacketSecOption(context, option);
        ackPacket.SetSecLabel(option.securityLabel);
        ackPacket.SetSecFlag(option.securityFlag);
        uint64_t dbCreateTime = 0;
        int errCode =
            (static_cast<SyncGenericInterface *>(storageInterface_))->GetDatabaseCreateTimestamp(dbCreateTime);
        if (errCode != E_OK) {
            LOGE("[AbilitySync][SyncStart] GetDatabaseCreateTimestamp failed, err %d", errCode);
            ackCode = errCode;
        }
        DbAbility dbAbility;
        errCode = GetDbAbilityInfo(dbAbility);
        if (errCode != E_OK) {
            LOGE("[AbilitySync][FillAbilityRequest] GetDbAbility failed, err %d", errCode);
            return errCode;
        }
        ackPacket.SetDbCreateTime(dbCreateTime);
        ackPacket.SetDbAbility(dbAbility);
    }
    auto [ret, schemaVersion] = metadata_->GetLocalSchemaVersion();
    if (ret != E_OK) {
        return ret;
    }
    ackPacket.SetAckCode(ackCode);
    ackPacket.SetSchemaVersion(schemaVersion);
    return E_OK;
}

void AbilitySync::SetAbilityAckSchemaInfo(AbilitySyncAckPacket &ackPacket, const ISchema &schemaObj)
{
    ackPacket.SetSchema(schemaObj.ToSchemaString());
    ackPacket.SetSchemaType(static_cast<uint32_t>(schemaObj.GetSchemaType()));
}

void AbilitySync::SetAbilityAckSyncOpinionInfo(AbilitySyncAckPacket &ackPacket, SyncOpinion localOpinion)
{
    ackPacket.SetPermitSync(localOpinion.permitSync);
    ackPacket.SetRequirePeerConvert(localOpinion.requirePeerConvert);
}

int AbilitySync::GetDbAbilityInfo(DbAbility &dbAbility)
{
    int errCode = E_OK;
    for (const auto &item : SyncConfig::ABILITYBITS) {
        errCode = dbAbility.SetAbilityItem(item, SUPPORT_MARK);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return errCode;
}

int AbilitySync::AckMsgCheck(const Message *message, ISyncTaskContext *context) const
{
    if (message == nullptr || context == nullptr) {
        return -E_INVALID_ARGS;
    }
    if (message->GetErrorNo() == E_FEEDBACK_UNKNOWN_MESSAGE) {
        LOGE("[AbilitySync][AckMsgCheck] Remote device dose not support this message id");
        context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_EARLIEST);
        context->SetTaskErrCode(-E_FEEDBACK_UNKNOWN_MESSAGE);
        return -E_FEEDBACK_UNKNOWN_MESSAGE;
    }
    if (message->GetErrorNo() == E_FEEDBACK_COMMUNICATOR_NOT_FOUND) {
        LOGE("[AbilitySync][AckMsgCheck] Remote db is closed");
        context->SetTaskErrCode(-E_FEEDBACK_COMMUNICATOR_NOT_FOUND);
        return -E_FEEDBACK_COMMUNICATOR_NOT_FOUND;
    }
    const AbilitySyncAckPacket *packet = message->GetObject<AbilitySyncAckPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    int ackCode = packet->GetAckCode();
    if (ackCode != E_OK) {
        LOGE("[AbilitySync][AckMsgCheck] received an errCode %d", ackCode);
        context->SetTaskErrCode(ackCode);
        return ackCode;
    }
    return E_OK;
}

bool AbilitySync::IsSingleKvVer() const
{
    return storageInterface_->GetInterfaceType() == ISyncInterface::SYNC_SVD;
}
bool AbilitySync::IsSingleRelationalVer() const
{
#ifdef RELATIONAL_STORE
    return storageInterface_->GetInterfaceType() == ISyncInterface::SYNC_RELATION;
#else
    return false;
#endif
}

int AbilitySync::HandleRequestRecv(const Message *message, ISyncTaskContext *context, bool isCompatible)
{
    const AbilitySyncRequestPacket *packet = message->GetObject<AbilitySyncRequestPacket>();
    if (packet == nullptr) {
        return -E_INVALID_ARGS;
    }
    uint32_t remoteSoftwareVersion = packet->GetSoftwareVersion();
    int ackCode;
    std::string schema = packet->GetSchema();
    if (remoteSoftwareVersion <= SOFTWARE_VERSION_RELEASE_2_0) {
        LOGI("[AbilitySync][RequestRecv] remote version = %u, CheckSchemaCompatible = %d",
            remoteSoftwareVersion, isCompatible);
        return SendAckWithEmptySchema(context, message, E_OK, false);
    }
    HandleVersionV3RequestParam(packet, context);
    if (SecLabelCheck(packet)) {
        ackCode = E_OK;
    } else {
        ackCode = -E_SECURITY_OPTION_CHECK_ERROR;
    }
    if (ackCode == E_OK && remoteSoftwareVersion > SOFTWARE_VERSION_RELEASE_3_0) {
        ackCode = metadata_->SetDbCreateTime(deviceId_, context->GetTargetUserId(), packet->GetDbCreateTime(), true);
    }
    if (ackCode == E_OK && remoteSoftwareVersion >= SOFTWARE_VERSION_RELEASE_9_0) {
        ackCode = metadata_->SetRemoteSchemaVersion(context->GetDeviceId(), context->GetTargetUserId(),
            packet->GetSchemaVersion());
    }
    AbilitySyncAckPacket ackPacket;
    if (IsSingleRelationalVer()) {
        ackPacket.SetRelationalSyncOpinion(MakeRelationSyncOpinion(packet, schema));
    } else {
        SetAbilityAckSyncOpinionInfo(ackPacket, MakeKvSyncOpinion(packet, schema, context));
    }
    LOGI("[AbilitySync][RequestRecv] remote dev=%s,ver=%u,schemaCompatible=%d", STR_MASK(deviceId_),
        remoteSoftwareVersion, isCompatible);
    int errCode = SendAck(context, message, ackCode, false, ackPacket);
    return ackCode != E_OK ? ackCode : errCode;
}

int AbilitySync::SendAck(const ISyncTaskContext *context, const Message *message, int ackCode, bool isAckNotify,
    AbilitySyncAckPacket &ackPacket)
{
    int errCode = SetAbilityAckBodyInfo(context, ackCode, isAckNotify, ackPacket);
    if (errCode != E_OK) {
        return errCode;
    }
    if (IsSingleRelationalVer()) {
        auto schemaObj = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
        SetAbilityAckSchemaInfo(ackPacket, schemaObj);
    } else if (IsSingleKvVer()) {
        SchemaObject schemaObject = static_cast<SingleVerKvDBSyncInterface *>(storageInterface_)->GetSchemaInfo();
        SetAbilityAckSchemaInfo(ackPacket, schemaObject);
    }
    return SendAck(message, ackPacket, isAckNotify);
}

int AbilitySync::SendAckWithEmptySchema(const ISyncTaskContext *context, const Message *message, int ackCode,
    bool isAckNotify)
{
    AbilitySyncAckPacket ackPacket;
    int errCode = SetAbilityAckBodyInfo(context, ackCode, isAckNotify, ackPacket);
    if (errCode != E_OK) {
        return errCode;
    }
    SetAbilityAckSchemaInfo(ackPacket, SchemaObject());
    return SendAck(message, ackPacket, isAckNotify);
}

int AbilitySync::SendAck(const Message *inMsg, const AbilitySyncAckPacket &ackPacket, bool isAckNotify)
{
    Message *ackMessage = new (std::nothrow) Message(ABILITY_SYNC_MESSAGE);
    if (ackMessage == nullptr) {
        LOGE("[AbilitySync][SendAck] message create failed, may be memleak!");
        return -E_OUT_OF_MEMORY;
    }
    int errCode = ackMessage->SetCopiedObject<>(ackPacket);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][SendAck] SetCopiedObject failed, err %d", errCode);
        delete ackMessage;
        ackMessage = nullptr;
        return errCode;
    }
    (!isAckNotify) ? ackMessage->SetMessageType(TYPE_RESPONSE) : ackMessage->SetMessageType(TYPE_NOTIFY);
    ackMessage->SetTarget(deviceId_);
    ackMessage->SetSessionId(inMsg->GetSessionId());
    ackMessage->SetSequenceId(inMsg->GetSequenceId());
    SendConfig conf;
    SetSendConfigParam(storageInterface_->GetDbProperties(), deviceId_, false, SEND_TIME_OUT, conf);
    errCode = communicator_->SendMessage(deviceId_, ackMessage, conf);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][SendAck] SendPacket failed, err %d", errCode);
        delete ackMessage;
        ackMessage = nullptr;
    }
    return errCode;
}

SyncOpinion AbilitySync::MakeKvSyncOpinion(const AbilitySyncRequestPacket *packet,
    const std::string &remoteSchema, ISyncTaskContext *context)
{
    uint8_t remoteSchemaType = packet->GetSchemaType();
    SchemaObject localSchema = (static_cast<SingleVerKvDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
    SyncOpinion localSyncOpinion = SchemaNegotiate::MakeLocalSyncOpinion(localSchema, remoteSchema, remoteSchemaType);
    if (IsBothKvAndOptAbilitySync(context->GetRemoteSoftwareVersion(),
        localSchema.GetSchemaType(), remoteSchemaType)) { // LCOV_EXCL_BR_LINE
        // both kv no need convert
        SyncStrategy localStrategy;
        localStrategy.permitSync = true;
        (static_cast<SingleVerKvSyncTaskContext *>(context))->SetSyncStrategy(localStrategy, true);
        SetAbilitySyncFinishedStatus(true, *context);
    }
    return localSyncOpinion;
}

RelationalSyncOpinion AbilitySync::MakeRelationSyncOpinion(const AbilitySyncRequestPacket *packet,
    const std::string &remoteSchema) const
{
    uint8_t remoteSchemaType = packet->GetSchemaType();
    RelationalSchemaObject localSchema = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
    return SchemaNegotiate::MakeLocalSyncOpinion(localSchema, remoteSchema, remoteSchemaType,
        packet->GetSoftwareVersion());
}

int AbilitySync::HandleKvAckSchemaParam(const AbilitySyncAckPacket *recvPacket,
    ISyncTaskContext *context, AbilitySyncAckPacket &sendPacket, std::pair<bool, bool> &schemaSyncStatus)
{
    std::string remoteSchema = recvPacket->GetSchema();
    uint8_t remoteSchemaType = recvPacket->GetSchemaType();
    bool permitSync = static_cast<bool>(recvPacket->GetPermitSync());
    bool requirePeerConvert = static_cast<bool>(recvPacket->GetRequirePeerConvert());
    SyncOpinion remoteOpinion = {permitSync, requirePeerConvert, true};
    SchemaObject localSchema = (static_cast<SingleVerKvDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
    SyncOpinion syncOpinion = SchemaNegotiate::MakeLocalSyncOpinion(localSchema, remoteSchema, remoteSchemaType);
    SyncStrategy localStrategy = SchemaNegotiate::ConcludeSyncStrategy(syncOpinion, remoteOpinion);
    SetAbilityAckSyncOpinionInfo(sendPacket, syncOpinion);
    (static_cast<SingleVerKvSyncTaskContext *>(context))->SetSyncStrategy(localStrategy, true);
    schemaSyncStatus = {
        localStrategy.permitSync,
        true
    };
    if (localStrategy.permitSync) { // LCOV_EXCL_BR_LINE
        RecordAbilitySyncFinish(recvPacket->GetSchemaVersion(), *context);
    }
    if (IsBothKvAndOptAbilitySync(context->GetRemoteSoftwareVersion(),
        localSchema.GetSchemaType(), remoteSchemaType)) { // LCOV_EXCL_BR_LINE
        return -E_ABILITY_SYNC_FINISHED;
    }
    return E_OK;
}

int AbilitySync::HandleRelationAckSchemaParam(const AbilitySyncAckPacket *recvPacket, AbilitySyncAckPacket &sendPacket,
    ISyncTaskContext *context, bool sendOpinion, std::pair<bool, bool> &schemaSyncStatus)
{
    std::string remoteSchema = recvPacket->GetSchema();
    uint8_t remoteSchemaType = recvPacket->GetSchemaType();
    auto localSchema = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->GetSchemaInfo();
    auto localOpinion = SchemaNegotiate::MakeLocalSyncOpinion(localSchema, remoteSchema, remoteSchemaType,
        recvPacket->GetSoftwareVersion());
    auto localStrategy = SchemaNegotiate::ConcludeSyncStrategy(localOpinion,
        recvPacket->GetRelationalSyncOpinion());
    (static_cast<SingleVerRelationalSyncTaskContext *>(context))->SetRelationalSyncStrategy(localStrategy, true);
    bool permitSync = std::any_of(localStrategy.begin(), localStrategy.end(),
        [] (const std::pair<std::string, SyncStrategy> &it) {
        return it.second.permitSync;
        });
    if (permitSync) {
        int innerErrCode = (static_cast<RelationalDBSyncInterface *>(storageInterface_)->SaveRemoteDeviceSchema(
            deviceId_, remoteSchema, remoteSchemaType));
        if (innerErrCode != E_OK) {
            LOGE("[AbilitySync][AckRecv] save remote device Schema failed,errCode=%d", innerErrCode);
            return innerErrCode;
        }
    }
    int errCode = (static_cast<RelationalDBSyncInterface *>(storageInterface_))->
        CreateDistributedDeviceTable(context->GetDeviceId(), localStrategy);
    if (errCode != E_OK) {
        LOGE("[AbilitySync][AckRecv] create distributed device table failed,errCode=%d", errCode);
    }
    if (sendOpinion) {
        sendPacket.SetRelationalSyncOpinion(localOpinion);
    }
    auto singleVerContext = static_cast<SingleVerSyncTaskContext *>(context);
    auto strategy = localStrategy.find(singleVerContext->GetQuery().GetRelationTableName());
    schemaSyncStatus = {
        !(strategy == localStrategy.end()) && strategy->second.permitSync,
        true
    };
    if (permitSync) {
        RecordAbilitySyncFinish(recvPacket->GetSchemaVersion(), *context);
    }
    return errCode;
}

int AbilitySync::AckRecvWithHighVersion(const Message *message, ISyncTaskContext *context,
    const AbilitySyncAckPacket *packet)
{
    HandleVersionV3AckSecOptionParam(packet, context);
    AbilitySyncAckPacket ackPacket;
    std::pair<bool, bool> schemaSyncStatus;
    int errCode = E_OK;
    if (context->GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_3_0) {
        errCode = metadata_->SetDbCreateTime(deviceId_, context->GetTargetUserId(), packet->GetDbCreateTime(), true);
        if (errCode != E_OK) {
            LOGE("[AbilitySync][AckRecv] set db create time failed,errCode=%d", errCode);
            context->SetTaskErrCode(errCode);
            return errCode;
        }
    }
    errCode = HandleVersionV3AckSchemaParam(packet, ackPacket, context, true, schemaSyncStatus);
    DbAbility remoteDbAbility = packet->GetDbAbility();
    auto singleVerContext = static_cast<SingleVerSyncTaskContext *>(context);
    singleVerContext->SetDbAbility(remoteDbAbility);
    if (errCode == -E_ABILITY_SYNC_FINISHED) {
        return errCode;
    }
    if (errCode != E_OK) {
        context->SetTaskErrCode(errCode);
        return errCode;
    }
    if (!schemaSyncStatus.first) {
        singleVerContext->SetTaskErrCode(-E_SCHEMA_MISMATCH);
        LOGE("[AbilitySync][AckRecv] scheme check failed");
        return -E_SCHEMA_MISMATCH;
    }
    (void)SendAck(context, message, AbilitySync::CHECK_SUCCESS, true, ackPacket);
    return E_OK;
}

int32_t AbilitySync::TransformSecLabelIfNeed(int32_t originLabel, int targetLabel)
{
    if ((originLabel == S0 && targetLabel == S1) || (originLabel == S1 && targetLabel == S0)) {
        LOGI("[AbilitySync] Accept SecLabel From %d To %d", originLabel, targetLabel);
        return targetLabel;
    }
    return originLabel;
}

bool AbilitySync::IsBothKvAndOptAbilitySync(uint32_t remoteVersion, SchemaType localType, uint8_t remoteType)
{
    return remoteVersion >= SOFTWARE_VERSION_RELEASE_8_0 && localType == SchemaType::NONE &&
        static_cast<SchemaType>(remoteType) == SchemaType::NONE;
}

void AbilitySync::InitAbilitySyncFinishStatus(ISyncTaskContext &context)
{
    if (!metadata_->IsAbilitySyncFinish(context.GetDeviceId(), context.GetTargetUserId())) {
        return;
    }
    LOGI("[AbilitySync] Mark ability sync finish from db status");
    syncFinished_ = true;
    if (context.GetRemoteSoftwareVersion() == 0u) { // LCOV_EXCL_BR_LINE
        LOGI("[AbilitySync] Init remote version with default");
        context.SetRemoteSoftwareVersion(SOFTWARE_VERSION_RELEASE_9_0); // remote version >= 109
    }
    InitRemoteDBAbility(context);
}

void AbilitySync::InitRemoteDBAbility(ISyncTaskContext &context)
{
    DbAbility ability;
    int errCode = GetDbAbilityInfo(ability);
    if (errCode != E_OK) {
        return;
    }
    context.SetDbAbility(ability);
    auto version = static_cast<uint32_t>(metadata_->GetRemoteSoftwareVersion(context.GetDeviceId(),
        context.GetTargetUserId()));
    if (version > 0) {
        context.SetRemoteSoftwareVersion(version);
    }
}

void AbilitySync::RecordAbilitySyncFinish(uint64_t remoteSchemaVersion, ISyncTaskContext &context)
{
    SetAbilitySyncFinishedStatus(true, context);
    if (context.GetRemoteSoftwareVersion() >= SOFTWARE_VERSION_RELEASE_9_0) { // LCOV_EXCL_BR_LINE
        (void)metadata_->SetRemoteSchemaVersion(deviceId_, context.GetTargetUserId(), remoteSchemaVersion);
    }
    (void)metadata_->SetRemoteSoftwareVersion(deviceId_, context.GetTargetUserId(), context.GetRemoteSoftwareVersion());
}
} // namespace DistributedDB