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

#include <sys/time.h>
#include <gtest/gtest.h>

#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"

using namespace testing::ext;
using namespace  DistributedDB;
using namespace  DistributedDBUnitTest;
using namespace std;

namespace {
constexpr const char *DB_SUFFIX = ".db";
constexpr const char *STORE_ID = "Relational_Store_ID";
std::string g_dbDir;
std::string g_testDir;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);

constexpr int E_OK = 0;
constexpr int E_ERROR = 1;
const int WAIT_TIME = 1000; // 1000ms
constexpr static uint64_t TO_100_NS = 10; // 1us to 100ns
const uint64_t MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS = 1000000;

class DistributedDBCloudInterfacesRelationalExtTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

void DistributedDBCloudInterfacesRelationalExtTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void DistributedDBCloudInterfacesRelationalExtTest::TearDownTestCase(void)
{
}

void DistributedDBCloudInterfacesRelationalExtTest::SetUp()
{
}

void DistributedDBCloudInterfacesRelationalExtTest::TearDown()
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

static int GetCurrentSysTimeIn100Ns(uint64_t &outTime)
{
    struct timeval rawTime;
    int errCode = gettimeofday(&rawTime, nullptr);
    if (errCode < 0) {
        return -E_ERROR;
    }
    outTime = static_cast<uint64_t>(rawTime.tv_sec) * MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS +
        static_cast<uint64_t>(rawTime.tv_usec);
    outTime *= TO_100_NS;
    return E_OK;
}

/**
 * @tc.name: GetRawSysTimeTest001
 * @tc.desc: Test get_raw_sys_time has been registered in sqlite
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, GetRawSysTimeTest001, TestSize.Level0)
{
    const std::string sql = "select get_raw_sys_time();";
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    uint64_t curTime = 0;
    int errCode = GetCurrentSysTimeIn100Ns(curTime);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql, nullptr, [curTime] (sqlite3_stmt *stmt) {
        int64_t diff = MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS * TO_100_NS;
        EXPECT_LT(sqlite3_column_int64(stmt, 0) - curTime, diff);
        return OK;
    }), SQLITE_OK);

    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

void PrepareData(const std::string &tableName, bool primaryKeyIsRowId)
{
    /**
     * @tc.steps:step1. create db, create table.
     * @tc.expected: step1. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    std::string sql;
    if (primaryKeyIsRowId) {
        sql = "create table " + tableName + "(rowid INTEGER primary key, id int, name TEXT);";
    } else {
        sql = "create table " + tableName + "(rowid int, id int, name TEXT, PRIMARY KEY(id, name));";
    }

    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);

    /**
     * @tc.steps:step2. create distributed table.
     * @tc.expected: step2. return ok.
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->CreateDistributedTable(tableName, DistributedDB::CLOUD_COOPERATION), OK);
    EXPECT_EQ(g_mgr.CloseStore(delegate), OK);
    delegate = nullptr;
}

/**
 * @tc.name: InsertTriggerTest001
 * @tc.desc: Test insert trigger in sqlite
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, InsertTriggerTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData(tableName, false);

    /**
     * @tc.steps:step2. insert data into sync_data_tmp.
     * @tc.expected: step2. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    std::string sql = "insert into " + tableName + " VALUES(2, 1, 'zhangsan');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

    /**
     * @tc.steps:step3. select data from log table.
     * @tc.expected: step3. return ok.
     */
    sql = "select * from " + DBConstant::RELATIONAL_PREFIX + tableName + "_log;";
    uint64_t curTime = 0;
    int errCode = GetCurrentSysTimeIn100Ns(curTime);
    EXPECT_EQ(errCode, E_OK);

    int resultCount = 0;
    errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [curTime, &resultCount] (sqlite3_stmt *stmt) {
        EXPECT_EQ(sqlite3_column_int64(stmt, 0), 2); // 2 is row id
        std::string device = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, device), E_OK);
        EXPECT_EQ(device, "");
        std::string oriDevice = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 2, oriDevice), E_OK); // 2 is column index
        EXPECT_EQ(oriDevice, "");

        int64_t timestamp = sqlite3_column_int64(stmt, 3); // 3 is column index
        int64_t wtimestamp = sqlite3_column_int64(stmt, 4); // 4 is column index
        int64_t diff = MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS * TO_100_NS;
        EXPECT_LT(labs(timestamp - wtimestamp), diff);
        EXPECT_LT(labs(timestamp - curTime), diff);
        EXPECT_EQ(sqlite3_column_int(stmt, 5), 2); // 5 is column index flag == 2
        resultCount++;
        return OK;
    });
    EXPECT_EQ(errCode, SQLITE_OK);
    EXPECT_EQ(resultCount, 1);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

