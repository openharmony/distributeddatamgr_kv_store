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
#include "cloud/asset_operation_utils.h"
#include "cloud/cloud_storage_utils.h"
#include "cloud/cloud_db_constant.h"
#include "cloud_db_sync_utils_test.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "mock_asset_loader.h"
#include "process_system_api_adapter_impl.h"
#include "relational_store_client.h"
#include "relational_store_delegate_impl.h"
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
const string STORE_ID = "Relational_Store_SYNC";
const string DB_SUFFIX = ".db";
const string ASSETS_TABLE_NAME = "student";
const string ASSETS_TABLE_NAME_SHARED = "student_shared";
const string NO_PRIMARY_TABLE = "teacher";
const string NO_PRIMARY_TABLE_SHARED = "teacher_shared";
const string COMPOUND_PRIMARY_TABLE = "worker1";
const string COMPOUND_PRIMARY_TABLE_SHARED = "worker1_shared";
const string DEVICE_CLOUD = "cloud_dev";
const string COL_ID = "id";
const string COL_NAME = "name";
const string COL_HEIGHT = "height";
const string COL_ASSET = "asset";
const string COL_ASSETS = "assets";
const string COL_AGE = "age";
const int64_t SYNC_WAIT_TIME = 600;
const int64_t COMPENSATED_SYNC_WAIT_TIME = 5;
const std::vector<Field> CLOUD_FIELDS = {{COL_ID, TYPE_INDEX<int64_t>, true}, {COL_NAME, TYPE_INDEX<std::string>},
    {COL_HEIGHT, TYPE_INDEX<double>}, {COL_ASSET, TYPE_INDEX<Asset>}, {COL_ASSETS, TYPE_INDEX<Assets>},
    {COL_AGE, TYPE_INDEX<int64_t>}};
const std::vector<Field> NO_PRIMARY_FIELDS = {{COL_ID, TYPE_INDEX<int64_t>}, {COL_NAME, TYPE_INDEX<std::string>},
    {COL_HEIGHT, TYPE_INDEX<double>}, {COL_ASSET, TYPE_INDEX<Asset>}, {COL_ASSETS, TYPE_INDEX<Assets>},
    {COL_AGE, TYPE_INDEX<int64_t>}};
const std::vector<Field> COMPOUND_PRIMARY_FIELDS = {{COL_ID, TYPE_INDEX<int64_t>, true},
    {COL_NAME, TYPE_INDEX<std::string>}, {COL_HEIGHT, TYPE_INDEX<double>}, {COL_ASSET, TYPE_INDEX<Asset>},
    {COL_ASSETS, TYPE_INDEX<Assets>}, {COL_AGE, TYPE_INDEX<int64_t>, true}};
const string CREATE_SINGLE_PRIMARY_KEY_TABLE = "CREATE TABLE IF NOT EXISTS " + ASSETS_TABLE_NAME + "(" + COL_ID +
    " INTEGER PRIMARY KEY," + COL_NAME + " TEXT ," + COL_HEIGHT + " REAL ," + COL_ASSET + " ASSET," +
    COL_ASSETS + " ASSETS," + COL_AGE + " INT);";
const string CREATE_NO_PRIMARY_KEY_TABLE = "CREATE TABLE IF NOT EXISTS " + NO_PRIMARY_TABLE + "(" + COL_ID +
    " INTEGER," + COL_NAME + " TEXT ," + COL_HEIGHT + " REAL ," + COL_ASSET + " ASSET," + COL_ASSETS +
    " ASSETS," + COL_AGE + " INT);";
const string CREATE_COMPOUND_PRIMARY_KEY_TABLE = "CREATE TABLE IF NOT EXISTS " + COMPOUND_PRIMARY_TABLE + "(" + COL_ID +
    " INTEGER," + COL_NAME + " TEXT ," + COL_HEIGHT + " REAL ," + COL_ASSET + " ASSET," + COL_ASSETS + " ASSETS," +
    COL_AGE + " INT, PRIMARY KEY (id, age));";
const Asset ASSET_COPY = {.version = 1,
    .name = "Phone",
    .assetId = "0",
    .subpath = "/local/sync",
    .uri = "/local/sync",
    .modifyTime = "123456",
    .createTime = "",
    .size = "256",
    .hash = "ASE"};
const Asset ASSET_COPY2 = {.version = 1,
    .name = "Phone_copy_2",
    .assetId = "0",
    .subpath = "/local/sync",
    .uri = "/local/sync",
    .modifyTime = "123456",
    .createTime = "",
    .size = "256",
    .hash = "ASE"};
const Assets ASSETS_COPY1 = { ASSET_COPY, ASSET_COPY2 };
const std::string QUERY_CONSISTENT_SQL = "select count(*) from naturalbase_rdb_aux_student_log where flag&0x20=0;";
const std::string QUERY_COMPENSATED_SQL = "select count(*) from naturalbase_rdb_aux_student_log where flag&0x10!=0;";

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
using CloudSyncStatusCallback = std::function<void(const std::map<std::string, SyncProcess> &onProcess)>;

