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

#ifndef OHOS_ABILITY_RUNTIME_IABILITY_MANAGER_COLLABORATOR_H
#define OHOS_ABILITY_RUNTIME_IABILITY_MANAGER_COLLABORATOR_H

#include "ability_info.h"
#include "iremote_broker.h"
#include "iremote_object.h"
#include "mission_info.h"
#include "session_info.h"
#include "want.h"

namespace OHOS {
namespace AppExecFwk {
class Configuration;
}
}

namespace OHOS {
namespace AAFwk {
namespace {
constexpr int32_t CURRENT_USER_ID = -1;
}
class SessionInfo;

class IAbilityManagerCollaborator : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.aafwk.IAbilityManagerCollaborator");

    /**
     * @brief Notify collaborator to StartAbility.
     * @param AbilityInfo ability info from bms
     * @param userId userId.
     * @param want targert info, will modify by collaborator.
     * @param accessTokenIDEx accessToken
     * @return 0 when notify start ability success or else failed.
     */
    virtual int32_t NotifyStartAbility(const AppExecFwk::AbilityInfo &abilityInfo,
        int32_t userId, Want &want, uint64_t accessTokenIDEx, int32_t windowMode) = 0;

    /**
     * @brief Notify collaborator to app preload.
     * @param bundleName bundlName.
     * @return 0 means success or else failed.
     */
    virtual int32_t NotifyPreloadAbility(const std::string &bundleName)
    {
        return 0;
    }

    /**
     * @brief Notify when mission is created.
     * @param missionId missionId.
     * @param want target info.
     * @return 0 when notify mission created success or else failed.
     */
    virtual int32_t NotifyMissionCreated(int32_t missionId, const Want &want) = 0;

    /**
     * @brief Notify when mission is created.
     * @param sessionInfo sessionInfo.
     * @return 0 when notify mission created success or else failed.
     */
    virtual int32_t NotifyMissionCreated(const sptr<SessionInfo> &sessionInfo) = 0;

    /**
     * @brief Notify when start loading ability record.
     * @param AbilityInfo ability info from bms.
     * @param missionId missionId.
     * @param want target info.
     * @return 0 when notify load ability success or else failed.
    */
    virtual int32_t NotifyLoadAbility(
        const AppExecFwk::AbilityInfo &abilityInfo, int32_t missionId, const Want &want) = 0;

    /**
     * @brief Notify when start loading ability record.
     * @param AbilityInfo ability info from bms.
     * @param sessionInfo sessionInfo.
     * @return 0 when notify load ability success or else failed.
    */
    virtual int32_t NotifyLoadAbility(
        const AppExecFwk::AbilityInfo &abilityInfo, const sptr<SessionInfo> &sessionInfo) = 0;

    /**
     * @brief Notify when notify app to background.
     * @param missionId missionId.
     * @param userId, the user id.
     * @return 0 when notify move mission to background success or else failed.
     */
    virtual int32_t NotifyMoveMissionToBackground(int32_t missionId, int32_t userId = CURRENT_USER_ID) = 0;

    /**
     * @brief Notify when notify app to foreground.
     * @param missionId missionId.
     * @param userId, the user id.
     * @return 0 when notify move mission to foreground success or else failed.
     */
    virtual int32_t NotifyMoveMissionToForeground(int32_t missionId, int32_t userId = CURRENT_USER_ID) = 0;

    /**
     * @brief Notify when notify ability is terminated, but mission is not cleared.
     * @param missionId missionId.
     * @param userId, the user id.
     * @return 0 when notify terminate mission success or else failed.
     */
    virtual int32_t NotifyTerminateMission(int32_t missionId, int32_t userId = CURRENT_USER_ID) = 0;

    /**
     * @brief Notify to broker when clear mission.
     * @param missionId missionId.
     * @param userId, the user id.
     * @return 0 when notify clear mission success or else failed.
     */
    virtual int32_t NotifyClearMission(int32_t missionId, int32_t userId = CURRENT_USER_ID) = 0;

    /**
     * @brief Notify to broker when clear mission.
     * @param pid pid of shell process.
     * @param type died type.
     * @param reason addational message for died reason.
     * @return 0 when notify remove shell process success or else failed.
     */
    virtual int32_t NotifyRemoveShellProcess(int32_t pid, int32_t type, const std::string &reason) = 0;

    /**
     * @brief Update mission info to real element by broker.
     * @param sessionInfo sessionInfo.
     */
    virtual void UpdateMissionInfo(sptr<SessionInfo> &sessionInfo) = 0;

    /**
     * @brief Check the call permission from shell assistant.
     * @param want target info.
     */
    virtual int32_t CheckCallAbilityPermission(const Want &want)
    {
        return -1;
    }

    /**
     * @brief Notify application update system environment changes.
     * @param config System environment change parameters.
     * @param userId userId Designation User ID.
     * @return Return true to notify changes successfully, or false to failed.
     */
    virtual bool UpdateConfiguration(const AppExecFwk::Configuration &config, int32_t userId)
    {
        return true;
    }

