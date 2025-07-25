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

#include <gtest/gtest.h>
#include <queue>
#include <random>

#include "cloud_db_sync_utils_test.h"
#include "cloud/cloud_db_types.h"
#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "platform_specific.h"
#include "relational_store_manager.h"
#include "relational_store_sqlite_ext.h"
#include "relational_virtual_device.h"
#include "runtime_config.h"
#include "virtual_relational_ver_sync_db_interface.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char *DB_SUFFIX = ".db";
    constexpr const char *STORE_ID = "Relational_Store_tracker";
    constexpr const char *STORE_ID2 = "Relational_Store_tracker2";
    std::string g_testDir;
    std::string g_dbDir;
    const string TABLE_NAME1 = "worKer1";
    const string TABLE_NAME2 = "worKer2";
    const string TABLE_NAME3 = "worKer3";
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreDelegate *g_delegate = nullptr;
    VirtualCommunicatorAggregator *g_communicatorAggregator = nullptr;
    sqlite3 *g_db = nullptr;
    const int HALF = 2;

    const std::string CREATE_LOCAL_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS " + TABLE_NAME1 + "(" \
    "name TEXT PRIMARY KEY," \
    "height REAL ," \
    "married BOOLEAN ," \
    "photo BLOB NOT NULL," \
    "assert BLOB," \
    "age INT);";

    const std::string CREATE_LOCAL_PK_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS " + TABLE_NAME2 + "(" \
    "id INTEGER PRIMARY KEY AUTOINCREMENT," \
    "name TEXT ," \
    "height REAL ," \
    "photo BLOB ," \
    "asserts BLOB," \
    "age INT);";

    const std::string CREATE_LOCAL_PK_TABLE_SQL2 =
    "CREATE TABLE IF NOT EXISTS " + TABLE_NAME3 + "(" \
    "id INTEGER PRIMARY KEY," \
    "name asseT ," \
    "age ASSETs);";

    const std::string EXTEND_COL_NAME1 = "xxx";
    const std::string EXTEND_COL_NAME2 = "name";
    const std::string EXTEND_COL_NAME3 = "age";
    const std::set<std::string> LOCAL_TABLE_TRACKER_NAME_SET1 = { "name1" };
    const std::set<std::string> LOCAL_TABLE_TRACKER_NAME_SET2 = { "name" };
    const std::set<std::string> LOCAL_TABLE_TRACKER_NAME_SET3 = { "height" };
    const std::set<std::string> LOCAL_TABLE_TRACKER_NAME_SET4 = { "height", "name" };
    const std::set<std::string> LOCAL_TABLE_TRACKER_NAME_SET5 = { "name", "" };
    TrackerSchema g_normalSchema1 = {
        .tableName = TABLE_NAME2, .extendColNames = {EXTEND_COL_NAME2}, .trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET2
    };
    TrackerSchema g_normalSchema2 = {
        .tableName = TABLE_NAME2, .extendColNames = {EXTEND_COL_NAME3}, .trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET2
    };
    TrackerSchema g_normalSchema3 = {
        .tableName = TABLE_NAME2, .extendColNames = {EXTEND_COL_NAME3}, .trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET4
    };

    void CreateMultiTable()
    {
        g_db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
        ASSERT_NE(g_db, nullptr);
        EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, CREATE_LOCAL_TABLE_SQL), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, CREATE_LOCAL_PK_TABLE_SQL), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, CREATE_LOCAL_PK_TABLE_SQL2), SQLITE_OK);
    }

    void BatchInsertTableName2Data(uint64_t num)
    {
        for (size_t i = 0; i < num; i++) {
            string sql = "INSERT INTO " + TABLE_NAME2
                + " (name, height, photo, asserts, age) VALUES ('Local" + std::to_string(i) +
                "', '175.8', 175.88888888888, 'x', '18');";
            EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
        }
    }

    void BatchUpdateTableName2Data(uint64_t num, const std::set<std::string> &colNames)
    {
        std::string sql = "UPDATE " + TABLE_NAME2 + " SET ";
        for (const auto &col: colNames) {
            sql += col + " = '1',";
        }
        sql.pop_back();
        sql += " where id = ";
        for (size_t i = 1; i <= num; i++) {
            EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql + std::to_string(i)), SQLITE_OK);
        }
    }

    void BatchDeleteTableName2Data(uint64_t num)
    {
        std::string sql = "DELETE FROM " + TABLE_NAME2 + " WHERE id <= " + std::to_string(num);
        EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
    }

    void BatchOperatorTableName2Data(uint64_t num, const std::set<std::string> &colNames)
    {
        for (int i = 0; i < HALF; i++) {
            BatchInsertTableName2Data(num);
        }
        BatchUpdateTableName2Data(num, colNames);
        BatchDeleteTableName2Data(num);
    }

    void CheckExtendAndCursor(uint64_t num, int start, const std::string &tableName, bool addNum = true)
    {
        int index = 0;
        string querySql = "select json_extract(extend_field, '$.name'), cursor from " + std::string(DBConstant::RELATIONAL_PREFIX) + tableName +
            "_log" + " where data_key <= " + std::to_string(num);
        sqlite3_stmt *stmt = nullptr;
        EXPECT_EQ(SQLiteUtils::GetStatement(g_db, querySql, stmt), E_OK);
        while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::string extendVal;
            EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, extendVal), E_OK);
            ASSERT_NE(num, 0uL);
            EXPECT_EQ(extendVal, "Local" + std::to_string(index % num));
            std::string cursorVal;
            EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, cursorVal), E_OK);
            auto expectCursor = (++index) + start;
            auto expectCursorStr = addNum ? std::to_string(num + expectCursor) : std::to_string(expectCursor);
            EXPECT_EQ(cursorVal, expectCursorStr);
        }
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }

    void CheckExtendAndCursor(uint64_t num, int start)
    {
        CheckExtendAndCursor(num, start, TABLE_NAME2);
    }

    void OpenStore()
    {
        if (g_db == nullptr) {
            g_db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
            ASSERT_NE(g_db, nullptr);
        }
        DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, g_delegate);
        EXPECT_EQ(status, OK);
        ASSERT_NE(g_delegate, nullptr);
    }

    void CloseStore()
    {
        if (g_db != nullptr) {
            EXPECT_EQ(sqlite3_close_v2(g_db), SQLITE_OK);
            g_db = nullptr;
        }
        if (g_delegate != nullptr) {
            EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
            g_delegate = nullptr;
        }
    }

    void CheckDropTableAndReopenDb(bool isDistributed)
    {
        /**
         * @tc.steps:step1. SetTrackerTable, init data and drop table
         * @tc.expected: step1. Return OK.
         */
        CreateMultiTable();
        OpenStore();
        if (isDistributed) {
            EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
        }
        TrackerSchema schema = g_normalSchema1;
        EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
        uint64_t num = 10;
        BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);
        std::string sql = "drop table if exists " + TABLE_NAME2;
        EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
        CloseStore();

        /**
         * @tc.steps:step2. reopen db, check log table
         * @tc.expected: step2. Return OK.
         */
        OpenStore();
        sql = "select count(*) from sqlite_master where name = '" + std::string(DBConstant::RELATIONAL_PREFIX) +
            TABLE_NAME2 + "_log'";
        EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(0), nullptr), SQLITE_OK);

        /**
         * @tc.steps:step3. check tracker schema
         * @tc.expected: step3. Return OK.
         */
        const Key schemaKey(DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY.begin(),
            DBConstant::RELATIONAL_TRACKER_SCHEMA_KEY.end());
        sql = "SELECT value FROM " + std::string(DBConstant::RELATIONAL_PREFIX) + "metadata WHERE key=?;";
        sqlite3_stmt *stmt = nullptr;
        EXPECT_EQ(SQLiteUtils::GetStatement(g_db, sql, stmt), E_OK);
        EXPECT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, schemaKey, false), E_OK);
        Value schemaVal;
        while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            SQLiteUtils::GetColumnBlobValue(stmt, 0, schemaVal);
        }
        EXPECT_TRUE(schemaVal.size() != 0);
        int errCode;
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        std::string schemaStr;
        DBCommon::VectorToString(schemaVal, schemaStr);
        RelationalSchemaObject schemaObject;
        EXPECT_EQ(schemaObject.ParseFromTrackerSchemaString(schemaStr), E_OK);
        EXPECT_EQ(schemaObject.GetTrackerTable(TABLE_NAME2).IsEmpty(), true);
        CloseStore();
    }

    class DistributedDBInterfacesRelationalTrackerTableTest : public testing::Test {
    public:
        static void SetUpTestCase(void);

        static void TearDownTestCase(void);

        void SetUp();

        void TearDown();
    };

    void DistributedDBInterfacesRelationalTrackerTableTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        LOGD("Test dir is %s", g_testDir.c_str());
        g_dbDir = g_testDir + "/";
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    }

    void DistributedDBInterfacesRelationalTrackerTableTest::TearDownTestCase(void)
    {
    }

    void DistributedDBInterfacesRelationalTrackerTableTest::SetUp(void)
    {
        g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
        ASSERT_TRUE(g_communicatorAggregator != nullptr);
        RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
        if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
            LOGE("rm test db files error.");
        }
        DistributedDBToolsUnitTest::PrintTestCaseInfo();
    }

    void DistributedDBInterfacesRelationalTrackerTableTest::TearDown(void)
    {
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
        RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    }

