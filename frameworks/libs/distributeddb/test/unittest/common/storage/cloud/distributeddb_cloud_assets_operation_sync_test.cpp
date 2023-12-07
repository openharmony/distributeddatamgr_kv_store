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
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "distributeddb_data_generate_unit_test.h"
#include "log_print.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "time_helper.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"
#include "virtual_cloud_db.h"
#include "sqlite_relational_utils.h"
#include "cloud/cloud_storage_utils.h"

namespace {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
const char *g_createSQL =
    "CREATE TABLE IF NOT EXISTS DistributedDBCloudAssetsOperationSyncTest(" \
    "id TEXT PRIMARY KEY," \
    "name TEXT," \
    "height REAL ," \
    "photo BLOB," \
    "asset ASSET," \
    "assets ASSETS," \
    "age INT);";
const int64_t g_syncWaitTime = 60;
const Asset g_localAsset = {
    .version = 2, .name = "Phone", .assetId = "0", .subpath = "/local/sync", .uri = "/cloud/sync",
    .modifyTime = "123456", .createTime = "0", .size = "1024", .hash = "DEC"
};

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
    LOGW("begin call sync");
    ASSERT_EQ(delegate->Sync({ "CLOUD" }, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), OK);
    std::unique_lock<std::mutex> uniqueLock(dataMutex);
    cv.wait(uniqueLock, [&finish]() {
        return finish;
    });
    LOGW("end call sync");
}

class DistributedDBCloudAssetsOperationSyncTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
protected:
    void InitTestDir();
    DataBaseSchema GetSchema();
    void CloseDb();
    void InsertUserTableRecord(const std::string &tableName, int64_t begin, int64_t count, int64_t photoSize,
        bool assetIsNull);
    void CheckAssetsCount(const std::vector<size_t> &expectCount);
    void UpdateCloudTableRecord(int64_t begin, int64_t count, bool assetIsNull);
    std::string testDir_;
    std::string storePath_;
    sqlite3 *db_ = nullptr;
    RelationalStoreDelegate *delegate_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader_ = nullptr;
    std::shared_ptr<VirtualCloudDataTranslate> virtualTranslator_ = nullptr;
    std::shared_ptr<RelationalStoreManager> mgr_ = nullptr;
    std::string tableName_ = "DistributedDBCloudAssetsOperationSyncTest";
};

void DistributedDBCloudAssetsOperationSyncTest::SetUpTestCase()
{
    RuntimeConfig::SetCloudTranslate(std::make_shared<VirtualCloudDataTranslate>());
}

void DistributedDBCloudAssetsOperationSyncTest::TearDownTestCase()
{}

void DistributedDBCloudAssetsOperationSyncTest::SetUp()
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
    virtualTranslator_ = std::make_shared<VirtualCloudDataTranslate>();
    DataBaseSchema dataBaseSchema = GetSchema();
    ASSERT_EQ(delegate_->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
}

void DistributedDBCloudAssetsOperationSyncTest::TearDown()
{
    CloseDb();
    EXPECT_EQ(sqlite3_close_v2(db_), SQLITE_OK);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(testDir_) != E_OK) {
        LOGE("rm test db files error.");
    }
}

void DistributedDBCloudAssetsOperationSyncTest::InitTestDir()
{
    if (!testDir_.empty()) {
        return;
    }
    DistributedDBToolsUnitTest::TestDirInit(testDir_);
    storePath_ = testDir_ + "/" + STORE_ID_1 + ".db";
    LOGI("The test db is:%s", testDir_.c_str());
}

DataBaseSchema DistributedDBCloudAssetsOperationSyncTest::GetSchema()
{
    DataBaseSchema schema;
    TableSchema tableSchema;
    tableSchema.name = tableName_;
    tableSchema.fields = {
        {"id", TYPE_INDEX<std::string>, true}, {"name", TYPE_INDEX<std::string>}, {"height", TYPE_INDEX<double>},
        {"photo", TYPE_INDEX<Bytes>}, {"asset", TYPE_INDEX<Asset>}, {"assets", TYPE_INDEX<Assets>},
        {"age", TYPE_INDEX<int64_t>}
    };
    schema.tables.push_back(tableSchema);
    return schema;
}

