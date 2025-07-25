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

std::pair<std::vector<VBucket>, std::vector<VBucket>> GenerateDataRecords(int64_t begin, int64_t count,
    int64_t gidStart, bool hasAsset)
{
    std::pair<std::vector<VBucket>, std::vector<VBucket>> res;
    auto &[record, extend] = res;
    for (int64_t i = begin; i < begin + count; i++) {
        Assets assets;
        if (hasAsset) {
            Asset asset = ASSET_COPY;
            asset.name = ASSET_COPY.name + std::to_string(i);
            assets.emplace_back(asset);
            asset.name = ASSET_COPY.name + std::to_string(i) + "_copy";
            assets.emplace_back(asset);
        }
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
    return res;
}

void GenerateDataRecords(
    int64_t begin, int64_t count, int64_t gidStart, std::vector<VBucket> &record, std::vector<VBucket> &extend)
{
    std::tie(record, extend) = GenerateDataRecords(begin, count, gidStart, true);
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

void InsertCloudDBData(int64_t begin, int64_t count, int64_t gidStart, const std::string &tableName,
    bool hasAsset = true)
{
    auto [record, extend] = GenerateDataRecords(begin, count, gidStart, hasAsset);
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

void CallSync(const std::vector<std::string> &tableNames, SyncMode mode, DBStatus dbStatus, DBStatus errCode = OK,
    bool isMerge = false)
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
    option.merge = isMerge;
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

class DistributedDBCloudSyncerDownloadAssetsTest : public testing::Test {
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

void DistributedDBCloudSyncerDownloadAssetsTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/" + STORE_ID + DB_SUFFIX;
    LOGI("The test db is:%s", g_storePath.c_str());
    g_virtualCloudDataTranslate = std::make_shared<VirtualCloudDataTranslate>();
    RuntimeConfig::SetCloudTranslate(g_virtualCloudDataTranslate);
}

void DistributedDBCloudSyncerDownloadAssetsTest::TearDownTestCase(void) {}

void DistributedDBCloudSyncerDownloadAssetsTest::SetUp(void)
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

void DistributedDBCloudSyncerDownloadAssetsTest::TearDown(void)
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

void DistributedDBCloudSyncerDownloadAssetsTest::CheckLocaLAssets(const std::string &tableName,
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

void DistributedDBCloudSyncerDownloadAssetsTest::CheckLocalAssetIsEmpty(const std::string &tableName)
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

void DistributedDBCloudSyncerDownloadAssetsTest::CheckCursorData(const std::string &tableName, int begin)
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

void DistributedDBCloudSyncerDownloadAssetsTest::WaitForSync(int &syncCount)
{
    std::unique_lock<std::mutex> lock(g_processMutex);
    bool result = g_processCondition.wait_for(lock, std::chrono::seconds(COMPENSATED_SYNC_WAIT_TIME),
        [&syncCount]() { return syncCount == 2; }); // 2 is compensated sync
    ASSERT_EQ(result, true);
}

const RelationalSyncAbleStorage* DistributedDBCloudSyncerDownloadAssetsTest::GetRelationalStore()
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

void DistributedDBCloudSyncerDownloadAssetsTest::InitDataStatusTest(bool needDownload)
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

void DistributedDBCloudSyncerDownloadAssetsTest::DataStatusTest001(bool needDownload)
{
    int cloudCount = 20;
    int count = 0;
    g_cloudStoreHook->SetSyncFinishHook([&count, cloudCount, this]() {
        count++;
        if (count == 1) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (2,3,12,13)) or (status = 1 and data_key in (11, 4)) or (status = 0)";
            CloudDBSyncUtilsTest::CheckCount(db, sql, cloudCount);
        }
        if (count == 2) { // 2 is compensated sync
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (2,3,12,13)) or (status = 0)";
            CloudDBSyncUtilsTest::CheckCount(db, sql, cloudCount);
            g_processCondition.notify_one();
        }
    });
    InitDataStatusTest(needDownload);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    WaitForSync(count);
}

void DistributedDBCloudSyncerDownloadAssetsTest::DataStatusTest003()
{
    int count = 0;
    g_cloudStoreHook->SetSyncFinishHook([&count, this]() {
        count++;
        if (count == 1) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (0,2,3,12,13)) or (status = 0 and data_key = 11)";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 6); // 6 is match count
        }
        if (count == 2) { // 2 is compensated sync
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (0,2,3,12,13) or (status = 0))";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 20); // 20 is match count
            g_processCondition.notify_one();
        }
    });
    int downLoadCount = 0;
    g_virtualAssetLoader->ForkDownload([this, &downLoadCount](const std::string &tableName,
        std::map<std::string, Assets> &assets) {
        downLoadCount++;
        if (downLoadCount == 1) {
            std::vector<std::vector<uint8_t>> hashKey;
            CloudDBSyncUtilsTest::GetHashKey(ASSETS_TABLE_NAME, " data_key = 0 ", db, hashKey);
            EXPECT_EQ(Lock(ASSETS_TABLE_NAME, hashKey, db), OK);
        }
    });
    InitDataStatusTest(true);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    WaitForSync(count);
    g_virtualAssetLoader->ForkDownload(nullptr);
}

void DistributedDBCloudSyncerDownloadAssetsTest::DataStatusTest004()
{
    int count = 0;
    g_cloudStoreHook->SetSyncFinishHook([&count, this]() {
        count++;
        if (count == 1) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (2,3,12,13)) or (status = 1 and data_key in (-1,11))";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 5); // 5 is match count
        }
        if (count == 2) { // 2 is compensated sync
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (2,3,12,13)) or (status = 0)";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 19); // 19 is match count
            g_processCondition.notify_one();
        }
    });
    int downLoadCount = 0;
    g_virtualAssetLoader->ForkDownload([this, &downLoadCount](const std::string &tableName,
        std::map<std::string, Assets> &assets) {
        downLoadCount++;
        if (downLoadCount == 1) {
            std::vector<std::vector<uint8_t>> hashKey;
            CloudDBSyncUtilsTest::GetHashKey(ASSETS_TABLE_NAME, " data_key = 0 ", db, hashKey);
            EXPECT_EQ(Lock(ASSETS_TABLE_NAME, hashKey, db), OK);
            std::string sql = "delete from " + ASSETS_TABLE_NAME + " WHERE id=0";
            EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        }
    });
    int queryIdx = 0;
    g_virtualCloudDb->ForkQuery([this, &queryIdx](const std::string &, VBucket &) {
        LOGD("query index:%d", ++queryIdx);
        if (queryIdx == 4) { // 4 is compensated sync
            std::string sql = "update " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) +
                " SET status = 1 where data_key=15;";
            EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        }
    });
    InitDataStatusTest(true);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    WaitForSync(count);
    g_virtualAssetLoader->ForkDownload(nullptr);
    g_virtualCloudDb->ForkQuery(nullptr);
}

void DistributedDBCloudSyncerDownloadAssetsTest::DataStatusTest005()
{
    int count = 0;
    g_cloudStoreHook->SetSyncFinishHook([&count, this]() {
        count++;
        if (count == 1) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (0,2,3,12,13)) or (status = 0 and data_key in (11))";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 6); // 6 is match count
        }
        if (count == 2) { // 2 is compensated sync
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (0,2,3,12,13)) or (status = 0)";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 20); // 20 is match count
            g_processCondition.notify_one();
        }
    });
    int downLoadCount = 0;
    g_virtualAssetLoader->ForkDownload([this, &downLoadCount](const std::string &tableName,
        std::map<std::string, Assets> &assets) {
        downLoadCount++;
        if (downLoadCount == 1) {
            std::vector<std::vector<uint8_t>> hashKey;
            CloudDBSyncUtilsTest::GetHashKey(ASSETS_TABLE_NAME, " data_key = 0 ", db, hashKey);
            EXPECT_EQ(Lock(ASSETS_TABLE_NAME, hashKey, db), OK);
            std::string sql = "update " + ASSETS_TABLE_NAME + " set name='x' WHERE id=0";
            EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
        }
    });
    InitDataStatusTest(true);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    WaitForSync(count);
    g_virtualAssetLoader->ForkDownload(nullptr);
}

