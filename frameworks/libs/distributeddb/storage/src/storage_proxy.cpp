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

#include "storage_proxy.h"

#include "cloud/cloud_storage_utils.h"
#include "cloud/schema_mgr.h"
#include "db_common.h"
#include "store_types.h"

namespace DistributedDB {
StorageProxy::StorageProxy(ICloudSyncStorageInterface *iCloud)
    :store_(iCloud),
    transactionExeFlag_(false),
    isWrite_(false)
{
}

std::shared_ptr<StorageProxy> StorageProxy::GetCloudDb(ICloudSyncStorageInterface *iCloud)
{
    std::shared_ptr<StorageProxy> proxy = std::make_shared<StorageProxy>(iCloud);
    proxy->Init();
    return proxy;
}

void StorageProxy::Init()
{
    cloudMetaData_ = std::make_shared<CloudMetaData>(store_);
}

int StorageProxy::Close()
{
    std::unique_lock<std::shared_mutex> writeLock(storeMutex_);
    if (transactionExeFlag_.load()) {
        LOGE("the transaction has been started, storage proxy can not closed");
        return -E_BUSY;
    }
    store_ = nullptr;
    cloudMetaData_ = nullptr;
    return E_OK;
}

int StorageProxy::GetLocalWaterMark(const std::string &tableName, Timestamp &localMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (transactionExeFlag_.load() && isWrite_.load()) {
        LOGE("the write transaction has been started, can not get meta");
        return -E_BUSY;
    }
    return cloudMetaData_->GetLocalWaterMark(AppendWithUserIfNeed(tableName), localMark);
}

int StorageProxy::GetLocalWaterMarkByMode(const std::string &tableName, CloudWaterType mode, Timestamp &localMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    return cloudMetaData_->GetLocalWaterMarkByType(AppendWithUserIfNeed(tableName), mode, localMark);
}

int StorageProxy::PutLocalWaterMark(const std::string &tableName, Timestamp &localMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (transactionExeFlag_.load() && isWrite_.load()) {
        LOGE("the write transaction has been started, can not put meta");
        return -E_BUSY;
    }
    return cloudMetaData_->SetLocalWaterMark(AppendWithUserIfNeed(tableName), localMark);
}

int StorageProxy::PutWaterMarkByMode(const std::string &tableName, CloudWaterType mode, Timestamp &localMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (transactionExeFlag_.load() && isWrite_.load()) {
        LOGE("the write transaction has been started, can not put meta");
        return -E_BUSY;
    }
    return cloudMetaData_->SetLocalWaterMarkByType(AppendWithUserIfNeed(tableName), mode, localMark);
}

int StorageProxy::GetCloudWaterMark(const std::string &tableName, std::string &cloudMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    return cloudMetaData_->GetCloudWaterMark(AppendWithUserIfNeed(tableName), cloudMark);
}

int StorageProxy::SetCloudWaterMark(const std::string &tableName, std::string &cloudMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    return cloudMetaData_->SetCloudWaterMark(AppendWithUserIfNeed(tableName), cloudMark);
}

int StorageProxy::StartTransaction(TransactType type, bool isAsyncDownload)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = store_->StartTransaction(type, isAsyncDownload);
    if (errCode == E_OK && !isAsyncDownload) {
        transactionExeFlag_.store(true);
        isWrite_.store(type == TransactType::IMMEDIATE);
    }
    return errCode;
}

int StorageProxy::Commit(bool isAsyncDownload)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = store_->Commit(isAsyncDownload);
    if (errCode == E_OK && !isAsyncDownload) {
        transactionExeFlag_.store(false);
    }
    return errCode;
}

int StorageProxy::Rollback(bool isAsyncDownload)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = store_->Rollback(isAsyncDownload);
    if (errCode == E_OK && !isAsyncDownload) {
        transactionExeFlag_.store(false);
    }
    return errCode;
}