void SetTrackerTableTest(const TrackerSchema &schema, DBStatus expect)
{
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), expect);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest001
  * @tc.desc: Test set tracker table with invalid table name
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest001, TestSize.Level0)
{
    CreateMultiTable();
    OpenStore();
    /**
     * @tc.steps:step1. table name is empty
     * @tc.expected: step1. Return INVALID_ARGS.
     */
    TrackerSchema schema;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), INVALID_ARGS);

    /**
     * @tc.steps:step2. table name is no exist
     * @tc.expected: step2. Return NOT_FOUND.
     */
    schema.tableName = "xx";
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);

    /**
     * @tc.steps:step3. table name is illegal table name
     * @tc.expected: step3. Return INVALID_ARGS.
     */
    schema.tableName = DBConstant::SYSTEM_TABLE_PREFIX + "_1";
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), INVALID_ARGS);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest002
  * @tc.desc: Test set tracker table with empty colNames
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. trackerColNames is empty
     * @tc.expected: step1. Return OK.
     */
    TrackerSchema schema;
    schema.tableName = TABLE_NAME1;
    SetTrackerTableTest(schema, OK);

    /**
     * @tc.steps:step2. trackerColNames is empty but extendColName is no exist
     * @tc.expected: step2. Return OK.
     */
    schema.extendColNames = {EXTEND_COL_NAME1};
    SetTrackerTableTest(schema, SCHEMA_MISMATCH);

    /**
     * @tc.steps:step1. param valid but extend name is empty
     * @tc.expected: step1. Return OK.
     */
    schema.trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET2;
    schema.extendColNames = {};
    SetTrackerTableTest(schema, OK);
}

/**
  * @tc.name: TrackerTableTest003
  * @tc.desc: Test set tracker table with invalid col name
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. tracker col name is no exist
     * @tc.expected: step1. Return SCHEMA_MISMATCH.
     */
    TrackerSchema schema;
    schema.tableName = TABLE_NAME1;
    schema.trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET1;
    SetTrackerTableTest(schema, SCHEMA_MISMATCH);

    /**
     * @tc.steps:step2. tracker col names contains empty name
     * @tc.expected: step2. Return INVALID_ARGS.
     */
    schema.trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET5;
    SetTrackerTableTest(schema, INVALID_ARGS);

    /**
     * @tc.steps:step3. extend name is no exist
     * @tc.expected: step3. Return SCHEMA_MISMATCH.
     */
    schema.trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET2;
    schema.extendColNames = {EXTEND_COL_NAME1};
    SetTrackerTableTest(schema, SCHEMA_MISMATCH);

    /**
     * @tc.steps:step4. extend name is no exist when tracker col name is enpty
     * @tc.expected: step4. Return SCHEMA_MISMATCH.
     */
    schema.trackerColNames.clear();
    schema.extendColNames = {EXTEND_COL_NAME1};
    SetTrackerTableTest(schema, SCHEMA_MISMATCH);
}