void DistributedDBCloudSyncerDownloadAssetsTest::DataStatusTest006()
{
    int count = 0;
    g_cloudStoreHook->SetSyncFinishHook([&count, this]() {
        count++;
        if (count == 1) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (2,3,12,13)) or (status = 1 and data_key in (0)) or "
                "(status = 0 and data_key in (11))";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 6); // 6 is match count
        }
        if (count == 2) { // 2 is compensated sync
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (2,3,12,13)) or (status = 0)";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 20); // 20 is match count
            g_processCondition.notify_one();
        }
    });
    int downLoadCount = 0;
    g_virtualAssetLoader->ForkDownload([this, &downLoadCount](const std::string &tableName,
        std::map<std::string, Assets> &assets) {
        downLoadCount++;
        if (downLoadCount == 1) {
            std::vector<std::vector<uint8_t>> hashKey;
            CloudDBSyncUtilsTest::GetHashKey(ASSETS_TABLE_NAME, " data_key = 0 ", db, hashKey);
            EXPECT_EQ(Lock(ASSETS_TABLE_NAME, hashKey, db), OK);
            std::string sql = "update " + ASSETS_TABLE_NAME + " set name='x' WHERE id=0";
            EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
            EXPECT_EQ(UnLock(ASSETS_TABLE_NAME, hashKey, db), WAIT_COMPENSATED_SYNC);
        }
    });
    InitDataStatusTest(true);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    WaitForSync(count);
    g_virtualAssetLoader->ForkDownload(nullptr);
}

void DistributedDBCloudSyncerDownloadAssetsTest::DataStatusTest007()
{
    int count = 0;
    g_cloudStoreHook->SetSyncFinishHook([&count, this]() {
        count++;
        if (count == 1) {
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (2,3,13)) or (status = 1 and data_key in (1,11))";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 5); // 5 is match count
        }
        if (count == 2) { // 2 is compensated sync
            std::string sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " WHERE "
                " (status = 3 and data_key in (2,3,13)) or (status = 1 and data_key in (1,11))";
            CloudDBSyncUtilsTest::CheckCount(db, sql, 5); // 5 is match count
            g_processCondition.notify_one();
        }
    });
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly([](const std::string &, const std::string &gid, const Type &,
            std::map<std::string, Assets> &assets) {
            return CLOUD_ERROR;
        });
    InitDataStatusTest(true);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);
    WaitForSync(count);
}

/*
 * @tc.name: DownloadAssetForDupDataTest001
 * @tc.desc: Test the download interface call with duplicate data for the same primary key.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liufuchenxing
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetForDupDataTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Mock asset download interface.
     * @tc.expected: step1. return OK and interface will be called 4 times. delete 1, delete 2, insert 1, insert 2
     */
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 1;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .Times(2)
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
    InsertLocalData(db, 0, 10, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step3. delete cloud data [1, 2], then insert cloud data [1,2] with new gid. Finally sync data.
     * @tc.expected: step3. sync success.
     */
    DeleteCloudDBData(1, 2, ASSETS_TABLE_NAME);
    InsertCloudDBData(1, 2, 10, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
}

/**
 * @tc.name: FillAssetId001
 * @tc.desc: Test if assetId is filled in single primary key table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocalAssetIsEmpty(ASSETS_TABLE_NAME);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId002
 * @tc.desc: Test if assetId is filled in no primary key table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, NO_PRIMARY_TABLE);
    CallSync({NO_PRIMARY_TABLE}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(NO_PRIMARY_TABLE, "10", {});

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    UpdateLocalData(db, NO_PRIMARY_TABLE, ASSETS_COPY1);
    CallSync({NO_PRIMARY_TABLE}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(NO_PRIMARY_TABLE, "10", {});
}

/**
 * @tc.name: FillAssetId003
 * @tc.desc: Test if assetId is filled in compound primary key table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, COMPOUND_PRIMARY_TABLE);
    CallSync({COMPOUND_PRIMARY_TABLE}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(COMPOUND_PRIMARY_TABLE, "10", {});

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    UpdateLocalData(db, COMPOUND_PRIMARY_TABLE, ASSETS_COPY1);
    CallSync({COMPOUND_PRIMARY_TABLE}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(COMPOUND_PRIMARY_TABLE, "10", {});
}

/**
 * @tc.name: FillAssetId004
 * @tc.desc: Test if assetId is filled in single primary key table when CLOUD_FORCE_PUSH
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_FORCE_PUSH, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_FORCE_PUSH, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId001
 * @tc.desc: Test if assetId is filled in no primary key table when CLOUD_FORCE_PUSH
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, NO_PRIMARY_TABLE);
    CallSync({NO_PRIMARY_TABLE}, SYNC_MODE_CLOUD_FORCE_PUSH, DBStatus::OK);
    CheckLocaLAssets(NO_PRIMARY_TABLE, "10", {});

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    UpdateLocalData(db, NO_PRIMARY_TABLE, ASSETS_COPY1);
    CallSync({NO_PRIMARY_TABLE}, SYNC_MODE_CLOUD_FORCE_PUSH, DBStatus::OK);
    CheckLocaLAssets(NO_PRIMARY_TABLE, "10", {});
}

/**
 * @tc.name: FillAssetId006
 * @tc.desc: Test if assetId is filled in compound primary key table when CLOUD_FORCE_PUSH
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId006, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, COMPOUND_PRIMARY_TABLE);
    CallSync({COMPOUND_PRIMARY_TABLE}, SYNC_MODE_CLOUD_FORCE_PUSH, DBStatus::OK);
    CheckLocaLAssets(COMPOUND_PRIMARY_TABLE, "10", {});

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    UpdateLocalData(db, COMPOUND_PRIMARY_TABLE, ASSETS_COPY1);
    CallSync({COMPOUND_PRIMARY_TABLE}, SYNC_MODE_CLOUD_FORCE_PUSH, DBStatus::OK);
    CheckLocaLAssets(COMPOUND_PRIMARY_TABLE, "10", {});
}

/**
 * @tc.name: FillAssetId007
 * @tc.desc: Test if assetId is filled when extend lack of assets
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId007, TestSize.Level1)
{
    CloudSyncConfig config;
    config.maxUploadCount = 200; // max upload 200
    g_delegate->SetCloudSyncConfig(config);
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
        extend.erase("assets");
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "0", {});

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    int addLocalCount = 10;
    InsertLocalData(db, localCount, addLocalCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end()) {
            for (auto &asset : std::get<Assets>(extend["assets"])) {
                asset.name = "pad";
            }
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    int beginFailFillNum = 101;
    int endFailFillNum = 120;
    std::set<int> index;
    for (int i = beginFailFillNum; i <= endFailFillNum; i++) {
        index.insert(i);
    }
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", index);

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    g_virtualCloudDb->ForkUpload(nullptr);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId008
 * @tc.desc: Test if assetId is filled when extend lack of assetId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId008, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end()) {
            for (auto &asset : std::get<Assets>(extend["assets"])) {
                asset.assetId = "";
            }
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "0", {});

    /**
     * @tc.steps:step2. local update assets and sync ,check the local assetId.
     * @tc.expected: step2. sync success.
     */
    g_virtualCloudDb->ForkUpload(nullptr);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId009
 * @tc.desc: Test if assetId is filled when extend exists useless assets
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId009, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end()) {
            Asset asset = ASSET_COPY2;
            Assets &assets = std::get<Assets>(extend["assets"]);
            assets.push_back(asset);
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId010
 * @tc.desc: Test if assetId is filled when some success and some fail
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId010, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 30;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->SetInsertFailed(1);
    std::atomic<int> count = 0;
    g_virtualCloudDb->ForkUpload([&count](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end() && count == 0) {
            extend["#_error"] = static_cast<int64_t>(DBStatus::CLOUD_NETWORK_ERROR);
            count++;
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", { 1, 2 }); // 1st, 2nd asset do not fill
}

/**
 * @tc.name: FillAssetId011
 * @tc.desc: Test if assetId is null when removedevicedata in FLAG_ONLY
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId011, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});

    g_delegate->RemoveDeviceData("", FLAG_ONLY);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "", {});
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId012
 * @tc.desc: Test if assetid is filled when extend size is not equal to record size
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId012, TestSize.Level1)
{
    /**
     * @tc.steps:step1. set extend size missing then sync, check the asseid.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    std::atomic<int> count = 1;
    g_virtualCloudDb->SetClearExtend(count);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "0", {});

    /**
     * @tc.steps:step2. set extend size normal then sync, check the asseid.
     * @tc.expected: step2. return OK.
     */
    g_virtualCloudDb->SetClearExtend(0);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});

    /**
     * @tc.steps:step3. set extend size large then sync, check the asseid.
     * @tc.expected: step3. return OK.
     */
    count = -1; // -1 means extend push a empty vBucket
    g_virtualCloudDb->SetClearExtend(count);
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);
}

