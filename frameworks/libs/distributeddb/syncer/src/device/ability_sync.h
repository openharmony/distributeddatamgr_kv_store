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

#ifndef ABILITY_SYNC_H
#define ABILITY_SYNC_H

#include <cstdint>
#include <string>

#include "db_ability.h"
#include "icommunicator.h"
#include "ikvdb_sync_interface.h"
#include "isync_task_context.h"
#include "parcel.h"
#ifdef RELATIONAL_STORE
#include "ischema.h"
#endif
#include "schema_negotiate.h"

namespace DistributedDB {
class AbilitySyncRequestPacket {
public:
    AbilitySyncRequestPacket();
    ~AbilitySyncRequestPacket();

    void SetProtocolVersion(uint32_t protocolVersion);
    uint32_t GetProtocolVersion() const;

    void SetSendCode(int32_t sendCode);
    int32_t GetSendCode() const;

    void SetSoftwareVersion(uint32_t swVersion);
    uint32_t GetSoftwareVersion() const;

    void SetSchema(const std::string &schema);
    std::string GetSchema() const;

    void SetSchemaType(uint32_t schemaType);
    uint32_t GetSchemaType() const;

    void SetSecLabel(int32_t secLabel);
    int32_t GetSecLabel() const;

    void SetSecFlag(int32_t secFlag);
    int32_t GetSecFlag() const;

    void SetDbCreateTime(uint64_t dbCreateTime);
    uint64_t GetDbCreateTime() const;

    uint32_t CalculateLen() const;

    DbAbility GetDbAbility() const;

    void SetDbAbility(const DbAbility &dbAbility);

    void SetSchemaVersion(uint64_t schemaVersion);

    uint64_t GetSchemaVersion() const;

private:
    uint32_t protocolVersion_;
    int32_t sendCode_;
    uint32_t softwareVersion_;
    std::string schema_;
    int32_t secLabel_;
    int32_t secFlag_;
    uint32_t schemaType_;
    uint64_t dbCreateTime_;
    DbAbility dbAbility_;
    uint64_t schemaVersion_;
};

class AbilitySyncAckPacket {
public:
    AbilitySyncAckPacket();
    ~AbilitySyncAckPacket();

    void SetProtocolVersion(uint32_t protocolVersion);
    uint32_t GetProtocolVersion() const;

    void SetSoftwareVersion(uint32_t swVersion);
    uint32_t GetSoftwareVersion() const;

    void SetAckCode(int32_t ackCode);
    int32_t GetAckCode() const;

    void SetSchema(const std::string &schema);
    std::string GetSchema() const;

    void SetSchemaType(uint32_t schemaType);
    uint32_t GetSchemaType() const;

    void SetSecLabel(int32_t secLabel);
    int32_t GetSecLabel() const;

    void SetSecFlag(int32_t secFlag);
    int32_t GetSecFlag() const;

    void SetPermitSync(uint32_t permitSync);
    uint32_t GetPermitSync() const;

    void SetRequirePeerConvert(uint32_t requirePeerConvert);
    uint32_t GetRequirePeerConvert() const;

    void SetDbCreateTime(uint64_t dbCreateTime);
    uint64_t GetDbCreateTime() const;

    uint32_t CalculateLen() const;

    DbAbility GetDbAbility() const;

    void SetDbAbility(const DbAbility &dbAbility);

    void SetSchemaVersion(uint64_t schemaVersion);

    uint64_t GetSchemaVersion() const;

    void SetRelationalSyncOpinion(const RelationalSyncOpinion &relationalSyncOpinion);

    RelationalSyncOpinion GetRelationalSyncOpinion() const;

private:
    uint32_t protocolVersion_;
    uint32_t softwareVersion_;
    int32_t ackCode_;
    std::string schema_;
    int32_t secLabel_;
    int32_t secFlag_;
    uint32_t schemaType_;
    uint32_t permitSync_;
    uint32_t requirePeerConvert_;
    uint64_t dbCreateTime_;
    uint64_t schemaVersion_;
    DbAbility dbAbility_;
    RelationalSyncOpinion relationalSyncOpinion_;
};

class AbilitySync {
public:
    static const int CHECK_SUCCESS = 0;
    static const int LAST_NOTIFY = 0xfe;
    AbilitySync();
    ~AbilitySync();

    // Start Ability Sync
    int SyncStart(uint32_t sessionId, uint32_t sequenceId, uint16_t remoteCommunicatorVersion,
        const CommErrHandler &handler = nullptr, const ISyncTaskContext *context = nullptr);

    int Initialize(ICommunicator *inCommunicator, ISyncInterface *inStorage,
        const std::shared_ptr<Metadata> &inMetadata, const std::string &deviceId);

    int AckRecv(const Message *message, ISyncTaskContext *context);

    int RequestRecv(const Message *message, ISyncTaskContext *context);

    int AckNotifyRecv(const Message *message, ISyncTaskContext *context);

