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

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_common.h"
#include "rdb_general_ut.h"
#include "sqlite_utils.h"
#include "time_helper.h"

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
    static constexpr int DATA_COUNT_PER_OP = 20;
    static constexpr int ASSET_COUNT = 30;
    static constexpr const int64_t BASE_MODIFY_TIME = 12345678L;
    static constexpr const int64_t BASE_CREATE_TIME = 12345679L;
    void InitTables(const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitCompositeTable(const StoreInfo &info1, const StoreInfo &info2,
        const std::string &table = CLOUD_SYNC_TABLE_B);
    void InitSchema(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitCompositeSchema(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_B);
    void InitDistributedTable(const StoreInfo &info1, const StoreInfo &info2,
        const std::vector<std::string> &tables = {CLOUD_SYNC_TABLE_A});
    void ExpireCursorWithEmptyGid(bool isLogicDelete, SyncMode mode = SyncMode::SYNC_MODE_CLOUD_MERGE);
    int GetActualTableColumnCount(const StoreInfo &info, const std::string &table);
    
    void InsertCloudData(int64_t begin, int64_t count, const std::string &tableName);
    void UpdateCloudData(int64_t begin, int64_t count, const std::string &tableName);
    void DeleteCloudData(int64_t begin, int64_t count, const std::string &tableName);
    void InsertLocalData(int64_t begin, int64_t count, const StoreInfo &info, const std::string &tableName);
    void UpdateLocalData(int64_t begin, int64_t count, const StoreInfo &info, const std::string &tableName);
    void DeleteLocalData(int64_t begin, int64_t count, const StoreInfo &info, const std::string &tableName);
    void SetAssetsStatus(int64_t begin, int64_t count, const StoreInfo &info, const std::string &tableName,
        AssetStatus assetStatus = AssetStatus::DOWNLOADING);
    int GetLocalDataCount(const StoreInfo &info, const std::string &tableName);
    int GetCloudDataCount(const std::string &tableName);
    
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
        "intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT, uuidCol TEXT,"
        "assetCol ASSET, assetsCol ASSETS)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
    EXPECT_EQ(ExecuteSQL(sql, info2_), E_OK);
}

void DistributedDBRDBComplexCloudTest::InitCompositeTable(const StoreInfo &info1, const StoreInfo &info2,
    const std::string &table)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + "("
        "id INTEGER, intCol INTEGER, stringCol1 TEXT, stringCol2 TEXT,"
        "assetCol ASSET, assetsCol ASSETS,"
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
        {{"assetCol", TYPE_INDEX<Asset>, false, true}, false},
        {{"assetsCol", TYPE_INDEX<Assets>, false, true}, false},
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
        {{"assetCol", TYPE_INDEX<Asset>, false, true}, false},
        {{"assetsCol", TYPE_INDEX<Assets>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = table, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info, schemaInfo);
}

void DistributedDBRDBComplexCloudTest::InitDistributedTable(const StoreInfo &info1,
    const StoreInfo &info2, const std::vector<std::string> &tables)
{
    RDBGeneralUt::SetCloudDbConfig(info1);
    RDBGeneralUt::SetCloudDbConfig(info2);
    ASSERT_EQ(SetDistributedTables(info1, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
}

int DistributedDBRDBComplexCloudTest::GetActualTableColumnCount(const StoreInfo &info, const std::string &table)
{
    auto db = GetSqliteHandle(info);
    if (db == nullptr) {
        LOGE("[DistributedDBRDBComplexCloudTest] Get null sqlite handle");
        return -1;
    }

    std::string sql = "PRAGMA table_info('" + table + "')";
    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        LOGE("[DistributedDBRDBComplexCloudTest] Prepare PRAGMA failed: %d", rc);
        return -1;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        count++;
    }
    sqlite3_finalize(stmt);

    LOGI("[DistributedDBRDBComplexCloudTest] Table %s has %d columns", table.c_str(), count);
    return count;
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
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1', NULL, NULL)",
        info1_);
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
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1', NULL, NULL)",
        info1_);
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
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1', NULL, NULL)",
        info1_);
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
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1', NULL, NULL)",
        info1_);
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
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1', NULL, NULL)",
        info1_);
    ASSERT_EQ(ret, E_OK);
    ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(2, 2, 'text3', 'text4', 'uuid2', NULL, NULL)", info2_);
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
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_B VALUES(1, 1, 'text1', 'text2', NULL, NULL)", info1);
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
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1', NULL, NULL)",
        info1_);
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
 * @tc.name: ExpireCursor011
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
    auto ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1', NULL, NULL)",
        info1_);
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

