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
#include "ability_local_record.h"
#include "extension.h"
#include "appkit/ability_runtime/context/context.h"
#include "appkit/ability_runtime/context/context_impl.h"
#include "appkit/ability_runtime/context/application_context.h"
#include "appkit/ability_runtime/extension_context.h"
#include "extension_module_loader.h"
namespace OHOS::AbilityRuntime {
const size_t Context::CONTEXT_TYPE_ID = 0;
const size_t ExtensionContext::CONTEXT_TYPE_ID = 0;
const size_t ApplicationContext::CONTEXT_TYPE_ID = 0;
std::shared_ptr<ApplicationContext> Context::GetApplicationContext() { return nullptr;}
void Extension::Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord> &record,
                     const std::shared_ptr<AppExecFwk::OHOSApplication> &application,
                     std::shared_ptr<AppExecFwk::AbilityHandler> &handler, const sptr<IRemoteObject> &token) {}
void Extension::SetLaunchWant(const Want &want){}
void Extension::SetLastRequestWant(const Want &want) {}
void Extension::OnStart(const Want &want) {}
sptr<IRemoteObject> Extension::OnConnect(const Want &want) { return sptr<IRemoteObject>(); }
void Extension::OnDisconnect(const Want &want) {}
void Extension::OnCommand(const Want &want, bool restart, int startId) {}
void Extension::OnStop() {}
void Extension::OnConfigurationUpdated(const AppExecFwk::Configuration &configuration) {}
void Extension::Dump(const std::vector<std::string> &params, std::vector<std::string> &info) {}
void Extension::SetCallingInfo(const CallingInfo &callingInfo)
{
    callingInfo_ = std::make_shared<CallingInfo>(callingInfo);
}
std::shared_ptr<CallingInfo> Extension::GetCallingInfo() { return callingInfo_; }
void Extension::OnMemoryLevel(int level) {}
std::string ContextImpl::GetBundleName() const { return "Test"; }
std::string ContextImpl::GetBundleCodeDir() { return ""; }
std::string ContextImpl::GetCacheDir() { return ""; }
bool ContextImpl::IsUpdatingConfigurations() { return false; }
bool ContextImpl::PrintDrawnCompleted() { return false; }
std::string ContextImpl::GetTempDir() { return ""; }
std::string ContextImpl::GetFilesDir() { return ""; }
std::string ContextImpl::GetDatabaseDir() { return ""; }
std::string ContextImpl::GetPreferencesDir() { return ""; }
std::string ContextImpl::GetDistributedFilesDir() { return ""; }
void ContextImpl::SwitchArea(int mode) {}
std::shared_ptr<Context> ContextImpl::CreateModuleContext(const std::string &moduleName) { return shared_from_this(); }
std::shared_ptr<Context> ContextImpl::CreateModuleContext(const std::string &bundleName, const std::string &moduleName)
{
    return shared_from_this();
}
int ContextImpl::GetArea() { return 0; }
void ContextImpl::SetResourceManager(const std::shared_ptr<Global::Resource::ResourceManager> &resourceManager) {}
std::shared_ptr<Global::Resource::ResourceManager> ContextImpl::GetResourceManager() const { return resourceManager_; }
std::shared_ptr<Context> ContextImpl::CreateBundleContext(const std::string &bundleName) { return shared_from_this(); }
//sptr<AppExecFwk::IBundleMgr> ContextImpl::GetBundleManager() const { return sptr<AppExecFwk::IBundleMgr>(); }
void ContextImpl::SetApplicationInfo(
    const std::shared_ptr<AppExecFwk::ApplicationInfo> &info) { applicationInfo_ = info; }
std::shared_ptr<AppExecFwk::ApplicationInfo> ContextImpl::GetApplicationInfo() const { return applicationInfo_; }
void ContextImpl::SetParentContext(const std::shared_ptr<Context> &context) { parentContext_ = context; }
std::string ContextImpl::GetBundleCodePath() const { return std::string(); }
std::shared_ptr<AppExecFwk::HapModuleInfo> ContextImpl::GetHapModuleInfo() const { return hapModuleInfo_; }
void ContextImpl::InitHapModuleInfo(const std::shared_ptr<AppExecFwk::AbilityInfo> &abilityInfo) {}
void ContextImpl::InitHapModuleInfo(const AppExecFwk::HapModuleInfo &hapModuleInfo) {}
void ContextImpl::SetToken(const sptr<IRemoteObject> &token) {}
sptr<IRemoteObject> ContextImpl::GetToken() { return token_; }
void ContextImpl::SetConfiguration(const std::shared_ptr<AppExecFwk::Configuration> &config) {}
std::shared_ptr<AppExecFwk::Configuration> ContextImpl::GetConfiguration() const { return config_; }
std::string ContextImpl::GetBaseDir() const { return ""; }

