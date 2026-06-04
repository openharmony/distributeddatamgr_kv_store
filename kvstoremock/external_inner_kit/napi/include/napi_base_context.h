/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_ABILITYRUNTIME_NAPI_BASE_CONTEXT_H
#define FOUNDATION_ABILITYRUNTIME_NAPI_BASE_CONTEXT_H

#include "napi/native_api.h"
#include "extension_context.h"
#include "ability.h"
#include <memory>

namespace OHOS {
namespace AbilityRuntime {
class NapiBaseContext {
public:
    static napi_value CreateJsBaseContext(napi_env env, std::shared_ptr<Context> context, bool keepContext = false);
    static void* GetContextFromInstance(napi_env env, napi_value value);
};

napi_status IsStageContext(napi_env env, napi_value value, bool &mode);

std::shared_ptr<Context> GetStageModeContext(napi_env env, napi_value value);

std::shared_ptr<AppExecFwk::Ability> GetCurrentAbility(napi_env env);

}  // namespace AbilityRuntime
}  // namespace OHOS

#endif /* FOUNDATION_ABILITYRUNTIME_NAPI_BASE_CONTEXT_H */