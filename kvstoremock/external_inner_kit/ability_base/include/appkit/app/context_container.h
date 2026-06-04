/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_CONTEXT_CONTAINER_H
#define OHOS_ABILITY_RUNTIME_CONTEXT_CONTAINER_H

#include "context_deal.h"

namespace OHOS {
namespace AppExecFwk {
class BundleMgrHelper;
class ContextContainer : public Context {
public:
    ContextContainer() = default;
    virtual ~ContextContainer() = default;

    /**
     * Attaches a Context object to the current ability.
     * Generally, this method is called after Ability is loaded to provide the application context for the current
     * ability.
     *
     * @param base Indicates a Context object.
     */
    void AttachBaseContext(const std::shared_ptr<ContextDeal> &base);

    /**
     * @brief Detach a attatched context.
     *
     */
    void DetachBaseContext();

    /**
     * Called when getting the ProcessInfo
     *
     * @return ProcessInfo
     */
    std::shared_ptr<ProcessInfo> GetProcessInfo() const override;

    /**
     * Called when setting the ProcessInfo
     *
     * @param info ProcessInfo instance
     */
    void SetProcessInfo(const std::shared_ptr<ProcessInfo> &info);

    /**
     * @brief Obtains information about the current application. The returned application information includes basic
     * information such as the application name and application permissions.
     *
     * @return Returns the ApplicationInfo for the current application.
     */
    std::shared_ptr<ApplicationInfo> GetApplicationInfo() const override;

    /**
     * @brief Obtains the Context object of the application.
     *
     * @return Returns the Context object of the application.
     */
    std::shared_ptr<Context> GetApplicationContext() const override;

    /**
     * @brief Obtains the path of the package containing the current ability. The returned path contains the resources,
     *  source code, and configuration files of a module.
     *
     * @return Returns the path of the package file.
     */
    virtual std::string GetBundleCodePath() override;

    /**
     * @brief Obtains information about the current ability.
     * The returned information includes the class name, bundle name, and other information about the current ability.
     *
     * @return Returns the AbilityInfo object for the current ability.
     */
    virtual const std::shared_ptr<AbilityInfo> GetAbilityInfo() override;

    /**
     * @brief Obtains the Context object of the application.
     *
     * @return Returns the Context object of the application.
     */
    std::shared_ptr<Context> GetContext() override;

    /**
     * @brief Obtains an BundleMgrHelper instance.
     * You can use this instance to obtain information about the application bundle.
     *
     * @return Returns an BundleMgrHelper instance.
     */
    std::shared_ptr<BundleMgrHelper> GetBundleManager() const override;

    /**
     * @brief Obtains a resource manager.
     *
     * @return Returns a ResourceManager object.
     */
    std::shared_ptr<Global::Resource::ResourceManager> GetResourceManager() const override;

    /**
     * @brief Obtains the local database path.
     * If the local database path does not exist, the system creates one and returns the created path.
     *
     * @return Returns the local database file.
     */
    std::string GetDatabaseDir() override;

    /**
     * @brief Obtains the absolute path where all private data files of this application are stored.
     *
     * @return Returns the absolute path storing all private data files of this application.
     */
    std::string GetDataDir() override;

    /**
     * @brief Obtains the directory for storing custom data files of the application.
     * You can use the returned File object to create and access files in this directory. The files
     * can be accessible only by the current application.
     *
     * @param name Indicates the name of the directory to retrieve. This directory is created as part
     * of your application data.
     * @param mode Indicates the file operating mode. The value can be 0 or a combination of MODE_PRIVATE.
     *
     * @return Returns a File object for the requested directory.
     */
    std::string GetDir(const std::string &name, int mode) override;

    /**
     * @brief Obtains the directory for storing files for the application on the device's internal storage.
     *
     * @return Returns the application file directory.
     */
    std::string GetFilesDir() override;

    /**
     * @brief Obtains the bundle name of the current ability.
     *
     * @return Returns the bundle name of the current ability.
     */
    std::string GetBundleName() const override;

    /**
     * @brief Obtains the path of the OHOS Ability Package (HAP} containing this ability.
     *
     * @return Returns the path of the HAP containing this ability.
     */
    std::string GetBundleResourcePath() override;

    /**
     * @brief Obtains an ability manager.
     * The ability manager provides information about running processes and memory usage of an application.
     *
     * @return Returns an IAbilityManager instance.
     */
    sptr<AAFwk::IAbilityManager> GetAbilityManager() override;

    /**
     * @brief Obtains the type of this application.
     *
     * @return Returns system if this application is a system application;
     * returns normal if it is released in OHOS AppGallery;
     * returns other if it is released by a third-party vendor;
     * returns an empty string if the query fails.
     */
    std::string GetAppType() override;

    /**
     * @brief Sets the pattern of this Context based on the specified pattern ID.
     *
     * @param patternId Indicates the resource ID of the pattern to set.
     */
    void SetPattern(int32_t patternId) override;

