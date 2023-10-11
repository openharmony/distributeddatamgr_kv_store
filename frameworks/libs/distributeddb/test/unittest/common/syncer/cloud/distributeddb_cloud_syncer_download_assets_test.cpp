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
#include "cloud/cloud_storage_utils.h"
#include "cloud_db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "mock_asset_loader.h"
#include "process_system_api_adapter_impl.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "sqlite_relational_store.h"
#include "sqlite_relational_utils.h"
#include "time_helper.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
const string STORE_ID = "Relational_Store_SYNC";
const string DB_SUFFIX = ".db";
const string ASSETS_TABLE_NAME = "student";
const string DEVICE_CLOUD = "cloud_dev";
const string COL_ID = "id";
const string COL_NAME = "name";
const string COL_HEIGHT = "height";
const string COL_ASSETS = "assets";
const string COL_AGE = "age";
const int64_t SYNC_WAIT_TIME = 600;
const std::vector<Field> CLOUD_FIELDS = {{COL_ID, TYPE_INDEX<int64_t>, true}, {COL_NAME, TYPE_INDEX<std::string>},
    {COL_HEIGHT, TYPE_INDEX<double>}, {COL_ASSETS, TYPE_INDEX<Assets>}, {COL_AGE, TYPE_INDEX<int64_t>}};
const string CREATE_SINGLE_PRIMARY_KEY_TABLE = "CREATE TABLE IF NOT EXISTS " + ASSETS_TABLE_NAME + "(" + COL_ID +
                                               " INTEGER PRIMARY KEY," + COL_NAME + " TEXT ," + COL_HEIGHT + " REAL ," +
                                               COL_ASSETS + " BLOB," + COL_AGE + " INT);";
const string INSERT_SQL = "insert or replace into " + ASSETS_TABLE_NAME + " values (?,?,?,?,?);";
const Asset ASSET_COPY = {.version = 1,
    .name = "Phone",
    .assetId = "0",
    .subpath = "/local/sync",
    .uri = "/local/sync",
    .modifyTime = "123456",
    .createTime = "",
    .size = "256",
    .hash = "ASE"};

string g_storePath;
string g_testDir;
RelationalStoreObserverUnitTest *g_observer = nullptr;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
std::shared_ptr<VirtualCloudDb> g_virtualCloudDb;
std::shared_ptr<VirtualAssetLoader> g_virtualAssetLoader;
SyncProcess g_syncProcess;
std::condition_variable g_processCondition;
std::mutex g_processMutex;
using CloudSyncStatusCallback = std::function<void(const std::map<std::string, SyncProcess> &onProcess)>;

void InitDatabase(sqlite3 *&db)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_SINGLE_PRIMARY_KEY_TABLE), SQLITE_OK);
}

void GetCloudDbSchema(DataBaseSchema &dataBaseSchema)
{
    TableSchema assetsTableSchema = {.name = ASSETS_TABLE_NAME, .fields = CLOUD_FIELDS};
    dataBaseSchema.tables.push_back(assetsTableSchema);
}

void GenerateDataRecords(
    int64_t begin, int64_t count, int64_t gidStart, std::vector<VBucket> &record, std::vector<VBucket> &extend)
{
    for (int64_t i = begin; i < begin + count; i++) {
        Assets assets;
        Asset asset = ASSET_COPY;
        asset.name = ASSET_COPY.name + std::to_string(i);
        assets.emplace_back(asset);
        VBucket data;
        data.insert_or_assign(COL_ID, i);
        data.insert_or_assign(COL_NAME, "name" + std::to_string(i));
        data.insert_or_assign(COL_HEIGHT, 166.0 * i); // 166.0 is random double value
        data.insert_or_assign(COL_ASSETS, assets);
        data.insert_or_assign(COL_AGE, 18L + i); // 18 is random int value
        record.push_back(data);

        VBucket log;
        Timestamp now = TimeHelper::GetSysCurrentTime();
        log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND);
        log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND);
        log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
        log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(i + gidStart));
        extend.push_back(log);
    }
}

void InsertLocalData(sqlite3 *&db, int64_t begin, int64_t count)
{
    int errCode;
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    std::vector<uint8_t> assetBlob;
    GenerateDataRecords(begin, count, 0, record, extend);
    for (VBucket vBucket : record) {
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db, INSERT_SQL, stmt), E_OK);
        ASSERT_EQ(SQLiteUtils::BindInt64ToStatement(stmt, 1, std::get<int64_t>(vBucket[COL_ID])), E_OK); // 1 is id
        ASSERT_EQ(SQLiteUtils::BindTextToStatement(stmt, 2, std::get<string>(vBucket[COL_NAME])), E_OK); // 2 is name
        ASSERT_EQ(SQLiteUtils::MapSQLiteErrno(
            sqlite3_bind_double(stmt, 3, std::get<double>(vBucket[COL_HEIGHT]))), E_OK); // 3 is height
        RuntimeContext::GetInstance()->AssetsToBlob(std::get<Assets>(vBucket[COL_ASSETS]), assetBlob);
        ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 4, assetBlob, false), E_OK); // 4 is assets
        ASSERT_EQ(SQLiteUtils::BindInt64ToStatement(stmt, 5, std::get<int64_t>(vBucket[COL_AGE])), E_OK); // 5 is age
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
    LOGD("insert into student table [primary key]:[%" PRId64 " - %" PRId64 ")", begin, begin + count);
}

void DeleteCloudDBData(int64_t begin, int64_t count)
{
    for (int64_t i = begin; i < begin + count; i++) {
        VBucket idMap;
        idMap.insert_or_assign("#_gid", std::to_string(i));
        ASSERT_EQ(g_virtualCloudDb->DeleteByGid(ASSETS_TABLE_NAME, idMap), DBStatus::OK);
    }
}

