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

#ifndef STORAGE_PROXY_H
#define STORAGE_PROXY_H

#include <atomic>
#include <shared_mutex>

#include "cloud/cloud_store_types.h"
#include "cloud/cloud_meta_data.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/schema_mgr.h"
#include "data_transformer.h"
#include "icloud_sync_storage_interface.h"


namespace DistributedDB {
class StorageProxy {
public:
    StorageProxy(ICloudSyncStorageInterface *iCloud);
    virtual ~StorageProxy() {};

    static std::shared_ptr<StorageProxy> GetCloudDb(ICloudSyncStorageInterface *iCloud);

    int Close();

    int GetLocalWaterMark(const std::string &tableName, LocalWaterMark &localMark);

    int PutLocalWaterMark(const std::string &tableName, LocalWaterMark &localMark);

    int GetCloudWaterMark(const std::string &tableName, CloudWaterMark &cloudMark);

    int PutCloudWaterMark(const std::string &tableName, CloudWaterMark &cloudMark);
    
    int StartTransaction(TransactType type = TransactType::DEFERRED);

    int Commit();

    int Rollback();

    int GetUploadCount(const std::string &tableName, const LocalWaterMark &timestamp, const bool isCloudForcePush,
        int64_t &count);

    int FillCloudGid(const CloudSyncData &data);

    int GetCloudData(const std::string &tableName, const Timestamp &timeRange,
        ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult);

    int GetCloudDataNext(ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) const;

    int GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket,
        DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo);

    int PutCloudSyncData(const std::string &tableName, DownloadData &downloadData);

    int CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList, std::vector<Asset> &assets);
    
    int CheckSchema(const TableName &tableName) const;

    int CheckSchema(std::vector<std::string> &tables);

    int GetPrimaryColNamesWithAssetsFields(const TableName &tableName, std::vector<std::string> &colNames,
        std::vector<Field> &assetFields);

    int NotifyChangedData(const std::string deviceName, ChangedData &&changedData);

    int ReleaseContinueToken(ContinueToken &continueStmtToken);

    int FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isFullReplace);

    int FillCloudAssetForUpload(const CloudSyncData &data);

protected:
    void Init();
private:
    ICloudSyncStorageInterface *store_;
    mutable std::shared_mutex storeMutex_;
    mutable std::shared_mutex cloudDbMutex_;
    std::atomic<bool> transactionExeFlag_;
    std::shared_ptr<CloudMetaData> cloudMetaData_;
    std::atomic<bool> isWrite_;
};
}

#endif //STORAGE_PROXY_H
