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
namespace OHOS::DistributedKv {
class TaskExecutor {
public:
    using TaskId = ExecutorPool::TaskId;
    using Task = std::function<void()>;
    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();

    static TaskExecutor &GetInstance();
    TaskId Execute(Task &&task, int32_t delay = 0);
    TaskId Schedule(Task &&task, int32_t interval, int32_t delay = 0);
    TaskId Schedule(Task &&task, int32_t interval, int32_t delay, uint64_t times = UNLIMITED_TIMES);
    bool Remove(TaskId taskId, bool wait = false);
    TaskId Reset(TaskId taskId, int32_t interval, int32_t delay = 0);

private:
    size_t MAX_THREADS = 2;
    size_t MIN_THREADS = 0;
    TaskExecutor();
    ~TaskExecutor();

    std::shared_ptr<ExecutorPool> pool_;
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATAMGR_DATAMGR_EXECUTOR_FACTORY_H
