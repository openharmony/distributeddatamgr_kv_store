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

    int CreateDistributedTable(const std::string &tableName, TableSyncType syncType);

    int RemoveDeviceData();
    int RemoveDeviceData(const std::string &device, const std::string &tableName);

    void RegisterObserverAction(uint64_t connectionId, const RelationalObserverAction &action);
    int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier);

    std::string GetStorePath() const override;

    RelationalDBProperties GetProperties() const override;

    void StopSync(uint64_t connectionId);

    void Dump(int fd) override;

    int RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        uint64_t connectionId, std::shared_ptr<ResultSet> &result);

    int SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb);

    int SetCloudDbSchema(const DataBaseSchema &schema);

    int SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader);

    int ChkSchema(const TableName &tableName);

    int Sync(const std::vector<std::string> &devices, SyncMode mode, const Query &query,
        const SyncProcessCallback &onProcess, int64_t waitTime);

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

    SQLiteSingleVerRelationalStorageExecutor *GetHandleAndStartTransaction(int &errCode) const;

    int RemoveDeviceDataInner(const std::string &mappingDev, const std::string &device,
        const std::string &tableName, bool isNeedHash);

    int GetExistDevices(std::set<std::string> &hashDevices) const;

    std::vector<std::string> GetAllDistributedTableName();

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