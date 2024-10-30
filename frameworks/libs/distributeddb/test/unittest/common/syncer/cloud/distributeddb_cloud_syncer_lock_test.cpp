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
#include "virtual_communicator_aggregator.h"
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
const string ASSET_SUFFIX = "_copy";

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
    void DeleteCloudDBData(int64_t beginGid, int64_t count, const std::string &tableName);
    void CallSync(const CloudSyncOption &option, DBStatus expectResult = OK);

    void TestConflictSync001(bool isUpdate);
    void CheckAssetStatusNormal();
    void UpdateCloudAssets(Asset &asset, Assets &assets, const std::string &version);
    void CheckUploadAbnormal(OpType opType, int64_t expCnt, bool isCompensated = false);
    sqlite3 *db = nullptr;
    VirtualCommunicatorAggregator *communicatorAggregator_ = nullptr;
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
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(false);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    LOGD("Test dir is %s", g_testDir.c_str());
    Init();
    g_cloudStoreHook = (ICloudSyncStorageHook *) GetRelationalStore();
    ASSERT_NE(g_cloudStoreHook, nullptr);
    communicatorAggregator_ = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator_ != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator_);
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
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    communicatorAggregator_ = nullptr;
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
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
        VBucket data;
        data.insert_or_assign(COL_ASSET, asset);
        asset.name = ASSET_COPY.name + std::to_string(i) + "_copy";
        assets.emplace_back(asset);
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

void DistributedDBCloudSyncerLockTest::DeleteCloudDBData(int64_t beginGid, int64_t count,
    const std::string &tableName)
{
    Timestamp now = TimeHelper::GetSysCurrentTime();
    std::vector<VBucket> extend;
    for (int64_t i = 0; i < count; ++i) {
        VBucket log;
        log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND + i);
        log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND + i);
        log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(beginGid + i));
        extend.push_back(log);
    }
    ASSERT_EQ(g_virtualCloudDb->BatchDelete(tableName, extend), DBStatus::OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(count));
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
    SyncProcess last;
    auto callback = [&last, &cv, &dataMutex, &finish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    finish = true;
                    last = item.second;
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
    g_syncProcess = last;
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

void DistributedDBCloudSyncerLockTest::CheckAssetStatusNormal()
{
    std::string sql = "SELECT asset, assets FROM " + ASSETS_TABLE_NAME + ";";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_BLOB);
        ASSERT_EQ(sqlite3_column_type(stmt, 1), SQLITE_BLOB);
        Type assetBlob;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Asset>, 0, assetBlob), E_OK);
        Asset asset = g_virtualCloudDataTranslate->BlobToAsset(std::get<Bytes>(assetBlob));
        EXPECT_EQ(asset.status, static_cast<uint32_t>(AssetStatus::NORMAL));
        Type assetsBlob;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Assets>, 0, assetsBlob), E_OK);
        Assets assets = g_virtualCloudDataTranslate->BlobToAssets(std::get<Bytes>(assetsBlob));
        for (const auto &as : assets) {
            EXPECT_EQ(as.status, static_cast<uint32_t>(AssetStatus::NORMAL));
        }
    }
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void DistributedDBCloudSyncerLockTest::UpdateCloudAssets(Asset &asset, Assets &assets, const std::string &version)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    VBucket data;
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    asset.name.empty() ? data.insert_or_assign(COL_ASSET, Nil()) : data.insert_or_assign(COL_ASSET, asset);
    data.insert_or_assign(COL_ID, 0L);
    data.insert_or_assign(COL_NAME, "name" + std::to_string(g_nameId++));
    assets.empty() ? data.insert_or_assign(COL_ASSETS, Nil()) : data.insert_or_assign(COL_ASSETS, assets);
    record.push_back(data);
    VBucket log;
    Timestamp now = TimeHelper::GetSysCurrentTime();
    log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND);
    log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND);
    log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
    log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(0));
    log.insert_or_assign(CloudDbConstant::VERSION_FIELD, version);
    extend.push_back(log);
    ASSERT_EQ(g_virtualCloudDb->BatchUpdate(ASSETS_TABLE_NAME, std::move(record), extend), DBStatus::OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void DistributedDBCloudSyncerLockTest::CheckUploadAbnormal(OpType opType, int64_t expCnt, bool isCompensated)
{
    std::string sql = "SELECT count(*) FROM " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE ";
    switch (opType) {
        case OpType::INSERT:
            sql += isCompensated ? " cloud_gid != '' AND version !='' AND flag&0x10=0" :
                   " cloud_gid != '' AND version !='' AND flag=flag|0x10";
            break;
        case OpType::UPDATE:
            sql += isCompensated ? " cloud_gid != '' AND version !='' AND flag&0x10=0" :
                   " cloud_gid == '' AND version =='' AND flag=flag|0x10";
            break;
        case OpType::DELETE:
            sql += " cloud_gid == '' AND version ==''";
            break;
        default:
            break;
    }
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(expCnt), nullptr), SQLITE_OK);
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

    /**
     * @tc.steps:step2. insert or replace, check version
     * @tc.expected: step2. return ok.
     */
    std::string sql = "INSERT OR REPLACE INTO " + ASSETS_TABLE_NAME + " VALUES('0', 'XX', '', '');";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql.c_str()), SQLITE_OK);
    sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) +
        " where version != '' and version is not null;";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
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
        for (auto &[cloudRecord, cloudExtend]: cloudDataVec) {
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
                extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_VERSION_CONFLICT);
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
    for (const auto &table : g_syncProcess.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.failCount, 0u);
    }
}

