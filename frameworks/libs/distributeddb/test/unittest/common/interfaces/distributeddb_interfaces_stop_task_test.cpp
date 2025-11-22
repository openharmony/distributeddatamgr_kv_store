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

#ifdef USE_DISTRIBUTEDDB_CLOUD
#include <gtest/gtest.h>
#include <random>

#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "virtual_cloud_db.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

const int ONE_BATCH_NUM = 100;
constexpr const char* DB_SUFFIX = ".db";
constexpr const char* STORE_ID = "RelationalAsyncCreateDistributedDBTableTest";
const std::string NORMAL_CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS test_table(" \
    "id          INT PRIMARY KEY," \
    "name        TEXT," \
    "number      INT NOT NULL);";
const std::vector<Field> CLOUD_FIELDS = {
    {.colName = "id", .type = TYPE_INDEX<int64_t>, .primary = true},
    {.colName = "name", .type = TYPE_INDEX<std::string>},
    {.colName = "number", .type = TYPE_INDEX<int64_t>, .nullable = false}
};
std::string g_testDir;
std::string g_dbDir;
const std::string NORMAL_TABLE = "test_table";
const std::string NORMAL_SHARED_TABLE = "test_table_shared";
const std::string META_TABLE = "naturalbase_rdb_aux_metadata";
sqlite3 *g_db = nullptr;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
std::shared_ptr<VirtualCloudDb> g_virtualCloudDb;
SyncProcess g_syncProcess;

class DistributedDBInterfacesStopTaskTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    void InsertData(int startId, int dataNum, const std::string &tableName = NORMAL_TABLE);
    void CallSync(DBStatus expectResult = OK);
    int GetDataCount(const std::string &tableName, int &count);
};

void GetCloudDbSchema(DataBaseSchema &dataBaseSchema)
{
    TableSchema assetsTableSchema = {.name = NORMAL_TABLE, .sharedTableName = NORMAL_TABLE + "_shared",
        .fields = CLOUD_FIELDS};
    dataBaseSchema.tables.push_back(assetsTableSchema);
}

void DistributedDBInterfacesStopTaskTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void DistributedDBInterfacesStopTaskTest::TearDownTestCase(void)
{
}

void DistributedDBInterfacesStopTaskTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    g_db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(g_db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);

    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, g_delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(g_delegate, nullptr);
    g_virtualCloudDb = make_shared<VirtualCloudDb>();
    ASSERT_EQ(g_delegate->SetCloudDB(g_virtualCloudDb), DBStatus::OK);
}

void DistributedDBInterfacesStopTaskTest::TearDown(void)
{
    if (g_db != nullptr) {
        EXPECT_EQ(sqlite3_close_v2(g_db), SQLITE_OK);
        g_db = nullptr;
    }
    if (g_delegate != nullptr) {
        DBStatus status = g_mgr.CloseStore(g_delegate);
        g_delegate = nullptr;
        EXPECT_EQ(status, OK);
    }
    g_virtualCloudDb = nullptr;
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void DistributedDBInterfacesStopTaskTest::InsertData(int startId, int dataNum, const std::string &tableName)
{
    ASSERT_NE(g_db, nullptr);
    SQLiteUtils::BeginTransaction(g_db, TransactType::IMMEDIATE);
    sqlite3_stmt *stmt = nullptr;
    std::string sql = "INSERT INTO " + tableName + "(id, name, number) VALUES(?, ?, ?);";
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, sql, stmt), E_OK);
    int resetRet = E_OK;
    for (int i = startId; i < startId + dataNum; i++) {
        EXPECT_EQ(SQLiteUtils::BindInt64ToStatement(stmt, 1, i), E_OK); // 1st is id
        EXPECT_EQ(SQLiteUtils::BindTextToStatement(stmt, 2, "name_" + std::to_string(i)), E_OK); // 2nd is name
        EXPECT_EQ(SQLiteUtils::BindInt64ToStatement(stmt, 3, i + i), E_OK); // 3rd is number
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtils::ResetStatement(stmt, false, resetRet);
    }
    SQLiteUtils::ResetStatement(stmt, true, resetRet);
    SQLiteUtils::CommitTransaction(g_db);
}

