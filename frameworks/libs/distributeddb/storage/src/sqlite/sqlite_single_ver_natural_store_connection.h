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

#ifndef SQLITE_SINGLE_VER_NATURAL_STORE_CONNECTION_H
#define SQLITE_SINGLE_VER_NATURAL_STORE_CONNECTION_H
#include <atomic>
#include "single_ver_natural_store_connection.h"
#include "sync_able_kvdb_connection.h"
#include "sqlite_single_ver_storage_executor.h"
#include "db_types.h"
#include "runtime_context.h"

namespace DistributedDB {
class SQLiteSingleVerNaturalStore;

class SQLiteSingleVerNaturalStoreConnection : public SingleVerNaturalStoreConnection {
public:
    explicit SQLiteSingleVerNaturalStoreConnection(SQLiteSingleVerNaturalStore *kvDB);
    ~SQLiteSingleVerNaturalStoreConnection() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteSingleVerNaturalStoreConnection);

    // Get the value from the database
    int Get(const IOption &option, const Key &key, Value &value) const override;

    // Clear all the data from the database
    int Clear(const IOption &option) override;

    // Get all the data from the database
    int GetEntries(const IOption &option, const Key &keyPrefix, std::vector<Entry> &entries) const override;

    int GetEntries(const IOption &option, const Query &query, std::vector<Entry> &entries) const override;

    int GetCount(const IOption &option, const Query &query, int &count) const override;

    // Put the batch values to the database.
    int PutBatch(const IOption &option, const std::vector<Entry> &entries) override;

    // Delete the batch values from the database.
    int DeleteBatch(const IOption &option, const std::vector<Key> &keys) override;

    // Get the snapshot
    int GetSnapshot(IKvDBSnapshot *&snapshot) const override;

    // Release the created snapshot
    void ReleaseSnapshot(IKvDBSnapshot *&snapshot) override;

    // Start the transaction
    int StartTransaction() override;

    // Commit the transaction
    int Commit() override;

    // Roll back the transaction
    int RollBack() override;

    // Check if the transaction already started manually
    bool IsTransactionStarted() const override;

    // Pragma interface.
    int Pragma(int cmd, void *parameter) override;

    // Parse event types(from observer mode).
    int TranslateObserverModeToEventTypes(unsigned mode, std::list<int> &eventTypes) const override;

    // Register a conflict notifier.
    int SetConflictNotifier(int conflictType, const KvDBConflictAction &action) override;

    int Rekey(const CipherPassword &passwd) override;

    int Export(const std::string &filePath, const CipherPassword &passwd) override;

    int Import(const std::string &filePath, const CipherPassword &passwd) override;

    // Get the result set
    int GetResultSet(const IOption &option, const Key &keyPrefix, IKvDBResultSet *&resultSet) const override;

    int GetResultSet(const IOption &option, const Query &query, IKvDBResultSet *&resultSet) const override;

    // Release the result set
    void ReleaseResultSet(IKvDBResultSet *&resultSet) override;

    int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier) override;

    // Called when Close and delete the connection.
    int PreClose() override;

    int CheckIntegrity() const override;

    int GetKeys(const IOption &option, const Key &keyPrefix, std::vector<Key> &keys) const override;

    int UpdateKey(const UpdateKeyCallback &callback) override;

    int SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema) override;

    int RegisterObserverAction(const KvStoreObserver *observer, const ObserverAction &action) override;

    int UnRegisterObserverAction(const KvStoreObserver *observer) override;

    int RemoveDeviceData(const std::string &device, ClearMode mode) override;

    int RemoveDeviceData(const std::string &device, const std::string &user, ClearMode mode) override;

    int GetCloudVersion(const std::string &device, std::map<std::string, std::string> &versionMap) override;
