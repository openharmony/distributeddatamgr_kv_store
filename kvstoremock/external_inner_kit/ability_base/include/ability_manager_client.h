/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_CLIENT_H
#define OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_CLIENT_H

#include <mutex>

//#include "ability_connect_callback_interface.h"
//#include "ability_manager_errors.h"
#include "ability_manager_interface.h"
#include "ability_scheduler_interface.h"
#include "auto_startup_info.h"
#include "caller_info.h"
#include "iremote_object.h"
#include "iprepare_terminate_callback_interface.h"
#include "mission_info.h"
//#include "system_memory_attr.h"
#include "ui_extension_window_command.h"
#include "want.h"
//#include "intent_exemption_info.h"
//#include "ihidden_start_observer.h"

namespace OHOS {
namespace AAFwk {
class Snapshot;
class ISnapshotHandler;
using AutoStartupInfo = AbilityRuntime::AutoStartupInfo;
/**
 * @class AbilityManagerClient
 * AbilityManagerClient is used to access ability manager services.
 */
class AbilityManagerClient {
public:
    virtual ~AbilityManagerClient();
    static std::shared_ptr<AbilityManagerClient> GetInstance();
    void RemoveDeathRecipient();

    /**
     * StartSelfUIAbility with want, start self uiability only on 2-in-1 devices.
     *
     * @param want, the want of the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartSelfUIAbility(const Want &want);

    /**
     * StartSelfUIAbility from ApplicationContext and force launch in current process.
     *
     * @param want, the want of the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartSelfUIAbilityByAppContext(const Want &want);

    /**
     * StartSelfUIAbility with want and startOptions, start self uiability only on 2-in-1 devices.
     *
     * @param want, the want of the ability to start.
     * @param options, the startOptions of the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartSelfUIAbilityWithStartOptions(const Want &want, const StartOptions &options);

    /**
     * Starts self UIAbility with start options and receives the process ID. Supported only on 2-in-1 devices.
     *
     * @param want, the want of the ability to start.
     * @param options, the startOptions of the ability to start.
     * @param callbackId, the id of the callback to get target process id.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartSelfUIAbilityWithPidResult(const Want &want, StartOptions &options, uint64_t callbackId);

    ErrCode StartSelfUIAbilityWithToken(const Want &want, sptr<IRemoteObject> callerToken);

    ErrCode StartSelfUIAbilityWithStartOptionsAndToken(const Want &want,
        const StartOptions &options, sptr<IRemoteObject> callerToken);

    /**
     * AttachAbilityThread, ability call this interface after loaded.
     *
     * @param scheduler,.the interface handler of kit ability.
     * @param token,.ability's token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode AttachAbilityThread(sptr<IAbilityScheduler> scheduler, sptr<IRemoteObject> token);

    /**
     * AbilityTransitionDone, ability call this interface after lift cycle was changed.
     *
     * @param token,.ability's token.
     * @param state,.the state of ability lift cycle.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode AbilityTransitionDone(sptr<IRemoteObject> token, int state, const PacMap &saveData);

    /**
     * AbilityWindowConfigTransitionDone, ability call this interface after lift cycle was changed.
     *
     * @param token,.ability's token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode AbilityWindowConfigTransitionDone(sptr<IRemoteObject> token, const WindowConfig &windowConfig);

    /**
     * ScheduleConnectAbilityDone, service ability call this interface while session was connected.
     *
     * @param token,.service ability's token.
     * @param remoteObject,.the session proxy of service ability.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ScheduleConnectAbilityDone(sptr<IRemoteObject> token, sptr<IRemoteObject> remoteObject);

    /**
     * ScheduleDisconnectAbilityDone, service ability call this interface while session was disconnected.
     *
     * @param token,.service ability's token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ScheduleDisconnectAbilityDone(sptr<IRemoteObject> token);

    /**
     * ScheduleCommandAbilityDone, service ability call this interface while session was commanded.
     *
     * @param token,.service ability's token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ScheduleCommandAbilityDone(sptr<IRemoteObject> token);

    ErrCode ScheduleCommandAbilityWindowDone(
        sptr<IRemoteObject> token,
        sptr<SessionInfo> sessionInfo,
        WindowCommand winCmd,
        AbilityCommand abilityCmd);

    /**
     * Get top ability.
     *
     * @param isNeedLocalDeviceId is need local device id.
     * @return Returns front desk focus ability elementName.
     */
    AppExecFwk::ElementName GetTopAbility(bool isNeedLocalDeviceId = true);

    /**
     * Get element name by token.
     *
     * @param token ability's token.
     * @param isNeedLocalDeviceId is need local device id.
     * @return Returns front desk focus ability elementName by token.
     */
    AppExecFwk::ElementName GetElementNameByToken(sptr<IRemoteObject> token, bool isNeedLocalDeviceId = true);

    /**
     * StartAbility with want, send want to ability manager service.
     *
     * @param want Ability want.
     * @param requestCode Ability request code.
     * @param specifiedFullTokenId, The specified full token ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbility(const Want &want, int requestCode = DEFAULT_INVAL_VALUE,
        int32_t userId = DEFAULT_INVAL_VALUE, uint64_t specifiedFullTokenId = 0);

    /**
     * StartAbilityWithWait, send want and abilityStartWithWaitObserver to abms.
     *
     * @param want Ability want.
     * @param observer ability foreground notify observer for aa tool.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityWithWait(Want &want, sptr<IAbilityStartWithWaitObserver> observer);

    /**
     * StartAbility with want, send want to ability manager service.
     *
     * @param want Ability want.
     * @param callerToken caller ability token.
     * @param requestCode Ability request code.
     * @param specifiedFullTokenId, The specified full token ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbility(
        const Want &want,
        sptr<IRemoteObject> callerToken,
        int requestCode = DEFAULT_INVAL_VALUE,
        int32_t userId = DEFAULT_INVAL_VALUE,
        uint64_t specifiedFullTokenId = 0);

    /**
     * StartAbility by insight intent, send want to ability manager service.
     *
     * @param want Ability want.
     * @param callerToken caller ability token.
     * @param intentId insight intent id.
     * @param userId userId of target ability.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityByInsightIntent(
        const Want &want,
        sptr<IRemoteObject> callerToken,
        uint64_t intentId,
        int32_t userId = DEFAULT_INVAL_VALUE);

     /**
      * Starts a new ability by oe extension.
      *
      * @param want Indicates the ability to start.
      * @param callerToken Indicates the caller ability token.
      * @param hostPid Indicates the host process ID.
      * @param specifiedFlag Indicates the specified flag for the target UIAbility for specified mode.
      * @return Returns ERR_OK on success, others on failure.
      */
    ErrCode StartAbilityByOEExt(
        const Want &want,
        sptr<IRemoteObject> callerToken,
        int32_t hostPid,
        const std::string &specifiedFlag);

    /**
     * Starts a new ability with specific start settings.
     *
     * @param want Indicates the ability to start.
     * @param requestCode the resultCode of the ability to start.
     * @param abilityStartSetting Indicates the setting ability used to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbility(
        const Want &want,
        const AbilityStartSetting &abilityStartSetting,
        sptr<IRemoteObject> callerToken,
        int requestCode = DEFAULT_INVAL_VALUE,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Starts a new ability with specific start options.
     *
     * @param want, the want of the ability to start.
     * @param startOptions Indicates the options used to start.
     * @param callerToken caller ability token.
     * @param requestCode the resultCode of the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbility(
        const Want &want,
        const StartOptions &startOptions,
        sptr<IRemoteObject> callerToken,
        int requestCode = DEFAULT_INVAL_VALUE,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Starts a new ability using the original caller information.
     *
     * @param want Ability want.
     * @param callerToken current caller ability token.
     * @param asCallerSourceToken source caller ability token.
     * @param requestCode Ability request code.
     * @param userId Ability userId
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityAsCaller(
            const Want &want,
            sptr<IRemoteObject> callerToken,
            sptr<IRemoteObject> asCallerSourceToken,
            int requestCode = DEFAULT_INVAL_VALUE,
            int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Starts a new ability using the original caller information.
     *
     * @param want Indicates the ability to start.
     * @param startOptions current Indicates the options used to start.
     * @param callerToken caller ability token.
     * @param asCallerSourceToken source caller ability token.
     * @param requestCode the resultCode of the ability to start.
     * @param userId Ability userId
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityAsCaller(
            const Want &want,
            const StartOptions &startOptions,
            sptr<IRemoteObject> callerToken,
            sptr<IRemoteObject> asCallerSourceToken,
            int requestCode = DEFAULT_INVAL_VALUE,
            int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Starts a new ability for result using the original caller information.
     *
     * @param want the want of the ability to start.
     * @param callerToken current caller ability token.
     * @param requestCode the resultCode of the ability to start.
     * @param userId Designation User ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityForResultAsCaller(
        const Want &want,
        sptr<IRemoteObject> callerToken,
        int requestCode = DEFAULT_INVAL_VALUE,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Starts a new ability for result using the original caller information.
     *
     * @param want the want of the ability to start.
     * @param startOptions Indicates the options used to start.
     * @param callerToken current caller ability token.
     * @param requestCode the resultCode of the ability to start.
     * @param userId Designation User ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityForResultAsCaller(
        const Want &want,
        const StartOptions &startOptions,
        sptr<IRemoteObject> callerToken,
        int requestCode = DEFAULT_INVAL_VALUE,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Start ui session ability with windowId and want, send windowId and want to ability manager service.
     *
     * @param primaryWindowId the id of window.
     * @param secondaryWant the want of the ability to start.
     * @param callerToken current caller ability token.
     * @return Returns ERR_OK if success.
     */
    ErrCode StartUIAbilitiesInSplitWindowMode(
        int32_t primaryWindowId,
        const AAFwk::Want &secondaryWant,
        sptr<IRemoteObject> callerToken);

