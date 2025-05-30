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
#ifndef SINGLE_VER_DATA_SYNC_UTIL_H
#define SINGLE_VER_DATA_SYNC_UTIL_H

#include "message.h"
#include "single_ver_data_packet.h"
#include "single_ver_data_sync.h"
#include "single_ver_sync_task_context.h"
namespace DistributedDB {
class SingleVerDataSyncUtils {
public:
    static int QuerySyncCheck(const SingleVerSyncTaskContext *context, bool &isCheckStatus);

    static int AckMsgErrnoCheck(const SingleVerSyncTaskContext *context, const Message *message);

    static int RequestQueryCheck(const DataRequestPacket *packet, SyncGenericInterface *storage);

    static bool IsPermitLocalDeviceRecvData(const std::string &deviceId, const SecurityOption &remoteSecOption);

    static bool IsPermitRemoteDeviceRecvData(const std::string &deviceId, const SecurityOption &remoteSecOption,
        SyncGenericInterface *storage);

    static void TransDbDataItemToSendDataItem(const std::string &localHashName,
        std::vector<SendDataItem> &outData);

    static std::string TransferForeignOrigDevName(const std::string &deviceName, const std::string &localHashName);

    static void TransSendDataItemToLocal(const SingleVerSyncTaskContext *context,
        const std::string &localHashName, const std::vector<SendDataItem> &data);

    static void TranslateErrCodeIfNeed(int mode, uint32_t version, int &errCode);

    static int RunPermissionCheck(SingleVerSyncTaskContext *context, const SyncGenericInterface* storage,
        const std::string &label, const DataRequestPacket *packet);

    static int RunPermissionCheck(SingleVerSyncTaskContext *context, const SyncGenericInterface* storage,
        const std::string &label, int mode);

    static bool CheckPermitReceiveData(const SingleVerSyncTaskContext *context, const ICommunicator *communicator,
        const SyncGenericInterface *storage);

    static void SetPacketId(DataRequestPacket *packet, SingleVerSyncTaskContext *context, uint32_t version);

    static int GetMessageId(SyncType syncType);

    static void PushAndPullKeyRevokHandle(SingleVerSyncTaskContext *context);

    static int GetReSendMode(int mode, uint32_t sequenceId, SyncType syncType);

    static void FillControlRequestPacket(ControlRequestPacket *packet, SingleVerSyncTaskContext *context);

    static ControlCmdType GetControlCmdType(int mode);

    static int GetModeByControlCmdType(ControlCmdType controlCmd);

    static bool IsNeedTriggerQueryAutoSync(Message *inMsg, QuerySyncObject &query);

    static void ControlAckErrorHandle(const SingleVerSyncTaskContext *context,
        const std::shared_ptr<SubscribeManager> &subManager);

    static void SetMessageHeadInfo(Message &message, uint16_t inMsgType,
        const std::string &inTarget, uint32_t inSequenceId, uint32_t inSessionId);

    static bool IsGetDataSuccessfully(int errCode);

    static Timestamp GetMaxSendDataTime(const std::vector<SendDataItem> &inData);

    static SyncTimeRange GetFullSyncDataTimeRange(const std::vector<SendDataItem> &inData, WaterMark localMark,
        UpdateWaterMark &isUpdate);

    static SyncTimeRange GetQuerySyncDataTimeRange(const std::vector<SendDataItem> &inData, WaterMark localMark,
        WaterMark deleteLocalMark, UpdateWaterMark &isUpdate);

    static SyncTimeRange ReviseLocalMark(SyncType syncType, const SyncTimeRange &dataTimeRange,
        UpdateWaterMark updateMark);

    static SyncTimeRange GetRecvDataTimeRange(SyncType syncType,
        const std::vector<SendDataItem> &data, UpdateWaterMark &isUpdate);

    static SyncTimeRange GetSyncDataTimeRange(SyncType syncType, WaterMark localMark, WaterMark deleteMark,
        const std::vector<SendDataItem> &inData, UpdateWaterMark &isUpdate);

    static std::pair<TimeOffset, TimeOffset> GetTimeOffsetFromRequestMsg(const Message *message);

    static void RecordClientId(const SingleVerSyncTaskContext &context, const SyncGenericInterface &storage,
        std::shared_ptr<Metadata> &metadata);;

    static void SetDataRequestCommonInfo(const SingleVerSyncTaskContext &context,
        const SyncGenericInterface &storage, DataRequestPacket &packet, std::shared_ptr<Metadata> &metadata);

    static int SchemaVersionMatchCheck(const SingleVerSyncTaskContext &context, const DataRequestPacket &packet,
        std::shared_ptr<Metadata> &metadata);

    static int GetUnsyncTotal(const SingleVerSyncTaskContext *context, const SyncGenericInterface *storage,
        uint32_t &total);
    static int GetUnsyncTotal(const SingleVerSyncTaskContext *context, const SyncGenericInterface *storage,
        SyncTimeRange &waterMarkInfo, uint32_t &total);

    static bool IsSupportRequestTotal(uint32_t version);

    static void UpdateSyncProcess(SingleVerSyncTaskContext *context, const DataRequestPacket *packet);

    static void CacheInitWaterMark(SingleVerSyncTaskContext *context, SingleVerDataSync *dataSync);

    static QuerySyncObject GetQueryFromDataRequest(const DataRequestPacket &packet,
        const SingleVerSyncTaskContext &context, uint32_t sessionId);
private:
    static int RunPermissionCheckInner(const SingleVerSyncTaskContext *context, const SyncGenericInterface* storage,
        const std::string &label, const DataRequestPacket *packet, int mode);
};
}
#endif // SINGLE_VER_DATA_SYNC_UTIL_H