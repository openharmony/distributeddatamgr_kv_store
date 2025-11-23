/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

void CloudSyncer::SetCloudConflictHandler(const std::shared_ptr<ICloudConflictHandler> &handler)
{
    cloudDB_.SetCloudConflictHandler(handler);
}
} // namespace DistributedDB