int StorageProxy::GetUploadCount(const QuerySyncObject &query, const bool isCloudForcePush,
    bool isCompensatedTask, bool isUseWaterMark, int64_t &count)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    std::vector<Timestamp> timeStampVec;
    std::vector<CloudWaterType> waterTypeVec = DBCommon::GetWaterTypeVec();
    for (size_t i = 0; i < waterTypeVec.size(); i++) {
        Timestamp tmpMark = 0u;
        if (isUseWaterMark) {
            int errCode = cloudMetaData_->GetLocalWaterMarkByType(AppendWithUserIfNeed(query.GetTableName()),
                waterTypeVec[i], tmpMark);
            if (errCode != E_OK) {
                return errCode;
            }
        }
        timeStampVec.push_back(tmpMark);
    }
    return store_->GetAllUploadCount(query, timeStampVec, isCloudForcePush, isCompensatedTask, count);
}

int StorageProxy::GetUploadCount(const std::string &tableName, const Timestamp &localMark,
    const bool isCloudForcePush, int64_t &count)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    QuerySyncObject query;
    query.SetTableName(tableName);
    return store_->GetUploadCount(query, localMark, isCloudForcePush, false, count);
}

int StorageProxy::GetUploadCount(const QuerySyncObject &query, const Timestamp &localMark,
    bool isCloudForcePush, bool isCompensatedTask, int64_t &count)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->GetUploadCount(query, localMark, isCloudForcePush, isCompensatedTask, count);
}

int StorageProxy::GetCloudData(const std::string &tableName, const Timestamp &timeRange,
    ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult)
{
    QuerySyncObject querySyncObject;
    querySyncObject.SetTableName(tableName);
    return GetCloudData(querySyncObject, timeRange, continueStmtToken, cloudDataResult);
}

int StorageProxy::GetCloudData(const QuerySyncObject &querySyncObject, const Timestamp &timeRange,
    ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    TableSchema tableSchema;
    int errCode = store_->GetCloudTableSchema(querySyncObject.GetRelationTableName(), tableSchema);
    if (errCode != E_OK) {
        return errCode;
    }
    return store_->GetCloudData(tableSchema, querySyncObject, timeRange, continueStmtToken, cloudDataResult);
}

int StorageProxy::GetCloudDataNext(ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) const
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->GetCloudDataNext(continueStmtToken, cloudDataResult);
}

int StorageProxy::GetCloudGid(const QuerySyncObject &querySyncObject, bool isCloudForcePush,
    bool isCompensatedTask, std::vector<std::string> &cloudGid)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    TableSchema tableSchema;
    int errCode = store_->GetCloudTableSchema(querySyncObject.GetRelationTableName(), tableSchema);
    if (errCode != E_OK) {
        return errCode;
    }
    return store_->GetCloudGid(tableSchema, querySyncObject, isCloudForcePush, isCompensatedTask, cloudGid);
}

int StorageProxy::GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket, bool useTransaction,
    DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (useTransaction && !transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
    }

    int errCode = store_->GetInfoByPrimaryKeyOrGid(tableName, vBucket, useTransaction, dataInfoWithLog, assetInfo);
    if ((dataInfoWithLog.logInfo.flag & static_cast<uint64_t>(LogInfoFlag::FLAG_LOGIC_DELETE)) != 0) {
        assetInfo.clear();
    }
    return errCode;
}

int StorageProxy::SetCursorIncFlag(bool flag)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->SetCursorIncFlag(flag);
}

int StorageProxy::GetCursor(const std::string &tableName, uint64_t &cursor)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->GetCursor(tableName, cursor);
}

int StorageProxy::PutCloudSyncData(const std::string &tableName, DownloadData &downloadData)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
    }
    downloadData.user = user_;
    return store_->PutCloudSyncData(tableName, downloadData);
}

