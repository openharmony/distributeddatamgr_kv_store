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

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <variant>

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

enum class ClearMetaDataMode : uint64_t {
    CLOUD_WATERMARK = 0x01,  // clear watermark of device to cloud sync
    BUTT,
};

struct ClearMetaDataOption {
    ClearMetaDataMode mode = ClearMetaDataMode::CLOUD_WATERMARK;
    std::set<std::string> tableNameList;  // an empty set means clearing meta data on all tables
};

enum class ClearKvMetaOpType : uint64_t {
    CLEAN_CLOUD_WATERMARK = 0x01,  // clear watermark of device to cloud sync
};

struct ClearKvMetaDataOption {
    ClearKvMetaOpType type = ClearKvMetaOpType::CLEAN_CLOUD_WATERMARK;
};

struct ClearDeviceDataOption {
    ClearMode mode = ClearMode::DEFAULT;
    std::string device;
    std::vector<std::string> tableList; // when left empty tableList, clear all tables
};

using VBucket = std::map<std::string, Type>;
using GenerateCloudVersionCallback = std::function<std::string(const std::string &originVersion)>;

struct Field {
    std::string colName;
    int32_t type; // get value from TYPE_INDEX;
    bool primary = false;
    bool nullable = true;
    bool dupCheckCol = false; // use for calculate hash_key when it was true
    bool operator==(const Field &comparedField) const
    {
        return (colName == comparedField.colName) && (type == comparedField.type) &&
            (primary == comparedField.primary) && (nullable == comparedField.nullable) &&
            (dupCheckCol == comparedField.dupCheckCol);
    }
};

struct TableSchema {
    std::string name;
    std::string sharedTableName; // if table is shared table, its sharedtablename is ""
    std::vector<Field> fields;
};

struct DataBaseSchema {
    std::vector<TableSchema> tables;
};

enum class CloudQueryType : int64_t {
    FULL_TABLE = 0, // query full table
    QUERY_FIELD = 1 // query with some fields
};

enum class LockAction : uint32_t {
    NONE = 0,
    INSERT = 0x1,
    UPDATE = 0x2,
    DELETE = 0x4,
    DOWNLOAD = 0x8
};

enum class QueryMode : uint32_t {
    UPLOAD_AND_DOWNLOAD = 0,
    UPLOAD_ONLY = 1
};

enum class SyncFlowType : uint32_t {
    NORMAL = 0, // upload and download
    DOWNLOAD_ONLY = 1
};

struct CloudSyncOption {
    std::vector<std::string> devices;
    SyncMode mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    Query query;
    int64_t waitTime = 0;
    bool priorityTask = false;
    int32_t priorityLevel = 0; // [0, 2]
    bool compensatedSyncOnly = false;
    std::vector<std::string> users;
    bool merge = false;
    // default, upload insert need lock
    LockAction lockAction = LockAction::INSERT;
    std::string prepareTraceId;
    bool asyncDownloadAssets = false;
    QueryMode queryMode = QueryMode::UPLOAD_AND_DOWNLOAD;
    SyncFlowType syncFlowType = SyncFlowType::NORMAL;
};

enum class QueryNodeType : uint32_t {
    ILLEGAL = 0,
    IN = 1,
    NOT_IN = 2,
    OR = 0x101,
    AND,
    EQUAL_TO = 0x201,
    NOT_EQUAL_TO,
    BEGIN_GROUP = 0x301,
    END_GROUP,
    GREATER_THAN = 0x401,
    LESS_THAN,
    GREATER_THAN_OR_EQUAL_TO,
    LESS_THAN_OR_EQUAL_TO
};

struct QueryNode {
    QueryNodeType type = QueryNodeType::ILLEGAL;
    std::string fieldName;
    std::vector<Type> fieldValue;
};

struct SqlCondition {
    std::string sql;  // The sql statement;
    std::vector<Type> bindArgs;  // The bind args.
    bool readOnly = false;
};

enum class RecordStatus {
    WAIT_COMPENSATED_SYNC,
    NORMAL
};

enum class LockStatus : uint32_t {
    UNLOCK = 0,
    UNLOCKING,
    LOCK,
    LOCK_CHANGE,
    BUTT,
};

enum class AssetConflictPolicy : uint32_t {
    CONFLICT_POLICY_DEFAULT = 0,
    CONFLICT_POLICY_TIME_FIRST,
    CONFLICT_POLICY_TEMP_PATH,
    BUTT,
};

struct CloudSyncConfig {
    std::optional<int32_t> maxUploadCount;          // default max upload 30 records
    std::optional<int32_t> maxUploadSize;           // default max upload 1024 * 512 * 3 = 1.5m
    std::optional<int32_t> maxRetryConflictTimes;   // default max retry -1 is unlimited retry times
    std::optional<bool> isSupportEncrypt;           // default false encryption is not supported
    std::optional<bool> skipDownloadAssets;         // default false
    std::optional<AssetConflictPolicy> assetPolicy; // default CONFLICT_POLICY_DEFAULT
};

struct AsyncDownloadAssetsConfig {
    uint32_t maxDownloadTask = 1; // valid range in [1, 12] max async download task in process
    uint32_t maxDownloadAssetsCount = 100; // valid range in [1, 2000] max async download assets count in one batch
};

enum class TaskType : uint32_t {
    BACKGROUND_TASK = 0,
    ONLY_CLOUD_SYNC_TASK,
    BUTT
};
} // namespace DistributedDB
#endif // CLOUD_STORE_TYPE_H