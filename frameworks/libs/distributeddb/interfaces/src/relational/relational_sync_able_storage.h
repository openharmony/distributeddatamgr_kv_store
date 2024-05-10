/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef RELATIONAL_SYNC_ABLE_STORAGE_H
#define RELATIONAL_SYNC_ABLE_STORAGE_H
#ifdef RELATIONAL_STORE

#include "lru_map.h"
#include "icloud_sync_storage_interface.h"
#include "relational_db_sync_interface.h"
#include "relationaldb_properties.h"
#include "cloud/schema_mgr.h"
#include "sqlite_single_relational_storage_engine.h"
#include "sqlite_single_ver_relational_continue_token.h"
#include "sync_able_engine.h"

namespace DistributedDB {
using RelationalObserverAction =
    std::function<void(const std::string &device, ChangedData &&changedData, bool isChangedData)>;
class RelationalSyncAbleStorage : public RelationalDBSyncInterface, public ICloudSyncStorageInterface,
    public virtual RefObject {
public:
    explicit RelationalSyncAbleStorage(std::shared_ptr<SQLiteSingleRelationalStorageEngine> engine);
    ~RelationalSyncAbleStorage() override;

    // Get interface type of this kvdb.
    int GetInterfaceType() const override;

    // Get the interface ref-count, in order to access asynchronously.
    void IncRefCount() override;

    // Drop the interface ref-count.
    void DecRefCount() override;

    // Get the identifier of this rdb.
    std::vector<uint8_t> GetIdentifier() const override;

    // Get the dual tuple identifier of this rdb.
    std::vector<uint8_t> GetDualTupleIdentifier() const override;

    // Get the max timestamp of all entries in database.
    void GetMaxTimestamp(Timestamp &stamp) const override;

    // Get the max timestamp of one table.
    int GetMaxTimestamp(const std::string &tableName, Timestamp &stamp) const override;

    // Get meta data associated with the given key.
    int GetMetaData(const Key &key, Value &value) const override;

    // Put meta data as a key-value entry.
    int PutMetaData(const Key &key, const Value &value) override;

    int PutMetaData(const Key &key, const Value &value, bool isInTransaction) override;

    // Delete multiple meta data records in a transaction.
    int DeleteMetaData(const std::vector<Key> &keys) override;

    // Delete multiple meta data records with key prefix in a transaction.
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const override;

    // Get all meta data keys.
    int GetAllMetaKeys(std::vector<Key> &keys) const override;

    const RelationalDBProperties &GetDbProperties() const override;

    // Get the data which would be synced with query condition
    int GetSyncData(QueryObject &query, const SyncTimeRange &timeRange,
        const DataSizeSpecInfo &dataSizeInfo, ContinueToken &continueStmtToken,
        std::vector<SingleVerKvEntry *> &entries) const override;

    int GetSyncDataNext(std::vector<SingleVerKvEntry *> &entries, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const override;

    int PutSyncDataWithQuery(const QueryObject &object, const std::vector<SingleVerKvEntry *> &entries,
        const DeviceID &deviceName) override;

    int RemoveDeviceData(const std::string &deviceName, bool isNeedNotify) override;

    RelationalSchemaObject GetSchemaInfo() const override;

    int GetSecurityOption(SecurityOption &option) const override;

    void NotifyRemotePushFinished(const std::string &deviceId) const override;

    // Get the timestamp when database created or imported
    int GetDatabaseCreateTimestamp(Timestamp &outTime) const override;

    std::vector<QuerySyncObject> GetTablesQuery() override;

    int LocalDataChanged(int notifyEvent, std::vector<QuerySyncObject> &queryObj) override;

    int InterceptData(std::vector<SingleVerKvEntry *> &entries, const std::string &sourceID,
        const std::string &targetID) const override;

    int CheckAndInitQueryCondition(QueryObject &query) const override;
    int RegisterObserverAction(uint64_t connectionId, const StoreObserver *observer,
        const RelationalObserverAction &action);
    int UnRegisterObserverAction(uint64_t connectionId, const StoreObserver *observer);
    void TriggerObserverAction(const std::string &deviceName, ChangedData &&changedData, bool isChangedData) override;

    int CreateDistributedDeviceTable(const std::string &device, const RelationalSyncStrategy &syncStrategy) override;

    int RegisterSchemaChangedCallback(const std::function<void()> &callback) override;

    void NotifySchemaChanged();

    void RegisterHeartBeatListener(const std::function<void()> &listener);

    int GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const override;

    bool CheckCompatible(const std::string &schema, uint8_t type) const override;

    int ExecuteQuery(const PreparedStmt &prepStmt, size_t packetSize, RelationalRowDataSet &data,
        ContinueToken &token) const override;

    int SaveRemoteDeviceSchema(const std::string &deviceId, const std::string &remoteSchema, uint8_t type) override;

    int GetRemoteDeviceSchema(const std::string &deviceId, RelationalSchemaObject &schemaObj) override;

    void ReleaseRemoteQueryContinueToken(ContinueToken &token) const override;

    // recycling the write handle
    void SetReusedHandle(StorageExecutor *handle);

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

    int ChkSchema(const TableName &tableName) override;

    int SetCloudDbSchema(const DataBaseSchema &schema) override;

    int GetCloudDbSchema(std::shared_ptr<DataBaseSchema> &cloudSchema) override;

    int GetCloudTableSchema(const TableName &tableName, TableSchema &tableSchema) override;

    int GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket,
        DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo) override;

    int PutCloudSyncData(const std::string &tableName, DownloadData &downloadData) override;

    int CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
        const RelationalSchemaObject &localSchema, std::vector<Asset> &assets) override;

