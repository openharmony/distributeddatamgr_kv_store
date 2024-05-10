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
#include "cloud_db_sync_utils_test.h"
#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "log_print.h"
#include "relational_store_delegate.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "relational_sync_able_storage.h"
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
const char *g_createNonPrimaryKeySQL =
    "CREATE TABLE IF NOT EXISTS NonPrimaryKeyTable(" \
    "id TEXT," \
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
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, g_createNonPrimaryKeySQL), SQLITE_OK);
}

void PrepareOption(CloudSyncOption &option, const Query &query, bool isPriorityTask, bool isCompensatedSyncOnly = false)
{
    option.devices = { "CLOUD" };
    option.mode = SYNC_MODE_CLOUD_MERGE;
    option.query = query;
    option.waitTime = g_syncWaitTime;
    option.priorityTask = isPriorityTask;
    option.compensatedSyncOnly = isCompensatedSyncOnly;
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

void BlockPrioritySync(const Query &query, RelationalStoreDelegate *delegate, bool isPriority, DBStatus expectResult,
    bool isCompensatedSyncOnly = false)
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
    CloudSyncOption option;
    PrepareOption(option, query, isPriority, isCompensatedSyncOnly);
    ASSERT_EQ(delegate->Sync(option, callback), expectResult);
    if (expectResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }
}

int QueryCountCallback(void *data, int count, char **colValue, char **colName)
{
    if (count != 1) {
        return 0;
    }
    auto expectCount = reinterpret_cast<int64_t>(data);
    EXPECT_EQ(strtol(colValue[0], nullptr, 10), expectCount); // 10: decimal
    return 0;
}

void CheckUserTableResult(sqlite3 *&db, const std::string &tableName, int64_t expectCount)
{
    string query = "select count(*) from " + tableName + ";";
    EXPECT_EQ(sqlite3_exec(db, query.c_str(), QueryCountCallback,
        reinterpret_cast<void *>(expectCount), nullptr), SQLITE_OK);
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
    void InsertUserTableRecord(const std::string &tableName, int64_t recordCounts, int64_t begin = 0);
    void InsertCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize, bool assetIsNull);
    void InsertCloudTableRecord(const std::string &tableName, int64_t begin, int64_t count, int64_t photoSize,
        bool assetIsNull);
    void DeleteUserTableRecord(int64_t id);
    void DeleteCloudTableRecord(int64_t gid);
    void CheckCloudTableCount(const std::string &tableName, int64_t expectCount);
    void PriorityAndNormalSync(const Query &normalQuery, const Query &priorityQuery,
        RelationalStoreDelegate *delegate);
    void DeleteCloudDBData(int64_t begin, int64_t count);
    void SetForkQueryForCloudPrioritySyncTest007(std::atomic<int> &count);
    void SetForkQueryForCloudPrioritySyncTest008(std::atomic<int> &count);
    void InitLogicDeleteDataEnv(int64_t dataCount);
    void CheckLocalCount(int64_t expectCount);
    void CheckLogCleaned(int64_t expectCount);
    void SyncDataStatusTest(bool isCompensatedSyncOnly);
    std::string testDir_;
    std::string storePath_;
    sqlite3 *db_ = nullptr;
    RelationalStoreDelegate *delegate_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<VirtualAssetLoader> virtualAssetLoader_ = nullptr;
    std::shared_ptr<RelationalStoreManager> mgr_ = nullptr;
    std::string tableName_ = "DistributedDBCloudCheckSyncTest";
    std::string tableNameShared_ = "DistributedDBCloudCheckSyncTest_shared";
    std::string tableWithoutPrimaryName_ = "NonPrimaryKeyTable";
    std::string tableWithoutPrimaryNameShared_ = "NonPrimaryKeyTable_shared";
    std::string lowerTableName_ = "distributeddbCloudCheckSyncTest";
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
    ASSERT_EQ(delegate_->CreateDistributedTable(tableWithoutPrimaryName_, CLOUD_COOPERATION), DBStatus::OK);
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    virtualAssetLoader_ = std::make_shared<VirtualAssetLoader>();
    ASSERT_EQ(delegate_->SetCloudDB(virtualCloudDb_), DBStatus::OK);
    ASSERT_EQ(delegate_->SetIAssetLoader(virtualAssetLoader_), DBStatus::OK);
    DataBaseSchema dataBaseSchema = GetSchema();
    ASSERT_EQ(delegate_->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
}

void DistributedDBCloudCheckSyncTest::TearDown()
{
    virtualCloudDb_->ForkQuery(nullptr);
    virtualCloudDb_->SetCloudError(false);
    CloseDb();
    EXPECT_EQ(sqlite3_close_v2(db_), SQLITE_OK);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(testDir_) != E_OK) {
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
    tableSchema.sharedTableName = tableName_ + "_shared";
    tableSchema.fields = {
        {"id", TYPE_INDEX<std::string>, true}, {"name", TYPE_INDEX<std::string>}, {"height", TYPE_INDEX<double>},
        {"photo", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>}
    };
    TableSchema tableWithoutPrimaryKeySchema;
    tableWithoutPrimaryKeySchema.name = tableWithoutPrimaryName_;
    tableWithoutPrimaryKeySchema.sharedTableName = tableWithoutPrimaryNameShared_;
    tableWithoutPrimaryKeySchema.fields = {
        {"id", TYPE_INDEX<std::string>}, {"name", TYPE_INDEX<std::string>}, {"height", TYPE_INDEX<double>},
        {"photo", TYPE_INDEX<Bytes>}, {"age", TYPE_INDEX<int64_t>}
    };
    schema.tables.push_back(tableSchema);
    schema.tables.push_back(tableWithoutPrimaryKeySchema);
    return schema;
}

void DistributedDBCloudCheckSyncTest::CloseDb()
{
    virtualCloudDb_ = nullptr;
    if (mgr_ != nullptr) {
        EXPECT_EQ(mgr_->CloseStore(delegate_), DBStatus::OK);
        delegate_ = nullptr;
        mgr_ = nullptr;
    }
}

