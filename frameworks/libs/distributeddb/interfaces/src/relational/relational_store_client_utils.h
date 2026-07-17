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

    static void StringToUpper(std::string &str);

    static int ArchiveSyncedData(sqlite3 *db, const std::string &tableName, uint64_t cursor);

    static int DeleteSyncedData(sqlite3 *db, const std::string &tableName,
        const std::vector<std::vector<Type>> &keys);

    static int CheckTable(sqlite3 *db, const std::string &tableName, bool isCheckTableMode, bool isTracker = false);

    static int CheckTable(sqlite3 *db, const std::string &tableName, const RelationalSchemaObject &rdbSchema,
        bool isCheckTableMode, bool isTracker);
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

    static int ArchiveSyncedDataInner(sqlite3 *db, const std::string &tableName, const TrackerTable &table,
        uint64_t cursor, bool isTracker);

    static int DeleteSyncedDataInner(sqlite3 *db, const std::string &tableName,
        const std::vector<std::vector<Type>> &keys);

    static std::pair<int, std::vector<uint8_t>> GetHashKey(const std::vector<Type> &keys);
};
} // namespace DistributedDB
#endif // RELATIONAL_STORE_CLIENT_UTILS_H