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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_CONSTANTS_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_CONSTANTS_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "appexecfwk_errors.h"

namespace OHOS {
namespace AppExecFwk {
namespace Constants {
constexpr const char* TYPE_ONLY_MATCH_WILDCARD = "reserved/wildcard";
constexpr const char* TYPE_WILDCARD = "*/*";
constexpr const char* GENERAL_OBJECT = "general.object";
constexpr const char* EMPTY_STRING = "";
constexpr const char* FILE_UNDERLINE = "_";
constexpr const char* BUNDLE_CODE_DIR = "/data/app/el1/bundle/public";
constexpr const char* BASE_SKILL_DIR = "/data/app/el1/skills/public";
constexpr const char* CACHE_DIR = "cache";
constexpr const char* UID_SEPARATOR = "#";
constexpr int8_t START_USERID = 100;
constexpr int8_t DEFAULT_USERID = 0;
constexpr int8_t U1 = 1;
constexpr int8_t INVALID_USERID = -1;
constexpr int8_t UNSPECIFIED_USERID = -2;
constexpr int8_t ALL_USERID = -3;
constexpr int8_t ANY_USERID = -4;
constexpr int8_t PERMISSION_GRANTED = 0;
constexpr int8_t PERMISSION_NOT_GRANTED = -1;
constexpr int8_t DEFAULT_STREAM_FD = -1;
constexpr int8_t DUMP_INDENT = 4;
constexpr int8_t INVALID_API_VERSION = -1;
constexpr int8_t ALL_VERSIONCODE = -1;
constexpr int8_t INVALID_UDID = -1;
constexpr int8_t DEFAULT_INSTALLERID = -1;
constexpr int8_t DEFAULT_APP_INDEX = 0;
constexpr int16_t MAX_FILE_NAME_LENGTH = 256;
constexpr int16_t BMS_MAX_PATH_LENGTH = 4096;
constexpr int16_t MAX_UNINSTALL_PREINSTALLED_APP_NUM = 200;

// uid and gid
constexpr int32_t INVALID_UID = -1;
constexpr int32_t ROOT_UID = 0;
constexpr int32_t FOUNDATION_UID = 5523;
constexpr int32_t CODE_PROTECT_UID = 7666;
constexpr int32_t BASE_APP_UID = 10000;
constexpr int32_t BASE_USER_RANGE = 200000;
constexpr int32_t MAX_APP_UID = 65535;
constexpr int32_t STORAGE_MANAGER_UID = 1090;
constexpr int32_t EDC_UID = 7200;
constexpr int32_t DEV_ASSISTANT_UID = 7400;
constexpr int32_t CODE_SIGN_UID = 3074;
constexpr int32_t ACCOUNT_UID = 3058;
constexpr int32_t EDM_UID = 3057;

// Render Process: START_ID_FOR_RENDER_PROCESS_ISOLATION and
// END_ID_FOR_RENDER_PROCESS_ISOLATION are defined in process_uid_define.h
//
// Note: START_UID_FOR_RENDER_PROCESS and END_UID_FOR_RENDER_PROCESS
// are deprecated
constexpr int32_t START_UID_FOR_RENDER_PROCESS = 1000000;
constexpr int32_t END_UID_FOR_RENDER_PROCESS = 1099999;

/*
all of cache path for bundle:
modulename max is 128
/data/app/el1~el5/userid/base/bundlename/cache;
/data/app/el2/userid/sharefiles/bundlename/cache;
/data/app/el1~el5/userid/base/bundlename/haps/modulename/cache;
/data/app/el2/userid/sharefiles/bundlename/haps/modulename/cache;
*/
constexpr int16_t MAX_CACHE_DIR_SIZE = 774;

// permissions
constexpr const char* PERMISSION_INSTALL_ALLOW_DOWNGRADE = "ohos.permission.INSTALL_ALLOW_DOWNGRADE";
constexpr const char* PERMISSION_U1_ENABLED = "ohos.permission.SUPPORT_INSTALL_ON_U1";
constexpr const char* PERMISSION_INSTALL_BUNDLE = "ohos.permission.INSTALL_BUNDLE";
constexpr const char* PERMISSION_UNINSTALL_BUNDLE = "ohos.permission.UNINSTALL_BUNDLE";
constexpr const char* PERMISSION_UNINSTALL_CLONE_BUNDLE = "ohos.permission.UNINSTALL_CLONE_BUNDLE";
constexpr const char* PERMISSION_GET_BUNDLE_INFO = "ohos.permission.GET_BUNDLE_INFO";
constexpr const char* PERMISSION_GET_BUNDLE_INFO_PRIVILEGED = "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED";
constexpr const char* PERMISSION_CHANGE_ABILITY_ENABLED_STATE = "ohos.permission.CHANGE_ABILITY_ENABLED_STATE";
constexpr const char* PERMISSION_REMOVECACHEFILE = "ohos.permission.REMOVE_CACHE_FILES";
constexpr const char* PERMISSION_GET_DEFAULT_APPLICATION = "ohos.permission.GET_DEFAULT_APPLICATION";
constexpr const char* PERMISSION_SET_DEFAULT_APPLICATION = "ohos.permission.SET_DEFAULT_APPLICATION";
constexpr const char* PERMISSION_GET_INSTALLED_BUNDLE_LIST = "ohos.permission.GET_INSTALLED_BUNDLE_LIST";
constexpr const char* PERMISSION_RUN_DYN_CODE = "ohos.permission.RUN_DYN_CODE";
constexpr const char* PERMISSION_ACCESS_DYNAMIC_ICON = "ohos.permission.ACCESS_DYNAMIC_ICON";
constexpr const char* PERMISSION_START_SHORTCUT = "ohos.permission.START_SHORTCUT";
constexpr const char* PERMISSION_INSTALL_CLONE_BUNDLE = "ohos.permission.INSTALL_CLONE_BUNDLE";
constexpr const char* PERMISSION_MANAGER_SHORTCUT = "ohos.permission.MANAGE_SHORTCUTS";
constexpr const char* PERMISSION_ACCESS_APP_INSTALL_DIR = "ohos.permission.ACCESS_APP_INSTALL_DIR";
constexpr const char* PERMISSION_GET_SIGNATURE_INFO = "ohos.permission.GET_SIGNATURE_INFO";
constexpr const char* PERMISSION_MIGRATE_DATA = "ohos.permission.MIGRATE_DATA";
constexpr const char* PERMISSION_INSTALL_PLUGIN = "ohos.permission.INSTALL_PLUGIN_BUNDLE";
constexpr const char* PERMISSION_UNINSTALL_PLUGIN = "ohos.permission.UNINSTALL_PLUGIN_BUNDLE";
constexpr const char* PERMISSION_PERFORM_LOCAL_DEBUG = "ohos.permission.PERFORM_LOCAL_DEBUG";
constexpr const char* PERMISSION_GET_ABILITY_INFO = "ohos.permission.GET_ABILITY_INFO";
constexpr const char* PERMISSION_BMS_INTERACT_ACROSS_LOCAL_ACCOUNTS = "ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS";
constexpr const char* PERMISSION_RECOVER_BUNDLE = "ohos.permission.RECOVER_BUNDLE";
constexpr const char* PERMISSION_CLEAN_APPLICATION_DATA = "ohos.permission.CLEAN_APPLICATION_DATA";
constexpr const char* PERMISSION_MANAGE_SELF_SKILLS = "ohos.permission.MANAGE_SELF_SKILLS";
constexpr const char* PERMISSION_MANAGE_SKILL_PRIVILEGE = "ohos.permission.MANAGE_SKILL_PRIVILEGE";
constexpr const char* PERMISSION_MANAGE_SKILL = "ohos.permission.MANAGE_SKILL";
constexpr const char* PERMISSION_MANAGE_SKILL_AND_INTERACT_ACROSS_LOCAL_ACCOUNTS =
    "ohos.permission.MANAGE_SKILL_PRIVILEGE or ohos.permission.MANAGE_SKILL or "
    "((ohos.permission.MANAGE_SKILL_PRIVILEGE or ohos.permission.MANAGE_SKILL) "
    "and ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS)";
constexpr const char* REAL_SKILL_PATH_PREFIX = "/data/app/el1/skills/public";
constexpr const char* SANDBOX_SKILL_PATH_PREFIX = "/data/skills";
constexpr const char* PERMISSION_GET_BUNDLE_INFO_AND_INTERACT_ACROSS_LOCAL_ACCOUNTS =
    "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED or "
    "(ohos.permission.GET_BUNDLE_INFO_PRIVILEGED and ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS)";
constexpr const char* PERMISSION_ENTERPRISE_GET_INSTALLED_BUNDLE_LIST =
    "ohos.permission.ENTERPRISE_GET_INSTALLED_BUNDLE_LIST";

enum class AppType {
    SYSTEM_APP = 0,
    THIRD_SYSTEM_APP,
    THIRD_PARTY_APP,
};

enum NoGetBundleStatsFlag : uint32_t {
    GET_BUNDLE_WITH_ALL_SIZE = 0x00000000,
    GET_BUNDLE_WITHOUT_INSTALL_SIZE = 0x00000001,
    GET_BUNDLE_WITHOUT_DATA_SIZE = 0x00000002,
    GET_BUNDLE_WITHOUT_CACHE_SIZE = 0x00000004,
};

enum AbilityProfileType : uint32_t {
    ABILITY_PROFILE = 0,
    EXTENSION_PROFILE = 1,
    UNKNOWN_PROFILE = 2
};

enum ShortcutSourceType : int32_t {
    DEFAULT_SHORTCUT = 0,
    STATIC_SHORTCUT = 1,
    DYNAMIC_SHORTCUT = 2
};

constexpr const char* ACTION_HOME = "action.system.home";
constexpr const char* WANT_ACTION_HOME = "ohos.want.action.home";
constexpr const char* ENTITY_HOME = "entity.system.home";

constexpr uint8_t MAX_BUNDLE_NAME = 128;
constexpr uint8_t MIN_BUNDLE_NAME = 7;
constexpr uint8_t MAX_JSON_ELEMENT_LENGTH = 255;
constexpr uint16_t MAX_JSON_ARRAY_LENGTH = 1024;

constexpr uint16_t MAX_JSON_STRING_LENGTH = 4096;
constexpr const char* UID = "uid";
constexpr const char* USER_ID = "userId";
constexpr const char* BUNDLE_NAME = "bundleName";
constexpr const char* MODULE_NAME = "moduleName";
constexpr const char* ABILITY_NAME = "abilityName";
constexpr const char* HAP_PATH = "hapPath";
constexpr const char* APP_INDEX = "appIndex";
constexpr int8_t MAX_LIMIT_SIZE = 4;

constexpr const char* PARAM_URI_SEPARATOR = ":///";
constexpr uint8_t PARAM_URI_SEPARATOR_LEN = 4;
constexpr const char* URI_SEPARATOR = "://";

// ipc
constexpr int32_t CAPACITY_SIZE = 1 * 1024 * 1000; // 1M
constexpr int32_t MAX_PARCEL_CAPACITY = 128 * 1024 * 1024; // 128M

// permission
constexpr const char* LISTEN_BUNDLE_CHANGE = "ohos.permission.LISTEN_BUNDLE_CHANGE";

// sandbox application
constexpr const char* SANDBOX_APP_INDEX = "sandbox_app_index";
constexpr int16_t INITIAL_APP_INDEX = 0;
constexpr int16_t INITIAL_SANDBOX_APP_INDEX = 1000;
constexpr int16_t MAX_APP_INDEX = 100;
constexpr int16_t MAX_SANDBOX_APP_INDEX = INITIAL_SANDBOX_APP_INDEX + 100;

// app-distribution-type
constexpr const char* APP_DISTRIBUTION_TYPE_NONE = "none";
constexpr const char* APP_DISTRIBUTION_TYPE_APP_GALLERY = "app_gallery";
constexpr const char* APP_DISTRIBUTION_TYPE_ENTERPRISE = "enterprise";
constexpr const char* APP_DISTRIBUTION_TYPE_ENTERPRISE_NORMAL = "enterprise_normal";
constexpr const char* APP_DISTRIBUTION_TYPE_ENTERPRISE_MDM = "enterprise_mdm";
constexpr const char* APP_DISTRIBUTION_TYPE_INTERNALTESTING = "internaltesting";
constexpr const char* APP_DISTRIBUTION_TYPE_OS_INTEGRATION = "os_integration";
constexpr const char* APP_DISTRIBUTION_TYPE_CROWDTESTING = "crowdtesting";
// app provision type
constexpr const char* APP_PROVISION_TYPE_DEBUG = "debug";
constexpr const char* APP_PROVISION_TYPE_RELEASE = "release";
constexpr const char* APP_PROVISION_TYPE_FILE_NAME = "debug_type";
constexpr const char* DEBUG_TYPE_VALUE = "3";
constexpr const char* RELEASE_TYPE_VALUE = "2";
// crowdtesting
constexpr int8_t INVALID_CROWDTEST_DEADLINE = -1;
constexpr int8_t INHERIT_CROWDTEST_DEADLINE = -2;
// app sign type
constexpr const char* APP_SIGN_TYPE_NONE = "none";
constexpr const char* APP_SIGN_TYPE_ENTERPRISE_RE_SIGN = "enterpriseReSign";
// app build version
constexpr const char* BUILD_VERSION = "buildVersion";

// overlay installation
constexpr const char* OVERLAY_STATE = "overlayState";
constexpr const char* PERMISSION_CHANGE_OVERLAY_ENABLED_STATE = "ohos.permission.CHANGE_OVERLAY_ENABLED_STATE";

constexpr const char* SCENE_BOARD_BUNDLE_NAME = "com.ohos.sceneboard";

// clone application
constexpr int32_t MAIN_APP_INDEX = 0;
constexpr int32_t CLONE_APP_INDEX_MAX = 5;
constexpr int32_t ALL_CLONE_APP_INDEX = -1;

//param key-value
constexpr const char* VERIFY_UNINSTALL_RULE_KEY = "ohos.bms.param.verifyUninstallRule";
constexpr const char* VERIFY_UNINSTALL_RULE_VALUE = "true";


constexpr const char* SUPPORT_APP_TYPES_SEPARATOR = ",";
constexpr const char* SCHEME_SEPARATOR = "://";
constexpr const char* APP_DISTRIBUTION_TYPE_WHITE_LIST = "appDistributionTypeWhiteList";

//param key-value
constexpr const char* VERIFY_UNINSTALL_FORCED_KEY = "ohos.bms.param.verifyUninstallForced";
constexpr const char* VERIFY_UNINSTALL_FORCED_VALUE = "true";

constexpr const char* ARKTS_MODE = "arkTSMode";
constexpr const char* MODULE_ARKTS_MODE = "moduleArkTSMode";
constexpr const char* AOT_COMPILE_STATUS = "aotCompileStatus";
constexpr const char* ARKTS_MODE_DYNAMIC = "dynamic";
constexpr const char* ARKTS_MODE_STATIC = "static";
constexpr const char* ARKTS_MODE_HYBRID = "hybrid";
constexpr const char* FOUNDATION_PROCESS_NAME = "foundation";

inline constexpr const char* NAME = "name";
inline constexpr const char* SRC_ENTRANCE = "srcEntrance";
inline constexpr const char* ICON = "icon";
inline constexpr const char* ICON_ID = "iconId";
inline constexpr const char* LABEL = "label";
inline constexpr const char* LABEL_ID = "labelId";
inline constexpr const char* DESCRIPTION = "description";
inline constexpr const char* DESCRIPTION_ID = "descriptionId";
inline constexpr const char* PRIORITY = "priority";
inline constexpr const char* TYPE = "type";
inline constexpr const char* EXTENSION_TYPE_NAME = "extensionTypeName";
inline constexpr const char* PERMISSIONS = "permissions";
inline constexpr const char* APPIDENTIFIER_ALLOW_LIST = "appIdentifierAllowList";
inline constexpr const char* READ_PERMISSION = "readPermission";
inline constexpr const char* WRITE_PERMISSION = "writePermission";
inline constexpr const char* URI = "uri";
inline constexpr const char* VISIBLE = "visible";
inline constexpr const char* META_DATA = "metadata";
inline constexpr const char* RESOURCE_PATH = "resourcePath";
inline constexpr const char* ENABLED = "enabled";
inline constexpr const char* PROCESS = "process";
inline constexpr const char* COMPILE_MODE = "compileMode";
inline constexpr const char* EXTENSION_PROCESS_MODE = "extensionProcessMode";
inline constexpr const char* SKILLS = "skills";
inline constexpr const char* NEED_CREATE_SANDBOX = "needCreateSandbox";
inline constexpr const char* DATA_GROUP_IDS = "dataGroupIds";
inline constexpr const char* VALID_DATA_GROUP_IDS = "validDataGroupIds";
inline constexpr const char* CUSTOM_PROCESS = "customProcess";
inline constexpr const char* ISOLATION_PROCESS = "isolationProcess";
inline constexpr const char* SKIP_ABILITY_STAGE_LIFECYCLE = "skipAbilityStageLifecycle";
inline constexpr const char* SRC_LANGUAGE_JS = "js";
inline constexpr const char* PREFER_MULTI_WINDOW_ORIENTATION_DEFAULT = "default";
}  // namespace Constants
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_CONSTANTS_H