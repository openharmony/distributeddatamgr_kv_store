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

#include "sqlite_cloud_store.h"

namespace DistributedDB {
SqliteCloudStore::SqliteCloudStore(IStorageHandle *handle)
    : storageHandle_(handle)
{
}

int SqliteCloudStore::GetMetaData(const Key &key, Value &value) const
{
    return E_OK;
}

int SqliteCloudStore::PutMetaData(const Key &key, const Value &value)
{
    return E_OK;
}

int SqliteCloudStore::ChkSchema(const TableName &tableName)
{
    return E_OK;
}

int SqliteCloudStore::SetCloudDbSchema(const DataBaseSchema &schema)
{
    return E_OK;
}

int SqliteCloudStore::GetCloudDbSchema(std::shared_ptr<DataBaseSchema> &cloudSchema)
{
    return E_OK;
}

int SqliteCloudStore::GetCloudTableSchema(const TableName &tableName,
    TableSchema &tableSchema)
{
    return E_OK;
}

int SqliteCloudStore::StartTransaction(TransactType type)
{
    return E_OK;
}

int SqliteCloudStore::Commit()
{
    return E_OK;
}

int SqliteCloudStore::Rollback()
{
    return E_OK;
}

int SqliteCloudStore::GetUploadCount(const QuerySyncObject &query,
    const Timestamp &timestamp, bool isCloudForcePush,
    int64_t &count)
{
    return E_OK;
}

int SqliteCloudStore::GetCloudData(const TableSchema &tableSchema, const QuerySyncObject &object,
    const Timestamp &beginTime, ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult)
{
    return E_OK;
}

int SqliteCloudStore::GetCloudDataNext(ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult)
{
    return E_OK;
}

int SqliteCloudStore::GetCloudGid(const TableSchema &tableSchema, const QuerySyncObject &querySyncObject,
    bool isCloudForcePush, std::vector<std::string> &cloudGid)
{
    return E_OK;
}

int SqliteCloudStore::ReleaseCloudDataToken(ContinueToken &continueStmtToken)
{
    return E_OK;
}

int SqliteCloudStore::GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket,
    DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo)
{
    return E_OK;
}

int SqliteCloudStore::PutCloudSyncData(const std::string &tableName, DownloadData &downloadData)
{
    return E_OK;
}

int SqliteCloudStore::CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
    const RelationalSchemaObject &localSchema, std::vector<Asset> &assets)
{
    return E_OK;
}

void SqliteCloudStore::TriggerObserverAction(const std::string &deviceName, ChangedData &&changedData,
    bool isChangedData)
{
}

int SqliteCloudStore::FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess)
{
    return E_OK;
}

int SqliteCloudStore::SetLogTriggerStatus(bool status)
{
    return E_OK;
}

int SqliteCloudStore::FillCloudLogAndAsset(OpType opType, const CloudSyncData &data, bool fillAsset,
    bool ignoreEmptyGid)
{
    return E_OK;
}

std::string SqliteCloudStore::GetIdentify() const
{
    return "";
}

int SqliteCloudStore::GetCloudDataGid(const QuerySyncObject &query, Timestamp beginTime, std::vector<std::string> &gid)
{
    return E_OK;
}

int SqliteCloudStore::CheckQueryValid(const QuerySyncObject &query)
{
    return E_OK;
}

int SqliteCloudStore::CreateTempSyncTrigger(const std::string &tableName)
{
    return ICloudSyncStorageInterface::CreateTempSyncTrigger(tableName);
}

int SqliteCloudStore::GetAndResetServerObserverData(const std::string &tableName, ChangeProperties &changeProperties)
{
    return ICloudSyncStorageInterface::GetAndResetServerObserverData(tableName, changeProperties);
}

int SqliteCloudStore::ClearAllTempSyncTrigger()
{
    return ICloudSyncStorageInterface::ClearAllTempSyncTrigger();
}

bool SqliteCloudStore::IsSharedTable(const std::string &tableName)
{
    return false;
}

void SqliteCloudStore::SetCloudTaskConfig(const CloudTaskConfig &config)
{
    ICloudSyncStorageInterface::SetCloudTaskConfig(config);
}

int SqliteCloudStore::GetAssetsByGidOrHashKey(const TableSchema &tableSchema, const std::string &gid,
    const Bytes &hashKey, VBucket &assets)
{
    return ICloudSyncStorageInterface::GetAssetsByGidOrHashKey(tableSchema, gid, hashKey, assets);
}

int SqliteCloudStore::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    return ICloudSyncStorageInterface::SetIAssetLoader(loader);
}

int SqliteCloudStore::UpdateRecordFlag(const std::string &tableName, bool recordConflict, const LogInfo &logInfo)
{
    return ICloudSyncStorageInterface::UpdateRecordFlag(tableName, recordConflict, logInfo);
}

int SqliteCloudStore::GetCompensatedSyncQuery(std::vector<QuerySyncObject> &syncQuery)
{
    return ICloudSyncStorageInterface::GetCompensatedSyncQuery(syncQuery);
}

int SqliteCloudStore::MarkFlagAsConsistent(const std::string &tableName, const DownloadData &downloadData,
    const std::set<std::string> &gidFilters)
{
    return ICloudSyncStorageInterface::MarkFlagAsConsistent(tableName, downloadData, gidFilters);
}
}