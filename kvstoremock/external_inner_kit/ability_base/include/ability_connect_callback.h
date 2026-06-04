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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_CONNECT_CALLBACK_H
#define OHOS_ABILITY_RUNTIME_ABILITY_CONNECT_CALLBACK_H

#include "ability_connect_callback_interface.h"

namespace OHOS {
namespace AAFwk {
class AbilityConnectCallback : public IAbilityConnection {
public:
    virtual void OnAbilityConnectDone(
        const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override {}
    virtual void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) override {}
};
}  // namespace AAFwk
}  // namespace OHOS

namespace OHOS {
namespace AbilityRuntime {
using AAFwk::AbilityConnectCallback;
}
}

#endif  // OHOS_ABILITY_RUNTIME_ABILITY_CONNECT_CALLBACK_H