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
#include "data_donation_utils.h"
#include "rdb_general_ut.h"
#include "sqlite_relational_utils.h"
#include "relational_store_client.h"
#include "relational_store_client_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

#ifdef USE_DISTRIBUTEDDB_CLOUD
namespace {
const std::string g_deviceA = "dev1";

class DistributedDBRdbFullCloudSyncTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;

    void LocalInsertAndSync(int count);
    void CloudInsertAndSync(int count);
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema();
protected:
    static constexpr const char *CLOUD_SYNC_TABLE = "CLOUD_SYNC_TABLE";
};

void DistributedDBRdbFullCloudSyncTest::SetUp()
{
    RDBGeneralUt::SetUp();
    auto info1 = GetStoreInfo1();
    RDBGeneralUt::SetSchemaInfo(info1, GetDefaultSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    ASSERT_EQ(SetDistributedTables(info1, {CLOUD_SYNC_TABLE}, TableSyncType::CLOUD_COOPERATION), E_OK);
}

void DistributedDBRdbFullCloudSyncTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

UtDateBaseSchemaInfo DistributedDBRdbFullCloudSyncTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema());
    return info;
}

UtTableSchemaInfo DistributedDBRdbFullCloudSyncTest::GetTableSchema()
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = CLOUD_SYNC_TABLE;
    UtFieldInfo field;
    field.field.colName = "id";
    field.field.type = TYPE_INDEX<int64_t>;
    field.field.primary = true;
    tableSchema.fieldInfo.push_back(field);
    return tableSchema;
}

void DistributedDBRdbFullCloudSyncTest::LocalInsertAndSync(int count)
{
    // step1 local insert data
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(InsertLocalDBData(0, count, info1), E_OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), count);
    // step2 cloud sync
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE});
    ASSERT_NO_FATAL_FAILURE(RDBGeneralUt::CloudBlockSync(info1, query));
}

void DistributedDBRdbFullCloudSyncTest::CloudInsertAndSync(int count)
{
    // step1 cloud insert data
    auto info1 = GetStoreInfo1();
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, count, 0, RDBGeneralUt::GetSchema(info1), virtualCloudDb), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), 0);
    // step2 cloud sync
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE});
    ASSERT_NO_FATAL_FAILURE(RDBGeneralUt::CloudBlockSync(info1, query));
}

