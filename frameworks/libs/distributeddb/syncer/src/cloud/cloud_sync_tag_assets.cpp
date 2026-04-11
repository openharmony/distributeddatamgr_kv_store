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
#include "cloud/asset_operation_utils.h"
#include "db_common.h"

namespace DistributedDB {
namespace {
void TagSingleAssetForDownload(AssetOpType flag, Asset &asset, Assets &res, int &errCode)
{
    uint32_t newStatus = static_cast<uint32_t>(AssetStatus::DOWNLOADING);
    if (flag == AssetOpType::DELETE &&
        (AssetOperationUtils::EraseBitMask(asset.status) == AssetStatus::ABNORMAL ||
         asset.status == (AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL))) {
        asset.flag = static_cast<uint32_t>(AssetOpType::DELETE);
        res.push_back(asset);
        return;
    }
    if (AssetOperationUtils::EraseBitMask(asset.status) == static_cast<uint32_t>(AssetStatus::DELETE)) {
        newStatus = AssetStatus::DELETE;
        asset.flag = static_cast<uint32_t>(AssetOpType::DELETE);
    } else {
        asset.flag = static_cast<uint32_t>(flag);
    }
    if (flag == AssetOpType::INSERT) {
        newStatus |= AssetStatus::DOWNLOAD_WITH_NULL;
    }
    asset.status = static_cast<uint32_t>(newStatus);

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

void TagSingleAssetForUpload(AssetOpType flag, Asset &asset, Assets &res)
{
    uint32_t lowBitStatus = AssetOperationUtils::EraseBitMask(asset.status);
    if (lowBitStatus == static_cast<uint32_t>(AssetStatus::DELETE)) {
        return;
    }
    switch (flag) {
        case AssetOpType::INSERT: {
            asset.assetId.clear();
            asset.status = static_cast<uint32_t>(AssetStatus::INSERT);
            break;
        }
        case AssetOpType::DELETE: {
            if (lowBitStatus != static_cast<uint32_t>(AssetStatus::DELETE)) {
                asset.status = static_cast<uint32_t>(AssetStatus::DELETE | AssetStatus::HIDDEN);
            }
            break;
        }
        case AssetOpType::UPDATE: {
            asset.status = static_cast<uint32_t>(AssetStatus::UPDATE);
            break;
        }
        case AssetOpType::NO_CHANGE: {
            asset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
            break;
        }
        default:
            break;
    }
    res.push_back(asset);
}

void TagAssetWithNormalStatus(const bool isNormalStatus, AssetOpType flag,
    Asset &asset, Assets &res, int &errCode)
{
    if (isNormalStatus) {
        TagSingleAssetForUpload(flag, asset, res);
        return;
    }
    TagSingleAssetForDownload(flag, asset, res, errCode);
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
    Type type;
    bool isExisted = CloudStorageUtils::GetTypeCaseInsensitive(assetFieldName, data, type);
    if (!isExisted) {
        return false;
    }
    // When type of Assets is not Nil but a vector which size is 0, we think data is not contain this field.
    if (type.index() == TYPE_INDEX<Assets>) {
        if (std::get<Assets>(type).empty()) {
            return false;
        }
    }
    if (type.index() != TYPE_INDEX<T>) {
        return false;
    }
    return true;
}

void TagAssetWithSameHash(const bool isNormalStatus, Asset &beCoveredAsset, Asset &coveredAsset, Assets &res,
    int &errCode)
{
    TagAssetWithNormalStatus(isNormalStatus, (
        AssetOperationUtils::EraseBitMask(beCoveredAsset.status) == AssetStatus::DELETE ||
        AssetOperationUtils::EraseBitMask(beCoveredAsset.status) == AssetStatus::ABNORMAL ||
        beCoveredAsset.status == (AssetStatus::DOWNLOADING | DOWNLOAD_WITH_NULL)) ?
        AssetOpType::INSERT : AssetOpType::NO_CHANGE, coveredAsset, res, errCode);
}

std::pair<bool, Assets> TagForNotContainsAsset(
    const std::string &assetFieldName, TagAssetsInfo &tagAssetsInfo, int &errCode)
{
    bool setNormalStatus = tagAssetsInfo.setNormalStatus;
    VBucket &coveredData = tagAssetsInfo.coveredData;
    VBucket &beCoveredData = tagAssetsInfo.beCoveredData;
    std::pair<bool, Assets> res = { true, {} };
    bool beCoveredHasAssets = IsDataContainField<Assets>(assetFieldName, beCoveredData);
    bool coveredHasAssets = IsDataContainField<Assets>(assetFieldName, coveredData);
    if (!beCoveredHasAssets) {
        if (coveredHasAssets) {
            // all the element in assets will be set to INSERT
            TagAssetsWithNormalStatus(setNormalStatus, AssetOpType::INSERT,
                std::get<Assets>(CloudSyncTagAssets::GetAssets(assetFieldName, coveredData)), res.second, errCode);
        }
        return res;
    }
    if (!coveredHasAssets) {
        // all the element in assets will be set to DELETE
        TagAssetsWithNormalStatus(setNormalStatus, AssetOpType::DELETE,
            std::get<Assets>(CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData)), res.second, errCode);
        CloudSyncTagAssets::GetAssets(assetFieldName, coveredData) = res.second;
        return res;
    }
    return { false, {} };
}

static Assets TagAssetsInner(const std::string &assetFieldName, TagAssetsInfo &tagAssetsInfo, int &errCode)
{
    VBucket &coveredData = tagAssetsInfo.coveredData;
    VBucket &beCoveredData = tagAssetsInfo.beCoveredData;
    bool setNormalStatus = tagAssetsInfo.setNormalStatus;
    bool isForcePullAssets = tagAssetsInfo.isForcePullAssets;
    Assets res = {};
    if (!std::holds_alternative<Assets>(CloudSyncTagAssets::GetAssets(assetFieldName, coveredData)) ||
        !std::holds_alternative<Assets>(CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData))) {
        LOGE("[TagAssetsInner] coveredData or beCoveredData does not have assets");
        return {};
    }
    Assets &covered = std::get<Assets>(CloudSyncTagAssets::GetAssets(assetFieldName, coveredData));
    Assets &beCovered = std::get<Assets>(CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData));
    std::map<std::string, size_t> coveredAssetsIndexMap = CloudStorageUtils::GenAssetsIndexMap(covered);
    for (Asset &beCoveredAsset : beCovered) {
        auto it = coveredAssetsIndexMap.find(beCoveredAsset.name);
        if (it == coveredAssetsIndexMap.end()) {
            TagAssetWithNormalStatus(setNormalStatus, AssetOpType::DELETE, beCoveredAsset, res, errCode);
            covered.push_back(beCoveredAsset);
            continue;
        }
        Asset &coveredAsset = covered[it->second];
        if (setNormalStatus) {
            // fill asset id for upload data
            coveredAsset.assetId = beCoveredAsset.assetId;
        }
        if (!setNormalStatus && (beCoveredAsset.hash != coveredAsset.hash || isForcePullAssets)) {
            TagAssetWithNormalStatus(setNormalStatus, AssetOpType::UPDATE, coveredAsset, res, errCode);
        } else if (setNormalStatus && beCoveredAsset.hash != coveredAsset.hash) {
            TagAssetWithNormalStatus(setNormalStatus, AssetOpType::UPDATE, coveredAsset, res, errCode);
        } else {
            TagAssetWithSameHash(setNormalStatus, beCoveredAsset, coveredAsset, res, errCode);
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
// use VBucket rather than Type because we need to check whether it is empty
Assets TagAssets(const std::string &assetFieldName, TagAssetsInfo &tagAssetsInfo, int &errCode)
{
    auto [isReturn, resAsset] = TagForNotContainsAsset(assetFieldName, tagAssetsInfo, errCode);
    if (isReturn) {
        return resAsset;
    }
    return TagAssetsInner(assetFieldName, tagAssetsInfo, errCode);
}

static void TagCoveredAssetInner(TagAssetInfo &tagAssetInfo, Assets &res,
    int &errCode)
{
    Asset &covered = tagAssetInfo.covered;
    Asset &beCovered = tagAssetInfo.beCovered;
    bool setNormalStatus = tagAssetInfo.setNormalStatus;
    bool isForcePullAssets = tagAssetInfo.isForcePullAssets;
    if (!setNormalStatus && AssetOperationUtils::IsAssetNeedDownload(beCovered)) {
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::INSERT, covered, res, errCode);
    } else if (covered.hash != beCovered.hash || isForcePullAssets) {
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::UPDATE, covered, res, errCode);
    } else {
        Assets tmpAssets = {};
        TagAssetWithNormalStatus(true, AssetOpType::NO_CHANGE, covered, tmpAssets, errCode);
    }
}

void TagAssetCoveredWithNoAsset(
    const std::string &assetFieldName, TagAssetsInfo &tagAssetsInfo, Assets &res, int &errCode)
{
    VBucket &coveredData = tagAssetsInfo.coveredData;
    VBucket &beCoveredData = tagAssetsInfo.beCoveredData;
    bool setNormalStatus = tagAssetsInfo.setNormalStatus;
    if (CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData).index() == TYPE_INDEX<Asset>) {
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::DELETE,
            std::get<Asset>(CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData)), res, errCode);
        if (!setNormalStatus) {
            // only not normal need fillback asset data
            coveredData[assetFieldName] = std::get<Asset>(CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData));
        }
    } else if (CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData).index() == TYPE_INDEX<Assets>) {
        TagAssetsWithNormalStatus(setNormalStatus, AssetOpType::DELETE,
            std::get<Assets>(CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData)), res, errCode);
    }
}

