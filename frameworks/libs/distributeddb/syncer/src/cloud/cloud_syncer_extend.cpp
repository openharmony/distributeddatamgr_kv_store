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
#include "kv_store_errno.h"
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

void CloudSyncer::ReloadCloudWaterMarkIfNeed(const std::string &tableName, std::string &cloudWaterMark)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    std::string cacheCloudWaterMark = currentContext_.cloudWaterMarks[currentContext_.currentUserIndex][tableName];
    cloudWaterMark = cacheCloudWaterMark.empty() ? cloudWaterMark : cacheCloudWaterMark;
}

void CloudSyncer::ReloadUploadInfoIfNeed(const UploadParam &param, InnerProcessInfo &info)
{
    info.upLoadInfo.total = static_cast<uint32_t>(param.count);
    Info lastUploadInfo;
    UploadRetryInfo retryInfo;
    GetLastUploadInfo(info.tableName, lastUploadInfo, retryInfo);
    info.upLoadInfo.total += lastUploadInfo.successCount;
    info.upLoadInfo.successCount += lastUploadInfo.successCount;
    info.upLoadInfo.failCount += lastUploadInfo.failCount;
    info.upLoadInfo.insertCount += lastUploadInfo.insertCount;
    info.upLoadInfo.updateCount += lastUploadInfo.updateCount;
    info.upLoadInfo.deleteCount += lastUploadInfo.deleteCount;
    info.retryInfo.uploadBatchRetryCount = retryInfo.uploadBatchRetryCount;
    LOGD("[CloudSyncer] resume upload, last success count %" PRIu32 ", last fail count %" PRIu32,
        lastUploadInfo.successCount, lastUploadInfo.failCount);
}

void CloudSyncer::GetLastUploadInfo(const std::string &tableName, Info &lastUploadInfo, UploadRetryInfo &retryInfo)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return currentContext_.notifier->GetLastUploadInfo(tableName, lastUploadInfo, retryInfo);
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
    int errCode = storageProxy_->GetCloudGid(obj, isCloudForcePush, IsCompensatedTask(taskId), cloudGid);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Failed to get cloud gid, %d.", errCode);
    } else if (!cloudGid.empty()) {
        obj.SetCloudGid(cloudGid);
    }
    LOGI("[CloudSyncer] get cloud gid size:%zu", cloudGid.size());
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

void CloudSyncer::UpdateProcessWhenUploadFailed(InnerProcessInfo &info)
{
    info.tableStatus = ProcessStatus::FINISHED;
    std::lock_guard<std::mutex> autoLock(dataLock_);
    currentContext_.notifier->UpdateProcess(info);
    currentContext_.notifier->UpdateUploadRetryInfo(info);
}

void CloudSyncer::NotifyUploadFailed(int errCode, InnerProcessInfo &info)
{
    if (errCode == -E_CLOUD_VERSION_CONFLICT) {
        LOGI("[CloudSyncer] Stop upload due to version conflict, %d", errCode);
    } else {
        LOGE("[CloudSyncer] Failed to do upload, %d", errCode);
        info.upLoadInfo.failCount = info.upLoadInfo.total - info.upLoadInfo.successCount;
    }
    UpdateProcessWhenUploadFailed(info);
}

int CloudSyncer::BatchInsert(Info &insertInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo)
{
    uint32_t retryCount = 0;
    int errCode = cloudDB_.BatchInsert(uploadData.tableName, uploadData.insData.record,
        uploadData.insData.extend, insertInfo, retryCount);
    innerProcessInfo.upLoadInfo.successCount += insertInfo.successCount;
    innerProcessInfo.upLoadInfo.failCount += insertInfo.failCount;
    innerProcessInfo.upLoadInfo.insertCount += insertInfo.successCount;
    if (errCode == -E_CLOUD_VERSION_CONFLICT) {
        ProcessVersionConflictInfo(innerProcessInfo, retryCount);
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer][BatchInsert] BatchInsert with error, ret is %d.", errCode);
    }
    if (uploadData.isCloudVersionRecord) {
        return errCode;
    }
    bool isSharedTable = false;
    int ret = storageProxy_->IsSharedTable(uploadData.tableName, isSharedTable);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] DoBatchUpload cannot judge the table is shared table. %d", ret);
        return ret;
    }
    if (!isSharedTable) {
        ret = CloudSyncUtils::FillAssetIdToAssets(uploadData.insData, errCode, CloudWaterType::INSERT);
        if (ret != E_OK) {
            LOGW("[CloudSyncer][BatchInsert] FillAssetIdToAssets with error, ret is %d.", ret);
        }
    }
    if (errCode != E_OK) {
        storageProxy_->FillCloudGidIfSuccess(OpType::INSERT, uploadData);
        bool isSkip = CloudSyncUtils::IsSkipAssetsMissingRecord(uploadData.insData.extend);
        if (isSkip) {
            LOGI("[CloudSyncer][BatchInsert] Try to FillCloudLogAndAsset when assets missing. errCode: %d", errCode);
            return E_OK;
        } else {
            LOGE("[CloudSyncer][BatchInsert] errCode: %d, can not skip assets missing record.", errCode);
            return errCode;
        }
    }
    // we need to fill back gid after insert data to cloud.
    int errorCode = storageProxy_->FillCloudLogAndAsset(OpType::INSERT, uploadData);
    if ((errorCode != E_OK) || (ret != E_OK)) {
        LOGE("[CloudSyncer] Failed to fill back when doing upload insData, %d.", errorCode);
        return ret == E_OK ? errorCode : ret;
    }
    return E_OK;
}

int CloudSyncer::BatchUpdate(Info &updateInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo)
{
    uint32_t retryCount = 0;
    int errCode = cloudDB_.BatchUpdate(uploadData.tableName, uploadData.updData.record,
        uploadData.updData.extend, updateInfo, retryCount);
    innerProcessInfo.upLoadInfo.successCount += updateInfo.successCount;
    innerProcessInfo.upLoadInfo.failCount += updateInfo.failCount;
    innerProcessInfo.upLoadInfo.updateCount += updateInfo.successCount;
    if (errCode == -E_CLOUD_VERSION_CONFLICT) {
        ProcessVersionConflictInfo(innerProcessInfo, retryCount);
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer][BatchUpdate] BatchUpdate with error, ret is %d.", errCode);
    }
    if (uploadData.isCloudVersionRecord) {
        return errCode;
    }
    bool isSharedTable = false;
    int ret = storageProxy_->IsSharedTable(uploadData.tableName, isSharedTable);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] DoBatchUpload cannot judge the table is shared table. %d", ret);
        return ret;
    }
    if (!isSharedTable) {
        ret = CloudSyncUtils::FillAssetIdToAssets(uploadData.updData, errCode, CloudWaterType::UPDATE);
        if (ret != E_OK) {
            LOGW("[CloudSyncer][BatchUpdate] FillAssetIdToAssets with error, ret is %d.", ret);
        }
    }
    if (errCode != E_OK) {
        storageProxy_->FillCloudGidIfSuccess(OpType::UPDATE, uploadData);
        bool isSkip = CloudSyncUtils::IsSkipAssetsMissingRecord(uploadData.updData.extend);
        if (isSkip) {
            LOGI("[CloudSyncer][BatchUpdate] Try to FillCloudLogAndAsset when assets missing. errCode: %d", errCode);
            return E_OK;
        } else {
            LOGE("[CloudSyncer][BatchUpdate] errCode: %d, can not skip assets missing record.", errCode);
            return errCode;
        }
    }
    int errorCode = storageProxy_->FillCloudLogAndAsset(OpType::UPDATE, uploadData);
    if ((errorCode != E_OK) || (ret != E_OK)) {
        LOGE("[CloudSyncer] Failed to fill back when doing upload updData, %d.", errorCode);
        return ret == E_OK ? errorCode : ret;
    }
    return E_OK;
}

