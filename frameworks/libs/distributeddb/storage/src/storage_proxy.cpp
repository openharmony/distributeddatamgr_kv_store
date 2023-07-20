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

int StorageProxy::GetLocalWaterMark(const std::string &tableName, LocalWaterMark &localMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (transactionExeFlag_.load() && isWrite_.load()) {
        LOGE("the write transaction has been started, can not get meta");
        return -E_BUSY;
    }
    return cloudMetaData_->GetLocalWaterMark(tableName, localMark);
}

int StorageProxy::PutLocalWaterMark(const std::string &tableName, LocalWaterMark &localMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (transactionExeFlag_.load() && isWrite_.load()) {
        LOGE("the write transaction has been started, can not put meta");
        return -E_BUSY;
    }
    return cloudMetaData_->SetLocalWaterMark(tableName, localMark);
}

int StorageProxy::GetCloudWaterMark(const std::string &tableName, CloudWaterMark &cloudMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    return cloudMetaData_->GetCloudWaterMark(tableName, cloudMark);
}

int StorageProxy::SetCloudWaterMark(const std::string &tableName, CloudWaterMark &cloudMark)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (cloudMetaData_ == nullptr) {
        return -E_INVALID_DB;
    }
    return cloudMetaData_->SetCloudWaterMark(tableName, cloudMark);
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

int StorageProxy::GetUploadCount(const std::string &tableName, const LocalWaterMark &localMark,
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
    return store_->GetUploadCount(tableName, localMark, isCloudForcePush, count);
}

int StorageProxy::FillCloudGid(const CloudSyncData &data)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
    }
    return store_->FillCloudGid(data);
}

int StorageProxy::GetCloudData(const std::string &tableName, const Timestamp &timeRange,
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
    int errCode = store_->GetCloudTableSchema(tableName, tableSchema);
    if (errCode != E_OK) {
        return errCode;
    }
    return store_->GetCloudData(tableSchema, timeRange, continueStmtToken, cloudDataResult);
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

    return store_->GetInfoByPrimaryKeyOrGid(tableName, vBucket, dataInfoWithLog, assetInfo);
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
    if (colNames.empty()) {
        colNames.push_back(CloudDbConstant::ROW_ID_FIELD_NAME);
    }
    return E_OK;
}

int StorageProxy::NotifyChangedData(const std::string deviceName, ChangedData &&changedData)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    store_->TriggerObserverAction(deviceName, std::move(changedData), true);
    return E_OK;
}

int StorageProxy::FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isFullReplace)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    return store_->FillCloudAssetForDownload(tableName, asset, isFullReplace);
}

int StorageProxy::FillCloudGidAndAsset(const OpType &opType, const CloudSyncData &data)
{
    std::shared_lock<std::shared_mutex> readLock(storeMutex_);
    if (store_ == nullptr) {
        return -E_INVALID_DB;
    }
    if (!transactionExeFlag_.load()) {
        LOGE("the transaction has not been started");
        return -E_TRANSACT_STATE;
    }
    return store_->FillCloudGidAndAsset(opType, data);
}
}
