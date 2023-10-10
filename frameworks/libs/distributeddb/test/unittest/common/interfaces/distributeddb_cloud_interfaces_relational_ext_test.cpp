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
#include "relational_store_client.h"
#include "relational_store_manager.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
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
std::mutex g_mutex;
std::condition_variable g_cv;
bool g_alreadyNotify = false;

class DistributedDBCloudInterfacesRelationalExtTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
    void CheckTriggerObserverTest002(const std::string &tableName, std::atomic<int> &count);
    void ClientObserverFunc(ClientChangedData &clientChangedData)
    {
        for (const auto &tableName : clientChangedData.tableNames) {
            LOGD("client observer fired, table: %s", tableName.c_str());
            triggerTableNames_.insert(tableName);
        }
        triggeredCount_++;
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.notify_one();
        g_alreadyNotify = true;
    }

    void ClientObserverFunc2(ClientChangedData &clientChangedData)
    {
        triggeredCount2_++;
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.notify_one();
        g_alreadyNotify = true;
    }

    std::set<std::string> triggerTableNames_;
    int triggeredCount_ = 0;
    int triggeredCount2_ = 0;
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

void DistributedDBCloudInterfacesRelationalExtTest::CheckTriggerObserverTest002(const std::string &tableName,
    std::atomic<int> &count)
{
    count++;
    ASSERT_EQ(triggerTableNames_.size(), 1u);
    EXPECT_EQ(*triggerTableNames_.begin(), tableName);
    EXPECT_EQ(triggeredCount_, count);
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
    errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [curTime] (sqlite3_stmt *stmt) {
        EXPECT_GT(static_cast<uint64_t>(sqlite3_column_int64(stmt, 0)), curTime);
        return OK;
    });
    EXPECT_EQ(errCode, SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

void PrepareData(const std::vector<std::string> &tableNames, bool primaryKeyIsRowId, bool userDefineRowid = true)
{
    /**
     * @tc.steps:step1. create db, create table.
     * @tc.expected: step1. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    std::string sql;
    for (const auto &tableName : tableNames) {
        if (primaryKeyIsRowId) {
            sql = "create table " + tableName + "(rowid INTEGER primary key, id int, name TEXT);";
        } else {
            if (userDefineRowid) {
                sql = "create table " + tableName + "(rowid int, id int, name TEXT, PRIMARY KEY(id, name));";
            } else {
                sql = "create table " + tableName + "(id int, name TEXT, PRIMARY KEY(id));";
            }
        }
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);

    /**
     * @tc.steps:step2. create distributed table.
     * @tc.expected: step2. return ok.
     */
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);
    for (const auto &tableName : tableNames) {
        EXPECT_EQ(delegate->CreateDistributedTable(tableName, DistributedDB::CLOUD_COOPERATION), OK);
    }
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
    PrepareData({tableName}, false);

    /**
     * @tc.steps:step2. insert data into sync_data.
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
        EXPECT_TRUE(wtimestamp - timestamp < diff);
        EXPECT_TRUE(static_cast<int64_t>(curTime - timestamp) < diff);
        EXPECT_EQ(sqlite3_column_int(stmt, 5), 2); // 5 is column index flag == 2
        resultCount++;
        return OK;
    });
    EXPECT_EQ(errCode, SQLITE_OK);
    EXPECT_EQ(resultCount, 1);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: InsertTriggerTest002
 * @tc.desc: Test insert trigger in sqlite when use "insert or replace"
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, InsertTriggerTest002, TestSize.Level1)
{
    /**
    * @tc.steps:step1. prepare data.
    * @tc.expected: step1. return ok.
    */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false);

    /**
     * @tc.steps:step2. insert data into sync_data.
     * @tc.expected: step2. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    std::string sql = "insert into " + tableName + " VALUES(2, 1, 'zhangsan1');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

    // update cloud_gid in log table
    std::string gid = "test_gid";
    sql = "update " + DBCommon::GetLogTableName(tableName) + " set cloud_gid = '" + gid + "'";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    // use insert or replace to update data
    sql = "insert or replace into " + tableName + " VALUES(3, 1, 'zhangsan1');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

    /**
     * @tc.steps:step3. select data from log table.
     * @tc.expected: step3. return ok.
     */
    sql = "select data_key, device, ori_device, flag, cloud_gid from " + DBCommon::GetLogTableName(tableName);
    int resultCount = 0;
    int errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [&resultCount, gid] (sqlite3_stmt *stmt) {
        EXPECT_EQ(sqlite3_column_int64(stmt, 0), 3); // 3 is row id
        std::string device = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, device), E_OK);
        EXPECT_EQ(device, "");
        std::string oriDevice = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 2, oriDevice), E_OK); // 2 is column index
        EXPECT_EQ(oriDevice, "");

        EXPECT_EQ(sqlite3_column_int(stmt, 3), 2); // 3 is column index flag == 2
        std::string gidStr;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 4, gidStr), E_OK); // 4 is column index
        EXPECT_EQ(gid, gidStr);
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
    PrepareData({tableName}, primaryKeyIsRowId);

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
        EXPECT_TRUE(timestamp - wtimestamp > diff);
        EXPECT_TRUE(static_cast<int64_t>(curTime - timestamp) < diff);

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
    PrepareData({tableName}, true);

    /**
     * @tc.steps:step2. insert data into sync_data.
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

        std::string device = "de";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, device), E_OK);
        EXPECT_EQ(device, "");
        std::string oriDevice = "de";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 2, oriDevice), E_OK); // 2 is column index
        EXPECT_EQ(oriDevice, "");

        int64_t timestamp = sqlite3_column_int64(stmt, 3); // 3 is column index
        int64_t wtimestamp = sqlite3_column_int64(stmt, 4); // 4 is column index
        int64_t diff = MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS * TO_100_NS;
        EXPECT_TRUE(timestamp - wtimestamp > diff);
        EXPECT_TRUE(static_cast<int64_t>(curTime - timestamp) < diff);

        resultCount++;
        return OK;
    });
    EXPECT_EQ(errCode, SQLITE_OK);
    EXPECT_EQ(resultCount, 1);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: TriggerObserverTest001
 * @tc.desc: Test invalid args for RegisterClientObserver and UnRegisterClientObserver
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, TriggerObserverTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. call RegisterClientObserver and UnRegisterClientObserver with db = nullptr.
     * @tc.expected: step1. return INVALID_ARGS.
     */
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(nullptr, clientObserver), INVALID_ARGS);
    EXPECT_EQ(UnRegisterClientObserver(nullptr), INVALID_ARGS);

    /**
     * @tc.steps:step2. call RegisterClientObserver with nullptr clientObserver.
     * @tc.expected: step2. return INVALID_ARGS.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RegisterClientObserver(db, nullptr), INVALID_ARGS);

    /**
     * @tc.steps:step3. call RegisterClientObserver and UnRegisterClientObserver with closed db handle.
     * @tc.expected: step3. return INVALID_ARGS.
     */
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), INVALID_ARGS);
    EXPECT_EQ(UnRegisterClientObserver(db), INVALID_ARGS);
}

