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

#include <sys/time.h>
#include <gtest/gtest.h>

#include "cloud_db_sync_utils_test.h"
#include "cloud_sync_log_table_manager.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_client.h"
#include "simple_tracker_log_table_manager.h"
#include "sqlite_relational_utils.h"
#include "table_info.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
constexpr const char *DB_SUFFIX = ".db";
constexpr const char *STORE_ID = "Relational_Store_ID";
std::string g_dbDir;
std::string g_testDir;

constexpr int E_ERROR = 1;
const int WAIT_TIME = 1000; // 1000ms
constexpr static uint64_t TO_100_NS = 10; // 1us to 100ns
const uint64_t MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS = 1000000;
std::mutex g_mutex;
std::condition_variable g_cv;
bool g_alreadyNotify = false;

class DistributedDBCloudInterfacesRelationalExtClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
    void CheckTriggerObserverTest002(const std::string &tableName, std::atomic<int> &count);

    void ClientObserverFunc(ClientChangedData &clientChangedData)
    {
        for (const auto &tableEntry : clientChangedData.tableData) {
            LOGD("client observer fired, table: %s", tableEntry.first.c_str());
            triggerTableData_.insert_or_assign(tableEntry.first, tableEntry.second);
        }
        triggeredCount_++;
        {
            std::unique_lock<std::mutex> lock(g_mutex);
            g_alreadyNotify = true;
        }
        g_cv.notify_one();
    }

    void ClientObserverFunc2(ClientChangedData &clientChangedData)
    {
        triggeredCount2_++;
        {
            std::unique_lock<std::mutex> lock(g_mutex);
            g_alreadyNotify = true;
        }
        g_cv.notify_one();
    }

    void CheckTriggerTableData(size_t dataSize, const std::string &tableName, ChangeProperties &properties,
        int triggerCount)
    {
        ASSERT_EQ(triggerTableData_.size(), dataSize);
        EXPECT_EQ(triggerTableData_.begin()->first, tableName);
        EXPECT_EQ(triggerTableData_.begin()->second.isTrackedDataChange, properties.isTrackedDataChange);
        EXPECT_EQ(triggerTableData_.begin()->second.isCloudSyncDataChange, properties.isCloudSyncDataChange);
        EXPECT_EQ(triggeredCount_, triggerCount);
    }

    void WaitAndResetNotify()
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        WaitAndResetNotifyWithLock(lock);
    }

    void WaitAndResetNotifyWithLock(std::unique_lock<std::mutex> &lock)
    {
        g_cv.wait(lock, []() {
            return g_alreadyNotify;
        });
        g_alreadyNotify = false;
    }

    std::map<std::string, ChangeProperties> triggerTableData_;
    int triggeredCount_ = 0;
    int triggeredCount2_ = 0;
};

void DistributedDBCloudInterfacesRelationalExtClientTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void DistributedDBCloudInterfacesRelationalExtClientTest::TearDownTestCase(void)
{
}

void DistributedDBCloudInterfacesRelationalExtClientTest::SetUp()
{
}

