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

int RelationalSyncAbleStorage::FillCloudAssetForAsyncDownload(const std::string &tableName, VBucket &asset,
    bool isDownloadSuccess)
{
    if (storageEngine_ == nullptr) {
        LOGE("[RelationalSyncAbleStorage]storage is null when fill asset for async download");
        return -E_INVALID_DB;
    }
    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("executor is null when fill asset for async download.");
        return errCode;
    }
    TableSchema tableSchema;
    errCode = GetCloudTableSchema(tableName, tableSchema);
    if (errCode != E_OK) {
        ReleaseHandle(handle);
        LOGE("Get cloud schema failed when fill cloud asset, %d, tableName:%s, length:%zu",
            errCode, DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
        return errCode;
    }
    errCode = handle->FillCloudAssetForDownload(tableSchema, asset, isDownloadSuccess);
    if (errCode != E_OK) {
        LOGE("fill cloud asset for download failed:%d, tableName:%s, length:%zu",
            errCode, DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
    }
    ReleaseHandle(handle);
    return errCode;
}
 
int RelationalSyncAbleStorage::UpdateRecordFlagForAsyncDownload(const std::string &tableName, bool recordConflict,
    const LogInfo &logInfo)
{
    int errCode = E_OK;
    auto *handle = GetHandle(true, errCode);
    if (handle == nullptr) {
        LOGE("executor is null when update flag for async download.");
        return errCode;
    }
    std::string sql = CloudStorageUtils::GetUpdateRecordFlagSql(tableName, recordConflict, logInfo);
    errCode = handle->UpdateRecordFlag(tableName, sql, logInfo);
    if (errCode != E_OK) {
        LOGE("update flag for async download failed:%d, tableName:%s, length:%zu",
            errCode, DBCommon::StringMiddleMasking(tableName).c_str(), tableName.size());
    }
    ReleaseHandle(handle);
    return errCode;
}
 
int RelationalSyncAbleStorage::SetLogTriggerStatusForAsyncDownload(bool status)
{
    int errCode = E_OK;
    auto *handle = GetHandle(false, errCode);
    if (handle == nullptr) {
        LOGE("executor is null when set trigger status for async download.");
        return errCode;
    }
    errCode = handle->SetLogTriggerStatus(status);
    if (errCode != E_OK) {
        LOGE("set trigger status for async download failed:%d");
    }
    ReleaseHandle(handle);
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
}
#endif