void DistributedDBCloudCheckSyncTest::InsertUserTableRecord(const std::string &tableName,
    int64_t recordCounts, int64_t begin)
{
    ASSERT_NE(db_, nullptr);
    for (int64_t i = begin; i < begin + recordCounts; ++i) {
        string sql = "INSERT OR REPLACE INTO " + tableName
            + " (id, name, height, photo, age) VALUES ('" + std::to_string(i) + "', 'Local"
            + std::to_string(i) + "', '155.10',  'text', '21');";
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    }
}

void DistributedDBCloudCheckSyncTest::InsertCloudTableRecord(int64_t begin, int64_t count, int64_t photoSize,
    bool assetIsNull)
{
    InsertCloudTableRecord(tableName_, begin, count, photoSize, assetIsNull);
}

void DistributedDBCloudCheckSyncTest::InsertCloudTableRecord(const std::string &tableName, int64_t begin, int64_t count,
    int64_t photoSize, bool assetIsNull)
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
    ASSERT_EQ(virtualCloudDb_->BatchInsert(tableName, std::move(record1), extend1), DBStatus::OK);
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

void DistributedDBCloudCheckSyncTest::CheckCloudTableCount(const std::string &tableName, int64_t expectCount)
{
    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(0);
    int64_t realCount = 0;
    std::vector<VBucket> data;
    virtualCloudDb_->Query(tableName, extend, data);
    for (size_t j = 0; j < data.size(); ++j) {
        auto entry = data[j].find(CloudDbConstant::DELETE_FIELD);
        if (entry != data[j].end() && std::get<bool>(entry->second)) {
            continue;
        }
        realCount++;
    }
    EXPECT_EQ(realCount, expectCount); // ExpectCount represents the total amount of cloud data.
}

void DistributedDBCloudCheckSyncTest::PriorityAndNormalSync(const Query &normalQuery, const Query &priorityQuery,
    RelationalStoreDelegate *delegate)
{
    std::mutex dataMutex;
    std::condition_variable cv;
    bool normalFinish = false;
    bool priorityFinish = false;
    auto normalCallback = [&cv, &dataMutex, &normalFinish, &priorityFinish](
        const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    normalFinish = true;
                }
                ASSERT_EQ(priorityFinish, true);
                cv.notify_one();
            }
        }
    };
    auto priorityCallback = [&priorityFinish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                priorityFinish = true;
            }
        }
    };
    CloudSyncOption option;
    PrepareOption(option, normalQuery, false);
    virtualCloudDb_->SetBlockTime(500); // 500 ms
    ASSERT_EQ(delegate->Sync(option, normalCallback), OK);
    PrepareOption(option, priorityQuery, true);
    ASSERT_EQ(delegate->Sync(option, priorityCallback), OK);
    std::unique_lock<std::mutex> uniqueLock(dataMutex);
    cv.wait(uniqueLock, [&normalFinish]() {
        return normalFinish;
    });
}

void DistributedDBCloudCheckSyncTest::DeleteCloudDBData(int64_t begin, int64_t count)
{
    for (int64_t i = begin; i < begin + count; i++) {
        VBucket idMap;
        idMap.insert_or_assign("#_gid", std::to_string(i));
        ASSERT_EQ(virtualCloudDb_->DeleteByGid(tableName_, idMap), DBStatus::OK);
    }
}

