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
#ifndef DATA_DONATION_TYPES_H
#define DATA_DONATION_TYPES_H

#include <vector>
#include <memory>
#include <cstring>
#include <securec.h>
#include "store_types.h"
#include "db_types.h"
#include "unique_queue.h"

#ifdef RELATIONAL_STORE
namespace DistributedDB {

// Generic variable-length data structure
struct DdData {
    VBucket data;
    int16_t opType = 0;
    int32_t fileIdx = 0;
    uint64_t cursor = 0;                    // Actual cursor used by the underlying layer

    // Default constructor
    DdData() = default;

    explicit DdData(VBucket &bucket)
    {
        data = bucket;
    }

    // Check if valid
    bool IsValid() const
    {
        return !data.empty();
    }

    bool operator ==(const DdData &other) const
    {
        return data == other.data;
    }
};

constexpr int HASH_LEFT_SHIFT = 6;
constexpr int HASH_RIGHT_SHIFT = 2;
constexpr size_t HASH_MAGIC = 0x9e3779b9;
struct VariantHash {
    size_t operator()(const Nil &) const
    {
        return 0;
    }

    size_t operator()(const int64_t &v) const
    {
        return std::hash<int64_t>{}(v);
    }

    size_t operator()(const double &v) const
    {
        return std::hash<double>{}(v);
    }

    size_t operator()(const std::string &v) const
    {
        return std::hash<std::string>{}(v);
    }

    size_t operator()(const bool &v) const
    {
        return std::hash<bool>{}(v);
    }

    size_t operator()(const Bytes &v) const
    {
        size_t h = 0;
        for (auto b : v) {
            h ^= std::hash<uint8_t>{}(b)
                + HASH_MAGIC + (h << HASH_LEFT_SHIFT) + (h >> HASH_RIGHT_SHIFT);
        }
        return h;
    }

    size_t operator()(const Asset &) const
    {
        return 0;
    }

    size_t operator()(const Assets &) const
    {
        return 0;
    }

    size_t operator()(const Entries &) const
    {
        return 0;
    }
};

struct TypeHash {
    size_t operator()(const Type &value) const
    {
        return std::visit(VariantHash{}, value);
    }
};

struct DdDataHash {
    size_t operator ()(const DdData &data) const
    {
        size_t hash = 0;
        for (const auto &pair : data.data) {
            // hash key
            hash ^= std::hash<std::string>{}(pair.first)
                + HASH_MAGIC + (hash << HASH_LEFT_SHIFT) + (hash >> HASH_RIGHT_SHIFT);
            // hash value
            hash ^= TypeHash{}(pair.second)
                + HASH_MAGIC + (hash << HASH_LEFT_SHIFT) + (hash >> HASH_RIGHT_SHIFT);
        }
        return hash;
    }
};

enum class DonationType {
    GET_ALL = 0,
    GET_NEW = 1,
    INVALID_BUTT,
};

struct DdCursor {
    DonationType type = DonationType::GET_ALL;
    uint64_t cursor = 0;
};

}  // namespace DistributedDB
#endif  // RELATIONAL_STORE
#endif  // DATA_DONATION_TYPES_H
