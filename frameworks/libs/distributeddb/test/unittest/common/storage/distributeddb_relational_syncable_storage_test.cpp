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
#include "sqlite_relational_store_connection.h"
#include "sqlite_relational_utils.h"
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

#ifdef USE_DISTRIBUTEDDB_DEVICE
/**
 * @tc.name: FuncExceptionTest001
 * @tc.desc: Test the interception expection of the delegate interface when the store is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalSyncableStorageTest, FuncExceptionTest001, TestSize.Level1)
{
    auto conn = std::make_shared<SQLiteRelationalStoreConnection>(nullptr);
    EXPECT_EQ(conn->Close(), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->GetIdentifier().size(), 0u);
    EXPECT_EQ(conn->CreateDistributedTable("", {}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->RemoveDeviceData(), -E_INVALID_CONNECTION);

#ifdef USE_DISTRIBUTEDB_CLOUD
    EXPECT_EQ(conn->GetCloudSyncTaskCount(), -1);
    EXPECT_EQ(conn->DoClean({}, {}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->ClearCloudWatermark({}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->SetCloudDB({}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->PrepareAndSetCloudDbSchema({}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->SetIAssetLoader({}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->SetCloudSyncConfig({}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->Sync({}, {}, 0L), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->GetCloudTaskStatus(0u).errCode, DB_ERROR);
#endif
    EXPECT_EQ(conn->RemoveDeviceData({}), -E_INVALID_CONNECTION);
    std::vector<std::string> devices;
    Query query;
    RelationalStoreConnection::SyncInfo syncInfo{devices, {}, nullptr, query, true};
    EXPECT_EQ(conn->SyncToDevice(syncInfo), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->RegisterLifeCycleCallback({}), -E_INVALID_CONNECTION);
    std::shared_ptr<ResultSet> resultSet = nullptr;
    EXPECT_EQ(conn->RemoteQuery({}, {}, 0u, resultSet), -E_INVALID_CONNECTION);
    std::string userId;
    std::string appId;
    std::string storeId;
    EXPECT_EQ(conn->GetStoreInfo(userId, appId, storeId), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->SetTrackerTable({}), -E_INVALID_CONNECTION);
    std::vector<VBucket> records;
    EXPECT_EQ(conn->ExecuteSql({}, records), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->SetReference({}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->CleanTrackerData({}, 0L), -E_INVALID_CONNECTION);
    PragmaData pragmaData;
    PragmaCmd cmd = AUTO_SYNC;
    EXPECT_EQ(conn->Pragma(cmd, pragmaData), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->UpsertData({}, {}, {}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->SetDistributedDbSchema({}, false), -E_INVALID_CONNECTION);
    int32_t count;
    EXPECT_EQ(conn->GetDownloadingAssetsCount(count), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->SetTableMode({}), -E_INVALID_CONNECTION);
    EXPECT_EQ(conn->OperateDataStatus(0u), -E_INVALID_CONNECTION);
    conn = nullptr;
}
#endif

/**
 * @tc.name: FuncExceptionTest002
 * @tc.desc: Test the interception expection of the delegate interface when the store is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBRelationalSyncableStorageTest, FuncExceptionTest002, TestSize.Level1)
{
    sqlite3 *db = nullptr;
    int64_t timeOffset;
    EXPECT_EQ(SQLiteRelationalUtils::GetExistedDataTimeOffset(db, {}, false, timeOffset), -E_INVALID_DB);
    std::unique_ptr<SqliteLogTableManager> logMgrPtr = nullptr;
    SQLiteRelationalUtils::GenLogParam param;
    param.db = db;
    EXPECT_EQ(SQLiteRelationalUtils::GeneLogInfoForExistedData({}, {}, logMgrPtr, param), -E_INVALID_DB);
    EXPECT_EQ(SQLiteRelationalUtils::SetLogTriggerStatus(db, false), -E_INVALID_DB);
    EXPECT_EQ(SQLiteRelationalUtils::InitCursorToMeta(db, false, {}), -E_INVALID_DB);
    EXPECT_EQ(SQLiteRelationalUtils::PutKvData(db, false, {}, {}), -E_INVALID_DB);
    Value value;
    EXPECT_EQ(SQLiteRelationalUtils::GetKvData(db, false, {}, value), -E_INVALID_DB);
    EXPECT_EQ(SQLiteRelationalUtils::CreateRelationalMetaTable(db), -E_INVALID_DB);
    EXPECT_EQ(SQLiteRelationalUtils::GetCurrentVirtualTime(db).first, -E_INVALID_DB);
    int64_t tOffset;
    EXPECT_EQ(SQLiteRelationalUtils::GetMetaLocalTimeOffset(db, tOffset), -E_INVALID_DB);
    EXPECT_EQ(SQLiteRelationalUtils::OperateDataStatus(db, {}, 0), E_OK);
    uint64_t cursor;
    EXPECT_EQ(SQLiteRelationalUtils::GetCursor(db, {}, cursor), -E_INVALID_DB);
    int64_t count;
    EXPECT_EQ(SQLiteRelationalUtils::QueryCount(db, {}, count), -E_INVALID_DB);

    LogInfo logInfo;
    EXPECT_EQ(SQLiteRelationalUtils::GetLogInfoPre(nullptr, {}, {}, logInfo), -E_INVALID_ARGS);
    DistributedSchema schema;
    schema.tables.push_back({STORE_ID, {}});
    schema.tables.push_back({STORE_ID, {}});
    EXPECT_NE(SQLiteRelationalUtils::FilterRepeatDefine(schema).tables.size(), 0u);
    EXPECT_EQ(SQLiteRelationalUtils::CheckDistributedSchemaValid({}, {}, false, nullptr), -E_INVALID_ARGS);
    auto executor =
        std::make_shared<SQLiteSingleVerRelationalStorageExecutor>(db, true, DistributedTableMode::COLLABORATION);
    EXPECT_EQ(SQLiteRelationalUtils::CheckDistributedSchemaValid({}, {}, false, executor.get()), -E_NOT_FOUND);
    int32_t localCount = 0;
    EXPECT_EQ(executor->GetFlagIsLocalCount("tableName", localCount), -E_INVALID_DB);
}
#endif // RELATIONAL_STORE