int CloudSyncer::DownloadAssetsOneByOne(const InnerProcessInfo &info, DownloadItem &downloadItem,
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

std::pair<int, uint32_t> CloudSyncer::GetDBAssets(bool isSharedTable, const InnerProcessInfo &info,
    const DownloadItem &downloadItem, VBucket &dbAssets)
{
    std::pair<int, uint32_t> res = { E_OK, static_cast<uint32_t>(LockStatus::UNLOCK) };
    auto &errCode = res.first;
    if (!isSharedTable) {
        errCode = storageProxy_->StartTransaction(TransactType::IMMEDIATE);
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] begin transaction before download failed %d", errCode);
        return res;
    }
    res = storageProxy_->GetAssetsByGidOrHashKey(info.tableName, downloadItem.gid,
        downloadItem.hashKey, dbAssets);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        if (errCode != -E_CLOUD_GID_MISMATCH) {
            LOGE("[CloudSyncer] get assets from db failed %d", errCode);
        }
        if (!isSharedTable) {
            (void)storageProxy_->Rollback();
        }
        return res;
    }
    if (!isSharedTable) {
        errCode = storageProxy_->Commit();
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] commit transaction before download failed %d", errCode);
    }
    return res;
}

std::map<std::string, Assets>& CloudSyncer::BackFillAssetsAfterDownload(int downloadCode, int deleteCode,
    std::map<std::string, std::vector<uint32_t>> &tmpFlags, std::map<std::string, Assets> &tmpAssetsToDownload,
    std::map<std::string, Assets> &tmpAssetsToDelete)
{
    std::map<std::string, Assets> &downloadAssets = tmpAssetsToDownload;
    for (auto &[col, assets] : tmpAssetsToDownload) {
        int i = 0;
        for (auto &asset : assets) {
            asset.flag = tmpFlags[col][i++];
            if (asset.flag == static_cast<uint32_t>(AssetOpType::NO_CHANGE)) {
                continue;
            }
            if (downloadCode == E_OK) {
                asset.status = NORMAL;
            } else {
                asset.status = (asset.status == NORMAL) ? NORMAL : ABNORMAL;
            }
        }
    }
    for (auto &[col, assets] : tmpAssetsToDelete) {
        for (auto &asset : assets) {
            asset.flag = static_cast<uint32_t>(AssetOpType::DELETE);
            if (deleteCode == E_OK) {
                asset.status = NORMAL;
            } else {
                asset.status = ABNORMAL;
            }
            downloadAssets[col].push_back(asset);
        }
    }
    return downloadAssets;
}

int CloudSyncer::IsNeedSkipDownload(bool isSharedTable, int &errCode, const InnerProcessInfo &info,
    const DownloadItem &downloadItem, VBucket &dbAssets)
{
    auto [tmpCode, status] = GetDBAssets(isSharedTable, info, downloadItem, dbAssets);
    if (tmpCode == -E_CLOUD_GID_MISMATCH) {
        LOGW("[CloudSyncer] skip download asset because gid mismatch");
        errCode = E_OK;
        return true;
    }
    if (CloudStorageUtils::IsDataLocked(status)) {
        LOGI("[CloudSyncer] skip download asset because data lock:%u", status);
        errCode = E_OK;
        return true;
    }
    if (tmpCode != E_OK) {
        LOGE("[CloudSyncer] Get assets from DB failed: %d, return errCode: %d", tmpCode, errCode);
        errCode = (errCode != E_OK) ? errCode : tmpCode;
        return true;
    }
    return false;
}

bool CloudSyncer::CheckDownloadOrDeleteCode(int &errCode, int downloadCode, int deleteCode, DownloadItem &downloadItem)
{
    if (downloadCode == -E_CLOUD_RECORD_EXIST_CONFLICT || deleteCode == -E_CLOUD_RECORD_EXIST_CONFLICT) {
        downloadItem.recordConflict = true;
        errCode = E_OK;
        return false;
    }
    errCode = (errCode != E_OK) ? errCode : deleteCode;
    errCode = (errCode != E_OK) ? errCode : downloadCode;
    if (downloadCode == -E_NOT_SET || deleteCode == -E_NOT_SET) {
        return false;
    }
    return true;
}

void GetAssetsToDownload(std::map<std::string, Assets> &downloadAssets, VBucket &dbAssets, bool isSharedTable,
    std::map<std::string, Assets> &assetsToDownload, std::map<std::string, std::vector<uint32_t>> &tmpFlags)
{
    if (isSharedTable) {
        LOGD("[CloudSyncer] skip download for shared table");
        return;
    }
    for (auto &[col, assets] : downloadAssets) {
        for (auto &asset : assets) {
            if (asset.flag != static_cast<uint32_t>(AssetOpType::DELETE) &&
                AssetOperationUtils::CalAssetOperation(col, asset, dbAssets,
                AssetOperationUtils::CloudSyncAction::START_DOWNLOAD) == AssetOperationUtils::AssetOpType::HANDLE) {
                asset.status = asset.flag == static_cast<uint32_t>(AssetOpType::INSERT) ?
                    static_cast<uint32_t>(AssetStatus::INSERT) : static_cast<uint32_t>(AssetStatus::UPDATE);
                assetsToDownload[col].push_back(asset);
                tmpFlags[col].push_back(asset.flag);
            } else {
                LOGD("[CloudSyncer] skip download asset...");
            }
        }
    }
}

void GetAssetsToRemove(std::map<std::string, Assets> &downloadAssets, VBucket &dbAssets, bool isSharedTable,
    std::map<std::string, Assets> &assetsToRemove)
{
    if (isSharedTable) {
        LOGD("[CloudSyncer] skip remove for shared table");
        return;
    }
    for (auto &[col, assets] : downloadAssets) {
        for (auto &asset : assets) {
            if (asset.flag == static_cast<uint32_t>(AssetOpType::DELETE) &&
                AssetOperationUtils::CalAssetRemoveOperation(col, asset, dbAssets) ==
                AssetOperationUtils::AssetOpType::HANDLE) {
                asset.status = static_cast<uint32_t>(AssetStatus::DELETE);
                assetsToRemove[col].push_back(asset);
            }
        }
    }
}

int CloudSyncer::DownloadAssetsOneByOneInner(bool isSharedTable, const InnerProcessInfo &info,
    DownloadItem &downloadItem, std::map<std::string, Assets> &downloadAssets)
{
    int errCode = E_OK;
    VBucket dbAssets;
    std::map<std::string, Assets> tmpAssetsToRemove;
    if (!IsNeedSkipDownload(isSharedTable, errCode, info, downloadItem, dbAssets)) {
        GetAssetsToRemove(downloadAssets, dbAssets, isSharedTable, tmpAssetsToRemove);
    }
    auto deleteCode = cloudDB_.RemoveLocalAssets(info.tableName, downloadItem.gid, downloadItem.prefix,
        tmpAssetsToRemove);

    std::map<std::string, Assets> tmpAssetsToDownload;
    std::map<std::string, std::vector<uint32_t>> tmpFlags;
    if (!IsNeedSkipDownload(isSharedTable, errCode, info, downloadItem, dbAssets)) {
        GetAssetsToDownload(downloadAssets, dbAssets, isSharedTable, tmpAssetsToDownload, tmpFlags);
    }
    auto downloadCode = cloudDB_.Download(info.tableName, downloadItem.gid, downloadItem.prefix, tmpAssetsToDownload);
    if (!CheckDownloadOrDeleteCode(errCode, downloadCode, deleteCode, downloadItem)) {
        return errCode;
    }

    // copy asset back
    downloadAssets = BackFillAssetsAfterDownload(downloadCode, deleteCode, tmpFlags, tmpAssetsToDownload,
        tmpAssetsToRemove);
    return errCode;
}

