/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef KV_STORE_TYPE_H
#define KV_STORE_TYPE_H

#include <functional>
#include <map>
#include <set>
#include <string>

#include "types_export.h"

namespace DistributedDB {
enum DBStatus {
    DB_ERROR = -1,
    OK = 0,
    BUSY,
    NOT_FOUND,
    INVALID_ARGS,
    TIME_OUT,
    NOT_SUPPORT,
    INVALID_PASSWD_OR_CORRUPTED_DB,
    OVER_MAX_LIMITS,
    INVALID_FILE,
    NO_PERMISSION,
    FILE_ALREADY_EXISTED,
    SCHEMA_MISMATCH,
    INVALID_SCHEMA,
    READ_ONLY,
    INVALID_VALUE_FIELDS, // invalid put value for json schema.
    INVALID_FIELD_TYPE, // invalid put value field type for json schema.
    CONSTRAIN_VIOLATION, // invalid put value constrain for json schema.
    INVALID_FORMAT, // invalid put value format for json schema.
    STALE, // new record is staler compared to the same key existed in db.
    LOCAL_DELETED, // local data is deleted by the unpublish.
    LOCAL_DEFEAT, // local data defeat the sync data while unpublish.
    LOCAL_COVERED, // local data is covered by the sync data while unpublish.
    INVALID_QUERY_FORMAT,
    INVALID_QUERY_FIELD,
    PERMISSION_CHECK_FORBID_SYNC, // permission check result , forbid sync.
    ALREADY_SET, // already set.
    COMM_FAILURE, // communicator may get some error.
    EKEYREVOKED_ERROR, // EKEYREVOKED error when operating db file
    SECURITY_OPTION_CHECK_ERROR, // such as remote device's SecurityOption not equal to local
    SCHEMA_VIOLATE_VALUE, // Values already exist in dbFile do not match new schema
    INTERCEPT_DATA_FAIL, // Interceptor push data failed.
    LOG_OVER_LIMITS, // Log size is over the limits.
    DISTRIBUTED_SCHEMA_NOT_FOUND, // the sync table is not a relational table
    DISTRIBUTED_SCHEMA_CHANGED, // the schema was changed
    MODE_MISMATCH,
    NOT_ACTIVE,
    USER_CHANGED,
    NONEXISTENT,  // for row record, pass invalid column name or invalid column index.
    TYPE_MISMATCH,  // for row record, get value with mismatch func.
    REMOTE_OVER_SIZE, // for remote query, the data is too many, only get part or data.
    RATE_LIMIT,
    DATA_HANDLE_ERROR, // remote handle data failed
    CONSTRAINT, // constraint check failed in sqlite
    CLOUD_ERROR, // cloud error
    QUERY_END, // Indicates that query function has queried last data from cloud
    DB_CLOSED, // db is closed
    UNSET_ERROR, // something should be set not be set
    CLOUD_NETWORK_ERROR, // network error in cloud
    CLOUD_SYNC_UNSET, // not set sync option in cloud
    CLOUD_FULL_RECORDS, // cloud's record is full
    CLOUD_LOCK_ERROR, // cloud failed to get sync lock
    CLOUD_ASSET_SPACE_INSUFFICIENT, // cloud failed to download asset
    PROPERTY_CHANGED, // reference property changed
    CLOUD_VERSION_CONFLICT, // cloud failed to update version
    CLOUD_RECORD_EXIST_CONFLICT, // this error happen in Download/BatchInsert/BatchUpdate
    REMOTE_ASSETS_FAIL, // remove local assets failed
};

struct KvStoreConfig {
    std::string dataDir;
};

enum PragmaCmd {
    AUTO_SYNC = 1,
    SYNC_DEVICES = 2, // this cmd will be removed in the future, don't use it
    RM_DEVICE_DATA = 3, // this cmd will be removed in the future, don't use it
    PERFORMANCE_ANALYSIS_GET_REPORT,
    PERFORMANCE_ANALYSIS_OPEN,
    PERFORMANCE_ANALYSIS_CLOSE,
    PERFORMANCE_ANALYSIS_SET_REPORTFILENAME,
    GET_IDENTIFIER_OF_DEVICE,
    GET_DEVICE_IDENTIFIER_OF_ENTRY,
    GET_QUEUED_SYNC_SIZE,
    SET_QUEUED_SYNC_LIMIT,
    GET_QUEUED_SYNC_LIMIT,
    SET_WIPE_POLICY,  // set the policy of wipe remote stale data
    RESULT_SET_CACHE_MODE, // Accept ResultSetCacheMode Type As PragmaData
    RESULT_SET_CACHE_MAX_SIZE, // Allowed Int Type Range [1,16], Unit MB
    SET_SYNC_RETRY,
    SET_MAX_LOG_LIMIT,
    EXEC_CHECKPOINT,
    LOGIC_DELETE_SYNC_DATA,
};

enum ResolutionPolicyType {
    AUTO_LAST_WIN = 0,      // resolve conflicts by timestamp(default value)
    CUSTOMER_RESOLUTION = 1 // resolve conflicts by user
};

enum ObserverMode {
    OBSERVER_CHANGES_NATIVE = 1,
    OBSERVER_CHANGES_FOREIGN = 2,
    OBSERVER_CHANGES_LOCAL_ONLY = 4,
};

enum SyncMode {
    SYNC_MODE_PUSH_ONLY,
    SYNC_MODE_PULL_ONLY,
    SYNC_MODE_PUSH_PULL,
    SYNC_MODE_CLOUD_MERGE = 4,
    SYNC_MODE_CLOUD_FORCE_PUSH,
    SYNC_MODE_CLOUD_FORCE_PULL,
};

enum ConflictResolvePolicy {
    LAST_WIN = 0,
    DEVICE_COLLABORATION,
};

struct TableStatus {
    std::string tableName;
    DBStatus status;
};

enum ProcessStatus {
    PREPARED = 0,
    PROCESSING = 1,
    FINISHED = 2,
};

enum class CollateType : uint32_t {
    COLLATE_NONE = 0,
    COLLATE_NOCASE,
    COLLATE_RTRIM,
    COLLATE_BUTT
};

struct Info {
    uint32_t batchIndex = 0;
    uint32_t total = 0;
    uint32_t successCount = 0; // merge or upload success count
    uint32_t failCount = 0;
};

struct TableProcessInfo {
    ProcessStatus process = PREPARED;
    Info downLoadInfo;
    Info upLoadInfo;
};

struct SyncProcess {
    ProcessStatus process = PREPARED;
    DBStatus errCode = OK;
    std::map<std::string, TableProcessInfo> tableProcess;
};

using KvStoreCorruptionHandler = std::function<void (const std::string &appId, const std::string &userId,
    const std::string &storeId)>;
using StoreCorruptionHandler = std::function<void (const std::string &appId, const std::string &userId,
    const std::string &storeId)>;
using SyncStatusCallback = std::function<void(const std::map<std::string, std::vector<TableStatus>> &devicesMap)>;

using SyncProcessCallback = std::function<void(const std::map<std::string, SyncProcess> &process)>;

struct RemoteCondition {
    std::string sql;  // The sql statement;
    std::vector<std::string> bindArgs;  // The bind args.
};
using UpdateKeyCallback = std::function<void (const Key &originKey, Key &newKey)>;

struct TrackerSchema {
    std::string tableName;
    std::string extendColName;
    std::set<std::string> trackerColNames;
};

struct TableReferenceProperty {
    std::string sourceTableName;
    std::string targetTableName;
    std::map<std::string, std::string> columns; // key is sourceTable column, value is targetTable column
};

static constexpr const char *GAUSSDB_RD = "gaussdb_rd";
static constexpr const char *SQLITE = "sqlite";
struct ChangeProperties {
    bool isTrackedDataChange = false;
};

struct Rdconfig {
    bool readOnly = false;
};
} // namespace DistributedDB
#endif // KV_STORE_TYPE_H