    /**
     * Start UI abilities simultaneously.
     *
     * @param wantList a list of want to start UI abilities.
     * @param requestKey, The unique key of this StartUIAbilities request.
     * @param callerToken current caller ability token.
     * @return Returns ERR_OK if success.
     */
    ErrCode StartUIAbilities(const std::vector<AAFwk::Want> &wantList,
        const std::string &requestKey, sptr<IRemoteObject> callerToken);

    /**
     * RecordAppWithReasonByUserId, record app exit reason by userId.
     *
     * @param userId The user id.
     * @param exitReason The reason of app exit.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RecordAppWithReasonByUserId(int32_t userId, const ExitReasonCompability &exitReason);

    /**
     * Start ui session ability with extension session info, send session info to ability manager service.
     *
     * @param want Ability want.
     * @param callerToken caller ability token.
     * @param sessionInfo the information of UIExtensionContentSession.
     * @param requestCode Ability request code.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityByUIContentSession(
        const Want &want,
        sptr<IRemoteObject> callerToken,
        sptr<AAFwk::SessionInfo> sessionInfo,
        int requestCode = DEFAULT_INVAL_VALUE,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Start ui session ability with extension session info, send session info to ability manager service.
     *
     * @param want, the want of the ability to start.
     * @param startOptions Indicates the options used to start.
     * @param callerToken caller ability token.
     * @param sessionInfo the information of UIExtensionContentSession.
     * @param requestCode the resultCode of the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityByUIContentSession(
        const Want &want,
        const StartOptions &startOptions,
        sptr<IRemoteObject> callerToken,
        sptr<AAFwk::SessionInfo> sessionInfo,
        int requestCode = DEFAULT_INVAL_VALUE,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Start ui ability
     *
     * @param want the want of the ability to start.
     * @param callerToken caller ability token.
     * @param specifyTokenId The Caller ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityOnlyUIAbility(
        const Want &want,
        sptr<IRemoteObject> callerToken,
        uint32_t specifyTokenId);

    /**
     * Start UIAbility with callback to receive the request result, the callback is valid only for SA callers.
     *
     * @param want Indicates the ability to start.
     * @param callerToken Indicates the caller ability token.
     * @param callback Indicates the callback used to receive the result of request start ability.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartUIAbilityWithCallback(
        const Want &want,
        sptr<IRemoteObject> callerToken,
        sptr<IRequestStartAbilityCallback> callback);

    /**
     * Start extension ability with want, send want to ability manager service.
     *
     * @param want, the want of the ability to start.
     * @param callerToken, caller ability token.
     * @param userId, Designation User ID.
     * @param extensionType If an ExtensionAbilityType is set, only extension of that type can be started.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartExtensionAbility(
        const Want &want,
        sptr<IRemoteObject> callerToken,
        int32_t userId = DEFAULT_INVAL_VALUE,
        AppExecFwk::ExtensionAbilityType extensionType = AppExecFwk::ExtensionAbilityType::UNSPECIFIED);

    /**
     * Create UIExtension with want, send want to ability manager service.
     *
     * @param want, the want of the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RequestModalUIExtension(const Want &want);

    /**
     * Request modal UIExtension with account id.
     *
     * @param want, the want of the modal UIExtension to request.
     * @param accountId, the account id for multi-user scenario.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RequestModalUIExtensionWithAccount(const Want &want, int32_t accountId);

    /**
     * Preload UIExtension with want, send want to ability manager service.
     *
     * @param want, the want of the ability to start.
     * @param hostBundleName, the caller application bundle name.
     * @param requestCode the resultCode of the preload ui extension ability to start.
     * @param userId, the extension runs in.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode PreloadUIExtensionAbility(const Want &want, std::string &hostBundleName,
        int32_t userId = DEFAULT_INVAL_VALUE, int32_t hostPid = DEFAULT_INVAL_VALUE,
        int32_t requestCode = DEFAULT_INVAL_VALUE);

    /**
     * Change the visibility state of an UIAbility.
     *
     * @param token The destination UIAbility.
     * @param isShow The wanted state, show or hide.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ChangeAbilityVisibility(sptr<IRemoteObject> token, bool isShow);

    /**
     * Change the visibility state of an UIAbility by SCB.
     *
     * @param sessionInfo The destination UIAbility.
     * @param isShow The wanted state, show or hide.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ChangeUIAbilityVisibilityBySCB(sptr<SessionInfo> sessionInfo, bool isShow);

    /**
     * Start ui extension ability with extension session info, send extension session info to ability manager service.
     *
     * @param extensionSessionInfo the extension session info of the ability to start.
     * @param userId, Designation User ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartUIExtensionAbility(
        sptr<SessionInfo> extensionSessionInfo,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Start ui ability with want, send want to ability manager service.
     *
     * @param sessionInfo the session info of the ability to start.
     * @param params start parameters.
     * @param isColdStart the session info of the ability is or not cold start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartUIAbilityBySCB(sptr<SessionInfo> sessionInfo, AbilityRuntime::StartParamsBySCB &params,
        bool &isColdStart);

    /**
     * Stop extension ability with want, send want to ability manager service.
     *
     * @param want, the want of the ability to stop.
     * @param callerToken, caller ability token.
     * @param userId, Designation User ID.
     * @param extensionType If an ExtensionAbilityType is set, only extension of that type can be stopped.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StopExtensionAbility(
        const Want& want,
        sptr<IRemoteObject> callerToken,
        int32_t userId = DEFAULT_INVAL_VALUE,
        AppExecFwk::ExtensionAbilityType extensionType = AppExecFwk::ExtensionAbilityType::UNSPECIFIED);

    /**
     * TerminateAbility with want, return want from ability manager service.
     *
     * @param token Ability token.
     * @param resultCode resultCode.
     * @param Want Ability want returned.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode TerminateAbility(sptr<IRemoteObject> token, int resultCode, const Want *resultWant);

    /**
     * StartSelf, start the ability itself with token.
     *
     * @param token, the token of the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartSelf(sptr<IRemoteObject> token);

    /**
     * BackToCallerAbilityWithResult, return to the caller ability.
     *
     * @param token, the token of the ability to terminate.
     * @param resultCode, the resultCode of the ability to terminate.
     * @param resultWant, the Want of the ability to return.
     * @param callerRequestCode, the requestCode of caller ability.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode BackToCallerAbilityWithResult(const sptr<IRemoteObject> &token, int resultCode,
        const Want *resultWant, int64_t callerRequestCode);

    /**
     * TerminateUIServiceExtensionAbility with token.
     *
     * @param token Ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode TerminateUIServiceExtensionAbility(sptr<IRemoteObject> token);

    /**
     * TerminateUIExtensionAbility with want, return want from ability manager service.
     *
     * @param extensionSessionInfo the extension session info of the ability to terminate.
     * @param resultCode resultCode.
     * @param Want Ability want returned.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode TerminateUIExtensionAbility(sptr<SessionInfo> extensionSessionInfo,
        int resultCode = DEFAULT_INVAL_VALUE, const Want *resultWant = nullptr);

    /**
     * CloseUIExtensionAbilityBySCB, terminate the specified ui extension ability by SCB.
     *
     * @param token the ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode CloseUIExtensionAbilityBySCB(const sptr<IRemoteObject> token);

    /**
     *  CloseUIAbilityBySCB, close the special ability by scb.
     *
     * @param sessionInfo the session info of the ability to terminate.
     * @param isUserRequestedExit determine whether it is a user request to exit.
     * @param sceneFlag the reason info of the ability to terminate.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode CloseUIAbilityBySCB(sptr<SessionInfo> sessionInfo,
        bool isUserRequestedExit = false, uint32_t sceneFlag = 0);

    /**
     * SendResultToAbility with want, return resultWant from ability manager service.
     *
     * @param requestCode requestCode.
     * @param resultCode resultCode.
     * @param resultWant Ability want returned.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode SendResultToAbility(int requestCode, int resultCode, Want& resultWant);

    /**
     * MoveAbilityToBackground.
     *
     * @param token Ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode MoveAbilityToBackground(sptr<IRemoteObject> token);

    /**
     * Move the UIAbility to background, called by app self.
     *
     * @param token the token of the ability to move.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode MoveUIAbilityToBackground(const sptr<IRemoteObject> token);

    /**
     * CloseAbility with want, return want from ability manager service.
     *
     * @param token Ability token.
     * @param resultCode resultCode.
     * @param Want Ability want returned.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode CloseAbility(sptr<IRemoteObject> token, int resultCode = DEFAULT_INVAL_VALUE,
        const Want *resultWant = nullptr);

    /**
     * MinimizeAbility, minimize the special ability.
     *
     * @param token, ability token.
     * @param fromUser mark the minimize operation source.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode MinimizeAbility(sptr<IRemoteObject> token, bool fromUser = false);

    /**
     * MinimizeUIExtensionAbility, minimize the special ui extension ability.
     *
     * @param extensionSessionInfo the extension session info of the ability to minimize.
     * @param fromUser mark the minimize operation source.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode MinimizeUIExtensionAbility(sptr<SessionInfo> extensionSessionInfo, bool fromUser = false);

    /**
     * MinimizeUIAbilityBySCB, minimize the special ability by scb.
     *
     * @param sessionInfo the session info of the ability to minimize.
     * @param fromUser, Whether form user.
     * @param backgroundReason The reason for moving to background (3: screen off).
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode MinimizeUIAbilityBySCB(sptr<SessionInfo> sessionInfo, bool fromUser = false, uint32_t sceneFlag = 0,
        int32_t backgroundReason = 0);

    /**
     * ConnectAbility, connect session with service ability.
     *
     * @param want, Special want for service type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectAbility(const Want &want, sptr<IAbilityConnection> connect, int32_t userId);

    /**
     * ConnectAbility, connect session with service ability.
     *
     * @param want, Special want for service type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param loadTimeout, timeout multiply for ability loading stage, range 1-30, not work on asan
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectAbility(const Want &want, sptr<IAbilityConnection> connect, int32_t userId,
        int32_t loadTimeout);

    /**
     * ConnectAbility, connect session with service ability.
     *
     * @param want, Special want for service type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param callerToken, caller ability token.
     * @param specifiedFullTokenId, The specified full token ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectAbility(
        const Want &want,
        sptr<IAbilityConnection> connect,
        sptr<IRemoteObject> callerToken,
        int32_t userId = DEFAULT_INVAL_VALUE,
        uint64_t specifiedFullTokenId = 0);

    /**
     * ConnectAbilityWithIndirectCallerInfo, connect session with service ability.
     *
     * @param want, Special want for service type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param callerToken, caller ability token.
     * @param extensionType If an ExtensionAbilityType is set, only extension of that type can be connected.
     * @param indirectCallerInfo, Indirect caller information.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectAbilityWithIndirectCallerInfo(
        const Want &want,
        sptr<IAbilityConnection> connect,
        sptr<IRemoteObject> callerToken,
        int32_t userId = DEFAULT_INVAL_VALUE,
        AppExecFwk::ExtensionAbilityType extensionType = AppExecFwk::ExtensionAbilityType::UNSPECIFIED,
        std::shared_ptr<IndirectCallerInfo> indirectCallerInfo = nullptr);
    
    /**
     * ConnectAbilityWithExtensionType, connect session with specified extentionType ability.
     *
     * @param want, Special want for appService type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param callerToken, caller ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectAbilityWithExtensionType(
        const Want &want,
        sptr<IAbilityConnection> connect,
        sptr<IRemoteObject> callerToken,
        int32_t userId = DEFAULT_INVAL_VALUE,
        AppExecFwk::ExtensionAbilityType extensionType = AppExecFwk::ExtensionAbilityType::SERVICE);

    /**
     * ConnectUIServiceExtensionAbility, connect session with uiService ability.
     *
     * @param want, Special want for uiService type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param callerToken, caller ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectUIServiceExtesnionAbility(
        const Want &want,
        sptr<IAbilityConnection> connect,
        sptr<IRemoteObject> callerToken,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Connect data share extension ability.
     *
     * @param want, special want for the data share extension ability.
     * @param connect, callback used to notify caller the result of connecting or disconnecting.
     * @param userId, the extension runs in.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectDataShareExtensionAbility(const Want &want, sptr<IAbilityConnection> connect,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Connect extension ability.
     *
     * @param want, special want for the extension ability.
     * @param connect, callback used to notify caller the result of connecting or disconnecting.
     * @param userId, the extension runs in.
     * @param loadTimeout, timeout multiply for ability loading stage, range 1-30, not work on asan
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectExtensionAbility(const Want &want, sptr<IAbilityConnection> connect,
        int32_t userId = DEFAULT_INVAL_VALUE, int32_t loadTimeout = 0);

    /**
     * Connect ui extension ability.
     *
     * @param want, special want for the ui extension ability.
     * @param connect, callback used to notify caller the result of connecting or disconnecting.
     * @param sessionInfo the extension session info of the ability to connect.
     * @param userId, the extension runs in.
     * @param connectInfo the connect info.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ConnectUIExtensionAbility(const Want &want, sptr<IAbilityConnection> connect,
        sptr<SessionInfo> sessionInfo, int32_t userId = DEFAULT_INVAL_VALUE,
        sptr<UIExtensionAbilityConnectInfo> connectInfo = nullptr);

    /**
     * DisconnectAbility, disconnect session with service ability.
     *
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode DisconnectAbility(sptr<IAbilityConnection> connect);

    /**
     * AcquireDataAbility, acquire a data ability by its authority, if it not existed,
     * AMS loads it synchronously.
     *
     * @param uri, data ability uri.
     * @param tryBind, true: when a data ability is died, ams will kill this client, or do nothing.
     * @param callerToken, specifies the caller ability token.
     * @return returns the data ability ipc object, or nullptr for failed.
     */
    sptr<IAbilityScheduler> AcquireDataAbility(const Uri &uri, bool tryBind, sptr<IRemoteObject> callerToken);

