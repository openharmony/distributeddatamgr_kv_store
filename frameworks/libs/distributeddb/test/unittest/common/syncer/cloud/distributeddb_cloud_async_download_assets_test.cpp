/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cloud/assets_download_manager.h"
#include "cloud/cloud_storage_utils.h"
#include "cloud/mock_icloud_sync_storage_interface.h"
#include "cloud/virtual_asset_loader.h"
#include "cloud/virtual_cloud_data_translate.h"
#include "cloud/virtual_cloud_syncer.h"
#include "cloud_db_sync_utils_test.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "res_finalizer.h"
#include "rdb_data_generator.h"
#include "relational_store_client.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
const std::string QUERY_INCONSISTENT_SQL = 
    "select count(*) from naturalbase_rdb_aux_AsyncDownloadAssetsTest_log where flag&0x20!=0;";
IRelationalStore *g_store = nullptr;
ICloudSyncStorageHook *g_cloudStoreHook = nullptr;
RelationalStoreManager g_mgr(APP_ID, USER_ID);
typedef struct SkipAssetTestParam {
    DBStatus downloadRes;
    bool useBatch;
    bool useAsync;
    int startIndex;
    int expectInconsistentCount;
    DBStatus expectSyncRes;
} SkipAssetTestParamT;
class DistributedDBCloudAsyncDownloadAssetsTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
protected:
    static DataBaseSchema GetSchema(bool multiTables = false);
    static TableSchema GetTableSchema(const std::string &tableName, bool withoutAsset = false);
    static CloudSyncOption GetAsyncCloudSyncOption();
    static int GetAssetFieldCount();
    void InitStore();
    void CloseDb();
    void DoSkipAssetDownload(SkipAssetTestParamT param);
    void UpdateLocalData(sqlite3 *&db, const std::string &tableName, int32_t begin, int32_t end);
    void DeleteLocalData(sqlite3 *&db, const std::string &tableName);
    void CheckLogTable(sqlite3 *&db, const std::string &tableName, int count);
    void UpdateLocalAndCheckUploadCount(const bool &isAsync, const int &dataCount, const int &expectCount);
    std::string storePath_;
    sqlite3 *db_ = nullptr;
    RelationalStoreDelegate *delegate_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader_ = nullptr;
    VirtualCommunicatorAggregator *communicatorAggregator_ = nullptr;
};

void DistributedDBCloudAsyncDownloadAssetsTest::SetUpTestCase()
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBCloudAsyncDownloadAssetsTest::TearDownTestCase()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBCloudAsyncDownloadAssetsTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(true);
    InitStore();
    communicatorAggregator_ = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator_ != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator_);
}

void DistributedDBCloudAsyncDownloadAssetsTest::TearDown()
{
    RefObject::DecObjRef(g_store);
    CloseDb();
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    virtualCloudDb_ = nullptr;
    virtualAssetLoader_ = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    communicatorAggregator_ = nullptr;
}

DataBaseSchema DistributedDBCloudAsyncDownloadAssetsTest::GetSchema(bool multiTables)
{
    DataBaseSchema schema;
    schema.tables.push_back(GetTableSchema("AsyncDownloadAssetsTest"));
    if (multiTables) {
        schema.tables.push_back(GetTableSchema("TABLE1"));
        schema.tables.push_back(GetTableSchema("TABLE2"));
    }
    return schema;
}

TableSchema DistributedDBCloudAsyncDownloadAssetsTest::GetTableSchema(const std::string &tableName, bool withoutAsset)
{
    TableSchema tableSchema;
    tableSchema.name = tableName;
    Field field;
    field.primary = true;
    field.type = TYPE_INDEX<int64_t>;
    field.colName = "pk";
    tableSchema.fields.push_back(field);
    field.primary = false;
    field.colName = "int_field";
    tableSchema.fields.push_back(field);
    if (withoutAsset) {
        return tableSchema;
    }
    field.type = TYPE_INDEX<Assets>;
    field.colName = "assets_1";
    tableSchema.fields.push_back(field);
    field.colName = "asset_1";
    field.type = TYPE_INDEX<Asset>;
    tableSchema.fields.push_back(field);
    return tableSchema;
}

CloudSyncOption DistributedDBCloudAsyncDownloadAssetsTest::GetAsyncCloudSyncOption()
{
    CloudSyncOption option;
    std::vector<std::string> tables;
    auto schema = GetSchema();
    for (const auto &table : schema.tables) {
        tables.push_back(table.name);
        LOGW("[DistributedDBCloudAsyncDownloadAssetsTest] Sync with table %s", table.name.c_str());
    }
    option.devices = {"cloud"};
    option.query = Query::Select().FromTable(tables);
    option.mode = SYNC_MODE_CLOUD_MERGE;
    option.asyncDownloadAssets = true;
    return option;
}

int DistributedDBCloudAsyncDownloadAssetsTest::GetAssetFieldCount()
{
    int count = 0;
    auto schema = GetSchema();
    for (const auto &table : schema.tables) {
        for (const auto &field : table.fields) {
            if (field.type == TYPE_INDEX<Assets> || field.type == TYPE_INDEX<Asset>) {
                count++;
            }
        }
    }
    return count;
}

const RelationalSyncAbleStorage *GetRelationalStore()
{
    RelationalDBProperties properties;
    CloudDBSyncUtilsTest::InitStoreProp(g_testDir + "/" + STORE_ID_1 + ".db", APP_ID, USER_ID, STORE_ID_1, properties);
    int errCode = E_OK;
    g_store = RelationalStoreInstance::GetDataBase(properties, errCode);
    if (g_store == nullptr) {
        LOGE("Get db failed:%d", errCode);
        return nullptr;
    }
    return static_cast<SQLiteRelationalStore *>(g_store)->GetStorageEngine();
}

