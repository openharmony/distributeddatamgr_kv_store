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
#include "rd_single_ver_storage_executor.h"

#include <algorithm>
#include <string>

#include "db_common.h"
#include "grd_db_api.h"
#include "grd_document_api.h"
#include "grd_error.h"
#include "grd_resultset_api.h"
#include "grd_type_export.h"
#include "ikvdb_result_set.h"
#include "rd_utils.h"
#include "sqlite_single_ver_storage_executor_sql.h"

namespace DistributedDB {
RDStorageExecutor::RDStorageExecutor(GRD_DB *db, bool isWrite) : StorageExecutor(isWrite), db_(db)
{
}

RDStorageExecutor::~RDStorageExecutor()
{
}

int RDStorageExecutor::Reset()
{
    return -E_NOT_SUPPORT;
}

int RDStorageExecutor::GetDbHandle(GRD_DB *&dbHandle) const
{
    dbHandle = db_;
    return E_OK;
}

RdSingleVerStorageExecutor::RdSingleVerStorageExecutor(GRD_DB *db, bool isWrite) : RDStorageExecutor(db, isWrite)
{
    LOGD("[RdSingleVerStorageExecutor] RdSingleVerStorageExecutor Created");
}

RdSingleVerStorageExecutor::~RdSingleVerStorageExecutor()
{
    int ret = GRD_OK;
    if (db_ != nullptr) {
        ret = RdDBClose(db_, 0);
        LOGD("[RdSingleVerStorageExecutor] rd has been closed");
    }
    if (ret != GRD_OK) {
        LOGE("Can not close db %d", ret);
    }
    db_ = nullptr;
    LOGD("[RdSingleVerStorageExecutor] RdSingleVerStorageExecutor has been deconstructed");
}

int RdSingleVerStorageExecutor::OpenResultSet(const Key &key, GRD_KvScanModeE mode, GRD_ResultSet **resultSet)
{
    int errCode = RdKVScan(db_, SYNC_COLLECTION_NAME.c_str(), key, mode, resultSet);
    if (errCode != E_OK) {
        LOGE("Can not open rd result set.");
    }
    return errCode;
}

int RdSingleVerStorageExecutor::OpenResultSet(const Key &beginKey, const Key &endKey, GRD_ResultSet **resultSet)
{
    int errCode = RdKVRangeScan(db_, SYNC_COLLECTION_NAME.c_str(), beginKey, endKey, resultSet);
    if (errCode != E_OK) {
        LOGE("[RdSingleVerStorageExecutor][OpenResultSet] Can not open rd result set.");
    }
    return errCode;
}

int RdSingleVerStorageExecutor::CloseResultSet(GRD_ResultSet *resultSet)
{
    int errCode = RdFreeResultSet(resultSet);
    if (errCode != E_OK) {
        LOGE("[RdSingleVerStorageExecutor] failed to free result set.");
    }
    return errCode;
}

int RdSingleVerStorageExecutor::InnerMoveToHead(const int position, GRD_ResultSet *resultSet, int &currPosition)
{
    int errCode = E_OK;
    while (true) {
        errCode = TransferGrdErrno(GRD_Prev(resultSet));
        if (errCode == -E_NOT_FOUND) {
            currPosition = 0;
            int ret = TransferGrdErrno(GRD_Next(resultSet));
            if (ret != E_OK) {
                LOGE("[RdSingleVerStorageExecutor] failed to move next for result set.");
                currPosition = position <= INIT_POSITION ? INIT_POSITION : currPosition;
                return ret;
            }
            ret = TransferGrdErrno(GRD_Prev(resultSet));
            if (ret != E_OK) {
                LOGE("[RdSingleVerStorageExecutor] failed to move prev for result set.");
                return ret;
            }
            break;
        } else if (errCode != E_OK) {
            LOGE("[RdSingleVerStorageExecutor] failed to move prev for result set.");
            return errCode;
        }
        currPosition--;
    }
    return E_OK;
}

int RdSingleVerStorageExecutor::MoveTo(const int position, GRD_ResultSet *resultSet, int &currPosition)
{
    int errCode = E_OK; // incase it never been move before
    if (currPosition == INIT_POSITION) {
        errCode = TransferGrdErrno(GRD_Next(resultSet)); // but when we have only 1 element ?
        if (errCode == -E_NOT_FOUND) {
            LOGE("[RdSingleVerStorageExecutor] result set is empty when move to");
            return -E_RESULT_SET_EMPTY;
        }
        if (errCode != E_OK) {
            LOGE("[RdSingleVerStorageExecutor] failed to move next for result set.");
            return errCode;
        }
        currPosition++;
    }
    errCode = InnerMoveToHead(position, resultSet, currPosition);
    if (errCode != E_OK) {
        return errCode;
    }
    if (position <= INIT_POSITION) {
        LOGE("[RdSingleVerStorageExecutor] current position must > -1 when move to.");
        int ret = TransferGrdErrno(GRD_Prev(resultSet));
        if (ret != E_OK && ret != -E_NOT_FOUND) {
            LOGE("[RdSingleVerStorageExecutor] failed to move prev for result set.");
            return ret;
        }
        currPosition = -1;
        return -E_INVALID_ARGS;
    }
    currPosition = 0;
    while (currPosition < position) {
        errCode = TransferGrdErrno(GRD_Next(resultSet));
        if (errCode == -E_NOT_FOUND) {
            LOGE("[RdSingleVerStorageExecutor] move to position: %d, out of bounds", position);
            currPosition++;
            return -E_INVALID_ARGS;
        } else if (errCode != E_OK) {
            LOGE("[RdSingleVerStorageExecutor] failed to move next for result set.");
            return errCode;
        }
        currPosition++;
    }
    return E_OK;
}

int RdSingleVerStorageExecutor::MoveToNext(GRD_ResultSet *resultSet)
{
    int errCode = TransferGrdErrno(GRD_Next(resultSet));
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[RdSingleVerStorageExecutor] failed to move next for result set.");
    }
    return errCode;
}