/**
 * @tc.name: QueryCursorTest001
 * @tc.desc: Test cursor after querying no data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, QueryCursorTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data and Query with cursor tha exceeds range
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 20;
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    VBucket extend;
    extend[CloudDbConstant::CURSOR_FIELD] = std::to_string(30);
    std::vector<VBucket> data;

    /**
     * @tc.steps:step2. check cursor output param
     * @tc.expected: step2. return QUERY_END.
     */
    EXPECT_EQ(g_virtualCloudDb->Query(ASSETS_TABLE_NAME, extend, data), QUERY_END);
    EXPECT_EQ(std::get<std::string>(extend[CloudDbConstant::CURSOR_FIELD]), std::to_string(cloudCount));
}

/**
 * @tc.name: QueryCursorTest002
 * @tc.desc: Test cursor in conditional query sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, QueryCursorTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return ok.
     */
    int count = 10;
    InsertCloudDBData(0, count, 0, ASSETS_TABLE_NAME);
    InsertLocalData(0, count, ASSETS_TABLE_NAME, true);
    std::vector<int> idVec = {2, 3};
    CloudSyncOption option = PrepareOption(Query::Select().From(ASSETS_TABLE_NAME).In("id", idVec),
        LockAction::DOWNLOAD, true);
    int index = 0;

    /**
     * @tc.steps:step2. sync and check cursor
     * @tc.expected: step2. return ok.
     */
    g_virtualCloudDb->ForkQuery([&index](const std::string &, VBucket &extend) {
        if (index == 1) {
            std::string cursor;
            CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::CURSOR_FIELD, extend, cursor);
            EXPECT_EQ(cursor, std::string(""));
        }
        index++;
    });
    CallSync(option);
}

/**
 * @tc.name: DownloadAssetStatusTest001
 * @tc.desc: Test download assets status for INSERT
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, DownloadAssetStatusTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init cloud assert {a, b1, b2}
     * @tc.expected: step1. return ok.
     */
    int count = 1;
    InsertCloudDBData(0, count, 0, ASSETS_TABLE_NAME);
    /**
     * @tc.steps:step2. sync
     * @tc.expected: step2. assets status is INSERT before download.
     */
    g_virtualAssetLoader->ForkDownload([](std::map<std::string, Assets> &assets) {
        for (const auto &item: assets) {
            for (const auto &asset: item.second) {
                EXPECT_EQ(asset.status, static_cast<uint32_t>(AssetStatus::INSERT));
            }
        }
    });
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);
    CheckAssetStatusNormal();
    g_virtualAssetLoader->ForkDownload(nullptr);
}

