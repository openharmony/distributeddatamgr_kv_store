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

#ifndef FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_INSTALL_EXTENDED_INFO_H
#define FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_INSTALL_EXTENDED_INFO_H

#include <map>
#include <string>
#include "parcel.h"
#include "shared/shared_bundle_info.h"


namespace OHOS {
namespace AppExecFwk {
struct AppInstallExtendedInfo : public Parcelable {
    uint32_t compatibleVersion = 0;
    uint32_t compatibleMinorVersion = 0;
    uint32_t compatiblePatchVersion = 0;
    int64_t crowdtestDeadline = 0;
    std::string bundleName;
    std::string specifiedDistributionType;
    std::string installSource;
    std::string additionalInfo;
    std::map<std::string, std::string> hashParam;
    std::vector<std::string> hapPath;
    std::map<std::string, std::map<std::string, std::vector<std::string>>> requiredDeviceFeatures;
    std::vector<SharedBundleInfo> sharedBundleInfos;

    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static AppInstallExtendedInfo *Unmarshalling(Parcel &parcel);
};
} // AppExecFwk
} // OHOS

#endif // FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_INSTALL_EXTENDED_INFO_H