int RdSingleVerStorageExecutor::MoveToPrev(GRD_ResultSet *resultSet)
{
    int errCode = TransferGrdErrno(GRD_Prev(resultSet));
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[RdSingleVerStorageExecutor] failed to move prev for result set.");
    }
    return errCode;
}

int RdSingleVerStorageExecutor::GetEntry(GRD_ResultSet *resultSet, Entry &entry)
{
    int errCode = RdKvFetch(resultSet, entry.key, entry.value);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[RdSingleVerStorageExecutor] failed to get entry form result set.");
    }
    return errCode;
}

int RdSingleVerStorageExecutor::GetCountInner(GRD_ResultSet *tmpResultSet, int &count)
{
    bool isFirstMove = true;
    int ret = E_OK;
    int errCode = E_OK;
    while (errCode == E_OK) {
        errCode = TransferGrdErrno(GRD_Next(tmpResultSet));
        if (isFirstMove && errCode == -E_NOT_FOUND) {
            ret = CloseResultSet(tmpResultSet);
            if (ret != E_OK) {
                return ret;
            }
            return -E_RESULT_SET_EMPTY;
        } else if (errCode == -E_NOT_FOUND) {
            break;
        } else if (errCode != E_OK) {
            LOGE("[RdSingleVerStorageExecutor] failed to get count when move next.");
            ret = CloseResultSet(tmpResultSet);
            if (ret != E_OK) {
                return ret;
            }
            return errCode;
        }
        ++count;
        isFirstMove = false;
    }
    ret = CloseResultSet(tmpResultSet);
    if (ret != E_OK) {
        return ret;
    }
    return E_OK;
}

int RdSingleVerStorageExecutor::GetCount(const Key &key, int &count, GRD_KvScanModeE kvScanMode)
{
    count = 0;
    GRD_ResultSet *tmpResultSet = nullptr;
    int errCode = RdKVScan(db_, SYNC_COLLECTION_NAME.c_str(), key, kvScanMode, &tmpResultSet);
    if (errCode != E_OK) {
        LOGE("[RdSingleVerStorageExecutor] failed to get count for current key.");
        return errCode;
    }
    return GetCountInner(tmpResultSet, count);
}

