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
#ifndef SQLITE_SINGLE_VER_RELATIONAL_STORAGE_EXECUTOR_H
#define SQLITE_SINGLE_VER_RELATIONAL_STORAGE_EXECUTOR_H
#ifdef RELATIONAL_STORE

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_store_types.h"
#include "data_transformer.h"
#include "db_types.h"
#include "icloud_sync_storage_interface.h"
#include "macro_utils.h"
#include "query_object.h"
#include "relational_row_data.h"
#include "relational_store_delegate.h"
#include "relational_sync_data_inserter.h"
#include "sqlite_single_ver_relational_continue_token.h"
#include "sqlite_storage_executor.h"
#include "sqlite_utils.h"

namespace DistributedDB {
class SQLiteSingleVerRelationalStorageExecutor : public SQLiteStorageExecutor {
public:
    SQLiteSingleVerRelationalStorageExecutor(sqlite3 *dbHandle, bool writable, DistributedTableMode mode);
    ~SQLiteSingleVerRelationalStorageExecutor() override = default;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteSingleVerRelationalStorageExecutor);

    // The parameter "identity" is a hash string that identifies a device
    int CreateDistributedTable(DistributedTableMode mode, bool isUpgraded, const std::string &identity,
        TableInfo &table, TableSyncType syncType);

    int UpgradeDistributedTable(const std::string &tableName, DistributedTableMode mode, bool &schemaChanged,
        RelationalSchemaObject &schema);

    int StartTransaction(TransactType type);
    int Commit();
    int Rollback();

    // For Get sync data
    int GetSyncDataByQuery(std::vector<DataItem> &dataItems, size_t appendLength, const DataSizeSpecInfo &sizeInfo,
        std::function<int(sqlite3 *, sqlite3_stmt *&, sqlite3_stmt *&, bool &)> getStmt, const TableInfo &tableInfo);

    // operation of meta data
    int GetKvData(const Key &key, Value &value) const;
    int PutKvData(const Key &key, const Value &value) const;
    int DeleteMetaData(const std::vector<Key> &keys) const;
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const;
    int GetAllMetaKeys(std::vector<Key> &keys) const;

    // For Put sync data
    int SaveSyncItems(RelationalSyncDataInserter &inserter, bool useTrans = true);

    int CheckDBModeForRelational();

    int DeleteDistributedDeviceTable(const std::string &device, const std::string &tableName);

    int DeleteDistributedAllDeviceTableLog(const std::string &tableName);

    int DeleteDistributedDeviceTableLog(const std::string &device, const std::string &tableName);

    int DeleteDistributedLogTable(const std::string &tableName);

    int CheckAndCleanDistributedTable(const std::vector<std::string> &tableNames,
        std::vector<std::string> &missingTables);

    int CreateDistributedDeviceTable(const std::string &device, const TableInfo &baseTbl, const StoreInfo &info);

    int CheckQueryObjectLegal(const TableInfo &table, QueryObject &query, const std::string &schemaVersion);

    int GetMaxTimestamp(const std::vector<std::string> &tablesName, Timestamp &maxTimestamp) const;

    int ExecuteQueryBySqlStmt(const std::string &sql, const std::vector<std::string> &bindArgs, int packetSize,
        std::vector<std::string> &colNames, std::vector<RelationalRowData *> &data);

    int SaveSyncDataItems(RelationalSyncDataInserter &inserter);

    int CheckEncryptedOrCorrupted() const;

    int GetExistsDeviceList(std::set<std::string> &devices) const;

    int GetUploadCount(const std::string &tableName, const Timestamp &timestamp, const bool isCloudForcePush,
        int64_t &count);

    int UpdateCloudLogGid(const CloudSyncData &cloudDataResult);

    int GetSyncCloudData(CloudSyncData &cloudDataResult, const uint32_t &maxSize,
        SQLiteSingleVerRelationalContinueToken &token);

    int GetInfoByPrimaryKeyOrGid(const TableSchema &tableSchema, const VBucket &vBucket,
        DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo);

    int PutCloudSyncData(const std::string &tableName, const TableSchema &tableSchema, DownloadData &downloadData);

    int FillCloudAsset(const TableSchema &tableSchema, VBucket &vBucket, bool isFullReplace);
    int DoCleanInner(ClearMode mode, const std::vector<std::string> &tableNameList,
        const std::vector<TableSchema> &tableSchemaList, std::vector<Asset> &assets);

