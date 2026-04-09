/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "res_finalizer.h"
#include "runtime_context.h"
#include "store_types.h"
#include "version.h"

namespace DistributedDB {
namespace {
    constexpr const int MAX_EXPIRED_CURSOR_COUNT = 1;
    constexpr const uint64_t MAX_DOWNLOAD_LOOP_TIMES = 10000;
    constexpr const uint64_t WARNING_DOWNLOAD_PERIOD = 100;
}
int CloudSyncer::HandleDownloadResultForAsyncDownload(const DownloadItem &downloadItem, InnerProcessInfo &info,
    DownloadCommitList &commitList, uint32_t &successCount)
{
    int errCode = storageProxy_->StartTransaction(TransactType::IMMEDIATE, true);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] start transaction Failed before handle async download.");
        return errCode;
    }
    errCode = CommitDownloadAssetsForAsyncDownload(downloadItem, info, commitList, successCount);
    if (errCode != E_OK) {
        successCount = 0;
        int ret = E_OK;
        if (errCode == -E_REMOVE_ASSETS_FAILED) {
            // remove assets failed no effect to asset status, just commit
            ret = storageProxy_->Commit(true);
            LOGE("[CloudSyncer] commit async download assets failed %d commit ret %d", errCode, ret);
        } else {
            ret = storageProxy_->Rollback(true);
            LOGE("[CloudSyncer] commit async download assets failed %d rollback ret %d", errCode, ret);
        }
        return errCode;
    }
    errCode = storageProxy_->Commit(true);
    if (errCode != E_OK) {
        successCount = 0;
        LOGE("[CloudSyncer] commit async download assets failed %d", errCode);
    }
    return errCode;
}

void CloudSyncer::TriggerAsyncDownloadAssetsIfNeed()
{
    if (!storageProxy_->IsExistTableContainAssets()) {
        LOGD("[CloudSyncer] No exist table contain assets, skip async download asset check");
        return;
    }
    TaskId taskId = INVALID_TASK_ID;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (asyncTaskId_ != INVALID_TASK_ID || closed_) {
            LOGI("[CloudSyncer] No need generate async task now asyncTaskId %" PRIu64 " closed %d",
                asyncTaskId_, static_cast<int>(closed_));
            return;
        }
        lastTaskId_--;
        if (lastTaskId_ == INVALID_TASK_ID) {
            lastTaskId_ = UINT64_MAX;
        }
        taskId = lastTaskId_;
        IncObjRef(this);
    }
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([taskId, this]() {
        LOGI("[CloudSyncer] Exec asyncTaskId %" PRIu64 " begin", taskId);
        ExecuteAsyncDownloadAssets(taskId);
        LOGI("[CloudSyncer] Exec asyncTaskId %" PRIu64 " finished", taskId);
        scheduleTaskCount_--;
        DecObjRef(this);
    });
    if (errCode == E_OK) {
        LOGI("[CloudSyncer] Schedule asyncTaskId %" PRIu64 " success", taskId);
        scheduleTaskCount_++;
    } else {
        LOGW("[CloudSyncer] Schedule BackgroundDownloadAssetsTask failed %d", errCode);
        DecObjRef(this);
    }
}

void CloudSyncer::ExecuteAsyncDownloadAssets(TaskId taskId)
{
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (asyncTaskId_ != INVALID_TASK_ID || closed_) {
            LOGI("[CloudSyncer] No need exec async task now asyncTaskId %" PRIu64 " closed %d",
                asyncTaskId_, static_cast<int>(closed_));
            return;
        }
        asyncTaskId_ = taskId;
    }
    BackgroundDownloadAssetsTask();
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        asyncTaskId_ = INVALID_TASK_ID;
    }
    asyncTaskCv_.notify_all();
}

TaskId CloudSyncer::GetCurrentTaskId()
{
    std::lock_guard<std::mutex> guard(dataLock_);
    return currentContext_.currentTaskId;
}