// AssetOpType and AssetStatus will be tagged, assets to be changed will be returned
Assets TagAsset(const std::string &assetFieldName, TagAssetsInfo &tagAssetsInfo, int &errCode)
{
    VBucket &coveredData = tagAssetsInfo.coveredData;
    VBucket &beCoveredData = tagAssetsInfo.beCoveredData;
    bool setNormalStatus = tagAssetsInfo.setNormalStatus;
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
            std::get<Asset>(CloudSyncTagAssets::GetAssets(assetFieldName, coveredData)), res, errCode);
        return res;
    }
    if (!coveredHasAsset) {
        TagAssetCoveredWithNoAsset(assetFieldName, tagAssetsInfo, res, errCode);
        return res;
    }
    Asset &covered = std::get<Asset>(CloudSyncTagAssets::GetAssets(assetFieldName, coveredData));
    Asset beCovered;
    if (CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData).index() == TYPE_INDEX<Asset>) {
        // This indicates that asset in cloudData is stored as Asset
        beCovered = std::get<Asset>(CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData));
    } else if (CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData).index() == TYPE_INDEX<Assets>) {
        // Stored as ASSETS, first element in assets will be the target asset
        beCovered = (std::get<Assets>(CloudSyncTagAssets::GetAssets(assetFieldName, beCoveredData)))[0];
    } else {
        LOGE("The type of data is neither Asset nor Assets");
        return res;
    }
    if (covered.name != beCovered.name) {
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::INSERT, covered, res, errCode);
        TagAssetWithNormalStatus(setNormalStatus, AssetOpType::DELETE, beCovered, res, errCode);
        return res;
    }
    if (setNormalStatus) {
        // fill asset id for upload data
        covered.assetId = beCovered.assetId;
    }
    TagAssetInfo tagAssetInfo = {covered, beCovered, tagAssetsInfo.setNormalStatus, tagAssetsInfo.isForcePullAssets};
    TagCoveredAssetInner(tagAssetInfo, res, errCode);
    return res;
}

