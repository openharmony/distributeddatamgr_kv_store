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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_COMMON_JSON_CONVERTER_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_COMMON_JSON_CONVERTER_H

#include "app_log_wrapper.h"
#include "json_util.h"

namespace OHOS {
namespace AppExecFwk {
inline constexpr const char* JSON_KEY_NAME = "name";
inline constexpr const char* JSON_KEY_PACKAGE = "package";
inline constexpr const char* JSON_KEY_APPLICATION_NAME = "applicationName";
inline constexpr const char* JSON_KEY_ICON_PATH = "iconPath";
inline constexpr const char* JSON_KEY_THEME = "theme";
inline constexpr const char* JSON_KEY_KIND = "kind";
inline constexpr const char* JSON_KEY_EXTENSION_ABILITY_TYPE = "extensionAbilityType";
inline constexpr const char* JSON_KEY_ORIENTATION = "orientation";
inline constexpr const char* JSON_KEY_LAUNCH_MODE = "launchMode";
inline constexpr const char* JSON_KEY_CODE_PATH = "codePath";
inline constexpr const char* JSON_KEY_DEVICE_TYPES = "deviceTypes";
inline constexpr const char* JSON_KEY_DEVICE_CAPABILITIES = "deviceCapabilities";
inline constexpr const char* JSON_KEY_IS_LAUNCHER_ABILITY = "isLauncherAbility";
inline constexpr const char* JSON_KEY_REMOVE_MISSION_AFTER_TERMINATE = "removeMissionAfterTerminate";
inline constexpr const char* JSON_KEY_ALLOW_SELF_REDIRECT = "allowSelfRedirect";
inline constexpr const char* JSON_KEY_IS_NATIVE_ABILITY = "isNativeAbility";
inline constexpr const char* JSON_KEY_SUPPORT_PIP_MODE = "supportPipMode";
inline constexpr const char* JSON_KEY_TARGET_ABILITY = "targetAbility";
inline constexpr const char* JSON_KEY_CONFIG_CHANGES = "configChanges";
inline constexpr const char* JSON_KEY_FORM_ENTITY = "formEntity";
inline constexpr const char* JSON_KEY_MIN_FORM_HEIGHT = "minFormHeight";
inline constexpr const char* JSON_KEY_DEFAULT_FORM_HEIGHT = "defaultFormHeight";
inline constexpr const char* JSON_KEY_MIN_FORM_WIDTH = "minFormWidth";
inline constexpr const char* JSON_KEY_DEFAULT_FORM_WIDTH = "defaultFormWidth";
inline constexpr const char* JSON_KEY_BACKGROUND_MODES = "backgroundModes";
inline constexpr const char* JSON_KEY_CUSTOMIZE_DATA = "customizeData";
inline constexpr const char* JSON_KEY_META_DATA = "metaData";
inline constexpr const char* JSON_KEY_META_VALUE = "value";
inline constexpr const char* JSON_KEY_META_EXTRA = "extra";
inline constexpr const char* JSON_KEY_FORM_ENABLED = "formEnabled";
inline constexpr const char* JSON_KEY_SRC_PATH = "srcPath";
inline constexpr const char* JSON_KEY_SRC_LANGUAGE = "srcLanguage";
inline constexpr const char* JSON_KEY_START_WINDOW = "startWindow";
inline constexpr const char* JSON_KEY_START_WINDOW_ID = "startWindowId";
inline constexpr const char* JSON_KEY_START_WINDOW_RESOURCE = "startWindowResource";
inline constexpr const char* JSON_KEY_START_WINDOW_ICON = "startWindowIcon";
inline constexpr const char* JSON_KEY_START_WINDOW_ICON_ID = "startWindowIconId";
inline constexpr const char* JSON_KEY_START_WINDOW_BACKGROUND = "startWindowBackground";
inline constexpr const char* JSON_KEY_START_WINDOW_BACKGROUND_ID = "startWindowBackgroundId";
inline constexpr const char* JSON_KEY_IS_MODULE_JSON = "isModuleJson";
inline constexpr const char* JSON_KEY_IS_STAGE_BASED_MODEL = "isStageBasedModel";
inline constexpr const char* JSON_KEY_IS_CONTINUABLE = "continuable";
inline constexpr const char* JSON_KEY_SUPPORT_WINDOW_MODE = "supportWindowMode";
inline constexpr const char* JSON_KEY_MAX_WINDOW_RATIO = "maxWindowRatio";
inline constexpr const char* JSON_KEY_MIN_WINDOW_RATIO = "minWindowRatio";
inline constexpr const char* JSON_KEY_MAX_WINDOW_WIDTH = "maxWindowWidth";
inline constexpr const char* JSON_KEY_MIN_WINDOW_WIDTH = "minWindowWidth";
inline constexpr const char* JSON_KEY_MAX_WINDOW_HEIGHT = "maxWindowHeight";
inline constexpr const char* JSON_KEY_MIN_WINDOW_HEIGHT = "minWindowHeight";
inline constexpr const char* JSON_KEY_EXCLUDE_FROM_MISSIONS = "excludeFromMissions";
inline constexpr const char* JSON_KEY_UNCLEARABLE_MISSION = "unclearableMission";
inline constexpr const char* JSON_KEY_EXCLUDE_FROM_DOCK_MISSION = "excludeFromDock";
inline constexpr const char* JSON_KEY_PREFER_MULTI_WINDOW_ORIENTATION_MISSION = "preferMultiWindowOrientation";
inline constexpr const char* JSON_KEY_RECOVERABLE = "recoverable";
inline constexpr const char* JSON_KEY_SUPPORT_EXT_NAMES = "supportExtNames";
inline constexpr const char* JSON_KEY_SUPPORT_MIME_TYPES = "supportMimeTypes";
inline constexpr const char* JSON_KEY_ISOLATION_PROCESS = "isolationProcess";
inline constexpr const char* JSON_KEY_CONTINUE_TYPE = "continueType";
inline constexpr const char* JSON_KEY_ORIENTATION_ID = "orientationId";
inline constexpr const char* JSON_KEY_CONTINUE_BUNDLE_NAME = "continueBundleName";

template<typename T>
void AbilityFromJson(const nlohmann::json &jsonObject, T &abilityInfo)
{
    APP_LOGD("AbilityFromJson begin");
    const auto &jsonObjectEnd = jsonObject.end();
    int32_t parseResult = ERR_OK;
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::NAME,
        abilityInfo.name,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::LABEL,
        abilityInfo.label,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::DESCRIPTION,
        abilityInfo.description,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_ICON_PATH,
        abilityInfo.iconPath,
        false,
        parseResult);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        Constants::LABEL_ID,
        abilityInfo.labelId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        Constants::DESCRIPTION_ID,
        abilityInfo.descriptionId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        Constants::ICON_ID,
        abilityInfo.iconId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_THEME,
        abilityInfo.theme,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::VISIBLE,
        abilityInfo.visible,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_KIND,
        abilityInfo.kind,
        false,
        parseResult);
    GetValueIfFindKey<AbilityType>(jsonObject,
        jsonObjectEnd,
        Constants::TYPE,
        abilityInfo.type,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<ExtensionAbilityType>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_EXTENSION_ABILITY_TYPE,
        abilityInfo.extensionAbilityType,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<DisplayOrientation>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_ORIENTATION,
        abilityInfo.orientation,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<LaunchMode>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_LAUNCH_MODE,
        abilityInfo.launchMode,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_SRC_PATH,
        abilityInfo.srcPath,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_SRC_LANGUAGE,
        abilityInfo.srcLanguage,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::ARKTS_MODE,
        abilityInfo.arkTSMode,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        Constants::PERMISSIONS,
        abilityInfo.permissions,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::PROCESS,
        abilityInfo.process,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_DEVICE_TYPES,
        abilityInfo.deviceTypes,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_DEVICE_CAPABILITIES,
        abilityInfo.deviceCapabilities,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::URI,
        abilityInfo.uri,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_TARGET_ABILITY,
        abilityInfo.targetAbility,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_IS_LAUNCHER_ABILITY,
        abilityInfo.isLauncherAbility,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_IS_NATIVE_ABILITY,
        abilityInfo.isNativeAbility,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::ENABLED,
        abilityInfo.enabled,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_SUPPORT_PIP_MODE,
        abilityInfo.supportPipMode,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_FORM_ENABLED,
        abilityInfo.formEnabled,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::READ_PERMISSION,
        abilityInfo.readPermission,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::WRITE_PERMISSION,
        abilityInfo.writePermission,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_CONFIG_CHANGES,
        abilityInfo.configChanges,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_FORM_ENTITY,
        abilityInfo.formEntity,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_MIN_FORM_HEIGHT,
        abilityInfo.minFormHeight,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_DEFAULT_FORM_HEIGHT,
        abilityInfo.defaultFormHeight,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_MIN_FORM_WIDTH,
        abilityInfo.minFormWidth,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_DEFAULT_FORM_WIDTH,
        abilityInfo.defaultFormWidth,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<MetaData>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_META_DATA,
        abilityInfo.metaData,
        JsonType::OBJECT,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_BACKGROUND_MODES,
        abilityInfo.backgroundModes,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_PACKAGE,
        abilityInfo.package,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::BUNDLE_NAME,
        abilityInfo.bundleName,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::MODULE_NAME,
        abilityInfo.moduleName,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_APPLICATION_NAME,
        abilityInfo.applicationName,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_CODE_PATH,
        abilityInfo.codePath,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::RESOURCE_PATH,
        abilityInfo.resourcePath,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::HAP_PATH,
        abilityInfo.hapPath,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::SRC_ENTRANCE,
        abilityInfo.srcEntrance,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<Metadata>>(jsonObject,
        jsonObjectEnd,
        Constants::META_DATA,
        abilityInfo.metadata,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::OBJECT);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_IS_MODULE_JSON,
        abilityInfo.isModuleJson,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_IS_STAGE_BASED_MODEL,
        abilityInfo.isStageBasedModel,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_IS_CONTINUABLE,
        abilityInfo.continuable,
        false,
        parseResult);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        Constants::PRIORITY,
        abilityInfo.priority,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_START_WINDOW_ICON,
        abilityInfo.startWindowIcon,
        false,
        parseResult);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_START_WINDOW_ICON_ID,
        abilityInfo.startWindowIconId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_START_WINDOW_BACKGROUND,
        abilityInfo.startWindowBackground,
        false,
        parseResult);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_START_WINDOW_BACKGROUND_ID,
        abilityInfo.startWindowBackgroundId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_REMOVE_MISSION_AFTER_TERMINATE,
        abilityInfo.removeMissionAfterTerminate,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_ALLOW_SELF_REDIRECT,
        abilityInfo.allowSelfRedirect,
        false,
        parseResult);
    GetValueIfFindKey<CompileMode>(jsonObject,
        jsonObjectEnd,
        Constants::COMPILE_MODE,
        abilityInfo.compileMode,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<SupportWindowMode>>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_SUPPORT_WINDOW_MODE,
        abilityInfo.windowModes,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::NUMBER);
    GetValueIfFindKey<double>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_MAX_WINDOW_RATIO,
        abilityInfo.maxWindowRatio,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<double>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_MIN_WINDOW_RATIO,
        abilityInfo.minWindowRatio,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_MAX_WINDOW_WIDTH,
        abilityInfo.maxWindowWidth,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_MIN_WINDOW_WIDTH,
        abilityInfo.minWindowWidth,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_MAX_WINDOW_HEIGHT,
        abilityInfo.maxWindowHeight,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_MIN_WINDOW_HEIGHT,
        abilityInfo.minWindowHeight,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        Constants::UID,
        abilityInfo.uid,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_EXCLUDE_FROM_MISSIONS,
        abilityInfo.excludeFromMissions,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_UNCLEARABLE_MISSION,
        abilityInfo.unclearableMission,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_EXCLUDE_FROM_DOCK_MISSION,
        abilityInfo.excludeFromDock,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_PREFER_MULTI_WINDOW_ORIENTATION_MISSION,
        abilityInfo.preferMultiWindowOrientation,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_RECOVERABLE,
        abilityInfo.recoverable,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_SUPPORT_EXT_NAMES,
        abilityInfo.supportExtNames,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_SUPPORT_MIME_TYPES,
        abilityInfo.supportMimeTypes,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_ISOLATION_PROCESS,
        abilityInfo.isolationProcess,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_CONTINUE_TYPE,
        abilityInfo.continueType,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::unordered_set<std::string>>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_CONTINUE_BUNDLE_NAME,
        abilityInfo.continueBundleNames,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<Skill>>(jsonObject,
        jsonObjectEnd,
        Constants::SKILLS,
        abilityInfo.skills,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::OBJECT);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        Constants::APP_INDEX,
        abilityInfo.appIndex,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_ORIENTATION_ID,
        abilityInfo.orientationId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        JSON_KEY_START_WINDOW,
        abilityInfo.startWindow,
        false,
        parseResult);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_START_WINDOW_ID,
        abilityInfo.startWindowId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<StartWindowResource>(jsonObject,
        jsonObjectEnd,
        JSON_KEY_START_WINDOW_RESOURCE,
        abilityInfo.startWindowResource,
        JsonType::OBJECT,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    if (parseResult != ERR_OK) {
        APP_LOGE("AbilityFromJson error : %{public}d", parseResult);
    }
}

