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
    VirtualCommunicatorAggregator* g_communAggregator = nullptr;
    const std::string DEVICE_A = "real_device";
    const std::string DEVICE_B = "deviceB";
    const std::string KEY_INSTANCE_ID = "INSTANCE_ID";
    const std::string KEY_SUB_USER = "SUB_USER";
    KvVirtualDevice *g_deviceB = nullptr;
    KvDBToolsUnitTest g_tool;
    CloudSyncOption g_CloudSyncoption;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = nullptr;
    std::shared_ptr<VirtualCloudDataTranslate> g_virtualCloudDataTranslate;
    const string ASSETS_TABLE_NAME = "student";
    const string ASSETS_TABLE_NAME_SHARED = "student_shared";
    const string COL_ID = "id";
    const string COL_NAME = "name";
    const string COL_ASSERT = "assetTmp";
    const string COL_ASSERTS = "assets";
    const int64_t WAIT_TIME = 5;
    const std::vector<Field> CLOUD_FIELDS = {{cloId, TYPE_INDEX<int64_t>, true}, {colName, TYPE_INDEX<std::string>},
        {colAsset, TYPE_INDEX<Asset>}, {colAssert, TYPE_INDEX<Assets>}};
    const string CREATE_SINGLE_PRIMARY_KEY_TABLE = "CREATE TABLE IF NOT EXISTS " + ASSETS_TABLE_NAME + "(" + cloId +
        " INTEGER PRIMARY KEY," + colName + " TEXT ," + colAsset + " ASSET," + colAssert + " ASSETS" + ");";
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

    DBStatus OpenDelegate(const std::string &path, KvStoreNbDelegate *&kvPtr,
        KvStoreDelegateManager &mgr, bool syncDualTupleMode = false)
    {
        if (g_testDir == nullptr) {
            return DB_ERROR;
        }
        std::string dbPath = *g_testDir + path;
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

        KvStoreNbDelegate::Option syncOption;
        syncOption.syncDualTupleMode = syncDualTupleMode;
        DBStatus res = OK;
        mgr.GetKvStore(STORE_ID_1, syncOption, [&kvPtr, &res](DBStatus status, KvStoreNbDelegate *delegate) {
            kvPtr = delegate;
            res = status;
        });
        return res;
    }

    DataBaseSchema GetDataBaseSchema()
    {
        DataBaseSchema schema;
        TableSchema tableSchema;
        tableSchema.name = CloudDbConstant::CLOUD_KV_TABLE_NAME;
        Field file;
        file.colName = CloudDbConstant::CLOUD_KV_FIELD_KEY;
        file.type = TYPE_INDEX<std::string>;
        file.primary = true;
        tableSchema.fields.push_back(file);
        file.colName = CloudDbConstant::CLOUD_KV_FIELD_DEVICE;
        file.primary = false;
        tableSchema.fields.push_back(file);
        file.colName = CloudDbConstant::CLOUD_KV_FIELD_ORI_DEVICE;
        tableSchema.fields.push_back(file);
        file.colName = CloudDbConstant::CLOUD_KV_FIELD_VALUE;
        tableSchema.fields.push_back(file);
        file.colName = CloudDbConstant::CLOUD_KV_FIELD_DEVICE_CREATE_TIME;
        file.type = TYPE_INDEX<int64_t>;
        tableSchema.fields.push_back(file);
        schema.tables.push_back(tableSchema);
        return schema;
    }

    DBStatus SetCloudDB(KvStoreNbDelegate *&kvPtr)
    {
        std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
        cloudDbs[USER_ID] = virtualCloudDb;
        kvPtr->SetCloudDB(cloudDbs);
        std::map<std::string, DataBaseSchema> schemas;
        schemas[USER_ID] = GetDataBaseSchema();
        return kvPtr->SetCloudDbSchema(schemas);
    }

    DBStatus OpenDelegate(const std::string &path, KvStoreDelegate *&rdbDelegatePtr,
        RelationalStoreManager &mgr, sqlite3 *&db)
    {
        if (g_testDir == nullptr) {
            return DB_ERROR;
        }
        std::string dbDir = *g_testDir + path;
        OS::MakeDBDirectory(dbDir);
        std::string dbPath = dbDir + "/test.db";
        db = RelationalTest::CreateDataBase(dbPath);
        EXPECT_EQ(RelationalTest::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(RelationalTest::ExecSql(db, CREATE_SINGLE_PRIMARY_KEY_TABLE), SQLITE_OK);
        RelationalStoreObserverUnitTest *observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        KvStoreDelegate::Option syncOption = { .observer = observer };
        return mgr.OpenStore(dbPath, STORE_ID_1, syncOption, rdbDelegatePtr);
    }

    void GetCloudDbSchema(DataBaseSchema &dataBase)
    {
        TableSchema assetsTableSchema = {.name = ASSETS_TABLE_NAME, .sharedTableName = ASSETS_TABLE_NAME_SHARED,
            .fields = CLOUD_FIELDS};
        dataBase.tables.push_back(assetsTableSchema);
    }

    DBStatus SetCloudDB(KvStoreDelegate *&kvPtr)
    {
        EXPECT_EQ(kvPtr->CreateDistributedTable(ASSETS_TABLE_NAME, CLOUD_COOPERATION), DBStatus::OK);
        std::shared_ptr<VirtualAssetLoader> virtualAssetLoader = make_shared<VirtualAssetLoader>();
        EXPECT_EQ(kvPtr->SetCloudDB(virtualCloudDb), DBStatus::OK);
        EXPECT_EQ(kvPtr->SetIAssetLoader(virtualAssetLoader), DBStatus::OK);
        DataBaseSchema dataBase;
        GetCloudDbSchema(dataBase);
        return kvPtr->SetCloudDbSchema(dataBase);
    }

    void CloseDelegate(KvStoreNbDelegate *&kvPtr, KvStoreDelegateManager &mgr, std::string storeID)
    {
        if (kvPtr == nullptr) {
            return;
        }
        EXPECT_EQ(mgr.CloseKvStore(kvPtr), OK);
        kvPtr = nullptr;
        EXPECT_EQ(mgr.DeleteKvStore(storeID), OK);
    }

    void CloseDelegate(KvStoreDelegate *&kvPtr, RelationalStoreManager &mgr, sqlite3 *&db)
    {
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
        db = nullptr;
        if (kvPtr == nullptr) {
            return;
        }
        EXPECT_EQ(mgr.CloseStore(kvPtr), OK);
        kvPtr = nullptr;
    }

class KvDBSingleVerMultiSubUserTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    void BlockSync(KvStoreNbDelegate *del, DBStatus expectDBStatus, CloudSyncOption syncOption,
        int expectSyncResult = OK);
    void InsertLocalData(int64_t begin, int64_t count, const std::string &tableName, bool isAssetNull, sqlite3 *&db);
    void GenerateDataRecords(int64_t begin, int64_t count, int64_t gidStart, std::vector<VBucket> &vec,
        std::vector<VBucket> &extend);
    CloudSyncOption PrepareOption(const Query &query, LockAction action);
    void CallSync(KvStoreDelegate *del, const CloudSyncOption &syncOption, DBStatus expectResult = OK);
    SyncProcess lastProcess_;
};

void KvDBSingleVerMultiSubUserTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    std::string dir;
    KvDBToolsUnitTest::TestDirInit(dir);
    if (g_testDir == nullptr) {
        g_testDir = std::make_shared<std::string>(dir);
    }

    g_communAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communAggregator);

    syncOption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    syncOption.users.push_back(USER_ID);
    syncOption.devices.push_back("cloud");
    g_virtualCloudDataTranslate = std::make_shared<VirtualCloudDataTranslate>();
    Runtime::SetCloudTranslate(g_virtualCloudDataTranslate);
}

void KvDBSingleVerMultiSubUserTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (g_testDir != nullptr && KvDBToolsUnitTest::RemoveTestDbFiles(*g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void KvDBSingleVerMultiSubUserTest::SetUp(void)
{
    virtualCloudDb = std::make_shared<VirtualCloudDb>();
    KvDBToolsUnitTest::PrintTestCaseInfo();
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communAggregator, syncInterfaceB), E_OK);
}

void KvDBSingleVerMultiSubUserTest::TearDown(void)
{
    virtualCloudDb = nullptr;
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    PermissionCheckCallbackV3 callback = nullptr;
    Runtime::SetPermissionCheckCallback(callback);
    SyncActivationCheckCallbackV2 activeCallBack = nullptr;
    Runtime::SetSyncActivationCheck(activeCallBack);
    Runtime::SetPermissionConditionCallback(nullptr);
    if (KvDBToolsUnitTest::RemoveTestDbFiles(*g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

void KvDBSingleVerMultiSubUserTest::BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus,
    CloudSyncOption syncOption, int expectSyncResult)
{
    if (delegate == nullptr) {
        return;
    }
    std::mutex dataMutex;
    std::condition_variable cv;
    bool isFinish = false;
    SyncProcess last;
    auto callback = [expectDBStatus, &last, &cv, &dataMutex, &isFinish, &syncOption](const std::map<std::string,
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
                if (notifyCnt == syncOption.users.size()) {
                    isFinish = true;
                    last = item.second;
                    cv.notify_one();
                }
            }
        }
    };
    EXPECT_EQ(delegate->Sync(syncOption, callback), expectSyncResult);
    if (expectSyncResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&isFinish]() {
            return isFinish;
        });
    }
    lastProcess_ = last;
}

