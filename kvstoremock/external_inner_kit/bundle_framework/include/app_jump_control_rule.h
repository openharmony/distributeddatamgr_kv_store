/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_JUMP_CONTROL_RULE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_JUMP_CONTROL_RULE_H

#include <string>

#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {
constexpr const char* PARAMETER_APP_JUMP_INTERCEPTOR_ENABLE = "is_app_jump_interceptor_enable";
enum class AbilityJumpMode {
    DIRECT = 0,
    INTERCEPT,
    FORBID,
};
struct AppJumpControlRule : public Parcelable {
    enum AbilityJumpMode jumpMode = AbilityJumpMode::DIRECT;
    std::string callerPkg;
    std::string targetPkg;
    std::string controlMessage;

    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static AppJumpControlRule *Unmarshalling(Parcel &parcel);
};
} // AppExecFwk
} // OHOS
#endif // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_APP_JUMP_CONTROL_RULE_H
