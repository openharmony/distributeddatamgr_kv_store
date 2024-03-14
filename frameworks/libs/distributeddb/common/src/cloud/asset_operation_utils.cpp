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
#include "cloud/asset_operation_utils.h"

#include <mutex>
#include "cloud/cloud_db_types.h"
#include "runtime_context.h"
namespace DistributedDB {
using RecordAssetOpType = AssetOperationUtils::RecordAssetOpType;
using CloudSyncAction = AssetOperationUtils::CloudSyncAction;
namespace {
std::once_flag g_init;
using Reaction = std::function<AssetOperationUtils::AssetOpType (const Asset &, const Assets &)>;
std::map<CloudSyncAction, Reaction> g_reactions;
Reaction GetReaction(const CloudSyncAction &action)
{
    if (g_reactions.find(action) != g_reactions.end()) {
        return g_reactions[action];
    } else {
        return g_reactions[CloudSyncAction::DEFAULT_ACTION];
    }
}
}

RecordAssetOpType AssetOperationUtils::CalAssetOperation(const VBucket &cacheAssets,
    const VBucket &dbAssets, const CloudSyncAction &action)
{
    std::call_once(g_init, Init);
    // switch produce function by action
    Reaction reaction = GetReaction(action);
    RecordAssetOpType res;
    // check each cache asset with db asset by same col name and asset name
    for (const auto &[colName, colData] : cacheAssets) {
        auto checkAssets = GetAssets(colName, dbAssets);
        if (TYPE_INDEX<Asset> == colData.index()) {
            auto asset = std::get<Asset>(colData);
            res[colName][asset.name] = reaction(asset, checkAssets);
        } else if (TYPE_INDEX<Assets> == colData.index()) {
            auto assets = std::get<Assets>(colData);
            for (const auto &asset : assets) {
                res[colName][asset.name] = reaction(asset, checkAssets);
            }
        }
    }
    return res;
}

AssetOperationUtils::AssetOpType AssetOperationUtils::CalAssetOperation(const std::string &colName,
    const Asset &cacheAsset, const VBucket &dbAssets, const AssetOperationUtils::CloudSyncAction &action)
{
    std::call_once(g_init, Init);
    // switch produce function by action
    Reaction reaction = GetReaction(action);
    return reaction(cacheAsset, GetAssets(colName, dbAssets));
}

uint32_t AssetOperationUtils::EraseBitMask(uint32_t status)
{
    return ((status << BIT_MASK_COUNT) >> BIT_MASK_COUNT);
}

void AssetOperationUtils::UpdateAssetsFlag(std::vector<VBucket> &from, std::vector<VBucket> &target)
{
    if (from.size() != target.size()) {
        LOGW("the num of VBucket are not equal when update assets flag.");
        return;
    }
    for (size_t i = 0; i < from.size(); ++i) {
        VBucket &fromRecord = from[i];
        VBucket &targetRecord = target[i];
        if (targetRecord.empty()) {
            continue;
        }
        for (auto &[colName, colData] : targetRecord) {
            auto fromAssets = GetAssets(colName, fromRecord);
            MergeAssetsFlag(fromAssets, colData);
        }
    }
}

void AssetOperationUtils::FilterDeleteAsset(VBucket &record)
{
    int filterCount = 0;
    for (auto &item : record) {
        if (item.second.index() == TYPE_INDEX<Asset>) {
            auto &asset = std::get<Asset>(item.second);
            if (EraseBitMask(asset.status) == static_cast<uint32_t>(AssetStatus::DELETE)) {
                item.second = Nil();
                filterCount++;
            }
            continue;
        }
        if (item.second.index() != TYPE_INDEX<Assets>) {
            continue;
        }
        auto &assets = std::get<Assets>(item.second);
        auto it = assets.begin();
        while (it != assets.end()) {
            if (EraseBitMask(it->status) == static_cast<uint32_t>(AssetStatus::DELETE)) {
                it = assets.erase(it);
                filterCount++;
            }
            it++;
        }
    }
    if (filterCount > 0) {
        LOGW("[AssetOperationUtils] Filter %d asset", filterCount);
    }
}

void AssetOperationUtils::Init()
{
    g_reactions[CloudSyncAction::DEFAULT_ACTION] = DefaultOperation;
    g_reactions[CloudSyncAction::START_DOWNLOAD] = CheckBeforeDownload;
    g_reactions[CloudSyncAction::START_UPLOAD] = HandleIfExistAndSameStatus;
    g_reactions[CloudSyncAction::END_DOWNLOAD] = CheckAfterDownload;
    g_reactions[CloudSyncAction::END_UPLOAD] = CheckAfterUpload;
}

AssetOperationUtils::AssetOpType AssetOperationUtils::DefaultOperation(const Asset &, const Assets &)
{
    return AssetOpType::HANDLE;
}

AssetOperationUtils::AssetOpType AssetOperationUtils::CheckBeforeDownload(const Asset &cacheAsset,
    const Assets &dbAssets)
{
    return CheckWithDownload(true, cacheAsset, dbAssets);
}

AssetOperationUtils::AssetOpType AssetOperationUtils::CheckAfterDownload(const Asset &cacheAsset,
    const Assets &dbAssets)
{
    return CheckWithDownload(false, cacheAsset, dbAssets);
}

AssetOperationUtils::AssetOpType AssetOperationUtils::CheckWithDownload(bool before, const Asset &cacheAsset,
    const Assets &dbAssets)
{
    for (const auto &dbAsset : dbAssets) {
        if (dbAsset.name != cacheAsset.name) {
            continue;
        }
        if (EraseBitMask(dbAsset.status) == AssetStatus::DOWNLOADING) {
            return AssetOpType::HANDLE;
        }
        return AssetOpType::NOT_HANDLE;
    }
    if (before) {
        if (cacheAsset.status == (AssetStatus::DOWNLOADING | AssetStatus::DOWNLOAD_WITH_NULL) ||
            EraseBitMask(cacheAsset.status) == AssetStatus::ABNORMAL) {
            return AssetOpType::NOT_HANDLE;
        }
        return (cacheAsset.flag == static_cast<uint32_t>(DistributedDB::AssetOpType::DELETE) &&
            EraseBitMask(cacheAsset.status) != AssetStatus::DELETE) ?
            AssetOpType::HANDLE : AssetOpType::NOT_HANDLE;
    }
    return AssetOpType::NOT_HANDLE;
}

AssetOperationUtils::AssetOpType AssetOperationUtils::CheckAfterUpload(const Asset &cacheAsset, const Assets &dbAssets)
{
    for (const auto &dbAsset : dbAssets) {
        if (dbAsset.name != cacheAsset.name) {
            continue;
        }
        if ((dbAsset.status & static_cast<uint32_t>(AssetStatus::UPLOADING)) ==
            static_cast<uint32_t>(AssetStatus::UPLOADING)) {
            return AssetOpType::HANDLE;
        }
        return AssetOpType::NOT_HANDLE;
    }
    return AssetOpType::NOT_HANDLE;
}

Assets AssetOperationUtils::GetAssets(const std::string &colName, const VBucket &rowData)
{
    if (rowData.find(colName) == rowData.end()) {
        return {};
    }
    Assets res;
    auto value = rowData.at(colName);
    if (TYPE_INDEX<Asset> == value.index()) {
        res.push_back(std::get<Asset>(value));
    } else if (TYPE_INDEX<Assets> == value.index()) {
        for (const auto &asset : std::get<Assets>(value)) {
            res.push_back(asset);
        }
    }
    return res;
}

AssetOperationUtils::AssetOpType AssetOperationUtils::HandleIfExistAndSameStatus(const Asset &cacheAsset,
    const Assets &dbAssets)
{
    for (const auto &dbAsset : dbAssets) {
        if (dbAsset.name != cacheAsset.name) {
            continue;
        }
        if (dbAsset.status == cacheAsset.status) {
            return AssetOpType::HANDLE;
        }
        return AssetOpType::NOT_HANDLE;
    }
    return AssetOpType::NOT_HANDLE;
}

void AssetOperationUtils::MergeAssetsFlag(const Assets &from, Type &target)
{
    if (TYPE_INDEX<Asset> == target.index()) {
        MergeAssetFlag(from, std::get<Asset>(target));
    } else if (TYPE_INDEX<Assets> == target.index()) {
        for (auto &targetAsset : std::get<Assets>(target)) {
            MergeAssetFlag(from, targetAsset);
        }
    }
}

void AssetOperationUtils::MergeAssetFlag(const Assets &from, Asset &target)
{
    for (const auto &fromAsset : from) {
        if (fromAsset.name == target.name) {
            target.flag = fromAsset.flag;
        }
    }
}
}