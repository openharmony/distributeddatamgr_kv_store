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

void CloudSyncer::ReloadUploadInfoIfNeed(TaskId taskId, const UploadParam &param, InnerProcessInfo &info)
{
    info.upLoadInfo.total = static_cast<uint32_t>(param.count);
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (!cloudTaskInfos_[taskId].resume) {
            return;
        }
    }

    Info lastUploadInfo;
    GetLastUploadInfo(info.tableName, lastUploadInfo);
    info.upLoadInfo.total += lastUploadInfo.successCount;
    info.upLoadInfo.successCount += lastUploadInfo.successCount;
    info.upLoadInfo.failCount += lastUploadInfo.failCount;
    info.upLoadInfo.insertCount += lastUploadInfo.insertCount;
    info.upLoadInfo.updateCount += lastUploadInfo.updateCount;
    info.upLoadInfo.deleteCount += lastUploadInfo.deleteCount;
    LOGD("[CloudSyncer] resume upload, last success count %" PRIu32 ", last fail count %" PRIu32,
        lastUploadInfo.successCount, lastUploadInfo.failCount);
}

void CloudSyncer::GetLastUploadInfo(const std::string &tableName, Info &lastUploadInfo)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return currentContext_.notifier->GetLastUploadInfo(tableName, lastUploadInfo);
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
    int errCode = cloudDB_.BatchInsert(uploadData.tableName, uploadData.insData.record,
        uploadData.insData.extend, insertInfo);
    innerProcessInfo.upLoadInfo.successCount += insertInfo.successCount;
    innerProcessInfo.upLoadInfo.insertCount += insertInfo.successCount;
    innerProcessInfo.upLoadInfo.total -= insertInfo.total - insertInfo.successCount - insertInfo.failCount;
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
    int errCode = cloudDB_.BatchUpdate(uploadData.tableName, uploadData.updData.record,
        uploadData.updData.extend, updateInfo);
    innerProcessInfo.upLoadInfo.successCount += updateInfo.successCount;
    innerProcessInfo.upLoadInfo.updateCount += updateInfo.successCount;
    innerProcessInfo.upLoadInfo.total -= updateInfo.total - updateInfo.successCount - updateInfo.failCount;
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

int CloudSyncer::DownloadAssetsOneByOneInner(bool isSharedTable, const InnerProcessInfo &info,
    DownloadItem &downloadItem, std::map<std::string, Assets> &downloadAssets)
{
    int errCode = E_OK;
    std::map<std::string, Assets> tmpAssetsToDownload;
    std::map<std::string, Assets> tmpAssetsToDelete;
    std::map<std::string, std::vector<uint32_t>> tmpFlags;
    for (auto &[col, assets] : downloadAssets) {
        for (auto &asset : assets) {
            VBucket dbAssets;
            if (IsNeedSkipDownload(isSharedTable, errCode, info, downloadItem, dbAssets)) {
                break;
            }
            if (!isSharedTable && asset.flag == static_cast<uint32_t>(AssetOpType::DELETE)) {
                asset.status = static_cast<uint32_t>(AssetStatus::DELETE);
                tmpAssetsToDelete[col].push_back(asset);
            } else if (!isSharedTable && AssetOperationUtils::CalAssetOperation(col, asset, dbAssets,
                AssetOperationUtils::CloudSyncAction::START_DOWNLOAD) == AssetOperationUtils::AssetOpType::HANDLE) {
                asset.status = asset.flag == static_cast<uint32_t>(AssetOpType::INSERT) ?
                    static_cast<uint32_t>(AssetStatus::INSERT) : static_cast<uint32_t>(AssetStatus::UPDATE);
                tmpAssetsToDownload[col].push_back(asset);
                tmpFlags[col].push_back(asset.flag);
            } else {
                LOGD("[CloudSyncer] skip download asset...");
            }
        }
    }
    auto deleteCode = cloudDB_.RemoveLocalAssets(info.tableName, downloadItem.gid, downloadItem.prefix,
        tmpAssetsToDelete);
    auto downloadCode = cloudDB_.Download(info.tableName, downloadItem.gid, downloadItem.prefix, tmpAssetsToDownload);
    if (!CheckDownloadOrDeleteCode(errCode, downloadCode, deleteCode, downloadItem)) {
        return errCode;
    }

    // copy asset back
    downloadAssets = BackFillAssetsAfterDownload(downloadCode, deleteCode, tmpFlags, tmpAssetsToDownload,
        tmpAssetsToDelete);
    return errCode;
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
        VBucket &assets = setAllNormal ? normalAssets : failedAssets;
        for (auto &[key, asset] : assetsMap) {
            assets[key] = std::move(asset);
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
    bool isShared = false;
    errCode = storageProxy_->IsSharedTable(table, isShared);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] check shared table failed %d", errCode);
        return errCode;
    }
    // shared table not allow logic delete
    storageProxy_->SetCloudTaskConfig({ !isShared });
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
    int errCode = cloudDB_.BatchDelete(uploadData.tableName, uploadData.delData.record,
        uploadData.delData.extend, deleteInfo);
    innerProcessInfo.upLoadInfo.successCount += deleteInfo.successCount;
    innerProcessInfo.upLoadInfo.deleteCount += deleteInfo.successCount;
    innerProcessInfo.upLoadInfo.total -= deleteInfo.total - deleteInfo.successCount - deleteInfo.failCount;
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
            LOGD("[CloudSyncer] try download table, index: %zu", i);
            currentContext_.tableName = taskInfo.table[i];
            table = currentContext_.tableName;
        }
        int errCode = PrepareAndDownload(table, taskInfo, isFirstDownload);
        if (errCode != E_OK) {
            return errCode;
        }
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
    // table exist reference should upload every times
    if (storageProxy_->IsTableExistReference(table)) {
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
}
