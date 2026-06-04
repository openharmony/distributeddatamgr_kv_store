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

#ifndef OHOS_ABILITY_RUNTIME_CONTEXT_IMPL_H
#define OHOS_ABILITY_RUNTIME_CONTEXT_IMPL_H

#include <memory>
#include <string>
#include "context.h"

namespace OHOS {
namespace AppExecFwk {
class Configuration;
class ApplicationInfo;
class HapModuleInfo;
struct RunningProcessInfo;
}

namespace Global {
namespace Resource {
class ResourceManager;
enum DeviceType : int32_t;
}
}

namespace AbilityRuntime {

class ContextImpl : public Context {
public:
    ContextImpl() = default;
    virtual ~ContextImpl() = default;

    std::string GetBundleName() const override;
    std::string GetBundleCodeDir() override;
    std::string GetCacheDir() override;
    bool IsUpdatingConfigurations() override;
    bool PrintDrawnCompleted() override;
    std::string GetTempDir() override;
    std::string GetFilesDir() override;
    std::string GetDatabaseDir() override;
    std::string GetPreferencesDir() override;
    std::string GetDistributedFilesDir() override;
    void SwitchArea(int mode) override;
    std::shared_ptr<Context> CreateModuleContext(const std::string &moduleName) override;
    std::shared_ptr<Context> CreateModuleContext(const std::string &bundleName, const std::string &moduleName) override;
    int GetArea() override;
    void SetResourceManager(const std::shared_ptr<Global::Resource::ResourceManager> &resourceManager);
    std::shared_ptr<Global::Resource::ResourceManager> GetResourceManager() const override;
    std::shared_ptr<Context> CreateBundleContext(const std::string &bundleName) override;
    ErrCode GetBundleManager();
    void SetApplicationInfo(const std::shared_ptr<AppExecFwk::ApplicationInfo> &info);
    std::shared_ptr<AppExecFwk::ApplicationInfo> GetApplicationInfo() const override;
    void SetParentContext(const std::shared_ptr<Context> &context);
    std::string GetBundleCodePath() const override;
    std::shared_ptr<AppExecFwk::HapModuleInfo> GetHapModuleInfo() const override;
    void InitHapModuleInfo(const std::shared_ptr<AppExecFwk::AbilityInfo> &abilityInfo);
    void InitHapModuleInfo(const AppExecFwk::HapModuleInfo &hapModuleInfo);
    void SetToken(const sptr<IRemoteObject> &token) override;
    sptr<IRemoteObject> GetToken() override;
    void SetConfiguration(const std::shared_ptr<AppExecFwk::Configuration> &config);
    std::shared_ptr<AppExecFwk::Configuration> GetConfiguration() const override;
    std::string GetBaseDir() const override;
    void KillProcessBySelf();
    int32_t GetProcessRunningInformation(AppExecFwk::RunningProcessInfo &info);
    Global::Resource::DeviceType GetDeviceType() const override;
    void InitResourceManager(const AppExecFwk::BundleInfo &bundleInfo,
        const std::shared_ptr<ContextImpl> &appContext, bool currentBundle = false, const std::string &moduleName = "");
    bool IsCreateBySystemApp() const;
    int GetCurrentAccountId() const;
    void SetFlags(int64_t flags);
    int GetCurrentActiveAccountId() const;
    void CreateDirIfNotExist(const std::string &dirPath, const mode_t &mode) const;
    void ChangeToLocalPath(const std::string &bundleName, const std::string &sourcDir, std::string &localPath);
    void CreateDirIfNotExistWithCheck(const std::string &dirPath, const mode_t &mode, bool checkExist = true);
    int32_t GetDatabaseDirWithCheck(bool checkExist, std::string &databaseDir);
    int32_t GetGroupDatabaseDirWithCheck(const std::string &groupId, bool checkExist, std::string &databaseDir);
    int32_t GetPreferencesDirWithCheck(bool checkExist, std::string &preferencesDir);
    int32_t GetGroupPreferencesDirWithCheck(const std::string &groupId, bool checkExist, std::string &preferencesDir);
    int32_t GetGroupDirWithCheck(const std::string &groupId, bool checkExist, std::string &groupDir);
    int32_t GetSystemDatabaseDir(const std::string &groupId, bool checkExist, std::string &databaseDir) override;
    int32_t GetSystemPreferencesDir(const std::string &groupId, bool checkExist, std::string &preferencesDir) override;
    std::string GetGroupDir(std::string groupId) override;

protected:
    sptr<IRemoteObject> token_;

private:
    std::shared_ptr<AppExecFwk::ApplicationInfo> applicationInfo_ = nullptr;
    std::shared_ptr<Context> parentContext_ = nullptr;
    std::shared_ptr<Global::Resource::ResourceManager> resourceManager_ = nullptr;
    std::shared_ptr<AppExecFwk::HapModuleInfo> hapModuleInfo_ = nullptr;
    std::shared_ptr<AppExecFwk::Configuration> config_ = nullptr;
};

#ifdef __cplusplus
extern "C" {
#endif

struct AbilityRuntime_Context {
    OHOS::AppExecFwk::ExtensionAbilityType type;
    std::weak_ptr<OHOS::AbilityRuntime::Context> context;
};

#ifdef __cplusplus
} // extern "C"
#endif

}

}

#endif