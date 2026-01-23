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
#ifndef DISTRIBUTED_KVSTORE_MOCK_IF_SYSTEM_ABILITY_MANAGER_H
#define DISTRIBUTED_KVSTORE_MOCK_IF_SYSTEM_ABILITY_MANAGER_H
#include "iremote_broker.h"

namespace OHOS {

class ISystemAbilityManager : public IRemoteBroker {
public:
    ISystemAbilityManager() = default;
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId)
    {
        return nullptr;
    }
};
}  // namespace OHOS
#endif  // DISTRIBUTED_KVSTORE_MOCK_IF_SYSTEM_ABILITY_MANAGER_H
