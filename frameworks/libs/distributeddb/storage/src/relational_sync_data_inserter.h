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

#ifndef RELATIONAL_SYNC_DATA_INSERTER_H
#define RELATIONAL_SYNC_DATA_INSERTER_H

#include <vector>
#include "db_types.h"
#include "query_object.h"
#include "relational_schema_object.h"

namespace DistributedDB {

struct SaveSyncDataStmt {
    sqlite3_stmt *saveDataStmt = nullptr;
    sqlite3_stmt *saveLogStmt = nullptr;
    sqlite3_stmt *queryStmt = nullptr;
    sqlite3_stmt *rmDataStmt = nullptr;
    sqlite3_stmt *rmLogStmt = nullptr;
    SaveSyncDataStmt() {}
    ~SaveSyncDataStmt()
    {
        ResetStatements(true);
    }

    int ResetStatements(bool isNeedFinalize);
};

class RelationalSyncDataInserter {
public:
    RelationalSyncDataInserter();
    ~RelationalSyncDataInserter();

    static RelationalSyncDataInserter CreateInserter(const std::string &deviceName, const QueryObject &query,
        const RelationalSchemaObject &localSchema, const std::vector<FieldInfo> &remoteFields,
        const StoreInfo &info);

    void SetHashDevId(const std::string &hashDevId);
    // Set remote fields in cid order
    void SetRemoteFields(std::vector<FieldInfo> remoteFields);
    void SetEntries(std::vector<DataItem> entries);

    void SetLocalTable(TableInfo localTable);
    const TableInfo &GetLocalTable() const;

    void SetQuery(QueryObject query);
    void SetInsertTableName(std::string tableName);

    void SetTableMode(DistributedTableMode mode);

    int Iterate(const std::function<int (DataItem &)> &);
    int BindInsertStatement(sqlite3_stmt *stmt, const DataItem &dataItem);

    int PrepareStatement(sqlite3 *db, SaveSyncDataStmt &stmt);
    int GetDeleteLogStmt(sqlite3 *db, sqlite3_stmt *&stmt);
    int GetDeleteSyncDataStmt(sqlite3 *db, sqlite3_stmt *&stmt);

private:

    int GetInsertStatement(sqlite3 *db, sqlite3_stmt *&stmt);

    int GetSaveLogStatement(sqlite3 *db, sqlite3_stmt *&logStmt, sqlite3_stmt *&queryStmt);

    std::string hashDevId_;
    std::vector<FieldInfo> remoteFields_;
    std::vector<DataItem> entries_;
    TableInfo localTable_;
    QueryObject query_;
    std::string insertTableName_; // table name to save sync data
    DistributedTableMode mode_ = DistributedTableMode::SPLIT_BY_DEVICE;
};
}
#endif // RELATIONAL_SYNC_DATA_INSERTER_H