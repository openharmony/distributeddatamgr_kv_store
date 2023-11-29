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
#include "cloud_syncer.h"

#include <cstdint>
#include <utility>
#include <unordered_map>

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "cloud/icloud_db.h"
#include "cloud_sync_tag_assets.h"
#include "cloud_sync_utils.h"
#include "db_errno.h"
#include "log_print.h"
#include "runtime_context.h"
#include "storage_proxy.h"
#include "store_types.h"
#include "strategy_factory.h"
#include "version.h"

namespace DistributedDB {
void CloudSyncer::ReloadWaterMarkIfNeed(TaskId taskId, WaterMark &waterMark)
{
    Timestamp cacheWaterMark = GetResumeWaterMark(taskId);
    waterMark = cacheWaterMark == 0u ? waterMark : cacheWaterMark;
    RecordWaterMark(taskId, 0u);
}

void CloudSyncer::ReloadUploadInfoIfNeed(TaskId taskId, const UploadParam &param, InnerProcessInfo &info)
{
    info.upLoadInfo.total = static_cast<uint32_t>(param.count);
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (!cloudTaskInfos_[taskId].resume) {
            return;
        }
    }
    uint32_t lastSuccessCount = GetLastUploadSuccessCount(info.tableName);
    if (lastSuccessCount == 0) {
        return;
    }
    info.upLoadInfo.total += lastSuccessCount;
    info.upLoadInfo.successCount += lastSuccessCount;
    LOGD("[CloudSyncer] resume upload, last success count %" PRIu32, lastSuccessCount);
}

uint32_t CloudSyncer::GetLastUploadSuccessCount(const std::string &tableName)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return currentContext_.notifier->GetLastUploadSuccessCount(tableName);
}

int CloudSyncer::FillDownloadExtend(TaskId taskId, const std::string &tableName, const std::string &cloudWaterMark,
    VBucket &extend)
{
    extend = {
        {CloudDbConstant::CURSOR_FIELD, cloudWaterMark}
    };

    QuerySyncObject obj = GetQuerySyncObject(tableName);
    if (obj.IsContainQueryNodes()) {
        int errCode = GetCloudGid(taskId, tableName, obj);
        if (errCode != E_OK) {
            LOGE("[CloudSyncer] Failed to get cloud gid when fill extend, %d.", errCode);
            return errCode;
        }
        Bytes bytes;
        bytes.resize(obj.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
        Parcel parcel(bytes.data(), bytes.size());
        errCode = obj.SerializeData(parcel, SOFTWARE_VERSION_CURRENT);
        if (errCode != E_OK) {
            LOGE("[CloudSyncer] Query serialize failed %d", errCode);
            return errCode;
        }
        extend[CloudDbConstant::TYPE_FIELD] = static_cast<int64_t>(CloudQueryType::QUERY_FIELD);
        extend[CloudDbConstant::QUERY_FIELD] = bytes;
    } else {
        extend[CloudDbConstant::TYPE_FIELD] = static_cast<int64_t>(CloudQueryType::FULL_TABLE);
    }
    return E_OK;
}

int CloudSyncer::GetCloudGid(TaskId taskId, const std::string &tableName, QuerySyncObject &obj)
{
    std::vector<std::string> cloudGid;
    bool isCloudForcePush = cloudTaskInfos_[taskId].mode == SYNC_MODE_CLOUD_FORCE_PUSH;
    int errCode = storageProxy_->GetCloudGid(obj, isCloudForcePush, cloudGid);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Failed to get cloud gid, %d.", errCode);
    } else if (!cloudGid.empty()) {
        obj.SetCloudGid(cloudGid);
    }
    return errCode;
}

QuerySyncObject CloudSyncer::GetQuerySyncObject(const std::string &tableName)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    for (const auto &item : cloudTaskInfos_[currentContext_.currentTaskId].queryList) {
        if (item.GetTableName() == tableName) {
            return item;
        }
    }
    LOGW("[CloudSyncer] not found query in cache");
    QuerySyncObject querySyncObject;
    querySyncObject.SetTableName(tableName);
    return querySyncObject;
}

void CloudSyncer::NotifyUploadFailed(int errCode, InnerProcessInfo &info)
{
    LOGE("[CloudSyncer] Failed to do upload, %d", errCode);
    info.upLoadInfo.failCount = info.upLoadInfo.total - info.upLoadInfo.successCount;
    info.tableStatus = ProcessStatus::FINISHED;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentContext_.notifier->UpdateProcess(info);
    }
}

int CloudSyncer::BatchInsert(Info &insertInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo)
{
    int errCode = cloudDB_.BatchInsert(uploadData.tableName, uploadData.insData.record,
        uploadData.insData.extend, insertInfo);
    bool isSharedTable = false;
    int ret = storageProxy_->IsSharedTable(uploadData.tableName, isSharedTable);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] DoBatchUpload cannot judge the table is shared table. %d", ret);
        return ret;
    }
    if (!isSharedTable) {
        ret = CloudSyncUtils::FillAssetIdToAssets(uploadData.insData);
    }
    if (errCode != E_OK) {
        storageProxy_->FillCloudGidIfSuccess(OpType::INSERT, uploadData);
        return errCode;
    }
    // we need to fill back gid after insert data to cloud.
    int errorCode = storageProxy_->FillCloudLogAndAsset(OpType::INSERT, uploadData);
    if ((errorCode != E_OK) || (ret != E_OK)) {
        LOGE("[CloudSyncer] Failed to fill back when doing upload insData, %d.", errorCode);
        return ret == E_OK ? errorCode : ret;
    }
    if (isSharedTable) {
        ret = storageProxy_->FillCloudLogAndAsset(OpType::UPDATE_VERSION, uploadData);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Failed to fill back version when doing upload insData, %d.", ret);
            return ret;
        }
    }
    innerProcessInfo.upLoadInfo.successCount += insertInfo.successCount;
    return E_OK;
}

