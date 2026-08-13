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
#include "sqlite_meta_executor.h"
#include "virtual_cloud_db.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

const int ONE_BATCH_NUM = 1000;
constexpr std::chrono::milliseconds SYNC_MAX_TIME = std::chrono::milliseconds(10 * 60 * 1000);
constexpr const char* DB_SUFFIX = ".db";
constexpr const char* STORE_ID = "RelationalAsyncCreateDistributedDBTableTest";
constexpr const char *ASYNC_GEN_LOG_TASK_PREFIX = "async_generate_log_task_";
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
const std::string TEST_TABLE_2 = "test_table_2";
const std::string NORMAL_SHARED_TABLE = "test_table_shared";
const std::string META_TABLE = "naturalbase_rdb_aux_metadata";
sqlite3 *g_db = nullptr;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
std::shared_ptr<VirtualCloudDb> g_virtualCloudDb;
VirtualCommunicatorAggregator *g_communicatorAggregator = nullptr;
SyncProcess g_syncProcess;

class DistributedDBInterfacesStopTaskTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    static void InsertData(int startId, int dataNum, const std::string &tableName = NORMAL_TABLE);
    static void CallSync(DBStatus expectResult = OK);
    static void CallSyncWithQuery(const Query &query, DBStatus expectResult = OK);
    static int GetDataCount(const std::string &tableName, int &count);
    static void WaitForTaskFinished(const std::string &tableName = NORMAL_TABLE);
    static void CheckAsyncGenLogFlag(const std::string &tableName, bool &isExist);
    static void PrepareTestTable2(bool isCreateDistributedTable = true);
    static void PrepareTestTables();
};

void GetCloudDbSchema(DataBaseSchema &dataBaseSchema)
{
    TableSchema normalTableSchema = {.name = NORMAL_TABLE, .sharedTableName = NORMAL_TABLE + "_shared",
        .fields = CLOUD_FIELDS};
    dataBaseSchema.tables.push_back(normalTableSchema);
}

void DistributedDBInterfacesStopTaskTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBInterfacesStopTaskTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
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
    DataBaseSchema dataBaseSchema;
    GetCloudDbSchema(dataBaseSchema);
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
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

