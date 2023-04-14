/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_POOL_H
#define OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_POOL_H
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "executor.h"
#include "pool.h"
#include "priority_queue.h"
namespace OHOS {
class ExecutorPool {
public:
    using TaskId = Executor::TaskId;
    using Task = Executor::Task;
    using Duration = Executor::Duration;
    using Time = Executor::Time;
    using InnerTask = Executor::InnerTask;
    using Status = Executor::Status;
    static constexpr Time INVALID_TIME = std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>();
    static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();
    static constexpr Duration INVALID_DELAY = std::chrono::seconds(0);
    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);

    ExecutorPool(size_t max, size_t min)
    {
        pool_ = new (std::nothrow) Pool<Executor>(max, min);
        execs_ = new PriorityQueue<InnerTask, Time, TaskId>();
        delayTasks_ = new PriorityQueue<InnerTask, Time, TaskId>();
        taskId_ = INVALID_TASK_ID;
    }
    ~ExecutorPool()
    {
        poolStatus = Status::IS_STOPPING;
        execs_->Clean();
        delayTasks_->Clean();
        std::shared_ptr<Executor> scheduler;
        {
            std::lock_guard<decltype(mtx_)> scheduleLock(mtx_);
            scheduler = std::move(scheduler_);
        }
        if (scheduler != nullptr) {
            scheduler->Stop(true);
        }
        pool_->Clean([](std::shared_ptr<Executor> executor) {
            executor->Stop(true);
        });
        delete execs_;
        delete delayTasks_;
        delete pool_;
        poolStatus = Status::STOPPED;
    }

    TaskId Execute(Task task)
    {
        if (poolStatus != Status::RUNNING) {
            return INVALID_TASK_ID;
        }
        auto run = [task](InnerTask innerTask) {
            task();
        };
        return Execute(std::move(run), GenTaskId());
    }

    TaskId Execute(Task task, Duration delay)
    {
        return Schedule(std::move(task), delay, INVALID_INTERVAL, 1);
    }

    TaskId Schedule(Task task, Duration interval)
    {
        return Schedule(std::move(task), INVALID_DELAY, interval, UNLIMITED_TIMES);
    }

    TaskId Schedule(Task task, Duration delay, Duration interval)
    {
        return Schedule(std::move(task), delay, interval, UNLIMITED_TIMES);
    }

    TaskId Schedule(Task task, Duration delay, Duration interval, uint64_t times)
    {
        InnerTask innerTask;
        auto run = [task](InnerTask innerTask) {
            task();
        };
        innerTask.exec = std::move(run);
        innerTask.startTime = std::chrono::steady_clock::now() + delay;
        innerTask.interval = interval;
        innerTask.times = times;
        innerTask.taskId = GenTaskId();
        return Schedule(std::move(innerTask));
    }

    bool Remove(TaskId taskId, bool wait = false)
    {
        bool res = true;
        if (!delayTasks_->Find(taskId).Valid() && !execs_->Find(taskId).Valid()) {
            res = false;
        }
        delayTasks_->Remove(taskId, wait);
        execs_->Remove(taskId, wait);
        return res;
    }

    TaskId Reset(TaskId taskId, Duration interval)
    {
        return Reset(taskId, INVALID_DELAY, interval);
    }

    TaskId Reset(TaskId taskId, Duration delay, Duration interval)
    {
        auto innerTask = delayTasks_->Find(taskId);
        if (!innerTask.Valid()) {
            return INVALID_TASK_ID;
        }
        delayTasks_->Remove(taskId, false);
        innerTask.startTime = std::chrono::steady_clock::now() + delay;
        innerTask.interval = interval;
        delayTasks_->Push(std::move(innerTask));
        return taskId;
    }

private:
    TaskId Execute(std::function<void(InnerTask)> task, TaskId taskId)
    {
        InnerTask innerTask;
        innerTask.exec = task;
        innerTask.taskId = taskId;
        execs_->Push(std::move(innerTask));
        auto executor = pool_->Get();
        if (executor == nullptr) {
            return taskId;
        }
        executor->Bind(
            execs_,
            [this](std::shared_ptr<Executor> exe) {
                pool_->Idle(exe);
                return true;
            },
            [this](std::shared_ptr<Executor> exe, bool force) -> bool {
                return pool_->Release(exe, force);
            });
        return taskId;
    }

    TaskId Schedule(InnerTask innerTask)
    {
        auto func = innerTask.exec;
        auto id = innerTask.taskId;
        auto run = [this, func, id](InnerTask task) {
            if (task.interval != INVALID_INTERVAL && --task.times > 0) {
                task.startTime = std::chrono::steady_clock::now() + task.interval;
                delayTasks_->Update(task.taskId, task.startTime);
            }
            Execute(func, id);
        };
        innerTask.exec = run;
        delayTasks_->Push(innerTask);
        std::lock_guard<decltype(mtx_)> scheduleLock(mtx_);
        if (scheduler_ == nullptr) {
            scheduler_ = pool_->Get(true);
            scheduler_->Bind(
                delayTasks_,
                [this](std::shared_ptr<Executor> exe) {
                    std::unique_lock<decltype(mtx_)> lock(mtx_);
                    if (delayTasks_->Size() != 0) {
                        return false;
                    }
                    scheduler_ = nullptr;
                    pool_->Idle(exe);
                    return true;
                },
                [this](std::shared_ptr<Executor> exe, bool force) -> bool {
                    return pool_->Release(exe, force);
                });
        }
        return innerTask.taskId;
    }

    TaskId GenTaskId()
    {
        auto taskId = ++taskId_;
        if (taskId == INVALID_TASK_ID) {
            taskId = ++taskId_;
        }
        return taskId;
    }

    Status poolStatus = Status::RUNNING;
    std::mutex mtx_;
    Pool<Executor> *pool_;
    std::shared_ptr<Executor> scheduler_ = nullptr;
    PriorityQueue<InnerTask, Time, TaskId> *execs_;
    PriorityQueue<InnerTask, Time, TaskId> *delayTasks_;
    std::atomic<TaskId> taskId_;
};
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_POOL_H
