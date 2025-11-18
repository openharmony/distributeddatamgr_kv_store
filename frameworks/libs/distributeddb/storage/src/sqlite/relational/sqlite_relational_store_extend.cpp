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
#ifdef RELATIONAL_STORE
#include "sqlite_relational_store.h"

namespace DistributedDB {
void SQLiteRelationalStore::StopAllBackgroundTask()
{
#ifdef USE_DISTRIBUTEDDB_CLOUD
    if (sqliteStorageEngine_ == nullptr) {
        LOGW("[RelationalStore] Storage engine was not initialized when stop all background task");
    } else {
        sqliteStorageEngine_->StopGenLogTask();
    }
    if (cloudSyncer_ == nullptr) {
        LOGW("[RelationalStore] cloudSyncer was not initialized when stop all background task");
    } else {
        (void) cloudSyncer_->StopSyncTask(nullptr);
    }
#endif
}

#ifdef USE_DISTRIBUTEDDB_CLOUD
int SQLiteRelationalStore::StopGenLogTask(const std::vector<std::string> &tableList)
{
    if (sqliteStorageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    return sqliteStorageEngine_->StopGenLogTaskWithTables(tableList);
}

SyncProcess SQLiteRelationalStore::GetCloudTaskStatus(uint64_t taskId) const
{
    return cloudSyncer_->GetCloudTaskStatus(taskId);
}
#endif
} // namespace DistributedDB
#endif