    int FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess) override;

    int SetLogTriggerStatus(bool status) override;

    int FillCloudLogAndAsset(OpType opType, const CloudSyncData &data, bool fillAsset, bool ignoreEmptyGid) override;

    void SetSyncAbleEngine(std::shared_ptr<SyncAbleEngine> syncAbleEngine);

    std::string GetIdentify() const override;

    void EraseDataChangeCallback(uint64_t connectionId);

    void ReleaseContinueToken(ContinueToken &continueStmtToken) const override;

    int CheckQueryValid(const QuerySyncObject &query) override;

    int CreateTempSyncTrigger(const std::string &tableName) override;
    int GetAndResetServerObserverData(const std::string &tableName, ChangeProperties &changeProperties) override;
    int ClearAllTempSyncTrigger() override;
    bool IsSharedTable(const std::string &tableName) override;

    std::map<std::string, std::string> GetSharedTableOriginNames();

    void SetLogicDelete(bool logicDelete);

    void SetCloudTaskConfig(const CloudTaskConfig &config) override;

    std::pair<int, uint32_t> GetAssetsByGidOrHashKey(const TableSchema &tableSchema, const std::string &gid,
        const Bytes &hashKey, VBucket &assets) override;

    int SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader) override;

    int UpsertData(RecordStatus status, const std::string &tableName, const std::vector<VBucket> &records);

    int UpdateRecordFlag(const std::string &tableName, bool recordConflict, const LogInfo &logInfo) override;

    int GetCompensatedSyncQuery(std::vector<QuerySyncObject> &syncQuery) override;

    int MarkFlagAsConsistent(const std::string &tableName, const DownloadData &downloadData,
        const std::set<std::string> &gidFilters) override;

    void SyncFinishHook() override;

    int SetSyncFinishHook(const std::function<void (void)> &func) override;

    void SetDoUploadHook(const std::function<void (void)> &) override;

    void DoUploadHook() override;
protected:
    int FillReferenceData(CloudSyncData &syncData);

    int GetInfoByPrimaryKeyOrGidInner(SQLiteSingleVerRelationalStorageExecutor *handle, const std::string &tableName,
        const VBucket &vBucket, DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo);

    int PutCloudSyncDataInner(SQLiteSingleVerRelationalStorageExecutor *handle, const std::string &tableName,
        DownloadData &downloadData);

    virtual int GetReferenceGid(const std::string &tableName, const CloudSyncBatch &syncBatch,
        std::map<int64_t, Entries> &referenceGid);

    int FillCloudLogAndAssetInner(SQLiteSingleVerRelationalStorageExecutor *handle, OpType opType,
        const CloudSyncData &data, bool fillAsset, bool ignoreEmptyGid);

    int UpdateRecordFlagAfterUpload(SQLiteSingleVerRelationalStorageExecutor *handle, const std::string &tableName,
        const CloudSyncBatch &updateData, bool isLock = false);

    static int FillReferenceDataIntoExtend(const std::vector<int64_t> &rowid,
        const std::map<int64_t, Entries> &referenceGid, std::vector<VBucket> &extend);

