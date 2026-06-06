/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_STATE_H
#define OHOS_ABILITY_RUNTIME_ABILITY_STATE_H

#include <string>

namespace OHOS {
namespace AAFwk {

enum AbilityState {
    INITIAL = 0,
    INACTIVE,
    ACTIVE,
    INACTIVATING = 5,
    ACTIVATING,
    TERMINATING = 8,
    FOREGROUND,
    BACKGROUND,
    FOREGROUNDING,
    BACKGROUNDING,
    FOREGROUND_FAILED,
    BACKGROUND_FAILED,
    FOREGROUND_INVALID_MODE,
    FOREGROUND_WINDOW_FREEZED,
    FOREGROUND_DO_NOTHING,
};

enum Reason {
    REASON_MIN = 0,
    REASON_UNKNOWN = REASON_MIN,
    REASON_NORMAL,
    REASON_CPP_CRASH,
    REASON_JS_ERROR,
    REASON_CJ_ERROR,
    REASON_APP_FREEZE,
    REASON_PERFORMANCE_CONTROL,
    REASON_RESOURCE_CONTROL,
    REASON_UPGRADE,
    REASON_USER_REQUEST,
    REASON_SIGNAL,
    REASON_MAX = REASON_SIGNAL,
};

enum UserStatus {
    ASSERT_TERMINATE = 0,
    ASSERT_CONTINUE,
    ASSERT_RETRY,
};

enum class AbilityNativeState : int8_t {
    NONE,
    NORMAL,
    INIT,
    ATTACHED,
    CREATED,
    ON_FOREGROUND,
};

}
}

#endif