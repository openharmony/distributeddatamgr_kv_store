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
#ifdef RELATIONAL_STORE
#include "cloud/asset_operation_utils.h"
#include "cloud/cloud_storage_utils.h"
#include "cloud/cloud_db_constant.h"
#include "cloud_db_sync_utils_test.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "process_system_api_adapter_impl.h"
#include "relational_store_client.h"
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
const string STORE_ID = "Relational_Store_Lock_Sync";
const string DB_SUFFIX = ".db";
const string ASSETS_TABLE_NAME = "student";
const string ASSETS_TABLE_NAME_SHARED = "student_shared";
const string DEVICE_CLOUD = "cloud_dev";
const string COL_ID = "id";
const string COL_NAME = "name";
const string COL_HEIGHT = "height";
const string COL_ASSET = "asset";
const string COL_ASSETS = "assets";
const string COL_AGE = "age";
const int64_t WAIT_TIME = 5;
const std::vector<Field> CLOUD_FIELDS = {{COL_ID, TYPE_INDEX<int64_t>, true}, {COL_NAME, TYPE_INDEX<std::string>},
    {COL_ASSET, TYPE_INDEX<Asset>}, {COL_ASSETS, TYPE_INDEX<Assets>}};
const string CREATE_SINGLE_PRIMARY_KEY_TABLE = "CREATE TABLE IF NOT EXISTS " + ASSETS_TABLE_NAME + "(" + COL_ID +
    " INTEGER PRIMARY KEY," + COL_NAME + " TEXT ," + COL_ASSET + " ASSET," + COL_ASSETS + " ASSETS" + ");";
const Asset ASSET_COPY = {.version = 1,
    .name = "Phone",
    .assetId = "0",
    .subpath = "/local/sync",
    .uri = "/local/sync",
    .modifyTime = "123456",
    .createTime = "",
    .size = "256",
    .hash = "ASE"};
const Assets ASSETS_COPY1 = { ASSET_COPY };

string g_storePath;
string g_testDir;
RelationalStoreObserverUnitTest *g_observer = nullptr;
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;
std::shared_ptr<VirtualCloudDb> g_virtualCloudDb;
std::shared_ptr<VirtualAssetLoader> g_virtualAssetLoader;
std::shared_ptr<VirtualCloudDataTranslate> g_virtualCloudDataTranslate;
SyncProcess g_syncProcess;
std::condition_variable g_processCondition;
std::mutex g_processMutex;
IRelationalStore *g_store = nullptr;
ICloudSyncStorageHook *g_cloudStoreHook = nullptr;
int64_t g_nameId;
using CloudSyncStatusCallback = std::function<void(const std::map<std::string, SyncProcess> &onProcess)>;

void GetCloudDbSchema(DataBaseSchema &dataBaseSchema)
{
    TableSchema assetsTableSchema = {.name = ASSETS_TABLE_NAME, .sharedTableName = ASSETS_TABLE_NAME_SHARED,
        .fields = CLOUD_FIELDS};
    dataBaseSchema.tables.push_back(assetsTableSchema);
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

class DistributedDBCloudSyncerLockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    void Init();
    const RelationalSyncAbleStorage *GetRelationalStore();
    void InsertLocalData(int64_t begin, int64_t count, const std::string &tableName, bool isAssetNull = true);
    void GenerateDataRecords(
        int64_t begin, int64_t count, int64_t gidStart, std::vector<VBucket> &record, std::vector<VBucket> &extend);
    void InsertCloudDBData(int64_t begin, int64_t count, int64_t gidStart, const std::string &tableName);
    void UpdateCloudDBData(int64_t begin, int64_t count, int64_t gidStart, int64_t versionStart,
        const std::string &tableName);
    void CallSync(const CloudSyncOption &option, DBStatus expectResult = OK);

    void TestConflictSync001(bool isUpdate);
    sqlite3 *db = nullptr;
};

void DistributedDBCloudSyncerLockTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/" + STORE_ID + DB_SUFFIX;
    LOGI("The test db is:%s", g_storePath.c_str());
    g_virtualCloudDataTranslate = std::make_shared<VirtualCloudDataTranslate>();
    RuntimeConfig::SetCloudTranslate(g_virtualCloudDataTranslate);
}

void DistributedDBCloudSyncerLockTest::TearDownTestCase(void) {}

void DistributedDBCloudSyncerLockTest::SetUp(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    LOGD("Test dir is %s", g_testDir.c_str());
    Init();
    g_cloudStoreHook = (ICloudSyncStorageHook *) GetRelationalStore();
    ASSERT_NE(g_cloudStoreHook, nullptr);
}

void DistributedDBCloudSyncerLockTest::TearDown(void)
{
    RefObject::DecObjRef(g_store);
    g_virtualCloudDb->ForkUpload(nullptr);
    CloseDb();
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

void DistributedDBCloudSyncerLockTest::Init()
{
    db = RelationalTestUtils::CreateDataBase(g_storePath);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_SINGLE_PRIMARY_KEY_TABLE), SQLITE_OK);
    g_observer = new (std::nothrow) RelationalStoreObserverUnitTest();
    ASSERT_NE(g_observer, nullptr);
    ASSERT_EQ(g_mgr.OpenStore(g_storePath, STORE_ID, RelationalStoreDelegate::Option{.observer = g_observer},
        g_delegate), DBStatus::OK);
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
    g_nameId = 0;
}