void DistributedDBInterfacesStopTaskTest::CallSyncWithQuery(const Query &query, DBStatus expectResult)
{
    CloudSyncOption option;
    option.devices = { "CLOUD" };
    option.query = query;
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
    auto start = std::chrono::steady_clock::now();
    bool isTimeOut = false;
    std::thread timeoutCheckThread([&isTimeOut, &dataMutex, &finish, &cv, &start]() {
        while (!finish) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                start);
            if (duration > SYNC_MAX_TIME) {
                isTimeOut = true;
                finish = true;
                cv.notify_all();
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    ASSERT_EQ(g_delegate->Sync(option, callback), expectResult);
    if (expectResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    } else {
        finish = true;
    }
    timeoutCheckThread.join();
    EXPECT_FALSE(isTimeOut);
    g_syncProcess = last;
}

void DistributedDBInterfacesStopTaskTest::CallSync(DBStatus expectResult)
{
    Query query = Query::Select().FromTable({NORMAL_TABLE});
    CallSyncWithQuery(query, expectResult);
}

int DistributedDBInterfacesStopTaskTest::GetDataCount(const std::string &tableName, int &count)
{
    std::string sql = "select count(*) from " + tableName;
    return SQLiteUtils::GetCountBySql(g_db, sql, count);
}

void DistributedDBInterfacesStopTaskTest::WaitForTaskFinished(const std::string &tableName)
{
    int loopTimes = 100;
    bool isExistTask = true;
    while (loopTimes > 0 && isExistTask) {
        loopTimes--;
        CheckAsyncGenLogFlag(tableName, isExistTask);
        std::this_thread::sleep_for(chrono::seconds(1)); // wait 1s to query again.
    }
}

void DistributedDBInterfacesStopTaskTest::CheckAsyncGenLogFlag(const std::string &tableName, bool &isExist)
{
    isExist = false;
    Key keyPrefix;
    DBCommon::StringToVector(ASYNC_GEN_LOG_TASK_PREFIX, keyPrefix);
    std::map<Key, Value> asyncTaskMap;
    EXPECT_EQ(SqliteMetaExecutor::GetMetaDataByPrefixKey(g_db, false, META_TABLE, keyPrefix, asyncTaskMap), E_OK);
    for (const auto &task : asyncTaskMap) {
        std::string taskTableName;
        DBCommon::VectorToString(task.second, taskTableName);
        if (taskTableName == tableName) {
            isExist = true;
        }
    }
}

void DistributedDBInterfacesStopTaskTest::PrepareTestTable2(bool isCreateDistributedTable)
{
    const std::vector<Field> cloudFields = {
        {.colName = "id", .type = TYPE_INDEX<int64_t>, .primary = true},
        {.colName = "name", .type = TYPE_INDEX<std::string>}
    };
    TableSchema tableSchema = {.name = TEST_TABLE_2, .sharedTableName = TEST_TABLE_2 + "_shared",
        .fields = cloudFields};
    DataBaseSchema dataBaseSchema;
    GetCloudDbSchema(dataBaseSchema);
    dataBaseSchema.tables.push_back(tableSchema);
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    std::string sql = "CREATE TABLE IF NOT EXISTS " + TEST_TABLE_2 + "(id INT PRIMARY KEY, name TEXT);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
    if (isCreateDistributedTable) {
        EXPECT_EQ(g_delegate->CreateDistributedTable(TEST_TABLE_2, CLOUD_COOPERATION), OK);
    }
}

void DistributedDBInterfacesStopTaskTest::PrepareTestTables()
{
    DataBaseSchema dataBaseSchema;
    int tableCount = 10;
    for (int i = 0; i < tableCount; i++) {
        std::string tableName = NORMAL_TABLE + std::to_string(i);
        std::string sql = "CREATE TABLE IF NOT EXISTS " + tableName + "(" \
            "id          INTEGER PRIMARY KEY AUTOINCREMENT," \
            "name        TEXT," \
            "number      INT NOT NULL);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
        for (int j = 0; j < ONE_BATCH_NUM; j++) {
            std::string insertSql = "INSERT INTO " + tableName + "(name, number) VALUES('name_" + std::to_string(j) +
                "', " + std::to_string(j) + ");";
            SqlCondition condition;
            condition.sql = insertSql;
            std::vector<VBucket> records;
            EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
        }
        TableSchema tableSchema = {.name = tableName, .sharedTableName = tableName + "_shared",
            .fields = CLOUD_FIELDS};
        dataBaseSchema.tables.push_back(tableSchema);
    }
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest001
  * @tc.desc: Test abort create distributed table.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for task stopped
    int logCount = 0;
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_TRUE(logCount > 0);
    EXPECT_TRUE(logCount < dataCount);
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_TRUE(isExistFlag);
    /**
     * @tc.steps:step3. Create distributed table again.
     * @tc.expected: step3. Return OK.
     */
    int newDataCount = 1;
    InsertData(dataCount, newDataCount);
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    WaitForTaskFinished();
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_EQ(logCount, dataCount + newDataCount);
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest002
  * @tc.desc: Test sync after async create distributed table.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    int logCount = 0;
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_TRUE(logCount > 0);
    EXPECT_TRUE(logCount < dataCount);
    /**
     * @tc.steps:step3. Sync to cloud.
     * @tc.expected: step3. Return OK.
     */
    CallSync();
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);
    ASSERT_EQ(g_syncProcess.tableProcess.size(), 1u);
    for (const auto &process : g_syncProcess.tableProcess) {
        EXPECT_EQ(process.second.process, FINISHED);
        EXPECT_EQ(process.second.upLoadInfo.successCount, dataCount);
    }
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest003
  * @tc.desc: Test update/delete after async create distributed table.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    int logCount = 0;
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_TRUE(logCount < dataCount);
    /**
     * @tc.steps:step3. update/delete/insert data and create distributed table again.
     * @tc.expected: step3. Return OK.
     */
    std::string sql = "delete from " + NORMAL_TABLE + " where id >= 0 and id < 10;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    sql = "update " + NORMAL_TABLE + " set number = number + 1 where id >= 10 and id < 20;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    InsertData(dataCount, ONE_BATCH_NUM);

    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    WaitForTaskFinished();
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);;
    int actualDeleteCount = 0;
    int expectDeleteCount = 10;
    EXPECT_TRUE(logCount == dataCount + ONE_BATCH_NUM || logCount == dataCount + ONE_BATCH_NUM - expectDeleteCount);
    sql = "select count(*) from " + DBCommon::GetLogTableName(NORMAL_TABLE) + " where data_key = -1";
    EXPECT_EQ(SQLiteUtils::GetCountBySql(g_db, sql, actualDeleteCount), E_OK);
    EXPECT_TRUE(actualDeleteCount == expectDeleteCount || actualDeleteCount == 0);
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest004
  * @tc.desc: Test update/delete after async create distributed table, and then sync to cloud.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    /**
     * @tc.steps:step3. update/delete/insert data sync.
     * @tc.expected: step3. Return OK.
    */
    int deleteCount = 10;
    std::string sql = "delete from " + NORMAL_TABLE + " where id >= 0 and id < 10;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    sql = "update " + NORMAL_TABLE + " set number = number + 1 where id >= 10 and id < 20;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    InsertData(dataCount, ONE_BATCH_NUM);
    CallSync();
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);
    ASSERT_EQ(g_syncProcess.tableProcess.size(), 1u);
    for (const auto &process : g_syncProcess.tableProcess) {
        EXPECT_EQ(process.second.process, FINISHED);
        EXPECT_EQ(process.second.upLoadInfo.successCount, dataCount + ONE_BATCH_NUM - deleteCount);
    }

    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest005
  * @tc.desc: Test update/delete after abort create distributed table, and then sync to cloud.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    bool isExistFlag = false;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_TRUE(isExistFlag);
    /**
     * @tc.steps:step3. update/delete/insert data sync.
     * @tc.expected: step3. Return OK.
    */
    std::string sql = "delete from " + NORMAL_TABLE + " where id >= 0 and id < 10;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    sql = "update " + NORMAL_TABLE + " set number = number + 1 where id >= 10 and id < 20;";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    InsertData(dataCount, ONE_BATCH_NUM);
    CallSync();
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);
    ASSERT_EQ(g_syncProcess.tableProcess.size(), 1u);
    for (const auto &process : g_syncProcess.tableProcess) {
        int deleteCount = 10;
        EXPECT_EQ(process.second.process, FINISHED);
        EXPECT_EQ(process.second.upLoadInfo.successCount, dataCount + ONE_BATCH_NUM - deleteCount);
    }

    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest006
  * @tc.desc: Test create distributed table again after abort create distributed table, and then sync to cloud.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest006, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    bool isExistFlag = false;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_TRUE(isExistFlag);
    /**
     * @tc.steps:step3. Create distributed table again and sync.
     * @tc.expected: step3. Return OK.
    */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    CallSync();
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);
    ASSERT_EQ(g_syncProcess.tableProcess.size(), 1u);
    for (const auto &process : g_syncProcess.tableProcess) {
        EXPECT_EQ(process.second.process, FINISHED);
        EXPECT_EQ(process.second.upLoadInfo.successCount, dataCount);
    }

    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest007
  * @tc.desc: Test create distributed table again after finish async create distributed table, and then sync to cloud.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest007, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    WaitForTaskFinished();
    /**
     * @tc.steps:step3. Create distributed table again and sync.
     * @tc.expected: step3. Return OK.
    */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    CallSync();
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);
    ASSERT_EQ(g_syncProcess.tableProcess.size(), 1u);
    for (const auto &process : g_syncProcess.tableProcess) {
        EXPECT_EQ(process.second.process, FINISHED);
        EXPECT_EQ(process.second.upLoadInfo.successCount, dataCount);
    }

    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest008
  * @tc.desc: Test remove device data when async gen log task exist.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest008, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and remove device data.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    EXPECT_EQ(g_delegate->RemoveDeviceData("dev", ClearMode::FLAG_AND_DATA), OK);
    bool isExistFlag = false;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_TRUE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest009
  * @tc.desc: Test remove device data of table which in async gen log task.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest009, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and remove device data.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    ClearDeviceDataOption option = {.mode = ClearMode::FLAG_AND_DATA, .device = "dev", .tableList = {NORMAL_TABLE}};
    EXPECT_EQ(g_delegate->RemoveDeviceData(option), OK);
    bool isExistFlag = false;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_TRUE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest010
  * @tc.desc: Test remove device data of table which not in async gen log task.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest010, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create another table.
     * @tc.expected: step2. Return OK.
     */
    PrepareTestTable2();
    /**
     * @tc.steps:step3. Create distributed table and remove device data.
     * @tc.expected: step3. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    ClearDeviceDataOption option = {.mode = ClearMode::FLAG_AND_DATA, .device = "dev", .tableList = {TEST_TABLE_2}};
    EXPECT_EQ(g_delegate->RemoveDeviceData(option), OK);
    WaitForTaskFinished();
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest011
  * @tc.desc: Test async create distributed table without data.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest011, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create distributed table without data
     * @tc.expected: step1. Return OK.
    */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
    /**
     * @tc.steps:step2. Prepare data and sync to cloud
     * @tc.expected: step2. Return OK.
    */
    int dataCount = 10;
    InsertData(0, dataCount);

    CallSync();
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);
    ASSERT_EQ(g_syncProcess.tableProcess.size(), 1u);
    for (const auto &process : g_syncProcess.tableProcess) {
        EXPECT_EQ(process.second.process, FINISHED);
        EXPECT_EQ(process.second.upLoadInfo.successCount, dataCount);
    }

    isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest012
  * @tc.desc: Test async create distributed table without data, and then create again after insert data.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest012, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create distributed table without data
     * @tc.expected: step1. Return OK.
    */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
    /**
     * @tc.steps:step2. Prepare data and create distributed table again
     * @tc.expected: step2. Return OK.
    */
    int dataCount = 100;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step3. Sync to cloud.
     * @tc.expected: step3. Return OK.
    */
    CallSync();
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);
    ASSERT_EQ(g_syncProcess.tableProcess.size(), 1u);
    for (const auto &process : g_syncProcess.tableProcess) {
        EXPECT_EQ(process.second.process, FINISHED);
        EXPECT_EQ(process.second.upLoadInfo.successCount, dataCount);
    }

    isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest013
  * @tc.desc: Test sync with table which not create distributed table.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest013, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create distributed table without data
     * @tc.expected: step1. Return OK.
    */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    /**
     * @tc.steps:step2. Prepare data and create another table.
     * @tc.expected: step2. Return OK.
    */
    int dataCount = 100;
    InsertData(0, dataCount);
    PrepareTestTable2(false);
    /**
     * @tc.steps:step3. Sync to cloud.
     * @tc.expected: step3. Sync failed.
    */
    Query query = Query::Select().FromTable({NORMAL_TABLE, TEST_TABLE_2});
    CallSyncWithQuery(query, SCHEMA_MISMATCH);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest014
  * @tc.desc: Test create 2 distributed table, then sync 1 table to cloud
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest014, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data and create another table.
     * @tc.expected: step1. Return OK.
    */
    PrepareTestTable2();
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create 2 distributed table without data
     * @tc.expected: step2. Return OK.
    */
    PrepareTestTable2();
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TEST_TABLE_2, CLOUD_COOPERATION, {true}), OK);
    /**
     * @tc.steps:step3. Sync to cloud.
     * @tc.expected: step3. Sync succ.
    */
    Query query = Query::Select().FromTable({TEST_TABLE_2});
    CallSyncWithQuery(query);
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);

    CallSync();
    EXPECT_EQ(g_syncProcess.process, FINISHED);
    EXPECT_EQ(g_syncProcess.errCode, E_OK);
    ASSERT_EQ(g_syncProcess.tableProcess.size(), 1u);
    for (const auto &process : g_syncProcess.tableProcess) {
        EXPECT_EQ(process.second.process, FINISHED);
        EXPECT_EQ(process.second.upLoadInfo.successCount, dataCount);
    }

    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest016
  * @tc.desc: Test async create distributed table with type DEVICE_COOPERATION
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest016, TestSize.Level0)
{
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, DEVICE_COOPERATION, {true}), NOT_SUPPORT);
}

