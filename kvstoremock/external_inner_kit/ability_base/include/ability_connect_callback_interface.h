/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
/**
 * @class IAbilityConnection
 * IAbilityConnection is used to notify caller ability that connect or disconnect is complete.
 */
class IAbilityConnection : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.abilityshell.DistributedConnection");

    /**
     * OnAbilityConnectDone, AbilityMs notify caller ability the result of connect.
     *
     * @param element, service ability's ElementName.
     * @param remoteObject,.the session proxy of service ability.
     * @param resultCode, ERR_OK on success, others on failure.
     */
    virtual void OnAbilityConnectDone(
        const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) = 0;

    /**
     * OnAbilityDisconnectDone, AbilityMs notify caller ability the result of disconnect.
     *
     * @param element, service ability's ElementName.
     * @param resultCode, ERR_OK on success, others on failure.
     */
    virtual void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) = 0;

    enum {
        // ipc id for OnAbilityConnectDone
        ON_ABILITY_CONNECT_DONE = 1,

        // ipc id for OnAbilityDisConnectDone
        ON_ABILITY_DISCONNECT_DONE,

        ON_REMOTE_STATE_CHANGED,

        ON_CONNECT_SYSTEM_COMMON_DIALOG
    };
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_ABILITY_CONNECT_CALLBACK_INTERFACE_H
