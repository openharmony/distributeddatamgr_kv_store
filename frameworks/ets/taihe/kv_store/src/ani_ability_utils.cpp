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
#define LOG_TAG "AniAbilityUtils"
#include "ani_ability_utils.h"
#include "ani_utils.h"
#include "log_print.h"

#include <string>

namespace ani_abilityutils {

using namespace taihe;
using namespace OHOS;
using namespace OHOS::DistributedKVStore;

static constexpr int32_t INVALID_HAP_VERSION = -1;
static constexpr int DEFAULT_API_VERSION = 9;

#define API_VERSION_MOD 100

int32_t GetHapVersion(ani_env *env, ani_object value)
{
    auto stageContext = OHOS::AbilityRuntime::GetStageModeContext(env, value);
    if (stageContext == nullptr) {
        ZLOGE("GetStageModeContext failed.");
        return INVALID_HAP_VERSION ;
    }
    auto appInfo = stageContext->GetApplicationInfo();
    if (appInfo != nullptr) {
        return appInfo->apiTargetVersion % API_VERSION_MOD;
    }
    ZLOGW("GetApplicationInfo failed.");
    return INVALID_HAP_VERSION ;
}

int32_t GetApiVersion(ani_env* env, ani_object value)
{
    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, value);
    if (context == nullptr) {
        ZLOGW("get context fail.");
        return DEFAULT_API_VERSION;
    }
    auto appInfo = context->GetApplicationInfo();
    if (appInfo == nullptr) {
        ZLOGW("get app info fail.");
        return DEFAULT_API_VERSION;
    }
    return appInfo->apiTargetVersion % API_VERSION_MOD;
}

std::shared_ptr<AbilityRuntime::Context> GetStageModeContext(ani_env *env, ani_object value)
{
    return OHOS::AbilityRuntime::GetStageModeContext(env, value);
}

int32_t AniGetContext(ani_object jsValue, ContextParam &param)
{
    ani_env *env = taihe::get_env();
    if (jsValue == nullptr) {
        ZLOGE("jsValue nullptr");
        return ANI_INVALID_ARGS;
    }
    int32_t status = ani_utils::AniGetProperty(env, jsValue, "databaseDir", param.baseDir);
    if (status != ANI_OK) {
        ZLOGE("get databaseDir failed.");
        return ANI_INVALID_ARGS;
    }
    status = ani_utils::AniGetProperty(env, jsValue, "area", param.area, true);
    if (status != ANI_OK) {
        ZLOGE("get area failed.");
        return ANI_INVALID_ARGS;
    }
    ani_object hapInfo = nullptr;
    status = ani_utils::AniGetProperty(env, jsValue, "currentHapModuleInfo", hapInfo);
    if (status != ANI_OK) {
        ZLOGE("get currentHapModuleInfo failed.");
        return ANI_INVALID_ARGS;
    }
    if (hapInfo != nullptr) {
        status = ani_utils::AniGetProperty(env, hapInfo, "name", param.hapName);
        if (status != ANI_OK) {
            ZLOGE("get hap name failed");
            return ANI_INVALID_ARGS;
        }
    }
    ani_object appInfo = nullptr;
    status = ani_utils::AniGetProperty(env, jsValue, "applicationInfo", appInfo);
    if (status != ANI_OK) {
        ZLOGE("get applicationInfo failed.");
        return ANI_INVALID_ARGS;
    }
    if (appInfo != nullptr) {
        status = ani_utils::AniGetProperty(env, appInfo, "systemApp", param.isSystemApp, true);
        if (status != ANI_OK) {
            ZLOGE("get applicationInfo.systemApp failed");
            return ANI_INVALID_ARGS;
        }
        param.apiVersion = GetApiVersion(env, jsValue);
    }
    return ANI_OK;
}

} //namespace ani_abilityutils