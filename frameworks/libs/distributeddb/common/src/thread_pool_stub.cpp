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
#include <atomic>

#include "db_errno.h"
#include "log_print.h"
#include "thread_pool_stub.h"

namespace DistributedDB {
#ifdef RUNNING_ON_TESTCASE
static std::atomic_uint taskID = 0;
#endif

ThreadPoolStub::ThreadPoolStub()
    : taskPool_(nullptr)
{}

ThreadPoolStub::~ThreadPoolStub() {}

ThreadPoolStub& ThreadPoolStub::GetInstance()
{
    static ThreadPoolStub instThreadPool;
    return instThreadPool;
}

void ThreadPoolStub::SetThreadPool(const std::shared_ptr<IThreadPool> &threadPool)
{
    std::unique_lock<std::shared_mutex> writeLock(threadPoolLock_);
    threadPool_ = threadPool;
    LOGD("[RuntimeContext] Set thread pool finished");
}

std::shared_ptr<IThreadPool> ThreadPoolStub::GetThreadPool() const
{
    std::shared_lock<std::shared_mutex> readLock(threadPoolLock_);
    return threadPool_;
}

int ThreadPoolStub::ScheduleTask(const TaskAction &task, TaskId &taskId)
{
    if (ScheduleTaskByThreadPool(task, taskId) == E_OK) {
        return E_OK;
    }
    std::lock_guard<std::mutex> autoLock(taskLock_);
    int errCode = PrepareTaskPool();
    if (errCode != E_OK) {
        LOGE("Schedule task failed, fail to prepare task pool.");
        return errCode;
    }
#ifdef RUNNING_ON_TESTCASE
    auto id = taskID++;
    LOGI("Schedule task succeed, ID:%u", id);
    return taskPool_->Schedule([task, id] {
        LOGI("Execute task, ID:%u", id);
        task();
    });
#else
    return taskPool_->Schedule(task);
#endif
}

bool ThreadPoolStub::RemoveTask(const TaskId &taskId, bool wait) const
{
    std::shared_ptr<IThreadPool> threadPool = GetThreadPool();
    if (threadPool == nullptr) {
        return false;
    }
    return threadPool->Remove(taskId, wait);
}

int ThreadPoolStub::PrepareTaskPool()
{
    if (taskPool_ != nullptr) {
        return E_OK;
    }

    int errCode = E_OK;
    TaskPool *taskPool = TaskPool::Create(MAX_TP_THREADS, MIN_TP_THREADS, errCode);
    if (taskPool == nullptr) {
        return errCode;
    }

    errCode = taskPool->Start();
    if (errCode != E_OK) {
        taskPool->Release(taskPool);
        return errCode;
    }

    taskPool_ = taskPool;
    return E_OK;
}

int ThreadPoolStub::ScheduleQueuedTask(const std::string &queueTag, const TaskAction &task)
{
    TaskId taskId = 0L;
    if (ScheduleTaskByThreadPool(task, taskId) == E_OK) {
        return E_OK;
    }
    std::lock_guard<std::mutex> autoLock(taskLock_);
    int errCode = PrepareTaskPool();
    if (errCode != E_OK) {
        LOGE("Schedule queued task failed, fail to prepare task pool.");
        return errCode;
    }
#ifdef RUNNING_ON_TESTCASE
    auto id = taskID++;
    LOGI("Schedule queued task succeed, ID:%u", id);
    return taskPool_->Schedule(queueTag, [task, id] {
        LOGI("Execute queued task, ID:%u", id);
        task();
    });
#else
    return taskPool_->Schedule(queueTag, task);
#endif
}

void ThreadPoolStub::ShrinkMemory(const std::string &description)
{
    std::lock_guard<std::mutex> autoLock(taskLock_);
    if (taskPool_ != nullptr) {
        taskPool_->ShrinkMemory(description);
    }
}

void ThreadPoolStub::StopTaskPool()
{
    std::lock_guard<std::mutex> autoLock(taskLock_);
    if (taskPool_ != nullptr) {
        taskPool_->Stop();
        TaskPool::Release(taskPool_);
        taskPool_ = nullptr;
    }
}

int ThreadPoolStub::ScheduleTaskByThreadPool(const TaskAction &task, TaskId &taskId) const
{
    std::shared_ptr<IThreadPool> threadPool = GetThreadPool();
    if (threadPool == nullptr) {
        return -E_NOT_SUPPORT;
    }
    taskId = threadPool->Execute(task);
    return E_OK;
}
} // namespace DistributedDB