void InitDatabase(sqlite3 *&db)
{
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_SINGLE_PRIMARY_KEY_TABLE), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_NO_PRIMARY_KEY_TABLE), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_COMPOUND_PRIMARY_KEY_TABLE), SQLITE_OK);
}

void GetCloudDbSchema(DataBaseSchema &dataBaseSchema)
{
    TableSchema assetsTableSchema = {.name = ASSETS_TABLE_NAME, .sharedTableName = ASSETS_TABLE_NAME_SHARED,
                                     .fields = CLOUD_FIELDS};
    dataBaseSchema.tables.push_back(assetsTableSchema);
    assetsTableSchema = {.name = NO_PRIMARY_TABLE, .sharedTableName = NO_PRIMARY_TABLE_SHARED,
                         .fields = NO_PRIMARY_FIELDS};
    dataBaseSchema.tables.push_back(assetsTableSchema);
    assetsTableSchema = {.name = COMPOUND_PRIMARY_TABLE, .sharedTableName = COMPOUND_PRIMARY_TABLE_SHARED,
                         .fields = COMPOUND_PRIMARY_FIELDS};
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
        asset.name = ASSET_COPY.name + std::to_string(i) + "_copy";
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

void InsertLocalData(sqlite3 *&db, int64_t begin, int64_t count, const std::string &tableName, bool isAssetNull = true)
{
    int errCode;
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    GenerateDataRecords(begin, count, 0, record, extend);
    const string sql = "insert or replace into " + tableName + " values (?,?,?,?,?,?);";
    for (VBucket vBucket : record) {
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
        ASSERT_EQ(SQLiteUtils::BindInt64ToStatement(stmt, 1, std::get<int64_t>(vBucket[COL_ID])), E_OK); // 1 is id
        ASSERT_EQ(SQLiteUtils::BindTextToStatement(stmt, 2, std::get<string>(vBucket[COL_NAME])), E_OK); // 2 is name
        ASSERT_EQ(SQLiteUtils::MapSQLiteErrno(
            sqlite3_bind_double(stmt, 3, std::get<double>(vBucket[COL_HEIGHT]))), E_OK); // 3 is height
        if (isAssetNull) {
            ASSERT_EQ(sqlite3_bind_null(stmt, 4), SQLITE_OK); // 4 is asset
        } else {
            std::vector<uint8_t> assetBlob = g_virtualCloudDataTranslate->AssetToBlob(ASSET_COPY);
            ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 4, assetBlob, false), E_OK); // 4 is asset
        }
        std::vector<uint8_t> assetsBlob = g_virtualCloudDataTranslate->AssetsToBlob(
            std::get<Assets>(vBucket[COL_ASSETS]));
        ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 5, assetsBlob, false), E_OK); // 5 is assets
        ASSERT_EQ(SQLiteUtils::BindInt64ToStatement(stmt, 6, std::get<int64_t>(vBucket[COL_AGE])), E_OK); // 6 is age
        EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtils::ResetStatement(stmt, true, errCode);
    }
}

void UpdateLocalData(sqlite3 *&db, const std::string &tableName, const Assets &assets, bool isEmptyAssets = false)
{
    int errCode;
    std::vector<uint8_t> assetBlob;
    const string sql = "update " + tableName + " set assets=?;";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    if (isEmptyAssets) {
        ASSERT_EQ(sqlite3_bind_null(stmt, 1), SQLITE_OK);
    } else {
        assetBlob = g_virtualCloudDataTranslate->AssetsToBlob(assets);
        ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false), E_OK);
    }
    EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void UpdateLocalData(sqlite3 *&db, const std::string &tableName, const Assets &assets, int32_t begin, int32_t end)
{
    int errCode;
    std::vector<uint8_t> assetBlob;
    const string sql = "update " + tableName + " set assets=? " + "where id>=" + std::to_string(begin) +
        " and id<=" + std::to_string(end) + ";";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    assetBlob = g_virtualCloudDataTranslate->AssetsToBlob(assets);
    ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false), E_OK);
    EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void DeleteLocalRecord(sqlite3 *&db, int64_t begin, int64_t count, const std::string &tableName)
{
    ASSERT_NE(db, nullptr);
    for (int64_t i = begin; i < begin + count; i++) {
        string sql = "DELETE FROM " + tableName + " WHERE id ='" + std::to_string(i) + "';";
        ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    }
}

void DeleteCloudDBData(int64_t begin, int64_t count, const std::string &tableName)
{
    for (int64_t i = begin; i < begin + count; i++) {
        VBucket idMap;
        idMap.insert_or_assign("#_gid", std::to_string(i));
        ASSERT_EQ(g_virtualCloudDb->DeleteByGid(tableName, idMap), DBStatus::OK);
    }
}

void UpdateCloudDBData(int64_t begin, int64_t count, int64_t gidStart, int64_t versionStart,
    const std::string &tableName)
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

int QueryStatusCallback(void *data, int count, char **colValue, char **colName)
{
    auto status = static_cast<std::vector<int64_t> *>(data);
    const int decimal = 10;
    for (int i = 0; i < count; i++) {
        status->push_back(strtol(colValue[0], nullptr, decimal));
    }
    return 0;
}

