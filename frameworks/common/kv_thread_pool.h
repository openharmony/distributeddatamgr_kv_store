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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_KV_THREAD_POOL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_KV_THREAD_POOL_H
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace OHOS {
class KVThreadPool {
public:
    using TaskId = uint64_t;
    using Duration = std::chrono::steady_clock::duration;
    using Task = std::function<void()>;

    static constexpr TaskId INVALID_ID = static_cast<uint64_t>(0l);
    static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();

    KVThreadPool(size_t maxThread, size_t minThread, Duration idleTime = std::chrono::seconds(2))
    {
        maxThread_ = maxThread;
        minThread_ = minThread;
        threadNum_ = 0;
        idleThread_ = 0;
        idleTime_ = idleTime;
        for (int i = 0; i < minThread_; ++i) {
            AutoScaling();
        }
    }
    ~KVThreadPool()
    {
        {
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            isRunning_ = false;
            indexes_.clear();
            runningIndexes_.clear();
            tasks_.clear();
            condition_.notify_all();
        }
        for (auto &item : threads_) {
            item.join();
        }
    }

    TaskId Execute(Task task)
    {
        return At(std::move(task), std::chrono::steady_clock::now());
    }

    TaskId Execute(Task task, Duration delay)
    {
        return At(std::move(task), std::chrono::steady_clock::now() + delay);
    }

    TaskId Schedule(Task task, Duration interval)
    {
        return At(std::move(task), std::chrono::steady_clock::now() + interval, interval);
    }

    TaskId Schedule(Task task, Duration delay, Duration interval)
    {
        return At(std::move(task), std::chrono::steady_clock::now() + delay, interval);
    }

    TaskId Schedule(Task task, Duration delay, Duration interval, uint64_t times)
    {
        return At(std::move(task), std::chrono::steady_clock::now() + delay, interval, times);
    }

    void Remove(TaskId taskId, bool wait = false)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto runningIndex = runningIndexes_.find(taskId);
        if (runningIndex != runningIndexes_.end()) {
            runningIndex->second->second.needDelete = true;
            delCond_.wait(lock, [this, taskId, wait] {
                return (!wait || runningIndexes_.find(taskId) == runningIndexes_.end());
            });
            return;
        }
        auto index = indexes_.find(taskId);
        if (index == indexes_.end()) {
            return;
        }
        tasks_.erase(index->second);
        indexes_.erase(index);
        condition_.notify_one();
    }

    TaskId Reset(TaskId taskId, Duration interval)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        auto runningIndex = runningIndexes_.find(taskId);
        if (runningIndex != runningIndexes_.end() && runningIndex->second->second.interval != INVALID_INTERVAL) {
            runningIndex->second->second.interval = interval;
            return runningIndex->second->second.taskId;
        }

        auto index = indexes_.find(taskId);
        if (index == indexes_.end()) {
            return INVALID_ID;
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

private:
    using Time = std::chrono::steady_clock::time_point;

    struct InnerTask {
        TaskId taskId = INVALID_ID;
        Duration interval = INVALID_INTERVAL;
        uint64_t times = UNLIMITED_TIMES;
        bool needDelete = false;
        std::function<void()> exec;
    };

    TaskId At(Task task, Time begin, Duration interval = INVALID_INTERVAL, uint64_t times = UNLIMITED_TIMES)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        InnerTask innerTask;
        innerTask.times = times;
        innerTask.taskId = GenTaskId();
        innerTask.interval = interval;
        innerTask.exec = std::move(task);
        auto it = tasks_.insert({ begin, innerTask });
        if (it == tasks_.begin()) {
            condition_.notify_one();
        }
        AutoScaling();
        indexes_[innerTask.taskId] = it;
        return innerTask.taskId;
    }

    void AutoScaling()
    {
        if (idleThread_ != 0 || threadNum_ >= maxThread_) {
            return;
        }
        threadNum_++;
        threads_.emplace_back(std::thread([this]() {
            Time startIdle = std::chrono::steady_clock::now();
            idleThread_++;
            while (isRunning_) {
                InnerTask innerTask;
                {
                    std::unique_lock<decltype(mutex_)> lock(mutex_);
                    auto waitRes = condition_.wait_until(lock, startIdle + idleTime_, [this] {
                        return (!tasks_.empty() || !isRunning_);
                    });

                    if (!waitRes && threadNum_ > minThread_) {
                        threadNum_--;
                        break;
                    }
                    if (tasks_.empty()) {
                        continue;
                    }
                    if (tasks_.begin()->first > std::chrono::steady_clock::now()) {
                        auto time = tasks_.begin()->first;
                        condition_.wait_until(lock, time);
                        continue;
                    }

                    idleThread_--;
                    auto it = tasks_.begin();
                    innerTask = it->second;
                    indexes_.erase(innerTask.taskId);
                    runningIndexes_[innerTask.taskId] = it;
                    tasks_.erase(it);
                    innerTask.times--;
                }

                if (innerTask.exec) {
                    innerTask.exec();
                }

                std::unique_lock<decltype(mutex_)> lock(mutex_);
                runningIndexes_.erase(innerTask.taskId);
                idleThread_++;
                startIdle = std::chrono::steady_clock::now();

                if (isRunning_ && !innerTask.needDelete && innerTask.interval != INVALID_INTERVAL &&
                    innerTask.times > 0) {
                    auto it = tasks_.insert({ std::chrono::steady_clock::now() + innerTask.interval, innerTask });
                    indexes_[innerTask.taskId] = it;
                }
                innerTask = InnerTask();
                delCond_.notify_all();
            }
        }));
    }

    TaskId GenTaskId()
    {
        auto taskId = ++taskId_;
        if (taskId == INVALID_ID) {
            return ++taskId_;
        }
        return taskId;
    }

    size_t maxThread_;
    size_t minThread_;
    std::atomic<size_t> threadNum_;
    std::atomic<size_t> idleThread_;
    Duration idleTime_;
    std::mutex mutex_;
    bool isRunning_ = true;
    std::condition_variable condition_;
    std::condition_variable delCond_;
    std::multimap<Time, InnerTask> tasks_;
    std::vector<std::thread> threads_;
    std::map<TaskId, decltype(tasks_)::iterator> indexes_;
    std::map<TaskId, decltype(tasks_)::iterator> runningIndexes_;
    std::atomic<uint64_t> taskId_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_KV_THREAD_POOL_H
