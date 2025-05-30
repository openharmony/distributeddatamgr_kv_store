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
#ifndef I_CLOUD_SYNCER_H
#define I_CLOUD_SYNCER_H
#include <cstdint>
#include <string>
#include "cloud/cloud_store_types.h"
#include "icloud_sync_storage_interface.h"
#include "query_sync_object.h"
#include "ref_object.h"
namespace DistributedDB {
using DownloadAssetUnit = std::tuple<std::string, Type, OpType, std::map<std::string, Assets>, Key,
    std::vector<Type>, Timestamp>;
using DownloadList = std::vector<DownloadAssetUnit>;
class ICloudSyncer : public virtual RefObject {
public:
    using TaskId = uint64_t;
    struct CloudTaskInfo {
        bool priorityTask = false;
        int32_t priorityLevel = 0;
        bool compensatedTask = false;
        bool isAssetsOnly = false;
        bool pause = false;
        bool resume = false;
        bool merge = false;
        bool asyncDownloadAssets = false;
        int errCode = 0;
        SyncMode mode = SyncMode::SYNC_MODE_PUSH_ONLY;
        ProcessStatus status = ProcessStatus::PREPARED;
        LockAction lockAction = LockAction::INSERT;
        TaskId taskId = 0u;
        int64_t timeout = 0;
        SyncProcessCallback callback;
        std::vector<std::string> table;
        std::vector<std::string> devices;
        std::vector<QuerySyncObject> queryList;
        std::vector<std::string> users;
        std::string storeId;
        std::string prepareTraceId;
        uint32_t groupNum = 0;
        AssetsGroupMap assetsGroupMap;
    };

    struct UploadRetryInfo {
        uint32_t uploadBatchRetryCount = 0;
        uint32_t downloadBatchOpCount = 0;
    };

    struct InnerProcessInfo {
        std::string tableName;
        ProcessStatus tableStatus = ProcessStatus::PREPARED;
        Info downLoadInfo;
        Info upLoadInfo;
        UploadRetryInfo retryInfo;
        bool isAsyncDownload = false;
    };

    struct WithoutRowIdData {
        std::vector<size_t> insertData = {};
        std::vector<std::tuple<size_t, size_t>> updateData = {};
        std::vector<std::tuple<size_t, size_t>> assetInsertData = {};
    };

    struct SyncParam {
        DownloadData downloadData;
        ChangedData changedData;
        InnerProcessInfo info;
        DownloadList assetsDownloadList;
        std::string cloudWaterMark;
        std::vector<std::string> pkColNames;
        std::set<Key> deletePrimaryKeySet;
        std::set<Key> dupHashKeySet;
        std::string tableName;
        bool isSinglePrimaryKey = false;
        bool isLastBatch = false;
        WithoutRowIdData withoutRowIdData;
        std::vector<std::vector<Type>> insertPk;
        bool isAssetsOnly = false;
        bool isVaildForAssetsOnly = false;
        uint32_t groupNum = 0;
        AssetsGroupMap assetsGroupMap;
        std::string cloudWaterMarkForAssetsOnly;
        std::map<std::string, AssetsMap> gidAssetsMap; // only used for assets only.
        bool isForcePullAseets = false;
    };

    struct DataInfo {
        DataInfoWithLog localInfo;
        LogInfo cloudLogInfo;
    };

    virtual std::string GetIdentify() const = 0;

    virtual bool IsClosed() const = 0;
};
}
#endif // I_CLOUD_SYNCER_H