int32_t CloudSyncer::GetHeartbeatCount(TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(heartbeatMutex_);
    return heartbeatCount_[taskId];
}

void CloudSyncer::RemoveHeartbeatData(TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(heartbeatMutex_);
    heartbeatCount_.erase(taskId);
    failedHeartbeatCount_.erase(taskId);
}

void CloudSyncer::ExecuteHeartBeatTask(TaskId taskId)
{
    if (GetCurrentTaskId() != taskId) {
        RemoveHeartbeatData(taskId);
        DecObjRef(this);
        return;
    }
    if (GetHeartbeatCount(taskId) >= HEARTBEAT_PERIOD) {
        // heartbeat block twice should finish task now
        SetTaskFailed(taskId, -E_CLOUD_ERROR);
    } else {
        int ret = cloudDB_.HeartBeat();
        if (ret != E_OK) {
            HeartBeatFailed(taskId, ret);
        } else {
            std::lock_guard<std::mutex> autoLock(heartbeatMutex_);
            failedHeartbeatCount_[taskId] = 0;
        }
    }
    {
        std::lock_guard<std::mutex> autoLock(heartbeatMutex_);
        heartbeatCount_[taskId]--;
    }
    if (GetCurrentTaskId() != taskId) {
        RemoveHeartbeatData(taskId);
    }
    DecObjRef(this);
}

void CloudSyncer::SetCloudConflictHandler(const std::shared_ptr<ICloudConflictHandler> &handler)
{
    cloudDB_.SetCloudConflictHandler(handler);
}

int CloudSyncer::WaitAsyncGenLogTaskFinished(TaskId triggerTaskId)
{
    if (storageProxy_ == nullptr) {
        LOGE("[WaitAsyncGenLogTaskFinished] Invalid storage.");
        return -E_INVALID_DB;
    }
    std::vector<std::string> tables;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (cloudTaskInfos_.find(triggerTaskId) == cloudTaskInfos_.end()) {
            LOGW("[WaitAsyncGenLogTaskFinished] Abort wait because of invalid task id");
            return -E_INVALID_DB;
        }
        tables = cloudTaskInfos_[triggerTaskId].table;
    }
    return storageProxy_->WaitAsyncGenLogTaskFinished(tables);
}

void CloudSyncer::RetainCurrentTaskInfo(TaskId taskId)
{
    std::multimap<int, TaskId, std::greater<int>> retainQueue;
    for (const auto &kv : std::as_const(taskQueue_)) {
        if (kv.second == taskId) {
            retainQueue.emplace(kv.first, kv.second);
            break;
        }
    }
    taskQueue_ = std::move(retainQueue);
    // clear task info and retain current taskinfo
    auto cloudTaskIter = cloudTaskInfos_.find(taskId);
    if (cloudTaskIter != cloudTaskInfos_.end()) {
        const CloudTaskInfo cloudTaskInfo = cloudTaskIter->second;
        cloudTaskInfos_.clear();
        cloudTaskInfos_.emplace(taskId, cloudTaskInfo);
    } else {
        cloudTaskInfos_.clear();
    }
    auto resumeTaskIter = resumeTaskInfos_.find(taskId);
    if (resumeTaskIter != resumeTaskInfos_.end()) {
        const ResumeTaskInfo resumeTaskInfo = resumeTaskIter->second;
        resumeTaskInfos_.clear();
        resumeTaskInfos_.emplace(taskId, resumeTaskInfo);
    } else {
        resumeTaskInfos_.clear();
    }
}

void CloudSyncer::SetCurrentTmpError(int errCode)
{
    std::lock_guard<std::mutex> guard(dataLock_);
    if (cloudTaskInfos_.find(currentContext_.currentTaskId) == cloudTaskInfos_.end()) {
        return;
    }
    cloudTaskInfos_[currentContext_.currentTaskId].errCode = errCode;
    cloudTaskInfos_[currentContext_.currentTaskId].tempErrCode = errCode;
}

