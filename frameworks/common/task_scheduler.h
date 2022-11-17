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
#include <memory>
#include <thread>
#include <limits>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include "visibility.h"
namespace OHOS {
class API_LOCAL TaskScheduler {
public:
    using Time = std::chrono::system_clock::time_point;
    using Duration = std::chrono::system_clock::duration;
    using System = std::chrono::system_clock;
    using Iterator = std::map<std::chrono::system_clock::time_point, std::function<void()>>::iterator;
    using Task = std::function<void()>;
    TaskScheduler(size_t capacity = std::numeric_limits<size_t>::max())
    {
        capacity_ = capacity;
        isRunning_ = true;
        thread_ = std::make_unique<std::thread>([this]() { this->Loop(); });
    }
    ~TaskScheduler()
    {
        isRunning_ = false;
        At(std::chrono::system_clock::now(), [](){});
        thread_->join();
    }
    // execute task at specific time
    Iterator At(const Time &time, std::function<void()> task)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (tasks_.size() >= capacity_) {
            return tasks_.end();
        }

        auto it = tasks_.insert({time, task});
        if (it == tasks_.begin()) {
            condition_.notify_one();
        }
        return it;
    }
    Iterator Reset(Iterator task, const Time &time, const Duration &interval)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (tasks_.begin()->first > time) {
            return {};
        }

        auto current = std::chrono::system_clock::now();
        if (current >= time) {
            return {};
        }

        auto it = tasks_.insert({ current + interval, std::move(task->second) });
        if (it == tasks_.begin()) {
            condition_.notify_one();
        }
        tasks_.erase(task);
        return it;
    }
    void Clean()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.clear();
    }

    // execute task periodically with duration
    void Every(Duration interval, Task task)
    {
        std::function<void()> waitFunc = [this, interval, task]() {
            task();
            this->Every(interval, task);
        };
        At(std::chrono::system_clock::now() + interval, waitFunc);
    }
    // remove task in SchedulerTask
    void Remove(const Iterator &task)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = tasks_.find(task->first);
        if (it != tasks_.end()) {
            tasks_.erase(it);
            condition_.notify_one();
        }
    }
    // execute task periodically with duration after delay
    void Every(Duration delay, Duration interval, Task task)
    {
        std::function<void()> waitFunc = [this, interval, task]() {
            task();
            Every(interval, task);
        };
        At(std::chrono::system_clock::now() + delay, waitFunc);
    }
    // execute task for some times periodically with duration after delay
    void Every(int32_t times, Duration delay, Duration interval, Task task)
    {
        std::function<void()> waitFunc = [this, times, interval, task]() {
            task();
            int count = times;
            count--;
            if (times > 1) {
                Every(count, interval, interval, task);
            }
        };

        At(std::chrono::system_clock::now() + delay, waitFunc);
    }

private:
    void Loop()
    {
        while (isRunning_) {
            std::function<void()> exec;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (tasks_.empty()) {
                    condition_.wait(lock);
                } else {
                    condition_.wait_until(lock, tasks_.begin()->first);
                }
                if ((!tasks_.empty()) && (tasks_.begin()->first <= std::chrono::system_clock::now())) {
                    exec = tasks_.begin()->second;
                    tasks_.erase(tasks_.begin());
                }
            }

            if (exec) {
                exec();
            }
        }
    }
    volatile bool isRunning_;
    size_t capacity_;
    std::multimap<std::chrono::system_clock::time_point, std::function<void()>> tasks_;
    std::mutex mutex_;
    std::unique_ptr<std::thread> thread_;
    std::condition_variable condition_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_TASK_SCHEDULER_H