/**
* @tc.name: AsyncCreateDistributedDBTableTest017
* @tc.desc: Test async create distributed table with multi tables
* @tc.require:
* @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest017, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data and create another table.
     * @tc.expected: step1. Return OK.
    */
    PrepareTestTable2();
    int dataCount = ONE_BATCH_NUM * 5;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create 2 distributed table without data
     * @tc.expected: step2. Return OK.
    */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TEST_TABLE_2, CLOUD_COOPERATION, {true}), OK);
    /**
     * @tc.steps:step3. Wait NORMAL_TABLE finished, check TEST_TABLE_2
     * @tc.expected: step3. Return OK.
    */
    WaitForTaskFinished();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(TEST_TABLE_2, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest018
  * @tc.desc: Test stop async create distributed table and then create distributed table
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest018, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for task stopped
    int logCount = 0;
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_TRUE(logCount > 0);
    EXPECT_TRUE(logCount < dataCount);
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_TRUE(isExistFlag);
    /**
     * @tc.steps:step3. Create distributed table again.
     * @tc.expected: step3. Return OK.
     */
    int newDataCount = 1;
    InsertData(dataCount, newDataCount);
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION), OK);
    EXPECT_EQ(GetDataCount(DBCommon::GetLogTableName(NORMAL_TABLE), logCount), E_OK);
    EXPECT_EQ(logCount, dataCount + newDataCount);
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest019
  * @tc.desc: Test async create distributed table when gen log before cloud sync
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest019, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    PrepareTestTable2();
    int dataCount = ONE_BATCH_NUM * 5;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for task stopped
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_TRUE(isExistFlag);
    /**
     * @tc.steps:step3. Sync and create another distributed table.
     * @tc.expected: step3. Return OK.
     */
    std::thread syncThread([]() {
        CallSync();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(g_delegate->CreateDistributedTable(TEST_TABLE_2, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    WaitForTaskFinished(TEST_TABLE_2);
    syncThread.join();
}
/**
  * @tc.name: AsyncCreateDistributedDBTableTest020
  * @tc.desc: Test async create distributed table and set tracker table
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest020, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM * 10;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Create distributed table and stop task.
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_delegate->StopTask(TaskType::BACKGROUND_TASK);
    /**
     * @tc.steps:step3. Set tracker table.
     * @tc.expected: step3. Return NOT_SUPPORT.
     */
    TrackerSchema schema;
    schema.tableName = NORMAL_TABLE;
    schema.extendColNames = {"name"};
    schema.trackerColNames = {"name"};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_SUPPORT);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest021
  * @tc.desc: Test async create distributed table and set tracker table
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest021, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Set tracker table.
     * @tc.expected: step1. Return OK.
    */
    TrackerSchema schema;
    schema.tableName = NORMAL_TABLE;
    schema.extendColNames = {"name"};
    schema.trackerColNames = {"name"};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step2. Async create distributed table.
     * @tc.expected: step2. Return NOT_SUPPORT.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), NOT_SUPPORT);
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest022
  * @tc.desc: Test IsStringAllDigit
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest022, TestSize.Level1)
{
    EXPECT_FALSE(DBCommon::IsStringAllDigit(""));
    EXPECT_TRUE(DBCommon::IsStringAllDigit("123"));
    EXPECT_FALSE(DBCommon::IsStringAllDigit("a123"));
    EXPECT_FALSE(DBCommon::IsStringAllDigit("123a"));
    EXPECT_FALSE(DBCommon::IsStringAllDigit("#$%^"));
    EXPECT_FALSE(DBCommon::IsStringAllDigit("<>+\n"));
}

