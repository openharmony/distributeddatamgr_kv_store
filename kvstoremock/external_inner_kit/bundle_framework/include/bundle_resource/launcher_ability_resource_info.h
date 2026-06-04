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

#ifndef FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_LAUNCHER_ABILITY_RESOURCE_INFO_H
#define FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_LAUNCHER_ABILITY_RESOURCE_INFO_H

#include <string>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct LauncherAbilityResourceInfo : Parcelable {
    int32_t appIndex = 0;
    int32_t extensionAbilityType = -1;
    bool isDefaultApp = false;
    std::string bundleName;
    std::string moduleName;
    std::string abilityName;
    std::string label;
    std::string icon;

    std::vector<uint8_t> foreground;
    std::vector<uint8_t> background;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static LauncherAbilityResourceInfo *Unmarshalling(Parcel &parcel);
};
} // AppExecFwk
} // OHOS
#endif // FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_LAUNCHER_ABILITY_RESOURCE_INFO_H