/**
 * @tc.name: UpgradeDistributedTable001
 * @tc.desc: Test upgrade distributed table with schema consistency during transaction.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, UpgradeDistributedTable001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Get column count from actual database before modification
     * @tc.expected: step1.
     *  Get original column count (6 columns: id, intCol, stringCol1, stringCol2, assetCol, assetsCol).
     */
    int originalColumnCount = GetActualTableColumnCount(info1_, CLOUD_SYNC_TABLE_A);
    EXPECT_GE(originalColumnCount, 0);
    LOGI("Step1: originalColumnCount = %d", originalColumnCount);

    /**
     * @tc.steps:step2. Alter table to add new column
     * @tc.expected: step2. alter table success.
     */
    auto ret = ExecuteSQL("ALTER TABLE CLOUD_SYNC_TABLE_A ADD COLUMN newCol INTEGER", info1_);
    EXPECT_EQ(ret, E_OK);

    /**
     * @tc.steps:step3. Verify column count increased after ALTER TABLE
     * @tc.expected: step3. column count should be original + 1.
     */
    int columnCountAfterAlter = GetActualTableColumnCount(info1_, CLOUD_SYNC_TABLE_A);
    EXPECT_EQ(columnCountAfterAlter, originalColumnCount + 1);
    LOGI("Step3: columnCountAfterAlter = %d", columnCountAfterAlter);

    /**
     * @tc.steps:step4. Insert data with new column
     * @tc.expected: step4. insert success.
     */
    ret = ExecuteSQL("INSERT INTO CLOUD_SYNC_TABLE_A VALUES(1, 1, 'text1', 'text2', 'uuid1', NULL, NULL, 100)",
        info1_);
    EXPECT_EQ(ret, E_OK);

    /**
     * @tc.steps:step5. Upgrade distributed table to get latest schema
     * @tc.expected: step5. upgrade success.
     */
    ret = SetDistributedTables(info1_, {CLOUD_SYNC_TABLE_A}, TableSyncType::CLOUD_COOPERATION);
    EXPECT_EQ(ret, E_OK);
    LOGI("Step5: SetDistributedTables success");

    /**
     * @tc.steps:step6. Verify table structure remains consistent after upgrade
     * @tc.expected: step6. table should still have same number of columns.
     */
    int columnCountAfterUpgrade = GetActualTableColumnCount(info1_, CLOUD_SYNC_TABLE_A);
    EXPECT_EQ(columnCountAfterUpgrade, columnCountAfterAlter);
    LOGI("Step6: columnCountAfterUpgrade = %d", columnCountAfterUpgrade);

    /**
     * @tc.steps:step7. Verify data count
     * @tc.expected: step7. should have at least 1 data.
     */
    auto count = CountTableData(info1_, CLOUD_SYNC_TABLE_A);
    EXPECT_GE(count, 1);
}

void DistributedDBRDBComplexCloudTest::InsertCloudData(int64_t begin, int64_t count, const std::string &tableName)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    std::vector<VBucket> records;
    std::vector<VBucket> extends;
    for (int64_t i = begin; i < begin + count; ++i) {
        int num = 10;
        VBucket record;
        record["id"] = i;
        record["intCol"] = i * num;
        record["stringCol1"] = "cloud_insert_" + std::to_string(i);
        record["stringCol2"] = "cloud_insert_str2_" + std::to_string(i);
        record["uuidCol"] = "uuid_" + std::to_string(i);
        record["assetCol"] = Asset{};
        record["assetsCol"] = Assets{};
        records.push_back(std::move(record));
        
        VBucket extend;
        extend[CloudDbConstant::GID_FIELD] = std::to_string(i);
        extend[CloudDbConstant::CREATE_FIELD] = BASE_CREATE_TIME;
        extend[CloudDbConstant::MODIFY_FIELD] = BASE_MODIFY_TIME;
        extend[CloudDbConstant::DELETE_FIELD] = false;
        extends.push_back(std::move(extend));
    }
    EXPECT_EQ(cloudDB->BatchInsertWithGid(tableName, std::move(records), extends), DBStatus::OK);
    LOGI("[DistributedDBRDBComplexCloudTest] InsertCloudData success, begin=%" PRId64 ", count=%" PRId64,
        begin, count);
}

