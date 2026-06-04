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

#ifndef OHOS_ABILITY_RUNTIME_CONTEXT_H
#define OHOS_ABILITY_RUNTIME_CONTEXT_H

#include <memory>

#include "ability_info.h"
#include "ability_manager_interface.h"
#include "bundle_mgr_interface.h"
#include "dummy_hap_module_info.h"
#include "hap_module_info.h"
#include "process_info.h"
#include "resource_manager.h"
#include "task/task_priority.h"

namespace OHOS {
namespace DataShare {
class DataShareHelper;
}
namespace AppExecFwk {
class BundleMgrHelper;
using Want = OHOS::AAFwk::Want;
using AbilityStartSetting = AAFwk::AbilityStartSetting;
using PermissionRequestTask = std::function<void(const std::vector<std::string>&, const std::vector<int>&)>;
// Request permissions for user
#define OHOS_REQUEST_PERMISSION_BUNDLENAME "com.ohos.systemui"
#define OHOS_REQUEST_PERMISSION_ABILITY_NAME "com.ohos.systemui.systemdialog.MainAbility"

#define OHOS_REQUEST_PERMISSION_KEY "OHOS_REQUEST_PERMISSION_KEY"
#define OHOS_REQUEST_PERMISSIONS_LIST "OHOS_REQUEST_PERMISSIONS_LIST"
#define OHOS_REQUEST_PERMISSIONS_DES_LIST "OHOS_REQUEST_PERMISSIONS_DES_LIST"
#define OHOS_REQUEST_CALLER_BUNDLERNAME "OHOS_REQUEST_CALLER_BUNDLERNAME"

#define OHOS_RESULT_PERMISSION_KEY "OHOS_RESULT_PERMISSION_KEY"
#define OHOS_RESULT_PERMISSIONS_LIST "OHOS_RESULT_PERMISSIONS_LIST"
#define OHOS_RESULT_PERMISSIONS_LIST_YES "OHOS_RESULT_PERMISSIONS_LIST_YES"
#define OHOS_RESULT_PERMISSIONS_LIST_NO "OHOS_RESULT_PERMISSIONS_LIST_NO"
#define OHOS_RESULT_CALLER_BUNDLERNAME "OHOS_RESULT_CALLER_BUNDLERNAME"

#define OHOS_REQUEST_PERMISSION_VALUE 1

constexpr int INVALID_RESOURCE_VALUE = -1;  // GetColor() Failed return Value
constexpr int DEFAULT_ACCOUNT_ID = -1;

class DataAbilityHelperImpl;
class ContinuationConnector;
class IAbilityManager;
class Context {
public:
    Context() = default;
    virtual ~Context() = default;

    /**
     * Called when getting the ProcessInfo
     *
     * @return ProcessInfo
     */
    virtual std::shared_ptr<ProcessInfo> GetProcessInfo() const = 0;

    /**
     * @brief Obtains information about the current application. The returned application information includes basic
     * information such as the application name and application permissions.
     *
     * @return Returns the ApplicationInfo for the current application.
     */
    virtual std::shared_ptr<ApplicationInfo> GetApplicationInfo() const = 0;

    /**
     * @brief Obtains the Context object of the application.
     *
     * @return Returns the Context object of the application.
     */
    virtual std::shared_ptr<Context> GetApplicationContext() const = 0;

    /**
     * @brief Obtains the path of the package containing the current ability. The returned path contains the resources,
     *  source code, and configuration files of a module.
     *
     * @return Returns the path of the package file.
     */
    virtual std::string GetBundleCodePath() = 0;

    /**
     * @brief Obtains information about the current ability.
     * The returned information includes the class name, bundle name, and other information about the current ability.
     *
     * @return Returns the AbilityInfo object for the current ability.
     */
    virtual const std::shared_ptr<AbilityInfo> GetAbilityInfo() = 0;

    /**
     * @brief Obtains the Context object of the application.
     *
     * @return Returns the Context object of the application.
     */
    virtual std::shared_ptr<Context> GetContext() = 0;

    /**
     * @brief Obtains an BundleMgrHelper instance.
     * You can use this instance to obtain information about the application bundle.
     *
     * @return Returns an BundleMgrHelper instance.
     */
    virtual std::shared_ptr<BundleMgrHelper> GetBundleManager() const = 0;