void DistributedDBCloudAssetsOperationSyncTest::CloseDb()
{
    virtualCloudDb_->ForkUpload(nullptr);
    virtualCloudDb_ = nullptr;
    EXPECT_EQ(mgr_->CloseStore(delegate_), DBStatus::OK);
    delegate_ = nullptr;
    mgr_ = nullptr;
}

void DistributedDBCloudAssetsOperationSyncTest::InsertUserTableRecord(const std::string &tableName, int64_t begin,
    int64_t count, int64_t photoSize, bool assetIsNull)
{
    std::string photo(photoSize, 'v');
    int errCode;
    std::vector<uint8_t> assetBlob;
    std::vector<uint8_t> assetsBlob;
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string name = g_localAsset.name + std::to_string(i);
        Asset asset = g_localAsset;
        asset.name = name;
        RuntimeContext::GetInstance()->AssetToBlob(asset, assetBlob);
        std::vector<Asset> assets;
        asset.name = name + "_1";
        asset.status = static_cast<uint32_t>(AssetStatus::INSERT);
        assets.push_back(asset);
        asset.name = name + "_2";
        assets.push_back(asset);
        string sql = "INSERT OR REPLACE INTO " + tableName +
            " (id, name, height, photo, asset, assets, age) VALUES ('" + std::to_string(i) +
            "', 'local', '178.0', '" + photo + "', ?, ?, '18');";
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
        RuntimeContext::GetInstance()->AssetsToBlob(assets, assetsBlob);
        if (assetIsNull) {
            ASSERT_EQ(sqlite3_bind_null(stmt, 1), SQLITE_OK); // 1 is bind asset
            ASSERT_EQ(sqlite3_bind_null(stmt, 2), SQLITE_OK); // 2 is bind assets
        } else {
            ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false), E_OK); // 1 is bind asset
            ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 2, assetsBlob, false), E_OK); // 2 is bind assets
        }
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
}

void DistributedDBCloudAssetsOperationSyncTest::UpdateCloudTableRecord(int64_t begin, int64_t count, bool assetIsNull)
{
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    Timestamp now = TimeHelper::GetSysCurrentTime();
    for (int64_t i = begin; i < (begin + count); ++i) {
        VBucket data;
        data.insert_or_assign("id", std::to_string(i));
        data.insert_or_assign("name", "Cloud" + std::to_string(i));
        Assets assets;
        for (int j = 1; j <= 2; ++j) { // add 2 asset in assets col
            Asset asset;
            asset.name = "Phone_" + std::to_string(j);
            asset.assetId = std::to_string(j);
            asset.status = AssetStatus::UPDATE;
        }
        record.push_back(data);
        VBucket log;
        log.insert_or_assign(CloudDbConstant::CREATE_FIELD, static_cast<int64_t>(
            now / CloudDbConstant::TEN_THOUSAND));
        log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, static_cast<int64_t>(
            now / CloudDbConstant::TEN_THOUSAND));
        log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
        log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(i));
        extend.push_back(log);
    }

    ASSERT_EQ(virtualCloudDb_->BatchUpdate(tableName_, std::move(record), extend), DBStatus::OK);
}

void DistributedDBCloudAssetsOperationSyncTest::CheckAssetsCount(const std::vector<size_t> &expectCount)
{
    std::vector<VBucket> allData;
    auto dbSchema = GetSchema();
    ASSERT_GT(dbSchema.tables.size(), 0u);
    ASSERT_EQ(RelationalTestUtils::SelectData(db_, dbSchema.tables[0], allData), E_OK);
    int index = 0;
    ASSERT_EQ(allData.size(), expectCount.size());
    for (const auto &data : allData) {
        auto colIter = data.find("assets");
        EXPECT_NE(colIter, data.end());
        if (colIter == data.end()) {
            index++;
            continue;
        }
        Type colValue = data.at("assets");
        auto translate = std::dynamic_pointer_cast<ICloudDataTranslate>(virtualTranslator_);
        auto assets = RelationalTestUtils::GetAssets(colValue, translate);
        LOGI("[DistributedDBCloudAssetsOperationSyncTest] Check data index %d", index);
        EXPECT_EQ(assets.size(), expectCount[index]);
        for (const auto &item : assets) {
            LOGI("[DistributedDBCloudAssetsOperationSyncTest] Asset name %s status %" PRIu32, item.name.c_str(),
                item.status);
        }
        index++;
    }
}