void KvDBSingleVerMultiSubUserTest::GenerateDataRecords(
    int64_t begin, int64_t count, int64_t gidStart, std::vector<VBucket> &vec, std::vector<VBucket> &extend)
{
    for (int64_t i = begin; i < begin + count; i++) {
        Assets assets;
        Asset assetTmp = ASSET_COPY;
        assetTmp.name = ASSET_COPY.name + std::to_string(i);
        assets.emplace_back(assetTmp);
        assetTmp.name = ASSET_COPY.name + std::to_string(i) + "_copy";
        assets.emplace_back(assetTmp);
        VBucket data;
        data.insert_or_assign(cloId, i);
        data.insert_or_assign(colName, "name" + std::to_string(g_nameId++));
        data.insert_or_assign(colAssert, assets);
        vec.push_back(data);

        VBucket log;
        Timestamp now = TimeHelper::GetSysCurrentTime();
        log.insert_or_assign(CloudDbConstant::CREATE_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND);
        log.insert_or_assign(CloudDbConstant::MODIFY_FIELD, (int64_t)now / CloudDbConstant::TEN_THOUSAND);
        log.insert_or_assign(CloudDbConstant::DELETE_FIELD, false);
        log.insert_or_assign(CloudDbConstant::GID_FIELD, std::to_string(i + gidStart));
        extend.push_back(log);
    }
}

void KvDBSingleVerMultiSubUserTest::InsertLocalData(int64_t begin, int64_t count,
    const std::string &tableName, bool isAssetNull, sqlite3 *&db)
{
    int errCode;
    std::vector<VBucket> vec;
    std::vector<VBucket> extend;
    GenerateDataRecords(begin, count, 0, vec, extend);
    const string sql = "insert or replace into " + tableName + " values (?,?,?,?);";
    for (VBucket vBucket : vec) {
        sqlite3_stmt *stmt = nullptr;
        ASSERT_EQ(SQLiteUtil::GetStatement(db, sql, stmt), E_OK);
        ASSERT_EQ(SQLiteUtil::BindInt64ToStatement(stmt, 1, std::get<int64_t>(vBucket[cloId])), E_OK); // 1 is id
        ASSERT_EQ(SQLiteUtil::BindTextToStatement(stmt, 2, std::get<string>(vBucket[colName])), E_OK); // 2 is name
        if (isAssetNull) {
            ASSERT_EQ(sqlite3_bind_null(stmt, 3), SQLITE_OK); // 3 is assetTmp
        } else {
            std::vector<uint8_t> assetBlob = g_virtualCloudDataTranslate->AssetToBlob(ASSET_COPY);
            ASSERT_EQ(SQLiteUtil::BindBlobToStatement(stmt, 3, assetBlob, false), E_OK); // 3 is assetTmp
        }
        std::vector<uint8_t> assetsBlob = g_virtualCloudDataTranslate->AssetsToBlob(
            std::get<Assets>(vBucket[colAssert]));
        ASSERT_EQ(SQLiteUtil::BindBlobToStatement(stmt, 4, assetsBlob, false), E_OK); // 4 is assets
        EXPECT_EQ(SQLiteUtil::StepWithRetry(stmt), SQLiteUtil::MapSQLiteErrno(SQLITE_DONE));
        SQLiteUtil::ResetStatement(stmt, true, errCode);
    }
}