void CheckLockStatus(sqlite3 *db, int startId, int endId, LockStatus lockStatus)
{
    std::string logName = DBCommon::GetLogTableName(ASSETS_TABLE_NAME);
    std::string sql = "select status from " + logName + " where data_key >=" + std::to_string(startId) +
        " and data_key <=" +  std::to_string(endId) + ";";
    std::vector<int64_t> status;
    char *str = NULL;
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), QueryStatusCallback, static_cast<void *>(&status), &str),
        SQLITE_OK);
    ASSERT_EQ(static_cast<size_t>(endId - startId + 1), status.size());

    for (auto stat : status) {
        ASSERT_EQ(static_cast<int64_t>(lockStatus), stat);
    }
}

void InsertCloudDBData(int64_t begin, int64_t count, int64_t gidStart, const std::string &tableName)
{
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    GenerateDataRecords(begin, count, gidStart, record, extend);
    if (tableName == ASSETS_TABLE_NAME_SHARED) {
        for (auto &vBucket: record) {
            vBucket.insert_or_assign(CloudDbConstant::CLOUD_OWNER, std::string("cloudA"));
        }
    }
    ASSERT_EQ(g_virtualCloudDb->BatchInsertWithGid(tableName, std::move(record), extend), DBStatus::OK);
}

void WaitForSyncFinish(SyncProcess &syncProcess, const int64_t &waitTime)
{
    std::unique_lock<std::mutex> lock(g_processMutex);
    bool result = g_processCondition.wait_for(
        lock, std::chrono::seconds(waitTime), [&syncProcess]() { return syncProcess.process == FINISHED; });
    ASSERT_EQ(result, true);
    LOGD("-------------------sync end--------------");
}

void CallSync(const std::vector<std::string> &tableNames, SyncMode mode, DBStatus dbStatus, DBStatus errCode = OK)
{
    g_syncProcess = {};
    Query query = Query::Select().FromTable(tableNames);
    std::vector<SyncProcess> expectProcess;
    CloudSyncStatusCallback callback = [&errCode](const std::map<std::string, SyncProcess> &process) {
        ASSERT_EQ(process.begin()->first, DEVICE_CLOUD);
        std::unique_lock<std::mutex> lock(g_processMutex);
        g_syncProcess = process.begin()->second;
        if (g_syncProcess.process == FINISHED) {
            g_processCondition.notify_one();
            ASSERT_EQ(g_syncProcess.errCode, errCode);
        }
    };
    CloudSyncOption option;
    option.devices = {DEVICE_CLOUD};
    option.mode = mode;
    option.query = query;
    option.waitTime = SYNC_WAIT_TIME;
    option.lockAction = static_cast<LockAction>(0xff); // lock all
    ASSERT_EQ(g_delegate->Sync(option, callback), dbStatus);

    if (dbStatus == DBStatus::OK) {
        WaitForSyncFinish(g_syncProcess, SYNC_WAIT_TIME);
    }
}

void CheckDownloadForTest001(int index, map<std::string, Assets> &assets)
{
    for (auto &item : assets) {
        for (auto &asset : item.second) {
            EXPECT_EQ(AssetOperationUtils::EraseBitMask(asset.status), static_cast<uint32_t>(AssetStatus::INSERT));
            if (index < 4) { // 1-4 is inserted
                EXPECT_EQ(asset.flag, static_cast<uint32_t>(AssetOpType::INSERT));
            }
            LOGD("asset [name]:%s, [status]:%u, [flag]:%u, [index]:%d", asset.name.c_str(), asset.status, asset.flag,
                index);
        }
    }
}