void CloudSyncer::SeparateNormalAndFailAssets(const std::map<std::string, Assets> &assetsMap, VBucket &normalAssets,
    VBucket &failedAssets)
{
    for (auto &[key, assets] : assetsMap) {
        Assets tempFailedAssets;
        Assets tempNormalAssets;
        int32_t otherStatusCnt = 0;
        for (auto &asset : assets) {
            if (asset.status == AssetStatus::NORMAL) {
                tempNormalAssets.push_back(asset);
            } else if (asset.status == AssetStatus::ABNORMAL) {
                tempFailedAssets.push_back(asset);
            } else {
                otherStatusCnt++;
                LOGW("[CloudSyncer] download err, statue %d count %d", asset.status, otherStatusCnt);
            }
        }
        LOGI("[CloudSyncer] download asset times, normalCount %d, abnormalCount %d, otherStatusCnt %d",
            tempNormalAssets.size(), tempFailedAssets.size(), otherStatusCnt);
        if (tempFailedAssets.size() > 0) {
            failedAssets[key] = std::move(tempFailedAssets);
        }
        if (tempNormalAssets.size() > 0) {
            normalAssets[key] = std::move(tempNormalAssets);
        }
    }
}

int CloudSyncer::CommitDownloadAssets(const DownloadItem &downloadItem, const std::string &tableName,
    DownloadCommitList &commitList, uint32_t &successCount)
{
    int errCode = storageProxy_->SetLogTriggerStatus(false);
    if (errCode != E_OK) {
        return errCode;
    }
    for (auto &item : commitList) {
        std::string gid = std::get<0>(item); // 0 means gid is the first element in assetsInfo
        // 1 means assetsMap info [colName, assets] is the forth element in downloadList[i]
        std::map<std::string, Assets> assetsMap = std::get<1>(item);
        bool setAllNormal = std::get<2>(item); // 2 means whether the download return is E_OK
        VBucket normalAssets;
        VBucket failedAssets;
        normalAssets[CloudDbConstant::GID_FIELD] = gid;
        failedAssets[CloudDbConstant::GID_FIELD] = gid;
        if (setAllNormal) {
            for (auto &[key, asset] : assetsMap) {
                normalAssets[key] = std::move(asset);
            }
        } else {
            SeparateNormalAndFailAssets(assetsMap, normalAssets, failedAssets);
        }
        if (!downloadItem.recordConflict) {
            errCode = FillCloudAssets(tableName, normalAssets, failedAssets);
            if (errCode != E_OK) {
                break;
            }
        }
        LogInfo logInfo;
        logInfo.cloudGid = gid;
        // download must contain gid, just set the default value here.
        logInfo.dataKey = DBConstant::DEFAULT_ROW_ID;
        logInfo.hashKey = downloadItem.hashKey;
        logInfo.timestamp = downloadItem.timestamp;
        // there are failed assets, reset the timestamp to prevent the flag from being marked as consistent.
        if (failedAssets.size() > 1) {
            logInfo.timestamp = 0u;
        }

        errCode = storageProxy_->UpdateRecordFlag(tableName, downloadItem.recordConflict, logInfo);
        if (errCode != E_OK) {
            break;
        }
        successCount++;
    }
    int ret = storageProxy_->SetLogTriggerStatus(true);
    return errCode == E_OK ? ret : errCode;
}

void CloudSyncer::GenerateCompensatedSync(CloudTaskInfo &taskInfo)
{
    std::vector<QuerySyncObject> syncQuery;
    std::vector<std::string> users;
    int errCode = storageProxy_->GetCompensatedSyncQuery(syncQuery, users);
    if (errCode != E_OK) {
        LOGW("[CloudSyncer] Generate compensated sync failed by get query! errCode = %d", errCode);
        return;
    }
    if (syncQuery.empty()) {
        LOGD("[CloudSyncer] Not need generate compensated sync");
        return;
    }
    taskInfo.users.clear();
    auto cloudDBs = cloudDB_.GetCloudDB();
    for (auto &[user, cloudDb] : cloudDBs) {
        auto it = std::find(users.begin(), users.end(), user);
        if (it != users.end()) {
            taskInfo.users.push_back(user);
        }
    }
    for (const auto &query : syncQuery) {
        taskInfo.table.push_back(query.GetRelationTableName());
        taskInfo.queryList.push_back(query);
    }
    Sync(taskInfo);
    LOGI("[CloudSyncer] Generate compensated sync finished");
}

void CloudSyncer::ChkIgnoredProcess(InnerProcessInfo &info, const CloudSyncData &uploadData, UploadParam &uploadParam)
{
    if (uploadData.ignoredCount == 0) { // LCOV_EXCL_BR_LINE
        return;
    }
    info.upLoadInfo.total -= static_cast<uint32_t>(uploadData.ignoredCount);
    if (info.upLoadInfo.successCount + info.upLoadInfo.failCount != info.upLoadInfo.total) { // LCOV_EXCL_BR_LINE
        return;
    }
    if (!CloudSyncUtils::CheckCloudSyncDataEmpty(uploadData)) { // LCOV_EXCL_BR_LINE
        return;
    }
    info.tableStatus = ProcessStatus::FINISHED;
    info.upLoadInfo.batchIndex++;
    NotifyInBatchUpload(uploadParam, info, true);
}

int CloudSyncer::SaveCursorIfNeed(const std::string &tableName)
{
    std::string cursor = "";
    int errCode = storageProxy_->GetCloudWaterMark(tableName, cursor);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] get cloud water mark before download failed %d", errCode);
        return errCode;
    }
    if (!cursor.empty()) {
        return E_OK;
    }
    auto res = cloudDB_.GetEmptyCursor(tableName);
    if (res.first != E_OK) {
        LOGE("[CloudSyncer] get empty cursor failed %d", res.first);
        return res.first;
    }
    if (res.second.empty()) {
        LOGE("[CloudSyncer] get cursor is empty %d", -E_CLOUD_ERROR);
        return -E_CLOUD_ERROR;
    }
    errCode = storageProxy_->SetCloudWaterMark(tableName, res.second);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] set cloud water mark before download failed %d", errCode);
    }
    return errCode;
}

int CloudSyncer::PrepareAndDownload(const std::string &table, const CloudTaskInfo &taskInfo, bool isFirstDownload)
{
    std::string hashDev;
    int errCode = RuntimeContext::GetInstance()->GetLocalIdentity(hashDev);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Failed to get local identity.");
        return errCode;
    }
    errCode = SaveCursorIfNeed(table);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = CheckTaskIdValid(taskInfo.taskId);
    if (errCode != E_OK) {
        LOGW("[CloudSyncer] task is invalid, abort sync");
        return errCode;
    }
    errCode = DoDownload(taskInfo.taskId, isFirstDownload);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] download failed %d", errCode);
    }
    return errCode;
}

bool CloudSyncer::IsClosed() const
{
    return closed_ || IsKilled();
}

int CloudSyncer::UpdateFlagForSavedRecord(const SyncParam &param)
{
    DownloadList downloadList;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        downloadList = currentContext_.assetDownloadList;
    }
    std::set<std::string> gidFilters;
    for (const auto &tuple: downloadList) {
        gidFilters.insert(std::get<CloudSyncUtils::GID_INDEX>(tuple));
    }
    return storageProxy_->MarkFlagAsConsistent(param.tableName, param.downloadData, gidFilters);
}

int CloudSyncer::BatchDelete(Info &deleteInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo)
{
    uint32_t retryCount = 0;
    int errCode = cloudDB_.BatchDelete(uploadData.tableName, uploadData.delData.record,
        uploadData.delData.extend, deleteInfo, retryCount);
    innerProcessInfo.upLoadInfo.successCount += deleteInfo.successCount;
    innerProcessInfo.upLoadInfo.deleteCount += deleteInfo.successCount;
    innerProcessInfo.upLoadInfo.failCount += deleteInfo.failCount;
    if (errCode == -E_CLOUD_VERSION_CONFLICT) {
        ProcessVersionConflictInfo(innerProcessInfo, retryCount);
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Failed to batch delete, %d", errCode);
        storageProxy_->FillCloudGidIfSuccess(OpType::DELETE, uploadData);
        return errCode;
    }
    errCode = storageProxy_->FillCloudLogAndAsset(OpType::DELETE, uploadData);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Failed to fill back when doing upload delData, %d.", errCode);
    }
    return errCode;
}

