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
#ifndef DISTRIBUTED_DATA_TASK_EXECUTOR_H
#define DISTRIBUTED_DATA_TASK_EXECUTOR_H
#include "executor_pool.h"
namespace OHOS {
class TaskExecutor {
public:
    using TaskId = ExecutorPool::TaskId;
    using Task = std::function<void()>;
    using Duration = std::chrono::steady_clock::duration;
    static constexpr Duration INVALID_DURATION = std::chrono::milliseconds(0);
    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();

    static TaskExecutor &GetInstance();
    TaskId Execute(const Task &task);
    TaskId Schedule(Duration delay, const Task &task, Duration interval = INVALID_DURATION,
        uint64_t times = UNLIMITED_TIMES);
    bool Remove(TaskId taskId, bool wait = false);
    TaskId Reset(TaskId taskId, Duration interval);
    void SetExecutors(std::shared_ptr<ExecutorPool> executors);

private:
    std::mutex mtx_;
    size_t MAX_THREADS = 6;
    size_t MIN_THREADS = 0;
    TaskExecutor() = default;
    ~TaskExecutor();
    void GenerateExecutors();
    std::shared_ptr<ExecutorPool> pool_ = nullptr;
};
} // namespace OHOS
#endif // DISTRIBUTED_DATA_TASK_EXECUTOR_H
