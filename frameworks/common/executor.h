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
#ifndef OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_H
#define OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_H
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "priority_queue.h"
namespace OHOS {
class Executor : public std::enable_shared_from_this<Executor> {
public:
    using TaskId = uint64_t;
    using Task = std::function<void()>;
    using Duration = std::chrono::steady_clock::duration;
    using Time = std::chrono::steady_clock::time_point;
    static constexpr Time INVALID_TIME = std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>();
    static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();
    static constexpr Duration INVALID_DELAY = std::chrono::seconds(0);
    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);

    enum Status {
        RUNNING,
        IS_STOPPING,
        STOPPED
    };
    struct InnerTask {
        Task managed = [] {};
        std::function<void(InnerTask &)> exec = [this](InnerTask innerTask = InnerTask(INVALID_TASK_ID)) {
            innerTask.managed();
        };
        Duration interval = INVALID_INTERVAL;
        uint64_t times = UNLIMITED_TIMES;
        TaskId taskId = INVALID_TASK_ID;
        Time startTime = INVALID_TIME;
        InnerTask() = default;

        explicit InnerTask(TaskId id) : taskId(id)
        {
            managed = [] {};
        }

        bool Valid() const
        {
            return taskId != INVALID_TASK_ID;
        }

        TaskId GetId()
        {
            return this->taskId;
        }
    };

    void Start()
    {
        thisPtr_ = shared_from_this();
        thread_ = std::thread([this] {
            Run();
        });
    }

    void Bind(std::shared_ptr<PriorityQueue<InnerTask, Time, TaskId>> &queue,
        std::function<void(std::shared_ptr<Executor>)> idle,
        std::function<bool(std::shared_ptr<Executor>, bool)> release, InnerTask &innerTask)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        waits_ = queue;
        idle_ = std::move(idle);
        release_ = std::move(release);
        currentTask_ = innerTask;
        condition_.notify_one();
    }

    void Stop(bool wait = false)
    {
        {
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            running_ = IS_STOPPING;
            condition_.notify_one();
        }
        if (wait) {
            thread_.join();
        }
    }
    void SetTask(InnerTask &innerTask)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        currentTask_ = innerTask;
    }

    bool IsRunningTask(TaskId taskId)
    {
        return taskId == currentTask_.taskId;
    }

private:
    static constexpr Duration TIME_OUT = std::chrono::seconds(2);
    void Run()
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        bool forceStop = false;
        do {
            do {
                condition_.wait(lock, [this] {
                    return running_ == IS_STOPPING || (waits_ != nullptr && currentTask_.Valid());
                });
                InnerTask innerTask = currentTask_;
                while (running_ == RUNNING && innerTask.Valid()) {
                    lock.unlock();
                    innerTask.exec(innerTask);
                    lock.lock();
                    currentTask_ = waits_->Pop();
                    innerTask = currentTask_;
                }
                waits_ = nullptr;
                idle_(thisPtr_);
            } while (running_ == RUNNING &&
                     condition_.wait_until(lock, std::chrono::steady_clock::now() + TIME_OUT, [this]() {
                         return (currentTask_.Valid());
                     }));
            forceStop = running_ != RUNNING;
        } while (!release_(thisPtr_, forceStop));
        running_ = STOPPED;
    }

    Status running_ = RUNNING;
    std::mutex mutex_;
    std::shared_ptr<PriorityQueue<InnerTask, Time, TaskId>> waits_;
    InnerTask currentTask_ = InnerTask();
    std::thread thread_;
    std::shared_ptr<Executor> thisPtr_;
    std::condition_variable condition_;
    std::function<void(std::shared_ptr<Executor>)> idle_;
    std::function<bool(std::shared_ptr<Executor>, bool)> release_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_EXECUTOR_H
