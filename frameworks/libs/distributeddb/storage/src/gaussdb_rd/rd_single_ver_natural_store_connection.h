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

#ifndef RD_SINGLE_VER_NATURAL_STORE_CONNECTION_H
#define RD_SINGLE_VER_NATURAL_STORE_CONNECTION_H
#include <atomic>

#include "db_common.h"
#include "db_types.h"
#include "rd_single_ver_natural_store.h"
#include "rd_single_ver_storage_executor.h"
#include "runtime_context.h"
#include "sync_able_kvdb_connection.h"
#include "single_ver_natural_store_connection.h"

namespace DistributedDB {
class SQLiteSingleVerNaturalStore;

class RdSingleVerNaturalStoreConnection : public SingleVerNaturalStoreConnection {
public:
    explicit RdSingleVerNaturalStoreConnection(RdSingleVerNaturalStore *kvDB);
    ~RdSingleVerNaturalStoreConnection() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(RdSingleVerNaturalStoreConnection);

    // Get the value from the database
    int Get(const IOption &option, const Key &key, Value &value) const override;

    // Clear all the data from the database
    int Clear(const IOption &option) override;

    // Get all the data from the database
    int GetEntries(const IOption &option, const Key &keyPrefix, std::vector<Entry> &entries) const override;

    int GetEntries(const IOption &option, const Query &query, std::vector<Entry> &entries) const override;

    int GetCount(const IOption &option, const Query &query, int &count) const override;

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

    int GetSyncDataSize(const std::string &device, size_t &size) const override;

private:
    int GetEntriesInner(bool isGetValue, const IOption &option,
        const Key &keyPrefix, std::vector<Entry> &entries) const;

    int GetEntriesInner(const IOption &option, const Query &query,
        std::vector<Entry> &entries) const;

    static int CheckOption(const IOption &option, SingleVerDataType &type);
    int ForceCheckPoint() const;
    mutable std::mutex kvDbResultSetsMutex_;
    mutable std::set<IKvDBResultSet *> kvDbResultSets_;

    // ResultSet Related Info
    static constexpr std::size_t MAX_RESULTSET_SIZE = 8; // Max 8 ResultSet At The Same Time

    int CheckSyncEntriesValid(const std::vector<Entry> &entries) const override;

    int PutBatchInner(const IOption &option, const std::vector<Entry> &entries) override;

    int SaveSyncEntries(const std::vector<Entry> &entries, bool isDelete);

    // This func currently only be called in local procedure to change sync_data table, do not use in sync procedure.
    // It will check and amend value when need if it is a schema database. return error if some value disagree with the
    // schema. But in sync procedure, we just neglect the value that disagree with schema.
    int SaveEntry(const Entry &entry, bool isDelete, Timestamp timestamp = 0);

    int SaveEntryNormally(const Entry &entry, bool isDelete);

    RdSingleVerStorageExecutor *GetExecutor(bool isWrite, int &errCode) const;

    void ReleaseExecutor(RdSingleVerStorageExecutor *&executor) const;

    void ReleaseCommitData(SingleVerNaturalStoreCommitNotifyData *&committedData);

    int StartTransactionInner(TransactType transType = TransactType::DEFERRED);

    int StartTransactionNormally(TransactType transType = TransactType::DEFERRED);

    int CommitInner();

    int RollbackInner();

    void CommitAndReleaseNotifyData(SingleVerNaturalStoreCommitNotifyData *&committedData,
        bool isNeedCommit, int eventType);

    int CheckReadDataControlled() const;

    int CheckSyncKeysValid(const std::vector<Key> &keys) const override;

    int DeleteBatchInner(const IOption &option, const std::vector<Key> &keys) override;

    int DeleteSyncEntries(const std::vector<Key> &keys);

    bool IsSinglePutOrDelete(const std::vector<Entry> &entries)
    {
        return entries.size() == 1;
    }

    SingleVerNaturalStoreCommitNotifyData *committedData_; // used for transaction

    mutable std::mutex transactionMutex_;
    RdSingleVerStorageExecutor *writeHandle_; // only existed while in transaction.
};
}
#endif