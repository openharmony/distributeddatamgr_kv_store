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

#ifndef CLOUD_DB_CONSTANT_H
#define CLOUD_DB_CONSTANT_H

#include "cloud/cloud_store_types.h"
#include <string>

namespace DistributedDB {
class CloudDbConstant {
public:
    static constexpr const char *CLOUD_META_TABLE_PREFIX = "naturalbase_cloud_meta_";
    static constexpr const char *GID_FIELD = "#_gid";
    static constexpr const char *CREATE_FIELD = "#_createTime";
    static constexpr const char *MODIFY_FIELD = "#_modifyTime";
    static constexpr const char *DELETE_FIELD = "#_deleted";
    static constexpr const char *CURSOR_FIELD = "#_cursor";
    static constexpr const char *TYPE_FIELD = "#_type";
    static constexpr const char *QUERY_FIELD = "#_query";
    static constexpr const char *REFERENCE_FIELD = "#_reference";
    static constexpr const char *VERSION_FIELD = "#_version";
    static constexpr const char *ERROR_FIELD = "#_error";
    static constexpr const char *SHARING_RESOURCE_FIELD = "#_sharing_resource";
    static constexpr const char *HASH_KEY_FIELD = "#_hash_key";
    static constexpr const char *ASSET = "asset";
    static constexpr const char *ASSETS = "assets";
    static constexpr const char *SHARED = "_shared";
    static constexpr const char *CLOUD_OWNER = "cloud_owner";
    static constexpr const char *CLOUD_PRIVILEGE = "cloud_privilege";
    static constexpr const char *DEFAULT_CLOUD_DEV = "cloud";

    // use for inner
    static constexpr const char *FLAG = "flag";
    static constexpr const char *TIMESTAMP = "timestamp";
    static constexpr const char *HASH_KEY = "hash_key";
    static constexpr const char *STATUS = "status";
    static constexpr const char *CLOUD_FLAG = "cloud_flag";

    // kv cloud field
    static constexpr const char *CLOUD_KV_FIELD_KEY = "key";
    static constexpr const char *CLOUD_KV_FIELD_VALUE = "value";
    static constexpr const char *CLOUD_KV_FIELD_DEVICE = "cur_device";
    static constexpr const char *CLOUD_KV_FIELD_ORI_DEVICE = "ori_device";
    static constexpr const char *CLOUD_KV_FIELD_DEVICE_CREATE_TIME = "device_create_time";
    static constexpr const char *CLOUD_KV_TABLE_NAME = "sync_data";
    static constexpr const char *CLOUD_KV_FIELD_USERID = "userid";

    // data status changes to lock_change
    static constexpr const char *TO_LOCAL_CHANGE = "status = CASE WHEN status == 2 THEN 3 ELSE status END";
    // data status changes from unlocking to unlock
    static constexpr const char *UNLOCKING_TO_UNLOCK = "status = CASE WHEN status == 1 THEN 0 ELSE status END";
    // data status changes to unlock
    static constexpr const char *TO_UNLOCK = "status = CASE WHEN status == 2 THEN 0 WHEN status == 3 THEN 1"
        " ELSE status END";
    // data status changes to lock
    static constexpr const char *TO_LOCK = "status = CASE WHEN status == 0 THEN 2 WHEN status == 1 THEN 3"
        " ELSE status END";

    static constexpr const char *CLOUD_VERSION_RECORD_PREFIX_KEY = "naturalbase_cloud_version_";

    static constexpr const char *FLAG_AND_DATA_MODE_NOTIFY = "DELETE#ALL_CLOUDDATA";

    static constexpr const char *FLAG_ONLY_MODE_NOTIFY = "RESERVE#ALL_CLOUDDATA";

    // cloud data timestamp is utc ms precision
    // used for 100ns to ms when handle cloud data timestamp
    static constexpr const uint32_t TEN_THOUSAND = 10000;
    static constexpr const int64_t CLOUD_DEFAULT_TIMEOUT = -1;
    static constexpr const uint32_t ONE_SECOND = 1000 * TEN_THOUSAND; // 1000ms

    static constexpr const int32_t MAX_UPLOAD_BATCH_COUNT = 2000;
    static constexpr const int32_t MIN_UPLOAD_BATCH_COUNT = 1;
    static constexpr const int32_t MAX_UPLOAD_SIZE = 128 * 1024 * 1024; // 128M
    static constexpr const int32_t MIN_UPLOAD_SIZE = 1024; // 1024Bytes
    static constexpr const int32_t MIN_RETRY_CONFLICT_COUNTS = -1; // unlimited

    static constexpr const int32_t MAX_ASYNC_DOWNLOAD_TASK = 12; // max async download task in process
    static constexpr const int32_t MIN_ASYNC_DOWNLOAD_TASK = 1;
    static constexpr const int32_t MAX_ASYNC_DOWNLOAD_ASSETS = 2000; // max async download assets count in one batch
    static constexpr const int32_t MIN_ASYNC_DOWNLOAD_ASSETS = 1;

    static constexpr const int32_t COMMON_TASK_PRIORITY_LEVEL = -1;
    static constexpr const int32_t PRIORITY_TASK_DEFALUT_LEVEL = 0;
    static constexpr const int32_t PRIORITY_TASK_MAX_LEVEL = 2;
    static constexpr const int32_t MAX_CONDITIONS_SIZE = 100;

    static constexpr const uint32_t ON_CHANGE_TRACKER = 0x1;
    static constexpr const uint32_t ON_CHANGE_P2P = 0x2;
    static constexpr const uint32_t ON_CHANGE_KNOWLEDGE = 0x4;
};
} // namespace DistributedDB
#endif // CLOUD_DB_CONSTANT_H