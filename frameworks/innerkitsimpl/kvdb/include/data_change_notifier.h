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
    void DoNotifyChange(const std::string &appId, std::set<StoreId> storeIds);

private:
    static inline uint64_t GetTimeStamp(uint32_t offset = 0)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            (std::chrono::steady_clock::now() + std::chrono::milliseconds(offset)).time_since_epoch())
            .count();
    }
    static constexpr uint32_t NOTIFY_INTERVAL = 200; // ms
    static constexpr uint32_t RECOVER_INTERVAL = 60;  // s
    DataChangeNotifier() = default;
    ~DataChangeNotifier() = default;
    std::function<void()> GarbageCollect() __attribute__((no_sanitize("cfi")));
    void StartTimer();
    void DoNotify(const std::string& appId, const std::vector<StoreId> &stores);
    ConcurrentMap<std::string, std::map<StoreId, uint64_t>> stores_;
    TaskExecutor::TaskId taskId_;
    std::mutex mutex_;
};
} // namespace OHOS::DistributedKv
#endif // SDB_AUTO_SYNC_TIMER_H
