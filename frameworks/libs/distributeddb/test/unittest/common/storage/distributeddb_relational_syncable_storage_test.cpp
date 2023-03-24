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

#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "distributeddb_tools_unit_test.h"
#include "relational_sync_able_storage.h"
#include "sqlite_single_relational_storage_engine.h"
#include "sqlite_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
constexpr const char* DB_SUFFIX = ".db";
constexpr const char* STORE_ID = "Relational_Store_ID";
std::string g_testDir;
std::string g_dbDir;

const std::string NORMAL_CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS sync_data(" \
    "key         BLOB NOT NULL UNIQUE," \
    "value       BLOB," \
    "timestamp   INT  NOT NULL," \
    "flag        INT  NOT NULL," \
    "device      BLOB," \
    "ori_device  BLOB," \
    "hash_key    BLOB PRIMARY KEY NOT NULL," \
    "w_timestamp INT," \
    "UNIQUE(device, ori_device));" \
    "CREATE INDEX key_index ON sync_data (key, flag);";
}

class DistributedDBRelationalSyncableStorageTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};


void DistributedDBRelationalSyncableStorageTest::SetUpTestCase(void)
{}

void DistributedDBRelationalSyncableStorageTest::TearDownTestCase(void)
{}

void DistributedDBRelationalSyncableStorageTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
}

void DistributedDBRelationalSyncableStorageTest::TearDown(void)
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

/**
 * @tc.name: SchemaRefTest001
 * @tc.desc: Test sync interface get schema
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBRelationalSyncableStorageTest, SchemaRefTest001, TestSize.Level1)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);

    RelationalDBProperties properties;
    auto sqliteStorageEngine = std::make_shared<SQLiteSingleRelationalStorageEngine>(properties);
    RelationalSyncAbleStorage *syncAbleStorage = new RelationalSyncAbleStorage(sqliteStorageEngine);

    static std::atomic<bool> isFinish(false);
    int getCount = 0;
    std::thread th([syncAbleStorage, &getCount]() {
        while (!isFinish.load()) {
            RelationalSchemaObject schema = syncAbleStorage->GetSchemaInfo();
            (void)schema.ToSchemaString();
            getCount++;
        }
    });

    RelationalSchemaObject schema;
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, NORMAL_CREATE_TABLE_SQL), SQLITE_OK);
    TableInfo table;
    SQLiteUtils::AnalysisSchema(db, "sync_data", table);
    schema.AddRelationalTable(table);

    int setCount = 0;
    for (; setCount < 1000; setCount++) { // 1000: run times
        RelationalSchemaObject tmpSchema = schema;
        sqliteStorageEngine->SetSchema(tmpSchema);
    }

    isFinish.store(true);
    th.join();

    LOGD("run round set: %d, get: %d", getCount, setCount);

    RefObject::DecObjRef(syncAbleStorage);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}
#endif // RELATIONAL_STORE
