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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_MGR_PROXY_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_MGR_PROXY_H

#include <string>
#include <shared_mutex>
#include <vector>

#include "bundle_event_callback_interface.h"
#include "bundle_framework_core_ipc_interface_code.h"
#include "bundle_mgr_interface.h"
#include "bundle_status_callback_interface.h"
#include "clean_cache_callback_interface.h"
#include "element_name.h"
#include "iremote_proxy.h"
#include "preinstalled_application_info.h"
#include "process_cache_callback_interface.h"

namespace OHOS {
namespace AppExecFwk {

#if defined(__GNUC__) && __GNUC__ >= 4
    #define BUNDLE_HIDDEN __attribute__((visibility("hidden")))
#else
    #define BUNDLE_HIDDEN
#endif
class BundleMgrProxy : public IRemoteProxy<IBundleMgr> {
public:
    explicit BundleMgrProxy(const sptr<IRemoteObject> &impl);
    virtual ~BundleMgrProxy() override;
    /**
     * @brief Obtains the ApplicationInfo based on a given bundle name through the proxy object.
     * @param appName Indicates the application bundle name to be queried.
     * @param flag Indicates the flag used to specify information contained
     *             in the ApplicationInfo object that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfo Indicates the obtained ApplicationInfo object.
     * @return Returns true if the application is successfully obtained; returns false otherwise.
     */
    virtual bool GetApplicationInfo(
        const std::string &appName, const ApplicationFlag flag, const int userId, ApplicationInfo &appInfo) override;
    /**
     * @brief Obtains the ApplicationInfo based on a given bundle name through the proxy object.
     * @param appName Indicates the application bundle name to be queried.
     * @param flags Indicates the flag used to specify information contained
     *             in the ApplicationInfo object that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfo Indicates the obtained ApplicationInfo object.
     * @return Returns true if the application is successfully obtained; returns false otherwise.
     */
    virtual bool GetApplicationInfo(
        const std::string &appName, int32_t flags, int32_t userId, ApplicationInfo &appInfo) override;
    /**
     * @brief Obtains the ApplicationInfo based on a given bundle name through the proxy object.
     * @param appName Indicates the application bundle name to be queried.
     * @param flags Indicates the flag used to specify information contained
     *             in the ApplicationInfo object that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfo Indicates the obtained ApplicationInfo object.
     * @return Returns ERR_OK if the application is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetApplicationInfoV9(
        const std::string &appName, int32_t flags, int32_t userId, ApplicationInfo &appInfo) override;
    /**
     * @brief Obtains information about all installed applications of a specified user through the proxy object.
     * @param flag Indicates the flag used to specify information contained
     *             in the ApplicationInfo objects that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfos Indicates all of the obtained ApplicationInfo objects.
     * @return Returns true if the application is successfully obtained; returns false otherwise.
     */
    virtual bool GetApplicationInfos(
        const ApplicationFlag flag, int userId, std::vector<ApplicationInfo> &appInfos) override;
    /**
     * @brief Obtains information about all installed applications of a specified user through the proxy object.
     * @param flags Indicates the flag used to specify information contained
     *             in the ApplicationInfo objects that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfos Indicates all of the obtained ApplicationInfo objects.
     * @return Returns true if the application is successfully obtained; returns false otherwise.
     */
    virtual bool GetApplicationInfos(
        int32_t flags, int32_t userId, std::vector<ApplicationInfo> &appInfos) override;
    /**
     * @brief Obtains information about all installed applications of a specified user through the proxy object.
     * @param flags Indicates the flag used to specify information contained
     *             in the ApplicationInfo objects that will be returned.
     * @param userId Indicates the user ID.
     * @param appInfos Indicates all of the obtained ApplicationInfo objects.
     * @return Returns ERR_OK if the application is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetApplicationInfosV9(
        int32_t flags, int32_t userId, std::vector<ApplicationInfo> &appInfos) override;
    /**
     * @brief Obtains the BundleInfo based on a given bundle name through the proxy object.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flag Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @param userId Indicates the user ID.
     * @return Returns true if the BundleInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleInfo(const std::string &bundleName, const BundleFlag flag,
        BundleInfo &bundleInfo, int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Obtains the BundleInfo based on a given bundle name through the proxy object.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @param userId Indicates the user ID.
     * @return Returns true if the BundleInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleInfo(const std::string &bundleName, int32_t flags,
        BundleInfo &bundleInfo, int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Obtains the BundleInfo based on a given bundle name through the proxy object.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetBundleInfoV9(const std::string &bundleName, int32_t flags,
        BundleInfo &bundleInfo, int32_t userId) override;

    /**
     * @brief Obtains the BundleInfo based on a given bundle name through the proxy object.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param bundleInfoForException Indicates the obtained BundleInfo object.
     * @param userId Indicates the user ID.
     * @param catchSoNum Indicates the num of catched hash values of so.
     * @param catchSoMaxSize Indicates the max size of catched so.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetBundleInfoForException(const std::string &bundleName,
        int32_t userId, uint32_t catchSoNum, uint64_t catchSoMaxSize,
        BundleInfoForException &bundleInfoForException) override;
    /**
     * @brief Obtains the BundleInfo based on a given bundle name.
     * @param uid Indicates the uid.
     * @param assetGroupInfo Indicates the obtained AssetGroupInfo object.
     * @return Returns ERR_OK if the AssetGroupInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetAssetGroupsInfo(const int32_t uid, AssetGroupInfo &assetGroupInfo) override;
    /**
     * @brief Obtains the BundleInfos by the given want list through the proxy object.
     * @param wants Indicates the imformation of the abilities to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfos Indicates the obtained BundleInfo list object.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode BatchGetBundleInfo(const std::vector<Want> &wants, int32_t flags,
        std::vector<BundleInfo> &bundleInfos, int32_t userId) override;
    /**
     * @brief Obtains the BundleInfos based on a given bundle name list through the proxy object.
     * @param bundleNames Indicates the application bundle name list to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfos Indicates the obtained BundleInfo list object.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode BatchGetBundleInfo(const std::vector<std::string> &bundleNames, int32_t flags,
        std::vector<BundleInfo> &bundleInfos, int32_t userId) override;
    /**
     * @brief Obtains the BundleInfo based on calling app.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetBundleInfoForSelf(int32_t flags, BundleInfo &bundleInfo) override;
    /**
     * @brief Obtains the BundleInfo based on calling app.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetBundleInfoForSelfWithCache(int32_t flags, BundleInfo &bundleInfo) override;
    /**
     * @brief Obtains the BundleInfo based on calling app.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetBundleInfoForSelfWithOutCache(int32_t flags, BundleInfo &bundleInfo) override;
    /**
     * @brief Obtains the BundleInfo based on a given bundle name, which the calling app depends on.
     * @param sharedBundleName Indicates the bundle name to be queried.
     * @param sharedBundleInfo Indicates the obtained BundleInfo object.
     * @param flag Indicates the flag, GetDependentBundleInfoFlag.
     * @return Returns ERR_OK if the BundleInfo is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetDependentBundleInfo(const std::string &sharedBundleName, BundleInfo &sharedBundleInfo,
        GetDependentBundleInfoFlag flag = GetDependentBundleInfoFlag::GET_APP_CROSS_HSP_BUNDLE_INFO) override;
    /**
     * @brief Obtains the BundlePackInfo based on a given bundle name.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param BundlePackInfo Indicates the obtained BundlePackInfo object.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundlePackInfo is successfully obtained; returns other ErrCode otherwise.
     */
    virtual ErrCode GetBundlePackInfo(const std::string &bundleName, const BundlePackFlag flags,
        BundlePackInfo &bundlePackInfo, int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Obtains the BundlePackInfo based on a given bundle name.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param BundlePackInfo Indicates the obtained BundlePackInfo object.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundlePackInfo is successfully obtained; returns other ErrCode otherwise.
     */
    virtual ErrCode GetBundlePackInfo(const std::string &bundleName, int32_t flags,
        BundlePackInfo &bundlePackInfo, int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Obtains BundleInfo of all bundles available in the system through the proxy object.
     * @param flag Indicates the flag used to specify information contained in the BundleInfo that will be returned.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @param userId Indicates the user ID.
     * @return Returns true if the BundleInfos is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleInfos(const BundleFlag flag, std::vector<BundleInfo> &bundleInfos,
        int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Obtains BundleInfo of all bundles available in the system through the proxy object.
     * @param flags Indicates the flag used to specify information contained in the BundleInfo that will be returned.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @param userId Indicates the user ID.
     * @return Returns true if the BundleInfos is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleInfos(int32_t flags, std::vector<BundleInfo> &bundleInfos,
        int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Obtains BundleInfo of all bundles available in the system through the proxy object.
     * @param flags Indicates the flag used to specify information contained in the BundleInfo that will be returned.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundleInfos is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetBundleInfosV9(int32_t flags, std::vector<BundleInfo> &bundleInfos, int32_t userId) override;
    /**
     * @brief Obtains BundleInfo of all bundles with only permission check.
     * @param flags Indicates the flag used to specify information contained in the BundleInfo.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if the BundleInfos is successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetInstalledBundleList(uint32_t flags, int32_t userId,
        std::vector<BundleInfo> &bundleInfos) override;
    /**
     * @brief Obtains the application UID based on the given bundle name and user ID through the proxy object.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @return Returns the uid if successfully obtained; returns -1 otherwise.
     */
    virtual int GetUidByBundleName(const std::string &bundleName, const int userId) override;
    /**
     * @brief Obtains the application UID based on the given bundle name and user ID through the proxy object.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @param userId Indicates the app Index.
     * @return Returns the uid if successfully obtained; returns -1 otherwise.
     */
    virtual int32_t GetUidByBundleName(const std::string &bundleName, const int32_t userId, int32_t appIndex) override;
    /**
     * @brief Obtains the debug application UID based on the given bundle name and user ID through the proxy object.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @return Returns the uid if successfully obtained; returns -1 otherwise.
     */
    virtual int GetUidByDebugBundleName(const std::string &bundleName, const int userId) override;

    /**
     * @brief Obtains the application ID based on the given bundle name and user ID.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @return Returns the application ID if successfully obtained; returns empty string otherwise.
     */
    virtual std::string GetAppIdByBundleName(const std::string &bundleName, const int userId) override;
    /**
     * @brief Obtains the bundle name of a specified application based on the given UID through the proxy object.
     * @param uid Indicates the uid.
     * @param bundleName Indicates the obtained bundle name.
     * @return Returns true if the bundle name is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleNameForUid(const int uid, std::string &bundleName) override;
    /**
     * @brief Obtains all bundle names of a specified application based on the given application UID the proxy object.
     * @param uid Indicates the uid.
     * @param bundleNames Indicates the obtained bundle names.
     * @return Returns true if the bundle names is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundlesForUid(const int uid, std::vector<std::string> &bundleNames) override;
    /**
     * @brief Obtains the formal name associated with the given UID.
     * @param uid Indicates the uid.
     * @param name Indicates the obtained formal name.
     * @return Returns ERR_OK if execute success; returns errCode otherwise.
     */
    virtual ErrCode GetNameForUid(const int uid, std::string &name) override;
    /**
     * @brief Obtains the formal name associated with the given UID.
     * @param uid Indicates the uid.
     * @param name Indicates the obtained formal bundleName.
     * @param name Indicates the obtained appIndex.
     * @return Returns ERR_OK if execute success; returns errCode otherwise.
     */
    virtual ErrCode GetNameAndIndexForUid(const int32_t uid, std::string &bundleName, int32_t &appIndex) override;
    /**
     * @brief Obtains the appIdentifier and appIndex with the given access tokenId.
     * @param accessTokenId Indicates the access tokenId of the application.
     * @param appIdentifier Indicates the app identifier of the application.
     * @param appIndex Indicates the app index of the application.
     * @return Returns ERR_OK if execute success; returns errCode otherwise.
     */
    virtual ErrCode GetAppIdentifierAndAppIndex(const uint32_t accessTokenId,
        std::string &appIdentifier, int32_t &appIndex) override;
    /**
     * @brief Obtains the formal name associated with the given UID.
     * @param uids Indicates the uid list.
     * @param simpleAppInfo Indicates all of the obtained SimpleAppInfo objects.
     * @return Returns ERR_OK if execute success; returns errCode otherwise.
     */
    virtual ErrCode GetSimpleAppInfoForUid(
        const std::vector<std::int32_t> &uids, std::vector<SimpleAppInfo> &simpleAppInfo) override;
    /**
     * @brief Obtains an array of all group IDs associated with a specified bundle through the proxy object.
     * @param bundleName Indicates the bundle name.
     * @param gids Indicates the group IDs associated with the specified bundle.
     * @return Returns true if the gids is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleGids(const std::string &bundleName, std::vector<int> &gids) override;
    /**
     * @brief Obtains an array of all group IDs associated with the given bundle name and UID.
     * @param bundleName Indicates the bundle name.
     * @param uid Indicates the uid.
     * @param gids Indicates the group IDs associated with the specified bundle.
     * @return Returns true if the gids is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleGidsByUid(const std::string &bundleName, const int &uid, std::vector<int> &gids) override;
    /**
     * @brief Obtains the type of a specified application based on the given bundle name through the proxy object.
     * @param bundleName Indicates the bundle name.
     * @return Returns "system" if the bundle is a system application; returns "third-party" otherwise.
     */
    virtual std::string GetAppType(const std::string &bundleName) override;
    /**
     * @brief Check whether the app is system app by it's UID through the proxy object.
     * @param uid Indicates the uid.
     * @return Returns true if the bundle is a system application; returns false otherwise.
     */
    virtual bool CheckIsSystemAppByUid(const int uid) override;
    /**
     * @brief Obtains the BundleInfo of application bundles based on the specified metaData through the proxy object.
     * @param metaData Indicates the metadata to get in the bundle.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @return Returns true if the BundleInfos is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleInfosByMetaData(const std::string &metaData, std::vector<BundleInfo> &bundleInfos) override;
    /**
     * @brief Query the AbilityInfo by the given Want through the proxy object.
     * @param want Indicates the information of the ability.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAbilityInfo(const Want &want, AbilityInfo &abilityInfo) override;
    /**
     * @brief Query the AbilityInfo by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @param callBack Indicates the callback to be invoked for return ability manager service the operation result.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAbilityInfo(const Want &want, int32_t flags, int32_t userId,
        AbilityInfo &abilityInfo, const sptr<IRemoteObject> &callBack) override;
    /**
     * @brief Silent install by the given Want.
     * @param want Indicates the information of the want.
     * @param userId Indicates the user ID.
     * @param callBack Indicates the callback to be invoked for return the operation result.
     * @return Returns true if silent install successfully; returns false otherwise.
     */
    virtual bool SilentInstall(const Want &want, int32_t userId, const sptr<IRemoteObject> &callBack) override;
    /**
     * @brief Upgrade atomic service
     * @param want Indicates the information of the ability.
     * @param userId Indicates the user ID.
     */
    virtual void UpgradeAtomicService(const Want &want, int32_t userId) override;
    /**
     * @brief Query the AbilityInfo by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAbilityInfo(const Want &want, int32_t flags, int32_t userId, AbilityInfo &abilityInfo) override;
    /**
     * @brief Query the AbilityInfo of list by the given Want.
     * @param want Indicates the information of the ability.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns true if the AbilityInfos is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAbilityInfos(const Want &want, std::vector<AbilityInfo> &abilityInfos) override;
    /**
     * @brief Query the AbilityInfo of list by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns true if the AbilityInfos is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAbilityInfos(
        const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos) override;
    /**
     * @brief Query the AbilityInfo of list by the given Want.
     * @param want Indicates the information of the ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns ERR_OK if the AbilityInfos is successfully obtained; returns errCode otherwise.
     */
    virtual ErrCode QueryAbilityInfosV9(
        const Want &want, int32_t flags, int32_t userId, std::vector<AbilityInfo> &abilityInfos) override;
    /**
     * @brief Get the AbilityInfo of list by the given uri.
     * @param uri Indicates the uri used for matching ability.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns ERR_OK if the AbilityInfos is successfully obtained; returns errCode otherwise.
     */
    virtual ErrCode GetAbilityInfos(
        const std::string &uri, uint32_t flags, std::vector<AbilityInfo> &abilityInfos) override;
    /**
     * @brief Query the AbilityInfo of list by the given Wants.
     * @param wants Indicates the information of the abilities.
     * @param flags Indicates the information contained in the AbilityInfo object to be returned.
     * @param userId Indicates the user ID.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns ERR_OK if the AbilityInfos is successfully obtained; returns errCode otherwise.
     */
    virtual ErrCode BatchQueryAbilityInfos(const std::vector<Want> &wants, int32_t flags, int32_t userId,
        std::vector<AbilityInfo> &abilityInfos) override;
    /**
     * @brief Query the launcher AbilityInfo of list by the given Want.
     * @param want Indicates the information of the ability.
     * @param userId Indicates the user ID.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns ERR_OK if the AbilityInfos is successfully obtained; returns errCode otherwise.
     */
    virtual ErrCode QueryLauncherAbilityInfos(
        const Want &want, int32_t userId, std::vector<AbilityInfo> &abilityInfo) override;
    /**
     * @brief Public interface query the AbilityInfo of list by the given Wants.
     * @param bundleName Indicates the bundle name.
     * @param userId Indicates the user ID.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns ERR_OK if the AbilityInfos is successfully obtained; returns errCode otherwise.
     */
    virtual ErrCode GetLauncherAbilityInfoSync(
        const std::string &bundleName, int32_t userId, std::vector<AbilityInfo> &abilityInfo) override;
    /**
     * @brief Query the AbilityInfo of list for all service on launcher.
     * @param userId Indicates the information of the user.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns true if the AbilityInfos is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAllAbilityInfos(
        const Want &want, int32_t userId, std::vector<AbilityInfo> &abilityInfos) override;
    /**
     * @brief Query the AbilityInfo by ability.uri in config.json through the proxy object.
     * @param abilityUri Indicates the uri of the ability.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAbilityInfoByUri(const std::string &abilityUri, AbilityInfo &abilityInfo) override;
    /**
     * @brief Query the AbilityInfo by ability.uri in config.json.
     * @param abilityUri Indicates the uri of the ability.
     * @param abilityInfos Indicates the obtained AbilityInfos object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAbilityInfosByUri(const std::string &abilityUri, std::vector<AbilityInfo> &abilityInfos) override;
    /**
     * @brief Query the AbilityInfo by ability.uri in config.json through the proxy object.
     * @param abilityUri Indicates the uri of the ability.
     * @param userId Indicates the user ID.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    virtual bool QueryAbilityInfoByUri(
        const std::string &abilityUri, int32_t userId, AbilityInfo &abilityInfo) override;
    /**
     * @brief Obtains the BundleInfo of all keep-alive applications in the system through the proxy object.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @return Returns true if the BundleInfos is successfully obtained; returns false otherwise.
     */
    virtual bool QueryKeepAliveBundleInfos(std::vector<BundleInfo> &bundleInfos) override;
    /**
     * @brief Obtains the label of a specified ability through the proxy object.
     * @param bundleName Indicates the bundle name.
     * @param abilityName Indicates the ability name.
     * @return Returns the label of the ability if exist; returns empty string otherwise.
     */
    virtual std::string GetAbilityLabel(const std::string &bundleName, const std::string &abilityName) override;
    /**
     * @brief Obtains the label of a specified ability through the proxy object.
     * @param bundleName Indicates the bundle name.
     * @param moduleName Indicates the module name.
     * @param abilityName Indicates the ability name.
     * @param label Indicates the obtained label.
     * @return Returns ERR_OK if called successfully; returns error code otherwise.
     */
    virtual ErrCode GetAbilityLabel(const std::string &bundleName, const std::string &moduleName,
        const std::string &abilityName, std::string &label) override;
    /**
     * @brief Obtains the label of a specified application.
     * @param bundleName Indicates the bundle name.
     * @param appIndex Indicates the app index for clone apps, 0 for main app.
     * @param label Indicates the obtained label.
     * @return Returns ERR_OK if called successfully; returns error code otherwise.
     */
    virtual ErrCode GetApplicationLabel(const std::string &bundleName, int32_t appIndex, std::string &label) override;
    /**
     * @brief Sets whether the application is first launch.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @param appIndex Indicates the app index, 0 for normal app, > 0 for clone app.
     * @param isBundleFirstLaunched Specifies whether the application is first launch.
     * @return Returns ERR_OK if successful; returns error code otherwise.
     */
    virtual ErrCode SetBundleFirstLaunch(const std::string &bundleName, int32_t userId,
        int32_t appIndex, bool isBundleFirstLaunched) override;
    /**
     * @brief Obtains information about an application bundle contained
     *          in an ohos Ability Package (HAP) through the proxy object.
     * @param hapFilePath Indicates the absolute file path of the HAP.
     * @param flag Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @return Returns true if the BundleInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleArchiveInfo(
        const std::string &hapFilePath, const BundleFlag flag, BundleInfo &bundleInfo) override;
    /**
     * @brief Obtains information about an application bundle contained
     *          in an ohos Ability Package (HAP) through the proxy object.
     * @param hapFilePath Indicates the absolute file path of the HAP.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @return Returns true if the BundleInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleArchiveInfo(
        const std::string &hapFilePath, int32_t flags, BundleInfo &bundleInfo) override;
    /**
     * @brief Obtains information about an application bundle contained
     *          in an ohos Ability Package (HAP) through the proxy object.
     * @param hapFilePath Indicates the absolute file path of the HAP.
     * @param flags Indicates the information contained in the BundleInfo object to be returned.
     * @param bundleInfo Indicates the obtained BundleInfo object.
     * @return Returns ERR_OK if this function is successfully called; returns errCode otherwise.
     */
    virtual ErrCode GetBundleArchiveInfoV9(
        const std::string &hapFilePath, int32_t flags, BundleInfo &bundleInfo) override;
    /**
     * @brief Obtain the HAP module info of a specific ability through the proxy object.
     * @param abilityInfo Indicates the ability.
     * @param hapModuleInfo Indicates the obtained HapModuleInfo object.
     * @return Returns true if the HapModuleInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetHapModuleInfo(const AbilityInfo &abilityInfo, HapModuleInfo &hapModuleInfo) override;
    /**
     * @brief Obtain the HAP module info of a specific ability through the proxy object.
     * @param abilityInfo Indicates the ability.
     * @param userId Indicates the userId.
     * @param hapModuleInfo Indicates the obtained HapModuleInfo object.
     * @return Returns true if the HapModuleInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetHapModuleInfo(
        const AbilityInfo &abilityInfo, int32_t userId, HapModuleInfo &hapModuleInfo) override;
    /**
     * @brief Obtains the Want for starting the main ability of an application
     *          based on the given bundle name through the proxy object.
     * @param bundleName Indicates the bundle name.
     * @param want Indicates the obtained launch Want object.
     * @param userId Indicates the userId.
     * @return Returns ERR_OK if this function is successfully called; returns errCode otherwise.
     */
    virtual ErrCode GetLaunchWantForBundle(
        const std::string &bundleName, Want &want, int32_t userId = Constants::UNSPECIFIED_USERID,
        bool isSync = false) override;
    /**
     * @brief Obtains detailed information about a specified permission through the proxy object.
     * @param permissionName Indicates the name of the ohos permission.
     * @param permissionDef Indicates the object containing detailed information about the given ohos permission.
     * @return Returns ERR_OK if the PermissionDef object is successfully obtained; returns other ErrCode otherwise.
     */
    virtual ErrCode GetPermissionDef(const std::string &permissionName, PermissionDef &permissionDef) override;
    /**
     * @brief Clears cache data of a specified size through the proxy object.
     * @param cacheSize Indicates the size of the cache data is to be cleared.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode CleanBundleCacheFilesAutomatic(uint64_t cacheSize) override;
    /**
     * @brief Clears cache data of a specified size through the proxy object.
     * @param cacheSize Indicates the size of the cache data is to be cleared.
     * @param cleanType Indicates the type of cache data to be cleared.
     * @param cleanedSize Indicates the size of the cache data that is actually cleared.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode CleanBundleCacheFilesAutomatic(uint64_t cacheSize, CleanType cleanType,
        std::optional<uint64_t>& cleanedSize) override;
    /**
     * @brief Clears cache data of a specified application through the proxy object.
     * @param bundleName Indicates the bundle name of the application whose cache data is to be cleared.
     * @param cleanCacheCallback Indicates the callback to be invoked for returning the operation result.
     * @param userId description the user id.
     * @param appIndex Indicates the app index.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode CleanBundleCacheFiles(
        const std::string &bundleName, const sptr<ICleanCacheCallback> cleanCacheCallback,
        int32_t userId = Constants::UNSPECIFIED_USERID, int32_t appIndex = 0) override;

    virtual ErrCode CleanBundleCacheFilesForSelf(const sptr<ICleanCacheCallback> cleanCacheCallback) override;

    /**
     * @brief Clears application running data of a specified application through the proxy object.
     * @param bundleName Indicates the bundle name of the application whose data is to be cleared.
     * @param userId Indicates the user id.
     * @param appIndex Indicates the app index.
     * @return Returns true if the data cleared successfully; returns false otherwise.
     */
    virtual bool CleanBundleDataFiles(const std::string &bundleName,
        const int userId = 0, const int appIndex = 0, const int callerUid = -1) override;
    /**
     * @brief Register the specific bundle status callback through the proxy object.
     * @param bundleStatusCallback Indicates the callback to be invoked for returning the bundle status changed result.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool RegisterBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback) override;

    virtual bool RegisterBundleEventCallback(const sptr<IBundleEventCallback> &bundleEventCallback) override;

    virtual bool UnregisterBundleEventCallback(const sptr<IBundleEventCallback> &bundleEventCallback) override;
    /**
     * @brief Clear the specific bundle status callback through the proxy object.
     * @param bundleStatusCallback Indicates the callback to be cleared.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool ClearBundleStatusCallback(const sptr<IBundleStatusCallback> &bundleStatusCallback) override;
    /**
     * @brief Unregister all the callbacks of status changed through the proxy object.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool UnregisterBundleStatusCallback() override;
    /**
     * @brief Dump the bundle informations with specific flags through the proxy object.
     * @param flag Indicates the information contained in the dump result.
     * @param bundleName Indicates the bundle name if needed.
     * @param userId Indicates the user ID.
     * @param result Indicates the dump information result.
     * @return Returns true if the dump result is successfully obtained; returns false otherwise.
     */
    virtual bool DumpInfos(
        const DumpFlag flag, const std::string &bundleName, int32_t userId, std::string &result) override;
    /**
     * @brief Compile the bundle informations with specific flags through the proxy object.
     * @param bundleName Indicates the bundle name if needed.
     * @param compileMode Indicates the mode name.
     * @param isAllBundle Does it represent all bundlenames.
     * @return Returns result of the operation.
     */
    virtual ErrCode CompileProcessAOT(const std::string &bundleName, const std::string &compileMode,
        bool isAllBundle, std::vector<std::string> &compileResults) override;
    /**
     * @brief Reset the bundle informations with specific flags through the proxy object.
     * @param bundleName Indicates the bundle name if needed.
     * @param isAllBundle Does it represent all bundlenames.
     * @return Returns result of the operation.
     */
    virtual ErrCode CompileReset(const std::string &bundleName, bool isAllBundle) override;
    /**
     * @brief Reset the bundle informations through the proxy object.
     * @return Returns result of the operation.
     */
    virtual ErrCode ResetAllAOT() override;
    /**
     * @brief copy ap file to /data/local/pgo through the proxy object.
     * @param bundleName Indicates the bundle name if needed.
     * @param isAllBundle Does it represent all bundlenames.
     * @param results Indicates the copy ap information result.
     * @return Returns ERR_OK if called successfully; returns error code otherwise.
     */
    virtual ErrCode CopyAp(const std::string &bundleName, bool isAllBundle, std::vector<std::string> &results) override;
    /**
     * @brief Checks whether a specified application is a debug application through the proxy object.
     * @param bundleName Indicates the bundle name of the application.
     * @param isEnable Indicates the application is a debug application.
     * @return Returns result of the operation.
     */
    virtual ErrCode IsDebuggableApplication(const std::string &bundleName, bool &isDebuggable) override;
    /**
     * @brief Checks whether a specified application is enabled through the proxy object.
     * @param bundleName Indicates the bundle name of the application.
     * @param isEnable Indicates the application status is enabled.
     * @return Returns result of the operation.
     */
    virtual ErrCode IsApplicationEnabled(const std::string &bundleName, bool &isEnable) override;
    /**
     * @brief Checks whether a specified clone application is enabled.
     * @param bundleName Indicates the bundle name of the application.
     * @param appIndex Indicates the app index of clone applications.
     * @param isEnable Indicates the application status is enabled.
     * @return Returns result of the operation.
     */
    virtual ErrCode IsCloneApplicationEnabled(
        const std::string &bundleName, int32_t appIndex, bool &isEnable) override;
    /**
     * @brief Sets whether to enable a specified application through the proxy object.
     * @param bundleName Indicates the bundle name of the application.
     * @param isEnable Specifies whether to enable the application.
     *                 The value true means to enable it, and the value false means to disable it.
     * @param userId description the user id.
     * @return Returns result of the operation.
     */
    virtual ErrCode SetApplicationEnabled(const std::string &bundleName, bool isEnable,
        int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Sets whether to enable a specified clone application.
     * @param bundleName Indicates the bundle name of the application.
     * @param appIndex Indicates the app index of clone applications.
     * @param isEnable Specifies whether to enable the application.
     *                 The value true means to enable it, and the value false means to disable it.
     * @param userId description the user id.
     * @return Returns result of the operation.
     */
    virtual ErrCode SetCloneApplicationEnabled(const std::string &bundleName, int32_t appIndex, bool isEnable,
        int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Sets whether to enable a specified ability through the proxy object.
     * @param abilityInfo Indicates information about the ability to check.
     * @param isEnable Indicates the ability status is enabled.
     * @return Returns result of the operation.
     */
    virtual ErrCode IsAbilityEnabled(const AbilityInfo &abilityInfo, bool &isEnable) override;
    /**
     * @brief Sets whether to enable a specified ability.
     * @param abilityInfo Indicates information about the ability to check.
     * @param appIndex Indicates the app index of clone applications.
     * @param isEnable Indicates the ability status is enabled.
     * @return Returns result of the operation.
     */
    virtual ErrCode IsCloneAbilityEnabled(const AbilityInfo &abilityInfo, int32_t appIndex, bool &isEnable) override;
    /**
     * @brief Sets whether to enable a specified ability through the proxy object.
     * @param abilityInfo Indicates information about the ability.
     * @param isEnabled Specifies whether to enable the ability.
     *                 The value true means to enable it, and the value false means to disable it.
     * @param userId description the user id.
     * @return Returns result of the operation.
     */
    virtual ErrCode SetAbilityEnabled(const AbilityInfo &abilityInfo, bool isEnabled,
        int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Sets whether to enable a specified ability.
     * @param abilityInfo Indicates information about the ability.
     * @param appIndex Indicates the app index of clone applications.
     * @param isEnabled Specifies whether to enable the ability.
     *                 The value true means to enable it, and the value false means to disable it.
     * @param userId description the user id.
     * @return Returns result of the operation.
     */
    virtual ErrCode SetCloneAbilityEnabled(const AbilityInfo &abilityInfo, int32_t appIndex, bool isEnabled,
        int32_t userId = Constants::UNSPECIFIED_USERID)  override;
    /**
     * @brief Obtains the interface used to install and uninstall bundles through the proxy object.
     * @return Returns a pointer to IBundleInstaller class if exist; returns nullptr otherwise.
     */
    virtual sptr<IBundleInstaller> GetBundleInstaller() override;
    /**
     * @brief Obtains the interface used to create or delete user.
     * @return Returns a pointer to IBundleUserMgr class if exist; returns nullptr otherwise.
     */
    virtual sptr<IBundleUserMgr> GetBundleUserMgr() override;
    /**
     * @brief Obtains the VerifyManager.
     * @return Returns a pointer to VerifyManager class if exist; returns nullptr otherwise.
     */
    virtual sptr<IVerifyManager> GetVerifyManager() override;
    /**
     * @brief  Obtains the FormInfo objects provided by all applications on the device.
     * @param  formInfos List of FormInfo objects if obtained; returns an empty List if no FormInfo is available on the
     * device.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool GetAllFormsInfo(std::vector<FormInfo> &formInfos) override;
    /**
     * @brief  Obtains the FormInfo objects provided by a specified application on the device.
     * @param  bundleName Indicates the bundle name of the application.
     * @param  formInfos List of FormInfo objects if obtained; returns an empty List if no FormInfo is available on the
     * device.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool GetFormsInfoByApp(const std::string &bundleName, std::vector<FormInfo> &formInfos) override;
    /**
     * @brief  Obtains the FormInfo objects provided by a specified module name.
     * @param  bundleName Indicates the bundle name of the application.
     * @param  moduleName Indicates the module name of the application.
     * @param  formInfos List of FormInfo objects if obtained; returns an empty List if no FormInfo is available on the
     * device.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool GetFormsInfoByModule(
        const std::string &bundleName, const std::string &moduleName, std::vector<FormInfo> &formInfos) override;
    /**
     * @brief Obtains the ShortcutInfo objects provided by a specified application on the device.
     * @param bundleName Indicates the bundle name of the application.
     * @param shortcutInfos List of ShortcutInfo objects if obtained.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool GetShortcutInfos(const std::string &bundleName, std::vector<ShortcutInfo> &shortcutInfos) override;

    /**
     * @brief Obtains the ShortcutInfo objects provided by a specified application on the device.
     * @param bundleName Indicates the bundle name of the application.
     * @param shortcutInfos List of ShortcutInfo objects if obtained.
     * @return Returns err code of result.
     */
    virtual ErrCode GetShortcutInfoV9(const std::string &bundleName,
        std::vector<ShortcutInfo> &shortcutInfos, int32_t userId = Constants::UNSPECIFIED_USERID) override;

    /**
     * @brief Obtains the ShortcutInfo objects provided by a specified application on the device.
     * @param bundleName Indicates the bundle name of the application.
     * @param shortcutInfos List of ShortcutInfo objects if obtained.
     * @param appIndex Indicates the app index of clone applications.
     * @param userId Indicates the user id.
     * @return Returns err code of result.
     */
    virtual ErrCode GetShortcutInfoByAppIndex(const std::string &bundleName, const int32_t appIndex,
        std::vector<ShortcutInfo> &shortcutInfos) override;

    /**
     * @brief Obtains the ShortcutInfo objects provided by a specified ability.
     * @param bundleName Indicates the bundle name of the application.
     * @param moduleName Indicates the module name.
     * @param abilityName Indicates the host ability name.
     * @param userId Indicates the user ID.
     * @param appIndex Indicates the app index of clone applications.
     * @param shortcutInfos List of ShortcutInfo objects if obtained.
     * @return Returns err code of result.
     */
    virtual ErrCode GetShortcutInfoByAbility(const std::string &bundleName,
        const std::string &moduleName, const std::string &abilityName,
        int32_t userId, int32_t appIndex, std::vector<ShortcutInfo> &shortcutInfos) override;

    /**
     * @brief Obtains the CommonEventInfo objects provided by an event key on the device.
     * @param eventKey Indicates the event of the subscribe.
     * @param commonEventInfos List of CommonEventInfo objects if obtained.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool GetAllCommonEventInfo(const std::string &eventKey,
        std::vector<CommonEventInfo> &commonEventInfos) override;
    /**
     * @brief Obtains the DistributedBundleInfo based on a given bundle name and networkId.
     * @param networkId Indicates the networkId of remote device.
     * @param bundleName Indicates the application bundle name to be queried.
     * @param distributedBundleInfo Indicates the obtained DistributedBundleInfo object.
     * @return Returns true if the DistributedBundleInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetDistributedBundleInfo(const std::string &networkId, const std::string &bundleName,
        DistributedBundleInfo &distributedBundleInfo) override;
    /**
     * @brief Get app privilege level.
     * @param bundleName Indicates the bundle name of the app privilege level.
     * @param userId Indicates the user id.
     * @return Returns app privilege level.
     */
    virtual std::string GetAppPrivilegeLevel(
        const std::string &bundleName, int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Query extension info.
     * @param Want Indicates the information of extension info.
     * @param flag Indicates the query flag which will fliter any specified stuff in the extension info.
     * @param userId Indicates the userId in the system.
     * @param extensionInfos Indicates the obtained extensions.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool QueryExtensionAbilityInfos(const Want &want, const int32_t &flag, const int32_t &userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos) override;
    /**
     * @brief Query extension info.
     * @param Want Indicates the information of extension info.
     * @param flags Indicates the query flag which will filter any specified stuff in the extension info.
     * @param userId Indicates the userId in the system.
     * @param extensionInfos Indicates the obtained extensions.
     * @return Returns ERR_OK if this function is successfully called; returns errCode otherwise.
     */
    virtual ErrCode QueryExtensionAbilityInfosV9(const Want &want, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos) override;
    /**
     * @brief Query extension info.
     * @param Want Indicates the information of extension info.
     * @param extensionType Indicates the type of the extension.
     * @param flag Indicates the query flag which will fliter any specified stuff in the extension info.
     * @param userId Indicates the userId in the system.
     * @param extensionInfos Indicates the obtained extensions.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool QueryExtensionAbilityInfos(const Want &want, const ExtensionAbilityType &extensionType,
        const int32_t &flag, const int32_t &userId, std::vector<ExtensionAbilityInfo> &extensionInfos) override;
    /**
     * @brief Query extension info.
     * @param Want Indicates the information of extension info.
     * @param extensionType Indicates the type of the extension.
     * @param flags Indicates the query flag which will filter any specified stuff in the extension info.
     * @param userId Indicates the userId in the system.
     * @param extensionInfos Indicates the obtained extensions.
     * @return Returns ERR_OK if this function is successfully called; returns errCode otherwise.
     */
    virtual ErrCode QueryExtensionAbilityInfosV9(const Want &want, const ExtensionAbilityType &extensionType,
        int32_t flags, int32_t userId, std::vector<ExtensionAbilityInfo> &extensionInfos) override;
    virtual bool QueryExtensionAbilityInfos(const ExtensionAbilityType &extensionType, const int32_t &userId,
        std::vector<ExtensionAbilityInfo> &extensionInfos) override;

    virtual bool VerifyCallingPermission(const std::string &permission) override;

    virtual bool QueryExtensionAbilityInfoByUri(const std::string &uri, int32_t userId,
        ExtensionAbilityInfo &extensionAbilityInfo) override;

    virtual bool ImplicitQueryInfoByPriority(const Want &want, int32_t flags, int32_t userId,
        AbilityInfo &abilityInfo, ExtensionAbilityInfo &extensionInfo) override;

    virtual bool ImplicitQueryInfos(const Want &want, int32_t flags, int32_t userId, bool withDefault,
        std::vector<AbilityInfo> &abilityInfos, std::vector<ExtensionAbilityInfo> &extensionInfos,
        bool &findDefaultApp) override;

    /**
     * @brief Obtains the AbilityInfo based on a given bundle name through the proxy object.
     * @param bundleName Indicates the bundle name to be queried.
     * @param abilityName Indicates the ability name to be queried.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetAbilityInfo(
        const std::string &bundleName, const std::string &abilityName, AbilityInfo &abilityInfo) override;

    /**
     * @brief Obtains the AbilityInfo based on a given bundle name through the proxy object.
     * @param bundleName Indicates the bundle name to be queried.
     * @param moduleName Indicates the module name to be queried.
     * @param abilityName Indicates the ability name to be queried.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @return Returns true if the AbilityInfo is successfully obtained; returns false otherwise.
     */
    virtual bool GetAbilityInfo(
        const std::string &bundleName, const std::string &moduleName,
        const std::string &abilityName, AbilityInfo &abilityInfo) override;

    virtual ErrCode GetSandboxBundleInfo(
        const std::string &bundleName, int32_t appIndex, int32_t userId, BundleInfo &info) override;
    /**
     * @brief Obtains sandbox data directory by bundleName and appIndex.
     * @param bundleName Indicates the bundle name of the sandbox application to be install.
     * @param appIndex Indicates application index of the sandbox application.
     * @return Returns ERR_OK if the get sandbox data dir successfully; returns errcode otherwise.
     */
    virtual ErrCode GetSandboxDataDir(
        const std::string &bundleName, int32_t appIndex, std::string &sandboxDataDir) override;
    /**
     * @brief Obtains the value of isRemovable based on a given bundle name through the proxy object.
     * @param bundleName Indicates the bundle name to be queried.
     * @param moduleName Indicates the module name to be queried.
     * @param isRemovable Indicates the module whether is removable.
     * @return Returns true if the isRemovable is successfully obtained; returns false otherwise.
     */
    virtual ErrCode IsModuleRemovable(const std::string &bundleName, const std::string &moduleName,
        bool &isRemovable) override;
    /**
     * @brief Sets whether to enable isRemovable based on a given bundle name through the proxy object.
     * @param bundleName Indicates the bundle name to be queried.
     * @param moduleName Indicates the module name to be queried.
     * @param isEnable Specifies whether to enable the isRemovable of InnerModuleInfo.
     *                 The value true means to enable it, and the value false means to disable it
     * @return Returns true if the isRemovable is successfully obtained; returns false otherwise.
     */
    virtual bool SetModuleRemovable(
        const std::string &bundleName, const std::string &moduleName, bool isEnable) override;

    /**
     * @brief Obtains the dependent module names.
     *
     * @param bundleName Indicates the bundle name to be queried.
     * @param moduleName Indicates the module name to be queried.
     * @param dependentModuleNames Indicates the obtained dependent module names.
     * @return Returns true if this function is successfully called; returns false otherwise.
     */
    virtual bool GetAllDependentModuleNames(const std::string &bundleName, const std::string &moduleName,
        std::vector<std::string> &dependentModuleNames) override;
    /**
     * @brief Obtains the value of upgradeFlag based on a given bundle name through the proxy object.
     * @param bundleName Indicates the bundle name to be queried.
     * @param moduleName Indicates the module name to be queried.
     * @return Returns true if the upgradeFlag is successfully obtained; returns false otherwise.
     */
    virtual bool GetModuleUpgradeFlag(const std::string &bundleName, const std::string &moduleName) override;
    /**
     * @brief Sets whether to enable upgradeFlag based on a given bundle name through the proxy object.
     * @param bundleName Indicates the bundle name to be queried.
     * @param moduleName Indicates the module name to be queried.
     * @param isEnable Specifies whether to enable the upgradeFlag of InnerModuleInfo.
     *                 The value true means to enable it, and the value false means to disable it
     * @return Returns ERR_OK if the upgradeFlag is successfully obtained; returns other ErrCode otherwise.
     */
    virtual ErrCode SetModuleUpgradeFlag(
        const std::string &bundleName, const std::string &moduleName, int32_t upgradeFlag) override;
    /**
     * @brief Process preload when ability or extensionAbility is running.
     * @param want Indicates the information of the ability.
     * @param preload Specifies whether to preload modules in atomicService.
     */
    virtual bool ProcessPreload(const Want &want) override;

    virtual ErrCode GetCloneAppIndexes(const std::string &bundleName, std::vector<int32_t> &appIndexes,
        int32_t userId) override;

    virtual ErrCode QueryCloneExtensionAbilityInfoWithAppIndex(const ElementName &elementName,
        int32_t flags, int32_t appIndex, ExtensionAbilityInfo &extensionAbilityInfo, int32_t userId) override;

    virtual bool ObtainCallingBundleName(std::string &bundleName) override;

    virtual bool GetBundleStats(const std::string &bundleName, int32_t userId,
        std::vector<int64_t> &bundleStats, int32_t appIndex = 0, uint32_t statFlag = 0) override;

    virtual ErrCode GetTopNLargestItemsInAppDataDir(const std::string &bundleName, const int32_t appIndex,
        const int32_t userId, const sptr<IGetLargestItemsCallback> getLargestItemsCallback) override;

    virtual ErrCode BatchGetBundleStats(const std::vector<std::string> &bundleNames, int32_t userId,
        std::vector<BundleStorageStats> &bundleStats) override;

    virtual bool GetAllBundleStats(int32_t userId, std::vector<int64_t> &bundleStats) override;

    virtual ErrCode GetBundleInodeCount(const std::string &bundleName, int32_t appIndex, int32_t userId,
        uint64_t &inodeCount) override;

    virtual ErrCode GetAllBundleCacheStat(const sptr<IProcessCacheCallback> processCacheCallback) override;

    ErrCode GetAllBundleCacheStatExec(const sptr<IProcessCacheCallback> processCacheCallback);

    virtual ErrCode CleanAllBundleCache(const sptr<IProcessCacheCallback> processCacheCallback) override;

    virtual sptr<IExtendResourceManager> GetExtendResourceManager() override;

    virtual bool CheckAbilityEnableInstall(
        const Want &want, int32_t missionId, int32_t userId, const sptr<IRemoteObject> &callback) override;

    virtual ErrCode GetMediaData(const std::string &bundleName, const std::string &moduleName,
        const std::string &abilityName, std::unique_ptr<uint8_t[]> &mediaDataPtr, size_t &len,
        int32_t userId = Constants::UNSPECIFIED_USERID) override;

    virtual std::string GetStringById(const std::string &bundleName, const std::string &moduleName,
        uint32_t resId, int32_t userId, const std::string &localeInfo = Constants::EMPTY_STRING) override;

    virtual std::string GetIconById(const std::string &bundleName, const std::string &moduleName,
        uint32_t resId, uint32_t density, int32_t userId) override;

    virtual ErrCode GetAppProvisionInfo(const std::string &bundleName, int32_t userId,
        AppProvisionInfo &appProvisionInfo) override;

    virtual ErrCode GetAllAppProvisionInfo(const int32_t userId,
        std::vector<AppProvisionInfo> &appProvisionInfos) override;

    virtual ErrCode GetAllAppInstallExtendedInfo(
        std::vector<AppInstallExtendedInfo> &appInstallExtendedInfos) override;

    virtual ErrCode GetAllSharedBundleInfo(std::vector<SharedBundleInfo> &sharedBundles) override;
    virtual ErrCode GetSharedBundleInfo(const std::string &bundleName, const std::string &moduleName,
        std::vector<SharedBundleInfo> &sharedBundles) override;

#ifdef BUNDLE_FRAMEWORK_DEFAULT_APP
    virtual sptr<IDefaultApp> GetDefaultAppProxy() override;
#endif

    virtual ErrCode GetSandboxAbilityInfo(const Want &want, int32_t appIndex, int32_t flags, int32_t userId,
        AbilityInfo &info) override;
    virtual ErrCode GetSandboxExtAbilityInfos(const Want &want, int32_t appIndex, int32_t flags, int32_t userId,
        std::vector<ExtensionAbilityInfo> &infos) override;
    virtual ErrCode GetSandboxHapModuleInfo(const AbilityInfo &abilityInfo, int32_t appIndex, int32_t userId,
        HapModuleInfo &info) override;

    virtual sptr<IQuickFixManager> GetQuickFixManagerProxy() override;

#ifdef BUNDLE_FRAMEWORK_APP_CONTROL
    virtual sptr<IAppControlMgr> GetAppControlProxy() override;
#endif

    virtual sptr<IBundleMgrExt> GetBundleMgrExtProxy() override;

    virtual ErrCode SetDebugMode(bool isDebug) override;

    virtual bool VerifySystemApi(int32_t beginApiVersion = Constants::INVALID_API_VERSION) override;

    virtual sptr<IOverlayManager> GetOverlayManagerProxy() override;

    virtual ErrCode GetBaseSharedBundleInfos(const std::string &bundleName,
        std::vector<BaseSharedBundleInfo> &baseSharedBundleInfos,
        GetDependentBundleInfoFlag flag = GetDependentBundleInfoFlag::GET_APP_CROSS_HSP_BUNDLE_INFO) override;

    virtual ErrCode GetSharedBundleInfoBySelf(const std::string &bundleName,
        SharedBundleInfo &sharedBundleInfo) override;

    virtual ErrCode GetSharedDependencies(const std::string &bundleName, const std::string &moduleName,
        std::vector<Dependency> &dependencies) override;

    virtual ErrCode GetAllProxyDataInfos(
        std::vector<ProxyData> &proxyDatas, int32_t userId = Constants::UNSPECIFIED_USERID) override;

    virtual ErrCode GetProxyDataInfos(const std::string &bundleName, const std::string &moduleName,
        std::vector<ProxyData> &proxyDatas, int32_t userId = Constants::UNSPECIFIED_USERID) override;

    virtual ErrCode GetSpecifiedDistributionType(const std::string &bundleName,
        std::string &specifiedDistributionType) override;

    virtual ErrCode BatchGetSpecifiedDistributionType(const std::vector<std::string> &bundleNames,
        std::vector<BundleDistributionType> &specifiedDistributionTypes) override;

    virtual ErrCode GetAdditionalInfo(const std::string &bundleName,
        std::string &additionalInfo) override;

    virtual ErrCode BatchGetAdditionalInfo(const std::vector<std::string> &bundleNames,
        std::vector<BundleAdditionalInfo> &additionalInfos) override;

    virtual ErrCode GetAdditionalInfoForAllUser(const std::string &bundleName,
        std::string &additionalInfo) override;

    virtual ErrCode SetExtNameOrMIMEToApp(const std::string &bundleName, const std::string &moduleName,
        const std::string &abilityName, const std::string &extName, const std::string &mimeType) override;

    virtual ErrCode DelExtNameOrMIMEToApp(const std::string &bundleName, const std::string &moduleName,
        const std::string &abilityName, const std::string &extName, const std::string &mimeType) override;

    virtual bool QueryDataGroupInfos(const std::string &bundleName, int32_t userId,
        std::vector<DataGroupInfo> &infos) override;

    virtual bool GetGroupDir(const std::string &dataGroupId, std::string &dir) override;

    virtual bool QueryAppGalleryBundleName(std::string &bundleName) override;

    /**
     * @brief Query extension info with type name.
     * @param Want Indicates the information of extension info.
     * @param extensionTypeName Indicates the type of the extension.
     * @param flag Indicates the query flag which will fliter any specified stuff in the extension info.
     * @param userId Indicates the userId in the system.
     * @param extensionInfos Indicates the obtained extensions.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode QueryExtensionAbilityInfosWithTypeName(const Want &want, const std::string &extensionTypeName,
        const int32_t flag, const int32_t userId, std::vector<ExtensionAbilityInfo> &extensionInfos) override;

    /**
     * @brief Query extension info only with type name.
     * @param extensionTypeName Indicates the type of the extension.
     * @param flag Indicates the query flag which will fliter any specified stuff in the extension info.
     * @param userId Indicates the userId in the system.
     * @param extensionInfos Indicates the obtained extensions.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode QueryExtensionAbilityInfosOnlyWithTypeName(const std::string &extensionTypeName,
        const uint32_t flag, const int32_t userId, std::vector<ExtensionAbilityInfo> &extensionInfos) override;

    virtual ErrCode ResetAOTCompileStatus(const std::string &bundleName, const std::string &moduleName,
        int32_t triggerMode) override;

    virtual ErrCode GetJsonProfile(ProfileType profileType, const std::string &bundleName,
        const std::string &moduleName, std::string &profile, int32_t userId = Constants::UNSPECIFIED_USERID) override;

    virtual sptr<IBundleResource> GetBundleResourceProxy() override;

    virtual sptr<IBundleSkillManager> GetSkillManagerProxy() override;

    virtual ErrCode GetRecoverableApplicationInfo(
        std::vector<RecoverableApplicationInfo> &recoverableApplications) override;

    virtual ErrCode GetUninstalledBundleInfo(const std::string bundleName, BundleInfo &bundleInfo) override;

    virtual ErrCode SetAdditionalInfo(const std::string &bundleName, const std::string &additionalInfo) override;

    virtual ErrCode CreateBundleDataDir(int32_t userId) override;

    virtual ErrCode CreateBundleDataDirWithEl(int32_t userId, DataDirEl dirEl) override;

    virtual ErrCode UpdateAppEncryptedStatus(const std::string &bundleName,
        bool isExisted, int32_t appIndex = 0) override;

    virtual ErrCode MigrateData(
        const std::vector<std::string> &sourcePaths, const std::string &destinationPath) override;

    /**
     * @brief Check whether the link can be opened.
     * @param link Indicates the link to be opened.
     * @param canOpen Indicates whether the link can be opened.
     * @return Returns result of the operation.
     */
    virtual ErrCode CanOpenLink(
        const std::string &link, bool &canOpen) override;

    /**
     * @brief Get odid of the current application.
     * @param odid Indicates the odid of application.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode GetOdid(std::string &odid) override;

    /**
     * @brief Obtains BundleInfo of all bundles available in the system through the developerId.
     * @param developerId Indicates the developerId of application.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode GetAllBundleInfoByDeveloperId(const std::string &developerId,
        std::vector<BundleInfo> &bundleInfos, int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Obtains all developerId list through the specified distribution type of application.
     * @param appDistributionType Indicates the distribution type of application.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode GetDeveloperIds(const std::string &appDistributionType,
        std::vector<std::string> &developerIdList, int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
     * @brief Switch uninstall state of a specified application.
     * @param bundleName Indicates the bundle name of the application.
     * @param state Indicates whether the specified application can be uninstalled.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode SwitchUninstallState(const std::string &bundleName, const bool &state,
        bool isNeedSendNotify = true) override;

    /**
     * @brief Query the AbilityInfo by continueType.
     * @param bundleName Indicates the bundle name of the application.
     * @param continueType Indicates the continue type of the ability.
     * @param abilityInfo Indicates the obtained AbilityInfo object.
     * @param userId Indicates the information of the user.
     * @return Returns true if the AbilityInfos is successfully obtained; returns false otherwise.
     */
    virtual ErrCode QueryAbilityInfoByContinueType(const std::string &bundleName, const std::string &continueType,
        AbilityInfo &abilityInfo, int32_t userId = Constants::UNSPECIFIED_USERID) override;

    /**
     * @brief Get preinstalled application infos.
     * @param preinstalledApplicationInfos Indicates all of the obtained PreinstalledApplicationInfo objects.
     * @return Returns ERR_OK if this function is successfully called; returns other ErrCode otherwise.
     */
    virtual ErrCode GetAllPreinstalledApplicationInfos(
        std::vector<PreinstalledApplicationInfo> &preinstalledApplicationInfos) override;

    virtual ErrCode GetAllNewPreinstalledApplicationInfos(
        std::vector<PreinstalledApplicationInfo> &preinstalledApplicationInfos) override;

    virtual ErrCode QueryCloneAbilityInfo(const ElementName &element,
        int32_t flags, int32_t appIndex, AbilityInfo &abilityInfo, int32_t userId) override;

    virtual ErrCode GetCloneBundleInfo(const std::string &bundleName, int32_t flag, int32_t appIndex,
        BundleInfo &bundleInfo, int32_t userId = Constants::UNSPECIFIED_USERID) override;

    virtual ErrCode GetCloneBundleInfoExt(const std::string &bundleName, uint32_t flags, int32_t appIndex,
        int32_t userId, BundleInfo &bundleInfo) override;

    virtual ErrCode GetLaunchWant(Want &want) override;

    virtual ErrCode GetSignatureInfoByBundleName(const std::string &bundleName, SignatureInfo &signatureInfo) override;

    virtual ErrCode GetOdidByBundleName(const std::string &bundleName, std::string &odid) override;

    virtual ErrCode GetOdidResetCount(const std::string &bundleName, std::string &odid, int32_t &count) override;

    virtual ErrCode GetSignatureInfoByUid(const int32_t uid, SignatureInfo &signatureInfo) override;

    virtual ErrCode GetApiTargetVersionByUid(const int32_t uid, int32_t &apiTargetVersion) override;

    virtual ErrCode AddDesktopShortcutInfo(const ShortcutInfo &shortcutInfo, int32_t userId) override;

    virtual ErrCode DeleteDesktopShortcutInfo(const ShortcutInfo &shortcutInfo, int32_t userId) override;

    virtual ErrCode GetAllDesktopShortcutInfo(int32_t userId, std::vector<ShortcutInfo> &shortcutInfos) override;

    /**
     * @brief Obtains BundleInfo of all continuable bundles available in the system through the proxy object.
     * @param flag Indicates the flag used to specify information contained in the BundleInfo that will be returned.
     * @param bundleInfos Indicates all of the obtained BundleInfo objects.
     * @param userId Indicates the user ID.
     * @return Returns true if the BundleInfos is successfully obtained; returns false otherwise.
     */
    virtual bool GetBundleInfosForContinuation(int32_t flags, std::vector<BundleInfo> &bundleInfos,
        int32_t userId = Constants::UNSPECIFIED_USERID) override;
    /**
    * @brief Obtains all bundle names of a specified user.
    * @param flags Indicates the flags to control the bundle list.
    * @param userId Indicates the user ID.
    * @param bundleNames Indicates the vector of the bundle names.
    * @param withExtBundle Indicates whether to include the extension bundle.
    * @return Returns ERR_OK if the operation is successful; returns other error codes otherwise.
    */
    virtual ErrCode GetAllBundleNames(const uint32_t flags, int32_t userId, bool withExtBundle,
        std::vector<std::string> &bundleNames) override;

    /**
     * @brief Get a list of application package names that continue the specified package name.
     * @param continueBundleName The package name that is being continued.
     * @param bundleNames Continue the list of specified package names.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if successfully obtained; returns error code otherwise.
     */
    ErrCode GetContinueBundleNames(const std::string &continueBundleName, std::vector<std::string> &bundleNames,
        int32_t userId = Constants::UNSPECIFIED_USERID) override;

    virtual ErrCode IsBundleInstalled(const std::string &bundleName, int32_t userId,
        int32_t appIndex, bool &isInstalled) override;

    virtual ErrCode GetCompatibleDeviceType(const std::string &bundleName, std::string &deviceType) override;

    virtual ErrCode BatchGetCompatibleDeviceType(const std::vector<std::string> &bundleNames,
        std::vector<BundleCompatibleDeviceType> &compatibleDeviceTypes) override;

    virtual ErrCode GetBundleNameByAppId(const std::string &appId, std::string &bundleName) override;

    virtual ErrCode GetAllPluginInfo(const std::string &hostBundleName, int32_t userId,
        std::vector<PluginBundleInfo> &pluginBundleInfos) override;

    virtual ErrCode GetPluginInfosForSelf(std::vector<PluginBundleInfo> &pluginBundleInfos) override;

    virtual ErrCode RegisterPluginEventCallback(const sptr<IBundleEventCallback> pluginEventCallback) override;

    virtual ErrCode UnregisterPluginEventCallback(const sptr<IBundleEventCallback> pluginEventCallback) override;

    virtual ErrCode GetDirByBundleNameAndAppIndex(const std::string &bundleName, const int32_t appIndex,
        std::string &dataDir) override;

    virtual ErrCode GetAllBundleDirs(int32_t userId, std::vector<BundleDir> &bundleDirs) override;

    virtual ErrCode SetAppDistributionTypes(std::set<AppDistributionTypeEnum> &appDistributionTypeEnums) override;

    virtual ErrCode GetPluginAbilityInfo(const std::string &hostBundleName, const std::string &pluginBundleName,
        const std::string &pluginModuleName, const std::string &pluginAbilityName,
        const int32_t userId, AbilityInfo &abilityInfo) override;

    virtual ErrCode GetPluginHapModuleInfo(const std::string &hostBundleName, const std::string &pluginBundleName,
        const std::string &pluginModuleName, const int32_t userId, HapModuleInfo &hapModuleInfo) override;

    virtual ErrCode SetShortcutVisibleForSelf(const std::string &shortcutId, bool visible) override;

    virtual ErrCode SetShortcutsEnabled(const std::vector<ShortcutInfo> &shortcutInfos, bool isEnabled) override;

    virtual bool GreatOrEqualTargetAPIVersion(const int32_t platformVersion,
        const int32_t minorVersion, const int32_t patchVersion) override;

    virtual ErrCode GetAllShortcutInfoForSelf(std::vector<ShortcutInfo> &shortcutInfos) override;

    virtual ErrCode GetAlternateIcons(std::vector<AlternateIconInfo> &alternateIcons) override;

    virtual ErrCode AddDynamicShortcutInfos(const std::vector<ShortcutInfo> &shortcutInfos, int32_t userId) override;

    virtual ErrCode DeleteDynamicShortcutInfos(const std::string &bundleName, const int32_t appIndex,
        const int32_t userId, const std::vector<std::string> &ids) override;

    virtual ErrCode GetPluginInfo(const std::string &hostBundleName, const std::string &pluginBundleName,
        const int32_t userId, PluginBundleInfo &pluginBundleInfo) override;

    virtual ErrCode GetTestRunner(const std::string &bundleName, const std::string &moduleName,
        ModuleTestRunner &testRunner) override;

    virtual ErrCode SwitchUninstallStateByUserId(const std::string &bundleName, const bool state,
        int32_t userId) override;

    virtual ErrCode SetAbilityFileTypesForSelf(const std::string &moduleName, const std::string &abilityName,
        const std::vector<std::string> &fileTypes) override;
    
    virtual ErrCode RecoverBackupBundleData(const std::string &bundleName,
        const int32_t userId, const int32_t appIndex) override;

    virtual ErrCode GetPluginBundlePathForSelf(
        const std::string &pluginBundleName, std::string &codePath) override;

    virtual ErrCode RemoveBackupBundleData(const std::string &bundleName,
        const int32_t userId, const int32_t appIndex) override;

    virtual ErrCode CreateNewBundleDir(int32_t userId) override;

    /**
     * @brief Obtains all JSON profile info designated by profileType and userId.
     * It does not support querying JSON profile info for disabled applications.
     * only support profileType: EASY_GO_PROFILE/SHARE_FILES_PROFILE
     * @param profileType Indicates the profile type.
     * @param userId Indicates the user ID.
     * @return Returns ERR_OK if successfully obtained; returns error code otherwise.
     */
    virtual ErrCode GetAllJsonProfile(ProfileType profileType, int32_t userId,
        std::vector<JsonProfileInfo> &profileInfos) override;

    virtual ErrCode GetBundleInstallStatus(const std::string &bundleName, const int32_t userId,
        BundleInstallStatus &bundleInstallStatus) override;

    virtual ErrCode GetPluginExtensionInfo(const std::string &hostBundleName,
        const Want &want, const int32_t userId, ExtensionAbilityInfo &extensionInfo) override;

    virtual ErrCode IsApplicationDisableForbidden(const std::string &bundleName, int32_t userId, int32_t appIndex,
        bool &forbidden) override;

    virtual ErrCode SetApplicationDisableForbidden(const std::string &bundleName, int32_t userId, int32_t appIndex,
        bool forbidden) override;
private:
    /**
     * @brief Send a command message from the proxy object.
     * @param code Indicates the message code to be sent.
     * @param data Indicates the objects to be sent.
     * @param reply Indicates the reply to be sent;
     * @return Returns true if message send successfully; returns false otherwise.
     */
    bool SendTransactCmd(BundleMgrInterfaceCode code, MessageParcel &data, MessageParcel &reply);
    ErrCode SendTransactCmdWithErrCode(BundleMgrInterfaceCode code, MessageParcel &data, MessageParcel &reply);
    ErrCode SendTransactCmdWithLogErrCode(BundleMgrInterfaceCode code, MessageParcel &data, MessageParcel &reply);
    /**
     * @brief Send a command message from the proxy object and  printf log.
     * @param code Indicates the message code to be sent.
     * @param data Indicates the objects to be sent.
     * @param reply Indicates the reply to be sent;
     * @return Returns true if message send successfully; returns false otherwise.
     */
    bool SendTransactCmdWithLog(BundleMgrInterfaceCode code, MessageParcel &data, MessageParcel &reply);

    /**
     * @brief Send a command message and then get a parcelable information object from the reply.
     * @param code Indicates the message code to be sent.
     * @param data Indicates the objects to be sent.
     * @param parcelableInfo Indicates the object to be got;
     * @return Returns true if objects get successfully; returns false otherwise.
     */
    template <typename T>
    bool GetParcelableInfo(BundleMgrInterfaceCode code, MessageParcel &data, T &parcelableInfo);

    template <typename T>
    ErrCode GetParcelableInfoWithErrCode(BundleMgrInterfaceCode code, MessageParcel &data, T &parcelableInfo);

    template <typename T>
    ErrCode GetParcelableInfoWithErrCodeReply(
        BundleMgrInterfaceCode code, MessageParcel &data, MessageParcel &reply, T &parcelableInfo);

    /**
     * @brief Send a command message and then get a vector of parcelable information objects from the reply.
     * @param code Indicates the message code to be sent.
     * @param data Indicates the objects to be sent.
     * @param parcelableInfos Indicates the vector objects to be got;
     * @return Returns true if the vector get successfully; returns false otherwise.
     */
    template <typename T>
    bool GetParcelableInfos(BundleMgrInterfaceCode code, MessageParcel &data, std::vector<T> &parcelableInfos);

    template <typename T>
    ErrCode GetParcelableInfosWithErrCode(BundleMgrInterfaceCode code, MessageParcel &data,
        std::vector<T> &parcelableInfos);

    template<typename T>
    bool GetVectorFromParcelIntelligent(
        BundleMgrInterfaceCode code, MessageParcel &data, std::vector<T> &parcelableInfos);

    template<typename T>
    ErrCode GetVectorFromParcelIntelligentWithErrCode(
        BundleMgrInterfaceCode code, MessageParcel &data, std::vector<T> &parcelableInfos);

    template<typename T>
    ErrCode InnerGetVectorFromParcelIntelligent(MessageParcel &reply, std::vector<T> &parcelableInfos);

    template<typename T>
    ErrCode GetParcelInfo(BundleMgrInterfaceCode code, MessageParcel &data, T &parcelInfo);

    template<typename T>
    ErrCode InnerGetParcelInfo(MessageParcel &reply, T &parcelInfo);

    template<typename T>
    ErrCode GetParcelInfoIntelligent(BundleMgrInterfaceCode code, MessageParcel &data, T &parcelInfo);

    template<typename T>
    ErrCode GetParcelInfoIntelligentWithReply(BundleMgrInterfaceCode code, MessageParcel &data,
        MessageParcel &reply, T &parcelInfo);

    ErrCode GetBigString(BundleMgrInterfaceCode code, MessageParcel &data, std::string &result);

    ErrCode InnerGetBigString(MessageParcel &reply, std::string &result);

    ErrCode GetMediaDataFromAshMem(MessageParcel &reply, std::unique_ptr<uint8_t[]> &mediaDataPtr, size_t &len);
    BUNDLE_HIDDEN static inline BrokerDelegator<BundleMgrProxy> delegator_;

    template<typename T>
    ErrCode WriteParcelInfoIntelligent(const T &parcelInfo, MessageParcel &reply) const;

    template<typename T>
    ErrCode WriteVectorToParcel(const std::vector<T> &parcelVector, MessageParcel &reply);

    ErrCode GetParcelInfoFromAshMem(MessageParcel &reply, void *&data);

    static void StopCache();
};

}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_MGR_PROXY_H
