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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_RECOVERABLE_APPLICATION_INFO_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_RECOVERABLE_APPLICATION_INFO_H

#include <string>

#include "application_info.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
struct RecoverableApplicationInfo : public Parcelable {
    bool systemApp = false;
    uint32_t labelId = 0;
    uint32_t iconId = 0;
    BundleType bundleType = BundleType::APP;
    std::string bundleName;
    std::string moduleName;
    std::vector<std::string> codePaths;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static RecoverableApplicationInfo *Unmarshalling(Parcel &parcel);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_SHARED_BUNDLE_INFO_H
