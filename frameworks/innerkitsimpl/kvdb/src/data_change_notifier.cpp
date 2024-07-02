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
        taskId_ = TaskExecutor::GetInstance().Schedule(std::chrono::seconds(RECOVER_INTERVAL), GarbageCollect());
        return;
    }
    taskId_ = TaskExecutor::GetInstance().Reset(taskId_, std::chrono::seconds(RECOVER_INTERVAL));
}

void DataChangeNotifier::DoNotifyChange(const std::string &appId, std::set<StoreId> storeIds)
{
    TaskExecutor::GetInstance().Execute([&appId, stores = std::move(storeIds), this]() {
        DoNotify(appId, { stores.begin(), stores.end() });
        StartTimer();
    });
}

std::function<void()> DataChangeNotifier::GarbageCollect()
{
    return [this]() {
        stores_.EraseIf([](const auto &key, std::map<StoreId, uint64_t> &value) {
            for (auto it = value.begin(); it != value.end();) {
                if (it->second < GetTimeStamp()) {
                    it = value.erase(it);
                } else {
                    ++it;
                }
            }
            return value.empty();
        });
        stores_.DoActionIfEmpty([this]() {
            std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
            TaskExecutor::GetInstance().Remove(taskId_);
            taskId_ = TaskExecutor::INVALID_TASK_ID;
        });
    };
}

void DataChangeNotifier::DoNotify(const std::string& appId, const std::vector<StoreId> &stores)
{
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return;
    }
    stores_.Compute(appId, [service, &stores](const auto &key, std::map<StoreId, uint64_t> &value) {
        for (const auto &store : stores) {
            auto it = value.find(store);
            if (it != value.end() && it->second > GetTimeStamp()) {
                continue;
            }
            auto status = service->NotifyDataChange({ key }, store, NOTIFY_INTERVAL);
            if (status != SUCCESS) {
                continue;
            }
            value.insert_or_assign(store, GetTimeStamp(NOTIFY_INTERVAL));
        }
        return true;
    });

    ZLOGD("Notify change appId:%{public}s store size:%{public}zu", appId.c_str(), stores.size());
}
} // namespace OHOS::DistributedKv