const RelationalSyncAbleStorage* DistributedDBCloudSyncerLockTest::GetRelationalStore()
{
    RelationalDBProperties properties;
    CloudDBSyncUtilsTest::InitStoreProp(g_storePath, APP_ID, USER_ID, STORE_ID, properties);
    int errCode = E_OK;
    g_store = RelationalStoreInstance::GetDataBase(properties, errCode);
    if (g_store == nullptr) {
        return nullptr;
    }
    return static_cast<SQLiteRelationalStore *>(g_store)->GetStorageEngine();
}


void DistributedDBCloudSyncerLockTest::GenerateDataRecords(
    int64_t begin, int64_t count, int64_t gidStart, std::vector<VBucket> &record, std::vector<VBucket> &extend)
{
    for (int64_t i = begin; i < begin + count; i++) {
        Assets assets;
        Asset asset = ASSET_COPY;
        asset.name = ASSET_COPY.name + std::to_string(i);
        assets.emplace_back(asset);
        asset.name = ASSET_COPY.name + std::to_string(i) + "_copy";
        assets.emplace_back(asset);
        VBucket data;
        data.insert_or_assign(COL_ID, i);
        data.insert_or_assign(COL_NAME, "name" + std::to_string(g_nameId++));
        data.insert_or_assign(COL_ASSETS, assets);
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

void DistributedDBCloudSyncerLockTest::InsertLocalData(int64_t begin, int64_t count,
    const std::string &tableName, bool isAssetNull)
{
    int errCode;
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    GenerateDataRecords(begin, count, 0, record, extend);
    const string sql = "insert or replace into " + tableName + " values (?,?,?,?);";
    for (VBucket vBucket : record) {
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
        ASSERT_EQ(SQLiteUtils::BindInt64ToStatement(stmt, 1, std::get<int64_t>(vBucket[COL_ID])), E_OK); // 1 is id
        ASSERT_EQ(SQLiteUtils::BindTextToStatement(stmt, 2, std::get<string>(vBucket[COL_NAME])), E_OK); // 2 is name
        if (isAssetNull) {
            ASSERT_EQ(sqlite3_bind_null(stmt, 3), SQLITE_OK); // 3 is asset
        } else {
            std::vector<uint8_t> assetBlob = g_virtualCloudDataTranslate->AssetToBlob(ASSET_COPY);
            ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 3, assetBlob, false), E_OK); // 3 is asset
        }
        std::vector<uint8_t> assetsBlob = g_virtualCloudDataTranslate->AssetsToBlob(
            std::get<Assets>(vBucket[COL_ASSETS]));
        ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 4, assetsBlob, false), E_OK); // 4 is assets
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
}

void DistributedDBCloudSyncerLockTest::InsertCloudDBData(int64_t begin, int64_t count, int64_t gidStart,
    const std::string &tableName)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    GenerateDataRecords(begin, count, gidStart, record, extend);
    ASSERT_EQ(g_virtualCloudDb->BatchInsertWithGid(tableName, std::move(record), extend), DBStatus::OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void DistributedDBCloudSyncerLockTest::UpdateCloudDBData(int64_t begin, int64_t count, int64_t gidStart,
    int64_t versionStart, const std::string &tableName)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    GenerateDataRecords(begin, count, gidStart, record, extend);
    for (auto &entry: extend) {
        entry[CloudDbConstant::VERSION_FIELD] = std::to_string(versionStart++);
    }
    ASSERT_EQ(g_virtualCloudDb->BatchUpdate(tableName, std::move(record), extend), DBStatus::OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

CloudSyncOption PrepareOption(const Query &query, LockAction action, bool isPriorityTask = false,
    bool isCompensatedSyncOnly = false)
{
    CloudSyncOption option;
    option.devices = { "CLOUD" };
    option.mode = SYNC_MODE_CLOUD_MERGE;
    option.query = query;
    option.waitTime = WAIT_TIME;
    option.priorityTask = isPriorityTask;
    option.compensatedSyncOnly = isCompensatedSyncOnly;
    option.lockAction = action;
    return option;
}

void DistributedDBCloudSyncerLockTest::CallSync(const CloudSyncOption &option, DBStatus expectResult)
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
    ASSERT_EQ(g_delegate->Sync(option, callback), expectResult);
    if (expectResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }
}

void DistributedDBCloudSyncerLockTest::TestConflictSync001(bool isUpdate)
{
    /**
     * @tc.steps:step1. init data and sync
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 20;
    int localCount = 10;
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    InsertLocalData(0, localCount, ASSETS_TABLE_NAME, true);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);

    /**
     * @tc.steps:step2. update local data to upload, and set hook before upload, operator cloud data which id is 1
     * @tc.expected: step2. return ok.
     */
    std::string sql;
    if (isUpdate) {
        sql = "update " + ASSETS_TABLE_NAME + " set name = 'xxx' where id = 1;";
    } else {
        sql = "delete from " + ASSETS_TABLE_NAME + " where id = 1;";
    }
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql.c_str()), SQLITE_OK);
    int index = 0;
    g_cloudStoreHook->SetDoUploadHook([&index, this]() {
        if (++index == 1) {
            UpdateCloudDBData(1, 1, 0, 21, ASSETS_TABLE_NAME); // 21 is version
        }
    });

    /**
     * @tc.steps:step3. sync and check local data
     * @tc.expected: step3. return ok.
     */
    CallSync(option);
    sql = "select count(*) from " + ASSETS_TABLE_NAME + " where name = 'name30' AND id = '1';";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1), nullptr), SQLITE_OK);
}