private:
    int CheckMonoStatus(OperatePerm perm);

    int GetDeviceIdentifier(PragmaEntryDeviceIdentifier *identifier);

    void ClearConflictNotifierCount();

    int PutBatchInner(const IOption &option, const std::vector<Entry> &entries) override;
    int DeleteBatchInner(const IOption &option, const std::vector<Key> &keys) override;

    int SaveSyncEntries(const std::vector<Entry> &entries);
    int SaveLocalEntries(const std::vector<Entry> &entries);
    int DeleteSyncEntries(const std::vector<Key> &keys);
    int DeleteLocalEntries(const std::vector<Key> &keys);

    int SaveEntry(const Entry &entry, bool isDelete, Timestamp timestamp = 0);

    int CheckDataStatus(const Key &key, const Value &value, bool isDelete) const;

    int CheckWritePermission() const override;

    int CheckSyncEntriesValid(const std::vector<Entry> &entries) const override;

    int CheckSyncKeysValid(const std::vector<Key> &keys) const override;

    int CheckLocalEntriesValid(const std::vector<Entry> &entries) const;

    int CheckLocalKeysValid(const std::vector<Key> &keys) const;

    void CommitAndReleaseNotifyData(SingleVerNaturalStoreCommitNotifyData *&committedData,
        bool isNeedCommit, int eventType);

    int StartTransactionInner(TransactType transType = TransactType::DEFERRED);

    int CommitInner();

    int RollbackInner();

    int PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
        const KvStoreNbPublishAction &onConflict);

    int PublishLocalCallback(bool updateTimestamp, const SingleVerRecord &localRecord,
        const SingleVerRecord &syncRecord, const KvStoreNbPublishAction &onConflict);

    int PublishInner(SingleVerNaturalStoreCommitNotifyData *committedData, bool updateTimestamp,
        SingleVerRecord &localRecord, SingleVerRecord &syncRecord, bool &isNeedCallback);

    int UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp);

    int UnpublishInner(SingleVerNaturalStoreCommitNotifyData *&committedData, const SingleVerRecord &syncRecord,
        bool updateTimestamp, int &innerErrCode);

    int UnpublishOper(SingleVerNaturalStoreCommitNotifyData *&committedData, const SingleVerRecord &syncRecord,
        bool updateTimestamp, int operType);

    void ReleaseCommitData(SingleVerNaturalStoreCommitNotifyData *&committedData);

    int PragmaPublish(void *parameter);

    int PragmaUnpublish(void *parameter);

    SQLiteSingleVerStorageExecutor *GetExecutor(bool isWrite, int &errCode) const;

    void ReleaseExecutor(SQLiteSingleVerStorageExecutor *&executor) const;

    int PragmaSetAutoLifeCycle(const uint32_t *lifeTime);
    void InitConflictNotifiedFlag();
    void AddConflictNotifierCount(int target);
    void ResetConflictNotifierCount(int target);

    int PragmaResultSetCacheMode(PragmaData inMode);
    int PragmaResultSetCacheMaxSize(PragmaData inSize);

    // use for getkvstore migrating cache data
    int PragmaTriggerToMigrateData(const SecurityOption &secOption) const;
    int CheckAmendValueContentForLocalProcedure(const Value &oriValue, Value &amendValue) const;

    int SaveLocalEntry(const Entry &entry, bool isDelete);
    int SaveLocalItem(const LocalDataItem &dataItem) const;
    int SaveLocalItemInCacheMode(const LocalDataItem &dataItem) const;
    int SaveEntryNormally(DataItem &dataItem);
    int SaveEntryInCacheMode(DataItem &dataItem, uint64_t recordVersion);

    int StartTransactionInCacheMode(TransactType transType = TransactType::DEFERRED);
    int StartTransactionNormally(TransactType transType = TransactType::DEFERRED);

    bool IsCacheDBMode() const;
    bool IsExtendedCacheDBMode() const override;
    int CheckReadDataControlled() const;
    bool IsFileAccessControlled() const;

    int PragmaSetMaxLogSize(uint64_t *limit);
    int ForceCheckPoint() const;

    bool CheckLogOverLimit(SQLiteSingleVerStorageExecutor *executor) const;
    int CalcHashDevID(PragmaDeviceIdentifier &pragmaDev);

    int GetEntriesInner(bool isGetValue, const IOption &option,
        const Key &keyPrefix, std::vector<Entry> &entries) const;

    DECLARE_OBJECT_TAG(SQLiteSingleVerNaturalStoreConnection);

    // ResultSet Related Info
    static constexpr std::size_t MAX_RESULT_SET_SIZE = 8; // Max 8 ResultSet At The Same Time
    std::atomic<ResultSetCacheMode> cacheModeForNewResultSet_{ResultSetCacheMode::CACHE_FULL_ENTRY};
    std::atomic<int> cacheMaxSizeForNewResultSet_{0}; // Will be init to default value in constructor

    int conflictType_;
    uint32_t transactionEntryLen_; // used for transaction
    Timestamp currentMaxTimestamp_; // used for transaction
    SingleVerNaturalStoreCommitNotifyData *committedData_; // used for transaction
    SingleVerNaturalStoreCommitNotifyData *localCommittedData_;
    std::atomic<bool> transactionExeFlag_;

    NotificationChain::Listener *conflictListener_;
    SQLiteSingleVerStorageExecutor *writeHandle_; // only existed while in transaction.
    mutable std::set<IKvDBResultSet *> kvDbResultSets_;
    std::mutex conflictMutex_;
    std::mutex rekeyMutex_;
    std::mutex importMutex_;
    mutable std::mutex kvDbResultSetsMutex_;
    mutable std::mutex transactionMutex_; // used for transaction
};
}

#endif