    /**
     * ReleaseDataAbility, release the data ability that referenced by 'dataAbilityToken'.
     *
     * @param dataAbilityToken, specifies the data ability that will be released.
     * @param callerToken, specifies the caller ability token.
     * @return returns ERR_OK if succeeded, or error codes for failed.
     */
    ErrCode ReleaseDataAbility(sptr<IAbilityScheduler> dataAbilityScheduler, sptr<IRemoteObject> callerToken);

    /**
     * dump ability stack info, about userID, mission stack info,
     * mission record info and ability info.
     *
     * @param state Ability stack info.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode DumpState(const std::string &args, std::vector<std::string> &state);
    ErrCode DumpSysState(
        const std::string& args, std::vector<std::string>& state, bool isClient, bool isUserID, int UserID);
    /**
     * Connect ability manager service.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode Connect();

    /**
     * Destroys this Service ability by Want.
     *
     * @param want, Special want for service type's ability.
     * @param token ability's token.
     * @return Returns true if this Service ability will be destroyed; returns false otherwise.
     */
    ErrCode StopServiceAbility(const Want &want, sptr<IRemoteObject> token = nullptr);

    /**
     * Kill the process immediately.
     *
     * @param bundleName.
     * @param clearPageStack.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode KillProcess(const std::string &bundleName, bool clearPageStack = false, int32_t appIndex = 0,
        const std::string& reason = "Abilityms::KillProcess");

    #ifdef ABILITY_COMMAND_FOR_TEST
    /**
     * Force ability timeout.
     *
     * @param abilityName.
     * @param state. ability lifecycle state.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ForceTimeoutForTest(const std::string &abilityName, const std::string &state);
    #endif

    /**
     * ContinueMission, continue ability from mission center.
     *
     * @param srcDeviceId, origin deviceId.
     * @param dstDeviceId, target deviceId.
     * @param missionId, indicates which ability to continue.
     * @param callBack, notify result back.
     * @param wantParams, extended params.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ContinueMission(const std::string &srcDeviceId, const std::string &dstDeviceId, int32_t missionId,
        sptr<IRemoteObject> callback, AAFwk::WantParams &wantParams);

    /**
     * ContinueMission, continue ability from mission center.
     *
     * @param srcDeviceId, origin deviceId.
     * @param dstDeviceId, target deviceId.
     * @param bundleName, indicates which bundleName to continue.
     * @param callBack, notify result back.
     * @param wantParams, extended params.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ContinueMission(AAFwk::ContinueMissionInfo continueMissionInfo, const sptr<IRemoteObject> &callback);

    /**
     * start continuation.
     * @param want, used to start a ability.
     * @param abilityToken, ability token.
     * @param status, continue status.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartContinuation(const Want &want, sptr<IRemoteObject> abilityToken, int32_t status);

    /**
     * notify continuation complete to dms.
     * @param deviceId, source device which start a continuation.
     * @param sessionId, represent a continuation.
     * @param isSuccess, continuation result.
     * @return
     */
    void NotifyCompleteContinuation(const std::string &deviceId, int32_t sessionId, bool isSuccess);