private:
    SQLiteSingleVerRelationalStorageExecutor *GetHandle(bool isWrite, int &errCode,
        OperatePerm perm = OperatePerm::NORMAL_PERM) const;
    SQLiteSingleVerRelationalStorageExecutor *GetHandleExpectTransaction(bool isWrite, int &errCode,
        OperatePerm perm = OperatePerm::NORMAL_PERM) const;
    void ReleaseHandle(SQLiteSingleVerRelationalStorageExecutor *&handle) const;

    // get
    int GetSyncDataForQuerySync(std::vector<DataItem> &dataItems, SQLiteSingleVerRelationalContinueToken *&token,
        const DataSizeSpecInfo &dataSizeInfo) const;
    int GetRemoteQueryData(const PreparedStmt &prepStmt, size_t packetSize,
        std::vector<std::string> &colNames, std::vector<RelationalRowData *> &data) const;

    int GetTableReference(const std::string &tableName,
        std::map<std::string, std::vector<TableReferenceProperty>> &reference);

    std::pair<std::string, int> GetSourceTableName(const std::string &tableName);

    std::pair<std::string, int> GetSharedTargetTableName(const std::string &tableName);
    // put
    int PutSyncData(const QueryObject &object, std::vector<DataItem> &dataItems, const std::string &deviceName);
    int SaveSyncDataItems(const QueryObject &object, std::vector<DataItem> &dataItems, const std::string &deviceName);
    void FilterChangeDataByDetailsType(ChangedData &changedData, uint32_t type);
    StoreInfo GetStoreInfo() const;

    bool IsCurrentLogicDelete() const;

    int UpsertDataInner(SQLiteSingleVerRelationalStorageExecutor *handle, const std::string &tableName,
        const std::vector<VBucket> &records);

    int UpsertDataInTransaction(SQLiteSingleVerRelationalStorageExecutor *handle, const std::string &tableName,
        const std::vector<VBucket> &records);

    int GetCloudTableWithoutShared(std::vector<TableSchema> &tables);

    int GetCompensatedSyncQueryInner(SQLiteSingleVerRelationalStorageExecutor *handle,
        const std::vector<TableSchema> &tables, std::vector<QuerySyncObject> &syncQuery);

    int GetSyncQueryByPk(const std::string &tableName, const std::vector<VBucket> &data,
        QuerySyncObject &querySyncObject);

    void FillQueryInKeys(const std::string &col, const std::vector<Type> &data, size_t valueType, Query &query);

    int CreateTempSyncTriggerInner(SQLiteSingleVerRelationalStorageExecutor *handle, const std::string &tableName);

    bool CheckTableSupportCompensatedSync(const TableSchema &table);

    void ExecuteDataChangeCallback(
        const std::pair<uint64_t, std::map<const StoreObserver *, RelationalObserverAction>> &item, int &observerCnt,
        const std::string &deviceName, const ChangedData &changedData, bool isChangedData);
    // data
    std::shared_ptr<SQLiteSingleRelationalStorageEngine> storageEngine_ = nullptr;
    std::function<void()> onSchemaChanged_;
    mutable std::mutex onSchemaChangedMutex_;
    std::mutex dataChangeDeviceMutex_;
    std::map<uint64_t, std::map<const StoreObserver *, RelationalObserverAction>> dataChangeCallbackMap_;
    std::function<void()> heartBeatListener_;
    mutable std::mutex heartBeatMutex_;

    LruMap<std::string, std::string> remoteDeviceSchema_;
    StorageExecutor *reusedHandle_;
    mutable std::mutex reusedHandleMutex_;

    // cache securityOption
    mutable std::mutex securityOptionMutex_;
    mutable SecurityOption securityOption_;
    mutable bool isCachedOption_;

    SQLiteSingleVerRelationalStorageExecutor *transactionHandle_ = nullptr;
    mutable std::shared_mutex transactionMutex_; // used for transaction

    SchemaMgr schemaMgr_;
    mutable std::shared_mutex schemaMgrMutex_;
    std::shared_ptr<SyncAbleEngine> syncAbleEngine_ = nullptr;

    std::atomic<bool> logicDelete_ = false;
    std::atomic<bool> allowLogicDelete_ = false;

    std::function<void (void)> syncFinishFunc_;
    std::function<void (void)> uploadStartFunc_;
};
}  // namespace DistributedDB
#endif
#endif // RELATIONAL_SYNC_ABLE_STORAGE_H