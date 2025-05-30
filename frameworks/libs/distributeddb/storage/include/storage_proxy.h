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

#ifndef STORAGE_PROXY_H
#define STORAGE_PROXY_H

#include <atomic>
#include <shared_mutex>

#include "cloud/cloud_store_types.h"
#include "cloud/cloud_meta_data.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/iAssetLoader.h"
#include "cloud/schema_mgr.h"
#include "data_transformer.h"
#include "icloud_sync_storage_interface.h"
#include "query_sync_object.h"


namespace DistributedDB {
class StorageProxy {
public:
    StorageProxy(ICloudSyncStorageInterface *iCloud);
    virtual ~StorageProxy() {};

    static std::shared_ptr<StorageProxy> GetCloudDb(ICloudSyncStorageInterface *iCloud);

    int Close();

    int GetLocalWaterMark(const std::string &tableName, Timestamp &localMark);

    int GetLocalWaterMarkByMode(const std::string &tableName, CloudWaterType mode, Timestamp &localMark);

    int PutLocalWaterMark(const std::string &tableName, Timestamp &localMark);

    int PutWaterMarkByMode(const std::string &tableName, CloudWaterType mode, Timestamp &localMark);

    int GetCloudWaterMark(const std::string &tableName, std::string &cloudMark);

    int SetCloudWaterMark(const std::string &tableName, std::string &cloudMark);

    int StartTransaction(TransactType type = TransactType::DEFERRED, bool isAsyncDownload = false);

    int Commit(bool isAsyncDownload = false);

    int Rollback(bool isAsyncDownload = false);

    int GetUploadCount(const std::string &tableName, const Timestamp &timestamp, const bool isCloudForcePush,
        int64_t &count);

    int GetUploadCount(const QuerySyncObject &query, const bool isCloudForcePush, bool isCompensatedTask,
        bool isUseWaterMark, int64_t &count);

    int GetUploadCount(const QuerySyncObject &query, const Timestamp &localMark, bool isCloudForcePush,
        bool isCompensatedTask, int64_t &count);

    int GetCloudData(const std::string &tableName, const Timestamp &timeRange,
        ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult);

    int GetCloudData(const QuerySyncObject &querySyncObject, const Timestamp &timeRange,
        ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult);

    int GetCloudDataNext(ContinueToken &continueStmtToken, CloudSyncData &cloudDataResult) const;

    int GetCloudGid(const QuerySyncObject &querySyncObject, bool isCloudForcePush,
        bool isCompensatedTask, std::vector<std::string> &cloudGid);

    int GetInfoByPrimaryKeyOrGid(const std::string &tableName, const VBucket &vBucket, bool useTransaction,
        DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo);

    int PutCloudSyncData(const std::string &tableName, DownloadData &downloadData);

    int UpdateAssetStatusForAssetOnly(const std::string &tableName, VBucket &asset);

    int CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
        const RelationalSchemaObject &localSchema, std::vector<Asset> &assets);

    int ClearCloudLogVersion(const std::vector<std::string> &tableNameList);

    int CheckSchema(const TableName &tableName) const;

    int CheckSchema(std::vector<std::string> &tables);

    int GetPrimaryColNamesWithAssetsFields(const TableName &tableName, std::vector<std::string> &colNames,
        std::vector<Field> &assetFields);

    int NotifyChangedData(const std::string &deviceName, ChangedData &&changedData);

    int ReleaseContinueToken(ContinueToken &continueStmtToken);

    int FillCloudAssetForDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess);

    int FillCloudAssetForAsyncDownload(const std::string &tableName, VBucket &asset, bool isDownloadSuccess);

    int SetLogTriggerStatus(bool status, bool isAsyncDownload = false);

    int SetCursorIncFlag(bool flag);

    int GetCursor(const std::string &tableName, uint64_t &cursor);

    int FillCloudLogAndAsset(OpType opType, const CloudSyncData &data);

    std::string GetIdentify() const;

    int CleanWaterMark(const TableName &tableName);

    int CleanWaterMarkInMemory(const TableName &tableName);

    int CreateTempSyncTrigger(const std::string &tableName);

    int ClearAllTempSyncTrigger();

    int IsSharedTable(const std::string &tableName, bool &isSharedTable);

    void FillCloudGidIfSuccess(const OpType opType, const CloudSyncData &data);

    std::pair<int, uint32_t> GetAssetsByGidOrHashKey(const std::string &tableName, bool isAsyncDownload,
        const std::string &gid, const Bytes &hashKey, VBucket &assets);

    int SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader);

    int UpdateRecordFlag(const std::string &tableName, bool isAsyncDownload, bool recordConflict,
        const LogInfo &logInfo);

    int GetCompensatedSyncQuery(std::vector<QuerySyncObject> &syncQuery, std::vector<std::string> &users,
        bool isQueryDownloadRecords);

    int ClearUnLockingNoNeedCompensated();

    int MarkFlagAsConsistent(const std::string &tableName, const DownloadData &downloadData,
        const std::set<std::string> &gidFilters);

    int MarkFlagAsAssetAsyncDownload(const std::string &tableName, const DownloadData &downloadData,
        const std::set<std::string> &gidFilters);

    void SetUser(const std::string &user);

    void OnSyncFinish();

    void OnUploadStart();

    void CleanAllWaterMark();

    std::string AppendWithUserIfNeed(const std::string &source) const;

    int GetCloudDbSchema(std::shared_ptr<DataBaseSchema> &cloudSchema);

    std::pair<int, CloudSyncData> GetLocalCloudVersion();

    CloudSyncConfig GetCloudSyncConfig() const;

    bool IsTableExistReference(const std::string &table);

    bool IsTableExistReferenceOrReferenceBy(const std::string &table);

    void ReleaseUploadRecord(const std::string &table, const CloudWaterType &type, Timestamp localWaterMark);

    bool IsTagCloudUpdateLocal(const LogInfo &localInfo, const LogInfo &cloudInfo,
        SingleVerConflictResolvePolicy policy);

    int ReviseLocalModTime(const std::string &tableName, const std::vector<ReviseModTimeInfo> &revisedData);

    bool IsCurrentLogicDelete() const;

    int GetLocalDataCount(const std::string &tableName, int &dataCount, int &logicDeleteDataCount);

    std::pair<int, std::vector<std::string>> GetDownloadAssetTable();

    std::pair<int, std::vector<std::string>> GetDownloadAssetRecords(const std::string &tableName,
        int64_t beginTime);

    void BeforeUploadTransaction();

    void PrintCursorChange(const std::string &tableName);

    int GetLockStatusByGid(const std::string &tableName, const std::string &gid, LockStatus &status);

    bool IsExistTableContainAssets();

    bool GetTransactionExeFlag();
protected:
    void Init();

    static Timestamp EraseNanoTime(Timestamp localTime);
private:
    ICloudSyncStorageInterface *store_;
    mutable std::shared_mutex storeMutex_;
    mutable std::shared_mutex cloudDbMutex_;
    std::atomic<bool> transactionExeFlag_;
    std::shared_ptr<CloudMetaData> cloudMetaData_;
    std::atomic<bool> isWrite_;
    std::string user_;
};
}

#endif // STORAGE_PROXY_H