/**
 * @tc.name: FillAssetId013
 * @tc.desc: Test fill assetId and removedevicedata when data is delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId013, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert data and sync, then delete local data and insert new data
     * @tc.expected: step1. return OK.
     */
    int localCount = 20;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    int deleteLocalCount = 10;
    DeleteLocalRecord(db, 0, deleteLocalCount, ASSETS_TABLE_NAME);
    int addLocalCount = 30;
    InsertLocalData(db, localCount, addLocalCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step2. RemoveDeviceData.
     * @tc.expected: step2. return OK.
     */
    g_delegate->RemoveDeviceData("", FLAG_ONLY);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "", {});
}

/**
 * @tc.name: FillAssetId014
 * @tc.desc: Test if asset status is reset when removedevicedata in FLAG_ONLY
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId014, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});

    /**
     * @tc.steps:step2. RemoveDeviceData
     * @tc.expected: step2. return OK.
     */
    Assets assets;
    std::vector<AssetStatus> statusVec = {
        AssetStatus::INSERT, AssetStatus::UPDATE, AssetStatus::DELETE, AssetStatus::NORMAL,
        AssetStatus::ABNORMAL, AssetStatus::DOWNLOADING, AssetStatus::DOWNLOAD_WITH_NULL
    };
    for (auto &status : statusVec) {
        Asset temp = ASSET_COPY;
        temp.name += std::to_string(status);
        temp.status = status | AssetStatus::UPLOADING;
        assets.emplace_back(temp);
    }
    UpdateLocalData(db, ASSETS_TABLE_NAME, assets);
    EXPECT_EQ(g_delegate->RemoveDeviceData("", FLAG_ONLY), OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "", {});

    /**
     * @tc.steps:step3. check status
     * @tc.expected: step3. return OK.
     */
    std::string sql = "SELECT assets FROM " + ASSETS_TABLE_NAME + ";";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    int index = 0;
    while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_BLOB);
        Type cloudValue;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Assets>, 0, cloudValue), E_OK);
        Assets newAssets = g_virtualCloudDataTranslate->BlobToAssets(std::get<Bytes>(cloudValue));
        for (const auto &ast : newAssets) {
            EXPECT_EQ(ast.status, statusVec[index++ % statusVec.size()]);
        }
    }
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

/**
 * @tc.name: FillAssetId015
 * @tc.desc: Test if fill assetId when upload return cloud network error
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId015, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert data and fork batchinsert return CLOUD_NETWORK_ERROR, then sync
     * @tc.expected: step1. return OK, errcode is CLOUD_NETWORK_ERROR.
     */
    int localCount = 20;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->SetCloudNetworkError(true);
    std::atomic<int> count = 0;
    g_virtualCloudDb->ForkUpload([&count](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end() && count == 0) {
            extend["#_error"] = static_cast<int64_t>(DBStatus::CLOUD_NETWORK_ERROR);
            count++;
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_NETWORK_ERROR);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", { 1, 2 }); // 1st, 2nd asset do not fill
    g_virtualCloudDb->SetCloudNetworkError(false);
    g_virtualCloudDb->ForkUpload(nullptr);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});

    /**
     * @tc.steps:step2. local insert data and fork batchinsert return CLOUD_NETWORK_ERROR, then sync.
     * @tc.expected: step2. return OK, errcode is CLOUD_ERROR.
     */
    int addLocalCount = 10;
    InsertLocalData(db, localCount, addLocalCount, ASSETS_TABLE_NAME);
    std::atomic<int> num = 0;
    g_virtualCloudDb->ForkUpload([&num](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end() && num == 0) {
            for (auto &asset : std::get<Assets>(extend["assets"])) {
                asset.name = "pad";
                break;
            }
            num++;
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {41}); // // 41th asset do not fill
}

/**
 * @tc.name: FillAssetId016
 * @tc.desc: Test fill assetId and removedevicedata when last data is delete
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId016, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert data and sync, then delete last local data
     * @tc.expected: step1. return OK.
     */
    int localCount = 20;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    int deletLocalCount = 10;
    DeleteLocalRecord(db, deletLocalCount, deletLocalCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step2. RemoveDeviceData.
     * @tc.expected: step2. return OK.
     */
    g_delegate->RemoveDeviceData("", FLAG_ONLY);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "", {});
}

/**
 * @tc.name: FillAssetId017
 * @tc.desc: Test cursor when download not change
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId017, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert data and sync,check cursor.
     * @tc.expected: step1. return OK.
     */
    int localCount = 20;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, false);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckCursorData(ASSETS_TABLE_NAME, 1);

    /**
     * @tc.steps:step2. sync again and optype is not change, check cursor.
     * @tc.expected: step2. return OK.
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckCursorData(ASSETS_TABLE_NAME, 1);
}

/**
 * @tc.name: FillAssetId018
 * @tc.desc: Test if assetId is filled when contains "#_error"
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId018, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 30;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    std::atomic<int> count = 0;
    g_virtualCloudDb->ForkUpload([&count](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end() && count == 0) {
            extend["#_error"] = std::string("test");
            count++;
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: DownloadAssetForDupDataTest002
 * @tc.desc: Test download failed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetForDupDataTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Mock asset download return CLOUD_ERROR.
     * @tc.expected: step1. return OK
     */
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 0;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(
            [&](const std::string &, const std::string &gid, const Type &, std::map<std::string, Assets> &assets) {
                LOGD("Download GID:%s, index:%d", gid.c_str(), ++index);
                return DBStatus::CLOUD_ERROR;
            });

    /**
     * @tc.steps:step2. Insert cloud data [0, 10), sync data
     * @tc.expected: step2. sync success.
     */
    InsertCloudDBData(0, 10, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);

    /**
     * @tc.steps:step3. check if the hash of assets in db is empty
     * @tc.expected: step3. OK
     */
    CheckDownloadFailedForTest002(db);
}

/**
 * @tc.name: DownloadAssetForDupDataTest003
 * @tc.desc: Test download failed and flag was modified
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetForDupDataTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Mock asset download return CLOUD_ERROR.
     * @tc.expected: step1. return OK
     */
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 0;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(
            [&](const std::string &, const std::string &gid, const Type &, std::map<std::string, Assets> &assets) {
                LOGD("Download GID:%s, index:%d", gid.c_str(), ++index);
                for (auto &item : assets) {
                    for (auto &asset : item.second) {
                        asset.flag = static_cast<uint32_t>(AssetOpType::NO_CHANGE);
                    }
                }
                return DBStatus::CLOUD_ERROR;
            });

    /**
     * @tc.steps:step2. Insert cloud data [0, 10), sync data
     * @tc.expected: step2. sync success.
     */
    InsertCloudDBData(0, 10, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);

    /**
     * @tc.steps:step3. check if the hash of assets in db is empty
     * @tc.expected: step3. OK
     */
    CheckDownloadFailedForTest002(db);
}

/**
 * @tc.name: DownloadAssetForDupDataTest004
 * @tc.desc: test sync with deleted assets
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetForDupDataTest004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Mock asset download return CLOUD_ERROR.
     * @tc.expected: step1. return OK
     */
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 0;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(
            [&](const std::string &, const std::string &gid, const Type &, std::map<std::string, Assets> &assets) {
                LOGD("Download GID:%s, index:%d", gid.c_str(), ++index);
                return DBStatus::OK;
            });

    /**
     * @tc.steps:step2. insert local data, update assets status to delete, then insert cloud data
     * @tc.expected: step2. return OK
     */
    InsertLocalData(db, 0, 10, ASSETS_TABLE_NAME); // 10 is num
    UpdateAssetsForLocal(db, 1, AssetStatus::DELETE); // 1 is id
    UpdateAssetsForLocal(db, 2, AssetStatus::DELETE | AssetStatus::UPLOADING); // 2 is id
    InsertCloudDBData(0, 10, 0, ASSETS_TABLE_NAME); // 10 is num

    /**
     * @tc.steps:step3. sync, check download num
     * @tc.expected: step3. return OK
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    EXPECT_GE(index, 2); // 2 is download num
}

/**
 * @tc.name: DownloadAssetForDupDataTest005
 * @tc.desc: test DOWNLOADING status of asset after uploading
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetForDupDataTest005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data and sync
     * @tc.expected: step1. return OK
     */
    InsertLocalData(db, 0, 10, ASSETS_TABLE_NAME); // 10 is num
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    UpdateAssetsForLocal(db, 6,  AssetStatus::DOWNLOADING); // 6 is id
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step2. check asset status
     * @tc.expected: step2. return OK
     */
    std::string sql = "SELECT assets from " + ASSETS_TABLE_NAME + " where id = 6;";
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
            EXPECT_EQ(assets[i].hash, ASSET_COPY.hash);
            EXPECT_EQ(assets[i].status, AssetStatus::NORMAL);
        }
    }
    int errCode;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

