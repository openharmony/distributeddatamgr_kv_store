/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef THREAD_POOL_STUB_H
#define THREAD_POOL_STUB_H

#include <memory>
#include <mutex>
#include <shared_mutex>

#include "ithread_pool.h"
#include "task_pool.h"

namespace DistributedDB {
using TaskAction = std::function<void(void)>;
class ThreadPoolStub {
    ThreadPoolStub();
    ~ThreadPoolStub();
    ThreadPoolStub(const ThreadPoolStub &) = delete;
    ThreadPoolStub &operator=(const ThreadPoolStub &) = delete;
    ThreadPoolStub(ThreadPoolStub &&) = delete;
    ThreadPoolStub &operator=(ThreadPoolStub &&) = delete;

public:
    static ThreadPoolStub &GetInstance();
    // thread pool interfaces.
    void SetThreadPool(const std::shared_ptr<IThreadPool> &threadPool);
    std::shared_ptr<IThreadPool> GetThreadPool() const;

    // Task interfaces.
    int ScheduleTask(const TaskAction &task);
    int ScheduleQueuedTask(const std::string &queueTag, const TaskAction &task);
    // Shrink as much memory as possible.
    void ShrinkMemory(const std::string &description);
    void StopTaskPool();
private:
    int ScheduleTaskByThreadPool(const TaskAction &task) const;
    mutable std::shared_mutex threadPoolLock_;
    std::shared_ptr<IThreadPool> threadPool_;

    // Task pool
    int PrepareTaskPool();
    static constexpr int MAX_TP_THREADS = 10;  // max threads of the task pool.
    static constexpr int MIN_TP_THREADS = 1;   // min threads of the task pool.
    std::mutex taskLock_;
    TaskPool *taskPool_;
};
}
#endif // THREAD_POOL_STUB_H