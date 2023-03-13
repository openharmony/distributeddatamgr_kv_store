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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_THREAD_POOL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_THREAD_POOL_H
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace OHOS {
class ThreadPool {
public:
    using TaskId = uint64_t;
    using Duration = std::chrono::steady_clock::duration;
    using Task = std::function<void()>;

    static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
    static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();

    ThreadPool(size_t maxThread, size_t minThread)
    {
        maxThread_ = maxThread;
        minThread_ = minThread;
        Start();
    }
    ~ThreadPool()
    {
        Clean();
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
        auto it = tasks_.begin();
        while (it != tasks_.end()) {
            if (it->taskId == taskId) {
                break;
            }
        }
        if (it == tasks_.end()) {
            return;
        }
        cond_.wait(lock, [wait, it]() {
            return (!wait || !it->isRunning);
        });
        tasks_.erase(it);
    }

    TaskId Reset(TaskId taskId, Duration delay, Duration interval)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = tasks_.begin();
        while (it != tasks_.end()) {
            if (it->taskId == taskId) {
                auto innerTask = it.base();
                tasks_.erase(it);
                innerTask->startTime = std::chrono::steady_clock::now() + delay;
                innerTask->interval = interval;
                tasks_.push_back(*innerTask);
                return innerTask->taskId;
            }
        }
        return INVALID_TASK_ID;
    }

private:
    using Time = std::chrono::steady_clock::time_point;
    const int HOUSE_KEEP_TIME = 30;

    struct InnerTask {
        TaskId taskId = INVALID_TASK_ID;
        Time startTime = std::chrono::steady_clock::now();
        Duration interval = INVALID_INTERVAL;
        uint64_t times = UNLIMITED_TIMES;
        std::function<void()> exec;
        bool isRunning;
    };

    class Thread {
    public:
        Thread(std::shared_ptr<ThreadPool> threadPool)
        {
            threadPool_ = std::move(threadPool);
            idleTime = std::chrono::steady_clock::now();
            InnerTask task;
            actualThread = std::make_unique<std::thread>([this, &task]() {
                Start(task);
            });
        }

        ~Thread()
        {
            Release();
        }

        bool IsIdle()
        {
            return isIdle;
        }

        Time GetIdleTime()
        {
            return idleTime;
        }

        void Release()
        {
            isRunning_ = false;
            actualThread->join();
        }

    private:
        void Start(InnerTask task)
        {
            while (isRunning_) {
                while (task.exec != nullptr || (task = GetTask()).exec != nullptr) {
                    isIdle = false;
                    std::unique_lock<decltype(mutex_)> lock(mutex_);
                    if (task.startTime > std::chrono::steady_clock::now()) {
                        condition_.wait_until(lock, task.startTime);
                    }
                    task.isRunning = true;
                    task.exec();
                    if (task.interval != INVALID_INTERVAL && task.times > 0) {
                        --task.times;
                        task.isRunning = false;
                        threadPool_->tasks_.push_back(task);
                    }
                    idleTime = std::chrono::steady_clock::now();
                    isIdle = true;
                }
            }
        }

        InnerTask GetTask()
        {
            return threadPool_->GetTask();
        }

        std::atomic<bool> isIdle = true;
        std::atomic<bool> isRunning_ = true;
        Time idleTime;
        std::mutex mutex_;
        std::condition_variable condition_;
        std::unique_ptr<std::thread> actualThread;
        std::shared_ptr<ThreadPool> threadPool_;
    };

    TaskId At(Task task, Time begin, Duration interval = INVALID_INTERVAL, uint64_t times = UNLIMITED_TIMES)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        InnerTask innerTask;
        innerTask.times = times;
        innerTask.taskId = GenTaskId();
        innerTask.startTime = begin;
        innerTask.interval = interval;
        innerTask.exec = std::move(task);
        tasks_.push_back(innerTask);
        if (!HasIdleThread() && threads_.size() < maxThread_) {
            CreateThread();
        }
        if (++times_ > HOUSE_KEEP_TIME) {
            HouseKeep();
        }
        return innerTask.taskId;
    }

    void Start()
    {
        for (int i = 0; i < minThread_; ++i) {
            CreateThread();
        }
    }

    void Clean()
    {
        for (auto thread : threads_) {
            thread->Release();
        }
        threads_.clear();
        tasks_.clear();
    }

    void HouseKeep()
    {
        times_ = 0;
        typedef std::vector<std::shared_ptr<Thread>> ThreadVec;
        ThreadVec idleThreads;
        ThreadVec expiredThreads;
        ThreadVec activeThreads;
        idleThreads.reserve(threads_.size());
        activeThreads.reserve(threads_.size());

        for (auto thread : threads_) {
            if (thread->IsIdle()) {
                if (std::chrono::steady_clock::now() - thread->GetIdleTime() < idleTime_) {
                    idleThreads.push_back(thread);
                } else {
                    expiredThreads.push_back(thread);
                }
            } else {
                activeThreads.push_back(thread);
            }
        }
        int n = (int)activeThreads.size();
        int limit = (int)idleThreads.size() + n;
        if (limit < minThread_) {
            limit = minThread_;
        }
        idleThreads.insert(idleThreads.end(), expiredThreads.begin(), expiredThreads.end());
        threads_.clear();
        for (auto idleThread : idleThreads) {
            if (n < limit) {
                threads_.push_back(idleThread);
                ++n;
            } else {
                idleThread->Release();
            }
        }
        threads_.insert(threads_.end(), activeThreads.begin(), activeThreads.end());
    }

    void CreateThread()
    {
        auto thread = std::make_shared<Thread>(std::shared_ptr<ThreadPool>(this));
        threads_.push_back(thread);
    }

    bool HasIdleThread()
    {
        for (std::shared_ptr<Thread> thread : threads_) {
            if (thread->IsIdle()) {
                return true;
            }
        }
        return false;
    }

    InnerTask GetTask()
    {
        std::unique_lock<std::mutex> lock(this->mutex_);
        InnerTask innerTask;
        if (tasks_.empty()) {
            return innerTask;
        }
        innerTask = tasks_.front();
        tasks_.erase(tasks_.begin());
        return innerTask;
    }

    TaskId GenTaskId()
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        auto taskId = ++taskId_;
        if (taskId == INVALID_TASK_ID) {
            return ++taskId_;
        }
        return taskId;
    }

    std::string name_;
    size_t maxThread_;
    size_t minThread_;
    Duration idleTime_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::vector<InnerTask> tasks_;
    std::vector<std::shared_ptr<Thread>> threads_;
    std::atomic<uint64_t> taskId_;
    uint8_t times_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_THREAD_POOL_H
