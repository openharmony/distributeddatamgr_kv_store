/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef RDB_GENERAL_UT_H
#define RDB_GENERAL_UT_H

#include "basic_unit_test.h"
#include "rdb_data_generator.h"
#include "relational_store_manager.h"

namespace DistributedDB {
const std::string g_defaultTable1 = "defaultTable1";
const std::string g_defaultTable2 = "defaultTable2";

class RDBGeneralUt : public BasicUnitTest {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    int InitDelegate(const StoreInfo &info) override;
    int CloseDelegate(const StoreInfo &info) override;
    void CloseAllDelegate() override;

    // If SetOption is not invoked before InitDelegate is invoked, the default data of Option is used to open store.
    void SetOption(const RelationalStoreDelegate::Option& option);
    // If AddSchemaInfo is not invoked before InitDelegate is invoked, g_defaultSchemaInfo is used to create table.
    void AddSchemaInfo(const StoreInfo &info, const DistributedDBUnitTest::UtDateBaseSchemaInfo& schemaInfo);
    DataBaseSchema GetSchema(const StoreInfo &info);
    TableSchema GetTableSchema(const StoreInfo &info, const std::string &tableName = g_defaultTable1);
    std::vector<TrackerSchema> GetAllTrackerSchema(const StoreInfo &info, const std::vector<std::string> &tables);

    int InitDatabase(const StoreInfo &info);

    int InsertLocalDBData(int64_t begin, int64_t count, const StoreInfo &info);

    int CreateDistributedTable(const StoreInfo &info, const std::string &table,
        TableSyncType type = TableSyncType::DEVICE_COOPERATION);

    int SetDistributedTables(const StoreInfo &info, const std::vector<std::string> &tables,
        TableSyncType type = TableSyncType::DEVICE_COOPERATION);

    void BlockPush(const StoreInfo &from, const StoreInfo &to, const std::string &table,
        DBStatus expectRet = DBStatus::OK);

    DistributedDBUnitTest::UtDateBaseSchemaInfo GetTableSchemaInfo(const StoreInfo &info);
    sqlite3 *GetSqliteHandle(const StoreInfo &info);
    RelationalStoreDelegate *GetDelegate(const StoreInfo &info);

    int CountTableData(const StoreInfo &info, const std::string &table);

    int CountTableDataByDev(const StoreInfo &info, const std::string &table, const std::string &dev);

    int SetTrackerTables(const StoreInfo &info, const std::vector<std::string> &tables);

    RelationalStoreDelegate::Option option_;
    mutable std::mutex storeMutex_;
    std::map<StoreInfo, RelationalStoreDelegate *, StoreComparator> stores_;
    std::map<StoreInfo, sqlite3 *, StoreComparator> sqliteDb_;
    std::map<StoreInfo, DistributedDBUnitTest::UtDateBaseSchemaInfo, StoreComparator> schemaInfoMap_;
};
}
#endif // RDB_GENERAL_UT_H
