/*
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

/**
 * @addtogroup AccessToken
 * @{
 *
 * @brief Provides permission management.
 *
 * Provides tokenID-based application permission verification mechanism.
 * When an application accesses sensitive data or APIs, this module can check
 * whether the application has the corresponding permission. Allows applications
 * to query their access token information or APL levcels based on token IDs.
 *
 * @since 7.0
 * @version 7.0
 */

/**
 * @file accesstoken_kit.h
 *
 * @brief Declares access token interfaces.
 *
 * @since 7.0
 * @version 7.0
 */

#ifndef INTERFACES_INNER_KITS_ACCESSTOKEN_KIT_H
#define INTERFACES_INNER_KITS_ACCESSTOKEN_KIT_H

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "access_token.h"
#include "atm_tools_param_info.h"
#include "claw_permission_info.h"
#include "hap_token_info.h"
#include "native_token_info.h"
#include "permission_def.h"
#include "permission_list_state.h"
#include "permission_grant_info.h"
#include "permission_state_change_info.h"
#include "permission_state_full.h"
#include "perm_state_change_callback_customize.h"
#ifdef SECURITY_COMPONENT_ENHANCE_ENABLE
#include "sec_comp_enhance_data.h"
#endif
#ifdef TOKEN_SYNC_ENABLE
#include "token_sync_kit_interface.h"
#endif // TOKEN_SYNC_ENABLE

namespace OHOS {
namespace Security {
namespace AccessToken {
/**
 * @brief Declares AccessTokenKit class
 */
class AccessTokenKit {
public:
    /**
     * @brief Get permission used type by tokenID.
     * @param tokenID token id
     * @param permissionName permission to be checked
     * @return enum PermUsedTypeEnum, see access_token.h
     */
    static PermUsedTypeEnum GetPermissionUsedType(AccessTokenID tokenID, const std::string& permissionName);

    /**
     * @brief Grant input permission to input tokenID flag for specified time.
     * @param tokenID token id
     * @param permissionName permission name quote
     * @param onceTime the time it takes to work, the unit is second.
     * @return error code, see access_token_error.h
     */
    static int GrantPermissionForSpecifiedTime(
        AccessTokenID tokenID, const std::string& permissionName, uint32_t onceTime);