int CloudSyncer::DoDownloadInner(CloudSyncer::TaskId taskId, SyncParam &param, bool isFirstDownload)
{
    // Query data by batch until reaching end and not more data need to be download
    int ret = PreCheck(taskId, param.info.tableName);
    if (ret != E_OK) {
        return ret;
    }
    int expiredCursorCount = 0;
    uint64_t loopCount = 0;
    do {
        ret = DownloadOneBatch(taskId, param, isFirstDownload);
        if (ret == -E_EXPIRED_CURSOR) {
            expiredCursorCount++;
            if (expiredCursorCount > MAX_EXPIRED_CURSOR_COUNT) {
                LOGE("[CloudSyncer] Table[%s] too much expired cursor count[%d]",
                    DBCommon::StringMiddleMasking(param.info.tableName).c_str(), expiredCursorCount);
                return ret;
            }
            param.isLastBatch = false;
            ret = DoUpdateExpiredCursor(taskId, param.info.tableName, param.cloudWaterMark);
        }
        if (ret != E_OK) {
            return ret;
        }
        loopCount++;
        if (loopCount > MAX_DOWNLOAD_LOOP_TIMES && (loopCount % WARNING_DOWNLOAD_PERIOD == 0)) {
            LOGW("[CloudSyncer] Table[%s] download too much times, current[%" PRIu64 "]",
                DBCommon::StringMiddleMasking(param.info.tableName).c_str(), loopCount);
        }
    } while (!param.isLastBatch);
    return E_OK;
}

int CloudSyncer::DoUpdateExpiredCursor(TaskId taskId, const std::string &table, std::string &newCursor)
{
    LOGI("[CloudSyncer] Update expired cursor now, table[%s]", DBCommon::StringMiddleMasking(table).c_str());
    if (storageProxy_ == nullptr) {
        LOGE("[CloudSyncer] storage is nullptr when update expired cursor");
        return -E_INTERNAL_ERROR;
    }
    SyncParam param;
    param.tableName = table;
    if (isKvScene_) {
        return UpdateCloudMarkAndCleanExpiredCursor(param, newCursor);
    }
    auto errCode = storageProxy_->GetCloudGidCursor(param.tableName, param.cloudWaterMark);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = DoUpdatePotentialCursorIfNeed(param.tableName);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = DoQueryAllGid(taskId, std::move(param));
    if (errCode != E_OK) {
        return errCode;
    }
    return UpdateCloudMarkAndCleanExpiredCursor(param, newCursor);
}

int CloudSyncer::DoQueryAllGid(TaskId taskId, SyncParam &&param)
{
    uint64_t count = 0;
    int errCode = storageProxy_->GetGidRecordCount(param.tableName, count);
    if (errCode != E_OK) {
        return errCode;
    }
    int retryCount = 0;
    do {
        if (count == 0) {
            LOGI("[CloudSyncer] Skip query[%s] all gid by not exists gid record",
                DBCommon::StringMiddleMasking(param.tableName).c_str());
            break;
        }
        param.downloadData.data.clear();
        int ret = DownloadOneBatchGID(taskId, param);
        if (ret == -E_EXPIRED_CURSOR && retryCount < MAX_EXPIRED_CURSOR_COUNT) {
            retryCount++;
            param.cloudWaterMark = "";
            errCode = DropTempTable(param.tableName);
            if (errCode != E_OK) {
                LOGE("[CloudSyncer] drop temp table failed after download gid: %d", errCode);
                return errCode;
            }
            errCode = SaveGIDCursor(param);
            if (errCode != E_OK) {
                LOGE("[CloudSyncer] save gid cursor failed after download gid: %d", errCode);
                return errCode;
            }
            continue;
        }
        if (ret != E_OK) {
            return ret;
        }
    } while (!param.isLastBatch);
    if (count == 0) {
        errCode = DropTempTable(param.tableName);
    } else {
        std::pair<bool, bool> isNeedDeleted = {IsModeForcePush(taskId), IsModeForcePull(taskId)};
        errCode = storageProxy_->DeleteCloudNoneExistRecord(param.tableName, isNeedDeleted);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return errCode;
}

int CloudSyncer::DoUpdatePotentialCursorIfNeed(const std::string &table)
{
    std::string backupCursor;
    auto errCode = storageProxy_->GetBackupCloudCursor(table, backupCursor);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!backupCursor.empty()) {
        LOGI("[CloudSyncer] Table[%s] already exist backup cursor", DBCommon::StringMiddleMasking(table).c_str());
        return E_OK;
    }
    std::tie(errCode, backupCursor) = cloudDB_.GetEmptyCursor(table);
    if (errCode != E_OK) {
        return errCode;
    }
    return storageProxy_->PutBackupCloudCursor(table, backupCursor);
}

