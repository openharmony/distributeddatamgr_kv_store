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
#include <iservice_registry.h>
#include <mock_starter.h>
#include <system_ability.h>

#include "mock_system_ability_manager.h"
#include "system_ability_load_callback_stub.h"
namespace OHOS {
SystemAbilityManagerClient &SystemAbilityManagerClient::GetInstance()
{
    static SystemAbilityManagerClient instance;
    return instance;
}

sptr<ISystemAbilityManager> SystemAbilityManagerClient::GetSystemAbilityManager()
{
    if (systemAbilityManager_ == nullptr) {
        systemAbilityManager_ = new MockSystemAbilityManager();
    }
    return systemAbilityManager_;
}

void SystemAbilityManagerClient::DestroySystemAbilityManagerObject() {};
SystemAbility::SystemAbility(bool) {};
SystemAbility::SystemAbility(int saId, bool isRunOnCreate) :saId_(saId), isRunOnCreate_(isRunOnCreate) {}
SystemAbility::~SystemAbility(){}
void SystemAbility::OnDump() {}
void SystemAbility::OnStart() {}
void SystemAbility::OnStop() {}
void SystemAbility::OnStart(const SystemAbilityOnDemandReason& reason) {}
int32_t SystemAbility::OnIdle(const SystemAbilityOnDemandReason& idleReason) { return 0; }
void SystemAbility::OnActive(const SystemAbilityOnDemandReason& activeReason) {}
void SystemAbility::OnStop(const SystemAbilityOnDemandReason& stopReason) {}
void SystemAbility::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId) {}
bool SystemAbility::Publish(sptr<IRemoteObject> remote)
{
    publishObj_ = remote;
    SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager()->AddSystemAbility(saId_, publishObj_);
    return true;
}

bool SystemAbility::MakeAndRegisterAbility(SystemAbility* ability)
{
    MockStarter::Instance()->RegisterRunnable([ability]() { ability->OnStart(); });
    return true;
}
void SystemAbility::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{

}
bool SystemAbility::AddSystemAbilityListener(int32_t systemAbilityId)
{
    return true;
}
bool SystemAbility::RemoveSystemAbilityListener(int32_t systemAbilityId)
{
    return false;
}
int32_t SystemAbility::OnSvcCmd(int32_t fd, const std::vector<std::u16string> &args)
{
    return 0;
}
sptr<IRemoteObject> SystemAbility::GetSystemAbility(int32_t systemAbilityId)
{
    return sptr<IRemoteObject>();
}
bool SystemAbility::CancelIdle()
{
    return false;
}
void SystemAbility::StopAbility(int32_t systemAbilityId) {}
SystemAbilityState SystemAbility::GetAbilityState()
{
    return SystemAbilityState::IDLE;
}
void SystemAbility::OnDeviceLevelChanged(int32_t type, int32_t level, std::string &action) {}
int32_t SystemAbility::OnExtension(const std::string &extension, MessageParcel &data, MessageParcel &reply)
{
    return 0;
}

int32_t SystemAbilityLoadCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    return 0;
}
}