    /**
     * ContinueMission, continue ability from mission center.
     * @param deviceId, target deviceId.
     * @param missionId, indicates which ability to continue.
     * @param versionCode, version of the remote target ability.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ContinueAbility(const std::string &deviceId, int32_t missionId, uint32_t versionCode);

    /**
     * notify continuation result to application.
     * @param missionId, indicates which ability to notify.
     * @param result, continuation result.
     * @return
     */
    ErrCode NotifyContinuationResult(int32_t missionId, int32_t result);

    /**
     * @brief Lock specified mission.
     * @param missionId The id of target mission.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode LockMissionForCleanup(int32_t missionId);

    /**
     * @brief Unlock specified mission.
     * @param missionId The id of target mission.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode UnlockMissionForCleanup(int32_t missionId);

    /**
     * @brief change specified AbilityRecord lockState.
     * @param sessionId The id of target AbilityRecord.
     * @param lockState The lockState of target AbilityRecord.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    void SetLockedState(int32_t sessionId, bool lockedState);

    /**
     * @brief Register mission listener to ability mgr.
     * @param listener The handler of listener.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterMissionListener(sptr<IMissionListener> listener);

    /**
     * @brief UnRegister mission listener from ability mgr.
     * @param listener The handler of listener.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode UnRegisterMissionListener(sptr<IMissionListener> listener);

    /**
     * @brief Register mission listener to ability manager service.
     * @param deviceId The remote device Id.
     * @param listener The handler of listener.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterMissionListener(const std::string &deviceId, sptr<IRemoteMissionListener> listener);

    /**
     * @brief Register mission listener to ability manager service.
     * @param deviceId The remote device Id.
     * @param listener The handler of listener.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterOnListener(const std::string &type, sptr<IRemoteOnListener> listener);

    /**
     * @brief Register mission listener to ability manager service.
     * @param deviceId The remote device Id.
     * @param listener The handler of listener.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterOffListener(const std::string &type, sptr<IRemoteOnListener> listener);

    /**
     * @brief UnRegister mission listener from ability manager service.
     * @param deviceId The remote device Id.
     * @param listener The handler of listener.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode UnRegisterMissionListener(const std::string &deviceId, sptr<IRemoteMissionListener> listener);

    /**
     * @brief Get mission infos from ability mgr.
     * @param deviceId local or remote deviceId.
     * @param numMax max number of missions.
     * @param missionInfos mission info result.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetMissionInfos(const std::string &deviceId, int32_t numMax, std::vector<MissionInfo> &missionInfos);

    /**
     * @brief Get mission info by id.
     * @param deviceId local or remote deviceId.
     * @param missionId Id of target mission.
     * @param missionInfo mission info of target mission.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetMissionInfo(const std::string &deviceId, int32_t missionId, MissionInfo &missionInfo);

    /**
     * @brief Get mission info by id.
     * @param deviceId local or remote deviceId.
     * @param missionId Id of target mission.
     * @param missionInfo mission info of target mission.
     * @param displayInfo display info of target mission.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetMissionInfo(const std::string& deviceId, int32_t missionId, MissionInfo &missionInfo,
        DisplayInfo &displayInfo);

    /**
     * @brief Get the Mission Snapshot Info object
     * @param deviceId local or remote deviceId.
     * @param missionId Id of target mission.
     * @param snapshot snapshot of target mission.
     * @param isLowResolution get low resolution snapshot.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetMissionSnapshot(const std::string& deviceId, int32_t missionId,
        MissionSnapshot& snapshot, bool isLowResolution = false);

    /**
     * @brief Clean mission by id.
     * @param missionId Id of target mission.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode CleanMission(int32_t missionId);

    /**
     * @brief Clean all missions in system.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode CleanAllMissions();

    /**
     * @brief Move a mission to front.
     * @param missionId Id of target mission.
     * @param startOptions Special startOptions for target mission.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode MoveMissionToFront(int32_t missionId);
    ErrCode MoveMissionToFront(int32_t missionId, const StartOptions &startOptions);

    /**
     * Move missions to front
     * @param missionIds Ids of target missions
     * @param topMissionId Indicate which mission will be moved to top, if set to -1, missions' order won't change
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode MoveMissionsToForeground(const std::vector<int32_t>& missionIds, int32_t topMissionId);

    /**
     * Move missions to background
     * @param missionIds Ids of target missions
     * @param result The result of move missions to background, and the array is sorted by zOrder
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode MoveMissionsToBackground(const std::vector<int32_t>& missionIds, std::vector<int32_t>& result);

    /**
     * @brief Get mission id by ability token.
     *
     * @param token ability token.
     * @param missionId output mission id.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetMissionIdByToken(sptr<IRemoteObject> token, int32_t &missionId);

    /**
     * Start Ability, connect session with common ability.
     *
     * @param want, Special want for service type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param isSilent, whether show window when start fail by interceptorExecuter.
     * @param isVisible, whether show window when start fail by afterCheckExecuter.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityByCall(const Want &want, sptr<IAbilityConnection> connect, bool isSilent = false,
        bool isVisible = false);

    /**
     * Start Ability, connect session with common ability.
     *
     * @param want, Special want for service type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param callerToken Indicates the caller's identity
     * @param accountId Indicates the account to start.
     * @param isSilent, whether show window when start fail by interceptorExecuter.
     * @param promotePriority, whether to promote priority for sa.
     * @param isVisible, whether show window when start fail by afterCheckExecuter.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityByCall(const Want &want, sptr<IAbilityConnection> connect,
        sptr<IRemoteObject> callToken, int32_t accountId = DEFAULT_INVAL_VALUE, bool isSilent = false,
        bool promotePriority = false, bool isVisible = false);

    /**
     * Start Ability, connect session with common ability.
     *
     * @param want, Special want for service type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param callerToken Indicates the caller's identity
     * @param accountId Indicates the account to start.
     * @param errMsg Out parameter, indicates the failed reason.
     * @param isSilent, whether show window when start fail by interceptorExecuter.
     * @param promotePriority, whether to promote priority for sa.
     * @param isVisible, whether show window when start fail by afterCheckExecuter.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t StartAbilityByCallWithErrMsg(const Want &want, sptr<IAbilityConnection> connect,
        sptr<IRemoteObject> callToken, int32_t accountId, std::string &errMsg, bool isSilent = false,
        bool promotePriority = false, bool isVisible = false);

    /**
     * Start Ability for prelaunch
     *
     * @param want, Special want for service type's ability.
     * @param frameNum, Special frameNum for remove start window num.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartAbilityForPrelaunch(const Want &want, const int32_t frameNum = 0);
    /**
     * CallRequestDone, after invoke callRequest, ability will call this interface to return callee.
     *
     * @param token, ability's token.
     * @param callStub, ability's callee.
     */
    void CallRequestDone(sptr<IRemoteObject> token, sptr<IRemoteObject> callStub);

    /**
     * Get ability token by connect.
     *
     * @param token The token of ability.
     * @param callStub The callee object.
     */
    void GetAbilityTokenByCalleeObj(sptr<IRemoteObject> callStub, sptr<IRemoteObject> &token);