CloudSyncOption KvDBSingleVerMultiSubUserTest::PrepareOption(const Query &query, LockAction action)
{
    CloudSyncOption syncOption;
    syncOption.devices = { "CLOUD" };
    syncOption.mode = SYNC_MODE_CLOUD_MERGE;
    syncOption.query = query;
    syncOption.waitTime = WAIT_TIME;
    syncOption.priorityTask = false;
    syncOption.compensatedSyncOnly = false;
    syncOption.lockAction = action;
    return syncOption;
}

void KvDBSingleVerMultiSubUserTest::CallSync(KvStoreDelegate *delegate, const CloudSyncOption &syncOption,
    DBStatus expectResult)
{
    std::mutex dataMutex;
    std::condition_variable cv;
    bool isFinish = false;
    auto callback = [&cv, &dataMutex, &isFinish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    isFinish = true;
                }
                cv.notify_one();
            }
        }
    };
    ASSERT_EQ(delegate->Sync(syncOption, callback), expectResult);
    if (expectResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&isFinish]() {
            return isFinish;
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
HWTEST_F(KvDBSingleVerMultiSubUserTest, KvDelegateInvalidParamTest001, TestSize.Level1)
{
    std::string user1 = std::string(129, 'a');
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, user1, INSTANCE_ID_1);
    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/user1", ptr1, mgr1), INVALID_ARGS);
    ASSERT_EQ(ptr1, nullptr);

    std::string user2 = "user1-1";
    KvStoreDelegateManager mgr2(APP_ID, USER_ID, user2, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/user1", delegatePtr2, mgr2), INVALID_ARGS);
    ASSERT_EQ(delegatePtr2, nullptr);

    std::string user3 = std::string(128, 'a');
    KvStoreDelegateManager mgr3(APP_ID, USER_ID, user3, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr3 = nullptr;
    EXPECT_EQ(OpenDelegate("/user1", delegatePtr3, mgr3), OK);
    ASSERT_NE(delegatePtr3, nullptr);

    CloseDelegate(delegatePtr3, mgr3, STORE_ID_1);
}

/**
 * @tc.name: SubUserPermissionCheck
 * @tc.desc: permission check subuser
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: luoguo
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, SubUserPermissionCheck, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set permission check callback
     * @tc.expected: step1. set OK.
     */
    std::string user1 = std::string(128, 'a');
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, user1, INSTANCE_ID_1);
    auto permissionCheckCallback = [] (const PermissionCheckParamV4 &param, uint8_t flag) -> bool {
        if (param.deviceId == g_deviceB->GetDeviceId() && (flag && CHECK_FLAG_SPONSOR)) {
            LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
            return false;
        } else {
            LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
            return true;
        }
    };
    EXPECT_EQ(mgr1.SetPermissionCheckCallback(permissionCheckCallback), OK);

    /**
     * @tc.steps: step2. deviceB put {1,1}
     * @tc.expected: step2. put success
     */
    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/user1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);

    DBStatus status = OK;
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    Key key = {'1'};
    Value value = {'1'};
    g_deviceB->PutData(key, value, 0, 0);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceB push data
     * @tc.expected: step3. return PERMISSION_CHECK_FORBID_SYNC.
     */
    std::map<std::string, DBStatus> error;
    error = g_tool.SyncTest(ptr1, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(error == OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));

    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, error %d", pair.first.c_str(), pair.second);
        if (g_deviceB->GetDeviceId() == pair.first) {
            EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
        } else {
            EXPECT_TRUE(pair.second == OK);
        }
    }

    PermissionCheckCallbackV4 callback = nullptr;
    EXPECT_EQ(mgr1.SetPermissionCheckCallback(callback), OK);
}

