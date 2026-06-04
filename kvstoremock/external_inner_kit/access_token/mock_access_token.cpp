/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <mutex>
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
static constexpr const char *NativeName[4] = { "invalid", "foundation", "distributed_test", "msdp_sa" };
namespace OHOS::Security::AccessToken {
static std::mutex g_mutex;
static std::vector<HapTokenInfo> g_HapInfo = {
    { .ver = 0, .userID = 100, .bundleName = "invalid"},
    { .ver = 0, .userID = 100, .bundleName = "com.huawei.ohos.toteweather"},
    { .ver = 0, .userID = 100, .bundleName = "com.ohos.kvdatamanager.test"},
    { .ver = 0, .userID = 100, .bundleName = "ohos.test.demo"},
    { .ver = 0, .userID = 100, .bundleName = "ohos.test.demo1"},
    { .ver = 0, .userID = 100, .bundleName = "ohos.test.demo2"},
    { .ver = 0, .userID = 100, .bundleName = "test_cloud_bundleName"},
};

ATokenTypeEnum AccessTokenKit::GetTokenTypeFlag(AccessTokenID tokenID)
{
    AccessTokenIDInner *inner = (AccessTokenIDInner *)&tokenID;
    return ATokenTypeEnum(inner->type);
}

int AccessTokenKit::VerifyAccessToken(AccessTokenID tokenID, const std::string &permissionName)
{
    return PERMISSION_GRANTED;
}

int AccessTokenKit::GetNativeTokenInfo(AccessTokenID tokenID, NativeTokenInfo &nativeTokenInfoRes)
{
    (void)tokenID;
    nativeTokenInfoRes.processName = "msdp_sa";
    return 0;
}

AccessTokenID AccessTokenKit::GetNativeTokenId(const std::string &processName)
{
    uint32_t tokenId = 0;
    AccessTokenIDInner *inner = (AccessTokenIDInner *)&tokenId;
    inner->type = TOKEN_NATIVE;
    for (size_t i = 0; i < sizeof(NativeName) / sizeof(NativeName[0]); ++i) {
        if (processName == NativeName[i]) {
            inner->tokenUniqueID = i;
        }
    }
    return tokenId;
}

int AccessTokenKit::GetHapTokenInfo(AccessTokenID tokenID, HapTokenInfo &hapTokenInfoRes)
{
    AccessTokenIDInner *inner = (AccessTokenIDInner *)&tokenID;
    hapTokenInfoRes.ver = 0;
    hapTokenInfoRes.userID = 100;
    if (inner->type == TOKEN_NATIVE) {
        if (inner->tokenUniqueID >= sizeof(NativeName) / sizeof(NativeName[0])) {
            return -1;
        }
        hapTokenInfoRes.bundleName = NativeName[inner->tokenUniqueID % sizeof(NativeName) / sizeof(NativeName[0])];
    } else {
        std::lock_guard<decltype(g_mutex)> lockGuard(g_mutex);
        if (inner->tokenUniqueID >= g_HapInfo.size()) {
            return -1;
        }
        hapTokenInfoRes = g_HapInfo[inner->tokenUniqueID];
    }
    hapTokenInfoRes.tokenID = tokenID;
    return 0;
}

AccessTokenID AccessTokenKit::GetHapTokenID(int userID, const std::string &bundleName, int instIndex)
{
    uint32_t tokenId = 0;
    auto *inner = (AccessTokenIDInner *)&tokenId;
    inner->type = 3;
    std::lock_guard<decltype(g_mutex)> lockGuard(g_mutex);
    for (size_t i = 0; i < g_HapInfo.size(); ++i) {
        auto &info = g_HapInfo[i];
        if (info.bundleName == bundleName && info.instIndex == instIndex && info.userID == userID) {
            inner->type = TOKEN_HAP;
            inner->tokenUniqueID = i;
            break;
        }
    }
    return tokenId;
}
AccessTokenIDEx AccessTokenKit::AllocHapToken(const HapInfoParams &info, const HapPolicyParams &policy)
{
    HapTokenInfo hapTokenInfo;
    hapTokenInfo.userID = info.userID;
    hapTokenInfo.bundleName = info.bundleName;
    hapTokenInfo.apiVersion = info.apiVersion;
    hapTokenInfo.instIndex = info.instIndex;
    hapTokenInfo.dlpType = info.dlpType;
    hapTokenInfo.tokenID = info.tokenID;
    hapTokenInfo.tokenAttr = info.isSystemApp;
    std::lock_guard<decltype(g_mutex)> lockGuard(g_mutex);
    auto index = g_HapInfo.size();
    AccessTokenIDEx result;
    AccessTokenIDInner *inner = (AccessTokenIDInner *)&result.tokenIdExStruct.tokenID;
    inner->tokenUniqueID = index;
    inner->type = TOKEN_HAP;
    inner->dlpFlag = info.dlpType;
    hapTokenInfo.tokenID = result.tokenIdExStruct.tokenID;
    hapTokenInfo.tokenAttr = result.tokenIdExStruct.tokenAttr;
    g_HapInfo.emplace_back(hapTokenInfo);
    return result;
}
int AccessTokenKit::DeleteToken(AccessTokenID tokenID)
{
    return 0;
}

int32_t AccessTokenKit::GetHapDlpFlag(AccessTokenID tokenID)
{
    return 0;
}
int32_t AccessTokenKit::ReloadNativeTokenInfo()
{
    return 0;
}
AccessTokenIDEx AccessTokenKit::GetHapTokenIDEx(int32_t userID, const std::string &bundleName, int32_t instIndex)
{
    AccessTokenIDEx result;
    auto *inner = (AccessTokenIDInner *)&result.tokenIdExStruct.tokenID;
    inner->type = 3;
    std::lock_guard<decltype(g_mutex)> lockGuard(g_mutex);
    for (size_t i = 0; i < g_HapInfo.size(); ++i) {
        auto &info = g_HapInfo[i];
        if (info.bundleName == bundleName && info.instIndex == instIndex && info.userID == userID) {
            inner->type = TOKEN_HAP;
            inner->tokenUniqueID = i;
            result.tokenIdExStruct.tokenAttr = info.tokenAttr;
            break;
        }
    }
    return result;
}
ATokenTypeEnum AccessTokenKit::GetTokenType(AccessTokenID tokenID)
{
    return TOKEN_HAP;
}
PermUsedTypeEnum AccessTokenKit::GetPermissionUsedType(AccessTokenID tokenID, const std::string &permissionName)
{
    return PermUsedTypeEnum::INVALID_USED_TYPE;
}
int AccessTokenKit::GrantPermissionForSpecifiedTime(AccessTokenID tokenID, const std::string &permissionName,
    uint32_t onceTime)
{
    return 0;
}
int32_t AccessTokenKit::InitHapToken(const HapInfoParams &info, HapPolicyParams &policy, AccessTokenIDEx &fullTokenId)
{
    return 0;
}
int32_t AccessTokenKit::InitHapToken(const HapInfoParams &info, HapPolicyParams &policy, AccessTokenIDEx &fullTokenId,
    HapInfoCheckResult &result)
{
    return 0;
}
FullTokenID AccessTokenKit::AllocLocalTokenID(const std::string &remoteDeviceID, AccessTokenID remoteTokenID)
{
    (void)remoteDeviceID;
    (void)remoteTokenID;
    return 0;
}
int32_t AccessTokenKit::UpdateHapToken(AccessTokenIDEx &tokenIdEx, const UpdateHapInfoParams &info,
    const HapPolicyParams &policy)
{
    return 0;
}
int32_t AccessTokenKit::UpdateHapToken(AccessTokenIDEx &tokenIdEx, const UpdateHapInfoParams &info,
    const HapPolicyParams &policy, HapInfoCheckResult &result)
{
    return 0;
}
ATokenTypeEnum AccessTokenKit::GetTokenType(FullTokenID tokenID)
{
    return TOKEN_HAP;
}
ATokenTypeEnum AccessTokenKit::GetTokenTypeFlag(FullTokenID tokenID)
{
    return TOKEN_HAP;
}
int32_t AccessTokenKit::GetTokenIDByUserID(int32_t userID, std::unordered_set<AccessTokenID> &tokenIdList)
{
    return 0;
}
int AccessTokenKit::VerifyAccessToken(AccessTokenID callerTokenID, AccessTokenID firstTokenID,
    const std::string &permissionName)
{
    return 0;
}
int AccessTokenKit::VerifyAccessToken(AccessTokenID tokenID, const std::string &permissionName, bool crossIpc)
{
    return 0;
}
int AccessTokenKit::VerifyAccessToken(AccessTokenID callerTokenID, AccessTokenID firstTokenID,
    const std::string &permissionName, bool crossIpc)
{
    return 0;
}
int AccessTokenKit::VerifyAccessToken(AccessTokenID tokenID, const std::vector<std::string> &permissionList,
    std::vector<int32_t> &permStateList, bool crossIpc)
{
    return 0;
}
int AccessTokenKit::GetDefPermission(const std::string &permissionName, PermissionDef &permissionDefResult)
{
    return 0;
}
int AccessTokenKit::GetReqPermissions(AccessTokenID tokenID, std::vector<PermissionStateFull> &reqPermList,
    bool isSystemGrant)
{
    return 0;
}
int AccessTokenKit::GetPermissionFlag(AccessTokenID tokenID, const std::string &permissionName, uint32_t &flag)
{
    return 0;
}
int32_t AccessTokenKit::SetPermissionRequestToggleStatus(const std::string &permissionName, uint32_t status,
    int32_t userID)
{
    return 0;
}
int32_t AccessTokenKit::GetPermissionRequestToggleStatus(const std::string &permissionName, uint32_t &status,
    int32_t userID)
{
    return 0;
}
int32_t AccessTokenKit::RequestAppPermOnSetting(AccessTokenID tokenID)
{
    return 0;
}
int32_t AccessTokenKit::GetSelfPermissionStatus(const std::string &permissionName, PermissionOper &status)
{
    return 0;
}
PermissionOper AccessTokenKit::GetSelfPermissionsState(std::vector<PermissionListState> &permList,
    PermissionGrantInfo &info)
{
    return PASS_OPER;
}
int32_t AccessTokenKit::GetPermissionsStatus(AccessTokenID tokenID, std::vector<PermissionListState> &permList)
{
    return 0;
}
int AccessTokenKit::GrantPermission(AccessTokenID tokenID, const std::string &permissionName,
    uint32_t flag, UpdatePermissionFlag updateFlag)
{
    (void)tokenID;
    (void)permissionName;
    (void)flag;
    (void)updateFlag;
    return 0;
}
int AccessTokenKit::RevokePermission(AccessTokenID tokenID, const std::string &permissionName,
    uint32_t flag, UpdatePermissionFlag updateFlag, bool killProcess)
{
    (void)tokenID;
    (void)permissionName;
    (void)flag;
    (void)updateFlag;
    (void)killProcess;
    return 0;
}
int AccessTokenKit::ClearUserGrantedPermissionState(AccessTokenID tokenID)
{
    return 0;
}
int32_t AccessTokenKit::RegisterPermStateChangeCallback(
    const std::shared_ptr<PermStateChangeCallbackCustomize> &callback)
{
    return 0;
}
int32_t AccessTokenKit::UnRegisterPermStateChangeCallback(
    const std::shared_ptr<PermStateChangeCallbackCustomize> &callback)
{
    return 0;
}
int32_t AccessTokenKit::RegisterSelfPermStateChangeCallback(
    const std::shared_ptr<PermStateChangeCallbackCustomize> &callback)
{
    return 0;
}
int32_t AccessTokenKit::UnRegisterSelfPermStateChangeCallback(
    const std::shared_ptr<PermStateChangeCallbackCustomize> &callback)
{
    return 0;
}
int32_t AccessTokenKit::GetVersion(uint32_t &version)
{
    return 0;
}
int AccessTokenKit::GetHapTokenInfoExtension(AccessTokenID tokenID, HapTokenInfoExt &info)
{
    return 0;
}
int32_t AccessTokenKit::SetPermDialogCap(const HapBaseInfo &hapBaseInfo, bool enable)
{
    return 0;
}
void AccessTokenKit::DumpTokenInfo(const AtmToolsParamInfo &info, std::string &dumpInfo) {}
void AccessTokenKit::GetPermissionManagerInfo(PermissionGrantInfo &info) {}
int32_t AccessTokenKit::SetUserPolicy(const std::vector<UserPermissionPolicy> &userList)
{
    (void)userList;
    return 0;
}
int32_t AccessTokenKit::ClearUserPolicy(const std::vector<std::string> &permissionList)
{
    (void)permissionList;
    return 0;
}
bool AccessTokenKit::IsSystemAppByFullTokenID(uint64_t tokenId)
{
    return false;
}
uint64_t AccessTokenKit::GetRenderTokenID(uint64_t tokenId)
{
    return 0;
}
int32_t AccessTokenKit::GetKernelPermissions(AccessTokenID tokenID, std::vector<PermissionWithValue> &kernelPermList)
{
    return 0;
}
int32_t AccessTokenKit::GetReqPermissionByName(AccessTokenID tokenID, const std::string &permissionName,
    std::string &value)
{
    return 0;
}
bool AccessTokenKit::IsAtomicServiceByFullTokenID(uint64_t tokenId)
{
    return false;
}
} // namespace OHOS::Security::AccessToken

uint64_t GetAccessTokenId(NativeTokenInfoParams *params)
{
    uint32_t tokenId = 0;
    auto *inner = (OHOS::Security::AccessToken::AccessTokenIDInner *)&tokenId;
    inner->type = 3;
    for (size_t i = 0; i < (sizeof(NativeName) / sizeof(NativeName[0])); ++i) {
        if (NativeName[i] == nullptr) {
            break;
        }
        if (strcmp(NativeName[i], params->processName) == 0) {
            inner->type = OHOS::Security::AccessToken::TOKEN_NATIVE;
            inner->tokenUniqueID = i;
            break;
        }
    }
    return tokenId;
}

uint64_t GetSelfTokenID(void)
{
    return 1;
}

int SetSelfTokenID(uint64_t tokenID)
{
    (void)tokenID;
    return 0;
}

uint64_t GetFirstCallerTokenID(void)
{
    return 1;
}

int SetFirstCallerTokenID(uint64_t tokenID)
{
    (void)tokenID;
    return 0;
}