    /**
     * Release the call between Ability, disconnect session with common ability.
     *
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ReleaseCall(sptr<IAbilityConnection> connect, const AppExecFwk::ElementName &element);

    /**
     * @brief Get the ability running information.
     *
     * @param info Ability running information.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetAbilityRunningInfos(std::vector<AbilityRunningInfo> &info);

    /**
     * @brief Get the extension running information.
     *
     * @param upperLimit The maximum limit of information wish to get.
     * @param info Extension running information.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetExtensionRunningInfos(int upperLimit, std::vector<ExtensionRunningInfo> &info);

    /**
     * @brief Get running process information.
     *
     * @param info Running process information.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetProcessRunningInfos(std::vector<AppExecFwk::RunningProcessInfo> &info);

    /**
     * Start synchronizing remote device mission
     * @param devId, deviceId.
     * @param fixConflict, resolve synchronizing conflicts flag.
     * @param tag, call tag.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartSyncRemoteMissions(const std::string &devId, bool fixConflict, int64_t tag);

    /**
     * Stop synchronizing remote device mission
     * @param devId, deviceId.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StopSyncRemoteMissions(const std::string &devId);

    /**
     * @brief start user.
     * @param accountId accountId.
     * @param displayId logical screen id.
     * @param accountId is appRecovery or not.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartUser(int accountId, uint64_t displayId, sptr<IUserCallback> callback, bool isAppRecovery = false);

    /**
     * @brief stop user.
     * @param accountId accountId.
     * @param callback callback.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StopUser(int accountId, sptr<IUserCallback> callback);

    /**
     * @brief logout user.
     * @param accountId accountId.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode LogoutUser(int32_t accountId, sptr<IUserCallback> callback = nullptr);

    /**
     * @brief Register the snapshot handler
     * @param handler snapshot handler
     * @return ErrCode Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterSnapshotHandler(sptr<ISnapshotHandler> handler);

    /**
     * PrepareTerminateAbility with want, if terminate, return want from ability manager service.
     *
     * @param token Ability token.
     * @param callback callback.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode PrepareTerminateAbility(sptr<IRemoteObject> token, sptr<IPrepareTerminateCallback> callback);

    ErrCode RegisterStatusBarDelegate(sptr<AbilityRuntime::IStatusBarDelegate> delegate);

    ErrCode KillProcessWithPrepareTerminate(const std::vector<int32_t> &pids, bool clear = false);

    /**
     * kill the process with reason
     *
     * @param pid id of process.
     * @param  reason, kill process reason.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode KillProcessWithReason(int32_t pid, const ExitReason &reason);

    /**
     * @brief Register auto start up callback for system api.
     * @param callback The point of JsAbilityAutoStartupCallBack.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterAutoStartupSystemCallback(sptr<IRemoteObject> callback);

    /**
     * @brief Unregister auto start up callback for system api.
     * @param callback The point of JsAbilityAutoStartupCallBack.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode UnregisterAutoStartupSystemCallback(sptr<IRemoteObject> callback);

    /**
     * @brief Set every application auto start up state.
     * @param info The auto startup info,include bundle name, module name, ability name.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode SetApplicationAutoStartup(const AutoStartupInfo &info);

    /**
     * @brief Cancel every application auto start up .
     * @param info The auto startup info,include bundle name, module name, ability name.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode CancelApplicationAutoStartup(const AutoStartupInfo &info);

    /**
     * @brief Query auto startup state all application.
     * @param infoList Output parameters, return auto startup info list.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode QueryAllAutoStartupApplications(std::vector<AutoStartupInfo> &infoList);

    /**
     * @brief Retrieves the auto startup status of the current application.
     * @param isAutoStartEnabled Indicates whether auto startup is enabled for the current application.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetAutoStartupStatusForSelf(bool &isAutoStartEnabled);

    /**
     * @brief Manual start auto startup apps, EDM use only.
     * @param userId Indicates which user's auto startup apps to be started.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ManualStartAutoStartupApps(int32_t userId);

    /**
     * @brief Query the caller's Token ID for anco.
     * @param userId Indicates the user ID.
     * @param asCallerForAncoSessionId Indicates the anco session Id of cached information.
     * @param callerTokenId Indicates the output caller Token ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode QueryCallerTokenIdForAnco(int32_t userId, const std::string &asCallerForAncoSessionId,
        uint32_t &callerTokenId);

    /**
     * @brief Launch game customized with game SA verification.
     * @param bundleName Name of the game application.
     * @param userId Indicates the user ID.
     * @param appIndex app clone index. Currently, only appIndex = 0 is supported.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode LaunchGameCustomized(const std::string &bundleName, int32_t userId, int32_t appIndex = 0);
    
    /**
     * SetGamePreLaunchCompleteTime, set the complete time (in milliseconds) for the game pre-launch.
     *
     * @param userId Indicates the user ID.
     * @param completeTime The complete time (in milliseconds) for the game pre-launch.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode SetGamePreLaunchCompleteTime(int32_t userId, int64_t completeTime);

    /**
     * PrepareTerminateAbilityBySCB, prepare to terminate ability by scb.
     *
     * @param sessionInfo the session info of the ability to terminate.
     * @param isPrepareTerminate the result of ability onPrepareToTerminate.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode PrepareTerminateAbilityBySCB(sptr<SessionInfo> sessionInfo, bool &isPrepareTerminate);

    /**
     * Set mission continue state of this ability.
     *
     * @param token Indicate token of ability.
     * @param state the mission continuation state of this ability.
     * @return Returns ERR_OK if success.
     */
    ErrCode SetMissionContinueState(sptr<IRemoteObject> token, const AAFwk::ContinueState &state,
        sptr<IRemoteObject> sessionToken);

#ifdef SUPPORT_SCREEN
    /**
     * Set mission label of this ability.
     *
     * @param abilityToken Indicate token of ability.
     * @param label Indicate the label showed of the ability in recent missions.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode SetMissionLabel(sptr<IRemoteObject> abilityToken, const std::string &label);

    /**
     * Set mission icon of this ability.
     *
     * @param abilityToken Indicate token of ability.
     * @param icon Indicate the icon showed of the ability in recent missions.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode SetMissionIcon(sptr<IRemoteObject> abilityToken,
        std::shared_ptr<OHOS::Media::PixelMap> icon);

    /**
     * Register the WindowManagerService handler
     *
     * @param handler Indicate handler of WindowManagerService.
     * @return ErrCode Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterWindowManagerServiceHandler(sptr<IWindowManagerServiceHandler> handler,
        bool animationEnabled = true);

    /**
     * WindowManager notification AbilityManager after the first frame is drawn.
     *
     * @param abilityToken Indicate token of ability.
     */
    void CompleteFirstFrameDrawing(sptr<IRemoteObject> abilityToken);

    /**
     * WindowManager notification AbilityManager after the first frame is drawn.
     *
     * @param sessionId Indicate session id.
     */
    void CompleteFirstFrameDrawing(int32_t sessionId);

    /**
     * Called to update mission snapshot.
     * @param token The target ability.
     * @param pixelMap The snapshot.
     */
    void UpdateMissionSnapShot(sptr<IRemoteObject> token,
        std::shared_ptr<OHOS::Media::PixelMap> pixelMap);

    ErrCode GetDialogSessionInfo(const std::string &dialogSessionId, sptr<DialogSessionInfo> &info);
    ErrCode SendDialogResult(const Want &want, const std::string &dialogSessionId, bool isAllow);
#endif

