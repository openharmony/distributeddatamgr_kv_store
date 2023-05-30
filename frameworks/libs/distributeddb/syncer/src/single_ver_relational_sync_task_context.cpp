/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "single_ver_relational_sync_task_context.h"
#include "db_common.h"

#ifdef RELATIONAL_STORE
namespace DistributedDB {
SingleVerRelationalSyncTaskContext::SingleVerRelationalSyncTaskContext()
    : SingleVerSyncTaskContext()
{}

SingleVerRelationalSyncTaskContext::~SingleVerRelationalSyncTaskContext()
{
}

std::string SingleVerRelationalSyncTaskContext::GetQuerySyncId() const
{
    return querySyncId_;
}

std::string SingleVerRelationalSyncTaskContext::GetDeleteSyncId() const
{
    return deleteSyncId_;
}

void SingleVerRelationalSyncTaskContext::Clear()
{
    querySyncId_.clear();
    deleteSyncId_.clear();
    SingleVerSyncTaskContext::Clear();
}

void SingleVerRelationalSyncTaskContext::CopyTargetData(const ISyncTarget *target, const TaskParam &taskParam)
{
    SingleVerSyncTaskContext::CopyTargetData(target, taskParam);
    std::string hashTableName = DBCommon::TransferHashString(query_.GetRelationTableName());
    std::string hexTableName = DBCommon::TransferStringToHex(hashTableName);
    querySyncId_ = hexTableName + query_.GetIdentify(); // save as deviceId + hexTableName + queryId
    deleteSyncId_ = GetDeviceId() + hexTableName; // save as deviceId + hexTableName
}

void SingleVerRelationalSyncTaskContext::SetRelationalSyncStrategy(RelationalSyncStrategy &strategy, bool isSchemaSync)
{
    std::lock_guard<std::mutex> autoLock(syncStrategyMutex_);
    relationalSyncStrategy_ = strategy;
    isSchemaSync_ = isSchemaSync;
}

std::pair<bool, bool> SingleVerRelationalSyncTaskContext::GetSchemaSyncStatus(QuerySyncObject &querySyncObject) const
{
    std::lock_guard<std::mutex> autoLock(syncStrategyMutex_);
    auto it = relationalSyncStrategy_.find(querySyncObject.GetRelationTableName());
    if (it == relationalSyncStrategy_.end()) {
        return {false, isSchemaSync_};
    }
    return {it->second.permitSync, isSchemaSync_};
}

void SingleVerRelationalSyncTaskContext::SchemaChange()
{
    SetIsNeedResetAbilitySync(true);
    RelationalSyncStrategy strategy;
    SetRelationalSyncStrategy(strategy, false);
}
}
#endif