void DistributedDBRDBComplexCloudTest::UpdateCloudData(int64_t begin, int64_t count, const std::string &tableName)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    std::vector<VBucket> records;
    std::vector<VBucket> extends;
    for (int64_t i = begin; i < begin + count; ++i) {
        VBucket record;
        int num = 100;
        record["id"] = i;
        record["intCol"] = i * num;
        record["stringCol1"] = "cloud_update_" + std::to_string(i);
        record["stringCol2"] = "cloud_update_str2_" + std::to_string(i);
        record["uuidCol"] = "uuid_update_" + std::to_string(i);
        record["assetCol"] = Asset{};
        record["assetsCol"] = Assets{};
        records.push_back(std::move(record));
        
        VBucket extend;
        extend[CloudDbConstant::GID_FIELD] = std::to_string(i);
        extend[CloudDbConstant::CREATE_FIELD] = BASE_CREATE_TIME;
        extend[CloudDbConstant::MODIFY_FIELD] = BASE_MODIFY_TIME + i;
        extends.push_back(std::move(extend));
    }
    EXPECT_EQ(cloudDB->BatchUpdate(tableName, std::move(records), extends), DBStatus::OK);
    LOGI("[DistributedDBRDBComplexCloudTest] UpdateCloudData success, begin=%" PRId64 ", count=%" PRId64,
        begin, count);
}

void DistributedDBRDBComplexCloudTest::DeleteCloudData(int64_t begin, int64_t count, const std::string &tableName)
{
    auto cloudDB = GetVirtualCloudDb();
    ASSERT_NE(cloudDB, nullptr);
    std::vector<VBucket> extends;
    for (int64_t i = begin; i < begin + count; ++i) {
        VBucket extend;
        extend[CloudDbConstant::GID_FIELD] = std::to_string(i);
        extend[CloudDbConstant::CREATE_FIELD] = BASE_CREATE_TIME;
        extend[CloudDbConstant::MODIFY_FIELD] = BASE_MODIFY_TIME + i;
        extends.push_back(std::move(extend));
    }
    EXPECT_EQ(cloudDB->BatchDelete(tableName, extends), DBStatus::OK);
    LOGI("[DistributedDBRDBComplexCloudTest] DeleteCloudData success, begin=%" PRId64 ", count=%" PRId64,
        begin, count);
}

void DistributedDBRDBComplexCloudTest::InsertLocalData(int64_t begin, int64_t count, const StoreInfo &info,
    const std::string &tableName)
{
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string sql = "INSERT INTO " + tableName + " VALUES(" + std::to_string(i) + ", " +
            std::to_string(i * 10) + ", 'local_insert_" + std::to_string(i) + "', 'local_str2_" +
            std::to_string(i) + "', 'local_uuid_" + std::to_string(i) + "', NULL, NULL)";
        EXPECT_EQ(ExecuteSQL(sql, info), E_OK);
    }
    LOGI("[DistributedDBRDBComplexCloudTest] InsertLocalData success, begin=%" PRId64 ", count=%" PRId64,
        begin, count);
}

void DistributedDBRDBComplexCloudTest::UpdateLocalData(int64_t begin, int64_t count, const StoreInfo &info,
    const std::string &tableName)
{
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string sql = "UPDATE " + tableName + " SET intCol=" + std::to_string(i * 100) +
            ", stringCol1='local_update_" + std::to_string(i) + "' WHERE id=" + std::to_string(i);
        EXPECT_EQ(ExecuteSQL(sql, info), E_OK);
    }
    LOGI("[DistributedDBRDBComplexCloudTest] UpdateLocalData success, begin=%" PRId64 ", count=%" PRId64,
        begin, count);
}

void DistributedDBRDBComplexCloudTest::DeleteLocalData(int64_t begin, int64_t count, const StoreInfo &info,
    const std::string &tableName)
{
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string sql = "DELETE FROM " + tableName + " WHERE id=" + std::to_string(i);
        EXPECT_EQ(ExecuteSQL(sql, info), E_OK);
    }
    LOGI("[DistributedDBRDBComplexCloudTest] DeleteLocalData success, begin=%" PRId64 ", count=%" PRId64,
        begin, count);
}

