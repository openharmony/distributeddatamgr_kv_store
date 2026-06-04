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

#ifndef CLAW_PERMISSION_INFO_H
#define CLAW_PERMISSION_INFO_H

#include <cstdint>
#include <string>
#include <vector>

#include "access_token.h"

namespace OHOS {
namespace Security {
namespace AccessToken {
constexpr size_t MAX_CLAW_AGENT_ID_LEN = 48;
constexpr int32_t INVALID_TOOL_TOKEN_USER_ID = -1;
constexpr size_t MAX_CLAW_CLI_INFO_LIST_SIZE = 99;
constexpr size_t MAX_CLAW_CLI_NAME_LEN = 256;
constexpr size_t MAX_CLAW_SUB_CLI_NAME_LEN = 256;

enum class PermissionDecisionStatus : int32_t {
    NEED_PERMISSION_DIALOG = 0,
    NO_DIALOG_DENIED = 1,
    NO_DIALOG_RESTRICTED = 2,
    NO_DIALOG_GRANTED = 3,
    NO_DIALOG_NOT_DECLARED = 4,
    NO_DIALOG_CLI_PERMISSION_RESOLVED = 5,
};

struct CliInfo final {
    std::string cliName;
    std::string subCliName;
};

struct CliInitInfo final {
    AccessTokenID hostTokenId = INVALID_TOKENID;
    std::string challenge;
    CliInfo cliInfo;
};

struct CliTokenInfo final {
    AccessTokenID hostTokenId = INVALID_TOKENID;
    int32_t userId = INVALID_TOOL_TOKEN_USER_ID;
    std::string cliName;
    std::string subCliName;
};

struct SkillInfo final {
    std::string skillName;
    std::string bundleName;
    std::string moduleName;
};

struct SkillInitInfo final {
    AccessTokenID hostTokenId = INVALID_TOKENID;
    std::string challenge;
    SkillInfo skillInfo;
};

struct SkillTokenInfo final {
    AccessTokenID hostTokenId = INVALID_TOKENID;
    int32_t userId = INVALID_TOOL_TOKEN_USER_ID;
    std::string skillName;
    std::string bundleName;
    std::string moduleName;
};

struct PermissionDialogDetail final {
    bool needPermissionDialog = false;
    std::vector<std::string> permissionNameList;
    std::vector<PermissionDecisionStatus> statusList;
    std::string authResult;
};

struct PermissionDialogResult final {
    std::vector<PermissionDialogDetail> detailList;
};

struct CliPermissionDetail final {
    std::string requiredCliPermission;
    PermissionDecisionStatus cliPermissionStatus = PermissionDecisionStatus::NO_DIALOG_NOT_DECLARED;
    std::vector<std::string> usedPermissions;
};

struct CliCommandPermissionResult final {
    std::vector<CliPermissionDetail> requiredCliPermissions;
};

struct CliPermissionsResult final {
    std::vector<CliCommandPermissionResult> permList;
};

struct SkillCommandPermissionResult final {
    std::vector<std::string> usedPermissions;
    std::vector<PermissionDecisionStatus> statusList;
};

struct SkillPermissionsResult final {
    std::vector<SkillCommandPermissionResult> permList;
};

struct CliAuthInfo final {
    CliInfo cliInfo;
    std::vector<std::string> permissionNames;
    std::vector<bool> authorizationResults;
};

struct SkillAuthInfo final {
    SkillInfo skillInfo;
    std::vector<std::string> permissionNames;
    std::vector<bool> authorizationResults;
};

struct ToolAuthResult final {
    std::vector<std::string> authResults;
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif // CLAW_PERMISSION_INFO_H
