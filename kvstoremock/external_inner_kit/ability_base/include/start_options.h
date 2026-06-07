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

#ifndef OHOS_ABILITY_RUNTIME_START_OPTIONS_H
#define OHOS_ABILITY_RUNTIME_START_OPTIONS_H

#include <string>
#include <functional>
#include "ability_info.h"
#include "ability_window_configuration.h"
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
class ElementName;
}
namespace AbilityRuntime {
using OnRequestResult = std::function<void(const AppExecFwk::ElementName&, const std::string&)>;
using OnAtomicRequestSuccess = std::function<void(const std::string&)>;
using OnAtomicRequestFailure = std::function<void(const std::string&, int32_t, const std::string&)>;
}

namespace AAFwk {

class StartOptions : public Parcelable {
public:
    StartOptions() = default;
    ~StartOptions() = default;
    StartOptions(const StartOptions &other) = default;
    StartOptions &operator=(const StartOptions &other) = default;

    void SetWindowMode(int32_t windowMode) { windowMode_ = windowMode; }
    int32_t GetWindowMode() const { return windowMode_; }

    void SetDisplayID(int32_t displayId) { displayId_ = displayId; }
    int32_t GetDisplayID() const { return displayId_; }

    void SetWindowLeft(int32_t windowLeft) { windowLeft_ = windowLeft; }
    int32_t GetWindowLeft() const { return windowLeft_; }

    void SetWindowTop(int32_t windowTop) { windowTop_ = windowTop; }
    int32_t GetWindowTop() const { return windowTop_; }

    void SetWindowWidth(int32_t windowWidth) { windowWidth_ = windowWidth; }
    int32_t GetWindowWidth() const { return windowWidth_; }

    void SetWindowHeight(int32_t windowHeight) { windowHeight_ = windowHeight; }
    int32_t GetWindowHeight() const { return windowHeight_; }

    virtual bool Marshalling(Parcel &parcel) const override { return true; }
    static StartOptions *Unmarshalling(Parcel &parcel) { return new StartOptions(); }

private:
    int32_t windowMode_ = AAFwk::MULTI_WINDOW_DISPLAY_UNDEFINED;
    int32_t displayId_ = -1;
    int32_t windowLeft_ = 0;
    int32_t windowTop_ = 0;
    int32_t windowWidth_ = 0;
    int32_t windowHeight_ = 0;
};

}
}

#endif