void DistributedDBRDBComplexCloudTest::SetAssetsStatus(int64_t begin, int64_t count, const StoreInfo &info,
    const std::string &tableName, AssetStatus assetStatus)
{
    auto db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);
    for (int64_t i = begin; i < begin + count; ++i) {
        Asset asset{};
        asset.name = "Asset" + std::to_string(i);
        asset.status = assetStatus;
        std::vector<uint8_t> assetBlob;
        RuntimeContext::GetInstance()->AssetToBlob(asset, assetBlob);
        std::vector<Asset> assets;
        std::vector<uint8_t> assetsBlob;
        RuntimeContext::GetInstance()->AssetsToBlob(assets, assetsBlob);
        std::string sql = "UPDATE " + tableName + " SET assetCol = ?, assetsCol = ? WHERE id=" + std::to_string(i);
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
        ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false), E_OK);
        ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 2, assetsBlob, false), E_OK); // 2 is assetsBlob
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        int errCode = 0;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
    LOGI("[DistributedDBRDBComplexCloudTest] SetAssetsStatus success, begin=%" PRId64 ", count=%" PRId64,
        begin, count);
}

int DistributedDBRDBComplexCloudTest::GetLocalDataCount(const StoreInfo &info, const std::string &tableName)
{
    return CountTableData(info, tableName);
}

int DistributedDBRDBComplexCloudTest::GetCloudDataCount(const std::string &tableName)
{
    return RDBGeneralUt::GetCloudDataCount(tableName);
}

void PrepareOption(CloudSyncOption &option, const Query &query, SyncMode mode, bool merge = false,
    SyncFlowType flowType = SyncFlowType::NORMAL)
{
    option.devices = { "CLOUD" };
    option.mode = mode;
    option.syncFlowType = flowType;
    option.query = query;
    option.priorityTask = false;
    option.compensatedSyncOnly = false;
    option.merge = merge;
    option.waitTime = DBConstant::MAX_TIMEOUT;
}

/**
 * @tc.name: DownloadOnlySync001
 * @tc.desc: Test downloadOnly sync with MERGE mode, local has no data, cloud has insert/update/delete data.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, DownloadOnlySync001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare cloud data: insert/update/delete 20 records each
     * @tc.expected: step1. cloud data prepared successfully.
     */
    InsertCloudData(1, DATA_COUNT_PER_OP * 3, CLOUD_SYNC_TABLE_A);
    UpdateCloudData(DATA_COUNT_PER_OP + 1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);
    DeleteCloudData(DATA_COUNT_PER_OP * 2 + 1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);

    int cloudCountBefore = GetCloudDataCount(CLOUD_SYNC_TABLE_A);
    LOGI("[DownloadOnlySync001] Cloud data count before sync: %d", cloudCountBefore);
    
    /**
     * @tc.steps:step2. Execute downloadOnly sync with MERGE mode
     * @tc.expected: step2. sync success.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    CloudSyncOption option;
    PrepareOption(option, query, SyncMode::SYNC_MODE_CLOUD_MERGE, false, SyncFlowType::DOWNLOAD_ONLY);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, option, OK, OK));

    /**
     * @tc.steps:step3. Verify local data is consistent with cloud
     * @tc.expected: step3. local data count matches cloud.
     */
    int localCountAfter = GetLocalDataCount(info1_, CLOUD_SYNC_TABLE_A);
    int cloudCountAfter = GetCloudDataCount(CLOUD_SYNC_TABLE_A);
    LOGI("[DownloadOnlySync001] Local count after sync: %d, Cloud count: %d", localCountAfter, cloudCountAfter);
    EXPECT_EQ(localCountAfter, cloudCountAfter);
}

