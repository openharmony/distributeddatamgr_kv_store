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

#ifndef FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_QUICK_FIX_H
#define FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_QUICK_FIX_H

#include <string>

#include "appqf_info.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct AppQuickFix : public Parcelable {
    uint32_t versionCode = 0; // original bundle version code
    std::string bundleName; // original bundle name
    std::string versionName; // original bundle version name

    AppqfInfo deployedAppqfInfo; // deployed quick fix patch
    AppqfInfo deployingAppqfInfo; // deploying quick fix patch

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static AppQuickFix *Unmarshalling(Parcel &parcel);
};
}
}
#endif // FOUNDATION_BUNDLEMANAGER_BUNDLE_FRAMEWORK_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_QUICK_FIX_H