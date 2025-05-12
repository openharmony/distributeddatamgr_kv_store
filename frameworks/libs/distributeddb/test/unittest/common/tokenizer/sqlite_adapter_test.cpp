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

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <sstream>

#include "log_print.h"
#ifndef USE_SQLITE_SYMBOLS
#include "sqlite3.h"
#else
#include "sqlite3sym.h"
#endif

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;

class SqliteAdapterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SqliteAdapterTest::SetUpTestCase(void)
{}

void SqliteAdapterTest::TearDownTestCase(void)
{}

void SqliteAdapterTest::SetUp()
{}

void SqliteAdapterTest::TearDown()
{}

using Clock = std::chrono::system_clock;

static bool g_needSkip = false;

static void HandleRc(sqlite3 *db, int rc)
{
    if (rc != SQLITE_OK) {
        LOGE("Failed!!!!!!!!!! rc: %d", rc);
        g_needSkip = true;
    }
}
static sqlite3 *g_sqliteDb = nullptr;
static char *g_errMsg = nullptr;

static int Callback(void *data, int argc, char **argv, char **colName)
{
    for (int i = 0; i < argc; i++) {
        LOGD("%s = %s\n", colName[i], argv[i] ? argv[i] : "nullptr");
    }
    return 0;  // 返回 0 表示继续执行查询
}

static void SQLTest(const char *sql)
{
    if (g_needSkip) {
        // 蓝区没有so导致失败，直接跳过测试
        return;
    }
    int errCode = sqlite3_exec(g_sqliteDb, sql, Callback, nullptr, &g_errMsg);
    if (errCode != SQLITE_OK) {
        if (g_errMsg != nullptr) {
            LOGE("SQL error: %s\n", g_errMsg);
            sqlite3_free(g_errMsg);
            g_errMsg = nullptr;
            ASSERT_TRUE(false);
        }
    }
}

static int g_rank = 0;
static const char *g_dbPath = "test.db";

/**
 * @tc.name: SqliteAdapterTest001
 * @tc.desc: Get blob size over limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wanghaishuo
 */
HWTEST_F(SqliteAdapterTest, SqliteAdapterTest001, TestSize.Level0)
{
    // Save any error messages
    char *zErrMsg = nullptr;

    // Save the connection result
    int rc = sqlite3_open_v2(g_dbPath, &g_sqliteDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    HandleRc(g_sqliteDb, rc);

    auto before = Clock::now();

    rc = sqlite3_db_config(g_sqliteDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_load_extension(g_sqliteDb, "libcustomtokenizer.z.so", nullptr, nullptr);
    HandleRc(g_sqliteDb, rc);

    // create fts table
    string sql = "CREATE VIRTUAL TABLE example USING fts5(name, content, tokenize = 'customtokenizer')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    HandleRc(g_sqliteDb, rc);

    const char *SQLINSERT1 =
        "INSERT INTO example(name, content) VALUES('文档1', '这是一个测试文档，用于测试中文文本的分词和索引。');";
    SQLTest(SQLINSERT1);
    const char *SQLINSERT2 =
        "INSERT INTO example(name, content) VALUES('文档2', '我们将使用这个示例来演示如何在SQLite中进行全文搜索。');";
    SQLTest(SQLINSERT2);
    const char *SQLINSERT3 =
        "INSERT INTO example(name, content) VALUES('文档3', 'ICU分词器能够很好地处理中文文本的分词和分析。');";
    SQLTest(SQLINSERT3);
    const char *SQLINSERT4 = "INSERT INTO example(name, content) VALUES('uksdhf', '');";
    SQLTest(SQLINSERT4);
    const char *SQLINSERT5 = "INSERT INTO example(name, content) VALUES('', '');";
    SQLTest(SQLINSERT5);
    const char *SQLQUERY1 = "SELECT * FROM example WHERE example MATCH '分* OR SQLite';";
    SQLTest(SQLQUERY1);

    const char *SQLQUERY2 = "SELECT * FROM example WHERE example MATCH '分词' ORDER BY rank;";
    SQLTest(SQLQUERY2);

    const char *SQLQUERY3 = "SELECT * FROM example WHERE name MATCH 'uksdhf';";
    SQLTest(SQLQUERY3);

    const char *SQLDROP = "DROP TABLE IF EXISTS example;";
    SQLTest(SQLDROP);

    // Close the connection
    sqlite3_close(g_sqliteDb);
}
 