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
#include "rd_single_ver_natural_store_connection.h"
#include "rd_single_ver_result_set.h"

#include <algorithm>

#include "db_common.h"
#include "db_constant.h"
#include "db_dfx_adapter.h"
#include "db_errno.h"
#include "get_query_info.h"
#include "kvdb_observer_handle.h"
#include "kvdb_pragma.h"
#include "log_print.h"
#include "store_types.h"
#include "sqlite_single_ver_storage_engine.h"

namespace DistributedDB {

RdSingleVerNaturalStoreConnection::RdSingleVerNaturalStoreConnection(RdSingleVerNaturalStore *kvDB)
    : SingleVerNaturalStoreConnection(kvDB), committedData_(nullptr), writeHandle_(nullptr)
{
    LOGD("RdSingleVerNaturalStoreConnection Created");
}

RdSingleVerNaturalStoreConnection::~RdSingleVerNaturalStoreConnection()
{
}

int RdSingleVerNaturalStoreConnection::GetEntries(const IOption &option, const Key &keyPrefix,
    std::vector<Entry> &entries) const
{
    if (option.dataType != IOption::SYNC_DATA) {
        LOGE("[RdSingleVerStorageExecutor][GetEntries] IOption only support SYNC_DATA type.");
        return -E_NOT_SUPPORT;
    }
    return GetEntriesInner(true, option, keyPrefix, entries);
}

int RdSingleVerNaturalStoreConnection::CheckOption(const IOption &option, SingleVerDataType &type)
{
    if (option.dataType == IOption::SYNC_DATA) {
        type = SingleVerDataType::SYNC_TYPE;
    } else {
        LOGE("IOption only support SYNC_DATA type.");
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

int RdSingleVerNaturalStoreConnection::GetEntriesInner(bool isGetValue, const IOption &option,
    const Key &keyPrefix, std::vector<Entry> &entries) const
{
    if (keyPrefix.size() > DBConstant::MAX_KEY_SIZE) {
        return -E_INVALID_ARGS;
    }

    SingleVerDataType type;
    int errCode = CheckOption(option, type);
    if (errCode != E_OK) {
        return errCode;
    }

    DBDfxAdapter::StartTracing();
    {
        std::lock_guard<std::mutex> lock(transactionMutex_);
        if (writeHandle_ != nullptr) {
            LOGD("[RdSingleVerNaturalStoreConnection] Transaction started already.");
            errCode = writeHandle_->GetEntries(KV_SCAN_PREFIX, std::pair<Key, Key>(keyPrefix, {}), entries);
            DBDfxAdapter::FinishTracing();
            return errCode;
        }
    }
    RdSingleVerStorageExecutor *handle = GetExecutor(false, errCode);
    if (handle == nullptr) {
        LOGE("[RdSingleVerNaturalStoreConnection]::[GetEntries] Get executor failed, errCode = [%d]", errCode);
        DBDfxAdapter::FinishTracing();
        return errCode;
    }
    errCode = handle->GetEntries(KV_SCAN_PREFIX, std::pair<Key, Key>(keyPrefix, {}), entries);
    ReleaseExecutor(handle);
    DBDfxAdapter::FinishTracing();
    return errCode;
}

int RdSingleVerNaturalStoreConnection::GetResultSet(const IOption &option, const Key &keyPrefix,
    IKvDBResultSet *&resultSet) const
{
    // maximum of result set size is 4
    std::lock_guard<std::mutex> lock(kvDbResultSetsMutex_);
    if (kvDbResultSets_.size() >= MAX_RESULTSET_SIZE) {
        LOGE("Over max result set size");
        return -E_MAX_LIMITS;
    }

    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        return -E_INVALID_DB;
    }

    RdSingleVerResultSet *tmpResultSet = new (std::nothrow) RdSingleVerResultSet(naturalStore, keyPrefix);
    if (tmpResultSet == nullptr) {
        LOGE("Create single version result set failed.");
        return -E_OUT_OF_MEMORY;
    }
    int errCode = tmpResultSet->Open(false);
    if (errCode != E_OK) {
        delete tmpResultSet;
        resultSet = nullptr;
        tmpResultSet = nullptr;
        LOGE("Open result set failed.");
        return errCode;
    }
    resultSet = (IKvDBResultSet *)tmpResultSet;
    kvDbResultSets_.insert(resultSet);
    return E_OK;
}

static void PrintResultsetKeys(const QueryExpression &queryExpression)
{
    std::vector<uint8_t> beginKeyVec = queryExpression.GetBeginKey();
    std::vector<uint8_t> endKeyVec = queryExpression.GetEndKey();
    std::string beginKey;
    std::string endKey;
    if (beginKeyVec.size() == 0) {
        beginKey = "NULL";
    } else {
        beginKey.assign(beginKeyVec.begin(), beginKeyVec.end());
    }
    if (endKeyVec.size() == 0) {
        endKey = "NULL";
    } else {
        endKey.assign(endKeyVec.begin(), endKeyVec.end());
    }

    LOGD("begin key: %s, end key: %s", beginKey.c_str(), endKey.c_str());
}

int RdSingleVerNaturalStoreConnection::GetResultSet(const IOption &option,
    const Query &query, IKvDBResultSet *&resultSet) const
{
    // maximum of result set size is 4
    std::lock_guard<std::mutex> lock(kvDbResultSetsMutex_);
    if (kvDbResultSets_.size() >= MAX_RESULTSET_SIZE) {
        LOGE("Over max result set size");
        return -E_MAX_LIMITS;
    }

    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        return -E_INVALID_DB;
    }
    QueryExpression queryExpression = GetQueryInfo::GetQueryExpression(query);
    int errCode = GetQueryInfo::GetQueryExpression(query).RangeParamCheck();
    if (errCode != E_OK) {
        return errCode;
    }
    RdSingleVerResultSet *tmpResultSet = new (std::nothrow) RdSingleVerResultSet(naturalStore,
        queryExpression.GetBeginKey(), queryExpression.GetEndKey(), KV_SCAN_RANGE);
    if (tmpResultSet == nullptr) {
        LOGE("Create single version result set failed.");
        return -E_OUT_OF_MEMORY;
    }
    errCode = tmpResultSet->Open(false);
    if (errCode != E_OK) {
        delete tmpResultSet;
        resultSet = nullptr;
        tmpResultSet = nullptr;
        LOGE("Open result set failed.");
        return errCode;
    }
    resultSet = (IKvDBResultSet *)tmpResultSet;
    kvDbResultSets_.insert(resultSet);
    return E_OK;
}


void RdSingleVerNaturalStoreConnection::ReleaseResultSet(IKvDBResultSet *&resultSet)
{
    if (resultSet == nullptr) {
        return;
    }
    RdSingleVerResultSet *tmpResultSet = (RdSingleVerResultSet *)resultSet;
    int errCode = tmpResultSet->Close();
    if (errCode != E_OK) {
        LOGE("Open result set failed.");
        return;
    }
    std::lock_guard<std::mutex> lock(kvDbResultSetsMutex_);
    kvDbResultSets_.erase(resultSet);
    delete resultSet;
    resultSet = nullptr;
    return;
}

int RdSingleVerNaturalStoreConnection::Pragma(int cmd, void *parameter)
{
    switch (cmd) {
        case PRAGMA_EXEC_CHECKPOINT:
            return ForceCheckPoint();
        default:
            break;
    }
    LOGD("Rd Pragma only support check point for now:%d", cmd);
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::TranslateObserverModeToEventTypes(
    unsigned mode, std::list<int> &eventTypes) const
{
    int errCode = E_OK;
    switch (mode) {
        case static_cast<unsigned>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT):
            eventTypes.push_back(static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
            break;
        default:
            errCode = -E_NOT_SUPPORT;
            break;
    }
    return errCode;
}

int RdSingleVerNaturalStoreConnection::ForceCheckPoint() const
{
    int errCode = E_OK;
    RdSingleVerStorageExecutor *handle = GetExecutor(false, errCode);
    if (handle == nullptr) {
        LOGE("[Connection]::[GetEntries] Get executor failed, errCode = [%d]", errCode);
        return errCode;
    }

    errCode = handle->ForceCheckPoint();
    ReleaseExecutor(handle);
    return errCode;
}

void RdSingleVerNaturalStoreConnection::CommitAndReleaseNotifyData(
    SingleVerNaturalStoreCommitNotifyData *&committedData, bool isNeedCommit, int eventType)
{
    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if ((naturalStore != nullptr) && (committedData != nullptr)) {
        if (isNeedCommit) {
            if (!committedData->IsChangedDataEmpty()) {
                naturalStore->CommitNotify(eventType, committedData);
            }
        }
    }
    ReleaseCommitData(committedData);
}

int RdSingleVerNaturalStoreConnection::Get(const IOption &option, const Key &key, Value &value) const
{
    SingleVerDataType dataType;
    int errCode = CheckOption(option, dataType);
    if (errCode != E_OK) {
        return errCode;
    }

    if (key.size() > DBConstant::MAX_KEY_SIZE || key.empty()) {
        return -E_INVALID_ARGS;
    }

    errCode = CheckReadDataControlled();
    if (errCode != E_OK) {
        LOGE("[Get] Do not allowed to read data with errCode = [%d]!", errCode);
        return errCode;
    }

    DBDfxAdapter::StartTracing();
    {
        // need to check if the transaction started
        std::lock_guard<std::mutex> lock(transactionMutex_);
        if (writeHandle_ != nullptr) {
            Timestamp recordTimestamp;
            errCode = writeHandle_->GetKvData(dataType, key, value, recordTimestamp);
            if (errCode != E_OK) {
                LOGE("[RdSingleVerStorageExecutor][Get] Cannot get the data %d", errCode);
            }
            DBDfxAdapter::FinishTracing();
            return errCode;
        }
    }
    RdSingleVerStorageExecutor *handle = GetExecutor(false, errCode);
    if (handle == nullptr) {
        DBDfxAdapter::FinishTracing();
        return errCode;
    }

    Timestamp timestamp;
    errCode = handle->GetKvData(dataType, key, value, timestamp);
    if (errCode != E_OK) {
        LOGE("[RdSingleVerStorageExecutor][Get] Cannot get the data %d", errCode);
    }
    ReleaseExecutor(handle);
    DBDfxAdapter::FinishTracing();
    return errCode;
}

// Clear all the data from the database
int RdSingleVerNaturalStoreConnection::Clear(const IOption &option)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::GetEntriesInner(const IOption &option, const Query &query,
    std::vector<Entry> &entries) const
{
    QueryExpression queryExpression = GetQueryInfo::GetQueryExpression(query);
    int errCode = GetQueryInfo::GetQueryExpression(query).RangeParamCheck();
    if (errCode != E_OK) {
        return errCode;
    }
    DBDfxAdapter::StartTracing();
    {
        std::lock_guard<std::mutex> lock(transactionMutex_);
        if (writeHandle_ != nullptr) {
            LOGD("[RdSingleVerNaturalStoreConnection] Transaction started already.");
            errCode = writeHandle_->GetEntries(KV_SCAN_RANGE, std::pair<Key, Key>(queryExpression.GetBeginKey(),
                queryExpression.GetEndKey()), entries);
            DBDfxAdapter::FinishTracing();
            return errCode;
        }
    }
    RdSingleVerStorageExecutor *handle = GetExecutor(false, errCode);
    if (handle == nullptr) {
        LOGE("[RdSingleVerNaturalStoreConnection]::[GetEntries] Get executor failed, errCode = [%d]", errCode);
        DBDfxAdapter::FinishTracing();
        return errCode;
    }

    errCode = handle->GetEntries(KV_SCAN_RANGE, std::pair<Key, Key>(queryExpression.GetBeginKey(),
        queryExpression.GetEndKey()), entries);
    ReleaseExecutor(handle);
    DBDfxAdapter::FinishTracing();
    if (errCode != -E_NOT_FOUND) {
        PrintResultsetKeys(queryExpression);
    }
    return errCode;
}
int RdSingleVerNaturalStoreConnection::GetEntries(const IOption &option, const Query &query,
    std::vector<Entry> &entries) const
{
    if (option.dataType != IOption::SYNC_DATA) {
        LOGE("[RdSingleVerStorageExecutor][GetEntries]unsupported data type");
        return -E_INVALID_ARGS;
    }
    return GetEntriesInner(option, query, entries);
}

int RdSingleVerNaturalStoreConnection::GetCount(const IOption &option, const Query &query, int &count) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::GetSnapshot(IKvDBSnapshot *&snapshot) const
{
    return -E_NOT_SUPPORT;
}

void RdSingleVerNaturalStoreConnection::ReleaseSnapshot(IKvDBSnapshot *&snapshot)
{
    return;
}

int RdSingleVerNaturalStoreConnection::StartTransaction()
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::Commit()
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::RollBack()
{
    return -E_NOT_SUPPORT;
}

bool RdSingleVerNaturalStoreConnection::IsTransactionStarted() const
{
    return false;
}

int RdSingleVerNaturalStoreConnection::Rekey(const CipherPassword &passwd)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::Export(const std::string &filePath, const CipherPassword &passwd)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::Import(const std::string &filePath, const CipherPassword &passwd)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier)
{
    return -E_NOT_SUPPORT;
}

// Called when Close and delete the connection.
int RdSingleVerNaturalStoreConnection::PreClose()
{
    // check if result set closed
    std::lock_guard<std::mutex> lock(kvDbResultSetsMutex_);
    if (kvDbResultSets_.size() > 0) {
        LOGE("The rd connection have [%zu] active result set, can not close.", kvDbResultSets_.size());
        return -E_BUSY;
    }
    // check if transaction closed
    {
        std::lock_guard<std::mutex> transactionLock(transactionMutex_);
        if (writeHandle_ != nullptr) {
            LOGW("Transaction started, rollback before close.");
            int errCode = RollbackInner();
            if (errCode != E_OK) {
                LOGE("cannot rollback %d.", errCode);
            }
            ReleaseExecutor(writeHandle_);
        }
    }
    return E_OK;
}

int RdSingleVerNaturalStoreConnection::CheckIntegrity() const
{
    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        LOGE("[SingleVerConnection] the store is null");
        return -E_NOT_INIT;
    }
    return naturalStore->CheckIntegrity();
}

