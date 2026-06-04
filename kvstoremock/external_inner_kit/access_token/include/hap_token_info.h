/**
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
#ifndef ACCESSTOKEN_HAP_TOKEN_INFO_H
#define ACCESSTOKEN_HAP_TOKEN_INFO_H

#include "access_token.h"
#include "permission_def.h"
#include "permission_state_full.h"
#include "permission_status.h"
#include <map>
#include <string>
#include <vector>

namespace OHOS {
namespace Security {
namespace AccessToken {
class HapInfoParams final {
public:
    int userID;
    std::string bundleName;
    int instIndex;
    int dlpType;
    std::string appIDDesc;
    int32_t apiVersion;
    bool isSystemApp;
    std::string appDistributionType;
    bool isRestore = false;
    AccessTokenID tokenID = INVALID_TOKENID;
    bool isAtomicService = false;
    std::string appProvisionType = "release";
    bool isSkillHap = false;
};

class UpdateHapInfoParams final {
public:
    std::string appIDDesc;
    int32_t apiVersion;
    bool isSystemApp;
    std::string appDistributionType;
    bool isAtomicService = false;
    bool dataRefresh = false;
    std::string appProvisionType = "release";
    bool isSkillHap = false;
};

class HapTokenInfo final {
public:
    char ver;
    int userID = 0;
    std::string bundleName;
    int32_t apiVersion;
    int instIndex = 0;
    int dlpType;
    AccessTokenID tokenID;
    AccessTokenAttr tokenAttr;
};

class HapTokenInfoForSync final {
public:
    HapTokenInfo baseInfo;
    std::vector<PermissionStatus> permStateList;
};

class HapTokenInfoExt final {
public:
    HapTokenInfo baseInfo;
    std::string appID;
};

class HapBaseInfo final {
public:
    int32_t userID;
    std::string bundleName = "";
    int32_t instIndex = 0;
};

class PreAuthorizationInfo final {
public:
    std::string permissionName;
    bool userCancelable = false;
};

class HapPolicyParams final {
public:
    ATokenAplEnum apl;
    std::string domain;
    std::vector<PermissionDef> permList;
    std::vector<PermissionStateFull> permStateList;
    std::vector<std::string> aclRequestedList;
    std::vector<PreAuthorizationInfo> preAuthorizationInfo;
    HapPolicyCheckIgnore checkIgnore = HapPolicyCheckIgnore::NONE;
    std::map<std::string, std::string> aclExtendedMap;
    bool isDebugGrant = false;
};

class PermissionInfoCheckResult final {
public:
    std::string permissionName;
    PermissionRulesEnum rule;
};

class HapInfoCheckResult final {
public:
    PermissionInfoCheckResult permCheckResult;
};

class HapPolicy final {
public:
    ATokenAplEnum apl;
    std::string domain;
    std::vector<PermissionDef> permList;
    std::vector<PermissionStatus> permStateList;
    std::vector<std::string> aclRequestedList;
    std::vector<PreAuthorizationInfo> preAuthorizationInfo;
    HapPolicyCheckIgnore checkIgnore = HapPolicyCheckIgnore::NONE;
    std::map<std::string, std::string> aclExtendedMap;
    bool isDebugGrant = false;
};

class PermissionWithValue final {
public:
    std::string permissionName;
    std::string value;
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif // ACCESSTOKEN_HAP_TOKEN_INFO_H