void MarkAssetForUpload(bool isInsert, Asset &asset)
{
    uint32_t lowBitStatus = AssetOperationUtils::EraseBitMask(asset.status);
    if (lowBitStatus == AssetStatus::DELETE) {
        asset.flag = static_cast<uint32_t>(AssetOpType::DELETE);
    } else if (isInsert) {
        asset.flag = static_cast<uint32_t>(AssetOpType::INSERT);
        lowBitStatus = static_cast<uint32_t>(AssetStatus::INSERT);
    } else if (asset.status == AssetStatus::NORMAL) {
        asset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
    } else if (asset.assetId.empty()) {
        asset.flag = static_cast<uint32_t>(AssetOpType::INSERT);
        lowBitStatus = static_cast<uint32_t>(AssetStatus::INSERT);
    } else if (!asset.assetId.empty()) {
        asset.flag = static_cast<uint32_t>(AssetOpType::UPDATE);
        lowBitStatus = static_cast<uint32_t>(AssetStatus::UPDATE);
    } else {
        asset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
    }
    asset.status = lowBitStatus;
    Timestamp timestamp;
    int errCode = OS::GetCurrentSysTimeInMicrosecond(timestamp);
    if (errCode != E_OK) {
        LOGE("Can not get current timestamp. %d", errCode);
        return;
    }
    asset.timestamp = static_cast<int64_t>(timestamp / CloudDbConstant::TEN_THOUSAND);
}

void TagAssetsForUpload(const std::string &filedName, bool isInsert, VBucket &coveredData)
{
    if (!IsDataContainField<Assets>(filedName, coveredData)) {
        return;
    }
    Assets &covered = std::get<Assets>(CloudSyncTagAssets::GetAssets(filedName, coveredData));
    for (auto &asset: covered) {
        MarkAssetForUpload(isInsert, asset);
    }
}

