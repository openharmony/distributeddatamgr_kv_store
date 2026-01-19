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
#include "cloud/cloud_db_types.h"
#include "db_types.h"
#include "icloud_sync_storage_interface.h"
#include "types_export.h"

namespace DistributedDB {

class CloudMetaData {
public:
    explicit CloudMetaData(ICloudSyncStorageInterface *store);
    ~CloudMetaData() = default;

    int GetLocalWaterMark(const TableName &tableName, Timestamp &localMark);
    int GetLocalWaterMarkByType(const TableName &tableName, CloudWaterType type, Timestamp &localMark);
    int GetCloudWaterMark(const TableName &tableName, std::string &cloudMark);

    int SetLocalWaterMark(const TableName &tableName, Timestamp localMark);
    int SetLocalWaterMarkByType(const TableName &tableName, CloudWaterType type, Timestamp localMark);
    int SetCloudWaterMark(const TableName &tableName, std::string &cloudMark);

    int CleanWaterMark(const TableName &tableName);

    void CleanAllWaterMark();

    void CleanWaterMarkInMemory(const TableName &tableName);

    int GetCloudGidCursor(const std::string &tableName, std::string &cursor) const;
    int PutCloudGidCursor(const std::string &tableName, const std::string &cursor) const;
    int GetBackupCloudCursor(const std::string &tableName, std::string &cursor) const;
    int PutBackupCloudCursor(const std::string &tableName, const std::string &cursor) const;
    int CleanCloudInfo(const std::string &tableName) const;

private:
    typedef struct CloudMetaValue {
        Timestamp localMark = 0u;
        Timestamp insertLocalMark = 0u;
        Timestamp updateLocalMark = 0u;
        Timestamp deleteLocalMark = 0u;
        std::string cloudMark;
    } CloudMetaValue;

    typedef struct CloudInfoValue {
        uint64_t version = 0u;
        std::string gidCloudMark;
        // when query all gid finished, use it as query watermark
        std::string backupCloudCursor;
    } CloudInfoValue;

    int ReadMarkFromMeta(const TableName &tableName);
    int WriteMarkToMeta(const TableName &tableName, Timestamp localmark, std::string &cloudMark);
    int WriteTypeMarkToMeta(const TableName &tableName, CloudMetaValue &cloudMetaValue);
    int SerializeWaterMark(CloudMetaValue &cloudMetaValue, Value &blobMetaVal);
    int DeserializeMark(Value &blobMark, CloudMetaValue &cloudMetaValue);
    uint64_t GetParcelCurrentLength(CloudMetaValue &cloudMetaValue);

    int GetCloudInfo(const std::string &tableName, CloudInfoValue &info) const ;
    int PutCursorInner(const std::string &tableName, const std::string &cursor, bool isGidCursor) const;
    static int DeserializeCloudInfo(Value &blobMark, CloudInfoValue &info);
    static int SerializeCloudInfo(const CloudInfoValue &info, Value &blobMark);

    static uint64_t GetParcelCurrentLength(const CloudInfoValue &info);
    static Key GetCloudInfoKey(const std::string &tableName);

    mutable std::mutex cloudMetaMutex_;
    std::unordered_map<TableName, CloudMetaValue> cloudMetaVals_;
    ICloudSyncStorageInterface *store_ = nullptr;
};

} // namespace DistributedDB
#endif // CLOUD_META_DATA_H