int StorageProxy::UpdateAssetStatusForAssetOnly(const std::string &tableName, VBucket &asset)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("the store is nullptr");
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
    }
    int ret = SetCursorIncFlag(false);
    if (ret != E_OK) {
        LOGE("set curosr inc flag false fail when update for assets only. [table: %s length: %lu] err:%d",
            DBCommon::StringMiddleMasking(tableName).c_str(), tableName.length(), ret);
        return ret;
    }
    ret = SetLogTriggerStatus(false);
    if (ret != E_OK) {
        LOGE("set log trigger false fail when update for assets only. [table: %s length: %lu] err:%d",
            DBCommon::StringMiddleMasking(tableName).c_str(), tableName.length(), ret);
        return ret;
    }
    ret = store_->UpdateAssetStatusForAssetOnly(tableName, asset);
    if (ret != E_OK) {
        LOGE("update for assets only fail. [table: %s length: %lu] err:%d",
            DBCommon::StringMiddleMasking(tableName).c_str(), tableName.length(), ret);
        return ret;
    }
    ret = SetLogTriggerStatus(true);
    if (ret != E_OK) {
        LOGE("set log trigger false true when update for assets only. table: %s err:%d",
            DBCommon::StringMiddleMasking(tableName).c_str(), ret);
        return ret;
    }
    return SetCursorIncFlag(true);
}

int StorageProxy::CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
    const RelationalSchemaObject &localSchema, std::vector<Asset> &assets)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
    }
    return store_->CleanCloudData(mode, tableNameList, localSchema, assets);
}

int StorageProxy::ClearCloudLogVersion(const std::vector<std::string> &tableNameList)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("[StorageProxy][ClearCloudLogVersion] the transaction has not been started");
        return -E_TRANSACT_STATE;
    }
    return store_->ClearCloudLogVersion(tableNameList);
}

int StorageProxy::ReleaseContinueToken(ContinueToken &continueStmtToken)
{
    return store_->ReleaseCloudDataToken(continueStmtToken);
}

int StorageProxy::CheckSchema(const TableName &tableName) const
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->ChkSchema(tableName);
}

int StorageProxy::CheckSchema(std::vector<std::string> &tables)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (tables.empty()) {
        return -E_INVALID_ARGS;
    }
    for (const auto &table : tables) {
        int ret = store_->ChkSchema(table);
        if (ret != E_OK) {
            return ret;
        }
    }
    return E_OK;
}

int StorageProxy::GetPrimaryColNamesWithAssetsFields(const TableName &tableName, std::vector<std::string> &colNames,
    std::vector<Field> &assetFields)
{
    if (!colNames.empty()) {
        // output parameter should be empty
        return -E_INVALID_ARGS;
    }

    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    // GetTableInfo
    TableSchema tableSchema;
    int ret = store_->GetCloudTableSchema(tableName, tableSchema);
    if (ret != E_OK) {
        LOGE("Cannot get cloud table schema: %d", ret);
        return ret;
    }
    for (const auto &field : tableSchema.fields) {
        if (field.primary) {
            colNames.push_back(field.colName);
        }
        if (field.type == TYPE_INDEX<Asset> || field.type == TYPE_INDEX<Assets>) {
            assetFields.push_back(field);
        }
    }
    if (colNames.empty() || colNames.size() > 1) {
        (void)colNames.insert(colNames.begin(), DBConstant::ROWID);
    }
    return E_OK;
}

int StorageProxy::NotifyChangedData(const std::string &deviceName, ChangedData &&changedData)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    ChangeProperties changeProperties;
    store_->GetAndResetServerObserverData(changedData.tableName, changeProperties);
    changedData.properties = changeProperties;
    store_->TriggerObserverAction(deviceName, std::move(changedData), true);
    return E_OK;
}

int StorageProxy::FillCloudAssetForAsyncDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[StorageProxy]the store is nullptr when fill asset for async download");
        return -E_INVALID_DB;
    }
    return store_->FillCloudAssetForAsyncDownload(tableName, asset, isDownloadSuccess);
}

void StorageProxy::PrintCursorChange(const std::string &tableName)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return;
    }
    store_->PrintCursorChange(tableName);
}

