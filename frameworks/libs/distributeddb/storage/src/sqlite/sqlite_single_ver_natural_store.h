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
#ifndef SQLITE_SINGLE_VER_NATURAL_STORE_H
#define SQLITE_SINGLE_VER_NATURAL_STORE_H
#include <atomic>
#include <mutex>

#include "isyncer.h"
#include "kv_storage_handle.h"
#include "kv_store_nb_conflict_data_impl.h"
#include "runtime_context.h"
#include "single_ver_natural_store.h"
#include "single_ver_natural_store_commit_notify_data.h"
#include "sqlite_cloud_kv_store.h"
#include "sqlite_single_ver_continue_token.h"
#include "sqlite_single_ver_storage_engine.h"
#include "sqlite_utils.h"

namespace DistributedDB {
class SQLiteSingleVerNaturalStore : public SingleVerNaturalStore, public KvStorageHandle {
public:
    SQLiteSingleVerNaturalStore();
    ~SQLiteSingleVerNaturalStore() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteSingleVerNaturalStore);

    // Open the database
    int Open(const KvDBProperties &kvDBProp) override;

    // Invoked automatically when connection count is zero
    void Close() override;

    // Create a connection object.
    GenericKvDBConnection *NewConnection(int &errCode) override;

    // Get interface type of this kvdb.
    int GetInterfaceType() const override;

    // Get the interface ref-count, in order to access asynchronously.1
    void IncRefCount() override;

    // Drop the interface ref-count.
    void DecRefCount() override;

    // Get the identifier of this kvdb.
    std::vector<uint8_t> GetIdentifier() const override;
    // Get the dual tuple identifier of this kvdb.
    std::vector<uint8_t> GetDualTupleIdentifier() const override;

    // Get interface for syncer.
    IKvDBSyncInterface *GetSyncInterface() override;

    int GetMetaData(const Key &key, Value &value) const override;

    int PutMetaData(const Key &key, const Value &value, bool isInTransaction) override;

    // Delete multiple meta data records in a transaction.
    int DeleteMetaData(const std::vector<Key> &keys) override;
    // Delete multiple meta data records with key prefix in a transaction.
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const override;

    int GetAllMetaKeys(std::vector<Key> &keys) const override;