/**
 * @tc.name: DownloadAssetStatusTest002
 * @tc.desc: Test download assets status for DELETE
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, DownloadAssetStatusTest002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init cloud assert {a, b1, b2} and sync to local
     * @tc.expected: step1. return ok.
     */
    int count = 1;
    InsertCloudDBData(0, count, 0, ASSETS_TABLE_NAME);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);

    /**
     * @tc.steps:step2. change cloud assets {b1, b3}
     * @tc.expected: step2. return ok.
     */
    Asset asset = {};
    Asset b1 = ASSET_COPY;
    b1.name = ASSET_COPY.name + std::string("0");
    Asset b2 = ASSET_COPY;
    b2.name = ASSET_COPY.name + std::string("0") + ASSET_SUFFIX;
    Asset b3 = ASSET_COPY;
    b3.name = ASSET_COPY.name + std::string("0") + ASSET_SUFFIX + ASSET_SUFFIX;
    Assets assets = { b1, b3 };
    UpdateCloudAssets(asset, assets, std::string("0")); // 1 is version
    /**
     * @tc.steps:step3. sync
     * @tc.expected: step3. download status is a -> DELETE, b2 -> DELETE, b3 -> INSERT
     */
    g_virtualAssetLoader->ForkDownload([&b1, &b3](std::map<std::string, Assets> &assets) {
        auto it = assets.find(COL_ASSETS);
        ASSERT_EQ(it != assets.end(), true);
        ASSERT_EQ(it->second.size(), 1u); // 1 is download size
        for (const auto &b: it->second) {
            if (b.name == b3.name) {
                EXPECT_EQ(b.status, static_cast<uint32_t>(AssetStatus::INSERT));
            }
        }
    });
    g_virtualAssetLoader->SetRemoveLocalAssetsCallback([&b2](std::map<std::string, Assets> &assets) {
        auto it = assets.find(COL_ASSET);
        EXPECT_EQ(it != assets.end(), true);
        EXPECT_EQ(it->second.size(), 1u);
        EXPECT_EQ(it->second[0].status, static_cast<uint32_t>(AssetStatus::DELETE));
        it = assets.find(COL_ASSETS);
        EXPECT_EQ(it != assets.end(), true);
        EXPECT_EQ(it->second.size(), 1u); // 1 is remove size
        for (const auto &b: it->second) {
            if (b.name == b2.name) {
                EXPECT_EQ(b.status, static_cast<uint32_t>(AssetStatus::DELETE));
            }
        }
        return DBStatus::OK;
    });
    CallSync(option);
    g_virtualAssetLoader->ForkDownload(nullptr);
    g_virtualAssetLoader->SetRemoveLocalAssetsCallback(nullptr);
}

/**
 * @tc.name: DownloadAssetStatusTest003
 * @tc.desc: Test download assets status for UPDATE
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, DownloadAssetStatusTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init cloud assert {a, b1, b2} and sync to local
     * @tc.expected: step1. return ok.
     */
    int count = 1;
    InsertCloudDBData(0, count, 0, ASSETS_TABLE_NAME);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);
    /**
     * @tc.steps:step2. change cloud assets {a, b2}
     * @tc.expected: step2. return ok.
     */
    Asset asset = ASSET_COPY;
    asset.name = asset.name + "0";
    asset.hash = "new_hash";
    Asset b1 = ASSET_COPY;
    b1.name = ASSET_COPY.name + std::string("0");
    Asset b2 = ASSET_COPY;
    b2.name = ASSET_COPY.name + std::string("0") + ASSET_SUFFIX;
    b2.hash = "new_hash";
    Assets assets = { b1, b2 };
    UpdateCloudAssets(asset, assets, std::string("0")); // 1 is version
    /**
     * @tc.steps:step3. sync
     * @tc.expected: step3. download status is a -> UPDATE, b2 -> UPDATE
     */
    g_virtualAssetLoader->ForkDownload([&b1, &b2](std::map<std::string, Assets> &assets) {
        auto it = assets.find(COL_ASSET);
        ASSERT_EQ(it != assets.end(), true);
        ASSERT_EQ(it->second.size(), 1u);
        EXPECT_EQ(it->second[0].status, static_cast<uint32_t>(AssetStatus::UPDATE));

        it = assets.find(COL_ASSETS);
        ASSERT_EQ(it != assets.end(), true);
        ASSERT_EQ(it->second.size(), 1u); // 1 is download size
        for (const auto &b: it->second) {
            if (b.name == b2.name) {
                EXPECT_EQ(b.status, static_cast<uint32_t>(AssetStatus::UPDATE));
            }
        }
    });
    CallSync(option);
    g_virtualAssetLoader->ForkDownload(nullptr);
    g_virtualAssetLoader->SetRemoveLocalAssetsCallback(nullptr);
}

