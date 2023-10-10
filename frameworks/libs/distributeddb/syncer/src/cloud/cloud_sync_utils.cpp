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
#include "db_errno.h"
#include "log_print.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_sync_utils.h"

namespace DistributedDB {

int GetCloudPkVals(const VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey,
    std::vector<Type> &cloudPkVals)
{
    if (!cloudPkVals.empty()) {
        LOGE("[CloudSyncer] Output paramater should be empty");
        return -E_INVALID_ARGS;
    }
    for (const auto &pkColName : pkColNames) {
        // if data is primary key or is a composite primary key, then use rowID as value
        // The single primary key table, does not contain rowid.
        if (pkColName == CloudDbConstant::ROW_ID_FIELD_NAME) {
            cloudPkVals.emplace_back(dataKey);
            continue;
        }
        auto iter = datum.find(pkColName);
        if (iter == datum.end()) {
            LOGE("[CloudSyncer] Cloud data do not contain expected primary field value");
            return -E_CLOUD_ERROR;
        }
        cloudPkVals.push_back(iter->second);
    }
    return E_OK;
}

ChangeType OpTypeToChangeType(OpType strategy)
{
    switch (strategy) {
        case OpType::INSERT:
            return OP_INSERT;
        case OpType::DELETE:
            return OP_DELETE;
        case OpType::UPDATE:
            return OP_UPDATE;
        default:
            return OP_BUTT;
    }
}

bool IsSinglePrimaryKey(const std::vector<std::string> &pkColNames)
{
    return pkColNames.size() == 1 && pkColNames[0] != CloudDbConstant::ROW_ID_FIELD_NAME;
}

void RemoveDataExceptExtendInfo(VBucket &datum, const std::vector<std::string> &pkColNames)
{
    for (auto item = datum.begin(); item != datum.end();) {
        const auto &key = item->first;
        if (key != CloudDbConstant::GID_FIELD &&
            key != CloudDbConstant::CREATE_FIELD &&
            key != CloudDbConstant::MODIFY_FIELD &&
            key != CloudDbConstant::DELETE_FIELD &&
            key != CloudDbConstant::CURSOR_FIELD &&
            (std::find(pkColNames.begin(), pkColNames.end(), key) == pkColNames.end())) {
                item = datum.erase(item);
            } else {
                item++;
            }
    }
}

AssetOpType StatusToFlag(AssetStatus status)
{
    switch (status) {
        case AssetStatus::INSERT:
            return AssetOpType::INSERT;
        case AssetStatus::DELETE:
            return AssetOpType::DELETE;
        case AssetStatus::UPDATE:
            return AssetOpType::UPDATE;
        case AssetStatus::NORMAL:
            return AssetOpType::NO_CHANGE;
        default:
            LOGW("[CloudSyncer] Unexpected Situation and won't be handled"
                ", Caller should ensure that current situation won't occur");
            return AssetOpType::NO_CHANGE;
    }
}

void StatusToFlagForAsset(Asset &asset)
{
    asset.flag = static_cast<uint32_t>(StatusToFlag(static_cast<AssetStatus>(asset.status)));
    asset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
}

void StatusToFlagForAssets(Assets &assets)
{
    for (Asset &asset : assets) {
        StatusToFlagForAsset(asset);
    }
}

void StatusToFlagForAssetsInRecord(const std::vector<Field> &fields, VBucket &record)
{
    for (const Field &field : fields) {
        if (field.type == TYPE_INDEX<Assets> && record[field.colName].index() == TYPE_INDEX<Assets>) {
            StatusToFlagForAssets(std::get<Assets>(record[field.colName]));
        } else if (field.type == TYPE_INDEX<Asset> && record[field.colName].index() == TYPE_INDEX<Asset>) {
            StatusToFlagForAsset(std::get<Asset>(record[field.colName]));
        }
    }
}

bool IsChngDataEmpty(const ChangedData &changedData)
{
    return changedData.primaryData[ChangeType::OP_INSERT].empty() ||
           changedData.primaryData[ChangeType::OP_UPDATE].empty() ||
           changedData.primaryData[ChangeType::OP_DELETE].empty();
}

bool EqualInMsLevel(const Timestamp cmp, const Timestamp beCmp)
{
    return cmp / CloudDbConstant::TEN_THOUSAND == beCmp / CloudDbConstant::TEN_THOUSAND;
}

bool NeedSaveData(const LogInfo &localLogInfo, const LogInfo &cloudLogInfo)
{
    // if timeStamp, write timestamp, cloudGid are all the same,
    // we thought that the datum is mostly be the same between cloud and local
    // However, there are still slightly possibility that it may be created from different device,
    // So, during the strategy policy [i.e. TagSyncDataStatus], the datum was tagged as UPDATE
    // But we won't notify the datum
    bool isSame = localLogInfo.timestamp == cloudLogInfo.timestamp &&
        EqualInMsLevel(localLogInfo.wTimestamp, cloudLogInfo.wTimestamp) &&
        localLogInfo.cloudGid == cloudLogInfo.cloudGid;
    return !isSame;
}
}