    /**
     * @brief Obtains the HapModuleInfo object of the application.
     *
     * @return Returns the HapModuleInfo object of the application.
     */
    std::shared_ptr<HapModuleInfo> GetHapModuleInfo() override;

    /**
     * @brief Obtains the name of the current process.
     *
     * @return Returns the current process name.
     */
    std::string GetProcessName() override;

    /**
     * @brief Creates a Context object for an application with the given bundle name.
     *
     * @param bundleName Indicates the bundle name of the application.
     * @param flag  Indicates the flag for creating a Context object. It can be 0, any of
     * the following values, or any combination of the following values: CONTEXT_IGNORE_SECURITY,
     * CONTEXT_INCLUDE_CODE, and CONTEXT_RESTRICTED. The value 0 indicates that there is no restriction
     * on creating contexts for applications.
     * @param accountId Indicates the account id.
     *
     * @return Returns a Context object created for the specified application.
     */
    std::shared_ptr<Context> CreateBundleContext(const std::string &bundleName, int flag,
        int accountId = DEFAULT_ACCOUNT_ID);

    /**
     * @brief Obtains information about the caller of this ability.
     *
     * @return Returns the caller information.
     */
    Uri GetCaller() override;

    /**
     * @brief SetUriString
     *
     * @param uri the uri to set.
     */
    void SetUriString(const std::string &uri);

    /**
     * @brief InitResourceManager
     *
     * @param bundleInfo  BundleInfo
     */
    void InitResourceManager(BundleInfo &bundleInfo, std::shared_ptr<ContextDeal> &deal);

    /**
     * @brief Get the string of this Context based on the specified resource ID.
     *
     * @param resId Indicates the resource ID of the string to get.
     *
     * @return Returns the string of this Context.
     */
    std::string GetString(int resId) override;

    /**
     * @brief Get the string array of this Context based on the specified resource ID.
     *
     * @param resId Indicates the resource ID of the string array to get.
     *
     * @return Returns the string array of this Context.
     */
    std::vector<std::string> GetStringArray(int resId) override;

    /**
     * @brief Get the integer array of this Context based on the specified resource ID.
     *
     * @param resId Indicates the resource ID of the integer array to get.
     *
     * @return Returns the integer array of this Context.
     */
    std::vector<int> GetIntArray(int resId) override;

    /**
     * @brief Obtains the theme of this Context.
     *
     * @return theme Returns the theme of this Context.
     */
    std::map<std::string, std::string> GetTheme() override;

    /**
     * @brief Sets the theme of this Context based on the specified theme ID.
     *
     * @param themeId Indicates the resource ID of the theme to set.
     */
    void SetTheme(int themeId) override;

    /**
     * @brief Obtains the pattern of this Context.
     *
     * @return getPattern in interface Context
     */
    std::map<std::string, std::string> GetPattern() override;

    /**
     * @brief Get the color of this Context based on the specified resource ID.
     *
     * @param resId Indicates the resource ID of the color to get.
     *
     * @return Returns the color value of this Context.
     */
    int GetColor(int resId) override;

    /**
     * @brief Obtains the theme id of this Context.
     *
     * @return int Returns the theme id of this Context.
     */
    int GetThemeId() override;

    /**
     * @brief Obtains the current display orientation of this ability.
     *
     * @return Returns the current display orientation.
     */
    int GetDisplayOrientation() override;

    /**
     * @brief Obtains the path storing the preference file of the application.
     *        If the preference file path does not exist, the system creates one and returns the created path.
     *
     * @return Returns the preference file path .
     */
    std::string GetPreferencesDir() override;

    /**
     * @brief Set color mode
     *
     * @param the value of color mode.
     */
    void SetColorMode(int mode) override;

    /**
     * @brief Obtains color mode.
     *
     * @return Returns the color mode value.
     */
    int GetColorMode() override;

    /**
     * @brief Obtains the unique ID of the mission containing this ability.
     *
     * @return Returns the unique mission ID.
     */
    int GetMissionId() override;

    /**
     * @brief Obtains the lifecycle state info.
     *
     * @return Returns the lifecycle state info.
     */
    AAFwk::LifeCycleStateInfo GetLifeCycleStateInfo() const;

    /**
     * @brief Set the LifeCycleStateInfo to the deal.
     *
     * @param info the info to set.
     */
    void SetLifeCycleStateInfo(const AAFwk::LifeCycleStateInfo &info);

private:
    void LoadResources(BundleInfo &bundleInfo, std::shared_ptr<Global::Resource::ResourceManager> &resourceManager,
        std::unique_ptr<Global::Resource::ResConfig> &resConfig, std::shared_ptr<ContextDeal> &deal);

    std::shared_ptr<ContextDeal> baseContext_ = nullptr;
    std::shared_ptr<ProcessInfo> processInfo_ = nullptr;
    AAFwk::LifeCycleStateInfo lifeCycleStateInfo_;
    std::string uriString_ = "";
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_CONTEXT_CONTAINER_H
