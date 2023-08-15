/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "cloud_db_types.h"
#include "distributeddb_data_generate_unit_test.h"
#include "log_print.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"
namespace {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
const char *g_createSQL =
    "CREATE TABLE IF NOT EXISTS DistributedDBCloudCheckSyncTest(" \
    "id TEXT PRIMARY KEY," \
    "name TEXT," \
    "height REAL ," \
    "photo BLOB," \
    "age INT);";
const int64_t g_syncWaitTime = 60;

void CreateUserDBAndTable(sqlite3 *&db)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, g_createSQL), SQLITE_OK);
}

void BlockSync(const Query &query, RelationalStoreDelegate *delegate)
{
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    auto callback = [&cv, &dataMutex, &finish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    finish = true;
                }
                cv.notify_one();
            }
        }
    };
    ASSERT_EQ(delegate->Sync({ "CLOUD" }, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), OK);
    std::unique_lock<std::mutex> uniqueLock(dataMutex);
    cv.wait(uniqueLock, [&finish]() {
        return finish;
    });
}

class DistributedDBCloudCheckSyncTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
protected:
    void InitTestDir();
    DataBaseSchema GetSchema();
    void CloseDb();
    void InsertUserTableRecord(int64_t recordCounts, int64_t begin = 0);
    std::string testDir_;
    std::string storePath_;
    sqlite3 *db_ = nullptr;
    RelationalStoreDelegate *delegate_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader_ = nullptr;
    std::shared_ptr<RelationalStoreManager> mgr_ = nullptr;
    std::string tableName_ = "DistributedDBCloudCheckSyncTest";
};

void DistributedDBCloudCheckSyncTest::SetUpTestCase()
{
    RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
}

void DistributedDBCloudCheckSyncTest::TearDownTestCase()
{}

void DistributedDBCloudCheckSyncTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    InitTestDir();
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(testDir_) != 0) {
        LOGE("rm test db files error.");
    }
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    LOGD("Test dir is %s", testDir_.c_str());
    db_ = RelationalTestUtils::CreateDataBase(storePath_);
    ASSERT_NE(db_, nullptr);
    CreateUserDBAndTable(db_);
    mgr_ = std::make_shared<RelationalStoreManager>(APP_ID, USER_ID);
    RelationalStoreDelegate::Option option;
    ASSERT_EQ(mgr_->OpenStore(storePath_, STORE_ID_1, option, delegate_), DBStatus::OK);
    ASSERT_NE(delegate_, nullptr);
    ASSERT_EQ(delegate_->CreateDistributedTable(tableName_, CLOUD_COOPERATION), DBStatus::OK);
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    virtualAssetLoader_ = std::make_shared<VirtualAssetLoader>();
    ASSERT_EQ(delegate_->SetCloudDB(virtualCloudDb_), DBStatus::OK);
    ASSERT_EQ(delegate_->SetIAssetLoader(virtualAssetLoader_), DBStatus::OK);
    DataBaseSchema dataBaseSchema = GetSchema();
    ASSERT_EQ(delegate_->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
}

void DistributedDBCloudCheckSyncTest::TearDown()
{
    CloseDb();
    EXPECT_EQ(sqlite3_close_v2(db_), SQLITE_OK);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(testDir_) != 0) {
        LOGE("rm test db files error.");
    }
}

void DistributedDBCloudCheckSyncTest::InitTestDir()
{
    if (!testDir_.empty()) {
        return;
    }
    DistributedDBToolsUnitTest::TestDirInit(testDir_);
    storePath_ = testDir_ + "/" + STORE_ID_1 + ".db";
    LOGI("The test db is:%s", testDir_.c_str());
}

DataBaseSchema DistributedDBCloudCheckSyncTest::GetSchema()
{
    DataBaseSchema schema;
    TableSchema tableSchema;
    tableSchema.name = tableName_;
    tableSchema.fields = {
        {"id", TYPE_INDEX<std::string>, true}, {"name", TYPE_INDEX<std::string>}, {"height", TYPE_INDEX<double>},
        {"photo", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>}
    };
    schema.tables.push_back(tableSchema);
    return schema;
}

void DistributedDBCloudCheckSyncTest::CloseDb()
{
    virtualCloudDb_ = nullptr;
    EXPECT_EQ(mgr_->CloseStore(delegate_), DBStatus::OK);
    delegate_ = nullptr;
    mgr_ = nullptr;
}

void DistributedDBCloudCheckSyncTest::InsertUserTableRecord(int64_t recordCounts, int64_t begin)
{
    ASSERT_NE(db_, nullptr);
    for (int64_t i = begin; i < recordCounts; ++i) {
        string sql = "INSERT OR REPLACE INTO " + tableName_
            + " (id, name, height, photo, age) VALUES ('" + std::to_string(i) + "', 'Local"
            + std::to_string(i) + "', '155.10',  'text', '21');";
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    }
}

/**
 * @tc.name: CloudSyncTest001
 * @tc.desc: sync with device sync query
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudSyncTest001, TestSize.Level0)
{
    // prepare data
    const int actualCount = 10;
    InsertUserTableRecord(actualCount);
    // sync twice
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);
    BlockSync(query, delegate_);
    // remove cloud data
    delegate_->RemoveDeviceData("CLOUD", ClearMode::FLAG_AND_DATA);
    // check local data
    int dataCnt = -1;
    std::string checkLogSql = "SELECT count(*) FROM " + tableName_;
    RelationalTestUtils::ExecSql(db_, checkLogSql, nullptr, [&dataCnt](sqlite3_stmt *stmt) {
        dataCnt = sqlite3_column_int(stmt, 0);
        return E_OK;
    });
    EXPECT_EQ(dataCnt, actualCount);
}
}
#endif