/**
 * @tc.name: FillAssetId019
 * @tc.desc: Test the stability of cleaning asset id
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId019, TestSize.Level1)
{
    /**
     * @tc.steps:step1. local insert assets and sync.
     * @tc.expected: step1. return OK.
     */
    int localCount = 20;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, false);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step2. construct multiple abnormal data_key, then RemoveDeviceData.
     * @tc.expected: step2. return OK.
     */
    std::string sql = "update " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME)
        + " set data_key='999' where data_key>'10';";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
    EXPECT_EQ(g_delegate->RemoveDeviceData("", FLAG_ONLY), OK);
}

/**
 * @tc.name: FillAssetId020
 * @tc.desc: Test if assetId is filled when extend(lack of assets/assetId is empty/modify asset info)
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangtao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId020, TestSize.Level1)
{
    CloudSyncConfig config;
    config.maxUploadCount = 200; // max upload 200
    g_delegate->SetCloudSyncConfig(config);

    /**
     * @tc.steps:step1. local insert assets and erase assets extends
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
        extend.erase("assets");
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "0", {});

    /**
     * @tc.steps:step2. local insert assets and modify assetId to empty
     * @tc.expected: step2. return OK.
     */
    int addLocalCount = 10;
    InsertLocalData(db, localCount, addLocalCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end()) {
            for (auto &asset : std::get<Assets>(extend["assets"])) {
                asset.assetId = "";
            }
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    int beginFailFillNum = 101;
    int endFailFillNum = 120;
    std::set<int> index;
    for (int i = beginFailFillNum; i <= endFailFillNum; i++) {
        index.insert(i);
    }
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", index);

    /**
     * @tc.steps:step3. local insert assets and modify assetId info such as asset.name
     * @tc.expected: step3. return OK.
     */
    InsertLocalData(db, localCount + addLocalCount, addLocalCount, ASSETS_TABLE_NAME);
    g_virtualCloudDb->ForkUpload([](const std::string &tableName, VBucket &extend) {
        if (extend.find("assets") != extend.end()) {
            for (auto &asset : std::get<Assets>(extend["assets"])) {
                asset.name = "mod_pat";
            }
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    beginFailFillNum = 121;
    endFailFillNum = 140;
    std::set<int> newIndex;
    for (int i = beginFailFillNum; i <= endFailFillNum; i++) {
        newIndex.insert(i);
    }
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", newIndex);

    /**
     * @tc.steps:step4. local update assets and sync, check the local assetId.
     * @tc.expected: step4. sync success.
     */
    g_virtualCloudDb->ForkUpload(nullptr);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId021
 * @tc.desc: Test if local assets missing, one records's assets missing will not mark the whole sync progress failure
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangtao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId021, TestSize.Level1)
{
    CloudSyncConfig config;
    config.maxUploadCount = 200; // max upload 200
    g_delegate->SetCloudSyncConfig(config);

    /**
     * @tc.steps:step1. local insert assets and erase assets extends
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. ForkInsertConflict, make one record assets missing during batch insert
     * @tc.expected: step2. SyncProgress return OK. One record's assets missing will not block other progress.
     */
    int uploadFailId = 0;
    g_virtualCloudDb->ForkInsertConflict([&uploadFailId](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        uploadFailId++;
        if (uploadFailId == 25) { // 25 is the middle record
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::LOCAL_ASSET_NOT_FOUND);
            return DBStatus::LOCAL_ASSET_NOT_FOUND;
        }
        return OK;
    });

    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    int beginFailFillNum = 49;
    int endFailFillNum = 50;
    std::set<int> index;
    for (int i = beginFailFillNum; i <= endFailFillNum; i++) {
        index.insert(i);
    }
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", index);
    g_virtualCloudDb->ForkUpload(nullptr);
}

/**
 * @tc.name: FillAssetId022
 * @tc.desc: Test if local assets missing, many records's assets missing will not mark the whole sync progress failure
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangtao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId022, TestSize.Level1)
{
    CloudSyncConfig config;
    config.maxUploadCount = 200; // max upload 200
    g_delegate->SetCloudSyncConfig(config);

    /**
     * @tc.steps:step1. local insert assets and erase assets extends
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. ForkInsertConflict, make one record assets missing during batch insert
     * @tc.expected: step2. SyncProgress return OK. One record's assets missing will not block other progress.
     */
    int uploadFailId = 0;
    g_virtualCloudDb->ForkInsertConflict([&uploadFailId](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        uploadFailId++;
        if (uploadFailId >= 25 && uploadFailId <= 27) { // 25-27 is the middle record
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::LOCAL_ASSET_NOT_FOUND);
            return DBStatus::LOCAL_ASSET_NOT_FOUND;
        }
        return OK;
    });

    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    int beginFailFillNum = 49;
    int endFailFillNum = 54;
    std::set<int> index;
    for (int i = beginFailFillNum; i <= endFailFillNum; i++) {
        index.insert(i);
    }
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", index);
    g_virtualCloudDb->ForkUpload(nullptr);
}

/**
 * @tc.name: FillAssetId023
 * @tc.desc: Test if BatchUpdate with local assets missing
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangtao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId023, TestSize.Level1)
{
    /**
     * @tc.steps:step1. set extend size missing then sync, check the asseid.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    std::atomic<int> count = 1;
    g_virtualCloudDb->SetClearExtend(count);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "0", {});

    /**
     * @tc.steps:step2. set extend size normal and BatchUpdate with local assets missing then sync, check the asseid.
     * @tc.expected: step2. return OK.
     */
    g_virtualCloudDb->SetClearExtend(0);

    int uploadFailId = 0;
    g_virtualCloudDb->ForkInsertConflict([&uploadFailId](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        uploadFailId++;
        if (uploadFailId == 25) { // 25 is the middle record
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::LOCAL_ASSET_NOT_FOUND);
            return DBStatus::LOCAL_ASSET_NOT_FOUND;
        }
        return OK;
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId024
 * @tc.desc: Test if BatchUpdate with multiple local assets missing
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangtao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId024, TestSize.Level1)
{
    /**
     * @tc.steps:step1. set extend size missing then sync, check the asseid.
     * @tc.expected: step1. return OK.
     */
    int localCount = 50;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    std::atomic<int> count = 1;
    g_virtualCloudDb->SetClearExtend(count);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "0", {});

    /**
     * @tc.steps:step2. set extend size normal and BatchUpdate with 3 local assets missing then sync, check the asseid.
     * @tc.expected: step2. return OK.
     */
    g_virtualCloudDb->SetClearExtend(0);

    int uploadFailId = 0;
    g_virtualCloudDb->ForkInsertConflict([&uploadFailId](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        uploadFailId++;
        if (uploadFailId >= 25 && uploadFailId <= 27) { // 25-27 is the middle record
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::LOCAL_ASSET_NOT_FOUND);
            return DBStatus::LOCAL_ASSET_NOT_FOUND;
        }
        return OK;
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}