void DistributedDBCloudCheckSyncTest::SetForkQueryForCloudPrioritySyncTest007(std::atomic<int> &count)
{
    virtualCloudDb_->ForkQuery([this, &count](const std::string &, VBucket &) {
        count++;
        if (count == 1) { // taskid1
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (count == 3) { // 3 means taskid3 because CheckCloudTableCount will query then count++
            CheckCloudTableCount(tableName_, 1); // 1 is count of cloud records after last sync
        }
        if (count == 6) { // 6 means taskid2 because CheckCloudTableCount will query then count++
            CheckCloudTableCount(tableName_, 2); // 2 is count of cloud records after last sync
        }
        if (count == 9) { // 9 means taskid4 because CheckCloudTableCount will query then count++
            CheckCloudTableCount(tableName_, 10); // 10 is count of cloud records after last sync
        }
    });
}

void DistributedDBCloudCheckSyncTest::SetForkQueryForCloudPrioritySyncTest008(std::atomic<int> &count)
{
    virtualCloudDb_->ForkQuery([this, &count](const std::string &, VBucket &) {
        count++;
        if (count == 1) { // taskid1
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (count == 3) { // 3 means taskid3 because CheckCloudTableCount will query then count++
            CheckCloudTableCount(tableName_, 1); // 1 is count of cloud records after last sync
        }
        if (count == 6) { // 6 means taskid2 because CheckCloudTableCount will query then count++
            CheckCloudTableCount(tableName_, 1); // 1 is count of cloud records after last sync
        }
        if (count == 9) { // 9 means taskid4 because CheckCloudTableCount will query then count++
            CheckCloudTableCount(tableName_, 10); // 10 is count of cloud records after last sync
        }
    });
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

void DistributedDBCloudCheckSyncTest::CheckLogCleaned(int64_t expectCount)
{
    std::string sql1 = "select count(*) from " + DBCommon::GetLogTableName(tableName_) +
        " where device = 'cloud';";
    EXPECT_EQ(sqlite3_exec(db_, sql1.c_str(), QueryCountCallback,
        reinterpret_cast<void *>(expectCount), nullptr), SQLITE_OK);
    std::string sql2 = "select count(*) from " + DBCommon::GetLogTableName(tableName_) + " where cloud_gid "
        " is not null and cloud_gid != '';";
    EXPECT_EQ(sqlite3_exec(db_, sql2.c_str(), QueryCountCallback,
        reinterpret_cast<void *>(expectCount), nullptr), SQLITE_OK);
    std::string sql3 = "select count(*) from " + DBCommon::GetLogTableName(tableName_) +
        " where flag & 0x02 = 0;";
    EXPECT_EQ(sqlite3_exec(db_, sql3.c_str(), QueryCountCallback,
        reinterpret_cast<void *>(expectCount), nullptr), SQLITE_OK);
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
    InsertUserTableRecord(tableName_, actualCount);
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
    InsertUserTableRecord(tableName_, actualCount);
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
}

/**
 * @tc.name: CloudSyncTest003
 * @tc.desc: local data is delete before sync, then sync, cloud data will insert into local
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudSyncTest003, TestSize.Level0)
{
    // prepare data
    const int actualCount = 1;
    InsertUserTableRecord(tableName_, actualCount);

    InsertCloudTableRecord(0, actualCount, 0, false);
    // delete local data
    DeleteUserTableRecord(0);
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);

    // check local data, cloud date will insert into local
    int dataCnt = -1;
    std::string checkLogSql = "SELECT count(*) FROM " + tableName_;
    RelationalTestUtils::ExecSql(db_, checkLogSql, nullptr, [&dataCnt](sqlite3_stmt *stmt) {
        dataCnt = sqlite3_column_int(stmt, 0);
        return E_OK;
    });
    EXPECT_EQ(dataCnt, actualCount);
}

/**
 * @tc.name: CloudSyncTest004
 * @tc.desc: sync after insert failed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudSyncTest004, TestSize.Level0)
{
    // prepare data
    const int actualCount = 1;
    InsertUserTableRecord(tableName_, actualCount);
    // sync twice
    Query query = Query::Select().FromTable({ tableName_ });
    LOGW("Block Sync");
    virtualCloudDb_->SetInsertFailed(1);
    BlockSync(query, delegate_);
    // delete local data
    DeleteUserTableRecord(0); // 0 is id
    LOGW("Block Sync");
    // sync again and this record with be synced to cloud
    BlockSync(query, delegate_);
    bool deleteStatus = true;
    EXPECT_EQ(virtualCloudDb_->GetDataStatus("0", deleteStatus), OK);
    EXPECT_EQ(deleteStatus, true);
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
    InsertUserTableRecord(tableName_, actualCount);

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
 * @tc.name: CloudPrioritySyncTest001
 * @tc.desc: use priority sync interface when query in or from table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record and query in 3 records, then priority sync.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableName_, actualCount);
    std::vector<std::string> idValue = {"0", "1", "2"};
    Query query = Query::Select().From(tableName_).In("id", idValue);

    /**
     * @tc.steps:step2. check ParserQueryNodes
     * @tc.expected: step2. ok.
     */
    virtualCloudDb_->ForkQuery([this, &idValue](const std::string &tableName, VBucket &extend) {
        EXPECT_EQ(tableName_, tableName);
        if (extend.find(CloudDbConstant::QUERY_FIELD) == extend.end()) {
            return;
        }
        Bytes bytes = std::get<Bytes>(extend[CloudDbConstant::QUERY_FIELD]);
        DBStatus status = OK;
        auto queryNodes = RelationalStoreManager::ParserQueryNodes(bytes, status);
        EXPECT_EQ(status, OK);
        ASSERT_EQ(queryNodes.size(), 1u);
        EXPECT_EQ(queryNodes[0].type, QueryNodeType::IN);
        EXPECT_EQ(queryNodes[0].fieldName, "id");
        ASSERT_EQ(queryNodes[0].fieldValue.size(), idValue.size());
        for (size_t i = 0u; i < idValue.size(); i++) {
            std::string val = std::get<std::string>(queryNodes[0].fieldValue[i]);
            EXPECT_EQ(val, idValue[i]);
        }
    });
    BlockPrioritySync(query, delegate_, true, OK);
    virtualCloudDb_->ForkQuery(nullptr);
    CheckCloudTableCount(tableName_, 3); // 3 is count of cloud records

    /**
     * @tc.steps:step3. use priority sync interface but not priority.
     * @tc.expected: step3. ok.
     */
    query = Query::Select().FromTable({ tableName_ });
    BlockPrioritySync(query, delegate_, false, OK);
    CheckCloudTableCount(tableName_, 10); // 10 is count of cloud records

    /**
     * @tc.steps:step4. insert user table record and query from table, then priority sync.
     * @tc.expected: step4. ok.
     */
    InsertUserTableRecord(tableName_, actualCount, actualCount);
    BlockPrioritySync(query, delegate_, true, OK);
    CheckCloudTableCount(tableName_, 20); // 20 is count of cloud records
}


/**
 * @tc.name: CloudPrioritySyncTest002
 * @tc.desc: priority sync in some abnormal query situations
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 1; // 1 is count of records
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. query select tablename then priority sync.
     * @tc.expected: step2. invalid.
     */
    Query query = Query::Select(tableName_);
    BlockPrioritySync(query, delegate_, true, INVALID_ARGS);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step3. query select without from then priority sync.
     * @tc.expected: step3. invalid.
     */
    query = Query::Select();
    BlockPrioritySync(query, delegate_, true, INVALID_ARGS);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step4. query select and from without in then priority sync.
     * @tc.expected: step4. invalid.
     */
    query = Query::Select().From(tableName_);
    BlockPrioritySync(query, delegate_, true, INVALID_ARGS);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step5. query select and fromtable then priority sync.
     * @tc.expected: step5. not support.
     */
    query = Query::Select().From(tableName_).FromTable({tableName_});
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step6. query select and from with other predicates then priority sync.
     * @tc.expected: step6. not support.
     */
    query = Query::Select().From(tableName_).IsNotNull("id");
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step7. query select and from with in and other predicates then priority sync.
     * @tc.expected: step7 not support.
     */
    std::vector<std::string> idValue = {"0"};
    query = Query::Select().From(tableName_).IsNotNull("id").In("id", idValue);
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step8. query select and from with in non-primary key then priority sync.
     * @tc.expected: step8. not support.
     */
    std::vector<std::string> heightValue = {"155.10"};
    query = Query::Select().From(tableName_).In("height", heightValue);
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step9. query in count greater than 100.
     * @tc.expected: step9. over max limits.
     */
    idValue.resize(101); // 101 > 100
    query = Query::Select().From(tableName_).In("id", idValue);
    BlockPrioritySync(query, delegate_, true, OVER_MAX_LIMITS);
    CheckCloudTableCount(tableName_, 0);
}

/**
 * @tc.name: CloudPrioritySyncTest003
 * @tc.desc: priority sync when normal syncing
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. begin normal sync and priority sync.
     * @tc.expected: step2. ok.
     */
    Query normalQuery = Query::Select().FromTable({tableName_});
    std::vector<std::string> idValue = {"0", "1", "2"};
    Query priorityQuery = Query::Select().From(tableName_).In("id", idValue);
    PriorityAndNormalSync(normalQuery, priorityQuery, delegate_);
    CheckCloudTableCount(tableName_, 10); // 10 is count of cloud records
    EXPECT_EQ(virtualCloudDb_->GetLockCount(), 2);
    virtualCloudDb_->Reset();
    EXPECT_EQ(virtualCloudDb_->GetLockCount(), 0);
}