void DistributedDBCloudAsyncDownloadAssetsTest::InitStore()
{
    if (storePath_.empty()) {
        storePath_ = g_testDir + "/" + STORE_ID_1 + ".db";
    }
    db_ = RelationalTestUtils::CreateDataBase(storePath_);
    ASSERT_NE(db_, nullptr);
    auto schema = GetSchema(true);
    EXPECT_EQ(RDBDataGenerator::InitDatabase(schema, *db_), SQLITE_OK);
    ASSERT_EQ(g_mgr.OpenStore(storePath_, STORE_ID_1, {}, delegate_), OK);
    ASSERT_NE(delegate_, nullptr);
    for (const auto &table : schema.tables) {
        EXPECT_EQ(delegate_->CreateDistributedTable(table.name, TableSyncType::CLOUD_COOPERATION), OK);
        LOGI("[DistributedDBCloudAsyncDownloadAssetsTest] CreateDistributedTable %s", table.name.c_str());
    }
    g_cloudStoreHook = (ICloudSyncStorageHook *) GetRelationalStore();
    ASSERT_NE(g_cloudStoreHook, nullptr);
    virtualCloudDb_ = make_shared<VirtualCloudDb>();
    ASSERT_NE(virtualCloudDb_, nullptr);
    ASSERT_EQ(delegate_->SetCloudDB(virtualCloudDb_), DBStatus::OK);
    virtualAssetLoader_ = make_shared<VirtualAssetLoader>();
    ASSERT_NE(virtualAssetLoader_, nullptr);
    ASSERT_EQ(delegate_->SetIAssetLoader(virtualAssetLoader_), DBStatus::OK);
    RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());

    ASSERT_EQ(delegate_->SetCloudDbSchema(schema), DBStatus::OK);
}

void CheckInconsistentCount(sqlite3 *db, int64_t expectCount)
{
    EXPECT_EQ(sqlite3_exec(db, QUERY_INCONSISTENT_SQL.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(expectCount), nullptr), SQLITE_OK);
}

void DistributedDBCloudAsyncDownloadAssetsTest::CloseDb()
{
    if (db_ != nullptr) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
    if (delegate_ != nullptr) {
        EXPECT_EQ(g_mgr.CloseStore(delegate_), OK);
        delegate_ = nullptr;
    }
}

/**
 * @tc.name: AsyncDownloadAssetConfig001
 * @tc.desc: Test config with valid and invalid param.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncDownloadAssetConfig001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set valid param
     * @tc.expected: step1.ok
     */
    AsyncDownloadAssetsConfig config;
    AssetsDownloadManager manager;
    EXPECT_EQ(manager.SetAsyncDownloadAssetsConfig(config), E_OK);
    config.maxDownloadTask = CloudDbConstant::MAX_ASYNC_DOWNLOAD_TASK;
    EXPECT_EQ(manager.SetAsyncDownloadAssetsConfig(config), E_OK);
    config.maxDownloadAssetsCount = CloudDbConstant::MAX_ASYNC_DOWNLOAD_ASSETS;
    EXPECT_EQ(manager.SetAsyncDownloadAssetsConfig(config), E_OK);

    /**
     * @tc.steps: step2. Set invalid param
     * @tc.expected: step2.invalid args
     */
    config.maxDownloadTask += 1u;
    EXPECT_EQ(manager.SetAsyncDownloadAssetsConfig(config), -E_INVALID_ARGS);
    config.maxDownloadTask = CloudDbConstant::MAX_ASYNC_DOWNLOAD_TASK;
    config.maxDownloadAssetsCount += 1u;
    EXPECT_EQ(manager.SetAsyncDownloadAssetsConfig(config), -E_INVALID_ARGS);
}

/**
 * @tc.name: AsyncDownloadAssetConfig002
 * @tc.desc: Test config work correctly.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncDownloadAssetConfig002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set valid param twice
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 10;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    config.maxDownloadTask = 1;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 20;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Begin download first, block async task
     * @tc.expected: step3. ok
     */
    auto manager = RuntimeContext::GetInstance()->GetAssetsDownloadManager();
    int finishCount = 0;
    std::mutex finishMutex;
    std::condition_variable cv;
    auto finishAction = [&finishCount, &finishMutex, &cv](void *) {
        std::lock_guard<std::mutex> autoLock(finishMutex);
        finishCount++;
        cv.notify_all();
    };
    auto [errCode, listener] = manager->BeginDownloadWithListener(finishAction);
    ASSERT_EQ(errCode, E_OK);
    ASSERT_EQ(listener, nullptr);
    ASSERT_EQ(manager->GetCurrentDownloadCount(), 1u);
    std::tie(errCode, listener) = manager->BeginDownloadWithListener(finishAction);
    ASSERT_EQ(errCode, -E_MAX_LIMITS);
    ASSERT_NE(listener, nullptr);
    /**
     * @tc.steps: step4. Async cloud data
     * @tc.expected: step4. ok and async task still one
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    EXPECT_EQ(manager->GetCurrentDownloadCount(), 1u);
    /**
     * @tc.steps: step5. Notify async task finish
     * @tc.expected: step5. wait util another async task finish
     */
    manager->FinishDownload();
    std::unique_lock uniqueLock(finishMutex);
    auto res = cv.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT), [&finishCount]() {
        return finishCount >= 2; // 2 async task
    });
    EXPECT_TRUE(res);
    listener->Drop(true);
}

/**
 * @tc.name: AsyncDownloadAssetConfig003
 * @tc.desc: Test asyncDownloadAssets and compensatedSyncOnly both true.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncDownloadAssetConfig003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Insert cloud data
     * @tc.expected: step1. ok
     */
    const int cloudCount = 10;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step2. set compensatedSyncOnly true and sync return NOT_SUPPORT.
     * @tc.expected: step2. NOT_SUPPORT
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    option.compensatedSyncOnly = true;
    DBStatus result = delegate_->Sync(option, nullptr);
    EXPECT_EQ(result, NOT_SUPPORT);
}

