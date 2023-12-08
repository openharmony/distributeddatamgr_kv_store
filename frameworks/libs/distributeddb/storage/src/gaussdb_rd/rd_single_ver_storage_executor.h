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
 * See the License for the spRdSingleVerStorageExecutorecific language governing permissions and
 * limitations under the License.
 */

#ifndef RD_SINGLE_VER_STORAGE_EXECUTOR_H
#define RD_SINGLE_VER_STORAGE_EXECUTOR_H
#include "macro_utils.h"
#include "db_types.h"
#include "grd_kv_api.h"
#include "grd_resultset_api.h"
#include "query_object.h"
#include "sqlite_utils.h"
#include "sqlite_single_ver_storage_executor.h"
#include "single_ver_natural_store_commit_notify_data.h"

namespace DistributedDB {
class RDStorageExecutor : public StorageExecutor {
public:
    RDStorageExecutor(GRD_DB *db, bool isWrite);
    ~RDStorageExecutor() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(RDStorageExecutor);

    int Reset() override;

    int GetDbHandle(GRD_DB *&dbHandle) const;

protected:
    GRD_DB *db_;
};

class RdSingleVerStorageExecutor : public RDStorageExecutor {
public:
    RdSingleVerStorageExecutor(GRD_DB *db, bool isWrite);
    ~RdSingleVerStorageExecutor() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(RdSingleVerStorageExecutor);

    // Get the Kv data according the type(sync, meta, local data).
    int GetKvData(SingleVerDataType type, const Key &key, Value &value, Timestamp &timestamp) const;

    // Get the sync data record by hash key.
    int GetKvDataByHashKey(const Key &hashKey, SingleVerRecord &result) const;

    // Put the Kv data according the type(meta and the local data).
    virtual int PutKvData(SingleVerDataType type, const Key &key, const Value &value,
        Timestamp timestamp, SingleVerNaturalStoreCommitNotifyData *committedData);

    int GetEntries(const GRD_KvScanModeE mode, const std::pair<Key, Key> &pairKey,
        std::vector<Entry> &entries) const;

    int GetCount(const Key &key, int &count, GRD_KvScanModeE kvScanMode);

    int GetCount(const Key &beginKey, const Key &endKey, int &count, GRD_KvScanModeE kvScanMode);

    // Get all the meta keys.
    int GetAllMetaKeys(std::vector<Key> &keys) const;

    int GetAllSyncedEntries(const std::string &hashDev, std::vector<Entry> &entries) const;

    int BatchSaveEntries(const std::vector<Entry> &entries, bool isDelete,
        SingleVerNaturalStoreCommitNotifyData *committedData);

    int SaveSyncDataItem(const Entry &entry, SingleVerNaturalStoreCommitNotifyData *committedData, bool isDelete);

    int PrepareNotifyForEntries(const std::vector<Entry> &entries,
        SingleVerNaturalStoreCommitNotifyData *committedData, std::vector<NotifyConflictAndObserverData> &notifys,
        bool isDelete);

    int DeleteLocalKvData(const Key &key, SingleVerNaturalStoreCommitNotifyData *committedData, Value &value,
        Timestamp &timestamp);

    int EraseSyncData(const Key &hashKey);

    int RemoveDeviceData(const std::string &deviceName);
    int RemoveDeviceDataInCacheMode(const std::string &hashDev, bool isNeedNotify, uint64_t recordVersion) const;

    void InitCurrentMaxStamp(Timestamp &maxStamp);

    void ReleaseContinueStatement();

    int GetSyncDataByTimestamp(std::vector<DataItem> &dataItems, size_t appendLength, Timestamp begin,
        Timestamp end, const DataSizeSpecInfo &dataSizeInfo) const;

    int GetDeletedSyncDataByTimestamp(std::vector<DataItem> &dataItems, size_t appendLength, Timestamp begin,
        Timestamp end, const DataSizeSpecInfo &dataSizeInfo) const;

    int GetDeviceIdentifier(PragmaEntryDeviceIdentifier *identifier);

    int OpenResultSet(const Key &key, GRD_KvScanModeE mode, GRD_ResultSet **resultSet);

    int OpenResultSet(const Key &beginKey, const Key &endKey, GRD_ResultSet **resultSet);

    int CloseResultSet(GRD_ResultSet *resultSet);

    int MoveTo(const int position, GRD_ResultSet *resultSet, int &currPosition);

    int MoveToNext(GRD_ResultSet *resultSet);

