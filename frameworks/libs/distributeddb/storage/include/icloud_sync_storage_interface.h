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

#ifndef RELATIONAL_DB_CLOUD_INTERFACE_H
#define RELATIONAL_DB_CLOUD_INTERFACE_H

#include "cloud/cloud_db_types.h"
#include "data_transformer.h"
#include "sqlite_utils.h"
#include "store_observer.h"

namespace DistributedDB {

enum class OpType : uint8_t {
    INSERT = 1,
    UPDATE, // update data, gid and timestamp at same time
    DELETE,
    ONLY_UPDATE_GID,
    NOT_HANDLE
};

typedef struct DownloadData {
    std::vector<VBucket> data;
    std::vector<OpType> opType;
} DownloadData;

class ICloudSyncStorageInterface {
public:
    ICloudSyncStorageInterface() = default;
    virtual ~ICloudSyncStorageInterface() = default;

    virtual int GetMetaData(const Key &key, Value &value) const = 0;

    virtual int PutMetaData(const Key &key, const Value &value) = 0;

    virtual int ChkSchema(const TableName &tableName) = 0;

    virtual int SetCloudDbSchema(const DataBaseSchema &schema) = 0;

    virtual int GetCloudDbSchema(DataBaseSchema &cloudSchema) = 0;

    virtual int GetCloudTableSchema(const TableName &tableName, TableSchema &tableSchema) = 0;

    virtual int StartTransaction(TransactType type) = 0;

    virtual int Commit() = 0;

    virtual int Rollback() = 0;

    virtual int GetUploadCount(const std::string &tableName, const Timestamp &timestamp, int64_t &count) = 0;

    virtual int FillCloudGid(const CloudSyncData &data) = 0;

    virtual int GetCloudData(const TableSchema &tableSchema, const Timestamp &beginTime,
        ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) = 0;

    virtual int GetCloudDataNext(ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) = 0;

    virtual int ReleaseCloudDataToken(ContinueToken &continueStmtToken) = 0;

    virtual int GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket,
        LogInfo &logInfo, VBucket &assetInfo) = 0;

    virtual int PutCloudSyncData(const std::string &tableName, DownloadData &downloadData) = 0;

    virtual void TriggerObserverAction(const std::string &deviceName, ChangedData &&changedData,
        bool isChangedData) = 0;

    virtual int FillCloudAsset(const std::string &tableName, VBucket &asset, bool isFullReplace) = 0;
};
}

#endif //RELATIONAL_DB_CLOUD_INTERFACE_H