/**
 * @tc.name: FinishListener001
 * @tc.desc: Test listen download finish event.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, FinishListener001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Begin download first time
     * @tc.expected: step1.ok
     */
    auto manager = RuntimeContext::GetInstance()->GetAssetsDownloadManager();
    std::atomic<bool> finished = false;
    auto finishAction = [&finished](void *) {
        EXPECT_TRUE(finished);
    };
    auto [errCode, listener] = manager->BeginDownloadWithListener(finishAction);
    ASSERT_EQ(errCode, E_OK);
    ASSERT_EQ(listener, nullptr);
    /**
     * @tc.steps: step2. Begin download twice
     * @tc.expected: step2. -E_MAX_LIMITS because default one task
     */
    std::tie(errCode, listener) = manager->BeginDownloadWithListener(finishAction);
    EXPECT_EQ(errCode, -E_MAX_LIMITS);
    EXPECT_NE(listener, nullptr);
    /**
     * @tc.steps: step3. Finish download
     * @tc.expected: step3. finished is true in listener
     */
    finished = true;
    manager->FinishDownload();
    listener->Drop(true);
}

/**
 * @tc.name: AsyncComplexDownload001
 * @tc.desc: Test complex async download.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncComplexDownload001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    config.maxDownloadAssetsCount = 1;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 10;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Async cloud data
     * @tc.expected: step3. ok
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    /**
     * @tc.steps: step3. Block download cloud data
     * @tc.expected: step3. ok
     */
    option.asyncDownloadAssets = false;
    RelationalTestUtils::CloudBlockSync(option, delegate_);
}

/**
 * @tc.name: AsyncComplexDownload002
 * @tc.desc: Test complex async download.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncComplexDownload002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    config.maxDownloadAssetsCount = 1;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 10;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Complex cloud data
     * @tc.expected: step3. ok
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    for (int i = 0; i < 10; ++i) { // loop 10 times
        option.asyncDownloadAssets = false;
        RelationalTestUtils::CloudBlockSync(option, delegate_);
        option.asyncDownloadAssets = true;
        RelationalTestUtils::CloudBlockSync(option, delegate_);
    }
}

/**
 * @tc.name: AsyncAbnormalDownload001
 * @tc.desc: Test abnormal async download.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload001, TestSize.Level4)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    config.maxDownloadAssetsCount = 1;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 10;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Fork download abnormal
     */
    virtualAssetLoader_->SetDownloadStatus(DB_ERROR);
    /**
     * @tc.steps: step4. Async cloud data
     * @tc.expected: step4. ok
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, cloudCount * GetAssetFieldCount());
    EXPECT_FALSE(RelationalTestUtils::IsExistEmptyHashAsset(db_, GetTableSchema("AsyncDownloadAssetsTest")));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step5. Async cloud data with download ok
     * @tc.expected: step5. ok
     */
    virtualAssetLoader_->SetDownloadStatus(OK);
    LOGW("set download ok");
    int count = 0;
    std::mutex countMutex;
    std::condition_variable cv;
    virtualAssetLoader_->ForkDownload([&count, &countMutex, &cv](const std::string &tableName,
        std::map<std::string, Assets> &) {
        std::lock_guard<std::mutex> autoLock(countMutex);
        count++;
        if (count == 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        cv.notify_all();
    });
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    std::unique_lock<std::mutex> uniqueLock(countMutex);
    cv.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT), [&count]() {
        return count >= cloudCount;
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::tie(status, downloadCount) = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, 0);
    virtualAssetLoader_->ForkDownload(nullptr);
}

/**
 * @tc.name: AsyncAbnormalDownload002
 * @tc.desc: Test abnormal sync download.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Insert cloud data
     * @tc.expected: step1. ok
     */
    const int cloudCount = 10;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step2. Fork download abnormal
     */
    virtualAssetLoader_->SetDownloadStatus(DB_ERROR);
    /**
     * @tc.steps: step3. Sync cloud data
     * @tc.expected: step3. DB_ERROR and not exist downloading assets count
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    option.asyncDownloadAssets = false;
    RelationalTestUtils::CloudBlockSync(option, delegate_, OK, CLOUD_ERROR);
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, 0);
    virtualAssetLoader_->SetDownloadStatus(OK);
}

/**
 * @tc.name: AsyncAbnormalDownload003
 * @tc.desc: Test abnormal async download.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload003, TestSize.Level4)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    config.maxDownloadAssetsCount = 4; // 1 record has 2 asset, config 1 batch has 2 record
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 5;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Fork download abnormal
     */
    virtualAssetLoader_->SetDownloadStatus(DB_ERROR);
    /**
     * @tc.steps: step4. Sync cloud data
     * @tc.expected: step4. DB_ERROR and exist downloading assets count
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate_));
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, cloudCount * 2); // 1 record has 2 asset
    CheckInconsistentCount(db_, 5);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step5. Sync cloud data again and block upload
     * @tc.expected: step5. DB_ERROR and exist downloading assets count
     */
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(cloudCount + 1, 1, db_, GetSchema()), E_OK);
    virtualAssetLoader_->SetDownloadStatus(OK);
    virtualCloudDb_->ForkUpload([delegate = delegate_](const std::string &, VBucket &) {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep 2s
        auto [ret, count] = delegate->GetDownloadingAssetsCount();
        EXPECT_EQ(ret, OK);
        EXPECT_EQ(count, 0);
    });
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate_));
    virtualAssetLoader_->ForkBatchDownload(nullptr);
}