    int FillCloudAssetForUpload(const CloudSyncData &data);

private:
    int DoCleanLogs(const std::vector<std::string> &tableNameList);

    int DoCleanLogAndData(const std::vector<std::string> &tableNameList,
        const std::vector<TableSchema> &tableSchemaList, std::vector<Asset> &assets);

    int CleanCloudDataOnLogTable(const std::string &logTableName);

    int CleanCloudDataAndLogOnUserTable(const std::string &tableName, const std::string &logTableName);

    int GetCleanCloudDataKeys(const std::string &logTableName, std::vector<int64_t> &dataKeys);

    int GetCloudAssets(const std::string &tableName, const std::vector<AssetField> assetFields,
        const std::vector<int64_t> &datakeys, std::vector<Asset> &assets);

    int GetCloudAssetOnTable(const std::string &tableName,
        const AssetField &assetField, const std::vector<int64_t> &datakeys, std::vector<Asset> &assets);

    int GetCloudAssetsOnTable(const std::string &tableName,
        const AssetField &assetField, const std::vector<int64_t> &datakeys, std::vector<Asset> &assets);

    int GetDataItemForSync(sqlite3_stmt *statement, DataItem &dataItem, bool isGettingDeletedData) const;

    int GetSyncDataPre(const DataItem &dataItem, sqlite3_stmt *queryStmt, DataItem &itemGet);

    int CheckDataConflictDefeated(const DataItem &item, sqlite3_stmt *queryStmt,  bool &isDefeated);

    int SaveSyncDataItem(RelationalSyncDataInserter &inserter, SaveSyncDataStmt &saveStmt, DataItem &item);

    int SaveSyncDataItem(const DataItem &dataItem, SaveSyncDataStmt &saveStmt, RelationalSyncDataInserter &inserter,
        int64_t &rowid);

    int DeleteSyncDataItem(const DataItem &dataItem, RelationalSyncDataInserter &inserter, sqlite3_stmt *&rmDataStmt);

    int GetLogInfoPre(sqlite3_stmt *queryStmt, const DataItem &dataItem, LogInfo &logInfoGet);

    int SaveSyncLog(sqlite3_stmt *statement, sqlite3_stmt *queryStmt, const DataItem &dataItem, int64_t rowid);

    int AlterAuxTableForUpgrade(const TableInfo &oldTableInfo, const TableInfo &newTableInfo);

    int DeleteSyncLog(const DataItem &item, RelationalSyncDataInserter &inserter, sqlite3_stmt *&rmLogStmt);
    int ProcessMissQueryData(const DataItem &item, RelationalSyncDataInserter &inserter, sqlite3_stmt *&rmDataStmt,
        sqlite3_stmt *&rmLogStmt);
    int GetMissQueryData(sqlite3_stmt *fullStmt, DataItem &item);
    int GetQueryDataAndStepNext(bool isFirstTime, bool isGettingDeletedData, sqlite3_stmt *queryStmt, DataItem &item,
        Timestamp &queryTime);
    int GetMissQueryDataAndStepNext(sqlite3_stmt *fullStmt, DataItem &item, Timestamp &missQueryTime);

    int SetLogTriggerStatus(bool status);

    void SetTableInfo(const TableInfo &tableInfo);  // When put or get sync data, must call the func first.

    int GeneLogInfoForExistedData(sqlite3 *db, const std::string &tableName, const TableInfo &table,
        const std::string &calPrimaryKeyHash);

    int GetCloudDataForSync(sqlite3_stmt *statement, CloudSyncData &cloudDataResult, uint32_t &totalSize,
        const uint32_t &maxSize);

    int PutVBucketByType(VBucket &vBucket, const Field &field, Type &cloudValue);

    int ExecutePutCloudData(const std::string &tableName, const TableSchema &tableSchema, DownloadData &downloadData,
        std::map<int, int> &statisticMap);

    std::string GetInsertSqlForCloudSync(const TableSchema &tableSchema);

    int GetPrimaryKeyHashValue(const VBucket &vBucket, const TableSchema &tableSchema, std::vector<uint8_t> &hashValue);

