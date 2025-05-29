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
#include "runtime_context.h"
#include "store_types.h"
#include "strategy_factory.h"
#include "version.h"

namespace DistributedDB {
int CloudSyncer::StopTaskBeforeSetReference(std::function<int(void)> &setReferenceFunc)
{
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
        errCode = setReferenceFunc();
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] setReferenceFunc execute failed errCode: %d.", errCode);
    }
    return errCode;
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
} // namespace DistributedDB