bool CloudSyncer::IsCompensatedTask(TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return cloudTaskInfos_[taskId].compensatedTask;
}

int CloudSyncer::SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs)
{
    return cloudDB_.SetCloudDB(cloudDBs);
}

void CloudSyncer::CleanAllWaterMark()
{
    storageProxy_->CleanAllWaterMark();
}

void CloudSyncer::GetDownloadItem(const DownloadList &downloadList, size_t i, DownloadItem &downloadItem)
{
    downloadItem.gid = std::get<CloudSyncUtils::GID_INDEX>(downloadList[i]);
    downloadItem.prefix = std::get<CloudSyncUtils::PREFIX_INDEX>(downloadList[i]);
    downloadItem.strategy = std::get<CloudSyncUtils::STRATEGY_INDEX>(downloadList[i]);
    downloadItem.assets = std::get<CloudSyncUtils::ASSETS_INDEX>(downloadList[i]);
    downloadItem.hashKey = std::get<CloudSyncUtils::HASH_KEY_INDEX>(downloadList[i]);
    downloadItem.primaryKeyValList = std::get<CloudSyncUtils::PRIMARY_KEY_INDEX>(downloadList[i]);
    downloadItem.timestamp = std::get<CloudSyncUtils::TIMESTAMP_INDEX>(downloadList[i]);
}

void CloudSyncer::DoNotifyInNeed(const CloudSyncer::TaskId &taskId, const std::vector<std::string> &needNotifyTables,
    const bool isFirstDownload)
{
    bool isNeedNotify = false;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        // only when the first download and the task no need upload actually, notify the process, otherwise,
        // the process will notify in the upload procedure, which can guarantee the notify order of the tables
        isNeedNotify = isFirstDownload && !currentContext_.isNeedUpload;
    }
    if (!isNeedNotify) {
        return;
    }
    for (size_t i = 0; i < needNotifyTables.size(); ++i) {
        UpdateProcessInfoWithoutUpload(taskId, needNotifyTables[i], i != (needNotifyTables.size() - 1u));
    }
}

int CloudSyncer::GetUploadCountByTable(const CloudSyncer::TaskId &taskId, int64_t &count)
{
    std::string tableName;
    int ret = GetCurrentTableName(tableName);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid table name for get local water mark: %d", ret);
        return ret;
    }

    ret = storageProxy_->StartTransaction();
    if (ret != E_OK) {
        LOGE("[CloudSyncer] start transaction failed before getting upload count.");
        return ret;
    }

    ret = storageProxy_->GetUploadCount(GetQuerySyncObject(tableName), IsModeForcePush(taskId),
        IsCompensatedTask(taskId), IsNeedGetLocalWater(taskId), count);
    if (ret != E_OK) {
        // GetUploadCount will return E_OK when upload count is zero.
        LOGE("[CloudSyncer] Failed to get Upload Data Count, %d.", ret);
    }
    // No need Rollback when GetUploadCount failed
    storageProxy_->Commit();
    return ret;
}

void CloudSyncer::UpdateProcessInfoWithoutUpload(CloudSyncer::TaskId taskId, const std::string &tableName,
    bool needNotify)
{
    LOGI("[CloudSyncer] There is no need to doing upload, as the upload data count is zero.");
    InnerProcessInfo innerProcessInfo;
    innerProcessInfo.tableName = tableName;
    innerProcessInfo.upLoadInfo.total = 0;  // count is zero
    innerProcessInfo.tableStatus = ProcessStatus::FINISHED;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (!needNotify) {
            currentContext_.notifier->UpdateProcess(innerProcessInfo);
        } else {
            currentContext_.notifier->NotifyProcess(cloudTaskInfos_[taskId], innerProcessInfo);
        }
    }
}

int CloudSyncer::DoDownloadInNeed(const CloudTaskInfo &taskInfo, const bool needUpload, bool isFirstDownload)
{
    std::vector<std::string> needNotifyTables;
    for (size_t i = 0; i < taskInfo.table.size(); ++i) {
        std::string table;
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            if (currentContext_.processRecorder->IsDownloadFinish(currentContext_.currentUserIndex,
                taskInfo.table[i])) {
                continue;
            }
            LOGD("[CloudSyncer] try download table, index: %zu, table name: %s, length: %u",
                i, DBCommon::StringMiddleMasking(taskInfo.table[i]).c_str(), taskInfo.table[i].length());
            currentContext_.tableName = taskInfo.table[i];
            table = currentContext_.tableName;
        }
        int errCode = PrepareAndDownload(table, taskInfo, isFirstDownload);
        if (errCode != E_OK) {
            return errCode;
        }
        CheckDataAfterDownload(table);
        MarkDownloadFinishIfNeed(table);
        // needUpload indicate that the syncMode need push
        if (needUpload) {
            int64_t count = 0;
            errCode = GetUploadCountByTable(taskInfo.taskId, count);
            if (errCode != E_OK) {
                LOGE("[CloudSyncer] GetUploadCountByTable failed %d", errCode);
                return errCode;
            }
            // count > 0 means current table need upload actually
            if (count > 0) {
                {
                    std::lock_guard<std::mutex> autoLock(dataLock_);
                    currentContext_.isNeedUpload = true;
                }
                continue;
            }
            needNotifyTables.emplace_back(table);
        }
        errCode = SaveCloudWaterMark(taskInfo.table[i], taskInfo.taskId);
        if (errCode != E_OK) {
            LOGE("[CloudSyncer] Can not save cloud water mark after downloading %d", errCode);
            return errCode;
        }
    }
    DoNotifyInNeed(taskInfo.taskId, needNotifyTables, isFirstDownload);
    return E_OK;
}

bool CloudSyncer::IsNeedGetLocalWater(TaskId taskId)
{
    return !IsModeForcePush(taskId) && (!IsPriorityTask(taskId) || IsQueryListEmpty(taskId)) &&
        !IsCompensatedTask(taskId);
}

