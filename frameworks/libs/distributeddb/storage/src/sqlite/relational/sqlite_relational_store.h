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
#ifndef SQLITE_RELATIONAL_STORE_H
#define SQLITE_RELATIONAL_STORE_H
#ifdef RELATIONAL_STORE

#include <functional>
#include <memory>
#include <vector>

#include "irelational_store.h"
#include "sqlite_single_relational_storage_engine.h"
#include "isyncer.h"
#include "sync_able_engine.h"
#include "relational_sync_able_storage.h"
#include "runtime_context.h"
#include "cloud/cloud_syncer.h"

namespace DistributedDB {
using RelationalObserverAction =
    std::function<void(const std::string &device, ChangedData &&changedData, bool isChangedData)>;
class SQLiteRelationalStore : public IRelationalStore {
public:
    SQLiteRelationalStore() = default;
    ~SQLiteRelationalStore() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteRelationalStore);

    RelationalStoreConnection *GetDBConnection(int &errCode) override;
    int Open(const RelationalDBProperties &properties) override;
    void OnClose(const std::function<void(void)> &notifier);

    SQLiteSingleVerRelationalStorageExecutor *GetHandle(bool isWrite, int &errCode) const;
    void ReleaseHandle(SQLiteSingleVerRelationalStorageExecutor *&handle) const;

    int Sync(const ISyncer::SyncParma &syncParam, uint64_t connectionId);

    int32_t GetCloudSyncTaskCount();

    int CleanCloudData(ClearMode mode);

    void ReleaseDBConnection(uint64_t connectionId, RelationalStoreConnection *connection);

    void WakeUpSyncer() override;

    // for test mock
    const RelationalSyncAbleStorage *GetStorageEngine()
    {
        return storageEngine_;
    }

    int CreateDistributedTable(const std::string &tableName, TableSyncType syncType, bool trackerSchemaChanged = false);

    int RemoveDeviceData();
    int RemoveDeviceData(const std::string &device, const std::string &tableName);

    int RegisterObserverAction(uint64_t connectionId, const StoreObserver *observer,
        const RelationalObserverAction &action);
    int UnRegisterObserverAction(uint64_t connectionId, const StoreObserver *observer);
    int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier);

    std::string GetStorePath() const override;

    RelationalDBProperties GetProperties() const override;

    void StopSync(uint64_t connectionId);

    void Dump(int fd) override;

    int RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        uint64_t connectionId, std::shared_ptr<ResultSet> &result);

    int SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb);

    int PrepareAndSetCloudDbSchema(const DataBaseSchema &schema);

    int SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader);

    int ChkSchema(const TableName &tableName);

    int Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess);

    int SetTrackerTable(const TrackerSchema &trackerSchema);

    int ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records);

    int CleanTrackerData(const std::string &tableName, int64_t cursor);

    int SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty);

    int Pragma(PragmaCmd cmd, PragmaData &pragmaData);

    int UpsertData(RecordStatus status, const std::string &tableName, const std::vector<VBucket> &records);
private:
    void ReleaseResources();

    // 1 store 1 connection
    void DecreaseConnectionCounter(uint64_t connectionId);
    int CheckDBMode();
    int GetSchemaFromMeta(RelationalSchemaObject &schema);
    int SaveSchemaToMeta();
    int CheckTableModeFromMeta(DistributedTableMode mode, bool isUnSet);
    int SaveTableModeToMeta(DistributedTableMode mode);
    int CheckProperties(RelationalDBProperties properties);

    int SaveLogTableVersionToMeta();

    int CleanDistributedDeviceTable();

    int StopLifeCycleTimer();
    int StartLifeCycleTimer(const DatabaseLifeCycleNotifier &notifier);
    void HeartBeat();
    int ResetLifeCycleTimer();

    void IncreaseConnectionCounter();
    int InitStorageEngine(const RelationalDBProperties &properties);

    int EraseAllDeviceWatermark(const std::vector<std::string> &tableNameList);

    std::string GetDevTableName(const std::string &device, const std::string &hashDev) const;

    int GetHandleAndStartTransaction(SQLiteSingleVerRelationalStorageExecutor *&handle) const;

    int RemoveDeviceDataInner(const std::string &mappingDev, const std::string &device,
        const std::string &tableName, bool isNeedHash);

    int GetExistDevices(std::set<std::string> &hashDevices) const;

    std::vector<std::string> GetAllDistributedTableName();

    int CheckBeforeSync(const CloudSyncOption &option);

    int CheckQueryValid(const CloudSyncOption &option);

    int CheckObjectValid(bool priorityTask, const std::vector<QuerySyncObject> &object, bool isFromTable);

    int CheckTableName(const std::vector<std::string> &tableNames);

    void FillSyncInfo(const CloudSyncOption &option, const SyncProcessCallback &onProcess,
        CloudSyncer::CloudTaskInfo &info);

    int CleanWaterMark(std::set<std::string> &clearWaterMarkTable);

    int InitTrackerSchemaFromMeta();

    void AddFields(const std::vector<Field> &newFields, const std::set<std::string> &equalFields,
        std::vector<Field> &addFields);

    bool CheckFields(const std::vector<Field> &newFields, const TableInfo &tableInfo, std::vector<Field> &addFields);

    bool PrepareSharedTable(const DataBaseSchema &schema, std::vector<std::string> &deleteTableNames,
        std::map<std::string, std::vector<Field>> &updateTableNames,
        std::map<std::string, std::string> &alterTableNames);

    int ExecuteCreateSharedTable(const DataBaseSchema &schema);

    int CheckParamForUpsertData(RecordStatus status, const std::string &tableName, const std::vector<VBucket> &records);

    int CheckSchemaForUpsertData(const std::string &tableName, const std::vector<VBucket> &records);

    int InitSQLiteStorageEngine(const RelationalDBProperties &properties);

    static int ReFillSyncInfoTable(const std::vector<std::string> &actualTable, CloudSyncer::CloudTaskInfo &info);

    int CheckCloudSchema(const DataBaseSchema &schema);

    // use for sync Interactive
    std::shared_ptr<SyncAbleEngine> syncAbleEngine_ = nullptr; // For storage operate sync function
    // use ref obj same as kv
    RelationalSyncAbleStorage *storageEngine_ = nullptr; // For storage operate data
    std::shared_ptr<SQLiteSingleRelationalStorageEngine> sqliteStorageEngine_;
    CloudSyncer *cloudSyncer_ = nullptr;

    std::mutex connectMutex_;
    std::atomic<int> connectionCount_ = 0;
    std::vector<std::function<void(void)>> closeNotifiers_;

    mutable std::mutex initalMutex_;
    bool isInitialized_ = false;

    // lifeCycle
    std::mutex lifeCycleMutex_;
    DatabaseLifeCycleNotifier lifeCycleNotifier_;
    TimerId lifeTimerId_ {};
};
}  // namespace DistributedDB
#endif
#endif // SQLITE_RELATIONAL_STORE_H