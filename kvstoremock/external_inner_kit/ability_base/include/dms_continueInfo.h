/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_DMS_CONTINUEINFO_INFO_H
#define OHOS_ABILITY_RUNTIME_DMS_CONTINUEINFO_INFO_H

#include <string>

#include "parcel.h"
#include "want.h"
#include "want_params.h"

namespace OHOS {
namespace AAFwk {
/**
 * @struct AutoStartupInfo
 * Defines auto startup info.
 */
struct ContinueMissionInfo  {
    std::string dstDeviceId;
    std::string srcDeviceId;
    std::string bundleName;
    std::string srcBundleName;
    std::string continueType;
    AAFwk::WantParams wantParams;
};
} // namespace AAFwk
} // namespace OHOS
#endif // OHOS_ABILITY_RUNTIME_DMS_CONTINUEINFO_INFO_H