void DistributedDBCloudInterfacesRelationalExtClientTest::TearDown()
{
    g_alreadyNotify = false;
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void DistributedDBCloudInterfacesRelationalExtClientTest::CheckTriggerObserverTest002(const std::string &tableName,
    std::atomic<int> &count)
{
    count++;
    ASSERT_EQ(triggerTableData_.size(), 1u);
    EXPECT_EQ(triggerTableData_.begin()->first, tableName);
    EXPECT_EQ(triggerTableData_.begin()->second.isTrackedDataChange, false);
    EXPECT_TRUE(triggerTableData_.begin()->second.isCloudSyncDataChange);
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

static void ExecSqlAndWaitForObserver(sqlite3 *db, const std::string &sql, std::unique_lock<std::mutex> &lock)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    g_cv.wait(lock, []() {
        return g_alreadyNotify;
    });
    g_alreadyNotify = false;
}

/**
 * @tc.name: GetRawSysTimeTest001
 * @tc.desc: Test get_raw_sys_time has been registered in sqlite
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, GetRawSysTimeTest001, TestSize.Level0)
{
    const std::string sql = "select get_raw_sys_time();";
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    uint64_t curTime = 0;
    int errCode = GetCurrentSysTimeIn100Ns(curTime);
    EXPECT_EQ(errCode, E_OK);
    errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [curTime] (sqlite3_stmt *stmt) {
        EXPECT_GT(static_cast<uint64_t>(sqlite3_column_int64(stmt, 0)), curTime);
        return E_OK;
    });
    EXPECT_EQ(errCode, SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

static void PrepareData(const std::vector<std::string> &tableNames, bool primaryKeyIsRowId,
    DistributedDB::TableSyncType tableSyncType, bool userDefineRowid = true, bool createDistributeTable = true)
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
                sql = "create table " + tableName + "(rowid int, id int, name TEXT, PRIMARY KEY(id));";
            } else {
                sql = "create table " + tableName + "(id int, name TEXT, PRIMARY KEY(id));";
            }
        }
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }

    /**
     * @tc.steps:step2. create distributed table.
     * @tc.expected: step2. return ok.
     */
    if (createDistributeTable) {
        for (const auto &tableName : tableNames) {
            TableInfo table;
            table.SetTableName(tableName);
            table.SetTableSyncType(tableSyncType);
            std::unique_ptr<SqliteLogTableManager> tableManager = std::make_unique<CloudSyncLogTableManager>();
            EXPECT_NE(tableManager, nullptr);
            EXPECT_EQ(SQLiteUtils::AnalysisSchema(db, tableName, table), E_OK);
            std::vector<FieldInfo> fieldInfos = table.GetFieldInfos();
            EXPECT_EQ(SQLiteRelationalUtils::CreateRelationalMetaTable(db), E_OK);
            EXPECT_EQ(SQLiteRelationalUtils::InitCursorToMeta(db, false, tableName), E_OK);
            EXPECT_EQ(tableManager->CreateRelationalLogTable(db, table), E_OK);
            EXPECT_EQ(tableManager->AddRelationalLogTableTrigger(db, table, ""), E_OK);
            EXPECT_EQ(SQLiteRelationalUtils::SetLogTriggerStatus(db, true), E_OK);
        }
    }
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

static void SaveTrackerSchemaToMetaTable(sqlite3 *db, const std::string tableName, const TableInfo &tableInfo)
{
    RelationalSchemaObject tracker;
    tracker.SetTableMode(DistributedDB::DistributedTableMode::SPLIT_BY_DEVICE);
    tracker.RemoveRelationalTable(tableName);
    tracker.AddRelationalTable(tableInfo);
    const Key schemaKey(DBConstant::RELATIONAL_SCHEMA_KEY,
        DBConstant::RELATIONAL_SCHEMA_KEY + strlen(DBConstant::RELATIONAL_SCHEMA_KEY));
    Value schemaVal;
    auto schemaStr = tracker.ToSchemaString();
    EXPECT_FALSE(schemaStr.size() > SchemaConstant::SCHEMA_STRING_SIZE_LIMIT);
    DBCommon::StringToVector(schemaStr, schemaVal);
    EXPECT_EQ(SQLiteRelationalUtils::PutKvData(db, false, schemaKey, schemaVal), E_OK);
}

static void SetTracerSchema(const std::vector<std::string> &tableNames, DistributedDB::TableSyncType tableSyncType)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    for (const auto &tableName : tableNames) {
        TableInfo tableInfo;
        tableInfo.SetTableName(tableName);
        tableInfo.SetTableSyncType(tableSyncType);
        std::unique_ptr<SqliteLogTableManager> tableManager = std::make_unique<SimpleTrackerLogTableManager>();
        EXPECT_NE(tableManager, nullptr);
        TrackerSchema schema;
        schema.tableName = tableName;
        schema.extendColNames = {"id"};
        schema.trackerColNames = {"name"};
        TrackerTable trackerTable;
        trackerTable.Init(schema);
        EXPECT_EQ(SQLiteRelationalUtils::AnalysisTrackerTable(db, trackerTable, tableInfo), E_OK);
        EXPECT_EQ(tableManager->CreateRelationalLogTable(db, tableInfo), E_OK);
        EXPECT_EQ(SQLiteRelationalUtils::InitCursorToMeta(db, false, tableName), E_OK);
        EXPECT_EQ(SQLiteRelationalUtils::SetLogTriggerStatus(db, true), E_OK);
        EXPECT_EQ(tableManager->AddRelationalLogTableTrigger(db, tableInfo, ""), E_OK);
        SaveTrackerSchemaToMetaTable(db, tableName, tableInfo);
    }
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: InsertTriggerTest003
 * @tc.desc: Test insert trigger in sqlite when use "insert or replace"
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, InsertTriggerTest003, TestSize.Level1)
{
    /**
    * @tc.steps:step1. prepare data.
    * @tc.expected: step1. return ok.
    */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, DistributedDB::CLOUD_COOPERATION);

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
        EXPECT_EQ(sqlite3_column_int64(stmt, 0), 2); // 2 is row id
        std::string device = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, device), E_OK);
        EXPECT_EQ(device, "");
        std::string oriDevice = "";
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 2, oriDevice), E_OK); // 2 is column index
        EXPECT_EQ(oriDevice, "");

        EXPECT_EQ(sqlite3_column_int(stmt, 3), 0x02|0x20); // 3 is column index flag == 0x02|0x20
        std::string gidStr;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 4, gidStr), E_OK); // 4 is column index
        EXPECT_EQ(gid, gidStr);
        resultCount++;
        return E_OK;
    });
    EXPECT_EQ(errCode, SQLITE_OK);
    EXPECT_EQ(resultCount, 1);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

