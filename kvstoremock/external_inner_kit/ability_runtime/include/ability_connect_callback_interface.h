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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_CONNECT_CALLBACK_INTERFACE_H
#define OHOS_ABILITY_RUNTIME_ABILITY_CONNECT_CALLBACK_INTERFACE_H

#include "want.h"
#include "element_name.h"

#include "iremote_broker.h"

namespace OHOS {
namespace AAFwk {
class IAbilityConnection : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.abilityshell.DistributedConnection");

    virtual void OnAbilityConnectDone(
        const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) = 0;

    virtual void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) = 0;

    enum {
        ON_ABILITY_CONNECT_DONE = 1,
        ON_ABILITY_DISCONNECT_DONE,
        ON_REMOTE_STATE_CHANGED,
        ON_CONNECT_SYSTEM_COMMON_DIALOG
    };
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_ABILITY_CONNECT_CALLBACK_INTERFACE_H