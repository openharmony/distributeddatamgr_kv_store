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

#ifndef OHOS_ABILITY_RUNTIME_OPEN_LINK_OPTIONS_H
#define OHOS_ABILITY_RUNTIME_OPEN_LINK_OPTIONS_H

#include <string>
#include <functional>
#include "parcel.h"

namespace OHOS {
namespace AbilityRuntime {
using OnRequestResult = std::function<void(const AppExecFwk::ElementName&, const std::string&)>;
}

namespace AAFwk {

class OpenLinkOptions : public Parcelable {
public:
    OpenLinkOptions() = default;
    ~OpenLinkOptions() = default;
    OpenLinkOptions(const OpenLinkOptions &other) = default;
    OpenLinkOptions &operator=(const OpenLinkOptions &other) = default;

    void SetAppLinkingOnly(bool appLinkingOnly) { appLinkingOnly_ = appLinkingOnly; }
    bool GetAppLinkingOnly() const { return appLinkingOnly_; }

    void SetHideFailureTipDialog(bool hideFailureTipDialog) { hideFailureTipDialog_ = hideFailureTipDialog; }
    bool GetHideFailureTipDialog() const { return hideFailureTipDialog_; }

    void SetParameters(void* parameters) {}
    void* GetParameters() const { return nullptr; }

    virtual bool Marshalling(Parcel &parcel) const override { return true; }
    static OpenLinkOptions *Unmarshalling(Parcel &parcel) { return new OpenLinkOptions(); }

private:
    bool appLinkingOnly_ = false;
    bool hideFailureTipDialog_ = false;
};

}
}

#endif