/**
  * @tc.name: TrackerTableTest005
  * @tc.desc: Test set tracker table in same delegate
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. SetTrackerTable twice in same delegate
     * @tc.expected: step1. Return WITH_INVENTORY_DATA for the first time, return OK again
     */
    TrackerSchema schema = g_normalSchema1;
    CreateMultiTable();
    OpenStore();
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);

    /**
     * @tc.steps:step2. SetTrackerTable again with different schema
     * @tc.expected: step2. Return OK.
     */
    schema = g_normalSchema3;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET2);

    /**
     * @tc.steps:step3. SetTrackerTable again with different table
     * @tc.expected: step3. Return OK.
     */
    schema.tableName = TABLE_NAME1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step4. unSetTrackerTable
     * @tc.expected: step4. Return OK.
     */
    schema.tableName = TABLE_NAME2;
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET2);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest006
  * @tc.desc: Test set tracker table in diff delegate
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest006, TestSize.Level1)
{
    /**
     * @tc.steps:step1. SetTrackerTable
     * @tc.expected: step1. Return OK.
     */
    TrackerSchema schema = g_normalSchema1;
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    CloseStore();

    /**
     * @tc.steps:step2. reopen db and SetTrackerTable
     * @tc.expected: step2. Return OK.
     */
    OpenStore();
    schema = g_normalSchema2;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET2);
    CloseStore();

    /**
     * @tc.steps:step3. reopen db and SetTrackerTable with diff table
     * @tc.expected: step3. Return OK.
     */
    OpenStore();
    schema.tableName = TABLE_NAME1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    CloseStore();

    /**
     * @tc.steps:step4. reopen db and unSetTrackerTable
     * @tc.expected: step4. Return OK.
     */
    OpenStore();
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest007
  * @tc.desc: Test set tracker table to upgrade inventory data
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable when the db exist data
     * @tc.expected: step1. Return WITH_INVENTORY_DATA.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();
    TrackerSchema schema = g_normalSchema3;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);

    /**
     * @tc.steps:step2. SetTrackerTable again with diff tracker schema
     * @tc.expected: step2. Return OK.
     */
    schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step3. check extend_field and cursor
     * @tc.expected: step3. Return OK.
     */
    CheckExtendAndCursor(num, 0);

    /**
     * @tc.steps:step4. update extend field and check
     * @tc.expected: step4. Return OK.
     */
    EXPECT_EQ(g_delegate->SetTrackerTable(g_normalSchema3), OK);
    std::string sql = "UPDATE " + TABLE_NAME2 + " SET age='666'";
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
    sql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where json_extract(extend_field, '$.age')=666;";
    char *errmsg;
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num), &errmsg), SQLITE_OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest008
  * @tc.desc: Test set tracker table to check extend_field and cursor
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return WITH_INVENTORY_DATA.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);

    /**
     * @tc.steps:step2. Update non tracker columns,then check extend_field and cursor
     * @tc.expected: step2. Return OK.
     */
    uint64_t updateNum = 2;
    BatchUpdateTableName2Data(updateNum, LOCAL_TABLE_TRACKER_NAME_SET3);
    int index = 0;
    string querySql = "select json_extract(extend_field, '$.name'), cursor from " +
       DBCommon::GetLogTableName(TABLE_NAME2) + " where data_key <= " + std::to_string(updateNum);
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, querySql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string extendVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, extendVal), E_OK);
        EXPECT_EQ(extendVal, "Local" + std::to_string(index % num));
        std::string cursorVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, cursorVal), E_OK);
        EXPECT_EQ(cursorVal, std::to_string(++index));
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);

    /**
     * @tc.steps:step3. Update tracker columns,then check extend_field and cursor
     * @tc.expected: step3. Return OK.
     */
    BatchUpdateTableName2Data(updateNum, LOCAL_TABLE_TRACKER_NAME_SET2);
    stmt = nullptr;
    index = 0;
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, querySql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string extendVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, extendVal), E_OK);
        EXPECT_EQ(extendVal, "1");
        std::string cursorVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, cursorVal), E_OK);
        EXPECT_EQ(cursorVal, std::to_string(num + (++index)));
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest009
  * @tc.desc: Test set tracker table to check extend_field and cursor after delete
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest009, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return WITH_INVENTORY_DATA.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);

    /**
     * @tc.steps:step2. select extend_field and cursor after delete
     * @tc.expected: step2. Return OK.
     */
    BatchDeleteTableName2Data(num);
    CheckExtendAndCursor(num, 0);

    /**
     * @tc.steps:step3. Set tracker, where extendColNames changed, then check the extend_field of deleted data
     * @tc.expected: step3. Return OK.
     */
    EXPECT_EQ(g_delegate->SetTrackerTable(g_normalSchema2), OK);
    CheckExtendAndCursor(num, 0);

    /**
     * @tc.steps:step4. Set tracker, where extendColNames no changed, then check the extend_field of deleted data
     * @tc.expected: step4. Return OK.
     */
    EXPECT_EQ(g_delegate->SetTrackerTable(g_normalSchema3), OK);
    CheckExtendAndCursor(num, 0);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest010
  * @tc.desc: Test set tracker table after unSetTrackerTable
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest010, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. unSetTrackerTable
     * @tc.expected: step2. Return OK.
     */
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step3. SetTrackerTable again
     * @tc.expected: step3. Return OK.
     */
    schema.trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET2;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step4. operator data
     * @tc.expected: step4. Return OK.
     */
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET2);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest011
  * @tc.desc: Test CreateDistributedTable after set tracker table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest011, TestSize.Level1)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. CreateDistributedTable on table2 and insert data
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
    uint64_t num = 10;
    BatchInsertTableName2Data(num);

    /**
     * @tc.steps:step3. CreateDistributedTable on table2 again, but schema not change
     * @tc.expected: step3. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
    CheckExtendAndCursor(num, -num);

    /**
     * @tc.steps:step4. operator data on table2
     * @tc.expected: step4. Return OK.
     */
    std::string sql = "ALTER TABLE " + TABLE_NAME2 + " ADD COLUMN xxx INT";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
    CheckExtendAndCursor(num, 0);

    /**
     * @tc.steps:step5. unSetTrackerTable
     * @tc.expected: step5. Return OK.
     */
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step6. operator data on table2
     * @tc.expected: step6. Return OK.
     */
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest012
  * @tc.desc: Test CreateDistributedTable after set tracker table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest012, TestSize.Level1)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. CreateDistributedTable on table2 without data
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step3. CreateDistributedTable on table1
     * @tc.expected: step3. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME1, CLOUD_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step4. operator data on table2
     * @tc.expected: step4. Return OK.
     */
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);

    /**
     * @tc.steps:step5. unSetTrackerTable
     * @tc.expected: step5. Return OK.
     */
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step6. operator data on table2
     * @tc.expected: step6. Return OK.
     */
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest013
  * @tc.desc: Test CreateDistributedTable after clean table data
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest013, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data and SetTrackerTable on table2
     * @tc.expected: step1. Return WITH_INVENTORY_DATA.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);

    /**
     * @tc.steps:step2. CreateDistributedTable on table2
     * @tc.expected: step2. Return NOT_SUPPORT.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);

    /**
     * @tc.steps:step3. delete all data but keep the log , then CreateDistributedTable
     * @tc.expected: step3. Return OK.
     */
    BatchDeleteTableName2Data(num);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);
    BatchInsertTableName2Data(num);

    /**
     * @tc.steps:step4. Set tracker, where extendColNames changed, then check the extend_field of deleted data
     * @tc.expected: step4. Return OK.
     */
    EXPECT_EQ(g_delegate->SetTrackerTable(g_normalSchema2), OK);
    CheckExtendAndCursor(num, 0);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest014
  * @tc.desc: Test SetTrackerTable after CreateDistributedTable
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest014, TestSize.Level1)
{
    /**
     * @tc.steps:step1. CreateDistributedTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step2. SetTrackerTable on table2
     * @tc.expected: step2. Return OK.
     */
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step3. operator data and check extend_filed and cursor
     * @tc.expected: step3. Return OK.
     */
    uint64_t num = 10;
    int begin = -10;
    BatchInsertTableName2Data(num);
    CheckExtendAndCursor(num, begin);
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET2);

    /**
     * @tc.steps:step4. unSetTrackerTable
     * @tc.expected: step4. Return OK.
     */
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest015
  * @tc.desc: Test operator data on Distributed tracker table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest015, TestSize.Level1)
{
    /**
     * @tc.steps:step1. CreateDistributedTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step2. operator data
     * @tc.expected: step2. Return OK.
     */
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET2);

    /**
     * @tc.steps:step3. SetTrackerTable on table2
     * @tc.expected: step3. Return WITH_INVENTORY_DATA.
     */
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);
    string querySql = "select json_extract(extend_field, '$.name') from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where data_key = 15;";
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, querySql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string extendVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, extendVal), E_OK);
        EXPECT_EQ(extendVal, "Local4");
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);

    /**
     * @tc.steps:step4. operator data
     * @tc.expected: step4. Return OK.
     */
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET2);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest016
  * @tc.desc: Test operator data on tracker Distributed table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest016, TestSize.Level1)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. reopen store and create distributed table
     * @tc.expected: step2. Return OK.
     */
    CloseStore();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step3. operator data
     * @tc.expected: step3. Return OK.
     */
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET2);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest017
  * @tc.desc: Test set tracker table with DEVICE_COOPERATION mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest017, TestSize.Level1)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. Create DEVICE_COOPERATION DistributedTable
     * @tc.expected: step2. Return NOT_SUPPORT.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step3. operator data on table2
     * @tc.expected: step3. Return OK.
     */
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);

    /**
     * @tc.steps:step4. unSetTrackerTable
     * @tc.expected: step4. Return OK.
     */
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step5. There is still data in the table
     * @tc.expected: step5. Return NOT_SUPPORT.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), OK);

    /**
     * @tc.steps:step6. clear all data and create DEVICE_COOPERATION table
     * @tc.expected: step6. Return OK.
     */
    BatchDeleteTableName2Data(num + num);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), OK);

    /**
     * @tc.steps:step7. operator data on table2
     * @tc.expected: step7. Return OK.
     */
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest018
  * @tc.desc: Test set tracker table with DEVICE_COOPERATION mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest018, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Create DEVICE_COOPERATION DistributedTable
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step2. SetTrackerTable on table2
     * @tc.expected: step2. Return OK.
     */
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step3. operator data on table2
     * @tc.expected: step3. Return OK.
     */
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);

    /**
     * @tc.steps:step4. unSetTrackerTable
     * @tc.expected: step4. Return OK.
     */
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step5. operator data on table2
     * @tc.expected: step5. Return OK.
     */
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest019
  * @tc.desc: Test set tracker table with DEVICE_COOPERATION mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest019, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. CleanTrackerData with table name no exist
     * @tc.expected: step2. Return DB_ERROR.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    BatchDeleteTableName2Data(num / HALF);
    EXPECT_EQ(g_delegate->CleanTrackerData("xx", num), DB_ERROR);

    /**
     * @tc.steps:step3. CleanTrackerData with empty table name
     * @tc.expected: step3. Return INVALID_ARGS.
     */
    EXPECT_EQ(g_delegate->CleanTrackerData("", num), INVALID_ARGS);

    /**
     * @tc.steps:step4. CleanTrackerData
     * @tc.expected: step4. Return OK.
     */
    EXPECT_EQ(g_delegate->CleanTrackerData(TABLE_NAME2, num + (num / HALF)), OK);
    std::string sql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where extend_field is NULL;";
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest020
  * @tc.desc: Test drop and rebuild table in same delegate
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest020, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable and init data
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);

    /**
     * @tc.steps:step2. drop and rebuild table, then SetTrackerTable
     * @tc.expected: step2. Return OK.
     */
    std::string sql = "drop table if exists " + TABLE_NAME2;
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, CREATE_LOCAL_PK_TABLE_SQL), SQLITE_OK);
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step3. check the extend_field and cursor is null
     * @tc.expected: step3. Return OK.
     */
    sql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where extend_field is NULL " + " AND cursor is NULL";
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);

    /**
     * @tc.steps:step4. set diff schema, check the extend_field and cursor is null
     * @tc.expected: step4. Return OK.
     */
    schema.extendColNames = {EXTEND_COL_NAME3};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    sql = "select count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 +
        "_log where extend_field is NULL " + " AND cursor is NULL";
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest021
  * @tc.desc: Test non distributed table delete table and reopen db
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest021, TestSize.Level0)
{
    CheckDropTableAndReopenDb(false);
}

