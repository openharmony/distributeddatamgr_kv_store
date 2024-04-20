/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cloud_db_types.h"
#include "distributeddb_tools_unit_test.h"
#include "runtime_config.h"
#include "sqlite_relational_utils.h"
#include "virtual_cloud_data_translate.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    constexpr const char *CREATE_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS worker1(" \
        "id INT PRIMARY KEY,"
        "name TEXT," \
        "height REAL ," \
        "married BOOLEAN ," \
        "photo BLOB," \
        "asset BLOB," \
        "assets BLOB);";
    const Asset g_localAsset = {
        .version = 1, .name = "Phone", .assetId = "", .subpath = "/local/sync", .uri = "/local/sync",
        .modifyTime = "123456", .createTime = "", .size = "256", .hash = "ASE"
    };
    std::string g_testDir;
    std::string g_dbDir;
    sqlite3 *g_db = nullptr;

class DistributedDBSqliteRelationalUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    void CreateUserDBAndTable();
    void PrepareStatement(const std::string &sql, sqlite3_stmt *&statement, bool hasCloudDataTranslate);
    std::shared_ptr<VirtualCloudDataTranslate> virtualCloudDataTranslate_ = nullptr;
};

void DistributedDBSqliteRelationalUtilsTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGI("The test db is:%s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
}

void DistributedDBSqliteRelationalUtilsTest::TearDownTestCase(void)
{
}

void DistributedDBSqliteRelationalUtilsTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    g_db = RelationalTestUtils::CreateDataBase(g_dbDir + "test.db");
    CreateUserDBAndTable();
}

void DistributedDBSqliteRelationalUtilsTest::TearDown()
{
    sqlite3_close_v2(g_db);
    g_db = nullptr;
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

void DistributedDBSqliteRelationalUtilsTest::CreateUserDBAndTable()
{
    ASSERT_EQ(RelationalTestUtils::ExecSql(g_db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    ASSERT_EQ(RelationalTestUtils::ExecSql(g_db, CREATE_TABLE_SQL), SQLITE_OK);
}

void DistributedDBSqliteRelationalUtilsTest::PrepareStatement(const std::string &sql, sqlite3_stmt *&statement,
    bool hasCloudDataTranslate)
{
    ASSERT_EQ(SQLiteUtils::GetStatement(g_db, sql, statement), E_OK);
    VBucket data;
    data.insert_or_assign("id", 1L);
    data.insert_or_assign("name", "lihua");
    data.insert_or_assign("height", 166.0); // 166.0 is random double value
    data.insert_or_assign("married", true);
    std::vector<uint8_t> photo(1, 'v');
    data.insert_or_assign("photo", photo);
    data.insert_or_assign("asset", g_localAsset);
    Assets assets = { g_localAsset };
    data.insert_or_assign("assets", assets);
    EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 1, data["id"]), E_OK); // 1 is id cid
    EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 2, data["name"]), E_OK); // 2 is name cid
    EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 3, data["height"]), E_OK); // 3 is height cid
    EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 4, data["married"]), E_OK); // 4 is married cid
    EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 5, data["photo"]), E_OK); // 5 is photo cid
    if (hasCloudDataTranslate) {
        EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 6, data["asset"]), E_OK); // 6 is asset cid
        EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 7, data["assets"]), E_OK); // 7 is assets cid
    } else {
        RuntimeConfig::SetCloudTranslate(nullptr);
        EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 6, data["asset"]), -E_NOT_INIT); // 6 is asset
        EXPECT_EQ(SQLiteRelationalUtils::BindStatementByType(statement, 7, data["assets"]), -E_NOT_INIT); // 7 is assets
    }
}

/**
 * @tc.name: SqliteRelationalUtilsTest001
 * @tc.desc: Test interfaces when statement is nullptr
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBSqliteRelationalUtilsTest, SqliteRelationalUtilsTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. statement is nullptr
     * @tc.expected: step1. return OK
     */
    sqlite3_stmt *statement = nullptr;

    /**
     * @tc.steps:step2. test interface
     * @tc.expected: step2. return OK
     */
    DataValue dataValue;
    EXPECT_EQ(SQLiteRelationalUtils::GetDataValueByType(statement, 0, dataValue), -E_INVALID_ARGS);
    Type typeValue;
    EXPECT_EQ(SQLiteRelationalUtils::GetCloudValueByType(statement, TYPE_INDEX<Nil>, 0, typeValue), -E_INVALID_ARGS);
    EXPECT_EQ(SQLiteUtils::StepNext(statement, false), -E_INVALID_ARGS);
    std::string tableName = "worker1";
    EXPECT_NE(SQLiteRelationalUtils::SelectServerObserver(g_db, tableName, false), E_OK);

    /**
     * @tc.steps:step3. close db and test interface
     * @tc.expected: step3. return OK
     */
    sqlite3_close_v2(g_db);
    g_db = nullptr;
    std::string fileName = "";
    EXPECT_FALSE(SQLiteRelationalUtils::GetDbFileName(g_db, fileName));
    EXPECT_EQ(SQLiteRelationalUtils::SelectServerObserver(g_db, tableName, false), -E_INVALID_ARGS);
}

/**
 * @tc.name: SqliteRelationalUtilsTest002
 * @tc.desc: Test BindStatementByType if dataTranselate is nullptr
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBSqliteRelationalUtilsTest, SqliteRelationalUtilsTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. prepare sql and bind statement
     * @tc.expected: step1. return OK
     */
    std::string sql = "INSERT OR REPLACE INTO worker1(id, name, height, married, photo, asset, assets)" \
        "VALUES (?,?,?,?,?,?,?);";
    sqlite3_stmt *statement = nullptr;
    PrepareStatement(sql, statement, false);
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(statement, true, errCode);
    ASSERT_EQ(statement, nullptr);
}

/**
 * @tc.name: SqliteRelationalUtilsTest003
 * @tc.desc: Test GetSelectVBucket
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBSqliteRelationalUtilsTest, SqliteRelationalUtilsTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. set CloudDataTranslate
     * @tc.expected: step1. return OK
     */
    virtualCloudDataTranslate_ = std::make_shared<VirtualCloudDataTranslate>();
    RuntimeConfig::SetCloudTranslate(virtualCloudDataTranslate_);

    /**
     * @tc.steps:step2. insert data
     * @tc.expected: step2. return OK
     */
    std::string sql = "INSERT OR REPLACE INTO worker1(id, name, height, married, photo, asset, assets)" \
        "VALUES (?,?,?,?,?,?,?);";
    sqlite3_stmt *statement = nullptr;
    PrepareStatement(sql, statement, true);
    EXPECT_EQ(SQLiteUtils::StepWithRetry(statement), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(statement, true, errCode);
    ASSERT_EQ(statement, nullptr);

    /**
     * @tc.steps:step3. test GetSelectVBucket
     * @tc.expected: step3. return OK
     */
    VBucket data;
    ASSERT_EQ(SQLiteRelationalUtils::GetSelectVBucket(statement, data), -E_INVALID_ARGS);
    sql = "SELECT id, name, height, married, photo, asset, assets from worker1;";
    ASSERT_EQ(SQLiteUtils::GetStatement(g_db, sql, statement), E_OK);
    EXPECT_EQ(SQLiteRelationalUtils::GetSelectVBucket(statement, data), E_OK);
    SQLiteUtils::ResetStatement(statement, true, errCode);
    ASSERT_EQ(statement, nullptr);
}
}