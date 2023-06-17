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

#ifndef STORE_OBSERVER_H
#define STORE_OBSERVER_H

#include "cloud/cloud_store_types.h"
#include "store_changed_data.h"

namespace DistributedDB {

enum ChangeType : uint32_t {
    OP_INSERT = 0,
    OP_UPDATE,
    OP_DELETE,
    OP_BUTT,
};

enum ChangedDataType : uint32_t {
    DATA = 0,
    ASSET = 1,
};

struct ChangedData {
    std::string tableName;
    ChangedDataType type;
    // CLOUD_COOPERATION mode, primaryData store primary keys
    // primayData store row id if have no data
    std::vector<std::vector<Type>> primaryData[OP_BUTT];
    std::vector<std::string> field;
};

enum Origin : int32_t {
    ORIGIN_CLOUD,
    ORIGIN_LOCAL,
    ORIGIN_REMOTE,
    ORIGIN_ALL,
    ORIGIN_BUTT
};
class StoreObserver {
public:
    virtual ~StoreObserver() {}

    // Data change callback
    virtual void OnChange(const StoreChangedData &data) {};

    virtual void OnChange(Origin origin, const std::string &originalId, ChangedData &&data) {};
};
} // namespace DistributedDB

#endif // STORE_OBSERVER_H