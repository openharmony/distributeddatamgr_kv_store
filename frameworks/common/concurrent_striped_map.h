/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_CONCURRENT_STRIPED_MAP_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_CONCURRENT_STRIPED_MAP_H
#include <functional>
#include <map>
#include <memory>
#include <mutex>
namespace OHOS {
template<typename _Key, typename _Tp>
class ConcurrentStripedMap {
public:
    using key_type = typename std::map<_Key, _Tp>::key_type;
    using mapped_type = typename std::map<_Key, _Tp>::mapped_type;
    using value_type = typename std::map<_Key, _Tp>::value_type;
    using size_type = typename std::map<_Key, _Tp>::size_type;
    using reference = typename std::map<_Key, _Tp>::reference;
    using action_type = typename std::function<bool(const key_type &, mapped_type &)>;
    ConcurrentStripedMap() = default;
    ~ConcurrentStripedMap() {}

    ConcurrentStripedMap(const ConcurrentStripedMap &other) = delete;
    ConcurrentStripedMap &operator=(const ConcurrentStripedMap &other) noexcept = delete;
    ConcurrentStripedMap(ConcurrentStripedMap &&other) noexcept = delete;
    ConcurrentStripedMap &operator=(ConcurrentStripedMap &&other) noexcept = delete;

    bool Empty() const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_.empty();
    }

    size_type Size() const noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return entries_.size();
    }

    // The action`s return true means meeting the erase condition
    // The action`s return false means not meeting the erase condition
    size_type EraseIf(const std::function<bool(const key_type &key, mapped_type &value)> &action) noexcept
    {
        if (action == nullptr) {
            return 0;
        }
        auto entries = Clone();
        size_type count = 0;
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            if (it->second == nullptr) {
                continue;
            }
            bool reserved = it->second->DoAction([it, &action](mapped_type &value, bool isValid) {
                if (!isValid) {
                    return false;
                }
                return !action(it->first, value);
            });
            if (!reserved) {
                count++;
            }
        }
        return count;
    }

    // The action's return value means that the element is keep in map or not; true means keeping, false means removing.
    bool Compute(const key_type &key, const action_type &action)
    {
        if (action == nullptr) {
            return false;
        }
        std::shared_ptr<Node> node = GetNode(key, true);
        // 冗余判断，实际不会为空
        if (node == nullptr) {
            return false;
        }
        node->DoAction([&action, &key](mapped_type &value, bool isValid) {
            return action(key, value);
        });
        return true;
    }

    // The action's return value means that the element is keep in map or not; true means keeping, false means removing.
    bool ComputeIfPresent(const key_type &key, const std::function<bool(const key_type &, mapped_type &)> &action)
    {
        if (action == nullptr) {
            return false;
        }
        std::shared_ptr<Node> node = GetNode(key, false);
        if (node == nullptr) {
            return false;
        }
        return node->DoAction([&action, &key](mapped_type &value, bool isValid) {
            if (!isValid) {
                return false;
            }
            return action(key, value);
        });
    }

    bool DoAction(const std::function<bool(void)> &action)
    {
        if (action == nullptr) {
            return false;
        }
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        return action();
    }

private:
    struct Node {
    public:
        Node(const _Tp &value) : value_(value) {}
        Node(_Tp &&value) : value_(std::move(value)) {}
        bool DoAction(std::function<bool(mapped_type &value, bool isValid)> action)
        {
            // 冗余判断，实际不会为空
            if (action == nullptr) {
                return false;
            }
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            isValid_ = action(value_, isValid_);
            // 如果不需要保留，则清空数据
            if (!isValid_) {
                value_ = _Tp();
            }
            return isValid_;
        }

        bool IsValid() const noexcept
        {
            return isValid_;
        }

    private:
        mutable std::recursive_mutex mutex_;
        _Tp value_;
        bool isValid_ = false;
    };

    std::shared_ptr<Node> Convert2AutoNode(std::shared_ptr<Node> node, const _Key &key)
    {
        // 这个node只会在map的方法中使用，所以可以捕获this。TODD:减少key的拷贝
        return std::shared_ptr<Node>(node.get(), [holder = node, this, key](auto *p) {
            if (holder->IsValid() || holder.use_count() > 2) {
                return;
            }
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            if (holder->IsValid() || holder.use_count() > 2) {
                return;
            }
            entries_.erase(key);
            return;
        });
    }

    std::shared_ptr<Node> GetNode(const _Key &key, bool needCreate = true) noexcept
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto it = entries_.find(key);
        if (it != entries_.end() && it->second != nullptr) {
            return Convert2AutoNode(it->second, key);
        }
        if (!needCreate) {
            return nullptr;
        }
        auto node = std::make_shared<Node>(_Tp());
        entries_.insert(typename std::map<_Key, std::shared_ptr<Node>>::value_type(key, node));
        return Convert2AutoNode(node, key);
    }

    std::map<_Key, std::shared_ptr<Node>> Clone() noexcept
    {
        std::map<_Key, std::shared_ptr<Node>> entries;
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        for (const auto &[key, node] : entries_) {
            if (node == nullptr) {
                continue;
            }
            entries.emplace(key, Convert2AutoNode(node, key));
        }
        return entries;
    }

private:
    mutable std::recursive_mutex mutex_;
    std::map<_Key, std::shared_ptr<Node>> entries_;
};
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_CONCURRENT_STRIPED_MAP_H