void InsertCloudDBData(int64_t begin, int64_t count, int64_t gidStart)
{
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    GenerateDataRecords(begin, count, gidStart, record, extend);
    ASSERT_EQ(g_virtualCloudDb->BatchInsertWithGid(ASSETS_TABLE_NAME, std::move(record), extend), DBStatus::OK);
}

void WaitForSyncFinish(SyncProcess &syncProcess, const int64_t &waitTime)
{
    std::unique_lock<std::mutex> lock(g_processMutex);
    bool result = g_processCondition.wait_for(
        lock, std::chrono::seconds(waitTime), [&syncProcess]() { return syncProcess.process == FINISHED; });
    ASSERT_EQ(result, true);
    LOGD("-------------------sync end--------------");
}

void CallSync(const std::vector<std::string> &tableNames, SyncMode mode, DBStatus dbStatus)
{
    g_syncProcess = {};
    Query query = Query::Select().FromTable(tableNames);
    std::vector<SyncProcess> expectProcess;
    CloudSyncStatusCallback callback = [](const std::map<std::string, SyncProcess> &process) {
        ASSERT_EQ(process.begin()->first, DEVICE_CLOUD);
        g_syncProcess = std::move(process.begin()->second);
        if (g_syncProcess.process == FINISHED) {
            g_processCondition.notify_one();
        }
    };
    ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, mode, query, callback, SYNC_WAIT_TIME), dbStatus);

    if (dbStatus == DBStatus::OK) {
        WaitForSyncFinish(g_syncProcess, SYNC_WAIT_TIME);
    }
}

void CheckDownloadForTest001(int index, map<std::string, Assets> &assets)
{
    for (auto &item : assets) {
        for (auto &asset : item.second) {
            EXPECT_EQ(asset.status, static_cast<uint32_t>(AssetStatus::DOWNLOADING));
            if (index > 2) { // 1, 2 is deleted; 3, 4 is inserted
                EXPECT_EQ(asset.flag, static_cast<uint32_t>(AssetOpType::INSERT));
            } else {
                EXPECT_EQ(asset.flag, static_cast<uint32_t>(AssetOpType::DELETE));
            }
            LOGD("asset [name]:%s, [status]:%u, [flag]:%u, [index]:%d", asset.name.c_str(), asset.status, asset.flag,
                index);
        }
    }
}

void CloseDb()
{
    delete g_observer;
    g_virtualCloudDb = nullptr;
    if (g_delegate != nullptr) {
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
        g_delegate = nullptr;
    }
}

class DistributedDBCloudSyncerDownloadAssetsTest : public testing::Test {
  public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

  protected:
    sqlite3 *db = nullptr;
};

void DistributedDBCloudSyncerDownloadAssetsTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/" + STORE_ID + DB_SUFFIX;
    LOGI("The test db is:%s", g_storePath.c_str());
    RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
}

void DistributedDBCloudSyncerDownloadAssetsTest::TearDownTestCase(void) {}

void DistributedDBCloudSyncerDownloadAssetsTest::SetUp(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    LOGD("Test dir is %s", g_testDir.c_str());
    db = RelationalTestUtils::CreateDataBase(g_storePath);
    ASSERT_NE(db, nullptr);
    InitDatabase(db);
    g_observer = new (std::nothrow) RelationalStoreObserverUnitTest();
    ASSERT_NE(g_observer, nullptr);
    ASSERT_EQ(
        g_mgr.OpenStore(g_storePath, STORE_ID, RelationalStoreDelegate::Option{.observer = g_observer}, g_delegate),
        DBStatus::OK);
    ASSERT_NE(g_delegate, nullptr);
    ASSERT_EQ(g_delegate->CreateDistributedTable(ASSETS_TABLE_NAME, CLOUD_COOPERATION), DBStatus::OK);
    g_virtualCloudDb = make_shared<VirtualCloudDb>();
    g_virtualAssetLoader = make_shared<VirtualAssetLoader>();
    g_syncProcess = {};
    ASSERT_EQ(g_delegate->SetCloudDB(g_virtualCloudDb), DBStatus::OK);
    ASSERT_EQ(g_delegate->SetIAssetLoader(g_virtualAssetLoader), DBStatus::OK);
    DataBaseSchema dataBaseSchema;
    GetCloudDbSchema(dataBaseSchema);
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
}

void DistributedDBCloudSyncerDownloadAssetsTest::TearDown(void)
{
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

/**
 * @tc.name: DownloadAssetForDupDataTest001
 * @tc.desc: Test the download interface call with duplicate data for the same primary key.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liufuchenxing
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetForDupDataTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Mock asset download interface.
     * @tc.expected: step1. return OK and interface will be called 4 times. delete 1, delete 2, insert 1, insert 2
     */
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 1;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .Times(4)
        .WillRepeatedly(
            [&index](const std::string &, const std::string &gid, const Type &, std::map<std::string, Assets> &assets) {
                LOGD("Download GID:%s", gid.c_str());
                CheckDownloadForTest001(index, assets);
                index++;
                return DBStatus::OK;
            });

    /**
     * @tc.steps:step2. Insert local data [0, 10), sync data
     * @tc.expected: step2. sync success.
     */
    InsertLocalData(db, 0, 10);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step3. delete cloud data [1, 2], then insert cloud data [1,2] with new gid. Finally sync data.
     * @tc.expected: step3. sync success.
     */
    DeleteCloudDBData(1, 2);
    InsertCloudDBData(1, 2, 10);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step4. close db.
     * @tc.expected: step4. close success.
     */
    CloseDb();
}
} // namespace
#endif // RELATIONAL_STORE