/**
 * @tc.name: CloudPrioritySyncTest004
 * @tc.desc: non-primarykey table priority sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user non-primarykey table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableWithoutPrimaryName_, actualCount);

    /**
     * @tc.steps:step2. begin priority sync.
     * @tc.expected: step2. not support.
     */
    std::vector<std::string> idValue = {"0", "1", "2"};
    Query query = Query::Select().From(tableWithoutPrimaryName_).In("id", idValue);
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableWithoutPrimaryName_, 0);

    /**
     * @tc.steps:step3. begin priority sync when in rowid.
     * @tc.expected: step3. invalid.
     */
    std::vector<int64_t> rowidValue = {0, 1, 2}; // 0,1,2 are rowid value
    query = Query::Select().From(tableWithoutPrimaryName_).In("rowid", rowidValue);
    BlockPrioritySync(query, delegate_, true, INVALID_ARGS);
    CheckCloudTableCount(tableWithoutPrimaryName_, 0);
}

/**
 * @tc.name: CloudPrioritySyncTest005
 * @tc.desc: priority sync but don't have records
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user non-primarykey table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableWithoutPrimaryName_, actualCount);

    /**
     * @tc.steps:step2. begin DistributedDBCloudCheckSyncTest priority sync and check records.
     * @tc.expected: step2. ok.
     */
    std::vector<std::string> idValue = {"0", "1", "2"};
    Query query = Query::Select().From(tableName_).In("id", idValue);
    BlockPrioritySync(query, delegate_, true, OK);
    CheckCloudTableCount(tableWithoutPrimaryName_, 0);
    CheckCloudTableCount(tableName_, 0);
}

/**
 * @tc.name: CloudPrioritySyncTest006
 * @tc.desc: priority sync tasks greater than limit
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. begin 32 priority sync tasks and then begin 1 priority sync task.
     * @tc.expected: step2. ok.
     */
    std::vector<std::string> idValue = {"0", "1", "2"};
    Query query = Query::Select().From(tableName_).In("id", idValue);
    std::mutex dataMutex;
    std::condition_variable cv;
    std::mutex callbackMutex;
    std::condition_variable callbackCv;
    bool finish = false;
    size_t finishCount = 0u;
    virtualCloudDb_->ForkQuery([&cv, &finish, &dataMutex](const std::string &tableName, VBucket &extend) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    });
    auto callback = [&callbackCv, &callbackMutex, &finishCount](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> callbackAutoLock(callbackMutex);
                    finishCount++;
                }
                callbackCv.notify_one();
            }
        }
    };
    CloudSyncOption option;
    PrepareOption(option, query, true);
    for (int i = 0; i < 32; i++) { // 32 is count of sync tasks
        ASSERT_EQ(delegate_->Sync(option, callback), OK);
    }
    ASSERT_EQ(delegate_->Sync(option, nullptr), BUSY);
    {
        std::lock_guard<std::mutex> autoLock(dataMutex);
        finish = true;
    }
    cv.notify_all();
    virtualCloudDb_->ForkQuery(nullptr);
    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    callbackCv.wait(callbackLock, [&finishCount]() {
        return (finishCount == 32u); // 32 is count of finished sync tasks
    });
    CheckCloudTableCount(tableName_, 3); // 3 is count of cloud records
}

/**
 * @tc.name: CloudPrioritySyncTest007
 * @tc.desc: priority normal priority normal when different query
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. set callback to check during sync.
     * @tc.expected: step2. ok.
     */
    std::atomic<int> count = 0;
    SetForkQueryForCloudPrioritySyncTest007(count);

    /**
     * @tc.steps:step3. perform priority normal priority normal sync.
     * @tc.expected: step3. ok.
     */
    std::vector<std::string> idValue = {"0"};
    Query priorytyQuery = Query::Select().From(tableName_).In("id", idValue);
    CloudSyncOption option;
    PrepareOption(option, priorytyQuery, true);
    std::mutex callbackMutex;
    std::condition_variable callbackCv;
    size_t finishCount = 0u;
    auto callback = [&callbackCv, &callbackMutex, &finishCount](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> callbackAutoLock(callbackMutex);
                    finishCount++;
                }
                callbackCv.notify_one();
            }
        }
    };
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    Query normalQuery = Query::Select().FromTable({tableName_});
    PrepareOption(option, normalQuery, false);
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    idValue = {"1"};
    priorytyQuery = Query::Select().From(tableName_).In("id", idValue);
    PrepareOption(option, priorytyQuery, true);
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    PrepareOption(option, normalQuery, false);
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    callbackCv.wait(callbackLock, [&finishCount]() {
        return (finishCount == 4u); // 4 is count of finished sync tasks
    });
    CheckCloudTableCount(tableName_, 10); // 10 is count of cloud records
}

