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

#ifndef RELATIONAL_STORE_DELEGATE_H
#define RELATIONAL_STORE_DELEGATE_H

#include <map>
#include <memory>
#include <optional>
#include "distributeddb/result_set.h"
#include "cloud/cloud_store_types.h"
#include "cloud/icloud_db.h"
#include "cloud/icloud_data_translate.h"
#include "cloud/iAssetLoader.h"
#include "query.h"
#include "store_types.h"
#include "store_observer.h"

namespace DistributedDB {
class RelationalStoreDelegate {
public:
    DB_API virtual ~RelationalStoreDelegate() = default;

    struct Option {
        StoreObserver *observer = nullptr;
        // communicator label use dualTuple hash or not;
        bool syncDualTupleMode = false;
        bool isEncryptedDb = false;
        CipherType cipher = CipherType::DEFAULT;
        CipherPassword passwd;
        uint32_t iterateTimes = 0;
        DistributedTableMode tableMode = DistributedTableMode::SPLIT_BY_DEVICE;
    };

    struct StoreConfig {
        std::optional<DistributedTableMode> tableMode;
    };

    DB_API virtual DBStatus SetStoreConfig(const StoreConfig &config)
    {
        return OK;
    }

    DB_API DBStatus CreateDistributedTable(const std::string &tableName, TableSyncType type = DEVICE_COOPERATION)
    {
        return CreateDistributedTableInner(tableName, type);
    }

    DB_API virtual DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const Query &query, const SyncStatusCallback &onComplete, bool wait) = 0;

    DB_API virtual int32_t GetCloudSyncTaskCount()
    {
        return 0;
    }

    DB_API DBStatus RemoveDeviceData(const std::string &device, ClearMode mode = DEFAULT)
    {
        return RemoveDeviceDataInner(device, mode);
    }

    DB_API virtual DBStatus RemoveDeviceData(const std::string &device, const std::string &tableName) = 0;

    // timeout is in ms.
    DB_API virtual DBStatus RemoteQuery(const std::string &device, const RemoteCondition &condition,
        uint64_t timeout, std::shared_ptr<ResultSet> &result) = 0;

    // remove all device data
    DB_API virtual DBStatus RemoveDeviceData() = 0;

    DB_API virtual DBStatus Sync([[gnu::unused]] const std::vector<std::string> &devices,
         [[gnu::unused]] SyncMode mode, [[gnu::unused]] const Query &query,
         [[gnu::unused]] const SyncProcessCallback &onProcess, [[gnu::unused]] int64_t waitTime)
    {
        return OK;
    }

    DB_API virtual DBStatus SetCloudDB([[gnu::unused]] const std::shared_ptr<ICloudDb> &cloudDb)
    {
        return OK;
    }

    DB_API virtual DBStatus SetCloudDbSchema([[gnu::unused]] const DataBaseSchema &schema)
    {
        return OK;
    }

    // just support one observer exist at same time
    DB_API virtual DBStatus RegisterObserver(StoreObserver *observer) = 0;

    DB_API virtual DBStatus UnRegisterObserver() = 0;

    DB_API virtual DBStatus UnRegisterObserver(StoreObserver *observer) = 0;

    DB_API virtual DBStatus SetIAssetLoader([[gnu::unused]] const std::shared_ptr<IAssetLoader> &loader)
    {
        return OK;
    }

    DB_API virtual DBStatus Sync([[gnu::unused]] const CloudSyncOption &option,
        [[gnu::unused]] const SyncProcessCallback &onProcess)
    {
        return OK;
    }

    DB_API virtual DBStatus SetTrackerTable(const TrackerSchema &schema) = 0;

    DB_API virtual DBStatus ExecuteSql(const SqlCondition &condition, std::vector<VBucket> &records) = 0;

    DB_API virtual DBStatus SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty) = 0;

    DB_API virtual DBStatus CleanTrackerData(const std::string &tableName, int64_t cursor) = 0;

    DB_API virtual DBStatus Pragma(PragmaCmd cmd, PragmaData &pragmaData) = 0;

    DB_API virtual DBStatus UpsertData(const std::string &tableName, const std::vector<VBucket> &records,
        RecordStatus status = RecordStatus::WAIT_COMPENSATED_SYNC) = 0;

    // set the config for cloud sync task
    DB_API virtual DBStatus SetCloudSyncConfig([[gnu::unused]] const CloudSyncConfig &config)
    {
        return OK;
    }

    DB_API virtual DBStatus Sync([[gnu::unused]] const CloudSyncOption &option,
        [[gnu::unused]] const SyncProcessCallback &onProcess,
        [[gnu::unused]] uint64_t taskId)
    {
        return OK;
    }

    DB_API virtual SyncProcess GetCloudTaskStatus([[gnu::unused]] uint64_t taskId)
    {
        return {};
    }

    DB_API virtual DBStatus SetDistributedSchema([[gnu::unused]] const DistributedSchema &schema,
            [[gnu::unused]] bool isForceUpgrade = false)
    {
        return OK;
    }

    DB_API virtual std::pair<DBStatus, int32_t> GetDownloadingAssetsCount()
    {
        return {OK, 0};
    }
protected:
    virtual DBStatus RemoveDeviceDataInner(const std::string &device, ClearMode mode) = 0;
    virtual DBStatus CreateDistributedTableInner(const std::string &tableName, TableSyncType type) = 0;
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE_DELEGATE_H