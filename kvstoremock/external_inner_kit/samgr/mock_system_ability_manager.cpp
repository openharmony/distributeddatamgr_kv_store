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
#include "mock_system_ability_manager.h"
namespace OHOS {
MockSystemAbilityManager::MockSystemAbilityManager()
{
}
MockSystemAbilityManager::~MockSystemAbilityManager()
{
}
std::vector<std::u16string> MockSystemAbilityManager::ListSystemAbilities(unsigned int dumpFlags)
{
    return std::vector<std::u16string>();
}
sptr<IRemoteObject> MockSystemAbilityManager::GetSystemAbility(int32_t systemAbilityId)
{
    if (abilities_.find(systemAbilityId) == abilities_.end()) {
        return nullptr;
    }
    return abilities_[systemAbilityId];
}
sptr<IRemoteObject> MockSystemAbilityManager::CheckSystemAbility(int32_t systemAbilityId)
{
    if (abilities_.find(systemAbilityId) == abilities_.end()) {
        return nullptr;
    }
    return abilities_[systemAbilityId];
}
int32_t MockSystemAbilityManager::RemoveSystemAbility(int32_t systemAbilityId)
{
    return 0;
}
sptr<IRemoteObject> MockSystemAbilityManager::GetSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    return sptr<IRemoteObject>();
}
sptr<IRemoteObject> MockSystemAbilityManager::CheckSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    return sptr<IRemoteObject>();
}
int32_t MockSystemAbilityManager::AddOnDemandSystemAbilityInfo(
    int32_t systemAbilityId, const std::u16string &localAbilityManagerName)
{
    return 0;
}
sptr<IRemoteObject> MockSystemAbilityManager::CheckSystemAbility(int32_t systemAbilityId, bool &isExist)
{
    if (abilities_.find(systemAbilityId) == abilities_.end()) {
        isExist = false;
        return nullptr;
    }
    return abilities_[systemAbilityId];
}
int32_t MockSystemAbilityManager::AddSystemAbility(
    int32_t systemAbilityId, const sptr<IRemoteObject> &ability, const ISystemAbilityManager::SAExtraProp &extraProp)
{
    if (abilities_.find(systemAbilityId) != abilities_.end()) {
        return -1;
    }
    abilities_.insert(std::pair(systemAbilityId, ability));
    return 0;
}
sptr<IRemoteObject> MockSystemAbilityManager::AsObject()
{
    return sptr<IRemoteObject>();
}
int32_t MockSystemAbilityManager::SubscribeSystemAbility(int32_t systemAbilityId,
                                                         const sptr<ISystemAbilityStatusChange> &listener)
{
    return 0;
}
int32_t MockSystemAbilityManager::UnSubscribeSystemAbility(int32_t systemAbilityId,
                                                           const sptr<ISystemAbilityStatusChange> &listener)
{
    return 0;
}
int32_t MockSystemAbilityManager::AddSystemProcess(const std::u16string &procName,
                                                   const sptr<IRemoteObject> &procObject)
{
    return 0;
}
int32_t MockSystemAbilityManager::LoadSystemAbility(int32_t systemAbilityId,
                                                    const sptr<ISystemAbilityLoadCallback> &callback)
{
    return 0;
}
}
