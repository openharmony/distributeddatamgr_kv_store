/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "single_ver_relational_syncer.h"
#ifdef RELATIONAL_STORE
#include "db_common.h"
#include "relational_db_sync_interface.h"
#include "single_ver_sync_engine.h"

namespace DistributedDB {
int SingleVerRelationalSyncer::Initialize(ISyncInterface *syncInterface, bool isNeedActive)
{
    int errCode = SingleVerSyncer::Initialize(syncInterface, isNeedActive);
    if (errCode != E_OK) {
        return errCode;
    }
    auto callback = [this] { SchemaChangeCallback(); };
    return static_cast<RelationalDBSyncInterface *>(syncInterface)->
        RegisterSchemaChangedCallback(callback);
}

int SingleVerRelationalSyncer::Sync(const SyncParma &param, uint64_t connectionId)
{
    int errCode = QuerySyncPreCheck(param);
    if (errCode != E_OK) {
        return errCode;
    }
    return GenericSyncer::Sync(param, connectionId);
}

int SingleVerRelationalSyncer::PrepareSync(const SyncParma &param, uint32_t syncId, uint64_t connectionId)
{
    const auto &syncInterface = static_cast<RelationalDBSyncInterface *>(syncInterface_);
    std::vector<QuerySyncObject> tablesQuery;
    if (param.isQuerySync) {
        tablesQuery = GetQuerySyncObject(param);
    } else {
        tablesQuery = syncInterface->GetTablesQuery();
    }
    std::set<uint32_t> subSyncIdSet;
    int errCode = GenerateEachSyncTask(param, syncId, tablesQuery, connectionId, subSyncIdSet);
    if (errCode != E_OK) {
        DoRollBack(subSyncIdSet);
        return errCode;
    }
    if (param.wait) {
        bool connectionClose = false;
        {
            std::lock_guard<std::mutex> lockGuard(syncIdLock_);
            connectionClose = connectionIdMap_.find(connectionId) == connectionIdMap_.end();
        }
        if (!connectionClose) {
            DoOnComplete(param, syncId);
        }
    }
    return E_OK;
}

int SingleVerRelationalSyncer::GenerateEachSyncTask(const SyncParma &param, uint32_t syncId,
    const std::vector<QuerySyncObject> &tablesQuery, uint64_t connectionId, std::set<uint32_t> &subSyncIdSet)
{
    SyncParma subParam = param;
    subParam.isQuerySync = true;
    int errCode = E_OK;
    for (const QuerySyncObject &table : tablesQuery) {
        uint32_t subSyncId = GenerateSyncId();
        std::string hashTableName = DBCommon::TransferHashString(table.GetRelationTableName());
        LOGI("[SingleVerRelationalSyncer] SubSyncId %" PRIu32 " create by SyncId %" PRIu32 ", hashTableName = %s",
            subSyncId, syncId, STR_MASK(DBCommon::TransferStringToHex(hashTableName)));
        subParam.syncQuery = table;
        subParam.onComplete = [this, subSyncId, syncId, subParam](const std::map<std::string, int> &devicesMap) {
            DoOnSubSyncComplete(subSyncId, syncId, subParam, devicesMap);
        };
        {
            std::lock_guard<std::mutex> lockGuard(syncMapLock_);
            fullSyncIdMap_[syncId].insert(subSyncId);
        }
        errCode = GenericSyncer::PrepareSync(subParam, subSyncId, connectionId);
        if (errCode != E_OK) {
            LOGW("[SingleVerRelationalSyncer] PrepareSync failed errCode:%d", errCode);
            std::lock_guard<std::mutex> lockGuard(syncMapLock_);
            fullSyncIdMap_[syncId].erase(subSyncId);
            break;
        }
        subSyncIdSet.insert(subSyncId);
    }
    return errCode;
}

void SingleVerRelationalSyncer::DoOnSubSyncComplete(const uint32_t subSyncId, const uint32_t syncId,
    const SyncParma &param, const std::map<std::string, int> &devicesMap)
{
    bool allFinish = true;
    {
        std::lock_guard<std::mutex> lockGuard(syncMapLock_);
        fullSyncIdMap_[syncId].erase(subSyncId);
        allFinish = fullSyncIdMap_[syncId].empty();
        TableStatus tableStatus;
        tableStatus.tableName = param.syncQuery.GetRelationTableName();
        for (const auto &item : devicesMap) {
            tableStatus.status = static_cast<DBStatus>(item.second);
            resMap_[syncId][item.first].push_back(tableStatus);
        }
    }
    // block sync do callback in sync function
    if (allFinish && !param.wait) {
        DoOnComplete(param, syncId);
    }
}

void SingleVerRelationalSyncer::DoRollBack(std::set<uint32_t> &subSyncIdSet)
{
    for (const auto &removeId : subSyncIdSet) {
        int retCode = RemoveSyncOperation(static_cast<int>(removeId));
        if (retCode != E_OK) {
            LOGW("[SingleVerRelationalSyncer] RemoveSyncOperation failed errCode:%d, syncId:%d", retCode, removeId);
        }
    }
}

void SingleVerRelationalSyncer::DoOnComplete(const SyncParma &param, uint32_t syncId)
{
    if (!param.relationOnComplete) {
        return;
    }
    std::map<std::string, std::vector<TableStatus>> syncRes;
    std::map<std::string, std::vector<TableStatus>> tmpMap;
    {
        std::lock_guard<std::mutex> lockGuard(syncMapLock_);
        tmpMap = resMap_[syncId];
    }
    for (const auto &devicesRes : tmpMap) {
        for (const auto &tableRes : devicesRes.second) {
            syncRes[devicesRes.first].push_back(
                {tableRes.tableName, static_cast<DBStatus>(tableRes.status)});
        }
    }
    param.relationOnComplete(syncRes);
    {
        std::lock_guard<std::mutex> lockGuard(syncMapLock_);
        resMap_.erase(syncId);
        fullSyncIdMap_.erase(syncId);
    }
}

void SingleVerRelationalSyncer::EnableAutoSync(bool enable)
{
    (void)enable;
}

void SingleVerRelationalSyncer::LocalDataChanged(int notifyEvent)
{
    (void)notifyEvent;
}

void SingleVerRelationalSyncer::SchemaChangeCallback()
{
    if (syncEngine_ == nullptr) {
        return;
    }
    syncEngine_->SchemaChange();
    int errCode = UpgradeSchemaVerInMeta();
    if (errCode != E_OK) {
        LOGE("[SingleVerRelationalSyncer] upgrade schema version in meta failed:%d", errCode);
    }
}

int SingleVerRelationalSyncer::SyncConditionCheck(const SyncParma &param, const ISyncEngine *engine,
    ISyncInterface *storage) const
{
    if (!param.isQuerySync) {
        return E_OK;
    }
    auto queryList = GetQuerySyncObject(param);
    const RelationalSchemaObject schemaObj = static_cast<RelationalDBSyncInterface *>(storage)->GetSchemaInfo();
    const std::vector<DistributedTable> &sTable = schemaObj.GetDistributedSchema().tables;
    if (schemaObj.GetTableMode() == DistributedTableMode::COLLABORATION && sTable.empty()) {
        LOGE("[SingleVerRelationalSyncer] Distributed schema not set in COLLABORATION mode");
        return -E_SCHEMA_MISMATCH;
    }
    for (auto &item : queryList) {
        int errCode = static_cast<RelationalDBSyncInterface *>(storage)->CheckAndInitQueryCondition(item);
        if (errCode != E_OK) {
            LOGE("[SingleVerRelationalSyncer] table %s[length: %zu] query check failed %d",
                DBCommon::StringMiddleMasking(item.GetTableName()).c_str(), item.GetTableName().size(), errCode);
            return errCode;
        }
        if (schemaObj.GetTableMode() == DistributedTableMode::COLLABORATION) {
            auto iter = std::find_if(sTable.begin(), sTable.end(), [&item](const DistributedTable &table) {
                return DBCommon::ToLowerCase(table.tableName) == DBCommon::ToLowerCase(item.GetTableName());
            });
            if (iter == sTable.end()) {
                LOGE("[SingleVerRelationalSyncer] table %s[length: %zu] mismatch distributed schema",
                    DBCommon::StringMiddleMasking(item.GetTableName()).c_str(), item.GetTableName().size());
                return -E_SCHEMA_MISMATCH;
            }
        }
    }
    if (param.mode == SUBSCRIBE_QUERY) {
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

int SingleVerRelationalSyncer::QuerySyncPreCheck(const SyncParma &param) const
{
    if (!param.isQuerySync) {
        return E_OK;
    }
    if (param.mode == SYNC_MODE_PUSH_PULL) {
        LOGE("[SingleVerRelationalSyncer] sync with not support push_pull mode");
        return -E_NOT_SUPPORT;
    }
    if (param.syncQuery.IsUseFromTables() && param.syncQuery.GetRelationTableNames().empty()) {
        LOGE("[SingleVerRelationalSyncer] sync with from table but no table found");
        return -E_INVALID_ARGS;
    }
    if (!param.syncQuery.IsUseFromTables() && param.syncQuery.GetRelationTableName().empty()) {
        LOGE("[SingleVerRelationalSyncer] sync with empty table");
        return -E_NOT_SUPPORT;
    }
    return E_OK;
}

std::vector<QuerySyncObject> SingleVerRelationalSyncer::GetQuerySyncObject(const SyncParma &param)
{
    std::vector<QuerySyncObject> res;
    auto tables = param.syncQuery.GetRelationTableNames();
    if (!tables.empty()) {
        for (const auto &it : tables) {
            res.emplace_back(Query::Select(it));
        }
    } else {
        res.push_back(param.syncQuery);
    }
    return res;
}
}
#endif