int RdSingleVerStorageExecutor::GetCount(const Key &beginKey, const Key &endKey, int &count, GRD_KvScanModeE kvScanMode)
{
    count = 0;
    GRD_ResultSet *tmpResultSet = nullptr;
    int errCode = RdKVRangeScan(db_, SYNC_COLLECTION_NAME.c_str(), beginKey, endKey, &tmpResultSet);
    if (errCode != E_OK) {
        LOGE("[RdSingleVerStorageExecutor] failed to get count for current key.");
        return errCode;
    }
    return GetCountInner(tmpResultSet, count);
}

int RdSingleVerStorageExecutor::PrepareNotifyForEntries(const std::vector<Entry> &entries,
    SingleVerNaturalStoreCommitNotifyData *committedData, std::vector<NotifyConflictAndObserverData> &notifys,
    bool isDelete)
{
    for (const auto &entry : entries) {
        NotifyConflictAndObserverData notify = {
            .committedData = committedData
        };
        int errCode = PrepareForNotifyConflictAndObserver(entry, notify, isDelete);
        if (errCode != E_OK) {
            return errCode;
        }
        notifys.push_back(notify);
    }
    return E_OK;
}

int RdSingleVerStorageExecutor::GetKvData(SingleVerDataType type, const Key &key, Value &value,
    Timestamp &timestamp) const
{
    if (key.empty()) {
        LOGE("[RdSingleVerStorageExecutor][GetKvData] empty key.");
        return -E_INVALID_ARGS;
    }

    return RdKVGet(db_, SYNC_COLLECTION_NAME.c_str(), key, value);
}

int RdSingleVerStorageExecutor::ClearEntriesAndFreeResultSet(std::vector<Entry> &entries, GRD_ResultSet *resultSet)
{
    entries.clear();
    entries.shrink_to_fit();
    int errCode = RdFreeResultSet(resultSet);
    if (errCode != E_OK) {
        LOGE("[RdSingleVerStorageExecutor] failed to free result set.");
    }
    return errCode;
}

int RdSingleVerStorageExecutor::GetEntriesPrepare(GRD_DB *db, const GRD_KvScanModeE mode,
    const std::pair<Key, Key> &pairKey, std::vector<Entry> &entries, GRD_ResultSet **resultSet)
{
    int ret = E_OK;
    switch (mode) {
        case KV_SCAN_PREFIX: {
            ret = RdKVScan(db, SYNC_COLLECTION_NAME.c_str(), pairKey.first, KV_SCAN_PREFIX, resultSet);
            break;
        }
        case KV_SCAN_RANGE: {
            ret = RdKVRangeScan(db, SYNC_COLLECTION_NAME.c_str(), pairKey.first, pairKey.second, resultSet);
            break;
        }
        default:
            return -E_INVALID_ARGS;
    }
    if (ret != E_OK) {
        LOGE("[RdSingleVerStorageExecutor][GetEntriesPrepare]ERROR %d", ret);
        return ret;
    }
    entries.clear();
    entries.shrink_to_fit();
    return E_OK;
}

int RdSingleVerStorageExecutor::GetEntries(const GRD_KvScanModeE mode, const std::pair<Key, Key> &pairKey,
    std::vector<Entry> &entries) const
{
    GRD_ResultSet *resultSet = nullptr;
    int ret = GetEntriesPrepare(db_, mode, pairKey, entries, &resultSet);
    if (ret != E_OK) {
        return ret;
    }

    int innerCode = E_OK;
    ret = TransferGrdErrno(GRD_Next(resultSet));
    if (ret == -E_NOT_FOUND) {
        innerCode = ClearEntriesAndFreeResultSet(entries, resultSet);
        if (innerCode != E_OK) {
            return innerCode;
        }
        return ret;
    }
    while (ret == E_OK) {
        Entry tmpEntry;
        ret = RdKvFetch(resultSet, tmpEntry.key, tmpEntry.value);
        if (ret != E_OK && ret != -E_NOT_FOUND) {
            LOGE("[RdSingleVerStorageExecutor][GetEntries]fail to fetch, %d", ret);
            innerCode = ClearEntriesAndFreeResultSet(entries, resultSet);
            if (innerCode != E_OK) {
                return innerCode;
            }
            return ret;
        }
        entries.push_back(std::move(tmpEntry));
        ret = TransferGrdErrno(GRD_Next(resultSet));
    }
    if (ret != -E_NOT_FOUND) {
        LOGE("[RdSingleVerStorageExecutor][GetEntries]fail to move, %d", ret);
        innerCode = ClearEntriesAndFreeResultSet(entries, resultSet);
        if (innerCode != E_OK) {
            return innerCode;
        }
        return ret;
    }

    ret = RdFreeResultSet(resultSet);
    if (ret != E_OK) {
        LOGE("[RdSingleVerStorageExecutor] failed to free result set.");
        return ret;
    }
    return E_OK;
}