/**
 * @tc.name: AsyncAbnormalDownload004
 * @tc.desc: Test update trigger retain 0x1000 flag.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set async config
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 1;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Fork download abnormal
     */
    virtualAssetLoader_->SetDownloadStatus(DB_ERROR);
    /**
     * @tc.steps: step4. Sync cloud data
     * @tc.expected: step4. DB_ERROR and exist downloading assets count
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate_));
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, cloudCount * 2); // 1 record has 2 asset
    /**
     * @tc.steps: step5. Update local data
     * @tc.expected: step5. Exist downloading assets count
     */
    EXPECT_EQ(RDBDataGenerator::UpsertLocalDBData(0, cloudCount, db_,
        GetTableSchema("AsyncDownloadAssetsTest", true)), OK);
    std::tie(status, downloadCount) = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, cloudCount * 2); // 1 record has 2 asset
}

/**
 * @tc.name: AsyncAbnormalDownload005
 * @tc.desc: Test download assets which was locked
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set async config
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Init data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 10;
    auto schema = GetSchema();
    ASSERT_TRUE(!schema.tables.empty());
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    CloudSyncOption option = GetAsyncCloudSyncOption();
    option.asyncDownloadAssets = false;
    RelationalTestUtils::CloudBlockSync(option, delegate_, OK, OK);
    /**
     * @tc.steps: step3. Update cloud data and lock local data
     * @tc.expected: step3. ok
     */
    auto [records, extends] =
        RDBDataGenerator::GenerateDataRecords(0, cloudCount, 0, schema.tables.front().fields);
    Asset asset = {.name = "asset_1", .hash = "new_hash"};
    for (auto &record : records) {
        record.insert_or_assign("asset_1", asset);
    }
    std::string table = schema.tables.front().name;
    EXPECT_EQ(virtualCloudDb_->BatchUpdate(table, std::move(records), extends), OK);
    virtualAssetLoader_->ForkDownload([&](const std::string &tableName,
        std::map<std::string, Assets> &) {
        std::vector<std::vector<uint8_t>> hashKey;
        CloudDBSyncUtilsTest::GetHashKey(table, "data_key < 5", db_, hashKey); // lock half of the data
        EXPECT_EQ(Lock(table, hashKey, db_), OK);
    });
    /**
     * @tc.steps: step4. Sync and check data
     * @tc.expected: step4. ok
     */
    option.asyncDownloadAssets = true;
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate_));
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, cloudCount / 2); // half of the data was not downloaded due to being locked
    virtualAssetLoader_->ForkDownload(nullptr);
}

/**
 * @tc.name: AsyncNormalDownload001
 * @tc.desc: Test abnormal async download.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncNormalDownload001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Register observer
     * @tc.expected: step1. ok
     */
    auto rdbObserver = new(std::nothrow) RelationalStoreObserverUnitTest();
    ASSERT_NE(rdbObserver, nullptr);
    ResFinalizer resFinalizer([rdbObserver, this]() {
        delegate_->UnRegisterObserver(rdbObserver);
        delete rdbObserver;
    });
    EXPECT_EQ(delegate_->RegisterObserver(rdbObserver), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 1;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    EXPECT_EQ(RDBDataGenerator::InsertLocalDBData(cloudCount + 1, 1, db_, GetSchema()), E_OK);
    /**
     * @tc.steps: step3. Block upload while async donwload asset
     * @tc.expected: step3. Sync ok
     */
    auto hook = RelationalTestUtils::GetRDBStorageHook(USER_ID, APP_ID, STORE_ID_1, storePath_);
    ASSERT_NE(hook, nullptr);
    hook->SetBeforeUploadTransaction([]() {
        int count = 1;
        const int maxLoop = 5;
        do {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            count++;
        } while (RuntimeContext::GetInstance()->GetAssetsDownloadManager()->GetCurrentDownloadCount() > 0 &&
            count < maxLoop);
        LOGW("AsyncNormalDownload001 End hook");
    });
    CloudSyncOption option = GetAsyncCloudSyncOption();
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate_));
    EXPECT_TRUE(rdbObserver->IsAssetChange(GetTableSchema("AsyncDownloadAssetsTest").name));
    hook->SetBeforeUploadTransaction(nullptr);
}

/**
 * @tc.name: AsyncNormalDownload002
 * @tc.desc: Test sync download when download task pool is full
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lhy
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncNormalDownload002, TestSize.Level4)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. Ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. Ok
     */
    const int cloudCount = 1;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Fork download abnormal
     */
    virtualAssetLoader_->SetDownloadStatus(DB_ERROR);
    /**
     * @tc.steps: step4. Async cloud data, with abnormal result
     * @tc.expected: step4. Ok
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, cloudCount * GetAssetFieldCount());
    EXPECT_FALSE(RelationalTestUtils::IsExistEmptyHashAsset(db_, GetTableSchema("AsyncDownloadAssetsTest")));
    /**
     * @tc.steps: step5. Wait for failed download to finish
     * @tc.expected: step5. Download task changes from 1 to 0
     */
    auto manager = RuntimeContext::GetInstance()->GetAssetsDownloadManager();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(manager->GetCurrentDownloadCount(), 0u);
    /**
     * @tc.steps: step6. Start a new download task to reach maxDownloadTask
     * @tc.expected: step6. 1 task is downloading
     */
    auto [errCode, listener] = manager->BeginDownloadWithListener(nullptr);
    ASSERT_EQ(errCode, E_OK);
    ASSERT_EQ(listener, nullptr);
    ASSERT_EQ(manager->GetCurrentDownloadCount(), 1u);
    /**
     * @tc.steps: step7. Set download status to ok then try sync while task pool is full
     * @tc.expected: step7. Download should be waiting instead of doing compensated sync
     */
    virtualAssetLoader_->SetDownloadStatus(OK);
    LOGW("set download ok");
    int count = 0;
    std::mutex countMutex;
    std::condition_variable cv;
    virtualAssetLoader_->ForkDownload([&count, &countMutex, &cv](const std::string &tableName,
        std::map<std::string, Assets> &) {
        std::lock_guard<std::mutex> autoLock(countMutex);
        count++;
        if (count == 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        cv.notify_all();
    });
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(count, 0);
    /**
     * @tc.steps: step8. Finish the download task, to let sync continue
     * @tc.expected: step8. Only 1 actural download through virtualAssetLoader_
     */
    manager->FinishDownload();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::unique_lock<std::mutex> uniqueLock(countMutex);
    cv.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT), [&count]() {
        return count >= cloudCount;
    });
    EXPECT_EQ(count, 1);
    virtualAssetLoader_->ForkDownload(nullptr);
}