    /**
     * @brief start user test.
     * @param want the want of the ability user test to start.
     * @param observer test observer callback.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartUserTest(const Want &want, sptr<IRemoteObject> observer);

    /**
     * @brief Finish user test.
     * @param msg user test message.
     * @param resultCode user test result Code.
     * @param bundleName user test bundleName.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode FinishUserTest(const std::string &msg, const int64_t &resultCode, const std::string &bundleName);

     /**
     * GetTopAbility, get the token of top ability.
     *
     * @param token, the token of top ability.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetTopAbility(sptr<IRemoteObject> &token);

    ErrCode CheckUIExtensionIsFocused(uint32_t uiExtensionTokenId, bool& isFocused, uint64_t displayId = 0);

    /**
     * DelegatorDoAbilityForeground, the delegator calls this interface to move the ability to the foreground.
     *
     * @param token, ability's token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode DelegatorDoAbilityForeground(sptr<IRemoteObject> token);

    /**
     * DelegatorDoAbilityBackground, the delegator calls this interface to move the ability to the background.
     *
     * @param token, ability's token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode DelegatorDoAbilityBackground(sptr<IRemoteObject> token);

   /**
     * Calls this interface to move the ability to the foreground.
     *
     * @param token, ability's token.
     * @param flag, use for lock or unlock flag and so on.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode DoAbilityForeground(sptr<IRemoteObject> token, uint32_t flag);

    /**
     * Calls this interface to move the ability to the background.
     *
     * @param token, ability's token.
     * @param flag, use for lock or unlock flag and so on.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode DoAbilityBackground(sptr<IRemoteObject> token, uint32_t flag);

    /**
     * Set ability controller.
     *
     * @param abilityController, The ability controller.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int SetAbilityController(sptr<AppExecFwk::IAbilityController> abilityController,
        bool imAStabilityTest);

    /**
     * Free install ability from remote DMS.
     *
     * @param want Ability want.
     * @param callback Callback used to notify free install result.
     * @param userId User ID.
     * @param requestCode Ability request code.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode FreeInstallAbilityFromRemote(const Want &want, sptr<IRemoteObject> callback, int32_t userId,
        int requestCode = DEFAULT_INVAL_VALUE);

    /**
     * Called when client complete dump.
     *
     * @param infos The dump info.
     * @param callerToken The caller ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode DumpAbilityInfoDone(std::vector<std::string> &infos, sptr<IRemoteObject> callerToken);

    /**
     * @brief Enable recover ability.
     *
     * @param token Ability identify.
     */
    void EnableRecoverAbility(sptr<IRemoteObject> token);

    /**
     * @brief Submit save recovery info.
     *
     * @param token Ability identify.
     */
    void SubmitSaveRecoveryInfo(sptr<IRemoteObject> token);

    /**
     * @brief Schedule recovery ability.
     *
     * @param token Ability identify.
     * @param reason See AppExecFwk::StateReason.
     * @param want Want information.
     */
    void ScheduleRecoverAbility(sptr<IRemoteObject> token, int32_t reason, const Want *want = nullptr);

    /**
     * @brief Schedule clear recovery page stack.
     *
     * @param bundleName application bundleName.
     */
    void ScheduleClearRecoveryPageStack();

    /**
     * @brief Add free install observer.
     *
     * @param callerToken The caller ability token.
     * @param observer Free install observer.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode AddFreeInstallObserver(const sptr<IRemoteObject> callToken,
        const sptr<AbilityRuntime::IFreeInstallObserver> observer);

    /**
     * Called to verify that the MissionId is valid.
     * @param missionIds Query mission list.
     * @param results Output parameters, return results up to 20 query results.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t IsValidMissionIds(const std::vector<int32_t> &missionIds, std::vector<MissionValidResult> &results);

    /**
     * Query whether the application of the specified PID and UID has been granted a certain permission
     * @param permission
     * @param pid Process id
     * @param uid
     * @return Returns ERR_OK if the current process has the permission, others on failure.
     */
    ErrCode VerifyPermission(const std::string &permission, int pid, int uid);

    /**
     * Acquire the shared data.
     * @param missionId The missionId of Target ability.
     * @param The IAcquireShareDataCallback object.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode AcquireShareData(int32_t missionId, sptr<IAcquireShareDataCallback> shareData);

    /**
     * Notify sharing data finished.
     * @param resultCode The result of sharing data.
     * @param uniqueId The uniqueId from request object.
     * @param wantParam The params of acquiring sharing data from target ability.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ShareDataDone(
        sptr<IRemoteObject> token, int32_t resultCode, int32_t uniqueId, WantParams &wantParam);

    /**
     * Request dialog service with want, send want to ability manager service.
     *
     * @param want target component.
     * @param callerToken caller ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RequestDialogService(
        const Want &want,
        sptr<IRemoteObject> callerToken);

    /**
     * Force app exit and record exit reason.
     * @param pid Process id .
     * @param exitReason The reason of app exit.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ForceExitApp(const int32_t pid, const ExitReason &exitReason);

    /**
     * Record app exit reason.
     * @param exitReason The reason of app exit.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RecordAppExitReason(const ExitReason &exitReason);

    /**
     * Record the process exit reason before the process being killed.
     * @param pid The process id.
     * @param exitReason The reason of process exit.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t RecordProcessExitReason(const int32_t pid, const ExitReason &exitReason);

    /**
     * Record the exit reason of a killed process.
     * @param pid The process id.
     * @param uid The process uid.
     * @param exitReason The reason of process exit.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RecordProcessExitReason(int32_t pid, int32_t uid, const ExitReason &exitReason);

    /**
     * kill app reason.
     * @param pid The process id.
     * @param exitReason The reason of kill app.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode KillAppWithReason(int32_t pid, const ExitReasonCompability &exitReason);

    /**
     * Force bundle exit and record exit reason.
     * @param bundleName Bundle name of kill app.
     * @param appIndex The app index of app clone.
     * @param userId User ID.
     * @param exitReason The reason of app exit.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode KillBundleWithReason(
        const std::string &bundleName, int32_t userId, int32_t appIndex, const ExitReasonCompability &exitReason);

    /**
     * Record app exit reason.
     * @param pid The process id.
     * @param uid The process uid.
     * @param exitReason The reason of app exit.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RecordAppWithReason(int32_t pid, int32_t uid, const ExitReasonCompability &exitReason);

    /**
     * Set rootSceneSession by SCB.
     *
     * @param rootSceneSession Indicates root scene session of SCB.
     */
    void SetRootSceneSession(sptr<IRemoteObject> rootSceneSession);

    /**
     * Call UIAbility by SCB.
     *
     * @param sessionInfo the session info of the ability to be called.
     * @param params start parameters.
     * @param isColdStart the session of the ability is or not cold start.
     */
    void CallUIAbilityBySCB(sptr<SessionInfo> sessionInfo, AbilityRuntime::StartParamsBySCB &params, bool &isColdStart);

    /**
     * Start specified ability by SCB.
     *
     * @param want Want information.
     * @param params The parameters to start specified ability.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t StartSpecifiedAbilityBySCB(const Want &want, const StartSpecifiedAbilityParams &params);

    /**
     * Notify sandbox app the result of saving file.
     * @param want Result of saving file, which contains the file's uri if success.
     * @param resultCode Indicates the action's result.
     * @param requestCode Pass the requestCode to match request.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode NotifySaveAsResult(const Want &want, int resultCode, int requestCode);

    /**
     * Set sessionManagerService
     * @param sessionManagerService the point of sessionManagerService.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode SetSessionManagerService(sptr<IRemoteObject> sessionManagerService);

    ErrCode ReportDrawnCompleted(sptr<IRemoteObject> token);

    /**
     * @brief Register collaborator.
     * @param type collaborator type.
     * @param impl collaborator.
     * @return Returns ERR_OK on success, others on failure.
    */
    ErrCode RegisterIAbilityManagerCollaborator(
        int32_t type, sptr<IAbilityManagerCollaborator> impl);

    /**
     * @brief Unregister collaborator.
     * @param type collaborator type.
     * @return Returns ERR_OK on success, others on failure.
    */
    ErrCode UnregisterIAbilityManagerCollaborator(int32_t type);

    /**
     * @brief get ability manager collaborator.
     * @return Returns object pointer on success, others on null.
     */
    sptr<IAbilityManagerCollaborator> GetAbilityManagerCollaborator();

    /**
     * @brief Register session handler.
     * @param object The handler.
     *
     * @return Returns ERR_OK on success, others on failure.
    */
    ErrCode RegisterSessionHandler(sptr<IRemoteObject> object);

    /**
     * @brief Register app debug listener.
     * @param listener App debug listener.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterAppDebugListener(sptr<AppExecFwk::IAppDebugListener> listener);

    /**
     * @brief Unregistering app debug listener.
     * @param listener App debug listener.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode UnregisterAppDebugListener(sptr<AppExecFwk::IAppDebugListener> listener);

    /**
     * @brief Attach app debug.
     * @param bundleName The application bundle name.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode AttachAppDebug(const std::string &bundleName, bool isDebugFromLocal = false);

    /**
     * @brief Detach app debug.
     * @param bundleName The application bundle name.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode DetachAppDebug(const std::string &bundleName, bool isDebugFromLocal = false);

    /**
     * @brief Check if ability controller can start.
     * @param want The want of ability to start.
     * @return Return true to allow ability to start, or false to reject.
     */
    bool IsAbilityControllerStart(const Want &want);

    /**
     * @brief Open file by uri.
     * @param uri The file uri.
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @return int The file descriptor.
     */
    int32_t OpenFile(const Uri& uri, uint32_t flag);

