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

#ifndef ASSET_OPERATION_UTILS_H
#define ASSET_OPERATION_UTILS_H
#include "cloud/cloud_store_types.h"

namespace DistributedDB {
class AssetOperationUtils {
public:
    enum class AssetOpType {
        HANDLE,
        NOT_HANDLE,
    };
    enum class CloudSyncAction : uint16_t {
        START_DOWNLOAD = 0,
        START_UPLOAD,
        END_DOWNLOAD,
        END_UPLOAD,
        DEFAULT_ACTION
    };
    using RecordAssetOpType = std::map<std::string, std::map<std::string, AssetOperationUtils::AssetOpType>>;
    static RecordAssetOpType CalAssetOperation(const VBucket &cacheAssets, const VBucket &dbAssets,
        const CloudSyncAction &action);
    static AssetOperationUtils::AssetOpType CalAssetOperation(const std::string &colName, const Asset &cacheAsset,
        const VBucket &dbAssets, const CloudSyncAction &action);
    static uint32_t EraseBitMask(uint32_t status);
    static void UpdateAssetsFlag(std::vector<VBucket> &from, std::vector<VBucket> &target);
    static void FilterDeleteAsset(VBucket &record);
private:
    static void Init();
    static AssetOperationUtils::AssetOpType DefaultOperation(const Asset &, const Assets &);
    static AssetOperationUtils::AssetOpType CheckBeforeDownload(const Asset &cacheAsset, const Assets &dbAssets);
    static AssetOperationUtils::AssetOpType CheckAfterDownload(const Asset &cacheAsset, const Assets &dbAssets);
    static AssetOperationUtils::AssetOpType CheckWithDownload(bool before, const Asset &cacheAsset,
        const Assets &dbAssets);
    static AssetOperationUtils::AssetOpType CheckAfterUpload(const Asset &cacheAsset, const Assets &dbAssets);
    static AssetOperationUtils::AssetOpType HandleIfExistAndSameStatus(const Asset &cacheAsset, const Assets &dbAssets);
    static Assets GetAssets(const std::string &colName, const VBucket &rowData);
    static void MergeAssetsFlag(const Assets &from, Type &target);
    static void MergeAssetFlag(const Assets &from, Asset &target);
    static constexpr uint32_t BIT_MASK_COUNT = 16;
};
}
#endif // ASSET_OPERATION_UTILS_H
