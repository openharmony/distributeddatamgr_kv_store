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
#include <map>
#include <mutex>
#include <queue>
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
    static constexpr Duration TIME_OUT = std::chrono::seconds(2);

    KVThreadPool(size_t maxThread, size_t minThread)
    {
        max_ = maxThread;
        min_ = minThread;
        curNum_ = 0;
        idleNum_ = 0;
        taskId_ = 0;
        busyHead_.running_ = false;
        idleHead_.running_ = false;
        CreateManageThread();
    }
    ~KVThreadPool()
    {
        {
            std::unique_lock<decltype(mtx_)> lock(mtx_);
            isRunning_ = false;
            indexes_.clear();
            taskNodeMap_.clear();
            tasks_.clear();
        }
        manager_.join();
        auto node = busyHead_.next_;
        while (node != &busyHead_) {
            node = node->next_;
            auto deNode = node->prev_;
            Remove(deNode);
            Delete(deNode);
        }
        node = idleHead_.next_;
        while (node != &idleHead_) {
            node = node->next_;
            auto deNode = node->prev_;
            Remove(deNode);
            Delete(deNode);
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
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        delCond_.wait(lock, [this, taskId, wait] {
            return (!wait || taskNodeMap_.find(taskId) == taskNodeMap_.end());
        });
        auto index = indexes_.find(taskId);
        if (index == indexes_.end()) {
            return;
        }
        tasks_.erase(index->second);
        indexes_.erase(index);
    }

    TaskId Reset(TaskId taskId, Duration interval)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        auto index = indexes_.find(taskId);
        if (index == indexes_.end()) {
            return INVALID_ID;
        }

        auto &innerTask = index->second->second;
        if (innerTask.interval != INVALID_INTERVAL) {
            innerTask.interval = interval;
        }

        auto it = tasks_.insert({ std::chrono::steady_clock::now() + interval, std::move(innerTask) });
        tasks_.erase(index->second);
        indexes_[taskId] = it;
        return taskId;
    }