    /**
     * @brief Open file by uri.
     * @param uri The file uri.
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @param tokenId caller tokenId.
     * @return int The file descriptor.
     */
    virtual int32_t OpenFile(const Uri& uri, uint32_t flag, uint32_t tokenId)
    {
        return -1;
    }

    /**
     * @brief grant uri permission
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @param targetTokenId the tokenId of target application.
     * @param targetBundleName the bundleName of target application.
     * @return 0 when on success or else failed.
     */
    virtual int32_t GrantUriPermission(const std::vector<std::string> &uriVec, uint32_t flag, uint32_t targetTokenId,
        const std::string &targetBundleName)
    {
        return -1;
    }
    
    /**
     * @brief grant uri permission
     * @param tokenId the tokenId of target application.
     * @return 0 when on success or else failed.
     */
    virtual int32_t RevokeUriPermission(uint32_t tokenId)
    {
        return -1;
    }

    /**
     * @brief verify uri permission
     * @param uriVec The uri list to verify permission.
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @param tokenId the tokenId of caller application.
     * @param checkResults The result of verify uri permission.
     * @return 0 when on success or else failed.
     */
    virtual int32_t VerifyUriPermission(const std::vector<std::string> &uriVec, uint32_t flag, uint32_t tokenId,
        std::vector<bool> &checkResults)
    {
        return -1;
    }

    virtual void NotifyMissionBindPid(int32_t missionId, int32_t pid, int32_t userId = CURRENT_USER_ID)
    {
        return;
    }

    virtual int32_t CheckStaticCfgPermission(const Want &want, bool isImplicit)
    {
        return 0;
    }

    /**
     * @brief Update caller if need.
     * @param want to start ability
     * @return 0 when update caller successfully or else failed.
     */
    virtual int32_t UpdateCallerIfNeed(Want &want)
    {
        return 0;
    }

    /**
     * @brief Update target if need.
     * @param want to start ability
     * @return 0 when update target successfully or else failed.
     */
    virtual int32_t UpdateTargetIfNeed(Want &want)
    {
        return 0;
    }

    virtual int32_t RemoveCallerIfNeed(Want &want)
    {
        return 0;
    }

    /**
     * @brief kill processes by bundleName.
     * @param bundleName the bundleName of processes to be killed.
     * @param userId the user id of processes to be killed.
     * @return 0 when on success or else failed.
     */
    virtual int32_t NotifyKillProcesses(const std::string &bundleName, int32_t userId)
    {
        return 0;
    }

    /**
     * @brief Notify collaborator start grantUriPermission.
     * @param uris The uri list to grant permission.
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @param userId the user id of target application.
     * @return 0 when on success or else failed.
     */
    virtual int32_t NotifyGrantUriPermissionStart(const std::vector<std::string> &uris, uint32_t flag, int32_t userId)
    {
        return 0;
    }

    /**
     * @brief Notify collaborator start grantUriPermission.
     * @param uris The uri list to grant permission.
     * @param flag Want::FLAG_AUTH_READ_URI_PERMISSION or Want::FLAG_AUTH_WRITE_URI_PERMISSION.
     * @param userId The user id of target application.
     * @param checkResults The result of check uri permission.
     * @return 0 when on success or else failed.
     */
    virtual int32_t NotifyGrantUriPermissionEnd(const std::vector<std::string> &uris, uint32_t flag, int32_t userId,
        const std::vector<bool> &checkResults)
    {
        return 0;
    }

    enum {
        NOTIFY_START_ABILITY = 1,
        NOTIFY_MISSION_CREATED,
        NOTIFY_LOAD_ABILITY,
        NOTIFY_MOVE_MISSION_TO_BACKGROUND,
        NOTIFY_MOVE_MISSION_TO_FOREGROUND,
        NOTIFY_TERMINATE_MISSION,
        NOTIFY_CLEAR_MISSION,
        NOTIFY_REMOVE_SHELL_PROCESS,
        NOTIFY_MISSION_CREATED_BY_SCB = 10,
        NOTIFY_LOAD_ABILITY_BY_SCB,
        UPDATE_MISSION_INFO_BY_SCB,
        NOTIFY_PRELOAD_ABILITY,
        CHECK_CALL_ABILITY_PERMISSION,
        UPDATE_CONFIGURATION,
        OPEN_FILE,
        NOTIFY_MISSION_BIND_PID,
        CHECK_STATIC_CFG_PERMISSION,
        UPDATE_CALLER_IF_NEED,
        UPDATE_TARGET_IF_NEED,
        NOTIFY_KILL_PROCESSES,
        GRANT_URI_PERMISSION,
        REVOKE_URI_PERMISSION,
        NOTIFY_GRANT_URI_PERMISSION_START,
        NOTIFY_GRANT_URI_PERMISSION_END,
        REMOVE_CALLER_IF_NEED,
        VERIFY_URI_PERMISSION,
    };
};
}  // namespace AAFwk
}  // namespace OHOS
#endif // OHOS_ABILITY_RUNTIME_IABILITY_MANAGER_COLLABORATOR_H