/**
 * @tc.name: TriggerObserverTest002
 * @tc.desc: Test trigger client observer in sqlite
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, TriggerObserverTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, false);

    /**
    * @tc.steps:step2. register client observer.
    * @tc.expected: step2. return ok.
    */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);

    /**
     * @tc.steps:step3. insert data into sync_data, check observer.
     * @tc.expected: step3. check observer ok.
     */
    std::string sql = "insert into " + tableName + " VALUES(1, 'zhangsan'), (2, 'lisi'), (3, 'wangwu');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    std::unique_lock<std::mutex> lock(g_mutex);
    g_cv.wait(lock, []() {
        return g_alreadyNotify;
    });
    g_alreadyNotify = false;
    std::atomic<int> count = 0; // 0 is observer triggered counts
    CheckTriggerObserverTest002(tableName, count);

    /**
     * @tc.steps:step4. update data, check observer.
     * @tc.expected: step4. check observer ok.
     */
    sql = "update " + tableName + " set name = 'lisi1' where id = 2;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    g_cv.wait(lock, []() {
        return g_alreadyNotify;
    });
    g_alreadyNotify = false;
    CheckTriggerObserverTest002(tableName, count);

    /**
     * @tc.steps:step4. delete data, check observer.
     * @tc.expected: step4. check observer ok.
     */
    sql = "delete from " + tableName + " where id = 3;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    g_cv.wait(lock, []() {
        return g_alreadyNotify;
    });
    g_alreadyNotify = false;
    CheckTriggerObserverTest002(tableName, count);

    /**
     * @tc.steps:step5. register another observer, update data, check observer.
     * @tc.expected: step5. check observer ok.
     */
    triggeredCount_ = 0;
    ClientObserver clientObserver2 = std::bind(&DistributedDBCloudInterfacesRelationalExtTest::ClientObserverFunc2,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver2), OK);
    sql = "update " + tableName + " set name = 'lisi2' where id = 2;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    g_cv.wait(lock, []() {
        return g_alreadyNotify;
    });
    g_alreadyNotify = false;
    EXPECT_EQ(triggeredCount_, 0);
    EXPECT_EQ(triggeredCount2_, 1);

    /**
     * @tc.steps:step6. UnRegisterClientObserver, update data, check observer.
     * @tc.expected: step6. check observer ok.
     */
    triggeredCount2_ = 0;
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    sql = "update " + tableName + " set name = 'lisi3' where id = 2;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    EXPECT_EQ(triggeredCount2_, 0); // observer2 will not be triggered
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: TriggerObserverTest003
 * @tc.desc: Test RegisterClientObserver and UnRegisterClientObserver concurrently
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, TriggerObserverTest003, TestSize.Level1)
{
    for (int i = 0; i < 1000; i++) { // 1000 is loop times
        std::thread t1 ([this]() {
            sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
            EXPECT_NE(db, nullptr);
            ClientObserver clientObserver = std::bind(
                &DistributedDBCloudInterfacesRelationalExtTest::ClientObserverFunc, this, std::placeholders::_1);
            EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
            EXPECT_EQ(UnRegisterClientObserver(db), OK);
            EXPECT_EQ(sqlite3_close_v2(db), E_OK);
        });

        std::thread t2 ([this]() {
            sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
            EXPECT_NE(db, nullptr);
            ClientObserver clientObserver = std::bind(
                &DistributedDBCloudInterfacesRelationalExtTest::ClientObserverFunc2, this, std::placeholders::_1);
            EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
            EXPECT_EQ(UnRegisterClientObserver(db), OK);
            EXPECT_EQ(sqlite3_close_v2(db), E_OK);
        });

        t1.join();
        t2.join();
    }
}

