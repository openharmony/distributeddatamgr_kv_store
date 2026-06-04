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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_ALTERNATE_ICON_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_ALTERNATE_ICON_INFO_H

#include <string>

#include "bundle_constants.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct AlternateIconInfo : public Parcelable {
    std::string alternateIconName;
    std::string filePath;
    int32_t userId = Constants::INVALID_USERID;
    uint32_t iconId = 0;
    bool enabled = false;

    bool ReadFromParcel(Parcel &parcel);
    bool Marshalling(Parcel &parcel) const override;
    static AlternateIconInfo *Unmarshalling(Parcel &parcel);
};
} // AppExecFwk
} // OHOS
#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_ALTERNATE_ICON_INFO_H