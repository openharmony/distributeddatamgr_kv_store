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

#include "store_types.h"

namespace DistributedDB {
    enum TableSyncType {
        DEVICE_COOPERATION = 0,
        CLOUD_COOPERATION = 1,
    };

    enum ClearMode {
        DATA_INCLUDE = 0,
        FLAG_ONLY = 1,
    };

    struct Asset {
        uint32_t version = 0;
        std::string name;
        std::string uri;
        std::string modifyTime;
        std::string createTime;
        std::string size;
        std::string hash;
    };

    using Assets = std::vector<Asset>;
    using Bytes = std::vector<uint8_t>;
    using CloudValue = std::variant<std::monostate, int64_t, double, std::string, bool, Bytes, Asset, Assets>;
    using VBucket = std::map<std::string, CloudValue>;

    enum ProcessStatus {
        PREPARED = 0,
        PROCESSING = 1,
        FINISHED = 2,
    };

    struct Info {
        uint32_t batchIndex = 0;
        uint32_t total = 0;
        uint32_t successCount = 0; // merge or upload success count
        uint32_t failCount = 0;
    };

    struct SyncProcess {
        ProcessStatus process = PREPARED;
        DBStatus errCode = OK;
        Info downLoadInfo;
        Info upLoadInfo;
    };

    struct Field {
        std::string colName;
        int32_t type; // get value from TYPE_INDEX;
        bool primary = false;
        bool nullable = true;
    };

    struct TableSchema {
        std::string name;
        std::vector<Field> fields;
    };

    struct DataBaseSchema {
        std::vector<TableSchema> tables;
    };
} // namespace DistributedDB
#endif // CLOUD_STORE_TYPE_H
