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

#ifndef CLOUD_DB_TYPES_H
#define CLOUD_DB_TYPES_H

#include "cloud/cloud_store_types.h"
#include <string>

namespace DistributedDB {
enum class CloudWaterType {
    INSERT,
    UPDATE,
    DELETE,
    BUTT // invalid cloud water type
};

struct CloudSyncBatch {
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    std::vector<int64_t> rowid;
    std::vector<int64_t> timestamp;
    std::vector<VBucket> assets;
    std::vector<Bytes> hashKey;
};

struct ReviseModTimeInfo {
    Bytes hashKey;
    int64_t curTime;
    int64_t invalidTime;
};

struct CloudSyncData {
    std::string tableName;
    CloudSyncBatch insData;
    CloudSyncBatch updData;
    CloudSyncBatch delData;
    CloudSyncBatch lockData;
    std::vector<ReviseModTimeInfo> revisedData;
    bool isCloudForcePushStrategy = false;
    bool isCompensatedTask = false;
    bool isShared = false;
    int ignoredCount = 0;
    bool isCloudVersionRecord = false;
    CloudWaterType mode = CloudWaterType::BUTT;
    CloudSyncData() = default;
    CloudSyncData(const std::string &_tableName) : tableName(_tableName) {};
    CloudSyncData(const std::string &_tableName, CloudWaterType _mode) : tableName(_tableName), mode(_mode) {};
};

struct CloudTaskConfig {
    bool allowLogicDelete = false;
};

} // namespace DistributedDB
#endif // CLOUD_DB_TYPES_H
