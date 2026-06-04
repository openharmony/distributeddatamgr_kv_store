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

#ifndef FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_LAUNCHER_ABILITYINFO_H
#define FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_LAUNCHER_ABILITYINFO_H

#include <string>

#include "application_info.h"
#include "element_name.h"

namespace OHOS {
namespace AppExecFwk {
struct LauncherAbilityInfo {
    uint32_t labelId = 0;
    uint32_t iconId = 0;
    int32_t userId = Constants::UNSPECIFIED_USERID;
    int64_t installTime = 0;
    ApplicationInfo applicationInfo;
    ElementName elementName; // bundle name , ability name, deviceid
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_SERVICES_BUNDLEMGR_INCLUDE_LAUNCHER_ABILITYINFO_H