/**
 * @tc.name: FillAssetId025
 * @tc.desc: Test if BatchInsert with local assets missing and missing record added into successCount
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangtao
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId025, TestSize.Level1)
{
    CloudSyncConfig config;
    config.maxUploadCount = 200; // max upload 200
    g_delegate->SetCloudSyncConfig(config);
    /**
     * @tc.steps:step1. insert local data.
     * @tc.expected: step1. return OK.
     */
    int localCount = 40;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. BatchInsert with local assets missing then sync, check the asseid.
     * @tc.expected: step2. return OK.
     */
    int uploadFailId = 0;
    g_virtualCloudDb->ForkInsertConflict([&uploadFailId](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        uploadFailId++;
        if (uploadFailId == 25) { // 25 is the middle record
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::LOCAL_ASSET_NOT_FOUND);
            return DBStatus::LOCAL_ASSET_NOT_FOUND;
        }
        return OK;
    });
    g_syncProcess = {};
    Query query = Query::Select().FromTable({ ASSETS_TABLE_NAME });
    std::vector<TableProcessInfo> expectProcess = {
        { PROCESSING, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
        { FINISHED, { 0, 0, 0, 0 }, { 1, 40, 40, 0 } } // 1 is index, 40 is count
    };

    /**
     * @tc.steps:step3. Check if sync process consistent with exptectProcess
     * @tc.expected: step3. return OK.
     */
    int index = 0;
    CloudSyncStatusCallback callback = [&index, &expectProcess](const std::map<std::string, SyncProcess> &process) {
        g_syncProcess = std::move(process.begin()->second);
        ASSERT_LT(index, 2);
        for (const auto &[tableName, info]: g_syncProcess.tableProcess) {
            EXPECT_EQ(info.process, expectProcess[index].process);
            EXPECT_EQ(info.upLoadInfo.batchIndex, expectProcess[index].upLoadInfo.batchIndex);
            EXPECT_EQ(info.upLoadInfo.total, expectProcess[index].upLoadInfo.total);
            EXPECT_EQ(info.upLoadInfo.successCount, expectProcess[index].upLoadInfo.successCount);
            EXPECT_EQ(tableName, ASSETS_TABLE_NAME);
        }
        index++;
        if (g_syncProcess.process == FINISHED) {
            g_processCondition.notify_one();
            ASSERT_EQ(g_syncProcess.errCode, DBStatus::OK);
        }
    };
    ASSERT_EQ(g_delegate->Sync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, query, callback, SYNC_WAIT_TIME), OK);
    WaitForSyncFinish(g_syncProcess, SYNC_WAIT_TIME);

    /**
     * @tc.steps:step4. Check assets results
     * @tc.expected: step4. return OK.
     */
    int beginFailFillNum = 49;
    int endFailFillNum = 50;
    std::set<int> indexes;
    for (int i = beginFailFillNum; i <= endFailFillNum; i++) {
        indexes.insert(i);
    }
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", indexes);
    g_virtualCloudDb->ForkUpload(nullptr);
}

/**
 * @tc.name: ConsistentFlagTest001
 * @tc.desc:Assets are the different, check the 0x20 bit of flag after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, ConsistentFlagTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data for the different asset, sync and check flag
     * @tc.expected: step1. return OK.
     */
    int localCount = 10; // 10 is num of local
    int cloudCount = 20; // 20 is num of cloud
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, false);
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckConsistentCount(db, cloudCount);

    /**
     * @tc.steps:step2. update local data, sync and check flag
     * @tc.expected: step2. return OK.
     */
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    DeleteCloudDBData(1, 1, ASSETS_TABLE_NAME);
    CheckConsistentCount(db, 0L);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckConsistentCount(db, cloudCount);
}

/**
 * @tc.name: ConsistentFlagTest002
 * @tc.desc: Assets are the same, check the 0x20 bit of flag after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, ConsistentFlagTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data for the same asset, sync and check flag
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 20; // 20 is num of cloud
    InsertLocalData(db, 0, cloudCount, ASSETS_TABLE_NAME, true);
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckConsistentCount(db, cloudCount);

    /**
     * @tc.steps:step2. update local data, sync and check flag
     * @tc.expected: step2. return OK.
     */
    int deleteLocalCount = 5;
    DeleteLocalRecord(db, 0, deleteLocalCount, ASSETS_TABLE_NAME);
    CheckConsistentCount(db, cloudCount - deleteLocalCount);
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckConsistentCount(db, cloudCount);
}

