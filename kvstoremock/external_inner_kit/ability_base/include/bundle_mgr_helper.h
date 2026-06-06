/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_BUNDLE_MGR_HELPER_H
#define OHOS_ABILITY_RUNTIME_BUNDLE_MGR_HELPER_H

#include <singleton.h>

#include "bundle_mgr_interface.h"

namespace OHOS {
constexpr static int REPOLL_TIME_MICRO_SECONDS = 1000000;

namespace AppExecFwk {
using Want = OHOS::AAFwk::Want;

class BundleMgrHelper : public std::enable_shared_from_this<BundleMgrHelper> {
public:
    DISALLOW_COPY_AND_MOVE(BundleMgrHelper);
    void PreConnect();
    void ConnectTillSuccess();
    void SetBmsReady(bool bmsReady);
    ErrCode GetNameForUid(const int32_t uid, std::string &name);
    ErrCode GetNameAndIndexForUid(const int32_t uid, std::string &bundleName, int32_t &appIndex);
    bool GetBundleInfo(const std::string &bundleName, const BundleFlag flag, BundleInfo &bundleInfo, int32_t userId);
    ErrCode InstallSandboxApp(const std::string &bundleName, int32_t dlpType, int32_t userId, int32_t &appIndex);
    ErrCode UninstallSandboxApp(const std::string &bundleName, int32_t appIndex, int32_t userId);
    ErrCode GetUninstalledBundleInfo(const std::string bundleName, BundleInfo &bundleInfo);
    ErrCode GetSandboxBundleInfo(const std::string &bundleName, int32_t appIndex, int32_t userId, BundleInfo &info);
    ErrCode GetSandboxAbilityInfo(const Want &want, int32_t appIndex, int32_t flags, int32_t userId,
        AbilityInfo &abilityInfo);
    ErrCode GetSandboxExtAbilityInfos(const Want &want, int32_t appIndex, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos);
    ErrCode GetSandboxHapModuleInfo(const AbilityInfo &abilityInfo, int32_t appIndex, int32_t userId,
        HapModuleInfo &hapModuleInfo);
    bool GetBundleInfo(const std::string &bundleName, int32_t flags, BundleInfo &bundleInfo, int32_t userId);
    std::string GetAppIdByBundleName(const std::string &bundleName, const int32_t userId);
    bool GetHapModuleInfo(const AbilityInfo &abilityInfo, HapModuleInfo &hapModuleInfo);
    ErrCode GetPluginHapModuleInfo(const std::string &hostBundleName, const std::string &pluginBundleName,
        const std::string &pluginModuleName, const int32_t userId, HapModuleInfo &hapModuleInfo);
    std::string GetAbilityLabel(const std::string &bundleName, const std::string &abilityName);
    std::string GetAppType(const std::string &bundleName);
    ErrCode GetBaseSharedBundleInfos(
        const std::string &bundleName, std::vector<BaseSharedBundleInfo> &baseSharedBundleInfos,
        GetDependentBundleInfoFlag flag = GetDependentBundleInfoFlag::GET_APP_CROSS_HSP_BUNDLE_INFO);
    ErrCode GetBundleInfoForSelf(int32_t flags, BundleInfo &bundleInfo);
    ErrCode GetBundleInfoForSelfWithOutCache(int32_t flags, BundleInfo &bundleInfo);
    ErrCode GetDependentBundleInfo(const std::string &sharedBundleName, BundleInfo &sharedBundleInfo,
        GetDependentBundleInfoFlag flag = GetDependentBundleInfoFlag::GET_APP_CROSS_HSP_BUNDLE_INFO);
    bool GetGroupDir(const std::string &dataGroupId, std::string &dir);
    sptr<IOverlayManager> GetOverlayManagerProxy();
    bool QueryAbilityInfo(const Want &want, AbilityInfo &abilityInfo);
    bool QueryAbilityInfo(const Want &want, int32_t flags, int32_t userId, AbilityInfo &abilityInfo);
    bool GetBundleInfos(int32_t flags,
        std::vector<BundleInfo> &bundleInfos, int32_t userId = Constants::UNSPECIFIED_USERID);
    sptr<IQuickFixManager> GetQuickFixManagerProxy();
    bool ProcessPreload(const Want &want);
    sptr<IAppControlMgr> GetAppControlProxy();
    bool QueryExtensionAbilityInfos(const Want &want, const int32_t &flag, const int32_t &userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos);
    ErrCode GetBundleInfoV9(
        const std::string &bundleName, int32_t flags, BundleInfo &bundleInfo, int32_t userId);
    ErrCode GetBundleInfosV9(int32_t flags, std::vector<BundleInfo> &bundleInfos, int32_t userId);
    bool GetApplicationInfo(
        const std::string &appName, const ApplicationFlag flag, const int32_t userId, ApplicationInfo &appInfo);
    bool GetApplicationInfo(const std::string &appName, int32_t flags, int32_t userId, ApplicationInfo &appInfo);
    bool GetApplicationInfoWithAppIndex(const std::string &appName, int32_t appIndex,
        int32_t userId, ApplicationInfo &appInfo);
    ErrCode GetJsonProfile(ProfileType profileType, const std::string &bundleName,
        const std::string &moduleName, std::string &profile, int32_t userId = Constants::UNSPECIFIED_USERID);
    bool UnregisterBundleEventCallback(const sptr<IBundleEventCallback> &bundleEventCallback);
    bool QueryExtensionAbilityInfoByUri(
        const std::string &uri, int32_t userId, ExtensionAbilityInfo &extensionAbilityInfo);
    bool ImplicitQueryInfoByPriority(const Want &want, int32_t flags, int32_t userId,
        AbilityInfo &abilityInfo, ExtensionAbilityInfo &extensionInfo);
    bool QueryAbilityInfoByUri(const std::string &abilityUri, int32_t userId, AbilityInfo &abilityInfo);
    bool QueryAbilityInfo(const Want &want,
        int32_t flags, int32_t userId, AbilityInfo &abilityInfo, const sptr<IRemoteObject> &callBack);
    void UpgradeAtomicService(const Want &want, int32_t userId);
    bool ImplicitQueryInfos(const Want &want, int32_t flags, int32_t userId, bool withDefault,
        std::vector<AbilityInfo> &abilityInfos, std::vector<ExtensionAbilityInfo> &extensionInfos,
        bool &findDefaultApp);
    bool CleanBundleDataFiles(
        const std::string &bundleName, int32_t userId, int32_t appCloneIndex, int32_t callerUid);
    bool QueryDataGroupInfos(const std::string &bundleName, int32_t userId, std::vector<DataGroupInfo> &infos);
    bool RegisterBundleEventCallback(const sptr<IBundleEventCallback> &bundleEventCallback);
    bool GetBundleInfos(
        const BundleFlag flag, std::vector<BundleInfo> &bundleInfos, int32_t userId = Constants::UNSPECIFIED_USERID);
    bool GetHapModuleInfo(const AbilityInfo &abilityInfo, int32_t userId, HapModuleInfo &hapModuleInfo);
    bool QueryAppGalleryBundleName(std::string &bundleName);
    ErrCode GetUidByBundleName(const std::string &bundleName, int32_t userId, int32_t appCloneIndex);
    ErrCode QueryExtensionAbilityInfosOnlyWithTypeName(const std::string &extensionTypeName,
        const uint32_t flag, const int32_t userId, std::vector<ExtensionAbilityInfo> &extensionInfos);
    sptr<IDefaultApp> GetDefaultAppProxy();
    ErrCode GetLaunchWantForBundle(const std::string &bundleName, Want &want, int32_t userId);
    ErrCode QueryCloneAbilityInfo(const ElementName &element, int32_t flags, int32_t appCloneIndex,
        AbilityInfo &abilityInfo, int32_t userId);
    ErrCode GetCloneBundleInfo(const std::string &bundleName, int32_t flags, int32_t appCloneIndex,
        BundleInfo &bundleInfo, int32_t userId);
    ErrCode QueryCloneExtensionAbilityInfoWithAppIndex(const ElementName &element, int32_t flags, int32_t appCloneIndex,
        ExtensionAbilityInfo &extensionInfo, int32_t userId);
    ErrCode GetCloneAppIndexes(const std::string &bundleName, std::vector<int32_t> &appIndexes, int32_t userId);
    ErrCode GetSignatureInfoByBundleName(const std::string &bundleName, SignatureInfo &signatureInfo);
    std::string GetStringById(
        const std::string &bundleName, const std::string &moduleName, uint32_t resId, int32_t userId);
    std::string GetDataDir(const std::string &bundleName, const int32_t appIndex);
    ErrCode GetPluginInfosForSelf(std::vector<PluginBundleInfo> &pluginBundleInfos);
    ErrCode GetPluginAbilityInfo(const std::string &hostBundleName, const std::string &pluginBundleName,
        const std::string &pluginModuleName, const std::string &pluginAbilityName, int32_t userId,
        AbilityInfo &pluginAbilityInfo);
    ErrCode RegisterPluginEventCallback(sptr<IBundleEventCallback> pluginEventCallback);
    ErrCode UnregisterPluginEventCallback(sptr<IBundleEventCallback> pluginEventCallback);
    // for collaborator (along with normal)
    ErrCode GetCloneBundleInfoExt(const std::string &bundleName, uint32_t flags, int32_t appIndex,
        int32_t userId, BundleInfo &bundleInfo);
    ErrCode GetLauncherAbilityInfoSync(const std::string &bundleName, int32_t userId,
        std::vector<AbilityInfo> &abilityInfo);
    ErrCode GetPluginInfoForTarget(const std::string &hostBundleName, const std::string &pluginBundleName,
        int32_t userId, PluginBundleInfo &pluginBundleInfo);
    ErrCode GetTestRunnerTypeAndPath(const std::string &bundleName, const std::string &moduleName,
        ModuleTestRunner &testRunner);
    ErrCode GetPluginExtensionInfo(const std::string &hostBundleName, const Want &want,
        int32_t userId, ExtensionAbilityInfo &pluginExtensionInfo);
    /**
     * @brief Sets whether the bundle is first launch.
     * @param bundleName Indicates the bundle name.
     * @param userId Indicates the user id.
     * @param appIndex Indicates the app index, 0 for normal app, > 0 for clone app.
     * @param isBundleFirstLaunched Specifies whether the bundle is first launch.
     * @return Returns ERR_OK if successful; returns error code otherwise.
     */
    ErrCode SetBundleFirstLaunch(const std::string &bundleName, int32_t userId, int32_t appIndex,
        bool isBundleFirstLaunched);

private:
    sptr<IBundleMgr> Connect();
    sptr<IBundleMgr> Connect(bool checkBmsReady);
    sptr<IBundleInstaller> ConnectBundleInstaller();
    void OnDeath();
    std::string ParseBundleNameByAppId(const std::string &appId) const;

private:
    DECLARE_DELAYED_SINGLETON(BundleMgrHelper)
    bool bmsReady_ = true;
    sptr<IBundleMgr> bundleMgr_;
    sptr<IBundleInstaller> bundleInstaller_;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ = nullptr;
    std::mutex mutex_;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_BUNDLE_MGR_HELPER_H