    /**
     * @brief Create a unique hap token by input values.
     * @param info struct HapInfoParams quote, see hap_token_info.h
     * @param policy struct HapPolicyParams quote, see hap_token_info.h
     * @return union AccessTokenIDEx, see access_token.h
     */
    static AccessTokenIDEx AllocHapToken(const HapInfoParams& info, const HapPolicyParams& policy);
    /**
     * @brief Create a unique hap token by input values and init the permission state.
     * @param info struct HapInfoParams quote, see hap_token_info.h
     * @param policy struct HapPolicyParams quote, see hap_token_info.h
     * @return union AccessTokenIDEx, see access_token.h
     */
    static int32_t InitHapToken(const HapInfoParams& info, HapPolicyParams& policy, AccessTokenIDEx& fullTokenId);
    /**
     * @brief Create a unique hap token by input values and init the permission state.
     * @param info struct HapInfoParams quote, see hap_token_info.h
     * @param policy struct HapPolicyParams quote, see hap_token_info.h
     * @param result struct HapInfoCheckResult, see hap_token_info.h
     * @return union AccessTokenIDEx, see access_token.h
     */
    static int32_t InitHapToken(const HapInfoParams& info, HapPolicyParams& policy,
        AccessTokenIDEx& fullTokenId, HapInfoCheckResult& result);
    /**
     * @brief Create a unique mapping token binding remote tokenID and DeviceID.
     * @param remoteDeviceID remote device deviceID
     * @param remoteTokenID remote device tokenID
     * @return local tokenID which mapped by local token
     */
    static FullTokenID AllocLocalTokenID(const std::string& remoteDeviceID, AccessTokenID remoteTokenID);
    /**
     * @brief Update hap token info.
     * @param tokenIdEx union AccessTokenIDEx quote, see access_token.h
     * @param isSystemApp is system app or not
     * @param appIDDesc app id description quote
     * @param apiVersion app api version
     * @param policy struct HapPolicyParams quote, see hap_token_info.h
     * @return error code, see access_token_error.h
     */
    static int32_t UpdateHapToken(
        AccessTokenIDEx& tokenIdEx, const UpdateHapInfoParams& info, const HapPolicyParams& policy);
    /**
     * @brief Update hap token info.
     * @param tokenIdEx union AccessTokenIDEx quote, see access_token.h
     * @param isSystemApp is system app or not
     * @param appIDDesc app id description quote
     * @param apiVersion app api version
     * @param policy struct HapPolicyParams quote, see hap_token_info.h
     * @param result struct HapInfoCheckResult, see hap_token_info.h
     * @return error code, see access_token_error.h
     */
    static int32_t UpdateHapToken(AccessTokenIDEx& tokenIdEx, const UpdateHapInfoParams& info,
        const HapPolicyParams& policy, HapInfoCheckResult& result);
    /**
     * @brief Delete token info.
     * @param tokenID token id
     * @return error code, see access_token_error.h
     */
    static int DeleteToken(AccessTokenID tokenID);
    /**
     * @brief Delete token info.
     * @param tokenID token id
     * @param isTokenReserved whether to reserve the token
     * @return error code, see access_token_error.h
     */
    static int DeleteToken(AccessTokenID tokenID, bool isTokenReserved);
    /**
     * @brief Delete claw token info.
     * @param pid caller pid
     * @return error code, see access_token_error.h
     */
    static int32_t DeleteToolTokenByPid(int32_t pid);
    /**
     * @brief Get token type by ATM service.
     * @param tokenID token id
     * @return token type enum, see access_token.h
     */
    static ATokenTypeEnum GetTokenType(AccessTokenID tokenID);
    /**
     * @brief Get token type from flag in tokenId, which doesn't depend on ATM service.
     * @param tokenID token id
     * @return token type enum, see access_token.h
     */
    static ATokenTypeEnum GetTokenTypeFlag(AccessTokenID tokenID);
    /**
     * @brief Get token type by ATM service with uint_64 parameters.
     * @param tokenID token id
     * @return token type enum, see access_token.h
     */
    static ATokenTypeEnum GetTokenType(FullTokenID tokenID);
    /**
     * @brief Get token type from flag in tokenId, which doesn't depend
     *        on ATM service, with uint_64 parameters.
     * @param tokenID token id
     * @return token type enum, see access_token.h
     */
    static ATokenTypeEnum GetTokenTypeFlag(FullTokenID tokenID);
    /**
    * @brief Get tool token type from flag in tokenId, which doesn't depend
    *        on ATM service, with uint_64 parameters.
    * @param tokenID token id
    * @return token type enum, see access_token.h
    */
    static bool IsCliToolToken(FullTokenID tokenId);
    /**
     * @brief Get token id by user id.
     * @param userID user id
     * @param tokenIdList token id list
     * @return error code, see access_token_error.h
     */
    static int32_t GetTokenIDByUserID(int32_t userID, std::unordered_set<AccessTokenID>& tokenIdList);
    /**
     * @brief Query hap tokenID by input prarms.
     * @param userID user id
     * @param bundleName bundle name
     * @param instIndex inst index
     * @return token id if exsit or 0 if not exsit
     */
    static AccessTokenID GetHapTokenID(int32_t userID, const std::string& bundleName, int32_t instIndex);
    /**
     * @brief Query hap token attribute by input prarms.
     * @param userID user id
     * @param bundleName bundle name
     * @param instIndex inst index
     * @return union AccessTokenIDEx, see access_token.h
     */
    static AccessTokenIDEx GetHapTokenIDEx(int32_t userID, const std::string& bundleName, int32_t instIndex);
    /**
     * @brief Get hap token info by token id.
     * @param tokenID token id
     * @param hapTokenInfoRes HapTokenInfo quote, as query result
     * @return error code, see access_token_error.h
     */
    static int GetHapTokenInfo(AccessTokenID tokenID, HapTokenInfo& hapTokenInfoRes);
    /**
     * @brief Get native token info by token id.
     * @param tokenID token id
     * @param nativeTokenInfoRes NativeTokenInfo quote, as query result
     * @return error code, see access_token_error.h
     */
    static int GetNativeTokenInfo(AccessTokenID tokenID, NativeTokenInfo& nativeTokenInfoRes);
    /**
     * @brief Check if the input tokenID has been granted the input permission.
     * @param tokenID token id
     * @param permissionName permission to be checked
     * @return enum PermissionState, see access_token.h
     */
    static int VerifyAccessToken(AccessTokenID tokenID, const std::string& permissionName);
    /**
     * @brief Check if the input tokenID has been granted the input permission.
     * @param callerTokenID the tokenID of caller native process or hap process
     * @param firstTokenID the tokenID of first caller native process or hap process
     * @param permissionName permission to be checked
     * @return enum PermissionState, see access_token.h
     */
    static int VerifyAccessToken(
        AccessTokenID callerTokenID, AccessTokenID firstTokenID, const std::string& permissionName);
    /**
     * @brief Check if the input tokenID has been granted the input permission.
     * @param tokenID token id
     * @param permissionName permission to be checked
     * @param crossIpc whether to cross ipc
     * @return enum PermissionState, see access_token.h
     */
    static int VerifyAccessToken(AccessTokenID tokenID, const std::string& permissionName, bool crossIpc);
    /**
     * @brief Check if the input tokenID has been granted the input permission.
     * @param callerTokenID the tokenID of caller native process or hap process
     * @param firstTokenID the tokenID of first caller native process or hap process
     * @param permissionName permission to be checked
     * @param crossIpc whether to cross ipc
     * @return enum PermissionState, see access_token.h
     */
    static int VerifyAccessToken(AccessTokenID callerTokenID,
        AccessTokenID firstTokenID, const std::string& permissionName, bool crossIpc);
    /**
     * @brief Check if the input tokenID has been granted the input permission list.
     * @param tokenID token id
     * @param permissionList permission list to be checked
     * @param permStateList enum PermissionState list, as result
     * @param crossIpc whether to cross ipc
     * @return error code, see access_token_error.h
     */
    static int VerifyAccessToken(AccessTokenID tokenID, const std::vector<std::string>& permissionList,
        std::vector<int32_t>& permStateList, bool crossIpc = false);

