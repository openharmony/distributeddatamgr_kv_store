/**
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
#ifndef INTERFACES_INNER_KITS_ACCESSTOKEN_PERMISSION_DEF_H
#define INTERFACES_INNER_KITS_ACCESSTOKEN_PERMISSION_DEF_H

#include <string>
#include "access_token.h"

namespace OHOS {
namespace Security {
namespace AccessToken {
class PermissionDef final {
public:
    std::string permissionName;
    std::string bundleName = "";
    int grantMode;
    ATokenAplEnum availableLevel;
    bool provisionEnable;
    bool distributedSceneEnable;
    std::string label = "";
    int labelId = 0;
    std::string description = "";
    int descriptionId = 0;
    ATokenAvailableTypeEnum availableType = NORMAL;
    bool isKernelEffect = false;
    bool hasValue = false;
};

class PermissionDefData final {
public:
    AccessTokenID tokenId;
    PermissionDef permDef;
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif // INTERFACES_INNER_KITS_ACCESSTOKEN_PERMISSION_DEF_H