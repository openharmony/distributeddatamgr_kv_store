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
#include "ability_manager_client.h"
#include "launch_param.h"
#include "ability_connect_callback_stub.h"
#include "dataobs_mgr_client.h"
#include "data_ability_observer_stub.h"
#include "extension_manager_proxy.h"
#include "last_exit_detail_info.h"
#include "lifecycle_state_info.h"
#include "caller_info.h"
namespace OHOS::AAFwk {
bool LaunchParam::ReadFromParcel(Parcel &parcel) { return false; }
bool LaunchParam::Marshalling(Parcel &parcel) const { return false; }
LaunchParam *LaunchParam::Unmarshalling(Parcel &parcel) { return nullptr; }

bool LastExitDetailInfo::ReadFromParcel(Parcel &parcel) { return false; }
bool LastExitDetailInfo::Marshalling(Parcel &parcel) const { return false; }
LastExitDetailInfo *LastExitDetailInfo::Unmarshalling(Parcel &parcel) { return nullptr; }

bool LifeCycleStateInfo::ReadFromParcel(Parcel &parcel) { return false; }
bool LifeCycleStateInfo::Marshalling(Parcel &parcel) const { return false; }
LifeCycleStateInfo *LifeCycleStateInfo::Unmarshalling(Parcel &parcel) { return nullptr; }

bool CallerInfo::ReadFromParcel(Parcel &parcel) { return false; }
bool CallerInfo::Marshalling(Parcel &parcel) const { return false; }
CallerInfo *CallerInfo::Unmarshalling(Parcel &parcel) { return nullptr; }

bool IndirectCallerInfo::ReadFromParcel(Parcel &parcel) { return false; }
bool IndirectCallerInfo::Marshalling(Parcel &parcel) const { return false; }
IndirectCallerInfo *IndirectCallerInfo::Unmarshalling(Parcel &parcel) { return nullptr; }
AbilityManagerClient::AbilityManagerClient() {}
AbilityManagerClient::~AbilityManagerClient() {}
std::shared_ptr<AbilityManagerClient> AbilityManagerClient::GetInstance()
{
    static std::shared_ptr<AbilityManagerClient> instance = std::make_shared<AbilityManagerClient>();
    return instance;
}
ErrCode AbilityManagerClient::AttachAbilityThread(sptr<IAbilityScheduler> scheduler, sptr<IRemoteObject> token)
{ return 0; }
ErrCode AbilityManagerClient::AbilityTransitionDone(sptr<IRemoteObject> token, int state, const PacMap &saveData)
{
    return 0;
}
ErrCode AbilityManagerClient::ScheduleConnectAbilityDone(sptr<IRemoteObject> token,
                                                         sptr<IRemoteObject> remoteObject) { return 0; }
ErrCode AbilityManagerClient::ScheduleDisconnectAbilityDone(sptr<IRemoteObject> token) { return 0; }
ErrCode AbilityManagerClient::ScheduleCommandAbilityDone(sptr<IRemoteObject> token) { return 0; }
ErrCode AbilityManagerClient::StartAbility(const Want &want, int requestCode, int32_t userId,
                                           uint64_t specifiedFullTokenId) { return 0; }
ErrCode AbilityManagerClient::StartAbility(const Want &want, sptr<IRemoteObject> callerToken, int requestCode,
                                           int32_t userId, uint64_t specifiedFullTokenId) { return 0; }
ErrCode AbilityManagerClient::StartAbility(const Want &want, const AbilityStartSetting &abilityStartSetting,
                                           sptr<IRemoteObject> callerToken, int requestCode, int32_t userId)
{
    return 0;
}
ErrCode AbilityManagerClient::StartAbility(const Want &want, const StartOptions &startOptions,
                                           sptr<IRemoteObject> callerToken, int requestCode, int32_t userId)
{
    return 0;
}
ErrCode AbilityManagerClient::StartExtensionAbility(const Want &want, sptr<IRemoteObject> callerToken,
                                                    int32_t userId, AppExecFwk::ExtensionAbilityType extensionType)
{
    return 0;
}
ErrCode AbilityManagerClient::StopExtensionAbility(const Want &want, sptr<IRemoteObject> callerToken,
                                                   int32_t userId, AppExecFwk::ExtensionAbilityType extensionType)
{
    return 0;
}
ErrCode AbilityManagerClient::TerminateAbility(sptr<IRemoteObject> token, int resultCode, const Want *resultWant)
{
    return 0;
}
ErrCode AbilityManagerClient::SendResultToAbility(int requestCode, int resultCode, Want &resultWant) { return 0; }
ErrCode AbilityManagerClient::CloseAbility(sptr<IRemoteObject> token, int resultCode, const Want *resultWant)
{
    return 0;
}
ErrCode AbilityManagerClient::MinimizeAbility(sptr<IRemoteObject> token, bool fromUser) { return 0; }
ErrCode AbilityManagerClient::ConnectAbility(const Want &want, sptr<IAbilityConnection> connect, int32_t userId)
{
    return 0;
}
ErrCode AbilityManagerClient::ConnectAbility(const Want &want, sptr<IAbilityConnection> connect, int32_t userId,
                                             int32_t loadTimeout) { return 0; }
ErrCode AbilityManagerClient::ConnectAbility(const Want &want, sptr<IAbilityConnection> connect,
                                             sptr<IRemoteObject> callerToken, int32_t userId,
                                             uint64_t specifiedFullTokenId) { return 0; }
ErrCode AbilityManagerClient::DisconnectAbility(sptr<IAbilityConnection> connect) { return 0; }
sptr<IAbilityScheduler> AbilityManagerClient::AcquireDataAbility(const Uri &uri, bool tryBind,
                                                                 sptr<IRemoteObject> callerToken)
{
    return sptr<IAbilityScheduler>();
}
ErrCode AbilityManagerClient::ReleaseDataAbility(sptr<IAbilityScheduler> dataAbilityScheduler,
                                                 sptr<IRemoteObject> callerToken) { return 0; }
ErrCode AbilityManagerClient::DumpState(const std::string &args, std::vector<std::string> &state) { return 0; }
ErrCode AbilityManagerClient::DumpSysState(const std::string &args, std::vector<std::string> &state, bool isClient,
                                            bool isUserID, int UserID) { return 0; }
ErrCode AbilityManagerClient::Connect() { return 0; }
ErrCode AbilityManagerClient::StopServiceAbility(const Want &want, sptr<IRemoteObject> token) { return 0; }
ErrCode AbilityManagerClient::KillProcess(const std::string &bundleName, bool clearPageStack, int32_t appIndex,
                                           const std::string &reason) { return 0; }
ErrCode AbilityManagerClient::ContinueMission(const std::string &srcDeviceId, const std::string &dstDeviceId,
                                              int32_t missionId, sptr<IRemoteObject> callback,
                                              WantParams &wantParams) { return 0; }
ErrCode AbilityManagerClient::StartContinuation(const Want &want, sptr<IRemoteObject> abilityToken,
                                                int32_t status) { return 0; }
void AbilityManagerClient::NotifyCompleteContinuation(const std::string &deviceId, int32_t sessionId, bool isSuccess)
{
}
ErrCode AbilityManagerClient::ContinueAbility(const std::string &deviceId, int32_t missionId, uint32_t versionCode)
{
    return 0;
}
ErrCode AbilityManagerClient::NotifyContinuationResult(int32_t missionId, int32_t result) { return 0; }
ErrCode AbilityManagerClient::LockMissionForCleanup(int32_t missionId) { return 0; }
ErrCode AbilityManagerClient::UnlockMissionForCleanup(int32_t missionId) { return 0; }
ErrCode AbilityManagerClient::RegisterMissionListener(sptr<IMissionListener> listener) { return 0; }
ErrCode AbilityManagerClient::UnRegisterMissionListener(sptr<IMissionListener> listener) { return 0; }
ErrCode AbilityManagerClient::RegisterMissionListener(const std::string &deviceId,
                                                      sptr<IRemoteMissionListener> listener) { return 0; }
ErrCode AbilityManagerClient::UnRegisterMissionListener(const std::string &deviceId,
                                                        sptr<IRemoteMissionListener> listener) { return 0; }
ErrCode AbilityManagerClient::GetMissionInfos(const std::string &deviceId, int32_t numMax,
                                              std::vector<MissionInfo> &missionInfos) { return 0; }
ErrCode AbilityManagerClient::GetMissionInfo(const std::string &deviceId, int32_t missionId, MissionInfo &missionInfo)
{
    return 0;
}
ErrCode AbilityManagerClient::GetMissionSnapshot(const std::string &deviceId, int32_t missionId,
                                                 MissionSnapshot &snapshot, bool isLowResolution) { return 0; }
ErrCode AbilityManagerClient::CleanMission(int32_t missionId) { return 0; }
ErrCode AbilityManagerClient::CleanAllMissions() { return 0; }
ErrCode AbilityManagerClient::MoveMissionToFront(int32_t missionId) { return 0; }
ErrCode AbilityManagerClient::MoveMissionToFront(int32_t missionId, const StartOptions &startOptions) { return 0; }
ErrCode AbilityManagerClient::GetMissionIdByToken(sptr<IRemoteObject> token, int32_t &missionId) { return 0; }
ErrCode AbilityManagerClient::StartAbilityByCall(const Want &want, sptr<IAbilityConnection> connect,
                                                 bool isSilent, bool isVisible) { return 0; }
ErrCode AbilityManagerClient::StartAbilityByCall(const Want &want, sptr<IAbilityConnection> connect,
    sptr<IRemoteObject> callToken, int32_t accountId, bool isSilent, bool promotePriority, bool isVisible)
{ return 0; }
ErrCode AbilityManagerClient::GetAbilityRunningInfos(std::vector<AbilityRunningInfo> &info) { return 0; }
ErrCode AbilityManagerClient::GetExtensionRunningInfos(int upperLimit, std::vector<ExtensionRunningInfo> &info)
{
    return 0;
}
ErrCode AbilityManagerClient::GetProcessRunningInfos(std::vector<AppExecFwk::RunningProcessInfo> &info) { return 0; }
ErrCode AbilityManagerClient::StartSyncRemoteMissions(const std::string &devId, bool fixConflict,
                                                      int64_t tag) { return 0; }
ErrCode AbilityManagerClient::StopSyncRemoteMissions(const std::string &devId) { return 0; }
ErrCode AbilityManagerClient::StopUser(int accountId, sptr<IUserCallback> callback) { return 0; }
ErrCode AbilityManagerClient::RegisterSnapshotHandler(sptr<ISnapshotHandler> handler) { return 0; }
ErrCode AbilityManagerClient::StartUserTest(const Want &want, sptr<IRemoteObject> observer) { return 0; }
ErrCode AbilityManagerClient::FinishUserTest(const std::string &msg, const int64_t &resultCode,
                                             const std::string &bundleName) { return 0; }
ErrCode AbilityManagerClient::GetTopAbility(sptr<IRemoteObject> &token) { return 0; }
ErrCode AbilityManagerClient::DelegatorDoAbilityForeground(sptr<IRemoteObject> token) { return 0; }
ErrCode AbilityManagerClient::DelegatorDoAbilityBackground(sptr<IRemoteObject> token) { return 0; }
ErrCode AbilityManagerClient::DoAbilityForeground(sptr<IRemoteObject> token, uint32_t flag) { return 0; }
ErrCode AbilityManagerClient::DoAbilityBackground(sptr<IRemoteObject> token, uint32_t flag) { return 0; }
int AbilityManagerClient::SetAbilityController(sptr<AppExecFwk::IAbilityController> abilityController,
                                               bool imAStabilityTest) { return 0; }
ErrCode AbilityManagerClient::FreeInstallAbilityFromRemote(const Want &want, sptr<IRemoteObject> callback,
                                                           int32_t userId, int requestCode) { return 0; }
ErrCode AbilityManagerClient::DumpAbilityInfoDone(std::vector<std::string> &infos,
                                                  sptr<IRemoteObject> callerToken) { return 0; }
sptr<IAbilityManager> AbilityManagerClient::GetAbilityManager() { return sptr<IAbilityManager>(); }
void AbilityManagerClient::ResetProxy(wptr<IRemoteObject> remote) {}
void AbilityManagerClient::HandleDlpApp(Want &want) {}
AbilityConnectionStub::AbilityConnectionStub() {}
AbilityConnectionStub::~AbilityConnectionStub() {}
int AbilityConnectionStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                           MessageOption &option)
{
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
DataObsMgrClient::DataObsMgrClient() {}
DataObsMgrClient::~DataObsMgrClient() {}
std::shared_ptr<DataObsMgrClient> DataObsMgrClient::GetInstance()
{
    static std::shared_ptr<DataObsMgrClient> instance = std::make_shared<DataObsMgrClient>();
    return instance;
}
ErrCode DataObsMgrClient::RegisterObserver(const Uri &uri, sptr<IDataAbilityObserver> dataObserver, int32_t userId,
    DataObsOption opt)
{
    return 0;
}
ErrCode DataObsMgrClient::UnregisterObserver(const Uri &uri, sptr<IDataAbilityObserver> dataObserver, int32_t userId,
    DataObsOption opt)
{
    return 0;
}
ErrCode DataObsMgrClient::NotifyChange(const Uri &uri, int32_t userId, DataObsOption opt)
{
    return 0;
}
Status DataObsMgrClient::RegisterObserverExt(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
    bool isDescendants, DataObsOption opt)
{
    return IPC_ERROR;
}
Status DataObsMgrClient::UnregisterObserverExt(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
    DataObsOption opt)
{
    return IPC_ERROR;
}
Status DataObsMgrClient::UnregisterObserverExt(sptr<IDataAbilityObserver> dataObserver, DataObsOption opt)
{
    return IPC_ERROR;
}
Status DataObsMgrClient::NotifyChangeExt(const ChangeInfo &changeInfo, DataObsOption opt)
{
    return IPC_ERROR;
}
Status DataObsMgrClient::NotifyProcessObserver(const std::string &key, const sptr<IRemoteObject> &observer,
    DataObsOption opt)
{
    return IPC_ERROR;
}
ErrCode DataObsMgrClient::RegisterObserverFromExtension(const Uri &uri, sptr<IDataAbilityObserver> dataObserver,
    int32_t userId, DataObsOption opt)
{
    return 0;
}
ErrCode DataObsMgrClient::NotifyChangeFromExtension(const Uri &uri, int32_t userId, DataObsOption opt)
{
    return 0;
}

int ExtensionManagerProxy::ConnectAbilityCommon(const Want &want, sptr<IRemoteObject> connect,
    const sptr<IRemoteObject> &callerToken, AppExecFwk::ExtensionAbilityType extensionType, int32_t userId,
    bool isQueryExtensionOnly)
{
    return 0;
}
int ExtensionManagerProxy::DisconnectAbility(const sptr<IRemoteObject> &connect)
{
    return 0;
}
int ExtensionManagerProxy::GetExtensionRunningInfos(int upperLimit, std::vector<ExtensionRunningInfo> &info)
{
    return 0;
}
int32_t ExtensionManagerProxy::TransferAbilityResultForExtension(const sptr<IRemoteObject> &callerToken,
    int32_t resultCode, const Want &want)
{
    return 0;
}
bool ExtensionManagerProxy::WriteInterfaceToken(MessageParcel &data)
{
    return false;
}
ErrCode ExtensionManagerProxy::SendRequest(AbilityManagerInterfaceCode code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    return 0;
}
}

