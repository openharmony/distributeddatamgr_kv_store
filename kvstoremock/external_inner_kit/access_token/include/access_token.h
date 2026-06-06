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
#ifndef ACCESS_TOKEN_H
#define ACCESS_TOKEN_H

#include <string>

namespace OHOS {
namespace Security {
namespace AccessToken {
typedef unsigned int AccessTokenID;
typedef uint64_t FullTokenID;
typedef unsigned int AccessTokenAttr;
constexpr const int DEFAULT_TOKEN_VERSION = 1;
constexpr const AccessTokenID INVALID_TOKENID = 0;

enum class PermUsedTypeEnum {
    INVALID_USED_TYPE = -1,
    NORMAL_TYPE,
    PICKER_TYPE,
    SEC_COMPONENT_TYPE,
    PERM_USED_TYPE_BUTT,
};

enum AccessTokenKitRet {
    RET_FAILED = -1,
    RET_SUCCESS = 0,
};

typedef struct {
    unsigned int tokenUniqueID : 20;
    unsigned int res : 3;
    unsigned int toolFlag : 1;
    unsigned int cloneFlag : 1;
    unsigned int renderFlag : 1;
    unsigned int dlpFlag : 1;
    unsigned int type : 2;
    unsigned int version : 3;
} AccessTokenIDInner;

typedef enum TypeATokenTypeEnum {
    TOKEN_INVALID = -1,
    TOKEN_HAP = 0,
    TOKEN_NATIVE,
    TOKEN_SHELL,
    TOKEN_TYPE_BUTT,
} ATokenTypeEnum;

typedef enum TypeATokenAplEnum {
    APL_INVALID = 0,
    APL_NORMAL = 1,
    APL_SYSTEM_BASIC = 2,
    APL_SYSTEM_CORE = 3,
    APL_ENUM_BUTT,
} ATokenAplEnum;

typedef enum TypeATokenAvailableTypeEnum {
    INVALID = -1,
    NORMAL = 0,
    SYSTEM,
    MDM,
    SYSTEM_AND_MDM,
    SERVICE,
    ENTERPRISE_NORMAL,
    AVAILABLE_TYPE_BUTT,
} ATokenAvailableTypeEnum;

typedef union {
    unsigned long long tokenIDEx;
    struct {
        AccessTokenID tokenID;
        AccessTokenAttr tokenAttr;
    } tokenIdExStruct;
} AccessTokenIDEx;

typedef enum TypePermissionRequestToggleStatus {
    CLOSED = 0,
    OPEN = 1,
} PermissionRequestToggleStatus;

typedef enum TypePermissionState {
    PERMISSION_DENIED = -1,
    PERMISSION_GRANTED = 0,
} PermissionState;

typedef enum TypeGrantMode {
    USER_GRANT = 0,
    SYSTEM_GRANT = 1,
    MANUAL_SETTINGS = 2,
} GrantMode;

typedef enum TypeUpdatePermissionFlag {
    USER_GRANTED_PERM = 0,
    OPERABLE_PERM = 1,
} UpdatePermissionFlag;

typedef enum TypePermissionFlag {
    PERMISSION_DEFAULT_FLAG = 0,
    PERMISSION_USER_SET = 1 << 0,
    PERMISSION_USER_FIXED = 1 << 1,
    PERMISSION_SYSTEM_FIXED = 1 << 2,
    PERMISSION_PRE_AUTHORIZED_CANCELABLE = 1 << 3,
    PERMISSION_COMPONENT_SET = 1 << 4,
    PERMISSION_FIXED_FOR_SECURITY_POLICY = 1 << 5,
    PERMISSION_ALLOW_THIS_TIME = 1 << 6,
    PERMISSION_FIXED_BY_ADMIN_POLICY = 1 << 7,
    PERMISSION_ADMIN_POLICIES_CANCEL = 1 << 8,
} PermissionFlag;

typedef enum TypePermissionOper {
    SETTING_OPER = -1,
    PASS_OPER = 0,
    DYNAMIC_OPER = 1,
    INVALID_OPER = 2,
    FORBIDDEN_OPER = 3,
    BUTT_OPER,
} PermissionOper;

typedef enum TypePermissionErrorReason {
    REQ_SUCCESS = 0,
    PERM_INVALID = 1,
    PERM_NOT_DECLEARED = 2,
    CONDITIONS_NOT_MET = 3,
    PRIVACY_STATEMENT_NOT_AGREED = 4,
    UNABLE_POP_UP = 5,
    MANUAL_SETTING_PERM = 6,
    SERVICE_ABNORMAL = 12,
} PermissionErrorReason;

typedef enum DlpType {
    DLP_COMMON = 0,
    DLP_READ = 1,
    DLP_FULL_CONTROL = 2,
    BUTT_DLP_TYPE,
} HapDlpType;

typedef struct {
    int32_t userId;
    bool isRestricted;
} UserPolicy;

typedef struct {
    std::string permissionName;
    std::vector<UserPolicy> userPolicyList;
} UserPermissionPolicy;

typedef enum UpdateWhiteListType {
    ADD = 0,
    DELETE,
} UpdateWhiteListType;

typedef enum TypePermissionRulesEnum {
    PERMISSION_EDM_RULE = 0,
    PERMISSION_ACL_RULE,
    PERMISSION_ENTERPRISE_NORMAL_RULE
} PermissionRulesEnum;

typedef enum RegisterPermissionChangeType {
    SYSTEM_REGISTER_TYPE = 0,
    SELF_REGISTER_TYPE = 1,
} RegisterPermChangeType;

typedef enum HapPolicyCheckIgnoreType {
    NONE = 0,
    ACL_IGNORE_CHECK,
} HapPolicyCheckIgnore;
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif // ACCESS_TOKEN_H