template<typename T>
void AbilityToJson(nlohmann::json &jsonObject, const T &abilityInfo)
{
    APP_LOGD("AbilityToJson begin");
    jsonObject = nlohmann::json {
        {Constants::NAME, abilityInfo.name},
        {Constants::LABEL, abilityInfo.label},
        {Constants::DESCRIPTION, abilityInfo.description},
        {JSON_KEY_ICON_PATH, abilityInfo.iconPath},
        {Constants::LABEL_ID, abilityInfo.labelId},
        {Constants::DESCRIPTION_ID, abilityInfo.descriptionId},
        {Constants::ICON_ID, abilityInfo.iconId},
        {JSON_KEY_THEME, abilityInfo.theme},
        {Constants::VISIBLE, abilityInfo.visible},
        {JSON_KEY_KIND, abilityInfo.kind},
        {Constants::TYPE, abilityInfo.type},
        {JSON_KEY_EXTENSION_ABILITY_TYPE, abilityInfo.extensionAbilityType},
        {JSON_KEY_ORIENTATION, abilityInfo.orientation},
        {JSON_KEY_LAUNCH_MODE, abilityInfo.launchMode},
        {JSON_KEY_SRC_PATH, abilityInfo.srcPath},
        {JSON_KEY_SRC_LANGUAGE, abilityInfo.srcLanguage},
        {Constants::ARKTS_MODE, abilityInfo.arkTSMode},
        {Constants::PERMISSIONS, abilityInfo.permissions},
        {Constants::PROCESS, abilityInfo.process},
        {JSON_KEY_DEVICE_TYPES, abilityInfo.deviceTypes},
        {JSON_KEY_DEVICE_CAPABILITIES, abilityInfo.deviceCapabilities},
        {Constants::URI, abilityInfo.uri},
        {JSON_KEY_TARGET_ABILITY, abilityInfo.targetAbility},
        {JSON_KEY_IS_LAUNCHER_ABILITY, abilityInfo.isLauncherAbility},
        {JSON_KEY_IS_NATIVE_ABILITY, abilityInfo.isNativeAbility},
        {Constants::ENABLED, abilityInfo.enabled},
        {JSON_KEY_SUPPORT_PIP_MODE, abilityInfo.supportPipMode},
        {JSON_KEY_FORM_ENABLED, abilityInfo.formEnabled},
        {Constants::READ_PERMISSION, abilityInfo.readPermission},
        {Constants::WRITE_PERMISSION, abilityInfo.writePermission},
        {JSON_KEY_CONFIG_CHANGES, abilityInfo.configChanges},
        {JSON_KEY_FORM_ENTITY, abilityInfo.formEntity},
        {JSON_KEY_MIN_FORM_HEIGHT, abilityInfo.minFormHeight},
        {JSON_KEY_DEFAULT_FORM_HEIGHT, abilityInfo.defaultFormHeight},
        {JSON_KEY_MIN_FORM_WIDTH, abilityInfo.minFormWidth},
        {JSON_KEY_DEFAULT_FORM_WIDTH, abilityInfo.defaultFormWidth},
        {JSON_KEY_META_DATA, abilityInfo.metaData},
        {JSON_KEY_BACKGROUND_MODES, abilityInfo.backgroundModes},
        {JSON_KEY_PACKAGE, abilityInfo.package},
        {Constants::BUNDLE_NAME, abilityInfo.bundleName},
        {Constants::MODULE_NAME, abilityInfo.moduleName},
        {JSON_KEY_APPLICATION_NAME, abilityInfo.applicationName},
        {JSON_KEY_CODE_PATH, abilityInfo.codePath},
        {Constants::RESOURCE_PATH, abilityInfo.resourcePath},
        {Constants::HAP_PATH, abilityInfo.hapPath},
        {Constants::SRC_ENTRANCE, abilityInfo.srcEntrance},
        {Constants::META_DATA, abilityInfo.metadata},
        {JSON_KEY_IS_MODULE_JSON, abilityInfo.isModuleJson},
        {JSON_KEY_IS_STAGE_BASED_MODEL, abilityInfo.isStageBasedModel},
        {JSON_KEY_IS_CONTINUABLE, abilityInfo.continuable},
        {Constants::PRIORITY, abilityInfo.priority},
        {JSON_KEY_START_WINDOW, abilityInfo.startWindow},
        {JSON_KEY_START_WINDOW_ID, abilityInfo.startWindowId},
        {JSON_KEY_START_WINDOW_ICON, abilityInfo.startWindowIcon},
        {JSON_KEY_START_WINDOW_ICON_ID, abilityInfo.startWindowIconId},
        {JSON_KEY_START_WINDOW_BACKGROUND, abilityInfo.startWindowBackground},
        {JSON_KEY_START_WINDOW_BACKGROUND_ID, abilityInfo.startWindowBackgroundId},
        {JSON_KEY_REMOVE_MISSION_AFTER_TERMINATE, abilityInfo.removeMissionAfterTerminate},
        {JSON_KEY_ALLOW_SELF_REDIRECT, abilityInfo.allowSelfRedirect},
        {Constants::COMPILE_MODE, abilityInfo.compileMode},
        {JSON_KEY_SUPPORT_WINDOW_MODE, abilityInfo.windowModes},
        {JSON_KEY_MAX_WINDOW_WIDTH, abilityInfo.maxWindowWidth},
        {JSON_KEY_MIN_WINDOW_WIDTH, abilityInfo.minWindowWidth},
        {JSON_KEY_MAX_WINDOW_HEIGHT, abilityInfo.maxWindowHeight},
        {JSON_KEY_MIN_WINDOW_HEIGHT, abilityInfo.minWindowHeight},
        {Constants::UID, abilityInfo.uid},
        {JSON_KEY_EXCLUDE_FROM_MISSIONS, abilityInfo.excludeFromMissions},
        {JSON_KEY_UNCLEARABLE_MISSION, abilityInfo.unclearableMission},
        {JSON_KEY_EXCLUDE_FROM_DOCK_MISSION, abilityInfo.excludeFromDock},
        {JSON_KEY_PREFER_MULTI_WINDOW_ORIENTATION_MISSION, abilityInfo.preferMultiWindowOrientation},
        {JSON_KEY_RECOVERABLE, abilityInfo.recoverable},
        {JSON_KEY_SUPPORT_EXT_NAMES, abilityInfo.supportExtNames},
        {JSON_KEY_SUPPORT_MIME_TYPES, abilityInfo.supportMimeTypes},
        {JSON_KEY_ISOLATION_PROCESS, abilityInfo.isolationProcess},
        {JSON_KEY_CONTINUE_TYPE, abilityInfo.continueType},
        {JSON_KEY_CONTINUE_BUNDLE_NAME, abilityInfo.continueBundleNames},
        {Constants::SKILLS, abilityInfo.skills},
        {Constants::APP_INDEX, abilityInfo.appIndex},
        {JSON_KEY_ORIENTATION_ID, abilityInfo.orientationId},
        {JSON_KEY_START_WINDOW_RESOURCE, abilityInfo.startWindowResource}
    };
    if (abilityInfo.maxWindowRatio == 0) {
        // maxWindowRatio in json string will be 0 instead of 0.0
        jsonObject[JSON_KEY_MAX_WINDOW_RATIO] = 0;
    } else {
        jsonObject[JSON_KEY_MAX_WINDOW_RATIO] = abilityInfo.maxWindowRatio;
    }

    if (abilityInfo.minWindowRatio == 0) {
        jsonObject[JSON_KEY_MIN_WINDOW_RATIO] = 0;
    } else {
        jsonObject[JSON_KEY_MIN_WINDOW_RATIO] = abilityInfo.minWindowRatio;
    }
}