void CheckDownloadFailedForTest002(sqlite3 *&db)
{
    std::string sql = "SELECT assets from " + ASSETS_TABLE_NAME;
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
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
            EXPECT_EQ(assets[i].status, AssetStatus::ABNORMAL);
        }
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void UpdateAssetsForLocal(sqlite3 *&db, int id, uint32_t status)
{
    Assets assets;
    Asset asset = ASSET_COPY;
    asset.name = ASSET_COPY.name + std::to_string(id);
    asset.status = status;
    assets.emplace_back(asset);
    asset.name = ASSET_COPY.name + std::to_string(id) + "_copy";
    assets.emplace_back(asset);
    int errCode;
    std::vector<uint8_t> assetBlob;
    const string sql = "update " + ASSETS_TABLE_NAME + " set assets=? where id = " + std::to_string(id);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    assetBlob = g_virtualCloudDataTranslate->AssetsToBlob(assets);
    ASSERT_EQ(SQLiteUtils::BindBlobToStatement(stmt, 1, assetBlob, false), E_OK);
    EXPECT_EQ(SQLiteUtils::StepWithRetry(stmt), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void CheckConsistentCount(sqlite3 *db, int64_t expectCount)
{
    EXPECT_EQ(sqlite3_exec(db, QUERY_CONSISTENT_SQL.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(expectCount), nullptr), SQLITE_OK);
}

void CheckCompensatedCount(sqlite3 *db, int64_t expectCount)
{
    EXPECT_EQ(sqlite3_exec(db, QUERY_COMPENSATED_SQL.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(expectCount), nullptr), SQLITE_OK);
}

void CloseDb()
{
    if (g_delegate != nullptr) {
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
        g_delegate = nullptr;
    }
    delete g_observer;
    g_virtualCloudDb = nullptr;
}

class DistributedDBCloudSyncerDownloadAssetsOnlyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    void CheckLocaLAssets(const std::string &tableName, const std::string &expectAssetId,
        const std::set<int> &failIndex);
    void CheckLocalAssetIsEmpty(const std::string &tableName);
    void CheckCursorData(const std::string &tableName, int begin);
    void WaitForSync(int &syncCount);
    const RelationalSyncAbleStorage *GetRelationalStore();
    void InitDataStatusTest(bool needDownload);
    void DataStatusTest001(bool needDownload);
    void DataStatusTest003();
    void DataStatusTest004();
    void DataStatusTest005();
    void DataStatusTest006();
    void DataStatusTest007();
    sqlite3 *db = nullptr;
    VirtualCommunicatorAggregator *communicatorAggregator_ = nullptr;
};

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/" + STORE_ID + DB_SUFFIX;
    LOGI("The test db is:%s", g_storePath.c_str());
    g_virtualCloudDataTranslate = std::make_shared<VirtualCloudDataTranslate>();
    RuntimeConfig::SetCloudTranslate(g_virtualCloudDataTranslate);
}

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::TearDownTestCase(void) {}

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::SetUp(void)
{
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(false);
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
    ASSERT_EQ(g_delegate->CreateDistributedTable(NO_PRIMARY_TABLE, CLOUD_COOPERATION), DBStatus::OK);
    ASSERT_EQ(g_delegate->CreateDistributedTable(COMPOUND_PRIMARY_TABLE, CLOUD_COOPERATION), DBStatus::OK);
    g_virtualCloudDb = make_shared<VirtualCloudDb>();
    g_virtualAssetLoader = make_shared<VirtualAssetLoader>();
    g_syncProcess = {};
    ASSERT_EQ(g_delegate->SetCloudDB(g_virtualCloudDb), DBStatus::OK);
    ASSERT_EQ(g_delegate->SetIAssetLoader(g_virtualAssetLoader), DBStatus::OK);
    DataBaseSchema dataBaseSchema;
    GetCloudDbSchema(dataBaseSchema);
    ASSERT_EQ(g_delegate->SetCloudDbSchema(dataBaseSchema), DBStatus::OK);
    g_cloudStoreHook = (ICloudSyncStorageHook *) GetRelationalStore();
    ASSERT_NE(g_cloudStoreHook, nullptr);
    communicatorAggregator_ = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator_ != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator_);
}

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::TearDown(void)
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

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::CheckLocaLAssets(const std::string &tableName,
    const std::string &expectAssetId, const std::set<int> &failIndex)
{
    std::string sql = "SELECT assets FROM " + tableName + ";";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    int index = 0;
    while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_BLOB);
        Type cloudValue;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Assets>, 0, cloudValue), E_OK);
        Assets assets = g_virtualCloudDataTranslate->BlobToAssets(std::get<Bytes>(cloudValue));
        for (const auto &asset : assets) {
            index++;
            if (failIndex.find(index) != failIndex.end()) {
                EXPECT_EQ(asset.assetId, "0");
            } else {
                EXPECT_EQ(asset.assetId, expectAssetId);
            }
        }
    }
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::CheckLocalAssetIsEmpty(const std::string &tableName)
{
    std::string sql = "SELECT asset FROM " + tableName + ";";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_NULL);
    }
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::CheckCursorData(const std::string &tableName, int begin)
{
    std::string logTableName = DBCommon::GetLogTableName(tableName);
    std::string sql = "SELECT cursor FROM " + logTableName + ";";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_INTEGER);
        Type cloudValue;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Assets>, 0, cloudValue), E_OK);
        EXPECT_EQ(std::get<int64_t>(cloudValue), begin);
        begin++;
    }
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::WaitForSync(int &syncCount)
{
    std::unique_lock<std::mutex> lock(g_processMutex);
    bool result = g_processCondition.wait_for(lock, std::chrono::seconds(COMPENSATED_SYNC_WAIT_TIME),
        [&syncCount]() { return syncCount == 2; }); // 2 is compensated sync
    ASSERT_EQ(result, true);
}

const RelationalSyncAbleStorage* DistributedDBCloudSyncerDownloadAssetsOnlyTest::GetRelationalStore()
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

void DistributedDBCloudSyncerDownloadAssetsOnlyTest::InitDataStatusTest(bool needDownload)
{
    int cloudCount = 20;
    int localCount = 10;
    InsertLocalData(db, 0, cloudCount, ASSETS_TABLE_NAME, true);
    if (needDownload) {
        UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    }
    std::string logName = DBCommon::GetLogTableName(ASSETS_TABLE_NAME);
    std::string sql = "update " + logName + " SET status = 1 where data_key in (1,11);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    sql = "update " + logName + " SET status = 2 where data_key in (2,12);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    sql = "update " + logName + " SET status = 3 where data_key in (3,13);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    InsertCloudDBData(0, localCount, 0, ASSETS_TABLE_NAME);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    sql = "update " + ASSETS_TABLE_NAME + " set age='666' where id in (4);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    sql = "update " + logName + " SET status = 1 where data_key in (4);";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
}

