/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "kvdb_service_client.h"
#include "kvstore_service_death_notifier.h"
#include "switch_observer_bridge.h"

namespace OHOS::DistributedKv {
static constexpr int32_t INTERVAL = 500; // ms
SwitchObserverBridge::SwitchObserverBridge(const AppId &appId)
{
    switchAppId_ = appId;
}

void SwitchObserverBridge::AddSwitchCallback(std::shared_ptr<KvStoreObserver> observer)
{
    if (observer == nullptr) {
        return;
    }
    switchObservers_.InsertOrAssign(uintptr_t(observer.get()), observer);
}

void SwitchObserverBridge::DeleteSwitchCallback(std::shared_ptr<KvStoreObserver> observer)
{
    if (observer == nullptr) {
        return;
    }
    switchObservers_.Erase(uintptr_t(observer.get()));
}

void SwitchObserverBridge::OnRemoteDied()
{
    std::lock_guard<decltype(switchMutex_)> lock(switchMutex_);
    if (taskId_ > 0) {
        return;
    }
    taskId_ = TaskExecutor::GetInstance().Schedule(std::chrono::milliseconds(INTERVAL), [this]() {
        RegisterSwitchObserver();
    });
}

void SwitchObserverBridge::RegisterSwitchObserver()
{
    if (!switchAppId_.IsValid() || switchObservers_.Empty()) {
        return;
    }
    std::lock_guard<decltype(switchMutex_)> lock(switchMutex_);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return;
    }
    auto serviceAgent = service->GetServiceAgent(switchAppId_);
    if (serviceAgent == nullptr) {
        return;
    }
    auto status = service->SubscribeSwitchData(switchAppId_);
    if (status != SUCCESS) {
        taskId_ = TaskExecutor::GetInstance().Schedule(std::chrono::milliseconds(INTERVAL), [this]() {
            RegisterSwitchObserver();
        });
        return;
    }
    taskId_ = 0;
    switchObservers_.ForEach([&](auto &, auto &switchObserver) {
        if (switchObserver != nullptr) {
            serviceAgent->AddSwitchCallback(switchAppId_, switchObserver);
            return true;
        }
        return false;
    });
}
} // namespace OHOS::DistributedKv