/**
 * @tc.name: RDBUnlockCloudSync001
 * @tc.desc: Test sync with no lock
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, RDBUnlockCloudSync001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data and sync with none lock
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 20;
    int localCount = 10;
    InsertLocalData(0, cloudCount, ASSETS_TABLE_NAME, true);
    InsertCloudDBData(0, localCount, 0, ASSETS_TABLE_NAME);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::NONE);
    CallSync(option);
}

/**
 * @tc.name: RDBConflictCloudSync001
 * @tc.desc: Both cloud and local are available, local version is empty, with cloud updates before upload
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, RDBConflictCloudSync001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data and set hook before upload, update cloud data which gid is 1
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 20;
    int localCount = 10;
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    InsertLocalData(0, localCount, ASSETS_TABLE_NAME, true);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    int index = 0;
    g_cloudStoreHook->SetDoUploadHook([&index, this]() {
        if (++index == 1) {
            UpdateCloudDBData(1, 1, 0, 1, ASSETS_TABLE_NAME);
        }
    });

    /**
     * @tc.steps:step2. sync and check local data
     * @tc.expected: step2. return ok.
     */
    CallSync(option);
    std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) +
        " where flag&0x02=0 AND version='20' AND cloud_gid = '1';";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1), nullptr), SQLITE_OK);
}

/**
 * @tc.name: RDBConflictCloudSync002
 * @tc.desc: Both cloud and local are available, with cloud updates before upload
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, RDBConflictCloudSync002, TestSize.Level0)
{
    TestConflictSync001(true);
}

/**
 * @tc.name: RDBConflictCloudSync003
 * @tc.desc: Both cloud and local are available, with cloud deletes before upload
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, RDBConflictCloudSync003, TestSize.Level0)
{
    TestConflictSync001(false);
}

/**
 * @tc.name: RDBConflictCloudSync003
 * @tc.desc: Both cloud and local are available, with cloud inserts before upload
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, RDBConflictCloudSync004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data and sync
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 20;
    int localCount = 10;
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    InsertLocalData(0, localCount, ASSETS_TABLE_NAME, true);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);

    /**
     * @tc.steps:step2. insert local data and set hook before upload, insert cloud data which id is 20
     * @tc.expected: step2. return ok.
     */
    std::string sql = "INSERT INTO " + ASSETS_TABLE_NAME + " VALUES('20', 'XXX', NULL, NULL);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql.c_str()), SQLITE_OK);
    int index = 0;
    g_cloudStoreHook->SetDoUploadHook([&index, cloudCount, this]() {
        if (++index == 1) {
            InsertCloudDBData(cloudCount, 1, cloudCount, ASSETS_TABLE_NAME);
        }
    });

    /**
     * @tc.steps:step3. set hook for batch insert, return CLOUD_VERSION_CONFLICT err
     * @tc.expected: step3. return ok.
     */
    g_virtualCloudDb->ForkInsertConflict([](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        for (const auto &[cloudRecord, cloudExtend]: cloudDataVec) {
            int64_t cloudPk;
            CloudStorageUtils::GetValueFromVBucket<int64_t>(COL_ID, record, cloudPk);
            int64_t localPk;
            CloudStorageUtils::GetValueFromVBucket<int64_t>(COL_ID, cloudRecord, localPk);
            if (cloudPk != localPk) {
                continue;
            }
            std::string localVersion;
            CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::VERSION_FIELD, extend, localVersion);
            std::string cloudVersion;
            CloudStorageUtils::GetValueFromVBucket<std::string>(CloudDbConstant::VERSION_FIELD, cloudExtend,
                cloudVersion);
            if (localVersion != cloudVersion) {
                return CLOUD_VERSION_CONFLICT;
            }
        }
        return OK;
    });

    /**
     * @tc.steps:step3. sync and check local data
     * @tc.expected: step3. return ok.
     */
    CallSync(option);
    sql = "select count(*) from " + ASSETS_TABLE_NAME + " where name = 'name30' AND id = '20';";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1), nullptr), SQLITE_OK);
}

} // namespace
#endif // RELATIONAL_STORE