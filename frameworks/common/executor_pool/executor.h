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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_EXECUTOR_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_EXECUTOR_H

#include <condition_variable>
#include <thread>

#include "inner_task.h"
#include "node_manager.h"
#include "priority_queue.h"
namespace OHOS {
class Executor {
public:
    using Duration = std::chrono::steady_clock::duration;

    static constexpr Duration TIME_OUT = std::chrono::seconds(2);

    Executor(bool isForcedTimer = false, Task task = Task()) : isForcedTimer_(isForcedTimer), exec_(task)
    {
        thread_ = std::thread([this] {
            {
                std::unique_lock<decltype(mutex_)> lock(mutex_);
                cv_.wait(lock);
            }
            while (isWorking_) {
                if (exec_) {
                    exec_();
                }
                Release();
            }
        });
    }
    ~Executor(){
        isWorking_ = false;
        thread_.join();
    }
    void Release()
    {
        if (isForcedTimer_) {
            isTimer_ = false;
            isWorking_ = false;
            return;
        }
        nodeManager_->Release(outerNode_);
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        auto waitRes = cv_.wait_until(lock, std::chrono::steady_clock::now() + TIME_OUT);
        if (waitRes == std::cv_status::timeout) {
            nodeManager_->Remove(outerNode_);
            isTimer_ = false;
            isWorking_ = false;
        }
    }
    void SetExec(Task task)
    {
        this->exec_ = task;
    }

    bool Valid()
    {
        return isWorking_;
    }
    Task exec_;
    std::thread thread_;
    bool isForcedTimer_;
    bool isWorking_ = true;
    bool isTimer_;
    std::shared_ptr<NodeManager<Executor>> nodeManager_;
    std::mutex mutex_;
    std::condition_variable cv_;
    Node<Executor> *outerNode_;
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_EXECUTOR_H