int StorageProxy::FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load() || !isWrite_.load()) {
        LOGE("the write transaction has not started before fill download assets");
        return -E_TRANSACT_STATE;
    }
    return store_->FillCloudAssetForDownload(tableName, asset, isDownloadSuccess);
}

int StorageProxy::SetLogTriggerStatus(bool status, bool isAsyncDownload)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (isAsyncDownload) {
        return store_->SetLogTriggerStatusForAsyncDownload(status);
    } else {
        return store_->SetLogTriggerStatus(status);
    }
}

int StorageProxy::FillCloudLogAndAsset(OpType opType, const CloudSyncData &data)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->FillCloudLogAndAsset(opType, data, true, false);
}

std::string StorageProxy::GetIdentify() const
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGW("[StorageProxy] store is nullptr return default");
        return "";
    }
    return store_->GetIdentify();
}

Timestamp StorageProxy::EraseNanoTime(DistributedDB::Timestamp localTime)
{
    return localTime / CloudDbConstant::TEN_THOUSAND * CloudDbConstant::TEN_THOUSAND;
}

int StorageProxy::CleanWaterMark(const DistributedDB::TableName &tableName)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        LOGW("[StorageProxy] meta is nullptr return default");
        return -E_INVALID_DB;
    }
    return cloudMetaData_->CleanWaterMark(AppendWithUserIfNeed(tableName));
}

int StorageProxy::CleanWaterMarkInMemory(const DistributedDB::TableName &tableName)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        LOGW("[StorageProxy] CleanWaterMarkInMemory is nullptr return default");
        return -E_INVALID_DB;
    }
    cloudMetaData_->CleanWaterMarkInMemory(AppendWithUserIfNeed(tableName));
    return E_OK;
}

void StorageProxy::SetUser(const std::string &user)
{
    user_ = user;
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ != nullptr) {
        store_->SetUser(user);
    }
}

int StorageProxy::CreateTempSyncTrigger(const std::string &tableName)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->CreateTempSyncTrigger(tableName);
}

int StorageProxy::ClearAllTempSyncTrigger()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    // Clean up all temporary triggers
    return store_->ClearAllTempSyncTrigger();
}

int StorageProxy::IsSharedTable(const std::string &tableName, bool &IsSharedTable)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    IsSharedTable = store_->IsSharedTable(tableName);
    return E_OK;
}

void StorageProxy::FillCloudGidAndLogIfSuccess(const OpType opType, const CloudSyncData &data)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGW("[StorageProxy] fill gid failed with store invalid");
        return;
    }
    int errCode = store_->FillCloudLogAndAsset(opType, data, true, true);
    if (errCode != E_OK) {
        LOGW("[StorageProxy] fill gid failed %d", errCode);
    }
}

std::pair<int, uint32_t> StorageProxy::GetAssetsByGidOrHashKey(const std::string &tableName, bool isAsyncDownload,
    const std::string &gid, const Bytes &hashKey, VBucket &assets)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return { -E_INVALID_DB, static_cast<uint32_t>(LockStatus::UNLOCK) };
    }
    TableSchema tableSchema;
    int errCode = store_->GetCloudTableSchema(tableName, tableSchema);
    if (errCode != E_OK) {
        LOGE("get cloud table schema failed: %d", errCode);
        return { errCode, static_cast<uint32_t>(LockStatus::UNLOCK) };
    }
    if (isAsyncDownload) {
        return store_->GetAssetsByGidOrHashKeyForAsyncDownload(tableSchema, gid, hashKey, assets);
    } else {
        return store_->GetAssetsByGidOrHashKey(tableSchema, gid, hashKey, assets);
    }
}

int StorageProxy::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->SetIAssetLoader(loader);
}

int StorageProxy::UpdateRecordFlag(const std::string &tableName, bool isAsyncDownload, bool recordConflict,
    const LogInfo &logInfo)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (isAsyncDownload) {
        return store_->UpdateRecordFlagForAsyncDownload(tableName, recordConflict, logInfo);
    } else {
        return store_->UpdateRecordFlag(tableName, recordConflict, logInfo);
    }
}