void TagAssetForUpload(const std::string &filedName, bool isInsert, VBucket &coveredData)
{
    if (!IsDataContainField<Asset>(filedName, coveredData)) {
        return;
    }
    Asset &asset = std::get<Asset>(CloudSyncTagAssets::GetAssets(filedName, coveredData));
    MarkAssetForUpload(isInsert, asset);
}
} // namespace

Assets CloudSyncTagAssets::TagAssetsInSingleCol(TagAssetsInfo &tagAssetsInfo, const Field &assetField, int &errCode)
{
    // Define a list to store the tagged result
    Assets assets = {};
    switch (assetField.type) {
        case TYPE_INDEX<Assets>: {
            assets = TagAssets(assetField.colName, tagAssetsInfo, errCode);
            break;
        }
        case TYPE_INDEX<Asset>: {
            assets = TagAsset(assetField.colName, tagAssetsInfo, errCode);
            break;
        }
        default:
            LOGW("[CloudSyncer] Meet an unexpected type %d", assetField.type);
            break;
    }
    return assets;
}

Type &CloudSyncTagAssets::GetAssets(const std::string &assetFieldName, VBucket &vBucket)
{
    for (auto &item : vBucket) {
        if (DBCommon::CaseInsensitiveCompare(item.first, assetFieldName)) {
            return item.second;
        }
    }
    return vBucket[assetFieldName];
}

void CloudSyncTagAssets::TagAssetsInSingleCol(const Field &assetField, bool isInsert, VBucket &coveredData)
{
    switch (assetField.type) {
        case TYPE_INDEX<Assets>: {
            TagAssetsForUpload(assetField.colName, isInsert, coveredData);
            break;
        }
        case TYPE_INDEX<Asset>: {
            TagAssetForUpload(assetField.colName, isInsert, coveredData);
            break;
        }
        default:
            LOGW("[CloudSyncer] Meet an unexpected type %d", assetField.type);
            break;
    }
}

int CloudSyncTagAssets::TagDownloadAssetsTimeFirst(const AssetRecordInfo &info, ICloudSyncer::SyncParam &param,
    VBucket &localAssetInfo)
{
    auto idx = info.idx;
    auto &dataInfo = info.dataInfo;
    Type prefix;
    std::vector<Type> pkVals;
    auto ret = CloudSyncTagAssets::GetRecordPrefix(idx, param, dataInfo, prefix, pkVals);
    if (ret != E_OK) {
        return ret;
    }
    AssetOperationUtils::FilterDeleteAsset(param.downloadData.data[idx]);
    std::map<std::string, Assets> assetsMap;
    TagDownloadAssetsTimeFirstInner(info, localAssetInfo, param.downloadData.data[idx], assetsMap);
    AssetOperationUtils::SetToDownload(info.isSkipDownloadAsset, param.downloadData.data[idx]);
    auto strategy = CloudSyncUtils::CalOpType(param, idx);
    if (!param.isSinglePrimaryKey && strategy == OpType::INSERT) {
        param.withoutRowIdData.assetInsertData.emplace_back(idx, param.assetsDownloadList.size());
    }
    param.assetsDownloadList.push_back(
        std::make_tuple(dataInfo.cloudLogInfo.cloudGid, prefix, strategy, assetsMap, info.hashKey,
            pkVals, dataInfo.cloudLogInfo.timestamp));
    return ret;
}

int CloudSyncTagAssets::TagDownloadAssetsTempPath(const AssetRecordInfo &info, ICloudSyncer::SyncParam &param,
    VBucket &localAssetInfo)
{
    auto idx = info.idx;
    auto &dataInfo = info.dataInfo;
    Type prefix;
    std::vector<Type> pkVals;
    auto ret = CloudSyncTagAssets::GetRecordPrefix(idx, param, dataInfo, prefix, pkVals);
    if (ret != E_OK) {
        return ret;
    }
    AssetOperationUtils::FilterDeleteAsset(param.downloadData.data[idx]);
    std::map<std::string, Assets> assetsMap;
    TagDownloadAssetsTempPathInner(info, localAssetInfo, param.downloadData.data[idx], assetsMap);
    AssetOperationUtils::SetToDownload(info.isSkipDownloadAsset, param.downloadData.data[idx]);
    auto strategy = CloudSyncUtils::CalOpType(param, idx);
    if (!param.isSinglePrimaryKey && strategy == OpType::INSERT) {
        param.withoutRowIdData.assetInsertData.emplace_back(idx, param.assetsDownloadList.size());
    }
    param.assetsDownloadList.push_back(
        std::make_tuple(dataInfo.cloudLogInfo.cloudGid, prefix, strategy, assetsMap, info.hashKey,
            pkVals, dataInfo.cloudLogInfo.timestamp));
    return ret;
}