    /**
     * @brief Get permission definition by permission name.
     * @param permissionName permission name quote
     * @param permissionDefResult PermissionDef quote, as query result
     * @return error code, see access_token_error.h
     */
    static int GetDefPermission(const std::string& permissionName, PermissionDef& permissionDefResult);
    /**
     * @brief Get all requested permission full state by token id and grant mode.
     * @param tokenID token id
     * @param reqPermList PermissionStateFull list quote, as query result
     * @param isSystemGrant grant mode
     * @return error code, see access_token_error.h
     */
    static int GetReqPermissions(
        AccessTokenID tokenID, std::vector<PermissionStateFull>& reqPermList, bool isSystemGrant);
    /**
     * @brief Get permission grant flag
     * @param tokenID token id
     * @param permissionName permission name quote
     * @param flag the permission grant flag, as query result
     * @return error code, see access_token_error.h
     */
    static int GetPermissionFlag(AccessTokenID tokenID, const std::string& permissionName, uint32_t& flag);
    /**
     * @brief Set permission request toggle status
     * @param permissionName permission name quote
     * @param status the permission request toggle status to set
     * @param userID the userID
     * @return error code, see access_token_error.h
     */
    static int32_t SetPermissionRequestToggleStatus(const std::string& permissionName, uint32_t status, int32_t userID);
    /**
     * @brief Get permission request toggle status
     * @param permissionName permission name quote
     * @param status the permission request toggle status to get
     * @param userID the userID
     * @return error code, see access_token_error.h
     */
    static int32_t GetPermissionRequestToggleStatus(const std::string& permissionName, uint32_t& status,
        int32_t userID);
    /**
     * @brief Starts the permission manager page of an application.
     * @param tokenID token id
     * @return error code, see access_token_error.h
     */
    static int32_t RequestAppPermOnSetting(AccessTokenID tokenID);
    /**
     * @brief Get self permission status
     * @param permissionName permission name quote
     * @param status the permission status
     * @return error code, see access_token_error.h
     */
    static int32_t GetSelfPermissionStatus(const std::string& permissionName, PermissionOper& status);
    /**
     * @brief Get requsted permission grant result
     * @param permList PermissionListState list quote, as input and query result
     * @return enum PermissionOper, see access_token.h
     */
    static PermissionOper GetSelfPermissionsState(std::vector<PermissionListState>& permList,
        PermissionGrantInfo& info);
    /**
     * @brief Get requsted permissions status
     * @param permList PermissionListState list quote, as input and query result
     * @return error code, see access_token_error.h
     */
    static int32_t GetPermissionsStatus(AccessTokenID tokenID, std::vector<PermissionListState>& permList);
    /**
     * @brief Grant input permission to input tokenID with input flag.
     * @param tokenID token id
     * @param permissionName permission name quote
     * @param flag enum PermissionFlag, see access_token.h
     * @param grantMode the final grantable type
     * @return error code, see access_token_error.h
     */
    static int GrantPermission(AccessTokenID tokenID,
        const std::string& permissionName, uint32_t flag, UpdatePermissionFlag updateFlag = USER_GRANTED_PERM);
    /**
     * @brief Revoke input permission to input tokenID with input flag.
     * @param tokenID token id
     * @param permissionName permission name quote
     * @param flag enum PermissionFlag, see access_token.h
     * @param grantMode the final revocable type
     * @param killProcess whether to kill the process when revoking permission
     * @return error code, see access_token_error.h
     */
    static int RevokePermission(AccessTokenID tokenID,
        const std::string& permissionName, uint32_t flag,
        UpdatePermissionFlag updateFlag = USER_GRANTED_PERM, bool killProcess = true);
    /**
     * @brief Clear all user granted permissions state in input tokenID.
     * @param tokenID token id
     * @return error code, see access_token_error.h
     */
    static int ClearUserGrantedPermissionState(AccessTokenID tokenID);
    /**
     * @brief Register permission state change callback.
     * @param callback smart point of class PermStateChangeCallbackCustomize quote
     * @return error code, see access_token_error.h
     */
    static int32_t RegisterPermStateChangeCallback(
        const std::shared_ptr<PermStateChangeCallbackCustomize>& callback);
    /**
     * @brief Unregister permission state change callback.
     * @param callback smart point of class PermStateChangeCallbackCustomize quote
     * @return error code, see access_token_error.h
     */
    static int32_t UnRegisterPermStateChangeCallback(const std::shared_ptr<PermStateChangeCallbackCustomize>& callback);
    /**
     * @brief Register permission state change callback for app.
     * @param callback smart point of class PermStateChangeCallbackCustomize quote
     * @return error code, see access_token_error.h
     */
    static int32_t RegisterSelfPermStateChangeCallback(
        const std::shared_ptr<PermStateChangeCallbackCustomize>& callback);
    /**
     * @brief Unregister permission state change callback for app.
     * @param callback smart point of class PermStateChangeCallbackCustomize quote
     * @return error code, see access_token_error.h
     */
    static int32_t UnRegisterSelfPermStateChangeCallback(
        const std::shared_ptr<PermStateChangeCallbackCustomize>& callback);
    /**
     * @brief Get current version.
     * @param version access token version.
     * @return error code, see access_token_error.h
     */
    static int32_t GetVersion(uint32_t& version);
    /**
     * @brief Get hap dlp flag by input tokenID.
     * @param tokenID token id
     * @return dlp flag in tokenID bitmap, or default -1
     */
    static int32_t GetHapDlpFlag(AccessTokenID tokenID);
    /**
     * @brief Reload native token info.
     * @return error code, see access_token_error.h
     */
    static int32_t ReloadNativeTokenInfo();
    /**
     * @brief Get tokenID by native process name.
     * @param processName native process name
     * @return token id of native process
     */
    static AccessTokenID GetNativeTokenId(const std::string& processName);