    int GetQueryLogStatement(const TableSchema &tableSchema, const VBucket &vBucket, const std::string &querySql,
        std::set<std::string> &pkSet, sqlite3_stmt *&selectStmt);

    int GetQueryLogSql(const std::string &tableName, const VBucket &vBucket, std::set<std::string> &pkSet,
        std::string &querySql);

    int GetQueryInfoSql(const std::string &tableName, const VBucket &vBucket, std::set<std::string> &pkSet,
        std::vector<Field> &assetFields, std::string &querySql);

    int GetQueryLogRowid(const std::string &tableName, const VBucket &vBucket, int64_t &rowId);

    int GetFillDownloadAssetStatement(const std::string &tableName, const VBucket &vBucket,
        const std::vector<Field> &fields, bool isFullReplace, sqlite3_stmt *&statement);

    int GetFillUploadAssetStatement(const std::string &tableName, const VBucket &vBucket, sqlite3_stmt *&statement);

    int CalculateHashKeyForOneField(const Field &field, const VBucket &vBucket, std::vector<uint8_t> &hashValue);

    void GetLogInfoByStatement(sqlite3_stmt *statement, LogInfo &logInfo);

    int GetInfoByStatement(sqlite3_stmt *statement, std::vector<Field> &assetFields,
        const std::map<std::string, Field> &pkMap, DataInfoWithLog &dataInfoWithLog, VBucket &assetInfo);

    int InsertCloudData(const std::string &tableName, VBucket &vBucket, const TableSchema &tableSchema);

    int InsertLogRecord(const TableSchema &tableSchema, VBucket &vBucket);

    int BindOneField(int index, const VBucket &vBucket, const Field &field, sqlite3_stmt *updateStmt);

    int BindValueToUpsertStatement(const VBucket &vBucket,  const std::vector<Field> &fields, sqlite3_stmt *upsertStmt);

    int BindHashKeyAndGidToInsertLogStatement(const VBucket &vBucket, const TableSchema &tableSchema,
        sqlite3_stmt *insertLogStmt);

    int BindValueToInsertLogStatement(VBucket &vBucket, const TableSchema &tableSchema, sqlite3_stmt *insertLogStmt);

    std::string GetWhereConditionForDataTable(const std::string &gidStr, const std::set<std::string> &pkSet,
        const std::string &tableName);

    int GetUpdateSqlForCloudSync(const TableSchema &tableSchema, const VBucket &vBucket, const std::string &gidStr,
        const std::set<std::string> &pkSet, std::string &updateSql);

    int GetUpdateDataTableStatement(const VBucket &vBucket, const TableSchema &tableSchema, sqlite3_stmt *&updateStmt);

    int UpdateCloudData(const std::string &tableName, const VBucket &vBucket, const TableSchema &tableSchema);

    int GetUpdateLogRecordStatement(const TableSchema &tableSchema, const VBucket &vBucket, OpType opType,
        std::vector<std::string> &updateColName, sqlite3_stmt *&updateLogStmt);

    int UpdateLogRecord(const VBucket &vBucket, const TableSchema &tableSchema, OpType opType);

    int BindValueToUpdateLogStatement(const VBucket &vBucket, const TableSchema &tableSchema,
        std::vector<std::string> &colNames, std::map<std::string, Field> &pkMap, sqlite3_stmt *updateLogStmt);

    int GetDeleteStatementForCloudSync(const TableSchema &tableSchema, const std::set<std::string> &pkSet,
        const VBucket &vBucket, sqlite3_stmt *&deleteStmt);

    int DeleteCloudData(const std::string &tableName, const VBucket &vBucket, const TableSchema &tableSchema);

    int UpdateCloudGidOrFlag(const VBucket &vBucket, const TableSchema &tableSchema, OpType opType);

    std::string baseTblName_;
    TableInfo table_;  // Always operating table, user table when get, device table when put.
    TableSchema tableSchema_; // for cloud table

    DistributedTableMode mode_;

    std::map<int32_t, std::function<int(int, const VBucket &, const Field &, sqlite3_stmt *)>> bindCloudFieldFuncMap_;
    std::map<int32_t, std::function<int(const VBucket &, const Field &, std::vector<uint8_t> &)>> toVectorFuncMap_;
};
} // namespace DistributedDB
#endif
#endif // SQLITE_SINGLE_VER_RELATIONAL_STORAGE_EXECUTOR_H