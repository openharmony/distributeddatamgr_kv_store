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
#ifndef RELATIONAL_STORE_CONNECTION_H
#define RELATIONAL_STORE_CONNECTION_H
#ifdef RELATIONAL_STORE

#include <atomic>
#include <string>

#include "db_types.h"
#include "iconnection.h"
#include "macro_utils.h"
#include "ref_object.h"
#include "relational_store_delegate.h"

namespace DistributedDB {
class IRelationalStore;
using RelationalObserverAction =
    std::function<void(const std::string &device, ChangedData &&changedData, bool isChangedData, Origin origin)>;
class RelationalStoreConnection : public IConnection, public virtual RefObject {
public:
    struct SyncInfo {
        const std::vector<std::string> &devices;
        SyncMode mode = SYNC_MODE_PUSH_PULL;
        const SyncStatusCallback onComplete = nullptr;
        const Query &query;
        bool wait = true;
    };

    RelationalStoreConnection();

    explicit RelationalStoreConnection(IRelationalStore *store);

    virtual ~RelationalStoreConnection() = default;

    DISABLE_COPY_ASSIGN_MOVE(RelationalStoreConnection);

    // Close and release the connection.
    virtual int Close() = 0;
    virtual int SyncToDevice(SyncInfo &info) = 0;
    virtual std::string GetIdentifier() = 0;
    virtual int CreateDistributedTable(const std::string &tableName, TableSyncType syncType) = 0;
    virtual int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier) = 0;

    virtual int RemoveDeviceData() = 0;
    virtual int RemoveDeviceData(const std::string &device) = 0;
    virtual int RemoveDeviceData(const std::string &device, const std::string &tableName) = 0;
    virtual int RegisterObserverAction(const StoreObserver *observer, const RelationalObserverAction &action) = 0;
    virtual int UnRegisterObserverAction(const StoreObserver *observer) = 0;
    virtual int RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        std::shared_ptr<ResultSet> &result) = 0;

    virtual int GetStoreInfo(std::string &userId, std::string &appId, std::string &storeId) = 0;

    virtual int SetTrackerTable(const TrackerSchema &schema) = 0;
    virtual int ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records) = 0;
    virtual int CleanTrackerData(const std::string &tableName, int64_t cursor) = 0;

    virtual int SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty) = 0;

    virtual int Pragma(PragmaCmd cmd, PragmaData &pragmaData) = 0;

    virtual int UpsertData(RecordStatus status, const std::string &tableName, const std::vector<VBucket> &records) = 0;

    virtual int GetDownloadingAssetsCount(int32_t &count) = 0;

    virtual int SetDistributedDbSchema(const DistributedSchema &schema, bool isForceUpgrade) = 0;

    virtual int SetTableMode(DistributedTableMode tableMode) = 0;

#ifdef USE_DISTRIBUTEDDB_CLOUD
    virtual int32_t GetCloudSyncTaskCount() = 0;

    virtual int DoClean(ClearMode mode) = 0;

    virtual int ClearCloudWatermark(const std::set<std::string> &tableNames) = 0;

    virtual int SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb) = 0;

    virtual int PrepareAndSetCloudDbSchema(const DataBaseSchema &schema) = 0;

    virtual int SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader) = 0;

    virtual int SetCloudSyncConfig(const CloudSyncConfig &config) = 0;

    virtual int Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess, uint64_t taskId) = 0;

    virtual SyncProcess GetCloudTaskStatus(uint64_t taskId) = 0;
#endif
    virtual int OperateDataStatus(uint32_t dataOperator) = 0;

    virtual int32_t GetDeviceSyncTaskCount() = 0;
protected:
    // Get the stashed 'RelationalDB_ pointer' without ref.
    template<typename DerivedDBType>
    DerivedDBType *GetDB() const
    {
        return static_cast<DerivedDBType *>(store_);
    }

    virtual int Pragma(int cmd, void *parameter);
    IRelationalStore *store_ = nullptr;
    std::atomic<bool> isExclusive_;
};
} // namespace DistributedDB
#endif
#endif // RELATIONAL_STORE_CONNECTION_H