int CloudSyncer::DownloadOneBatchGID(TaskId taskId, SyncParam &param)
{
    int ret = CheckTaskIdValid(taskId);
    if (ret != E_OK) {
        return ret;
    }
    ret = DownloadGIDFromCloud(param);
    if (ret != E_OK) {
        return ret;
    }
    ret = SaveGIDRecord(param);
    if (ret != E_OK) {
        return ret;
    }
    return SaveGIDCursor(param);
}

int CloudSyncer::UpdateCloudMarkAndCleanExpiredCursor(SyncParam &param, std::string &newCursor)
{
    int errCode = storageProxy_->GetBackupCloudCursor(param.tableName, newCursor);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = storageProxy_->CleanCloudInfo(param.tableName);
    if (errCode != E_OK) {
        return errCode;
    }
    return storageProxy_->SetCloudWaterMark(param.tableName, newCursor);
}

int CloudSyncer::DownloadGIDFromCloud(SyncParam &param)
{
    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = param.cloudWaterMark;
    int errCode = cloudDB_.QueryAllGid(param.tableName, extend, param.downloadData.data);
    if (errCode == -E_QUERY_END) {
        errCode = E_OK;
        param.isLastBatch = true;
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Query cloud gid failed[%d]", errCode);
    } else if (!param.downloadData.data.empty()) {
        const auto &record = param.downloadData.data[param.downloadData.data.size() - 1u];
        auto iter = record.find(CloudDbConstant::CURSOR_FIELD);
        if (iter == record.end()) {
            LOGE("[CloudSyncer] Cloud gid record no exist cursor");
            return -E_CLOUD_ERROR;
        }
        auto cursor = std::get_if<std::string>(&iter->second);
        if (cursor == nullptr) {
            LOGE("[CloudSyncer] Cloud gid record cursor is no str, type[%zu]", iter->second.index());
            return -E_CLOUD_ERROR;
        }
        if (cursor->size() > static_cast<size_t>(INT32_MAX)) {
            LOGE("[CloudSyncer] Cloud gid record cursor len over max limit, size[%zu]", cursor->size());
            return -E_CLOUD_ERROR;
        }
        param.cloudWaterMark = *cursor;
    }
    return errCode;
}

int CloudSyncer::SaveGIDRecord(SyncParam &param)
{
    return storageProxy_->PutCloudGid(param.tableName, param.downloadData.data);
}

int CloudSyncer::SaveGIDCursor(SyncParam &param)
{
    return storageProxy_->PutCloudGidCursor(param.tableName, param.cloudWaterMark);
}

int CloudSyncer::DropTempTable(const std::string &tableName)
{
    return storageProxy_->DropTempTable(tableName);
}

int CloudSyncer::GetTableNameAndCheck(const std::string &tableName, std::string &table)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (currentContext_.processRecorder == nullptr) {
        LOGE("[CloudSyncer] process recorder of current context is nullptr.");
        return -E_INTERNAL_ERROR;
    }
    if (currentContext_.processRecorder->IsDownloadFinish(currentContext_.currentUserIndex, tableName)) {
        return -E_FINISHED;
    }
    LOGI("[CloudSyncer] try download table, %s", DBCommon::StringMiddleMaskingWithLen(tableName).c_str());
    currentContext_.tableName = tableName;
    table = currentContext_.tableName;
    return E_OK;
}

