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

#include "rdb_general_ut.h"

#ifdef USE_DISTRIBUTEDDB_CLOUD
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBRDBComplexCloudTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    static constexpr const char *CLOUD_SYNC_TABLE_A = "CLOUD_SYNC_TABLE_A";
    static constexpr const char *CLOUD_SYNC_TABLE_B = "CLOUD_SYNC_TABLE_B";
    void InitTables(const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitCompositeTable(const StoreInfo &info1, const StoreInfo &info2, const std::string &table = CLOUD_SYNC_TABLE_B);
    void InitSchema(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitCompositeSchema(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_B);
    void InitDistributedTable(const StoreInfo &info1, const StoreInfo &info2, const std::vector<std::string> &tables = {CLOUD_SYNC_TABLE_A});
    void ExpireCursorWithEmptyGid(bool isLogicDelete, SyncMode mode = SyncMode::SYNC_MODE_CLOUD_MERGE);
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2_ = {USER_ID, APP_ID, STORE_ID_2};
};

void DistributedDBRDBComplexCloudTest::SetUp()
{
    RDBGeneralUt::SetUp();
    // create db first
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1_, "dev1"), E_OK);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2_, "dev2"), E_OK);
    EXPECT_NO_FATAL_FAILURE(InitTables());
    InitSchema(info1_);
    InitSchema(info2_);
    InitDistributedTable(info1_, info2_);
}

void DistributedDBRDBComplexCloudTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBRDBComplexCloudTest::InitTables(const std::string &table)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + "("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT, uuidCol TEXT)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
}

void DistributedDBRDBComplexCloudTest::InitCompositeTable(const StoreInfo &info1, const StoreInfo &info2, const std::string &table)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + "("
        "id INTEGER, intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT,"
        "PRIMARY KEY (id, intCol))";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2), E_OK);
}

void DistributedDBRDBComplexCloudTest::InitSchema(const StoreInfo &info, const std::string &table)
{
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, true}, false},
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false},
        {{"stringCol2", TYPE_INDEX<std::string>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = table, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info, schemaInfo);
}