/**
 * @tc.name: RecordConflictTest001
 * @tc.desc: Test the asset input param after download return CLOUD_RECORD_EXIST_CONFLICT
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, RecordConflictTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data and sync
     * @tc.expected: step1. return ok.
     */
    int count = 10;
    InsertCloudDBData(0, count, 0, ASSETS_TABLE_NAME);
    g_virtualAssetLoader->SetDownloadStatus(DBStatus::CLOUD_RECORD_EXIST_CONFLICT);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    int callCount = 0;
    g_cloudStoreHook->SetSyncFinishHook([&callCount]() {
        callCount++;
        g_processCondition.notify_all();
    });
    CallSync(option);
    {
        std::unique_lock<std::mutex> lock(g_processMutex);
        bool result = g_processCondition.wait_for(lock, std::chrono::seconds(WAIT_TIME),
            [&callCount]() { return callCount == 2; }); // 2 is compensated sync
        ASSERT_EQ(result, true);
    }

    /**
     * @tc.steps:step2. sync again and check asset
     * @tc.expected: step2. return ok.
     */
    g_virtualAssetLoader->SetDownloadStatus(DBStatus::OK);
    g_virtualAssetLoader->ForkDownload([](std::map<std::string, Assets> &assets) {
        EXPECT_EQ(assets.find(COL_ASSET) != assets.end(), true);
    });
    CallSync(option);
    {
        std::unique_lock<std::mutex> lock(g_processMutex);
        bool result = g_processCondition.wait_for(lock, std::chrono::seconds(WAIT_TIME),
            [&callCount]() { return callCount == 4; }); // 4 is compensated sync
        ASSERT_EQ(result, true);
    }
    g_cloudStoreHook->SetSyncFinishHook(nullptr);
}

/**
 * @tc.name: QueryCursorTest003
 * @tc.desc: Test whether cursor fallback
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, QueryCursorTest003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init cloud data and sync
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 10;
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);

    /**
     * @tc.steps:step2. delete cloud data and sync
     * @tc.expected: step2. return ok.
     */
    DeleteCloudDBData(0, cloudCount, ASSETS_TABLE_NAME);
    CallSync(option);

    /**
     * @tc.steps:step3. remove data
     * @tc.expected: step3. return ok.
     */
    std::string device = "";
    ASSERT_EQ(g_delegate->RemoveDeviceData(device, DistributedDB::FLAG_ONLY), DBStatus::OK);

    /**
     * @tc.steps:step4. insert local and check cursor
     * @tc.expected: step4. return ok.
     */
    InsertLocalData(0, 1, ASSETS_TABLE_NAME, true);
    std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) +
        " where cursor='31';";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1), nullptr), SQLITE_OK);
}

/**
 * @tc.name: QueryCursorTest004
 * @tc.desc: Test temp trigger under concurrency
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, QueryCursorTest004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init cloud data
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 10;
    InsertLocalData(0, cloudCount, ASSETS_TABLE_NAME, true);
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. set tracker table before saving cloud data
     * @tc.expected: step2. return ok.
     */
    g_virtualCloudDb->ForkQuery([](const std::string &table, VBucket &) {
        TrackerSchema schema = {
            .tableName = ASSETS_TABLE_NAME, .extendColName = COL_NAME, .trackerColNames = { COL_ID }
        };
        EXPECT_EQ(g_delegate->SetTrackerTable(schema), WITH_INVENTORY_DATA);
    });
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);

    /**
     * @tc.steps:step3. check extend_field and cursor
     * @tc.expected: step3. return ok.
     */
    std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) +
        " where data_key='0' and extend_field='name10' and cursor='32';";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1), nullptr), SQLITE_OK);
}