void PriorityLevelSync(int32_t priorityLevel, const Query &query, const CloudSyncStatusCallback &onFinish,
    SyncMode mode, DBStatus expectResult = DBStatus::OK)
{
    std::mutex processMutex;
    std::vector<SyncProcess> expectProcess;
    std::condition_variable cv;
    bool finish = false;
    auto callback = [&cv, &onFinish, &finish, &processMutex, &expectResult]
        (const std::map<std::string, SyncProcess> &process) {
        for (auto &item : process) {
            if (item.second.process == FINISHED) {
                if (onFinish) {
                    onFinish(process);
                }
                EXPECT_EQ(item.second.errCode, expectResult);
                std::unique_lock<std::mutex> lock(processMutex);
                finish = true;
                cv.notify_one();
            }
        }
    };
    CloudSyncOption option;
    option.devices = {DEVICE_CLOUD};
    option.query = query;
    option.mode = mode;
    option.priorityTask = true;
    option.priorityLevel = priorityLevel;
    DBStatus syncResult = g_delegate->Sync(option, callback);
    EXPECT_EQ(syncResult, DBStatus::OK);

    std::unique_lock<std::mutex> lock(processMutex);
    cv.wait(lock, [&finish]() {
        return finish;
    });
}

void CheckAsset(sqlite3 *db, const std::string &tableName, int id, const Asset &expectAsset, bool expectFound)
{
    std::string sql = "select assets from " + tableName + " where id = " + std::to_string(id);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    int errCode = SQLiteUtils::StepWithRetry(stmt);
    ASSERT_EQ(errCode, SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    if (expectFound) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_BLOB);
    }
    Type cloudValue;
    ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Assets>, 0, cloudValue), E_OK);
    Assets assets = g_virtualCloudDataTranslate->BlobToAssets(std::get<Bytes>(cloudValue));
    bool found = false;
    for (const auto &asset : assets) {
        if (asset.name != expectAsset.name) {
            continue;
        }
        found = true;
        EXPECT_EQ(asset.status, expectAsset.status);
        EXPECT_EQ(asset.hash, expectAsset.hash);
        EXPECT_EQ(asset.assetId, expectAsset.assetId);
        EXPECT_EQ(asset.uri, expectAsset.uri);
    }
    EXPECT_EQ(found, expectFound);
    errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    EXPECT_EQ(errCode, E_OK);
}

void CheckDBValue(sqlite3 *db, const std::string &tableName, int id, const std::string &field,
    const std::string &expectValue)
{
    std::string sql = "select " + field + " from " + tableName + " where id = " + std::to_string(id);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    int errCode = SQLiteUtils::StepWithRetry(stmt);
    if (expectValue.empty()) {
        EXPECT_EQ(errCode, SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    }
    ASSERT_EQ(errCode, SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));
    std::string str;
    (void)SQLiteUtils::GetColumnTextValue(stmt, 0, str);
    EXPECT_EQ(str, expectValue);
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    errCode = E_OK;
    EXPECT_EQ(errCode, E_OK);
}

/**
  * @tc.name: DownloadAssetsOnly001
  * @tc.desc: Test sync with priorityLevel
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 15; // 15 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    /**
     * @tc.steps:step2. Call sync with different priorityLevel
     * @tc.expected: step2. OK
     */
    int syncFinishCount = 0;
    g_virtualCloudDb->SetBlockTime(100);
    std::thread syncThread1([&]() {
        CloudSyncStatusCallback callback = [&syncFinishCount](const std::map<std::string, SyncProcess> &process) {
            syncFinishCount++;
            EXPECT_EQ(syncFinishCount, 3);
        };
        std::vector<int64_t> inValue = {0, 1, 2, 3, 4};
        Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue);
        PriorityLevelSync(1, query, callback, SyncMode::SYNC_MODE_CLOUD_MERGE);
    });

    std::thread syncThread2([&]() {
        CloudSyncStatusCallback callback = [&syncFinishCount](const std::map<std::string, SyncProcess> &process) {
            syncFinishCount++;
            EXPECT_EQ(syncFinishCount, 2);
        };
        std::vector<int64_t> inValue = {5, 6, 7, 8, 9};
        Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue);
        PriorityLevelSync(2, query, callback, SyncMode::SYNC_MODE_CLOUD_MERGE);
    });

    std::thread syncThread3([&]() {
        CloudSyncStatusCallback callback = [&syncFinishCount](const std::map<std::string, SyncProcess> &process) {
            syncFinishCount++;
            EXPECT_EQ(syncFinishCount, 1);
        };
        std::vector<int64_t> inValue = {10, 11, 12, 13, 14};
        Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue);
        PriorityLevelSync(3, query, callback, SyncMode::SYNC_MODE_CLOUD_MERGE);
    });
    syncThread1.join();
    syncThread2.join();
    syncThread3.join();
}

