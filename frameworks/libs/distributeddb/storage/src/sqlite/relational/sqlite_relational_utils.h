/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef SQLITE_RELATIONAL_UTILS_H
#define SQLITE_RELATIONAL_UTILS_H

#include <vector>
#include "cloud/cloud_store_types.h"
#include "data_value.h"
#include "sqlite_import.h"
#include "sqlite_single_ver_relational_storage_executor.h"
#include "table_info.h"

namespace DistributedDB {
class SQLiteRelationalUtils {
public:
    static int GetDataValueByType(sqlite3_stmt *statement, int cid, DataValue &value);

    static std::vector<DataValue> GetSelectValues(sqlite3_stmt *stmt);

    static int GetCloudValueByType(sqlite3_stmt *statement, int type, int cid, Type &cloudValue);

    static void CalCloudValueLen(Type &cloudValue, uint32_t &totalSize);

    static int BindStatementByType(sqlite3_stmt *statement, int cid, Type &typeVal);

    static int GetSelectVBucket(sqlite3_stmt *stmt, VBucket &bucket);

    static bool GetDbFileName(sqlite3 *db, std::string &fileName);

    static int SelectServerObserver(sqlite3 *db, const std::string &tableName, bool isChanged);

    static void AddUpgradeSqlToList(const TableInfo &tableInfo,
        const std::vector<std::pair<std::string, std::string>> &fieldList, std::vector<std::string> &sqlList);

    static int AnalysisTrackerTable(sqlite3 *db, const TrackerTable &trackerTable, TableInfo &tableInfo);

    static int QueryCount(sqlite3 *db, const std::string &tableName, int64_t &count);

    static int GetCursor(sqlite3 *db, const std::string &tableName, uint64_t &cursor);

    static int CheckDistributedSchemaValid(const RelationalSchemaObject &schemaObj, const DistributedSchema &schema,
        bool isForceUpgrade, SQLiteSingleVerRelationalStorageExecutor *executor);

    static DistributedSchema FilterRepeatDefine(const DistributedSchema &schema);

    static DistributedTable FilterRepeatDefine(const DistributedTable &table);

    static int GetLogData(sqlite3_stmt *logStatement, LogInfo &logInfo);

    static int GetLogInfoPre(sqlite3_stmt *queryStmt, DistributedTableMode mode, const DataItem &dataItem,
        LogInfo &logInfoGet);

    static int OperateDataStatus(sqlite3 *db, const std::vector<std::string> &tables, uint64_t virtualTime);

    static int GetMetaLocalTimeOffset(sqlite3 *db, int64_t &timeOffset);

    static std::pair<int, std::string> GetCurrentVirtualTime(sqlite3 *db);

    static int CreateRelationalMetaTable(sqlite3 *db);

    static int GetKvData(sqlite3 *db, bool isMemory, const Key &key, Value &value);
    static int PutKvData(sqlite3 *db, bool isMemory, const Key &key, const Value &value);

    static int InitCursorToMeta(sqlite3 *db, bool isMemory, const std::string &tableName);
    static int InitKnowledgeTableTypeToMeta(sqlite3 *db, bool isMemory, const std::string &tableName);
    static int SetLogTriggerStatus(sqlite3 *db, bool status);

    struct GenLogParam {
        sqlite3 *db = nullptr;
        bool isMemory = false;
        bool isTrackerTable = false;
    };

    static int GeneTimeStrForLog(const TableInfo &tableInfo, GenLogParam &param, std::string &timeStr);

    static int GeneLogInfoForExistedData(const std::string &identity, const TableInfo &tableInfo,
        std::unique_ptr<SqliteLogTableManager> &logMgrPtr, GenLogParam &param);

    static int GetExistedDataTimeOffset(sqlite3 *db, const std::string &tableName, bool isMem, int64_t &timeOffset);

    static std::string GetExtendValue(const TrackerTable &trackerTable);

    static int CleanTrackerData(sqlite3 *db, const std::string &tableName, int64_t cursor, bool isOnlyTrackTable);