/**
 * @tc.name: ConsistentFlagTest003
 * @tc.desc: Download returns a conflict, check the 0x20 bit of flag after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, ConsistentFlagTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int localCount = 20; // 20 is num of local
    int cloudCount = 10; // 10 is num of cloud
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, false);
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. fork download, return CLOUD_RECORD_EXIST_CONFLICT once
     * @tc.expected: step2. return OK.
     */
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 0;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(
            [&index](const std::string &, const std::string &gid, const Type &, std::map<std::string, Assets> &assets) {
                LOGD("download gid:%s, index:%d", gid.c_str(), ++index);
                if (index == 1) { // 1 is first download
                    return DBStatus::CLOUD_RECORD_EXIST_CONFLICT;
                }
                return DBStatus::OK;
            });

    /**
     * @tc.steps:step3. fork upload, check consistent count
     * @tc.expected: step3. return OK.
     */
    int upIdx = 0;
    g_virtualCloudDb->ForkUpload([this, localCount, cloudCount, &upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("upload index:%d", ++upIdx);
        if (upIdx == 1) { // 1 is first upload
            CheckConsistentCount(db, localCount - cloudCount - 1);
        }
    });

    /**
     * @tc.steps:step4. fork query, check consistent count
     * @tc.expected: step4. return OK.
     */
    int queryIdx = 0;
    g_virtualCloudDb->ForkQuery([this, localCount, &queryIdx](const std::string &, VBucket &) {
        LOGD("query index:%d", ++queryIdx);
        if (queryIdx == 3) { // 3 is the last query
            CheckConsistentCount(db, localCount - 1);
        }
    });
    int count = 0;
    g_cloudStoreHook->SetSyncFinishHook([&count]() {
        count++;
        if (count == 2) { // 2 is compensated sync
            g_processCondition.notify_one();
        }
    });
    /**
     * @tc.steps:step5. sync, check consistent count
     * @tc.expected: step5. return OK.
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    WaitForSync(count);
    CheckConsistentCount(db, localCount);
}

/**
 * @tc.name: ConsistentFlagTest004
 * @tc.desc: Upload returns error, check the 0x20 bit of flag after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, ConsistentFlagTest004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int localCount = 20; // 20 is num of local
    int cloudCount = 10; // 10 is num of cloud
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, false);
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. fork upload, return error filed of CLOUD_NETWORK_ERROR
     * @tc.expected: step2. return OK.
     */
    int upIdx = 0;
    g_virtualCloudDb->ForkUpload([&upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("upload index:%d", ++upIdx);
        if (upIdx == 1) {
            extend.insert_or_assign(CloudDbConstant::ERROR_FIELD, static_cast<int64_t>(DBStatus::CLOUD_NETWORK_ERROR));
        }
    });

    /**
     * @tc.steps:step3. sync, check consistent count
     * @tc.expected: step3. return OK.
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckConsistentCount(db, localCount - 1);

    /**
     * @tc.steps:step4. update local data, fork upload, return error filed of type int64_t
     * @tc.expected: step4. return OK.
     */
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    upIdx = 0;
    g_virtualCloudDb->ForkUpload([&upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("upload index:%d", ++upIdx);
        if (upIdx == 1) {
            int64_t err = DBStatus::CLOUD_RECORD_EXIST_CONFLICT;
            extend.insert_or_assign(CloudDbConstant::ERROR_FIELD, err);
        }
        if (upIdx == 2) {
            int64_t err = DBStatus::CLOUD_RECORD_EXIST_CONFLICT + 1;
            extend.insert_or_assign(CloudDbConstant::ERROR_FIELD, err);
        }
    });

    /**
     * @tc.steps:step5. sync, check consistent count
     * @tc.expected: step5. return OK.
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckConsistentCount(db, localCount - 2);
}

/**
 * @tc.name: ConsistentFlagTest005
 * @tc.desc: Local data changes during download, check the 0x20 bit of flag after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, ConsistentFlagTest005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int localCount = 20; // 20 is num of local
    int cloudCount = 10; // 10 is num of cloud
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, false);
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. fork download, update local assets where id=2
     * @tc.expected: step2. return OK.
     */
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 0;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(
            [this, &index](const std::string &, const std::string &gid, const Type &,
                std::map<std::string, Assets> &assets) {
                LOGD("download gid:%s, index:%d", gid.c_str(), ++index);
                if (index == 1) { // 1 is first download
                    std::string sql = "UPDATE " + ASSETS_TABLE_NAME + " SET assets=NULL where id=2;";
                    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
                }
                return DBStatus::OK;
            });

    /**
     * @tc.steps:step3. fork upload, check consistent count
     * @tc.expected: step3. return OK.
     */
    int upIdx = 0;
    g_virtualCloudDb->ForkUpload([this, localCount, cloudCount, &upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("upload index:%d", ++upIdx);
        if (upIdx == 1) { // 1 is first upload
            CheckConsistentCount(db, localCount - cloudCount - 1);
        }
    });

    /**
     * @tc.steps:step4. sync, check consistent count
     * @tc.expected: step4. return OK.
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckConsistentCount(db, localCount);
}

/**
 * @tc.name: ConsistentFlagTest006
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, ConsistentFlagTest006, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 10; // 10 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step2. fork download, update local assets where id=2
     * @tc.expected: step2. return OK.
     */
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    int delCount = 3; // 3 is num of cloud
    DeleteCloudDBData(1, delCount, ASSETS_TABLE_NAME);
    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 0;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(
            [&index](const std::string &, const std::string &gid, const Type &,
                std::map<std::string, Assets> &assets) {
                LOGD("download gid:%s, index:%d", gid.c_str(), ++index);
                if (index == 1) { // 1 is first download
                    return DBStatus::CLOUD_RECORD_EXIST_CONFLICT;
                }
                return DBStatus::OK;
            });

    /**
     * @tc.steps:step3. fork upload, check consistent count
     * @tc.expected: step3. return OK.
     */
    int upIdx = 0;
    g_virtualCloudDb->ForkUpload([this, delCount, &upIdx](const std::string &tableName, VBucket &extend) {
        LOGD("upload index:%d", ++upIdx);
        if (upIdx == 1) { // 1 is first upload
            CheckConsistentCount(db, delCount);
            CheckCompensatedCount(db, 0L);
        }
    });

    /**
     * @tc.steps:step4. sync, check consistent count
     * @tc.expected: step4. return OK.
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckConsistentCount(db, cloudCount);
}

/**
 * @tc.name: SyncDataStatusTest001
 * @tc.desc: No need to download asset, check status after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, SyncDataStatusTest001, TestSize.Level1)
{
    DataStatusTest001(false);
}

/**
 * @tc.name: SyncDataStatusTest002
 * @tc.desc: Need to download asset, check status after sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, SyncDataStatusTest002, TestSize.Level1)
{
    DataStatusTest001(true);
}

/**
 * @tc.name: SyncDataStatusTest003
 * @tc.desc: Lock during download and check status
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, SyncDataStatusTest003, TestSize.Level1)
{
    DataStatusTest003();
}

/**
 * @tc.name: SyncDataStatusTest004
 * @tc.desc: Lock and delete during download, check status
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, SyncDataStatusTest004, TestSize.Level1)
{
    DataStatusTest004();
}

/**
 * @tc.name: SyncDataStatusTest005
 * @tc.desc: Lock and update during download, check status
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, SyncDataStatusTest005, TestSize.Level1)
{
    DataStatusTest005();
}

/**
 * @tc.name: SyncDataStatusTest006
 * @tc.desc: Lock and update and Unlock during download, check status
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, SyncDataStatusTest006, TestSize.Level1)
{
    DataStatusTest006();
}

/**
 * @tc.name: SyncDataStatusTest007
 * @tc.desc: Download return error, check status
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, SyncDataStatusTest007, TestSize.Level1)
{
    DataStatusTest007();
}

/**
 * @tc.name: SyncDataStatusTest008
 * @tc.desc: Test upload process when data locked
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, SyncDataStatusTest008, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init local data
     * @tc.expected: step1. return OK.
     */
    int localCount = 40;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, true);
    std::string logName = DBCommon::GetLogTableName(ASSETS_TABLE_NAME);
    std::string sql = "update " + logName + " SET status = 2 where data_key >=20;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);

    /**
     * @tc.steps:step2. sync and check process
     * @tc.expected: step2. return OK.
     */
    g_syncProcess = {};
    Query query = Query::Select().FromTable({ ASSETS_TABLE_NAME });
    std::vector<TableProcessInfo> expectProcess = {
        { PROCESSING, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
        { FINISHED, { 0, 0, 0, 0 }, { 1, 40, 40, 0 } } // 1 is index, 40 is count
    };
    int index = 0;
    CloudSyncConfig config;
    config.maxUploadCount = 100; // max upload 100
    g_delegate->SetCloudSyncConfig(config);
    CloudSyncStatusCallback callback = [&index, &expectProcess](const std::map<std::string, SyncProcess> &process) {
        g_syncProcess = std::move(process.begin()->second);
        ASSERT_LT(index, 2);
        for (const auto &[tableName, info]: g_syncProcess.tableProcess) {
            EXPECT_EQ(info.process, expectProcess[index].process);
            EXPECT_EQ(info.upLoadInfo.batchIndex, expectProcess[index].upLoadInfo.batchIndex);
            EXPECT_EQ(info.upLoadInfo.total, expectProcess[index].upLoadInfo.total);
            EXPECT_EQ(info.upLoadInfo.successCount, expectProcess[index].upLoadInfo.successCount);
            EXPECT_EQ(tableName, ASSETS_TABLE_NAME);
        }
        index++;
        if (g_syncProcess.process == FINISHED) {
            g_processCondition.notify_one();
            ASSERT_EQ(g_syncProcess.errCode, DBStatus::OK);
        }
    };
    ASSERT_EQ(g_delegate->Sync({DEVICE_CLOUD}, SYNC_MODE_CLOUD_MERGE, query, callback, SYNC_WAIT_TIME), OK);
    WaitForSyncFinish(g_syncProcess, SYNC_WAIT_TIME);
}

/**
 * @tc.name: DownloadAssetTest001
 * @tc.desc: Test the asset status after the share table sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: bty
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetTest001, TestSize.Level1)
{
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(true);
    /**
     * @tc.steps:step1. init data and sync
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 10; // 10 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME_SHARED);
    CallSync({ASSETS_TABLE_NAME_SHARED}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step2. check asset status
     * @tc.expected: step2. return OK.
     */
    SqlCondition condition;
    condition.sql = "select assets from " + ASSETS_TABLE_NAME_SHARED + " where _rowid_ = 1;";
    condition.readOnly = true;
    std::vector<VBucket> records;
    EXPECT_EQ(g_delegate->ExecuteSql(condition, records), OK);
    for (const auto &data: records) {
        Assets assets;
        CloudStorageUtils::GetValueFromVBucket(COL_ASSETS, data, assets);
        for (const auto &asset: assets) {
            EXPECT_EQ(asset.status, AssetStatus::NORMAL);
        }
    }
    EXPECT_EQ(g_virtualAssetLoader->GetBatchDownloadCount(), 0u);
    RuntimeContext::GetInstance()->SetBatchDownloadAssets(false);
}