    /**
     * @brief Get hap token extension info by token id.
     * @param tokenID token id
     * @param info HapTokenInfoExt include appID
     * @return error code, see access_token_error.h
     */
    static int GetHapTokenInfoExtension(AccessTokenID tokenID, HapTokenInfoExt& info);

    /**
     * @brief Set permission dialog capability
     * @param hapBaseInfo base infomation of hap
     * @param enable status of enable dialog
     * @return error code, see access_token_error.h
     */
    static int32_t SetPermDialogCap(const HapBaseInfo& hapBaseInfo, bool enable);

#ifdef TOKEN_SYNC_ENABLE
    /**
     * @brief Get remote hap token info by remote token id.
     * @param tokenID remote token id
     * @param hapSync HapTokenInfoForSync quote, as query result
     * @return error code, see access_token_error.h
     */
    static int GetHapTokenInfoFromRemote(AccessTokenID tokenID, HapTokenInfoForSync& hapSync);

    /**
     * @brief Set remote hap token info with remote deviceID.
     * @param deviceID remote deviceID
     * @param hapSync hap token info to set
     * @return error code, see access_token_error.h
     */
    static int SetRemoteHapTokenInfo(const std::string& deviceID, const HapTokenInfoForSync& hapSync);
    /**
     * @brief Delete remote token by remote deviceID and remote tokenID.
     * @param deviceID remote deviceID
     * @param tokenID remote tokenID
     * @return error code, see access_token_error.h
     */
    static int DeleteRemoteToken(const std::string& deviceID, AccessTokenID tokenID);
    /**
     * @brief Get local mapping native tokenID by remote deviceID
     *        and remote tokenID.
     * @param deviceID remote deviceID
     * @param tokenID remote tokenID
     * @return token id of mapping native tokenID
     */
    static AccessTokenID GetRemoteNativeTokenID(const std::string& deviceID, AccessTokenID tokenID);
    /**
     * @brief Delete remote tokens by remote deviceID.
     * @param deviceID remote deviceID
     * @return error code, see access_token_error.h
     */
    static int DeleteRemoteDeviceTokens(const std::string& deviceID);
    /**
     * @brief Regist a token sync service callback
     * @param syncCallback token sync class
     * @return error code, see access_token_error.h
     */
    static int32_t RegisterTokenSyncCallback(const std::shared_ptr<TokenSyncKitInterface>& syncCallback);
    /**
     * @brief UnRegist a token sync service callback
     * @param syncCallback token sync class
     * @return error code, see access_token_error.h
     */
    static int32_t UnRegisterTokenSyncCallback();
#endif
    /**
     * @brief Dump all token infos in the cache.
     * @param tokenID token id, if tokenID is valid, only dump this token info
     * @param dumpInfo all token info
     */
    static void DumpTokenInfo(const AtmToolsParamInfo& info, std::string& dumpInfo);
    /**
     * @brief Get application info of permission manager.
     * @param info application info of permission manager
     */
    static void GetPermissionManagerInfo(PermissionGrantInfo& info);

