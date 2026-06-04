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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_EXTENSION_FORM_PROFILE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_EXTENSION_FORM_PROFILE_H

#include <string>
#include <vector>
#include "appexecfwk_errors.h"
#include "extension_form_info.h"

namespace OHOS {
namespace AppExecFwk {
namespace ExtensionFormProfileReader {
constexpr const char* FORMS = "forms";
constexpr const char* NAME = "name";
constexpr const char* DISPLAY_NAME = "displayName";
constexpr const char* DESCRIPTION = "description";
constexpr const char* SRC = "src";
constexpr const char* WINDOW = "window";
constexpr const char* WINDOW_DESIGN_WIDTH = "designWidth";
constexpr const char* WINDOW_AUTO_DESIGN_WIDTH = "autoDesignWidth";
constexpr const char* COLOR_MODE = "colorMode";
constexpr const char* RENDERING_MODE = "renderingMode";
constexpr const char* FORM_CONFIG_ABILITY = "formConfigAbility";
constexpr const char* TYPE = "type";
constexpr const char* UI_SYNTAX = "uiSyntax";
constexpr const char* FORM_VISIBLE_NOTIFY = "formVisibleNotify";
constexpr const char* IS_DEFAULT = "isDefault";
constexpr const char* UPDATE_ENABLED = "updateEnabled";
constexpr const char* SCHEDULED_UPDATE_TIME = "scheduledUpdateTime";
constexpr const char* MULTI_SCHEDULED_UPDATE_TIME = "multiScheduledUpdateTime";
constexpr const char* UPDATE_DURATION = "updateDuration";
constexpr const char* DEFAULT_DIMENSION = "defaultDimension";
constexpr const char* SUPPORT_DIMENSIONS = "supportDimensions";
constexpr const char* CONDITION_UPDATE = "conditionUpdate";
constexpr const char* METADATA = "metadata";
constexpr const char* PREVIEW_IMAGES = "previewImages";
constexpr const char* METADATA_NAME = "name";
constexpr const char* METADATA_VALUE = "value";
constexpr const char* DATA_PROXY_ENABLED = "dataProxyEnabled";
constexpr const char* IS_DYNAMIC = "isDynamic";
constexpr const char* TRANSPARENCY_ENABLED = "transparencyEnabled";
constexpr const char* PRIVACY_LEVEL = "privacyLevel";
constexpr const char* FONT_SCALE_FOLLOW_SYSTEM = "fontScaleFollowSystem";
constexpr const char* SUPPORT_SHAPES = "supportShapes";
constexpr const char* ENABLE_BLUR_BACKGROUND = "enableBlurBackground";
constexpr const char* FUN_INTERACTION_PARAMS = "funInteractionParams";
constexpr const char* SCENE_ANIMATION_PARAMS = "sceneAnimationParams";
constexpr const char* ABILITY_NAME = "abilityName";
constexpr const char* DISABLED_DESKTOP_BEHAVIORS = "disabledDesktopBehaviors";
constexpr const char* TARGET_BUNDLE_NAME = "targetBundleName";
constexpr const char* SUB_BUNDLE_NAME = "subBundleName";
constexpr const char* KEEP_STATE_DURATION = "keepStateDuration";
constexpr const char* RESIZABLE = "resizable";
constexpr const char* GROUP_ID = "groupId";
constexpr const char* SUPPORT_DEVICE_TYPES = "supportDeviceTypes";
constexpr const char* SUPPORT_DEVICE_PERFORMANCE_CLASSES = "supportDevicePerformanceClasses";
constexpr const char* STANDBY = "standby";
constexpr const char* STANDBY_IS_SUPPORTED = "isSupported";
constexpr const char* STANDBY_IS_ADAPTED = "isAdapted";
constexpr const char* STANDBY_IS_PRIVACY_SENSITIVE = "isPrivacySensitive";
constexpr const char* TRIGGER_TYPES = "triggerTypes";
}

class ExtensionFormProfile {
public:
    /**
     * @brief Transform the form profile to ExtensionFormInfos.
     * @param formProfile Indicates the string of the form profile.
     * @param infos Indicates the obtained ExtensionFormProfileInfo.
     * @param privacyLevel Indicates the form data privacy level.
     * @return Returns ERR_OK if the information transformed successfully; returns error code otherwise.
     */
    static ErrCode TransformTo(
        const std::string &formProfile, std::vector<ExtensionFormInfo> &infos, int32_t &privacyLevel);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_EXTENSION_FORM_PROFILE_H
