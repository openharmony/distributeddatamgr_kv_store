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
#include "relational_db_sync_interface.h"

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
    RelationalSchemaObject schemaObj;
    if (!IsRemoteSupportFieldSync() || GetDistributedSchema(schemaObj) != E_OK) {
        std::lock_guard<std::mutex> autoLock(querySyncIdMutex_);
        return querySyncId_;
    }

    std::string tableName = GetQuery().GetRelationTableName();
    std::vector<FieldInfo> fieldsInfo = schemaObj.GetSyncFieldInfo(tableName);
    std::vector<std::string> strFields(fieldsInfo.size());
    for (const auto &field : fieldsInfo) {
        strFields.push_back(field.GetFieldName());
    }
    std::sort(strFields.begin(), strFields.end());
    std::string splitFields;
    for (const auto &strField : strFields) {
        splitFields += strField;
    }
    std::string queryFieldsSyncId = DBCommon::TransferHashString(splitFields);
    std::lock_guard<std::mutex> autoLock(querySyncIdMutex_);
    return querySyncId_ + DBCommon::TransferStringToHex(queryFieldsSyncId);
}

std::string SingleVerRelationalSyncTaskContext::GetDeleteSyncId() const
{
    std::lock_guard<std::mutex> autoLock(deleteSyncIdMutex_);
    return deleteSyncId_;
}

void SingleVerRelationalSyncTaskContext::Clear()
{
    {
        std::lock_guard<std::mutex> autoLock(querySyncIdMutex_);
        querySyncId_.clear();
    }
    {
        std::lock_guard<std::mutex> autoLock(deleteSyncIdMutex_);
        deleteSyncId_.clear();
    }
    SingleVerSyncTaskContext::Clear();
}

void SingleVerRelationalSyncTaskContext::CopyTargetData(const ISyncTarget *target, const TaskParam &taskParam)
{
    SingleVerSyncTaskContext::CopyTargetData(target, taskParam);
    std::string hashTableName;
    std::string queryId;
    {
        std::lock_guard<std::mutex> autoLock(queryMutex_);
        hashTableName = DBCommon::TransferHashString(query_.GetRelationTableName());
        queryId = query_.GetIdentify();
    }
    std::string hexTableName = DBCommon::TransferStringToHex(hashTableName);
    {
        std::lock_guard<std::mutex> autoLock(querySyncIdMutex_);
        querySyncId_ = hexTableName + queryId; // save as deviceId + hexTableName + queryId
    }
    {
        std::lock_guard<std::mutex> autoLock(deleteSyncIdMutex_);
        deleteSyncId_ = GetDeviceId() + hexTableName; // save as deviceId + hexTableName
    }
}

void SingleVerRelationalSyncTaskContext::SetRelationalSyncStrategy(const RelationalSyncStrategy &strategy,
    bool isSchemaSync)
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
    RelationalSyncStrategy strategy;
    SetRelationalSyncStrategy(strategy, false);
    SyncTaskContext::SchemaChange();
}

bool SingleVerRelationalSyncTaskContext::IsSchemaCompatible() const
{
    std::lock_guard<std::mutex> autoLock(syncStrategyMutex_);
    for (const auto &strategy : relationalSyncStrategy_) {
        if (!strategy.second.permitSync) {
            return false;
        }
    }
    return true;
}

bool SingleVerRelationalSyncTaskContext::IsRemoteSupportFieldSync() const
{
    return GetRemoteSoftwareVersion() > SOFTWARE_VERSION_RELEASE_10_0;
}

int SingleVerRelationalSyncTaskContext::GetDistributedSchema(RelationalSchemaObject &schemaObj) const
{
    auto *relationalDbSyncInterface = static_cast<RelationalDBSyncInterface*>(syncInterface_);
    if (SyncTaskContext::GetMode() == SyncModeType::RESPONSE_PULL) {
        return relationalDbSyncInterface->GetRemoteDeviceSchema(GetDeviceId(), schemaObj);
    }
    schemaObj = relationalDbSyncInterface->GetSchemaInfo();
    return E_OK;
}
}
#endif