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

#ifndef OHOS_DISTRIBUTED_DATA_MOCK_SRC_MOCK_SYSTEM_ABILITY_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_MOCK_SRC_MOCK_SYSTEM_ABILITY_MANAGER_H
#include "if_system_ability_manager.h"
#include <map>
namespace OHOS {
class MockSystemAbilityManager : public ISystemAbilityManager {
public:
    MockSystemAbilityManager();
    virtual ~MockSystemAbilityManager();
    sptr<IRemoteObject> AsObject() override;
    std::vector<std::u16string> ListSystemAbilities(unsigned int dumpFlags) override;
    sptr<IRemoteObject> GetSystemAbility(int32_t systemAbilityId) override;
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId) override;
    int32_t RemoveSystemAbility(int32_t systemAbilityId) override;
    sptr<IRemoteObject> GetSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    int32_t AddOnDemandSystemAbilityInfo(
        int32_t systemAbilityId, const std::u16string &localAbilityManagerName) override;
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId, bool &isExist) override;
    int32_t SubscribeSystemAbility(int32_t systemAbilityId, const sptr<ISystemAbilityStatusChange> &listener) override;
    int32_t UnSubscribeSystemAbility(int32_t systemAbilityId,
                                     const sptr<ISystemAbilityStatusChange> &listener) override;
    int32_t AddSystemProcess(const std::u16string &procName, const sptr<IRemoteObject> &procObject) override;
    int32_t LoadSystemAbility(int32_t systemAbilityId, const sptr<ISystemAbilityLoadCallback> &callback) override;
    int32_t AddSystemAbility(
        int32_t systemAbilityId, const sptr<IRemoteObject> &ability, const SAExtraProp &extraProp) override;

    sptr<IRemoteObject> LoadSystemAbility(int32_t systemAbilityId, int32_t timeout) override
    {
        return nullptr;
    }
    int32_t LoadSystemAbility(int32_t systemAbilityId, const std::string& deviceId,
        const sptr<ISystemAbilityLoadCallback>& callback) override
    {
        return 0;
    }
    int32_t UnloadSystemAbility(int32_t systemAbilityId) override
    {
        return 0;
    }
    int32_t CancelUnloadSystemAbility(int32_t systemAbilityId) override
    {
        return 0;
    }
    int32_t UnloadAllIdleSystemAbility() override
    {
        return 0;
    }
    int32_t GetSystemProcessInfo(int32_t systemAbilityId, SystemProcessInfo& systemProcessInfo) override
    {
        return 0;
    }
    int32_t GetRunningSystemProcess(std::list<SystemProcessInfo>& systemProcessInfos) override
    {
        return 0;
    }
    int32_t SubscribeSystemProcess(const sptr<ISystemProcessStatusChange>& listener) override
    {
        return 0;
    }
    int32_t SendStrategy(int32_t type, std::vector<int32_t>& systemAbilityIds,
        int32_t level, std::string& action) override
    {
        return 0;
    }
    int32_t UnSubscribeSystemProcess(const sptr<ISystemProcessStatusChange>& listener) override
    {
        return 0;
    }
    int32_t GetExtensionSaIds(const std::string& extension, std::vector<int32_t> &saIds) override
    {
        return 0;
    }
    int32_t GetExtensionRunningSaList(const std::string& extension,
        std::vector<sptr<IRemoteObject>>& saList) override
    {
        return 0;
    }
    int32_t GetRunningSaExtensionInfoList(const std::string& extension,
        std::vector<SaExtensionInfo>& infoList) override
    {
        return 0;
    }
    int32_t GetCommonEventExtraDataIdlist(int32_t saId, std::vector<int64_t>& extraDataIdList,
        const std::string& eventName = "") override
    {
        return 0;
    }
    int32_t GetOnDemandReasonExtraData(int64_t extraDataId, MessageParcel& extraDataParcel) override
    {
        return 0;
    }
    int32_t GetOnDemandPolicy(int32_t systemAbilityId, OnDemandPolicyType type,
        std::vector<SystemAbilityOnDemandEvent>& abilityOnDemandEvents) override
    {
        return 0;
    }
    int32_t UpdateOnDemandPolicy(int32_t systemAbilityId, OnDemandPolicyType type,
        const std::vector<SystemAbilityOnDemandEvent>& abilityOnDemandEvents) override
    {
        return 0;
    }
    int32_t GetOnDemandSystemAbilityIds(std::vector<int32_t>& systemAbilityIds) override
    {
        return 0;
    }

private:
    std::map<int32_t, sptr<IRemoteObject>> abilities_;
};
}

#endif // OHOS_DISTRIBUTED_DATA_MOCK_SRC_MOCK_SYSTEM_ABILITY_MANAGER_H
