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
#include "cloud_db_constant.h"
#include "cloud_db_types.h"
#include "distributeddb_data_generate_unit_test.h"
#include "log_print.h"
#include "relational_store_delegate.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "time_helper.h"
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

const Asset g_cloudAsset = {
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
    void InsertCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull);
    void DeleteUserTableRecord(int64_t id);
    void DeleteCloudTableRecord(int64_t gid);
    void InitLogicDeleteDataEnv(int64_t dataCount);
    void CheckLocalCount(int64_t expectCount);
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

void DistributedDBCloudCheckSyncTest::InsertCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize,
    bool assetIsNull)
{
    std::vector<uint8_t> photo(photoSize, 'v');
    std::vector<VBucket> record1;
    std::vector<VBucket> extend1;
    std::vector<VBucket> record2;
    std::vector<VBucket> extend2;
    Timestamp now = TimeHelper::GetSysCurrentTime();
    for (int64_t i = begin; i < begin + count; ++i) {
        VBucket data;
        data.insert_or_assign("id", std::to_string(i));
        data.insert_or_assign("name", "Cloud" + std::to_string(i));
        data.insert_or_assign("height", 166.0); // 166.0 is random double value
        data.insert_or_assign("married", false);
        data.insert_or_assign("photo", photo);
        data.insert_or_assign("age", static_cast<int64_t>(13L)); // 13 is random age
        Asset asset = g_cloudAsset;
        asset.name = asset.name + std::to_string(i);
        assetIsNull ? data.insert_or_assign("assert", Nil()) : data.insert_or_assign("assert", asset);
        record1.push_back(data);
        VBucket log;
        log.insert_or_assign(CloudDbConstant::CREATE_FIELD, static_cast<int64_t>(
            now / CloudDbConstant::TEN_THOUSAND + i));
        log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, static_cast<int64_t>(
            now / CloudDbConstant::TEN_THOUSAND + i));
        log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
        extend1.push_back(log);

        std::vector<Asset> assets;
        data.insert_or_assign("height", 180.3); // 180.3 is random double value
        for (int64_t j = i; j <= i + 2; j++) { // 2 extra num
            asset.name = g_cloudAsset.name + std::to_string(j);
            assets.push_back(asset);
        }
        data.erase("assert");
        data.erase("married");
        assetIsNull ? data.insert_or_assign("asserts", Nil()) : data.insert_or_assign("asserts", assets);
        record2.push_back(data);
        extend2.push_back(log);
    }
    ASSERT_EQ(virtualCloudDb_->BatchInsert(tableName_, std::move(record1), extend1), DBStatus::OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(count));
}

void DistributedDBCloudCheckSyncTest::DeleteUserTableRecord(int64_t id)
{
    ASSERT_NE(db_, nullptr);
    string sql = "DELETE FROM " + tableName_ + " WHERE id ='" + std::to_string(id) + "';";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
}

void DistributedDBCloudCheckSyncTest::DeleteCloudTableRecord(int64_t gid)
{
    VBucket idMap;
    idMap.insert_or_assign("#_gid", std::to_string(gid));
    ASSERT_EQ(virtualCloudDb_->DeleteByGid(tableName_, idMap), DBStatus::OK);
}

void DistributedDBCloudCheckSyncTest::InitLogicDeleteDataEnv(int64_t dataCount)
{
    // prepare data
    InsertUserTableRecord(tableName_, dataCount);
    // sync
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);
    // delete cloud data
    for (int i = 0; i < dataCount; ++i) {
        DeleteCloudTableRecord(i);
    }
    // sync again
    BlockSync(query, delegate_);
}

