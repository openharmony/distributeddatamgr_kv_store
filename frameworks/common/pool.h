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

#ifndef OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_POOL_H
#define OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_POOL_H
#include <functional>
#include <mutex>
namespace OHOS {
template<typename T>
class Pool {
public:
    Pool(uint32_t capability, uint32_t min) : capability_(capability), min_(min) {}

    std::shared_ptr<T> Get(bool isForce = false)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        if (idle_ == nullptr) {
            if (!isForce && current_ >= capability_) {
                return nullptr;
            }
            auto cur = new Node();
            idle_ = cur;
            current_++;
        }
        Node *cur = idle_;
        idle_ = idle_->next;
        if (idle_ != nullptr) {
            idle_->prev = nullptr;
        }
        cur->next = busy_;
        if (busy_ != nullptr) {
            cur->prev = busy_->prev;
            busy_->prev = cur;
        }
        busy_ = cur;
        return cur->data;
    };

    int32_t Release(std::shared_ptr<T> data, bool force = false)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        Node *cur = idle_;
        if (!force && current_ <= min_) {
            return false;
        }
        while (cur != nullptr) {
            if (cur->data == data) {
                if (cur->next != nullptr) {
                    cur->next->prev = cur->prev;
                }
                if (cur->prev != nullptr) {
                    cur->prev->next = cur->next;
                }
                if (idle_ == cur) {
                    idle_ = cur->next;
                }
                current_--;
                delete cur;
                return true;
            } else {
                cur = cur->next;
                continue;
            }
        }
        return false;
    }

    void Idle(std::shared_ptr<T> data)
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        Node *cur = busy_;
        while (cur != nullptr && cur->data != data) {
            cur = cur->next;
        }
        if (cur == nullptr) {
            return;
        }
        if (cur == busy_) {
            busy_ = busy_->next;
        }
        if (cur->next != nullptr) {
            cur->next->prev = cur->prev;
        }
        if (cur->prev != nullptr) {
            cur->prev->next = cur->next;
        }
        cur->prev = nullptr;
        cur->next = idle_;
        if (idle_ != nullptr) {
            idle_->prev = cur;
        }
        idle_ = cur;
    }

    int32_t Clean(std::function<void(std::shared_ptr<T>)> close) noexcept
    {
        auto temp = min_;
        min_ = 0;
        while (busy_ != nullptr) {
            close(busy_->data);
        }
        while (idle_ != nullptr) {
            close(idle_->data);
        }
        min_ = temp;
        return true;
    }

private:
    struct Node {
        Node *prev = nullptr;
        Node *next = nullptr;
        std::shared_ptr<T> data = std::make_shared<T>();
    };

    uint32_t capability_;
    uint32_t min_;
    uint32_t current_ = 0;
    Node *idle_ = nullptr;
    Node *busy_ = nullptr;
    std::mutex mutex_;
};
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_KV_STORE_FRAMEWORKS_COMMON_POOL_H
