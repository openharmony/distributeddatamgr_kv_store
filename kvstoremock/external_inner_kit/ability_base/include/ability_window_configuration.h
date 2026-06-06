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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_WINDOW_CONFIGURATION_H
#define OHOS_ABILITY_RUNTIME_ABILITY_WINDOW_CONFIGURATION_H

namespace OHOS {
namespace AAFwk {
enum AbilityWindowConfiguration {
    MULTI_WINDOW_DISPLAY_UNDEFINED = 0,
    MULTI_WINDOW_DISPLAY_FULLSCREEN = 1,
    MULTI_WINDOW_DISPLAY_PRIMARY = 100,
    MULTI_WINDOW_DISPLAY_SECONDARY = 101,
    MULTI_WINDOW_DISPLAY_FLOATING = 102,
    MULTI_WINDOW_DISPLAY_SPLIT = 105
};
}
}

#endif