int32_t ContextImpl::GetSystemDatabaseDir(const std::string &groupId, bool checkExist, std::string &databaseDir)
{
    return 0;
}
int32_t ContextImpl::GetSystemPreferencesDir(const std::string &groupId, bool checkExist, std::string &preferencesDir)
{
    return 0;
}
std::string ContextImpl::GetGroupDir(std::string groupId)
{
    return std::string();
}
ErrCode ContextImpl::GetBundleManager()
{
    return 0;
}
void ContextImpl::KillProcessBySelf() {}
int32_t ContextImpl::GetProcessRunningInformation(AppExecFwk::RunningProcessInfo &info)
{
    return 0;
}
Global::Resource::DeviceType ContextImpl::GetDeviceType() const
{
    return Global::Resource::DEVICE_CAR;
}
void ContextImpl::InitResourceManager(const AppExecFwk::BundleInfo &bundleInfo,
    const std::shared_ptr<ContextImpl> &appContext, bool currentBundle, const std::string &moduleName)
{
}
bool ContextImpl::IsCreateBySystemApp() const
{
    return false;
}
int ContextImpl::GetCurrentAccountId() const
{
    return 0;
}
void ContextImpl::SetFlags(int64_t flags) {}
int ContextImpl::GetCurrentActiveAccountId() const
{
    return 0;
}
void ContextImpl::CreateDirIfNotExist(const std::string &dirPath, const mode_t &mode) const {}
void ContextImpl::ChangeToLocalPath(const std::string &bundleName, const std::string &sourcDir, std::string &localPath)
{
}
void ContextImpl::CreateDirIfNotExistWithCheck(const std::string &dirPath, const mode_t &mode, bool checkExist) {}
int32_t ContextImpl::GetDatabaseDirWithCheck(bool checkExist, std::string &databaseDir)
{
    return 0;
}
int32_t ContextImpl::GetGroupDatabaseDirWithCheck(const std::string &groupId, bool checkExist, std::string &databaseDir)
{
    return 0;
}
int32_t ContextImpl::GetPreferencesDirWithCheck(bool checkExist, std::string &preferencesDir)
{
    return 0;
}
int32_t ContextImpl::GetGroupPreferencesDirWithCheck(const std::string &groupId, bool checkExist,
    std::string &preferencesDir)
{
    return 0;
}
int32_t ContextImpl::GetGroupDirWithCheck(const std::string &groupId, bool checkExist, std::string &groupDir)
{
    return 0;
}

std::string ApplicationContext::GetBundleName() const
{
    return std::string();
}
void ApplicationContext::RegisterAbilityLifecycleCallback(
    const std::shared_ptr<AbilityLifecycleCallback> &abilityLifecycleCallback)
{
}
void ApplicationContext::UnregisterAbilityLifecycleCallback(
    const std::shared_ptr<AbilityLifecycleCallback> &abilityLifecycleCallback)
{
}
void ApplicationContext::RegisterEnvironmentCallback(const std::shared_ptr<EnvironmentCallback> &environmentCallback)
{
}
void ApplicationContext::UnregisterEnvironmentCallback(const std::shared_ptr<EnvironmentCallback> &environmentCallback)
{
}
void ApplicationContext::DispatchOnAbilityCreate(const AbilityLifecycleCallbackArgs &ability) {}
void ApplicationContext::DispatchOnAbilityCreate(std::shared_ptr<InteropObject> ability) {}
void ApplicationContext::DispatchOnWindowStageCreate(const AbilityLifecycleCallbackArgs &ability,
    const AbilityLifecycleCallbackArgs &windowStage) {}
void ApplicationContext::DispatchOnWindowStageCreate(std::shared_ptr<InteropObject> ability,
    std::shared_ptr<InteropObject> windowStage) {}
void ApplicationContext::DispatchOnWindowStageDestroy(const AbilityLifecycleCallbackArgs &ability,
    const AbilityLifecycleCallbackArgs &windowStage) {}
void ApplicationContext::DispatchOnWindowStageDestroy(std::shared_ptr<InteropObject> ability,
    std::shared_ptr<InteropObject> windowStage) {}
