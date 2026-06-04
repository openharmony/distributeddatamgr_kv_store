/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_OVERLAY_MODULE_INFO_H
#define FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_OVERLAY_MODULE_INFO_H

#include <string>

#include "parcel.h"
namespace OHOS {
namespace AppExecFwk {
enum OverlayState : int8_t {
    OVERLAY_DISABLED = 0,
    OVERLAY_ENABLE,
    OVERLAY_INVALID,
};

struct OverlayModuleInfo : public Parcelable {
    int32_t priority = 0;
    int32_t state = OVERLAY_INVALID; // 0 is for disable and 1 is for enable
    std::string bundleName;
    std::string moduleName;
    std::string targetModuleName;
    std::string hapPath;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static OverlayModuleInfo *Unmarshalling(Parcel &parcel);
};
}
}
#endif // FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_OVERLAY_MODULE_INFO_H