int RdSingleVerStorageExecutor::ForceCheckPoint() const
{
    return RdFlush(db_, 0);
}

int RdSingleVerStorageExecutor::SaveKvData(SingleVerDataType type, const Key &key, const Value &value)
{
    std::string collectionName;
    int ret = GetCollNameFromType(type, collectionName);
    if (ret != E_OK) {
        LOGE("Can not GetCollNameFromType");
        return ret;
    }
    return RdKVPut(db_, collectionName.c_str(), key, value);
}

int RdSingleVerStorageExecutor::DelKvData(const Key &key)
{
    return RdKVDel(db_, SYNC_COLLECTION_NAME.c_str(), key);
}

int RdSingleVerStorageExecutor::BatchSaveEntries(const std::vector<Entry> &entries, bool isDelete,
    SingleVerNaturalStoreCommitNotifyData *committedData)
{
    GRD_KVBatchT *batch = nullptr;
    int ret = RdKVBatchPrepare(entries.size(), &batch);
    if (ret != E_OK) {
        LOGE("[RdSingleVerStorageExecutor][BatchSaveEntries] Can not prepare KVBatch structure");
        return ret;
    }
    for (const auto &entry : entries) {
        ret = RdKVBatchPushback(batch, entry.key, entry.value);
        if (ret != E_OK) {
            (void)RdKVBatchDestroy(batch);
            LOGE("[RdSingleVerStorageExecutor][BatchSaveEntries] Can not push back entries to KVBatch structure");
            return ret;
        }
    }
    std::vector<NotifyConflictAndObserverData> notifys;
    ret = PrepareNotifyForEntries(entries, committedData, notifys, isDelete);
    if (ret != E_OK) {
        (void)RdKVBatchDestroy(batch);
        return ret;
    }
    if (isDelete) {
        ret = RdKVBatchDel(db_, SYNC_COLLECTION_NAME.c_str(), batch);
    } else {
        ret = RdKVBatchPut(db_, SYNC_COLLECTION_NAME.c_str(), batch);
    }
    if (ret != E_OK) {
        (void)RdKVBatchDestroy(batch);
        LOGE("[RdSingleVerStorageExecutor][BatchSaveEntries] Can not put or delete batchly with mode %d", isDelete);
        return ret;
    } else {
        for (size_t i = 0; i < entries.size(); i++) {
            PutIntoCommittedData(entries[i].key, entries[i].value, notifys[i]);
        }
    }
    int errCode = RdKVBatchDestroy(batch);
    if (errCode != E_OK) {
        LOGE("[RdSingleVerStorageExecutor][BatchSaveEntries] Can not destroy batch %d", isDelete);
    }
    return ret;
}

int RdSingleVerStorageExecutor::GetKvDataByHashKey(const Key &hashKey, SingleVerRecord &result) const
{
    return -E_NOT_SUPPORT;
}

// Put the Kv data according the type(meta and the local data).
int RdSingleVerStorageExecutor::PutKvData(SingleVerDataType type, const Key &key, const Value &value,
    Timestamp timestamp, SingleVerNaturalStoreCommitNotifyData *committedData)
{
    return -E_NOT_SUPPORT;
}