/**
 * @tc.name: AsyncNormalDownload003
 * @tc.desc: Test:async asset task paused
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncNormalDownload003, TestSize.Level4)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. Ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. Ok
     */
    const int cloudCount = 1;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. async asset task submit
     * @tc.expected: step3. Ok
     */
    int count = 0;
    int expQueryTimes = 3;
    std::mutex mutex;
    std::condition_variable cond;
    virtualCloudDb_->ForkQuery([&count, &cond, expQueryTimes](const std::string &, VBucket &extend) {
        count++;
        if (count == 1) {
            cond.notify_all();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (count == expQueryTimes) {
            std::string cursor;
            CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::CURSOR_FIELD, extend, cursor);
            EXPECT_EQ(cursor, std::string("1"));
        }
    });
    std::thread t1([this]{
        CloudSyncOption option = GetAsyncCloudSyncOption();
        EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate_));
    });
    /**
     * @tc.steps: step4. wait for async task query
     * @tc.expected: step4. Ok
     */
    {
        std::unique_lock<std::mutex> lock(mutex);
        (void)cond.wait_for(lock, std::chrono::seconds(1), [&count]() {
            return count == 1;
        });
    }
    /**
     * @tc.steps: step5. priority task submit
     * @tc.expected: step5. Ok
     */
    CloudSyncOption priOption = GetAsyncCloudSyncOption();
    std::vector<int64_t> inValue = {3, 4};
    priOption.priorityTask = true;
    priOption.query = Query::Select().From("AsyncDownloadAssetsTest").In("pk", inValue);
    priOption.asyncDownloadAssets = false;
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(priOption, delegate_));
    t1.join();
    virtualCloudDb_->ForkQuery(nullptr);
    EXPECT_EQ(count, expQueryTimes);
}

/**
 * @tc.name: AsyncNormalDownload004
 * @tc.desc: Test multiple tables and multiple batches of asset downloads
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncNormalDownload004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. Ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadAssetsCount = 25;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. Ok
     */
    const int cloudCount = 100;
    std::string table1 = "TABLE1";
    std::string table2 = "TABLE2";
    DataBaseSchema schema;
    schema.tables.push_back(GetTableSchema(table1));
    schema.tables.push_back(GetTableSchema(table2));
    EXPECT_EQ(RDBDataGenerator::InitDatabase(schema, *db_), SQLITE_OK);
    auto [record1, extend1] = RDBDataGenerator::GenerateDataRecords(0, cloudCount, 0, GetTableSchema(table1).fields);
    EXPECT_EQ(virtualCloudDb_->BatchInsertWithGid(table1, std::move(record1), extend1), OK);
    auto [record2, extend2] = RDBDataGenerator::GenerateDataRecords(0, cloudCount, cloudCount,
        GetTableSchema(table2).fields);
    EXPECT_EQ(virtualCloudDb_->BatchInsertWithGid(table2, std::move(record2), extend2), OK);
    /**
     * @tc.steps: step3. async asset task submit
     * @tc.expected: step3. Ok
     */
    int assetsDownloadTime = 0;
    virtualAssetLoader_->ForkDownload([&table1, &table2, &assetsDownloadTime](const std::string &tableName,
        std::map<std::string, Assets> &) {
        if (assetsDownloadTime < 100) { // 100 assets
            EXPECT_EQ(tableName, table1);
        } else {
            EXPECT_EQ(tableName, table2);
        }
        assetsDownloadTime++;
    });
    CloudSyncOption option;
    option.devices = {"cloud"};
    option.asyncDownloadAssets = true;
    Query query = Query::Select().FromTable({table1, table2});
    option.query = query;
    EXPECT_NO_FATAL_FAILURE(RelationalTestUtils::CloudBlockSync(option, delegate_));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    EXPECT_EQ(assetsDownloadTime, 200);
    virtualAssetLoader_->ForkDownload(nullptr);
}

/**
 * @tc.name: AsyncNormalDownload005
 * @tc.desc: Test concurrent async download of assets and sync data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncNormalDownload005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 12;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data and sync concurrently
     * @tc.expected: step2. ok
     */
    const int cloudCount = 200;
    auto schema = GetSchema();
    int threadNum = 10;
    CloudSyncOption option = GetAsyncCloudSyncOption();
    thread *syncThreads[threadNum];
    for (int i = 0; i < threadNum; i++) {
        syncThreads[i] = new thread([&]() {
            EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
            RelationalTestUtils::CloudBlockSync(option, delegate_);
            DeleteLocalData(db_, "AsyncDownloadAssetsTest");
        });
    }
    for (auto &thread : syncThreads) {
        thread->join();
        delete thread;
    }
}

