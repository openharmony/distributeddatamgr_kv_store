/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_QUICK_FIX_INFO_H
#define FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_QUICK_FIX_INFO_H

#include <string>
#include <vector>

#include "hqf_info.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct AppqfInfo : public Parcelable {
    QuickFixType type = QuickFixType::UNKNOWN; // quick fix type
    uint32_t versionCode = 0; // quick fix version code
    std::string versionName; // quick fix version name
    std::string cpuAbi; // quick fix abi
    std::string nativeLibraryPath; // quick fix so path
    std::vector<HqfInfo> hqfInfos;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static AppqfInfo *Unmarshalling(Parcel &parcel);
};
}
}
#endif // FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_QUICK_FIX_INFO_H