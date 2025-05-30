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
#include "cloud/cloud_upload_recorder.h"
#include "kv_storage_handle.h"

namespace DistributedDB {
class SqliteCloudKvStore : public ICloudSyncStorageInterface, public RefObject {
public:
    explicit SqliteCloudKvStore(KvStorageHandle *handle);
    ~SqliteCloudKvStore() override = default;

    int GetMetaData(const Key &key, Value &value) const override;

    int PutMetaData(const Key &key, const Value &value) override;

    int ChkSchema(const TableName &tableName) override;

    int SetCloudDbSchema(const DataBaseSchema &schema) override;

    int GetCloudDbSchema(std::shared_ptr<DataBaseSchema> &cloudSchema) override;

    int GetCloudTableSchema(const TableName &tableName, TableSchema &tableSchema) override;

    int StartTransaction(TransactType type, bool isAsyncDownload = false) override;

    int Commit(bool isAsyncDownload = false) override;

    int Rollback(bool isAsyncDownload = false) override;

    int GetUploadCount(const QuerySyncObject &query, const Timestamp &timestamp, bool isCloudForcePush,
        bool isCompensatedTask, int64_t &count) override;

    int GetAllUploadCount(const QuerySyncObject &query, const std::vector<Timestamp> &timestampVec,
        bool isCloudForcePush, bool isCompensatedTask, int64_t &count) override;

    int GetCloudData(const TableSchema &tableSchema, const QuerySyncObject &object, const Timestamp &beginTime,
        ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) override;

    int GetCloudDataNext(ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) override;

    int GetCloudGid(const TableSchema &tableSchema, const QuerySyncObject &querySyncObject, bool isCloudForcePush,
        bool isCompensatedTask, std::vector<std::string> &cloudGid) override;

    int ReleaseCloudDataToken(ContinueToken &continueStmtToken) override;

    int GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket, DataInfoWithLog &dataInfoWithLog,
        VBucket &assetInfo) override;

    int PutCloudSyncData(const std::string &tableName, DownloadData &downloadData) override;

    int UpdateAssetStatusForAssetOnly(const std::string &tableName, VBucket &vBucket) override;

    void TriggerObserverAction(const std::string &deviceName, ChangedData &&changedData, bool isChangedData) override;

    int FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess) override;

    int FillCloudAssetForAsyncDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess) override;

    int SetLogTriggerStatus(bool status) override;

    int SetLogTriggerStatusForAsyncDownload(bool status) override;

    int SetCursorIncFlag(bool status) override;

    int FillCloudLogAndAsset(OpType opType, const CloudSyncData &data, bool fillAsset, bool ignoreEmptyGid) override;

    std::string GetIdentify() const override;

    int CheckQueryValid(const QuerySyncObject &query) override;

    bool IsSharedTable(const std::string &tableName) override;

    void SetUser(const std::string &user) override;

    int SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema);

    void RegisterObserverAction(const KvStoreObserver *observer, const ObserverAction &action);

    void UnRegisterObserverAction(const KvStoreObserver *observer);

    int GetCloudVersion(const std::string &device, std::map<std::string, std::string> &versionMap);

    std::pair<int, CloudSyncData> GetLocalCloudVersion() override;

    void SetCloudSyncConfig(const CloudSyncConfig &config);

    CloudSyncConfig GetCloudSyncConfig() const override;

    std::map<std::string, DataBaseSchema> GetDataBaseSchemas();

    void ReleaseUploadRecord(const std::string &tableName, const CloudWaterType &type, Timestamp localMark) override;

    bool IsTagCloudUpdateLocal(const LogInfo &localInfo, const LogInfo &cloudInfo,
        SingleVerConflictResolvePolicy policy) override;

    int GetCompensatedSyncQuery(std::vector<QuerySyncObject> &syncQuery, std::vector<std::string> &users,
        bool isQueryDownloadRecords) override;

    int ReviseLocalModTime(const std::string &tableName,
        const std::vector<ReviseModTimeInfo> &revisedData) override;

    int GetLocalDataCount(const std::string &tableName, int &dataCount, int &logicDeleteDataCount) override;

    std::pair<int, std::vector<std::string>> GetDownloadAssetTable() override;

    std::pair<int, std::vector<std::string>> GetDownloadAssetRecords(const std::string &tableName,
        int64_t beginTime) override;

    int OperateDataStatus(uint32_t dataOperator);
private:
    std::pair<sqlite3 *, SQLiteSingleVerStorageExecutor *> GetTransactionDbHandleAndMemoryStatus(bool isWrite);

    static void FillTimestamp(Timestamp rawSystemTime, Timestamp virtualTime, CloudSyncBatch &syncBatch);

    static void FilterCloudVersionPrefixKey(std::vector<std::vector<Type>> &changeValList);

    bool CheckSchema(std::map<std::string, DataBaseSchema> schema);

    int ReviseOneLocalModTime(sqlite3_stmt *stmt, const ReviseModTimeInfo &data, bool isMemory);

    int OperateDataStatusInner(SQLiteSingleVerStorageExecutor *handle, const std::string &currentVirtualTime,
        const std::string &currentTime, uint32_t dataOperator);

    int CommitForAsyncDownload();

    int RollbackForAsyncDownload();

    int StartTransactionForAsyncDownload(TransactType type);

    KvStorageHandle *storageHandle_;

    std::mutex schemaMutex_;
    std::map<std::string, DataBaseSchema> schema_;

    mutable std::mutex transactionMutex_;
    SQLiteSingleVerStorageExecutor *transactionHandle_;
    SQLiteSingleVerStorageExecutor *asyncDownloadTransactionHandle_ = nullptr;

    std::string user_;

    std::mutex observerMapMutex_;
    std::map<const KvStoreObserver *, ObserverAction> cloudObserverMap_;

    mutable std::mutex configMutex_;
    CloudSyncConfig config_;

    CloudUploadRecorder recorder_;
};
}
#endif // SQLITE_CLOUD_STORE_H