/**
 * @tc.name: SyncWithAssetOperation001
 * @tc.desc: Delete Assets When Download
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudAssetsOperationSyncTest, SyncWithAssetOperation001, TestSize.Level0)
{
    const int actualCount = 10;
    const int photoSize = 10;
    const int deleteDataCount = 5;
    const int deleteAssetsCount = 4;
    InsertUserTableRecord(tableName_, 0, photoSize, actualCount, false);
    std::string tableName = tableName_;
    virtualCloudDb_->ForkUpload([this, deleteDataCount, deleteAssetsCount](const std::string &, VBucket &) {
        for (int64_t i = 0; i < deleteDataCount; i++) {
            std::string sql = "DELETE FROM " + tableName_ + " WHERE id = " + std::to_string(i) + ";";
            ASSERT_EQ(RelationalTestUtils::ExecSql(db_, sql), SQLITE_OK);
        }
        for (int64_t i = deleteDataCount; i < deleteDataCount + deleteAssetsCount; i++) {
            std::string sql = "UPDATE " + tableName_ + " SET asset = NULL, assets = NULL WHERE id = " +
                std::to_string(i) + ";";
            ASSERT_EQ(RelationalTestUtils::ExecSql(db_, sql), SQLITE_OK);
        }
    });
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);
    virtualCloudDb_->ForkUpload(nullptr);
    std::vector<size_t> expectCount(actualCount - deleteDataCount, 0);
    expectCount[expectCount.size() - 1] = 2; // default one row has 2 assets
    CheckAssetsCount(expectCount);
}

/**
 * @tc.name: SyncWithAssetOperation002
 * @tc.desc: Download Assets When local assets was removed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudAssetsOperationSyncTest, SyncWithAssetOperation002, TestSize.Level0)
{
    const int actualCount = 1;
    InsertUserTableRecord(tableName_, 0, actualCount, 10, false);
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);
    int downLoadCount = 0;
    virtualAssetLoader_->ForkDownload([this, &downLoadCount](std::map<std::string, Assets> &assets) {
        downLoadCount++;
        if (downLoadCount == 1) {
            std::string sql = "UPDATE " + tableName_ + " SET assets = NULL WHERE id = 0;";
            ASSERT_EQ(RelationalTestUtils::ExecSql(db_, sql), SQLITE_OK);
        }
    });
    int removeCount = 0;
    virtualAssetLoader_->ForkRemoveLocalAssets([&removeCount](const std::vector<Asset> &assets) {
        EXPECT_EQ(assets.size(), 2u);
        removeCount++;
        return OK;
    });
    UpdateCloudTableRecord(0, actualCount, false);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    EXPECT_EQ(downLoadCount, 3); // local asset was removed should download 3 times
    EXPECT_EQ(removeCount, 1);
    virtualAssetLoader_->ForkDownload(nullptr);
    virtualAssetLoader_->ForkRemoveLocalAssets(nullptr);

    std::vector<size_t> expectCount = { 0 };
    CheckAssetsCount(expectCount);
}

/**
 * @tc.name: SyncWithAssetOperation003
 * @tc.desc: Delete Assets When Download
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudAssetsOperationSyncTest, SyncWithAssetOperation003, TestSize.Level0)
{
    InsertUserTableRecord(tableName_, 0, 1, 10, false); // 1 is size, 10 is count
    int uploadCount = 0;
    virtualCloudDb_->ForkUpload([this, &uploadCount](const std::string &, VBucket &) {
        if (uploadCount > 0) {
            return;
        }
        SqlCondition condition;
        condition.sql = "UPDATE " + tableName_ + " SET age = '666' WHERE id = 0;";
        std::vector<VBucket> records;
        EXPECT_EQ(delegate_->ExecuteSql(condition, records), OK);
        uploadCount++;
    });
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);
    virtualCloudDb_->ForkUpload(nullptr);

    std::string sql = "SELECT assets from " + tableName_ + " where id = 0;";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db_, sql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_BLOB);
        Type cloudValue;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Assets>, 0, cloudValue), E_OK);
        std::vector<uint8_t> assetsBlob;
        Assets assets;
        ASSERT_EQ(CloudStorageUtils::GetValueFromOneField(cloudValue, assetsBlob), E_OK);
        ASSERT_EQ(RuntimeContext::GetInstance()->BlobToAssets(assetsBlob, assets), E_OK);
        ASSERT_EQ(assets.size(), 2u); // 2 is asset num
        for (size_t i = 0; i < assets.size(); ++i) {
            EXPECT_EQ(assets[i].status, AssetStatus::NORMAL);
        }
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

/**
 * @tc.name: IgnoreRecord001
 * @tc.desc: Download Assets When local assets was removed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudAssetsOperationSyncTest, IgnoreRecord001, TestSize.Level0)
{
    const int actualCount = 1;
    InsertUserTableRecord(tableName_, 0, actualCount, 10, false);
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);
    std::vector<size_t> expectCount = { 2 };
    CheckAssetsCount(expectCount);

    VBucket record;
    record["id"] = std::to_string(0);
    record["assets"] = Assets();
    EXPECT_EQ(delegate_->UpsertData(tableName_, { record }), OK);
    record["id"] = std::to_string(1);
    EXPECT_EQ(delegate_->UpsertData(tableName_, { record }), OK);
    expectCount = { 0, 0 };
    CheckAssetsCount(expectCount);
}

/**
 * @tc.name: IgnoreRecord002
 * @tc.desc: Ignore Assets When Download
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudAssetsOperationSyncTest, IgnoreRecord002, TestSize.Level0)
{
    const int actualCount = 1;
    InsertUserTableRecord(tableName_, 0, actualCount, 10, false);
    Query query = Query::Select().FromTable({ tableName_ });
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    UpdateCloudTableRecord(0, actualCount, false);

    virtualAssetLoader_->SetDownloadStatus(DBStatus::CLOUD_RECORD_EXIST_CONFLICT);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    virtualAssetLoader_->SetDownloadStatus(DBStatus::OK);
    std::vector<size_t> expectCount = { 2 };
    CheckAssetsCount(expectCount);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
}

/**
 * @tc.name: IgnoreRecord003
 * @tc.desc: Ignore Assets When Upload
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudAssetsOperationSyncTest, IgnoreRecord003, TestSize.Level0)
{
    const int actualCount = 1;
    InsertUserTableRecord(tableName_, 0, actualCount, 10, false);
    Query query = Query::Select().FromTable({ tableName_ });
    virtualCloudDb_->SetConflictInUpload(true);
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    virtualCloudDb_->SetConflictInUpload(false);
    std::vector<size_t> expectCount = { 2 };
    CheckAssetsCount(expectCount);
}

/**
 * @tc.name: SyncWithAssetConflict001
 * @tc.desc: Upload with asset no change
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudAssetsOperationSyncTest, SyncWithAssetConflict001, TestSize.Level0)
{
    // cloud and local insert same data
    const int actualCount = 1;
    RelationalTestUtils::InsertCloudRecord(0, actualCount, tableName_, virtualCloudDb_);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s for data conflict
    InsertUserTableRecord(tableName_, 0, actualCount, 1, false);
    // sync and local asset's status are normal
    Query query = Query::Select().FromTable({ tableName_ });
    RelationalTestUtils::CloudBlockSync(query, delegate_);
    auto dbSchema = GetSchema();
    ASSERT_GT(dbSchema.tables.size(), 0u);
    auto assets = RelationalTestUtils::GetAllAssets(db_, dbSchema.tables[0], virtualTranslator_);
    for (const auto &oneRow : assets) {
        for (const auto &asset : oneRow) {
            EXPECT_EQ(asset.status, static_cast<uint32_t>(AssetStatus::NORMAL));
        }
    }
}
}
#endif