/**
 * @tc.name: RDBDelegateInvalidParamTest001
 * @tc.desc: Test rdb delegate open with invalid subUser.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, RDBDelegateInvalidParamTest001, TestSize.Level1)
{
    std::string subUser1 = std::string(129, 'a');
    RelationalStoreManager mgr1(APP_ID, USER_ID, subUser1, INSTANCE_ID_1);
    KvStoreDelegate *kvptr1 = nullptr;
    sqlite3 *db1;
    EXPECT_EQ(OpenDelegate("/subUser1", kvptr1, mgr1, db1), INVALID_ARGS);
    ASSERT_EQ(kvptr1, nullptr);

    std::string subUser2 = "subUser-1";
    RelationalStoreManager mgr2(APP_ID, USER_ID, subUser2, INSTANCE_ID_1);
    KvStoreDelegate *kvDelegatePtr2 = nullptr;
    sqlite3 *db2;
    EXPECT_EQ(OpenDelegate("/subUser1", kvDelegatePtr2, mgr2, db2), INVALID_ARGS);
    ASSERT_EQ(kvDelegatePtr2, nullptr);

    std::string subUser3 = std::string(128, 'a');
    RelationalStoreManager mgr3(APP_ID, USER_ID, subUser3, INSTANCE_ID_1);
    KvStoreDelegate *kvDelegatePtr3 = nullptr;
    sqlite3 *db3;
    EXPECT_EQ(OpenDelegate("/subUser1", kvDelegatePtr3, mgr3, db3), OK);
    ASSERT_NE(kvDelegatePtr3, nullptr);

    CloseDelegate(kvDelegatePtr3, mgr3, db3);
}

/**
 * @tc.name: SameDelegateTest001
 * @tc.desc: Test kv delegate open with diff subUser.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, SameDelegateTest001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);

    KvStoreDelegateManager mgr2(APP_ID, USER_ID, SUB_USER_2, INSTANCE_ID_1);
    KvStoreNbDelegate *delegatePtr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser2", delegatePtr2, mgr2), OK);
    ASSERT_NE(delegatePtr2, nullptr);

    Key key1 = {'k', '1'};
    Value value2 = {'v', '1'};
    ptr1->Put(key1, value2);
    Key key2 = {'k', '2'};
    Value value2 = {'v', '2'};
    delegatePtr2->Put(key2, value2);

    Value value;
    EXPECT_EQ(ptr1->Get(key1, value), OK);
    EXPECT_EQ(value2, value);
    EXPECT_EQ(delegatePtr2->Get(key2, value), OK);
    EXPECT_EQ(value2, value);

    EXPECT_EQ(ptr1->Get(key2, value), NOT_FOUND);
    EXPECT_EQ(delegatePtr2->Get(key1, value), NOT_FOUND);

    CloseDelegate(ptr1, mgr1, STORE_ID_1);
    CloseDelegate(delegatePtr2, mgr2, STORE_ID_1);
}

/**
 * @tc.name: SameDelegateTest002
 * @tc.desc: Test rdb delegate open with diff subUser.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, SameDelegateTest002, TestSize.Level1)
{
    RelationalStoreManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreDelegate *kvptr1 = nullptr;
    sqlite3 *db1;
    EXPECT_EQ(OpenDelegate("/subUser1", kvptr1, mgr1, db1), OK);
    ASSERT_NE(kvptr1, nullptr);

    RelationalStoreManager mgr2(APP_ID, USER_ID, SUB_USER_2, INSTANCE_ID_1);
    KvStoreDelegate *kvDelegatePtr2 = nullptr;
    sqlite3 *db2;
    EXPECT_EQ(OpenDelegate("/subUser2", kvDelegatePtr2, mgr2, db2), OK);
    ASSERT_NE(kvDelegatePtr2, nullptr);

    int count = 10;
    InsertLocalData(0, count, ASSETS_TABLE_NAME, true, db1);
    InsertLocalData(10, count, ASSETS_TABLE_NAME, true, db2);

    std::string sql = "select count(*) from " + ASSETS_TABLE_NAME + " where id < 10;";
    std::string sql2 = "select count(*) from " + ASSETS_TABLE_NAME + " where id >= 10;";

    EXPECT_EQ(sqlite3_exec(db1, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(db2, sql2.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);

    EXPECT_EQ(sqlite3_exec(db1, sql2.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(db2, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);

    CloseDelegate(kvptr1, mgr1, db1);
    CloseDelegate(kvDelegatePtr2, mgr2, db2);
}

/**
 * @tc.name: SubUserDelegateCRUDTest001
 * @tc.desc: Test subUser rdb delegate crud function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, SubUserDelegateCRUDTest001, TestSize.Level1)
{
    RelationalStoreManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreDelegate *kvptr1 = nullptr;
    sqlite3 *db1;
    EXPECT_EQ(OpenDelegate("/subUser1", kvptr1, mgr1, db1), OK);
    ASSERT_NE(kvptr1, nullptr);

    int count = 10;
    InsertLocalData(0, count, TABLE_NAME, true, db1);
    std::string sql = "select count(*) from " + TABLE_NAME + ";";
    EXPECT_EQ(sqlite3_exec(db1, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);

    InsertLocalData(0, count, TABLE_NAME, true, db1);
    EXPECT_EQ(sqlite3_exec(db1, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);

    std::string sql2 = "delete from " + TABLE_NAME + ";";
    EXPECT_EQ(sqlite3_exec(db1, sql2.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);
    EXPECT_EQ(sqlite3_exec(db1, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(0), nullptr), SQLITE_OK);

    CloseDelegate(kvptr1, mgr1, db1);
}

/**
 * @tc.name: SubUserDelegateCRUDTest002
 * @tc.desc: Test subUser kv delegate crud function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, SubUserDelegateCRUDTest002, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);

    Key key1 = {'k', '1'};
    Value value2 = {'v', '1'};
    ptr1->Put(key1, value2);

    Value value3;
    EXPECT_EQ(ptr1->Get(key1, value3), OK);
    EXPECT_EQ(value2, value3);

    Value value2 = {'v', '2'};
    ptr1->Put(key1, value2);
    EXPECT_EQ(ptr1->Get(key1, value3), OK);
    EXPECT_EQ(value2, value3);

    EXPECT_EQ(ptr1->Delete(key1), OK);
    EXPECT_EQ(ptr1->Get(key1, value3), NOT_FOUND);

    CloseDelegate(ptr1, mgr1, STORE_ID_1);
}

/**
 * @tc.name: SubUserDelegateCloudSyncTest001
 * @tc.desc: Test subUser kv delegate cloud sync function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, SubUserDelegateCloudSyncTest001, TestSize.Level0)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(SetCloudDB(ptr1), OK);

    KvStoreDelegateManager mgr2(APP_ID, USER_ID, SUB_USER_2, INSTANCE_ID_1);
    KvStoreNbDelegate *ptr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser2", ptr2, mgr2), OK);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(SetCloudDB(ptr2), OK);

    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(ptr1->Put(key, expectValue), OK);
    BlockSync(ptr1, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
    }
    BlockSync(ptr2, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.downLoadInfo.total, 1u);
    }
    Value actualValue;
    EXPECT_EQ(ptr2->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);

    CloseDelegate(ptr1, mgr1, STORE_ID_1);
    CloseDelegate(ptr2, mgr2, STORE_ID_1);
}

/**
 * @tc.name: SubUserDelegateCloudSyncTest002
 * @tc.desc: Test subUser rdb delegate cloud sync function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, SubUserDelegateCloudSyncTest002, TestSize.Level0)
{
    RelationalStoreManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    KvStoreDelegate *kvptr1 = nullptr;
    sqlite3 *db1;
    EXPECT_EQ(OpenDelegate("/subUser1", kvptr1, mgr1, db1), OK);
    ASSERT_NE(kvptr1, nullptr);
    EXPECT_EQ(SetCloudDB(kvptr1), OK);

    RelationalStoreManager mgr2(APP_ID, USER_ID, SUB_USER_2, INSTANCE_ID_1);
    KvStoreDelegate *ptr2 = nullptr;
    sqlite3 *db2;
    EXPECT_EQ(OpenDelegate("/subUser2", ptr2, mgr2, db2), OK);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(SetCloudDB(ptr2), OK);

    int count = 10;
    InsertLocalData(0, count, ASSETS_TABLE_NAME, true, db1);
    CloudSyncOption syncOption = PrepareOption(Query::Select().FromTable({ ASSETS_TABLE_NAME }), LockAction::NONE);
    CallSync(kvptr1, syncOption);
    CallSync(ptr2, syncOption);

    std::string sql = "select count(*) from " + ASSETS_TABLE_NAME + ";";
    EXPECT_EQ(sqlite3_exec(db2, sql.c_str(), CloudDBSyncUtilsTest::QueryCountCallback,
        reinterpret_cast<void *>(10), nullptr), SQLITE_OK);

    CloseDelegate(kvptr1, mgr1, db1);
    CloseDelegate(ptr2, mgr2, db2);
}

/**
 * @tc.name: MultiSubUserDelegateSync001
 * @tc.desc: Test subUser delegate sync function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, MultiSubUserDelegateSync001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    Runtime::SetPermissionCheckCallback([](const PermissionCheckParam &param, uint8_t flag) {
        if ((flag & PermissionCheckFlag::CHECK_FLAG_RECEIVE) != 0) {
            bool ret = false;
            if (param.extraConditions.find(KEY_SUB_USER) != param.extraConditions.end()) {
                res = param.extraConditions.at(KEY_SUB_USER) == SUB_USER_1;
            }
            return ret;
        }
        if (param.userId != USER_ID || param.appId != APP_ID || param.storeId != STORE_ID_1 ||
            (param.instanceId != INSTANCE_ID_1 && param.instanceId != 0)) {
            return false;
        }
        return true;
    });
    Runtime::SetPermissionConditionCallback([](const PermissionConditionParam &param) {
        std::map<std::string, std::string> ret;
        ret.emplace(KEY_SUB_USER, SUB_USER_1);
        return ret;
    });

    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);

    Key key1 = {'k', '1'};
    Value value2 = {'v', '1'};
    ptr1->Put(key1, value2);

    std::map<std::string, DBStatus> ret;
    DBStatus status = g_tool.SyncTest(ptr1, { DEVICE_B }, SYNC_MODE_PUSH_ONLY, ret);
    EXPECT_TRUE(status == OK);
    EXPECT_EQ(ret[DEVICE_B], OK);

    CloseDelegate(ptr1, mgr1, STORE_ID_1);
}

/**
 * @tc.name: MultiSubUserDelegateSync002
 * @tc.desc: Test subUser delegate sync active if callback return true.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaoliang
 */
HWTEST_F(KvDBSingleVerMultiSubUserTest, MultiSubUserDelegateSync002, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, SUB_USER_1, INSTANCE_ID_1);
    Runtime::SetSyncActivationCheck([](const ActivationCheckParam &param) {
        if (param.userId == USER_ID && param.appId == APP_ID && param.storeId == STORE_ID_1 &&
            param.instanceId == INSTANCE_ID_1) {
            return true;
        }
        return false;
    });

    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/subUser1", ptr1, mgr1, true), OK);
    ASSERT_NE(ptr1, nullptr);

    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(ptr1, { DEVICE_B }, SYNC_MODE_PUSH_ONLY, result);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(result[DEVICE_B], OK);

    CloseDelegate(ptr1, mgr1, STORE_ID_1);
}

} // namespace