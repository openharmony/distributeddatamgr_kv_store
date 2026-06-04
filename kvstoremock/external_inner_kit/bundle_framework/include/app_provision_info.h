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

#ifndef FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_PROVISION_INFO_H
#define FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_PROVISION_INFO_H

#include <string>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct Validity {
    int64_t notBefore = 0;
    int64_t notAfter = 0;
};

// configuration information about a profile
struct AppProvisionInfo : public Parcelable {
    uint32_t versionCode = 0;
    std::string versionName;
    std::string uuid;
    std::string type;
    std::string appDistributionType;
    std::string developerId;
    std::string certificate;
    std::string apl;
    std::string issuer;
    std::string appIdentifier;
    std::string appServiceCapabilities;
    std::string organization;
    std::string bundleName;
    Validity validity;
    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static AppProvisionInfo *Unmarshalling(Parcel &parcel);
};
} // AppExecFwk
} // OHOS

#endif // FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_PROVISION_INFO_H