/**
 * @tc.name: AsyncAbnormalDownload006
 * @tc.desc: Test abnormal async download.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload006, TestSize.Level4)
{
    /**
     * @tc.steps: step1. Set config and insert 70 cloud data
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 5;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    const int cloudCount = 70;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);

    /**
     * @tc.steps: step2. Fork download abnormal for 0-10 records
     * @tc.expected: step2. ok
     */
    virtualAssetLoader_->SetDownloadStatus(DB_ERROR);
    uint32_t failNum = 10;
    const DownloadFailRange setRange = {.isAllFail = false, .failBeginIndex = 0, .failEndIndex = failNum};
    virtualAssetLoader_->SetDownloadFailRange(setRange);

    /**
     * @tc.steps: step3. Async cloud data
     * @tc.expected: step3. ok
     */
    int count = 0;
    std::mutex countMutex;
    std::condition_variable cv;
    virtualAssetLoader_->ForkDownload([&count, &countMutex, &cv](const std::string &tableName,
        std::map<std::string, Assets> &) {
        std::lock_guard<std::mutex> autoLock(countMutex);
        count++;
        if (count == 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        cv.notify_all();
    });

    CloudSyncOption option = GetAsyncCloudSyncOption();
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    std::unique_lock<std::mutex> uniqueLock(countMutex);
    cv.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT), [&count]() {
        return count >= cloudCount;
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, static_cast<int32_t>(failNum * 2)); // 1 record has 2 asset

    virtualAssetLoader_->SetDownloadStatus(OK);
    virtualAssetLoader_->Reset();
    virtualAssetLoader_->ForkDownload(nullptr);
}

void DistributedDBCloudAsyncDownloadAssetsTest::UpdateLocalData(
        sqlite3 *&db, const std::string &tableName, int32_t begin, int32_t end)
{
    const string sql = "update " + tableName + " set int_field = int_field+1 " + "where pk>=" + std::to_string(begin) +
        " and pk<=" + std::to_string(end) + ";";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    LOGW("update local data finished");
}

void DistributedDBCloudAsyncDownloadAssetsTest::DeleteLocalData(sqlite3 *&db, const std::string &tableName)
{
    const string sql = "delete from " + tableName + " where pk >= 0;";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    LOGW("delete local data finished");
}

void DistributedDBCloudAsyncDownloadAssetsTest::CheckLogTable(sqlite3 *&db, const std::string &tableName, int count)
{
    const string sql = "select COUNT(*) from " + DBCommon::GetLogTableName(tableName) + " where data_key>0;";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
                reinterpret_cast<void *>(count), nullptr), SQLITE_OK);
    LOGW("check log table finished");
}

void DistributedDBCloudAsyncDownloadAssetsTest::UpdateLocalAndCheckUploadCount(const bool &isAsync,
    const int &dataCount, const int &expectCount)
{
    /**
     * @tc.steps: step1. Set async config
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 12;
    config.maxDownloadAssetsCount = 100;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Init data, set download status false and sync
     * @tc.expected: step2. async download will return OK and sync download will return CLOUD_ERROR
     */
    const int cloudCount = dataCount;
    auto schema = GetSchema();
    ASSERT_TRUE(!schema.tables.empty());
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    virtualAssetLoader_->SetDownloadStatus(DB_ERROR);
    CloudSyncOption option = GetAsyncCloudSyncOption();
    option.asyncDownloadAssets = isAsync;
    DBStatus expectStatus = isAsync ? OK : CLOUD_ERROR;
    RelationalTestUtils::CloudBlockSync(option, delegate_, OK, expectStatus);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step3. Modify all local data
     * @tc.expected: step3. OK
     */
    UpdateLocalData(db_, "AsyncDownloadAssetsTest", 0, cloudCount);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step4. Fork upload to count number of uploaded records
     * @tc.expected: step4. OK
     */
    int count = 0;
    std::mutex countMutex;
    std::condition_variable cv;
    virtualCloudDb_->ForkUpload([&count, &countMutex, &cv](const std::string&, VBucket&) {
        std::lock_guard<std::mutex> autoLock(countMutex);
        count++;
        cv.notify_all();
    });
    virtualAssetLoader_->SetDownloadStatus(OK);
    virtualAssetLoader_->Reset();
    /**
     * @tc.steps: step5. Sync again and check upload count
     * @tc.expected: step5. OK
     */
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    std::unique_lock<std::mutex> uniqueLock(countMutex);
    cv.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MIN_TIMEOUT), [&count, &cloudCount, &isAsync]() {
        return !isAsync || count >= cloudCount;
    });
    EXPECT_EQ(count, expectCount);
    /**
     * @tc.steps: step6. Release resources
     * @tc.expected: step6. OK
     */
    virtualAssetLoader_->Reset();
    virtualAssetLoader_->ForkDownload(nullptr);
    virtualCloudDb_->ForkUpload(nullptr);
}