void UpdateTriggerTest(bool primaryKeyIsRowId)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData(tableName, primaryKeyIsRowId);

    /**
     * @tc.steps:step2. insert data into sync_data_tmp.
     * @tc.expected: step2. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    std::string sql = "insert into " + tableName + " VALUES(2, 1, 'zhangsan');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

    /**
     * @tc.steps:step3. update data.
     * @tc.expected: step3. return ok.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    sql = "update " + tableName + " set name = 'lisi';";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

    /**
     * @tc.steps:step4. select data from log table.
     * @tc.expected: step4. return ok.
     */
    sql = "select * from " + DBConstant::RELATIONAL_PREFIX + tableName + "_log;";
    uint64_t curTime = 0;
    int errCode = GetCurrentSysTimeIn100Ns(curTime);
    EXPECT_EQ(errCode, E_OK);

    int resultCount = 0;
    errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [curTime, &resultCount] (sqlite3_stmt *stmt) {
        EXPECT_EQ(sqlite3_column_int64(stmt, 0), 2); // 2 is row id
        EXPECT_EQ(sqlite3_column_int(stmt, 5), 2); // 5 is column index, flag == 2

        std::string device = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, device), E_OK);
        EXPECT_EQ(device, "");
        std::string oriDevice = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 2, oriDevice), E_OK); // 2 is column index
        EXPECT_EQ(oriDevice, "");

        int64_t timestamp = sqlite3_column_int64(stmt, 3); // 3 is column index
        int64_t wtimestamp = sqlite3_column_int64(stmt, 4); // 4 is column index
        int64_t diff = MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS * TO_100_NS;
        EXPECT_GT(labs(timestamp - wtimestamp), diff);
        EXPECT_LT(labs(timestamp - curTime), diff);

        resultCount++;
        return OK;
    });
    EXPECT_EQ(errCode, SQLITE_OK);
    EXPECT_EQ(resultCount, 1);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: UpdateTriggerTest001
 * @tc.desc: Test update trigger in sqlite for primary key is not row id
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, UpdateTriggerTest001, TestSize.Level0)
{
    UpdateTriggerTest(false);
}

/**
 * @tc.name: UpdateTriggerTest002
 * @tc.desc: Test update trigger in sqlite for primary key is row id
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, UpdateTriggerTest002, TestSize.Level0)
{
    UpdateTriggerTest(true);
}

/**
 * @tc.name: DeleteTriggerTest001
 * @tc.desc: Test delete trigger in sqlite
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, DeleteTriggerTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData(tableName, true);

    /**
     * @tc.steps:step2. insert data into sync_data_tmp.
     * @tc.expected: step2. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    std::string sql = "insert into " + tableName + " VALUES(2, 1, 'zhangsan');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

    /**
     * @tc.steps:step3. delete data.
     * @tc.expected: step3. return ok.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    sql = "delete from " + tableName + " where name = 'zhangsan';";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

    /**
     * @tc.steps:step4. select data from log table.
     * @tc.expected: step4. return ok.
     */
    sql = "select * from " + DBConstant::RELATIONAL_PREFIX + tableName + "_log;";
    uint64_t curTime = 0;
    int errCode = GetCurrentSysTimeIn100Ns(curTime);
    EXPECT_EQ(errCode, E_OK);

    int resultCount = 0;
    errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [curTime, &resultCount] (sqlite3_stmt *stmt) {
        EXPECT_EQ(sqlite3_column_int64(stmt, 0), -1);
        EXPECT_EQ(sqlite3_column_int(stmt, 5), 3); // 5 is column index, flag == 3

        std::string device = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, device), E_OK);
        EXPECT_EQ(device, "");
        std::string oriDevice = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 2, oriDevice), E_OK); // 2 is column index
        EXPECT_EQ(oriDevice, "");

        int64_t timestamp = sqlite3_column_int64(stmt, 3); // 3 is column index
        int64_t wtimestamp = sqlite3_column_int64(stmt, 4); // 4 is column index
        int64_t diff = MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS * TO_100_NS;
        EXPECT_GT(labs(timestamp - wtimestamp), diff);
        EXPECT_LT(labs(timestamp - curTime), diff);

        resultCount++;
        return OK;
    });
    EXPECT_EQ(errCode, SQLITE_OK);
    EXPECT_EQ(resultCount, 1);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}
}