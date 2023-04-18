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
#include "task_executor.h"

namespace OHOS::DistributedKv {
TaskExecutor &TaskExecutor::GetInstance()
{
    static TaskExecutor instance;
    return instance;
}

TaskExecutor::TaskId TaskExecutor::Execute(TaskExecutor::Task &&task, int32_t delay)
{
    if (pool_ == nullptr) {
        return INVALID_TASK_ID;
    }
    return pool_->Execute(std::move(task), std::chrono::milliseconds(delay));
}

TaskExecutor::TaskId TaskExecutor::Schedule(TaskExecutor::Task &&task, int32_t interval, int32_t delay)
{
    return Schedule(std::move(task), interval, delay, UNLIMITED_TIMES);
}

TaskExecutor::TaskId TaskExecutor::Schedule(TaskExecutor::Task &&task, int32_t interval, int32_t delay, int32_t times)
{
    if (pool_ == nullptr) {
        return INVALID_TASK_ID;
    }
    return pool_->Schedule(
        std::move(task), std::chrono::milliseconds(delay), std::chrono::milliseconds(interval), times);
}

bool TaskExecutor::Remove(TaskId taskId, bool wait)
{
    return pool_->Remove(taskId, wait);
}

TaskExecutor::TaskId TaskExecutor::Reset(TaskId taskId, int32_t interval, int32_t delay)
{
    return pool_->Reset(taskId, std::chrono::milliseconds(delay), std::chrono::milliseconds(interval));
}

TaskExecutor::TaskExecutor()
{
    pool_ = std::make_shared<ExecutorPool>(MAX_THREADS, MIN_THREADS);
}

TaskExecutor::~TaskExecutor()
{
    pool_ = nullptr;
}
} // namespace OHOS::DistributedKv