/**
 * @tc.name: TriggerObserverTest004
 * @tc.desc: Test batch insert/update/delete data then trigger client observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtTest, TriggerObserverTest004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, false);

    /**
    * @tc.steps:step2. register client observer.
    * @tc.expected: step2. return ok.
    */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);

    /**
     * @tc.steps:step3. insert data into sync_data, check observer.
     * @tc.expected: step3. check observer ok.
     */
    std::string sql;
    int dataCounts = 1000; // 1000 is count of insert options.
    for (int i = 1; i <= dataCounts; i++) {
        sql = "insert into " + tableName + " VALUES(" + std::to_string(i) + ", 'zhangsan" + std::to_string(i) + "');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    std::unique_lock<std::mutex> lock(g_mutex);
    g_cv.wait(lock, []() {
        return g_alreadyNotify;
    });
    g_alreadyNotify = false;
    ASSERT_EQ(triggerTableNames_.size(), 1u);
    EXPECT_EQ(*triggerTableNames_.begin(), tableName);
    EXPECT_EQ(triggeredCount_, dataCounts);

    /**
     * @tc.steps:step4. insert or replace, check observer.
     * @tc.expected: step5. check observer ok.
     */
    triggeredCount_ = 0;
    sql = "insert or replace into " + tableName + " VALUES(1000, 'lisi');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    g_cv.wait(lock, []() {
        return g_alreadyNotify;
    });
    g_alreadyNotify = false;
    EXPECT_EQ(triggeredCount_, 1); // 1 is trigger times, first delete then insert
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}
}