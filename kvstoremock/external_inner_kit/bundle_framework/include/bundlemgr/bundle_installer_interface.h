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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_INSTALLER_INTERFACE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_INSTALLER_INTERFACE_H

#include <vector>

#include "bundle_stream_installer_interface.h"
#include "status_receiver_interface.h"
#include "install_param.h"
#include "plugin/install_plugin_param.h"

namespace OHOS {
namespace AppExecFwk {
class DestroyAppCloneParam;
class IBundleInstaller : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.BundleInstaller");
    /**
     * @brief Installs an application, the final result will be notified from the statusReceiver object.
     * @attention Notice that the bundleFilePath should be an absolute path.
     * @param bundleFilePath Indicates the path for storing the ohos Ability Package (HAP) of the application
     *                       to install or update.
     * @param installParam Indicates the install parameters.
     * @param statusReceiver Indicates the callback object that using for notifing the install result.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool Install(const std::string &bundleFilePath, const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver) = 0;
    /**
     * @brief Installs an application by bundleName, the final result will be notified from the statusReceiver object.
     * @param bundleName Indicates the bundleName of the application to install.
     * @param installParam Indicates the install parameters.
     * @param statusReceiver Indicates the callback object that using for notifing the install result.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool Recover(const std::string &bundleName, const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver) = 0;

    /**
     * @brief Installs multiple haps, the final result will be notified from the statusReceiver object.
     * @attention Notice that the bundleFilePath should be an string vector of absolute paths.
     * @param bundleFilePaths Indicates the paths for storing the ohos Ability Packages (HAP) of the application
     *                       to install or update.
     * @param installParam Indicates the install parameters.
     * @param statusReceiver Indicates the callback object that using for notifing the install result.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool Install(const std::vector<std::string> &bundleFilePaths, const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver) = 0;

    /**
     * @brief Uninstalls an application, the result will be notified from the statusReceiver object.
     * @param bundleName Indicates the bundle name of the application to uninstall.
     * @param installParam Indicates the uninstall parameters.
     * @param statusReceiver Indicates the callback object that using for notifing the uninstall result.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool Uninstall(const std::string &bundleName, const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver) = 0;

    virtual bool Uninstall(const UninstallParam &uninstallParam,
        const sptr<IStatusReceiver> &statusReceiver) = 0;
    /**
     * @brief Uninstalls a module in an application, the result will be notified from the statusReceiver object.
     * @param bundleName Indicates the bundle name of the module to uninstall.
     * @param modulePackage Indicates the module package of the module to uninstall.
     * @param installParam Indicates the uninstall parameters.
     * @param statusReceiver Indicates the callback object that using for notifing the uninstall result.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool Uninstall(const std::string &bundleName, const std::string &modulePackage,
        const InstallParam &installParam, const sptr<IStatusReceiver> &statusReceiver) = 0;
    /**
     * @brief Installs an app by bundleName, only used in preInstall app.
     * @param bundleName Indicates the bundleName of the application to install.
     * @param installParam Indicates the install parameters.
     * @param statusReceiver Indicates the callback object that using for notifing the install result.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool InstallByBundleName(const std::string &bundleName, const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver)
    {
        return false;
    }

    /**
     * @brief Install sandbox application.
     * @param bundleName Indicates the bundle name of the sandbox application to be install.
     * @param dlpType Indicates type of the sandbox application.
     * @param userId Indicates the sandbox application will be installed under which user id.
     * @param appIndex Indicates the appIndex of the sandbox application installed under which user id.
     * @return Returns ERR_OK if the sandbox application is installed successfully; returns errcode otherwise.
     */
    virtual ErrCode InstallSandboxApp(const std::string &bundleName, int32_t dlpType, int32_t userId,
        int32_t &appIndex) = 0;

    /**
     * @brief Uninstall sandbox application.
     * @param bundleName Indicates the bundle name of the sandbox application to be install.
     * @param appIndex Indicates application index of the sandbox application.
     * @param userId Indicates the sandbox application will be uninstall under which user id.
     * @return Returns ERR_OK if the sandbox application is installed successfully; returns errcode otherwise.
     */
    virtual ErrCode UninstallSandboxApp(const std::string &bundleName, int32_t appIndex, int32_t userId) = 0;