    static std::pair<int, bool> CheckExistDirtyLog(sqlite3 *db, const std::string &oriTable);

    static int CleanDirtyLog(sqlite3 *db, const std::string &oriTable, const RelationalSchemaObject &obj);

    static std::pair<int, bool> ExecuteCheckSql(sqlite3 *db, const std::string &sql);

    static int ExecuteSql(sqlite3 *db, const std::string &sql,
        const std::function<void(sqlite3_stmt *stmt)> &checkFunc);

    static int ExecuteListAction(const std::vector<std::function<int()>> &actions);

    static int GetLocalLogInfo(const RelationalSyncDataInserter &inserter, const DataItem &dataItem,
        DistributedTableMode mode, LogInfo &logInfoGet, SaveSyncDataStmt &saveStmt);

    static int GetLocalLog(const DataItem &dataItem, const RelationalSyncDataInserter &inserter, sqlite3_stmt *stmt,
        LogInfo &logInfo);

    static std::pair<int, VBucket> GetDistributedPk(const DataItem &dataItem,
        const RelationalSyncDataInserter &inserter);

    static int BindDistributedPk(sqlite3_stmt *stmt, const RelationalSyncDataInserter &inserter,
        VBucket &distributedPk);

    static int BindOneField(sqlite3_stmt *stmt, int bindIdx, const FieldInfo &fieldInfo, VBucket &distributedPk);

    static std::string GetMismatchedDataKeys(sqlite3 *db, const std::string &tableName);

    static void DeleteMismatchLog(sqlite3 *db, const std::string &tableName, const std::string &misDataKeys);

    static const std::string GetTempUpdateLogCursorTriggerSql(const std::string &tableName);

    static std::pair<int, TableInfo> AnalyzeTable(sqlite3 *db, const std::string &tableName);
#ifdef USE_DISTRIBUTEDDB_CLOUD
    static int PutCloudGid(sqlite3 *db, const std::string &tableName, std::vector<VBucket> &data);

    struct CloudNotExistRecord {
        int64_t logRowid = 0;
        int64_t dataRowid = 0;
        uint32_t flag = 0;
        Type pkValue;
    };

    static int GetOneBatchCloudNotExistRecord(const std::string &tableName, sqlite3 *db,
        std::vector<CloudNotExistRecord> &records, const std::string &dataPk);

    static int DeleteOneRecord(const std::string &tableName, sqlite3 *db, const CloudNotExistRecord &record,
        bool isLogicDelete, std::vector<Type> &changePk);

    static int DropTempTable(const std::string &tableName, sqlite3 *db);

    static int PutCloudGidInner(sqlite3_stmt *stmt, std::vector<VBucket> &data);

    static int CheckUserCreateSharedTable(sqlite3 *db, const TableSchema &oriTable, const std::string &sharedTable);

    static std::map<int32_t, std::string> GetCloudFieldDataType();
#endif
    static int DeleteDistributedExceptDeviceTable(sqlite3 *db, const std::string &removedTable,
        const std::vector<std::string> &keepDevices);

    static int DeleteDistributedExceptDeviceTableLog(sqlite3 *db, const std::string &removedTable,
        const std::vector<std::string> &keepDevices);

    static int UpdateTrackerTableSyncDelete(sqlite3 *db, const std::string &removedTable,
        const std::vector<std::string> &keepDevices);
private:
    static int BindExtendStatementByType(sqlite3_stmt *statement, int cid, Type &typeVal);

    static int GetTypeValByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal);
    static int GetBlobByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal);

    static int UpdateLocalDataModifyTime(sqlite3 *db, const std::string &table, const std::string &modifyTime);

#ifdef USE_DISTRIBUTEDDB_CLOUD
    static int CheckUserCreateSharedTableInner(const TableSchema &oriTable, const TableInfo &sharedTableInfo);
#endif
};
} // namespace DistributedDB
#endif // SQLITE_RELATIONAL_UTILS_H
