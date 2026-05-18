/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef RELATIONAL_STORE_CLIENT_UTILS_H
#define RELATIONAL_STORE_CLIENT_UTILS_H

#include "relational_schema_object.h"
#include "relational_store_client.h"
#include "store_types.h"

namespace DistributedDB {
class RelationalStoreClientUtils {
public:
    static int UpdateDataLog(sqlite3 *db, const UpdateOption &option);

    static std::pair<int, RelationalSchemaObject> GetRDBSchema(sqlite3 *db, bool isTracker);

    static std::string GetInsertTrigger(const std::string &tableName, bool isRowid, const std::string &primaryKey);

    static std::string GetUpdateTrigger(const std::string &tableName, bool isRowid, const std::string &primaryKey);

    static std::string GetDeleteTrigger(const std::string &tableName, bool isRowid, const std::string &primaryKey);

    static MonitorTablesConfig *BinlogSchemaGet(const char *dbPath);

    static void StringToUpper(std::string &str);
private:
    static int CheckUpdateOption(sqlite3 *db, const UpdateOption &option);

    static int CheckSelectCondition(const std::optional<SelectCondition> &condition, const std::string &dfxLog);

    static int CheckUpdateContent(const UpdateContent &content);

    static int UpdateDataLogInner(sqlite3 *db, const UpdateOption &option);

    static int UpdateDataLogInTransaction(sqlite3 *db, const UpdateOption &option);

    static std::string GetUpdateSQL(const UpdateOption &option);

    static std::string GetUpdateLogSQL(const UpdateOption &option);

    static int BindDataLogValue(sqlite3_stmt *stmt, const UpdateOption &option);

    static int BindDataLogCondition(sqlite3_stmt *stmt, const std::optional<SelectCondition> &condition,
        bool isLog, int &index);

    static int GetTableAndColumnName(const JsonObject &jsonValue, std::string &tableName, std::string &columnName);
    
    static int InitNewTableEntry(MonitorTableCol &table, const std::string &tableName, const std::string &columnName);

    static int TryAddColumnToTable(MonitorTableCol &table, const std::string &columnName);

    static int AddColumnsToMonitor(const JsonObject &jsonValue, MonitorTablesConfig *monitorConfig);

    static int ReadJsonConfigFromFile(const std::string &dbPath, std::string &jsonStr);

    static int ParseSearchConfig(const std::string &jsonStr, JsonObject &searchConfig);

    static int ProcessMappings(const JsonObject &part, MonitorTablesConfig *monitorConfig);

    static int ProcessUTDMapping(const JsonObject &utdMapping, MonitorTablesConfig *monitorConfig);

    static int GetMonitorConfigFromFile(MonitorTablesConfig *monitorConfig, const std::string &dbPath);
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE_CLIENT_UTILS_H