void DistributedDBCloudCheckSyncTest::CheckLocalCount(int64_t expectCount)
{
    // check local data
    int dataCnt = -1;
    std::string checkLogSql = "SELECT count(*) FROM " + tableName_;
    RelationalTestUtils::ExecSql(db_, checkLogSql, nullptr, [&dataCnt](sqlite3_stmt *stmt) {
        dataCnt = sqlite3_column_int(stmt, 0);
        return E_OK;
    });
    EXPECT_EQ(dataCnt, expectCount);
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

/**
 * @tc.name: CloudSyncTest002
 * @tc.desc: sync with same data in one batch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudSyncTest002, TestSize.Level0)
{
    // prepare data
    const int actualCount = 1;
    InsertUserTableRecord(actualCount);
    // sync twice
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);
    // cloud delete id=0 and insert id=0 but its gid is 1
    // local delete id=0
    DeleteCloudTableRecord(0); // cloud gid is 0
    InsertCloudTableRecord(0, actualCount, 0, false); // 0 is id
    DeleteUserTableRecord(0); // 0 is id
    BlockSync(query, delegate_);
    bool deleteStatus = true;
    EXPECT_EQ(virtualCloudDb_->GetDataStatus("1", deleteStatus), OK);
    EXPECT_EQ(deleteStatus, false);
    // check local data
    int dataCnt = -1;
    std::string checkLogSql = "SELECT count(*) FROM " + tableName_;
    RelationalTestUtils::ExecSql(db_, checkLogSql, nullptr, [&dataCnt](sqlite3_stmt *stmt) {
        dataCnt = sqlite3_column_int(stmt, 0);
        return E_OK;
    });
    EXPECT_EQ(dataCnt, actualCount);
}

/**
 * @tc.name: CloudSyncObserverTest001
 * @tc.desc: test cloud sync multi observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudSyncObserverTest001, TestSize.Level0)
{
    // prepare data
    const int actualCount = 10;
    InsertUserTableRecord(actualCount);

    /**
     * @tc.steps:step1. open two delegate with two observer.
     * @tc.expected: step1. ok.
     */
    RelationalStoreDelegate::Option option;
    auto observer1 = new (std::nothrow) RelationalStoreObserverUnitTest();
    ASSERT_NE(observer1, nullptr);
    option.observer = observer1;
    RelationalStoreDelegate *delegate1 = nullptr;
    EXPECT_EQ(mgr_->OpenStore(storePath_, STORE_ID_1, option, delegate1), DBStatus::OK);
    ASSERT_NE(delegate1, nullptr);

    auto observer2 = new (std::nothrow) RelationalStoreObserverUnitTest();
    ASSERT_NE(observer2, nullptr);
    option.observer = observer2;
    RelationalStoreDelegate *delegate2 = nullptr;
    EXPECT_EQ(mgr_->OpenStore(storePath_, STORE_ID_1, option, delegate2), DBStatus::OK);
    ASSERT_NE(delegate2, nullptr);

    /**
     * @tc.steps:step2. insert 1-10 cloud data, start.
     * @tc.expected: step2. ok.
     */
    InsertCloudTableRecord(0, actualCount, actualCount, false);
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);

    /**
     * @tc.steps:step3. check observer.
     * @tc.expected: step3. ok.
     */
    EXPECT_EQ(observer1->GetCloudCallCount(), 1u);
    EXPECT_EQ(observer2->GetCloudCallCount(), 1u);

    /**
     * @tc.steps:step4. insert 11-20 cloud data, start.
     * @tc.expected: step4. ok.
     */
    delegate2->UnRegisterObserver();
    observer2->ResetCloudSyncToZero();
    int64_t begin = 11;
    InsertCloudTableRecord(begin, actualCount, actualCount, false);
    BlockSync(query, delegate_);

    /**
     * @tc.steps:step5. check observer.
     * @tc.expected: step5. ok.
     */
    EXPECT_EQ(observer1->GetCloudCallCount(), 2u); // 2 is observer1 triggered times
    EXPECT_EQ(observer2->GetCloudCallCount(), 0u);

    delete observer1;
    observer1 = nullptr;
    EXPECT_EQ(mgr_->CloseStore(delegate1), DBStatus::OK);

    delete observer2;
    observer2 = nullptr;
    EXPECT_EQ(mgr_->CloseStore(delegate2), DBStatus::OK);
}

/**
 * @tc.name: LogicDeleteSyncTest001
 * @tc.desc: sync with logic delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, LogicDeleteSyncTest001, TestSize.Level0)
{
    bool logicDelete = true;
    auto data = static_cast<PragmaData>(&logicDelete);
    delegate_->Pragma(LOGIC_DELETE_SYNC_DATA, data);
    int actualCount = 10;
    InitLogicDeleteDataEnv(actualCount);
    CheckLocalCount(actualCount);
    std::string device = "";
    ASSERT_EQ(delegate_->RemoveDeviceData(device, DistributedDB::FLAG_AND_DATA), DBStatus::OK);
    CheckLocalCount(0);
}

/**
 * @tc.name: LogicDeleteSyncTest002
 * @tc.desc: sync without logic delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, LogicDeleteSyncTest002, TestSize.Level0)
{
    bool logicDelete = false;
    auto data = static_cast<PragmaData>(&logicDelete);
    delegate_->Pragma(LOGIC_DELETE_SYNC_DATA, data);
    int actualCount = 10;
    InitLogicDeleteDataEnv(actualCount);
    CheckLocalCount(0);
}

/**
 * @tc.name: LogicDeleteSyncTest003
 * @tc.desc: sync with logic delete and check observer
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, LogicDeleteSyncTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. register observer.
     * @tc.expected: step1. ok.
     */
    RelationalStoreDelegate::Option option;
    auto observer = new (std::nothrow) RelationalStoreObserverUnitTest();
    ASSERT_NE(observer, nullptr);
    observer->SetCallbackDetailsType(static_cast<uint32_t>(CallbackDetailsType::DETAILED));
    EXPECT_EQ(delegate_->RegisterObserver(observer), OK);
    ChangedData expectData;
    expectData.tableName = tableName_;
    expectData.type = ChangedDataType::DATA;
    expectData.field.push_back(std::string("id"));
    const int count = 10;
    for (int64_t i = 0; i < count; ++i) {
        expectData.primaryData[ChangeType::OP_DELETE].push_back({std::to_string(i)});
    }
    expectData.properties = { .isTrackedDataChange = true };
    observer->SetExpectedResult(expectData);

    /**
     * @tc.steps:step2. set logic delete and sync
     * @tc.expected: step2. ok.
     */
    bool logicDelete = true;
    auto data = static_cast<PragmaData>(&logicDelete);
    delegate_->Pragma(LOGIC_DELETE_SYNC_DATA, data);
    int actualCount = 10;
    InitLogicDeleteDataEnv(actualCount);
    CheckLocalCount(actualCount);
    EXPECT_EQ(observer->IsAllChangedDataEq(), true);

    EXPECT_EQ(delegate_->UnRegisterObserver(observer), OK);
    delete observer;
    observer = nullptr;
}
}
#endif