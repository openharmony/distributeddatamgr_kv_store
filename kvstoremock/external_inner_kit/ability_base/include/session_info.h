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

#ifndef OHOS_ABILITY_SESSION_INFO_H
#define OHOS_ABILITY_SESSION_INFO_H

#include <string>
#include "iremote_object.h"
#include "parcel.h"
#include "want.h"

namespace OHOS {
namespace AAFwk {

enum class CallToState : int32_t {
    UNKNOW = 0,
    CALL_TO_STATE_CONNECTED = 1,
    CALL_TO_STATE_DISCONNECTED = 2
};

class SessionInfo : public Parcelable {
public:
    SessionInfo() = default;
    virtual ~SessionInfo() = default;

    bool Marshalling(Parcel& parcel) const override { return true; }
    static SessionInfo* Unmarshalling(Parcel& parcel) { return new SessionInfo(); }

    sptr<IRemoteObject> sessionToken = nullptr;
    sptr<IRemoteObject> callerToken = nullptr;
    sptr<IRemoteObject> parentToken = nullptr;
    std::string identityToken;
    int32_t persistentId = 0;
    uint32_t hostWindowId = 0;
    bool hideStartWindow = false;
    std::string hostBundleName = "";
    CallToState state = CallToState::UNKNOW;
    int32_t resultCode = -1;
    int32_t requestCode = -1;
    int64_t uiAbilityId = 0;
    Want want;
};

}
}

#endif