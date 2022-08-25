/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_LRU_BUCKET_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_LRU_BUCKET_H

#include <map>
#include <mutex>
#include <list>

namespace OHOS {
template<typename _Key, typename _Tp>
class LRUBucket {
public:
    LRUBucket(size_t capacity)
        : size_(0), capacity_(capacity) {}

    LRUBucket(LRUBucket &&bucket) noexcept = delete;
    LRUBucket(const LRUBucket &bucket) = delete;
    LRUBucket &operator=(LRUBucket &&bucket) noexcept = delete;
    LRUBucket &operator=(const LRUBucket &bucket) = delete;

    ~LRUBucket()
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        while (size_ > 0) {
            PopBack();
        }
    }

    size_t Size() const
    {
        return size_;
    }

    size_t Capacity() const
    {
        return capacity_;
    }

    bool ResetCapacity(size_t capacity)
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        capacity_ = capacity;
        while (capacity_ < size_) {
            PopBack();
        }
        return capacity_ == capacity;
    }

    /**
     * The time complexity is O(log(index size))
     **/
    bool Get(const _Key &key, _Tp &value)
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = indexes_.find(key);
        if (it != indexes_.end()) {
            // move node from the list;
            Remove(it->second);
            // insert node to the head
            Insert(&head_, it->second);
            value = it->second->value_;
            return true;
        }
        return false;
    }

    /**
     * The time complexity is O(log(index size))
     **/
    bool Set(const _Key &key, const _Tp &value)
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (capacity_ == 0) {
            return false;
        }

        auto it = indexes_.find(key);
        if (it != indexes_.end()) {
            Update(it->second, value);
            Remove(it->second);
            Insert(&head_, it->second);
            return true;
        }

        while (capacity_ <= size_) {
            PopBack();
        }

        auto *node = new(std::nothrow) Node(value);
        if (node == nullptr) {
            return false;
        }

        Insert(&head_, node);
        auto pair = indexes_.emplace(key, node);
        node->iterator_ = pair.first;
        return true;
    }

    /**
     * Just update the values, not change the lru
     **/
    bool Update(const _Key &key, const _Tp &value)
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = indexes_.find(key);
        if (it != indexes_.end()) {
            Update(it->second, value);
            return true;
        }
        return false;
    }

    /**
     * The time complexity is O(min(indexes, values))
     * Just update the values, not change the lru chain
     */
    bool Update(const std::map<_Key, _Tp> &values)
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto idx = indexes_.begin();
        auto val = values.begin();
        bool updated = false;
        auto comp = indexes_.key_comp();
        while (idx != indexes_.end() && val != values.end()) {
            if (comp(idx->first, val->first)) {
                ++idx;
                continue;
            }
            if (comp(val->first, idx->first)) {
                ++val;
                continue;
            }
            updated = true;
            Update(idx->second, val->second);
            ++idx;
            ++val;
        }
        return updated;
    }

    /**
     * The time complexity is O(log(index size))
     * */
    bool Delete(const _Key &key)
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = indexes_.find(key);
        if (it != indexes_.end()) {
            Remove(it->second);
            Delete(it->second);
            return true;
        }
        return false;
    }

private:
    struct Node final {
        using iterator = typename std::map<_Key, Node *>::iterator;
        Node(const _Tp &value) : value_(value) {}
        Node() : value_() {}
        ~Node() = default;
        _Tp value_;
        iterator iterator_;
        Node *prev_ = this;
        Node *next_ = this;
    };

    void PopBack()
    {
        auto *node = head_.prev_;
        Remove(node);
        Delete(node);
    }

    void Update(Node *node, const _Tp &value)
    {
        node->value_ = value;
    }

    void Remove(Node *node)
    {
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;
        size_--;
    }

    void Insert(Node *prev, Node *node)
    {
        prev->next_->prev_ = node;
        node->next_ = prev->next_;
        prev->next_ = node;
        node->prev_ = prev;
        size_++;
    }

    void Delete(Node *node)
    {
        indexes_.erase(node->iterator_);
        delete node;
    }

    mutable std::recursive_mutex mutex_;
    std::map<_Key, Node *> indexes_;
    Node head_;
    size_t size_;
    size_t capacity_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_LRU_BUCKET_H
