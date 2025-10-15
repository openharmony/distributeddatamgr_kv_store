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
#include <filesystem>
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

static const char *g_dbPath = "test.db";

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

namespace fs = std::filesystem;

void SqliteAdapterTest::SetUp()
{
    fs::remove(g_dbPath);
}

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

static int QueryCallback(void *data, int argc, char **argv, char **colName)
{
    if (argc != 1) {
        return 0;
    }
    auto expectCount = reinterpret_cast<int64_t>(data);
    int base = 10;
    EXPECT_EQ(expectCount, strtol(argv[0], nullptr, base));
    return 0;
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

/**
 * @tc.name: SqliteAdapterTest002
 * @tc.desc: Test cut for short words
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(SqliteAdapterTest, SqliteAdapterTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare db
     * @tc.expected: step1. OK.
     */
    // Save any error messages
    char *zErrMsg = nullptr;

    // Save the connection result
    int rc = sqlite3_open_v2(g_dbPath, &g_sqliteDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_db_config(g_sqliteDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_load_extension(g_sqliteDb, "libcustomtokenizer.z.so", nullptr, nullptr);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step2. create table
     * @tc.expected: step2. OK.
     */
    string sql = "CREATE VIRTUAL TABLE example USING fts5(content, tokenize = 'customtokenizer cut_mode short_words')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step3. insert records
     * @tc.expected: step3. OK.
     */
    std::vector<std::string> records = {
        "电子邮件",
        "这是一封电子邮件",
        "这是一封关于少数民族的电子邮件",
        "华中师范大学是一所位于武汉市的全日制综合性师范大学",
        "中华人民共和国",
        "武汉市长江大桥Wuhan Yangtze River Bridge是武汉市最长的桥"
    };
    for (const auto &record : records) {
        std::string insertSql = "insert into example values('" + record + "');";
        SQLTest(insertSql.c_str());
    }
    /**
     * @tc.steps: step4. test cut for short words
     * @tc.expected: step4. OK.
     */
    std::vector<std::pair<std::string, int>> expectResult = {
        {"电子", 3}, {"邮件", 3}, {"电子邮件", 3}, {"少数", 1}, {"民族", 1}, {"少数民族", 1}, {"华中", 1},
        {"中师", 1}, {"师范", 1}, {"共和", 1}, {"共和国", 1}, {"人民共和国", 0}, {"Yangtze", 1}, {"Wuhan", 1},
        {"市长", 0}
    };
    // 蓝区没有so导致失败，直接跳过测试
    if (!g_needSkip) {
        for (const auto &[word, expectMatchNum] : expectResult) {
            std::string querySql = "SELECT count(*) FROM example WHERE content MATCH '" + word + "';";
            EXPECT_EQ(sqlite3_exec(g_sqliteDb, querySql.c_str(), QueryCallback,
                reinterpret_cast<void*>(expectMatchNum), nullptr), SQLITE_OK);
        }
    }

    const char *SQLDROP = "DROP TABLE IF EXISTS example;";
    SQLTest(SQLDROP);
    EXPECT_EQ(sqlite3_close(g_sqliteDb), SQLITE_OK);
}

/**
 * @tc.name: SqliteAdapterTest003
 * @tc.desc: Test invalid args
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(SqliteAdapterTest, SqliteAdapterTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare db
     * @tc.expected: step1. OK.
     */
    // Save any error messages
    char *zErrMsg = nullptr;

    // Save the connection result
    int rc = sqlite3_open_v2(g_dbPath, &g_sqliteDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_db_config(g_sqliteDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_load_extension(g_sqliteDb, "libcustomtokenizer.z.so", nullptr, nullptr);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step2. create table which invalid args
     * @tc.expected: step2. return SQLITE_ERROR.
     */
    string sql = "CREATE VIRTUAL TABLE example USING fts5(content, tokenize = 'customtokenizer cut_mode')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    EXPECT_EQ(SQLITE_ERROR, rc);

    sql = "CREATE VIRTUAL TABLE example USING fts5(content, tokenize = 'customtokenizer cut_mode xxx xxx')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    EXPECT_EQ(SQLITE_ERROR, rc);

    sql = "CREATE VIRTUAL TABLE example USING fts5(content, tokenize = 'customtokenizer cut_mode xxx')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    EXPECT_EQ(SQLITE_ERROR, rc);

    sql = "CREATE VIRTUAL TABLE example USING fts5(content, tokenize = 'customtokenizer xxx short_words')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    EXPECT_EQ(SQLITE_ERROR, rc);

    const char *SQLDROP = "DROP TABLE IF EXISTS example;";
    SQLTest(SQLDROP);
    EXPECT_EQ(sqlite3_close(g_sqliteDb), SQLITE_OK);
}

/**
 * @tc.name: SqliteAdapterTest004
 * @tc.desc: Test cut_mode default
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(SqliteAdapterTest, SqliteAdapterTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare db
     * @tc.expected: step1. OK.
     */
    // Save any error messages
    char *zErrMsg = nullptr;

    // Save the connection result
    int rc = sqlite3_open_v2(g_dbPath, &g_sqliteDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_db_config(g_sqliteDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_load_extension(g_sqliteDb, "libcustomtokenizer.z.so", nullptr, nullptr);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step2. create table
     * @tc.expected: step2. OK.
     */
    string sql = "CREATE VIRTUAL TABLE example USING fts5(content, tokenize = 'customtokenizer cut_mode default')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step3. insert records
     * @tc.expected: step3. OK.
     */
    std::vector<std::string> records = {
        "电子邮件",
        "这是一封电子邮件",
        "这是一封关于少数民族的电子邮件"
    };
    for (const auto &record : records) {
        std::string insertSql = "insert into example values('" + record + "');";
        SQLTest(insertSql.c_str());
    }
    /**
     * @tc.steps: step4. test cut for short words
     * @tc.expected: step4. OK.
     */
    std::vector<std::pair<std::string, int>> expectResult = {
        {"电子", 0}, {"邮件", 0}, {"电子邮件", 3}, {"少数", 0}, {"民族", 0}, {"少数民族", 1}
    };
    // 蓝区没有so导致失败，直接跳过测试
    if (!g_needSkip) {
        for (const auto &[word, expectMatchNum] : expectResult) {
            std::string querySql = "SELECT count(*) FROM example WHERE content MATCH '" + word + "';";
            EXPECT_EQ(sqlite3_exec(g_sqliteDb, querySql.c_str(), QueryCallback,
                reinterpret_cast<void*>(expectMatchNum), nullptr), SQLITE_OK);
        }
    }

    const char *SQLDROP = "DROP TABLE IF EXISTS example;";
    SQLTest(SQLDROP);
    EXPECT_EQ(sqlite3_close(g_sqliteDb), SQLITE_OK);
}

/**
 * @tc.name: SqliteAdapterTest008
 * @tc.desc: Test case Sensitive
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: whs
 */
HWTEST_F(SqliteAdapterTest, SqliteAdapterTest008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare db
     * @tc.expected: step1. OK.
     */
    // Save any error messages
    char *zErrMsg = nullptr;

    // Save the connection result
    int rc = sqlite3_open_v2(g_dbPath, &g_sqliteDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_db_config(g_sqliteDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    HandleRc(g_sqliteDb, rc);

    rc = sqlite3_load_extension(g_sqliteDb, "libcustomtokenizer.z.so", nullptr, nullptr);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step2. create table
     * @tc.expected: step2. OK.
     */
    string sql = "CREATE VIRTUAL TABLE example USING fts5(content, tokenize = 'customtokenizer cut_mode "
                 "short_words case_sensitive 0')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step3. insert records
     * @tc.expected: step3. OK.
     */
    std::vector<std::string> records = {
        "电子邮件",
        "这是一封电子邮件",
        "这是一封关于少数民族的电子邮件",
        "华中师范大学是一所位于武汉市的全日制综合性师范大学",
        "中华人民共和国",
        "武汉市长江大桥Wuhan Yangtze River Bridge是武汉市最长的桥"
    };
    for (const auto &record : records) {
        std::string insertSql = "insert into example values('" + record + "');";
        SQLTest(insertSql.c_str());
    }
    /**
     * @tc.steps: step4. test cut for short words
     * @tc.expected: step4. OK.
     */
    std::vector<std::pair<std::string, int>> expectResult = {
        {"电子", 3}, {"邮件", 3}, {"电子邮件", 3}, {"少数", 1}, {"民族", 1}, {"少数民族", 1}, {"华中", 1},
        {"中师", 1}, {"师范", 1}, {"共和", 1}, {"共和国", 1}, {"人民共和国", 0}, {"Yangtze", 1}, {"Wuhan", 1},
        {"市长", 0}, {"yangtze", 1}, {"WUHAN", 1}, {"river", 1},
    };
    // 平台没有so导致失败，直接跳过测试
    if (!g_needSkip) {
        for (const auto &[word, expectMatchNum] : expectResult) {
            std::string querySql = "SELECT count(*) FROM example WHERE content MATCH '" + word + "';";
            EXPECT_EQ(sqlite3_exec(g_sqliteDb, querySql.c_str(), QueryCallback,
                reinterpret_cast<void*>(expectMatchNum), nullptr), SQLITE_OK);
        }
    }

    const char *SQLDROP = "DROP TABLE IF EXISTS example;";
    SQLTest(SQLDROP);
    EXPECT_EQ(sqlite3_close(g_sqliteDb), SQLITE_OK);
}

/**
 * @tc.name: SqliteAdapterTest009
 * @tc.desc: Test case Sensitive highlight
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: whs
 */
HWTEST_F(SqliteAdapterTest, SqliteAdapterTest009, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare db
     * @tc.expected: step1. OK.
     */
    // Save any error messages
    char *zErrMsg = nullptr;
 
    // Save the connection result
    int rc = sqlite3_open_v2(g_dbPath, &g_sqliteDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    HandleRc(g_sqliteDb, rc);
 
    rc = sqlite3_db_config(g_sqliteDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    HandleRc(g_sqliteDb, rc);
 
    rc = sqlite3_load_extension(g_sqliteDb, "libcustomtokenizer.z.so", nullptr, nullptr);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step2. create table
     * @tc.expected: step2. OK.
     */
    string sql = "CREATE VIRTUAL TABLE example USING fts5(name, content, tokenize = 'customtokenizer cut_mode "
                 "short_words case_sensitive 0')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    HandleRc(g_sqliteDb, rc);
 
    const char *sqlInsert1 =
        "INSERT INTO example(name, content) VALUES('文档1', '这是一个测试文档，用于测试中文文本的分词和索引。');";
    SQLTest(sqlInsert1);
 
    const char *sqlInsert2 =
        "INSERT INTO example(name, content) VALUES('文档2', '这是一个检测报告，需要仔细查验和核查数据。');";
    SQLTest(sqlInsert2);
 
    const char *sqlInsert3 = "INSERT INTO example(name, content) VALUES('文档3', '日常工作报告，没有特别检测内容。');";
    SQLTest(sqlInsert3);
 
    const char *sqlQuery1 = "SELECT name, highlight(example, 1, '【', '】') as highlighted_content "
                            "FROM example WHERE example MATCH '测试';";
    SQLTest(sqlQuery1);
 
    EXPECT_EQ(sqlite3_close(g_sqliteDb), SQLITE_OK);
}
 
/**
 * @tc.name: SqliteAdapterTest009
 * @tc.desc: Test highlight
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: whs
 */
HWTEST_F(SqliteAdapterTest, SqliteAdapterTest010, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare db
     * @tc.expected: step1. OK.
     */
    // Save any error messages
    char *zErrMsg = nullptr;
 
    // Save the connection result
    int rc = sqlite3_open_v2(g_dbPath, &g_sqliteDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    HandleRc(g_sqliteDb, rc);
 
    rc = sqlite3_db_config(g_sqliteDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    HandleRc(g_sqliteDb, rc);
 
    rc = sqlite3_load_extension(g_sqliteDb, "libcustomtokenizer.z.so", nullptr, nullptr);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step2. create table
     * @tc.expected: step2. OK.
     */
    string sql = "CREATE VIRTUAL TABLE example USING fts5(name, content, tokenize = 'customtokenizer')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    HandleRc(g_sqliteDb, rc);
 
    const char *sqlInsert1 =
        "INSERT INTO example(name, content) VALUES('文档1', '这是一个测试文档，用于测试中文文本的分词和索引。');";
    SQLTest(sqlInsert1);
 
    const char *sqlInsert2 =
        "INSERT INTO example(name, content) VALUES('文档2', '这是一个检测报告，需要仔细查验和核查数据。');";
    SQLTest(sqlInsert2);
 
    const char *sqlInsert3 = "INSERT INTO example(name, content) VALUES('文档3', '日常工作报告，没有特别检测内容。');";
    SQLTest(sqlInsert3);
 
    const char *sqlQuery1 = "SELECT name, highlight(example, 1, '【', '】') as highlighted_content FROM"
                            " example WHERE example MATCH '测试';";
    SQLTest(sqlQuery1);
 
    EXPECT_EQ(sqlite3_close(g_sqliteDb), SQLITE_OK);
}

/**
 * @tc.name: SqliteAdapterTest011
 * @tc.desc: Test case Sensitive
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: whs
 */
HWTEST_F(SqliteAdapterTest, SqliteAdapterTest011, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare db
     * @tc.expected: step1. OK.
     */
    // Save any error messages
    char *zErrMsg = nullptr;
 
    // Save the connection result
    int rc = sqlite3_open_v2(g_dbPath, &g_sqliteDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    HandleRc(g_sqliteDb, rc);
 
    rc = sqlite3_db_config(g_sqliteDb, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    HandleRc(g_sqliteDb, rc);
 
    rc = sqlite3_load_extension(g_sqliteDb, "libcustomtokenizer.z.so", nullptr, nullptr);
    HandleRc(g_sqliteDb, rc);
    /**
     * @tc.steps: step2. create table
     * @tc.expected: step2. OK.
     */
    string sql = "CREATE VIRTUAL TABLE example USING fts5(name, content, "
                 "tokenize = 'customtokenizer cut_mode short_words case_sensitive 0')";
    rc = sqlite3_exec(g_sqliteDb, sql.c_str(), Callback, 0, &zErrMsg);
    HandleRc(g_sqliteDb, rc);
 
    const char *sqlInsert1 = "INSERT INTO example(name, content) VALUES('卡拉ok', "
                             "'\"C语言设计c++C语言设计X射线哆啦A梦qqq号250G硬盘usb接口k歌C++卡拉ok卡拉OK\"');";
    SQLTest(sqlInsert1);
 
    const char *sqlQuery1 = "SELECT name, highlight(example, 1, '【', '】') as highlighted_content FROM"
                            " example WHERE example MATCH '\"C++\"';";
    SQLTest(sqlQuery1);
 
    const char *sqlQuery2 = "SELECT name, highlight(example, 1, '【', '】') as highlighted_content FROM"
                            " example WHERE example MATCH '\"卡拉OK\"';";
    SQLTest(sqlQuery2);
 
    const char *sqlQuery3 = "SELECT name, highlight(example, 1, '【', '】') as highlighted_content FROM"
                            " example WHERE example MATCH '\"qq号\"';";
    SQLTest(sqlQuery3);
 
    EXPECT_EQ(sqlite3_close(g_sqliteDb), SQLITE_OK);
}
