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
#ifndef SDB_AUTO_SYNC_TIMER_H
#define SDB_AUTO_SYNC_TIMER_H
#include <set>
#include <vector>

#include "concurrent_map.h"
#include "kvdb_service.h"
#include "task_executor.h"
namespace OHOS::DistributedKv {
class DataChangeNotifier {
public:
    static DataChangeNotifier &GetInstance();
    void DoNotifyChange(const std::string &appId, std::set<StoreId> storeIds, bool now = false);

private:
    static constexpr uint32_t NOTIFY_DELAY = 1; // s
    DataChangeNotifier() = default;
    ~DataChangeNotifier() = default;
    std::map<std::string, std::vector<StoreId>> GetStoreIds();
    std::function<void()> GenTask() __attribute__((no_sanitize("cfi")));
    void StartTimer();
    void AddStores(const std::string &appId, std::set<StoreId> storeIds);
    void DoNotify(const std::string& appId, const std::vector<StoreId> &stores);
    bool HasStores();
    ConcurrentMap<std::string, std::vector<StoreId>> stores_;
    TaskExecutor::TaskId taskId_;
    std::mutex mutex_;
};
} // namespace OHOS::DistributedKv
#endif // SDB_AUTO_SYNC_TIMER_H
