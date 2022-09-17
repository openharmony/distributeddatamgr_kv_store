/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ABILITY_RUNTIME_NAPI_BASE_CONTEXT_H
#define ABILITY_RUNTIME_NAPI_BASE_CONTEXT_H

#include <memory>

#include "napi/native_api.h"

namespace OHOS {
namespace AppExecFwk {
class Ability;
}
namespace AbilityRuntime {
napi_value* GetFAModeContextClassObject();

napi_status IsStageContext(napi_env env, napi_value object, bool& stageMode);

class Context;
std::shared_ptr<Context> GetStageModeContext(napi_env env, napi_value object);

AppExecFwk::Ability* GetCurrentAbility(napi_env env);
}  // namespace AbilityRuntime
}  // namespace OHOS
#endif  // ABILITY_RUNTIME_NAPI_BASE_CONTEXT_H