int CloudSyncer::TryToAddSyncTask(CloudTaskInfo &&taskInfo)
{
    if (closed_) {
        LOGW("[CloudSyncer] syncer is closed, should not sync now");
        return -E_DB_CLOSED;
    }
    std::shared_ptr<DataBaseSchema> cloudSchema;
    int errCode = storageProxy_->GetCloudDbSchema(cloudSchema);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Get cloud schema failed %d when add task", errCode);
        return errCode;
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    errCode = CheckQueueSizeWithNoLock(taskInfo.priorityTask);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = GenerateTaskIdIfNeed(taskInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    auto taskId = taskInfo.taskId;
    cloudTaskInfos_[taskId] = std::move(taskInfo);
    if (cloudTaskInfos_[taskId].priorityTask) {
        priorityTaskQueue_.push_back(taskId);
        LOGI("[CloudSyncer] Add priority task ok, storeId %.3s, taskId %" PRIu64,
            cloudTaskInfos_[taskId].storeId.c_str(), cloudTaskInfos_[taskId].taskId);
        return E_OK;
    }
    if (!MergeTaskInfo(cloudSchema, taskId)) {
        taskQueue_.push_back(taskId);
        LOGI("[CloudSyncer] Add task ok, storeId %.3s, taskId %" PRIu64, cloudTaskInfos_[taskId].storeId.c_str(),
            cloudTaskInfos_[taskId].taskId);
    }
    return E_OK;
}

bool CloudSyncer::MergeTaskInfo(const std::shared_ptr<DataBaseSchema> &cloudSchema, TaskId taskId)
{
    if (!cloudTaskInfos_[taskId].merge) { // LCOV_EXCL_BR_LINE
        return false;
    }
    bool isMerge = false;
    bool mergeHappen = false;
    TaskId checkTaskId = taskId;
    do {
        std::tie(isMerge, checkTaskId) = TryMergeTask(cloudSchema, checkTaskId);
        mergeHappen = mergeHappen || isMerge;
    } while (isMerge);
    return mergeHappen;
}

std::pair<bool, TaskId> CloudSyncer::TryMergeTask(const std::shared_ptr<DataBaseSchema> &cloudSchema, TaskId tryTaskId)
{
    std::pair<bool, TaskId> res;
    auto &[merge, nextTryTask] = res;
    TaskId beMergeTask = INVALID_TASK_ID;
    TaskId runningTask = currentContext_.currentTaskId;
    for (const auto &taskId : taskQueue_) {
        if (taskId == runningTask || taskId == tryTaskId) { // LCOV_EXCL_BR_LINE
            continue;
        }
        if (!IsTasksCanMerge(taskId, tryTaskId)) { // LCOV_EXCL_BR_LINE
            continue;
        }
        if (MergeTaskTablesIfConsistent(taskId, tryTaskId)) { // LCOV_EXCL_BR_LINE
            beMergeTask = taskId;
            nextTryTask = tryTaskId;
            merge = true;
            break;
        }
        if (MergeTaskTablesIfConsistent(tryTaskId, taskId)) { // LCOV_EXCL_BR_LINE
            beMergeTask = tryTaskId;
            nextTryTask = taskId;
            merge = true;
            break;
        }
    }
    if (!merge) { // LCOV_EXCL_BR_LINE
        return res;
    }
    if (beMergeTask < nextTryTask) { // LCOV_EXCL_BR_LINE
        std::tie(beMergeTask, nextTryTask) = SwapTwoTaskAndCopyTable(beMergeTask, nextTryTask);
    }
    AdjustTableBasedOnSchema(cloudSchema, cloudTaskInfos_[nextTryTask]);
    auto processNotifier = std::make_shared<ProcessNotifier>(this);
    processNotifier->Init(cloudTaskInfos_[beMergeTask].table, cloudTaskInfos_[beMergeTask].devices,
        cloudTaskInfos_[beMergeTask].users);
    cloudTaskInfos_[beMergeTask].errCode = -E_CLOUD_SYNC_TASK_MERGED;
    cloudTaskInfos_[beMergeTask].status = ProcessStatus::FINISHED;
    processNotifier->SetAllTableFinish();
    processNotifier->NotifyProcess(cloudTaskInfos_[beMergeTask], {}, true);
    cloudTaskInfos_.erase(beMergeTask);
    taskQueue_.remove(beMergeTask);
    LOGW("[CloudSyncer] TaskId %" PRIu64 " has been merged", beMergeTask);
    return res;
}

bool CloudSyncer::IsTaskCanMerge(const CloudTaskInfo &taskInfo)
{
    return !taskInfo.compensatedTask && !taskInfo.priorityTask &&
        taskInfo.merge && taskInfo.mode == SYNC_MODE_CLOUD_MERGE;
}

bool CloudSyncer::IsTasksCanMerge(TaskId taskId, TaskId tryMergeTaskId)
{
    const auto &taskInfo = cloudTaskInfos_[taskId];
    const auto &tryMergeTaskInfo = cloudTaskInfos_[tryMergeTaskId];
    return IsTaskCanMerge(taskInfo) && IsTaskCanMerge(tryMergeTaskInfo) &&
        taskInfo.devices == tryMergeTaskInfo.devices;
}

bool CloudSyncer::MergeTaskTablesIfConsistent(TaskId sourceId, TaskId targetId)
{
    const auto &source = cloudTaskInfos_[sourceId];
    const auto &target = cloudTaskInfos_[targetId];
    bool isMerge = true;
    for (const auto &table : source.table) {
        if (std::find(target.table.begin(), target.table.end(), table) == target.table.end()) { // LCOV_EXCL_BR_LINE
            isMerge = false;
            break;
        }
    }
    return isMerge;
}

void CloudSyncer::AdjustTableBasedOnSchema(const std::shared_ptr<DataBaseSchema> &cloudSchema,
    CloudTaskInfo &taskInfo)
{
    std::vector<std::string> tmpTables = taskInfo.table;
    taskInfo.table.clear();
    taskInfo.queryList.clear();
    for (const auto &table : cloudSchema->tables) {
        if (std::find(tmpTables.begin(), tmpTables.end(), table.name) != tmpTables.end()) { // LCOV_EXCL_BR_LINE
            taskInfo.table.push_back(table.name);
            QuerySyncObject querySyncObject;
            querySyncObject.SetTableName(table.name);
            taskInfo.queryList.push_back(querySyncObject);
        }
    }
}

std::pair<TaskId, TaskId> CloudSyncer::SwapTwoTaskAndCopyTable(TaskId source, TaskId target)
{
    cloudTaskInfos_[source].table = cloudTaskInfos_[target].table;
    cloudTaskInfos_[source].queryList = cloudTaskInfos_[target].queryList;
    std::set<std::string> users;
    users.insert(cloudTaskInfos_[source].users.begin(), cloudTaskInfos_[source].users.end());
    users.insert(cloudTaskInfos_[target].users.begin(), cloudTaskInfos_[target].users.end());
    cloudTaskInfos_[source].users = std::vector<std::string>(users.begin(), users.end());
    return {target, source};
}

bool CloudSyncer::IsQueryListEmpty(TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return !std::any_of(cloudTaskInfos_[taskId].queryList.begin(), cloudTaskInfos_[taskId].queryList.end(),
        [](const auto &item) {
            return item.IsContainQueryNodes();
    });
}

bool CloudSyncer::IsNeedLock(const UploadParam &param)
{
    return param.lockAction == LockAction::INSERT && param.mode == CloudWaterType::INSERT;
}

std::pair<int, Timestamp> CloudSyncer::GetLocalWater(const std::string &tableName, UploadParam &uploadParam)
{
    std::pair<int, Timestamp> res = { E_OK, 0u };
    if (IsNeedGetLocalWater(uploadParam.taskId)) {
        res.first = storageProxy_->GetLocalWaterMarkByMode(tableName, uploadParam.mode, res.second);
    }
    uploadParam.localMark = res.second;
    return res;
}

int CloudSyncer::HandleBatchUpload(UploadParam &uploadParam, InnerProcessInfo &info,
    CloudSyncData &uploadData, ContinueToken &continueStmtToken, std::vector<ReviseModTimeInfo> &revisedData)
{
    int ret = E_OK;
    uint32_t batchIndex = GetCurrentTableUploadBatchIndex();
    bool isLocked = false;
    while (!CloudSyncUtils::CheckCloudSyncDataEmpty(uploadData)) {
        revisedData.insert(revisedData.end(), uploadData.revisedData.begin(), uploadData.revisedData.end());
        ret = PreProcessBatchUpload(uploadParam, info, uploadData);
        if (ret != E_OK) {
            break;
        }
        info.upLoadInfo.batchIndex = ++batchIndex;
        if (IsNeedLock(uploadParam) && !isLocked) {
            ret = LockCloudIfNeed(uploadParam.taskId);
            if (ret != E_OK) {
                break;
            }
            isLocked = true;
        }
        ret = DoBatchUpload(uploadData, uploadParam, info);
        if (ret != E_OK) {
            break;
        }
        uploadData = CloudSyncData(uploadData.tableName, uploadParam.mode);
        if (continueStmtToken == nullptr) {
            break;
        }
        SetUploadDataFlag(uploadParam.taskId, uploadData);
        LOGI("[CloudSyncer] Write local water after upload one batch, table[%s length[%u]], water[%llu]",
            DBCommon::StringMiddleMasking(uploadData.tableName).c_str(), uploadData.tableName.length(),
            uploadParam.localMark);
        RecordWaterMark(uploadParam.taskId, uploadParam.localMark);
        ret = storageProxy_->GetCloudDataNext(continueStmtToken, uploadData);
        if ((ret != E_OK) && (ret != -E_UNFINISHED)) {
            LOGE("[CloudSyncer] Failed to get cloud data next when doing upload, %d.", ret);
            break;
        }
        ChkIgnoredProcess(info, uploadData, uploadParam);
    }
    if ((ret != E_OK) && (ret != -E_UNFINISHED) && (ret != -E_TASK_PAUSED)) {
        NotifyUploadFailed(ret, info);
    }
    if (isLocked && IsNeedLock(uploadParam)) {
        UnlockIfNeed();
    }
    return ret;
}

int CloudSyncer::DoUploadInner(const std::string &tableName, UploadParam &uploadParam)
{
    InnerProcessInfo info = GetInnerProcessInfo(tableName, uploadParam);
    static std::vector<CloudWaterType> waterTypes = DBCommon::GetWaterTypeVec();
    int errCode = E_OK;
    for (const auto &waterType: waterTypes) {
        uploadParam.mode = waterType;
        errCode = DoUploadByMode(tableName, uploadParam, info);
        if (errCode != E_OK) {
            break;
        }
    }
    int ret = E_OK;
    if (info.upLoadInfo.successCount > 0) {
        ret = UploadVersionRecordIfNeed(uploadParam);
    }
    return errCode != E_OK ? errCode : ret;
}

int CloudSyncer::UploadVersionRecordIfNeed(const UploadParam &uploadParam)
{
    if (uploadParam.count == 0) {
        // no record upload
        return E_OK;
    }
    if (!cloudDB_.IsExistCloudVersionCallback()) {
        return E_OK;
    }
    auto [errCode, uploadData] = storageProxy_->GetLocalCloudVersion();
    if (errCode != E_OK) {
        return errCode;
    }
    bool isInsert = !uploadData.insData.record.empty();
    CloudSyncBatch &batchData = isInsert ? uploadData.insData : uploadData.updData;
    if (batchData.record.empty()) {
        LOGE("[CloudSyncer] Get invalid cloud version record");
        return -E_INTERNAL_ERROR;
    }
    std::string oriVersion;
    CloudStorageUtils::GetStringFromCloudData(CloudDbConstant::CLOUD_KV_FIELD_VALUE, batchData.record[0], oriVersion);
    std::string newVersion;
    std::tie(errCode, newVersion) = cloudDB_.GetCloudVersion(oriVersion);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Get cloud version error %d", errCode);
        return errCode;
    }
    batchData.record[0][CloudDbConstant::CLOUD_KV_FIELD_VALUE] = newVersion;
    InnerProcessInfo processInfo;
    Info info;
    std::vector<VBucket> copyRecord = batchData.record;
    WaterMark waterMark;
    CloudSyncUtils::GetWaterMarkAndUpdateTime(batchData.extend, waterMark);
    errCode = isInsert ? BatchInsert(info, uploadData, processInfo) : BatchUpdate(info, uploadData, processInfo);
    batchData.record = copyRecord;
    CloudSyncUtils::ModifyCloudDataTime(batchData.extend[0]);
    auto ret = storageProxy_->FillCloudLogAndAsset(isInsert ? OpType::INSERT : OpType::UPDATE, uploadData);
    return errCode != E_OK ? errCode : ret;
}