/**
 * @tc.name: DownloadAssetTest002
 * @tc.desc: Test asset download failed and re download
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 10; // 10 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. Set asset download status error and sync
     * @tc.expected: step2. sync successful but download assets fail.
     */
    g_virtualAssetLoader->SetDownloadStatus(DBStatus::CLOUD_ERROR);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);

    /**
     * @tc.steps:step3. Set asset download status OK and sync
     * @tc.expected: step3. return OK.
     */
    g_virtualAssetLoader->SetDownloadStatus(DBStatus::OK);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);

    /**
     * @tc.steps:step4. Check assets status
     * @tc.expected: step4. status is NORMAL.
     */
    std::string sql = "SELECT assets FROM " + ASSETS_TABLE_NAME + ";";
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, sql, stmt), E_OK);
    while (SQLiteUtils::StepWithRetry(stmt) != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        ASSERT_EQ(sqlite3_column_type(stmt, 0), SQLITE_BLOB);
        Type cloudValue;
        ASSERT_EQ(SQLiteRelationalUtils::GetCloudValueByType(stmt, TYPE_INDEX<Assets>, 0, cloudValue), E_OK);
        Assets assets = g_virtualCloudDataTranslate->BlobToAssets(std::get<Bytes>(cloudValue));
        for (const auto &asset : assets) {
            EXPECT_EQ(asset.status, AssetStatus::NORMAL);
        }
    }
    int errCode = E_OK;
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: DownloadAssetTest003
 * @tc.desc: Test asset download after sync task recovery
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetTest003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 10; // 10 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    DeleteCloudDBData(0, cloudCount, ASSETS_TABLE_NAME);
    InsertCloudDBData(0, cloudCount, 0, NO_PRIMARY_TABLE);
    /**
     * @tc.steps:step2. Set task interrupted before asset download
     * @tc.expected: step2. return OK.
     */
    int queryTime = 0;
    g_virtualCloudDb->ForkQuery([&](const std::string &, VBucket &) {
        queryTime++;
        if (queryTime != 1) {
            return;
        }
        Query query = Query::Select().FromTable({NO_PRIMARY_TABLE});
        CloudSyncOption option;
        option.priorityTask = true;
        option.devices = {DEVICE_CLOUD};
        option.mode = SYNC_MODE_CLOUD_MERGE;
        option.query = query;
        ASSERT_EQ(g_delegate->Sync(option, nullptr), OK);
    });
    /**
     * @tc.steps:step3. Sync
     * @tc.expected: step3. return OK.
     */
    int removeTime = 0;
    g_virtualAssetLoader->SetRemoveLocalAssetsCallback([&](std::map<std::string, Assets> &assets) {
        removeTime++;
        return OK;
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    /**
     * @tc.steps:step4. Check fork asset download time and observer
     * @tc.expected: step4. return OK.
     */
    EXPECT_EQ(removeTime, cloudCount);
    ChangedData expectedChangeData1;
    ChangedData expectedChangeData2;
    expectedChangeData1.tableName = NO_PRIMARY_TABLE;
    expectedChangeData2.tableName = ASSETS_TABLE_NAME;
    expectedChangeData1.type = ChangedDataType::ASSET;
    expectedChangeData2.type = ChangedDataType::ASSET;
    expectedChangeData1.field.push_back(std::string("rowid"));
    expectedChangeData2.field.push_back(std::string("id"));
    for (int i = 0; i < cloudCount; i++) {
        expectedChangeData1.primaryData[ChangeType::OP_INSERT].push_back({(int64_t)i + 1});
        expectedChangeData2.primaryData[ChangeType::OP_DELETE].push_back({(int64_t)i});
    }
    g_observer->SetExpectedResult(expectedChangeData1);
    g_observer->SetExpectedResult(expectedChangeData2);
    EXPECT_TRUE(g_observer->IsAllChangedDataEq());
    g_observer->ClearChangedData();

    g_virtualCloudDb->ForkInsertConflict(nullptr);
    g_virtualCloudDb->ForkQuery(nullptr);
    g_virtualAssetLoader->SetRemoveLocalAssetsCallback(nullptr);
}

/**
 * @tc.name: DownloadAssetTest004
 * @tc.desc: Test asset download failed with no asset data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, DownloadAssetTest004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 10; // 10 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME, true);
    InsertCloudDBData(cloudCount, cloudCount, cloudCount, ASSETS_TABLE_NAME, false);
    /**
     * @tc.steps:step2. Set asset download status error and sync
     * @tc.expected: step2. sync successful but download assets fail.
     */
    g_virtualAssetLoader->SetDownloadStatus(DBStatus::CLOUD_ERROR);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);
    /**
     * @tc.steps:step3. Check FLAG_DEVICE_CLOUD_INCONSISTENCY data
     * @tc.expected: step3. 10 data is cloud consistency because no asset.
     */
    std::string logName = DBCommon::GetLogTableName(ASSETS_TABLE_NAME);
    std::string sql = "SELECT COUNT(1) FROM " + logName + " WHERE flag & 0x20 = 0";
    int count = 0;
    ASSERT_EQ(SQLiteUtils::GetCountBySql(db, sql, count), E_OK);
    EXPECT_EQ(count, cloudCount);
}

/**
 * @tc.name: RecordLockFuncTest001
 * @tc.desc: UNLOCKING->UNLOCKING Synchronous download failure wholly.
 * @tc.type: FUNC
 * @tc.author: lijun
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, RecordLockFuncTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init local data
     * @tc.expected: step1. return OK.
     */
    int localCount = 100;
    int cloudCount = 100;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, true);
    std::string logName = DBCommon::GetLogTableName(ASSETS_TABLE_NAME);
    std::string sql = "update " + logName + " SET status = 2 where data_key >=70;";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    CheckLockStatus(db, 0, 69, LockStatus::UNLOCK);
    CheckLockStatus(db, 70, 99, LockStatus::LOCK);
    DeleteLocalRecord(db, 70, 30, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. init cloud data
     * @tc.expected: step2. return OK.
     */
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);
    UpdateCloudDBData(0, 70, 0, 0, ASSETS_TABLE_NAME);

    std::shared_ptr<MockAssetLoader> assetLoader = make_shared<MockAssetLoader>();
    ASSERT_EQ(g_delegate->SetIAssetLoader(assetLoader), DBStatus::OK);
    int index = 0;
    EXPECT_CALL(*assetLoader, Download(testing::_, testing::_, testing::_, testing::_))
        .WillRepeatedly(
            [&index](const std::string &, const std::string &gid, const Type &, std::map<std::string, Assets> &assets) {
                LOGD("Download GID:%s  %d", gid.c_str(), index);
                index++;
                if (index <= 30) {
                    return DBStatus::CLOUD_ERROR;
                } else {
                    return DBStatus::OK;
                }

            });

    std::mutex mtx;
    std::condition_variable cv;
    int queryIdx = 0;
    bool ready = false;
    g_virtualCloudDb->ForkQuery([&](const std::string &, VBucket &) {
        LOGD("query index:%d", ++queryIdx);
        if (queryIdx == 2) { // 2 is compensated sync
            std::unique_lock<std::mutex> lock(mtx);
            ready = true;
            cv.notify_one();
        }
    });
    g_virtualAssetLoader->SetDownloadStatus(DBStatus::CLOUD_ERROR);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::CLOUD_ERROR);

    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return ready; });
    }
    g_virtualAssetLoader->SetDownloadStatus(DBStatus::OK);

    std::this_thread::sleep_for(std::chrono::seconds(6));
    /**
     * @tc.steps:step3. check after compensated sync
     * @tc.expected: step3. all is UNLOCKING.
     */
    CheckLockStatus(db, 0, 99, LockStatus::UNLOCK);
}

/**
 * @tc.name: RecordLockFuncTest002
 * @tc.desc: Compensated synchronization, Locked data has not been synchronized. The first synchronization data is
 * based on the cloud, and the last synchronization data is based on the device.
 * @tc.type: FUNC
 * @tc.author: lijun
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, RecordLockFuncTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init local data, modify data Status and initiate synchronization
     * @tc.expected: step1. return OK.
     */
    int localCount = 120;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME, true);
    std::vector<std::vector<uint8_t>> hashKey;
    CloudDBSyncUtilsTest::GetHashKey(ASSETS_TABLE_NAME, " data_key >=100 ", db, hashKey);
    EXPECT_EQ(Lock(ASSETS_TABLE_NAME, hashKey, db), OK);
    CheckLockStatus(db, 0, 99, LockStatus::UNLOCK);
    CheckLockStatus(db, 100, 119, LockStatus::LOCK);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_FORCE_PULL, DBStatus::OK);

    /**
     * @tc.steps:step2. Check the synchronization result and log table status
     * @tc.expected: step2.100-109 is LOCK_CHANGE.
     */
    CheckLockStatus(db, 0, 99, LockStatus::UNLOCK);
    CheckLockStatus(db, 100, 119, LockStatus::LOCK);
    UpdateLocalData(db, ASSETS_TABLE_NAME, ASSETS_COPY1, 100, 109);
    CheckLockStatus(db, 100, 109, LockStatus::LOCK_CHANGE);
    CheckLockStatus(db, 110, 119, LockStatus::LOCK);

    /**
     * @tc.steps:step3. Synchronize and check the lock_change data status
     * @tc.expected: step3.100-119 is LOCK_CHANGE.
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLockStatus(db, 0, 99, LockStatus::UNLOCK);
    CheckLockStatus(db, 100, 119, LockStatus::LOCK_CHANGE);

    /**
     * @tc.steps:step4. Unlock,the lock_change data status changes to unlocking
     * @tc.expected: step4.100-119 is UNLOCKING.
     */
    EXPECT_EQ(UnLock(ASSETS_TABLE_NAME, hashKey, db), WAIT_COMPENSATED_SYNC);
    CheckLockStatus(db, 0, 99, LockStatus::UNLOCK);
    CheckLockStatus(db, 100, 119, LockStatus::UNLOCKING);

    /**
     * @tc.steps:step5. Lock,the unlocking data status changes to lock_change
     * @tc.expected: step5.100-119 is LOCK_CHANGE.
     */
    EXPECT_EQ(Lock(ASSETS_TABLE_NAME, hashKey, db), OK);
    CheckLockStatus(db, 0, 99, LockStatus::UNLOCK);
    CheckLockStatus(db, 100, 119, LockStatus::LOCK_CHANGE);

    /**
     * @tc.steps:step6. Synchronize and check the lock_change data status
     * @tc.expected: step6.100-119 is LOCK_CHANGE.
     */
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_FORCE_PUSH, DBStatus::OK);
    CheckLockStatus(db, 0, 99, LockStatus::UNLOCK);
    CheckLockStatus(db, 100, 119, LockStatus::LOCK_CHANGE);

    /**
     * @tc.steps:step7. Unlock,the lock_change data status changes to unlocking
     * @tc.expected: step7.100-119 is UNLOCKING.
     */
    EXPECT_EQ(UnLock(ASSETS_TABLE_NAME, hashKey, db), WAIT_COMPENSATED_SYNC);
    CheckLockStatus(db, 0, 99, LockStatus::UNLOCK);
    CheckLockStatus(db, 100, 119, LockStatus::UNLOCKING);

    /**
     * @tc.steps:step8. Synchronize data
     * @tc.expected: step8.return OK.
     */
    std::mutex mtx;
    std::condition_variable cv;
    int queryIdx = 0;
    bool ready = false;
    g_virtualCloudDb->ForkQuery([&](const std::string &, VBucket &) {
        LOGD("query index:%d", ++queryIdx);
        if (queryIdx == 5) { // 5 is compensated sync
            std::unique_lock<std::mutex> lock(mtx);
            ready = true;
            cv.notify_one();
        }
    });
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_FORCE_PUSH, DBStatus::OK);
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return ready; });
    }

    std::this_thread::sleep_for(std::chrono::seconds(6));
    /**
     * @tc.steps:step9. check after compensated sync
     * @tc.expected: step9. all is UNLOCK.
     */
    CheckLockStatus(db, 0, 119, LockStatus::UNLOCK);
}

