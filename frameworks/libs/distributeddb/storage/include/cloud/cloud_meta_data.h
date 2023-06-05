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

#ifndef CLOUD_META_DATA_H
#define CLOUD_META_DATA_H

#include <mutex>
#include <string>
#include <unordered_map>

#include "cloud/cloud_store_types.h"
#include "db_types.h"
#include "icloud_sync_storage_interface.h"
#include "types_export.h"

namespace DistributedDB {

class CloudMetaData {
public:
    explicit CloudMetaData(ICloudSyncStorageInterface *store);
    ~CloudMetaData() = default;

    int GetLocalWaterMark(TableName tableName, LocalWaterMark &localMark);
    int GetCloudWaterMark(TableName tableName, CloudWaterMark &cloudMark);

    int SetLocalWaterMark(TableName tableName, LocalWaterMark localMark);
    int SetCloudWaterMark(TableName tableName, CloudWaterMark &cloudMark);

private:
    typedef struct CloudMetaValue {
        LocalWaterMark localMark;
        CloudWaterMark cloudMark;
    } CloudMetaValue;

    int ReadMarkFromMeta(const TableName &tableName);
    int WriteMarkToMeta(TableName &tableName, LocalWaterMark localmark, CloudWaterMark &cloudMark);
    int SerializeMark(TableName tableName, LocalWaterMark localMark, CloudWaterMark &cloudMark, Value &blobMeta);
    int DeserializeMark(Value &blobMark, CloudMetaValue &cloudMetaValue);
    Key PrefixTableName(const TableName &tableName);

    mutable std::mutex cloudMetaMutex_;
    std::unordered_map<TableName, CloudMetaValue> cloudMetaVals_;
    ICloudSyncStorageInterface *store_ = nullptr;
};

} // namespace DistributedDB
#endif // DISTRIBUTEDDB_TYPES_EXPORT_H
