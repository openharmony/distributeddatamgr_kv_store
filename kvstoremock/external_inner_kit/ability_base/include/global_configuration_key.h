/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_BASE_GLOBAL_CONFIGURATION_KEY_H
#define OHOS_ABILITY_BASE_GLOBAL_CONFIGURATION_KEY_H

#include <string>

namespace OHOS {
namespace AAFwk {
namespace GlobalConfigurationKey {
    /* For the time being, there is no uniform standard */
    /* Must be synchronized with the keystore(SystemConfigurationKeyStore)in the configuration */
    constexpr const char* SYSTEM_LANGUAGE = "ohos.system.language";
    constexpr const char* IS_PREFERRED_LANGUAGE = "ohos.system.isPreferredLanguage";
    constexpr const char* SYSTEM_LOCALE = "ohos.system.locale";
    constexpr const char* SYSTEM_HOUR = "ohos.system.hour";
    constexpr const char* SYSTEM_COLORMODE = "ohos.system.colorMode";
    constexpr const char* INPUT_POINTER_DEVICE = "input.pointer.device";
    constexpr const char* DEVICE_TYPE = "const.build.characteristics";
    constexpr const char* COLORMODE_IS_SET_BY_APP = "ohos.system.colorMode.isSetByApp";
    constexpr const char* COLORMODE_IS_SET_BY_SA = "ohos.system.colorMode.isSetBySa";
    constexpr const char* THEME = "ohos.application.theme";
    constexpr const char* THEME_ID = "ohos.application.themeId";
    constexpr const char* THEME_ICON = "ohos.application.themeIcon";
    constexpr const char* THEME_SKIN = "ohos.application.themeSkin";
    constexpr const char* SYSTEM_FONT_ID = "ohos.system.fontId";
    constexpr const char* SYSTEM_FONT_SIZE_SCALE = "ohos.system.fontSizeScale";
    constexpr const char* SYSTEM_FONT_WEIGHT_SCALE = "ohos.system.fontWeightScale";
    constexpr const char* SYSTEM_MCC = "ohos.system.mcc";
    constexpr const char* SYSTEM_MNC = "ohos.system.mnc";
    constexpr const char* APPLICATION_FONT = "ohos.application.font";
    constexpr const char* APP_FONT_SIZE_SCALE = "ohos.app.fontSizeScale";
    constexpr const char* APP_FONT_MAX_SCALE = "ohos.app.fontMaxScale";
    // Used to notify arkui smart gesture switch
    constexpr const char* SYSTEM_SMART_GESTURE_SWITCH = "ohos.system.smartGesture";

} // namespace GlobalConfigurationKey
} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ABILITY_BASE_GLOBAL_CONFIGURATION_KEY_H
