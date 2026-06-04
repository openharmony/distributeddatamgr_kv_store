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

#ifndef FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_RESOURCE_INFO_H
#define FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_RESOURCE_INFO_H

#include <string>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
enum class ResourceFlag {
    // Used to obtain the icon and label resource info
    GET_RESOURCE_INFO_ALL = 0x00000001,
    // Used to obtained the label resource info
    GET_RESOURCE_INFO_WITH_LABEL = 0x00000002,
    // Used to obtained the icon resource info
    GET_RESOURCE_INFO_WITH_ICON = 0x00000004,
    // Used to obtain the resource info sorted by label
    GET_RESOURCE_INFO_WITH_SORTED_BY_LABEL = 0x00000008,
    // Used to obtain the icon drawable object
    GET_RESOURCE_INFO_WITH_DRAWABLE_DESCRIPTOR = 0x00000010,
    // Used to obtain only main ability resource
    GET_RESOURCE_INFO_ONLY_WITH_MAIN_ABILITY = 0x00000020,
};

struct BundleResourceInfo : Parcelable {
    int32_t appIndex = 0;
    std::string bundleName;
    std::string label;
    std::string icon;

    std::vector<uint8_t> foreground;
    std::vector<uint8_t> background;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static BundleResourceInfo *Unmarshalling(Parcel &parcel);
};
} // AppExecFwk
} // OHOS
#endif // FOUNDATION_BUNDLEMANAGER_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_RESOURCE_INFO_H
