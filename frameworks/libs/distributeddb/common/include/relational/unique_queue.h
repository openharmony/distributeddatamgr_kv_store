/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef UNIQUE_QUEUE_H
#define UNIQUE_QUEUE_H
#include <vector>
#include <unordered_map>
#include "store_types.h"
#include "db_types.h"
#include "db_errno.h"
#include "log_print.h"

#ifdef RELATIONAL_STORE

namespace DistributedDB {
using namespace std;

template <typename UqData, typename UqHash = std::hash<UqData>, typename UqEqualTo = std::equal_to<UqData>>
class UniqueQueue {
public:
    UniqueQueue()
    {
        data_.resize(capacity_);
    }

    ~UniqueQueue()
    {}

    static constexpr size_t INIT_CAP = 1024;     // default capacity
    static constexpr size_t EXTEND_STEP = 1024;  // expansion step size
    static constexpr size_t MAX_CAP = 10240;     // maximum capacity
    void InitByFront(size_t frontIn = 0)
    {
        front_ = frontIn % capacity_;
        rear_ = front_;
        read_ = front_;
        filter_.clear();
    }

    int Init(uint64_t newCap = INIT_CAP, size_t frontIn = 0)
    {
        // create new array
        data_.resize(newCap);
        capacity_ = newCap;
        InitByFront(frontIn);
        return E_OK;
    }

    // expand first, then batch insert, ensuring all-or-nothing for the batch
    int PushBatch(const std::vector<UqData> &dataIn, size_t num)
    {
        if (num == 0) {
            return E_OK;
        }
        int ret = E_OK;
        ret = ExpandIfNeed(num);
        if (ret != E_OK) {
            return ret;
        }

        for (size_t i = 0; i < num; i++) {
            Push(dataIn[i]);
        }
        return E_OK;
    }

    size_t ReadBatch(UqData *dataOut, size_t maxNum)
    {
        size_t readNum = std::min(RemainReadSize(), maxNum);
        for (size_t i = 0; i < readNum; ++i) {
            dataOut[i] = data_[(read_ + i) % capacity_];
        }
        AdvanceRead(readNum);
        return readNum;
    }

    int TryInitCursor(size_t readIn)
    {
        if (IndexHasRead(readIn)) {
            LOGI("read cache repeat, [%d, %d), cur %d.", front_, read_, readIn);
            read_ = readIn;
            return E_OK;
        }

        if (readIn == front_) {
        } else if (readIn == read_) {
            LOGW("read without clear read cache %d.", read_);
        } else if (QueueSize() == 0) {
            LOGI("Queue is empty, re-init");
            InitByFront(readIn);
        } else {
            LOGE("invalid read start %d, read cache %d. %d", readIn, read_, -E_INVALID_ARGS);
            return -E_INVALID_ARGS;
        }
        return E_OK;
    }

    bool IsFull() const
    {
        return (capacity_ == MAX_CAP && ((rear_ + 1) % capacity_) == front_);
    }

    bool IsEmpty() const
    {
        return rear_ == front_;
    }

    size_t Capacity() const
    {
        return capacity_;
    }

    size_t QueueSize() const
    {
        return (capacity_ + rear_ - front_) % capacity_;
    }

    size_t RemainReadSize() const
    {
        return (capacity_ + rear_ - read_) % capacity_;
    }

    size_t ReadCacheSize() const
    {
        return (capacity_ + read_ - front_) % capacity_;
    }

    UqData* AdvanceFront(size_t num)
    {
        if (IsEmpty()) {
            return nullptr;
        }

        size_t newFront = (front_ + num) % capacity_;
        if (newFront == read_ || IndexHasRead(newFront)) {
            front_ = newFront;
        } else {
            front_ = read_;
            LOGW("new front %d out of range, read %d, rear %d, cap %d, force set front %d.",
                newFront, read_, rear_, capacity_, front_);
        }
        return &data_[(front_ - 1 + capacity_) % capacity_];
    }

private:
    struct FilterNode {
        uint64_t loop;
        size_t index;
    };

    void AdvanceRear(size_t num)
    {
        rear_ = (rear_ + num) % capacity_;
        if (rear_ != 0) {
            return;
        }
        // queue wrapped around, update wrap count and filter info
        loop_++;
        for (auto it = filter_.begin(); it != filter_.end();) {
            if (it->second.loop + 1 < loop_ || !IndexInQueue(it->second.index)) {
                it = filter_.erase(it);
            } else {
                it++;
            }
        }
    }

