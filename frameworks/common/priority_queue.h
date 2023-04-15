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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_PRIORITY_QUEUE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_PRIORITY_QUEUE_H
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <shared_mutex>
namespace OHOS {
template<typename _Tsk, typename _Tme, typename _Tid>
class PriorityQueue {
public:
    struct PQMatrix {
        _Tsk task_;
        _Tid id_;
        PQMatrix(_Tsk task, _Tid id) : task_(task), id_(id) {}
    };
    using TskIndex = typename std::map<_Tme, PQMatrix>::iterator;

    PriorityQueue(const _Tsk &task) : INVALID_TSK(std::move(task)) {}
    _Tsk Pop()
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        while (!tasks_.empty()) {
            if (tasks_.begin()->first > std::chrono::steady_clock::now()) {
                popCv_.wait_until(lock, tasks_.begin()->first);
                continue;
            }
            auto temp = tasks_.begin();
            auto id = temp->second.id_;
            running.emplace(id);
            auto res = std::move(temp->second.task_);
            tasks_.erase(temp);
            indexes_.erase(id);
            return res;
        }
        return INVALID_TSK;
    }

    bool Push(_Tsk tsk, _Tid id, _Tme tme)
    {
        std::unique_lock<std::mutex> lock(pqMtx_);
        if (!tsk.Valid()) {
            return false;
        }
        auto temp = tasks_.emplace(tme, PQMatrix(std::move(tsk), id));
        indexes_.emplace(id, temp);
        popCv_.notify_all();
        return true;
    }

    size_t Size()
    {
        std::lock_guard<std::mutex> lock(pqMtx_);
        return tasks_.size();
    }

    _Tsk Find(_Tid id)
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        if (indexes_.find(id) != indexes_.end()) {
            return indexes_[id]->second.task_;
        }
        return INVALID_TSK;
    }

    bool Remove(_Tid id, bool wait)
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        removeCv_.wait(lock, [this, id, wait] {
            return !wait || running.find(id) == running.end();
        });
        auto index = indexes_.find(id);
        if (index == indexes_.end()) {
            return false;
        }
        tasks_.erase(index->second);
        indexes_.erase(index);
        popCv_.notify_all();
        return true;
    }

    void Clean()
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        indexes_.clear();
        tasks_.clear();
    }

    void Finish(_Tid id)
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        running.erase(id);
        removeCv_.notify_all();
    }

private:
    const _Tsk INVALID_TSK;
    std::mutex pqMtx_;
    std::condition_variable popCv_;
    std::condition_variable removeCv_;
    std::multimap<_Tme, PQMatrix> tasks_;
    std::set<_Tid> running;
    std::map<_Tid, TskIndex> indexes_;
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_PRIORITY_QUEUE_H