    /**
     * @brief Obtains a resource manager.
     *
     * @return Returns a ResourceManager object.
     */
    virtual std::shared_ptr<Global::Resource::ResourceManager> GetResourceManager() const = 0;

    /**
     * @brief Destroys another ability that uses the AbilityInfo.AbilityType.SERVICE template.
     * The current ability using either the AbilityInfo.AbilityType.SERVICE or AbilityInfo.AbilityType.PAGE
     * template can call this method to destroy another ability that uses the AbilityInfo.AbilityType.SERVICE
     * template. The current ability itself can be destroyed by calling the terminateAbility() method.
     *
     * @param want Indicates the Want containing information about the ability to destroy.
     *
     * @return Returns true if the ability is destroyed successfully; returns false otherwise.
     */
    virtual bool StopAbility(const AAFwk::Want &want) = 0;

    /**
     * @brief Obtains the local database path.
     * If the local database path does not exist, the system creates one and returns the created path.
     *
     * @return Returns the local database file.
     */
    virtual std::string GetDatabaseDir() = 0;

    /**
     * @brief Obtains the absolute path where all private data files of this application are stored.
     *
     * @return Returns the absolute path storing all private data files of this application.
     */
    virtual std::string GetDataDir() = 0;

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
    virtual std::string GetDir(const std::string &name, int mode) = 0;

    /**
     * @brief Obtains the absolute path to the application-specific cache directory
     * on the primary external or shared storage device.
     *
     * @return Returns the absolute path to the application-specific cache directory on the external or
     * shared storage device; returns null if the external or shared storage device is temporarily unavailable.
     */
    virtual std::string GetExternalCacheDir()
    {
        return "";
    }

    /**
     * @brief Obtains the absolute path to the directory for storing files for the application on the
     * primary external or shared storage device.
     *
     * @param type Indicates the type of the file directory to return
     *
     * @return Returns the absolute path to the application file directory on the external or shared storage
     * device; returns null if the external or shared storage device is temporarily unavailable.
     */
    virtual std::string GetExternalFilesDir(std::string &type)
    {
        return "";
    }

    /**
     * @brief Obtains the directory for storing files for the application on the device's internal storage.
     *
     * @return Returns the application file directory.
     */
    virtual std::string GetFilesDir() = 0;

    /**
     * @brief Checks whether the current process has the given permission.
     * You need to call requestPermissionsFromUser(std::vector<std::string>,std::vector<int>, int) to request
     * a permission only if the current process does not have the specific permission.
     *
     * @param permission Indicates the permission to check. This parameter cannot be null.
     *
     * @return Returns 0 (IBundleManager.PERMISSION_GRANTED) if the current process has the permission;
     * returns -1 (IBundleManager.PERMISSION_DENIED) otherwise.
     */
    virtual int VerifySelfPermission(const std::string &permission)
    {
        return 0;
    }

    /**
     * @brief Obtains the bundle name of the current ability.
     *
     * @return Returns the bundle name of the current ability.
     */
    virtual std::string GetBundleName() const = 0;

    /**
     * @brief Obtains the path of the OHOS Ability Package (HAP} containing this ability.
     *
     * @return Returns the path of the HAP containing this ability.
     */
    virtual std::string GetBundleResourcePath() = 0;

    /**
     * @brief Starts a new ability.
     * An ability using the AbilityInfo.AbilityType.SERVICE or AbilityInfo.AbilityType.PAGE template uses this method
     * to start a specific ability. The system locates the target ability from installed abilities based on the value
     * of the want parameter and then starts it. You can specify the ability to start using the want parameter.
     *
     * @param want Indicates the Want containing information about the target ability to start.
     *
     * @param requestCode Indicates the request code returned after the ability using the AbilityInfo.AbilityType.PAGE
     * template is started. You can define the request code to identify the results returned by abilities. The value
     * ranges from 0 to 65535. This parameter takes effect only on abilities using the AbilityInfo.AbilityType.PAGE
     * template.
     *
     * @return errCode ERR_OK on success, others on failure.
     */
    virtual ErrCode StartAbility(const AAFwk::Want &want, int requestCode) = 0;

