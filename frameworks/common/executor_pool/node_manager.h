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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_NODE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_NODE_H
#include <atomic>
#include <mutex>

namespace OHOS {
template<typename T>
class Node {
public:
    T *inner_;
    Node *prev_ = this;
    Node *next_ = this;
    Node()
    {
        inner_ = new T();
    }
    explicit Node(bool isHead) {}
    bool Valid()
    {
        return inner_->Valid();
    }
};

template<typename T>
class NodeManager {
public:
    NodeManager(size_t max, size_t min) : max_(max), min_(min)
    {
        busyHead_ = new Node<T>(true);
        idleHead_ = new Node<T>(true);
    }

    Node<T> *GetNode()
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        Node<T> *node;
        if (idleHead_->next_ != idleHead_) {
            node = idleHead_->next_;
            idleHead_->next_->next_->prev_ = idleHead_;
            idleHead_->next_ = idleHead_->next_->next_;
            idleNum_--;
        } else if (size_ < max_) {
            node = new Node<T>;
            size_++;
        }
        AddToBusy(node);
        return node;
    }

    void Release(Node<T> *node)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        if (busyHead_->next_ == node) {
            busyHead_->next_ = node->next_;
        }
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;
        AddToIdle(node);
    }

    void AddToIdle(Node<T> *node)
    {
        node->prev_ = idleHead_;
        node->next_ = idleHead_->next_;
        idleHead_->next_->prev_ = node;
        idleHead_->next_ = node;
        idleNum_++;
    }

    void AddToBusy(Node<T> *node)
    {
        if (busyHead_->next_ != busyHead_) {
            node->next_ = busyHead_->next_;
            node->prev_ = busyHead_->next_->prev_;
            busyHead_->next_->prev_->next_ = node;
            busyHead_->next_->prev_ = node;
        }
        busyHead_->next_ = node;
    }

    void Remove(Node<T> *node)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        if (size_ <= min_) {
            return;
        }
        node->next_->prev_ = node->prev_;
        node->prev_->next_ = node->next_;
        idleNum_--;
        size_--;
        delete node;
    }

    size_t size_ = 0;
    std::atomic<size_t> idleNum_ = 0;
    size_t max_;
    size_t min_;
    std::mutex mtx_;
    Node<T> *busyHead_;
    Node<T> *idleHead_;
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_NODE_H