/**
 * @tc.name: AsyncAbnormalDownload007
 * @tc.desc: Test in async download mode and asset is not downloaded, the local data can be uploaded
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload007, TestSize.Level1)
{
    int cloudCount = 10;
    EXPECT_NO_FATAL_FAILURE(UpdateLocalAndCheckUploadCount(true, cloudCount, cloudCount));
}

/**
 * @tc.name: AsyncAbnormalDownload009
 * @tc.desc: Test in sync download mode and asset is not downloaded, the local data will be ignored when upload
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload009, TestSize.Level1)
{
    int cloudCount = 10;
    UpdateLocalAndCheckUploadCount(false, cloudCount, 0);
}

DBStatus ModifySkippedAsset(int rowIndex, std::map<std::string, Assets> &assets, DBStatus fakeStatus)
{
    if (rowIndex != 1) {
        return OK;
    }
    for (auto &asset : assets) {
        for (auto &item : asset.second) {
            if (item.name == "asset_1" + std::to_string(rowIndex)) {
                item.status = static_cast<uint32_t>(AssetStatus::ABNORMAL);
            }
        }
    }
    return fakeStatus;
}

void DistributedDBCloudAsyncDownloadAssetsTest::DoSkipAssetDownload(SkipAssetTestParamT param)
{
    /**
     * @tc.steps: step1 change max download task
     * @tc.expected: step1. Ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = CloudDbConstant::MAX_ASYNC_DOWNLOAD_TASK; // maximum of tasks
    config.maxDownloadAssetsCount = CloudDbConstant::MAX_ASYNC_DOWNLOAD_ASSETS; // maximum of asset counts
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2 Insert cloud data
     * @tc.expected: step2. Ok
     */
    const int cloudCount = 5;
    auto schema = GetSchema();
    std::string tableName = "AsyncDownloadAssetsTest";
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(param.startIndex, cloudCount, 0, schema, virtualCloudDb_), OK);
    CloudDBSyncUtilsTest::CheckLocalRecordNum(db_, tableName, param.startIndex);
    /**
     * @tc.steps: step3 Fork download abnormal
     * @tc.expected: step3. Ok
     */
    int count = 0;
    std::mutex countMutex;
    std::condition_variable cv;
    if (param.useBatch) {
        RuntimeContext::GetInstance()->SetBatchDownloadAssets(true);
        virtualAssetLoader_->ForkBatchDownload([&count, &countMutex, &cv, param](int rowIndex,
            std::map<std::string, Assets> &assets) {
            std::lock_guard<std::mutex> autoLock(countMutex);
            count++;
            auto ret = ModifySkippedAsset(rowIndex, assets, param.downloadRes);
            if (count == 1) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            cv.notify_all();
            return ret;
        });
    } else {
        RuntimeContext::GetInstance()->SetBatchDownloadAssets(false);
        virtualAssetLoader_->SetDownloadStatus(param.downloadRes);
    }
    /**
     * @tc.steps: step4. sync cloud data
     * @tc.expected: step4. Ok
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    option.asyncDownloadAssets = param.useAsync;
    RelationalTestUtils::CloudBlockSync(option, delegate_, param.expectSyncRes);
    /**
     * @tc.steps: step5. wait for sync to finish
     * @tc.expected: step5. check local record number and inconsistent count
     */
    std::unique_lock<std::mutex> uniqueLock(countMutex);
    auto res = cv.wait_for(uniqueLock, std::chrono::seconds(DBConstant::MAX_SYNC_TIMEOUT),
        [&count, param, cloudCount] {
        return count >= cloudCount || !param.useBatch;
    });
    EXPECT_TRUE(res);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    CloudDBSyncUtilsTest::CheckLocalRecordNum(db_, tableName, param.startIndex + cloudCount);
    CheckInconsistentCount(db_, param.expectInconsistentCount);
    /**
     * @tc.steps: step6. clear
     */
    virtualAssetLoader_->SetDownloadStatus(OK);
    virtualAssetLoader_->Reset();
    virtualAssetLoader_->ForkBatchDownload(nullptr);
}

/**
 * @tc.name: SkipAssetDownloadTest001
 * @tc.desc: Test async batch download returns Skip_Assets
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, SkipAssetDownloadTest001, TestSize.Level1)
{
    /**
     * @tc.expected: step1. sync return OK, and has 1 inconsistent records
     */
    SkipAssetTestParamT param = {.downloadRes = SKIP_ASSET, .useBatch = true, .useAsync = true,
        .startIndex = 0, .expectInconsistentCount = 1, .expectSyncRes = OK};
    DoSkipAssetDownload(param);
}

/**
 * @tc.name: SkipAssetDownloadTest002
 * @tc.desc: Test sync batch download returns Skip_Assets
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, SkipAssetDownloadTest002, TestSize.Level1)
{
    /**
     * @tc.expected: step1. sync return OK, and has 1 inconsistent records
     */
    SkipAssetTestParamT param = {.downloadRes = SKIP_ASSET, .useBatch = true, .useAsync = false,
        .startIndex = 0, .expectInconsistentCount = 1, .expectSyncRes = OK};
    DoSkipAssetDownload(param);
}

/**
 * @tc.name: SkipAssetDownloadTest003
 * @tc.desc: Test sync one-by-one download returns Skip_Assets
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, SkipAssetDownloadTest003, TestSize.Level1)
{
    /**
     * @tc.expected: step1. sync return OK, and has 5 inconsistent records
     */
    SkipAssetTestParamT param = {.downloadRes = SKIP_ASSET, .useBatch = false, .useAsync = false,
        .startIndex = 0, .expectInconsistentCount = 5, .expectSyncRes = OK};
    DoSkipAssetDownload(param);
}

/**
 * @tc.name: AsyncAbnormalDownload008
 * @tc.desc: Test the total count of download after download failure
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    config.maxDownloadAssetsCount = 2;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 4;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Fork download return cloud error
     * @tc.expected: step3. CLOUD_ERROR
     */
    int downloadIndex = 0;
    int failedCount = 2;
    virtualAssetLoader_->ForkBatchDownload([&downloadIndex, failedCount](
        int rowIndex, std::map<std::string, Assets> &assets) {
        downloadIndex++;
        if (downloadIndex <= failedCount) {
            return CLOUD_ERROR;
        }
        return OK;
    });
    /**
     * @tc.steps: step4. Sync
     * @tc.expected: step4. ok
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step5. Check download count
     * @tc.expected: step5. ok
     */
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_EQ(downloadCount, cloudCount);
    virtualAssetLoader_->ForkBatchDownload(nullptr);
}