int StorageProxy::GetCompensatedSyncQuery(std::vector<QuerySyncObject> &syncQuery, std::vector<std::string> &users,
    bool isQueryDownloadRecords)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->GetCompensatedSyncQuery(syncQuery, users, isQueryDownloadRecords);
}

int StorageProxy::ClearUnLockingNoNeedCompensated()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->ClearUnLockingNoNeedCompensated();
}

int StorageProxy::MarkFlagAsConsistent(const std::string &tableName, const DownloadData &downloadData,
    const std::set<std::string> &gidFilters)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->MarkFlagAsConsistent(tableName, downloadData, gidFilters);
}

int StorageProxy::MarkFlagAsAssetAsyncDownload(const std::string &tableName, const DownloadData &downloadData,
    const std::set<std::string> &gidFilters)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->MarkFlagAsAssetAsyncDownload(tableName, downloadData, gidFilters);
}

void StorageProxy::OnSyncFinish()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return;
    }
    store_->SyncFinishHook();
}

void StorageProxy::OnUploadStart()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return;
    }
    store_->DoUploadHook();
}

void StorageProxy::CleanAllWaterMark()
{
    cloudMetaData_->CleanAllWaterMark();
}

std::string StorageProxy::AppendWithUserIfNeed(const std::string &source) const
{
    if (user_.empty()) {
        return source;
    }
    return source + "_" + user_;
}

int StorageProxy::GetCloudDbSchema(std::shared_ptr<DataBaseSchema> &cloudSchema)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->GetCloudDbSchema(cloudSchema);
}

std::pair<int, CloudSyncData> StorageProxy::GetLocalCloudVersion()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return {-E_INTERNAL_ERROR, {}};
    }
    return store_->GetLocalCloudVersion();
}

CloudSyncConfig StorageProxy::GetCloudSyncConfig() const
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return {};
    }
    return store_->GetCloudSyncConfig();
}

bool StorageProxy::IsTableExistReference(const std::string &table)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return false;
    }
    return store_->IsTableExistReference(table);
}

bool StorageProxy::IsTableExistReferenceOrReferenceBy(const std::string &table)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return false;
    }
    return store_->IsTableExistReferenceOrReferenceBy(table);
}

void StorageProxy::ReleaseUploadRecord(const std::string &table, const CloudWaterType &type, Timestamp localWaterMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return;
    }
    store_->ReleaseUploadRecord(table, type, localWaterMark);
}

bool StorageProxy::IsTagCloudUpdateLocal(const LogInfo &localInfo, const LogInfo &cloudInfo,
    SingleVerConflictResolvePolicy policy)
{
    if (store_ == nullptr) {
        return false;
    }
    return store_->IsTagCloudUpdateLocal(localInfo, cloudInfo, policy);
}

int StorageProxy::ReviseLocalModTime(const std::string &tableName,
    const std::vector<ReviseModTimeInfo> &revisedData)
{
    std::shared_lock<std::shared_mutex> writeLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->ReviseLocalModTime(tableName, revisedData);
}

bool StorageProxy::IsCurrentLogicDelete() const
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[StorageProxy] no store found when get logic delete flag");
        return false;
    }
    return store_->IsCurrentLogicDelete();
}

int StorageProxy::GetLocalDataCount(const std::string &tableName, int &dataCount, int &logicDeleteDataCount)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[StorageProxy] no store found when get local data");
        return -E_INVALID_DB;
    }
    return store_->GetLocalDataCount(tableName, dataCount, logicDeleteDataCount);
}

std::pair<int, std::vector<std::string>> StorageProxy::GetDownloadAssetTable()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[StorageProxy] no store found when get downloading assets table");
        return {-E_INVALID_DB, std::vector<std::string>()};
    }
    return store_->GetDownloadAssetTable();
}

std::pair<int, std::vector<std::string>> StorageProxy::GetDownloadAssetRecords(const std::string &tableName,
    int64_t beginTime)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[StorageProxy] no store found when get downloading assets");
        return {-E_INVALID_DB, std::vector<std::string>()};
    }
    return store_->GetDownloadAssetRecords(tableName, beginTime);
}