void DistributedDBRDBComplexCloudTest::InitCompositeSchema(const StoreInfo &info, const std::string &table)
{
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, true}, false},
        {{"intCol", TYPE_INDEX<int64_t>, true, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false},
        {{"stringCol2", TYPE_INDEX<std::string>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = table, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info, schemaInfo);
}

void DistributedDBRDBComplexCloudTest::InitDistributedTable(const StoreInfo &info1, const StoreInfo &info2, const std::vector<std::string> &tables)
{
    RDBGeneralUt::SetCloudDbConfig(info1);
    RDBGeneralUt::SetCloudDbConfig(info2);
    ASSERT_EQ(SetDistributedTables(info1, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
}

/**
 * @tc.name: ExpireCursor001
 * @tc.desc: Test sync with expire cursor.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. store1 insert one data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. store1 push and store2 pull
     * @tc.expected: step2. sync success and stringCol2 is null because store1 don't sync stringCol2.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    std::atomic<int> count = 0;
    cloudDB->ForkAfterQueryResult([&count](VBucket &, std::vector<VBucket> &) {
        count++;
        return count == 1 ? DBStatus::EXPIRED_CURSOR : DBStatus::QUERY_END;
    });
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    cloudDB->ForkAfterQueryResult(nullptr);
}

/**
 * @tc.name: ExpireCursor002
 * @tc.desc: Test sync with expire cursor.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor002, TestSize.Level2)
{
    /**
     * @tc.steps:step1. store1 insert one data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. store1 push and store2 pull
     * @tc.expected: step2. sync success and stringCol2 is null because store1 don't sync stringCol2.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    cloudDB->ForkAfterQueryResult([](VBucket &, std::vector<VBucket> &) {
        return DBStatus::EXPIRED_CURSOR;
    });
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, EXPIRED_CURSOR));
    cloudDB->ForkAfterQueryResult(nullptr);
}

/**
 * @tc.name: ExpireCursor003
 * @tc.desc: Test sync with expire cursor.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor003, TestSize.Level2)
{
    /**
     * @tc.steps:step1. store1 insert one data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. store1 push and store2 pull
     * @tc.expected: step2. sync success and stringCol2 is null because store1 don't sync stringCol2.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    cloudDB->ForkAfterQueryResult([](VBucket &, std::vector<VBucket> &) {
        return DBStatus::EXPIRED_CURSOR;
    });
    cloudDB->ForkQueryAllGid([](const std::string &, VBucket &, std::vector<VBucket> &) {
        return DBStatus::EXPIRED_CURSOR;
    });
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, EXPIRED_CURSOR));
    cloudDB->ForkAfterQueryResult(nullptr);
    cloudDB->ForkQueryAllGid(nullptr);
}

/**
 * @tc.name: ExpireCursor004
 * @tc.desc: Test sync with expire cursor.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor004, TestSize.Level2)
{
    /**
     * @tc.steps:step1. store1 insert one data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. store1 push and store2 pull
     * @tc.expected: step2. sync success and stringCol2 is null because store1 don't sync stringCol2.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    cloudDB->ForkAfterQueryResult([](VBucket &, std::vector<VBucket> &) {
        return DBStatus::EXPIRED_CURSOR;
    });
    cloudDB->ForkQueryAllGid([](const std::string &, VBucket &, std::vector<VBucket> &) {
        return DBStatus::DB_ERROR;
    });
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, CLOUD_ERROR));
    cloudDB->ForkAfterQueryResult(nullptr);
    cloudDB->ForkQueryAllGid(nullptr);
}

void DistributedDBRDBComplexCloudTest::ExpireCursorWithEmptyGid(bool isLogicDelete, SyncMode mode)
{
    /**
     * @tc.steps:step1. store1 insert one data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(2, 2, 'text3', 'text4', 'uuid2')", info2_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. store1 push and store2 pull
     * @tc.expected: step2. sync success.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    /**
     * @tc.steps:step3. mock query with expire cursor, and query all gid with empty
     * @tc.expected: step3. sync ok.
     */
    auto delegate = GetDelegate(info2_);
    ASSERT_NE(delegate, nullptr);
    PragmaData pragmaData = &isLogicDelete;
    ASSERT_EQ(delegate->Pragma(PragmaCmd::LOGIC_DELETE_SYNC_DATA, pragmaData), OK);
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    std::atomic<int> count = 0;
    cloudDB->ForkAfterQueryResult([&count](VBucket &, std::vector<VBucket> &) {
        count++;
        return count == 1 ? DBStatus::EXPIRED_CURSOR : DBStatus::QUERY_END;
    });
    cloudDB->ForkQueryAllGid([](const std::string &, VBucket &, std::vector<VBucket> &) {
        return DBStatus::QUERY_END;
    });
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, mode, OK, OK));
    if (mode == SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH) {
        EXPECT_EQ(RDBGeneralUt::CountTableData(info2_, CLOUD_SYNC_TABLE_A), count);
    }
    cloudDB->ForkAfterQueryResult(nullptr);
    cloudDB->ForkQueryAllGid(nullptr);
}