/**
 * @tc.name: QueryCursorTest006
 * @tc.desc: Test cursor increasing when remove assets fail and download assets success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, QueryCursorTest006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert local and sync
     * @tc.expected: step1. return ok.
     */
    InsertLocalData(0, 1, ASSETS_TABLE_NAME, false);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);

    /**
     * @tc.steps:step2. change asset/assets and set RemoveLocalAssets fail
     * @tc.expected: step2. return ok.
     */
    std::string sql = "SELECT asset, assets FROM " + ASSETS_TABLE_NAME + ";";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    Asset asset;
    Assets assets;
    while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_BLOB);
        ASSERT_EQ(sqlite3_column_type(stmt, 1), SQLITE_BLOB);
        Type assetBlob;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Asset>, 0, assetBlob), E_OK);
        asset = g_virtualCloudDataTranslate->BlobToAsset(std::get<Bytes>(assetBlob));
        Type assetsBlob;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Assets>, 0, assetsBlob), E_OK);
        assets = g_virtualCloudDataTranslate->BlobToAssets(std::get<Bytes>(assetsBlob));
    }
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    asset.hash = "new_hash";
    assets.pop_back();
    UpdateCloudAssets(asset, assets, std::string("0"));
    g_virtualAssetLoader->SetRemoveStatus(DBStatus::LOCAL_ASSET_NOT_FOUND);

    /**
     * @tc.steps:step3. sync and check cursor
     * @tc.expected: step3. return ok.
     */
    CallSync(option);
    sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) +
        " where cursor='3';";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(1), nullptr), SQLITE_OK);
    g_virtualAssetLoader->SetRemoveStatus(DBStatus::OK);
}

/**
 * @tc.name: UploadAbnormalSync001
 * @tc.desc: Test upload update record, cloud returned record not found.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, UploadAbnormalSync001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert local data and sync
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 1;
    InsertLocalData(0, cloudCount, ASSETS_TABLE_NAME, true);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::DOWNLOAD);
    CallSync(option);

    /**
     * @tc.steps:step2. update local data and sync, cloud returned record not found.
     * @tc.expected: step2. return ok.
     */
    std::string sql = "update " + ASSETS_TABLE_NAME + " set name = 'xxx' where id = 0;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql.c_str()), SQLITE_OK);
    int upIdx = 0;
    g_virtualCloudDb->ForkUpload([&upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("cloud db upload index:%d", ++upIdx);
        if (upIdx == 1) { // 1 is index
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_NOT_FOUND);
        }
    });
    int doUpIdx = 0;
    g_cloudStoreHook->SetDoUploadHook([&doUpIdx] {
        LOGD("begin upload index:%d", ++doUpIdx);
    });
    int callCount = 0;
    g_cloudStoreHook->SetSyncFinishHook([&callCount, this]() {
        LOGD("sync finish times:%d", ++callCount);
        if (callCount == 1) { // 1 is the normal sync
            CheckUploadAbnormal(OpType::UPDATE, 1L); // 1 is expected count
        } else {
            CheckUploadAbnormal(OpType::UPDATE, 1L, true); // 1 is expected count
        }
        g_processCondition.notify_all();
    });
    CallSync(option);
    {
        std::unique_lock<std::mutex> lock(g_processMutex);
        bool result = g_processCondition.wait_for(lock, std::chrono::seconds(WAIT_TIME),
            [&callCount]() { return callCount == 2; }); // 2 is sync times
        ASSERT_EQ(result, true);
    }
}