/**
  * @tc.name: DownloadAssetsOnly002
  * @tc.desc: Test download specified assets with unsupported mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int localDataCount = 10;
    InsertLocalData(db, 0, localDataCount, ASSETS_TABLE_NAME, true);
    UpdateLocalData(db, ASSETS_TABLE_NAME, {ASSET_COPY}, true);
    int cloudCount = 10;
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    /**
     * @tc.steps:step2. Download specified assets with mode SYNC_MODE_CLOUD_MERGE and SYNC_MODE_CLOUD_FORCE_PUSH
     * @tc.expected: step2. sync fail
    */
    std::vector<int64_t> inValue = {0};
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue).AssetsOnly(assets);

    CloudSyncOption option;
    option.devices = {DEVICE_CLOUD};
    option.query = query;
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.priorityTask = true;
    option.priorityLevel = 0u;
    EXPECT_EQ(g_delegate->Sync(option, nullptr), DBStatus::NOT_SUPPORT);

    option.mode = SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH;
    EXPECT_EQ(g_delegate->Sync(option, nullptr), DBStatus::NOT_SUPPORT);
}

/**
  * @tc.name: DownloadAssetsOnly004
  * @tc.desc: Test download specified assets
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    for (int i = 0; i < dataCount; i++) {
        Asset asset = ASSET_COPY;
        asset.name += std::to_string(i);
        asset.status = AssetStatus::UPDATE;
        asset.hash = "local_new";
        Assets assets = {asset};
        asset.name += "_new";
        assets.push_back(asset);
        UpdateLocalData(db, ASSETS_TABLE_NAME, assets, i, i);
    }
    /**
     * @tc.steps:step2. Download specified assets
     * @tc.expected: step2. return OK.
     */
    std::vector<int64_t> inValue = {0};
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue).AssetsOnly(assets);
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);

    Asset assetCloud = ASSET_COPY;
    assetCloud.name += std::to_string(0);
    Asset assetLocal = ASSET_COPY;
    assetLocal.name +=std::to_string(0) + "_new";
    assetLocal.hash = "local_new";
    assetLocal.status = AssetStatus::UPDATE;
    CheckAsset(db, ASSETS_TABLE_NAME, 0, assetCloud, true);
    CheckAsset(db, ASSETS_TABLE_NAME, 0, assetLocal, true);

    for (int i = 1; i < dataCount; i++) {
        Asset assetLocal1 = ASSET_COPY;
        assetLocal1.name += std::to_string(i);
        Asset assetLocal2 = ASSET_COPY;
        assetLocal2.name +=std::to_string(i) + "_new";
        assetLocal1.hash = "local_new";
        assetLocal2.hash = "local_new";
        assetLocal1.status = AssetStatus::UPDATE;
        assetLocal2.status = AssetStatus::UPDATE;
        CheckAsset(db, ASSETS_TABLE_NAME, i, assetLocal1, true);
        CheckAsset(db, ASSETS_TABLE_NAME, i, assetLocal2, true);
    }
}

/**
  * @tc.name: DownloadAssetsOnly004
  * @tc.desc: Test download specified assets
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    GenerateDataRecords(0, dataCount, 0, record, extend);
    for (int i = 0; i < dataCount; i++) {
        Asset asset1 = ASSET_COPY;
        Asset asset2 = ASSET_COPY;
        asset1.name += std::to_string(i);
        asset2.name += std::to_string(i) + "_new";
        asset1.hash = "cloud";
        asset2.hash = "cloud";
        Assets assets = {asset1, asset2};
        record[i].insert_or_assign(COL_ASSETS, assets);
        std::string newName = "name" + std::to_string(i) + "_new";
        record[i].insert_or_assign(COL_NAME, newName);
    }
    ASSERT_EQ(g_virtualCloudDb->BatchUpdate(ASSETS_TABLE_NAME, std::move(record), extend), DBStatus::OK);
    /**
     * @tc.steps:step2. Download specified assets
     * @tc.expected: step2. return OK.
     */
    std::vector<int64_t> inValue = {0};
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue).AssetsOnly(assets);
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);

    Asset assetCloud1 = ASSET_COPY;
    assetCloud1.name += std::to_string(0);
    assetCloud1.hash = "cloud";
    Asset assetCloud2 = ASSET_COPY;
    assetCloud2.name +=std::to_string(0) + "_new";
    assetCloud2.hash = "cloud";
    CheckAsset(db, ASSETS_TABLE_NAME, 0, assetCloud1, true);
    CheckAsset(db, ASSETS_TABLE_NAME, 0, assetCloud2, false);
    CheckDBValue(db, ASSETS_TABLE_NAME, 0, COL_NAME, "name0");

    for (int i = 1; i < dataCount; i++) {
        Asset assetLocal1 = ASSET_COPY;
        assetLocal1.name += std::to_string(i);
        Asset assetLocal2 = ASSET_COPY;
        assetLocal2.name +=std::to_string(i) + "_new";
        CheckAsset(db, ASSETS_TABLE_NAME, i, assetLocal1, true);
        CheckAsset(db, ASSETS_TABLE_NAME, i, assetLocal2, false);
        CheckDBValue(db, ASSETS_TABLE_NAME, i, COL_NAME, "name" + std::to_string(i));
    }
}