    bool GetAbilitySyncFinishedStatus() const;

    void SetAbilitySyncFinishedStatus(bool syncFinished, ISyncTaskContext &context);

    void InitAbilitySyncFinishStatus(ISyncTaskContext &context);

    static int RegisterTransformFunc();

    static uint32_t CalculateLen(const Message *inMsg);

    static int Serialization(uint8_t *buffer, uint32_t length, const Message *inMsg); // register to communicator

    static int DeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg); // register to communicator

private:
    static int RequestPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);

    static int AckPacketSerialization(uint8_t *buffer, uint32_t length, const Message *inMsg);

    static int RequestPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);

    static int AckPacketDeSerialization(const uint8_t *buffer, uint32_t length, Message *inMsg);

    static int RequestPacketCalculateLen(const Message *inMsg, uint32_t &len);

    static int AckPacketCalculateLen(const Message *inMsg, uint32_t &len);

    static int RequestPacketDeSerializationTailPart(Parcel &parcel, AbilitySyncRequestPacket *packet,
        uint32_t version);

    static int AckPacketDeSerializationTailPart(Parcel &parcel, AbilitySyncAckPacket *packet, uint32_t version);

    bool SecLabelCheck(const AbilitySyncRequestPacket *packet) const;

    static void HandleVersionV3RequestParam(const AbilitySyncRequestPacket *packet, ISyncTaskContext *context);

    static void HandleVersionV3AckSecOptionParam(const AbilitySyncAckPacket *packet,
        ISyncTaskContext *context);

    int HandleVersionV3AckSchemaParam(const AbilitySyncAckPacket *recvPacket,
        AbilitySyncAckPacket &sendPacket,  ISyncTaskContext *context, bool sendOpinion,
        std::pair<bool, bool> &schemaSyncStatus);

    int HandleKvAckSchemaParam(const AbilitySyncAckPacket *recvPacket,
        ISyncTaskContext *context, AbilitySyncAckPacket &sendPacket, std::pair<bool, bool> &schemaSyncStatus);

    int HandleRelationAckSchemaParam(const AbilitySyncAckPacket *recvPacket,
        AbilitySyncAckPacket &sendPacket, ISyncTaskContext *context, bool sendOpinion,
        std::pair<bool, bool> &schemaSyncStatus);

    void GetPacketSecOption(const ISyncTaskContext *context, SecurityOption &option) const;

    int SetAbilityRequestBodyInfo(uint16_t remoteCommunicatorVersion, const ISyncTaskContext *context,
        AbilitySyncRequestPacket &packet) const;

    int SetAbilityAckBodyInfo(const ISyncTaskContext *context, int ackCode, bool isAckNotify,
        AbilitySyncAckPacket &ackPacket) const;

    static void SetAbilityAckSchemaInfo(AbilitySyncAckPacket &ackPacket, const ISchema &schemaObj);

    static void SetAbilityAckSyncOpinionInfo(AbilitySyncAckPacket &ackPacket, SyncOpinion localOpinion);

    static int GetDbAbilityInfo(DbAbility &dbAbility);

    int AckMsgCheck(const Message *message, ISyncTaskContext *context) const;

    bool IsSingleKvVer() const;

    bool IsSingleRelationalVer() const;

    int SendAck(const ISyncTaskContext *context, const Message *message, int ackCode, bool isAckNotify,
        AbilitySyncAckPacket &ackPacket);

    int SendAckWithEmptySchema(const ISyncTaskContext *context, const Message *message, int ackCode, bool isAckNotify);

    int SendAck(const Message *inMsg, const AbilitySyncAckPacket &ackPacket, bool isAckNotify);

    int HandleRequestRecv(const Message *message, ISyncTaskContext *context, bool isCompatible);

    SyncOpinion MakeKvSyncOpinion(const AbilitySyncRequestPacket *packet, const std::string &remoteSchema,
        ISyncTaskContext *context);

    RelationalSyncOpinion MakeRelationSyncOpinion(const AbilitySyncRequestPacket *packet,
        const std::string &remoteSchema) const;

    int AckRecvWithHighVersion(const Message *message, ISyncTaskContext *context, const AbilitySyncAckPacket *packet);

    void InitRemoteDBAbility(ISyncTaskContext &context);

    void RecordAbilitySyncFinish(uint64_t remoteSchemaVersion, ISyncTaskContext &context);

    static int32_t TransformSecLabelIfNeed(int32_t originLabel, int targetLabel);

    static bool IsBothKvAndOptAbilitySync(uint32_t remoteVersion, SchemaType localType, uint8_t remoteType);

    ICommunicator *communicator_;
    ISyncInterface *storageInterface_;
    std::shared_ptr<Metadata> metadata_;
    std::string deviceId_;
    bool syncFinished_;
    static const int FAILED_GET_SEC_CLASSIFICATION = 0x55;
};
} // namespace DistributedDB
#endif // ABILITY_SYNC_H
