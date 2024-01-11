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

#ifndef ICLOUD_DB_H
#define ICLOUD_DB_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "cloud/cloud_store_types.h"

namespace DistributedDB {
class ICloudDb {
public:
    virtual ~ICloudDb() {};
    /**
     ** param[in & out] extend: will fill gid after insert ok
    **/
    virtual DBStatus BatchInsert(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend) = 0;
    virtual DBStatus BatchUpdate(const std::string &tableName, std::vector<VBucket> &&record,
        std::vector<VBucket> &extend) = 0;
    virtual DBStatus BatchDelete(const std::string &tableName, std::vector<VBucket> &extend) = 0;
    /**
     ** param[out] data: query data
    **/
    virtual DBStatus Query(const std::string &tableName, VBucket &extend, std::vector<VBucket> &data) = 0;
    virtual std::pair<DBStatus, std::string> GetEmptyCursor(const std::string &tableName) = 0;
    virtual std::pair<DBStatus, uint32_t> Lock() = 0;
    virtual DBStatus UnLock() = 0;
    virtual DBStatus HeartBeat() = 0;
    virtual DBStatus Close() = 0;
};
} // namespace DistributedDB

#endif // ICLOUD_DB_H