private:
    using Time = std::chrono::steady_clock::time_point;
    using Id = std::thread::id;

    struct MessageQueue {
        void Push(TaskId message)
        {
            std::unique_lock<decltype(queueMutex_)> lock(queueMutex_);
            this->queue.push(message);
        }
        void Pop()
        {
            std::unique_lock<decltype(queueMutex_)> lock(queueMutex_);
            if (queue.size() == 0) {
                return;
            }
            this->queue.pop();
        }
        TaskId Front()
        {
            std::unique_lock<decltype(queueMutex_)> lock(queueMutex_);
            return queue.front();
        }
        bool Empty()
        {
            std::unique_lock<decltype(queueMutex_)> lock(queueMutex_);
            return queue.empty();
        }
        std::mutex queueMutex_;
        std::queue<TaskId> queue;
    };

    struct InnerTask {
        TaskId taskId = INVALID_ID;
        Duration interval = INVALID_INTERVAL;
        uint64_t times = UNLIMITED_TIMES;
        std::function<void()> exec;
        Duration execTime;
    };

    struct Node {
        explicit Node(MessageQueue *queue)
        {
            this->queue_ = queue;
            workingThread_ = std::thread([this] {
                while (running_) {
                    {
                        std::unique_lock<decltype(nodeMtx_)> lock(nodeMtx_);
                        cv.wait(lock, [this] {
                            return (hasTask || !running_);
                        });
                    }
                    if (hasTask) {
                        innerTask.exec();
                    } else {
                        continue;
                    }
                    innerTask.times--;
                    queue_->Push(innerTask.taskId);
                    startIdleTime_ = std::chrono::steady_clock::now();
                    innerTask = InnerTask();
                    hasTask = false;
                }
            });
        }
        ~Node() = default;
        std::mutex nodeMtx_;
        InnerTask innerTask;
        MessageQueue *queue_;
        std::condition_variable cv;
        Time startIdleTime_ = std::chrono::steady_clock::now();
        bool running_ = true;
        bool hasTask = false;
        std::thread workingThread_;
        Node *prev_ = this;
        Node *next_ = this;
    };

    TaskId At(Task task, Time begin, Duration interval = INVALID_INTERVAL, uint64_t times = UNLIMITED_TIMES)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        InnerTask innerTask;
        innerTask.times = times;
        innerTask.taskId = GenTaskId();
        innerTask.interval = interval;
        innerTask.exec = std::move(task);
        auto it = tasks_.insert({ begin, innerTask });
        indexes_[innerTask.taskId] = it;
        condition_.notify_one();
        return innerTask.taskId;
    }

    TaskId GenTaskId()
    {
        auto taskId = ++taskId_;
        if (taskId == INVALID_ID) {
            return ++taskId_;
        }
        return taskId;
    }

    void CreateManageThread()
    {
        {
            manager_ = std::thread([this] {
                Time lastCleanTime = std::chrono::steady_clock::now();
                while (isRunning_) {
                    std::unique_lock<decltype(mtx_)> lock(mtx_);
                    if (!msgQueue_.Empty()) {
                        HandleTaskComplete();
                        continue;
                    }
                    if (tasks_.empty()) {
                        if (lastCleanTime + TIME_OUT < std::chrono::steady_clock::now()) {
                            CleanIdle(lastCleanTime);
                        }
                        continue;
                    }
                    if (tasks_.begin()->first > std::chrono::steady_clock::now()) {
                        auto time = tasks_.begin()->first;
                        if (lastCleanTime + TIME_OUT < time) {
                            CleanIdle(lastCleanTime);
                        }
                        condition_.wait_until(lock, time);
                        continue;
                    }
                    auto it = tasks_.begin();
                    InnerTask curTask = it->second;
                    indexes_.erase(curTask.taskId);
                    tasks_.erase(it);

                    if (idleHead_.next_ == &idleHead_ && curNum_ < max_) {
                        auto node = new (std::nothrow) Node(&msgQueue_);
                        Insert(&idleHead_, node);
                        curNum_++;
                        idleNum_++;
                    }
                    ScheduleTask(curTask);
                    if (curTask.interval != INVALID_INTERVAL && curTask.times > 0) {
                        auto it = tasks_.emplace(std::chrono::steady_clock::now() + curTask.interval, curTask);
                    }
                }
            });
            curNum_++;
        }
    }

    void Remove(Node *node)
    {
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;
    }

    void Insert(Node *prev, Node *node)
    {
        prev->next_->prev_ = node;
        node->next_ = prev->next_;
        prev->next_ = node;
        node->prev_ = prev;
    }

    void Delete(Node *node)
    {
        node->running_ = false;
        node->cv.notify_one();
        node->workingThread_.join();
        delete node;
    }

    void HandleTaskComplete()
    {
        auto node = taskNodeMap_[msgQueue_.Front()];
        if (node == nullptr) {
            return;
        }
        taskNodeMap_.erase(msgQueue_.Front());
        msgQueue_.Pop();
        Remove(node);
        Insert(&idleHead_, node);
        idleNum_++;
        delCond_.notify_one();
    }

    void ScheduleTask(InnerTask curTask)
    {
        auto node = idleHead_.next_;
        std::unique_lock<decltype(node->nodeMtx_)> lock(node->nodeMtx_);
        node->innerTask = curTask;
        node->hasTask = true;
        node->cv.notify_one();
        taskNodeMap_.emplace(curTask.taskId, node);
        Remove(node);
        idleNum_--;
        Insert(&busyHead_, node);
    }

    void CleanIdle(Time &lastCleanTime)
    {
        std::mutex cleanMutex;
        std::unique_lock<decltype(cleanMutex)> lock(cleanMutex);
        auto res = condition_.wait_until(lock, lastCleanTime + TIME_OUT);
        if (res == std::cv_status::no_timeout) {
            return;
        }
        auto node = idleHead_.prev_;
        while (node != &idleHead_) {
            auto curNode = node;
            if (std::chrono::steady_clock::now() - curNode->startIdleTime_ > TIME_OUT && curNum_ > min_) {
                curNode->cv.notify_one();
                Remove(curNode);
                Delete(curNode);
                idleNum_--;
                curNum_--;
            } else {
                break;
            }
            node = node->prev_;
        }
        lastCleanTime = std::chrono::steady_clock::now();
    }

    size_t max_;
    size_t min_;
    std::atomic<size_t> curNum_;
    std::atomic<size_t> idleNum_;
    std::thread manager_;
    std::mutex mtx_;
    bool isRunning_ = true;
    std::condition_variable delCond_;
    std::condition_variable condition_;
    std::multimap<Time, InnerTask> tasks_;
    std::map<TaskId, decltype(tasks_)::iterator> indexes_;
    std::map<TaskId, Node *> taskNodeMap_;
    std::atomic<uint64_t> taskId_;
    MessageQueue msgQueue_;
    Node busyHead_ = Node(&msgQueue_);
    Node idleHead_ = Node(&msgQueue_);
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_THREAD_POOL_H