/**
 * @tc.name: CloudPrioritySyncTest008
 * @tc.desc: priority normal priority normal when different query
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. set callback to check during sync.
     * @tc.expected: step2. ok.
     */
    std::atomic<int> count = 0;
    SetForkQueryForCloudPrioritySyncTest008(count);

    /**
     * @tc.steps:step3. perform priority normal priority normal sync.
     * @tc.expected: step3. ok.
     */
    std::vector<std::string> idValue = {"0"};
    Query priorytyQuery = Query::Select().From(tableName_).In("id", idValue);
    CloudSyncOption option;
    PrepareOption(option, priorytyQuery, true);
    std::mutex callbackMutex;
    std::condition_variable callbackCv;
    size_t finishCount = 0u;
    auto callback = [&callbackCv, &callbackMutex, &finishCount](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> callbackAutoLock(callbackMutex);
                    finishCount++;
                }
                callbackCv.notify_one();
            }
        }
    };
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    Query normalQuery = Query::Select().FromTable({tableName_});
    PrepareOption(option, normalQuery, false);
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    priorytyQuery = Query::Select().From(tableName_).In("id", idValue);
    PrepareOption(option, priorytyQuery, true);
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    PrepareOption(option, normalQuery, false);
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    callbackCv.wait(callbackLock, [&finishCount]() {
        return (finishCount == 4u); // 4 is count of finished sync tasks
    });
    CheckCloudTableCount(tableName_, 10); // 10 is count of cloud records
}

/**
 * @tc.name: CloudPrioritySyncTest009
 * @tc.desc: use priority sync interface when query equal to from table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest009, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record and query in 3 records, then priority sync.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 5; // 5 is count of records
    InsertUserTableRecord(tableName_, actualCount);
    Query query = Query::Select().From(tableName_).BeginGroup().EqualTo("id", "0").Or().EqualTo("id", "1").EndGroup();

    /**
     * @tc.steps:step2. check ParserQueryNodes
     * @tc.expected: step2. ok.
     */
    virtualCloudDb_->ForkQuery([this](const std::string &tableName, VBucket &extend) {
        EXPECT_EQ(tableName_, tableName);
        Bytes bytes = std::get<Bytes>(extend[CloudDbConstant::QUERY_FIELD]);
        DBStatus status = OK;
        auto queryNodes = RelationalStoreManager::ParserQueryNodes(bytes, status);
        EXPECT_EQ(status, OK);
        ASSERT_EQ(queryNodes.size(), 5u); // 5 is query nodes count
    });
    BlockPrioritySync(query, delegate_, true, OK);
    virtualCloudDb_->ForkQuery(nullptr);
    CheckCloudTableCount(tableName_, 2); // 2 is count of cloud records
}

/**
 * @tc.name: CloudPrioritySyncTest010
 * @tc.desc: priority sync after cloud delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest010, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. normal sync and then delete cloud records.
     * @tc.expected: step2. ok.
     */
    Query query = Query::Select().FromTable({tableName_});
    BlockSync(query, delegate_);
    CheckCloudTableCount(tableName_, 10); // 10 is count of cloud records after sync
    DeleteCloudDBData(0, 3); // delete 0 1 2 record in cloud
    CheckCloudTableCount(tableName_, 7); // 7 is count of cloud records after delete
    CheckUserTableResult(db_, tableName_, 10); // 10 is count of user records

    /**
     * @tc.steps:step3. priory sync and set query then check user table records.
     * @tc.expected: step3. ok.
     */
    std::vector<std::string> idValue = {"3", "4", "5"};
    query = Query::Select().From(tableName_).In("id", idValue);
    BlockPrioritySync(query, delegate_, true, OK);
    CheckUserTableResult(db_, tableName_, 10); // 10 is count of user records after sync
    idValue = {"0", "1", "2"};
    query = Query::Select().From(tableName_).In("id", idValue);
    BlockPrioritySync(query, delegate_, true, OK);
    CheckUserTableResult(db_, tableName_, 7); // 7 is count of user records after sync
}

/**
 * @tc.name: CloudPrioritySyncTest011
 * @tc.desc: priority sync after cloud insert
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest011, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertCloudTableRecord(0, actualCount, actualCount, false);
    std::vector<std::string> idValue = {"0", "1", "2"};
    Query query = Query::Select().From(tableName_).In("id", idValue);
    std::atomic<int> count = 0;

    /**
     * @tc.steps:step2. check user records when query.
     * @tc.expected: step1. ok.
     */
    virtualCloudDb_->ForkQuery([this, &count](const std::string &, VBucket &) {
        count++;
        if (count == 1) { // taskid1
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        if (count == 2) { // taskid2
            CheckUserTableResult(db_, tableName_, 3); // 3 is count of user records after first sync
        }
    });
    CloudSyncOption option;
    PrepareOption(option, query, true);
    std::mutex callbackMutex;
    std::condition_variable callbackCv;
    size_t finishCount = 0u;
    auto callback = [&callbackCv, &callbackMutex, &finishCount](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> callbackAutoLock(callbackMutex);
                    finishCount++;
                }
                callbackCv.notify_one();
            }
        }
    };

    /**
     * @tc.steps:step3. begin sync and check user record.
     * @tc.expected: step3. ok.
     */
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    idValue = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    query = Query::Select().From(tableName_).In("id", idValue);
    PrepareOption(option, query, true);
    ASSERT_EQ(delegate_->Sync(option, callback), OK);
    std::unique_lock<std::mutex> callbackLock(callbackMutex);
    callbackCv.wait(callbackLock, [&finishCount]() {
        return (finishCount == 2u); // 2 is count of finished sync tasks
    });
    CheckUserTableResult(db_, tableName_, 10); // 10 is count of user records
}

