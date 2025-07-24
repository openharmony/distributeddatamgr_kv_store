/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "relational_sync_able_storage.h"

#include <utility>

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "concurrent_adapter.h"
#include "data_compression.h"
#include "db_common.h"
#include "db_dfx_adapter.h"
#include "generic_single_ver_kv_entry.h"
#include "platform_specific.h"
#include "query_utils.h"
#include "relational_remote_query_continue_token.h"
#include "relational_sync_data_inserter.h"
#include "res_finalizer.h"
#include "runtime_context.h"
#include "time_helper.h"

namespace DistributedDB {
int RelationalSyncAbleStorage::MarkFlagAsAssetAsyncDownload(const std::string &tableName,
    const DownloadData &downloadData, const std::set<std::string> &gidFilters)
{
    if (transactionHandle_ == nullptr) {
        LOGE("[RelationalSyncAbleStorage] the transaction has not been started, tableName:%s, length:%zu",
            DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
        return -E_INVALID_DB;
    }
    int errCode = transactionHandle_->MarkFlagAsAssetAsyncDownload(tableName, downloadData, gidFilters);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncAbleStorage] mark flag as asset async download failed.%d, tableName:%s, length:%zu",
            errCode, DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
    }
    return errCode;
}

std::pair<int, std::vector<std::string>> RelationalSyncAbleStorage::GetDownloadAssetTable()
{
    int errCode = E_OK;
    auto *handle = GetHandle(false, errCode);
    if (handle == nullptr || errCode != E_OK) {
        LOGE("[RelationalSyncAbleStorage] Get handle failed when get downloading asset table: %d", errCode);
        return {errCode, {}};
    }
    std::vector<std::string> tableNames;
    auto allTableNames = storageEngine_->GetSchema().GetTableNames();
    for (const auto &it : allTableNames) {
        int32_t count = 0;
        errCode = handle->GetDownloadingCount(it, count);
        if (errCode != E_OK) {
            LOGE("[RelationalSyncAbleStorage] Get downloading asset count failed: %d", errCode);
            ReleaseHandle(handle);
            return {errCode, tableNames};
        }
        if (count > 0) {
            tableNames.push_back(it);
        }
    }
    ReleaseHandle(handle);
    return {errCode, tableNames};
}

std::pair<int, std::vector<std::string>> RelationalSyncAbleStorage::GetDownloadAssetRecords(
    const std::string &tableName, int64_t beginTime)
{
    TableSchema schema;
    int errCode = GetCloudTableSchema(tableName, schema);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncAbleStorage] Get schema when get asset records failed:%d, tableName:%s, length:%zu",
            errCode, DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
        return {errCode, {}};
    }
    auto *handle = GetHandle(false, errCode);
    if (handle == nullptr || errCode != E_OK) {
        LOGE("[RelationalSyncAbleStorage] Get handle when get asset records failed:%d, tableName:%s, length:%zu",
            errCode, DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
        return {errCode, {}};
    }
    std::vector<std::string> gids;
    errCode = handle->GetDownloadAssetRecordsInner(schema, beginTime, gids);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncAbleStorage] Get downloading asset records failed:%d, tableName:%s, length:%zu",
            errCode, DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
    }
    ReleaseHandle(handle);
    return {errCode, gids};
}

