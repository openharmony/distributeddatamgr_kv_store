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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_FORM_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_FORM_INFO_H

#include <string>
#include "extension_ability_info.h"
#include "extension_form_info.h"
#include "form_info_base.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct FormInfo : public Parcelable {
    bool defaultFlag = false;
    bool formVisibleNotify = false;
    bool updateEnabled = false;
    bool isStatic = true;
    bool dataProxyEnabled = false;
    bool isDynamic = true;
    bool transparencyEnabled = false;
    bool fontScaleFollowSystem = true;
    bool enableBlurBackground = false;
    bool appFormVisibleNotify = false;
    FormsColorMode colorMode = FormsColorMode::AUTO_MODE;
    FormsRenderingMode renderingMode = FormsRenderingMode::FULL_COLOR;
    uint32_t displayNameId = 0;
    uint32_t descriptionId = 0;
    uint32_t versionCode = 0;
    int32_t updateDuration = 0;
    int32_t defaultDimension = 0;
    int32_t privacyLevel = 0;
    FormType type = FormType::JS;
    FormType uiSyntax = FormType::JS;
    BundleType bundleType = BundleType::APP;
    FormWindow window;
    std::string package;
    std::string bundleName;
    std::string originalBundleName;
    std::string relatedBundleName;
    std::string moduleName;  // the "module.distro.moduleName" in config.json
    std::string abilityName;
    std::string name;
    std::string displayName;
    std::string description;
    std::string jsComponentName;
    std::string deepLink;
    std::string formConfigAbility;
    std::string scheduledUpdateTime = "";
    std::string multiScheduledUpdateTime = "";
    std::string src;
    std::vector<int32_t> supportDimensions;
    std::vector<std::string> landscapeLayouts;
    std::vector<std::string> portraitLayouts;
    std::vector<FormCustomizeData> customizeDatas;
    std::vector<int32_t> conditionUpdate;
    std::vector<int32_t> supportShapes;
    std::vector<uint32_t> formPreviewImages;
    std::vector<std::string> supportDeviceTypes;
    std::vector<int32_t> supportDevicePerformanceClasses;
    FormFunInteractionParams funInteractionParams;
    FormSceneAnimationParams sceneAnimationParams;
    bool resizable = false;
    bool isTemplateForm = false;
    std::string groupId;
    FormStandby standby;

    FormInfo() = default;
    explicit FormInfo(const ExtensionAbilityInfo &abilityInfo, const ExtensionFormInfo &formInfo);

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static FormInfo *Unmarshalling(Parcel &parcel);
    bool IsValid() const;

private:
    bool ReadCustomizeData(Parcel &parcel);
    void SetInfoByAbility(const ExtensionAbilityInfo &abilityInfo);
    void SetInfoByFormExt(const ExtensionFormInfo &formInfo);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_FORM_H