    int GetSyncData(Timestamp begin, Timestamp end, std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const override;

    int GetSyncData(Timestamp begin, Timestamp end, std::vector<SingleVerKvEntry *> &entries,
        ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const override;

    int GetSyncData(QueryObject &query, const SyncTimeRange &timeRange, const DataSizeSpecInfo &dataSizeInfo,
        ContinueToken &continueStmtToken, std::vector<SingleVerKvEntry *> &entries) const override;

    int GetSyncDataNext(std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const override;

    int GetSyncDataNext(std::vector<SingleVerKvEntry *> &entries, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const override;

    void ReleaseContinueToken(ContinueToken &continueStmtToken) const override;

    int PutSyncDataWithQuery(const QueryObject &query, const std::vector<SingleVerKvEntry *> &entries,
        const std::string &deviceName) override;

    void GetMaxTimestamp(Timestamp &stamp) const override;

    int Rekey(const CipherPassword &passwd) override;

    int Export(const std::string &filePath, const CipherPassword &passwd) override;

    int Import(const std::string &filePath, const CipherPassword &passwd) override;

    // In sync procedure, call this function
    int RemoveDeviceData(const std::string &deviceName, bool isNeedNotify) override;

    // In local procedure, call this function
    int RemoveDeviceData(const std::string &deviceName, bool isNeedNotify, bool isInSync);

    // remove device data for cloud
    int RemoveDeviceData(const std::string &deviceName, ClearMode mode);

    // remove device data for cloud and user
    int RemoveDeviceData(const std::string &deviceName, const std::string &user, ClearMode mode);
    SQLiteSingleVerStorageExecutor *GetHandle(bool isWrite, int &errCode,
        OperatePerm perm = OperatePerm::NORMAL_PERM) const;

    void ReleaseHandle(SQLiteSingleVerStorageExecutor *&handle) const;

    int TransObserverTypeToRegisterFunctionType(int observerType, RegisterFuncType &type) const override;

    int TransConflictTypeToRegisterFunctionType(int conflictType, RegisterFuncType &type) const override;

    bool CheckWritePermission() const override;

    SchemaObject GetSchemaInfo() const override;

    bool CheckCompatible(const std::string &schema, uint8_t type) const override;

    Timestamp GetCurrentTimestamp() override;

    SchemaObject GetSchemaObject() const;

    const SchemaObject &GetSchemaObjectConstRef() const;

    const KvDBProperties &GetDbProperties() const override;

    int GetKvDBSize(const KvDBProperties &properties, uint64_t &size) const override;
    KvDBProperties &GetDbPropertyForUpdate();

    int InitDatabaseContext(const KvDBProperties &kvDBProp, bool isNeedUpdateSecOpt = false);

    int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier);

    int SetAutoLifeCycleTime(uint32_t time);

    int GetSecurityOption(SecurityOption &option) const override;

    bool IsDataMigrating() const override;

    void SetConnectionFlag(bool isExisted) const override;

    int TriggerToMigrateData() const;

    int CheckValueAndAmendIfNeed(ValueSource sourceType, const Value &oriValue, Value &amendValue,
        bool &useAmendValue) const;

    int CheckReadDataControlled() const;
    bool IsCacheDBMode() const;
    bool IsExtendedCacheDBMode() const;

    void IncreaseCacheRecordVersion() const;
    uint64_t GetCacheRecordVersion() const;
    uint64_t GetAndIncreaseCacheRecordVersion() const;

    void NotifyRemotePushFinished(const std::string &targetId) const override;

    int GetDatabaseCreateTimestamp(Timestamp &outTime) const override;

    int CheckIntegrity() const override;

    int GetCompressionOption(bool &needCompressOnSync, uint8_t &compressionRate) const override;
    int GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const override;

    // Check and init query object for query sync and subscribe, flatbuffer schema will always return E_NOT_SUPPORT.
    // return E_OK if subscribe is legal, ERROR on exception.
    int CheckAndInitQueryCondition(QueryObject &query) const override;

    int InterceptData(std::vector<SingleVerKvEntry *> &entries, const std::string &sourceID,
        const std::string &targetID) const override;

    void SetDataInterceptor(const PushDataInterceptor &interceptor) override;

    int AddSubscribe(const std::string &subscribeId, const QueryObject &query, bool needCacheSubscribe) override;

    int RemoveSubscribe(const std::string &subscribeId) override;

    int RemoveSubscribe(const std::vector<std::string> &subscribeIds) override;

    int SetMaxLogSize(uint64_t limit);

    uint64_t GetMaxLogSize() const;

    void Dump(int fd) override;

    int IsSupportSubscribe() const override;

    void AbortHandle() override;

    void EnableHandle() override;

    int TryHandle() const override;

    std::pair<int, SQLiteSingleVerStorageExecutor*> GetStorageExecutor(bool isWrite) override;

    void RecycleStorageExecutor(SQLiteSingleVerStorageExecutor *executor) override;

    TimeOffset GetLocalTimeOffsetForCloud() override;

    int SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema);

    int RegisterObserverAction(const KvStoreObserver *observer, const ObserverAction &action);

    int UnRegisterObserverAction(const KvStoreObserver *observer);

    int GetCloudVersion(const std::string &device, std::map<std::string, std::string> &versionMap);
protected:
    void AsyncDataMigration(SQLiteSingleVerStorageEngine *storageEngine) const;

    void ReleaseResources();

    ICloudSyncStorageInterface *GetICloudSyncInterface() const override;
private:

    int CheckDatabaseRecovery(const KvDBProperties &kvDBProp);

    int RegisterNotification();

    int SaveSyncDataItems(const QueryObject &query, std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo,
        bool checkValueContent);

    int InitStorageEngine(const KvDBProperties &kvDBProp, bool isNeedUpdateSecOpt);

    void InitialLocalDataTimestamp();

    int GetSchema(SchemaObject &schema) const;

    static void InitDataBaseOption(const KvDBProperties &kvDBProp, OpenDbProperties &option);

    static int SetUserVer(const KvDBProperties &kvDBProp, int version);

    void NotifyRemovedData(std::vector<Entry> &entries);

    // Decide read only based on schema situation
    int DecideReadOnlyBaseOnSchema(const KvDBProperties &kvDBProp, bool &isReadOnly,
        SchemaObject &savedSchemaObj) const;

    void HeartBeatForLifeCycle() const;

    int StartLifeCycleTimer(const DatabaseLifeCycleNotifier &notifier) const;

    int ResetLifeCycleTimer() const;

    int StopLifeCycleTimer() const;
    void InitConflictNotifiedFlag(SingleVerNaturalStoreCommitNotifyData *committedData);

    // Change value that should be amended, and neglect value that is incompatible
    void CheckAmendValueContentForSyncProcedure(std::vector<DataItem> &dataItems) const;

    int RemoveDeviceDataInCacheMode(const std::string &hashDev, bool isNeedNotify) const;

    int RemoveDeviceDataNormally(const std::string &hashDev, bool isNeedNotify);

    int SaveSyncDataToMain(const QueryObject &query, std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo);

    // Currently, this function only suitable to be call from sync in insert_record_from_sync procedure
    // Take attention if future coder attempt to call it in other situation procedure
    int SaveSyncItems(const QueryObject& query, std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo,
        Timestamp &maxTimestamp, SingleVerNaturalStoreCommitNotifyData *commitData) const;

    int SaveSyncDataToCacheDB(const QueryObject &query, std::vector<DataItem> &dataItems,
        const DeviceInfo &deviceInfo);

    int SaveSyncItemsInCacheMode(SQLiteSingleVerStorageExecutor *handle, const QueryObject &query,
        std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo, Timestamp &maxTimestamp) const;

    int GetSyncDataForQuerySync(std::vector<DataItem> &dataItems, SQLiteSingleVerContinueToken *&continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const;

    int SaveCreateDBTime();
    int SaveCreateDBTimeIfNotExisted();

    virtual int GetAndInitStorageEngine(const KvDBProperties &kvDBProp);

    int RemoveAllSubscribe();

    int GetExistsDeviceList(std::set<std::string> &devices) const;

    int RemoveDeviceDataInner(const std::string &hashDev, bool isNeedNotify);

    int RemoveDeviceDataInner(const std::string &hashDev, ClearMode mode);

    int RemoveDeviceDataInner(const std::string &hashDev, const std::string &user, ClearMode mode);

    void GetAndResizeLocalIdentity(std::string &outTarget) const;

    DECLARE_OBJECT_TAG(SQLiteSingleVerNaturalStore);

    mutable std::shared_mutex engineMutex_;
    SQLiteSingleVerStorageEngine *storageEngine_;

    bool notificationEventsRegistered_;
    bool notificationConflictEventsRegistered_;
    bool isInitialized_;
    bool isReadOnly_;
    mutable std::mutex initialMutex_;
    mutable std::mutex lifeCycleMutex_;
    mutable DatabaseLifeCycleNotifier lifeCycleNotifier_;
    mutable TimerId lifeTimerId_;
    uint32_t autoLifeTime_;
    mutable Timestamp createDBTime_;
    mutable std::mutex createDBTimeMutex_;

    mutable std::shared_mutex dataInterceptorMutex_;
    PushDataInterceptor dataInterceptor_;
    std::atomic<uint64_t> maxLogSize_;

    mutable std::shared_mutex abortHandleMutex_;
    OperatePerm abortPerm_;

    mutable std::mutex cloudStoreMutex_;
    SqliteCloudKvStore *sqliteCloudKvStore_;
};
} // namespace DistributedDB
#endif // SQLITE_SINGLE_VER_NATURAL_STORE_H