int RelationalSyncAbleStorage::GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket,
    bool useTransaction, DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo)
{
    if (useTransaction && transactionHandle_ == nullptr) {
        LOGE(" the transaction has not been started");
        return -E_INVALID_DB;
    }
    SQLiteSingleVerRelationalStorageExecutor *handle;
    int errCode = E_OK;
    if (useTransaction) {
        handle = transactionHandle_;
    } else {
        errCode = E_OK;
        handle = GetHandle(false, errCode);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    errCode = GetInfoByPrimaryKeyOrGidInner(handle, tableName, vBucket, dataInfoWithLog, assetInfo);
    if (!useTransaction) {
        ReleaseHandle(handle);
    }
    return errCode;
}

int RelationalSyncAbleStorage::UpdateAssetStatusForAssetOnly(const std::string &tableName, VBucket &asset)
{
    if (transactionHandle_ == nullptr) {
        LOGE("the transaction has not been started");
        return -E_INVALID_DB;
    }

    TableSchema tableSchema;
    int errCode = GetCloudTableSchema(tableName, tableSchema);
    if (errCode != E_OK) {
        LOGE("Get cloud schema failed when save cloud data, %d", errCode);
        return errCode;
    }
    RelationalSchemaObject localSchema = GetSchemaInfo();
    transactionHandle_->SetLocalSchema(localSchema);
    transactionHandle_->SetLogicDelete(IsCurrentLogicDelete());
    errCode = transactionHandle_->UpdateAssetStatusForAssetOnly(tableSchema, asset);
    transactionHandle_->SetLogicDelete(false);
    return errCode;
}

void RelationalSyncAbleStorage::PrintCursorChange(const std::string &tableName)
{
    std::lock_guard lock(cursorChangeMutex_);
    auto iter = cursorChangeMap_.find(tableName);
    if (iter == cursorChangeMap_.end()) {
        return;
    }
    LOGI("[RelationalSyncAbleStorage] Upgrade cursor from %d to %d when asset download success.",
        cursorChangeMap_[tableName].first, cursorChangeMap_[tableName].second);
    cursorChangeMap_.erase(tableName);
}

void RelationalSyncAbleStorage::SaveCursorChange(const std::string &tableName, uint64_t currCursor)
{
    std::lock_guard lock(cursorChangeMutex_);
    auto iter = cursorChangeMap_.find(tableName);
    if (iter == cursorChangeMap_.end()) {
        std::pair<uint64_t, uint64_t> initCursors = {currCursor, currCursor};
        cursorChangeMap_.insert(std::pair<std::string, std::pair<uint64_t, uint64_t>>(tableName, initCursors));
        return;
    }
    std::pair<uint64_t, uint64_t> minMaxCursors = iter->second;
    uint64_t minCursor = std::min(minMaxCursors.first, currCursor);
    uint64_t maxCursor = std::max(minMaxCursors.second, currCursor);
    cursorChangeMap_[tableName] = {minCursor, maxCursor};
}

int RelationalSyncAbleStorage::FillCloudAssetForAsyncDownload(const std::string &tableName, VBucket &asset,
    bool isDownloadSuccess)
{
    if (storageEngine_ == nullptr) {
        LOGE("[RelationalSyncAbleStorage]storage is null when fill asset for async download");
        return -E_INVALID_DB;
    }
    if (asyncDownloadTransactionHandle_ == nullptr) {
        LOGE("the transaction has not been started when fill asset for async download.");
        return -E_INVALID_DB;
    }
    TableSchema tableSchema;
    int errCode = GetCloudTableSchema(tableName, tableSchema);
    if (errCode != E_OK) {
        LOGE("Get cloud schema failed when fill cloud asset, %d", errCode);
        return errCode;
    }
    uint64_t currCursor = DBConstant::INVALID_CURSOR;
    errCode = asyncDownloadTransactionHandle_->FillCloudAssetForDownload(
        tableSchema, asset, isDownloadSuccess, currCursor);
    if (errCode != E_OK) {
        LOGE("fill cloud asset for async download failed.%d", errCode);
    }
    return errCode;
}

int RelationalSyncAbleStorage::UpdateRecordFlagForAsyncDownload(const std::string &tableName, bool recordConflict,
    const LogInfo &logInfo)
{
    if (asyncDownloadTransactionHandle_ == nullptr) {
        LOGE("[RelationalSyncAbleStorage] the transaction has not been started");
        return -E_INVALID_DB;
    }
    TableSchema tableSchema;
    GetCloudTableSchema(tableName, tableSchema);
    std::vector<VBucket> assets;
    int errCode = asyncDownloadTransactionHandle_->GetDownloadAssetRecordsByGid(tableSchema, logInfo.cloudGid, assets);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncAbleStorage] get download asset by gid %s failed %d", logInfo.cloudGid.c_str(), errCode);
        return errCode;
    }
    bool isInconsistency = !assets.empty();
    UpdateRecordFlagStruct updateRecordFlag = {
        .tableName = tableName,
        .isRecordConflict = recordConflict,
        .isInconsistency = isInconsistency
    };
    std::string sql = CloudStorageUtils::GetUpdateRecordFlagSql(updateRecordFlag, logInfo);
    return asyncDownloadTransactionHandle_->UpdateRecordFlag(tableName, sql, logInfo);
}

