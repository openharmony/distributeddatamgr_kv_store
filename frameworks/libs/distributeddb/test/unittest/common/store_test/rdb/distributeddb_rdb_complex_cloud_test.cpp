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
    void InitTables(const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitSchema(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitDistributedTable(const std::vector<std::string> &tables = {CLOUD_SYNC_TABLE_A});
    void ExpireCursorWithEmptyGid(bool isLogicDelete);
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
    InitDistributedTable();
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

void DistributedDBRDBComplexCloudTest::InitDistributedTable(const std::vector<std::string> &tables)
{
    RDBGeneralUt::SetCloudDbConfig(info1_);
    RDBGeneralUt::SetCloudDbConfig(info2_);
    ASSERT_EQ(SetDistributedTables(info1_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
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

void DistributedDBRDBComplexCloudTest::ExpireCursorWithEmptyGid(bool isLogicDelete)
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
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info2_, query, SyncMode::SYNC_MODE_CLOUD_MERGE, OK, OK));
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
}
#endif