int CloudSyncTagAssets::GetRecordPrefix(size_t idx, ICloudSyncer::SyncParam &param,
    const ICloudSyncer::DataInfo &dataInfo, Type &prefix, std::vector<Type> &pkVals)
{
    OpType strategy = param.downloadData.opType[idx];
    bool isDelStrategy = (strategy == OpType::DELETE);
    int ret = CloudSyncUtils::GetCloudPkVals(isDelStrategy ? dataInfo.localInfo.primaryKeys :
        param.downloadData.data[idx], param.pkColNames, dataInfo.localInfo.logInfo.dataKey, pkVals);
    if (ret != E_OK || pkVals.empty()) {
        LOGE("[CloudSyncer] HandleTagAssets cannot get primary key value list. ret[%d] pk[%zu]", ret, pkVals.size());
        return ret == E_OK ? -E_INTERNAL_ERROR : ret;
    }
    prefix = param.isSinglePrimaryKey ? pkVals[0] : prefix;
    if (param.isSinglePrimaryKey && prefix.index() == TYPE_INDEX<Nil>) {
        LOGE("[CloudSyncer] Invalid primary key type in TagStatus, it's Nil.");
        return -E_INTERNAL_ERROR;
    }
    return E_OK;
}

namespace {
void MarkAsset(AssetOpType opType, AssetStatus status, Asset &asset)
{
    asset.flag = static_cast<uint32_t>(opType);
    asset.status = static_cast<uint32_t>(status);
    Timestamp timestamp = 0;
    OS::GetCurrentSysTimeInMicrosecond(timestamp);
    asset.timestamp = static_cast<int64_t>(timestamp / CloudDbConstant::TEN_THOUSAND);
}

// Mark assets as delete status with timestamp
Assets MarkAssetsAsDelete(const Assets &assets)
{
    Assets result;
    for (const auto &asset : assets) {
        Asset deleteAsset = asset;
        MarkAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, deleteAsset);
        result.push_back(deleteAsset);
    }
    return result;
}

// Mark single asset as delete status with timestamp
Asset MarkAssetAsDelete(const Asset &asset)
{
    Asset deleteAsset = asset;
    MarkAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, deleteAsset);
    return deleteAsset;
}

// Mark assets as insert status with timestamp
Assets MarkAssetsAsInsert(const Assets &assets)
{
    Assets result;
    for (const auto &asset : assets) {
        Asset insertAsset = asset;
        MarkAsset(AssetOpType::INSERT,
            static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL), insertAsset);
        result.push_back(insertAsset);
    }
    return result;
}

// Mark single asset as insert status with timestamp
Asset MarkAssetAsInsert(const Asset &asset)
{
    Asset insertAsset = asset;
    MarkAsset(AssetOpType::INSERT,
        static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL), insertAsset);
    return insertAsset;
}

// Mark assets as insert status with TEMP flag for temp path policy
Assets MarkAssetsAsInsertWithTemp(const Assets &assets)
{
    Assets result;
    for (const auto &asset : assets) {
        Asset insertAsset = asset;
        MarkAsset(AssetOpType::INSERT,
            static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL |
            AssetStatus::TEMP_PATH), insertAsset);
        result.push_back(insertAsset);
    }
    return result;
}

// Mark single asset as insert status with TEMP flag for temp path policy
Asset MarkAssetAsInsertWithTemp(const Asset &asset)
{
    Asset insertAsset = asset;
    MarkAsset(AssetOpType::INSERT,
        static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL |
        AssetStatus::TEMP_PATH), insertAsset);
    return insertAsset;
}

// Process cloud assets to local assets based on time-first conflict resolution policy
// Compare hash first, if same skip; if different compare modifyTime
Assets ProcessCloudAssetsToLocal(const Assets &cloudAssets, const Assets &localAssets,
    const std::map<std::string, size_t> &localIndexMap)
{
    Assets result;
    for (const auto &cloudAsset : cloudAssets) {
        auto it = localIndexMap.find(cloudAsset.name);
        if (it == localIndexMap.end()) {
            // Cloud asset not found locally, mark as insert
            Asset insertAsset = cloudAsset;
            MarkAsset(AssetOpType::INSERT,
                static_cast<AssetStatus>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL), insertAsset);
            result.push_back(insertAsset);
        } else {
            // Cloud asset found locally, compare hash and modifyTime
            const Asset &localAsset = localAssets[it->second];
            if (localAsset.hash == cloudAsset.hash) {
                continue;
            }
            if (DBCommon::GreaterEqualThan(cloudAsset.modifyTime, localAsset.modifyTime)) {
                // Cloud asset is newer, mark as update
                Asset updateAsset = cloudAsset;
                MarkAsset(AssetOpType::UPDATE,
                    static_cast<AssetStatus>(AssetStatus::DOWNLOADING), updateAsset);
                result.push_back(updateAsset);
            }
        }
    }
    return result;
}

