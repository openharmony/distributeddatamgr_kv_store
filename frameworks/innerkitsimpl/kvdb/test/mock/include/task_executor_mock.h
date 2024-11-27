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
#ifndef DISTRIBUTED_DATA_TASK_EXECUTOR_MOCK_H
#define DISTRIBUTED_DATA_TASK_EXECUTOR_MOCK_H
#include "task_executor.h"
#include <gmock/gmock.h>

namespace OHOS {
using TaskId = ExecutorPool::TaskId;
using Task = std::function<void()>;
using Duration = std::chrono::steady_clock::duration;

class BTaskExecutor {
public:
    BTaskExecutor() = default;
    virtual ~BTaskExecutor() = default;
    virtual TaskId Schedule(Duration, const Task &, Duration, uint64_t) = 0;

public:
    static inline std::shared_ptr<BTaskExecutor> taskExecutor = nullptr;
};

class TaskExecutorMock : public BTaskExecutor {
public:
    MOCK_METHOD(TaskId, Schedule, (Duration, const Task &, Duration, uint64_t));
};
} // namespace OHOS
#endif // DISTRIBUTED_DATA_TASK_EXECUTOR_MOCK_H