/**
 * @tc.name: CloudPrioritySyncTest012
 * @tc.desc: priority or normal sync when waittime > 300s or < -1
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest012, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertCloudTableRecord(0, actualCount, actualCount, false);
    std::vector<std::string> idValue = {"0", "1", "2"};
    Query query = Query::Select().From(tableName_).In("id", idValue);

    /**
     * @tc.steps:step2. set waittime < -1 then begin sync.
     * @tc.expected: step2. invalid.
     */
    CloudSyncOption option;
    PrepareOption(option, query, true);
    option.waitTime = -2; // -2 < -1;
    ASSERT_EQ(delegate_->Sync(option, nullptr), INVALID_ARGS);
    CheckUserTableResult(db_, tableName_, 0); // 0 is count of user records

    /**
     * @tc.steps:step3. set waittime > 300s then begin sync.
     * @tc.expected: step3. invalid.
     */

    option.waitTime = 300001; // 300001 > 300s
    ASSERT_EQ(delegate_->Sync(option, nullptr), INVALID_ARGS);
    CheckUserTableResult(db_, tableName_, 0); // 0 is count of user records
}

/**
 * @tc.name: CloudPrioritySyncTest013
 * @tc.desc: priority sync in some abnormal composite pk query situations
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloudPrioritySyncTest013, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert user table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 1; // 1 is count of records
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. query only begingroup then priority sync.
     * @tc.expected: step2. invalid.
     */
    Query query = Query::Select().From(tableName_).BeginGroup();
    BlockPrioritySync(query, delegate_, true, INVALID_ARGS);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step3. query only endgroup then priority sync.
     * @tc.expected: step3. invalid.
     */
    query = Query::Select().From(tableName_).EndGroup();
    BlockPrioritySync(query, delegate_, true, INVALID_ARGS);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step4. query only begingroup and endgroup then priority sync.
     * @tc.expected: step4. invalid.
     */
    query = Query::Select().From(tableName_).BeginGroup().EndGroup();
    BlockPrioritySync(query, delegate_, true, INVALID_ARGS);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step5. query and from table then priority sync.
     * @tc.expected: step5. invalid.
     */
    query = Query::Select().And().From(tableName_);
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step6. query or from table then priority sync.
     * @tc.expected: step6. invalid.
     */
    query = Query::Select().Or().From(tableName_);
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step7. query begingroup from table then priority sync.
     * @tc.expected: step7 invalid.
     */
    query = Query::Select().BeginGroup().From(tableName_);
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step8. query endgroup from table then priority sync.
     * @tc.expected: step8 invalid.
     */
    query = Query::Select().EndGroup().From(tableName_);
    BlockPrioritySync(query, delegate_, true, NOT_SUPPORT);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step9. query and in then priority sync.
     * @tc.expected: step9. invalid.
     */
    std::vector<std::string> idValue = {"0"};
    query = Query::Select().From(tableName_).And().In("id", idValue);
    BlockPrioritySync(query, delegate_, true, INVALID_ARGS);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step10. query when the table name does not exit then priority sync.
     * @tc.expected: step10. schema mismatch.
     */
    query = Query::Select().From("tableName").And().In("id", idValue);
    BlockPrioritySync(query, delegate_, true, SCHEMA_MISMATCH);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step11. query when the table name does not exit then priority sync.
     * @tc.expected: step11. schema mismatch.
     */
    query = Query::Select().From("tableName").In("id", idValue);
    BlockPrioritySync(query, delegate_, true, SCHEMA_MISMATCH);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step12. query when the table name does not exit then sync.
     * @tc.expected: step12. schema mismatch.
     */
    query = Query::Select().FromTable({"tableName"});
    BlockPrioritySync(query, delegate_, false, SCHEMA_MISMATCH);
    CheckCloudTableCount(tableName_, 0);
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
     * @tc.steps:step2. set tracker table
     * @tc.expected: step2. ok.
     */
    TrackerSchema trackerSchema;
    trackerSchema.tableName = tableName_;
    trackerSchema.trackerColNames = { "id" };
    EXPECT_EQ(delegate_->SetTrackerTable(trackerSchema), OK);

    /**
     * @tc.steps:step3. set logic delete and sync
     * @tc.expected: step3. ok.
     */
    bool logicDelete = true;
    auto data = static_cast<PragmaData>(&logicDelete);
    delegate_->Pragma(LOGIC_DELETE_SYNC_DATA, data);
    int actualCount = 10;
    InitLogicDeleteDataEnv(actualCount);
    CheckLocalCount(actualCount);
    EXPECT_EQ(observer->IsAllChangedDataEq(), true);
    observer->ClearChangedData();

    /**
     * @tc.steps:step4. unSetTrackerTable and sync
     * @tc.expected: step4. ok.
     */
    expectData.properties = { .isTrackedDataChange = false };
    observer->SetExpectedResult(expectData);
    trackerSchema.trackerColNames = {};
    EXPECT_EQ(delegate_->SetTrackerTable(trackerSchema), OK);
    InsertUserTableRecord(tableName_, actualCount);
    BlockSync(Query::Select().FromTable({ tableName_ }), delegate_);
    for (int i = 0; i < actualCount + actualCount; ++i) {
        DeleteCloudTableRecord(i);
    }
    BlockSync(Query::Select().FromTable({ tableName_ }), delegate_);
    EXPECT_EQ(observer->IsAllChangedDataEq(), true);

    EXPECT_EQ(delegate_->UnRegisterObserver(observer), OK);
    delete observer;
    observer = nullptr;
}

/**
 * @tc.name: LogicDeleteSyncTest004
 * @tc.desc: test removedevicedata in mode FLAG_ONLY when sync with logic delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, LogicDeleteSyncTest004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. set logic delete
     * @tc.expected: step1. ok.
     */
    bool logicDelete = true;
    auto data = static_cast<PragmaData>(&logicDelete);
    delegate_->Pragma(LOGIC_DELETE_SYNC_DATA, data);

    /**
     * @tc.steps:step2. cloud delete data then sync, check removedevicedata
     * @tc.expected: step2. ok.
     */
    int actualCount = 10;
    InitLogicDeleteDataEnv(actualCount);
    CheckLocalCount(actualCount);
    std::string device = "";
    ASSERT_EQ(delegate_->RemoveDeviceData(device, DistributedDB::FLAG_ONLY), DBStatus::OK);
    CheckLocalCount(actualCount);
    CheckLogCleaned(0);
}