// Process single cloud asset to local asset based on time-first conflict resolution policy
// Compare hash first, if same skip; if different compare modifyTime
Asset ProcessCloudAssetToLocal(const Asset &cloudAsset, const Asset &localAsset)
{
    if (localAsset.hash == cloudAsset.hash) {
        Asset noChangeAsset = localAsset;
        noChangeAsset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
        noChangeAsset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
        return noChangeAsset;
    }
    if (DBCommon::GreaterEqualThan(cloudAsset.modifyTime, localAsset.modifyTime)) {
        // Cloud asset is newer, mark as update
        Asset updateAsset = cloudAsset;
        MarkAsset(AssetOpType::UPDATE,
            static_cast<AssetStatus>(AssetStatus::DOWNLOADING), updateAsset);
        return updateAsset;
    }
    // Local asset is newer or same time, return no change
    Asset noChangeAsset = localAsset;
    noChangeAsset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
    noChangeAsset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
    return noChangeAsset;
}

// Process cloud assets to local assets based on temp path conflict resolution policy
// Compare hash first, if same skip; if different mark cloud as update (cloud wins)
Assets ProcessCloudAssetsToLocalTempPath(const Assets &cloudAssets, const Assets &localAssets,
    const std::map<std::string, size_t> &localIndexMap)
{
    Assets result;
    for (const auto &cloudAsset : cloudAssets) {
        auto it = localIndexMap.find(cloudAsset.name);
        if (it == localIndexMap.end()) {
            // Cloud asset not found locally, mark as insert with TEMP
            Asset insertAsset = cloudAsset;
            insertAsset.flag = static_cast<uint32_t>(AssetOpType::INSERT);
            insertAsset.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL |
                AssetStatus::TEMP_PATH);
            Timestamp timestamp;
            OS::GetCurrentSysTimeInMicrosecond(timestamp);
            insertAsset.timestamp = static_cast<int64_t>(timestamp / CloudDbConstant::TEN_THOUSAND);
            result.push_back(insertAsset);
        } else {
            // Cloud asset found locally, compare hash
            const Asset &localAsset = localAssets[it->second];
            if (localAsset.hash == cloudAsset.hash) {
                continue;
            }
            // Hash is different, cloud wins, mark as update with TEMP
            Asset updateAsset = localAsset;
            updateAsset.hash = cloudAsset.hash;
            updateAsset.assetId = cloudAsset.assetId;
            updateAsset.flag = static_cast<uint32_t>(AssetOpType::UPDATE);
            updateAsset.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING | AssetStatus::TEMP_PATH);
            Timestamp timestamp;
            OS::GetCurrentSysTimeInMicrosecond(timestamp);
            updateAsset.timestamp = static_cast<int64_t>(timestamp / CloudDbConstant::TEN_THOUSAND);
            result.push_back(updateAsset);
        }
    }
    return result;
}

// Process single cloud asset to local asset based on temp path conflict resolution policy
// Compare hash first, if same skip; if different mark cloud as update (cloud wins)
Asset ProcessCloudAssetToLocalTempPath(const Asset &cloudAsset, const Asset &localAsset)
{
    if (localAsset.hash == cloudAsset.hash) {
        Asset noChangeAsset = localAsset;
        noChangeAsset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
        noChangeAsset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
        return noChangeAsset;
    }
    // Hash is different, cloud wins, mark as update with TEMP
    Asset updateAsset = localAsset;
    updateAsset.hash = cloudAsset.hash;
    updateAsset.assetId = cloudAsset.assetId;
    updateAsset.flag = static_cast<uint32_t>(AssetOpType::UPDATE);
    updateAsset.status = static_cast<uint32_t>(AssetStatus::DOWNLOADING | AssetStatus::TEMP_PATH);
    Timestamp timestamp;
    OS::GetCurrentSysTimeInMicrosecond(timestamp);
    updateAsset.timestamp = static_cast<int64_t>(timestamp / CloudDbConstant::TEN_THOUSAND);
    return updateAsset;
}