    /**
     * @brief Execute intent.
     * @param key The key of intent executing client.
     * @param callerToken Caller ability token.
     * @param param The Intent execute param.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ExecuteIntent(uint64_t key, sptr<IRemoteObject> callerToken,
        const InsightIntentExecuteParam &param);

    /**
      * @brief Execute intent for distributed scenario.
      *
      * @param want The want containing intent execution information.
      * @param srcDeviceId The source device id.
      * @param requestCode The Intent id.
      * @param specifiedFullTokenId The caller token id.
      * @return Returns ERR_OK on success, others on failure.
      */
    ErrCode ExecuteIntentForDistributed(const Want &want, const std::string &srcDeviceId,
        uint64_t requestCode, uint64_t specifiedFullTokenId = 0);

    /**
     * @brief Query entity info.
     * @param key The key of intent executing client.
     * @param callerToken Caller ability token.
     * @param param The Intent query param.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode QueryEntityInfo(uint64_t key, sptr<IRemoteObject> callerToken,
        const InsightIntentQueryParam &param);
     
    /**
     * @brief Execute intent with result synchronously.
     * @param callerToken Caller ability token.
     * @param param The Intent execute param.
     * @param result The Intent execute result output.
     * @param timeoutMs Timeout in milliseconds, default 30000ms.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ExecuteIntentWithResult(const InsightIntentExecuteParam &param, InsightIntentExecuteResult &result,
        int32_t timeoutMs = 30000);

    /**
     * @brief Called when insight intent execute finished.
     *
     * @param token ability's token.
     * @param intentId insight intent id.
     * @param result insight intent execute result.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ExecuteInsightIntentDone(sptr<IRemoteObject> token, uint64_t intentId,
        const InsightIntentExecuteResult &result);

    /**
     * @brief Get foreground ui abilities.
     * @param list Foreground ui abilities.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t GetForegroundUIAbilities(std::vector<AppExecFwk::AbilityStateData> &list);

    /**
     * @brief Update session info.
     * @param sessionInfos The vector of session info.
     */
    int32_t UpdateSessionInfoBySCB(std::list<SessionInfo> &sessionInfos, int32_t userId,
        std::vector<int32_t> &sessionIds);