int RelationalSyncAbleStorage::SetLogTriggerStatusForAsyncDownload(bool status)
{
    int errCode = E_OK;
    auto *handle = GetHandleExpectTransactionForAsyncDownload(false, errCode);
    if (handle == nullptr) {
        return errCode;
    }
    errCode = handle->SetLogTriggerStatus(status);
    if (asyncDownloadTransactionHandle_ == nullptr) {
        ReleaseHandle(handle);
    }
    return errCode;
}

std::pair<int, uint32_t> RelationalSyncAbleStorage::GetAssetsByGidOrHashKeyForAsyncDownload(
    const TableSchema &tableSchema, const std::string &gid, const Bytes &hashKey, VBucket &assets)
{
    if (gid.empty() && hashKey.empty()) {
        LOGE("both gid and hashKey are empty.");
        return { -E_INVALID_ARGS, static_cast<uint32_t>(LockStatus::UNLOCK) };
    }
    int errCode = E_OK;
    auto *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        LOGE("executor is null when get assets by gid or hash for async download.");
        return {errCode, static_cast<uint32_t>(LockStatus::UNLOCK)};
    }
    auto [ret, status] = handle->GetAssetsByGidOrHashKey(tableSchema, gid, hashKey, assets);
    if (ret != E_OK && ret != -E_NOT_FOUND && ret != -E_CLOUD_GID_MISMATCH) {
        LOGE("get assets by gid or hashKey failed for async download. %d", ret);
    }
    ReleaseHandle(handle);
    return {ret, status};
}

int RelationalSyncAbleStorage::GetLockStatusByGid(const std::string &tableName, const std::string &gid,
    LockStatus &status)
{
    if (tableName.empty() || gid.empty()) {
        LOGE("[RelationalSyncAbleStorage] invalid table name or gid.");
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    auto *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        LOGE("[RelationalSyncAbleStorage] handle is null when get lock status by gid.");
        return errCode;
    }
    errCode = handle->GetLockStatusByGid(tableName, gid, status);
    ReleaseHandle(handle);
    return errCode;
}

bool RelationalSyncAbleStorage::IsExistTableContainAssets()
{
    std::shared_ptr<DataBaseSchema> cloudSchema = nullptr;
    int errCode = GetCloudDbSchema(cloudSchema);
    if (errCode != E_OK) {
        LOGE("Cannot get cloud schema: %d when check contain assets table", errCode);
        return false;
    }
    if (cloudSchema == nullptr) {
        LOGE("Not set cloud schema when check contain assets table");
        return false;
    }
    auto schema = GetSchemaInfo();
    for (const auto &table : cloudSchema->tables) {
        auto tableInfo = schema.GetTable(table.name);
        if (tableInfo.GetTableName().empty()) {
            continue; // ignore not distributed table
        }
        for (const auto &field : table.fields) {
            if (field.type == TYPE_INDEX<Asset> || field.type == TYPE_INDEX<Assets>) {
                return true;
            }
        }
    }
    return false;
}

bool RelationalSyncAbleStorage::IsSetDistributedSchema(const std::string &tableName, RelationalSchemaObject &schemaObj)
{
    if (schemaObj.GetTableMode() == DistributedTableMode::COLLABORATION) {
        const std::vector<DistributedTable> &tables = schemaObj.GetDistributedSchema().tables;
        if (tables.empty()) {
            LOGE("[RelationalSyncAbleStorage] Distributed schema not set in COLLABORATION mode");
            return false;
        }
        auto iter = std::find_if(tables.begin(), tables.end(), [tableName](const DistributedTable &table) {
            return DBCommon::CaseInsensitiveCompare(table.tableName, tableName);
        });
        if (iter == tables.end()) {
            LOGE("[RelationalSyncAbleStorage] table name mismatch");
            return false;
        }
    }
    return true;
}