// Handle conflict when both local and cloud have Assets type
static void HandleAssetsConflict(const Field &field, VBucket &local, VBucket &cloud,
    std::map<std::string, Assets> &assetsMap, bool useTempPath)
{
    auto localVal = local.find(field.colName);
    auto cloudVal = cloud.find(field.colName);
    auto &localAssets = std::get<Assets>(localVal->second);
    auto &cloudAssets = std::get<Assets>(cloudVal->second);
    std::map<std::string, size_t> localIndexMap = CloudStorageUtils::GenAssetsIndexMap(localAssets);
    Assets changedAssets = useTempPath ?
        ProcessCloudAssetsToLocalTempPath(cloudAssets, localAssets, localIndexMap) :
        ProcessCloudAssetsToLocal(cloudAssets, localAssets, localIndexMap);
    if (changedAssets.empty()) {
        return;
    }

    assetsMap[field.colName] = changedAssets;
    auto &cloudAssetsRef = std::get<Assets>(cloudVal->second);
    for (const auto &asset : changedAssets) {
        auto assetIt = std::find_if(cloudAssetsRef.begin(), cloudAssetsRef.end(),
            [&asset](const Asset &a) { return a.name == asset.name; });
        if (assetIt != cloudAssetsRef.end()) {
            assetIt->hash = asset.hash;
            assetIt->assetId = asset.assetId;
            assetIt->modifyTime = asset.modifyTime;
            assetIt->flag = asset.flag;
            assetIt->status = asset.status;
            assetIt->timestamp = asset.timestamp;
        }
    }
}

// Handle conflict when both local and cloud have Asset type
void HandleAssetConflict(const Field &field, VBucket &local, VBucket &cloud,
    std::map<std::string, Assets> &assetsMap, bool useTempPath)
{
    auto localVal = local.find(field.colName);
    auto cloudVal = cloud.find(field.colName);
    auto &localAsset = std::get<Asset>(localVal->second);
    auto &cloudAsset = std::get<Asset>(cloudVal->second);
    Asset changedAsset = useTempPath ?
        ProcessCloudAssetToLocalTempPath(cloudAsset, localAsset) :
        ProcessCloudAssetToLocal(cloudAsset, localAsset);
    if (changedAsset.flag != static_cast<uint32_t>(AssetOpType::NO_CHANGE)) {
        assetsMap[field.colName] = Assets{changedAsset};
    }
    cloud[field.colName] = changedAsset;
}

// Handle asset field conflict for both Asset and Assets types
void HandleAssetFieldConflict(const Field &field, VBucket &local, VBucket &cloud,
    std::map<std::string, Assets> &assetsMap, bool useTempPath)
{
    bool localHasAssets = IsDataContainField<Assets>(field.colName, local);
    bool localHasAsset = IsDataContainField<Asset>(field.colName, local);
    bool cloudHasAssets = IsDataContainField<Assets>(field.colName, cloud);
    bool cloudHasAsset = IsDataContainField<Asset>(field.colName, cloud);
    if ((!localHasAssets && !localHasAsset) || (!cloudHasAssets && !cloudHasAsset)) {
        return;
    }

    if (localHasAssets && cloudHasAssets) {
        HandleAssetsConflict(field, local, cloud, assetsMap, useTempPath);
    } else if (localHasAsset && cloudHasAsset) {
        HandleAssetConflict(field, local, cloud, assetsMap, useTempPath);
    }
}
} // namespace

// Handle local-only assets case
static void HandleLocalOnlyAssets(const Field &field, VBucket &local,
    std::map<std::string, Assets> &assetsMap)
{
    if (!IsDataContainField<Assets>(field.colName, local) &&
        !IsDataContainField<Asset>(field.colName, local)) {
        return;
    }
    auto localVal = local.find(field.colName);
    if (IsDataContainField<Assets>(field.colName, local)) {
        Assets assetsToDelete = MarkAssetsAsDelete(std::get<Assets>(localVal->second));
        if (!assetsToDelete.empty()) {
            assetsMap[field.colName] = assetsToDelete;
        }
    } else if (IsDataContainField<Asset>(field.colName, local)) {
        Asset assetToDelete = MarkAssetAsDelete(std::get<Asset>(localVal->second));
        assetsMap[field.colName] = Assets{assetToDelete};
    }
}