int CloudSyncer::BatchUpdate(Info &updateInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo)
{
    int errCode = cloudDB_.BatchUpdate(uploadData.tableName, uploadData.updData.record,
        uploadData.updData.extend, updateInfo);
    bool isSharedTable = false;
    int ret = storageProxy_->IsSharedTable(uploadData.tableName, isSharedTable);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] DoBatchUpload cannot judge the table is shared table. %d", ret);
        return ret;
    }
    if (!isSharedTable) {
        ret = CloudSyncUtils::FillAssetIdToAssets(uploadData.updData);
    }
    if (errCode != E_OK) {
        storageProxy_->FillCloudGidIfSuccess(OpType::UPDATE, uploadData);
        return errCode;
    }
    int errorCode = storageProxy_->FillCloudLogAndAsset(OpType::UPDATE, uploadData);
    if ((errorCode != E_OK) || (ret != E_OK)) {
        LOGE("[CloudSyncer] Failed to fill back when doing upload updData, %d.", errorCode);
        return ret == E_OK ? errorCode : ret;
    }
    if (isSharedTable) {
        ret = storageProxy_->FillCloudLogAndAsset(OpType::UPDATE_VERSION, uploadData);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Failed to fill back version when doing upload insData, %d.", ret);
            return ret;
        }
    }
    innerProcessInfo.upLoadInfo.successCount += updateInfo.successCount;
    return E_OK;
}

int CloudSyncer::DownloadAssetsOneByOne(const InnerProcessInfo &info, const DownloadItem &downloadItem,
    std::map<std::string, Assets> &downloadAssets)
{
    bool isSharedTable = false;
    int errCode = storageProxy_->IsSharedTable(info.tableName, isSharedTable);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] DownloadOneAssetRecord cannot judge the table is a shared table. %d", errCode);
        return errCode;
    }
    int transactionCode = E_OK;
    // shared table don't download, so just begin transaction once
    if (isSharedTable) {
        transactionCode = storageProxy_->StartTransaction(TransactType::IMMEDIATE);
    }
    if (transactionCode != E_OK) {
        LOGE("[CloudSyncer] begin transaction before download failed %d", transactionCode);
        return transactionCode;
    }
    errCode = DownloadAssetsOneByOneInner(isSharedTable, info, downloadItem, downloadAssets);
    if (isSharedTable) {
        transactionCode = storageProxy_->Commit();
        if (transactionCode != E_OK) {
            LOGW("[CloudSyncer] commit transaction after download failed %d", transactionCode);
        }
    }
    return (errCode == E_OK) ? transactionCode : errCode;
}

int CloudSyncer::GetDBAssets(bool isSharedTable, const InnerProcessInfo &info, const DownloadItem &downloadItem,
    VBucket &dbAssets)
{
    int transactionCode = E_OK;
    if (!isSharedTable) {
        transactionCode = storageProxy_->StartTransaction(TransactType::IMMEDIATE);
    }
    if (transactionCode != E_OK) {
        LOGE("[CloudSyncer] begin transaction before download failed %d", transactionCode);
        return transactionCode;
    }
    int errCode = storageProxy_->GetAssetsByGidOrHashKey(info.tableName, downloadItem.gid,
        downloadItem.hashKey, dbAssets);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        LOGE("[CloudSyncer] get assets from db failed %d", errCode);
        return errCode;
    }
    if (!isSharedTable) {
        transactionCode = storageProxy_->Commit();
    }
    if (transactionCode != E_OK) {
        LOGE("[CloudSyncer] commit transaction before download failed %d", transactionCode);
    }
    return transactionCode;
}

int CloudSyncer::DownloadAssetsOneByOneInner(bool isSharedTable, const InnerProcessInfo &info,
    const DownloadItem &downloadItem, std::map<std::string, Assets> &downloadAssets)
{
    int errCode = E_OK;
    for (auto &[col, assets] : downloadAssets) {
        Assets callDownloadAssets;
        for (auto &asset : assets) {
            std::map<std::string, Assets> tmpAssets;
            tmpAssets[col] = { asset };
            uint32_t tmpFlag = asset.flag;
            VBucket dbAssets;
            int tmpCode = GetDBAssets(isSharedTable, info, downloadItem, dbAssets);
            if (tmpCode != E_OK) {
                errCode = (errCode != E_OK) ? errCode : tmpCode;
                break;
            }
            if (!isSharedTable && AssetOperationUtils::CalAssetOperation(col, asset, dbAssets,
                AssetOperationUtils::CloudSyncAction::START_DOWNLOAD) == AssetOperationUtils::AssetOpType::HANDLE) {
                tmpCode = cloudDB_.Download(info.tableName, downloadItem.gid, downloadItem.prefix, tmpAssets);
            } else {
                LOGD("[CloudSyncer] skip download asset...");
                continue;
            }
            errCode = (errCode != E_OK) ? errCode : tmpCode;
            if (tmpCode == -E_NOT_SET) {
                break;
            }
            asset = tmpAssets[col][0]; // copy asset back
            asset.flag = tmpFlag;
            if (asset.flag != static_cast<uint32_t>(AssetOpType::NO_CHANGE)) {
                asset.status = (tmpCode == E_OK) ? NORMAL : ABNORMAL;
            }
            callDownloadAssets.push_back(asset);
        }
        assets = callDownloadAssets;
    }
    return errCode;
}
}