void ApplicationContext::DispatchOnAbilityDestroy(const AbilityLifecycleCallbackArgs &ability) {}
void ApplicationContext::DispatchOnAbilityDestroy(std::shared_ptr<InteropObject> ability) {}
void ApplicationContext::DispatchOnAbilityForeground(const AbilityLifecycleCallbackArgs &ability) {}
void ApplicationContext::DispatchOnAbilityForeground(std::shared_ptr<InteropObject> ability) {}
void ApplicationContext::DispatchOnAbilityBackground(const AbilityLifecycleCallbackArgs &ability) {}
void ApplicationContext::DispatchOnAbilityBackground(std::shared_ptr<InteropObject> ability) {}
void ApplicationContext::DispatchOnAbilityContinue(const AbilityLifecycleCallbackArgs &ability) {}
void ApplicationContext::DispatchConfigurationUpdated(const AppExecFwk::Configuration &config) {}
std::shared_ptr<Context> ApplicationContext::CreateBundleContext(const std::string &bundleName)
{
    return std::shared_ptr<Context>();
}
std::shared_ptr<Context> ApplicationContext::CreateModuleContext(const std::string &moduleName)
{
    return std::shared_ptr<Context>();
}
std::shared_ptr<Context> ApplicationContext::CreateModuleContext(const std::string &bundleName,
    const std::string &moduleName)
{
    return std::shared_ptr<Context>();
}
std::shared_ptr<AppExecFwk::ApplicationInfo> ApplicationContext::GetApplicationInfo() const
{
    return std::shared_ptr<AppExecFwk::ApplicationInfo>();
}
std::shared_ptr<Global::Resource::ResourceManager> ApplicationContext::GetResourceManager() const
{
    return std::shared_ptr<Global::Resource::ResourceManager>();
}
std::string ApplicationContext::GetBundleCodePath() const
{
    return std::string();
}
std::shared_ptr<AppExecFwk::HapModuleInfo> ApplicationContext::GetHapModuleInfo() const
{
    return std::shared_ptr<AppExecFwk::HapModuleInfo>();
}
std::string ApplicationContext::GetBundleCodeDir()
{
    return std::string();
}
std::string ApplicationContext::GetCacheDir()
{
    return std::string();
}
std::string ApplicationContext::GetTempDir()
{
    return std::string();
}
std::string ApplicationContext::GetFilesDir()
{
    return std::string();
}
bool ApplicationContext::IsUpdatingConfigurations()
{
    return false;
}
bool ApplicationContext::PrintDrawnCompleted()
{
    return false;
}
std::string ApplicationContext::GetDatabaseDir()
{
    return std::string();
}
std::string ApplicationContext::GetDistributedFilesDir()
{
    return std::string();
}
sptr<IRemoteObject> ApplicationContext::GetToken()
{
    return sptr<IRemoteObject>();
}
void ApplicationContext::SetToken(const sptr<IRemoteObject> &token) {}
void ApplicationContext::SwitchArea(int mode) {}
int ApplicationContext::GetArea()
{
    return 0;
}
std::shared_ptr<AppExecFwk::Configuration> ApplicationContext::GetConfiguration() const
{
    return std::shared_ptr<AppExecFwk::Configuration>();
}
std::string ApplicationContext::GetBaseDir() const
{
    return std::string();
}
std::string ApplicationContext::GetResourceDir(const std::string &moduleName)
{
    return std::string();
}
int32_t ApplicationContext::GetSystemDatabaseDir(const std::string &groupId, bool checkExist, std::string &databaseDir)
{
    return 0;
}
int32_t ApplicationContext::GetSystemPreferencesDir(const std::string &groupId, bool checkExist, std::string &preferencesDir)
{
    return 0;
}
std::string ApplicationContext::GetGroupDir(std::string groupId)
{
    return std::string();
}
std::string ApplicationContext::GetCloudFileDir()
{
    return std::string();
}
std::string ApplicationContext::GetLogFileDir()
{
    return std::string();
}
std::shared_ptr<Global::Resource::ResourceManager> ApplicationContext::CreateModuleResourceManager(
    const std::string &bundleName, const std::string &moduleName)
{
    return nullptr;
}
int32_t ApplicationContext::CreateSystemHspModuleResourceManager(const std::string &bundleName,
    const std::string &moduleName, std::shared_ptr<Global::Resource::ResourceManager> &resourceManager)
{
    return 0;
}
std::string ApplicationContext::GetProcessName()
{
    return std::string();
}
Global::Resource::DeviceType ApplicationContext::GetDeviceType() const
{
    return Global::Resource::DEVICE_CAR;
}
std::shared_ptr<Context> ApplicationContext::CreateAreaModeContext(int areaMode)
{
    return nullptr;
}
std::shared_ptr<Context> ApplicationContext::CreateModuleOrPluginContext(const std::string &bundleName,
    const std::string &moduleName)
{
    return nullptr;
}
ConfigUpdateReason ApplicationContext::GetConfigUpdateReason()
{
    return ConfigUpdateReason::CONFIG_UPDATE_REASON_DEFAULT;
}
void ApplicationContext::InitApplicationContext() {}
void ApplicationContext::AttachContextImpl(const std::shared_ptr<ContextImpl> &contextImpl) {}
std::string ApplicationContext::GetPreferencesDir()
{
    return std::string();
}
std::shared_ptr<AppExecFwk::AbilityInfo> ExtensionContext::GetAbilityInfo() const { return abilityInfo_; }
void ExtensionContext::SetAbilityInfo(const std::shared_ptr<OHOS::AppExecFwk::AbilityInfo> &abilityInfo) {}
ExtensionModuleLoader& ExtensionModuleLoader::GetLoader(const char* sharedLibrary) {
    class InnerLoader : public ExtensionModuleLoader {
    public:
        Extension *Create(const std::unique_ptr<Runtime> &runtime) const override
        {
            return nullptr;
        }
    };
    static InnerLoader instance;
    return instance;
}
}
