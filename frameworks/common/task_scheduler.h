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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_TASK_SCHEDULER_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_TASK_SCHEDULER_H
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>

#include "visibility.h"
namespace OHOS {
class API_LOCAL TaskScheduler {
public:
    using TaskId = uint64_t;
    using Time = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;
    using Clock = std::chrono::steady_clock;
    using Task = std::function<void()>;
    inline static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
    inline static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
    inline static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();
    TaskScheduler(size_t capacity, const std::string &name)
    {
        capacity_ = capacity;
        isRunning_ = true;
        taskId_ = INVALID_TASK_ID;
        running_ = InnerTask();
        thread_ = std::make_unique<std::thread>([this, name]() {
            auto realName = std::string("scheduler_") + name;
            pthread_setname_np(pthread_self(), realName.c_str());
            Loop();
        });
    }
    TaskScheduler(const std::string &name) : TaskScheduler(std::numeric_limits<size_t>::max(), name) {}
    TaskScheduler(size_t capacity = std::numeric_limits<size_t>::max()) : TaskScheduler(capacity, "") {}
    ~TaskScheduler()
    {
        isRunning_ = false;
        Clean();
        Execute([]() {});
        thread_->join();
    }
    // execute task at specific time
    TaskId At(const Time &begin, Task task, Duration interval = INVALID_INTERVAL, uint64_t times = UNLIMITED_TIMES)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        if (tasks_.size() >= capacity_) {
            return INVALID_TASK_ID;
        }
        InnerTask innerTask;
        innerTask.times = times;
        innerTask.taskId = GenTaskId();
        innerTask.interval = interval;
        innerTask.exec = std::move(task);
        auto it = tasks_.insert({ begin, innerTask});
        if (it == tasks_.begin()) {
            condition_.notify_one();
        }
        indexes_[innerTask.taskId] = it;
        return innerTask.taskId;
    }
    TaskId Reset(TaskId taskId, const Duration &interval)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        if (running_.taskId == taskId && running_.interval != INVALID_INTERVAL) {
            running_.interval = interval;
            return running_.taskId;
        }
        auto index = indexes_.find(taskId);
        if (index == indexes_.end()) {
            return INVALID_TASK_ID;
        }
        auto &innerTask = index->second->second;
        if (innerTask.interval != INVALID_INTERVAL) {
            innerTask.interval = interval;
        }
        auto it = tasks_.insert({ std::chrono::steady_clock::now() + interval, std::move(innerTask) });
        if (it == tasks_.begin() || index->second == tasks_.begin()) {
            condition_.notify_one();
        }
        tasks_.erase(index->second);
        indexes_[taskId] = it;
        return taskId;
    }
    void Clean()
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        indexes_.clear();
        tasks_.clear();
    }
    // execute task periodically with duration
    TaskId Every(Duration interval, Task task)
    {
        return At(std::chrono::steady_clock::now() + interval, task, interval);
    }
    // remove task in SchedulerTask
    void Remove(TaskId taskId, bool wait = false)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        cond_.wait(lock, [this, taskId, wait]() {
            return (!wait || running_.taskId != taskId);
        });
        auto index = indexes_.find(taskId);
        if (index == indexes_.end()) {
            return;
        }
        tasks_.erase(index->second);
        indexes_.erase(index);
        condition_.notify_one();
    }
    // execute task periodically with duration after delay
    TaskId Every(Duration delay, Duration interval, Task task)
    {
        return At(std::chrono::steady_clock::now() + delay, task, interval);
    }
    // execute task for some times periodically with duration after delay
    TaskId Every(int32_t times, Duration delay, Duration interval, Task task)
    {
        return At(std::chrono::steady_clock::now() + delay, task, interval, times);
    }
    TaskId Execute(Task task)
    {
        return At(std::chrono::steady_clock::now(), std::move(task));
    }
private:
    struct InnerTask {
        TaskId taskId = INVALID_TASK_ID;
        Duration interval = INVALID_INTERVAL;
        uint64_t times = UNLIMITED_TIMES;
        std::function<void()> exec;
    };
    void Loop()
    {
        while (isRunning_) {
            std::function<void()> exec;
            {
                std::unique_lock<decltype(mutex_)> lock(mutex_);
                condition_.wait(lock, [this] {
                    return !tasks_.empty();
                });
                if (tasks_.begin()->first > std::chrono::steady_clock::now()) {
                    auto time = tasks_.begin()->first;
                    condition_.wait_until(lock, time);
                    continue;
                }
                auto it = tasks_.begin();
                running_ = it->second;
                exec = running_.exec;
                indexes_.erase(running_.taskId);
                tasks_.erase(it);
                running_.times--;
            }
            if (exec) {
                exec();
            }
            {
                std::unique_lock<decltype(mutex_)> lock(mutex_);
                if (running_.interval != INVALID_INTERVAL && running_.times > 0) {
                    auto it = tasks_.insert({ std::chrono::steady_clock::now() + running_.interval, running_ });
                    indexes_[running_.taskId] = it;
                }
                running_ = InnerTask();
                cond_.notify_all();
            }
        }
    }

    TaskId GenTaskId()
    {
        auto taskId = ++taskId_;
        if (taskId == INVALID_TASK_ID) {
            return ++taskId_;
        }
        return taskId;
    }

    volatile bool isRunning_;
    size_t capacity_;
    std::multimap<Time, InnerTask> tasks_;
    std::map<TaskId, decltype(tasks_)::iterator> indexes_;
    InnerTask running_;
    std::mutex mutex_;
    std::unique_ptr<std::thread> thread_;
    std::condition_variable condition_;
    std::condition_variable cond_;
    std::atomic<uint64_t> taskId_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_TASK_SCHEDULER_H