int RelationalSyncAbleStorage::CommitForAsyncDownload()
{
    std::unique_lock<std::shared_mutex> lock(asyncDownloadtransactionMutex_);
    if (asyncDownloadTransactionHandle_ == nullptr) {
        LOGE("relation database is null or the transaction has not been started");
        return -E_INVALID_DB;
    }
    int errCode = asyncDownloadTransactionHandle_->Commit();
    ReleaseHandle(asyncDownloadTransactionHandle_);
    asyncDownloadTransactionHandle_ = nullptr;
    LOGD("connection commit transaction!");
    return errCode;
}

int RelationalSyncAbleStorage::RollbackForAsyncDownload()
{
    std::unique_lock<std::shared_mutex> lock(asyncDownloadtransactionMutex_);
    if (asyncDownloadTransactionHandle_ == nullptr) {
        LOGE("Invalid handle for rollback or the transaction has not been started.");
        return -E_INVALID_DB;
    }

    int errCode = asyncDownloadTransactionHandle_->Rollback();
    ReleaseHandle(asyncDownloadTransactionHandle_);
    asyncDownloadTransactionHandle_ = nullptr;
    LOGI("connection rollback transaction!");
    return errCode;
}

SQLiteSingleVerRelationalStorageExecutor *RelationalSyncAbleStorage::GetHandleExpectTransactionForAsyncDownload(
    bool isWrite, int &errCode, OperatePerm perm) const
{
    if (storageEngine_ == nullptr) {
        errCode = -E_INVALID_DB;
        return nullptr;
    }
    if (asyncDownloadTransactionHandle_ != nullptr) {
        return asyncDownloadTransactionHandle_;
    }
    auto handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
        storageEngine_->FindExecutor(isWrite, perm, errCode));
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        handle = nullptr;
    }
    return handle;
}

int RelationalSyncAbleStorage::StartTransactionForAsyncDownload(TransactType type)
{
    if (storageEngine_ == nullptr) {
        return -E_INVALID_DB;
    }
    std::unique_lock<std::shared_mutex> lock(asyncDownloadtransactionMutex_);
    if (asyncDownloadTransactionHandle_ != nullptr) {
        LOGD("async download transaction started already.");
        return -E_TRANSACT_STATE;
    }
    int errCode = E_OK;
    auto *handle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
        storageEngine_->FindExecutor(type == TransactType::IMMEDIATE, OperatePerm::NORMAL_PERM, errCode));
    if (handle == nullptr) {
        ReleaseHandle(handle);
        return errCode;
    }
    errCode = handle->StartTransaction(type);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        return errCode;
    }
    asyncDownloadTransactionHandle_ = handle;
    return errCode;
}

int RelationalSyncAbleStorage::GetCloudTableWithoutShared(std::vector<TableSchema> &tables)
{
    const auto tableInfos = GetSchemaInfo().GetTables();
    for (const auto &[tableName, info] : tableInfos) {
        if (info.GetSharedTableMark()) {
            continue;
        }
        TableSchema schema;
        int errCode = GetCloudTableSchema(tableName, schema);
        if (errCode == -E_NOT_FOUND) {
            continue;
        }
        if (errCode != E_OK) {
            LOGW("[RDBStorage] Get cloud table failed %d", errCode);
            return errCode;
        }
        tables.push_back(schema);
    }
    return E_OK;
}