/**
  * @tc.name: CloudTaskStatusTest001
  * @tc.desc: Test get cloud task status
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, CloudTaskStatusTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int cloudCount = 10; // 10 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. Sync and get cloud task status
     * @tc.expected: step2. OK
     */
    g_virtualCloudDb->SetBlockTime(1000);
    std::thread syncThread([&]() {
        CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    SyncProcess process1 = g_delegate->GetCloudTaskStatus(UINT64_MAX);
    EXPECT_EQ(process1.errCode, OK);
    syncThread.join();
    /**
     * @tc.steps:step3. Get cloud task status after sync finish
     * @tc.expected: step3. NOT_FOUND
     */
    SyncProcess process2 = g_delegate->GetCloudTaskStatus(1);
    EXPECT_EQ(process2.errCode, NOT_FOUND);

    /**
     * @tc.steps:step4. Get cloud task status after DB closed
     * @tc.expected: step4. DB_ERROR
     */
    auto delegateImpl = static_cast<RelationalStoreDelegateImpl *>(g_delegate);
    EXPECT_EQ(delegateImpl->Close(), DBStatus::OK);
    SyncProcess process3 = g_delegate->GetCloudTaskStatus(1);
    EXPECT_EQ(process3.errCode, DB_ERROR);
}

/**
  * @tc.name: CloudTaskStatusTest002
  * @tc.desc: Test get cloud task status when task merge
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: suyue
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, CloudTaskStatusTest002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK
     */
    int cloudCount = 100; // 100 is num of cloud
    InsertCloudDBData(0, cloudCount, 0, ASSETS_TABLE_NAME);

    /**
     * @tc.steps:step2. sync tasks 2 and 3 that can be merged when synchronizing task 1, get status of the merge task
     * @tc.expected: step2. the errCode of task 2 and task 3 is OK or NOT_FOUND
     */
    g_virtualCloudDb->SetBlockTime(1000);
    Query query = Query::Select().FromTable({ASSETS_TABLE_NAME});
    CloudSyncStatusCallback callback = [](const std::map<std::string, SyncProcess> &process) {
        std::unique_lock<std::mutex> lock(g_processMutex);
        if (process.begin()->second.process == FINISHED) {
            g_processCondition.notify_one();
        }
    };
    CloudSyncOption option = {.devices = {DEVICE_CLOUD}, .mode = SYNC_MODE_CLOUD_MERGE, .query = query,
        .waitTime = SYNC_WAIT_TIME, .lockAction = static_cast<LockAction>(0xff)};

    std::thread syncThread1([option, callback]() {
        ASSERT_EQ(g_delegate->Sync(option, callback), DBStatus::OK);
    });
    option.merge = true;
    std::thread syncThread2([option, callback]() {
        ASSERT_EQ(g_delegate->Sync(option, callback), DBStatus::OK);
    });
    std::thread syncThread3([option, callback]() {
        ASSERT_EQ(g_delegate->Sync(option, callback), DBStatus::OK);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    SyncProcess process1 = g_delegate->GetCloudTaskStatus(1);
    SyncProcess process2 = g_delegate->GetCloudTaskStatus(2);
    SyncProcess process3 = g_delegate->GetCloudTaskStatus(3);
    syncThread1.join();
    syncThread2.join();
    syncThread3.join();
    // Due to the task execution sequence, task 2 may be combined into 3 or task 3 may be combined into 2.
    // Therefore, the errCode of task 2 and 3 may be OK or NOT_FOUND.
    EXPECT_TRUE(process2.errCode == OK  || process2.errCode == NOT_FOUND);
    EXPECT_TRUE(process3.errCode == OK  || process3.errCode == NOT_FOUND);
}

/**
  * @tc.name: CompensatedSyncTest001
  * @tc.desc: test compensated count more than 100.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: tankaisheng
  */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, CompensatedSyncTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. init data
     * @tc.expected: step1. return OK.
     */
    int dataCount = 120;
    InsertCloudDBData(0, dataCount, 0, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK, DBStatus::OK);

    /**
     * @tc.steps:step2. set all data wait compensated.
     * @tc.expected: step2. return ok.
     */
    std::string sql = "update " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " set flag=flag|0x10;";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);
    sql = "select count(*) from " + DBCommon::GetLogTableName(ASSETS_TABLE_NAME) + " where flag&0x10=0x10;";
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(120u), nullptr), SQLITE_OK);
    
    /**
     * @tc.steps:step3. sync with compensated.
     * @tc.expected: step3. return ok.
     */
    std::mutex processMutex;
    std::vector<SyncProcess> expectProcess;
    std::condition_variable cv;
    bool finish = false;
    auto callback = [&cv, &finish, &processMutex]
        (const std::map<std::string, SyncProcess> &process) {
        for (auto &item : process) {
            if (item.second.process == FINISHED) {
                EXPECT_EQ(item.second.errCode, DBStatus::OK);
                std::unique_lock<std::mutex> lock(processMutex);
                finish = true;
                cv.notify_one();
            }
        }
    };
    CloudSyncOption option;
    option.devices = {DEVICE_CLOUD};
    option.priorityTask = true;
    option.compensatedSyncOnly = true;
    DBStatus syncResult = g_delegate->Sync(option, callback);
    EXPECT_EQ(syncResult, DBStatus::OK);

    /**
     * @tc.steps:step4. wait sync finish and check data.
     * @tc.expected: step4. return ok.
     */
    std::unique_lock<std::mutex> lock(processMutex);
    cv.wait(lock, [&finish]() {
        return finish;
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(sqlite3_exec(db, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0u), nullptr), SQLITE_OK);
}

/**
 * @tc.name: FillAssetId026
 * @tc.desc: Test if assetId is null when removedevicedata in FLAG_AND_DATA
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBCloudSyncerDownloadAssetsTest, FillAssetId026, TestSize.Level0)
{
    /**
     * @tc.steps:step1. local insert assets and sync, check the local assetId.
     * @tc.expected: step1. return OK.
     */
    int localCount = 10;
    InsertLocalData(db, 0, localCount, ASSETS_TABLE_NAME);
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});

    /**
     * @tc.steps:step2. remove device data then check record.
     * @tc.expected: step2. return OK.
     */
    g_delegate->RemoveDeviceData("", FLAG_AND_DATA);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "", {});
    CallSync({ASSETS_TABLE_NAME}, SYNC_MODE_CLOUD_MERGE, DBStatus::OK);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
    g_delegate->RemoveDeviceData("", CLEAR_SHARED_TABLE);
    CheckLocaLAssets(ASSETS_TABLE_NAME, "10", {});
}
} // namespace
#endif // RELATIONAL_STORE