void CloudSyncer::TagUploadAssets(CloudSyncData &uploadData)
{
    if (!IsDataContainAssets()) {
        return;
    }
    std::vector<Field> assetFields;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        assetFields = currentContext_.assetFields[currentContext_.tableName];
    }

    for (size_t i = 0; i < uploadData.insData.extend.size(); i++) {
        for (const Field &assetField : assetFields) {
            (void)TagAssetsInSingleCol(assetField, true, uploadData.insData.record[i]);
        }
    }
    for (size_t i = 0; i < uploadData.updData.extend.size(); i++) {
        for (const Field &assetField : assetFields) {
            (void)TagAssetsInSingleCol(assetField, false, uploadData.updData.record[i]);
        }
    }
}

bool CloudSyncer::IsLockInDownload()
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (cloudTaskInfos_.find(currentContext_.currentTaskId) == cloudTaskInfos_.end()) {
        return false;
    }
    auto currentLockAction = static_cast<uint32_t>(cloudTaskInfos_[currentContext_.currentTaskId].lockAction);
    return (currentLockAction & static_cast<uint32_t>(LockAction::DOWNLOAD)) != 0;
}

CloudSyncEvent CloudSyncer::SetCurrentTaskFailedInMachine(int errCode)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    cloudTaskInfos_[currentContext_.currentTaskId].errCode = errCode;
    return CloudSyncEvent::ERROR_EVENT;
}

void CloudSyncer::InitCloudSyncStateMachine()
{
    CloudSyncStateMachine::Initialize();
    cloudSyncStateMachine_.RegisterFunc(CloudSyncState::DO_DOWNLOAD, [this]() {
        return SyncMachineDoDownload();
    });
    cloudSyncStateMachine_.RegisterFunc(CloudSyncState::DO_UPLOAD, [this]() {
        return SyncMachineDoUpload();
    });
    cloudSyncStateMachine_.RegisterFunc(CloudSyncState::DO_FINISHED, [this]() {
        return SyncMachineDoFinished();
    });
    cloudSyncStateMachine_.RegisterFunc(CloudSyncState::DO_REPEAT_CHECK, [this]() {
        return SyncMachineDoRepeatCheck();
    });
}

CloudSyncEvent CloudSyncer::SyncMachineDoRepeatCheck()
{
    auto config = storageProxy_->GetCloudSyncConfig();
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (config.maxRetryConflictTimes < 0) { // unlimited repeat counts
            return CloudSyncEvent::REPEAT_DOWNLOAD_EVENT;
        }
        currentContext_.repeatCount++;
        if (currentContext_.repeatCount > config.maxRetryConflictTimes) {
            LOGD("[CloudSyncer] Repeat too much times current %d limit %" PRId32, currentContext_.repeatCount,
                config.maxRetryConflictTimes);
            SetCurrentTaskFailedWithoutLock(-E_CLOUD_VERSION_CONFLICT);
            return CloudSyncEvent::ERROR_EVENT;
        }
        LOGD("[CloudSyncer] Repeat taskId %" PRIu64 " download current %d", currentContext_.currentTaskId,
            currentContext_.repeatCount);
    }
    return CloudSyncEvent::REPEAT_DOWNLOAD_EVENT;
}

void CloudSyncer::MarkDownloadFinishIfNeed(const std::string &downloadTable, bool isFinish)
{
    // table exist reference should download every times
    if (IsLockInDownload() || storageProxy_->IsTableExistReferenceOrReferenceBy(downloadTable)) {
        return;
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    currentContext_.processRecorder->MarkDownloadFinish(currentContext_.currentUserIndex, downloadTable, isFinish);
}

int CloudSyncer::DoUploadByMode(const std::string &tableName, UploadParam &uploadParam, InnerProcessInfo &info)
{
    CloudSyncData uploadData(tableName, uploadParam.mode);
    SetUploadDataFlag(uploadParam.taskId, uploadData);
    auto [err, localWater] = GetLocalWater(tableName, uploadParam);
    LOGI("[CloudSyncer] Get local water before upload result: %d, table[%s length[%u]], water[%llu]", err,
        DBCommon::StringMiddleMasking(tableName).c_str(), tableName.length(), localWater);
    if (err != E_OK) {
        return err;
    }
    ContinueToken continueStmtToken = nullptr;
    int ret = storageProxy_->GetCloudData(GetQuerySyncObject(tableName), localWater, continueStmtToken, uploadData);
    if ((ret != E_OK) && (ret != -E_UNFINISHED)) {
        LOGE("[CloudSyncer] Failed to get cloud data when upload, %d.", ret);
        return ret;
    }
    uploadParam.count -= uploadData.ignoredCount;
    info.upLoadInfo.total -= static_cast<uint32_t>(uploadData.ignoredCount);
    std::vector<ReviseModTimeInfo> revisedData;
    ret = HandleBatchUpload(uploadParam, info, uploadData, continueStmtToken, revisedData);
    if (ret != -E_TASK_PAUSED) {
        // reset watermark to zero when task no paused
        RecordWaterMark(uploadParam.taskId, 0u);
    }
    if (continueStmtToken != nullptr) {
        storageProxy_->ReleaseContinueToken(continueStmtToken);
    }
    if (!revisedData.empty()) {
        int errCode = storageProxy_->ReviseLocalModTime(tableName, revisedData);
        if (errCode != E_OK) {
            LOGE("[CloudSyncer] Failed to revise local modify time: %d, table: %s",
                errCode, DBCommon::StringMiddleMasking(tableName).c_str());
        }
    }
    return ret;
}

bool CloudSyncer::IsTableFinishInUpload(const std::string &table)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return currentContext_.processRecorder->IsUploadFinish(currentContext_.currentUserIndex, table);
}

