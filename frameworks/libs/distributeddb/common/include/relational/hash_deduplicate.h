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
#ifndef HASH_DEDUPLICATE_H
#define HASH_DEDUPLICATE_H
#include <unordered_set>
#include "store_types.h"
#include "db_types.h"
#include "db_errno.h"

#ifdef RELATIONAL_STORE
namespace DistributedDB {
using namespace std;

// todo! Distinguish between key and value types
template <
    typename DdItem,
    typename Hash = std::hash<DdItem>,
    typename KeyCompare = std::equal_to<DdItem>,  // Used for deduplication
    typename KeyCompare2 = std::equal_to<DdItem>  // Used for determining invalidation and removal
>
class HashDeduplicate {
public:
    DdItem* Find(const DdItem &item) const
    {
        auto it = hashSet.find(item);
        if (it == hashSet.end()) {
            return nullptr;
        }
        return const_cast<DdItem*>(&(*it));
    }

    bool Insert(const DdItem &item)
    {
        CleanBucket(item);
        auto [it, inserted] = hashSet.insert(item);
        return inserted;
    }
    void Remove(const DdItem &item)
    {
        hashSet.erase(item);
    }
    bool Update(const DdItem &item)
    {
        Remove(item);
        return Insert(item);
    }
    void Clear()
    {
        SetType emptySet;
        hashSet.swap(emptySet);
    }

private:
    using SetType = std::unordered_set<DdItem, Hash, KeyCompare>;
    SetType hashSet;
    // Clean current bucket: use KeyCompare2 to determine "invalidated" → delete
    void CleanBucket(const DdItem &item)
    {
        if (hashSet.empty()) {
            return;
        }

        size_t hashCode = hashSet.hash_function()(item);
        size_t bucketIdx = hashCode % hashSet.bucket_count();
        auto &bucket = hashSet.bucket(bucketIdx);
        KeyCompare2 cmp2;

        for (auto it = bucket.begin(); it != bucket.end();) {
            // Key: KeyCompare2 returns true → means invalidated, can be deleted
            if (cmp2(*it, item)) {
                it = hashSet.erase(it);
            } else {
                ++it;
            }
        }
    }
};

}  // namespace DistributedDB
#endif  // RELATIONAL_STORE
#endif  // HASH_DEDUPLICATE_H