    /**
     * @brief Set user permission policy
     * @param userPermissionList list of permission polcy.
     * @return error code, see access_token_error.h
     */
    static int32_t SetUserPolicy(const std::vector<UserPermissionPolicy>& userPermissionList);

    /**
     * @brief Clear user permission policy
     * @param permissionList list of permission.
     * @return error code, see access_token_error.h
     */
    static int32_t ClearUserPolicy(const std::vector<std::string>& permissionList);

    /**
     * @brief Update user policy whitelist.
     * @param tokenId target token id.
     * @param permission permission name.
     * @param type whitelist update type.
     * @return error code, see access_token_error.h
     */
    static int32_t UpdatePolicyWhiteList(
        AccessTokenID tokenId, const std::string& permission, UpdateWhiteListType type);

    /**
     * @brief Get user policy whitelist.
     * @param permission permission name.
     * @param tokenIdList whitelist token id list.
     * @return error code, see access_token_error.h
     */
    static int32_t GetPolicyWhiteList(
        const std::string& permission, std::vector<AccessTokenID>& tokenIdList);

    /**
     * @brief Whether it is a system application
     * @param tokenId token id.
     * @return bool
     */
    static bool IsSystemAppByFullTokenID(uint64_t tokenId);

    /**
     * @brief Gets the render process tokenId.
     * @param tokenId token id.
     * @return tokenId
     */
    static uint64_t GetRenderTokenID(uint64_t tokenId);

    /**
     * @brief Get kernel permission and value by token id.
     * @param tokenID token id
     * @param kernelPermList PermissionWithValue quote, as query result
     * @return error code, see access_token_error.h
     */
    static int32_t GetKernelPermissions(AccessTokenID tokenID, std::vector<PermissionWithValue>& kernelPermList);
    /**
     * @brief Init cli token info.
     * @param info cli init input
     * @param tokenIdEx output token id
     * @param kernelPermList requested kernel permissions
     * @return error code, see access_token_error.h
     */
    static int32_t InitCliToken(const CliInitInfo& info,
        AccessTokenIDEx& tokenIdEx, std::vector<PermissionWithValue>& kernelPermList);