// Handle cloud-only assets case
static void HandleCloudOnlyAssets(const Field &field, VBucket &cloud,
    std::map<std::string, Assets> &assetsMap, bool useTempPath)
{
    if (!IsDataContainField<Assets>(field.colName, cloud) &&
        !IsDataContainField<Asset>(field.colName, cloud)) {
        return;
    }
    auto cloudVal = cloud.find(field.colName);
    if (IsDataContainField<Assets>(field.colName, cloud)) {
        Assets assetsToInsert = useTempPath ?
            MarkAssetsAsInsertWithTemp(std::get<Assets>(cloudVal->second)) :
            MarkAssetsAsInsert(std::get<Assets>(cloudVal->second));
        if (!assetsToInsert.empty()) {
            assetsMap[field.colName] = assetsToInsert;
        }
    } else if (IsDataContainField<Asset>(field.colName, cloud)) {
        Asset assetToInsert = useTempPath ?
            MarkAssetAsInsertWithTemp(std::get<Asset>(cloudVal->second)) :
            MarkAssetAsInsert(std::get<Asset>(cloudVal->second));
        assetsMap[field.colName] = Assets{assetToInsert};
    }
}

// Tag download assets with temp path conflict resolution policy
// Cloud wins for conflicts, mark assets with TEMP status
void CloudSyncTagAssets::TagDownloadAssetsTempPathInner(const AssetRecordInfo &info, VBucket &local,
    VBucket &cloud, std::map<std::string, Assets> &assetsMap)
{
    for (const auto &field : info.assetFields) {
        auto localVal = local.find(field.colName);
        auto cloudVal = cloud.find(field.colName);
        if (localVal == local.end() && cloudVal == cloud.end()) {
            continue;
        }
        if (localVal != local.end() && cloudVal == cloud.end()) {
            HandleLocalOnlyAssets(field, local, assetsMap);
            continue;
        }
        if (localVal == local.end() && cloudVal != cloud.end()) {
            HandleCloudOnlyAssets(field, cloud, assetsMap, true);
            continue;
        }
        HandleAssetFieldConflict(field, local, cloud, assetsMap, true);
    }
}

// Tag download assets with time-first conflict resolution policy
// Compare local and cloud asset data, resolve conflicts based on modifyTime
void CloudSyncTagAssets::TagDownloadAssetsTimeFirstInner(const AssetRecordInfo &info, VBucket &local,
    VBucket &cloud, std::map<std::string, Assets> &assetsMap)
{
    for (const auto &field : info.assetFields) {
        auto localVal = local.find(field.colName);
        auto cloudVal = cloud.find(field.colName);
        if (localVal == local.end() && cloudVal == cloud.end()) {
            continue;
        }
        if (localVal != local.end() && cloudVal == cloud.end()) {
            // Local exists, cloud not exists: mark local assets as delete
            if (!IsDataContainField<Assets>(field.colName, local) && !IsDataContainField<Asset>(field.colName, local)) {
                continue;
            }
            HandleAssetFieldCloudNoneExist(field, local, localVal->second, assetsMap);
            continue;
        }
        if (localVal == local.end() && cloudVal != cloud.end()) {
            // Local not exists, cloud exists: mark cloud assets as insert
            if (!IsDataContainField<Assets>(field.colName, cloud) && !IsDataContainField<Asset>(field.colName, cloud)) {
                continue;
            }
            HandleAssetFieldLocalNoneExist(field, cloud, cloudVal->second, assetsMap);
            continue;
        }
        // Both local and cloud exist: compare and resolve conflicts based on modifyTime
        HandleAssetFieldConflict(field, local, cloud, assetsMap, false);
    }
}

void CloudSyncTagAssets::HandleAssetFieldCloudNoneExist(const Field &field, const VBucket &local, const Type &localType,
    std::map<std::string, Assets> &assetsMap)
{
    if (IsDataContainField<Assets>(field.colName, local)) {
        Assets assetsToDelete = MarkAssetsAsDelete(std::get<Assets>(localType));
        if (!assetsToDelete.empty()) {
            assetsMap[field.colName] = assetsToDelete;
        }
    } else if (IsDataContainField<Asset>(field.colName, local)) {
        Asset assetToDelete = MarkAssetAsDelete(std::get<Asset>(localType));
        assetsMap[field.colName] = Assets{assetToDelete};
    }
}

void CloudSyncTagAssets::HandleAssetFieldLocalNoneExist(const Field &field, const VBucket &cloud, const Type &cloudType,
    std::map<std::string, Assets> &assetsMap)
{
    if (IsDataContainField<Assets>(field.colName, cloud)) {
        Assets assetsToInsert = MarkAssetsAsInsert(std::get<Assets>(cloudType));
        if (!assetsToInsert.empty()) {
            assetsMap[field.colName] = assetsToInsert;
        }
    } else if (IsDataContainField<Asset>(field.colName, cloud)) {
        Asset assetToInsert = MarkAssetAsInsert(std::get<Asset>(cloudType));
        assetsMap[field.colName] = Assets{assetToInsert};
    }
}
} // namespace DistributedDB
