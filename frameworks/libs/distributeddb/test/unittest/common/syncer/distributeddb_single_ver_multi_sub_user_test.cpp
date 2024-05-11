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

#include <condition_variable>
#include <gtest/gtest.h>
#include <thread>

#include "cloud_db_sync_utils_test.h"
#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "virtual_asset_loader.h"
#include "virtual_cloud_data_translate.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    std::shared_ptr<std::string> g_testDir = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    const std::string DEVICE_A = "real_device";
    const std::string DEVICE_B = "deviceB";
    const std::string KEY_INSTANCE_ID = "INSTANCE_ID";
    const std::string KEY_SUB_USER = "SUB_USER";
    KvVirtualDevice *g_deviceB = nullptr;
    DistributedDBToolsUnitTest g_tool;
    CloudSyncOption g_CloudSyncoption;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<VirtualCloudDataTranslate> g_virtualCloudDataTranslate;
    const string ASSETS_TABLE_NAME = "student";
    const string ASSETS_TABLE_NAME_SHARED = "student_shared";
    const string COL_ID = "id";
    const string COL_NAME = "name";
    const string COL_ASSET = "asset";
    const string COL_ASSETS = "assets";
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
    int64_t g_nameId = 0;

    DBStatus OpenDelegate(const std::string &dlpPath, KvStoreNbDelegate *&delegatePtr,
        KvStoreDelegateManager &mgr, bool syncDualTupleMode = false)
    {
        if (g_testDir == nullptr) {
            return DB_ERROR;
        }
        std::string dbPath = *g_testDir + dlpPath;
        KvStoreConfig storeConfig;
        storeConfig.dataDir = dbPath;
        OS::MakeDBDirectory(dbPath);
        mgr.SetKvStoreConfig(storeConfig);

        string dir = dbPath + "/single_ver";
        DIR* dirTmp = opendir(dir.c_str());
        if (dirTmp == nullptr) {
            OS::MakeDBDirectory(dir);
        } else {
            closedir(dirTmp);
        }

        KvStoreNbDelegate::Option option;
        option.syncDualTupleMode = syncDualTupleMode;
        DBStatus res = OK;
        mgr.GetKvStore(STORE_ID_1, option, [&delegatePtr, &res](DBStatus status, KvStoreNbDelegate *delegate) {
            delegatePtr = delegate;
            res = status;
        });
        return res;
    }

    DataBaseSchema GetDataBaseSchema()
    {
        DataBaseSchema schema;
        TableSchema tableSchema;
        tableSchema.name = CloudDbConstant::CLOUD_KV_TABLE_NAME;
        Field field;
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_KEY;
        field.type = TYPE_INDEX<std::string>;
        field.primary = true;
        tableSchema.fields.push_back(field);
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_DEVICE;
        field.primary = false;
        tableSchema.fields.push_back(field);
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_ORI_DEVICE;
        tableSchema.fields.push_back(field);
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_VALUE;
        tableSchema.fields.push_back(field);
        field.colName = CloudDbConstant::CLOUD_KV_FIELD_DEVICE_CREATE_TIME;
        field.type = TYPE_INDEX<int64_t>;
        tableSchema.fields.push_back(field);
        schema.tables.push_back(tableSchema);
        return schema;
    }

    DBStatus SetCloudDB(KvStoreNbDelegate *&delegatePtr)
    {
        std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
        cloudDbs[USER_ID] = virtualCloudDb_;
        delegatePtr->SetCloudDB(cloudDbs);
        std::map<std::string, DataBaseSchema> schemas;
        schemas[USER_ID] = GetDataBaseSchema();
        return delegatePtr->SetCloudDbSchema(schemas);
    }

    DBStatus OpenDelegate(const std::string &dlpPath, RelationalStoreDelegate *&rdbDelegatePtr,
        RelationalStoreManager &mgr, sqlite3 *&db)
    {
        if (g_testDir == nullptr) {
            return DB_ERROR;
        }
        std::string dbDir = *g_testDir + dlpPath;
        OS::MakeDBDirectory(dbDir);
        std::string dbPath = dbDir + "/test.db";
        db = RelationalTestUtils::CreateDataBase(dbPath);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, CREATE_SINGLE_PRIMARY_KEY_TABLE), SQLITE_OK);
        RelationalStoreObserverUnitTest *observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        RelationalStoreDelegate::Option option = { .observer = observer };
        return mgr.OpenStore(dbPath, STORE_ID_1, option, rdbDelegatePtr);
    }

    void GetCloudDbSchema(DataBaseSchema &dataBaseSchema)
    {
        TableSchema assetsTableSchema = {.name = ASSETS_TABLE_NAME, .sharedTableName = ASSETS_TABLE_NAME_SHARED,
            .fields = CLOUD_FIELDS};
        dataBaseSchema.tables.push_back(assetsTableSchema);
    }

    DBStatus SetCloudDB(RelationalStoreDelegate *&delegatePtr)
    {
        EXPECT_EQ(delegatePtr->CreateDistributedTable(ASSETS_TABLE_NAME, CLOUD_COOPERATION), DBStatus::OK);
        std::shared_ptr<VirtualAssetLoader> virtualAssetLoader = make_shared<VirtualAssetLoader>();
        EXPECT_EQ(delegatePtr->SetCloudDB(virtualCloudDb_), DBStatus::OK);
        EXPECT_EQ(delegatePtr->SetIAssetLoader(virtualAssetLoader), DBStatus::OK);
        DataBaseSchema dataBaseSchema;
        GetCloudDbSchema(dataBaseSchema);
        return delegatePtr->SetCloudDbSchema(dataBaseSchema);
    }

    void CloseDelegate(KvStoreNbDelegate *&delegatePtr, KvStoreDelegateManager &mgr, std::string storeID)
    {
        if (delegatePtr == nullptr) {
            return;
        }
        EXPECT_EQ(mgr.CloseKvStore(delegatePtr), OK);
        delegatePtr = nullptr;
        EXPECT_EQ(mgr.DeleteKvStore(storeID), OK);
    }

    void CloseDelegate(RelationalStoreDelegate *&delegatePtr, RelationalStoreManager &mgr, sqlite3 *&db)
    {
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
        db = nullptr;
        if (delegatePtr == nullptr) {
            return;
        }
        EXPECT_EQ(mgr.CloseStore(delegatePtr), OK);
        delegatePtr = nullptr;
    }

class DistributedDBSingleVerMultiSubUserTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    void BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus, CloudSyncOption option,
        int expectSyncResult = OK);
    void InsertLocalData(int64_t begin, int64_t count, const std::string &tableName, bool isAssetNull, sqlite3 *&db);
    void GenerateDataRecords(int64_t begin, int64_t count, int64_t gidStart, std::vector<VBucket> &record,
        std::vector<VBucket> &extend);
    CloudSyncOption PrepareOption(const Query &query, LockAction action);
    void CallSync(RelationalStoreDelegate *delegate, const CloudSyncOption &option, DBStatus expectResult = OK);
    SyncProcess lastProcess_;
};

void DistributedDBSingleVerMultiSubUserTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    std::string testDir;
    DistributedDBToolsUnitTest::TestDirInit(testDir);
    if (g_testDir == nullptr) {
        g_testDir = std::make_shared<std::string>(testDir);
    }

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);

    g_CloudSyncoption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    g_CloudSyncoption.users.push_back(USER_ID);
    g_CloudSyncoption.devices.push_back("cloud");
    g_virtualCloudDataTranslate = std::make_shared<VirtualCloudDataTranslate>();
    RuntimeConfig::SetCloudTranslate(g_virtualCloudDataTranslate);
}

void DistributedDBSingleVerMultiSubUserTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (g_testDir != nullptr && DistributedDBToolsUnitTest::RemoveTestDbFiles(*g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerMultiSubUserTest::SetUp(void)
{
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);
}

void DistributedDBSingleVerMultiSubUserTest::TearDown(void)
{
    virtualCloudDb_ = nullptr;
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    PermissionCheckCallbackV3 nullCallback = nullptr;
    RuntimeConfig::SetPermissionCheckCallback(nullCallback);
    SyncActivationCheckCallbackV2 activeCallBack = nullptr;
    RuntimeConfig::SetSyncActivationCheckCallback(activeCallBack);
    RuntimeConfig::SetPermissionConditionCallback(nullptr);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(*g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

void DistributedDBSingleVerMultiSubUserTest::BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus,
    CloudSyncOption option, int expectSyncResult)
{
    if (delegate == nullptr) {
        return;
    }
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    SyncProcess last;
    auto callback = [expectDBStatus, &last, &cv, &dataMutex, &finish, &option](const std::map<std::string,
        SyncProcess> &process) {
        size_t notifyCnt = 0;
        for (const auto &item: process) {
            LOGD("user = %s, status = %d", item.first.c_str(), item.second.process);
            if (item.second.process != DistributedDB::FINISHED) {
                continue;
            }
            EXPECT_EQ(item.second.errCode, expectDBStatus);
            {
                std::lock_guard<std::mutex> autoLock(dataMutex);
                notifyCnt++;
                if (notifyCnt == option.users.size()) {
                    finish = true;
                    last = item.second;
                    cv.notify_one();
                }
            }
        }
    };
    EXPECT_EQ(delegate->Sync(option, callback), expectSyncResult);
    if (expectSyncResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }
    lastProcess_ = last;
}

void DistributedDBSingleVerMultiSubUserTest::GenerateDataRecords(
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

void DistributedDBSingleVerMultiSubUserTest::InsertLocalData(int64_t begin, int64_t count,
    const std::string &tableName, bool isAssetNull, sqlite3 *&db)
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

CloudSyncOption DistributedDBSingleVerMultiSubUserTest::PrepareOption(const Query &query, LockAction action)
{
    CloudSyncOption option;
    option.devices = { "CLOUD" };
    option.mode = SYNC_MODE_CLOUD_MERGE;
    option.query = query;
    option.waitTime = WAIT_TIME;
    option.priorityTask = false;
    option.compensatedSyncOnly = false;
    option.lockAction = action;
    return option;
}

void DistributedDBSingleVerMultiSubUserTest::CallSync(RelationalStoreDelegate *delegate, const CloudSyncOption &option,
    DBStatus expectResult)
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
    ASSERT_EQ(delegate->Sync(option, callback), expectResult);
    if (expectResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }
}

/**
 * @tc.name: KvDelegateInvalidParamTest001
 * @tc.desc: Test kv delegate open with invalid subUser.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, KvDelegateInvalidParamTest001, TestSize.Level1)
{
    std::string subUser1 = std::string(129, 'a');
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, subUser1, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", delegatePtr1, mgr1), INVALID_ARGS);
    ASSERT_EQ(delegatePtr1, nullptr);

    std::string subUser2 = "subUser-1";
    KvStoreDelegateManager mgr2(APP_ID, USER_ID, subUser2, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", delegatePtr2, mgr2), INVALID_ARGS);
    ASSERT_EQ(delegatePtr2, nullptr);

    std::string subUser3 = std::string(128, 'a');
    KvStoreDelegateManager mgr3(APP_ID, USER_ID, subUser3, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr3 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", delegatePtr3, mgr3), OK);
    ASSERT_NE(delegatePtr3, nullptr);

    CloseDelegate(delegatePtr3, mgr3, STORE_ID_1);
}

/**
 * @tc.name: RDBDelegateInvalidParamTest001
 * @tc.desc: Test rdb delegate open with invalid subUser.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, RDBDelegateInvalidParamTest001, TestSize.Level1)
{
    std::string subUser1 = std::string(129, 'a');
    RelationalStoreManager mgr1(APP_ID, USER_ID, subUser1, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr1 = nullptr;
    sqlite3 *db1;
    EXPECT_EQ(OpenDelegate("/subUser1", rdbDelegatePtr1, mgr1, db1), INVALID_ARGS);
    ASSERT_EQ(rdbDelegatePtr1, nullptr);

    std::string subUser2 = "subUser-1";
    RelationalStoreManager mgr2(APP_ID, USER_ID, subUser2, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr2 = nullptr;
    sqlite3 *db2;
    EXPECT_EQ(OpenDelegate("/subUser1", rdbDelegatePtr2, mgr2, db2), INVALID_ARGS);
    ASSERT_EQ(rdbDelegatePtr2, nullptr);

    std::string subUser3 = std::string(128, 'a');
    RelationalStoreManager mgr3(APP_ID, USER_ID, subUser3, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr3 = nullptr;
    sqlite3 *db3;
    EXPECT_EQ(OpenDelegate("/subUser1", rdbDelegatePtr3, mgr3, db3), OK);
    ASSERT_NE(rdbDelegatePtr3, nullptr);

    CloseDelegate(rdbDelegatePtr3, mgr3, db3);
}

/**
 * @tc.name: SameDelegateTest001
 * @tc.desc: Test kv delegate open with diff subUser.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, SameDelegateTest001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", delegatePtr1, mgr1), OK);
    ASSERT_NE(delegatePtr1, nullptr);

    KvStoreDelegateManager mgr2(APP_ID, USER_ID, SUB_USER_2, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser2", delegatePtr2, mgr2), OK);
    ASSERT_NE(delegatePtr2, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    delegatePtr1->Put(key1, value1);
    Key key2 = {'k', '2'};
    Value value2 = {'v', '2'};
    delegatePtr2->Put(key2, value2);

    Value value;
    EXPECT_EQ(delegatePtr1->Get(key1, value), OK);
    EXPECT_EQ(value1, value);
    EXPECT_EQ(delegatePtr2->Get(key2, value), OK);
    EXPECT_EQ(value2, value);

    EXPECT_EQ(delegatePtr1->Get(key2, value), NOT_FOUND);
    EXPECT_EQ(delegatePtr2->Get(key1, value), NOT_FOUND);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
    CloseDelegate(delegatePtr2, mgr2, STORE_ID_1);
}

/**
 * @tc.name: SameDelegateTest002
 * @tc.desc: Test rdb delegate open with diff subUser.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, SameDelegateTest002, TestSize.Level1)
{
    RelationalStoreManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr1 = nullptr;
    sqlite3 *db1;
    EXPECT_EQ(OpenDelegate("/subUser1", rdbDelegatePtr1, mgr1, db1), OK);
    ASSERT_NE(rdbDelegatePtr1, nullptr);

    RelationalStoreManager mgr2(APP_ID, USER_ID, SUB_USER_2, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr2 = nullptr;
    sqlite3 *db2;
    EXPECT_EQ(OpenDelegate("/subUser2", rdbDelegatePtr2, mgr2, db2), OK);
    ASSERT_NE(rdbDelegatePtr2, nullptr);

    int localCount = 10;
    InsertLocalData(0, localCount, ASSETS_TABLE_NAME, true, db1);
    InsertLocalData(10, localCount, ASSETS_TABLE_NAME, true, db2);

    std::string sql1 = "select count(*) from " + ASSETS_TABLE_NAME + " where id < 10;";
    std::string sql2 = "select count(*) from " + ASSETS_TABLE_NAME + " where id >= 10;";

    EXPECT_EQ(sqlite3_exec(db1, sql1.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(db2, sql2.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);

    EXPECT_EQ(sqlite3_exec(db1, sql2.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(db2, sql1.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);

    CloseDelegate(rdbDelegatePtr1, mgr1, db1);
    CloseDelegate(rdbDelegatePtr2, mgr2, db2);
}

/**
 * @tc.name: SubUserDelegateCRUDTest001
 * @tc.desc: Test subUser rdb delegate crud function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, SubUserDelegateCRUDTest001, TestSize.Level1)
{
    RelationalStoreManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr1 = nullptr;
    sqlite3 *db1;
    EXPECT_EQ(OpenDelegate("/subUser1", rdbDelegatePtr1, mgr1, db1), OK);
    ASSERT_NE(rdbDelegatePtr1, nullptr);

    int localCount = 10;
    InsertLocalData(0, localCount, ASSETS_TABLE_NAME, true, db1);
    std::string sql1 = "select count(*) from " + ASSETS_TABLE_NAME + ";";
    EXPECT_EQ(sqlite3_exec(db1, sql1.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);

    InsertLocalData(0, localCount, ASSETS_TABLE_NAME, true, db1);
    EXPECT_EQ(sqlite3_exec(db1, sql1.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);

    std::string sql2 = "delete from " + ASSETS_TABLE_NAME + ";";
    EXPECT_EQ(sqlite3_exec(db1, sql2.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(db1, sql1.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);

    CloseDelegate(rdbDelegatePtr1, mgr1, db1);
}

/**
 * @tc.name: SubUserDelegateCRUDTest002
 * @tc.desc: Test subUser kv delegate crud function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, SubUserDelegateCRUDTest002, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", delegatePtr1, mgr1), OK);
    ASSERT_NE(delegatePtr1, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    delegatePtr1->Put(key1, value1);

    Value value;
    EXPECT_EQ(delegatePtr1->Get(key1, value), OK);
    EXPECT_EQ(value1, value);

    Value value2 = {'v', '2'};
    delegatePtr1->Put(key1, value2);
    EXPECT_EQ(delegatePtr1->Get(key1, value), OK);
    EXPECT_EQ(value2, value);

    EXPECT_EQ(delegatePtr1->Delete(key1), OK);
    EXPECT_EQ(delegatePtr1->Get(key1, value), NOT_FOUND);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
}

/**
 * @tc.name: SubUserDelegateCloudSyncTest001
 * @tc.desc: Test subUser kv delegate cloud sync function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, SubUserDelegateCloudSyncTest001, TestSize.Level0)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", delegatePtr1, mgr1), OK);
    ASSERT_NE(delegatePtr1, nullptr);
    EXPECT_EQ(SetCloudDB(delegatePtr1), OK);

    KvStoreDelegateManager mgr2(APP_ID, USER_ID, SUB_USER_2, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser2", delegatePtr2, mgr2), OK);
    ASSERT_NE(delegatePtr2, nullptr);
    EXPECT_EQ(SetCloudDB(delegatePtr2), OK);

    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(delegatePtr1->Put(key, expectValue), OK);
    BlockSync(delegatePtr1, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
    }
    BlockSync(delegatePtr2, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.downLoadInfo.total, 1u);
    }
    Value actualValue;
    EXPECT_EQ(delegatePtr2->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
    CloseDelegate(delegatePtr2, mgr2, STORE_ID_1);
}

/**
 * @tc.name: SubUserDelegateCloudSyncTest002
 * @tc.desc: Test subUser rdb delegate cloud sync function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, SubUserDelegateCloudSyncTest002, TestSize.Level0)
{
    RelationalStoreManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr1 = nullptr;
    sqlite3 *db1;
    EXPECT_EQ(OpenDelegate("/subUser1", rdbDelegatePtr1, mgr1, db1), OK);
    ASSERT_NE(rdbDelegatePtr1, nullptr);
    EXPECT_EQ(SetCloudDB(rdbDelegatePtr1), OK);

    RelationalStoreManager mgr2(APP_ID, USER_ID, SUB_USER_2, INSTANCE_ID_1);
    RelationalStoreDelegate *rdbDelegatePtr2 = nullptr;
    sqlite3 *db2;
    EXPECT_EQ(OpenDelegate("/subUser2", rdbDelegatePtr2, mgr2, db2), OK);
    ASSERT_NE(rdbDelegatePtr2, nullptr);
    EXPECT_EQ(SetCloudDB(rdbDelegatePtr2), OK);

    int localCount = 10;
    InsertLocalData(0, localCount, ASSETS_TABLE_NAME, true, db1);
    CloudSyncOption option = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::NONE);
    CallSync(rdbDelegatePtr1, option);
    CallSync(rdbDelegatePtr2, option);

    std::string sql = "select count(*) from " + ASSETS_TABLE_NAME + ";";
    EXPECT_EQ(sqlite3_exec(db2, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);

    CloseDelegate(rdbDelegatePtr1, mgr1, db1);
    CloseDelegate(rdbDelegatePtr2, mgr2, db2);
}

/**
 * @tc.name: MultiSubUserDelegateSync001
 * @tc.desc: Test subUser delegate sync function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, MultiSubUserDelegateSync001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    RuntimeConfig::SetPermissionCheckCallback([](const PermissionCheckParam &param, uint8_t flag) {
        if ((flag & PermissionCheckFlag::CHECK_FLAG_RECEIVE) != 0) {
            bool res = false;
            if (param.extraConditions.find(KEY_SUB_USER) != param.extraConditions.end()) {
                res = param.extraConditions.at(KEY_SUB_USER) == SUB_USER_1;
            }
            return res;
        }
        if (param.userId != USER_ID || param.appId != APP_ID || param.storeId != STORE_ID_1 ||
            (param.instanceId != INSTANCE_ID_1 && param.instanceId != 0)) {
            return false;
        }
        return true;
    });
    RuntimeConfig::SetPermissionConditionCallback([](const PermissionConditionParam &param) {
        std::map<std::string, std::string> res;
        res.emplace(KEY_SUB_USER, SUB_USER_1);
        return res;
    });

    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", delegatePtr1, mgr1), OK);
    ASSERT_NE(delegatePtr1, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    delegatePtr1->Put(key1, value1);

    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(delegatePtr1, { DEVICE_B }, SYNC_MODE_PUSH_ONLY, result);
    EXPECT_TRUE(status == OK);
    EXPECT_EQ(result[DEVICE_B], OK);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
}

/**
 * @tc.name: MultiSubUserDelegateSync002
 * @tc.desc: Test subUser delegate sync active if callback return true.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(DistributedDBSingleVerMultiSubUserTest, MultiSubUserDelegateSync002, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    RuntimeConfig::SetSyncActivationCheckCallback([](const ActivationCheckParam &param) {
        if (param.userId == USER_ID && param.appId == APP_ID && param.storeId == STORE_ID_1 &&
            param.instanceId == INSTANCE_ID_1) {
            return true;
        }
        return false;
    });

    KvStoreNbDelegate *delegatePtr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", delegatePtr1, mgr1, true), OK);
    ASSERT_NE(delegatePtr1, nullptr);

    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(delegatePtr1, { DEVICE_B }, SYNC_MODE_PUSH_ONLY, result);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(result[DEVICE_B], OK);

    CloseDelegate(delegatePtr1, mgr1, STORE_ID_1);
}

} // namespace