int RdSingleVerNaturalStoreConnection::GetKeys(const IOption &option, const Key &keyPrefix,
    std::vector<Key> &keys) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::UpdateKey(const UpdateKeyCallback &callback)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerNaturalStoreConnection::CheckSyncEntriesValid(const std::vector<Entry> &entries) const
{
    if (entries.size() > DBConstant::MAX_BATCH_SIZE) {
        return -E_INVALID_ARGS;
    }

    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        return -E_INVALID_DB;
    }

    for (const auto &entry : entries) {
        int errCode = naturalStore->CheckDataStatus(entry.key, entry.value, false);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int RdSingleVerNaturalStoreConnection::PutBatchInner(const IOption &option, const std::vector<Entry> &entries)
{
    LOGD("PutBatchInner");
    std::lock_guard<std::mutex> lock(transactionMutex_);
    bool isAuto = false;
    int errCode = E_OK;
    if (option.dataType != IOption::SYNC_DATA) {
        LOGE("LOCAL_DATA TYPE NOT SUPPORT in RD executor");
        return -E_NOT_SUPPORT;
    }
    if (writeHandle_ == nullptr) {
        isAuto = true;
        errCode = StartTransactionInner(TransactType::IMMEDIATE);
        if (errCode != E_OK) {
            return errCode;
        }
    }

    errCode = SaveSyncEntries(entries, false);

    if (isAuto) {
        if (errCode == E_OK) {
            errCode = CommitInner();
        } else {
            int innerCode = RollbackInner();
            errCode = (innerCode != E_OK) ? innerCode : errCode;
        }
    }
    return errCode;
}

int RdSingleVerNaturalStoreConnection::SaveSyncEntries(const std::vector<Entry> &entries, bool isDelete)
{
    if (IsSinglePutOrDelete(entries)) {
        return SaveEntry(entries[0], isDelete);
    }
    return writeHandle_->BatchSaveEntries(entries, isDelete, committedData_);
}

// This function currently only be called in local procedure to change sync_data table, do not use in sync procedure.
// It will check and amend value when need if it is a schema database. return error if some value disagree with the
// schema. But in sync procedure, we just neglect the value that disagree with schema.
int RdSingleVerNaturalStoreConnection::SaveEntry(const Entry &entry, bool isDelete, Timestamp timestamp)
{
    LOGD("Saving Entry");
    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        LOGE("[RdSingleVerNaturalStoreConnection][SaveEntry] the store is null");
        return -E_INVALID_DB;
    }

    if (IsExtendedCacheDBMode()) {
        return -E_NOT_SUPPORT;
    } else {
        return SaveEntryNormally(entry, isDelete);
    }
}

