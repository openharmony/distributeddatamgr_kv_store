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
        taskId_ = 0;
        idleTime_ = idleTime;
    }
    ~KVThreadPool()
    {
        {
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            isRunning_ = false;
            indexes_.clear();
            runningIndexes_.clear();
            tasks_.clear();
            auto it = threadCons_.begin();
            while (it != threadCons_.end()) {
                it->second->notify_one();
                it++;
            }
        }
        auto it = threadIndexes.begin();
        while (it != threadIndexes.end()) {
            it->second->second->join();
            it++;
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
        delCond_.wait(lock, [this, taskId, wait] {
            return (!wait || runningIndexes_.find(taskId) == runningIndexes_.end());
        });
        auto index = indexes_.find(taskId);
        if (index == indexes_.end()) {
            return;
        }
        tasks_.erase(index->second);
        indexes_.erase(index);
        threadCons_.find(threadBucket_.begin()->second->get_id())->second->notify_one();
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
            threadCons_.find(threadBucket_.begin()->second->get_id())->second->notify_one();
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
        if (it == tasks_.begin() && !threadCons_.empty()) {
            threadCons_.find(threadBucket_.begin()->second->get_id())->second->notify_one();
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
        std::shared_ptr<std::condition_variable> threadCon;
        threadNum_++;
        auto it = threadBucket_.insert(
            { std::chrono::steady_clock::now(), std::make_shared<std::thread>([this, threadCon]() {
                 Time startIdle = std::chrono::steady_clock::now();
                 idleThread_++;
                 InnerTask innerTask;
                 while (isRunning_) {
                     std::shared_ptr<std::thread> thread;
                     {
                         std::unique_lock<decltype(mutex_)> lock(mutex_);
                         auto waitRes = threadCon->wait_until(lock, startIdle + idleTime_, [this] {
                             return (tasks_.empty() || !isRunning_);
                         });
                         if (!waitRes && threadNum_ > minThread_) {
                             threadNum_--;
                             break;
                         }
                         if (tasks_.empty()) {
                             continue;
                         }
                         if (tasks_.begin()->first > std::chrono::steady_clock::now()) {
                             threadCon->wait_until(lock, tasks_.begin()->first);
                             continue;
                         }

                         idleThread_--;
                         auto it = tasks_.begin();
                         innerTask = it->second;
                         indexes_.erase(innerTask.taskId);
                         runningIndexes_[innerTask.taskId] = it;
                         tasks_.erase(it);
                         innerTask.times--;

                         auto threadIt = threadBucket_.begin();
                         thread = threadIt->second;
                         threadIndexes.erase(thread->get_id());
                         threadBucket_.erase(threadIt);
                         if (!threadBucket_.empty()) {
                             threadCons_.find(threadBucket_.begin()->second->get_id())->second->notify_one();
                         }
                     }

                     if (innerTask.exec) {
                         innerTask.exec();
                     }

                     std::unique_lock<decltype(mutex_)> lock(mutex_);
                     runningIndexes_.erase(innerTask.taskId);
                     idleThread_++;
                     startIdle = std::chrono::steady_clock::now();
                     auto it = threadBucket_.emplace(std::chrono::steady_clock::now(), thread);
                     threadIndexes[it->second->get_id()] = it;

                     if (isRunning_ && innerTask.interval != INVALID_INTERVAL && innerTask.times > 0) {
                         auto tasksIt = tasks_.insert({ std::chrono::steady_clock::now() + innerTask.interval, innerTask });
                         indexes_[innerTask.taskId] = tasksIt;
                     }
                     innerTask = InnerTask();
                     delCond_.notify_all();
                 }
                 threadCons_.erase(std::this_thread::get_id());
                 threadIndexes.erase(std::this_thread::get_id());
             }) });
        threadCons_[it->second->get_id()] = threadCon;
        threadIndexes[it->second->get_id()] = it;
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
    std::condition_variable delCond_;
    std::multimap<Time, InnerTask> tasks_;
    std::map<TaskId, decltype(tasks_)::iterator> indexes_;
    std::map<TaskId, decltype(tasks_)::iterator> runningIndexes_;
    std::multimap<Time, std::shared_ptr<std::thread>, std::greater<>> threadBucket_;
    std::map<std::thread::id, decltype(threadBucket_)::iterator> threadIndexes;
    std::map<std::thread::id, std::shared_ptr<std::condition_variable>> threadCons_;
    std::atomic<uint64_t> taskId_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_KV_THREAD_POOL_H
