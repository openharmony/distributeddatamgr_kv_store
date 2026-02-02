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
#include <sys/stat.h>

#include "distributeddb_tools_unit_test.h"
#include "kvdb_utils.h"
#include "native_sqlite.h"
#include "platform_specific.h"
#include "sqlite_cloud_kv_executor_utils.h"
#include "sqlite_import.h"
#include "sqlite_log_table_manager.h"
#include "sqlite_local_storage_executor.h"
#include "sqlite_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    string g_dbDir;
    sqlite3 *g_db = nullptr;
    std::string g_dirAll;
    std::string g_dirStoreOnly;
    std::string g_dbName = "test.db";
    std::string g_name = "test";

    const int MAX_BLOB_READ_SIZE = 64 * 1024 * 1024; // 64M limit
    const int MAX_TEXT_READ_SIZE = 5 * 1024 * 1024; // 5M limit

    void CreateTestDbFiles()
    {
        g_dirAll = g_dbDir + "all";
        g_dirStoreOnly = g_dbDir + "store";
        EXPECT_EQ(mkdir(g_dirAll.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
        EXPECT_EQ(mkdir(g_dirStoreOnly.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
        std::string dbFileNameAll = g_dirAll + '/' + g_dbName;
        std::ofstream dbAllFile(dbFileNameAll.c_str(), std::ios::out | std::ios::binary);
        ASSERT_TRUE(dbAllFile.is_open());
        dbAllFile.close();
    }
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
    const std::string createTableSql = "CREATE TABLE IF NOT EXISTS data (key BLOB PRIMARY KEY, value BLOB);";
    EXPECT_EQ(sqlite3_exec(g_db, createTableSql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
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
 * @tc.name: SQLiteLocalStorageExecutorTest001
 * @tc.desc: Test SQLiteLocalStorageExecutor construction
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, SQLiteLocalStorageExecutorTest001, TestSize.Level0)
{
    SQLiteLocalStorageExecutor executor(g_db, true, false);
    EXPECT_EQ(sqlite3_close(g_db), SQLITE_OK);
    EXPECT_EQ(sqlite3_open("test.db", &g_db), SQLITE_OK);
}

/**
 * @tc.name: SQLiteLocalStorageExecutorTest002
 * @tc.desc: Test SQLiteLocalStorageExecutor Put and Get
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, SQLiteLocalStorageExecutorTest002, TestSize.Level0)
{
    SQLiteLocalStorageExecutor executor(g_db, true, false);
    Key key = { 0x01 };
    Value value = { 0x02 };
    EXPECT_EQ(executor.Put(key, value), E_OK);
    Value outValue;
    EXPECT_EQ(executor.Get(key, outValue), E_OK);
    EXPECT_EQ(value, outValue);
    Key unknownKey = { 0x03 };
    EXPECT_EQ(executor.Get(unknownKey, outValue), -E_NOT_FOUND);
    SQLiteLocalStorageExecutor invalidExecutor(nullptr, true, false);
    EXPECT_EQ(invalidExecutor.Get(key, outValue), -E_INVALID_DB);
    EXPECT_EQ(invalidExecutor.Put(key, value), -E_INVALID_DB);
    Key emptyKey = {};
    EXPECT_EQ(executor.Get(emptyKey, outValue), -E_INVALID_ARGS);
    EXPECT_EQ(executor.Put(emptyKey, value), -E_INVALID_ARGS);
}

/**
 * @tc.name: SQLiteLocalStorageExecutorTest003
 * @tc.desc: Test SQLiteLocalStorageExecutor PutBatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, SQLiteLocalStorageExecutorTest003, TestSize.Level0)
{
    SQLiteLocalStorageExecutor executor(g_db, true, false);
    std::vector<Entry> entries;
    Key key1 = { 0x01 };
    Value value1 = { 0x02 };
    Entry entry1;
    entry1.key = key1;
    entry1.value = value1;
    Key key2 = { 0x03 };
    Value value2 = { 0x04 };
    Entry entry2;
    entry2.key = key2;
    entry2.value = value2;
    entries.push_back(entry1);
    entries.push_back(entry2);
    EXPECT_EQ(executor.PutBatch(entries), E_OK);
    Value outValue;
    EXPECT_EQ(executor.Get(key1, outValue), E_OK);
    EXPECT_EQ(outValue, value1);
    EXPECT_EQ(executor.Get(key2, outValue), E_OK);
    EXPECT_EQ(outValue, value2);
    std::vector<Entry> emptyEntries;
    EXPECT_EQ(executor.PutBatch(emptyEntries), -E_INVALID_ARGS);
    Key invalidKey;
    Entry invalidEntry;
    invalidEntry.key = invalidKey;
    invalidEntry.value = value2;
    entries.push_back(invalidEntry);
    EXPECT_EQ(executor.PutBatch(entries), -E_INVALID_ARGS);
}

/**
 * @tc.name: SQLiteLocalStorageExecutorTest004
 * @tc.desc: Test SQLiteLocalStorageExecutor Delete and GetEntries
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, SQLiteLocalStorageExecutorTest004, TestSize.Level0)
{
    SQLiteLocalStorageExecutor executor(g_db, true, false);
    Key key = { 0x01 };
    Key key1 = { 0x01, 0x02 };
    Key key2 = { 0x01, 0x03 };
    Value value;
    executor.Put(key1, value);
    executor.Put(key2, value);
    std::vector<Entry> entries;
    EXPECT_EQ(executor.Delete(key1), E_OK);
    EXPECT_EQ(executor.GetEntries(key, entries), E_OK);
    EXPECT_EQ(entries.size(), 1);
    SQLiteLocalStorageExecutor invalidExecutor(nullptr, true, false);
    EXPECT_EQ(invalidExecutor.GetEntries(key, entries), -E_INVALID_DB);
    Key invalidKey;
    invalidKey.resize(DBConstant::MAX_KEY_SIZE + 1);
    EXPECT_EQ(executor.GetEntries(invalidKey, entries), -E_SECUREC_ERROR);
    std::vector<Entry> emptyEntries = {};
    EXPECT_EQ(executor.Delete(key2), E_OK);
    EXPECT_EQ(executor.GetEntries(key, emptyEntries), -E_NOT_FOUND);
    Key emptyKey = {};
    EXPECT_EQ(executor.Delete(emptyKey), -E_INVALID_ARGS);
}

/**
 * @tc.name: SQLiteLocalStorageExecutorTest005
 * @tc.desc: Test SQLiteLocalStorageExecutor Transaction
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, SQLiteLocalStorageExecutorTest005, TestSize.Level0)
{
    SQLiteLocalStorageExecutor executor(g_db, true, false);
    Key key = { 0x01 };
    Value value = { 0x02 };
    EXPECT_EQ(executor.Put(key, value), E_OK);
    EXPECT_EQ(executor.Clear(), E_OK);
    Value outValue;
    EXPECT_EQ(executor.Get(key, outValue), -E_NOT_FOUND);
    EXPECT_EQ(executor.StartTransaction(), E_OK);
    EXPECT_EQ(executor.Put(key, value), E_OK);
    EXPECT_EQ(executor.RollBack(), E_OK);
    EXPECT_EQ(executor.Get(key, outValue), -E_NOT_FOUND);
}

/**
 * @tc.name: SQLiteLocalStorageExecutorTest006
 * @tc.desc: Test SQLiteLocalStorageExecutor DeleteBatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, SQLiteLocalStorageExecutorTest006, TestSize.Level0)
{
    SQLiteLocalStorageExecutor executor(g_db, true, false);
    Key key1 = { 0x01, 0x02 };
    Key key2 = { 0x01, 0x03 };
    Value value;
    executor.Put(key1, value);
    executor.Put(key2, value);
    std::vector<Key> keys;
    keys.push_back(key1);
    keys.push_back(key2);
    std::vector<Key> emptyKeys = {};
    EXPECT_EQ(executor.DeleteBatch(emptyKeys), -E_INVALID_ARGS);
    SQLiteLocalStorageExecutor invalidExecutor(nullptr, true, false);
    EXPECT_EQ(invalidExecutor.DeleteBatch(keys), -E_INVALID_DB);
    EXPECT_EQ(executor.DeleteBatch(keys), E_OK);
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
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::CheckTableExists(nullptr, tableName, isEmpty));
    EXPECT_EQ(-E_INVALID_DB, SQLiteUtils::PrintChangeRows(nullptr));
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

#ifdef USE_DISTRIBUTEDDB_CLOUD
/**
 * @tc.name: AbnormalSqliteCloudKvExecutorUtilsTest001
 * @tc.desc: Test sqlite cloud kv executor utils abnormal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBSqliteUtilsTest, AbnormalSqliteCloudKvExecutorUtilsTest001, TestSize.Level1)
{
    SqliteCloudKvExecutorUtils cloudKvObj;
    sqlite3 *db = nullptr;
    CloudSyncData data;
    CloudUploadRecorder recorder;

    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/test.db";
    EXPECT_EQ(sqlite3_open_v2(fileUrl.c_str(), &db, flag, nullptr), SQLITE_OK);
    ASSERT_NE(db, nullptr);

    data.isCloudVersionRecord = false;
    int ret = cloudKvObj.FillCloudLog({db, true}, OpType::INSERT, data, "", recorder);
    EXPECT_EQ(ret, -1); // -1 for table not exist

    std::vector<VBucket> dataVector;
    ret = cloudKvObj.GetWaitCompensatedSyncDataPk(db, true, dataVector);
    EXPECT_NE(ret, E_OK);
    std::vector<VBucket> dataUser;
    ret = cloudKvObj.GetWaitCompensatedSyncDataUserId(db, true, dataUser);
    EXPECT_NE(ret, E_OK);
    ret = cloudKvObj.GetWaitCompensatedSyncData(db, true, dataVector, dataUser);
    EXPECT_NE(ret, E_OK);
    std::vector<std::string> gid;
    std::string user("USER");
    QuerySyncObject query;
    ret = cloudKvObj.QueryCloudGid(db, true, user, query, gid);
    EXPECT_NE(ret, E_OK);
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}
#endif

/**
 * @tc.name: CheckIntegrityTest001
 * @tc.desc: Test CheckIntegrity function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, CheckIntegrityTest001, TestSize.Level0)
{
    CipherPassword passwd;
    CipherType invalidType = static_cast<CipherType>(999);
    EXPECT_EQ(SQLiteUtils::CheckIntegrity(g_dbDir, invalidType, passwd), -E_INVALID_ARGS);
}

/**
 * @tc.name: GetVersionTest001
 * @tc.desc: Test GetVersion function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetVersionTest001, TestSize.Level0)
{
    DistributedDB::OpenDbProperties properties;
    properties.uri = "";
    int version = 0;
    EXPECT_EQ(SQLiteUtils::GetVersion(properties, version), -E_INVALID_ARGS);
}

/**
 * @tc.name: SetUserVerTest001
 * @tc.desc: Test SetUserVer function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, SetUserVerTest001, TestSize.Level0)
{
    DistributedDB::OpenDbProperties properties;
    properties.uri = "";
    int version = 0;
    EXPECT_EQ(SQLiteUtils::SetUserVer(properties, version), -E_INVALID_ARGS);
}

/**
 * @tc.name: GetStoreDirectoryTest001
 * @tc.desc: Test GetStoreDirectory function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetStoreDirectoryTest001, TestSize.Level0)
{
    std::string directory = "data";
    std::string identifierName = "myDatabase";
    std::string expected = "data/myDatabase";
    KvDBUtils::GetStoreDirectory(directory, identifierName);
    EXPECT_EQ(directory, expected);
}

/**
 * @tc.name: RemoveKvDBTest001
 * @tc.desc: Test RemoveKvDB function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, RemoveKvDBTest001, TestSize.Level0)
{
    CreateTestDbFiles();
    std::string dbFileNameStore = g_dirStoreOnly + '/' + g_dbName;
    std::ofstream dbStoreFile(dbFileNameStore.c_str(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(dbStoreFile.is_open());
    dbStoreFile.close();
    EXPECT_EQ(chmod(g_dirAll.c_str(), (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |S_IXOTH)), E_OK);
    EXPECT_EQ(chmod(g_dirStoreOnly.c_str(), (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |S_IXOTH)), E_OK);
    if (access(g_dirAll.c_str(), W_OK) == 0 || access(g_dirStoreOnly.c_str(), W_OK) == 0) {
        LOGD("Modifying permissions is ineffective for execution\n");
        EXPECT_EQ(chmod(g_dirAll.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
        EXPECT_EQ(chmod(g_dirStoreOnly.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
    } else {
        EXPECT_EQ(KvDBUtils::RemoveKvDB(g_dirAll, g_dirStoreOnly, g_name), -E_REMOVE_FILE);
        EXPECT_EQ(chmod(g_dirAll.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
        EXPECT_EQ(KvDBUtils::RemoveKvDB(g_dirAll, g_dirStoreOnly, g_name), -E_REMOVE_FILE);
        EXPECT_EQ(chmod(g_dirStoreOnly.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
    }
}

/**
 * @tc.name: GetKvDbSizeTest001
 * @tc.desc: Test GetKvDbSize function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetKvDbSizeTest001, TestSize.Level0)
{
    uint64_t size = 0;
    CreateTestDbFiles();
    EXPECT_EQ(chmod(g_dirAll.c_str(), (S_IRUSR | S_IRGRP | S_IROTH)), E_OK);
    if (access(g_dirAll.c_str(), W_OK) == 0) {
        LOGD("Modifying permissions is ineffective for execution\n");
        EXPECT_EQ(chmod(g_dirAll.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
    } else {
        EXPECT_EQ(KvDBUtils::GetKvDbSize(g_dirAll, g_dirStoreOnly, g_name, size), -E_INVALID_DB);
        EXPECT_EQ(chmod(g_dirAll.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
    }
}

/**
 * @tc.name: GetKvDbSizeTest002
 * @tc.desc: Test GetKvDbSize function
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBSqliteUtilsTest, GetKvDbSizeTest002, TestSize.Level0)
{
    uint64_t size = 0;
    CreateTestDbFiles();
    std::string dbFileNameStore = g_dirStoreOnly + '/' + g_dbName;
    std::ofstream dbStoreFile(dbFileNameStore.c_str(), std::ios::out | std::ios::binary);
    ASSERT_TRUE(dbStoreFile.is_open());
    dbStoreFile.close();
    EXPECT_EQ(chmod(g_dirStoreOnly.c_str(), (S_IRUSR | S_IRGRP | S_IROTH)), E_OK);
    if (access(g_dirStoreOnly.c_str(), W_OK) == 0) {
        LOGD("Modifying permissions is ineffective for execution\n");
        EXPECT_EQ(chmod(g_dirStoreOnly.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
    } else {
        EXPECT_EQ(KvDBUtils::GetKvDbSize(g_dirAll, g_dirStoreOnly, g_name, size), -E_INVALID_DB);
        EXPECT_EQ(chmod(g_dirStoreOnly.c_str(), (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)), E_OK);
    }
}

/**
 * @tc.name: AttachSQLiteTest001
 * @tc.desc: Test attach sqlite with
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSqliteUtilsTest, AttachSQLiteTest001, TestSize.Level0)
{
    // create db1 first
    auto db1 = NativeSqlite::CreateDataBase(g_dbDir + "test1.db");
    ASSERT_NE(db1, nullptr);
    auto ret = sqlite3_close_v2(db1);
    ASSERT_EQ(ret, SQLITE_OK);
    // create db2 and attach db1
    auto db2 = NativeSqlite::CreateDataBase(g_dbDir + "test2.db");
    ASSERT_NE(db2, nullptr);
    ret = SQLiteUtils::AttachNewDatabase(db2, CipherType::DEFAULT, {}, g_dbDir + "test1.db", "test1");
    EXPECT_EQ(ret, E_OK);
    // wal not exist because not operate test1.db
    EXPECT_FALSE(OS::CheckPathExistence(g_dbDir + "test1.db-wal"));
    ret = SQLiteUtils::ExecuteRawSQL(db2, "CREATE TABLE IF NOT EXISTS test1.TEST(COL1 INTEGER PRIMARY KEY)");
    EXPECT_EQ(ret, E_OK);
    ret = sqlite3_close_v2(db2);
    ASSERT_EQ(ret, SQLITE_OK);
    // check db1 db2 wal exist
    EXPECT_TRUE(OS::CheckPathExistence(g_dbDir + "test1.db-wal"));
    EXPECT_FALSE(OS::CheckPathExistence(g_dbDir + "test2.db-wal"));
    ret = SQLiteUtils::AttachNewDatabase(nullptr, CipherType::DEFAULT, {}, g_dbDir + "testxx.db");
    EXPECT_EQ(ret, -E_INVALID_DB);
}