/**
 * @tc.name: DownloadOnlySync002
 * @tc.desc: Test downloadOnly sync with MERGE mode, both local and cloud have insert/update/delete data.
 * @tc.expected: Local data not uploaded, local data correctly updated.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, DownloadOnlySync002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare cloud data: insert/update/delete 20 records each (different range)
     * @tc.expected: step1. cloud data prepared successfully.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    InsertCloudData(1, DATA_COUNT_PER_OP * 3, CLOUD_SYNC_TABLE_A);
    CloudSyncOption option;
    PrepareOption(option, query, SyncMode::SYNC_MODE_CLOUD_MERGE, false, SyncFlowType::NORMAL);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, option, OK, OK));
    UpdateCloudData(DATA_COUNT_PER_OP + 1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);
    DeleteCloudData(DATA_COUNT_PER_OP * 2 + 1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);

    int cloudCountBefore = GetCloudDataCount(CLOUD_SYNC_TABLE_A);
    LOGI("[DownloadOnlySync002] Cloud data count before sync: %d", cloudCountBefore);

    /**
     * @tc.steps:step2. Prepare local data: insert/update/delete 20 records each
     * @tc.expected: step2. local data prepared successfully.
     */
    InsertLocalData(DATA_COUNT_PER_OP * 3 + 1, DATA_COUNT_PER_OP * 3, info1_, CLOUD_SYNC_TABLE_A);
    UpdateLocalData(DATA_COUNT_PER_OP * 4 + 1, DATA_COUNT_PER_OP, info1_, CLOUD_SYNC_TABLE_A);
    DeleteLocalData(DATA_COUNT_PER_OP * 5 + 1, DATA_COUNT_PER_OP, info1_, CLOUD_SYNC_TABLE_A);

    int localCountBefore = GetLocalDataCount(info1_, CLOUD_SYNC_TABLE_A);
    LOGI("[DownloadOnlySync002] Local data count before sync: %d", localCountBefore);

    /**
     * @tc.steps:step3. Execute downloadOnly sync with MERGE mode
     * @tc.expected: step3. sync success.
     */
    PrepareOption(option, query, SyncMode::SYNC_MODE_CLOUD_MERGE, false, SyncFlowType::DOWNLOAD_ONLY);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, option, OK, OK));

    /**
     * @tc.steps:step4. Verify local data not uploaded and correctly updated
     * @tc.expected: step4. cloud data count unchanged, local data merged correctly.
     */
    int cloudCountAfter = GetCloudDataCount(CLOUD_SYNC_TABLE_A);
    EXPECT_EQ(cloudCountAfter, cloudCountBefore);

    int localCountAfter = GetLocalDataCount(info1_, CLOUD_SYNC_TABLE_A);
    LOGI("[DownloadOnlySync002] Local count after sync: %d", localCountAfter);
    EXPECT_GE(localCountAfter, localCountBefore - DATA_COUNT_PER_OP);
}

/**
 * @tc.name: DownloadOnlySync003
 * @tc.desc: Test downloadOnly sync with MERGE mode, local has DOWNLOADING status assets.
 * @tc.expected: Local data not uploaded, local data correctly updated, assets downloaded.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, DownloadOnlySync003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare local data with DOWNLOADING status assets
     * @tc.expected: step1. local data prepared successfully.
     */
    InsertLocalData(1, DATA_COUNT_PER_OP, info1_, CLOUD_SYNC_TABLE_A);
    UpdateLocalData(1, DATA_COUNT_PER_OP / 2, info1_, CLOUD_SYNC_TABLE_A);
    DeleteLocalData(DATA_COUNT_PER_OP / 2 + 1, DATA_COUNT_PER_OP / 2, info1_, CLOUD_SYNC_TABLE_A);
    SetAssetsStatus(1, ASSET_COUNT, info1_, CLOUD_SYNC_TABLE_A);

    /**
     * @tc.steps:step2. Prepare cloud data
     * @tc.expected: step2. cloud data prepared successfully.
     */
    InsertCloudData(DATA_COUNT_PER_OP + 1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);
    UpdateCloudData(DATA_COUNT_PER_OP + 1, DATA_COUNT_PER_OP / 2, CLOUD_SYNC_TABLE_A);
    DeleteCloudData(DATA_COUNT_PER_OP + DATA_COUNT_PER_OP / 2 + 1, DATA_COUNT_PER_OP / 2, CLOUD_SYNC_TABLE_A);

    int cloudCountBefore = GetCloudDataCount(CLOUD_SYNC_TABLE_A);

    /**
     * @tc.steps:step3. Execute downloadOnly sync with MERGE mode
     * @tc.expected: step3. sync success.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    CloudSyncOption option;
    PrepareOption(option, query, SyncMode::SYNC_MODE_CLOUD_MERGE, false, SyncFlowType::DOWNLOAD_ONLY);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, option, OK, OK));

    /**
     * @tc.steps:step4. Verify local data not uploaded and correctly updated
     * @tc.expected: step4. cloud data count unchanged, local data merged correctly.
     */
    int cloudCountAfter = GetCloudDataCount(CLOUD_SYNC_TABLE_A);
    EXPECT_EQ(cloudCountAfter, cloudCountBefore);
}

