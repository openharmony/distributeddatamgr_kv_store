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
#ifndef SQLITE_RELATIONAL_STORE_CONNECTION_H
#define SQLITE_RELATIONAL_STORE_CONNECTION_H
#ifdef RELATIONAL_STORE

#include <atomic>
#include <string>
#include "macro_utils.h"
#include "relational_store_connection.h"
#include "sqlite_single_ver_relational_storage_executor.h"
#include "sqlite_relational_store.h"

namespace DistributedDB {
class SQLiteRelationalStoreConnection : public RelationalStoreConnection {
public:
    explicit SQLiteRelationalStoreConnection(SQLiteRelationalStore *store);

    ~SQLiteRelationalStoreConnection() override = default;

    DISABLE_COPY_ASSIGN_MOVE(SQLiteRelationalStoreConnection);

    // Close and release the connection.
    int Close() override;

    std::string GetIdentifier() override;
    int CreateDistributedTable(const std::string &tableName, TableSyncType syncType) override;
    int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier) override;

    int RegisterObserverAction(const StoreObserver *observer, const RelationalObserverAction &action) override;
    int UnRegisterObserverAction(const StoreObserver *observer) override;

    int GetStoreInfo(std::string &userId, std::string &appId, std::string &storeId) override;

    int SetTrackerTable(const TrackerSchema &schema) override;
    int ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records) override;
    int CleanTrackerData(const std::string &tableName, int64_t cursor) override;

    int SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty) override;

    int Pragma(PragmaCmd cmd, PragmaData &pragmaData) override;

    int UpsertData(RecordStatus status, const std::string &tableName, const std::vector<VBucket> &records) override;

    int GetDownloadingAssetsCount(int32_t &count) override;

    int SetTableMode(DistributedTableMode tableMode) override;
#ifdef USE_DISTRIBUTEDDB_CLOUD
    int DoClean(ClearMode mode, const std::vector<std::string> &tableList) override;

    int ClearCloudWatermark(const std::set<std::string> &tableNames) override;

    int32_t GetCloudSyncTaskCount() override;

    int SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb) override;

    int PrepareAndSetCloudDbSchema(const DataBaseSchema &schema) override;

    int SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader) override;

    int SetCloudSyncConfig(const CloudSyncConfig &config) override;

    int Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess, uint64_t taskId) override;

    SyncProcess GetCloudTaskStatus(uint64_t taskId) override;
#endif

#ifdef USE_DISTRIBUTEDDB_DEVICE
    int SyncToDevice(SyncInfo &info) override;

    int RemoveDeviceData() override;

    int RemoveDeviceData(const std::string &device) override;

    int RemoveDeviceData(const std::string &device, const std::string &tableName) override;

    int RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        std::shared_ptr<ResultSet> &result) override;

    int32_t GetDeviceSyncTaskCount() override;

    int SetDistributedDbSchema(const DistributedSchema &schema, bool isForceUpgrade) override;

    int RemoveExceptDeviceData(const std::map<std::string, std::vector<std::string>> &tableMap) override;
#endif
    int OperateDataStatus(uint32_t dataOperator) override;

    int SetProperty(const Property &property) override;
protected:

    int Pragma(int cmd, void *parameter) override;
private:

    SQLiteSingleVerRelationalStorageExecutor *GetExecutor(bool isWrite, int &errCode) const;
    void ReleaseExecutor(SQLiteSingleVerRelationalStorageExecutor *&executor) const;
    int StartTransaction();
    // Commit the transaction
    int Commit();

    // Roll back the transaction
    int RollBack();

    SQLiteSingleVerRelationalStorageExecutor *writeHandle_ = nullptr;
    mutable std::mutex transactionMutex_; // used for transaction
};
} // namespace DistributedDB
#endif
#endif // SQLITE_RELATIONAL_STORE_CONNECTION_H