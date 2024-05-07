/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "DataChangeNotifier"
#include "data_change_notifier.h"

#include "kvdb_service_client.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
DataChangeNotifier &DataChangeNotifier::GetInstance()
{
    static DataChangeNotifier instance;
    return instance;
}

void DataChangeNotifier::StartTimer()
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (taskId_ == TaskExecutor::INVALID_TASK_ID) {
        taskId_ = TaskExecutor::GetInstance().Schedule(std::chrono::seconds(NOTIFY_DELAY), GenTask());
        return;
    }
    taskId_ = TaskExecutor::GetInstance().Reset(taskId_, std::chrono::seconds(NOTIFY_DELAY));
}

void DataChangeNotifier::DoNotifyChange(const std::string &appId, std::set<StoreId> storeIds, bool now)
{
    if (now) {
        DoNotify(appId, { storeIds.begin(), storeIds.end() });
        return;
    }
    AddStores(appId, std::move(storeIds));
    StartTimer();
}

void DataChangeNotifier::AddStores(const std::string &appId, std::set<StoreId> storeIds)
{
    stores_.Compute(appId, [&storeIds](const auto &key, std::vector<StoreId> &value) {
        std::set<StoreId> tempStores(value.begin(), value.end());
        for (auto it = storeIds.begin(); it != storeIds.end(); it++) {
            if (tempStores.count(*it) == 0) {
                value.push_back(*it);
            }
        }
        return !value.empty();
    });
}

bool DataChangeNotifier::HasStores()
{
    return !stores_.Empty();
}

std::map<std::string, std::vector<StoreId>> DataChangeNotifier::GetStoreIds()
{
    std::map<std::string, std::vector<StoreId>> stores;
    stores_.EraseIf([&stores](const std::string &key, std::vector<StoreId> &value) {
        stores.insert({ key, std::move(value) });
        return true;
    });
    return stores;
}

std::function<void()> DataChangeNotifier::GenTask()
{
    return [this]() {
        {
            std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
            taskId_ = TaskExecutor::INVALID_TASK_ID;
        }
        auto storeIds = GetStoreIds();
        for (const auto &id : storeIds) {
            DoNotify({ id.first }, id.second);
        }
        if (HasStores()) {
            StartTimer();
        }
    };
}

void DataChangeNotifier::DoNotify(const std::string& appId, const std::vector<StoreId> &stores)
{
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return;
    }
    for (const auto &store : stores) {
        service->NotifyDataChange({ appId }, store);
    }
    ZLOGD("Notify change appId:%{public}s store size:%{public}zu", appId.c_str(), stores.size());
}
} // namespace OHOS::DistributedKv