void DistributedDBInterfacesStopTaskTest::CallSync(DBStatus expectResult)
{
    CloudSyncOption option;
    option.devices = { "CLOUD" };
    option.query = Query::Select().FromTable({NORMAL_TABLE});
    option.mode = SYNC_MODE_CLOUD_MERGE;
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    SyncProcess last;
    auto callback = [&last, &cv, &dataMutex, &finish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    finish = true;
                    last = item.second;
                }
                cv.notify_one();
            }
        }
    };
    ASSERT_EQ(g_delegate->Sync(option, callback), expectResult);
    if (expectResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }
    g_syncProcess = last;
}

int DistributedDBInterfacesStopTaskTest::GetDataCount(const std::string &tableName, int &count)
{
    std::string sql = "select count(*) from " + tableName;
    return SQLiteUtils::GetCountBySql(g_db, sql, count);
}

/**
  * @tc.name: AbortCreateDistributedDBTableTest001
  * @tc.desc: Test abort create distributed table.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AbortCreateDistributedDBTableTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    DataBaseSchema dataBaseSchema;
    GetCloudDbSchema(dataBaseSchema);
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    int dataCount = ONE_BATCH_NUM * 100;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    std::thread subThread([&]() {
        EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION), TASK_INTERRUPTED);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    subThread.join();
    int logCount = 0;
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_TRUE(logCount < dataCount);
    /**
     * @tc.steps:step3. Create distributed table again.
     * @tc.expected: step3. Return OK.
     */
    int newDataCount = 1;
    InsertData(dataCount, newDataCount);
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION), OK);
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_EQ(logCount, dataCount + newDataCount);
}

/**
  * @tc.name: AbortCreateDistributedDBTableTest002
  * @tc.desc: Test sync after abort create distributed table.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AbortCreateDistributedDBTableTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    DataBaseSchema dataBaseSchema;
    GetCloudDbSchema(dataBaseSchema);
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    int dataCount = ONE_BATCH_NUM * 100;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    std::thread subThread([&]() {
        EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION), TASK_INTERRUPTED);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    subThread.join();
    int logCount = 0;
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_TRUE(logCount < dataCount);
    /**
     * @tc.steps:step3. Sync to cloud.
     * @tc.expected: step3. Return SCHEMA_MISMATCH.
     */
    CallSync(SCHEMA_MISMATCH);
    /**
     * @tc.steps:step3. Create distributed table again and sync.
     * @tc.expected: step3. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION), OK);
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_EQ(logCount, dataCount);
    CallSync();
}

/**
  * @tc.name: AbortCreateDistributedDBTableTest003
  * @tc.desc: Test update/delete after abort create distributed table.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AbortCreateDistributedDBTableTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    DataBaseSchema dataBaseSchema;
    GetCloudDbSchema(dataBaseSchema);
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    int dataCount = ONE_BATCH_NUM * 100;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    std::thread subThread([&]() {
        EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION), TASK_INTERRUPTED);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    subThread.join();
    int logCount = 0;
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_TRUE(logCount < dataCount);
    /**
     * @tc.steps:step3. update/delete data and create distributed table again.
     * @tc.expected: step3. Return OK.
     */
    std::string sql = "delete from " + NORMAL_TABLE + " where id >= 0 and id < 10;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    sql = "update " + NORMAL_TABLE + " set number = number + 1 where id >= 10 and id < 20;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION), OK);
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_EQ(logCount, dataCount);
    int actualDeleteCount = 0;
    int expectDeleteCount = 10;
    sql = "select count(*) from " + DBCommon::GetLogTableName(NORMAL_TABLE) + " where data_key = -1";
    EXPECT_EQ(SQLiteUtils::GetCountBySql(g_db, sql, actualDeleteCount), E_OK);
    EXPECT_EQ(actualDeleteCount, expectDeleteCount);
}
#endif
