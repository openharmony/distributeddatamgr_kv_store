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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_EXECUTOR_POOL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_EXECUTOR_POOL_H
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "executor.h"
#include "inner_task.h"
#include "node_manager.h"
#include "priority_queue.h"

namespace OHOS {
class ExecutorPool {
public:
    using TaskId = uint64_t;
    using Duration = std::chrono::steady_clock::duration;
    using Task = std::function<void()>;
    using TaskQueue = PriorityQueue<InnerTask, Time, TaskId>;

    static constexpr TaskId INVALID_TASKID = static_cast<uint64_t>(0l);
    static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
    static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();
    static constexpr Time INVALID_TIME = std::chrono::time_point<std::chrono::steady_clock, std::chrono::seconds>();

    ExecutorPool(size_t maxThread, size_t minThread)
    {
        taskId_ = 0;
        threadManager_ = std::make_shared<NodeManager<Executor>>(maxThread, minThread);
        timerTasks_ = std::make_shared<TaskQueue>();
        execTasks_ = std::make_shared<TaskQueue>();
        timerThread_ = threadManager_->idleHead_->inner_;
    }
    ~ExecutorPool() {}

    TaskId Execute(Task task)
    {
        return At(std::move(task), std::chrono::steady_clock::now());
    }

    TaskId Execute(Task task, Duration delay)
    {
        return At(std::move(task), INVALID_TIME + delay);
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

    bool Remove(TaskId taskId, bool wait = false)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        if (timerTasks_->Remove(taskId)) {
            return true;
        }
        delCondition_.wait(lock, [this, taskId, wait] {
            return !wait || !execTasks_->Find(taskId).Valid();
        });
        return false;
    }

    TaskId Reset(TaskId taskId, Duration interval)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        auto it = timerTasks_->Find(taskId);
        if (!it.Valid()) {
            return INVALID_TASKID;
        }
        if (timerTasks_->Remove(taskId)) {
            return INVALID_TASKID;
        }
        it.interval = interval;
        it.startTime = std::chrono::steady_clock::now() + interval;
        if (timerTasks_->Push(it)) {
            return INVALID_TASKID;
        }
        CallTimerThread();
        return taskId;
    }

private:
    using Time = std::chrono::steady_clock::time_point;
    using ThreadId = uint64_t;

    TaskId At(Task task, Time begin, Duration interval = INVALID_INTERVAL, uint64_t times = UNLIMITED_TIMES)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        InnerTask innerTask;
        innerTask.times = times;
        innerTask.taskId = GenTaskId();
        innerTask.interval = interval;
        innerTask.exec = std::move(task);
        innerTask.startTime = begin;
        if (begin > std::chrono::steady_clock::now()) {
            timerTasks_->Push(innerTask);
        } else {
            innerTask.startTime = INVALID_TIME;
            execTasks_->Push(innerTask);
        }
        CallTimerThread();
        return innerTask.taskId;
    }
    
    void CallTimerThread()
    {
        if (timerThread_ != nullptr && timerThread_->isTimer_) {
            timerThread_->cv_.notify_one();
            timerCondition_.notify_one();
            return;
        }
        auto node = threadManager_->GetNode();
        Executor *thread;
        if (node == nullptr) {
            thread = new Executor(true, timerTask);
        } else {
            thread = node->inner_;
            thread->outerNode_ = node;
        }
        thread->isTimer_ = true;
        thread->exec_ = timerTask;
        thread->nodeManager_ = threadManager_;
        timerThread_ = thread;
        thread->cv_.notify_one();
        timerCondition_.notify_all();
    }

    bool NeedToAdd()
    {
        if (execTasks_->Empty()) {
            return false;
        }
        if (threadManager_->busyHead_->next_->next_ == threadManager_->busyHead_->next_) {
            return true;
        }
        if (threadManager_->idleNum_ != 0) {
            return true;
        }
        if (threadManager_->idleNum_ == 0 && threadManager_->size_ < threadManager_->max_) {
            return true;
        }
        return false;
    }

    void SetNode(Node<Executor> *node)
    {
        node->inner_->nodeManager_ = threadManager_;
        node->inner_->outerNode_ = node;
    }

    TaskId GenTaskId()
    {
        auto taskId = ++taskId_;
        if (taskId == INVALID_TASKID) {
            return ++taskId_;
        }
        return taskId;
    }

    std::mutex mtx_;
    std::condition_variable delCondition_;
    std::condition_variable timerCondition_;
    Executor *timerThread_;
    std::shared_ptr<TaskQueue> timerTasks_;
    std::shared_ptr<TaskQueue> execTasks_;
    std::shared_ptr<NodeManager<Executor>> threadManager_;
    std::atomic<uint64_t> taskId_;
    Task timerTask = [this]() {
        while (true) {
            if (timerTasks_->Empty() && execTasks_->Empty()) {
                break;
            }
            std::unique_lock<decltype(mtx_)> lock(mtx_);
            if (NeedToAdd()) {
                auto node = threadManager_->GetNode();
                SetNode(node);
                node->inner_->SetExec(execTask);
                node->inner_->cv_.notify_one();
            }
            if (!timerTasks_->Empty()) {
                if (timerTasks_->Top().startTime > std::chrono::steady_clock::now()) {
                    timerCondition_.wait_until(lock, timerTasks_->Top().startTime);
                    continue;
                }
                auto innerTask = timerTasks_->Poll();

                execTasks_->Push(innerTask);
                if (innerTask.interval != INVALID_INTERVAL && --innerTask.times > 0) {
                    innerTask.startTime = std::chrono::steady_clock::now() + innerTask.interval;
                    timerTasks_->Push(innerTask);
                }
            }
        }
    };
    Task execTask = [this] {
        while (true) {
            InnerTask innerTask_;
            {
                std::unique_lock<decltype(mtx_)> lock(mtx_);
                if (execTasks_->Empty()) {
                    break;
                }
                innerTask_ = execTasks_->Poll();
            }
            innerTask_.exec();
            delCondition_.notify_one();
        }
    };
};

} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_EXECUTOR_POOL_H
