/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef SQLITE_CLOUD_STORE_H
#define SQLITE_CLOUD_STORE_H

#include "icloud_sync_storage_interface.h"
#include "istorage_handle.h"

namespace DistributedDB {
class SqliteCloudStore : public ICloudSyncStorageInterface, public RefObject {
public:
    explicit SqliteCloudStore(IStorageHandle *handle);
    ~SqliteCloudStore() override = default;

    int GetMetaData(const Key &key, Value &value) const override;

    int PutMetaData(const Key &key, const Value &value) override;

    int ChkSchema(const TableName &tableName) override;

    int SetCloudDbSchema(const DataBaseSchema &schema) override;

    int GetCloudDbSchema(std::shared_ptr<DataBaseSchema> &cloudSchema) override;

    int GetCloudTableSchema(const TableName &tableName, TableSchema &tableSchema) override;

    int StartTransaction(TransactType type) override;

    int Commit() override;

    int Rollback() override;

    int GetUploadCount(const QuerySyncObject &query, const Timestamp &timestamp, bool isCloudForcePush,
        int64_t &count) override;

    int GetCloudData(const TableSchema &tableSchema, const QuerySyncObject &object, const Timestamp &beginTime,
        ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) override;

    int GetCloudDataNext(ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) override;

    int GetCloudGid(const TableSchema &tableSchema, const QuerySyncObject &querySyncObject, bool isCloudForcePush,
        std::vector<std::string> &cloudGid) override;

    int ReleaseCloudDataToken(ContinueToken &continueStmtToken) override;

    int GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket, DataInfoWithLog &dataInfoWithLog,
        VBucket &assetInfo) override;

    int PutCloudSyncData(const std::string &tableName, DownloadData &downloadData) override;

    int CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
        const RelationalSchemaObject &localSchema, std::vector<Asset> &assets) override;

    void TriggerObserverAction(const std::string &deviceName, ChangedData &&changedData, bool isChangedData) override;

    int FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess) override;

    int SetLogTriggerStatus(bool status) override;

    int FillCloudLogAndAsset(OpType opType, const CloudSyncData &data, bool fillAsset, bool ignoreEmptyGid) override;

    std::string GetIdentify() const override;

    int GetCloudDataGid(const QuerySyncObject &query, Timestamp beginTime, std::vector<std::string> &gid) override;

    int CheckQueryValid(const QuerySyncObject &query) override;

    int CreateTempSyncTrigger(const std::string &tableName) override;

    int GetAndResetServerObserverData(const std::string &tableName, ChangeProperties &changeProperties) override;

    int ClearAllTempSyncTrigger() override;

    bool IsSharedTable(const std::string &tableName) override;

    void SetCloudTaskConfig(const CloudTaskConfig &config) override;

    int GetAssetsByGidOrHashKey(const TableSchema &tableSchema, const std::string &gid, const Bytes &hashKey,
        VBucket &assets) override;

    int SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader) override;

    int UpdateRecordFlag(const std::string &tableName, bool recordConflict, const LogInfo &logInfo) override;

    int GetCompensatedSyncQuery(std::vector<QuerySyncObject> &syncQuery) override;

    int MarkFlagAsConsistent(const std::string &tableName, const DownloadData &downloadData,
        const std::set<std::string> &gidFilters) override;
private:
    IStorageHandle *storageHandle_;
};
}
#endif // SQLITE_CLOUD_STORE_H