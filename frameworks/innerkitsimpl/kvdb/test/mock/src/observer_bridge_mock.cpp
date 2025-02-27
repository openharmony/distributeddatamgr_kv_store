/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "include/observer_bridge_mock.h"

namespace OHOS::DistributedKv {
ObserverBridge::ObserverBridge(AppId appId, StoreId store, std::shared_ptr<Observer> observer, const Convertor &cvt)
    : appId_(std::move(appId)), storeId_(std::move(store)), observer_(std::move(observer)), convert_(cvt)
{
}

ObserverBridge::~ObserverBridge() { }

void ObserverBridge::OnServiceDeath()
{
    if (BObserverBridge::observerBridge != nullptr) {
        return BObserverBridge::observerBridge->OnServiceDeath();
    }
}

Status ObserverBridge::RegisterRemoteObserver(uint32_t realType)
{
    if (BObserverBridge::observerBridge != nullptr) {
        return BObserverBridge::observerBridge->RegisterRemoteObserver(realType);
    }
    return ERROR;
}

Status ObserverBridge::UnregisterRemoteObserver(uint32_t realType)
{
    return SUCCESS;
}

void ObserverBridge::OnChange(const DBChangedData &data) { }

ObserverBridge::ObserverClient::ObserverClient(std::shared_ptr<Observer> observer, const Convertor &cvt)
    : KvStoreObserverClient(observer), convert_(cvt), realType_(0)
{
}

void ObserverBridge::ObserverClient::OnChange(const ChangeNotification &data) { }

void ObserverBridge::ObserverClient::OnChange(const DataOrigin &origin, Keys &&keys) { }

template <class T>
std::vector<Entry> ObserverBridge::ConvertDB(const T &dbEntries, std::string &deviceId, const Convertor &convert)
{
    std::vector<Entry> entries(dbEntries.size());
    return entries;
}

} // namespace OHOS::DistributedKv