/**
 * @tc.name: AsyncAbnormalDownload010
 * @tc.desc: Test assets is async downloading when delete local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload010, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    config.maxDownloadAssetsCount = 2;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 2000;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Sync
     * @tc.expected: step3. ok
     */
    CloudSyncOption option = GetAsyncCloudSyncOption();
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    /**
     * @tc.steps: step4. Delete local data.
     * @tc.expected: step4. ok
     */
    DeleteLocalData(db_, "AsyncDownloadAssetsTest");
    /**
     * @tc.steps: step5. Check log table.
     * @tc.expected: step5. ok
     */
    CheckLogTable(db_, "AsyncDownloadAssetsTest", 0);
}

/**
 * @tc.name: TriggerAsyncTask001
 * @tc.desc: Test trigger async task.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, TriggerAsyncTask001, TestSize.Level1)
{
    auto storage = std::make_shared<MockICloudSyncStorageInterface>();
    ASSERT_NE(storage, nullptr);
    auto proxy = StorageProxy::GetCloudDb(storage.get());
    ASSERT_NE(proxy, nullptr);
    auto syncer = new(std::nothrow) VirtualCloudSyncer(proxy);
    ASSERT_NE(syncer, nullptr);
    /**
     * @tc.steps: step1. Trigger async task with not exist table contain assets.
     */
    syncer->TriggerAsyncTask();
    /**
     * @tc.steps: step2. Trigger async task with exist table contain assets.
     */
    EXPECT_CALL(*storage, IsExistTableContainAssets).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*storage, GetDownloadAssetTable).WillRepeatedly([]() {
        std::pair<int, std::vector<std::string>> res;
        return res;
    });
    syncer->TriggerAsyncTask();
    syncer->WaitTaskFinished();
    /**
     * @tc.steps: step3. Async trigger and wait finished.
     */
    std::thread t1([syncer]() {
        for (int i = 0; i < 1000; ++i) {
            syncer->TriggerAsyncTask();
        }
    });
    std::thread t2([syncer]() {
        for (int i = 0; i < 1000; ++i) {
            syncer->WaitTaskFinished();
        }
    });
    t1.join();
    t2.join();
    syncer->Close();
    RefObject::KillAndDecObjRef(syncer);
}

/**
 * @tc.name: AsyncAbnormalDownload011
 * @tc.desc: Test set reference when async downloading.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload011, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    config.maxDownloadAssetsCount = 2;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data
     * @tc.expected: step2. ok
     */
    const int cloudCount = 100;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    /**
     * @tc.steps: step3. Fork download set reference
     * @tc.expected: step3. ok
     */
    TableReferenceProperty tableReferenceProperty;
    tableReferenceProperty.sourceTableName = "AsyncDownloadAssetsTest";
    tableReferenceProperty.targetTableName = "TABLE1";
    std::map<std::string, std::string> columns;
    columns["int_field"] = "int_field";
    tableReferenceProperty.columns = columns;
    CloudSyncOption option = GetAsyncCloudSyncOption();
 
    std::mutex mtx;
    std::condition_variable cv;
    bool t2_started = false;
 
    std::thread t1([this, option, &mtx, &cv, &t2_started]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            t2_started = true;
            cv.notify_one();
        }
        RelationalTestUtils::CloudBlockSync(option, delegate_);
    });
 
    std::thread t2([this, tableReferenceProperty, &mtx, &cv, &t2_started] {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return t2_started; });
        EXPECT_EQ(delegate_->SetReference({tableReferenceProperty}), OK);
    });
 
    t1.join();
    t2.join();
    /**
     * @tc.steps: step5. Check download count
     * @tc.expected: step5. ok
     */
    auto [status, downloadCount] = delegate_->GetDownloadingAssetsCount();
    EXPECT_EQ(status, OK);
    EXPECT_NE(downloadCount, 0);
    virtualAssetLoader_->ForkBatchDownload(nullptr);
}

/**
 * @tc.name: AsyncAbnormalDownload012
 * @tc.desc: Test complex async download.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudAsyncDownloadAssetsTest, AsyncAbnormalDownload012, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Set max download task 1
     * @tc.expected: step1. ok
     */
    AsyncDownloadAssetsConfig config;
    config.maxDownloadTask = 1;
    config.maxDownloadAssetsCount = 1;
    EXPECT_EQ(RuntimeConfig::SetAsyncDownloadAssetsConfig(config), OK);
    /**
     * @tc.steps: step2. Insert cloud data and sync
     * @tc.expected: step2. ok
     */
    const int cloudCount = 1;
    auto schema = GetSchema();
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, cloudCount, 0, schema, virtualCloudDb_), OK);
    virtualAssetLoader_->SetDownloadStatus(DB_ERROR);
    CloudSyncOption option = GetAsyncCloudSyncOption();
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 1 is second to wait
    /**
     * @tc.steps: step3. Update asset then sync to check notify count
     * @tc.expected: step3. ok
     */
    EXPECT_EQ(sqlite3_exec(db_, "UPDATE AsyncDownloadAssetsTest SET assets_1 = NULL, asset_1 = NULL", nullptr,
        nullptr, nullptr), SQLITE_OK);
    virtualCloudDb_->ForkUpload([](const std::string &tableName, VBucket &extend) {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // 2 is second to wait
    });
    RelationalStoreObserverUnitTest *observer1 = new (std::nothrow) RelationalStoreObserverUnitTest();
    EXPECT_NE(observer1, nullptr);
    EXPECT_EQ(delegate_->RegisterObserver(observer1), OK);
    virtualAssetLoader_->SetDownloadStatus(OK);
    RelationalTestUtils::CloudBlockSync(option, delegate_);
    std::this_thread::sleep_for(std::chrono::seconds(3)); // 3 is second to wait
    int expCnt = 1;
    EXPECT_EQ(observer1->GetCloudCallCount(), expCnt);
    EXPECT_EQ(delegate_->UnRegisterObserver(observer1), OK);
    delete observer1;
}
}