// Get all the meta keys.
int RdSingleVerStorageExecutor::GetAllMetaKeys(std::vector<Key> &keys) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::GetAllSyncedEntries(const std::string &hashDev, std::vector<Entry> &entries) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::SaveSyncDataItem(const Entry &entry,
    SingleVerNaturalStoreCommitNotifyData *committedData, bool isDelete)
{
    NotifyConflictAndObserverData notify = {
        .committedData = committedData
    };

    int errCode = PrepareForNotifyConflictAndObserver(entry, notify, isDelete);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SaveSyncDataToDatabase(entry, isDelete);
    if (errCode == E_OK) {
        PutIntoCommittedData(entry.key, entry.value, notify);
    } else {
        LOGE("Save sync data to db failed:%d", errCode);
    }
    return errCode;
}

int RdSingleVerStorageExecutor::EraseSyncData(const Key &hashKey)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::RemoveDeviceData(const std::string &deviceName)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::RemoveDeviceDataInCacheMode(const std::string &hashDev, bool isNeedNotify,
    uint64_t recordVersion) const
{
    return -E_NOT_SUPPORT;
}

void RdSingleVerStorageExecutor::InitCurrentMaxStamp(Timestamp &maxStamp)
{
    return;
}

void RdSingleVerStorageExecutor::ReleaseContinueStatement()
{
    return;
}

int RdSingleVerStorageExecutor::GetSyncDataByTimestamp(std::vector<DataItem> &dataItems, size_t appendLength,
    Timestamp begin, Timestamp end, const DataSizeSpecInfo &dataSizeInfo) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::GetDeletedSyncDataByTimestamp(std::vector<DataItem> &dataItems, size_t appendLength,
    Timestamp begin, Timestamp end, const DataSizeSpecInfo &dataSizeInfo) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::GetDeviceIdentifier(PragmaEntryDeviceIdentifier *identifier)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::OpenResultSet(QueryObject &queryObj, int &count)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::StartTransaction(TransactType type)
{
    return E_OK;
}

int RdSingleVerStorageExecutor::Commit()
{
    return E_OK;
}

int RdSingleVerStorageExecutor::Rollback()
{
    return E_OK;
}

bool RdSingleVerStorageExecutor::CheckIfKeyExisted(const Key &key, bool isLocal, Value &value,
    Timestamp &timestamp) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::ResetForSavingData(SingleVerDataType type)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::UpdateLocalDataTimestamp(Timestamp timestamp)
{
    return -E_NOT_SUPPORT;
}

void RdSingleVerStorageExecutor::SetAttachMetaMode(bool attachMetaMode)
{
    return;
}

int RdSingleVerStorageExecutor::PutLocalDataToCacheDB(const LocalDataItem &dataItem) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::SaveSyncDataItemInCacheMode(DataItem &dataItem, const DeviceInfo &deviceInfo,
    Timestamp &maxStamp, uint64_t recordVersion, const QueryObject &query)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::PrepareForSavingCacheData(SingleVerDataType type)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::ResetForSavingCacheData(SingleVerDataType type)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::MigrateLocalData()
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::MigrateSyncDataByVersion(uint64_t recordVer, NotifyMigrateSyncData &syncData,
    std::vector<DataItem> &dataItems)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::GetMinVersionCacheData(std::vector<DataItem> &dataItems,
    uint64_t &minVerIncurCacheDb) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::GetMaxVersionInCacheDb(uint64_t &maxVersion) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::AttachMainDbAndCacheDb(CipherType type, const CipherPassword &passwd,
    const std::string &attachDbAbsPath, EngineState engineState)
{
    return -E_NOT_SUPPORT;
}

// Clear migrating data.
void RdSingleVerStorageExecutor::ClearMigrateData()
{
    return;
}

// Get current max timestamp.
int RdSingleVerStorageExecutor::GetMaxTimestampDuringMigrating(Timestamp &maxTimestamp) const
{
    return -E_NOT_SUPPORT;
}

void RdSingleVerStorageExecutor::SetConflictResolvePolicy(int policy)
{
    return;
}

// Delete multiple meta data records in a transaction.
int RdSingleVerStorageExecutor::DeleteMetaData(const std::vector<Key> &keys)
{
    return -E_NOT_SUPPORT;
}

// Delete multiple meta data records with key prefix in a transaction.
int RdSingleVerStorageExecutor::DeleteMetaDataByPrefixKey(const Key &keyPrefix)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::CheckIntegrity() const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::CheckQueryObjectLegal(QueryObject &queryObj) const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::CheckDataWithQuery(QueryObject query, std::vector<DataItem> &dataItems,
    const DeviceInfo &deviceInfo)
{
    return -E_NOT_SUPPORT;
}

