/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "distributeddb_tools_unit_test.h"
#include "native_sqlite.h"
#include "platform_specific.h"
#include "sqlite_import.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    string g_dbDir;
    sqlite3 *g_db = nullptr;

    const int MAX_BLOB_READ_SIZE = 5 * 1024 * 1024; // 5M limit
    const int MAX_TEXT_READ_SIZE = 5 * 1024 * 1024; // 5M limit
}

class DistributedDBSqliteUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSqliteUtilsTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGI("The test db is:%s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
}

void DistributedDBSqliteUtilsTest::TearDownTestCase(void)
{
}

void DistributedDBSqliteUtilsTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();

    g_db = NativeSqlite::CreateDataBase(g_dbDir + "test.db");
}

void DistributedDBSqliteUtilsTest::TearDown()
{
    sqlite3_close_v2(g_db);
    g_db = nullptr;
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

/**
 * @tc.name: GetBlobTest001
 * @tc.desc: Get blob size over limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetBlobTest001, TestSize.Level1)
{
    NativeSqlite::ExecSql(g_db, "CREATE TABLE IF NOT EXISTS t1 (a INT, b BLOB);");

    vector<uint8_t> blob;
    blob.resize(MAX_BLOB_READ_SIZE + 2); // 2: over limit
    std::fill(blob.begin(), blob.end(), static_cast<uint8_t>('a'));

    NativeSqlite::ExecSql(g_db, "INSERT INTO t1 VALUES(?, ?)", [&blob](sqlite3_stmt *stmt) {
        (void)SQLiteUtils::BindInt64ToStatement(stmt, 1, 1);
        (void)SQLiteUtils::BindBlobToStatement(stmt, 2, blob); // 2: bind index
        return E_OK;
    }, nullptr);

    NativeSqlite::ExecSql(g_db, "SELECT b FROM t1", nullptr, [](sqlite3_stmt *stmt) {
        Value val;
        EXPECT_EQ(SQLiteUtils::GetColumnBlobValue(stmt, 0, val), E_OK);
        EXPECT_EQ(static_cast<int>(val.size()), MAX_BLOB_READ_SIZE + 1);
        return E_OK;
    });
}

/**
 * @tc.name: GetBlobTest002
 * @tc.desc: Get blob size equal limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetBlobTest002, TestSize.Level1)
{
    NativeSqlite::ExecSql(g_db, "CREATE TABLE IF NOT EXISTS t1 (a INT, b BLOB);");

    vector<uint8_t> blob;
    blob.resize(MAX_BLOB_READ_SIZE);
    std::fill(blob.begin(), blob.end(), static_cast<uint8_t>('a'));

    NativeSqlite::ExecSql(g_db, "INSERT INTO t1 VALUES(?, ?)", [&blob](sqlite3_stmt *stmt) {
        (void)SQLiteUtils::BindInt64ToStatement(stmt, 1, 1);
        (void)SQLiteUtils::BindBlobToStatement(stmt, 2, blob); // 2: bind index
        return E_OK;
    }, nullptr);

    NativeSqlite::ExecSql(g_db, "SELECT b FROM t1", nullptr, [&blob](sqlite3_stmt *stmt) {
        Value val;
        EXPECT_EQ(SQLiteUtils::GetColumnBlobValue(stmt, 0, val), E_OK);
        EXPECT_EQ(val, blob);
        return E_OK;
    });
}

/**
 * @tc.name: GetBlobTest003
 * @tc.desc: Get blob size equal zero
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetBlobTest003, TestSize.Level1)
{
    NativeSqlite::ExecSql(g_db, "CREATE TABLE IF NOT EXISTS t1 (a INT, b BLOB);");

    vector<uint8_t> blob;
    blob.resize(0);

    NativeSqlite::ExecSql(g_db, "INSERT INTO t1 VALUES(?, ?)", [&blob](sqlite3_stmt *stmt) {
        (void)SQLiteUtils::BindInt64ToStatement(stmt, 1, 1);
        (void)SQLiteUtils::BindBlobToStatement(stmt, 2, blob); // 2: bind index
        return E_OK;
    }, nullptr);

    NativeSqlite::ExecSql(g_db, "SELECT b FROM t1", nullptr, [&blob](sqlite3_stmt *stmt) {
        Value val;
        EXPECT_EQ(SQLiteUtils::GetColumnBlobValue(stmt, 0, val), E_OK);
        EXPECT_EQ(val, blob);
        return E_OK;
    });
}

/**
 * @tc.name: GetTextTest001
 * @tc.desc: Get text size over limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetTextTest001, TestSize.Level1)
{
    NativeSqlite::ExecSql(g_db, "CREATE TABLE IF NOT EXISTS t1 (a INT, b TEXT);");

    std::string text;
    text.resize(MAX_TEXT_READ_SIZE + 2); // 2: over limit
    std::fill(text.begin(), text.end(), 'a');

    NativeSqlite::ExecSql(g_db, "INSERT INTO t1 VALUES(?, ?)", [&text](sqlite3_stmt *stmt) {
        (void)SQLiteUtils::BindInt64ToStatement(stmt, 1, 1);
        (void)SQLiteUtils::BindTextToStatement(stmt, 2, text); // 2: bind index
        return E_OK;
    }, nullptr);

    NativeSqlite::ExecSql(g_db, "SELECT b FROM t1", nullptr, [](sqlite3_stmt *stmt) {
        std::string val;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, val), E_OK);
        EXPECT_EQ(static_cast<int>(val.size()), MAX_TEXT_READ_SIZE + 1);
        return E_OK;
    });
}

/**
 * @tc.name: GetTextTest002
 * @tc.desc: Get text size equal limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetTextTest002, TestSize.Level1)
{
    NativeSqlite::ExecSql(g_db, "CREATE TABLE IF NOT EXISTS t1 (a INT, b TEXT);");

    std::string text;
    text.resize(MAX_TEXT_READ_SIZE);
    std::fill(text.begin(), text.end(), 'a');

    NativeSqlite::ExecSql(g_db, "INSERT INTO t1 VALUES(?, ?)", [&text](sqlite3_stmt *stmt) {
        (void)SQLiteUtils::BindInt64ToStatement(stmt, 1, 1);
        (void)SQLiteUtils::BindTextToStatement(stmt, 2, text); // 2: bind index
        return E_OK;
    }, nullptr);

    NativeSqlite::ExecSql(g_db, "SELECT b FROM t1", nullptr, [&text](sqlite3_stmt *stmt) {
        std::string val;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, val), E_OK);
        EXPECT_EQ(val, text);
        return E_OK;
    });
}

/**
 * @tc.name: GetTextTest003
 * @tc.desc: Get text size equal zero
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetTextTest003, TestSize.Level1)
{
    NativeSqlite::ExecSql(g_db, "CREATE TABLE IF NOT EXISTS t1 (a INT, b TEXT);");

    std::string text;
    text.resize(0);

    NativeSqlite::ExecSql(g_db, "INSERT INTO t1 VALUES(?, ?)", [&text](sqlite3_stmt *stmt) {
        (void)SQLiteUtils::BindInt64ToStatement(stmt, 1, 1);
        (void)SQLiteUtils::BindTextToStatement(stmt, 2, text); // 2: bind index
        return E_OK;
    }, nullptr);

    NativeSqlite::ExecSql(g_db, "SELECT b FROM t1", nullptr, [&text](sqlite3_stmt *stmt) {
        std::string val;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, val), E_OK);
        EXPECT_EQ(val, text);
        return E_OK;
    });
}