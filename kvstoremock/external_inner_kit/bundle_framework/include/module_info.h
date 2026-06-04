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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_MODULE_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_MODULE_INFO_H

#include <string>
#include <vector>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
enum ProfileType : int8_t {
    UNSPECIFIED_PROFILE = 0,
    INTENT_PROFILE = 1,
    ADDITION_PROFILE = 2,
    NETWORK_PROFILE = 3,
    UTD_SDT_PROFILE = 4,
    PKG_CONTEXT_PROFILE = 5,
    FILE_ICON_PROFILE = 6,
    INSIGHT_INTENT_PROFILE = 7,
    CLOUD_PROFILE = 8,
    EASY_GO_PROFILE = 9,
    SHARE_FILES_PROFILE = 10,
};

enum AppDistributionTypeEnum : int8_t {
    APP_DISTRIBUTION_TYPE_APP_GALLERY = 1,
    APP_DISTRIBUTION_TYPE_ENTERPRISE = 2,
    APP_DISTRIBUTION_TYPE_ENTERPRISE_NORMAL = 3,
    APP_DISTRIBUTION_TYPE_ENTERPRISE_MDM = 4,
    APP_DISTRIBUTION_TYPE_INTERNALTESTING = 5,
    APP_DISTRIBUTION_TYPE_CROWDTESTING = 6,
};

struct JsonProfileInfo : public Parcelable {
    ProfileType profileType = ProfileType::UNSPECIFIED_PROFILE;
    std::string bundleName;
    std::string moduleName;
    std::string profile;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static JsonProfileInfo *Unmarshalling(Parcel &parcel);
    std::string ToString() const;
};

struct BundleProfileData {
    std::string bundleName;
    std::string moduleName;
    std::string hapPath;
    std::string profilePath;
};

// stores module information about an application
struct ModuleInfo : public Parcelable {
    std::string moduleName;  // the "name" in module part in config.json
    std::string moduleSourceDir;
    std::vector<std::string> preloads;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static ModuleInfo *Unmarshalling(Parcel &parcel);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_MODULE_INFO_H