    /**
     * @brief Restart app self.
     * @param want The ability type must be UIAbility.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t RestartApp(const AAFwk::Want &want);

    /**
     * @brief Get host info of root caller.
     *
     * @param token The ability token.
     * @param hostInfo The host info of root caller.
     * @param userId The user id.
     * @return ErrCode Returns ERR_OK on success, others on failure.
     */
    ErrCode GetUIExtensionRootHostInfo(const sptr<IRemoteObject> token, UIExtensionHostInfo &hostInfo,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * @brief Get ui extension session info
     *
     * @param token The ability token.
     * @param uiExtensionSessionInfo The ui extension session info.
     * @param userId The user id.
     * @return int32_t Returns ERR_OK on success, others on failure.
     */
    ErrCode GetUIExtensionSessionInfo(const sptr<IRemoteObject> token, UIExtensionSessionInfo &uiExtensionSessionInfo,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Pop-up launch of full-screen atomic service.
     *
     * @param want The want with parameters.
     * @param callerToken caller ability token.
     * @param requestCode Ability request code.
     * @param userId The User ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t OpenAtomicService(Want& want, const StartOptions &options, sptr<IRemoteObject> callerToken,
        int32_t requestCode = DEFAULT_INVAL_VALUE, int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * @brief Querying whether to allow embedded startup of atomic service.
     *
     * @param token The caller UIAbility token.
     * @param appId The ID of the application to which this bundle belongs.
     * @return Returns true to allow ability to start, or false to reject.
     */
    bool IsEmbeddedOpenAllowed(sptr<IRemoteObject> callerToken, const std::string &appId);

    /**
     * @brief Request to display assert fault dialog.
     * @param callback Listen for user operation callbacks.
     * @param wantParams Assert dialog box display information.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t RequestAssertFaultDialog(const sptr<IRemoteObject> &callback, const AAFwk::WantParams &wantParams);

    /**
     * @brief Notify the operation status of the user.
     * @param assertFaultSessionId Indicates the request ID of AssertFault.
     * @param userStatus Operation status of the user.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t NotifyDebugAssertResult(uint64_t assertFaultSessionId, AAFwk::UserStatus userStatus);

    /**
     * Set the enable status for starting and stopping resident processes.
     * The caller application can only set the resident status of the configured process.
     * @param bundleName The bundle name of the resident process.
     * @param enable Set resident process enable status.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t SetResidentProcessEnabled(const std::string &bundleName, bool enable);

    /**
     * Starts a new ability with specific start options.
     *
     * @param want, the want of the ability to start.
     * @param startOptions Indicates the options used to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t StartShortcut(const Want &want, const StartOptions &startOptions);

    /**
     * Get ability state by persistent id.
     *
     * @param persistentId, the persistentId of the session.
     * @param state Indicates the ability state.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t GetAbilityStateByPersistentId(int32_t persistentId, bool &state);

    /**
     * Transfer resultCode & want to abms.
     *
     * @param callerToken caller ability token.
     * @param requestCode the resultCode of the ability to start.
     * @param want Indicates the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t TransferAbilityResultForExtension(const sptr<IRemoteObject> &callerToken, int32_t resultCode,
        const Want &want);

    /**
     * Notify ability manager service frozen process.
     *
     * @param pidList, the pid list of the frozen process.
     * @param uid, the uid of the frozen process.
     */
    void NotifyFrozenProcessByRSS(const std::vector<int32_t> &pidList, int32_t uid);

    /**
     * Open atomic service window prior to finishing free install.
     *
     * @param bundleName, the bundle name of the atomic service.
     * @param moduleName, the module name of the atomic service.
     * @param abilityName, the ability name of the atomic service.
     * @param startTime, the starting time of the free install task.
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t PreStartMission(const std::string& bundleName, const std::string& moduleName,
        const std::string& abilityName, const std::string& startTime);

    /**
     *  Request to clean UIAbility from user.
     *
     * @param sessionInfo the session info of the ability to clean.
     * @param isUserRequestedExit determine whether it is a user request to exit.
     * @param sceneFlag the reason info of the ability to terminate.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode CleanUIAbilityBySCB(sptr<SessionInfo> sessionInfo,
        bool isUserRequestedExit = false, uint32_t sceneFlag = 0);

    /**
     * Open link of ability and atomic service.
     *
     * @param want Ability want.
     * @param callerToken Caller ability token.
     * @param userId User ID.
     * @param requestCode Ability request code.
     * @return Returns ERR_OK on success, others on failure.
    */
    int32_t OpenLink(const Want &want, sptr<IRemoteObject> callerToken, int32_t userId, int requestCode,
        bool hideFailureTipDialog = false);

    /**
     * Terminate process by bundleName.
     *
     * @param missionId, The mission id of the UIAbility need to be terminated.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode TerminateMission(int32_t missionId);

    /**
     * Notify ability manager to set the flag to block all apps from starting.
     * Needs to apply for ohos.permission.BLOCK_ALL_APP_START.
     * @param flag, The flag to block all apps from starting
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode BlockAllAppStart(bool flag);

    /**
     * update associate config list by rss.
     *
     * @param configs The rss config info.
     * @param exportConfigs The rss export config info.
     * @param flag UPDATE_CONFIG_FLAG_COVER is cover config, UPDATE_CONFIG_FLAG_APPEND is append config.
     */
    ErrCode UpdateAssociateConfigList(const std::map<std::string, std::list<std::string>>& configs,
        const std::list<std::string>& exportConfigs, int32_t flag);

    ErrCode GetAllIntentExemptionInfo(std::vector<AppExecFwk::IntentExemptionInfo>& info);

    /**
     * Add query ERMS observer.
     *
     * @param callerToken, The caller ability token.
     * @param observer, The observer of the ability to query ERMS.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode AddQueryERMSObserver(sptr<IRemoteObject> callerToken,
        sptr<AbilityRuntime::IQueryERMSObserver> observer);

    /**
     * Query atomic service ERMS rule.
     *
     * @param callerToken, The caller ability token.
     * @param appId, The appId of the atomic service.
     * @param startTime, The startTime of the query.
     * @param rule, The returned ERMS rule.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode QueryAtomicServiceStartupRule(sptr<IRemoteObject> callerToken,
        const std::string &appId, const std::string &startTime, AtomicServiceStartupRule &rule);

    /**
     * Restart atomic service.
     *
     * @param callerToken, The caller ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
     ErrCode RestartSelfAtomicService(sptr<IRemoteObject> callerToken);

    /**
     * PrepareTerminateAbilityDone, called when PrepareTerminateAbility call is done.
     *
     * @param token, the token of the ability to terminate.
     * @param callback callback.
     */
    void PrepareTerminateAbilityDone(sptr<IRemoteObject> token, bool isTerminate);

    /**
     * KillProcessWithPrepareTerminateDone, called when KillProcessWithPrepareTerminate call is done.
     *
     * @param moduleName, the module name of the application.
     * @param prepareTermination, the result of prepareTermination call of the module.
     * @param isExist, whether the prepareTerminate functions are implemented.
     */
    void KillProcessWithPrepareTerminateDone(const std::string &moduleName, int32_t prepareTermination, bool isExist);

    /**
     * KillProcessForPermissionUpdate
     * force kill the application by accessTokenId, notify exception to SCB.
     *
     * @param  accessTokenId, accessTokenId.
     * @return ERR_OK, return back success, others fail.
     */
    ErrCode KillProcessForPermissionUpdate(uint32_t accessTokenId);

    /**
     * Register hidden start observer.
     * @param observer, ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterHiddenStartObserver(const sptr<IHiddenStartObserver> &observer);

    /**
     * Unregister hidden start observer.
     * @param observer, ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode UnregisterHiddenStartObserver(const sptr<IHiddenStartObserver> &observer);

    /**
     * Query preload uiextension record.
     *
     * @param element, The uiextension ElementName.
     * @param moduleName, The uiextension moduleName.
     * @param hostPid, The uiextension caller pid.
     * @param recordNum, The returned count of uiextension.
     * @param userId, The User Id.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode QueryPreLoadUIExtensionRecord(const AppExecFwk::ElementName &element,
                                          const std::string &moduleName,
                                          const int32_t hostPid,
                                          int32_t &recordNum,
                                          int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Revoke delegator.
     *
     * @param token, ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RevokeDelegator(sptr<IRemoteObject> token);

    /**
     * Get all insight intent infos.
     * @param flag, the get type.
     * @param infos, the insight intent infos.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetAllInsightIntentInfo(
        AbilityRuntime::GetInsightIntentFlag flag,
        std::vector<InsightIntentInfoForQuery> &infos,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Get specified bundleName insight intent infos.
     * @param flag, the get type.
     * @param infos, the insight intent infos.
     * @param bundleName, The get insightIntent bundleName.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetInsightIntentInfoByBundleName(
        AbilityRuntime::GetInsightIntentFlag flag,
        const std::string &bundleName,
        std::vector<InsightIntentInfoForQuery> &infos,
        int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * Get specified intentName insight intent infos.
     * @param flag, the get type.
     * @param infos, the insight intent infos.
     * @param bundleName, The get insightIntent bundleName.
     * @param moduleName, The get insightIntent moduleName.
     * @param intentName, The get intent name.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetInsightIntentInfoByIntentName(
        AbilityRuntime::GetInsightIntentFlag flag,
        const std::string &bundleName,
        const std::string &moduleName,
        const std::string &intentName,
        InsightIntentInfoForQuery &info,
        int32_t userId = DEFAULT_INVAL_VALUE);

    ErrCode UpdateKioskApplicationList(const std::vector<std::string> &appList);

    ErrCode EnterKioskMode(sptr<IRemoteObject> callerToken);

    ErrCode ExitKioskMode(sptr<IRemoteObject> callerToken);

    ErrCode GetKioskStatus(AAFwk::KioskStatus &kioskStatus);

    /**
     * Register sa interceptor.
     * @param interceptor, The sa interceptor.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterSAInterceptor(sptr<AbilityRuntime::ISAInterceptor> interceptor);

    /**
     * SuspendExtensionAbility, suspend session with service ability.
     *
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode SuspendExtensionAbility(sptr<IAbilityConnection> connect);

    /**
     * ResumeExtensionAbility, resume session with service ability.
     *
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ResumeExtensionAbility(sptr<IAbilityConnection> connect);

    ErrCode SetOnNewWantSkipScenarios(sptr<IRemoteObject> callerToken, int32_t scenarios);

    ErrCode NotifyStartupExceptionBySCB(int32_t requestId);

    /**
     * Preload application.
     * @param bundleName Name of the application.
     * @param userId user id.
     * @param appIndex app clone index. Reserved field, only appIndex=0 is supported.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode PreloadApplication(const std::string &bundleName, int32_t userId, int32_t appIndex);

    /**
     * Start self UIAbility in current process.
     * @param want Ability want.
     * @param specifiedFlag specified flag.
     * @param startOptions Indicates the options used to start.
     * @param hasOptions Is have start options.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode StartSelfUIAbilityInCurrentProcess(const Want &want, const std::string &specifiedFlag,
        const AAFwk::StartOptions &startOptions, bool hasOptions, sptr<IRemoteObject> callerToken);

    /**
     * @brief Notify cancel game prelaunch and kill the process.
     * @param callerToken Indicates the caller ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode NotifyCancelGamePreLaunch(const sptr<IRemoteObject> callerToken);

    /**
     * @brief Notify complete game prelaunch and clear the flag.
     * @param callerToken Indicates the caller ability token.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode NotifyCompleteGamePreLaunch(const sptr<IRemoteObject> callerToken);

    /**
     * Check if the app is restart-limited.
     * @return Returns true on being limited.
     */
    bool IsRestartAppLimit();

    /**
     * UnPreload UIExtension with want, send want to ability manager service.
     *
     * @param extensionAbilityId The extension ability Id.
     * @param userId The User Id.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ClearPreloadedUIExtensionAbility(int32_t extensionAbilityId, int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * clear all Preload UIExtension with want, send want to ability manager service.
     *
     * @param userId The User Id.
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode ClearPreloadedUIExtensionAbilities(int32_t userId = DEFAULT_INVAL_VALUE);

    /**
     * @brief Register preload ui extension host client.
     * @param callerToken Caller ability token.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode RegisterPreloadUIExtensionHostClient(const sptr<IRemoteObject> &callerToken);

    /**
     * @brief UnRegister preload ui extension host client.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode UnRegisterPreloadUIExtensionHostClient(int32_t callerPid = DEFAULT_INVAL_VALUE);

    /**
 	 * @brief Queries self modular object extension information.
     * @param extensionInfos get the queried extensionInfos.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode QuerySelfModularObjectExtensionInfos(std::vector<ModularObjectExtensionInfo> &extensionInfos);

    /**
     * @brief Get list of applications launched before the first unlock.
     * @param userId The User Id.
     * @param userLockedBundleList List of applications launched before the first unlock.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    ErrCode GetUserLockedBundleList(int32_t userId, std::unordered_set<std::string> &userLockedBundleList);

    /**
     * @brief UnRegister preload ui extension host client.
     *
     * @return Returns ERR_OK on success, others on failure.
     */
    int32_t SetAppRecoveryFlag(const sptr<IRemoteObject>& token, int flag);

    ErrCode ExecuteInAppSkill(const std::string &bundleName, const std::string &moduleName,
        const std::string &skillName, const std::string &arkTSPath = "",
        const std::string &funcName = "",
        const std::shared_ptr<AAFwk::WantParams> &skillArgs = nullptr,
        const sptr<ISkillExecuteCallback> &callback = nullptr);

    ErrCode ExecuteInAppSkillWithTokenId(const AppExecFwk::SkillExecuteRequest &request,
        const sptr<ISkillExecuteCallback> &callback);

    ErrCode ExecuteSkillDone(sptr<IRemoteObject> token, const std::string &requestCode,
        int32_t resultCode, const AppExecFwk::SkillExecuteResult &result);

    ErrCode QuerySkillType(const std::string &bundleName, const std::string &moduleName,
        const std::string &skillName, int32_t &skillType);

public:
    AbilityManagerClient();
private:
    DISALLOW_COPY_AND_MOVE(AbilityManagerClient);

    class AbilityMgrDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        AbilityMgrDeathRecipient() = default;
        ~AbilityMgrDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject>& remote) override;
    private:
        DISALLOW_COPY_AND_MOVE(AbilityMgrDeathRecipient);
    };

    sptr<IAbilityManager> GetAbilityManager();
    void ResetProxy(wptr<IRemoteObject> remote);
    void HandleDlpApp(Want &want);

    static std::once_flag singletonFlag_;
    static std::shared_ptr<AbilityManagerClient> instance_;
    sptr<IAbilityManager> proxy_;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_;
    std::recursive_mutex mutex_;
    std::mutex topAbilityMutex_;
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_ABILITY_MANAGER_CLIENT_H
