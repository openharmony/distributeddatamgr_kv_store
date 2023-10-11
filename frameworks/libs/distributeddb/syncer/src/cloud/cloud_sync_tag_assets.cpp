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
#include "cloud/cloud_sync_tag_assets.h"

namespace DistributedDB {
namespace {

void TagSingleAsset(AssetOpType flag, AssetStatus status, Asset &asset, Assets &res, int &errCode)
{
    if (asset.status == static_cast<uint32_t>(AssetStatus::DELETE)) {
        asset.flag = static_cast<uint32_t>(AssetOpType::DELETE);
    } else {
        asset.flag = static_cast<uint32_t>(flag);
    }
    asset.status = static_cast<uint32_t>(status);

    Timestamp timestamp;
    errCode = OS::GetCurrentSysTimeInMicrosecond(timestamp);
    if (errCode != E_OK) {
        LOGE("Can not get current timestamp.");
        return;
    }
    asset.timestamp = static_cast<int64_t>(timestamp / CloudDbConstant::TEN_THOUSAND);
    asset.status = asset.flag == static_cast<uint32_t>(AssetOpType::NO_CHANGE) ?
        static_cast<uint32_t>(AssetStatus::NORMAL) : asset.status;
    res.push_back(asset);
}

void TagAssetWithNormalStatus(const bool isNormalStatus, AssetOpType flag,
    Asset &asset, Assets &res, int &errCode)
{
    if (isNormalStatus) {
        TagSingleAsset(flag, AssetStatus::NORMAL, asset, res, errCode);
        return;
    }
    TagSingleAsset(flag, AssetStatus::DOWNLOADING, asset, res, errCode);
}

void TagAssetsWithNormalStatus(const bool isNormalStatus, AssetOpType flag,
    Assets &assets, Assets &res, int &errCode)
{
    for (Asset &asset : assets) {
        TagAssetWithNormalStatus(isNormalStatus, flag, asset, res, errCode);
        if (errCode != E_OK) {
            break;
        }
    }
}

template<typename T>
bool IsDataContainField(const std::string &assetFieldName, const VBucket &data)
{
    auto assetIter = data.find(assetFieldName);
    if (assetIter == data.end()) {
        return false;
    }
    // When type of Assets is not Nil but a vector which size is 0, we think data is not contain this field.
    if (assetIter->second.index() == TYPE_INDEX<Assets>) {
        if (std::get<Assets>(assetIter->second).empty()) {
            return false;
        }
    }
    if (assetIter->second.index() != TYPE_INDEX<T>) {
        return false;
    }
    return true;
}

// AssetOpType and AssetStatus will be tagged, assets to be changed will be returned
// use VBucket rather than Type because we need to check whether it is empty
Assets TagAssets(const std::string &assetFieldName, VBucket &coveredData, VBucket &beCoveredData,
    bool setNormalStatus, int &errCode)
{
    Assets res = {};
    bool beCoveredHasAssets = IsDataContainField<Assets>(assetFieldName, beCoveredData);
    bool coveredHasAssets = IsDataContainField<Assets>(assetFieldName, coveredData);
    if (!beCoveredHasAssets) {
        if (!coveredHasAssets) {
            return res;
        }
        // all the element in assets will be set to INSERT
        TagAssetsWithNormalStatus(setNormalStatus,
            AssetOpType::INSERT, std::get<Assets>(coveredData[assetFieldName]), res, errCode);
        return res;
    }
    if (!coveredHasAssets) {
        // all the element in assets will be set to DELETE
        TagAssetsWithNormalStatus(setNormalStatus,
            AssetOpType::DELETE, std::get<Assets>(beCoveredData[assetFieldName]), res, errCode);
        coveredData[assetFieldName] = res;
        return res;
    }
    Assets &covered = std::get<Assets>(coveredData[assetFieldName]);
    Assets &beCovered = std::get<Assets>(beCoveredData[assetFieldName]);
    std::map<std::string, size_t> coveredAssetsIndexMap = CloudStorageUtils::GenAssetsIndexMap(covered);
    for (Asset &beCoveredAsset : beCovered) {
        auto it = coveredAssetsIndexMap.find(beCoveredAsset.name);
        if (it == coveredAssetsIndexMap.end()) {
            TagAssetWithNormalStatus(setNormalStatus, AssetOpType::DELETE, beCoveredAsset, res, errCode);
            std::get<Assets>(coveredData[assetFieldName]).push_back(beCoveredAsset);
            continue;
        }
        Asset &coveredAsset = covered[it->second];
        if (beCoveredAsset.hash != coveredAsset.hash) {
            TagAssetWithNormalStatus(setNormalStatus, AssetOpType::UPDATE, coveredAsset, res, errCode);
        } else {
            TagAssetWithNormalStatus(setNormalStatus, AssetOpType::NO_CHANGE, coveredAsset, res, errCode);
        }
        // Erase element which has been handled, remaining element will be set to Insert
        coveredAssetsIndexMap.erase(it);
        if (errCode != E_OK) {
            LOGE("Tag assets UPDATE or NO_CHANGE fail!");
            return {};
        }
    }
    for (const auto &noHandledAssetKvPair : coveredAssetsIndexMap) {
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::INSERT,
            covered[noHandledAssetKvPair.second], res, errCode);
        if (errCode != E_OK) {
            LOGE("Tag assets INSERT fail!");
            return {};
        }
    }
    return res;
}

// AssetOpType and AssetStatus will be tagged, assets to be changed will be returned
Assets TagAsset(const std::string &assetFieldName, VBucket &coveredData, VBucket &beCoveredData,
    bool setNormalStatus, int &errCode)
{
    Assets res = {};
    bool beCoveredHasAsset = IsDataContainField<Asset>(assetFieldName, beCoveredData) ||
        IsDataContainField<Assets>(assetFieldName, beCoveredData);
    bool coveredHasAsset = IsDataContainField<Asset>(assetFieldName, coveredData);
    if (!beCoveredHasAsset) {
        if (!coveredHasAsset) {
            LOGD("[CloudSyncer] Both data do not contain certain asset field");
            return res;
        }
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::INSERT,
            std::get<Asset>(coveredData[assetFieldName]), res, errCode);
        return res;
    }
    if (!coveredHasAsset) {
        if (beCoveredData[assetFieldName].index() == TYPE_INDEX<Asset>) {
            TagAssetWithNormalStatus(setNormalStatus, AssetOpType::DELETE,
                std::get<Asset>(beCoveredData[assetFieldName]), res, errCode);
        } else if (beCoveredData[assetFieldName].index() == TYPE_INDEX<Assets>) {
            TagAssetsWithNormalStatus(setNormalStatus, AssetOpType::DELETE,
                std::get<Assets>(beCoveredData[assetFieldName]), res, errCode);
        }
        return res;
    }
    Asset &covered = std::get<Asset>(coveredData[assetFieldName]);
    Asset beCovered;
    if (beCoveredData[assetFieldName].index() == TYPE_INDEX<Asset>) {
        // This indicates that asset in cloudData is stored as Asset
        beCovered = std::get<Asset>(beCoveredData[assetFieldName]);
    } else if (beCoveredData[assetFieldName].index() == TYPE_INDEX<Assets>) {
        // Stored as ASSETS, first element in assets will be the target asset
        beCovered = (std::get<Assets>(beCoveredData[assetFieldName]))[0];
    } else {
        LOGE("The type of data is neither Asset nor Assets");
        return res;
    }
    if (covered.name != beCovered.name) {
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::INSERT, covered, res, errCode);
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::DELETE, beCovered, res, errCode);
        return res;
    }
    if (covered.hash != beCovered.hash) {
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::UPDATE, covered, res, errCode);
    } else {
        Assets tmpAssets = {};
        TagAssetWithNormalStatus(true, AssetOpType::NO_CHANGE, covered, tmpAssets, errCode);
    }
    return res;
}
} // namespace

Assets TagAssetsInSingleCol(
    VBucket &coveredData, VBucket &beCoveredData, const Field &assetField, bool setNormalStatus, int &errCode)
{
    // Define a list to store the tagged result
    Assets assets = {};
    switch (assetField.type) {
        case TYPE_INDEX<Assets>: {
            assets = TagAssets(assetField.colName, coveredData, beCoveredData, setNormalStatus, errCode);
            break;
        }
        case TYPE_INDEX<Asset>: {
            assets = TagAsset(assetField.colName, coveredData, beCoveredData, setNormalStatus, errCode);
            break;
        }
        default:
            LOGW("[CloudSyncer] Meet an unexpected type %d", assetField.type);
            break;
    }
    return assets;
}
} // namespace DistributedDB