/**
  * @tc.name: TrackerTableTest022
  * @tc.desc: Test distributed table delete table and reopen db
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest022, TestSize.Level1)
{
    CheckDropTableAndReopenDb(true);
}

/**
  * @tc.name: TrackerTableTest023
  * @tc.desc: Test drop and rebuild table,then insert data and set tracker table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest023, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable and init data
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    BatchDeleteTableName2Data(num);

    /**
     * @tc.steps:step2. drop and rebuild table,then insert data and set tracker table
     * @tc.expected: step2. Return OK.
     */
    std::string sql = "drop table if exists " + TABLE_NAME2;
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, CREATE_LOCAL_PK_TABLE_SQL), SQLITE_OK);
    BatchInsertTableName2Data(num);
    schema = g_normalSchema2;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step3. query cursor
     * @tc.expected: step3. Return OK.
     */
    string querySql = "select cursor from " + std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 + "_log";
    sqlite3_stmt *stmt = nullptr;
    int index = 20;
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, querySql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string cursorVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, cursorVal), E_OK);
        EXPECT_EQ(cursorVal, std::to_string(++index));
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest024
  * @tc.desc: Test set tracker table and set extend col as the asset type
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest024, TestSize.Level0)
{
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema;
    schema.tableName = TABLE_NAME3;
    schema.extendColNames = {EXTEND_COL_NAME3};
    schema.trackerColNames = { EXTEND_COL_NAME3 };
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), INVALID_ARGS);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest025
  * @tc.desc: Test CreateDistributedTable after insert data and set tracker table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lijun
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest025, TestSize.Level0)
{
    CreateMultiTable();
    OpenStore();
    uint64_t num = 10;

    /**
     * @tc.steps:step1. insert data
     * @tc.expected: step1. OK.
     */
    BatchInsertTableName2Data(num);
    TrackerSchema schema = g_normalSchema1;

    /**
     * @tc.steps:step2. SetTrackerTable on table2
     * @tc.expected: step2. Return WITH_INVENTORY_DATA.
     */
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);

    /**
     * @tc.steps:step3. check cursor before and after CreateDistributedTable
     * @tc.expected: step3. Cursor is no change.
     */
    CheckExtendAndCursor(num, -num);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
    CheckExtendAndCursor(num, -num);
    CloseStore();
}

/**
  * @tc.name: ExecuteSql001
  * @tc.desc: Test ExecuteSql with invalid param
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();

    /**
     * @tc.steps:step2. sql is empty
     * @tc.expected: step2. Return INVALID_ARGS.
     */
    SqlCondition condition;
    std::vector<VBucket> records;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), INVALID_ARGS);

    /**
     * @tc.steps:step3. SQL does not have placeholders but there are bind args present
     * @tc.expected: step3. Return INVALID_ARGS.
     */
    std::string querySql = "select * from " + TABLE_NAME2 + " where id = 1;";
    condition.sql = querySql;
    condition.bindArgs = {"1"};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), INVALID_ARGS);

    /**
     * @tc.steps:step4. More SQL binding parameters than SQL placeholders
     * @tc.expected: step4. Return INVALID_ARGS.
     */
    querySql = "select * from " + TABLE_NAME2 + " where id > ?;";
    condition.sql = querySql;
    condition.bindArgs = {};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), INVALID_ARGS);

    /**
     * @tc.steps:step5. More SQL placeholders than SQL binding parameters
     * @tc.expected: step5. Return INVALID_ARGS.
     */
    condition.bindArgs = {"1", "2"};
    condition.sql = querySql;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), INVALID_ARGS);
    CloseStore();
}

/**
  * @tc.name: ExecuteSql002
  * @tc.desc: Test ExecuteSql and check query result
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();

    /**
     * @tc.steps:step2. execute query sql and check result
     * @tc.expected: step2. Return OK.
     */
    int64_t beginId = 1;
    SqlCondition condition;
    std::vector<VBucket> records;
    std::string querySql = "select * from " + TABLE_NAME2 + " where id > ?;";
    condition.sql = querySql;
    condition.bindArgs = {beginId};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), static_cast<size_t>(num - beginId));
    for (const VBucket &item: records) {
        auto iter = item.find("id");
        ASSERT_NE(iter, item.end());
        ASSERT_TRUE(iter->second.index() == TYPE_INDEX<int64_t>);
        EXPECT_EQ(std::get<int64_t>(iter->second), beginId + 1);

        iter = item.find("name");
        ASSERT_NE(iter, item.end());
        ASSERT_TRUE(iter->second.index() == TYPE_INDEX<std::string>);
        EXPECT_EQ(std::get<std::string>(iter->second), "Local" + std::to_string(beginId));

        iter = item.find("height");
        ASSERT_NE(iter, item.end());
        ASSERT_TRUE(iter->second.index() == TYPE_INDEX<double>);
        EXPECT_EQ(std::get<double>(iter->second), 175.8);

        iter = item.find("photo");
        ASSERT_NE(iter, item.end());
        ASSERT_TRUE(iter->second.index() == TYPE_INDEX<double>);
        EXPECT_EQ(std::get<double>(iter->second), 175.88888888888);

        iter = item.find("asserts");
        ASSERT_NE(iter, item.end());
        ASSERT_TRUE(iter->second.index() == TYPE_INDEX<std::string>);
        EXPECT_EQ(std::get<std::string>(iter->second), "x");
        beginId++;
    }
    CloseStore();
}

/**
  * @tc.name: ExecuteSql003
  * @tc.desc: Test ExecuteSql and check update and delete result
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();

    /**
     * @tc.steps:step2. update sql
     * @tc.expected: step2. Return OK.
     */
    int64_t beginId = 1;
    SqlCondition condition;
    std::vector<VBucket> records;
    std::string updateSql = "update " + TABLE_NAME2 + " set age = 3 where id = ?;";
    condition.sql = updateSql;
    condition.bindArgs = {beginId};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), 0u);

    std::string delSql = "delete from " + TABLE_NAME2 + " where id = ?;";
    condition.sql = delSql;
    condition.bindArgs = {beginId + 1};
    std::vector<VBucket> records2;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records2), OK);
    EXPECT_EQ(records2.size(), 0u);

    string insSql = "INSERT INTO " + TABLE_NAME2 +
        " (name, height, photo, asserts, age) VALUES ('Local" + std::to_string(num + 1) +
        "', '175.8', '0', 'x', ?);";
    condition.sql = insSql;
    condition.bindArgs = {beginId};
    std::vector<VBucket> records3;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records3), OK);
    EXPECT_EQ(records3.size(), 0u);
    CloseStore();
}

/**
  * @tc.name: ExecuteSql004
  * @tc.desc: Test ExecuteSql after SetTrackerTable
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. batch insert
     * @tc.expected: step2. Return OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);

    /**
     * @tc.steps:step3. execute query sql and check result
     * @tc.expected: step3. Return OK.
     */
    int64_t begin = 0;
    SqlCondition condition;
    std::vector<VBucket> records;
    std::string querySql = "select " + TABLE_NAME2 + ".* from " + TABLE_NAME2 + " join ";
    querySql += std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 + "_log" + " as a on " + TABLE_NAME2 +
        "._rowid_ = ";
    querySql += "a.data_key where a.cursor > ?;";
    condition.sql = querySql;
    condition.bindArgs = {begin};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), num);

    /**
     * @tc.steps:step4. update
     * @tc.expected: step4. Return OK.
     */
    std::string updateSql = "update " + TABLE_NAME2 + " set name = '3' where _rowid_ <= 5;";
    condition.sql = updateSql;
    condition.bindArgs = {};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);

    /**
     * @tc.steps:step5. query after updating
     * @tc.expected: step5. Return OK.
     */
    records.clear();
    begin = 10;
    condition.sql = querySql;
    condition.bindArgs = {begin};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), 5u); // 5 is the num of update
    CloseStore();
}

