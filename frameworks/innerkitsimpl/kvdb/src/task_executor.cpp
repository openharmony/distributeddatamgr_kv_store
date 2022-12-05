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

bool TaskExecutor::Execute(TaskScheduler::Task &&task, int32_t interval)
{
    if (pool_ == nullptr) {
        return false;
    }
    auto time = TaskScheduler::Clock::now() + std::chrono::milliseconds(interval);
    pool_->At(time, std::move(task));
    return true;
}

TaskExecutor::TaskExecutor()
{
    pool_ = std::make_shared<TaskScheduler>();
}

TaskExecutor::~TaskExecutor()
{
    if (pool_ != nullptr) {
        pool_->Clean();
        pool_ = nullptr;
    }
}
} // namespace OHOS::DistributedKv