/**
 * @tc.name: ExpireCursor005
 * @tc.desc: Test sync with expire cursor with empty gid.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor005, TestSize.Level2)
{
    EXPECT_NO_FATAL_FAILURE(ExpireCursorWithEmptyGid(true));
}

/**
 * @tc.name: ExpireCursor006
 * @tc.desc: Test sync with expire cursor with empty gid.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor006, TestSize.Level2)
{
    EXPECT_NO_FATAL_FAILURE(ExpireCursorWithEmptyGid(false));
}

/**
 * @tc.name: ExpireCursor007
 * @tc.desc: Test sync with expire cursor with compositeTabel.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor007, TestSize.Level0)
{
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    StoreInfo info2 = {USER_ID, APP_ID, STORE_ID_2};
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2, "dev2"), E_OK);
    EXPECT_NO_FATAL_FAILURE(InitCompositeTable(info1, info2));
    InitCompositeSchema(info1);
    InitCompositeSchema(info2);
    InitDistributedTable(info1, info2, {CLOUD_SYNC_TABLE_B});
    /**
     * @tc.steps:step1. store1 insert one data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_B VALUES(1, 1, 'text1', 'text2')", info1);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. store1 push and store2 pull
     * @tc.expected: step2. sync success and stringCol2 is null because store1 don't sync stringCol2.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_B});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    std::atomic<int> count = 0;
    cloudDB->ForkAfterQueryResult([&count](VBucket &, std::vector<VBucket> &) {
        count++;
        return count == 1 ? DBStatus::EXPIRED_CURSOR : DBStatus::QUERY_END;
    });
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    cloudDB->ForkAfterQueryResult(nullptr);
}

/**
 * @tc.name: ExpireCursor008
 * @tc.desc: Test sync with expire cursor when query all gid failed.
 * @tc.type: FUNC
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. store1 insert one data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. store1 push and store2 pull
     * @tc.expected: step2. sync success and stringCol2 is null because store1 don't sync stringCol2.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    cloudDB->ForkAfterQueryResult([](VBucket &, std::vector<VBucket> &) {
        return DBStatus::EXPIRED_CURSOR;
    });
    int queryTime = 0;
    cloudDB->ForkQueryAllGid([&queryTime](const std::string &, VBucket &, std::vector<VBucket> &) {
        queryTime++;
        return queryTime == 1 ? DBStatus::OK : DBStatus::EXPIRED_CURSOR;
    });
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, EXPIRED_CURSOR));

    std::string sql = "select count(*) from sqlite_master where name = '" +
        DBCommon::GetTmpLogTableName(CLOUD_SYNC_TABLE_A) + "';";
    int count = -1;
    sqlite3 *db = GetSqliteHandle(info1_);
    EXPECT_EQ(SQLiteUtils::GetCountBySql(db, sql, count), E_OK);
    EXPECT_EQ(count, 0);

    cloudDB->ForkAfterQueryResult(nullptr);
    cloudDB->ForkQueryAllGid(nullptr);
}

/**
 * @tc.name: ExpireCursor009
 * @tc.desc: Test sync with expire cursor and SYNC_MODE_CLOUD_FORCE_PUSH mode.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor009, TestSize.Level2)
{
    EXPECT_NO_FATAL_FAILURE(ExpireCursorWithEmptyGid(true, SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH));
}

/**
 * @tc.name: ExpireCursor010
 * @tc.desc: Test sync with expire cursor and SYNC_MODE_CLOUD_FORCE_PULL mode.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor010, TestSize.Level2)
{
    EXPECT_NO_FATAL_FAILURE(ExpireCursorWithEmptyGid(false, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL));
}

/**
 * @tc.name: ExpireCursor010
 * @tc.desc: Test sync with expire cursor and local not exits gid.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, ExpireCursor011, TestSize.Level2)
{
    /**
     * @tc.steps:step1. store1 insert one data
     * @tc.expected: step1. insert success.
     */
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1')", info1_);
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step2. store1 push
     * @tc.expected: step2. sync success.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
    /**
     * @tc.steps:step3. store2 pull
     * @tc.expected: step3. sync failed with expired cursor too much times.
     */
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    cloudDB->ForkAfterQueryResult([](VBucket &, std::vector<VBucket> &) {
        return DBStatus::EXPIRED_CURSOR;
    });
    cloudDB->ForkQueryAllGid([](const std::string &, VBucket &, std::vector<VBucket> &) {
        return DBStatus::DB_ERROR;
    });
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, EXPIRED_CURSOR));
    cloudDB->ForkAfterQueryResult(nullptr);
    cloudDB->ForkQueryAllGid(nullptr);
}
}
#endif