/**
 * @tc.name: DownloadOnlySync004
 * @tc.desc: Test downloadOnly sync with FORCE_PULL mode, local has DOWNLOADING status assets.
 * @tc.expected: Local data not uploaded, local data correctly updated.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, DownloadOnlySync004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare local data with DOWNLOADING status assets
     * @tc.expected: step1. local data prepared successfully.
     */
    InsertLocalData(1, DATA_COUNT_PER_OP, info1_, CLOUD_SYNC_TABLE_A);
    UpdateLocalData(1, DATA_COUNT_PER_OP / 2, info1_, CLOUD_SYNC_TABLE_A);
    DeleteLocalData(DATA_COUNT_PER_OP / 2 + 1, DATA_COUNT_PER_OP / 2, info1_, CLOUD_SYNC_TABLE_A);
    SetAssetsStatus(1, ASSET_COUNT, info1_, CLOUD_SYNC_TABLE_A);

    /**
     * @tc.steps:step2. Prepare cloud data
     * @tc.expected: step2. cloud data prepared successfully.
     */
    InsertCloudData(1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);
    UpdateCloudData(1, DATA_COUNT_PER_OP / 2, CLOUD_SYNC_TABLE_A);
    DeleteCloudData(DATA_COUNT_PER_OP / 2 + 1, DATA_COUNT_PER_OP / 2, CLOUD_SYNC_TABLE_A);

    int cloudCountBefore = GetCloudDataCount(CLOUD_SYNC_TABLE_A);

    /**
     * @tc.steps:step3. Execute downloadOnly sync with FORCE_PULL mode
     * @tc.expected: step3. sync success.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    CloudSyncOption option;
    PrepareOption(option, query, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, false, SyncFlowType::DOWNLOAD_ONLY);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, option, OK, OK));

    /**
     * @tc.steps:step4. Verify local data not uploaded and correctly updated
     * @tc.expected: step4. cloud data count unchanged, local data updated with cloud priority.
     */
    int cloudCountAfter = GetCloudDataCount(CLOUD_SYNC_TABLE_A);
    EXPECT_EQ(cloudCountAfter, cloudCountBefore);
}

/**
 * @tc.name: DownloadOnlySync005
 * @tc.desc: Test downloadOnly sync with FORCE_PUSH mode, local has DOWNLOADING status assets.
 * @tc.expected: Local data not uploaded, local data correctly updated.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, DownloadOnlySync005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare local data with DOWNLOADING status assets
     * @tc.expected: step1. local data prepared successfully.
     */
    InsertLocalData(1, DATA_COUNT_PER_OP, info1_, CLOUD_SYNC_TABLE_A);
    UpdateLocalData(1, DATA_COUNT_PER_OP / 2, info1_, CLOUD_SYNC_TABLE_A);
    DeleteLocalData(DATA_COUNT_PER_OP / 2 + 1, DATA_COUNT_PER_OP / 2, info1_, CLOUD_SYNC_TABLE_A);
    SetAssetsStatus(1, ASSET_COUNT, info1_, CLOUD_SYNC_TABLE_A);

    /**
     * @tc.steps:step2. Prepare cloud data
     * @tc.expected: step2. cloud data prepared successfully.
     */
    InsertCloudData(1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);
    UpdateCloudData(1, DATA_COUNT_PER_OP / 2, CLOUD_SYNC_TABLE_A);
    DeleteCloudData(DATA_COUNT_PER_OP / 2 + 1, DATA_COUNT_PER_OP / 2, CLOUD_SYNC_TABLE_A);

    int cloudCountBefore = GetCloudDataCount(CLOUD_SYNC_TABLE_A);

    /**
     * @tc.steps:step3. Execute downloadOnly sync with FORCE_PUSH mode
     * @tc.expected: step3. sync success.
     */
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    CloudSyncOption option;
    PrepareOption(option, query, SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH, false, SyncFlowType::DOWNLOAD_ONLY);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, option, OK, OK));

    /**
     * @tc.steps:step4. Verify local data not uploaded (downloadOnly skips upload)
     * @tc.expected: step4. cloud data count unchanged, local data updated.
     */
    int cloudCountAfter = GetCloudDataCount(CLOUD_SYNC_TABLE_A);
    EXPECT_EQ(cloudCountAfter, cloudCountBefore);
}