    int MoveToPrev(GRD_ResultSet *resultSet);

    int GetEntry(GRD_ResultSet *resultSet, Entry &entry);

    int OpenResultSet(QueryObject &queryObj, int &count);

    int StartTransaction(TransactType type);

    int Commit();

    int Rollback();

    bool CheckIfKeyExisted(const Key &key, bool isLocal, Value &value, Timestamp &timestamp) const;

    int ResetForSavingData(SingleVerDataType type);

    int UpdateLocalDataTimestamp(Timestamp timestamp);

    void SetAttachMetaMode(bool attachMetaMode);

    int PutLocalDataToCacheDB(const LocalDataItem &dataItem) const;

    int SaveSyncDataItemInCacheMode(DataItem &dataItem, const DeviceInfo &deviceInfo, Timestamp &maxStamp,
        uint64_t recordVersion, const QueryObject &query);

    int PrepareForSavingCacheData(SingleVerDataType type);

    int ResetForSavingCacheData(SingleVerDataType type);

    int MigrateLocalData();

    int MigrateSyncDataByVersion(uint64_t recordVer, NotifyMigrateSyncData &syncData,
        std::vector<DataItem> &dataItems);

    int GetMinVersionCacheData(std::vector<DataItem> &dataItems, uint64_t &minVerIncurCacheDb) const;

    int GetMaxVersionInCacheDb(uint64_t &maxVersion) const;

    int AttachMainDbAndCacheDb(CipherType type, const CipherPassword &passwd,
        const std::string &attachDbAbsPath, EngineState engineState);

    // Clear migrating data.
    void ClearMigrateData();

    // Get current max timestamp.
    int GetMaxTimestampDuringMigrating(Timestamp &maxTimestamp) const;

    void SetConflictResolvePolicy(int policy);

    // Delete multiple meta data records in a transaction.
    int DeleteMetaData(const std::vector<Key> &keys);

    // Delete multiple meta data records with key prefix in a transaction.
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix);

    int CheckIntegrity() const;

    int CheckQueryObjectLegal(QueryObject &queryObj) const;

    int CheckDataWithQuery(QueryObject query, std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo);

    static size_t GetDataItemSerialSize(const DataItem &item, size_t appendLen);

    int AddSubscribeTrigger(QueryObject &query, const std::string &subscribeId);

    int RemoveSubscribeTrigger(const std::vector<std::string> &subscribeIds);

    int RemoveSubscribeTriggerWaterMark(const std::vector<std::string> &subscribeIds);

    int GetTriggers(const std::string &namePreFix, std::vector<std::string> &triggerNames);

    int RemoveTrigger(const std::vector<std::string> &triggers);

    int GetSyncDataWithQuery(const QueryObject &query, size_t appendLength, const DataSizeSpecInfo &dataSizeInfo,
        const std::pair<Timestamp, Timestamp> &timeRange, std::vector<DataItem> &dataItems) const;

    int ForceCheckPoint() const;

    uint64_t GetLogFileSize() const;

    int GetExistsDevicesFromMeta(std::set<std::string> &devices);

    int UpdateKey(const UpdateKeyCallback &callback);
protected:
    int SaveKvData(SingleVerDataType type, const Key &key, const Value &value);

    int DeleteLocalDataInner(SingleVerNaturalStoreCommitNotifyData *committedData, const Key &key, const Value &value);

private:
    void PutIntoCommittedData(const Key &key, const Value &value, NotifyConflictAndObserverData &data);

    int GetSyncDataPreByKey(const Key &key, DataItem &itemGet) const;

    int PrepareForNotifyConflictAndObserver(const Entry &entry,
        NotifyConflictAndObserverData &notify, bool isDelete = false);

    int DelKvData(const Key &key);

    int SaveSyncDataToDatabase(const Entry &entry, bool isDelete);

    int InnerMoveToHead(const int position, GRD_ResultSet *resultSet, int &currPosition);

    static int ClearEntriesAndFreeResultSet(std::vector<Entry> &entries, GRD_ResultSet *resultSet);
    
    static int GetEntriesPrepare(GRD_DB *db, const GRD_KvScanModeE mode, const std::pair<Key, Key> &pairKey,
        std::vector<Entry> &entries, GRD_ResultSet **resultSet);
    
    int GetCountInner(GRD_ResultSet *tmpResultSet, int &count);
};
} // namespace DistributedDB
#endif // RD_SINGLE_VER_STORAGE_EXECUTOR_H
