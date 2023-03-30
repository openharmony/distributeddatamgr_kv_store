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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_PRIORITY_QUEUE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_PRIORITY_QUEUE_H
#include <map>

namespace OHOS {
template<typename _Tsk, typename _Tme, typename _Tid>
class PriorityQueue {
public:
    struct Index {
        using TskIndex = typename std::map<_Tid, _Tsk>::iterator;
        _Tme time;
        TskIndex index;
        bool isValid = true;
        Index(_Tme time, TskIndex index) : time(time), index(index) {}
    };

    struct cmp {
        bool operator()(std::shared_ptr<Index> a, std::shared_ptr<Index> b)
        {
            return a->time > b->time;
        }
    };
    
    PriorityQueue() {}
    ~PriorityQueue() {}

    _Tsk Poll()
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        while (!queue_.empty() && !queue_.top()->isValid) {
            queue_.pop();
        }
        _Tsk res;
        if (!queue_.empty()) {
            auto taskId = queue_.top()->index->second.GetId();
            queue_.pop();
            res = tasks_.at(taskId);
            tasks_.erase(taskId);
        }
        return res;
    }

    _Tsk &Top()
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        while (!queue_.empty() && !queue_.top()->isValid) {
            queue_.pop();
        }
        return queue_.top()->index->second;
    }

    bool Push(_Tsk tsk)
    {
        std::unique_lock<std::mutex> lock(pqMtx_);
        if (!tsk.Valid()) {
            return false;
        }
        auto index = tasks_.emplace(tsk.GetId(), std::move(tsk)).first;
        auto item = std::make_shared<Index>(tsk.time, index);
        queue_.push(item);
        items_.emplace(tsk.GetId(), item);
        return true;
    }

    size_t Size()
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        return tasks_.size();
    }

    bool Empty()
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        return tasks_.size() == 0;
    }

    _Tsk Find(_Tid id)
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        _Tsk res = tasks_[id];
        if (!res.Valid()) {
            tasks_.erase(id);
        }
        return res;
    }

    bool Remove(_Tid id)
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        bool res = true;
        _Tsk task = tasks_[id];
        if (!task.Valid()) {
            res = false;
        } else {
            items_.at(id)->isValid = false;
            items_.erase(id);
        }
        tasks_.erase(id);
        return res;
    }

private:
    std::mutex pqMtx_;
    std::map<_Tid, _Tsk> tasks_;
    std::map<_Tid, std::shared_ptr<Index>> items_;
    std::priority_queue<std::shared_ptr<Index>, std::vector<std::shared_ptr<Index>>, cmp> queue_;
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_PRIORITY_QUEUE_H
