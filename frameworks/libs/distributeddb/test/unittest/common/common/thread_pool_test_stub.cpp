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
#include "thread_pool_test_stub.h"

#include "db_errno.h"
#include "log_print.h"
namespace DistributedDB {
const TaskId INVALID_ID = 0u;

ThreadPoolTestStub::~ThreadPoolTestStub()
{
    LOGI("ThreadPoolTestStub Exist");
    std::lock_guard<std::mutex> autoLock(taskPoolLock_);
    if (taskPool_ != nullptr) {
        taskPool_->Stop();
        TaskPool::Release(taskPool_);
    }
}

TaskId ThreadPoolTestStub::Execute(const Task &task)
{
    StartThreadIfNeed();
    std::lock_guard<std::mutex> autoLock(taskPoolLock_);
    if (taskPool_ == nullptr) {
        return INVALID_ID;
    }
    static std::atomic<uint64_t> taskActionID = 0;
    auto id = taskActionID++;
    LOGD("Schedule task succeed, ID:%" PRIu64, id);
    (void)taskPool_->Schedule([task, id]() {
        LOGD("Execute task, ID:%" PRIu64, id);
        task();
    });
    return id;
}

void ThreadPoolTestStub::StartThreadIfNeed()
{
    std::lock_guard<std::mutex> autoLock(taskPoolLock_);
    if (taskPool_ != nullptr) {
        return;
    }
    int errCode = E_OK;
    taskPool_ = TaskPool::Create(10, 1, errCode); // 10 is max thread, 1 is min thread
    if (errCode != E_OK) {
        LOGW("task pool create failed %d", errCode);
        return;
    }
    errCode = taskPool_->Start();
    if (errCode != E_OK) {
        LOGW("task pool start failed %d", errCode);
        TaskPool::Release(taskPool_);
    }
}

TaskId ThreadPoolTestStub::Execute(const Task &task, Duration delay)
{
    return Execute([task, delay]() {
        std::this_thread::sleep_for(delay);
        task();
    });
}

TaskId ThreadPoolTestStub::Schedule(const Task &task, Duration interval)
{
    return INVALID_ID;
}

TaskId ThreadPoolTestStub::Schedule(const Task &task, Duration delay, Duration interval)
{
    return INVALID_ID;
}

TaskId ThreadPoolTestStub::Schedule(const Task &task, Duration delay, Duration interval, uint64_t times)
{
    return INVALID_ID;
}

bool ThreadPoolTestStub::Remove(const TaskId &taskId, bool wait)
{
    return true;
}

TaskId ThreadPoolTestStub::Reset(const TaskId &taskId, Duration interval)
{
    return INVALID_ID;
}
}