void StorageProxy::BeforeUploadTransaction()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return;
    }
    store_->DoBeforeUploadTransaction();
}

int StorageProxy::GetLockStatusByGid(const std::string &tableName, const std::string &gid, LockStatus &status)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[StorageProxy] no store found when get lock status");
        return -E_INVALID_DB;
    }
    return store_->GetLockStatusByGid(tableName, gid, status);
}

bool StorageProxy::IsExistTableContainAssets()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("store is nullptr when check contain assets table");
        return false;
    }
    return store_->IsExistTableContainAssets();
}

bool StorageProxy::GetTransactionExeFlag()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    return transactionExeFlag_.load();
}

void StorageProxy::FilterDownloadRecordNotFound(const std::string &tableName, DownloadData &downloadData)
{
    std::vector<std::string> gids;
    for (auto data = downloadData.data.begin(); data != downloadData.data.end();) {
        if (!DBCommon::IsCloudRecordNotFound(*data)) {
            ++data;
            continue;
        }
        std::string gid;
        (void)CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::GID_FIELD, *data, gid);
        if (!gid.empty()) {
            gids.push_back(gid);
            data = downloadData.data.erase(data);
        } else {
            ++data;
        }
    }
    if (gids.empty()) {
        return;
    }
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGW("[FilterDownloadRecordNotFound] store is null");
        return;
    }
    LOGW("Download data from cloud contains record not found.");
    (void)store_->ConvertLogToLocal(tableName, gids);
}

int StorageProxy::PutCloudGid(const std::string &tableName, std::vector<VBucket> &data)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[PutCloudGid] store is null");
        return -E_INVALID_DB;
    }
    return store_->PutCloudGid(tableName, data);
}

int StorageProxy::DropTempTable(const std::string &tableName)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[DropTempTable] store is null");
        return -E_INVALID_DB;
    }
    return store_->DropTempTable(tableName);
}

int StorageProxy::GetCloudGidCursor(const std::string &tableName, std::string &cursor)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        LOGE("[GetCloudGidCursor] meta is null");
        return -E_INVALID_DB;
    }
    return cloudMetaData_->GetCloudGidCursor(tableName, cursor);
}

int StorageProxy::PutCloudGidCursor(const std::string &tableName, const std::string &cursor)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        LOGE("[PutCloudGidCursor] meta is null");
        return -E_INVALID_DB;
    }
    return cloudMetaData_->PutCloudGidCursor(tableName, cursor);
}

int StorageProxy::GetBackupCloudCursor(const std::string &tableName, std::string &cursor)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        LOGE("[GetBackupCloudCursor] meta is null");
        return -E_INVALID_DB;
    }
    return cloudMetaData_->GetBackupCloudCursor(tableName, cursor);
}

int StorageProxy::PutBackupCloudCursor(const std::string &tableName, const std::string &cursor)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        LOGE("[PutBackupCloudCursor] meta is null");
        return -E_INVALID_DB;
    }
    return cloudMetaData_->PutBackupCloudCursor(tableName, cursor);
}

int StorageProxy::CleanCloudInfo(const std::string &tableName)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        LOGE("[CleanCloudInfo] meta is null");
        return -E_INVALID_DB;
    }
    return cloudMetaData_->CleanCloudInfo(tableName);
}

int StorageProxy::DeleteCloudNoneExistRecord(const std::string &tableName, std::pair<bool, bool> isNeedDeleted)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[DeleteCloudNoneExistRecord] store is null");
        return -E_INVALID_DB;
    }
    return store_->DeleteCloudNoneExistRecord(tableName, isNeedDeleted);
}

int StorageProxy::GetGidRecordCount(const std::string &tableName, uint64_t &count) const
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGE("[CountGidRecords] store is null");
        return -E_INVALID_DB;
    }
    return store_->GetGidRecordCount(tableName, count);
}
}