    /**
     * @brief Remove permissions for all users who have access to specific permissions
     *
     * @param permission Indicates the permission to unauth. This parameter cannot be null.
     * @param uri Indicates the URI to unauth. This parameter cannot be null.
     * @param uid Indicates the UID of the unauth to check.
     *
     */
    virtual void UnauthUriPermission(const std::string &permission, const Uri &uri, int uid)
    {}

    /**
     * @brief Obtains an ability manager.
     * The ability manager provides information about running processes and memory usage of an application.
     *
     * @return Returns an IAbilityManager instance.
     */
    virtual sptr<AAFwk::IAbilityManager> GetAbilityManager() = 0;

    /**
     * @brief Obtains the type of this application.
     *
     * @return Returns system if this application is a system application;
     * returns normal if it is released in OHOS AppGallery;
     * returns other if it is released by a third-party vendor;
     * returns an empty string if the query fails.
     */
    virtual std::string GetAppType() = 0;

    /**
     * @brief Destroys the current ability.
     *
     * @return errCode ERR_OK on success, others on failure.
     */
    virtual ErrCode TerminateAbility() = 0;

    /**
     * @brief Query whether the application of the specified PID and UID has been granted a certain permission
     *
     * @param permissions Indicates the list of permissions to be requested. This parameter cannot be null.
     * @param pid Process id
     * @param uid
     * @return Returns 0 (IBundleManager.PERMISSION_GRANTED) if the current process has the permission;
     * returns -1 (IBundleManager.PERMISSION_DENIED) otherwise.
     */
    virtual int VerifyPermission(const std::string &permission, int pid, int uid)
    {
        return 0;
    }

    /**
     * @brief Sets the pattern of this Context based on the specified pattern ID.
     *
     * @param patternId Indicates the resource ID of the pattern to set.
     */
    virtual void SetPattern(int patternId) = 0;

    /**
     * @brief Obtains the Context object of this ability.
     *
     * @return Returns the Context object of this ability.
     */
    virtual std::shared_ptr<Context> GetAbilityPackageContext()
    {
        return nullptr;
    }

    /**
     * @brief Obtains the HapModuleInfo object of the application.
     *
     * @return Returns the HapModuleInfo object of the application.
     */
    virtual std::shared_ptr<HapModuleInfo> GetHapModuleInfo() = 0;

    /**
     * @brief Obtains the name of the current process.
     *
     * @return Returns the current process name.
     */
    virtual std::string GetProcessName() = 0;

    /**
     * @brief Obtains the bundle name of the ability that called the current ability.
     * You can use the obtained bundle name to check whether the calling ability is allowed to receive the data you will
     * send. If you did not use Ability.startAbilityForResult(ohos.aafwk.content.Want, int,
     * ohos.aafwk.ability.startsetting.AbilityStartSetting) to start the calling ability, null is returned.
     *
     * @return Returns the bundle name of the calling ability; returns null if no calling ability is available.
     */
    virtual std::string GetCallingBundle() = 0;

    /**
     * @brief Requests certain permissions from the system.
     * This method is called for permission request. This is an asynchronous method. When it is executed,
     * the task will be called back.
     *
     * @param permissions Indicates the list of permissions to be requested. This parameter cannot be null.
     * @param permissionsState Indicates the list of permissions' state to be requested. This parameter cannot be null.
     * @param task The callback or promise fo js interface.
     */
    virtual void RequestPermissionsFromUser(std::vector<std::string> &permissions, std::vector<int> &permissionsState,
        PermissionRequestTask &&task)
    {}

    /**
     * @brief Starts a new ability with special ability start setting.
     *
     * @param want Indicates the Want containing information about the target ability to start.
     * @param requestCode Indicates the request code returned after the ability is started. You can define the request
     * code to identify the results returned by abilities. The value ranges from 0 to 65535.
     * @param abilityStartSetting Indicates the special start setting used in starting ability.
     *
     * @return errCode ERR_OK on success, others on failure.
     */
    virtual ErrCode StartAbility(const Want &want, int requestCode, const AbilityStartSetting &abilityStartSetting) = 0;

