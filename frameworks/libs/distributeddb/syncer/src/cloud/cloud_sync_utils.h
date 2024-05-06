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
#ifndef CLOUD_SYNC_UTILS_H
#define CLOUD_SYNC_UTILS_H

#include <cstdint>
#include <string>
#include "cloud/cloud_store_types.h"
#include "cloud_syncer.h"
#include "icloud_sync_storage_interface.h"

namespace DistributedDB {
class CloudSyncUtils {
public:
    static constexpr const int GID_INDEX = 0;
    static constexpr const int PREFIX_INDEX = 1;
    static constexpr const int STRATEGY_INDEX = 2;
    static constexpr const int ASSETS_INDEX = 3;
    static constexpr const int HASH_KEY_INDEX = 4;
    static constexpr const int PRIMARY_KEY_INDEX = 5;
    static constexpr const int TIMESTAMP_INDEX = 6;

    static int GetCloudPkVals(const VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey,
        std::vector<Type> &cloudPkVals);

    static ChangeType OpTypeToChangeType(OpType strategy);

    static bool IsSinglePrimaryKey(const std::vector<std::string> &pKColNames);

    static void RemoveDataExceptExtendInfo(VBucket &datum, const std::vector<std::string> &pkColNames);

    static AssetOpType StatusToFlag(AssetStatus status);

    static void StatusToFlagForAsset(Asset &asset);

    static void StatusToFlagForAssets(Assets &assets);

    static void StatusToFlagForAssetsInRecord(const std::vector<Field> &fields, VBucket &record);

    static bool IsChangeDataEmpty(const ChangedData &changedData);

    static bool EqualInMsLevel(const Timestamp cmp, const Timestamp beCmp);

    static bool NeedSaveData(const LogInfo &localLogInfo, const LogInfo &cloudLogInfo);

    static int CheckParamValid(const std::vector<DeviceID> &devices, SyncMode mode);

    static LogInfo GetCloudLogInfo(VBucket &datum);

    static int SaveChangedDataByType(const VBucket &datum, ChangedData &changedData, const DataInfoWithLog &localInfo,
        ChangeType type);

    static int CheckCloudSyncDataValid(const CloudSyncData &uploadData, const std::string &tableName,
        const int64_t &count, uint64_t &taskId);

    static void ClearCloudSyncData(CloudSyncData &uploadData);

    static int GetWaterMarkAndUpdateTime(std::vector<VBucket>& extend, Timestamp &waterMark);

    static bool CheckCloudSyncDataEmpty(const CloudSyncData &uploadData);

    static void ModifyCloudDataTime(VBucket &data);

    static int UpdateExtendTime(CloudSyncData &uploadData, const int64_t &count, uint64_t taskId,
        Timestamp &waterMark);

    static void UpdateLocalCache(OpType opType, const LogInfo &cloudInfo,
        const LogInfo &localInfo, std::map<std::string, LogInfo> &localLogInfoCache);

    static int SaveChangedData(ICloudSyncer::SyncParam &param, size_t dataIndex, const ICloudSyncer::DataInfo &dataInfo,
        std::vector<std::pair<Key, size_t>> &deletedList);

    static void ClearWithoutData(ICloudSyncer::SyncParam &param);

    static int FillAssetIdToAssets(CloudSyncBatch &data, int errorCode);

    static int FillAssetIdToAssetData(const Type &extend, Type &assetData);

    static int FillAssetIdToAssetsData(const Assets &extend, Assets &assets);

    static bool CheckIfContainsInsertAssets(const Type &assetData);

    static void UpdateAssetsFlag(CloudSyncData &uploadData);

    static OpType CalOpType(ICloudSyncer::SyncParam &param, size_t dataIndex);

    static CloudSyncer::CloudTaskInfo InitCompensatedSyncTaskInfo();
private:
    static void InsertOrReplaceChangedDataByType(ChangeType type, std::vector<Type> &pkVal,
        ChangedData &changedData);
};
}
#endif // CLOUD_SYNC_UTILS_H