    void AdvanceRead(size_t num)
    {
        read_ = (read_ + num) % capacity_;
    }

    void ClearReadSize()
    {
        read_ = front_;
    }

    bool IndexHasRead(size_t index) const
    {
        if (front_ <= read_) {
            return (front_ <= index) && (index < read_);
        } else {
            return (front_ <= index) || (index < read_);
        }
    }

    bool IndexInQueue(size_t index) const
    {
        if (front_ <= rear_) {
            return (front_ <= index) && (index < rear_);
        } else {
            return (front_ <= index) || (index < rear_);
        }
    }

    int PushNew(const UqData &item)
    {
        auto i = rear_;
        FilterNode node = {loop_, i};
        data_[i] = item;
        AdvanceRear(1);
        filter_.insert({data_[i], node});
        return E_OK;
    }

    void UpdateRemainRead(size_t newIdx, const FilterNode &filterNode)
    {
        auto itRange = filter_.equal_range(data_[newIdx]);
        for (auto it = itRange.first; it != itRange.second; it++) {
            if (it->second.loop + 1 < loop_ || !IndexInQueue(it->second.index) || IndexHasRead(filterNode.index)) {
                continue;
            }
            it->second.index = newIdx;
        }
    }

    int Push(const UqData &item)
    {
        // insert when oldKey not exist
        auto oldKeyRange = filter_.equal_range(item);
        if (oldKeyRange.first == oldKeyRange.second) {
            return PushNew(item);
        }
        auto oldKey = oldKeyRange.first;
        for (; oldKey != oldKeyRange.second; oldKey++) {
            if (oldKey->second.loop + 1 < loop_ || !IndexInQueue(oldKey->second.index) ||
                IndexHasRead(oldKey->second.index)) {
                continue;
            }
            // oldKey not read, need update
            size_t i = oldKey->second.index % capacity_;
            data_[i] = item;
            // move updated key to the end
            for (; (i + 1) % capacity_ != rear_; i = (i + 1) % capacity_) {
                std::swap(data_[i], data_[(i + 1) % capacity_]);
                UpdateRemainRead(i, oldKey->second);
                UpdateRemainRead((i + 1) % capacity_, oldKey->second);
            }
            return E_OK;
        }

        return PushNew(item);
    }

    int Expand()
    {
        if (capacity_ >= MAX_CAP) {
            LOGE("UniqueQueue capacity reach limit.");
            return -E_MAX_LIMITS;
        }
        // preserve existing queue elements and their count, as well as read cache count
        size_t dataNum = QueueSize();
        size_t readNum = ReadCacheSize();
        size_t oldFront = front_;
        size_t oldCap = capacity_;
        std::vector<UqData> oldData = std::move(data_);
        size_t newCap = std::min(MAX_CAP, oldCap + EXTEND_STEP);
        int ret = Init(newCap, oldFront);
        if (ret != 0) {
            data_ = std::move(oldData);
            return ret;
        }

        for (size_t i = 0; i < dataNum; i++) {
            Push(oldData[(oldFront + i) % oldCap]);  // queue capacity is sufficient, won't re-enter Expand() or fail
            if (i < readNum) {
                AdvanceRead(1); // to keep deduplication logic consistent with before
            }
        }
        return E_OK;
    }

    int ExpandIfNeed(size_t num)
    {
        int ret = E_OK;
        while (num + QueueSize() + 1 > capacity_) {  // one slot in capacity is unusable
            ret = Expand();
            if (ret != E_OK) {
                LOGE("Expand capacity add %d failed. %d", num, ret);
                return ret;
            }
        }
        return ret;
    }
    size_t capacity_ = INIT_CAP;  // current capacity
    size_t front_ = 0; // queue front (next position to dequeue)
    size_t rear_ = 0; // queue rear (next position to enqueue)
    size_t read_ = 0; // already read (data from front to read is previously read-out data, temporarily cached)
    // queue wrap-around count, inherits previous loop on expansion;
    // edge case: loop incremented but no wrap after expansion, >2 covers this
    uint64_t loop_ = 0;

    std::vector<UqData> data_;
    std::unordered_multimap<UqData, FilterNode, UqHash, UqEqualTo> filter_;
};

}  // namespace DistributedDB
#endif  // RELATIONAL_STORE
#endif  // UNIQUE_QUEUE_H