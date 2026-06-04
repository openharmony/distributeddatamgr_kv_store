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

#ifndef FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_SKILL_SKILL_INFO_H
#define FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_SKILL_SKILL_INFO_H

#include <string>
#include <vector>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {

enum class SkillType {
    APP_SKILL = 0,
    INDEPENDENT_SKILL = 1,
};

enum class SkillInfoFlag {
    GET_SKILL_INFO_DEFAULT = 0x00000000,
    GET_SKILL_INFO_WITH_DESCRIPTION = 0x00000001,
    GET_SKILL_INFO_WITH_SRC_ENTRIES = 0x00000002,
    GET_SKILL_INFO_WITH_PERMISSIONS = 0x00000004,
    GET_SKILL_INFO_WITH_REQUEST_PERMISSIONS = 0x00000008,
};

struct SkillInfo : public Parcelable {
    std::string bundleName;
    std::string moduleName;
    std::string skillName;
    SkillType skillType = SkillType::APP_SKILL;
    std::string hapPath;
    std::string skillPath;
    uint32_t versionCode = 0;
    std::string abilityName;
    std::string description;
    std::vector<std::string> srcEntries;
    std::vector<std::string> permissions;
    std::vector<std::string> requestPermissions;

    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static SkillInfo *Unmarshalling(Parcel &parcel);
};

} // namespace AppExecFwk
} // namespace OHOS
#endif // FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_SKILL_SKILL_INFO_H