/**
 * @tc.name: LogicDeleteSyncTest005
 * @tc.desc: test pragma when set cmd is not logic delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, LogicDeleteSyncTest005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. set cmd is auto sync
     * @tc.expected: step1. ok.
     */
    bool logicDelete = true;
    auto data = static_cast<PragmaData>(&logicDelete);
    EXPECT_EQ(delegate_->Pragma(AUTO_SYNC, data), DBStatus::NOT_SUPPORT);
}

/**
 * @tc.name: LogicCreateRepeatedTableNameTest001
 * @tc.desc: test create repeated table name with different cases
 * @tc.type: FUNC
 * @tc.require:DTS2023120705927
 * @tc.author: wangxiangdong
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, LogicCreateRepeatedTableNameTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. CreateDistributedTable with same name but different cases.
     * @tc.expected: step1. operate successfully.
     */
    DBStatus createStatus = delegate_->CreateDistributedTable(lowerTableName_, CLOUD_COOPERATION);
    ASSERT_EQ(createStatus, DBStatus::OK);
}

/**
 * @tc.name: SaveCursorTest001
 * @tc.desc: test whether cloud cursor is saved when first sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, SaveCursorTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud records
     * @tc.expected: step1. OK
     */
    const int actualCount = 10;
    InsertCloudTableRecord(0, actualCount, 0, false);

    /**
     * @tc.steps:step2. check cursor when first sync
     * @tc.expected: step2. OK
     */
    virtualCloudDb_->ForkQuery([this](const std::string &tableName, VBucket &extend) {
        EXPECT_EQ(tableName_, tableName);
        auto cursor = std::get<std::string>(extend[CloudDbConstant::CURSOR_FIELD]);
        EXPECT_EQ(cursor, "0");
    });
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);
    CheckLocalCount(actualCount);
}

/**
 * @tc.name: SaveCursorTest002
 * @tc.desc: test whether cloud cursor is saved when first download failed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, SaveCursorTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert cloud records
     * @tc.expected: step1. OK
     */
    const int actualCount = 10;
    InsertCloudTableRecord(0, actualCount, 0, false);

    /**
     * @tc.steps:step2. set download failed
     * @tc.expected: step2. OK
     */
    virtualCloudDb_->SetCloudError(true);
    Query query = Query::Select().FromTable({ tableName_ });
    BlockPrioritySync(query, delegate_, false, OK);
    CheckLocalCount(0);

    /**
     * @tc.steps:step3. check cursor when query
     * @tc.expected: step3. OK
     */
    virtualCloudDb_->SetCloudError(false);
    virtualCloudDb_->ForkQuery([this](const std::string &tableName, VBucket &extend) {
        EXPECT_EQ(tableName_, tableName);
        auto cursor = std::get<std::string>(extend[CloudDbConstant::CURSOR_FIELD]);
        EXPECT_EQ(cursor, "0");
    });
    BlockSync(query, delegate_);
    CheckLocalCount(actualCount);
}

/**
 * @tc.name: SaveCursorTest003
 * @tc.desc: test whether cloud cursor is saved when first upload failed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, SaveCursorTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert local records
     * @tc.expected: step1. OK
     */
    const int actualCount = 10;
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. set upload failed
     * @tc.expected: step2. OK
     */
    virtualCloudDb_->SetCloudError(true);
    Query query = Query::Select().FromTable({ tableName_ });
    BlockPrioritySync(query, delegate_, false, OK);
    CheckCloudTableCount(tableName_, 0);

    /**
     * @tc.steps:step3. check cursor when query
     * @tc.expected: step3. OK
     */
    virtualCloudDb_->SetCloudError(false);
    virtualCloudDb_->ForkQuery([this](const std::string &tableName, VBucket &extend) {
        EXPECT_EQ(tableName_, tableName);
        auto cursor = std::get<std::string>(extend[CloudDbConstant::CURSOR_FIELD]);
        EXPECT_EQ(cursor, "0");
    });
    BlockSync(query, delegate_);
    CheckCloudTableCount(tableName_, actualCount);
}

/*
 * @tc.name: CreateDistributedTable001
 * @tc.desc: Test create distributed table when table not empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CreateDistributedTable001, TestSize.Level0)
{
    const std::string table = "CreateDistributedTable001";
    const std::string createSQL =
        "CREATE TABLE IF NOT EXISTS " + table + "(" \
        "id TEXT PRIMARY KEY," \
        "name TEXT," \
        "height REAL ," \
        "photo BLOB," \
        "age INT);";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db_, createSQL), SQLITE_OK);
    int actualCount = 10;
    InsertUserTableRecord(table, actualCount);
    InsertCloudTableRecord(table, 0, actualCount, 0, true);
    ASSERT_EQ(delegate_->CreateDistributedTable(table, CLOUD_COOPERATION), DBStatus::OK);
    DataBaseSchema dataBaseSchema = GetSchema();
    TableSchema schema = dataBaseSchema.tables.at(0);
    schema.name = table;
    schema.sharedTableName = "";
    dataBaseSchema.tables.push_back(schema);
    ASSERT_EQ(delegate_->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    /**
     * @tc.steps:step2. call sync, local has one batch id:0-4
     * @tc.expected: step2. OK
     */
    Query query = Query::Select().FromTable({ table });
    BlockSync(query, delegate_);
    CheckCloudTableCount(table, actualCount);
}

