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
#include "sqlite_log_table_manager.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    string g_dbDir;
    sqlite3 *g_db = nullptr;

    const int MAX_BLOB_READ_SIZE = 64 * 1024 * 1024; // 64M limit
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

/**
 * @tc.name: KVLogUpgrade001
 * @tc.desc: Get blob size over limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSqliteUtilsTest, KVLogUpgrade001, TestSize.Level0)
{
    const std::string tableName = "naturalbase_kv_aux_sync_data_log";
    const std::string primaryKey = "PRIMARY KEY(userid, hash_key)";
    std::string createTableSql = "CREATE TABLE IF NOT EXISTS " + tableName + "(" \
        "userid    TEXT NOT NULL," + \
        "hash_key  BLOB NOT NULL," + \
        "cloud_gid TEXT," + \
        "version   TEXT," + \
        primaryKey + ");";
    int errCode = SQLiteUtils::ExecuteRawSQL(g_db, createTableSql);
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(SqliteLogTableManager::CreateKvSyncLogTable(g_db), E_OK);
}

/**
 * @tc.name: BindErr
 * @tc.desc: Test bind err
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lijun
 */
HWTEST_F(DistributedDBSqliteUtilsTest, BindErr, TestSize.Level1)
{
    NativeSqlite::ExecSql(g_db, "CREATE TABLE IF NOT EXISTS t1 (a INT, b TEXT);");

    std::string text;
    text.resize(0);
    NativeSqlite::ExecSql(g_db, "INSERT INTO t1 VALUES(?, ?)", [&text](sqlite3_stmt *stmt) {
        (void)SQLiteUtils::BindInt64ToStatement(stmt, 1, 1);
        (void)SQLiteUtils::BindTextToStatement(stmt, 2, "text"); // 2: bind index
        EXPECT_NE(SQLiteUtils::BindTextToStatement(stmt, 5, text), E_OK);
        EXPECT_EQ(SQLiteUtils::BindTextToStatement(nullptr, 2, text), -E_INVALID_ARGS);
        EXPECT_NE(SQLiteUtils::BindInt64ToStatement(nullptr, -6, 0), E_OK);
        EXPECT_NE(SQLiteUtils::BindBlobToStatement(stmt, 7, {}), E_OK);
        EXPECT_NE(SQLiteUtils::BindPrefixKey(stmt, 8, {}), E_OK);
        EXPECT_NE(SQLiteUtils::BindPrefixKey(stmt, 2, {}), E_OK);
        return E_OK;
    }, nullptr);

    NativeSqlite::ExecSql(g_db, "SELECT b FROM t1", nullptr, [&text](sqlite3_stmt *stmt) {
        std::string val;
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(nullptr, 0, val), -E_INVALID_ARGS);
        EXPECT_EQ(SQLiteUtils::GetColumnTextValue(stmt, 0, val), E_OK);
        EXPECT_EQ(val, text);
        return E_OK;
    });
}

/**
 * @tc.name: AbnormalBranchErr
 * @tc.desc: Test abnormal branch error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lijun
 */
