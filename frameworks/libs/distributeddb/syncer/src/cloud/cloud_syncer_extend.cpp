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

void CloudSyncer::ReloadCloudWaterMarkIfNeed(const std::string tableName, std::string &cloudWaterMark)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    std::string cacheCloudWaterMark = currentContext_.cloudWaterMarks[tableName];
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

void CloudSyncer::NotifyUploadFailed(int errCode, InnerProcessInfo &info)
{
    if (errCode == -E_CLOUD_VERSION_CONFLICT) {
        LOGI("[CloudSyncer] Stop upload due to version conflict, %d", errCode);
    } else {
        LOGE("[CloudSyncer] Failed to do upload, %d", errCode);
    }
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
        ret = CloudSyncUtils::FillAssetIdToAssets(uploadData.insData, errCode);
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
    innerProcessInfo.upLoadInfo.successCount += insertInfo.successCount;
    return E_OK;
}

int CloudSyncer::BatchUpdate(Info &updateInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo)
{
    int errCode = cloudDB_.BatchUpdate(uploadData.tableName, uploadData.updData.record,
        uploadData.updData.extend, updateInfo);
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
        ret = CloudSyncUtils::FillAssetIdToAssets(uploadData.updData, errCode);
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
    innerProcessInfo.upLoadInfo.successCount += updateInfo.successCount;
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

int CloudSyncer::DownloadAssetsOneByOneInner(bool isSharedTable, const InnerProcessInfo &info,
    DownloadItem &downloadItem, std::map<std::string, Assets> &downloadAssets)
{
    int errCode = E_OK;
    for (auto &[col, assets] : downloadAssets) {
        Assets callDownloadAssets;
        for (auto &asset : assets) {
            std::map<std::string, Assets> tmpAssets;
            tmpAssets[col] = { asset };
            uint32_t tmpFlag = asset.flag;
            VBucket dbAssets;
            auto [tmpCode, status] = GetDBAssets(isSharedTable, info, downloadItem, dbAssets);
            if (tmpCode == -E_CLOUD_GID_MISMATCH) {
                LOGW("[CloudSyncer] skip download asset because gid mismatch");
                errCode = E_OK;
                break;
            }
            if (CloudStorageUtils::IsDataLocked(status)) {
                LOGI("[CloudSyncer] skip download asset because data lock:%u", status);
                errCode = E_OK;
                break;
            }
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
            if (tmpCode == -E_CLOUD_RECORD_EXIST_CONFLICT) {
                downloadItem.recordConflict = true;
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
        errCode = FillCloudAssets(tableName, normalAssets, failedAssets);
        if (errCode != E_OK) {
            break;
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
    int errCode = storageProxy_->GetCompensatedSyncQuery(syncQuery);
    if (errCode != E_OK) {
        LOGW("[CloudSyncer] Generate compensated sync failed by get query! errCode = %d", errCode);
        return;
    }
    if (syncQuery.empty()) {
        LOGD("[CloudSyncer] Not need generate compensated sync");
        return;
    }
    taskInfo.users.push_back("");
    for (const auto &query : syncQuery) {
        taskInfo.table.push_back(query.GetRelationTableName());
        taskInfo.queryList.push_back(query);
    }
    Sync(taskInfo);
    LOGI("[CloudSyncer] Generate compensated sync finished");
}

void CloudSyncer::ChkIgnoredProcess(InnerProcessInfo &info, const CloudSyncData &uploadData, UploadParam &uploadParam)
{
    if (uploadData.ignoredCount == 0) {
        return;
    }
    info.upLoadInfo.total -= static_cast<uint32_t>(uploadData.ignoredCount);
    if (info.upLoadInfo.successCount + info.upLoadInfo.failCount != info.upLoadInfo.total) {
        return;
    }
    if (!CloudSyncUtils::CheckCloudSyncDataEmpty(uploadData)) {
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
    int errCode = SaveCursorIfNeed(table);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] save cursor failed %d", errCode);
        return errCode;
    }
    bool isShared = false;
    errCode = storageProxy_->IsSharedTable(table, isShared);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] check shared table failed %d", errCode);
        return errCode;
    }
    // shared table not allow logic delete
    storageProxy_->SetCloudTaskConfig({ !taskInfo.priorityTask && !isShared });
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

int CloudSyncer::GetLocalInfo(size_t index, SyncParam &param, DataInfoWithLog &logInfo,
    std::map<std::string, LogInfo> &localLogInfoCache, VBucket &localAssetInfo)
{
    int errCode = storageProxy_->GetInfoByPrimaryKeyOrGid(param.tableName, param.downloadData.data[index],
        logInfo, localAssetInfo);
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        return errCode;
    }
    std::string hashKey(logInfo.logInfo.hashKey.begin(), logInfo.logInfo.hashKey.end());
    if (hashKey.empty()) {
        return errCode;
    }
    param.downloadData.existDataKey[index] = logInfo.logInfo.dataKey;
    param.downloadData.existDataHashKey[index] = logInfo.logInfo.hashKey;
    if (localLogInfoCache.find(hashKey) != localLogInfoCache.end()) {
        LOGD("[CloudSyncer] exist same record in one batch, override from cache record! hash=%.3s",
            DBCommon::TransferStringToHex(hashKey).c_str());
        logInfo.logInfo.flag = localLogInfoCache[hashKey].flag;
        logInfo.logInfo.wTimestamp = localLogInfoCache[hashKey].wTimestamp;
        logInfo.logInfo.timestamp = localLogInfoCache[hashKey].timestamp;
        logInfo.logInfo.cloudGid = localLogInfoCache[hashKey].cloudGid;
        logInfo.logInfo.device = localLogInfoCache[hashKey].device;
        logInfo.logInfo.sharingResource = localLogInfoCache[hashKey].sharingResource;
        logInfo.logInfo.status = localLogInfoCache[hashKey].status;
        // delete record should remove local asset info
        if ((localLogInfoCache[hashKey].flag & DataItem::DELETE_FLAG) == DataItem::DELETE_FLAG) {
            localAssetInfo.clear();
        }
        errCode = E_OK;
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
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Failed to batch delete, %d", errCode);
        return errCode;
    }
    innerProcessInfo.upLoadInfo.successCount += deleteInfo.successCount;
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

void CloudSyncer::DoNotifyInNeed(CloudSyncer::TaskId taskId, const std::vector<std::string> &needNotifyTables,
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

int CloudSyncer::GetUploadCountByTable(CloudSyncer::TaskId taskId, int64_t &count)
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
        if (currentContext_.isDownloadFinished[taskInfo.table[i]] == true) {
            continue;
        }
        LOGD("[CloudSyncer] try download table, index: %zu", i);
        std::string table;
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            currentContext_.tableName = taskInfo.table[i];
            table = currentContext_.tableName;
        }
        int errCode = PrepareAndDownload(table, taskInfo, isFirstDownload);
        if (errCode != E_OK) {
            return errCode;
        }
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
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            currentContext_.isDownloadFinished[taskInfo.table[i]] = true;
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

bool CloudSyncer::IsNeedLock(const UploadParam &param)
{
    return param.lockAction == LockAction::INSERT && param.mode == CloudWaterType::INSERT;
}

std::pair<int, Timestamp> CloudSyncer::GetLocalWater(const std::string &tableName, UploadParam &uploadParam)
{
    std::pair<int, Timestamp> res = { E_OK, 0u };
    if (IsNeedGetLocalWater(uploadParam.taskId)) {
        res.first = storageProxy_->GetLocalWaterMarkByMode(tableName, res.second, uploadParam.mode);
    }
    uploadParam.localMark = res.second;
    return res;
}

int CloudSyncer::HandleBatchUpload(UploadParam &uploadParam, InnerProcessInfo &info,
    CloudSyncData &uploadData, ContinueToken &continueStmtToken)
{
    int ret = E_OK;
    uint32_t batchIndex = GetCurrentTableUploadBatchIndex();
    bool isLocked = false;
    while (!CloudSyncUtils::CheckCloudSyncDataEmpty(uploadData)) {
        ret = PreProcessBatchUpload(uploadParam, info, uploadData);
        if (ret != E_OK) {
            break;
        }
        if (IsNeedLock(uploadParam) && !isLocked) {
            ret = LockCloudIfNeed(uploadParam.taskId);
            if (ret != E_OK) {
                break;
            }
            isLocked = true;
        }
        info.upLoadInfo.batchIndex = ++batchIndex;
        ret = DoBatchUpload(uploadData, uploadParam, info);
        if (ret != E_OK) {
            NotifyUploadFailed(ret, info);
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
    if (isLocked && IsNeedLock(uploadParam)) {
        UnlockIfNeed();
    }
    return ret;
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
    lastTaskId_++;
    if (lastTaskId_ == UINT64_MAX) {
        lastTaskId_ = 1u;
    }
    taskInfo.taskId = lastTaskId_;
    cloudTaskInfos_[lastTaskId_] = std::move(taskInfo);
    if (cloudTaskInfos_[lastTaskId_].priorityTask) {
        priorityTaskQueue_.push_back(lastTaskId_);
    } else {
        if (!MergeTaskInfo(cloudSchema, lastTaskId_)) {
            taskQueue_.push_back(lastTaskId_);
            LOGI("[CloudSyncer] Add task ok, taskId %" PRIu64, cloudTaskInfos_[lastTaskId_].taskId);
        }
    }
    return E_OK;
}

bool CloudSyncer::MergeTaskInfo(const std::shared_ptr<DataBaseSchema> &cloudSchema, TaskId taskId)
{
    if (!cloudTaskInfos_[taskId].merge) {
        return false;
    }
    bool isMerge = false;
    bool mergeHappen = false;
    TaskId checkTaskId = taskId;
    do {
        std::tie(isMerge, checkTaskId) = TryMergeTask(cloudSchema, checkTaskId);
        mergeHappen |= isMerge;
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
        if (taskId == runningTask || taskId == tryTaskId) {
            continue;
        }
        if (IsTaskCantMerge(taskId, tryTaskId)) {
            continue;
        }
        if (MergeTaskTablesIfConsistent(taskId, tryTaskId)) {
            beMergeTask = taskId;
            nextTryTask = tryTaskId;
            merge = true;
            break;
        }
        if (MergeTaskTablesIfConsistent(tryTaskId, taskId)) {
            beMergeTask = tryTaskId;
            nextTryTask = taskId;
            merge = true;
            break;
        }
    }
    if (!merge) {
        return res;
    }
    if (beMergeTask < nextTryTask) {
        std::tie(beMergeTask, nextTryTask) = SwapTwoTaskAndCopyTable(beMergeTask, nextTryTask);
    }
    AdjustTableBasedOnSchema(cloudSchema, cloudTaskInfos_[nextTryTask]);
    auto processNotifier = std::make_shared<ProcessNotifier>(this);
    processNotifier->Init(cloudTaskInfos_[beMergeTask].table, cloudTaskInfos_[beMergeTask].devices);
    cloudTaskInfos_[beMergeTask].errCode = -E_CLOUD_SYNC_TASK_MERGED;
    cloudTaskInfos_[beMergeTask].status = ProcessStatus::FINISHED;
    processNotifier->NotifyProcess(cloudTaskInfos_[beMergeTask], {}, true);
    cloudTaskInfos_.erase(beMergeTask);
    taskQueue_.remove(beMergeTask);
    LOGW("[CloudSyncer] TaskId %" PRIu64 " has been merged", beMergeTask);
    return res;
}

bool CloudSyncer::IsTaskCantMerge(TaskId taskId, TaskId tryTaskId)
{
    const auto &taskInfo = cloudTaskInfos_[taskId];
    const auto &tryTaskInfo = cloudTaskInfos_[tryTaskId];
    return taskInfo.compensatedTask || tryTaskInfo.compensatedTask ||
        taskInfo.priorityTask || tryTaskInfo.priorityTask ||
        !taskInfo.merge || taskInfo.devices != tryTaskInfo.devices ||
        tryTaskInfo.mode != SYNC_MODE_CLOUD_MERGE;
}

bool CloudSyncer::MergeTaskTablesIfConsistent(TaskId sourceId, TaskId targetId)
{
    const auto &source = cloudTaskInfos_[sourceId];
    const auto &target = cloudTaskInfos_[targetId];
    bool isMerge = true;
    for (const auto &table : source.table) {
        if (std::find(target.table.begin(), target.table.end(), table) == target.table.end()) {
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
        if (std::find(tmpTables.begin(), tmpTables.end(), table.name) != tmpTables.end()) {
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

int CloudSyncer::DoUploadInner(const std::string &tableName, UploadParam &uploadParam)
{
    InnerProcessInfo info = GetInnerProcessInfo(tableName, uploadParam);
    static std::vector<CloudWaterType> waterTypes = {
        CloudWaterType::DELETE, CloudWaterType::UPDATE, CloudWaterType::INSERT
    };
    for (const auto &waterType: waterTypes) {
        uploadParam.mode = waterType;
        int errCode = DoUploadByMode(tableName, uploadParam, info);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return UploadVersionRecordIfNeed(uploadParam);
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
    if (errCode != E_OK) {
        return errCode;
    }
    batchData.record = copyRecord;
    CloudSyncUtils::ModifyCloudDataTime(batchData.extend[0]);
    return storageProxy_->FillCloudLogAndAsset(isInsert ? OpType::INSERT : OpType::UPDATE, uploadData);
}

int CloudSyncer::TagUploadAssets(CloudSyncData &uploadData)
{
    if (!IsDataContainAssets()) {
        return E_OK;
    }
    std::vector<Field> assetFields;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        assetFields = currentContext_.assetFields[currentContext_.tableName];
    }

    for (size_t i = 0; i < uploadData.insData.extend.size(); i++) {
        for (const Field &assetField : assetFields) {
            (void)TagAssetsInSingleCol(assetField, uploadData.insData.record[i], true);
        }
    }
    for (size_t i = 0; i < uploadData.updData.extend.size(); i++) {
        for (const Field &assetField : assetFields) {
            (void)TagAssetsInSingleCol(assetField, uploadData.updData.record[i], false);
        }
    }
    return E_OK;
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
}