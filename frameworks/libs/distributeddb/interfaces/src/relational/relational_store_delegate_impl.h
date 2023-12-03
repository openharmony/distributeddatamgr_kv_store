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
#ifndef RELATIONAL_STORE_DELEGATE_IMPL_H
#define RELATIONAL_STORE_DELEGATE_IMPL_H
#ifdef RELATIONAL_STORE

#include "macro_utils.h"
#include "relational_store_connection.h"

namespace DistributedDB {
class RelationalStoreDelegateImpl final : public RelationalStoreDelegate {
public:
    RelationalStoreDelegateImpl() = default;
    ~RelationalStoreDelegateImpl() override;

    RelationalStoreDelegateImpl(RelationalStoreConnection *conn, const std::string &path);

    DISABLE_COPY_ASSIGN_MOVE(RelationalStoreDelegateImpl);

    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const Query &query, const SyncStatusCallback &onComplete, bool wait) override;

    int32_t GetCloudSyncTaskCount() override;

    DBStatus RemoveDeviceDataInner(const std::string &device, ClearMode mode) override;

    DBStatus CreateDistributedTableInner(const std::string &tableName, TableSyncType type) override;

    DBStatus RemoveDeviceData(const std::string &device, const std::string &tableName) override;

    // For connection
    DBStatus Close();

    void SetReleaseFlag(bool flag);

    DBStatus RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        std::shared_ptr<ResultSet> &result) override;

    DBStatus RemoveDeviceData() override;

    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode, const Query &query,
        const SyncProcessCallback &onProcess, int64_t waitTime) override;

    DBStatus SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb) override;

    DBStatus SetCloudDbSchema(const DataBaseSchema &schema) override;

    DBStatus RegisterObserver(StoreObserver *observer) override;

    DBStatus UnRegisterObserver() override;

    DBStatus UnRegisterObserver(StoreObserver *observer) override;

    DBStatus SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader) override;

    DBStatus Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess) override;

    DBStatus SetTrackerTable(const TrackerSchema &schema) override;

    DBStatus ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records) override;

    DBStatus SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty) override;

    DBStatus CleanTrackerData(const std::string &tableName, int64_t cursor) override;

    DBStatus Pragma(PragmaCmd cmd, PragmaData &pragmaData) override;

    DBStatus UpsertData(const std::string &tableName, const std::vector<VBucket> &records,
        RecordStatus status) override;
private:
    static void OnSyncComplete(const std::map<std::string, std::vector<TableStatus>> &devicesStatus,
        const SyncStatusCallback &onComplete);

    RelationalStoreConnection *conn_ = nullptr;
    std::string storePath_;
    std::atomic<bool> releaseFlag_ = false;
};
} // namespace DistributedDB
#endif
#endif // RELATIONAL_STORE_DELEGATE_IMPL_H