template<typename T>
void ExtensionFromJson(const nlohmann::json &jsonObject, T &extensionInfo)
{
    APP_LOGD("ExtensionFromJson begin");
    const auto &jsonObjectEnd = jsonObject.end();
    int32_t parseResult = ERR_OK;
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::BUNDLE_NAME,
        extensionInfo.bundleName,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::MODULE_NAME,
        extensionInfo.moduleName,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::NAME,
        extensionInfo.name,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::SRC_ENTRANCE,
        extensionInfo.srcEntrance,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::ICON,
        extensionInfo.icon,
        false,
        parseResult);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        Constants::ICON_ID,
        extensionInfo.iconId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::LABEL,
        extensionInfo.label,
        false,
        parseResult);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        Constants::LABEL_ID,
        extensionInfo.labelId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::DESCRIPTION,
        extensionInfo.description,
        false,
        parseResult);
    GetValueIfFindKey<uint32_t>(jsonObject,
        jsonObjectEnd,
        Constants::DESCRIPTION_ID,
        extensionInfo.descriptionId,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        Constants::PRIORITY,
        extensionInfo.priority,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<ExtensionAbilityType>(jsonObject,
        jsonObjectEnd,
        Constants::TYPE,
        extensionInfo.type,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::EXTENSION_TYPE_NAME,
        extensionInfo.extensionTypeName,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::READ_PERMISSION,
        extensionInfo.readPermission,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::WRITE_PERMISSION,
        extensionInfo.writePermission,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::URI,
        extensionInfo.uri,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        Constants::PERMISSIONS,
        extensionInfo.permissions,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        Constants::APPIDENTIFIER_ALLOW_LIST,
        extensionInfo.appIdentifierAllowList,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::VISIBLE,
        extensionInfo.visible,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<Metadata>>(jsonObject,
        jsonObjectEnd,
        Constants::META_DATA,
        extensionInfo.metadata,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::OBJECT);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::RESOURCE_PATH,
        extensionInfo.resourcePath,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::HAP_PATH,
        extensionInfo.hapPath,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::ENABLED,
        extensionInfo.enabled,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::PROCESS,
        extensionInfo.process,
        false,
        parseResult);
    GetValueIfFindKey<CompileMode>(jsonObject,
        jsonObjectEnd,
        Constants::COMPILE_MODE,
        extensionInfo.compileMode,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        Constants::UID,
        extensionInfo.uid,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<int32_t>(jsonObject,
        jsonObjectEnd,
        Constants::APP_INDEX,
        extensionInfo.appIndex,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<ExtensionProcessMode>(jsonObject,
        jsonObjectEnd,
        Constants::EXTENSION_PROCESS_MODE,
        extensionInfo.extensionProcessMode,
        JsonType::NUMBER,
        false,
        parseResult,
        ArrayType::NOT_ARRAY);
    GetValueIfFindKey<std::vector<Skill>>(jsonObject,
        jsonObjectEnd,
        Constants::SKILLS,
        extensionInfo.skills,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::OBJECT);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::NEED_CREATE_SANDBOX,
        extensionInfo.needCreateSandbox,
        false,
        parseResult);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        Constants::DATA_GROUP_IDS,
        extensionInfo.dataGroupIds,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    GetValueIfFindKey<std::vector<std::string>>(jsonObject,
        jsonObjectEnd,
        Constants::VALID_DATA_GROUP_IDS,
        extensionInfo.validDataGroupIds,
        JsonType::ARRAY,
        false,
        parseResult,
        ArrayType::STRING);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::CUSTOM_PROCESS,
        extensionInfo.customProcess,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::ISOLATION_PROCESS,
        extensionInfo.isolationProcess,
        false,
        parseResult);
    BMSJsonUtil::GetBoolValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::SKIP_ABILITY_STAGE_LIFECYCLE,
        extensionInfo.skipAbilityStageLifecycle,
        false,
        parseResult);
    BMSJsonUtil::GetStrValueIfFindKey(jsonObject,
        jsonObjectEnd,
        Constants::ARKTS_MODE,
        extensionInfo.arkTSMode,
        false,
        parseResult);
    if (parseResult != ERR_OK) {
        APP_LOGE("ExtensionFromJson error : %{public}d", parseResult);
    }
}