size_t RdSingleVerStorageExecutor::GetDataItemSerialSize(const DataItem &item, size_t appendLen)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::AddSubscribeTrigger(QueryObject &query, const std::string &subscribeId)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::RemoveSubscribeTrigger(const std::vector<std::string> &subscribeIds)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::RemoveSubscribeTriggerWaterMark(const std::vector<std::string> &subscribeIds)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::GetTriggers(const std::string &namePreFix, std::vector<std::string> &triggerNames)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::RemoveTrigger(const std::vector<std::string> &triggers)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::GetSyncDataWithQuery(const QueryObject &query, size_t appendLength,
    const DataSizeSpecInfo &dataSizeInfo, const std::pair<Timestamp, Timestamp> &timeRange,
    std::vector<DataItem> &dataItems) const
{
    return -E_NOT_SUPPORT;
}

uint64_t RdSingleVerStorageExecutor::GetLogFileSize() const
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::GetExistsDevicesFromMeta(std::set<std::string> &devices)
{
    return -E_NOT_SUPPORT;
}

int RdSingleVerStorageExecutor::UpdateKey(const UpdateKeyCallback &callback)
{
    return -E_NOT_SUPPORT;
}

void RdSingleVerStorageExecutor::PutIntoCommittedData(const Key &key, const Value &value,
    NotifyConflictAndObserverData &data)
{
    if (data.committedData == nullptr) {
        return;
    }

    Entry entry;
    int errCode = E_OK;
    if (!data.dataStatus.isDeleted) {
        entry.key = key;
        entry.value = value;
        DataType dataType = (data.dataStatus.preStatus == DataStatus::EXISTED) ? DataType::UPDATE : DataType::INSERT;
        errCode = data.committedData->InsertCommittedData(std::move(entry), dataType, true);
    } else {
        if (data.dataStatus.preStatus == DataStatus::NOEXISTED) {
            return;
        }
        entry.key = data.getData.key;
        entry.value = data.getData.value;
        errCode = data.committedData->InsertCommittedData(std::move(entry), DataType::DELETE, true);
    }

    if (errCode != E_OK) {
        LOGE("[SingleVerExe][PutCommitData] Rd Insert failed:%d", errCode);
    }
}

int RdSingleVerStorageExecutor::GetSyncDataPreByKey(const Key &key, DataItem &itemGet) const
{
    Timestamp recordTimestamp;
    Value value;
    int errCode = GetKvData(SingleVerDataType::SYNC_TYPE, key, value, recordTimestamp);
    if (errCode == E_OK) {
        itemGet.key = key;
        itemGet.value = value;
    }
    return errCode;
}

int RdSingleVerStorageExecutor::PrepareForNotifyConflictAndObserver(const Entry &entry,
    NotifyConflictAndObserverData &notify, bool isDelete)
{
    // Check sava data existed info
    int errCode = DBCommon::CalcValueHash(entry.key, notify.hashKey);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = GetSyncDataPreByKey(entry.key, notify.getData);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGD("[SingleVerExe][PrepareForNotifyConflictAndObserver] failed:%d", errCode);
        return errCode;
    }

    bool isHashKeyExisted = (errCode != -E_NOT_FOUND);

    LOGD("Preparing for notify conflict and observer");
    notify.dataStatus.isDeleted = isDelete;
    if (isHashKeyExisted) {
        notify.dataStatus.preStatus = DataStatus::EXISTED;
    } else {
        notify.dataStatus.preStatus = DataStatus::NOEXISTED;
    }
    InitCommitNotifyDataKeyStatus(notify.committedData, notify.hashKey, notify.dataStatus);
    return E_OK;
}

int RdSingleVerStorageExecutor::SaveSyncDataToDatabase(const Entry &entry, bool isDelete)
{
    if (isDelete) {
        return DelKvData(entry.key);
    }
    return SaveKvData(SingleVerDataType::SYNC_TYPE, entry.key, entry.value);
}
} // namespace DistributedDB