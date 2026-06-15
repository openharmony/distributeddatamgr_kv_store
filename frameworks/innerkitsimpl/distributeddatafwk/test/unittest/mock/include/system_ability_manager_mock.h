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

#ifndef KV_STORE_SYSTEM_ABILITY_MANAGER_MOCK_H
#define KV_STORE_SYSTEM_ABILITY_MANAGER_MOCK_H

#include <gmock/gmock.h>

#include "if_system_ability_manager.h"
#include "iremote_object.h"

namespace OHOS {
class ISystemAbilityBase {
public:
    virtual ~ISystemAbilityBase() = default;
    virtual sptr<ISystemAbilityManager> GetSystemAbilityManager() = 0;
public:
    static inline std::shared_ptr<ISystemAbilityBase> sab = nullptr;
};

class SystemAbilityMock : public ISystemAbilityBase {
public:
    MOCK_METHOD((sptr<ISystemAbilityManager>), GetSystemAbilityManager, ());
};

class SystemAbilityManagerMock : public ISystemAbilityManager {
public:
    MOCK_METHOD((sptr<IRemoteObject>), AsObject, ());
    MOCK_METHOD((std::vector<std::u16string>), ListSystemAbilities, (unsigned int));
    MOCK_METHOD((sptr<IRemoteObject>), GetSystemAbility, (int32_t));
    MOCK_METHOD((sptr<IRemoteObject>), CheckSystemAbility, (int32_t));
    MOCK_METHOD(int32_t, RemoveSystemAbility, (int32_t));
    MOCK_METHOD(int32_t, SubscribeSystemAbility, (int32_t, (const sptr<ISystemAbilityStatusChange>&)));
    MOCK_METHOD(int32_t, UnSubscribeSystemAbility, (int32_t, (const sptr<ISystemAbilityStatusChange>&)));
    MOCK_METHOD((sptr<IRemoteObject>), GetSystemAbility, (int32_t, const std::string&));
    MOCK_METHOD((sptr<IRemoteObject>), CheckSystemAbility, (int32_t, const std::string&));
    MOCK_METHOD(int32_t, AddOnDemandSystemAbilityInfo, (int32_t, const std::u16string&));
    MOCK_METHOD((sptr<IRemoteObject>), CheckSystemAbility, (int32_t, bool&));
    MOCK_METHOD(int32_t, AddSystemAbility, (int32_t, (const sptr<IRemoteObject>&), const SAExtraProp&));
    MOCK_METHOD(int32_t, AddSystemProcess, (const std::u16string&, (const sptr<IRemoteObject>&)));
    MOCK_METHOD((sptr<IRemoteObject>), LoadSystemAbility, (int32_t, int32_t));
    MOCK_METHOD(int32_t, LoadSystemAbility, (int32_t, (const sptr<ISystemAbilityLoadCallback>&)));
    MOCK_METHOD(int32_t, LoadSystemAbility, (int32_t, const std::string&, (const sptr<ISystemAbilityLoadCallback>&)));
    MOCK_METHOD(int32_t, UnloadSystemAbility, (int32_t));
    MOCK_METHOD(int32_t, CancelUnloadSystemAbility, (int32_t));
    MOCK_METHOD(int32_t, UnloadAllIdleSystemAbility, ());
    MOCK_METHOD(int32_t, GetSystemProcessInfo, (int32_t, SystemProcessInfo&));
    MOCK_METHOD(int32_t, GetRunningSystemProcess, (std::list<SystemProcessInfo>&));
    MOCK_METHOD(int32_t, SubscribeSystemProcess, (const sptr<ISystemProcessStatusChange>&));
    MOCK_METHOD(int32_t, SendStrategy, (int32_t, (std::vector<int32_t>&), int32_t, std::string&));
    MOCK_METHOD(int32_t, UnSubscribeSystemProcess, (const sptr<ISystemProcessStatusChange>&));
    MOCK_METHOD(int32_t, GetOnDemandReasonExtraData, (int64_t, MessageParcel&));
    MOCK_METHOD(int32_t, GetOnDemandPolicy, (int32_t, OnDemandPolicyType, (std::vector<SystemAbilityOnDemandEvent>&)));
    MOCK_METHOD(int32_t, UpdateOnDemandPolicy, (int32_t, OnDemandPolicyType,
        (const std::vector<SystemAbilityOnDemandEvent>&)));
    MOCK_METHOD(int32_t, GetOnDemandSystemAbilityIds, (std::vector<int32_t>&));
    MOCK_METHOD(int32_t, GetExtensionSaIds, (const std::string&, std::vector<int32_t>&));
    MOCK_METHOD(int32_t, GetExtensionRunningSaList, (const std::string&, (std::vector<sptr<IRemoteObject>>&)));
    MOCK_METHOD(int32_t, GetRunningSaExtensionInfoList, (const std::string&, (std::vector<SaExtensionInfo>&)));
    MOCK_METHOD(int32_t, GetCommonEventExtraDataIdlist, (int32_t, (std::vector<int64_t>&), const std::string&));
    MOCK_METHOD((sptr<IRemoteObject>), GetLocalAbilityManagerProxy, (int32_t));
};
} // namespace OHOS

#endif // KV_STORE_SYSTEM_ABILITY_MANAGER_MOCK_H
