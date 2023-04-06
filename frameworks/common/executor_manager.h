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

#ifndef OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_MANAGER_H
#include "executor_pool.h"
namespace OHOS {
class ExecutorManager {
public:
    using TaskId = ExecutorPool::TaskId;
    using Task = ExecutorPool::Task;
    using Time = ExecutorPool::Time;
    using Duration = ExecutorPool::Duration;
    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
    static ExecutorManager &GetInstance()
    {
        static ExecutorManager instance;
        return instance;
    }

    TaskId Execute(Task task)
    {
        return executorPool_->Execute(task);
    }

    TaskId Execute(Task task, Duration delay)
    {
        return executorPool_->Execute(task, delay);
    }

    TaskId Schedule(Task task, Duration interval)
    {
        return executorPool_->Schedule(task, interval);
    }
    TaskId Schedule(Task task, uint32_t interval)
    {
        return executorPool_->Schedule(task, std::chrono::milliseconds(interval));
    }

    TaskId Schedule(Task task, Duration delay, Duration interval)
    {
        return executorPool_->Schedule(task, delay, interval);
    }

    TaskId Schedule(Task task, Duration delay, Duration interval, uint64_t times)
    {
        return executorPool_->Schedule(task, delay, interval, times);
    }

    bool Remove(TaskId taskId, bool wait = false)
    {
        return executorPool_->Remove(taskId, wait);
    }

    TaskId Reset(TaskId taskId, Duration interval)
    {
        return executorPool_->Reset(taskId, interval);
    }
    
    TaskId Reset(TaskId taskId, Duration delay, Duration interval)
    {
        return executorPool_->Reset(taskId, delay, interval);
    }

private:
    static constexpr size_t maxThreads = 2;
    static constexpr size_t minThreads = 0;

    ExecutorManager()
    {
        executorPool_ = std::make_shared<ExecutorPool>(maxThreads, minThreads);
    };

    ~ExecutorManager() = default;
    std::shared_ptr<ExecutorPool> executorPool_;
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_MANAGER_H