    /**
     * @brief Init skill token info.
     * @param info skill init input
     * @param tokenIdEx output token id
     * @param kernelPermList requested kernel permissions
     * @return error code, see access_token_error.h
     */
    static int32_t InitSkillToken(const SkillInitInfo& info,
        AccessTokenIDEx& tokenIdEx, std::vector<PermissionWithValue>& kernelPermList);

    /**
     * @brief Get cli token info.
     * @param tokenID token id
     * @param info query result
     * @return error code, see access_token_error.h
     */
    static int32_t GetCliTokenInfo(AccessTokenID tokenID, CliTokenInfo& info);

    /**
     * @brief Get skill token info.
     * @param tokenID token id
     * @param info query result
     * @return error code, see access_token_error.h
     */
    static int32_t GetSkillTokenInfo(AccessTokenID tokenID, SkillTokenInfo& info);

    /**
     * @brief Get host token id by tool token id.
     * @param toolTokenId tool token id
     * @param hostTokenId query result
     * @return error code, see access_token_error.h
     */
    static int32_t GetHostTokenId(AccessTokenID toolTokenId, AccessTokenID& hostTokenId);

    /**
     * @brief Get extended value of permission by token id and permission name.
     * @param tokenID token id
     * @param permissionName permission name
     * @param value as result
     * @return error code, see access_token_error.h
     */
    static int32_t GetReqPermissionByName(
        AccessTokenID tokenID, const std::string& permissionName, std::string& value);
#ifdef SECURITY_COMPONENT_ENHANCE_ENABLE
    /**
     * @brief Register security component enhance data when security component service did not start
     * @param enhance enhance data
     * @return error code, see access_token_error.h
     */
    static int32_t RegisterSecCompEnhance(const SecCompEnhanceData& enhance);

    /**
     * @brief update security component enhance data
     * @param pid process id
     * @param seqNum sequence number
     * @return error code, see access_token_error.h
     */
    static int32_t UpdateSecCompEnhance(int32_t pid, uint32_t seqNum);

    /**
     * @brief get security component enhance data
     * @param pid process id
     * @param enhance enhance data
     * @return error code, see access_token_error.h
     */
    static int32_t GetSecCompEnhance(int32_t pid, SecCompEnhanceData& enhance);
#endif
    /**
     * Whether it is a atomic service
     * @param tokenId token id.
     * @return bool
     */
    static bool IsAtomicServiceByFullTokenID(uint64_t tokenId);

    /**
     * @brief Set multiple permissions status and flag with policy.
     * @param tokenID token id
     * @param permissionList permission list to be set
     * @param status the permission status to be set
     * @param flag enum PermissionFlag, see access_token.h
     * @return error code, see access_token_error.h
     */
    static int32_t SetPermissionStatusWithPolicy(
        uint32_t tokenID, const std::vector<std::string>& permissionList, int32_t status, uint32_t flag);

    /**
     * @brief Transfer permission to permCode.
     * @param permissionName permissionName
     * @param opCode permCode
     * @return result
     */
    static bool TransferPermissionToOpcode(const std::string& permissionName, uint32_t& permCode);

    /**
     * @brief Transfer permCode to permission.
     * @param opCode permCode
     * @param permissionName permissionName
     * @return result
     */
    static bool TransferOpcodeToPermission(uint32_t permCode, std::string& permissionName);

    /**
     * @brief Query all applications that have requested the specified permissions and their permission information.
     * @note For C++ native interface: Set onlyHap=true to query only HAP applications, or onlyHap=false
     *       to query all applications (HAP+Native processes).
     * @note For JS interface: Only supports querying HAP applications (equivalent to onlyHap=true).
     *       Native processes will not be included in JS query results.
     * @param permissionList The list of permissions to be queried
     * @param permissionInfoList Output parameter, returns the list of permission information
     * @param onlyHap Whether to query only HAP applications, true means only HAP applications,
     *               false means all applications (HAP+Native), default is true
     * @return Returns RET_SUCCESS(0) on success, returns corresponding error code on failure
     * @since 26
     */
    static int32_t QueryStatusByPermission(const std::vector<std::string>& permissionList,
        std::vector<PermissionStatus>& permissionInfoList, bool onlyHap = true);