/**
  * @tc.name: ExecuteSql005
  * @tc.desc: Test ExecuteSql interface only takes effect on the first SQL
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();

    /**
     * @tc.steps:step2. execute query sql but the table is no exist
     * @tc.expected: step2. Return DB_ERROR.
     */
    int64_t beginId = 1;
    SqlCondition condition;
    std::vector<VBucket> records;
    std::string querySql = "select * from " + TABLE_NAME2 + " where id > 1;";
    querySql += "select _rowid_ from " + TABLE_NAME2 + " where id = 1;";
    condition.sql = querySql;
    condition.bindArgs = {};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), static_cast<size_t>(num - beginId));

    /**
     * @tc.steps:step3. execute multi query sql and the num of bindArgs is greater than the first sql
     * @tc.expected: step3. Return INVALID_ARGS.
     */
    records = {};
    std::string querySql2 = "select * from " + TABLE_NAME2 + " where id > ?;";
    querySql2 += "select _rowid_ from " + TABLE_NAME2 + " where id = ?;";
    condition.sql = querySql2;
    condition.bindArgs = {beginId, beginId};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), INVALID_ARGS);
    EXPECT_EQ(records.size(), 0u);

    /**
     * @tc.steps:step4. execute multi query sql and the num of bindArgs is equal to the first sql
     * @tc.expected: step4. Return OK.
     */
    records = {};
    condition.bindArgs = {beginId};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), static_cast<size_t>(num - beginId));
    CloseStore();
}

/**
  * @tc.name: ExecuteSql006
  * @tc.desc: Test ExecuteSql interface only takes effect on the first SQL
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();

    /**
     * @tc.steps:step2. execute multi update sql
     * @tc.expected: step2. Return OK.
     */
    SqlCondition condition;
    std::vector<VBucket> records;
    std::string updateSql = "update " + TABLE_NAME2 + " SET age = 100; ";
    updateSql += "update " + TABLE_NAME2 + " SET age = 50;";
    condition.sql = updateSql;
    condition.bindArgs = {};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);

    /**
     * @tc.steps:step3. execute query sql where age is 100
     * @tc.expected: step3. Return OK.
     */
    records = {};
    std::string querySql = "select * from " + TABLE_NAME2 + " where age = 100;";
    condition.sql = querySql;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), num);

    /**
     * @tc.steps:step4. execute update sql and query sql
     * @tc.expected: step4. Return OK.
     */
    updateSql = "update " + TABLE_NAME2 + " SET age = 88; ";
    updateSql += "select * from " + TABLE_NAME2;
    condition.sql = updateSql;
    condition.bindArgs = {};
    records = {};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), 0u);

    /**
     * @tc.steps:step5. execute multi delete sql
     * @tc.expected: step5. Return OK.
     */
    records = {};
    std::string delSql = "DELETE FROM " + TABLE_NAME2 + " WHERE age = 100; ";
    delSql += "DELETE FROM " + TABLE_NAME2 + " WHERE age = 88;";
    condition.sql = delSql;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);

    /**
     * @tc.steps:step6. execute query sql where age is 100
     * @tc.expected: step6. Return OK.
     */
    records = {};
    condition.sql = "select * from " + TABLE_NAME2 + " where age = 88;";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), num);
    CloseStore();
}

/**
  * @tc.name: ExecuteSql006
  * @tc.desc: Test ExecuteSql interface only takes effect on the first SQL
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();

    /**
     * @tc.steps:step2. ExecuteSql with transaction
     * @tc.expected: step2. Return OK.
     */
    SqlCondition condition;
    std::vector<VBucket> records;
    condition.sql = "BEGIN";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "COMMIT;";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "BEGIN";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "ROLLBACK;";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "BEGIN TRANSACTION;";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "END TRANSACTION;";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "BEGIN IMMEDIATE TRANSACTION;";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "COMMIT;";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);

    /**
     * @tc.steps:step3. ExecuteSql with attach and detach
     * @tc.expected: step3. Return OK.
     */
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID2 + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    condition.sql = "ATTACH DATABASE '" + g_dbDir + STORE_ID2 + DB_SUFFIX + "' AS TEST";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "DETACH DATABASE TEST";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    db = nullptr;
    CloseStore();
}

/**
  * @tc.name: ExecuteSql008
  * @tc.desc: Test the ExecSql interface for bool type results
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    uint64_t num = 10;
    for (size_t i = 0; i < num; i++) {
        string sql = "INSERT OR REPLACE INTO " + TABLE_NAME1 +
            " (name, height, married, photo, assert, age) VALUES ('Tom" + std::to_string(i) +
            "', '175.8', '0', '', '' , '18');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
    }

    /**
     * @tc.steps:step2. check if the result is of bool type
     * @tc.expected: step2. Return OK.
     */
    SqlCondition condition;
    condition.sql = "select * from " + TABLE_NAME1;
    std::vector<VBucket> records;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), num);
    EXPECT_NE(records[0].find("married"), records[0].end());
    if (records[0].find("married") != records[0].end()) {
        Type married = records[0].find("married")->second;
        EXPECT_TRUE(married.index() == TYPE_INDEX<bool>);
    }
    CloseStore();
}

/**
  * @tc.name: ExecuteSql009
  * @tc.desc: Test ExecuteSql update using the parameter "readOnly"
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql009, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();
    /**
     * @tc.steps:step2. ExecuteSql update using the parameter "readOnly"
     * @tc.expected: step2. Return OK.
     */
    SqlCondition condition;
    std::vector<VBucket> records;
    condition.readOnly = true;
    condition.sql = "update " + TABLE_NAME2 + " set name = 'new_name' where age = 18";
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    CloseStore();
}

/**
  * @tc.name: ExecuteSql010
  * @tc.desc: Test ExecuteSql with temp table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql010, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    OpenStore();
    SqlCondition condition;
    Bytes photo = { 1, 2, 3, 4 };
    std::vector<VBucket> records;
    for (size_t i = 0; i < num; i++) {
        condition.sql = "INSERT INTO " + TABLE_NAME2
            + " (name, height, photo, asserts, age) VALUES ('Local" + std::to_string(i) +
            "', '175.8', ?, 'x', '18');";
        condition.bindArgs = {photo};
        EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    }

    /**
     * @tc.steps:step2. ExecuteSql with transaction
     * @tc.expected: step2. Return OK.
     */
    condition.sql = "create temp table AA as select * from " + TABLE_NAME2;
    condition.bindArgs = {};
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.sql = "select * from " + TABLE_NAME2;
    condition.readOnly = true;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), num);

    /**
     * @tc.steps:step3. ExecuteSql again
     * @tc.expected: step3. Return OK.
     */
    records.clear();
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    EXPECT_EQ(records.size(), num);
    CloseStore();
}

/**
  * @tc.name: ExecuteSql011
  * @tc.desc: Test ExecuteSql concurrently
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */

HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, ExecuteSql011, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init db data
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    CreateMultiTable();
    BatchInsertTableName2Data(num);
    OpenStore();
    /**
     * @tc.steps:step2. ExecuteSql concurrently
     * @tc.expected: step2. Return OK.
     */
    std::thread readThread([&](){
        SqlCondition condition;
        std::vector<VBucket> records;
        condition.readOnly = true;
        condition.sql = "select * from " + TABLE_NAME2;
        for (int i = 0; i < 100; i++) {
            EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
        }
    });
    std::thread transactionThread([&](){
        SqlCondition condition;
        condition.readOnly = true;
        std::vector<VBucket> records;
        for (int i = 0; i < 100; i++) {
            condition.sql = "BEGIN;";
            EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
            condition.sql = "select * from " + TABLE_NAME2;
            EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
            condition.sql = "COMMIT;";
            EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
        }
    });
    readThread.join();
    transactionThread.join();
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest026
  * @tc.desc: Test tracker table with case sensitive table name
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest026, TestSize.Level1)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. SetTrackerTable on table2 with case different
     * @tc.expected: step2. Return NOT_FOUND.
     */
    schema.tableName = "worker2";
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);
    schema.tableName = "workeR2";
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);
    schema.trackerColNames = {};
    schema.tableName = "WorkeR2";
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);

    schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest027
  * @tc.desc: Test tracker table with case sensitive distributed table name
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest027, TestSize.Level1)
{
    /**
     * @tc.steps:step1. create distributed table on table2 with case different
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    EXPECT_EQ(g_delegate->CreateDistributedTable("worker2", CLOUD_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step2. SetTrackerTable on table2 with case different
     * @tc.expected: step2. Return NOT_FOUND.
     */
    schema.tableName = "Worker2";
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);
    uint64_t num = 10;
    BatchOperatorTableName2Data(num, LOCAL_TABLE_TRACKER_NAME_SET3);
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);
    schema.tableName = "WOrker2";
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);
    schema.tableName = "Worker2";
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);

    /**
     * @tc.steps:step3. SetTrackerTable with "worKer2"
     * @tc.expected: step3. Return NOT_FOUND.
     */
    schema.tableName = g_normalSchema1.tableName;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), NOT_FOUND);

    /**
     * @tc.steps:step4. SetTrackerTable with "worKer2" after reopening db
     * @tc.expected: step4. Return OK.
     */
    CloseStore();
    OpenStore();
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    schema.trackerColNames = {};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest028
  * @tc.desc: Test set tracker table colNames from not empty to empty
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest028, TestSize.Level1)
{
    /**
     * @tc.steps:step1. trackerColNames is empty
     * @tc.expected: step1. Return OK.
     */
    TrackerSchema schema;
    schema.tableName = TABLE_NAME2;
    SetTrackerTableTest(schema, OK);

    /**
     * @tc.steps:step2. trackerColNames is not empty
     * @tc.expected: step2. Return OK.
     */
    schema.extendColNames = {EXTEND_COL_NAME2};
    schema.trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET2;
    SetTrackerTableTest(schema, OK);

    /**
     * @tc.steps:step3. trackerColNames is empty and track action
     * @tc.expected: step3. Return OK.
     */
    schema.trackerColNames = {};
    schema.isTrackAction = true;
    SetTrackerTableTest(schema, OK);
    SetTrackerTableTest(schema, OK);

    OpenStore();
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    CheckExtendAndCursor(num, 0, schema.tableName, false);
    BatchUpdateTableName2Data(num, {"age"});
    CheckExtendAndCursor(num, num, schema.tableName, false);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest029
  * @tc.desc: Test set tracker table with force upgrade
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest029, TestSize.Level0)
{
    CreateMultiTable();
    OpenStore();
    /**
     * @tc.steps:step1. set tracker table
     * @tc.expected: step1. Return OK.
     */
    TrackerSchema schema;
    schema.tableName = TABLE_NAME2;
    schema.trackerColNames = LOCAL_TABLE_TRACKER_NAME_SET2;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step2. rebuild table and insert data
     * @tc.expected: step2. Return OK.
     */
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, "DROP TABLE " + schema.tableName), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, CREATE_LOCAL_PK_TABLE_SQL), SQLITE_OK);
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    /**
     * @tc.steps:step3. rebuild table and insert data
     * @tc.expected: step3. Return OK.
     */
    schema.isForceUpgrade = true;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest030
  * @tc.desc: Test clean trackTable when table is distributedTable
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: wangxiangdong
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest030, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();

    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2), OK);

    /**
     * @tc.steps:step2. Insert data to table2
     * @tc.expected: step2. Return E_OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    BatchDeleteTableName2Data(num / HALF);

    /**
     * @tc.steps:step3. CleanTrackerData
     * @tc.expected: step3. Return OK.
     */
    EXPECT_EQ(g_delegate->CleanTrackerData(TABLE_NAME2, num + (num / HALF)), OK);
    std::string sql = "select count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 + "_log" +
        " where extend_field is NULL;";
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num), nullptr), SQLITE_OK);
    CloseStore();
}

/**
 * @tc.name: TrackerTableTest031
 * @tc.desc: Test set tracker table with trackerColNames emtpy
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: luoguo
 */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest031, TestSize.Level0)
{
    CreateMultiTable();
    OpenStore();

    /**
     * @tc.steps:step1. create distributed table and set data.
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);

    /**
     * @tc.steps:step2. set tracker table
     * @tc.expected: step2. Return OK.
     */
    TrackerSchema schema;
    schema.tableName = TABLE_NAME2;
    schema.extendColNames = {EXTEND_COL_NAME2};
    schema.trackerColNames = {};
    schema.isForceUpgrade = false;
    schema.isTrackAction = true;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);

    /**
     * @tc.steps:step3. check cursor count.
     * @tc.expected: step3. Return OK.
     */
    std::string sql = "select count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 + "_log" +
                      " where extend_field is NULL;";
    EXPECT_EQ(sqlite3_exec(
                  g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback, reinterpret_cast<void *>(0u), nullptr),
        SQLITE_OK);
    CloseStore();
}

/**
 * @tc.name: TrackerTableTest032
 * @tc.desc: Test after create distributed table cursor increace
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest032, TestSize.Level0)
{
    CreateMultiTable();
    OpenStore();

    /**
     * @tc.steps:step1. create distributed table and set data.
     * @tc.expected: step1. Return OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);

    /**
     * @tc.steps:step2. check cursor count.
     * @tc.expected: step2. Return OK.
     */
    std::string sql = "select count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 + "_log" +
        " where cursor = 10;";
    EXPECT_EQ(sqlite3_exec(
                  g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback, reinterpret_cast<void *>(1u), nullptr),
        SQLITE_OK);
    CloseStore();
}

/**
 * @tc.name: TrackerTableTest033
 * @tc.desc: Test CreateDistributedTable after insert data and set tracker table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: luoguo
 */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest033, TestSize.Level0)
{
    CreateMultiTable();
    OpenStore();
    uint64_t num = 10;

    /**
     * @tc.steps:step1. insert data
     * @tc.expected: step1. OK.
     */
    BatchInsertTableName2Data(num);
    TrackerSchema schema = g_normalSchema1;

    /**
     * @tc.steps:step2. SetTrackerTable on table2 and set status to 2.
     * @tc.expected: step2. ok.
     */
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);
    std::string sql = "UPDATE " + std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 + "_log SET status = 2;";
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    /**
     * @tc.steps:step3. check status after CreateDistributedTable
     * @tc.expected: step3. status is 0.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
    sql = "select count(*) from " + std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 + "_log where status = 0;";
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num), nullptr), SQLITE_OK);
    CloseStore();
}

/**
 * @tc.name: TrackerTableTest034
 * @tc.desc: Test set tracker table with multi extend names
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest034, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Init db and set tracker table with multi extend names
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    schema.extendColNames = {EXTEND_COL_NAME2, EXTEND_COL_NAME3};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step2. Insert data to table2
     * @tc.expected: step2. Return E_OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    /**
     * @tc.steps:step3. Check extend_field
     * @tc.expected: step3. Return E_OK.
     */
    std::string checkValidJsonSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where json_valid(extend_field) = 1";
    EXPECT_EQ(sqlite3_exec(g_db, checkValidJsonSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num), nullptr), SQLITE_OK);

    std::string checkAgeSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where json_extract(extend_field, '$.age') = 18";
    EXPECT_EQ(sqlite3_exec(g_db, checkAgeSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num), nullptr), SQLITE_OK);

    for (uint64_t i = 0; i < num; i++) {
        std::string expectName = "Local" + std::to_string(i);
        std::string checkNameSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
            " where json_extract(extend_field, '$.name') = 'Local" + std::to_string(i) + "'";
        EXPECT_EQ(sqlite3_exec(g_db, checkNameSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(1u), nullptr), SQLITE_OK);
    }
    CloseStore();
}

