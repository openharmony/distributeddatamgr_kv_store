/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef TASK_POOL_IMPL_H
#define TASK_POOL_IMPL_H

#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "task_pool.h"
#include "task_queue.h"

namespace DistributedDB {
class TaskPoolImpl : public TaskPool {
public:
    // maxThreads > 0.
    TaskPoolImpl(int maxThreads, int minThreads);

    // Start the task pool.
    int Start() override;

    // Stop the task pool.
    void Stop() override;

    // Schedule a task, the task can be ran in any thread.
    int Schedule(const Task &task) override;

    // Schedule tasks one by one.
    int Schedule(const std::string &queueTag, const Task &task) override;

    // Shrink memory associated with the given tag if possible.
    void ShrinkMemory(const std::string &tag) override;

protected:
    ~TaskPoolImpl();

private:
    int SpawnThreads(bool isStart);
    bool IdleExit(std::unique_lock<std::mutex> &lock);
    void SetThreadFree();
    Task ReapTask(TaskQueue *&queue);
    void GetTask(Task &task, TaskQueue *&queue);
    bool IsGenericWorker() const;
    void BecomeGenericWorker();
    void ExitWorker();
    void TaskWorker();
    void FinishExecuteTask(TaskQueue *taskQueue);
    void TryToSpawnThreads();
    bool IsExecutingTasksEmpty() const;

    // Member Variables.
    static constexpr int IDLE_WAIT_PERIOD = 1;  // wait 1 second before exiting.
    std::mutex tasksMutex_;
    std::condition_variable hasTasks_;
    std::map<std::string, TaskQueue> queuedTasks_;
    TaskQueue genericTasks_;
    std::thread::id genericThread_;  // execute generic task only.
    int genericTaskCount_;
    int queuedTaskCount_;
    bool isStarted_;
    bool isStopping_;   // Stop() invoked.
    std::condition_variable allThreadsExited_;

    // Thread counter.
    int maxThreads_;
    int minThreads_;
    int curThreads_;
    int idleThreads_;
    int exitingThreads_;
};
} // namespace DistributedDB

#endif // TASK_POOL_IMPL_H