    /**
     * @brief Query all permission information of the specified applications.
     * @note This interface only supports querying HAP (Harmony Ability Package) applications.
     *       For Native token IDs, the query will not return any permission information.
     * @param tokenIDList The list of token IDs to be queried
     * @param permissionInfoList Output parameter, returns the list of permission information
     * @return Returns RET_SUCCESS(0) on success, returns corresponding error code on failure
     * @since 26
     */
    static int32_t QueryStatusByTokenID(const std::vector<AccessTokenID>& tokenIDList,
        std::vector<PermissionStatus>& permissionInfoList);

    /**
     * @brief Query whether the current caller needs a permission dialog for CLI commands.
     * @note Returns ERR_PERMISSION_DENIED if the caller fails CLI control permission check.
     * @note CLI permissions returned by the agent fence are converted to PermissionDecisionStatus.
     * @note If no permission dialog is required, uses tokenId and command information to return mock challenge.
     * @param cliInfoList CLI command list.
     * @param result Output parameter, returns dialog decision result for each command.
     * @return Returns RET_SUCCESS(0) on success, returns corresponding error code on failure.
     */
    static int32_t GetCliPermissionRequestInfo(
        const std::string& agentID, const std::vector<CliInfo>& cliInfoList, PermissionDialogResult& result);

    /**
     * @brief Query whether the current caller needs a permission dialog for skill commands.
     * @note If no permission dialog is required, uses tokenId and command information to return mock challenge.
     * @param skillInfoList Skill command list.
     * @param result Output parameter, returns dialog decision result for each command.
     * @return Returns RET_SUCCESS(0) on success, returns corresponding error code on failure.
     */
    static int32_t GetSkillPermissionRequestInfo(
        const std::string& agentID, const std::vector<SkillInfo>& skillInfoList, PermissionDialogResult& result);

    /**
     * @brief Query CLI permission details for the target claw application.
     * @note Returns ERR_PERMISSION_DENIED if target claw fails CLI control permission check.
     * @note requiredCliPermission returns its own status directly when already granted, denied, restricted,
     *       or undeclared; otherwise mapped usedPermissions decide whether the final status is
     *       NEED_PERMISSION_DIALOG or NO_DIALOG_CLI_PERMISSION_RESOLVED.
     * @param hostTokenID Access token ID of the target host application.
     * @param cliInfoList CLI command list.
     * @param result Output parameter, returns CLI permission query result.
     * @return Returns RET_SUCCESS(0) on success, returns corresponding error code on failure.
     */
    static int32_t GetCliPermissions(AccessTokenID hostTokenID, const std::string& agentID,
        const std::vector<CliInfo>& cliInfoList, CliPermissionsResult& result);

    /**
     * @brief Query skill permission details for the target claw application.
     * @param hostTokenID Access token ID of the target host application.
     * @param skillInfoList Skill command list.
     * @param result Output parameter, returns skill permission query result.
     * @return Returns RET_SUCCESS(0) on success, returns corresponding error code on failure.
     */
    static int32_t GetSkillPermissions(AccessTokenID hostTokenID, const std::string& agentID,
        const std::vector<SkillInfo>& skillInfoList, SkillPermissionsResult& result);

    /**
     * @brief Generate auth info for CLI authorization result.
     * @param hostTokenID Access token ID of the target host application.
     * @param authInfoList CLI authorization result list.
     * @param result Output parameter, returns generated auth result.
     * @return Returns RET_SUCCESS(0) on success, returns corresponding error code on failure.
     */
    static int32_t GenerateCliAuthResult(AccessTokenID hostTokenID, const std::string& agentID,
        const std::vector<CliAuthInfo>& authInfoList, ToolAuthResult& result);

    /**
     * @brief Generate auth info for skill authorization result.
     * @param hostTokenID Access token ID of the target host application.
     * @param authInfoList Skill authorization result list.
     * @param result Output parameter, returns generated auth result.
     * @return Returns RET_SUCCESS(0) on success, returns corresponding error code on failure.
     */
    static int32_t GenerateSkillAuthResult(AccessTokenID hostTokenID, const std::string& agentID,
        const std::vector<SkillAuthInfo>& authInfoList, ToolAuthResult& result);
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif
