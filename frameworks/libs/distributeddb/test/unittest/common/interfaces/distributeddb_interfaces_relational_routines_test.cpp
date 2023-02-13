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

#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "native_sqlite.h"
#include "relational_store_manager.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char* DB_SUFFIX = ".db";
    constexpr const char* STORE_ID = "Relational_Store_ID";
    const std::string DEVICE_A = "DEVICE_A";
    std::string g_testDir;
    std::string g_dbDir;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
}

class DistributedDBInterfacesRelationalRoutinesTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
protected:
};

void DistributedDBInterfacesRelationalRoutinesTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBInterfacesRelationalRoutinesTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void DistributedDBInterfacesRelationalRoutinesTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBInterfacesRelationalRoutinesTest::TearDown()
{
}

/**
 * @tc.name: DBMaxTimeStampTest001
 * @tc.desc: check db max timestamp when open
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBInterfacesRelationalRoutinesTest, DBMaxTimeStampTest001, TestSize.Level1)
{
    sqlite3 *ndb = NativeSqlite::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(ndb, nullptr);
    EXPECT_EQ(NativeSqlite::ExecSql(ndb, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(NativeSqlite::ExecSql(ndb, "create table if not exists t2(a int, b int);"), SQLITE_OK);
    std::string t2Log = R""(CREATE TABLE naturalbase_rdb_aux_t2_log(
        data_key    INT NOT NULL,
        device      BLOB,
        ori_device  BLOB,
        timestamp   INT  NOT NULL,
        wtimestamp  INT  NOT NULL,
        flag        INT  NOT NULL,
        hash_key    BLOB NOT NULL,
        PRIMARY KEY(device, hash_key)
    );)"";
    EXPECT_EQ(NativeSqlite::ExecSql(ndb, t2Log), SQLITE_OK);

    int64_t fakeTimestamp = 26640033917191310; // 26640033917191310: Large enough fake timestamp
    std::string insertLog = "insert into naturalbase_rdb_aux_t2_log " \
        "values(1, '" + DEVICE_A + "', '" + DEVICE_A + "', " + std::to_string(fakeTimestamp) +
        ", " + std::to_string(fakeTimestamp) + ", 0, 'hash_key')";
    EXPECT_EQ(NativeSqlite::ExecSql(ndb, insertLog), SQLITE_OK);
    EXPECT_EQ(sqlite3_close_v2(ndb), SQLITE_OK);

    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "create table if not exists t1(a int, b int);"), SQLITE_OK);

    RelationalStoreDelegate *delegate = nullptr;
    DBStatus status = g_mgr.OpenStore(g_dbDir + STORE_ID + DB_SUFFIX, STORE_ID, {}, delegate);
    EXPECT_EQ(status, OK);
    ASSERT_NE(delegate, nullptr);
    delegate->CreateDistributedTable("t1");
    g_mgr.CloseStore(delegate);

    RelationalTestUtils::ExecSql(db, "insert into t1 values(1, 1);");

    int64_t logTime;
    std::string checkSql = "select timestamp from naturalbase_rdb_aux_t1_log where data_key = 1";
    RelationalTestUtils::ExecSql(db, checkSql, nullptr, [&logTime](sqlite3_stmt *stmt) {
        logTime = sqlite3_column_int64(stmt, 0);
        return E_OK;
    });

    EXPECT_GT(logTime, fakeTimestamp);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}