void CloudSyncer::MarkUploadFinishIfNeed(const std::string &table)
{
    // table exist reference or reference by should upload every times
    if (storageProxy_->IsTableExistReferenceOrReferenceBy(table)) {
        return;
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    currentContext_.processRecorder->MarkUploadFinish(currentContext_.currentUserIndex, table, true);
}

bool CloudSyncer::IsNeedUpdateAsset(const VBucket &data)
{
    for (const auto &item : data) {
        const Asset *asset = std::get_if<TYPE_INDEX<Asset>>(&item.second);
        if (asset != nullptr) {
            uint32_t lowBitStatus = AssetOperationUtils::EraseBitMask(asset->status);
            if (lowBitStatus == static_cast<uint32_t>(AssetStatus::ABNORMAL) ||
            lowBitStatus == static_cast<uint32_t>(AssetStatus::DOWNLOADING)) {
                return true;
            }
            continue;
        }
        const Assets *assets = std::get_if<TYPE_INDEX<Assets>>(&item.second);
        if (assets == nullptr) {
            continue;
        }
        for (const auto &oneAsset : *assets) {
            uint32_t lowBitStatus = AssetOperationUtils::EraseBitMask(oneAsset.status);
            if (lowBitStatus == static_cast<uint32_t>(AssetStatus::ABNORMAL) ||
            lowBitStatus == static_cast<uint32_t>(AssetStatus::DOWNLOADING)) {
                return true;
            }
        }
    }
    return false;
}

SyncProcess CloudSyncer::GetCloudTaskStatus(uint64_t taskId) const
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    auto iter = cloudTaskInfos_.find(taskId);
    SyncProcess syncProcess;
    if (iter == cloudTaskInfos_.end()) {
        syncProcess.process = ProcessStatus::FINISHED;
        syncProcess.errCode = NOT_FOUND;
        LOGE("[CloudSyncer] Not found task %" PRIu64, taskId);
        return syncProcess;
    }
    syncProcess.process = iter->second.status;
    syncProcess.errCode = TransferDBErrno(iter->second.errCode);
    std::shared_ptr<ProcessNotifier> notifier = nullptr;
    if (currentContext_.currentTaskId == taskId) {
        notifier = currentContext_.notifier;
    }
    bool hasNotifier = notifier != nullptr;
    if (hasNotifier) {
        syncProcess.tableProcess = notifier->GetCurrentTableProcess();
    }
    LOGI("[CloudSyncer] Found task %" PRIu64 " storeId %.3s status %d has notifier %d", taskId,
        iter->second.storeId.c_str(), static_cast<int64_t>(syncProcess.process), static_cast<int>(hasNotifier));
    return syncProcess;
}

int CloudSyncer::GenerateTaskIdIfNeed(CloudTaskInfo &taskInfo)
{
    if (taskInfo.taskId != INVALID_TASK_ID) {
        if (cloudTaskInfos_.find(taskInfo.taskId) != cloudTaskInfos_.end()) {
            LOGE("[CloudSyncer] Sync with exist taskId %" PRIu64 " storeId %.3s", taskInfo.taskId,
                taskInfo.storeId.c_str());
            return -E_INVALID_ARGS;
        }
        lastTaskId_ = std::max(lastTaskId_, taskInfo.taskId);
        LOGI("[CloudSyncer] Sync with taskId %" PRIu64 " storeId %.3s", taskInfo.taskId, taskInfo.storeId.c_str());
        return E_OK;
    }
    lastTaskId_++;
    if (lastTaskId_ == UINT64_MAX) {
        lastTaskId_ = 1u;
    }
    taskInfo.taskId = lastTaskId_;
    return E_OK;
}

void CloudSyncer::ProcessVersionConflictInfo(InnerProcessInfo &innerProcessInfo, uint32_t retryCount)
{
    innerProcessInfo.retryInfo.uploadBatchRetryCount = retryCount;
    CloudSyncConfig config = storageProxy_->GetCloudSyncConfig();
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (config.maxRetryConflictTimes >= 0 &&
            currentContext_.repeatCount + 1 > config.maxRetryConflictTimes) {
            innerProcessInfo.upLoadInfo.failCount =
                innerProcessInfo.upLoadInfo.total - innerProcessInfo.upLoadInfo.successCount;
        }
    }
}

std::string CloudSyncer::GetStoreIdByTask(TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return cloudTaskInfos_[taskId].storeId;
}

int CloudSyncer::CleanKvCloudData(std::function<int(void)> &removeFunc)
{
    hasKvRemoveTask = true;
    CloudSyncer::TaskId currentTask;
    {
        // stop task if exist
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentTask = currentContext_.currentTaskId;
    }
    if (currentTask != INVALID_TASK_ID) {
        StopAllTasks(-E_CLOUD_ERROR);
    }
    int errCode = E_OK;
    {
        std::lock_guard<std::mutex> lock(syncMutex_);
        errCode = removeFunc();
        hasKvRemoveTask = false;
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] removeFunc execute failed errCode: %d.", errCode);
    }
    return errCode;
}

void CloudSyncer::StopAllTasks(int errCode)
{
    CloudSyncer::TaskId currentTask;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentTask = currentContext_.currentTaskId;
    }
    // mark current task user_change
    SetTaskFailed(currentTask, errCode);
    UnlockIfNeed();
    WaitCurTaskFinished();

    std::vector<CloudTaskInfo> infoList = CopyAndClearTaskInfos();
    for (auto &info: infoList) {
        LOGI("[CloudSyncer] finished taskId %" PRIu64 " with errCode %d, isPriority %d.", info.taskId, errCode,
            info.priorityTask);
        auto processNotifier = std::make_shared<ProcessNotifier>(this);
        processNotifier->Init(info.table, info.devices, info.users);
        info.errCode = errCode;
        info.status = ProcessStatus::FINISHED;
        processNotifier->NotifyProcess(info, {}, true);
    }
}

int CloudSyncer::TagStatus(bool isExist, SyncParam &param, size_t idx, DataInfo &dataInfo, VBucket &localAssetInfo)
{
    OpType strategyOpResult = OpType::NOT_HANDLE;
    int errCode = TagStatusByStrategy(isExist, param, dataInfo, strategyOpResult);
    if (errCode != E_OK) {
        return errCode;
    }
    param.downloadData.opType[idx] = strategyOpResult;
    if (!IsDataContainAssets()) {
        return E_OK;
    }
    Key hashKey;
    if (isExist) {
        hashKey = dataInfo.localInfo.logInfo.hashKey;
    }
    return TagDownloadAssets(hashKey, idx, param, dataInfo, localAssetInfo);
}

int CloudSyncer::CloudDbBatchDownloadAssets(TaskId taskId, const DownloadList &downloadList,
    const std::set<Key> &dupHashKeySet, InnerProcessInfo &info, ChangedData &changedAssets)
{
    int errorCode = CheckTaskIdValid(taskId);
    if (errorCode != E_OK) {
        return errorCode;
    }
    bool isSharedTable = false;
    errorCode = storageProxy_->IsSharedTable(info.tableName, isSharedTable);
    if (errorCode != E_OK) {
        LOGE("[CloudSyncer] check is shared table failed %d", errorCode);
        return errorCode;
    }
    // prepare download data
    auto [downloadRecord, removeAssets, downloadAssets] =
        GetDownloadRecords(downloadList, dupHashKeySet, isSharedTable, info);
    std::tuple<DownloadItemRecords, RemoveAssetsRecords, DownloadAssetsRecords, bool> detail = {
        std::move(downloadRecord), std::move(removeAssets), std::move(downloadAssets), isSharedTable
    };
    return BatchDownloadAndCommitRes(downloadList, dupHashKeySet, info, changedAssets, detail);
}