int RdSingleVerNaturalStoreConnection::SaveEntryNormally(const Entry &entry, bool isDelete)
{
    int errCode = writeHandle_->SaveSyncDataItem(entry, committedData_, isDelete);
    if (errCode != E_OK) {
        LOGE("Save entry failed, err:%d", errCode);
    }
    return errCode;
}

RdSingleVerStorageExecutor *RdSingleVerNaturalStoreConnection::GetExecutor(bool isWrite, int &errCode) const
{
    LOGD("[RdSingleVerNaturalStoreConnection] Getting Executor ");
    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        errCode = -E_NOT_INIT;
        LOGE("[SingleVerConnection] the store is null");
        return nullptr;
    }
    return naturalStore->GetHandle(isWrite, errCode);
}

void RdSingleVerNaturalStoreConnection::ReleaseExecutor(RdSingleVerStorageExecutor *&executor) const
{
    kvDB_->ReEnableConnection(OperatePerm::NORMAL_WRITE);
    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore != nullptr) {
        naturalStore->ReleaseHandle(executor);
    }
}

void RdSingleVerNaturalStoreConnection::ReleaseCommitData(SingleVerNaturalStoreCommitNotifyData *&committedData)
{
    if (committedData != nullptr) {
        committedData->DecObjRef(committedData);
        committedData = nullptr;
    }
}