template<typename T>
void ExtensionToJson(nlohmann::json &jsonObject, const T &extensionInfo)
{
    APP_LOGD("ExtensionToJson begin");
    jsonObject = nlohmann::json {
        {Constants::BUNDLE_NAME, extensionInfo.bundleName},
        {Constants::MODULE_NAME, extensionInfo.moduleName},
        {Constants::NAME, extensionInfo.name},
        {Constants::SRC_ENTRANCE, extensionInfo.srcEntrance},
        {Constants::ICON, extensionInfo.icon},
        {Constants::ICON_ID, extensionInfo.iconId},
        {Constants::LABEL, extensionInfo.label},
        {Constants::LABEL_ID, extensionInfo.labelId},
        {Constants::DESCRIPTION, extensionInfo.description},
        {Constants::DESCRIPTION_ID, extensionInfo.descriptionId},
        {Constants::PRIORITY, extensionInfo.priority},
        {Constants::TYPE, extensionInfo.type},
        {Constants::EXTENSION_TYPE_NAME, extensionInfo.extensionTypeName},
        {Constants::READ_PERMISSION, extensionInfo.readPermission},
        {Constants::WRITE_PERMISSION, extensionInfo.writePermission},
        {Constants::URI, extensionInfo.uri},
        {Constants::PERMISSIONS, extensionInfo.permissions},
        {Constants::APPIDENTIFIER_ALLOW_LIST, extensionInfo.appIdentifierAllowList},
        {Constants::VISIBLE, extensionInfo.visible},
        {Constants::META_DATA, extensionInfo.metadata},
        {Constants::RESOURCE_PATH, extensionInfo.resourcePath},
        {Constants::HAP_PATH, extensionInfo.hapPath},
        {Constants::ENABLED, extensionInfo.enabled},
        {Constants::PROCESS, extensionInfo.process},
        {Constants::COMPILE_MODE, extensionInfo.compileMode},
        {Constants::UID, extensionInfo.uid},
        {Constants::APP_INDEX, extensionInfo.appIndex},
        {Constants::EXTENSION_PROCESS_MODE, extensionInfo.extensionProcessMode},
        {Constants::SKILLS, extensionInfo.skills},
        {Constants::NEED_CREATE_SANDBOX, extensionInfo.needCreateSandbox},
        {Constants::DATA_GROUP_IDS, extensionInfo.dataGroupIds},
        {Constants::VALID_DATA_GROUP_IDS, extensionInfo.validDataGroupIds},
        {Constants::CUSTOM_PROCESS, extensionInfo.customProcess},
        {Constants::ISOLATION_PROCESS, extensionInfo.isolationProcess},
        {Constants::SKIP_ABILITY_STAGE_LIFECYCLE, extensionInfo.skipAbilityStageLifecycle},
        {Constants::ARKTS_MODE, extensionInfo.arkTSMode}
    };
}
} // namespace AppExecFwk
} // namespace OHOS
#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_COMMON_JSON_CONVERTER_H
