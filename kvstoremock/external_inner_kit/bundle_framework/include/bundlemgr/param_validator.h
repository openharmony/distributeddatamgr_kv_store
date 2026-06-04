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

#ifndef FOUNDATION_BUNDLE_MANAGER_APPEXECFWK_CORE_PARAM_VALIDATOR_H
#define FOUNDATION_BUNDLE_MANAGER_APPEXECFWK_CORE_PARAM_VALIDATOR_H

#include <string>
#include <vector>

#include "errors.h"

namespace OHOS {
namespace AppExecFwk {
class ParamValidator final {
public:
    static ErrCode ValidateAbilityFileTypes(const std::string &moduleName, const std::string &abilityName,
        const std::vector<std::string> &fileTypes);
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_BUNDLE_MANAGER_APPEXECFWK_CORE_PARAM_VALIDATOR_H