/**
  * @tc.name: DownloadAssetsOnly005
  * @tc.desc: Test download asseets which local no found
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    InsertCloudDBData(dataCount, 1, 0, ASSETS_TABLE_NAME);
    /**
     * @tc.steps:step2. Download assets which local no found
     * @tc.expected: step2. return ASSET_NOT_FOUND_FOR_DOWN_ONLY.
     */
    std::vector<int64_t> inValue = {1,2,3,4,5,6,7,8,9,10};
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "10"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue).AssetsOnly(assets);
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::ASSET_NOT_FOUND_FOR_DOWN_ONLY);
}

/**
  * @tc.name: DownloadAssetsOnly006
  * @tc.desc: Test download asseets which cloud no found
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly006, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    InsertLocalData(db, dataCount, 1, ASSETS_TABLE_NAME, true);
    /**
     * @tc.steps:step2. Download assets which cloud no found
     * @tc.expected: step2. return ASSET_NOT_FOUND_FOR_DOWN_ONLY.
     */
    std::vector<int64_t> inValue = {1,2,3,4,5,6,7,8,9,10};
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "10"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue).AssetsOnly(assets);
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::ASSET_NOT_FOUND_FOR_DOWN_ONLY);
}

/**
  * @tc.name: DownloadAssetsOnly007
  * @tc.desc: Test download specified assets with group
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    for (int i = 0; i < dataCount; i++) {
        Asset asset = ASSET_COPY;
        asset.name += std::to_string(i);
        asset.status = AssetStatus::UPDATE;
        asset.hash = "local_new";
        Assets assets = {asset};
        asset.name += "_new";
        assets.push_back(asset);
        UpdateLocalData(db, ASSETS_TABLE_NAME, assets, i, i);
    }
    /**
     * @tc.steps:step2. Download specified assets
     * @tc.expected: step2. return OK.
     */
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    std::map<std::string, std::set<std::string>> assets1;
    assets1["assets"] = {ASSET_COPY.name + "1"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).AssetsOnly(assets).EndGroup().
        Or().BeginGroup().EqualTo("id", 1).AssetsOnly(assets1).EndGroup();
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);

    Asset assetCloud = ASSET_COPY;
    assetCloud.name += std::to_string(0);
    Asset assetLocal = ASSET_COPY;
    assetLocal.name +=std::to_string(0) + "_new";
    assetLocal.hash = "local_new";
    assetLocal.status = AssetStatus::UPDATE;
    CheckAsset(db, ASSETS_TABLE_NAME, 0, assetCloud, true);
    CheckAsset(db, ASSETS_TABLE_NAME, 0, assetLocal, true);

    for (int i = 2; i < dataCount; i++) {
        Asset assetLocal1 = ASSET_COPY;
        assetLocal1.name += std::to_string(i);
        Asset assetLocal2 = ASSET_COPY;
        assetLocal2.name +=std::to_string(i) + "_new";
        assetLocal1.hash = "local_new";
        assetLocal2.hash = "local_new";
        assetLocal1.status = AssetStatus::UPDATE;
        assetLocal2.status = AssetStatus::UPDATE;
        CheckAsset(db, ASSETS_TABLE_NAME, i, assetLocal1, true);
        CheckAsset(db, ASSETS_TABLE_NAME, i, assetLocal2, true);
    }
}

/**
  * @tc.name: DownloadAssetsOnly008
  * @tc.desc: Test download asseets which local no found
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    InsertCloudDBData(dataCount, 1, 0, ASSETS_TABLE_NAME);
    /**
     * @tc.steps:step2. Download assets which local no found
     * @tc.expected: step2. return ASSET_NOT_FOUND_FOR_DOWN_ONLY.
     */
    std::vector<int64_t> inValue = {1,2,3,4,5,6,7,8,9,10};
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    std::map<std::string, std::set<std::string>> assets1;
    assets1["assets"] = {ASSET_COPY.name + "10"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).AssetsOnly(assets).EndGroup().
        Or().BeginGroup().In("id", inValue).AssetsOnly(assets1).EndGroup();
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::ASSET_NOT_FOUND_FOR_DOWN_ONLY);
}

/**
  * @tc.name: DownloadAssetsOnly009
  * @tc.desc: Test download asseets which cloud no found
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly009, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    InsertLocalData(db, dataCount, 1, ASSETS_TABLE_NAME, true);
    /**
     * @tc.steps:step2. Download assets which cloud no found
     * @tc.expected: step2. return ASSET_NOT_FOUND_FOR_DOWN_ONLY.
     */
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    std::map<std::string, std::set<std::string>> assets1;
    assets1["assets"] = {ASSET_COPY.name + "10"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).AssetsOnly(assets).EndGroup().
        Or().BeginGroup().EqualTo("id", 10).AssetsOnly(assets1).EndGroup();
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::ASSET_NOT_FOUND_FOR_DOWN_ONLY);
}