/**
 * @tc.name: TrackerTableTest035
 * @tc.desc: Test set tracker table with multi extend names repeatedly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest035, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Init db and set tracker table with multi extend names
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);
    TrackerSchema schema = g_normalSchema1;
    schema.extendColNames = {EXTEND_COL_NAME3};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step2. Insert data to table2
     * @tc.expected: step2. Return E_OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    std::string sql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where json_valid(extend_field) = 1 and json_extract(extend_field, '$.name') is not null";
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0u), nullptr), SQLITE_OK);
    /**
     * @tc.steps:step3. Set tracker table repeatedly
     * @tc.expected: step3. Return E_OK.
     */
    schema.extendColNames = {EXTEND_COL_NAME2, EXTEND_COL_NAME3};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step4. Check extend_field
     * @tc.expected: step4. Return E_OK.
     */
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num), nullptr), SQLITE_OK);
    for (uint64_t i = 0; i < num; i++) {
        std::string expectName = "Local" + std::to_string(i);
        std::string checkNameSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
            " where json_extract(extend_field, '$.name') = 'Local" + std::to_string(i) + "'";
        EXPECT_EQ(sqlite3_exec(g_db, checkNameSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(1u), nullptr), SQLITE_OK);
    }
    CloseStore();
}

void SetLowVersionSchema(sqlite3 *db, const std::string &extendColName)
{
    std::string sql = "update naturalbase_rdb_aux_metadata set value = "
        "json_insert(value,'$.TABLES[0].EXTEND_NAME', '" + extendColName + "')"
        "where json_valid(value)=1 and json_extract(value, '$.TABLES[0].EXTEND_NAMES') is not null";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    sql = "update naturalbase_rdb_aux_metadata set value = json_remove(value,'$.TABLES[0].EXTEND_NAMES')"
        "where json_valid(value)=1 and json_extract(value, '$.TABLES[0].EXTEND_NAMES') is not null";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
}

/**
 * @tc.name: TrackerTableTest036
 * @tc.desc: Test Upgrade extend field
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest036, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Init db and init extend field to old version data
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);
    TrackerSchema schema = g_normalSchema1;
    schema.extendColNames = {EXTEND_COL_NAME3};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    std::string sql = "delete from " + TABLE_NAME2 + " where _rowid_ % 2 = 0";
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
    sql = "update " + DBCommon::GetLogTableName(TABLE_NAME2) + " set extend_field = 'old_age'";
    EXPECT_EQ(RelationalTestUtils::ExecSql(g_db, sql), SQLITE_OK);
    SetLowVersionSchema(g_db, "age");
    CloseStore();
    OpenStore();
    /**
     * @tc.steps:step2. Set tracker table
     * @tc.expected: step2. Return E_OK.
     */
    schema.extendColNames = {EXTEND_COL_NAME2, EXTEND_COL_NAME3};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step3. Check extend_field
     * @tc.expected: step3. Return E_OK.
     */
    std::string checkValidJsonSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where json_valid(extend_field) = 1";
    EXPECT_EQ(sqlite3_exec(g_db, checkValidJsonSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num), nullptr), SQLITE_OK);

    std::string checkAgeSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where json_extract(extend_field, '$.age') = 18";
    EXPECT_EQ(sqlite3_exec(g_db, checkAgeSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num / 2), nullptr), SQLITE_OK);
    checkAgeSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where json_extract(extend_field, '$.age') = 'old_age'";
    EXPECT_EQ(sqlite3_exec(g_db, checkAgeSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num / 2), nullptr), SQLITE_OK);

    checkAgeSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where json_extract(extend_field, '$.name') is null";
    EXPECT_EQ(sqlite3_exec(g_db, checkAgeSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num / 2), nullptr), SQLITE_OK);
    for (uint64_t i = 0; i < num; i += 2) {
        std::string expectName = "Local" + std::to_string(i);
        std::string checkNameSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
            " where json_extract(extend_field, '$.name') = 'Local" + std::to_string(i) + "'";
        EXPECT_EQ(sqlite3_exec(g_db, checkNameSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(1u), nullptr), SQLITE_OK);
    }
    CloseStore();
}

/**
 * @tc.name: TrackerTableTest037
 * @tc.desc: Test open low version db which extend name is empty
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest037, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Init db
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);
    TrackerSchema schema = g_normalSchema1;
    schema.extendColNames = {EXTEND_COL_NAME3};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step2. Set schema as low version which extend name is empty
     * @tc.expected: step2. Return E_OK.
     */
    SetLowVersionSchema(g_db, "");
    /**
     * @tc.steps:step3. close and open DB
     * @tc.expected: step3. Return E_OK.
     */
    CloseStore();
    OpenStore();
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest038
  * @tc.desc: Test create distributed table with DEVICE_COOPERATION mode then set tracker table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: tankaisheng
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest038, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create DEVICE_COOPERATION DistributedTable
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), DBStatus::OK);

    /**
     * @tc.steps:step2. SetTrackerTable on table2
     * @tc.expected: step2. Return OK.
     */
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step3. Insert data to table2 then check tracker table data
     * @tc.expected: step3. Return E_OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    std::string checkInsertSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where cursor='10';";
    ASSERT_EQ(sqlite3_exec(g_db, checkInsertSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1), nullptr), SQLITE_OK);

    /**
     * @tc.steps:step4. Update data to table2 then check tracker table data
     * @tc.expected: step4. Return E_OK.
     */
    uint64_t updateNum = 2;
    BatchUpdateTableName2Data(updateNum, LOCAL_TABLE_TRACKER_NAME_SET2);
    int index = 0;
    string checkUpdateSql = "select json_extract(extend_field, '$.name'), cursor from " +
       DBCommon::GetLogTableName(TABLE_NAME2) + " where data_key <= " + std::to_string(updateNum);
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, checkUpdateSql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string extendVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, extendVal), E_OK);
        EXPECT_EQ(extendVal, "1");
        std::string cursorVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 1, cursorVal), E_OK);
        EXPECT_EQ(cursorVal, std::to_string(num + (++index)));
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);

    /**
     * @tc.steps:step5. Delete data to table2 then check tracker table data
     * @tc.expected: step5. Return E_OK.
     */
    BatchDeleteTableName2Data(num / HALF);
    std::string checkDeleteSql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " where cursor='17';";
    ASSERT_EQ(sqlite3_exec(g_db, checkDeleteSql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1), nullptr), SQLITE_OK);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest039
  * @tc.desc: Test SetTrackerTable repeatedly and delete trigger
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest039, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Init db
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step2. delete triggers
     * @tc.expected: step2. Return OK.
     */
    std::vector<std::string> triggerTypes = {"INSERT", "UPDATE", "DELETE"};
    for (const auto &triggerType : triggerTypes) {
        std::string sql = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + TABLE_NAME2 + "_ON_" + triggerType;
        SQLiteUtils::ExecuteRawSQL(g_db, sql);
    }
    /**
     * @tc.steps:step3. SetTrackerTable repeatedly
     * @tc.expected: step3. Return OK.
     */
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step4. Check if the trigger exists
     * @tc.expected: step4. Check OK.
     */
    for (const auto &triggerType : triggerTypes) {
        std::string sql = "select count(*) from sqlite_master where type = 'trigger' and tbl_name = '" + TABLE_NAME2 +
            "' and name = 'naturalbase_rdb_" + TABLE_NAME2 + "_ON_" + triggerType + "';";
        EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
            reinterpret_cast<void *>(1), nullptr), SQLITE_OK);
    }
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest040
  * @tc.desc: Test set tracker table with invalid col name
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest040, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Init db and init extend field to old version data
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), OK);
    /**
     * @tc.steps:step2. Set tracker table with invalid col name
     * @tc.expected: step2. Return E_OK.
     */
    TrackerSchema schema = g_normalSchema1;
    schema.extendColNames = {EXTEND_COL_NAME1};
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), SCHEMA_MISMATCH);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest042
  * @tc.desc: tracker table update timestamp
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: tankaisheng
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest042, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable
     * @tc.expected: step1. Return OK.
     */
    TrackerSchema schema = g_normalSchema1;
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);

    /**
     * @tc.steps:step2. Insert data to table2
     * @tc.expected: step2. Return E_OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(
        g_db, "select timestamp,wtimestamp from naturalbase_rdb_aux_worKer2_log where data_key = 1", stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t beforTime = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    int64_t beforWTime = static_cast<int64_t>(sqlite3_column_int64(stmt, 1));
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);

    /**
     * @tc.steps:step3. CreateDistributedTable then checkout data
     * @tc.expected: step3. Return E_OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), OK);
    stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(
        g_db, "select timestamp,wtimestamp from naturalbase_rdb_aux_worKer2_log where data_key = 1", stmt), E_OK);
    ASSERT_EQ(SQLiteUtils::StepWithRetry(stmt, false), SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    int64_t afterTime = static_cast<int64_t>(sqlite3_column_int64(stmt, 0));
    int64_t afterWTime = static_cast<int64_t>(sqlite3_column_int64(stmt, 1));
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    EXPECT_NE(beforTime, afterTime);
    EXPECT_NE(beforWTime, afterWTime);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest043
  * @tc.desc: Test whether to save syncType after setting up the tracking table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest043, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable
     * @tc.expected: step1. Return OK.
     */
    TrackerSchema schema = g_normalSchema1;
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step2. Check sync type in metatable
     * @tc.expected: step2. Return E_OK.
     */
    std::string metaTableName = "naturalbase_rdb_aux_metadata";
    std::string sql = "select count(*) from " + metaTableName + " where key = ?";
    sqlite3_stmt* stmt = nullptr;
    SQLiteUtils::GetStatement(g_db, sql, stmt);
    std::string keyStr = "sync_table_type_" + schema.tableName;
    Key key(keyStr.begin(), keyStr.end());
    SQLiteUtils::BindBlobToStatement(stmt, 1, key);
    SQLiteUtils::StepWithRetry(stmt);

    int count = static_cast<int>(sqlite3_column_int(stmt, 0));
    EXPECT_EQ(count, 1);
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    CloseStore();
}