HWTEST_F(DistributedDBSqliteUtilsTest, AbnormalBranchErr, TestSize.Level1)
{
    CipherPassword passwd;
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::SetKey(nullptr, CipherType::DEFAULT, passwd, true,
        DBConstant::DEFAULT_ITER_TIMES));
    EXPECT_EQ(-1, SQLiteUtils::AttachNewDatabaseInner(g_db, CipherType::DEFAULT, {}, "path", "asname ? "));
    EXPECT_EQ(-E_SQLITE_CANT_OPEN, SQLiteUtils::CreateMetaDatabase("/axxbxxc/d sdsda"));

    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::CheckIntegrity(nullptr, ""));
    TableInfo table;
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::AnalysisSchema(nullptr, "", table, true));
    EXPECT_EQ(-E_NOT_SUPPORT, SQLiteUtils::AnalysisSchema(g_db, "###", table, true));
    EXPECT_EQ(-E_NOT_FOUND, SQLiteUtils::AnalysisSchema(g_db, "t2", table, true));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::AnalysisSchemaFieldDefine(nullptr, "ttt", table));

    int version;
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::GetVersion(nullptr, version));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::SetUserVer(nullptr, version));
    std::string mode;
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::GetJournalMode(nullptr, mode));

    EXPECT_EQ(-SQLITE_IOERR, SQLiteUtils::MapSQLiteErrno(SQLITE_IOERR));
    int oldErr = errno;
    errno = EKEYREVOKED;
    EXPECT_EQ(-E_EKEYREVOKED, SQLiteUtils::MapSQLiteErrno(SQLITE_IOERR));
    EXPECT_EQ(-E_EKEYREVOKED, SQLiteUtils::MapSQLiteErrno(SQLITE_ERROR));
    errno = oldErr;
    EXPECT_EQ(-E_SQLITE_CANT_OPEN, SQLiteUtils::MapSQLiteErrno(SQLITE_CANTOPEN));
    std::string strSchema;
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::SaveSchema(nullptr, strSchema));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::GetSchema(nullptr, strSchema));

    IndexName name;
    IndexInfo info;
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::IncreaseIndex(nullptr, name, info, SchemaType::JSON, 1));
    EXPECT_EQ(-E_NOT_PERMIT, SQLiteUtils::IncreaseIndex(g_db, name, info, SchemaType::JSON, 1));
    EXPECT_EQ(-E_NOT_PERMIT, SQLiteUtils::ChangeIndex(g_db, name, info, SchemaType::JSON, 1));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::ChangeIndex(nullptr, name, info, SchemaType::JSON, 1));

    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::RegisterJsonFunctions(nullptr));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::CloneIndexes(nullptr, strSchema, strSchema));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::GetRelationalSchema(nullptr, strSchema));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::GetLogTableVersion(nullptr, strSchema));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::RegisterFlatBufferFunction(nullptr, strSchema));
    EXPECT_EQ(-E_INTERNAL_ERROR, SQLiteUtils::RegisterFlatBufferFunction(g_db, strSchema));
    EXPECT_EQ(-E_INVALID_ARGS, SQLiteUtils::ExpandedSql(nullptr, strSchema));
    SQLiteUtils::ExecuteCheckPoint(nullptr);
    
    bool isEmpty;
    EXPECT_EQ(-E_INVALID_ARGS, SQLiteUtils::CheckTableEmpty(nullptr, strSchema, isEmpty));
    EXPECT_EQ(-E_INVALID_ARGS, SQLiteUtils::SetPersistWalMode(nullptr));
    EXPECT_EQ(-1, SQLiteUtils::GetLastRowId(nullptr));
    std::string tableName;
    EXPECT_EQ(-1, SQLiteUtils::CheckTableExists(nullptr, tableName, isEmpty));
}

/**
 * @tc.name: AbnormalBranchErrAfterClose
 * @tc.desc: Test abnormal branch error after close db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lijun
 */
HWTEST_F(DistributedDBSqliteUtilsTest, AbnormalBranchErrAfterClose, TestSize.Level1)
{
    int version;
    std::string mode;
    std::string strSchema;
    std::string tableName;
    bool isEmpty;

    sqlite3_close_v2(g_db);
    // After close db
    EXPECT_EQ(-SQLITE_MISUSE, SQLiteUtils::GetVersion(g_db, version));
    EXPECT_EQ(-SQLITE_MISUSE, SQLiteUtils::GetJournalMode(g_db, mode));
    EXPECT_EQ(-SQLITE_MISUSE, SQLiteUtils::SaveSchema(g_db, strSchema));
    EXPECT_EQ(-SQLITE_MISUSE, SQLiteUtils::GetSchema(g_db, strSchema));
    EXPECT_EQ(-SQLITE_MISUSE, SQLiteUtils::GetRelationalSchema(g_db, strSchema));
    EXPECT_EQ(-SQLITE_MISUSE, SQLiteUtils::GetLogTableVersion(g_db, strSchema));
    EXPECT_EQ(-SQLITE_MISUSE, SQLiteUtils::CheckTableExists(g_db, tableName, isEmpty));
}