void CloudSyncer::FillDownloadItem(const std::set<Key> &dupHashKeySet, const DownloadList &downloadList,
    const InnerProcessInfo &info, bool isSharedTable, DownloadItems &record)
{
    CloudStorageUtils::EraseNoChangeAsset(record.downloadItem.assets);
    if (record.downloadItem.assets.empty()) { // Download data (include deleting)
        return;
    }
    if (isSharedTable) {
        // share table will not download asset, need to reset the status
        for (auto &entry: record.downloadItem.assets) {
            for (auto &asset: entry.second) {
                asset.status = AssetStatus::NORMAL;
            }
        }
        return;
    }
    int errCode = E_OK;
    VBucket dbAssets;
    if (!IsNeedSkipDownload(isSharedTable, errCode, info, record.downloadItem, dbAssets)) {
        GetAssetsToRemove(record.downloadItem.assets, dbAssets, isSharedTable, record.assetsToRemove);
        GetAssetsToDownload(record.downloadItem.assets, dbAssets, isSharedTable, record.assetsToDownload, record.flags);
    }
}

CloudSyncer::DownloadAssetDetail CloudSyncer::GetDownloadRecords(const DownloadList &downloadList,
    const std::set<Key> &dupHashKeySet, bool isSharedTable, const InnerProcessInfo &info)
{
    DownloadItemRecords downloadRecord;
    RemoveAssetsRecords removeAssets;
    DownloadAssetsRecords downloadAssets;
    for (size_t i = 0; i < downloadList.size(); i++) {
        DownloadItems record;
        GetDownloadItem(downloadList, i, record.downloadItem);
        FillDownloadItem(dupHashKeySet, downloadList, info, isSharedTable, record);

        IAssetLoader::AssetRecord removeAsset = {
            record.downloadItem.gid, record.downloadItem.prefix, std::move(record.assetsToRemove)
        };
        removeAssets.push_back(std::move(removeAsset));
        IAssetLoader::AssetRecord downloadAsset = {
            record.downloadItem.gid, record.downloadItem.prefix, std::move(record.assetsToDownload)
        };
        downloadAssets.push_back(std::move(downloadAsset));
        downloadRecord.push_back(std::move(record));
    }
    return {downloadRecord, removeAssets, downloadAssets};
}

int CloudSyncer::BatchDownloadAndCommitRes(const DownloadList &downloadList, const std::set<Key> &dupHashKeySet,
    InnerProcessInfo &info, ChangedData &changedAssets,
    std::tuple<DownloadItemRecords, RemoveAssetsRecords, DownloadAssetsRecords, bool> &downloadDetail)
{
    auto &[downloadRecord, removeAssets, downloadAssets, isSharedTable] = downloadDetail;
    // download and remove in batch
    auto deleteRes = cloudDB_.BatchRemoveLocalAssets(info.tableName, removeAssets);
    auto downloadRes = cloudDB_.BatchDownload(info.tableName, downloadAssets);
    if (deleteRes == -E_NOT_SET || downloadRes == -E_NOT_SET) {
        return -E_NOT_SET;
    }
    int errorCode = E_OK;
    int index = 0;
    for (auto &item : downloadRecord) {
        auto deleteCode = removeAssets[index].status == OK ? E_OK : -E_REMOVE_ASSETS_FAILED;
        auto downloadCode = CloudDBProxy::GetInnerErrorCode(downloadAssets[index].status);
        downloadCode = downloadCode == -E_CLOUD_RECORD_EXIST_CONFLICT ? E_OK : downloadCode;
        if (!isSharedTable) {
            item.downloadItem.assets = BackFillAssetsAfterDownload(downloadCode, deleteCode, item.flags,
                downloadAssets[index].assets, removeAssets[index].assets);
        }
        StatisticDownloadRes(downloadAssets[index], removeAssets[index], info, item.downloadItem);
        AddNotifyDataFromDownloadAssets(dupHashKeySet, item.downloadItem, changedAssets);
        if (item.downloadItem.strategy == OpType::DELETE) {
            item.downloadItem.assets = {};
            item.downloadItem.gid = "";
        }
        // commit download res
        DownloadCommitList commitList;
        // Process result of each asset
        commitList.push_back(std::make_tuple(item.downloadItem.gid, std::move(item.downloadItem.assets),
            deleteCode == E_OK && downloadCode == E_OK));
        errorCode = (errorCode != E_OK) ? errorCode : deleteCode;
        errorCode = (errorCode != E_OK) ? errorCode : downloadCode;
        int currErrorCode = (deleteCode != E_OK) ? deleteCode : downloadCode;
        int ret = CommitDownloadResult(item.downloadItem, info, commitList, currErrorCode);
        if (ret != E_OK) {
            errorCode = errorCode == E_OK ? ret : errorCode;
        }
        index++;
    }
    return errorCode;
}

void CloudSyncer::StatisticDownloadRes(const IAssetLoader::AssetRecord &downloadRecord,
    const IAssetLoader::AssetRecord &removeRecord, InnerProcessInfo &info, DownloadItem &downloadItem)
{
    if ((downloadRecord.status == OK) && (removeRecord.status == OK)) {
        return;
    }
    if ((downloadRecord.status == CLOUD_RECORD_EXIST_CONFLICT) ||
        (removeRecord.status == CLOUD_RECORD_EXIST_CONFLICT)) {
        downloadItem.recordConflict = true;
        return;
    }
    info.downLoadInfo.failCount += 1;
    if (info.downLoadInfo.successCount == 0) {
        LOGW("[CloudSyncer] Invalid successCount");
    } else {
        info.downLoadInfo.successCount -= 1;
    }
}

void CloudSyncer::AddNotifyDataFromDownloadAssets(const std::set<Key> &dupHashKeySet, DownloadItem &downloadItem,
    ChangedData &changedAssets)
{
    if (downloadItem.assets.empty()) {
        return;
    }
    if (dupHashKeySet.find(downloadItem.hashKey) == dupHashKeySet.end()) {
        changedAssets.primaryData[CloudSyncUtils::OpTypeToChangeType(downloadItem.strategy)].push_back(
            downloadItem.primaryKeyValList);
    } else if (downloadItem.strategy == OpType::INSERT) {
        changedAssets.primaryData[ChangeType::OP_UPDATE].push_back(downloadItem.primaryKeyValList);
    }
}

void CloudSyncer::CheckDataAfterDownload(const std::string &tableName)
{
    int dataCount = 0;
    int logicDeleteDataCount = 0;
    int errCode = storageProxy_->GetLocalDataCount(tableName, dataCount, logicDeleteDataCount);
    if (errCode == E_OK) {
        LOGI("[CloudSyncer] Check local data after download[%s[%u]], data count: %d, logic delete data count: %d",
            DBCommon::StringMiddleMasking(tableName).c_str(), tableName.length(), dataCount, logicDeleteDataCount);
    } else {
        LOGW("[CloudSyncer] Get local data after download fail: %d", errCode);
    }
}

void CloudSyncer::CheckQueryCloudData(std::string &traceId, DownloadData &downloadData,
    std::vector<std::string> &pkColNames)
{
    for (auto &data : downloadData.data) {
        bool isVersionExist = data.count(CloudDbConstant::VERSION_FIELD) != 0;
        bool isContainAllPk = true;
        for (auto &pkColName : pkColNames) {
            if (data.count(pkColName) == 0) {
                isContainAllPk = false;
                break;
            }
        }
        std::string gid;
        (void)CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, data, gid);
        if (!isVersionExist || !isContainAllPk) {
            LOGE("[CloudSyncer] Invalid data from cloud, no version[%d], lost primary key[%d], gid[%s], traceId[%s]",
                static_cast<int>(!isVersionExist), static_cast<int>(!isContainAllPk), gid.c_str(), traceId.c_str());
        }
    }
}
}