int RelationalSyncAbleStorage::GetCompensatedSyncQueryInner(SQLiteSingleVerRelationalStorageExecutor *handle,
    const std::vector<TableSchema> &tables, std::vector<QuerySyncObject> &syncQuery, bool isQueryDownloadRecords)
{
    int errCode = E_OK;
    errCode = handle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }
    for (const auto &table : tables) {
        if (!CheckTableSupportCompensatedSync(table)) {
            continue;
        }

        std::vector<VBucket> syncDataPk;
        errCode = handle->GetWaitCompensatedSyncDataPk(table, syncDataPk, isQueryDownloadRecords);
        if (errCode != E_OK) {
            LOGW("[RDBStorageEngine] Get wait compensated sync data failed, continue! errCode=%d", errCode);
            errCode = E_OK;
            continue;
        }
        if (syncDataPk.empty()) {
            // no data need to compensated sync
            continue;
        }
        errCode = CloudStorageUtils::GetSyncQueryByPk(table.name, syncDataPk, false, syncQuery);
        if (errCode != E_OK) {
            LOGW("[RDBStorageEngine] Get compensated sync query happen error, ignore it! errCode = %d", errCode);
            errCode = E_OK;
            continue;
        }
    }
    if (errCode == E_OK) {
        errCode = handle->Commit();
        if (errCode != E_OK) {
            LOGE("[RDBStorageEngine] commit failed %d when get compensated sync query", errCode);
        }
    } else {
        int ret = handle->Rollback();
        if (ret != E_OK) {
            LOGW("[RDBStorageEngine] rollback failed %d when get compensated sync query", ret);
        }
    }
    return errCode;
}

int RelationalSyncAbleStorage::CreateTempSyncTriggerInner(SQLiteSingleVerRelationalStorageExecutor *handle,
    const std::string &tableName, bool flag)
{
    TrackerTable trackerTable = storageEngine_->GetTrackerSchema().GetTrackerTable(tableName);
    if (trackerTable.IsEmpty()) {
        trackerTable.SetTableName(tableName);
    }
    return handle->CreateTempSyncTrigger(trackerTable, flag);
}

bool RelationalSyncAbleStorage::CheckTableSupportCompensatedSync(const TableSchema &table)
{
    auto it = std::find_if(table.fields.begin(), table.fields.end(), [](const auto &field) {
        return field.primary && (field.type == TYPE_INDEX<Asset> || field.type == TYPE_INDEX<Assets> ||
            field.type == TYPE_INDEX<Bytes>);
    });
    if (it != table.fields.end()) {
        LOGI("[RDBStorageEngine] Table contain not support pk field type, ignored");
        return false;
    }
    // check whether reference exist
    std::map<std::string, std::vector<TableReferenceProperty>> tableReference;
    int errCode = RelationalSyncAbleStorage::GetTableReference(table.name, tableReference);
    if (errCode != E_OK) {
        LOGW("[RDBStorageEngine] Get table reference failed! errCode = %d", errCode);
        return false;
    }
    if (!tableReference.empty()) {
        LOGI("[RDBStorageEngine] current table exist reference property");
        return false;
    }
    return true;
}

int RelationalSyncAbleStorage::MarkFlagAsConsistent(const std::string &tableName, const DownloadData &downloadData,
    const std::set<std::string> &gidFilters)
{
    if (transactionHandle_ == nullptr) {
        LOGE("the transaction has not been started");
        return -E_INVALID_DB;
    }
    int errCode = transactionHandle_->MarkFlagAsConsistent(tableName, downloadData, gidFilters);
    if (errCode != E_OK) {
        LOGE("[RelationalSyncAbleStorage] mark flag as consistent failed.%d", errCode);
    }
    return errCode;
}

CloudSyncConfig RelationalSyncAbleStorage::GetCloudSyncConfig() const
{
    std::lock_guard<std::mutex> autoLock(configMutex_);
    return cloudSyncConfig_;
}

void RelationalSyncAbleStorage::SetCloudSyncConfig(const CloudSyncConfig &config)
{
    std::lock_guard<std::mutex> autoLock(configMutex_);
    cloudSyncConfig_ = config;
    LOGI("[RelationalSyncAbleStorage] SetCloudSyncConfig value:[%" PRId32 ", %" PRId32 ", %" PRId32 ", %d]",
        cloudSyncConfig_.maxUploadCount, cloudSyncConfig_.maxUploadSize,
        cloudSyncConfig_.maxRetryConflictTimes, cloudSyncConfig_.isSupportEncrypt);
}

bool RelationalSyncAbleStorage::IsTableExistReference(const std::string &table)
{
    // check whether reference exist
    std::map<std::string, std::vector<TableReferenceProperty>> tableReference;
    int errCode = RelationalSyncAbleStorage::GetTableReference(table, tableReference);
    if (errCode != E_OK) {
        LOGW("[RDBStorageEngine] Get table reference failed! errCode = %d", errCode);
        return false;
    }
    return !tableReference.empty();
}

