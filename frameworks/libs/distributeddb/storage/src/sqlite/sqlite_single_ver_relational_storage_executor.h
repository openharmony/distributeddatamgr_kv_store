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

#include "data_transformer.h"
#include "db_types.h"
#include "macro_utils.h"
#include "query_object.h"
#include "relational_row_data.h"
#include "relational_store_delegate.h"
#include "relational_sync_data_inserter.h"
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

    int GetMaxTimestamp(const std::vector<std::string> &tableNames, Timestamp &maxTimestamp) const;

    int ExecuteQueryBySqlStmt(const std::string &sql, const std::vector<std::string> &bindArgs, int packetSize,
        std::vector<std::string> &colNames, std::vector<RelationalRowData *> &data);

    int SaveSyncDataItems(RelationalSyncDataInserter &inserter);

    int CheckEncryptedOrCorrupted() const;

    int GetExistsDeviceList(std::set<std::string> &devices) const;

private:
    int GetDataItemForSync(sqlite3_stmt *stmt, DataItem &dataItem, bool isGettingDeletedData) const;

    int GetSyncDataPre(const DataItem &dataItem, sqlite3_stmt *queryStmt, DataItem &itemGet);

    int CheckDataConflictDefeated(const DataItem &dataItem, sqlite3_stmt *queryStmt,  bool &isDefeated);

    int SaveSyncDataItem(RelationalSyncDataInserter &inserter, SaveSyncDataStmt &saveStmt, DataItem &item);

    int SaveSyncDataItem(const DataItem &dataItem, SaveSyncDataStmt &saveStmt, RelationalSyncDataInserter &inserter,
        int64_t &rowid);

    int DeleteSyncDataItem(const DataItem &dataItem, RelationalSyncDataInserter &inserter, sqlite3_stmt *&rmDataStmt);

    int GetLogInfoPre(sqlite3_stmt *queryStmt, const DataItem &dataItem, LogInfo &logInfoGet);

    int SaveSyncLog(sqlite3_stmt *statement, sqlite3_stmt *queryStmt, const DataItem &dataItem, int64_t rowid);

    int AlterAuxTableForUpgrade(const TableInfo &oldTableInfo, const TableInfo &newTableInfo);

    int DeleteSyncLog(const DataItem &dataItem, RelationalSyncDataInserter &inserter, sqlite3_stmt *&rmLogStmt);
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

    std::string baseTblName_;
    TableInfo table_;  // Always operating table, user table when get, device table when put.

    DistributedTableMode mode_;
};
} // namespace DistributedDB
#endif
#endif // SQLITE_SINGLE_VER_RELATIONAL_STORAGE_EXECUTOR_H