int RdSingleVerNaturalStoreConnection::StartTransactionInner(TransactType transType)
{
    if (IsExtendedCacheDBMode()) {
        return -E_NOT_SUPPORT;
    } else {
        return StartTransactionNormally(transType);
    }
}

int RdSingleVerNaturalStoreConnection::StartTransactionNormally(TransactType transType)
{
    int errCode = E_OK;
    RdSingleVerStorageExecutor *handle = GetExecutor(true, errCode);
    if (handle == nullptr) {
        return errCode;
    }

    errCode = kvDB_->TryToDisableConnection(OperatePerm::NORMAL_WRITE);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        LOGE("Start transaction failed, %d", errCode);
        return errCode;
    }

    if (committedData_ == nullptr) {
        committedData_ = new (std::nothrow) SingleVerNaturalStoreCommitNotifyData;
        if (committedData_ == nullptr) {
            ReleaseExecutor(handle);
            return -E_OUT_OF_MEMORY;
        }
    }

    errCode = handle->StartTransaction(transType);
    if (errCode != E_OK) {
        ReleaseExecutor(handle);
        ReleaseCommitData(committedData_);
        return errCode;
    }

    writeHandle_ = handle;
    return E_OK;
}

int RdSingleVerNaturalStoreConnection::CommitInner()
{
    int errCode = writeHandle_->Commit();
    ReleaseExecutor(writeHandle_);
    writeHandle_ = nullptr;

    CommitAndReleaseNotifyData(committedData_, true,
        static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
    return errCode;
}

int RdSingleVerNaturalStoreConnection::RollbackInner()
{
    int errCode = writeHandle_->Rollback();
    ReleaseCommitData(committedData_);
    ReleaseExecutor(writeHandle_);
    writeHandle_ = nullptr;
    return errCode;
}

int RdSingleVerNaturalStoreConnection::CheckReadDataControlled() const
{
    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        LOGE("[SingleVerConnection] natural store is nullptr in CheckReadDataControlled.");
        return E_OK;
    }
    return naturalStore->CheckReadDataControlled();
}