    /**
     * @brief Connects the current ability to an ability using the AbilityInfo.AbilityType.SERVICE template.
     *
     * @param want Indicates the want containing information about the ability to connect
     *
     * @param conn Indicates the callback object when the target ability is connected.
     *
     * @return True means success and false means failure
     */
    virtual bool ConnectAbility(const Want &want, const sptr<AAFwk::IAbilityConnection> &conn) = 0;

    /**
     * @brief Disconnects the current ability from an ability
     *
     * @param conn Indicates the IAbilityConnection callback object passed by connectAbility after the connection
     *              is set up. The IAbilityConnection object uniquely identifies a connection between two abilities.
     *
     * @return errCode ERR_OK on success, others on failure.
     */
    virtual ErrCode DisconnectAbility(const sptr<AAFwk::IAbilityConnection> &conn) = 0;

    /**
     * @brief Obtains information about the caller of this ability.
     *
     * @return Returns the caller information.
     */
    virtual Uri GetCaller() = 0;

    /**
     * @brief Get the string of this Context based on the specified resource ID.
     *
     * @param resId Indicates the resource ID of the string to get.
     *
     * @return Returns the string of this Context.
     */
    virtual std::string GetString(int resId) = 0;

    /**
     * @brief Get the string array of this Context based on the specified resource ID.
     *
     * @param resId Indicates the resource ID of the string array to get.
     *
     * @return Returns the string array of this Context.
     */
    virtual std::vector<std::string> GetStringArray(int resId) = 0;

    /**
     * @brief Get the integer array of this Context based on the specified resource ID.
     *
     * @param resId Indicates the resource ID of the integer array to get.
     *
     * @return Returns the integer array of this Context.
     */
    virtual std::vector<int> GetIntArray(int resId) = 0;

    /**
     * @brief Obtains the theme of this Context.
     *
     * @return theme Returns the theme of this Context.
     */
    virtual std::map<std::string, std::string> GetTheme() = 0;

    /**
     * @brief Sets the theme of this Context based on the specified theme ID.
     *
     * @param themeId Indicates the resource ID of the theme to set.
     */
    virtual void SetTheme(int themeId) = 0;

    /**
     * @brief Obtains the pattern of this Context.
     *
     * @return getPattern in interface Context
     */
    virtual std::map<std::string, std::string> GetPattern() = 0;

    /**
     * @brief Get the color of this Context based on the specified resource ID.
     *
     * @param resId Indicates the resource ID of the color to get.
     *
     * @return Returns the color value of this Context.
     */
    virtual int GetColor(int resId) = 0;

    /**
     * @brief Obtains the theme id of this Context.
     *
     * @return int Returns the theme id of this Context.
     */
    virtual int GetThemeId() = 0;

    /**
     * @brief Obtains the current display orientation of this ability.
     *
     * @return Returns the current display orientation.
     */
    virtual int GetDisplayOrientation() = 0;

    /**
     * @brief Obtains the path storing the preference file of the application.
     *        If the preference file path does not exist, the system creates one and returns the created path.
     *
     * @return Returns the preference file path .
     */
    virtual std::string GetPreferencesDir() = 0;

    /**
     * @brief Set color mode
     *
     * @param the value of color mode.
     */
    virtual void SetColorMode(int mode) = 0;

    /**
     * @brief Obtains color mode.
     *
     * @return Returns the color mode value.
     */
    virtual int GetColorMode() = 0;

    /**
     * @brief Obtains the unique ID of the mission containing this ability.
     *
     * @return Returns the unique mission ID.
     */
    virtual int GetMissionId() = 0;

    /**
     * @brief Starts multiple abilities.
     *
     * @param wants Indicates the Want containing information array about the target ability to start.
     */
    virtual void StartAbilities(const std::vector<AAFwk::Want> &wants) = 0;

    /**
     * @brief Checks whether the configuration of this ability is changing.
     *
     * @return Returns true if the configuration of this ability is changing and false otherwise.
     */
    virtual bool IsUpdatingConfigurations()
    {
        return false;
    }

    /**
     * @brief Informs the system of the time required for drawing this Page ability.
     *
     * @return Returns the notification is successful or fail
     */
    virtual bool PrintDrawnCompleted()
    {
        return false;
    }

    virtual sptr<IRemoteObject> GetToken() = 0;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_CONTEXT_H
