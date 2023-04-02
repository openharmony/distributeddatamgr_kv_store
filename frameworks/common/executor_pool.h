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

#include "pool.h"
#include "priority_queue.h"
namespace OHOS {
class ExecutorPool {
public:
    using TaskId = uint64_t;
    using Task = std::function<void()>;
    using Duration = std::chrono::steady_clock::duration;
    using Time = std::chrono::steady_clock::time_point;
    static constexpr Time INVALID_TIME = std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>();
    static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();
    static constexpr Duration INVALID_DELAY = std::chrono::seconds(0);
    static constexpr TaskId START_TASK_ID = static_cast<uint64_t>(1l);
    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
    ExecutorPool(size_t max, size_t min)
    {
        pool_ = new Pool<Executor>(max, min);
        taskId_ = START_TASK_ID;
        execs_ = std::make_shared<PriorityQueue<InnerTask, Time, TaskId>>();
        delayTask_ = std::make_shared<PriorityQueue<InnerTask, Time, TaskId>>();
    }
    ~ExecutorPool()
    {
        pool_->Clean([](Executor *executor) {
            executor->Stop(true);
        });
    }

    TaskId Execute(Task task)
    {
        return Execute(task, GenTaskId());
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
        innerTask.managed = std::move(task);
        innerTask.startTime = std::chrono::steady_clock::now() + delay;
        innerTask.interval = interval;
        innerTask.times = times;
        innerTask.taskId = GenTaskId();
        return Schedule(innerTask);
    }

    bool Remove(TaskId taskId, bool wait = false)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        if (delayTask_->Remove(taskId)) {
            return true;
        }
        delCon_->wait(lock, [this, taskId, wait] {
            return (!wait || !execs_->Find(taskId).Valid());
        });
        return false;
    }

    TaskId Reset(TaskId taskId, Duration interval)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        auto it = delayTask_->Find(taskId);
        if (!it.Valid()) {
            return INVALID_TASK_ID;
        }
        if (!delayTask_->Remove(taskId)) {
            return INVALID_TASK_ID;
        }
        it.interval = interval;
        it.startTime = std::chrono::steady_clock::now() + interval;
        Schedule(it);
        return taskId;
    }

private:
    struct InnerTask {
        static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
        Task managed = [] {};
        std::function<void(InnerTask &)> exec = [](InnerTask innerTask = InnerTask(INVALID_TASK_ID)) {
            innerTask.managed();
        };
        Duration interval = INVALID_INTERVAL;
        uint64_t times = UNLIMITED_TIMES;
        TaskId taskId = INVALID_TASK_ID;
        Time startTime = INVALID_TIME;

        InnerTask() = default;
        explicit InnerTask(TaskId id) : taskId(id) {}
        bool Valid() const
        {
            return taskId != INVALID_TASK_ID;
        }
        TaskId GetId()
        {
            return this->taskId;
        }
    };
    class Executor {
    public:
        Executor()
        {
            thread_ = std::thread([this] {
                {
                    std::unique_lock<decltype(mutex_)> lock(mutex_);
                    condition_.wait(lock, [this] {
                        return currentTask_.Valid();
                    });
                }
                Run();
                thread_.detach();
                release_(this);
                stopCv_.notify_one();
            });
        }

        void SetTask(InnerTask &innerTask)
        {
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            currentTask_ = innerTask;
            condition_.notify_one();
        }

        void Bind(std::shared_ptr<PriorityQueue<InnerTask, Time, TaskId>> *queue, std::function<void(Executor *)> idle,
            std::function<bool(Executor *)> release, std::shared_ptr<std::condition_variable> delCon)
        {
            waits_ = *queue;
            idle_ = std::move(idle);
            release_ = std::move(release);
            delCon_ = delCon;
        };
        void Stop(bool wait = false)
        {
            running_ = false;
            condition_.notify_one();
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            stopCv_.wait(lock);
        };

    private:
        static constexpr Duration TIME_OUT = std::chrono::seconds(1);
        void Run()
        {
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            do {
                InnerTask innerTask = currentTask_;
                while (running_ && innerTask.Valid()) {
                    innerTask.exec(innerTask);
                    innerTask = waits_->Top();
                    currentTask_ = innerTask;
                    waits_->Poll();
                    delCon_->notify_one();
                }
                waits_ = nullptr;
                idle_(this);
            } while (running_ && condition_.wait_until(lock, std::chrono::steady_clock::now() + TIME_OUT, [this]() {
                return currentTask_.Valid();
            }));
        }

        bool running_ = true;
        std::mutex mutex_;
        std::shared_ptr<PriorityQueue<InnerTask, Time, TaskId>> waits_;
        InnerTask currentTask_;
        std::thread thread_;
        std::condition_variable condition_;
        std::condition_variable stopCv_;
        std::shared_ptr<std::condition_variable> delCon_;
        std::function<void(Executor *)> idle_;
        std::function<bool(Executor *)> release_;
    };

    TaskId Execute(Task task, TaskId taskId)
    {
        InnerTask innerTask;
        innerTask.managed = task;
        innerTask.taskId = taskId;
        auto executor = pool_->Get();
        if (executor == nullptr) {
            execs_->Push(innerTask);
            return innerTask.taskId;
        }
        executor->Bind(
            &execs_,
            [this](Executor *exe) {
                return pool_->Idle(exe);
            },
            [this](Executor *exe) -> bool {
                return pool_->Release(exe);
            },
            delCon_);
        executor->SetTask(innerTask);
        return innerTask.taskId;
    }

    TaskId Schedule(InnerTask innerTask)
    {
        if (scheduler_ == nullptr) {
            scheduler_ = std::shared_ptr<Executor>(pool_->Get(true));
            scheduler_->Bind(
                &delayTask_,
                [this](Executor *exe) {
                    return pool_->Idle(exe);
                },
                [this](Executor *exe) -> bool {
                    return pool_->Release(exe);
                },
                delCon_);
            scheduler_->SetTask(startTask);
        }

        auto run = [this](InnerTask &task) {
            if (task.startTime == INVALID_TIME) {
            }
            while (task.startTime != INVALID_TIME && task.startTime > std::chrono::steady_clock::now()) {
                std::unique_lock<decltype(mtx_)> lock(mtx_);
                cv_.wait_until(lock, task.startTime);
                continue;
            }
            Execute(task.managed, task.taskId);
            if (task.interval != INVALID_INTERVAL && --task.times > 0) {
                task.startTime = std::chrono::steady_clock::now() + task.interval;
                delayTask_->Push(task);
            }
        };
        innerTask.exec = run;
        delayTask_->Push(innerTask);
        cv_.notify_one();
        return innerTask.taskId;
    }

    TaskId GenTaskId()
    {
        auto taskId = ++taskId_;
        while (taskId == INVALID_TASK_ID || taskId == START_TASK_ID) {
            ++taskId_;
        }
        return taskId;
    }

    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<std::condition_variable> delCon_;
    Pool<Executor> *pool_;
    InnerTask startTask = InnerTask(START_TASK_ID);
    std::shared_ptr<Executor> scheduler_ = nullptr;
    std::shared_ptr<PriorityQueue<InnerTask, Time, TaskId>> execs_;
    std::shared_ptr<PriorityQueue<InnerTask, Time, TaskId>> delayTask_;
    std::atomic<TaskId> taskId_;
};
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_POOL_H
