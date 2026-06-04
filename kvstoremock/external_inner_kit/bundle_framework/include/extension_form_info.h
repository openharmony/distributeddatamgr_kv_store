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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_EXTENSION_FORM_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_EXTENSION_FORM_INFO_H

#include <string>
#include <vector>

#include "form_info_base.h"
#include "iosfwd"

namespace OHOS {
namespace AppExecFwk {
struct ExtensionFormInfo {
    bool formVisibleNotify = false;
    bool isDefault = false;
    bool updateEnabled = false;
    bool dataProxyEnabled = false;
    bool isDynamic = true;
    bool transparencyEnabled = true;
    bool fontScaleFollowSystem = true;
    bool enableBlurBackground = false;
    bool appFormVisibleNotify = false;
    int32_t updateDuration = 0;
    int32_t defaultDimension = 0;
    FormsColorMode colorMode = FormsColorMode::AUTO_MODE;
    FormsRenderingMode renderingMode = FormsRenderingMode::FULL_COLOR;
    FormType type = FormType::JS;
    FormType uiSyntax = FormType::JS;
    std::string scheduledUpdateTime = "";
    std::string multiScheduledUpdateTime = "";
    std::string name;
    std::string displayName;
    std::string description;
    std::string src;
    std::string formConfigAbility;
    FormWindow window;
    bool resizable = false;
    std::string groupId;
    FormStandby standby;

    std::vector<int32_t> supportDimensions {};
    std::vector<int32_t> conditionUpdate {};
    std::vector<FormCustomizeData> metadata {};
    std::vector<int32_t> supportShapes {};
    std::vector<uint32_t> previewImages {};
    std::vector<std::string> supportDeviceTypes {};
    std::vector<int32_t> supportDevicePerformanceClasses {};
    FormFunInteractionParams funInteractionParams;
    FormSceneAnimationParams sceneAnimationParams;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_EXTENSION_FORM_INFO_H
