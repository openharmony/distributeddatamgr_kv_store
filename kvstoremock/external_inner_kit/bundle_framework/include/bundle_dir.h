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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_DIR_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_DIR_H

#include <string>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
enum class DataDirEl : uint8_t {
    NONE = 0,
    EL1 = 1,
    EL2 = 2,
    EL3 = 3,
    EL4 = 4,
    EL5 = 5,
};
struct BundleDir : public Parcelable {
    int32_t appIndex = 0;
    std::string bundleName;
    std::string dir;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static BundleDir *Unmarshalling(Parcel &parcel);
    std::string ToString() const;
};
} // AppExecFwk
} // OHOS

#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_DIR_H
