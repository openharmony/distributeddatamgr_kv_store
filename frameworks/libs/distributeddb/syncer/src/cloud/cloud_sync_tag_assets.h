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
#ifndef CLOUD_SYNC_TAG_ASSETS_H
#define CLOUD_SYNC_TAG_ASSETS_H

#include <cstdint>
#include <string>

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "cloud/cloud_store_types.h"
#include "cloud/cloud_sync_utils.h"
#include "cloud/icloud_syncer.h"
#include "db_errno.h"
#include "icloud_sync_storage_interface.h"
#include "log_print.h"
#include "platform_specific.h"

namespace DistributedDB {
struct TagAssetsInfo {
    VBucket &coveredData;
    VBucket &beCoveredData;
    bool setNormalStatus = false;
    bool isForcePullAssets = false;
};

struct TagAssetInfo {
    Asset &covered;
    Asset &beCovered;
    bool setNormalStatus = false;
    bool isForcePullAssets = false;
};

struct AssetRecordInfo {
    bool isSkipDownloadAsset = false;
    size_t idx = 0;
    Key hashKey;
    ICloudSyncer::DataInfo dataInfo;
    std::vector<Field> assetFields;
};

class CloudSyncTagAssets {
public:
    static Assets TagAssetsInSingleCol(TagAssetsInfo &tagAssetsInfo, const Field &assetField, int &errCode);
    static Type &GetAssets(const std::string &assetFieldName, VBucket &vBucket);
    static void TagAssetsInSingleCol(const Field &assetField, bool isInsert, VBucket &coveredData);

    static int TagDownloadAssetsTimeFirst(const AssetRecordInfo &info, ICloudSyncer::SyncParam &param,
        VBucket &localAssetInfo);
    static int TagDownloadAssetsTempPath(const AssetRecordInfo &info, ICloudSyncer::SyncParam &param,
        VBucket &localAssetInfo);
    static int GetRecordPrefix(size_t idx, ICloudSyncer::SyncParam &param, const ICloudSyncer::DataInfo &dataInfo,
        Type &prefix, std::vector<Type> &pkVals);
private:
    static void TagDownloadAssetsTimeFirstInner(const AssetRecordInfo &info, VBucket &local, VBucket &cloud,
        std::map<std::string, Assets> &assetsMap);
    static void HandleAssetFieldCloudNoneExist(const Field &field, const VBucket &local, const Type &localType,
        std::map<std::string, Assets> &assetsMap);
    static void HandleAssetFieldLocalNoneExist(const Field &field, const VBucket &cloud, const Type &cloudType,
        std::map<std::string, Assets> &assetsMap);
    static void TagDownloadAssetsTempPathInner(const AssetRecordInfo &info, VBucket &local, VBucket &cloud,
        std::map<std::string, Assets> &assetsMap);
};
} // namespace DistributedDB
#endif // CLOUD_SYNC_TAG_ASSETS_H