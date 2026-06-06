/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_AUTO_STARTUP_INFO_H
#define OHOS_ABILITY_RUNTIME_AUTO_STARTUP_INFO_H

#include <string>
#include "parcel.h"

namespace OHOS {
namespace AbilityRuntime {

enum class AutoStartupSetterType : int32_t {
    UNSPECIFIED = -1,
    SYSTEM = 0,
    USER = 1,
};

struct AutoStartupInfo : public Parcelable {
public:
    int32_t appCloneIndex = 0;
    int32_t userId = -1;
    int32_t setterUserId = -1;
    bool canUserModify = false;
    bool isHiddenStart = false;
    std::string bundleName;
    std::string abilityName;
    std::string moduleName;
    std::string abilityTypeName;
    std::string accessTokenId;
    int32_t retryCount = 0;
    AutoStartupSetterType setterType = AutoStartupSetterType::UNSPECIFIED;

    bool ReadFromParcel(Parcel &parcel) { return true; }
    virtual bool Marshalling(Parcel &parcel) const override { return true; }
    static AutoStartupInfo *Unmarshalling(Parcel &parcel) { return new AutoStartupInfo(); }
};

struct AutoStartupStatus {
    bool isAutoStartup = false;
    bool isEdmForce = false;
    int32_t setterUserId = -1;
    AutoStartupSetterType setterType = AutoStartupSetterType::UNSPECIFIED;
    int32_t code = -1;
    bool isHiddenStart = false;
};

struct AutoStartupAbilityData {
    std::string abilityTypeName;
    std::string accessTokenId;
    int32_t userId = -1;
    int32_t setterUserId = -1;
    bool isVisible = false;
    bool hasStatusBarExtension = false;
};

} // namespace AbilityRuntime
} // namespace OHOS
#endif // OHOS_ABILITY_RUNTIME_AUTO_STARTUP_INFO_H