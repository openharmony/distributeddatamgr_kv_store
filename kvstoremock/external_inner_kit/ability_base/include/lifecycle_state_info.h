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

#ifndef OHOS_ABILITY_RUNTIME_LIFECYCLE_STATE_INFO_H
#define OHOS_ABILITY_RUNTIME_LIFECYCLE_STATE_INFO_H

#include <string>

#include "ability_start_setting.h"
#include "caller_info.h"
#include "launch_param.h"
#include "parcel.h"

namespace OHOS {
namespace AAFwk {
/**
 * @enum AbilityLifeCycleState
 * AbilityLifeCycleState defines the life cycle state of ability.
 */
enum AbilityLifeCycleState {
    ABILITY_STATE_INITIAL = 0,
    ABILITY_STATE_INACTIVE,
    ABILITY_STATE_ACTIVE,
    ABILITY_STATE_BACKGROUND,
    ABILITY_STATE_STARTED_NEW,
    ABILITY_STATE_FOREGROUND_NEW,
    ABILITY_STATE_BACKGROUND_NEW,
    ABILITY_STATE_STOPED_NEW,
    ABILITY_STATE_FOREGROUND_FAILED,
    ABILITY_STATE_BACKGROUND_FAILED,
    ABILITY_STATE_INVALID_WINDOW_MODE,
    ABILITY_STATE_WINDOW_FREEZED,
    ABILITY_STATE_DO_NOTHING,
};

enum class ErrorLifecycleState {
    ABILITY_STATE_LOAD_TIMEOUT = 0,
    ABILITY_STATE_FOREGROUND_TIMEOUT,
    ABILITY_STATE_DIED,
    ABILITY_STATE_PERMISSION_UPDATE,
    ABILITY_STATE_LOW_MEMORY_KILL,
    ABILITY_STATE_SKIP_KILL_IN_STARTUP,
    ABILITY_STATE_INTERCEPTOR_REJECT,
};

/**
 * @struct LifeCycleStateInfo
 * LifeCycleStateInfo is used to save information about ability life cycle state.
 */
struct LifeCycleStateInfo : public Parcelable {
    AbilityLifeCycleState state = AbilityLifeCycleState::ABILITY_STATE_INITIAL;
    bool isNewWant = false;
    int missionId = -1;
    uint32_t sceneFlag = 0;
    uint32_t sceneFlagBak = 0;
    std::shared_ptr<AbilityStartSetting> setting = nullptr;
    LaunchParam launchParam;
    CallerInfo caller;
    std::string pageConfig;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static LifeCycleStateInfo *Unmarshalling(Parcel &parcel);
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_LIFECYCLE_STATE_INFO_H