/*
 * @tc.name: CloseDbTest001
 * @tc.desc: Test process of db close during sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, CloseDbTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. insert user table record.
     * @tc.expected: step1. ok.
     */
    const int actualCount = 10; // 10 is count of records
    InsertUserTableRecord(tableName_, actualCount);

    /**
     * @tc.steps:step2. wait for 2 seconds during the query to close the database.
     * @tc.expected: step2. ok.
     */
    std::mutex callMutex;
    int callCount = 0;
    virtualCloudDb_->ForkQuery([](const std::string &, VBucket &) {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // block notify 2s
    });
    const auto callback = [&callCount, &callMutex](
        const std::map<std::string, SyncProcess> &) {
        {
            std::lock_guard<std::mutex> autoLock(callMutex);
            callCount++;
        }
    };
    Query query = Query::Select().FromTable({ tableName_ });
    ASSERT_EQ(delegate_->Sync({ "CLOUD" }, SYNC_MODE_CLOUD_MERGE, query, callback, g_syncWaitTime), OK);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // block notify 1s
    EXPECT_EQ(mgr_->CloseStore(delegate_), DBStatus::OK);
    delegate_ = nullptr;
    mgr_ = nullptr;

    /**
     * @tc.steps:step3. wait for 2 seconds to check the process call count.
     * @tc.expected: step3. ok.
     */
    std::this_thread::sleep_for(std::chrono::seconds(2)); // block notify 2s
    EXPECT_EQ(callCount, 0L);
}

/*
 * @tc.name: ConsistentFlagTest001
 * @tc.desc: Test the consistency flag of no asset table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, ConsistentFlagTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data and sync
     * @tc.expected: step1. ok.
     */
    const int localCount = 20; // 20 is count of local
    const int cloudCount = 10; // 10 is count of cloud
    InsertUserTableRecord(tableName_, localCount);
    InsertCloudTableRecord(tableName_, 0, cloudCount, 0, false);
    Query query = Query::Select().FromTable({ tableName_ });
    BlockSync(query, delegate_);

    /**
     * @tc.steps:step2. check the 0x20 bit of flag after sync
     * @tc.expected: step2. ok.
     */
    std::string querySql = "select count(*) from " + DBCommon::GetLogTableName(tableName_) +
        " where flag&0x20=0;";
    EXPECT_EQ(sqlite3_exec(db_, querySql.c_str(), QueryCountCallback,
        reinterpret_cast<void *>(localCount), nullptr), SQLITE_OK);

    /**
     * @tc.steps:step3. delete local data and check
     * @tc.expected: step3. ok.
     */
    std::string sql = "delete from " + tableName_ + " where id = '1';";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db_, sql), E_OK);
    EXPECT_EQ(sqlite3_exec(db_, querySql.c_str(), QueryCountCallback,
        reinterpret_cast<void *>(localCount - 1), nullptr), SQLITE_OK);

    /**
     * @tc.steps:step4. check the 0x20 bit of flag after sync
     * @tc.expected: step4. ok.
     */
    BlockSync(query, delegate_);
    EXPECT_EQ(sqlite3_exec(db_, querySql.c_str(), QueryCountCallback,
        reinterpret_cast<void *>(localCount), nullptr), SQLITE_OK);
}

void DistributedDBCloudCheckSyncTest::SyncDataStatusTest(bool isCompensatedSyncOnly)
{
    /**
     * @tc.steps:step1. init data and sync
     * @tc.expected: step1. ok.
     */
    const int localCount = 20; // 20 is count of local
    const int cloudCount = 10; // 10 is count of cloud
    InsertUserTableRecord(tableName_, localCount);
    std::string sql = "update " + DBCommon::GetLogTableName(tableName_) + " SET status = 1 where data_key in (1,11);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db_, sql), E_OK);
    sql = "update " + DBCommon::GetLogTableName(tableName_) + " SET status = 2 where data_key in (2,12);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db_, sql), E_OK);
    sql = "update " + DBCommon::GetLogTableName(tableName_) + " SET status = 3 where data_key in (3,13);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db_, sql), E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    InsertCloudTableRecord(tableName_, 0, cloudCount, 0, false);
    Query query = Query::Select().FromTable({tableName_});

    /**
     * @tc.steps:step2. check count
     * @tc.expected: step2. ok.
     */
    int64_t syncCount = 2;
    BlockPrioritySync(query, delegate_, false, OK, isCompensatedSyncOnly);
    if (!isCompensatedSyncOnly) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // wait compensated sync finish
    }
    std::string preSql = "select count(*) from " + DBCommon::GetLogTableName(tableName_);
    std::string querySql = preSql + " where status=0 and data_key in (1,11) and cloud_gid !='';";
    CloudDBSyncUtilsTest::CheckCount(db_, querySql, syncCount);
    if (isCompensatedSyncOnly) {
        querySql = preSql + " where status=2 and data_key in (2,12) and cloud_gid ='';";
        CloudDBSyncUtilsTest::CheckCount(db_, querySql, syncCount);
        querySql = preSql + " where status=3 and data_key in (3,13) and cloud_gid ='';";
        CloudDBSyncUtilsTest::CheckCount(db_, querySql, syncCount);
        querySql = preSql + " where status=0 and cloud_gid ='';";
        int unSyncCount = 14; // 14 is the num of unSync data with status 0
        CloudDBSyncUtilsTest::CheckCount(db_, querySql, unSyncCount);
    } else {
        // gid 12、13 are upload insert, lock to lock_change
        querySql = preSql + " where status=3 and data_key in (2,12) and cloud_gid !='';";
        CloudDBSyncUtilsTest::CheckCount(db_, querySql, syncCount);
        querySql = preSql + " where status=3 and data_key in (3,13) and cloud_gid !='';";
        CloudDBSyncUtilsTest::CheckCount(db_, querySql, syncCount);
        querySql = preSql + " where status=0 and cloud_gid !='';";
        int unSyncCount = 16; // 16 is the num of sync finish
        CloudDBSyncUtilsTest::CheckCount(db_, querySql, unSyncCount);
    }
}

/*
 * @tc.name: SyncDataStatusTest001
 * @tc.desc: Test the status after compensated sync the no asset table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, SyncDataStatusTest001, TestSize.Level1)
{
    SyncDataStatusTest(true);
}

/*
 * @tc.name: SyncDataStatusTest002
 * @tc.desc: Test the status after normal sync the no asset table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudCheckSyncTest, SyncDataStatusTest002, TestSize.Level1)
{
    SyncDataStatusTest(false);
}
}
#endif