int CloudSyncer::DoFullDownloadAssetsInSync()
{
    auto manager = RuntimeContext::GetInstance()->GetAssetsDownloadManager();
    auto [errCode, listener] = manager->BeginDownloadWithListener(nullptr, nullptr);
    if (errCode != E_OK) {
        LOGW("[CloudSyncer] BeginDownloadWithListener failed in sync, errCode=%d", errCode);
        return errCode;
    }
    ResFinalizer finalizer([manager]() {
        manager->FinishDownload();
    });
    errCode = DoBackgroundDownloadAssets(false);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] do full download assets in sync failed, %d", errCode);
        return errCode;
    }
    return E_OK;
}

int CloudSyncer::StopSyncTask(const std::function<int(void)> &removeFunc, int errCode)
{
    if (errCode == -E_TASK_INTERRUPTED) {
        int ret = cloudDB_.StopCloudSync();
        if (ret != E_OK) {
            LOGE("[CloudSyncer] stop cloud sync failed errCode: %d.", ret);
            return ret;
        }
    }
    hasKvRemoveTask = true;
    CloudSyncer::TaskId currentTask;
    CloudSyncer::TaskId asyncCurrentTask;
    {
        // stop task if exist
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentTask = currentContext_.currentTaskId;
        asyncCurrentTask = asyncTaskId_;
    }
    if (currentTask != INVALID_TASK_ID || asyncCurrentTask != INVALID_TASK_ID) {
        StopAllTasks(errCode);
    }
    errCode = E_OK;
    if (removeFunc != nullptr) {
        std::lock_guard<std::mutex> lock(syncMutex_);
        errCode = removeFunc();
    }
    hasKvRemoveTask = false;
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] removeFunc execute failed errCode: %d.", errCode);
        return errCode;
    }

    return errCode;
}

bool CloudSyncer::IsCurrentSkipDownloadAssets()
{
    return storageProxy_ == nullptr ? false : storageProxy_->IsSkipDownloadAssets();
}

int CloudSyncer::TagDownloadAssetsWithPolicy(const Key &hashKey, size_t idx, SyncParam &param,
    const DataInfo &dataInfo, VBucket &localAssetInfo)
{
    AssetConflictPolicy policy = GetAssetConflictPolicy();
    AssetRecordInfo info = {IsCurrentSkipDownloadAssets(), idx, hashKey, dataInfo, GetCurrentAssetFields()};
    switch (policy) {
        case AssetConflictPolicy::CONFLICT_POLICY_TIME_FIRST:
            return CloudSyncTagAssets::TagDownloadAssetsTimeFirst(info, param, localAssetInfo);
        case AssetConflictPolicy::CONFLICT_POLICY_TEMP_PATH:
            return CloudSyncTagAssets::TagDownloadAssetsTempPath(info, param, localAssetInfo);
        case AssetConflictPolicy::CONFLICT_POLICY_DEFAULT:
        default:
            return TagDownloadAssets(hashKey, idx, param, dataInfo, localAssetInfo);
    };
}

std::vector<Field> CloudSyncer::GetCurrentAssetFields()
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return currentContext_.assetFields[currentContext_.tableName];
}

AssetConflictPolicy CloudSyncer::GetAssetConflictPolicy()
{
    return storageProxy_ == nullptr ? AssetConflictPolicy::CONFLICT_POLICY_DEFAULT :
        storageProxy_->GetAssetConflictPolicy();
}

int CloudSyncer::UpdateAssetStatus(const std::string &table, std::vector<VBucket> &assetInfo)
{
    for (auto &item : assetInfo) {
        auto ret = storageProxy_->UpdateAssetStatusForAssetOnly(table, item);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Cannot save asset data due to error code %d", ret);
            return ret;
        }
    }
    return E_OK;
}
} // namespace DistributedDB