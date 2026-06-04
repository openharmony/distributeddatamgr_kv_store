/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_LAUNCHER_SERVICE_H
#define FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_LAUNCHER_SERVICE_H

#include <string>
#include <vector>
#include "appexecfwk_errors.h"
#include "bundle_mgr_interface.h"
#include "securec.h"
#include "system_ability_definition.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "bundle_monitor.h"
#include "bundle_status_callback_interface.h"
#include "launcher_ability_info.h"
#include "ability_info.h"
#include "application_info.h"
#include "want.h"
#include "app_log_wrapper.h"

namespace OHOS {
namespace AppExecFwk {
class LauncherService : public virtual RefBase {
public:
    using Want = OHOS::AAFwk::Want;
    LauncherService();
    virtual ~LauncherService();

    /**
     * @brief Registers a callback method for monitoring bundle installation, uninstallation, and update events.
     * @param callback Indicates the callback method to be called.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool RegisterCallback(const sptr<IBundleStatusCallback> &callback);
    /**
     * @brief Unregisters the callback method for monitoring bundle installation, uninstallation, and update events.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool UnRegisterCallback();
    /**
     * @brief Obtains launcher abilities based on the bundle name and user id.
     * @param bundleName Indicates the bundle name.
     * @param userId Indicates the user ID.
     * @param launcherAbilityInfos List of LauncherAbilityInfo objects if obtained;
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool GetAbilityList(
        const std::string &bundleName, const int userId, std::vector<LauncherAbilityInfo> &launcherAbilityInfos);
    /**
     * @brief Obtains information about the shortcuts of the application.
     * @param bundleName Indicates the bundle name of the application.
     * @param ShortcutInfo List of ShortcutInfo objects if obtained.
     * @return Returns true if the function is successfully called; returns false otherwise.
     */
    virtual ErrCode GetShortcutInfos(const std::string &bundleName, std::vector<ShortcutInfo> &shortcutInfo);
    /**
     * @brief Obtains information about the launcher ability of all application that appears on launcher.
     * @param userId Indicates the id for the user.
     * @param launcherAbilityInfo Indicates the obtained LauncherAbilityInfo object.
     * @return Returns true if the function is successfully called; returns false otherwise.
     */
    virtual bool GetAllLauncherAbilityInfos(int32_t userId, std::vector<LauncherAbilityInfo> &launcherAbilityInfos);
    /**
     * @brief Obtains information about the launcher ability of input bundleName and userId.
     * @param bundleName Indicates the bundleName of the application.
     * @param userId Indicates the id for the user.
     * @param launcherAbilityInfos Indicates the obtained vector of the input bundle.
     * @return Returns ERR code of the result.
     */
    ErrCode GetLauncherAbilityByBundleName(const std::string &bundleName, const int32_t userId,
        std::vector<LauncherAbilityInfo> &launcherAbilityInfos);
    /**
     * @brief Public interface obtains information about the launcher ability of input bundleName and userId.
     * @param bundleName Indicates the bundleName of the application.
     * @param userId Indicates the id for the user.
     * @param launcherAbilityInfos Indicates the obtained vector of the input bundle.
     * @return Returns ERR code of the result.
     */
    ErrCode GetLauncherAbilityInfoSync(const std::string &bundleName, const int32_t userId,
        std::vector<LauncherAbilityInfo> &launcherAbilityInfos);
    /**
     * @brief Obtains information about the launcher ability of input bundleName and userId.
     * @param userId Indicates the id for the user.
     * @param launcherAbilityInfos Indicates the obtained vector of all launcher bundle.
     * @return Returns ERR code of the result.
     */
    ErrCode GetAllLauncherAbility(const int32_t userId, std::vector<LauncherAbilityInfo> &launcherAbilityInfos);
    /**
     * @brief Obtains information about the shortcuts of the application.
     * @param bundleName Indicates the bundle name of the application.
     * @param ShortcutInfo List of ShortcutInfo objects if obtained.
     * @return Returns err code of result.
     */
    ErrCode GetShortcutInfoV9(const std::string &bundleName,
        std::vector<ShortcutInfo> &shortcutInfos, int32_t userId = Constants::UNSPECIFIED_USERID);

    /**
     * @brief Obtains information about the shortcuts of the application by appIndex.
     * @param bundleName Indicates the bundle name of the application.
     * @param ShortcutInfo List of ShortcutInfo objects if obtained.
     * @param appIndex Indicates the app index of clone applications.
     * @param userId Indicates the user ID.
     * @return Returns err code of result.
     */
    ErrCode GetShortcutInfoByAppIndex(const std::string &bundleName, const int32_t appIndex,
        std::vector<ShortcutInfo> &shortcutInfos);

private:
    void InitWant(Want &want, const std::string &bundleName);
    void ConvertAbilityToLauncherAbility(const AbilityInfo &ability, LauncherAbilityInfo &launcherAbility,
        const int32_t userId);

private:
    void Init();
    static OHOS::sptr<OHOS::AppExecFwk::IBundleMgr> GetBundleMgr();

    std::shared_ptr<BundleMonitor> bundleMonitor_ = nullptr;
    static OHOS::sptr<OHOS::AppExecFwk::IBundleMgr> bundleMgr_;
    static OHOS::sptr<IRemoteObject::DeathRecipient> deathRecipient_;
    static std::mutex bundleMgrMutex_;
    DISALLOW_COPY_AND_MOVE(LauncherService);

    class LauncherServiceDeathRecipient : public IRemoteObject::DeathRecipient {
        virtual void OnRemoteDied([[maybe_unused]] const wptr<IRemoteObject>& remote) override;
    };
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_LAUNCHER_SERVICE_H
