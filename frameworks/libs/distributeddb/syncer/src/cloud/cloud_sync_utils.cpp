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
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "db_errno.h"
#include "log_print.h"
#include "cloud/cloud_sync_utils.h"

namespace DistributedDB {
int CloudSyncUtils::GetCloudPkVals(const VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey,
    std::vector<Type> &cloudPkVals)
{
    if (!cloudPkVals.empty()) {
        LOGE("[CloudSyncer] Output parameter should be empty");
        return -E_INVALID_ARGS;
    }
    for (const auto &pkColName : pkColNames) {
        // If data is primary key or is a composite primary key, then use rowID as value
        // The single primary key table, does not contain rowid.
        if (pkColName == CloudDbConstant::ROW_ID_FIELD_NAME) {
            cloudPkVals.emplace_back(dataKey);
            continue;
        }
        Type type;
        bool isExisted = CloudStorageUtils::GetTypeCaseInsensitive(pkColName, datum, type);
        if (!isExisted) {
            LOGE("[CloudSyncer] Cloud data do not contain expected primary field value");
            return -E_CLOUD_ERROR;
        }
        cloudPkVals.push_back(type);
    }
    return E_OK;
}

ChangeType CloudSyncUtils::OpTypeToChangeType(OpType strategy)
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

bool CloudSyncUtils::IsSinglePrimaryKey(const std::vector<std::string> &pkColNames)
{
    return pkColNames.size() == 1 && pkColNames[0] != CloudDbConstant::ROW_ID_FIELD_NAME;
}

void CloudSyncUtils::RemoveDataExceptExtendInfo(VBucket &datum, const std::vector<std::string> &pkColNames)
{
    for (auto item = datum.begin(); item != datum.end();) {
        const auto &key = item->first;
        if (key != CloudDbConstant::GID_FIELD &&
            key != CloudDbConstant::CREATE_FIELD &&
            key != CloudDbConstant::MODIFY_FIELD &&
            key != CloudDbConstant::DELETE_FIELD &&
            key != CloudDbConstant::CURSOR_FIELD &&
            key != CloudDbConstant::VERSION_FIELD &&
            key != CloudDbConstant::SHARING_RESOURCE_FIELD &&
            (std::find(pkColNames.begin(), pkColNames.end(), key) == pkColNames.end())) {
                item = datum.erase(item);
            } else {
                item++;
            }
    }
}

AssetOpType CloudSyncUtils::StatusToFlag(AssetStatus status)
{
    auto tmpStatus = static_cast<AssetStatus>(AssetOperationUtils::EraseBitMask(static_cast<uint32_t>(status)));
    switch (tmpStatus) {
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

void CloudSyncUtils::StatusToFlagForAsset(Asset &asset)
{
    asset.flag = static_cast<uint32_t>(StatusToFlag(static_cast<AssetStatus>(asset.status)));
    asset.status = static_cast<uint32_t>(AssetStatus::NORMAL);
}

void CloudSyncUtils::StatusToFlagForAssets(Assets &assets)
{
    for (Asset &asset : assets) {
        StatusToFlagForAsset(asset);
    }
}

void CloudSyncUtils::StatusToFlagForAssetsInRecord(const std::vector<Field> &fields, VBucket &record)
{
    for (const Field &field : fields) {
        if (field.type == TYPE_INDEX<Assets> && record[field.colName].index() == TYPE_INDEX<Assets>) {
            StatusToFlagForAssets(std::get<Assets>(record[field.colName]));
        } else if (field.type == TYPE_INDEX<Asset> && record[field.colName].index() == TYPE_INDEX<Asset>) {
            StatusToFlagForAsset(std::get<Asset>(record[field.colName]));
        }
    }
}

bool CloudSyncUtils::IsChangeDataEmpty(const ChangedData &changedData)
{
    return changedData.primaryData[ChangeType::OP_INSERT].empty() ||
           changedData.primaryData[ChangeType::OP_UPDATE].empty() ||
           changedData.primaryData[ChangeType::OP_DELETE].empty();
}

bool CloudSyncUtils::EqualInMsLevel(const Timestamp cmp, const Timestamp beCmp)
{
    return (cmp / CloudDbConstant::TEN_THOUSAND) == (beCmp / CloudDbConstant::TEN_THOUSAND);
}

bool CloudSyncUtils::NeedSaveData(const LogInfo &localLogInfo, const LogInfo &cloudLogInfo)
{
    // If timeStamp, write timestamp, cloudGid are all the same,
    // We thought that the datum is mostly be the same between cloud and local
    // However, there are still slightly possibility that it may be created from different device,
    // So, during the strategy policy [i.e. TagSyncDataStatus], the datum was tagged as UPDATE
    // But we won't notify the datum
    bool isSame = localLogInfo.timestamp == cloudLogInfo.timestamp &&
        EqualInMsLevel(localLogInfo.wTimestamp, cloudLogInfo.wTimestamp) &&
        localLogInfo.cloudGid == cloudLogInfo.cloudGid &&
        localLogInfo.sharingResource == cloudLogInfo.sharingResource &&
        localLogInfo.version == cloudLogInfo.version &&
        (localLogInfo.flag & static_cast<uint64_t>(LogInfoFlag::FLAG_WAIT_COMPENSATED_SYNC)) == 0;
    return !isSame;
}

int CloudSyncUtils::CheckParamValid(const std::vector<DeviceID> &devices, SyncMode mode)
{
    if (devices.size() != 1) {
        LOGE("[CloudSyncer] invalid devices size %zu", devices.size());
        return -E_INVALID_ARGS;
    }
    for (const auto &dev: devices) {
        if (dev.empty() || dev.size() > DBConstant::MAX_DEV_LENGTH) {
            LOGE("[CloudSyncer] invalid device, size %zu", dev.size());
            return -E_INVALID_ARGS;
        }
    }
    if (mode >= SyncMode::SYNC_MODE_PUSH_ONLY && mode < SyncMode::SYNC_MODE_CLOUD_MERGE) {
        LOGE("[CloudSyncer] not support mode %d", static_cast<int>(mode));
        return -E_NOT_SUPPORT;
    }
    if (mode < SyncMode::SYNC_MODE_PUSH_ONLY || mode > SyncMode::SYNC_MODE_CLOUD_FORCE_PULL) {
        LOGE("[CloudSyncer] invalid mode %d", static_cast<int>(mode));
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

LogInfo CloudSyncUtils::GetCloudLogInfo(DistributedDB::VBucket &datum)
{
    LogInfo cloudLogInfo;
    cloudLogInfo.dataKey = 0;
    cloudLogInfo.timestamp = (Timestamp)std::get<int64_t>(datum[CloudDbConstant::MODIFY_FIELD]);
    cloudLogInfo.wTimestamp = (Timestamp)std::get<int64_t>(datum[CloudDbConstant::CREATE_FIELD]);
    cloudLogInfo.flag = (std::get<bool>(datum[CloudDbConstant::DELETE_FIELD])) ? 1u : 0u;
    cloudLogInfo.cloudGid = std::get<std::string>(datum[CloudDbConstant::GID_FIELD]);
    CloudStorageUtils::GetStringFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_DEVICE, datum, cloudLogInfo.device);
    (void)CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::SHARING_RESOURCE_FIELD,
        datum, cloudLogInfo.sharingResource);
    (void)CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::VERSION_FIELD,
        datum, cloudLogInfo.version);
    return cloudLogInfo;
}

int CloudSyncUtils::SaveChangedDataByType(const VBucket &datum, ChangedData &changedData,
    const DataInfoWithLog &localInfo, ChangeType type)
{
    int ret = E_OK;
    std::vector<Type> cloudPkVals;
    if (type == ChangeType::OP_DELETE) {
        ret = CloudSyncUtils::GetCloudPkVals(localInfo.primaryKeys, changedData.field, localInfo.logInfo.dataKey,
            cloudPkVals);
    } else {
        ret = CloudSyncUtils::GetCloudPkVals(datum, changedData.field, localInfo.logInfo.dataKey, cloudPkVals);
    }
    if (ret != E_OK) {
        return ret;
    }
    InsertOrReplaceChangedDataByType(type, cloudPkVals, changedData);
    return E_OK;
}

int CloudSyncUtils::CheckCloudSyncDataValid(const CloudSyncData &uploadData, const std::string &tableName,
    int64_t count)
{
    size_t insRecordLen = uploadData.insData.record.size();
    size_t insExtendLen = uploadData.insData.extend.size();
    size_t updRecordLen = uploadData.updData.record.size();
    size_t updExtendLen = uploadData.updData.extend.size();
    size_t delRecordLen = uploadData.delData.record.size();
    size_t delExtendLen = uploadData.delData.extend.size();

    bool syncDataValid = (uploadData.tableName == tableName) &&
        ((insRecordLen > 0 && insExtendLen > 0 && insRecordLen == insExtendLen) ||
        (updRecordLen > 0 && updExtendLen > 0 && updRecordLen == updExtendLen) ||
        (delRecordLen > 0 && delExtendLen > 0 && delRecordLen == delExtendLen) ||
        (uploadData.lockData.extend.size() > 0));
    if (!syncDataValid) {
        LOGE("[CloudSyncUtils] upload data is empty but upload count is not zero or upload table name"
            " is not the same as table name of sync data.");
        return -E_INTERNAL_ERROR;
    }
    int64_t syncDataCount = static_cast<int64_t>(insRecordLen) + static_cast<int64_t>(updRecordLen) +
        static_cast<int64_t>(delRecordLen);
    if (syncDataCount > count) {
        LOGE("[CloudSyncUtils] Size of a batch of sync data is greater than upload data size. count %d", count);
        return -E_INTERNAL_ERROR;
    }
    return E_OK;
}

void CloudSyncUtils::ClearCloudSyncData(CloudSyncData &uploadData)
{
    std::vector<VBucket>().swap(uploadData.insData.record);
    std::vector<VBucket>().swap(uploadData.insData.extend);
    std::vector<int64_t>().swap(uploadData.insData.rowid);
    std::vector<VBucket>().swap(uploadData.updData.record);
    std::vector<VBucket>().swap(uploadData.updData.extend);
    std::vector<VBucket>().swap(uploadData.delData.record);
    std::vector<VBucket>().swap(uploadData.delData.extend);
}

int CloudSyncUtils::GetWaterMarkAndUpdateTime(std::vector<VBucket> &extend, Timestamp &waterMark)
{
    for (auto &extendData: extend) {
        if (extendData.empty() || extendData.find(CloudDbConstant::MODIFY_FIELD) == extendData.end()) {
            LOGE("[CloudSyncer] VBucket is empty or MODIFY_FIELD doesn't exist.");
            return -E_INTERNAL_ERROR;
        }
        if (TYPE_INDEX<int64_t> != extendData.at(CloudDbConstant::MODIFY_FIELD).index()) {
            LOGE("[CloudSyncer] VBucket's MODIFY_FIELD doesn't fit int64_t.");
            return -E_INTERNAL_ERROR;
        }
        if (extendData.empty() || extendData.find(CloudDbConstant::CREATE_FIELD) == extendData.end()) {
            LOGE("[CloudSyncer] VBucket is empty or CREATE_FIELD doesn't exist.");
            return -E_INTERNAL_ERROR;
        }
        if (TYPE_INDEX<int64_t> != extendData.at(CloudDbConstant::CREATE_FIELD).index()) {
            LOGE("[CloudSyncer] VBucket's CREATE_FIELD doesn't fit int64_t.");
            return -E_INTERNAL_ERROR;
        }
        waterMark = std::max(int64_t(waterMark), std::get<int64_t>(extendData.at(CloudDbConstant::MODIFY_FIELD)));
        int64_t modifyTime =
            std::get<int64_t>(extendData.at(CloudDbConstant::MODIFY_FIELD)) / CloudDbConstant::TEN_THOUSAND;
        int64_t createTime =
            std::get<int64_t>(extendData.at(CloudDbConstant::CREATE_FIELD)) / CloudDbConstant::TEN_THOUSAND;
        extendData.insert_or_assign(CloudDbConstant::MODIFY_FIELD, modifyTime);
        extendData.insert_or_assign(CloudDbConstant::CREATE_FIELD, createTime);
    }
    return E_OK;
}

bool CloudSyncUtils::CheckCloudSyncDataEmpty(const CloudSyncData &uploadData)
{
    return uploadData.insData.extend.empty() && uploadData.insData.record.empty() &&
        uploadData.updData.extend.empty() && uploadData.updData.record.empty() &&
        uploadData.delData.extend.empty() && uploadData.delData.record.empty() &&
        uploadData.lockData.rowid.empty();
}

void CloudSyncUtils::ModifyCloudDataTime(DistributedDB::VBucket &data)
{
    // data already check field modify_field and create_field
    int64_t modifyTime = std::get<int64_t>(data[CloudDbConstant::MODIFY_FIELD]) * CloudDbConstant::TEN_THOUSAND;
    int64_t createTime = std::get<int64_t>(data[CloudDbConstant::CREATE_FIELD]) * CloudDbConstant::TEN_THOUSAND;
    data[CloudDbConstant::MODIFY_FIELD] = modifyTime;
    data[CloudDbConstant::CREATE_FIELD] = createTime;
}

// After doing a batch upload, we need to use CloudSyncData's maximum timestamp to update the water mark;
int CloudSyncUtils::UpdateExtendTime(CloudSyncData &uploadData, const int64_t &count, uint64_t taskId,
    Timestamp &waterMark)
{
    int ret = CloudSyncUtils::CheckCloudSyncDataValid(uploadData, uploadData.tableName, count);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid Sync Data when get local water mark.");
        return ret;
    }
    if (!uploadData.insData.extend.empty()) {
        if (uploadData.insData.record.size() != uploadData.insData.extend.size()) {
            LOGE("[CloudSyncer] Inconsistent size of inserted data.");
            return -E_INTERNAL_ERROR;
        }
        ret = CloudSyncUtils::GetWaterMarkAndUpdateTime(uploadData.insData.extend, waterMark);
        if (ret != E_OK) {
            return ret;
        }
    }

    if (!uploadData.updData.extend.empty()) {
        if (uploadData.updData.record.size() != uploadData.updData.extend.size()) {
            LOGE("[CloudSyncer] Inconsistent size of updated data, %d.", -E_INTERNAL_ERROR);
            return -E_INTERNAL_ERROR;
        }
        ret = CloudSyncUtils::GetWaterMarkAndUpdateTime(uploadData.updData.extend, waterMark);
        if (ret != E_OK) {
            return ret;
        }
    }

    if (!uploadData.delData.extend.empty()) {
        if (uploadData.delData.record.size() != uploadData.delData.extend.size()) {
            LOGE("[CloudSyncer] Inconsistent size of deleted data, %d.", -E_INTERNAL_ERROR);
            return -E_INTERNAL_ERROR;
        }
        ret = CloudSyncUtils::GetWaterMarkAndUpdateTime(uploadData.delData.extend, waterMark);
        if (ret != E_OK) {
            return ret;
        }
    }
    return E_OK;
}

void CloudSyncUtils::UpdateLocalCache(OpType opType, const LogInfo &cloudInfo, const LogInfo &localInfo,
    std::map<std::string, LogInfo> &localLogInfoCache)
{
    LogInfo updateLogInfo;
    std::string hashKey(localInfo.hashKey.begin(), localInfo.hashKey.end());
    bool updateCache = true;
    switch (opType) {
        case OpType::INSERT :
        case OpType::UPDATE :
        case OpType::DELETE: {
            updateLogInfo = cloudInfo;
            updateLogInfo.device = CloudDbConstant::DEFAULT_CLOUD_DEV;
            updateLogInfo.hashKey = localInfo.hashKey;
            if (opType == OpType::DELETE) {
                updateLogInfo.flag |= static_cast<uint64_t>(LogInfoFlag::FLAG_DELETE);
            } else if (opType == OpType::INSERT) {
                updateLogInfo.originDev = CloudDbConstant::DEFAULT_CLOUD_DEV;
            }
            break;
        }
        case OpType::CLEAR_GID:
        case OpType::UPDATE_TIMESTAMP: {
            updateLogInfo = localInfo;
            updateLogInfo.cloudGid.clear();
            updateLogInfo.sharingResource.clear();
            break;
        }
        default:
            updateCache = false;
            break;
    }
    if (updateCache) {
        localLogInfoCache[hashKey] = updateLogInfo;
    }
}

int CloudSyncUtils::SaveChangedData(ICloudSyncer::SyncParam &param, size_t dataIndex,
    const ICloudSyncer::DataInfo &dataInfo, std::vector<std::pair<Key, size_t>> &deletedList)
{
    OpType opType = CalOpType(param, dataIndex);
    Key hashKey = dataInfo.localInfo.logInfo.hashKey;
    if (param.deletePrimaryKeySet.find(hashKey) != param.deletePrimaryKeySet.end()) {
        if (opType == OpType::INSERT) {
            (void)param.dupHashKeySet.insert(hashKey);
            opType = OpType::UPDATE;
            // only composite primary key needs to be processed.
            if (!param.isSinglePrimaryKey) {
                param.withoutRowIdData.updateData.emplace_back(dataIndex,
                    param.changedData.primaryData[ChangeType::OP_UPDATE].size());
            }
        }
    }
    // INSERT: for no primary key or composite primary key situation
    if (!param.isSinglePrimaryKey && opType == OpType::INSERT) {
        param.withoutRowIdData.insertData.push_back(dataIndex);
        return E_OK;
    }
    switch (opType) {
        // INSERT: only for single primary key situation
        case OpType::INSERT:
            param.info.downLoadInfo.insertCount++;
            return CloudSyncUtils::SaveChangedDataByType(
                param.downloadData.data[dataIndex], param.changedData, dataInfo.localInfo, ChangeType::OP_INSERT);
        case OpType::UPDATE:
            param.info.downLoadInfo.updateCount++;
            if (CloudSyncUtils::NeedSaveData(dataInfo.localInfo.logInfo, dataInfo.cloudLogInfo)) {
                return CloudSyncUtils::SaveChangedDataByType(param.downloadData.data[dataIndex], param.changedData,
                    dataInfo.localInfo, ChangeType::OP_UPDATE);
            }
            return E_OK;
        case OpType::DELETE:
            param.info.downLoadInfo.deleteCount++;
            return CloudSyncUtils::SaveChangedDataByType(param.downloadData.data[dataIndex], param.changedData,
                dataInfo.localInfo, ChangeType::OP_DELETE);
        default:
            return E_OK;
    }
}

void CloudSyncUtils::ClearWithoutData(ICloudSyncer::SyncParam &param)
{
    param.withoutRowIdData.insertData.clear();
    param.withoutRowIdData.updateData.clear();
    param.withoutRowIdData.assetInsertData.clear();
}

bool CloudSyncUtils::IsSkipAssetsMissingRecord(const std::vector<VBucket> &extend)
{
    if (extend.empty()) {
        return false;
    }
    for (size_t i = 0; i < extend.size(); ++i) {
        if (DBCommon::IsRecordError(extend[i])) { // Kepp std::string type error
            return false;
        }
        if (DBCommon::IsIntTypeRecordError(extend[i]) && !DBCommon::IsRecordAssetsMissing(extend[i])) {
            return false;
        }
    }
    return true;
}

int CloudSyncUtils::FillAssetIdToAssets(CloudSyncBatch &data, int errorCode, const CloudWaterType &type)
{
    if (data.extend.size() != data.assets.size()) {
        LOGE("[CloudSyncUtils] size not match, extend:%zu assets:%zu.", data.extend.size(), data.assets.size());
        return -E_CLOUD_ERROR;
    }
    int errCode = E_OK;
    for (size_t i = 0; i < data.assets.size(); i++) {
        if (data.assets[i].empty() || DBCommon::IsRecordIgnored(data.extend[i]) ||
            (errorCode != E_OK &&
            (DBCommon::IsRecordError(data.extend[i]) || DBCommon::IsRecordAssetsMissing(data.extend[i]))) ||
            DBCommon::IsNeedCompensatedForUpload(data.extend[i], type)) {
            continue;
        }
        for (auto it = data.assets[i].begin(); it != data.assets[i].end();) {
            auto &[col, value] = *it;
            if (!CheckIfContainsInsertAssets(value)) {
                ++it;
                continue;
            }
            auto extendIt = data.extend[i].find(col);
            if (extendIt == data.extend[i].end()) {
                LOGI("[CloudSyncUtils] Asset field name can not find in extend. key is:%s.", col.c_str());
                it = data.assets[i].erase(it);
                continue;
            }
            if (extendIt->second.index() != value.index()) {
                LOGE("[CloudSyncUtils] Asset field type not same. extend:%zu, data:%zu",
                    extendIt->second.index(), value.index());
                errCode = -E_CLOUD_ERROR;
                ++it;
                continue;
            }
            int ret = FillAssetIdToAssetData(extendIt->second, value);
            if (ret != E_OK) {
                LOGE("[CloudSyncUtils] fail to fill assetId, %d.", ret);
                errCode = -E_CLOUD_ERROR;
            }
            ++it;
        }
    }
    return errCode;
}

int CloudSyncUtils::FillAssetIdToAssetData(const Type &extend, Type &assetData)
{
    if (extend.index() == TYPE_INDEX<Asset>) {
        if (std::get<Asset>(assetData).name != std::get<Asset>(extend).name) {
            LOGE("[CloudSyncUtils][FillAssetIdToAssetData] Asset name can not find in extend.");
            return -E_CLOUD_ERROR;
        }
        if (std::get<Asset>(extend).assetId.empty()) {
            LOGE("[CloudSyncUtils][FillAssetIdToAssetData] Asset id is empty.");
            return -E_CLOUD_ERROR;
        }
        std::get<Asset>(assetData).assetId = std::get<Asset>(extend).assetId;
    }
    if (extend.index() == TYPE_INDEX<Assets>) {
        FillAssetIdToAssetsData(std::get<Assets>(extend), std::get<Assets>(assetData));
    }
    return E_OK;
}

void CloudSyncUtils::FillAssetIdToAssetsData(const Assets &extend, Assets &assets)
{
    for (auto it = assets.begin(); it != assets.end();) {
        auto &asset = *it;
        if (asset.flag != static_cast<uint32_t>(AssetOpType::INSERT)) {
            ++it;
            continue;
        }
        auto extendAssets = extend;
        bool isAssetExisted = false;
        for (const auto &extendAsset : extendAssets) {
            if (asset.name == extendAsset.name && !extendAsset.assetId.empty()) {
                asset.assetId = extendAsset.assetId;
                isAssetExisted = true;
                break;
            }
        }
        if (!isAssetExisted) {
            LOGI("Unable to sync local asset, skip fill assetId.");
            it = assets.erase(it);
        } else {
            ++it;
        }
    }
}

bool CloudSyncUtils::CheckIfContainsInsertAssets(const Type &assetData)
{
    if (assetData.index() == TYPE_INDEX<Asset>) {
        if (std::get<Asset>(assetData).flag != static_cast<uint32_t>(AssetOpType::INSERT)) {
            return false;
        }
    } else if (assetData.index() == TYPE_INDEX<Assets>) {
        bool hasInsertAsset = false;
        for (const auto &asset : std::get<Assets>(assetData)) {
            if (asset.flag == static_cast<uint32_t>(AssetOpType::INSERT)) {
                hasInsertAsset = true;
                break;
            }
        }
        if (!hasInsertAsset) {
            return false;
        }
    }
    return true;
}

void CloudSyncUtils::UpdateAssetsFlag(CloudSyncData &uploadData)
{
    AssetOperationUtils::UpdateAssetsFlag(uploadData.insData.record, uploadData.insData.assets);
    AssetOperationUtils::UpdateAssetsFlag(uploadData.updData.record, uploadData.updData.assets);
    AssetOperationUtils::UpdateAssetsFlag(uploadData.delData.record, uploadData.delData.assets);
}

void CloudSyncUtils::InsertOrReplaceChangedDataByType(ChangeType type, std::vector<Type> &pkVal,
    ChangedData &changedData)
{
    // erase old changedData if exist
    for (auto &changePkValList : changedData.primaryData) {
        changePkValList.erase(std::remove_if(changePkValList.begin(), changePkValList.end(),
            [&pkVal](const std::vector<Type> &existPkVal) {
            return existPkVal == pkVal;
            }), changePkValList.end());
    }
    // insert new changeData
    changedData.primaryData[type].emplace_back(std::move(pkVal));
}

OpType CloudSyncUtils::CalOpType(ICloudSyncer::SyncParam &param, size_t dataIndex)
{
    OpType opType = param.downloadData.opType[dataIndex];
    if (opType != OpType::INSERT && opType != OpType::UPDATE) {
        return opType;
    }

    std::vector<Type> cloudPkVal;
    // use dataIndex as dataKey avoid get same pk with no pk schema
    int errCode = CloudSyncUtils::GetCloudPkVals(param.downloadData.data[dataIndex], param.changedData.field, dataIndex,
        cloudPkVal);
    if (errCode != E_OK) {
        LOGW("[CloudSyncUtils] Get pk from download data failed %d", errCode);
        // use origin opType
        return opType;
    }
    auto iter = std::find_if(param.insertPk.begin(), param.insertPk.end(), [&cloudPkVal](const auto &item) {
        return item == cloudPkVal;
    });
    if (opType == OpType::INSERT) {
        // record all insert pk in one batch
        if (iter == param.insertPk.end()) {
            param.insertPk.push_back(cloudPkVal);
        }
        return OpType::INSERT;
    }
    // notify with insert because this data not exist in local before query
    return (iter == param.insertPk.end()) ? OpType::UPDATE : OpType::INSERT;
}

CloudSyncer::CloudTaskInfo CloudSyncUtils::InitCompensatedSyncTaskInfo()
{
    CloudSyncer::CloudTaskInfo taskInfo;
    taskInfo.priorityTask = true;
    taskInfo.timeout = CloudDbConstant::CLOUD_DEFAULT_TIMEOUT;
    taskInfo.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    taskInfo.callback = nullptr;
    taskInfo.compensatedTask = true;
    return taskInfo;
}

CloudSyncer::CloudTaskInfo CloudSyncUtils::InitCompensatedSyncTaskInfo(const CloudSyncOption &option,
    const SyncProcessCallback &onProcess)
{
    CloudSyncer::CloudTaskInfo taskInfo = InitCompensatedSyncTaskInfo();
    taskInfo.callback = onProcess;
    taskInfo.devices = option.devices;
    if (option.users.empty()) {
        taskInfo.users.push_back("");
    } else {
        taskInfo.users = option.users;
    }
    return taskInfo;
}

CloudSyncer::CloudTaskInfo CloudSyncUtils::InitCompensatedSyncTaskInfo(const CloudSyncer::CloudTaskInfo &oriTaskInfo)
{
    CloudSyncer::CloudTaskInfo taskInfo = InitCompensatedSyncTaskInfo();
    taskInfo.lockAction = oriTaskInfo.lockAction;
    taskInfo.users = oriTaskInfo.users;
    taskInfo.devices = oriTaskInfo.devices;
    taskInfo.storeId = oriTaskInfo.storeId;
    return taskInfo;
}
}
