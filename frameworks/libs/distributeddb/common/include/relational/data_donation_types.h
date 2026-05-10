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
using namespace std;

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

struct DdDataHash {
    size_t operator ()(const DdData &data) const
    {
        if (data.data.empty()) {
            return 0;
        }
        std::string bufStr;
        for (const auto &pair : data.data) {
            bufStr += pair.first;
        }
        return std::hash<std::string>{}(bufStr);
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
