/*
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_RUNNING_PROCESS_INFO_H
#define OHOS_ABILITY_RUNTIME_RUNNING_PROCESS_INFO_H

#include <string>
#include <vector>

#include "ability_info.h"
//#include "app_mgr_constants.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
enum class AppProcessState {
    APP_STATE_CREATE = 0,
    APP_STATE_READY,
    APP_STATE_FOREGROUND,
    APP_STATE_FOCUS,
    APP_STATE_BACKGROUND,
    APP_STATE_TERMINATED,
    APP_STATE_END,
    APP_STATE_CACHED = 100,
    APP_STATE_PRE_FOREGROUND = 101,
};

enum class WeightReasonCode {
    REASON_UNKNOWN = 0,
    WEIGHT_FOREGROUND = 100,
    WEIGHT_FOREGROUND_SERVICE = 125,
    WEIGHT_VISIBLE = 200,
    WEIGHT_PERCEPTIBLE = 230,
    WEIGHT_SERVICE = 300,
    WEIGHT_TOP_SLEEPING = 325,
    WEIGHT_CANT_SAVE_STATE = 350,
    WEIGHT_CACHED = 400,
    WEIGHT_GONE = 1000,
};

enum class PreloadMode {
    PRELOAD_NONE = -1,
    PRESS_DOWN = 0,
    PRE_MAKE = 1,
    PRELOAD_MODULE = 2,
    PRE_LAUNCH = 4,
};

enum class ProcessType {
    NORMAL = 0,
    EXTENSION,
    RENDER,
    GPU,
    CHILD,
};

struct RunningProcessInfo : public Parcelable {
    bool isContinuousTask = false;
    bool isKeepAlive = false;
    bool isKeepAliveAppService = false;
    bool isFocused = false;
    bool isTestProcess = false;
    bool isAbilityForegrounding = false;
    bool isTestMode = false;
    bool isStrictMode = false;
    bool isDebugApp = false;
    bool isExiting = false;
    bool isPreForeground = false;
    bool isPreload = false;
    std::int32_t pid_;
    std::int32_t uid_;
    std::uint32_t accessTokenId_ = 0;
    std::int32_t bundleType = 0;
    std::int32_t appCloneIndex = -1;
    std::int32_t rssValue = 0;
    std::int32_t pssValue = 0;
    PreloadMode preloadMode_ = PreloadMode::PRELOAD_NONE;
    AppProcessState state_ = AppProcessState::APP_STATE_CREATE;
    std::int64_t startTimeMillis_;
    std::vector<std::string> bundleNames;
    std::string processName_;
    std::string instanceKey = "";
    AppExecFwk::MultiAppModeType appMode = AppExecFwk::MultiAppModeType::UNSPECIFIED;
 
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static RunningProcessInfo *Unmarshalling(Parcel &parcel);
    ProcessType processType_ = ProcessType::NORMAL;
    ExtensionAbilityType extensionType_ = ExtensionAbilityType::UNSPECIFIED;
    bool isCached = false;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // OHOS_ABILITY_RUNTIME_RUNNING_PROCESS_INFO_H