/**
  * @tc.name: DownloadAssetsOnly010
  * @tc.desc: Test assets only multi time.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly010, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(true);
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);

    /**
     * @tc.steps:step2. AssetsOnly twice
     * @tc.expected: step2. check notify count.
     */
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).AssetsOnly(assets).
        AssetsOnly(assets).EndGroup();
    g_observer->ResetCloudSyncToZero();
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);
    auto changedData = g_observer->GetSavedChangedData();
    EXPECT_EQ(changedData.size(), 0u);

    /**
     * @tc.steps:step3. AssetsOnly behine EndGroup
     * @tc.expected: step3. check notify count.
     */
    Query query1 = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).EndGroup().AssetsOnly(assets);
    g_observer->ResetCloudSyncToZero();
    PriorityLevelSync(0, query1, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);
    changedData = g_observer->GetSavedChangedData();
    EXPECT_EQ(changedData.size(), 0u);

    /**
     * @tc.steps:step4. AssetsOnly EndGroup use And
     * @tc.expected: step4. check notify count.
     */
    Query query2 = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).AssetsOnly(assets).EndGroup()
    .And().BeginGroup().EqualTo("id", 0).AssetsOnly(assets).EndGroup();
    g_observer->ResetCloudSyncToZero();
    PriorityLevelSync(0, query2, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);
    changedData = g_observer->GetSavedChangedData();
    EXPECT_EQ(changedData.size(), 0u);
}

/**
  * @tc.name: DownloadAssetsOnly011
  * @tc.desc: Check assets only sync will up.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly011, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(true);
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);

    /**
     * @tc.steps:step2. assets only sync
     * @tc.expected: step2. check assets sync result.
     */
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).AssetsOnly(assets).EndGroup();
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);

    /**
     * @tc.steps:step3. check cursor and flag
     * @tc.expected: step3. ok.
     */
    std::string sql = "select cursor from naturalbase_rdb_aux_student_log where data_key=0;";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(21u), nullptr), SQLITE_OK);

    sql = "select flag from naturalbase_rdb_aux_student_log where data_key=0;";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0u), nullptr), SQLITE_OK);
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(false);
}

/**
  * @tc.name: DownloadAssetsOnly012
  * @tc.desc: Test sync with same priorityLevel should be sync in order.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly012, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 15; // 15 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    /**
     * @tc.steps:step2. Call sync with same priorityLevel
     * @tc.expected: step2. OK
     */
    int syncFinishCount = 0;
    g_virtualCloudDb->SetBlockTime(1000);
    std::thread syncThread1([&]() {
        CloudSyncStatusCallback callback = [&syncFinishCount](const std::map<std::string, SyncProcess> &process) {
            syncFinishCount++;
            EXPECT_EQ(syncFinishCount, 1);
        };
        std::vector<int64_t> inValue = {0, 1, 2, 3, 4};
        Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue);
        PriorityLevelSync(0, query, callback, SyncMode::SYNC_MODE_CLOUD_MERGE);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::thread syncThread2([&]() {
        CloudSyncStatusCallback callback = [&syncFinishCount](const std::map<std::string, SyncProcess> &process) {
            syncFinishCount++;
            EXPECT_EQ(syncFinishCount, 2);
        };
        std::vector<int64_t> inValue = {5, 6, 7, 8, 9};
        Query query = Query::Select().From(ASSETS_TABLE_NAME).In("id", inValue);
        PriorityLevelSync(0, query, callback, SyncMode::SYNC_MODE_CLOUD_MERGE);
    });
    syncThread1.join();
    syncThread2.join();
}

/**
  * @tc.name: DownloadAssetsOnly013
  * @tc.desc: Check assets only sync no data notify.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly013, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(true);
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);

    /**
     * @tc.steps:step2. assets only sync
     * @tc.expected: step2. check notify count.
     */
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).AssetsOnly(assets).EndGroup();
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);
    auto changedData = g_observer->GetSavedChangedData();
    EXPECT_EQ(changedData.size(), 1u);
}

/**
  * @tc.name: DownloadAssetsOnly014
  * @tc.desc: test assets only sync with cloud delete data.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: luoguo
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsOnlyTest, DownloadAssetsOnly014, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 10;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    DeleteCloudDBData(0, 1, ASSETS_TABLE_NAME);
    /**
     * @tc.steps:step2. Download assets which cloud delete.
     * @tc.expected: step2. return ASSET_NOT_FOUND_FOR_DOWN_ONLY.
     */
    std::map<std::string, std::set<std::string>> assets;
    assets["assets"] = {ASSET_COPY.name + "0"};
    Query query = Query::Select().From(ASSETS_TABLE_NAME).BeginGroup().EqualTo("id", 0).AssetsOnly(assets).EndGroup();
    PriorityLevelSync(0, query, nullptr, SyncMode::SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::ASSET_NOT_FOUND_FOR_DOWN_ONLY);
}
} // namespace
#endif // RELATIONAL_STORE
