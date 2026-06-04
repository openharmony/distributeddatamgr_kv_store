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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_STORAGE_STATS_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_BUNDLE_STORAGE_STATS_H

#include "appexecfwk_errors.h"
#include "parcel.h"
#include <string>
#include <vector>

namespace OHOS {
namespace AppExecFwk {

struct BundleStorageStats : public Parcelable {
    virtual ~BundleStorageStats() = default;
    std::string bundleName;
    std::vector<int64_t> bundleStats;
    int32_t errCode = ERR_OK;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static BundleStorageStats *Unmarshalling(Parcel &parcel);
};
} // AppExecFwk
} // OHOS
#endif