/**
 * @tc.name: DownloadOnlySync006
 * @tc.desc: Test compatibility of downloadOnly sync with other functions.
 * @tc.expected: Local data not uploaded, local data correctly updated.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, DownloadOnlySync006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare cloud data
     * @tc.expected: step1. cloud data prepared successfully.
     */
    InsertCloudData(1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);

    /**
     * @tc.steps:step2. Execute downloadOnly sync with compensatedSyncOnly mode
     * @tc.expected: step2. sync failed.
     */
    auto delegate = GetDelegate(info1_);
    ASSERT_NE(delegate, nullptr);
    DistributedDB::CloudSyncOption option;
    option.devices = { "CLOUD" };
    option.mode = SyncMode::SYNC_MODE_CLOUD_FORCE_PULL;
    option.syncFlowType = SyncFlowType::DOWNLOAD_ONLY;
    option.compensatedSyncOnly = true;
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate, NOT_SUPPORT, OK));

    /**
     * @tc.steps:step3. Execute downloadOnly sync with asyncDownloadAssets mode
     * @tc.expected: step3. sync failed.
     */
    option.compensatedSyncOnly = false;
    option.asyncDownloadAssets = true;
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate, NOT_SUPPORT, OK));

    /**
     * @tc.steps:step4. Execute downloadOnly sync with invalid syncFlow
     * @tc.expected: step4. sync failed.
     */
    option.asyncDownloadAssets = false;
    int invalidType = -1;
    option.syncFlowType = static_cast<SyncFlowType>(invalidType);
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate, INVALID_ARGS, OK));

    invalidType = 2;
    option.syncFlowType = static_cast<SyncFlowType>(invalidType);
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate, INVALID_ARGS, OK));

    /**
     * @tc.steps:step5. Verify not local data
     * @tc.expected: step5. OK
     */
    EXPECT_GE(GetLocalDataCount(info1_, CLOUD_SYNC_TABLE_A), 0);
}

/**
 * @tc.name: SkipAssetTest001
 * @tc.desc: Test if skip download task with MERGE mode, local has assets but cloud not.
 * @tc.expected:
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRDBComplexCloudTest, SkipAssetTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare local data with INSERT status assets
     * @tc.expected: step1. local data prepared successfully.
     */
    InsertLocalData(1, DATA_COUNT_PER_OP, info1_, CLOUD_SYNC_TABLE_A);
    UpdateLocalData(1, DATA_COUNT_PER_OP / 2, info1_, CLOUD_SYNC_TABLE_A);
    DeleteLocalData(DATA_COUNT_PER_OP / 2 + 1, DATA_COUNT_PER_OP / 2, info1_, CLOUD_SYNC_TABLE_A);
    SetAssetsStatus(1, ASSET_COUNT, info1_, CLOUD_SYNC_TABLE_A, AssetStatus::INSERT);

    /**
     * @tc.steps:step2. Prepare cloud data without asset
     * @tc.expected: step2. cloud data prepared successfully.
     */
    InsertCloudData(1, DATA_COUNT_PER_OP, CLOUD_SYNC_TABLE_A);
    UpdateCloudData(1, DATA_COUNT_PER_OP / 2, CLOUD_SYNC_TABLE_A);
    DeleteCloudData(DATA_COUNT_PER_OP / 2 + 1, DATA_COUNT_PER_OP / 2, CLOUD_SYNC_TABLE_A);

    /**
     * @tc.steps:step3. set cloudSyncConfig sync with MERGE mode
     * @tc.expected: step3. sync success.
     */
    auto delegate = GetDelegate(info1_);
    ASSERT_NE(delegate, nullptr);
    CloudSyncConfig config;
    config.skipDownloadAssets = true;
    EXPECT_EQ(delegate->SetCloudSyncConfig(config), OK);
    Query query = Query::Select().FromTable({CLOUD_SYNC_TABLE_A});
    CloudSyncOption option;
    PrepareOption(option, query, SyncMode::SYNC_MODE_CLOUD_MERGE, false, SyncFlowType::NORMAL);
    EXPECT_NO_FATAL_FAILURE(CloudBlockSync(info1_, option, OK, OK));

    /**
     * @tc.steps:step4. Verify local data
     * @tc.expected: step4. local data asset delete.
     */
    EXPECT_EQ(CountTableData(info1_, CLOUD_SYNC_TABLE_A, "assetsCol IS NULL"), DATA_COUNT_PER_OP / 2);
}
}
#endif
