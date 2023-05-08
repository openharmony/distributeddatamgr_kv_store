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

#include "task_executor_adapter.h"
namespace OHOS::DistributedKv {
TaskExecutorAdapter::TaskId TaskExecutorAdapter::Execute(const TaskExecutorAdapter::Task &task)
{
    return TaskExecutor::GetInstance().Execute(task);
}
TaskExecutorAdapter::TaskId TaskExecutorAdapter::Execute(const TaskExecutorAdapter::Task &task,
    TaskExecutorAdapter::Duration delay)
{
    return TaskExecutor::GetInstance().Schedule(delay, task);
}
TaskExecutorAdapter::TaskId TaskExecutorAdapter::Schedule(const TaskExecutorAdapter::Task &task,
    TaskExecutorAdapter::Duration interval)
{
    return TaskExecutor::GetInstance().Schedule(TaskExecutor::INVALID_DURATION, task, interval);
}
TaskExecutorAdapter::TaskId TaskExecutorAdapter::Schedule(const TaskExecutorAdapter::Task &task,
    TaskExecutorAdapter::Duration delay, TaskExecutorAdapter::Duration interval)
{
    return TaskExecutor::GetInstance().Schedule(delay, task, interval);
}
TaskExecutorAdapter::TaskId TaskExecutorAdapter::Schedule(const TaskExecutorAdapter::Task &task,
    TaskExecutorAdapter::Duration delay, TaskExecutorAdapter::Duration interval, uint64_t times)
{
    return TaskExecutor::GetInstance().Schedule(delay, task, interval, times);
}

bool TaskExecutorAdapter::Remove(const TaskExecutorAdapter::TaskId &taskId, bool wait)
{
    return TaskExecutor::GetInstance().Remove(taskId, wait);
}
TaskExecutorAdapter::TaskId TaskExecutorAdapter::Reset(const TaskExecutorAdapter::TaskId &taskId,
    TaskExecutorAdapter::Duration interval)
{
    return TaskExecutor::GetInstance().Reset(taskId, interval);
}
TaskExecutorAdapter::TaskExecutorAdapter()
{
}
TaskExecutorAdapter::~TaskExecutorAdapter()
{}
} // namespace OHOS