void CheckCursor(int begin, int end, sqlite3 *db)
{
    std::string querySql = "select cursor from " + DBCommon::GetLogTableName(TABLE_NAME2) + " order by data_key;";
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(db, querySql, stmt), E_OK);
    int64_t cursor = begin;
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string extendVal;
        int64_t actualCursor = sqlite3_column_int64(stmt, 0);
        EXPECT_EQ(actualCursor, cursor);
        cursor++;
    }
    EXPECT_EQ(cursor, end + 1);
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

/**
  * @tc.name: TrackerTableTest044
  * @tc.desc: Test SetTrackerTable and CreateDistributedTable when there is data in the table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest044, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    /**
     * @tc.steps:step2. CreateDistributedTable on table2 and insert data
     * @tc.expected: step2. Return OK.
     */
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    CheckCursor(1, 10, g_db);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), DBStatus::OK);
    /**
     * @tc.steps:step3. Check log table
     * @tc.expected: step3. Return OK.
     */
    string querySql = "select json_extract(extend_field, '$.name') from " + DBCommon::GetLogTableName(TABLE_NAME2) +
        " order by data_key;";
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, querySql, stmt), E_OK);
    int count = 0;
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string extendVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, extendVal), E_OK);
        EXPECT_EQ(extendVal, "Local" + std::to_string(count));
        count++;
    }
    EXPECT_EQ(count, 10);
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    CheckCursor(11, 20, g_db);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest045
  * @tc.desc: Test SetTrackerTable and CreateDistributedTable when there is data in the table
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest045, TestSize.Level0)
{
    /**
     * @tc.steps:step1. CreateDistributedTable on table2 and insert data
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    uint64_t num = 10;
    BatchInsertTableName2Data(num);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), DBStatus::OK);
    /**
     * @tc.steps:step2. Check cursor
     * @tc.expected: step2. Return OK.
     */
    CheckCursor(1, 10, g_db);
    /**
     * @tc.steps:step3. SetTrackerTable on table2
     * @tc.expected: step3. Return WITH_INVENTORY_DATA.
     */
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);
    /**
     * @tc.steps:step4. Check cursor
     * @tc.expected: step4. Return OK.
     */
    CheckCursor(11, 20, g_db);
    CloseStore();
}

/**
  * @tc.name: TrackerTableTest046
  * @tc.desc: Test clear log of mismatched data
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest046, TestSize.Level0)
{
    /**
     * @tc.steps:step1. SetTrackerTable on table2
     * @tc.expected: step1. Return OK.
     */
    CreateMultiTable();
    OpenStore();
    int num = 10;
    BatchInsertTableName2Data(num);
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);

    /**
     * @tc.steps:step2. Recreate table2 by rename
     * @tc.expected: step2. Return OK.
     */
    std::string sql = "ALTER TABLE " + TABLE_NAME2 + " RENAME TO xxx";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, CREATE_LOCAL_PK_TABLE_SQL), E_OK);
    sql = "INSERT INTO " + TABLE_NAME2 + " SELECT id,name,height,photo,asserts,age FROM xxx";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    sql = "DROP TABLE xxx";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);
    sql = "DELETE FROM " + TABLE_NAME2 + " WHERE id in ('7', '9')";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(g_db, sql), E_OK);

    /**
     * @tc.steps:step3. Set tracker and check log count
     * @tc.expected: step3. Return OK.
     */
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    sql = "select count(*) from " + DBCommon::GetLogTableName(TABLE_NAME2);
    num = 8;
    EXPECT_EQ(sqlite3_exec(g_db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(num), nullptr), SQLITE_OK);
    CloseStore();
}

/**
  * @tc.name: SchemaStrTest001
  * @tc.desc: Test open reOpen stroe when schemaStr is empty
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, SchemaStrTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. set empty for relational schema str, reopen store
     * @tc.expected: step1. Return OK.
     */
    std::string updMetaSql = "UPDATE naturalbase_rdb_aux_metadata SET value=? where key=?;";
    CreateMultiTable();
    OpenStore();
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
    SqlCondition condition;
    std::vector<VBucket> records;
    condition.sql = updMetaSql;
    Key relationKey;
    DBCommon::StringToVector(DBConstant::RELATIONAL_SCHEMA_KEY, relationKey);
    condition.bindArgs = { std::string(""), relationKey };
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    CloseStore();
    OpenStore();

    /**
     * @tc.steps:step2. set empty for relational schema str, reopen store to upgrade
     * @tc.expected: step2. Return OK.
     */
    Value verVal;
    DBCommon::StringToVector("5.0", verVal);
    Key verKey;
    DBCommon::StringToVector("log_table_version", verKey);
    condition.bindArgs = { verVal, verKey };
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.bindArgs = { std::string(""), relationKey };
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    CloseStore();
    OpenStore();

    /**
     * @tc.steps:step3. set empty for tracker schema str, reopen store to upgrade
     * @tc.expected: step3. Return OK.
     */
    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    condition.bindArgs = { verVal, verKey };
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    condition.bindArgs = { std::string(""), relationKey };
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    Key trackerKey;
    DBCommon::StringToVector("relational_tracker_schema", trackerKey);
    condition.bindArgs = { std::string(""), trackerKey };
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    CloseStore();
    OpenStore();

    /**
     * @tc.steps:step4. try to create distributed table and set tracker table again
     * @tc.expected: step4. Return OK.
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, CLOUD_COOPERATION), DBStatus::OK);
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME1, CLOUD_COOPERATION), DBStatus::OK);
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), OK);
    CloseStore();
}

/**
 * @tc.name: TrackerTableTest041
 * @tc.desc: Test cursor increases when set tracker table after create distributed table by DEVICE_COOPERATION type 
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBInterfacesRelationalTrackerTableTest, TrackerTableTest041, TestSize.Level0)
{
    CreateMultiTable();
    OpenStore();

    /**
     * @tc.steps:step1. create distributed table by DEVICE_COOPERATION type and insert data
     * @tc.expected: step1. return OK
     */
    EXPECT_EQ(g_delegate->CreateDistributedTable(TABLE_NAME2, DEVICE_COOPERATION), DBStatus::OK);
    BatchInsertTableName2Data(10); // insert 10 data

    /**
     * @tc.steps:step2. set tracker table on table2 and check cursor
     * @tc.expected: step2. cursor increases
     */
    string querySql = "select cursor from " + std::string(DBConstant::RELATIONAL_PREFIX) + TABLE_NAME2 + "_log";
    sqlite3_stmt *stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, querySql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string cursorVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, cursorVal), E_OK);
        EXPECT_EQ(cursorVal, "0");
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);

    TrackerSchema schema = g_normalSchema1;
    EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);
    int index = 0;
    stmt = nullptr;
    EXPECT_EQ(SQLiteUtils::GetStatement(g_db, querySql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        std::string cursorVal;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, cursorVal), E_OK);
        EXPECT_EQ(cursorVal, std::to_string(++index));
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    CloseStore();
}
}
