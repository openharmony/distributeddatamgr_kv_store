/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_KV_STORE_ANI_ABILITY_UTILS_H_
#define OHOS_KV_STORE_ANI_ABILITY_UTILS_H_

#include "taihe/runtime.hpp"
#include "ani_base_context.h"
#include "types.h"

namespace OHOS {
namespace DistributedKVStore {
using namespace OHOS;

struct ContextParam {
    std::string baseDir = "";
    std::string hapName = "";
    int32_t area = DistributedKv::Area::EL1;
    bool isSystemApp = false;
    int32_t apiVersion = 9;
};

int32_t GetHapVersion(ani_env *env, ani_object value);
std::shared_ptr<AbilityRuntime::Context> GetStageModeContext(ani_env *env, ani_object value);
int32_t AniGetContext(ani_object jsValue, ContextParam &param);
}
}
#endif