/**
 * @tc.name: UploadAbnormalSync002
 * @tc.desc: Test upload insert record, cloud returned record already existed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, UploadAbnormalSync002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert a and sync
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 1;
    InsertLocalData(0, cloudCount, ASSETS_TABLE_NAME, true);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::DOWNLOAD);
    CallSync(option);

    /**
     * @tc.steps:step2. insert b and sync, cloud returned record not found.
     * @tc.expected: step2. return ok.
     */
    InsertLocalData(cloudCount, cloudCount, ASSETS_TABLE_NAME, true);
    int upIdx = 0;
    g_virtualCloudDb->ForkUpload([&upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("cloud db upload index:%d", ++upIdx);
        if (upIdx == 2) { // 2 is index
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_ALREADY_EXISTED);
        }
    });
    int doUpIdx = 0;
    g_cloudStoreHook->SetDoUploadHook([&doUpIdx, cloudCount, this] {
        LOGD("begin upload index:%d", ++doUpIdx);
        if (doUpIdx == 1) { // 1 is index
            InsertCloudDBData(cloudCount, cloudCount, cloudCount, ASSETS_TABLE_NAME);
        }
    });
    int callCount = 0;
    g_cloudStoreHook->SetSyncFinishHook([&callCount, this]() {
        LOGD("sync finish times:%d", ++callCount);
        if (callCount == 1) { // 1 is the normal sync
            CheckUploadAbnormal(OpType::INSERT, 1L); // 1 is expected count
        } else {
            CheckUploadAbnormal(OpType::INSERT, 2L, true); // 1 is expected count
        }
        g_processCondition.notify_all();
    });
    CallSync(option);
    {
        std::unique_lock<std::mutex> lock(g_processMutex);
        bool result = g_processCondition.wait_for(lock, std::chrono::seconds(WAIT_TIME),
            [&callCount]() { return callCount == 2; }); // 2 is sync times
        ASSERT_EQ(result, true);
    }
}

/**
 * @tc.name: UploadAbnormalSync003
 * @tc.desc: Test upload delete record, cloud returned record not found.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, UploadAbnormalSync003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert local data and sync
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 1;
    InsertLocalData(0, cloudCount, ASSETS_TABLE_NAME, true);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::DOWNLOAD);
    CallSync(option);

    /**
     * @tc.steps:step2. delete local data and sync, cloud returned record not found.
     * @tc.expected: step2. return ok.
     */
    std::string sql = "delete from " + ASSETS_TABLE_NAME + " where id = 0;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql.c_str()), SQLITE_OK);
    int upIdx = 0;
    g_virtualCloudDb->ForkUpload([&upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("cloud db upload index:%d", ++upIdx);
        if (upIdx == 2) { // 2 is index
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_RECORD_NOT_FOUND);
        }
    });
    int doUpIdx = 0;
    g_cloudStoreHook->SetDoUploadHook([&doUpIdx, cloudCount, this] {
        LOGD("begin upload index:%d", ++doUpIdx);
        if (doUpIdx == 1) { // 1 is index
            DeleteCloudDBData(0, cloudCount, ASSETS_TABLE_NAME);
        }
    });
    int callCount = 0;
    g_cloudStoreHook->SetSyncFinishHook([&callCount, this]() {
        LOGD("sync finish times:%d", ++callCount);
        if (callCount == 1) { // 1 is the normal sync
            CheckUploadAbnormal(OpType::DELETE, 1L); // 1 is expected count
        } else {
            CheckUploadAbnormal(OpType::DELETE, 1L, true); // 1 is expected count
        }
        g_processCondition.notify_all();
    });
    CallSync(option);
    {
        std::unique_lock<std::mutex> lock(g_processMutex);
        bool result = g_processCondition.wait_for(lock, std::chrono::seconds(WAIT_TIME),
            [&callCount]() { return callCount == 1; }); // 1 is sync times
        ASSERT_EQ(result, true);
    }
}

/**
 * @tc.name: ReviseLocalModTimeTest001
 * @tc.desc: test sync data with invalid timestamp.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudSyncerLockTest, ReviseLocalModTimeTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. insert local data
     * @tc.expected: step1. return ok.
     */
    int cloudCount = 31; // 31 records
    InsertLocalData(0, cloudCount, ASSETS_TABLE_NAME, true);
    /**
     * @tc.steps:step2. Modify time and sync
     * @tc.expected: step2. return ok.
     */
    uint64_t curTime = 0;
    EXPECT_EQ(TimeHelper::GetSysCurrentRawTime(curTime), E_OK);
    uint64_t invalidTime = curTime + curTime;
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) +
        " SET timestamp=" + std::to_string(invalidTime) + " where rowid>0";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql.c_str()), SQLITE_OK);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::INSERT);
    CallSync(option);
    /**
     * @tc.steps:step3. Check modify time in log table
     * @tc.expected: step3. return ok.
     */
    EXPECT_EQ(TimeHelper::GetSysCurrentRawTime(curTime), E_OK);
    sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) +
        " where timestamp < " + std::to_string(curTime);
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(cloudCount), nullptr), SQLITE_OK);
}
} // namespace
#endif // RELATIONAL_STORE