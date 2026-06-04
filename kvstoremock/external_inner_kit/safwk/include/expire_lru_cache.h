/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef EXPIRE_LRU_CACHE_H
#define EXPIRE_LRU_CACHE_H

#include <map>
#include <mutex>
#include <memory>
#include <list>
#include <algorithm>
#include "datetime_ex.h"

namespace OHOS {
namespace {
constexpr int64_t DEFAULT_EXPIRE_TIME = 1000;
}

template <typename TKey, typename TValue>
class ExpireLruCache {
public:
    ExpireLruCache(size_t cacheSize = 8, int64_t expireTimeSec = 1000) : size_(cacheSize),
        expireTimeMilliSec_(expireTimeSec)
    {
        size_ = (size_ > 0) ? size_ : 1;
        expireTimeMilliSec_ = (expireTimeMilliSec_ < 0) ? DEFAULT_EXPIRE_TIME : expireTimeMilliSec_;
    }
    ~ExpireLruCache() {}

    void Add(const TKey& key, const TValue& val)
    {
        std::lock_guard<std::mutex> lock(lock_);
        DoAdd(key, val);
        return;
    }

    std::shared_ptr<TValue> Get(const TKey& key)
    {
        std::lock_guard<std::mutex> lock(lock_);
        return DoGet(key);
    }

    void Remove(const TKey& key)
    {
        std::lock_guard<std::mutex> lock(lock_);
        return DoRemove(key);
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(lock_);
        DoClear();
        return;
    }
private:
    class Timestamp {
    public:
        Timestamp()
        {
            ts_ = GetTickCount();
        }
        ~Timestamp() {}

        int64_t operator-(const Timestamp& ts)
        {
            return ts_ - ts.ts_;
        }

        bool IsExpired(int64_t interval) const
        {
            Timestamp now;
            int64_t diff = now - *this;
            return diff > interval;
        }

        int64_t Raw() const
        {
            return ts_;
        }
    private:
        /* unit:ms */
        int64_t ts_;
    };
    size_t size_;
    int64_t expireTimeMilliSec_;
    std::mutex lock_;
    std::map<TKey, std::shared_ptr<TValue>> data_;
    std::map<TKey, Timestamp> timestamp_;
    std::list<TKey> keys_;

    void DoAdd(const TKey& key, const TValue& value)
    {
        Timestamp now;
        size_t curSize = data_.size();
        auto iter = data_.find(key);
        auto timestampIter = timestamp_.find(key);

        if (iter != data_.end()) {
            data_.erase(iter);
            timestamp_.erase(timestampIter);
            data_.insert(std::make_pair(key, std::make_shared<TValue>(value)));
            timestamp_.insert(std::make_pair(key, now));
            auto keyiter = std::find(keys_.begin(), keys_.end(), key);
            keys_.splice(keys_.begin(), keys_, keyiter);

            return;
        }

        if (curSize >= size_) {
            bool ifClearExpiredCache = false;
            if (expireTimeMilliSec_ > 0) {
                ifClearExpiredCache = DoClearExpiredCache();
            }
            if (ifClearExpiredCache == false) {
                auto lruKey = keys_.back();
                data_.erase(lruKey);
                timestamp_.erase(lruKey);
                keys_.pop_back();
            }
        }

        data_.insert(std::make_pair(key, std::make_shared<TValue>(value)));
        timestamp_.insert(std::make_pair(key, now));
        keys_.push_front(key);

        return;
    }

    std::shared_ptr<TValue> DoGet(const TKey& key)
    {
        auto dataIter = data_.find(key);
        if (dataIter == data_.end()) {
            return nullptr;
        }

        auto timestampIter = timestamp_.find(key);
        if ((expireTimeMilliSec_ > 0) && (timestampIter->second.IsExpired(expireTimeMilliSec_))) {
            data_.erase(dataIter);
            timestamp_.erase(timestampIter);
            keys_.remove(key);

            return nullptr;
        } else {
            auto keyiter = std::find(keys_.begin(), keys_.end(), key);
            keys_.splice(keys_.begin(), keys_, keyiter) ;

            return dataIter->second;
        }
    }

    void DoRemove(const TKey& key)
    {
        keys_.remove(key);
        data_.erase(key);
        timestamp_.erase(key);
    }

    void DoClear()
    {
        data_.clear();
        timestamp_.clear();
        keys_.clear();
    }

    bool DoClearExpiredCache()
    {
        bool ifClear = false;
        for (auto iter = timestamp_.begin(); iter != timestamp_.end();) {
            if (iter->second.IsExpired(expireTimeMilliSec_)) {
                ifClear = true;
                keys_.remove(iter->first);
                data_.erase(iter->first);
                iter = timestamp_.erase(iter);
            } else {
                iter++;
            }
        }
        return ifClear;
    }
};
}

#endif
