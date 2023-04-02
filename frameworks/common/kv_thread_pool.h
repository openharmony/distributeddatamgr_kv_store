/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*
*     http://www.apache.org/licenses/LICENSE-2.0
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_KV_THREAD_POOL_H
#define OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_KV_THREAD_POOL_H
#include "executor_pool.h"
namespace OHOS {
class KvThreadPool {
public:
    using TaskId = ExecutorPool::TaskId;
    using Task = ExecutorPool::Task;
    using Time = ExecutorPool::Time;
    using Duration = ExecutorPool::Duration;
    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
    static KvThreadPool &GetInstance()
    {
        static KvThreadPool instance;
        return instance;
    }

    TaskId Execute(Task task)
    {
        return threadPool_->Execute(task);
    }

    TaskId Execute(Task task, Duration delay)
    {
        return threadPool_->Execute(task, delay);
    }

    TaskId Schedule(Task task, Duration interval)
    {
        return threadPool_->Schedule(task, interval);
    }
    TaskId Schedule(Task task, uint32_t interval)
    {
        return threadPool_->Schedule(task, std::chrono::milliseconds(interval));
    }

    TaskId Schedule(Task task, Duration delay, Duration interval)
    {
        return threadPool_->Schedule(task, delay, interval);
    }

    TaskId Schedule(Task task, Duration delay, Duration interval, uint64_t times)
    {
        return threadPool_->Schedule(task, delay, interval, times);
    }

    bool Remove(TaskId taskId, bool wait = false)
    {
        return threadPool_->Remove(taskId, wait);
    }

    TaskId Reset(TaskId taskId, Duration interval)
    {
        return threadPool_->Reset(taskId, interval);
    }

private:
    static constexpr size_t maxThreads = 2;
    static constexpr size_t minThreads = 0;

    KvThreadPool()
    {
        threadPool_ = std::make_shared<ExecutorPool>(maxThreads, minThreads);
    };

    ~KvThreadPool() = default;
    std::shared_ptr<ExecutorPool> threadPool_;
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_KV_THREAD_POOL_H