/**
 * @tc.name: ArchiveSyncedData001
 * @tc.desc: Test archive local data after cloud sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, ArchiveSyncedData001, TestSize.Level0)
{
    const int count = 2;
    EXPECT_NO_FATAL_FAILURE(LocalInsertAndSync(count));
    // step3 archive synced data with cursor 1
    auto info1 = GetStoreInfo1();
    auto db = GetSqliteHandle(info1);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 1), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), 0); // archive 2 row and exist 0 row
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE)), count);
    EXPECT_EQ(SetTrackerTables(info1, {CLOUD_SYNC_TABLE}), WITH_INVENTORY_DATA);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 1), OK);
}

/**
 * @tc.name: ArchiveSyncedData002
 * @tc.desc: Test archive cloud data after cloud sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, ArchiveSyncedData002, TestSize.Level0)
{
    const int count = 2;
    EXPECT_NO_FATAL_FAILURE(CloudInsertAndSync(count));
    // step3 archive synced data with cursor 1
    auto info1 = GetStoreInfo1();
    auto db = GetSqliteHandle(info1);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 1), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), 1); // archive 1 row and exist 1 row
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE)), count);
    // step4 archive synced data with cursor 2
    db = GetSqliteHandle(info1);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 2), OK); // 2 is last data cursor
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), 0); // archive 1 row and exist 0 row
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE)), count);
}

/**
 * @tc.name: ArchiveSyncedData003
 * @tc.desc: Test archive local sync data and insert again.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, ArchiveSyncedData003, TestSize.Level1)
{
    auto info1 = GetStoreInfo1();
    const int count = 1;
    EXPECT_NO_FATAL_FAILURE(LocalInsertAndSync(count));
    // step3 archive synced data with cursor 1
    auto db = GetSqliteHandle(info1);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 1), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), 0); // archive 1 row and exist 0 row
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE)), count);
    // step4 insert local data and sync again
    ASSERT_EQ(InsertLocalDBData(0, count, info1), E_OK);
    auto cloud = GetVirtualCloudDb();
    ASSERT_NE(cloud, nullptr);
    auto before = cloud->GetUpdateCount();
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE});
    ASSERT_NO_FATAL_FAILURE(RDBGeneralUt::CloudBlockSync(info1, query));
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE),
        "cloud_gid=0 AND version=1"), count);
    auto after = cloud->GetUpdateCount();
    EXPECT_EQ(after - before, static_cast<size_t>(count));
}

/**
 * @tc.name: ArchiveSyncedData004
 * @tc.desc: Test archive local tracker data after cloud sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, ArchiveSyncedData004, TestSize.Level0)
{
    auto info1 = GetStoreInfo1();
    EXPECT_EQ(SetTrackerTables(info1, {CLOUD_SYNC_TABLE}), E_OK);
    const int count = 2;
    EXPECT_NO_FATAL_FAILURE(LocalInsertAndSync(count));
    // step3 archive synced data with cursor 1
    auto db = GetSqliteHandle(info1);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 1), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), 0); // archive 2 row and exist 0 row
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE)), count);
}

/**
 * @tc.name: DeleteSyncedData001
 * @tc.desc: Test delete local archived data after cloud sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, DeleteSyncedData001, TestSize.Level1)
{
    const int count = 2;
    EXPECT_NO_FATAL_FAILURE(LocalInsertAndSync(count));
    // step3 archive synced data with cursor 1
    auto info1 = GetStoreInfo1();
    auto db = GetSqliteHandle(info1);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 1), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), 0); // archive 2 row and exist 0 row
    std::string condition = "flag&" + DBCommon::FlagToStr(LogInfoFlag::FLAG_ARCHIVED) + "!=0";
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1,
        DBCommon::GetLogTableName(CLOUD_SYNC_TABLE), condition), count);
    // step4 delete archived data with pk 0
    EXPECT_EQ(DeleteSyncedData(db, CLOUD_SYNC_TABLE, {{static_cast<int64_t>(0)}}), OK);
    auto cloud = GetVirtualCloudDb();
    ASSERT_NE(cloud, nullptr);
    auto before = cloud->GetUpdateCount();
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE});
    ASSERT_NO_FATAL_FAILURE(RDBGeneralUt::CloudBlockSync(info1, query));
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE),
        "cloud_gid=''"), 1);
    auto after = cloud->GetUpdateCount();
    EXPECT_EQ(after - before, static_cast<size_t>(1));
}

/**
 * @tc.name: DeleteSyncedData002
 * @tc.desc: Test delete cloud archived data after cloud sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, DeleteSyncedData002, TestSize.Level1)
{
    const int count = 2;
    EXPECT_NO_FATAL_FAILURE(CloudInsertAndSync(count));
    // step3 archive synced data with cursor 1
    auto info1 = GetStoreInfo1();
    auto db = GetSqliteHandle(info1);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 1), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, CLOUD_SYNC_TABLE), 1); // archive 1 row and exist 1 row
    std::string condition = "flag&" + DBCommon::FlagToStr(LogInfoFlag::FLAG_ARCHIVED) + "!=0";
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1,
        DBCommon::GetLogTableName(CLOUD_SYNC_TABLE), condition), 1); // 1 log is archived
    // step4 delete archived data with pk 0
    EXPECT_EQ(DeleteSyncedData(db, CLOUD_SYNC_TABLE, {{static_cast<int64_t>(0)}}), OK);
    auto cloud = GetVirtualCloudDb();
    ASSERT_NE(cloud, nullptr);
    auto before = cloud->GetUpdateCount();
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE});
    ASSERT_NO_FATAL_FAILURE(RDBGeneralUt::CloudBlockSync(info1, query));
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(CLOUD_SYNC_TABLE),
        "cloud_gid=''"), 1);
    auto after = cloud->GetUpdateCount();
    EXPECT_EQ(after - before, static_cast<size_t>(1));
}

/**
 * @tc.name: FullSync001
 * @tc.desc: Test full cloud sync after normal cloud sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, FullSync001, TestSize.Level1)
{
    const int count = 2;
    EXPECT_NO_FATAL_FAILURE(LocalInsertAndSync(count));
    // step3 full sync again
    auto info1 = GetStoreInfo1();
    CloudSyncOption option = RDBGeneralUt::GetCloudSyncOption();
    option.query = Query::Select().FromTable({CLOUD_SYNC_TABLE});
    option.isFullSync = true;
    ASSERT_EQ(InsertLocalDBData(count, 1, info1), E_OK);
    ASSERT_NO_FATAL_FAILURE(RDBGeneralUt::CloudBlockSync(info1, option, OK, OK));
}

/**
 * @tc.name: InvalidParam001
 * @tc.desc: Test invalid param.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, InvalidParam001, TestSize.Level1)
{
    EXPECT_EQ(ArchiveSyncedData(nullptr, CLOUD_SYNC_TABLE, 1), INVALID_ARGS);
    EXPECT_EQ(DeleteSyncedData(nullptr, CLOUD_SYNC_TABLE, {}), INVALID_ARGS);
    auto info1 = GetStoreInfo1();
    auto db = GetSqliteHandle(info1);
    const std::string invalidTable = "not_exists_table";
    EXPECT_EQ(ArchiveSyncedData(db, invalidTable, 1), TABLE_NOT_FOUND);
    EXPECT_EQ(DeleteSyncedData(db, invalidTable, {}), TABLE_NOT_FOUND);
    std::string sql = "DROP TABLE " + DBCommon::GetMetaTableName();
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    EXPECT_EQ(ArchiveSyncedData(db, invalidTable, 1), DISTRIBUTED_SCHEMA_NOT_FOUND);
    EXPECT_EQ(DeleteSyncedData(db, invalidTable, {}), DISTRIBUTED_SCHEMA_NOT_FOUND);
    EXPECT_EQ(SQLiteUtils::TransactionProcess(nullptr, TransactType::IMMEDIATE, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(SQLiteUtils::TransactionProcess(nullptr, TransactType::IMMEDIATE, []() {
        return E_OK;
    }), -E_INVALID_DB);
    EXPECT_EQ(SQLiteUtils::TransactionProcess(db, TransactType::IMMEDIATE, []() {
        return -E_INVALID_ARGS;
    }), -E_INVALID_ARGS);
    std::vector<std::pair<std::string, std::function<void()>>> invalidSql;
    invalidSql.push_back({"", nullptr});
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(nullptr, invalidSql), -E_INVALID_DB);
}

/**
 * @tc.name: InvalidParam002
 * @tc.desc: Test invalid param.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, InvalidParam002, TestSize.Level1)
{
    auto info1 = GetStoreInfo1();
    auto db = GetSqliteHandle(info1);
    std::vector<std::vector<DistributedDB::Type>> keys;
    keys = {{Nil()}};
    EXPECT_EQ(DeleteSyncedData(db, CLOUD_SYNC_TABLE, keys), NOT_SUPPORT);
    keys = {{std::string("k")}};
    EXPECT_EQ(DeleteSyncedData(db, CLOUD_SYNC_TABLE, keys), OK);
    keys = {{}};
    EXPECT_EQ(DeleteSyncedData(db, CLOUD_SYNC_TABLE, keys), OK);
    keys = {{std::string("k1"), std::string("k2")}};
    EXPECT_EQ(DeleteSyncedData(db, CLOUD_SYNC_TABLE, keys), OK);
}

/**
 * @tc.name: InvalidParam003
 * @tc.desc: Test invalid tracker.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, InvalidParam003, TestSize.Level1)
{
    auto info1 = GetStoreInfo1();
    auto db = GetSqliteHandle(info1);
    EXPECT_EQ(SetTrackerTables(info1, {CLOUD_SYNC_TABLE}), OK);
    std::string schemaKey = DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY;
    const Key schema(schemaKey.begin(), schemaKey.end());
    Value schemaVal;
    auto errCode = SQLiteRelationalUtils::PutKvData(db, false, schema, schemaVal);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(ArchiveSyncedData(db, CLOUD_SYNC_TABLE, 1), DB_ERROR);
}

/**
 * @tc.name: InvalidParam004
 * @tc.desc: Test invalid utils.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRdbFullCloudSyncTest, InvalidParam004, TestSize.Level1)
{
    EXPECT_EQ(RelationalStoreClientUtils::CheckTable(nullptr, "table", false), -E_INVALID_DB);
    auto info1 = GetStoreInfo1();
    auto db = GetSqliteHandle(info1);
    EXPECT_EQ(RelationalStoreClientUtils::CheckTable(db, CLOUD_SYNC_TABLE, true), -E_NOT_SUPPORT);
}
}
#endif