    /**
     * @brief Install or update plugin application.
     * @param hostBundleName Indicates the bundle name of the host application.
     * @param pluginFilePath Indicates the paths for storing the HSP of the plugin to install or update.
     * @param installPluginParam Indicates the install parameters.
     * @return Returns ERR_OK if the plugin application is installed successfully; returns errcode otherwise.
     */
    virtual ErrCode InstallPlugin(const std::string &hostBundleName, const std::vector<std::string> &pluginFilePaths,
        const InstallPluginParam &installPluginParam)
    {
        return ERR_OK;
    }

    /**
     * @brief uninstall plugin application.
     * @param hostBundleName Indicates the bundle name of the host application.
     * @param pluginBundleName Indicates the plugin bundle name to uninstall.
     * @param installPluginParam Indicates the uninstall parameters.
     * @return Returns ERR_OK if the plugin application is uninstalled successfully; returns errcode otherwise.
     */
    virtual ErrCode UninstallPlugin(const std::string &hostBundleName, const std::string &pluginBundleName,
        const InstallPluginParam &installPluginParam)
    {
        return ERR_OK;
    }

    virtual sptr<IBundleStreamInstaller> CreateStreamInstaller(const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver, const std::vector<std::string> &originHapPaths) = 0;
    virtual bool DestoryBundleStreamInstaller(uint32_t streamInstallerId) = 0;
    virtual ErrCode StreamInstall(const std::vector<std::string> &bundleFilePaths, const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver) = 0;

    virtual bool UpdateBundleForSelf(const std::vector<std::string> &bundleFilePaths, const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver)
    {
        return true;
    }

    virtual bool UninstallAndRecover(const std::string &bundleName, const InstallParam &installParam,
        const sptr<IStatusReceiver> &statusReceiver)
    {
        return true;
    }

    virtual ErrCode InstallCloneApp(const std::string &bundleName, int32_t userId, int32_t& appIndex)
    {
        return ERR_OK;
    }

    virtual ErrCode UninstallCloneApp(const std::string &bundleName, int32_t userId, int32_t appIndex,
        const DestroyAppCloneParam &destroyAppCloneParam)
    {
        return ERR_OK;
    }

    virtual ErrCode InstallExisted(const std::string &bundleName, int32_t userId)
    {
        return ERR_OK;
    }

    virtual ErrCode InstallEnterpriseReSignatureCert(const std::string &certAlias, int32_t fd, int32_t userId)
    {
        return ERR_OK;
    }

    virtual ErrCode AddEnterpriseResignCert(
        const std::string &certAlias, const std::string &certContent, int32_t userId)
    {
        return ERR_OK;
    }

    virtual ErrCode UninstallEnterpriseReSignatureCert(const std::string &certificateAlias, int32_t userId)
    {
        return ERR_OK;
    }

    virtual ErrCode DeleteEnterpriseReSignatureCert(const std::string &certificateAlias, int32_t userId)
    {
        return ERR_OK;
    }

    virtual ErrCode GetEnterpriseReSignatureCert(int32_t userId, std::vector<std::string> &certificateAlias)
    {
        return ERR_OK;
    }

    virtual ErrCode UninstallNewPreinstalledApps(const std::vector<std::string> &bundleNames)
    {
        return ERR_OK;
    }

    virtual ErrCode DeleteReSignCert(int32_t userId)
    {
        return ERR_OK;
    }
};

#define PARCEL_WRITE_INTERFACE_TOKEN(parcel, token)                                 \
    do {                                                                            \
        bool ret = parcel.WriteInterfaceToken((token));                             \
        if (!ret) {                                                                 \
            APP_LOGE("write interface token failed");             \
            return false;                                                           \
        }                                                                           \
    } while (0)

#define PARCEL_WRITE(parcel, type, value)                                           \
    do {                                                                            \
        bool ret = parcel.Write##type((value));                                     \
        if (!ret) {                                                                 \
            APP_LOGE("write parameter failed");                   \
            return false;                                                           \
        }                                                                           \
    } while (0)
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_INSTALLER_INTERFACE_H
