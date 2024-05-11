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

#include "cloud/schema_mgr.h"
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

int StorageProxy::GetLocalWaterMarkByMode(const std::string &tableName, Timestamp &localMark, CloudWaterType mode)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (transactionExeFlag_.load() && isWrite_.load()) {
        LOGE("the write transaction has been started, can not get meta");
        return -E_BUSY;
    }
    if (user_.empty()) {
        return cloudMetaData_->GetLocalWaterMarkByType(tableName, mode, localMark);
    } else {
        return cloudMetaData_->GetLocalWaterMarkByType(tableName + "_" + user_, mode, localMark);
    }
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

int StorageProxy::PutWaterMarkByMode(const std::string &tableName, Timestamp &localMark, CloudWaterType mode)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (transactionExeFlag_.load() && isWrite_.load()) {
        LOGE("the write transaction has been started, can not put meta");
        return -E_BUSY;
    }
    if (user_.empty()) {
        return cloudMetaData_->SetLocalWaterMarkByType(tableName, mode, localMark);
    } else {
        return cloudMetaData_->SetLocalWaterMarkByType(tableName + "_" + user_, mode, localMark);
    }
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

int StorageProxy::StartTransaction(TransactType type)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = store_->StartTransaction(type);
    if (errCode == E_OK) {
        transactionExeFlag_.store(true);
        isWrite_.store(type == TransactType::IMMEDIATE);
    }
    return errCode;
}

int StorageProxy::Commit()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = store_->Commit();
    if (errCode == E_OK) {
        transactionExeFlag_.store(false);
    }
    return errCode;
}

int StorageProxy::Rollback()
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = store_->Rollback();
    if (errCode == E_OK) {
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
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
    }
    std::vector<Timestamp> timeStampVec;
    std::vector<CloudWaterType> waterTypeVec = {CloudWaterType::DELETE, CloudWaterType::UPDATE,
        CloudWaterType::INSERT};
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
    int errCode = store_->GetAllUploadCount(query, timeStampVec, isCloudForcePush, isCompensatedTask, count);
    if (errCode != E_OK) {
        return errCode;
    }
    return E_OK;
}

int StorageProxy::GetUploadCount(const std::string &tableName, const Timestamp &localMark,
    const bool isCloudForcePush, int64_t &count)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
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
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
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
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
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
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
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

int StorageProxy::GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket,
    DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
    }

    int errCode = store_->GetInfoByPrimaryKeyOrGid(tableName, vBucket, dataInfoWithLog, assetInfo);
    if (errCode == E_OK) {
        dataInfoWithLog.logInfo.timestamp = EraseNanoTime(dataInfoWithLog.logInfo.timestamp);
        dataInfoWithLog.logInfo.wTimestamp = EraseNanoTime(dataInfoWithLog.logInfo.wTimestamp);
    }
    if ((dataInfoWithLog.logInfo.flag & static_cast<uint64_t>(LogInfoFlag::FLAG_LOGIC_DELETE)) != 0) {
        assetInfo.clear();
    }
    return errCode;
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
        (void)colNames.insert(colNames.begin(), CloudDbConstant::ROW_ID_FIELD_NAME);
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

int StorageProxy::SetLogTriggerStatus(bool status)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->SetLogTriggerStatus(status);
}

int StorageProxy::FillCloudLogAndAsset(OpType opType, const CloudSyncData &data)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
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
    std::unique_lock<std::shared_mutex> writeLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    IsSharedTable = store_->IsSharedTable(tableName);
    return E_OK;
}

void StorageProxy::FillCloudGidIfSuccess(const OpType opType, const CloudSyncData &data)
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

void StorageProxy::SetCloudTaskConfig(const CloudTaskConfig &config)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        LOGW("[StorageProxy] fill gid failed with store invalid");
        return;
    }
    store_->SetCloudTaskConfig(config);
}

std::pair<int, uint32_t> StorageProxy::GetAssetsByGidOrHashKey(const std::string &tableName, const std::string &gid,
    const Bytes &hashKey, VBucket &assets)
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
    return store_->GetAssetsByGidOrHashKey(tableSchema, gid, hashKey, assets);
}

int StorageProxy::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return E_INVALID_DB;
    }
    return store_->SetIAssetLoader(loader);
}

int StorageProxy::UpdateRecordFlag(const std::string &tableName, bool recordConflict, const LogInfo &logInfo)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return E_INVALID_DB;
    }
    return store_->UpdateRecordFlag(tableName, recordConflict, logInfo);
}

int StorageProxy::GetCompensatedSyncQuery(std::vector<QuerySyncObject> &syncQuery)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return E_INVALID_DB;
    }
    return store_->GetCompensatedSyncQuery(syncQuery);
}

int StorageProxy::MarkFlagAsConsistent(const std::string &tableName, const DownloadData &downloadData,
    const std::set<std::string> &gidFilters)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return E_INVALID_DB;
    }
    return store_->MarkFlagAsConsistent(tableName, downloadData, gidFilters);
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
        return E_INVALID_DB;
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
}
