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

#ifndef FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_HQF_INFO_H
#define FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_HQF_INFO_H

#include <string>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
enum class QuickFixType : int8_t {
    UNKNOWN = 0,
    PATCH = 1,
    HOT_RELOAD = 2
};

struct HqfInfo : public Parcelable {
    QuickFixType type = QuickFixType::UNKNOWN; // quick fix type
    std::string moduleName;
    std::string hapSha256;
    std::string hqfFilePath;
    std::string cpuAbi;
    std::string nativeLibraryPath;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static HqfInfo *Unmarshalling(Parcel &parcel);
};
}
}
#endif // FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_HQF_INFO_H