static void UpdateTriggerTest(bool primaryKeyIsRowId)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, primaryKeyIsRowId, DistributedDB::CLOUD_COOPERATION);

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
    sql = "select * from " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName + "_log;";
    uint64_t curTime = 0;
    int errCode = GetCurrentSysTimeIn100Ns(curTime);
    EXPECT_EQ(errCode, E_OK);

    int resultCount = 0;
    errCode = RelationalTestUtils::ExecSql(db, sql, nullptr, [curTime, &resultCount, primaryKeyIsRowId] (
        sqlite3_stmt *stmt) {
        if (primaryKeyIsRowId) {
            EXPECT_EQ(sqlite3_column_int64(stmt, 0), 2); // 2 is row id
        } else {
            EXPECT_EQ(sqlite3_column_int64(stmt, 0), 1); // 1 is row id
        }

        EXPECT_EQ(sqlite3_column_int(stmt, 5), 0x02|0x20); // 5 is column index, flag == 0x02|0x20

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
        return E_OK;
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
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, UpdateTriggerTest001, TestSize.Level1)
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
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, UpdateTriggerTest002, TestSize.Level1)
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
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, DeleteTriggerTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, true, DistributedDB::CLOUD_COOPERATION);

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
    sql = "select * from " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName + "_log;";
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
        return E_OK;
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
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, TriggerObserverTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. call RegisterClientObserver and UnRegisterClientObserver with db = nullptr.
     * @tc.expected: step1. return INVALID_ARGS.
     */
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc,
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

    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: TriggerObserverTest002
 * @tc.desc: Test trigger client observer in sqlite
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, TriggerObserverTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, DistributedDB::CLOUD_COOPERATION, false);

    /**
    * @tc.steps:step2. register client observer.
    * @tc.expected: step2. return ok.
    */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
    RegisterDbHook(db);

    /**
     * @tc.steps:step3. insert data into sync_data, check observer.
     * @tc.expected: step3. check observer ok.
     */
    std::string sql = "insert into " + tableName + " VALUES(1, 'zhangsan'), (2, 'lisi'), (3, 'wangwu');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    WaitAndResetNotify();
    std::atomic<int> count = 0; // 0 is observer triggered counts
    CheckTriggerObserverTest002(tableName, count);

    /**
     * @tc.steps:step4. update data, check observer.
     * @tc.expected: step4. check observer ok.
     */
    sql = "update " + tableName + " set name = 'lisi1' where id = 2;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    WaitAndResetNotify();
    CheckTriggerObserverTest002(tableName, count);

    /**
     * @tc.steps:step4. delete data, check observer.
     * @tc.expected: step4. check observer ok.
     */
    sql = "delete from " + tableName + " where id = 3;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    WaitAndResetNotify();
    CheckTriggerObserverTest002(tableName, count);

    /**
     * @tc.steps:step5. register another observer, update data, check observer.
     * @tc.expected: step5. check observer ok.
     */
    triggeredCount_ = 0;
    ClientObserver clientObserver2 =
        std::bind(&DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc2, this,
        std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver2), OK);
    RegisterDbHook(db);
    sql = "update " + tableName + " set name = 'lisi2' where id = 2;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    WaitAndResetNotify();
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
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, TriggerObserverTest003, TestSize.Level1)
{
    for (int i = 0; i < 1000; i++) { // 1000 is loop times
        std::thread t1 ([this]() {
            sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
            EXPECT_NE(db, nullptr);
            ClientObserver clientObserver = std::bind(
                &DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc, this, std::placeholders::_1);
            EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
            EXPECT_EQ(UnRegisterClientObserver(db), OK);
            EXPECT_EQ(sqlite3_close_v2(db), E_OK);
        });

        std::thread t2 ([this]() {
            sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
            EXPECT_NE(db, nullptr);
            ClientObserver clientObserver = std::bind(
                &DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc2, this, std::placeholders::_1);
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
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, TriggerObserverTest004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, DistributedDB::CLOUD_COOPERATION, false);

    /**
    * @tc.steps:step2. register client observer.
    * @tc.expected: step2. return ok.
    */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
    RegisterDbHook(db);

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
    bool isEqual = g_cv.wait_for(lock, std::chrono::seconds(1), [this, dataCounts]() { // 1 is wait time
        return triggeredCount_ == dataCounts;
    });
    EXPECT_EQ(isEqual, true);
    WaitAndResetNotifyWithLock(lock);
    ASSERT_EQ(triggerTableData_.size(), 1u);
    EXPECT_EQ(triggerTableData_.begin()->first, tableName);
    EXPECT_TRUE(triggerTableData_.begin()->second.isCloudSyncDataChange);
    EXPECT_EQ(triggeredCount_, dataCounts);

    /**
     * @tc.steps:step4. insert or replace, check observer.
     * @tc.expected: step5. check observer ok.
     */
    triggeredCount_ = 0;
    sql = "insert or replace into " + tableName + " VALUES(1000, 'lisi');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    isEqual = g_cv.wait_for(lock, std::chrono::seconds(1), [this]() { // 1 is wait time
        return triggeredCount_ == 1;
    });
    EXPECT_EQ(isEqual, true);
    WaitAndResetNotifyWithLock(lock);
    EXPECT_EQ(triggeredCount_, 1); // 1 is trigger times, first delete then insert
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: TriggerObserverTest005
 * @tc.desc: Test commit and rollback for one table then trigger client observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, TriggerObserverTest005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, DistributedDB::CLOUD_COOPERATION, false);

    /**
    * @tc.steps:step2. register client observer.
    * @tc.expected: step2. return ok.
    */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
    RegisterDbHook(db);

    /**
     * @tc.steps:step3. begin transaction and commit.
     * @tc.expected: step3. check observer ok.
     */
    std::string sql = "begin;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    int dataCounts = 1000; // 1000 is count of insert options.
    for (int i = 1; i <= dataCounts; i++) {
        sql = "insert into " + tableName + " VALUES(" + std::to_string(i) + ", 'zhangsan" + std::to_string(i) + "');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    sql = "commit;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    WaitAndResetNotify();
    ASSERT_EQ(triggerTableData_.size(), 1u);
    EXPECT_EQ(triggerTableData_.begin()->first, tableName);
    EXPECT_TRUE(triggerTableData_.begin()->second.isCloudSyncDataChange);
    EXPECT_EQ(triggeredCount_, 1);

    /**
     * @tc.steps:step4. begin transaction and rollback.
     * @tc.expected: step3. check observer ok.
     */
    triggerTableData_.clear();
    triggeredCount_ = 0;
    sql = "begin;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    for (int i = dataCounts + 1; i <= 2 * dataCounts; i++) { // 2 is double dataCounts
        sql = "insert into " + tableName + " VALUES(" + std::to_string(i) + ", 'zhangsan" + std::to_string(i) + "');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    sql = "rollback;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    EXPECT_TRUE(triggerTableData_.empty());
    EXPECT_EQ(triggeredCount_, 0);

    /**
     * @tc.steps:step5. insert or replace, check observer.
     * @tc.expected: step5. check observer ok.
     */
    triggeredCount_ = 0;
    sql = "insert or replace into " + tableName + " VALUES(1000, 'lisi');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    WaitAndResetNotify();
    EXPECT_EQ(triggeredCount_, 1); // 1 is trigger times, first delete then insert
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: TriggerObserverTest006
 * @tc.desc: Test commit and rollback for multi-table then trigger client observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, TriggerObserverTest006, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName1 = "sync_data1";
    const std::string tableName2 = "sync_data2";
    PrepareData({tableName1, tableName2}, false, DistributedDB::CLOUD_COOPERATION, false);

    /**
    * @tc.steps:step2. register client observer.
    * @tc.expected: step2. return ok.
    */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
    RegisterDbHook(db);

    /**
     * @tc.steps:step3. begin transaction and commit.
     * @tc.expected: step3. check observer ok.
     */
    std::string sql = "insert into " + tableName1 + " VALUES(1, 'zhangsan'), (2, 'lisi'), (3, 'wangwu');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    WaitAndResetNotify();
    ASSERT_EQ(triggerTableData_.size(), 1u); // 1 is table size
    EXPECT_EQ(triggerTableData_.begin()->first, tableName1);
    EXPECT_TRUE(triggerTableData_.begin()->second.isCloudSyncDataChange);
    EXPECT_EQ(triggeredCount_, 1); // 1 is trigger count

    /**
     * @tc.steps:step4. UnRegisterClientObserver and insert table2.
     * @tc.expected: step3. check observer ok.
     */
    triggerTableData_.clear();
    triggeredCount_ = 0;
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    sql = "insert into " + tableName2 + " VALUES(1, 'zhangsan'), (2, 'lisi'), (3, 'wangwu');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    EXPECT_TRUE(triggerTableData_.empty());
    EXPECT_EQ(triggeredCount_, 0);

    /**
     * @tc.steps:step5. RegisterClientObserver again and insert table1, check observer.
     * @tc.expected: step5. check observer ok.
     */
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
    RegisterDbHook(db);
    sql = "insert into " + tableName1 + " VALUES(7, 'zhangjiu');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    WaitAndResetNotify();
    ASSERT_EQ(triggerTableData_.size(), 1u); // 1 is table size
    EXPECT_EQ(triggerTableData_.begin()->first, tableName1);
    EXPECT_TRUE(triggerTableData_.begin()->second.isCloudSyncDataChange);
    EXPECT_EQ(triggeredCount_, 1); // 1 is trigger count
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: TriggerObserverTest007
 * @tc.desc: Test trigger client observer in tracker table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, TriggerObserverTest007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. prepare data and set trackerTable
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, DistributedDB::CLOUD_COOPERATION, false);
    SetTracerSchema({tableName}, DistributedDB::CLOUD_COOPERATION);

    /**
    * @tc.steps:step2. register client observer.
    * @tc.expected: step2. return ok.
    */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);

    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
    RegisterDbHook(db);

    /**
     * @tc.steps:step3. insert data into sync_data, check observer.
     * @tc.expected: step3. check observer ok.
     */
    std::string sql = "insert into " + tableName + " VALUES(1, 'zhangsan'), (2, 'lisi'), (3, 'wangwu');";
    std::unique_lock<std::mutex> lock(g_mutex);
    ExecSqlAndWaitForObserver(db, sql, lock);
    ChangeProperties properties;
    properties.isTrackedDataChange = true;
    properties.isCloudSyncDataChange = false;
    int triggerCount = 1;
    CheckTriggerTableData(1u, tableName, properties, triggerCount);

    /**
     * @tc.steps:step4. update data, check observer.
     * @tc.expected: step4. check observer ok.
     */
    sql = "update " + tableName + " set name = 'lisi1' where id = 2;";
    ExecSqlAndWaitForObserver(db, sql, lock);
    CheckTriggerTableData(1u, tableName, properties, ++triggerCount);

    /**
     * @tc.steps:step5. update to the same data again, check observer.
     * @tc.expected: step5. check observer ok.
     */
    sql = "update " + tableName + " set name = 'lisi1' where id = 2;";
    ExecSqlAndWaitForObserver(db, sql, lock);
    properties.isTrackedDataChange = false;
    CheckTriggerTableData(1u, tableName, properties, ++triggerCount);

    /**
     * @tc.steps:step6. update to the same data again, set name is NULL, check observer.
     * @tc.expected: step6. check observer ok.
     */
    sql = "update " + tableName + " set name = NULL where id = 2;";
    ExecSqlAndWaitForObserver(db, sql, lock);
    properties.isTrackedDataChange = true;
    CheckTriggerTableData(1u, tableName, properties, ++triggerCount);

    /**
     * @tc.steps:step7. update to the same data again, set name is empty, check observer.
     * @tc.expected: step7. check observer ok.
     */
    sql = "update " + tableName + " set name = '' where id = 2;";
    ExecSqlAndWaitForObserver(db, sql, lock);
    CheckTriggerTableData(1u, tableName, properties, ++triggerCount);

    /**
     * @tc.steps:step8. delete data, check observer.
     * @tc.expected: step8. check observer ok.
     */
    sql = "delete from " + tableName + " where id = 2;";
    ExecSqlAndWaitForObserver(db, sql, lock);
    CheckTriggerTableData(1u, tableName, properties, ++triggerCount);
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: TriggerObserverTest008
 * @tc.desc: Test trigger client observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, TriggerObserverTest008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. prepare data
     * @tc.expected: step1. return ok.
     */
    const std::string tableName1 = "table1";
    PrepareData({tableName1}, false, DistributedDB::CLOUD_COOPERATION, false, true);
    const std::string tableName2 = "table2";
    PrepareData({tableName2}, false, DistributedDB::CLOUD_COOPERATION, false, true);

    /**
    * @tc.steps:step2. register client observer.
    * @tc.expected: step2. return ok.
    */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    ClientObserver clientObserver = std::bind(&DistributedDBCloudInterfacesRelationalExtClientTest::ClientObserverFunc,
        this, std::placeholders::_1);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);

    /**
     * @tc.steps:step3. insert data into sync_data, check observer.
     * @tc.expected: step3. check observer ok.
     */
    std::string sql = "insert into " + tableName1 + " VALUES(1, 'zhangsan'), (2, 'lisi'), (3, 'wangwu');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    EXPECT_EQ(triggerTableData_.size(), 0u);

    /**
     * @tc.steps:step4. re-register client observer and register hook
     * @tc.expected: step4. return ok.
     */
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    EXPECT_EQ(RegisterClientObserver(db, clientObserver), OK);
    RegisterDbHook(db);

    /**
     * @tc.steps:step5. insert data into sync_data, check observer.
     * @tc.expected: step5. check observer ok.
     */
    sql = "insert into " + tableName2 + " VALUES(4, 'aaa');";
    std::unique_lock<std::mutex> lock(g_mutex);
    ExecSqlAndWaitForObserver(db, sql, lock);
    EXPECT_EQ(triggerTableData_.size(), 1u);
    EXPECT_TRUE(triggerTableData_.begin()->second.isCloudSyncDataChange);
    EXPECT_EQ(UnRegisterClientObserver(db), OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

void InitLogicDeleteData(sqlite3 *&db, const std::string &tableName, uint64_t num)
{
    for (size_t i = 0; i < num; ++i) {
        std::string sql = "insert or replace into " + tableName + " VALUES('" + std::to_string(i) + "', 'zhangsan');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    std::string sql = "update " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName + "_log" +
        " SET flag = flag | 0x08";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
}

void CheckLogicDeleteData(sqlite3 *&db, const std::string &tableName, uint64_t expectNum)
{
    std::string sql = "select count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName + "_log"
        " where flag&0x08=0x08 and flag&0x01=0";
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        uint64_t count = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        EXPECT_EQ(count, expectNum);
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    stmt = nullptr;
    sql = "select count(*) from " + tableName;
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        uint64_t count = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        EXPECT_EQ(count, expectNum);
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

/**
 * @tc.name: DropDeleteData001
 * @tc.desc: Test trigger client observer in tracker table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, DropDeleteData001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. prepare data.
     * @tc.expected: step1. return ok.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, DistributedDB::CLOUD_COOPERATION, false);
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    uint64_t num = 10;
    InitLogicDeleteData(db, tableName, num);

    /**
     * @tc.steps:step2. db handle is nullptr
     * @tc.expected: step2. return INVALID_ARGS.
     */
    EXPECT_EQ(DropLogicDeletedData(nullptr, tableName, 0u), INVALID_ARGS);

    /**
     * @tc.steps:step3. tableName is empty
     * @tc.expected: step3. return INVALID_ARGS.
     */
    EXPECT_EQ(DropLogicDeletedData(db, "", 0u), INVALID_ARGS);

    /**
     * @tc.steps:step4. tableName is no exist
     * @tc.expected: step4. return INVALID_ARGS.
     */
    EXPECT_EQ(DropLogicDeletedData(db, tableName + "_", 0u), DB_ERROR);

    /**
     * @tc.steps:step5. cursor is 0
     * @tc.expected: step5. return OK.
     */
    EXPECT_EQ(DropLogicDeletedData(db, tableName, 0u), OK);
    CheckLogicDeleteData(db, tableName, 0u);

    /**
     * @tc.steps:step6. init data again, and cursor is 15
     * @tc.expected: step6. return OK.
     */
    uint64_t cursor = 15;
    InitLogicDeleteData(db, tableName, num);
    EXPECT_EQ(DropLogicDeletedData(db, tableName, cursor), OK);
    CheckLogicDeleteData(db, tableName, cursor - num);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

void InitDataStatus(const std::string &tableName, int count, sqlite3 *db)
{
    int type = 4; // the num of different status
    for (int i = 1; i <= type * count; i++) {
        std::string sql = "INSERT INTO " + tableName + " VALUES(" + std::to_string(i) + ", 'zhangsan" +
            std::to_string(i) + "');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    std::string countStr = std::to_string(count);
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(tableName) + " SET status=(CASE WHEN data_key<=" +
        countStr + " THEN 0 WHEN data_key>" + countStr + " AND data_key<=2*" + countStr + " THEN 1 WHEN data_key>2*" +
        countStr + " AND data_key<=3*" + countStr + " THEN 2 ELSE 3 END)";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
}

void CheckDataStatus(const std::string &tableName, const std::string &condition, sqlite3 *db, int64_t expect)
{
    std::string sql = "SELECT count(1) FROM " + DBCommon::GetLogTableName(tableName) + " WHERE " + condition;
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        int64_t count = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
        EXPECT_EQ(count, expect);
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

/**
 * @tc.name: LockDataTest001
 * @tc.desc: Test status after lock
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, LockDataTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data and lock, hashKey has no matching data
     * @tc.expected: step1. return NOT_FOUND.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, DistributedDB::CLOUD_COOPERATION, false);
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    int count = 10;
    InitDataStatus(tableName, count, db);
    std::vector<std::vector<uint8_t>> hashKey;
    hashKey.push_back({'1'});
    EXPECT_EQ(Lock(tableName, hashKey, db), NOT_FOUND);

    /**
     * @tc.steps:step2. init data and lock, hashKey has matching data
     * @tc.expected: step2. return OK.
     */
    hashKey.clear();
    CloudDBSyncUtilsTest::GetHashKey(tableName, " 1=1 ", db, hashKey);
    EXPECT_EQ(Lock(tableName, hashKey, db), OK);

    /**
     * @tc.steps:step3. check status
     * @tc.expected: step3. return OK.
     */
    CheckDataStatus(tableName, " status = 2 and data_key <= 10 ", db, count);
    CheckDataStatus(tableName, " status = 3 and data_key <= 20 ", db, count);
    CheckDataStatus(tableName, " status = 2 ", db, count + count);
    CheckDataStatus(tableName, " status = 3 ", db, count + count);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
 * @tc.name: LockDataTest002
 * @tc.desc: Test status after unLock
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, LockDataTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data and unLock, there is data to be compensated for
     * @tc.expected: step1. return WAIT_COMPENSATED_SYNC.
     */
    const std::string tableName = "sync_data";
    PrepareData({tableName}, false, DistributedDB::CLOUD_COOPERATION, false);
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    int count = 10;
    InitDataStatus(tableName, count, db);
    std::vector<std::vector<uint8_t>> hashKey;
    CloudDBSyncUtilsTest::GetHashKey(tableName, " 1=1 ", db, hashKey);
    EXPECT_EQ(UnLock(tableName, hashKey, db), WAIT_COMPENSATED_SYNC);

    /**
     * @tc.steps:step2. check status
     * @tc.expected: step2. return OK.
     */
    CheckDataStatus(tableName, " status = 0 and data_key <= 10 ", db, count);
    CheckDataStatus(tableName, " status = 1 and data_key <= 20 ", db, count);
    CheckDataStatus(tableName, " status = 0 ", db, count + count);
    CheckDataStatus(tableName, " status = 1 ", db, count + count);

    /**
     * @tc.steps:step3. unLock again, there is data to be compensated for
     * @tc.expected: step3. return WAIT_COMPENSATED_SYNC.
     */
    EXPECT_EQ(UnLock(tableName, hashKey, db), WAIT_COMPENSATED_SYNC);

    /**
     * @tc.steps:step4. unLock again, there is no data to be compensated for
     * @tc.expected: step4. return OK.
     */
    std::string sql = "update " + DBCommon::GetLogTableName(tableName) + " SET status=0";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    EXPECT_EQ(UnLock(tableName, hashKey, db), OK);

    /**
     * @tc.steps:step5. unLock again, hashKey has matching data
     * @tc.expected: step5. return NOT_FOUND.
     */
    hashKey.clear();
    hashKey.push_back({'1'});
    EXPECT_EQ(UnLock(tableName, hashKey, db), NOT_FOUND);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

DistributedDB::StoreObserver::StoreChangedInfo g_changedData;

class MockStoreObserver : public StoreObserver {
public:
    virtual ~MockStoreObserver() {};
    void OnChange(StoreChangedInfo &&data) override
    {
        g_changedData = data;
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.notify_one();
        g_alreadyNotify = true;
    };
};

void CreateTableForStoreObserver(sqlite3 *db, const std::string tableName)
{
    std::string sql = "create table " + tableName + "(id INTEGER primary key, name TEXT);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    sql = "create table no_" + tableName + "(id INTEGER, name TEXT);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    sql = "create table mult_" + tableName + "(id INTEGER, name TEXT, age int, ";
    sql += "PRIMARY KEY (id, name));";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
}

void PrepareDataForStoreObserver(sqlite3 *db, const std::string &tableName, int begin, int dataCounts)
{
    std::string sql = "begin;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    for (int i = begin; i < begin + dataCounts; i++) {
        sql = "insert into " + tableName + " VALUES(" + std::to_string(i + 1) + ", 'zhangsan" +
            std::to_string(i + 1) + "');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        sql = "insert into no_" + tableName +" VALUES(" + std::to_string(i + 1) + ", 'zhangsan" +
            std::to_string(i + 1) + "');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        sql = "insert into mult_" + tableName + " VALUES(" + std::to_string(i + 1) + ", 'zhangsan";
        sql += std::to_string(i + 1) + "', 18);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    for (int i = begin; i < dataCounts / 2 + begin; i++) { // 2 is half
        sql = "update " + tableName + " set name = 'lisi' where id = " + std::to_string(i + 1) + ";";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        sql = "update no_" + tableName + " set name = 'lisi' where _rowid_ = " + std::to_string(i + 1) + ";";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        sql = "update mult_" + tableName + " set age = 20 where id = " + std::to_string(i + 1);
        sql += " and name = 'zhangsan" + std::to_string(i + 1) + "';";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    for (int i = dataCounts / 2 + begin; i < dataCounts + begin; i++) { // 2 is half
        sql = "delete from " + tableName + " where id = " + std::to_string(i + 1) + ";";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        sql = "delete from no_" + tableName + " where _rowid_ = " + std::to_string(i + 1) + ";";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        sql = "delete from mult_" + tableName + " where id = " + std::to_string(i + 1);
        sql += " and name = 'zhangsan" + std::to_string(i + 1) + "';";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
    sql = "commit;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
}

void CheckChangedData(int num, int times = 0, int offset = 0)
{
    if (num == 1) {
        for (size_t i = 1; i <= g_changedData[num].primaryData[ChangeType::OP_INSERT].size(); i++) {
            EXPECT_EQ(std::get<int64_t>(g_changedData[num].primaryData[ChangeType::OP_INSERT][i - 1][0]),
                static_cast<int64_t>(i + offset - times * 5)); // 5 is rowid times
        }
        for (size_t i = 1; i <= g_changedData[num].primaryData[ChangeType::OP_DELETE].size(); i++) {
            EXPECT_EQ(std::get<int64_t>(g_changedData[num].primaryData[ChangeType::OP_DELETE][i - 1][0]),
                static_cast<int64_t>(i + offset + 5)); // 5 is offset
        }
        return;
    }
    for (size_t i = 1; i <= g_changedData[num].primaryData[ChangeType::OP_INSERT].size(); i++) {
        EXPECT_EQ(std::get<int64_t>(g_changedData[num].primaryData[ChangeType::OP_INSERT][i - 1][0]),
            static_cast<int64_t>(i + offset));
    }
    for (size_t i = 1; i <= g_changedData[num].primaryData[ChangeType::OP_UPDATE].size(); i++) {
        EXPECT_EQ(std::get<int64_t>(g_changedData[num].primaryData[ChangeType::OP_UPDATE][i - 1][0]),
            static_cast<int64_t>(i + offset));
    }
    for (size_t i = 1; i <= g_changedData[num].primaryData[ChangeType::OP_DELETE].size(); i++) {
        EXPECT_EQ(std::get<int64_t>(g_changedData[num].primaryData[ChangeType::OP_DELETE][i - 1][0]),
            static_cast<int64_t>(i + offset + 5)); // 5 is offset
    }
}

/**
 * @tc.name: RegisterStoreObserverTest001
 * @tc.desc: Test commit for three table then trigger store observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, RegisterStoreObserverTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare db and create table.
     * @tc.expected: step1. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    std::string tableName = "primary_test";
    CreateTableForStoreObserver(db, tableName);

    /**
    * @tc.steps:step2. register store observer and check onchange.
    * @tc.expected: step2. return ok.
    */
    auto storeObserver = std::make_shared<MockStoreObserver>();
    EXPECT_EQ(RegisterStoreObserver(db, storeObserver), OK);
    RegisterDbHook(db);
    EXPECT_TRUE(g_changedData.empty());
    int dataCounts = 10; // 10 is count of insert options.
    int begin = 0;
    PrepareDataForStoreObserver(db, tableName, begin, dataCounts);
    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.wait(lock, []() {
            return g_alreadyNotify;
        });
        g_alreadyNotify = false;
    }
    EXPECT_EQ(g_changedData[0].tableName, "primary_test");
    CheckChangedData(0);
    EXPECT_EQ(g_changedData[1].tableName, "no_primary_test");
    CheckChangedData(1);
    EXPECT_EQ(g_changedData[2].tableName, "mult_primary_test"); // 2 is mult primary table
    CheckChangedData(2); // 2 is mult primary table
    g_changedData.clear();

    /**
    * @tc.steps:step3. unregister store observer and update data check onchange.
    * @tc.expected: step3. return ok.
    */
    EXPECT_EQ(UnregisterStoreObserver(db), OK);
    begin = 10; // 10 is begin id
    PrepareDataForStoreObserver(db, tableName, begin, dataCounts);
    EXPECT_TRUE(g_changedData.empty());
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: RegisterStoreObserverTest002
 * @tc.desc: Test commit for three table then trigger client observer when register then create table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, RegisterStoreObserverTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare db and register store observer then create table.
     * @tc.expected: step1. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    auto storeObserver = std::make_shared<MockStoreObserver>();
    EXPECT_EQ(RegisterStoreObserver(db, storeObserver), OK);
    RegisterDbHook(db);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    std::string tableName = "primary_test";
    CreateTableForStoreObserver(db, tableName);

    /**
    * @tc.steps:step2. update data and check onchange.
    * @tc.expected: step2. return ok.
    */
    EXPECT_TRUE(g_changedData.empty());
    int dataCounts = 10; // 10 is count of insert options.
    int begin = 0;
    PrepareDataForStoreObserver(db, tableName, begin, dataCounts);
    WaitAndResetNotify();
    EXPECT_EQ(g_changedData[0].tableName, "primary_test");
    CheckChangedData(0);
    EXPECT_EQ(g_changedData[1].tableName, "no_primary_test");
    CheckChangedData(1);
    EXPECT_EQ(g_changedData[2].tableName, "mult_primary_test"); // 2 is mult primary table
    CheckChangedData(2); // 2 is mult primary table
    g_changedData.clear();

    /**
    * @tc.steps:step3. unregister store observer and update data check onchange.
    * @tc.expected: step3. return ok.
    */
    EXPECT_EQ(UnregisterStoreObserver(db), OK);
    begin = 10; // 11 is begin id
    PrepareDataForStoreObserver(db, tableName, begin, dataCounts);
    EXPECT_TRUE(g_changedData.empty());
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: RegisterStoreObserverTest003
 * @tc.desc: Test commit for three table then trigger client observer when register two observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, RegisterStoreObserverTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare db and register store observer then create table.
     * @tc.expected: step1. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    auto storeObserver1 = std::make_shared<MockStoreObserver>();
    auto storeObserver2 = std::make_shared<MockStoreObserver>();
    EXPECT_EQ(RegisterStoreObserver(db, storeObserver1), OK);
    EXPECT_EQ(RegisterStoreObserver(db, storeObserver2), OK);
    RegisterDbHook(db);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    std::string tableName = "primary_test";
    CreateTableForStoreObserver(db, tableName);

    /**
    * @tc.steps:step2. update data and check onchange.
    * @tc.expected: step2. return ok.
    */
    EXPECT_TRUE(g_changedData.empty());
    int dataCounts = 10; // 10 is count of insert options.
    int begin = 0;
    PrepareDataForStoreObserver(db, tableName, begin, dataCounts);
    WaitAndResetNotify();
    EXPECT_EQ(g_changedData[0].tableName, "primary_test");
    CheckChangedData(0);
    EXPECT_EQ(g_changedData[1].tableName, "no_primary_test");
    CheckChangedData(1);
    EXPECT_EQ(g_changedData[2].tableName, "mult_primary_test"); // 2 is mult primary table
    CheckChangedData(2); // 2 is mult primary table
    g_changedData.clear();

    /**
    * @tc.steps:step3. unregister store observer and update data check onchange.
    * @tc.expected: step3. return ok.
    */
    EXPECT_EQ(UnregisterStoreObserver(db, storeObserver1), OK);
    begin = 10; // 11 is begin id
    PrepareDataForStoreObserver(db, tableName, begin, dataCounts);
    EXPECT_EQ(g_changedData[0].tableName, "primary_test");
    CheckChangedData(0, 1, dataCounts);
    EXPECT_EQ(g_changedData[1].tableName, "no_primary_test");
    CheckChangedData(1, 1, dataCounts);
    EXPECT_EQ(g_changedData[2].tableName, "mult_primary_test"); // 2 is mult primary table
    CheckChangedData(2, 1, dataCounts); // 2 is mult primary table
    g_changedData.clear();

    EXPECT_EQ(UnregisterStoreObserver(db, storeObserver2), OK);
    begin = 20; // 21 is begin id
    PrepareDataForStoreObserver(db, tableName, begin, dataCounts);
    EXPECT_TRUE(g_changedData.empty());
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

/**
 * @tc.name: RegisterStoreObserverTest004
 * @tc.desc: Test register two same observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudInterfacesRelationalExtClientTest, RegisterStoreObserverTest004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. prepare db and register store observer then create table.
     * @tc.expected: step1. return ok.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    auto storeObserver = std::make_shared<MockStoreObserver>();
    EXPECT_EQ(RegisterStoreObserver(db, storeObserver), OK);
    EXPECT_EQ(RegisterStoreObserver(db, storeObserver), OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}
}