bool RelationalSyncAbleStorage::IsTableExistReferenceOrReferenceBy(const std::string &table)
{
    // check whether reference or reference by exist
    if (storageEngine_ == nullptr) {
        LOGE("[IsTableExistReferenceOrReferenceBy] storage is null when get reference gid");
        return false;
    }
    RelationalSchemaObject schema = storageEngine_->GetSchema();
    auto referenceProperty = schema.GetReferenceProperty();
    if (referenceProperty.empty()) {
        return false;
    }
    auto [sourceTableName, errCode] = GetSourceTableName(table);
    if (errCode != E_OK) {
        return false;
    }
    for (const auto &property : referenceProperty) {
        if (DBCommon::CaseInsensitiveCompare(property.sourceTableName, sourceTableName) ||
            DBCommon::CaseInsensitiveCompare(property.targetTableName, sourceTableName)) {
            return true;
        }
    }
    return false;
}

void RelationalSyncAbleStorage::ReleaseUploadRecord(const std::string &tableName, const CloudWaterType &type,
    Timestamp localMark)
{
    uploadRecorder_.ReleaseUploadRecord(tableName, type, localMark);
}

int RelationalSyncAbleStorage::ReviseLocalModTime(const std::string &tableName,
    const std::vector<ReviseModTimeInfo> &revisedData)
{
    if (storageEngine_ == nullptr) {
        LOGE("[ReviseLocalModTime] Storage is null");
        return -E_INVALID_DB;
    }
    int errCode = E_OK;
    auto writeHandle = static_cast<SQLiteSingleVerRelationalStorageExecutor *>(
            storageEngine_->FindExecutor(true, OperatePerm::NORMAL_PERM, errCode));
    if (writeHandle == nullptr) {
        LOGE("[ReviseLocalModTime] Get write handle fail: %d", errCode);
        return errCode;
    }
    errCode = writeHandle->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        LOGE("[ReviseLocalModTime] Start Transaction fail: %d", errCode);
        ReleaseHandle(writeHandle);
        return errCode;
    }
    errCode = writeHandle->ReviseLocalModTime(tableName, revisedData);
    if (errCode != E_OK) {
        LOGE("[ReviseLocalModTime] Revise local modify time fail: %d", errCode);
        writeHandle->Rollback();
        ReleaseHandle(writeHandle);
        return errCode;
    }
    errCode = writeHandle->Commit();
    ReleaseHandle(writeHandle);
    return errCode;
}

int RelationalSyncAbleStorage::GetCursor(const std::string &tableName, uint64_t &cursor)
{
    if (transactionHandle_ == nullptr) {
        LOGE("[RelationalSyncAbleStorage] the transaction has not been started");
        return -E_INVALID_DB;
    }
    return transactionHandle_->GetCursor(tableName, cursor);
}

int RelationalSyncAbleStorage::GetLocalDataCount(const std::string &tableName, int &dataCount,
    int &logicDeleteDataCount)
{
    int errCode = E_OK;
    auto *handle = GetHandle(false, errCode);
    if (handle == nullptr || errCode != E_OK) {
        LOGE("[RelationalSyncAbleStorage] Get handle failed when get local data count: %d", errCode);
        return errCode;
    }
    errCode = handle->GetLocalDataCount(tableName, dataCount, logicDeleteDataCount);
    ReleaseHandle(handle);
    return errCode;
}

int RelationalSyncAbleStorage::GetCompressionOption(bool &needCompressOnSync, uint8_t &compressionRate) const
{
    needCompressOnSync = storageEngine_->GetRelationalProperties().GetBoolProp(DBProperties::COMPRESS_ON_SYNC, false);
    compressionRate = storageEngine_->GetRelationalProperties().GetIntProp(DBProperties::COMPRESSION_RATE,
        DBConstant::DEFAULT_COMPTRESS_RATE);
    return E_OK;
}
}
#endif
