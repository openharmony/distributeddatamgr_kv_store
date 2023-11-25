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

#ifndef CLOUD_STORE_TYPE_H
#define CLOUD_STORE_TYPE_H

#include <functional>
#include <map>
#include <variant>
#include <string>
#include <stdint.h>

#include "query.h"
#include "store_types.h"

namespace DistributedDB {
enum TableSyncType {
    DEVICE_COOPERATION = 0,
    CLOUD_COOPERATION = 1,
};

enum ClearMode {
    DEFAULT = 0,        // use for device to device sync
    FLAG_AND_DATA = 1,  // use for device to cloud sync
    FLAG_ONLY = 2,
    CLEAR_SHARED_TABLE = 3,
    BUTT = 4,
};

enum class AssetOpType {
    NO_CHANGE = 0,
    INSERT,
    DELETE,
    UPDATE
};

enum AssetStatus : uint32_t {
    NORMAL = 0,
    DOWNLOADING,
    ABNORMAL,
    INSERT, // INSERT/DELETE/UPDATE are for client use
    DELETE,
    UPDATE
};

struct Asset {
    uint32_t version = 0;
    std::string name;
    std::string assetId;
    std::string subpath;
    std::string uri;
    std::string modifyTime;
    std::string createTime;
    std::string size;
    std::string hash;
    uint32_t flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
    uint32_t status = static_cast<uint32_t>(AssetStatus::NORMAL);
    int64_t timestamp = 0;
};
using Nil = std::monostate;
using Assets = std::vector<Asset>;
using Bytes = std::vector<uint8_t>;
using Entries = std::map<std::string, std::string>;
using Type = std::variant<Nil, int64_t, double, std::string, bool, Bytes, Asset, Assets, Entries>;
using VBucket = std::map<std::string, Type>;

struct Field {
    std::string colName;
    int32_t type; // get value from TYPE_INDEX;
    bool primary = false;
    bool nullable = true;
    bool operator==(const Field &comparedField) const
    {
        return (colName == comparedField.colName) && (type == comparedField.type) &&
            (primary == comparedField.primary) && (nullable == comparedField.nullable);
    }
};

struct TableSchema {
    std::string name;
    std::string sharedTableName;
    std::vector<Field> fields;
};

struct DataBaseSchema {
    std::vector<TableSchema> tables;
};

enum class CloudQueryType : int64_t {
    FULL_TABLE = 0, // query full table
    QUERY_FIELD = 1 // query with some fields
};

struct CloudSyncOption {
    std::vector<std::string> devices;
    SyncMode mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    Query query;
    int64_t waitTime = 0;
    bool priorityTask = false;
};

enum class QueryNodeType : uint32_t {
    ILLEGAL = 0,
    IN = 1,
    OR = 0x101,
    AND,
    EQUAL_TO = 0x201,
    BEGIN_GROUP = 0x301,
    END_GROUP
};

struct QueryNode {
    QueryNodeType type = QueryNodeType::ILLEGAL;
    std::string fieldName;
    std::vector<Type> fieldValue;
};

struct SqlCondition {
    std::string sql;  // The sql statement;
    std::vector<Type> bindArgs;  // The bind args.
};
} // namespace DistributedDB
#endif // CLOUD_STORE_TYPE_H