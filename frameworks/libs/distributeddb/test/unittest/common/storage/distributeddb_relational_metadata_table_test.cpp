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

#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "distributeddb_tools_unit_test.h"
#include "distributeddb_data_generate_unit_test.h"
#include "log_print.h"
#include "sqlite3.h"
#include "db_constant.h"
#include "sqlite_utils.h"
#include "relational_store_manager.h"
#include "relational_store_delegate.h"
#include "relational_store_instance.h"
#include "db_common.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
std::string g_testDir;
std::string g_storePath;
std::string g_storeID = "store_id_0123";
RelationalStoreManager *g_mgr = nullptr;
RelationalStoreDelegate *g_delegate = nullptr;
std::string g_tableName = "data";

bool IsMetaTableExists(sqlite3 *db)
{
    std::string metaTableName = std::string(DBConstant::RELATIONAL_PREFIX) + "metadata";
    std::string sql = "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='" + metaTableName + "';";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK || statement == nullptr) {
        return false;
    }
    int stepResult = sqlite3_step(statement);
    if (stepResult != SQLITE_ROW) {
        (void)SQLiteUtils::ResetStatement(statement, true, errCode);
        return false;
    }
    int count = sqlite3_column_int(statement, 0);
    (void)SQLiteUtils::ResetStatement(statement, true, errCode);
    return count > 0;
}

class DistributedDBRelationalMetadataTableTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

void DistributedDBRelationalMetadataTableTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/metadataTest.db";
    g_mgr = new (std::nothrow) RelationalStoreManager(APP_ID, USER_ID);
    LOGI("The test db dir is:%s", g_testDir.c_str());
}

void DistributedDBRelationalMetadataTableTest::TearDownTestCase(void)
{
    delete g_mgr;
    g_mgr = nullptr;
}

void DistributedDBRelationalMetadataTableTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBRelationalMetadataTableTest::TearDown()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

/**
 * @tc.name: MetadataTableTest001
 * @tc.desc: Test open store with SKIP_METADATA_TABLE set to true, metadata table should not be created
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuziran
 */
HWTEST_F(DistributedDBRelationalMetadataTableTest, MetadataTableTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a fresh db file with WAL mode
     * @tc.expected: step1. Succeed
     */
    std::string testDbPath = g_testDir + "/metadata_false_test.db";
    int rmRet = DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    ASSERT_EQ(rmRet, 0) << "Failed to remove test db files, error code: " << rmRet;

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(testDbPath.c_str(), &db), SQLITE_OK);
    std::string sql = "PRAGMA journal_mode=WAL;"
                 "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    char *errMsg = nullptr;
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg), SQLITE_OK);
    sqlite3_close(db);

    /**
     * @tc.steps: step2. Open store with skipMetadataTable = true via Option
     * @tc.expected: step2. Store opens successfully
     */
    RelationalStoreDelegate::Option option;
    option.skipMetadataTable = true;
    DBStatus status = g_mgr->OpenStore(testDbPath, g_storeID, option, g_delegate);
    ASSERT_EQ(status, DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);

    /**
     * @tc.steps: step3. Check if metadata table exists in the database
     * @tc.expected: step3. Metadata table does not exist (skipMetadataTable=true)
     */
    ASSERT_EQ(sqlite3_open(testDbPath.c_str(), &db), SQLITE_OK);
    EXPECT_FALSE(IsMetaTableExists(db));
    sqlite3_close(db);

    /**
     * @tc.steps: step4. Close store
     * @tc.expected: step4. Store closes successfully
     */
    EXPECT_EQ(g_mgr->CloseStore(g_delegate), DBStatus::OK);
    g_delegate = nullptr;
}

/**
 * @tc.name: MetadataTableTest002
 * @tc.desc: Test open store with SKIP_METADATA_TABLE set to false (default), metadata table should be created
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuziran
 */
HWTEST_F(DistributedDBRelationalMetadataTableTest, MetadataTableTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a fresh db file
     * @tc.expected: step1. Succeed
     */
    std::string testDbPath = g_testDir + "/metadata_false_test.db";
    int rmRet = DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    ASSERT_EQ(rmRet, 0) << "Failed to remove test db files, error code: " << rmRet;

    // Create an empty database file with WAL mode
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open(testDbPath.c_str(), &db), SQLITE_OK);
    std::string sql = "PRAGMA journal_mode=WAL;";
    char *errMsg = nullptr;
    ASSERT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg), SQLITE_OK);
    sqlite3_close(db);

    /**
     * @tc.steps: step2. Open store with skipMetadataTable = false (default)
     * @tc.expected: step2. Store opens successfully
     */
    RelationalStoreDelegate::Option option;
    // skipMetadataTable defaults to false, so no need to set explicitly
    DBStatus status = g_mgr->OpenStore(testDbPath, g_storeID, option, g_delegate);
    ASSERT_EQ(status, DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);

    /**
     * @tc.steps: step3. Check if metadata table exists in the database
     * @tc.expected: step3. Metadata table exists (skipMetadataTable=false)
     */
    ASSERT_EQ(sqlite3_open(testDbPath.c_str(), &db), SQLITE_OK);
    EXPECT_FALSE(IsMetaTableExists(db));
    sqlite3_close(db);

    /**
     * @tc.steps: step4. Close store
     * @tc.expected: step4. Store closes successfully
     */
    EXPECT_EQ(g_mgr->CloseStore(g_delegate), DBStatus::OK);
    g_delegate = nullptr;
}
}
#endif