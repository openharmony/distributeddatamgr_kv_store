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
    struct Index {
        using TskIndex = typename std::map<_Tid, std::pair<_Tsk, Index *>>::iterator;
        _Tme time;
        TskIndex index;
        bool isValid = true;
        bool isSchedule = false;

        Index(_Tme time) : time(time) {}
        void SetIndex(TskIndex tskIndex)
        {
            index = tskIndex;
        }
        bool operator<(const Index &other)
        {
            return time < other.time;
        }
        bool operator==(const Index &other)
        {
            return index->second.first.GetId() == other.index->second.first.GetId();
        }
    };
    struct cmp {
        bool operator()(Index *a, Index *b)
        {
            return a->time > b->time;
        }
    };

    PriorityQueue() {}
    ~PriorityQueue() {}

    _Tsk Pop()
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        _Tsk res;
        while (!queue_.empty()) {
            while (!queue_.empty() && queue_.top() != nullptr && !queue_.top()->isValid) {
                queue_.pop();
            }
            if (!queue_.empty()) {
                if (queue_.top()->index->second.first.startTime > std::chrono::steady_clock::now()) {
                    popCv_.wait_until(lock, queue_.top()->index->second.first.startTime);
                    continue;
                }
                auto index = queue_.top();
                res = index->index->second.first;
                queue_.pop();
                running.insert(index);
                break;
            }
        }
        return res;
    }

    bool Push(_Tsk tsk)
    {
        std::unique_lock<std::mutex> lock(pqMtx_);
        if (!tsk.Valid()) {
            return false;
        }
        auto item = new (std::nothrow) Index(tsk.startTime);
        auto index = tasks_.emplace(tsk.GetId(), std::pair{ tsk, item });
        if (!index.second) {
            return false;
        }
        item->SetIndex(index.first);
        queue_.push(item);
        popCv_.notify_all();
        return true;
    }

    bool Update(_Tid id, _Tme tme)
    {
        std::unique_lock<std::mutex> lock(pqMtx_);
        if (tasks_.find(id) == tasks_.end()) {
            return false;
        }
        auto runIdx = running.find(tasks_[id].second);
        if (runIdx != running.end()) {
            if (!(*runIdx)->isValid) {
                return false;
            }
            (*runIdx)->isSchedule = true;
        }
        tasks_[id].first.startTime = tme;
        --tasks_[id].first.times;
        tasks_[id].second->time = tme;
        queue_.push(tasks_[id].second);
        popCv_.notify_all();
        return true;
    }

    size_t Size()
    {
        std::lock_guard<std::mutex> lock(pqMtx_);
        return tasks_.size() - running.size();
    }

    _Tsk Find(_Tid id)
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        _Tsk res;
        if (tasks_.find(id) != tasks_.end()) {
            res = tasks_[id].first;
        }
        return res;
    }

    bool Remove(_Tid id, bool wait)
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        bool res = true;
        auto it = running.find(tasks_[id].second);
        if (it != running.end()) {
            res = false;
            removeCv_.wait(lock, [this, id, wait] {
                return !wait || running.find(tasks_[id].second) == running.end();
            });
        }
        if (tasks_.find(id) == tasks_.end()) {
            return false;
        }
        tasks_[id].second->isValid = false;
        tasks_.erase(id);
        popCv_.notify_all();
        return res;
    }

    void Clean()
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        auto it = tasks_.begin();
        while (it != tasks_.end()) {
            delete (*it).second.second;
            tasks_.erase(it);
            it++;
        }
        running.clear();
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

    void Finish(_Tid id)
    {
        std::unique_lock<decltype(pqMtx_)> lock(pqMtx_);
        if (running.empty() && tasks_.empty()) {
            return;
        }
        auto pair = tasks_[id];
        if (running.find(pair.second) != running.end()) {
            if (!pair.second->isSchedule) {
                delete pair.second;
                tasks_.erase(id);
            } else {
                pair.second->isSchedule = false;
            }
            running.erase(pair.second);
        }

        removeCv_.notify_all();
    }

private:
    std::mutex pqMtx_;
    std::condition_variable popCv_;
    std::condition_variable removeCv_;
    std::map<_Tid, std::pair<_Tsk, Index *>> tasks_;
    std::set<Index *> running;
    std::priority_queue<Index *, std::vector<Index *>, cmp> queue_;
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_PRIORITY_QUEUE_H
