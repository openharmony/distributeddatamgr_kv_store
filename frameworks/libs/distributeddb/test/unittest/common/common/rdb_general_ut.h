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
#include "virtual_asset_loader.h"

namespace DistributedDB {
const std::string g_defaultTable1 = "defaultTable1";
const std::string g_defaultTable2 = "defaultTable2";
const std::vector<uint8_t> PASSWD_VECTOR = {'P', 'a', 's', 's', 'w', 'o', 'r', 'd', '@', '1'};

class RDBGeneralUt : public BasicUnitTest {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    int InitDelegate(const StoreInfo &info) override;
    std::pair<int, RelationalStoreDelegate *> OpenRDBStore(const StoreInfo &info);
    int CloseDelegate(const StoreInfo &info) override;
    void CloseAllDelegate() override;

    // If SetOption is not invoked before InitDelegate is invoked, the default data of Option is used to open store.
    void SetOption(const RelationalStoreDelegate::Option& option);
    // If SetSchemaInfo is not invoked before InitDelegate is invoked, g_defaultSchemaInfo is used to create table.
    void SetSchemaInfo(const StoreInfo &info, const DistributedDBUnitTest::UtDateBaseSchemaInfo& schemaInfo);

    DistributedDBUnitTest::UtDateBaseSchemaInfo GetTableSchemaInfo(const StoreInfo &info) const;
    DataBaseSchema GetSchema(const StoreInfo &info) const;
    TableSchema GetTableSchema(const StoreInfo &info, const std::string &tableName = g_defaultTable1) const;
    std::vector<TrackerSchema> GetAllTrackerSchema(const StoreInfo &info, const std::vector<std::string> &tables) const;
    sqlite3 *GetSqliteHandle(const StoreInfo &info) const;
    RelationalStoreDelegate *GetDelegate(const StoreInfo &info)  const;
    RelationalStoreDelegate::Option GetOption() const;

    int InitDatabase(const StoreInfo &info);

    int InsertLocalDBData(int64_t begin, int64_t count, const StoreInfo &info);

    int ExecuteSQL(const std::string &sql, const StoreInfo &info);

    int CreateDistributedTable(const StoreInfo &info, const std::string &table,
        TableSyncType type = TableSyncType::DEVICE_COOPERATION);

    int SetDistributedTables(const StoreInfo &info, const std::vector<std::string> &tables,
        TableSyncType type = TableSyncType::DEVICE_COOPERATION);

    // use for device to device sync
    void BlockPush(const StoreInfo &from, const StoreInfo &to, const std::string &table,
        DBStatus expectRet = DBStatus::OK);

    void BlockPull(const StoreInfo &from, const StoreInfo &to, const std::string &table,
        DBStatus expectRet = DBStatus::OK);

    int CountTableData(const StoreInfo &info, const std::string &table, const std::string &condition = "");

    int CountTableDataByDev(const StoreInfo &info, const std::string &table, const std::string &dev,
        const std::string &condition = "");

    int SetTrackerTables(const StoreInfo &info, const std::vector<std::string> &tables);

    // use for cloud sync
    std::shared_ptr<VirtualCloudDb> GetVirtualCloudDb() const;

    std::shared_ptr<VirtualAssetLoader> GetVirtualAssetLoader() const;

    void CloudBlockSync(const StoreInfo &from, const Query &query, DBStatus exceptStatus = OK,
        DBStatus callbackExpect = OK);

    void SetCloudDbConfig(const StoreInfo &info) const;

    int GetCloudDataCount(const std::string &tableName) const;

    void SetIsDbEncrypted(bool isdbEncrypted);
    bool GetIsDbEncrypted() const;
    int EncryptedDb(sqlite3 *db);

    void BlockSync(const StoreInfo &from, const StoreInfo &to, const std::string &table, SyncMode mode,
        DBStatus expectRet);

    void RemoteQuery(const StoreInfo &from, const StoreInfo &to, const std::string &sql, DBStatus expectRet);

    mutable std::mutex storeMutex_;
    std::map<StoreInfo, RelationalStoreDelegate *> stores_;
    std::map<StoreInfo, sqlite3 *> sqliteDb_;
    std::map<StoreInfo, DistributedDBUnitTest::UtDateBaseSchemaInfo> schemaInfoMap_;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader_ = nullptr;
    RelationalStoreDelegate::Option option_;
    bool isDbEncrypted_ = false;
};
}
#endif // RDB_GENERAL_UT_H
