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

#ifndef OHOS_ABILITY_RUNTIME_APPLICATION_CONTEXT_H
#define OHOS_ABILITY_RUNTIME_APPLICATION_CONTEXT_H

#include <vector>
#include <shared_mutex>
#include <unordered_map>
#include <mutex>

#include "context.h"
#include "context_impl.h"
#include "ability_native_thread.h"
#include "interop_object.h"

namespace OHOS {
namespace AAFwk {
class Want;
struct ExitReason;
}
namespace AppExecFwk {
struct RunningProcessInfo;
}
namespace AbilityRuntime {
class AbilityLifecycleCallback;
class AbilityLifecycleCallbackArgs;
class ApplicationStateChangeCallback;
class ApplicationUpdateCallback;
class EnvironmentCallback;
class InteropAbilityLifecycleCallback;
class SystemConfigurationUpdatedCallback;
class ContextImpl;
using AppConfigUpdateCallback = std::function<void(const AppExecFwk::Configuration &config)>;
using AppProcessExitCallback = std::function<void(const AAFwk::ExitReason &exitReason)>;
using AppGetSpecifiedRuntimeCallback = std::function<const std::unique_ptr<Runtime>&(const std::string &)>;
class ApplicationContext : public Context {
public:
    ApplicationContext() = default;
    ~ApplicationContext() = default;
    void RegisterAbilityLifecycleCallback(const std::shared_ptr<AbilityLifecycleCallback> &abilityLifecycleCallback);
    void UnregisterAbilityLifecycleCallback(const std::shared_ptr<AbilityLifecycleCallback> &abilityLifecycleCallback);
    void RegisterInteropAbilityLifecycleCallback(std::shared_ptr<InteropAbilityLifecycleCallback> callback);
    void UnregisterInteropAbilityLifecycleCallback(std::shared_ptr<InteropAbilityLifecycleCallback> callback);
    bool IsAbilityLifecycleCallbackEmpty();
    bool IsInteropAbilityLifecycleCallbackEmpty();
    void RegisterEnvironmentCallback(const std::shared_ptr<EnvironmentCallback> &environmentCallback);
    void UnregisterEnvironmentCallback(const std::shared_ptr<EnvironmentCallback> &environmentCallback);
    void RegisterApplicationStateChangeCallback(
        const std::weak_ptr<ApplicationStateChangeCallback> &applicationStateChangeCallback);
    void RegisterApplicationUpdateCallback(
        const std::weak_ptr<ApplicationUpdateCallback> &applicationUpdateCallback);
    void DispatchOnAbilityCreate(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilityCreate(std::shared_ptr<InteropObject> ability);
    void DispatchOnWindowStageCreate(const AbilityLifecycleCallbackArgs &ability,
        const AbilityLifecycleCallbackArgs &windowStage);
    void DispatchOnWindowStageCreate(std::shared_ptr<InteropObject> ability,
        std::shared_ptr<InteropObject> windowStage);
    void DispatchOnWindowStageDestroy(const AbilityLifecycleCallbackArgs &ability,
        const AbilityLifecycleCallbackArgs &windowStage);
    void DispatchOnWindowStageDestroy(std::shared_ptr<InteropObject> ability,
        std::shared_ptr<InteropObject> windowStage);
    void DispatchWindowStageFocus(const AbilityLifecycleCallbackArgs &ability,
        const AbilityLifecycleCallbackArgs &windowStage);
    void DispatchWindowStageUnfocus(const AbilityLifecycleCallbackArgs &ability,
        const AbilityLifecycleCallbackArgs &windowStage);
    void DispatchOnAbilityDestroy(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilityDestroy(std::shared_ptr<InteropObject> ability);
    void DispatchOnAbilityForeground(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilityForeground(std::shared_ptr<InteropObject> ability);
    void DispatchOnAbilityBackground(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilityBackground(std::shared_ptr<InteropObject> ability);
    void DispatchOnAbilityContinue(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilityWillContinue(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnWindowStageWillRestore(const AbilityLifecycleCallbackArgs &ability,
        const AbilityLifecycleCallbackArgs &windowStage);
    void DispatchOnWindowStageRestore(const AbilityLifecycleCallbackArgs &ability,
        const AbilityLifecycleCallbackArgs &windowStage);
    void DispatchOnAbilityWillSaveState(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilitySaveState(const AbilityLifecycleCallbackArgs &ability);
    void DispatchConfigurationUpdated(const AppExecFwk::Configuration &config);
    void DispatchMemoryLevel(const int level);
    void NotifyApplicationForeground();
    void NotifyApplicationBackground();
    void NotifySystemConfigurationUpdated(const AppExecFwk::Configuration& configuration);
    void RegisterSystemConfigurationUpdatedCallback(const std::weak_ptr<SystemConfigurationUpdatedCallback> &Callback);
    void DispatchOnWillNewWant(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnNewWant(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilityWillCreate(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnWindowStageWillCreate(const AbilityLifecycleCallbackArgs &ability,
        const AbilityLifecycleCallbackArgs &windowStage);
    void DispatchOnWindowStageWillDestroy(const AbilityLifecycleCallbackArgs &ability,
        const AbilityLifecycleCallbackArgs &windowStage);
    void DispatchOnAbilityWillDestroy(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilityWillForeground(const AbilityLifecycleCallbackArgs &ability);
    void DispatchOnAbilityWillBackground(const AbilityLifecycleCallbackArgs &ability);

    // Native Module related methods
    bool CreateNativeThread(const AAFwk::NativeAbilityMetaData &metaData, const std::string &bundleName,
        const std::string &moduleName);
    std::shared_ptr<AppExecFwk::AbilityNativeThread> GetNativeThread();
    void AddNativeAbility(const std::string &instanceId, std::shared_ptr<AbilityRuntime_NativeAbilityWrapper> wrapper);
    std::shared_ptr<AbilityRuntime_NativeAbilityWrapper> GetNativeAbility(const std::string &instanceId);
    void RemoveNativeAbility(const std::string &instanceId);
    void PostAbility(const std::string &instanceId, std::shared_ptr<AbilityRuntime_NativeAbilityWrapper> wrapper);
    void DestroyAbility(const std::string &instanceId);
    void NotifyProcessExit();

    std::string GetBundleName() const override;
    std::shared_ptr<Context> CreateBundleContext(const std::string &bundleName) override;
    std::shared_ptr<Context> CreateModuleContext(const std::string &moduleName) override;
    std::shared_ptr<Context> CreateModuleContext(const std::string &bundleName, const std::string &moduleName) override;
    std::shared_ptr<Global::Resource::ResourceManager> CreateModuleResourceManager(
        const std::string &bundleName, const std::string &moduleName) override;
    int32_t CreateSystemHspModuleResourceManager(const std::string &bundleName,
        const std::string &moduleName, std::shared_ptr<Global::Resource::ResourceManager> &resourceManager) override;
    std::shared_ptr<Context> CreateAreaModeContext(int areaMode) override;
    std::shared_ptr<Context> CreateModuleOrPluginContext(const std::string &bundleName,
        const std::string &moduleName) override;
#ifdef SUPPORT_GRAPHICS
    std::shared_ptr<Context> CreateDisplayContext(uint64_t displayId) override;
#endif
    std::shared_ptr<AppExecFwk::ApplicationInfo> GetApplicationInfo() const override;
    void SetApplicationInfo(const std::shared_ptr<AppExecFwk::ApplicationInfo> &info);
    std::shared_ptr<Global::Resource::ResourceManager> GetResourceManager() const override;
    std::string GetBundleCodePath() const override;
    std::string GetBundleCodeDir() override;
    std::string GetCacheDir() override;
    std::string GetTempDir() override;
    std::string GetResourceDir(const std::string &moduleName = "") override;
    bool IsModuleExist(const std::string &moduleName);
    void GetAllTempBase(std::vector<std::string> &tempPaths);
    std::string GetFilesDir() override;
    bool IsUpdatingConfigurations() override;
    bool PrintDrawnCompleted() override;
    std::string GetDatabaseDir() override;
    std::string GetPreferencesDir() override;
    int32_t GetSystemDatabaseDir(const std::string &groupId, bool checkExist, std::string &databaseDir) override;
    int32_t GetSystemPreferencesDir(const std::string &groupId, bool checkExist, std::string &preferencesDir) override;
    std::string GetGroupDir(std::string groupId) override;
    std::string GetDistributedFilesDir() override;
    std::string GetCloudFileDir() override;
    std::string GetLogFileDir() override;
    sptr<IRemoteObject> GetToken() override;
    void SetToken(const sptr<IRemoteObject> &token) override;
    void SwitchArea(int mode) override;
    void SetColorMode(int32_t colorMode);
    void SetLanguage(const std::string &language);
    void SetFont(const std::string &font);
    bool SetFontSizeScale(double fontSizeScale);
    void SetMcc(const std::string &mcc);
    void SetMnc(const std::string &mnc);
    void ClearUpApplicationData();
    int GetArea() override;
    std::string GetProcessName() override;
    std::shared_ptr<AppExecFwk::Configuration> GetConfiguration() const override;
    void SetConfiguration(const std::shared_ptr<AppExecFwk::Configuration> &config);
    void AppHasDarkRes(bool &darkRes);
    std::string GetBaseDir() const override;
    Global::Resource::DeviceType GetDeviceType() const override;
    void KillProcessBySelf(const bool clearPageStack = false);
    int32_t GetProcessRunningInformation(AppExecFwk::RunningProcessInfo &info);
    int32_t RestartApp(const AAFwk::Want& want);
    int32_t EnableDelayedProcessExit();
    int32_t DisableDelayedProcessExit();
    int32_t StartSelfUIAbility(const AAFwk::Want &want);

    void AttachContextImpl(const std::shared_ptr<ContextImpl> &contextImpl);
    void InitApplicationContext();

    static std::shared_ptr<ApplicationContext> GetInstance();

    // unused
    std::shared_ptr<AppExecFwk::HapModuleInfo> GetHapModuleInfo() const override;

    bool GetApplicationInfoUpdateFlag() const;
    void SetApplicationInfoUpdateFlag(bool flag);
    void RegisterAppConfigUpdateObserver(AppConfigUpdateCallback appConfigChangeCallback);
    void RegisterAppFontObserver(AppConfigUpdateCallback appFontCallback);
    void RegisterProcessSecurityExit(AppProcessExitCallback appProcessExitCallback);
    void RegisterAppGetSpecifiedRuntime(AppGetSpecifiedRuntimeCallback appGetSpecifiedRuntimeCallback);

#ifdef SUPPORT_SCREEN
    /**
     * @brief Query all UIAbilities of the current process.
     * @param uIAbilities Output parameters, return UIAbilities list.
     */
    void GetAllUIAbilities(std::vector<std::shared_ptr<UIAbility>> &uiAbility);

    /**
     * @brief Register the callback function to get all UIAbilities of the current process.
     * @param getAllUIAbilitiesCallback The registered callback function.
     */
    void RegisterGetAllUIAbilitiesCallback(GetAllUIAbilitiesCallback getAllUIAbilitiesCallback);
#endif

#ifdef SUPPORT_GRAPHICS
    void RegisterGetDisplayConfig(GetDisplayConfigCallback getDisplayConfigCallback);
#endif

    std::string GetAppRunningUniqueId() const;
    void SetAppRunningUniqueId(const std::string &appRunningUniqueId);
    int32_t SetSupportedProcessCacheSelf(bool isSupport);
    int32_t GetCurrentAppCloneIndex();
    void SetCurrentAppCloneIndex(int32_t appIndex);
    std::string GetCurrentInstanceKey();
    void SetCurrentInstanceKey(const std::string& instanceKey);
    int32_t GetAllRunningInstanceKeys(std::vector<std::string> &instanceKeys);
    int32_t GetCurrentAppMode();
    void SetCurrentAppMode(int32_t appIndex);
    void ProcessSecurityExit(const AAFwk::ExitReason &exitReason);
    napi_env GetMainNapiEnv() const;

    int32_t GetImageProcessType() const;
    bool IsAbilityCreated() const;

    using SelfType = ApplicationContext;
    static const size_t CONTEXT_TYPE_ID;
    std::string GetDataDir();
    void SetLaunchParameter(const AAFwk::Want &want);
    void SetLatestParameter(const AAFwk::Want &want);
    std::string GetLaunchParameter() const;
    std::string GetLatestParameter() const;
    ConfigUpdateReason GetConfigUpdateReason() override;
    void SetConfigUpdateReason(ConfigUpdateReason reason);

protected:
    bool IsContext(size_t contextTypeId) override
    {
        return contextTypeId == CONTEXT_TYPE_ID || Context::IsContext(contextTypeId);
    }

private:
    bool IsDelayedProcessExitPending();
    std::vector<std::shared_ptr<InteropAbilityLifecycleCallback>> GetInteropCallbacks();

private:
    std::shared_ptr<ContextImpl> contextImpl_;
    static std::vector<std::shared_ptr<AbilityLifecycleCallback>> callbacks_;
    static std::vector<std::shared_ptr<InteropAbilityLifecycleCallback>> interopCallbacks_;
    static std::vector<std::shared_ptr<EnvironmentCallback>> envCallbacks_;
    static std::vector<std::weak_ptr<ApplicationStateChangeCallback>> applicationStateCallback_;
    static std::vector<std::weak_ptr<SystemConfigurationUpdatedCallback>> systemConfigurationUpdatedCallbacks_;
    std::recursive_mutex callbackLock_;
    std::mutex interopCallbackLock_;
    std::recursive_mutex envCallbacksLock_;
    std::recursive_mutex applicationStateCallbackLock_;
    bool applicationInfoUpdateFlag_ = false;
    AppConfigUpdateCallback appConfigChangeCallback_ = nullptr;
    AppConfigUpdateCallback appFontCallback_ = nullptr;
    AppProcessExitCallback appProcessExitCallback_ = nullptr;
    AppGetSpecifiedRuntimeCallback appGetSpecifiedRuntimeCallback_ = nullptr;
    std::string appRunningUniqueId_;
    int32_t appIndex_ = 0;
    int32_t appMode_ = 0;
    std::string instanceKey_;
    std::string dataDir_;
    std::mutex dataDirMutex_;
    std::mutex systemConfigurationUpdatedCallbackLock_;

    // Native Module related members
    std::shared_ptr<AppExecFwk::AbilityNativeThread> abilityNativeThread_;
    std::unordered_map<std::string, std::shared_ptr<AbilityRuntime_NativeAbilityWrapper>> nativeAbilities_;
    std::mutex nativeMutex_;
    std::mutex delayedProcessExitStateLock_;
    bool delayedProcessExitEnabled_ = false;
};
}  // namespace AbilityRuntime
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_APPLICATION_CONTEXT_H
