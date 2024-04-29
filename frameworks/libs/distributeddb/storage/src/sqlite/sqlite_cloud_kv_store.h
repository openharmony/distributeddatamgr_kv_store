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

    int StartTransaction(TransactType type) override;

    int Commit() override;

    int Rollback() override;

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

    void TriggerObserverAction(const std::string &deviceName, ChangedData &&changedData, bool isChangedData) override;

    int FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess) override;

    int SetLogTriggerStatus(bool status) override;

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
private:
    std::pair<sqlite3 *, bool> GetTransactionDbHandleAndMemoryStatus();

    static void FillTimestamp(Timestamp rawSystemTime, Timestamp virtualTime, CloudSyncBatch &syncBatch);

    static void FilterCloudVersionPrefixKey(std::vector<std::vector<Type>> &changeValList);

    KvStorageHandle *storageHandle_;

    std::mutex schemaMutex_;
    std::map<std::string, DataBaseSchema> schema_;

    mutable std::mutex transactionMutex_;
    SQLiteSingleVerStorageExecutor *transactionHandle_;

    std::string user_;

    std::mutex observerMapMutex_;
    std::map<const KvStoreObserver *, ObserverAction> cloudObserverMap_;
};
}
#endif // SQLITE_CLOUD_STORE_H