/**
  * @tc.name: AsyncCreateDistributedDBTableTest023
  * @tc.desc: Test invalid async gen log task flag
  * @tc.require:
  * @tc.author: liaoyonghuang
*/
HWTEST_F(DistributedDBInterfacesStopTaskTest, AsyncCreateDistributedDBTableTest023, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Prepare data
     * @tc.expected: step1. Return OK.
    */
    int dataCount = ONE_BATCH_NUM;
    InsertData(0, dataCount);
    /**
     * @tc.steps:step2. Insert an invalid flag.
     * @tc.expected: step2. Return OK.
     */
    Key taskKey;
    DBCommon::StringToVector(std::string(ASYNC_GEN_LOG_TASK_PREFIX) + "aaa", taskKey);
    Value taskVal;
    DBCommon::StringToVector("invalid_table", taskVal);
    std::string INSERT_META_SQL = "INSERT OR REPLACE INTO " + META_TABLE + " VALUES(?,?);";
    sqlite3_stmt *statement = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(g_db, INSERT_META_SQL, statement), E_OK);
    ASSERT_EQ(SQLiteUtils::BindBlobToStatement(statement, 1, taskKey, false), E_OK);  // 1 means key index
    ASSERT_EQ(SQLiteUtils::BindBlobToStatement(statement, 2, taskVal, true), E_OK);  // 2 means value index
    ASSERT_EQ(SQLiteUtils::StepWithRetry(statement, false), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(statement, true, errCode);
    /**
     * @tc.steps:step3. Insert a flag of no exist table.
     * @tc.expected: step3. Return OK.
     */
    DBCommon::StringToVector(std::string(ASYNC_GEN_LOG_TASK_PREFIX) + "2", taskKey);
    DBCommon::StringToVector("invalid_table", taskVal);
    statement = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(g_db, INSERT_META_SQL, statement), E_OK);
    ASSERT_EQ(SQLiteUtils::BindBlobToStatement(statement, 1, taskKey, false), E_OK);  // 1 means key index
    ASSERT_EQ(SQLiteUtils::BindBlobToStatement(statement, 2, taskVal, true), E_OK);  // 2 means value index
    ASSERT_EQ(SQLiteUtils::StepWithRetry(statement, false), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(statement, true, errCode);
    /**
     * @tc.steps:step4. Create distributed table and stop task.
     * @tc.expected: step4. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(NORMAL_TABLE, CLOUD_COOPERATION, {true}), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    WaitForTaskFinished();
    bool isExistFlag = true;
    CheckAsyncGenLogFlag(NORMAL_TABLE, isExistFlag);
    EXPECT_FALSE(isExistFlag);
}
#endif