int RdSingleVerNaturalStoreConnection::CheckSyncKeysValid(const std::vector<Key> &keys) const
{
    if (keys.size() > DBConstant::MAX_BATCH_SIZE) {
        return -E_INVALID_ARGS;
    }

    RdSingleVerNaturalStore *naturalStore = GetDB<RdSingleVerNaturalStore>();
    if (naturalStore == nullptr) {
        return -E_INVALID_DB;
    }

    for (const auto &key : keys) {
        int errCode = naturalStore->CheckDataStatus(key, {}, true);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

int RdSingleVerNaturalStoreConnection::DeleteBatchInner(const IOption &option, const std::vector<Key> &keys)
{
    DBDfxAdapter::StartTracing();
    bool isAuto = false;
    int errCode = E_OK;
    if (option.dataType != IOption::SYNC_DATA) {
        LOGE("LOCAL_DATA TYPE NOT SUPPORT in RD executor");
        DBDfxAdapter::FinishTracing();
        return -E_NOT_SUPPORT;
    }
    std::lock_guard<std::mutex> lock(transactionMutex_);
    if (writeHandle_ == nullptr) {
        isAuto = true;
        errCode = StartTransactionInner(TransactType::IMMEDIATE);
        if (errCode != E_OK) {
            DBDfxAdapter::FinishTracing();
            return errCode;
        }
    }

    errCode = DeleteSyncEntries(keys);

    if (isAuto) {
        if (errCode == E_OK) {
            errCode = CommitInner();
        } else {
            int innerCode = RollbackInner();
            errCode = (innerCode != E_OK) ? innerCode : errCode;
        }
    }
    DBDfxAdapter::FinishTracing();
    return errCode;
}

int RdSingleVerNaturalStoreConnection::DeleteSyncEntries(const std::vector<Key> &keys)
{
    std::vector<Entry> entries;
    for (const auto &key : keys) {
        Entry entry;
        entry.key = std::move(key);
        entries.emplace_back(std::move(entry));
    }

    int errCode = SaveSyncEntries(entries, true);
    if ((errCode != E_OK) && (errCode != -E_NOT_FOUND)) {
        LOGE("[DeleteSyncEntries] Delete data err:%d", errCode);
    }
    return (errCode == -E_NOT_FOUND) ? E_OK : errCode;
}

int RdSingleVerNaturalStoreConnection::GetSyncDataSize(const std::string &device, size_t &size) const
{
    return -E_NOT_SUPPORT;
}
}
// namespace DistributedDB
