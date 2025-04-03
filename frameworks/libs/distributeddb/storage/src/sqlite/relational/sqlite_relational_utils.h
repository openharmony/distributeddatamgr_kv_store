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
        SQLiteSingleVerRelationalStorageExecutor *executor);

    static DistributedSchema FilterRepeatDefine(const DistributedSchema &schema);

    static DistributedTable FilterRepeatDefine(const DistributedTable &table);

    static int GetLogData(sqlite3_stmt *logStatement, LogInfo &logInfo);

    static int GetLogInfoPre(sqlite3_stmt *queryStmt, DistributedTableMode mode, const DataItem &dataItem,
        LogInfo &logInfoGet);

    static int OperateDataStatus(sqlite3 *db, const std::vector<std::string> &tables);

    static int GetMetaLocalTimeOffset(sqlite3 *db, int64_t &timeOffset);

    static std::pair<int, std::string> GetCurrentVirtualTime(sqlite3 *db);

    static int CreateRelationalMetaTable(sqlite3 *db);

    static int GetKvData(sqlite3 *db, bool isMemory, const Key &key, Value &value);
    static int PutKvData(sqlite3 *db, bool isMemory, const Key &key, const Value &value);

    static int InitCursorToMeta(sqlite3 *db, bool isMemory, const std::string &tableName);
    static int SetLogTriggerStatus(sqlite3 *db, bool status);

    struct GenLogParam {
        sqlite3 *db = nullptr;
        bool isMemory = false;
        bool isTrackerTable = false;
    };
    static int GeneLogInfoForExistedData(const std::string &identity, const TableInfo &tableInfo,
        std::unique_ptr<SqliteLogTableManager> &logMgrPtr, GenLogParam &param);

    static int GetExistedDataTimeOffset(sqlite3 *db, const std::string &tableName, bool isMem, int64_t &timeOffset);

    static std::string GetExtendValue(const TrackerTable &trackerTable);

    static int CleanTrackerData(sqlite3 *db, const std::string &tableName, int64_t cursor, bool isOnlyTrackTable);
private:
    static int BindExtendStatementByType(sqlite3_stmt *statement, int cid, Type &typeVal);

    static int GetTypeValByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal);
    static int GetBlobByStatement(sqlite3_stmt *stmt, int cid, Type &typeVal);

    static int UpdateLocalDataModifyTime(sqlite3 *db, const std::string &table, const std::string &modifyTime);
};
} // namespace DistributedDB
#endif // SQLITE_RELATIONAL_UTILS_H
