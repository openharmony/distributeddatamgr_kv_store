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
    using TaskQueue = PriorityQueue<InnerTask, Time, TaskId>;
    static constexpr Time INVALID_TIME = std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>();
    static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();
    static constexpr Duration INVALID_DELAY = std::chrono::seconds(0);
    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);

    ExecutorPool(size_t max, size_t min)
        : pool_(max, min), delayTasks_(InnerTask(), NextTimer), taskId_(INVALID_TASK_ID)
    {
        // When max equals 1, timer thread schedules and executes tasks.
        if (max > 1) {
            execs_ = new (std::nothrow) TaskQueue(InnerTask());
        }
    }

    ~ExecutorPool()
    {
        poolStatus = Status::IS_STOPPING;
        if (execs_ != nullptr) {
            execs_->Clean();
        }
        delayTasks_.Clean();
        std::shared_ptr<Executor> scheduler;
        {
            std::lock_guard<decltype(mtx_)> scheduleLock(mtx_);
            scheduler = std::move(scheduler_);
        }
        if (scheduler != nullptr) {
            scheduler->Stop(true);
        }
        pool_.Clean([](std::shared_ptr<Executor> executor) {
            executor->Stop(true);
        });
        delete execs_;
        poolStatus = Status::STOPPED;
    }

    TaskId Execute(Task task)
    {
        if (poolStatus != Status::RUNNING) {
            return INVALID_TASK_ID;
        }

        if (execs_ == nullptr) {
            return Schedule(std::move(task), INVALID_DELAY, INVALID_INTERVAL, UNLIMITED_TIMES);
        }

        return Execute(std::move(task), GenTaskId());
    }

    TaskId Schedule(Duration delay, Task task)
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
        innerTask.exec = std::move(task);
        innerTask.interval = interval;
        innerTask.times = times;
        innerTask.taskId = GenTaskId();
        return Schedule(std::move(innerTask), std::chrono::steady_clock::now() + delay);
    }

    bool Remove(TaskId taskId, bool wait = false)
    {
        bool res = true;
        auto delay = delayTasks_.Find(taskId);
        if (!delay.Valid()) {
            res = false;
        }
        delayTasks_.Remove(taskId, wait);
        if (execs_ != nullptr) {
            execs_->Remove(taskId, wait);
        }
        return res;
    }

    TaskId Reset(TaskId taskId, Duration interval)
    {
        auto updated = delayTasks_.Update(taskId, [interval](InnerTask &task) -> std::pair<bool, Time> {
            if (task.interval != INVALID_INTERVAL) {
                task.interval = interval;
            }
            auto time = std::chrono::steady_clock::now() + interval;
            return std::pair{ task.interval != INVALID_INTERVAL, time };
        });
        return updated ? taskId : INVALID_TASK_ID;
    }

private:
    TaskId Execute(Task task, TaskId taskId)
    {
        InnerTask innerTask;
        innerTask.exec = task;
        innerTask.taskId = taskId;
        execs_->Push(std::move(innerTask), taskId, INVALID_TIME);
        auto executor = pool_.Get();
        if (executor == nullptr) {
            return taskId;
        }
        executor->Bind(
            execs_,
            [this](std::shared_ptr<Executor> exe) {
                pool_.Idle(exe);
                return true;
            },
            [this](std::shared_ptr<Executor> exe, bool force) -> bool {
                return pool_.Release(exe, force);
            });
        return taskId;
    }

    TaskId Schedule(InnerTask innerTask, Time delay)
    {
        auto id = innerTask.taskId;
        if (execs_ != nullptr) {
            auto func = innerTask.exec;
            auto run = [this, func, id]() {
                Execute(func, id);
            };
            innerTask.exec = run;
        }
        delayTasks_.Push(std::move(innerTask), id, delay);
        std::lock_guard<decltype(mtx_)> scheduleLock(mtx_);
        if (scheduler_ == nullptr) {
            scheduler_ = pool_.Get(true);
            scheduler_->Bind(
                &delayTasks_,
                [this](std::shared_ptr<Executor> exe) {
                    std::unique_lock<decltype(mtx_)> lock(mtx_);
                    if (delayTasks_.Size() != 0) {
                        return false;
                    }
                    scheduler_ = nullptr;
                    pool_.Idle(exe);
                    return true;
                },
                [this](std::shared_ptr<Executor> exe, bool force) -> bool {
                    return pool_.Release(exe, force);
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

    static std::pair<bool, Time> NextTimer(InnerTask &task)
    {
        if (task.interval != INVALID_INTERVAL && --task.times > 0) {
            auto time = std::chrono::steady_clock::now() + task.interval;
            return { true, time };
        }
        return { false, INVALID_TIME };
    }

    Status poolStatus = Status::RUNNING;
    std::mutex mtx_;
    Pool<Executor> pool_;
    TaskQueue delayTasks_;
    std::shared_ptr<Executor> scheduler_ = nullptr;
    TaskQueue *execs_ = nullptr;
    std::atomic<TaskId> taskId_;
};
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_POOL_H
