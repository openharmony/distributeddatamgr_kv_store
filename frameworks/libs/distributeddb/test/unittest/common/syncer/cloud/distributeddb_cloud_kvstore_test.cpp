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

#include "cloud/cloud_db_constant.h"
#include "db_base64_utils.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_virtual_device.h"
#include "kv_store_nb_delegate.h"
#include "kv_store_nb_delegate_impl.h"
#include "platform_specific.h"
#include "process_system_api_adapter_impl.h"
#include "virtual_communicator_aggregator.h"
#include "virtual_cloud_db.h"
#include "sqlite_utils.h"
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
static std::string HWM_HEAD = "naturalbase_cloud_meta_sync_data_";
string g_testDir;
KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
CloudSyncOption g_CloudSyncoption;
const std::string USER_ID_2 = "user2";
const std::string USER_ID_3 = "user3";
const Key KEY_1 = {'k', '1'};
const Key KEY_2 = {'k', '2'};
const Key KEY_3 = {'k', '3'};
const Value VALUE_1 = {'v', '1'};
const Value VALUE_2 = {'v', '2'};
const Value VALUE_3 = {'v', '3'};
class DistributedDBCloudKvStoreTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    void InsertRecord(int num);
    void SetDeviceId(const Key &key, const std::string &deviceId);
    void SetFlag(const Key &key, LogInfoFlag flag);
    int CheckFlag(const Key &key, LogInfoFlag flag);
    int CheckLogTable(const std::string &deviceId);
    int CheckWaterMark(const std::string &key);
    int ChangeUserId(const std::string &deviceId, const std::string &wantUserId);
    int ChangeHashKey(const std::string &deviceId);
protected:
    DBStatus GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId, KvStoreNbDelegate::Option option,
        bool invalidSchema = false);
    void CloseKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId);
    void BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus, CloudSyncOption option,
        DBStatus expectSyncResult = OK);
    void SyncAndGetProcessInfo(KvStoreNbDelegate *delegate, CloudSyncOption option);
    bool CheckUserSyncInfo(const vector<std::string> users, const vector<DBStatus> userStatus,
        const vector<Info> userExpectInfo);
    static DataBaseSchema GetDataBaseSchema(bool invalidSchema);
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    std::shared_ptr<VirtualCloudDb> virtualCloudDb2_ = nullptr;
    KvStoreConfig config_;
    KvStoreNbDelegate* kvDelegatePtrS1_ = nullptr;
    KvStoreNbDelegate* kvDelegatePtrS2_ = nullptr;
    SyncProcess lastProcess_;
    std::map<std::string, SyncProcess> lastSyncProcess_;
    VirtualCommunicatorAggregator *communicatorAggregator_ = nullptr;
    KvVirtualDevice *deviceB_ = nullptr;
};

void DistributedDBCloudKvStoreTest::SetUpTestCase()
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->ClearAllDeviceTimeInfo();
    g_CloudSyncoption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    g_CloudSyncoption.users.push_back(USER_ID);
    g_CloudSyncoption.devices.push_back("cloud");

    string dir = g_testDir + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }
}

void DistributedDBCloudKvStoreTest::TearDownTestCase()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBCloudKvStoreTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    config_.dataDir = g_testDir;
    /**
     * @tc.setup: create virtual device B and C, and get a KvStoreNbDelegate as deviceA
     */
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    virtualCloudDb2_ = std::make_shared<VirtualCloudDb>();
    g_mgr.SetKvStoreConfig(config_);
    KvStoreNbDelegate::Option option1;
    ASSERT_EQ(GetKvStore(kvDelegatePtrS1_, STORE_ID_1, option1), OK);
    // set aggregator after get store1, only store2 can sync with p2p
    communicatorAggregator_ = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator_ != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator_);
    KvStoreNbDelegate::Option option2;
    ASSERT_EQ(GetKvStore(kvDelegatePtrS2_, STORE_ID_2, option2), OK);

    deviceB_ = new (std::nothrow) KvVirtualDevice("DEVICE_B");
    ASSERT_TRUE(deviceB_ != nullptr);
    auto syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(deviceB_->Initialize(communicatorAggregator_, syncInterfaceB), E_OK);
}

void DistributedDBCloudKvStoreTest::TearDown()
{
    CloseKvStore(kvDelegatePtrS1_, STORE_ID_1);
    CloseKvStore(kvDelegatePtrS2_, STORE_ID_2);
    virtualCloudDb_ = nullptr;
    virtualCloudDb2_ = nullptr;
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }

    if (deviceB_ != nullptr) {
        delete deviceB_;
        deviceB_ = nullptr;
    }

    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    communicatorAggregator_ = nullptr;
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

void DistributedDBCloudKvStoreTest::BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus,
    CloudSyncOption option, DBStatus expectSyncResult)
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
                std::set<std::string> userSet(option.users.begin(), option.users.end());
                if (notifyCnt == userSet.size()) {
                    finish = true;
                    last = item.second;
                    cv.notify_one();
                }
            }
        }
    };
    auto actualRet = delegate->Sync(option, callback);
    EXPECT_EQ(actualRet, expectSyncResult);
    if (actualRet == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }
    lastProcess_ = last;
}

DataBaseSchema DistributedDBCloudKvStoreTest::GetDataBaseSchema(bool invalidSchema)
{
    DataBaseSchema schema;
    TableSchema tableSchema;
    tableSchema.name = invalidSchema ? "invalid_schema_name" : CloudDbConstant::CLOUD_KV_TABLE_NAME;
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


DBStatus DistributedDBCloudKvStoreTest::GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId,
    KvStoreNbDelegate::Option option, bool invalidSchema)
{
    DBStatus openRet = OK;
    g_mgr.GetKvStore(storeId, option, [&openRet, &delegate](DBStatus status, KvStoreNbDelegate *openDelegate) {
        openRet = status;
        delegate = openDelegate;
    });
    EXPECT_EQ(openRet, OK);
    EXPECT_NE(delegate, nullptr);

    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
    cloudDbs[USER_ID] = virtualCloudDb_;
    cloudDbs[USER_ID_2] = virtualCloudDb2_;
    delegate->SetCloudDB(cloudDbs);
    std::map<std::string, DataBaseSchema> schemas;
    schemas[USER_ID] = GetDataBaseSchema(invalidSchema);
    schemas[USER_ID_2] = GetDataBaseSchema(invalidSchema);
    return delegate->SetCloudDbSchema(schemas);
}

void DistributedDBCloudKvStoreTest::CloseKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId)
{
    if (delegate != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(delegate), OK);
        delegate = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(storeId);
        LOGD("delete kv store status %d store %s", status, storeId.c_str());
        ASSERT_EQ(status, OK);
    }
}

void DistributedDBCloudKvStoreTest::SyncAndGetProcessInfo(KvStoreNbDelegate *delegate, CloudSyncOption option)
{
    if (delegate == nullptr) {
        return;
    }
    std::mutex dataMutex;
    std::condition_variable cv;
    bool isFinish = false;
    vector<std::map<std::string, SyncProcess>> lists;
    auto callback = [&cv, &dataMutex, &isFinish, &option, &lists](const std::map<std::string, SyncProcess> &process) {
        size_t notifyCnt = 0;
        for (const auto &item: process) {
            LOGD("user = %s, status = %d", item.first.c_str(), item.second.process);
            if (item.second.process != DistributedDB::FINISHED) {
                continue;
            }
            {
                std::lock_guard<std::mutex> autoLock(dataMutex);
                notifyCnt++;
                std::set<std::string> userSet(option.users.begin(), option.users.end());
                if (notifyCnt == userSet.size()) {
                    isFinish = true;
                    cv.notify_one();
                }
                lists.push_back(process);
            }
        }
    };
    auto ret = delegate->Sync(option, callback);
    EXPECT_EQ(ret, OK);
    if (ret == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&isFinish]() {
            return isFinish;
        });
    }
    lastSyncProcess_ = lists.back();
}

bool DistributedDBCloudKvStoreTest::CheckUserSyncInfo(const vector<std::string> users,
    const vector<DBStatus> userStatus, const vector<Info> userExpectInfo)
{
    uint32_t idx = 0;
    for (auto &it: lastSyncProcess_) {
        if ((idx >= users.size()) || (idx >= userStatus.size()) || (idx >= userExpectInfo.size())) {
            return false;
        }
        string user = it.first;
        if (user.compare(0, user.length(), users[idx]) != 0) {
            return false;
        }
        SyncProcess actualSyncProcess = it.second;
        EXPECT_EQ(actualSyncProcess.process, FINISHED);
        EXPECT_EQ(actualSyncProcess.errCode, userStatus[idx]);
        for (const auto &table : actualSyncProcess.tableProcess) {
            EXPECT_EQ(table.second.upLoadInfo.total, userExpectInfo[idx].total);
            EXPECT_EQ(table.second.upLoadInfo.successCount, userExpectInfo[idx].successCount);
            EXPECT_EQ(table.second.upLoadInfo.insertCount, userExpectInfo[idx].insertCount);
            EXPECT_EQ(table.second.upLoadInfo.failCount, userExpectInfo[idx].failCount);
        }
        idx++;
    }
    return true;
}

/**
 * @tc.name: SyncOptionCheck001
 * @tc.desc: Test sync without user.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvStoreTest, SyncOptionCheck001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Device 1 inserts a piece of data.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    /**
     * @tc.steps:step2. Set option without user, and attempt to sync
     * @tc.expected: step2 return INVALID_ARGS.
     */
    CloudSyncOption option;
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS1_, OK, option, INVALID_ARGS);
    /**
     * @tc.steps:step3. Device 2 sync and attempt to get data.
     * @tc.expected: step3 sync OK but data NOT_FOUND.
     */
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), NOT_FOUND);
}

/**
 * @tc.name: SyncOptionCheck002
 * @tc.desc: Test sync with invalid waitTime.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvStoreTest, SyncOptionCheck002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Device 1 inserts a piece of data.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    /**
     * @tc.steps:step2. Set invalid waitTime of sync option and sync.
     * @tc.expected: step2 return INVALID_ARGS.
     */
    CloudSyncOption option;
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    option.waitTime = -2; // -2 is invalid waitTime.
    BlockSync(kvDelegatePtrS1_, OK, option, INVALID_ARGS);
    /**
     * @tc.steps:step3. Device 2 sync and attempt to get data.
     * @tc.expected: step3 sync OK but data NOT_FOUND.
     */
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), NOT_FOUND);
}

/**
 * @tc.name: SyncOptionCheck003
 * @tc.desc: Test sync with users which have not been sync to cloud.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvStoreTest, SyncOptionCheck003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Device 1 inserts a piece of data.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    /**
     * @tc.steps:step2. Set user1 and user3 to option and sync.
     * @tc.expected: step2 return INVALID_ARGS.
     */
    CloudSyncOption option;
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.users.push_back(USER_ID);
    option.users.push_back(USER_ID_3);
    option.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS1_, OK, option, INVALID_ARGS);
    /**
     * @tc.steps:step3. Device 2 sync and attempt to get data.
     * @tc.expected: step3 sync OK but data NOT_FOUND.
     */
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), NOT_FOUND);
}

/**
 * @tc.name: SyncOptionCheck004
 * @tc.desc: Test sync with user when schema is not same.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, SyncOptionCheck004, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Device 1 inserts a piece of data.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    /**
     * @tc.steps:step2. Set user1 to option and user2 to schema and sync.
     * @tc.expected: step2 return SCHEMA_MISMATCH.
     */
    CloudSyncOption option;
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    std::map<std::string, DataBaseSchema> schemas;
    schemas[USER_ID_2] = GetDataBaseSchema(false);
    kvDelegatePtrS1_->SetCloudDbSchema(schemas);
    BlockSync(kvDelegatePtrS1_, OK, option, SCHEMA_MISMATCH);
}

/**
 * @tc.name: SyncOptionCheck005
 * @tc.desc: Testing registration of observer exceeded the upper limit.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvStoreTest, SyncOptionCheck005, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Register MAX_OBSERVER_COUNT observers.
     * @tc.expected: step1 OK.
     */
    std::vector<KvStoreObserverUnitTest *> observerList;
    for (int i = 0; i < DBConstant::MAX_OBSERVER_COUNT; i++) {
        auto *observer = new (std::nothrow) KvStoreObserverUnitTest;
        observerList.push_back(observer);
        EXPECT_EQ(kvDelegatePtrS1_->RegisterObserver({}, OBSERVER_CHANGES_CLOUD, observer), OK);
    }
    /**
     * @tc.steps:step2. Register one more observer.
     * @tc.expected: step2 Registration failed, return OVER_MAX_LIMITS.
     */
    auto *overMaxObserver = new (std::nothrow) KvStoreObserverUnitTest;
    EXPECT_EQ(kvDelegatePtrS1_->RegisterObserver({}, OBSERVER_CHANGES_CLOUD, overMaxObserver), OVER_MAX_LIMITS);
    /**
     * @tc.steps:step3. UnRegister all observers.
     * @tc.expected: step3 OK.
     */
    EXPECT_EQ(kvDelegatePtrS1_->UnRegisterObserver(overMaxObserver), NOT_FOUND);
    delete overMaxObserver;
    overMaxObserver = nullptr;
    for (auto &observer : observerList) {
        EXPECT_EQ(kvDelegatePtrS1_->UnRegisterObserver(observer), OK);
        delete observer;
        observer = nullptr;
    }
}

/**
 * @tc.name: SyncOptionCheck006
 * @tc.desc: Test sync with schema db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvStoreTest, SyncOptionCheck006, TestSize.Level0)
{
    KvStoreNbDelegate::Option option;
    option.schema = "{\"SCHEMA_VERSION\":\"1.0\","
        "\"SCHEMA_MODE\":\"STRICT\","
        "\"SCHEMA_DEFINE\":{"
        "\"field_name1\":\"BOOL\","
        "\"field_name2\":\"INTEGER, NOT NULL\""
        "},"
        "\"SCHEMA_INDEXES\":[\"$.field_name1\", \"$.field_name2\"]}";
    KvStoreNbDelegate *kvDelegatePtr = nullptr;
    ASSERT_EQ(GetKvStore(kvDelegatePtr, STORE_ID_4, option), OK);
    ASSERT_NE(kvDelegatePtr, nullptr);
    BlockSync(kvDelegatePtr, NOT_SUPPORT, g_CloudSyncoption, NOT_SUPPORT);
    EXPECT_EQ(g_mgr.CloseKvStore(kvDelegatePtr), OK);
}

/**
 * @tc.name: SyncOptionCheck007
 * @tc.desc: Test sync with repetitive user.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvStoreTest, SyncOptionCheck007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Device 1 inserts a piece of data.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    /**
     * @tc.steps:step2. Set user1 2 times to option sync.
     * @tc.expected: step2 return OK.
     */
    CloudSyncOption option;
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.users.push_back(USER_ID);
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS1_, OK, option, OK);
}

/**
 * @tc.name: SyncOptionCheck008
 * @tc.desc: Test kc sync with query .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: luoguo
 */
HWTEST_F(DistributedDBCloudKvStoreTest, SyncOptionCheck008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Device 1 inserts a piece of data.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    /**
     * @tc.steps:step2. Set query to option sync.
     * @tc.expected: step2 return OK.
     */
    std::set<Key> keys;
    CloudSyncOption option;
    option.query = Query::Select().InKeys(keys);
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.users.push_back(USER_ID);
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS1_, OK, option, OK);
}

void DistributedDBCloudKvStoreTest::SetFlag(const Key &key, LogInfoFlag flag)
{
    sqlite3 *db_;
    uint64_t openFlag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    ASSERT_TRUE(sqlite3_open_v2(fileUrl.c_str(), &db_, openFlag, nullptr) == SQLITE_OK);
    int errCode = E_OK;
    std::string sql = "UPDATE sync_data SET flag=? WHERE Key=?";
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
    }
    ASSERT_EQ(errCode, E_OK);
    errCode = SQLiteUtils::BindInt64ToStatement(statement, 1, static_cast<int64_t>(flag)); // 1st arg.
    ASSERT_EQ(errCode, E_OK);
    errCode = SQLiteUtils::BindBlobToStatement(statement, 2, key, true); // 2nd arg.
    ASSERT_EQ(errCode, E_OK);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
    }
    EXPECT_EQ(SQLiteUtils::StepWithRetry(statement), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(statement, true, errCode);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close_v2(db_);
}

int DistributedDBCloudKvStoreTest::CheckFlag(const Key &key, LogInfoFlag flag)
{
    sqlite3 *db_;
    uint64_t openFlag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &db_, openFlag, nullptr);
    if (errCode != E_OK) {
        return NOT_FOUND;
    }
    std::string sql = "SELECT * FROM sync_data WHERE Key =? AND (flag=?)";
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return NOT_FOUND;
    }
    std::vector<uint8_t> keyVec(key.begin(), key.end());
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, keyVec, true); // 1st arg.
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return NOT_FOUND;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(statement, 2, static_cast<int64_t>(flag)); // 2nd arg.
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return NOT_FOUND;
    }
    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        sqlite3_close_v2(db_);
        return NOT_FOUND; // cant find.
    }
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        sqlite3_close_v2(db_);
        return OK;
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close_v2(db_);
    return NOT_FOUND;
}

int DistributedDBCloudKvStoreTest::CheckWaterMark(const std::string &user)
{
    sqlite3 *db_;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &db_, flag, nullptr);
    if (errCode != E_OK) {
        return NOT_FOUND;
    }
    std::string sql;
    if (user.empty()) {
        sql = "SELECT * FROM meta_data WHERE KEY LIKE 'naturalbase_cloud_meta_sync_data_%'";
    } else {
        sql = "SELECT * FROM meta_data WHERE KEY =?;";
    }
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return NOT_FOUND;
    }
    if (!user.empty()) {
        std::string waterMarkKey = HWM_HEAD + user;
        std::vector<uint8_t> keyVec(waterMarkKey.begin(), waterMarkKey.end());
        errCode = SQLiteUtils::BindBlobToStatement(statement, 1, keyVec, true); // only one arg.
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(statement, true, errCode);
            return NOT_FOUND;
        }
    }
    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        sqlite3_close_v2(db_);
        return NOT_FOUND; // cant find.
    }
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        sqlite3_close_v2(db_);
        return OK;
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close_v2(db_);
    return NOT_FOUND;
}

void DistributedDBCloudKvStoreTest::SetDeviceId(const Key &key, const std::string &deviceId)
{
    sqlite3 *db_;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    ASSERT_TRUE(sqlite3_open_v2(fileUrl.c_str(), &db_, flag, nullptr) == SQLITE_OK);
    int errCode = E_OK;
    std::string sql = "UPDATE sync_data SET device=? WHERE Key=?";
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
    }
    ASSERT_EQ(errCode, E_OK);
    std::string hashDevice = DBCommon::TransferHashString(deviceId);
    std::vector<uint8_t> deviceIdVec(hashDevice.begin(), hashDevice.end());
    int bindIndex = 1;
    errCode = SQLiteUtils::BindBlobToStatement(statement, bindIndex, deviceIdVec, true); // only one arg.
    ASSERT_EQ(errCode, E_OK);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
    }
    bindIndex++;
    errCode = SQLiteUtils::BindBlobToStatement(statement, bindIndex, key, true); // only one arg.
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
    }
    EXPECT_EQ(SQLiteUtils::StepWithRetry(statement), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(statement, true, errCode);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close_v2(db_);
}

int DistributedDBCloudKvStoreTest::CheckLogTable(const std::string &deviceId)
{
    sqlite3 *db_;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &db_, flag, nullptr);
    if (errCode != E_OK) {
        return NOT_FOUND;
    }
    std::string sql = "SELECT * FROM naturalbase_kv_aux_sync_data_log WHERE hash_key IN" \
        "(SELECT hash_key FROM sync_data WHERE device =?);";
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return NOT_FOUND;
    }
    std::string hashDevice = DBCommon::TransferHashString(deviceId);
    std::vector<uint8_t> deviceIdVec(hashDevice.begin(), hashDevice.end());
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, deviceIdVec, true); // only one arg.
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return NOT_FOUND;
    }
    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        sqlite3_close_v2(db_);
        return NOT_FOUND; // cant find.
    }
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        sqlite3_close_v2(db_);
        return OK;
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close_v2(db_);
    return NOT_FOUND;
}

int DistributedDBCloudKvStoreTest::ChangeUserId(const std::string &deviceId, const std::string &wantUserId)
{
    sqlite3 *db_;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &db_, flag, nullptr);
    if (errCode != E_OK) {
        return INVALID_ARGS;
    }
    std::string sql = "UPDATE naturalbase_kv_aux_sync_data_log SET userid =? WHERE hash_key IN" \
        "(SELECT hash_key FROM sync_data WHERE device =? AND (flag=0x100));";
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return INVALID_ARGS;
    }
    int bindIndex = 1;
    errCode = SQLiteUtils::BindTextToStatement(statement, bindIndex, wantUserId); // only one arg.
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return INVALID_ARGS;
    }
    bindIndex++;
    std::string hashDevice = DBCommon::TransferHashString(deviceId);
    std::vector<uint8_t> deviceIdVec(hashDevice.begin(), hashDevice.end());
    errCode = SQLiteUtils::BindBlobToStatement(statement, bindIndex, deviceIdVec, true); // only one arg.
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return INVALID_ARGS;
    }
    EXPECT_EQ(SQLiteUtils::StepWithRetry(statement), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(statement, true, errCode);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close_v2(db_);
    return INVALID_ARGS;
}

int DistributedDBCloudKvStoreTest::ChangeHashKey(const std::string &deviceId)
{
    sqlite3 *db_;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &db_, flag, nullptr);
    if (errCode != E_OK) {
        return INVALID_ARGS;
    }
    std::string updataLogTableSql = "UPDATE naturalbase_kv_aux_sync_data_log SET hash_Key ='99';";
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, updataLogTableSql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return INVALID_ARGS;
    }
    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
    }

    std::string sql = "UPDATE sync_data SET hash_Key ='99' WHERE device =? AND (flag=0x100);";
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return INVALID_ARGS;
    }
    std::string hashDevice = DBCommon::TransferHashString(deviceId);
    std::vector<uint8_t> deviceIdVec(hashDevice.begin(), hashDevice.end());
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, deviceIdVec, true); // only one arg.
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        sqlite3_close_v2(db_);
        return OK; // cant find.
    }
    EXPECT_EQ(SQLiteUtils::StepWithRetry(statement), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(statement, true, errCode);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close_v2(db_);
    return INVALID_ARGS;
}

void DistributedDBCloudKvStoreTest::InsertRecord(int num)
{
    for (int i = 0; i < num; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value value;
        value.push_back('k');
        value.push_back('0' + i);
        ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
        BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    }
}

/**
 * @tc.name: RemoveDeviceTest001
 * @tc.desc: remove all log table record with empty deviceId and FLAG_ONLY flag
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: mazhao
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record (Key:k0, device:0, userId:user0), (Key:k1, device:1, userId:user0),
     * (Key:k2, device:2, userId:0)
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    InsertRecord(recordNum);
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE);
        SetDeviceId(key, std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    }
    /**
     * @tc.steps: step2. Check three Log record whether exist or not;
     * * @tc.expected: step2. record exist
    */
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), OK);
        EXPECT_EQ(CheckFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE), OK);
        EXPECT_EQ(CheckWaterMark(""), OK);
    }
    /**
     * @tc.steps: step3. remove log data with empty deviceId.
     * * @tc.expected: step3. remove OK, there are not user record exist in log table.
    */
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData("", ClearMode::FLAG_ONLY), OK);
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), NOT_FOUND);
        EXPECT_EQ(CheckFlag(key, LogInfoFlag::FLAG_LOCAL), OK);
        EXPECT_EQ(CheckWaterMark(""), NOT_FOUND);
    }
}

/**
 * @tc.name: RemoveDeviceTest002
 * @tc.desc: remove all record with empty deviceId and FLAG_AND_DATA flag
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: mazhao
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record (Key:k0, device:0, userId:user0), (Key:k1, device:1, userId:user0),
     * (Key:k2, device:2, userId:0)
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    InsertRecord(recordNum);
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE);
        SetDeviceId(key, std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    }
    /**
     * @tc.steps: step2. Check three Log record whether exist or not;
     * * @tc.expected: step2. record exist
    */
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), OK);
        EXPECT_EQ(CheckWaterMark(""), OK);
    }
    /**
     * @tc.steps: step3. remove log data with empty deviceId.
     * * @tc.expected: step3. remove OK, there are not user record exist in log table.
    */
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData("", ClearMode::FLAG_AND_DATA), OK);
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), NOT_FOUND);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), NOT_FOUND);
        EXPECT_EQ(CheckWaterMark(""), NOT_FOUND);
    }
}

/**
 * @tc.name: RemoveDeviceTest003
 * @tc.desc: remove record with deviceId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: mazhao
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record (Key:k0, device:0, userId:user0), (Key:k1, device:1, userId:user0),
     * (Key:k2, device:2, userId:0)
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    InsertRecord(recordNum);
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE);
        SetDeviceId(key, std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    }
    /**
     * @tc.steps: step2. Check three Log record whether exist or not;
     * * @tc.expected: step2. record exist
    */
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), OK);
        EXPECT_EQ(CheckFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE), OK); // flag become 0x2;
        EXPECT_EQ(CheckWaterMark(""), OK);
    }
    /**
     * @tc.steps: step3. remove "2" deviceId log data with FLAG_AND_DATA, remove "1" with FLAG_ONLY.
     * * @tc.expected: step3. remove OK
    */
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData("1", ClearMode::FLAG_ONLY), OK);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData("2", ClearMode::FLAG_AND_DATA), OK);
    Key key1({'k', '1'});
    std::string deviceId1 = "1";
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue), OK);
    EXPECT_EQ(CheckLogTable(deviceId1), NOT_FOUND);
    EXPECT_EQ(CheckFlag(key1, LogInfoFlag::FLAG_LOCAL), OK); // flag become 0x2;
    Key key2({'k', '2'});
    std::string deviceId2 = "2";
    EXPECT_EQ(kvDelegatePtrS1_->Get(key2, actualValue), NOT_FOUND);
    EXPECT_EQ(CheckLogTable(deviceId2), NOT_FOUND);
    EXPECT_EQ(CheckWaterMark(""), NOT_FOUND);
}

/**
 * @tc.name: RemoveDeviceTest004
 * @tc.desc: remove all record with userId and empty deviceId.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: mazhao
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record (Key:k0, device:0, userId:user0), (Key:k1, device:1, userId:user1),
     * (Key:k2, device:2, userId:2)
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    std::string userHead = "user";
    InsertRecord(recordNum);
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE);
        SetDeviceId(key, std::to_string(i));
        ChangeUserId(std::to_string(i), userHead + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
        EXPECT_EQ(CheckFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE), OK);
    }
    EXPECT_EQ(CheckWaterMark(userHead + "0"), OK);
    /**
     * @tc.steps: step2. Check three Log record whether exist or not;
     * * @tc.expected: step2. record exist
    */
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), OK);
    }
    /**
     * @tc.steps: step3. remove "user1" userid log data with FLAG_AND_DATA, remove "user2" userid with FLAG_ONLY.
     * * @tc.expected: step3. remove OK
    */
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData("", "user0", ClearMode::FLAG_ONLY), OK);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData("", "user2", ClearMode::FLAG_AND_DATA), OK);
    Key key0({'k', '0'});
    std::string deviceId1 = "0";
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS1_->Get(key0, actualValue), OK);
    EXPECT_EQ(CheckLogTable(deviceId1), NOT_FOUND);
    EXPECT_EQ(CheckFlag(key0, LogInfoFlag::FLAG_LOCAL), OK); // flag become 0x2;
    EXPECT_EQ(CheckWaterMark(userHead + "0"), NOT_FOUND);
    Key key2({'k', '2'});
    std::string deviceId2 = "2";
    EXPECT_EQ(kvDelegatePtrS1_->Get(key2, actualValue), NOT_FOUND);
    EXPECT_EQ(CheckLogTable(deviceId2), NOT_FOUND);
}

/**
 * @tc.name: RemoveDeviceTest005
 * @tc.desc: remove record with userId and deviceId.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: mazhao
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record (Key:k0, device:0, userId:user0), (Key:k1, device:1, userId:user1),
     * (Key:k2, device:2, userId:2)
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    InsertRecord(recordNum);
    std::string userHead = "user";
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE);
        SetDeviceId(key, std::to_string(i));
        ChangeUserId(std::to_string(i), userHead + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    }
    EXPECT_EQ(CheckWaterMark(userHead + "0"), OK);
    /**
     * @tc.steps: step2. Check three Log record whether exist or not;
     * * @tc.expected: step2. record exist
    */
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), OK);
        EXPECT_EQ(CheckFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE), OK);
    }
    /**
     * @tc.steps: step3. remove "user1" userid log data with FLAG_AND_DATA, remove "user0" userid with FLAG_ONLY.
     * remove "user2" userid log data with dismatch deviceId, it cant not remove the data.
     * * @tc.expected: step3. remove OK
    */
    std::string deviceId0 = "0";
    std::string deviceId1 = "1";
    std::string deviceId2 = "2";
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId0, "user0", ClearMode::FLAG_ONLY), OK);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId1, "user1", ClearMode::FLAG_AND_DATA), OK);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId0, "user2", ClearMode::FLAG_AND_DATA), OK);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId0, "user2", ClearMode::FLAG_ONLY), OK);
    Key key0({'k', '0'});
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS1_->Get(key0, actualValue), OK);
    EXPECT_EQ(CheckLogTable(deviceId0), NOT_FOUND);
    EXPECT_EQ(CheckFlag(key0, LogInfoFlag::FLAG_LOCAL), OK); // flag become 0x2;
    EXPECT_EQ(CheckWaterMark(userHead + "0"), NOT_FOUND);
    Key key1({'k', '1'});
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue), NOT_FOUND);
    EXPECT_EQ(CheckLogTable(deviceId1), NOT_FOUND);
    Key key2({'k', '2'});;
    EXPECT_EQ(kvDelegatePtrS1_->Get(key2, actualValue), OK);
    EXPECT_EQ(CheckLogTable(deviceId2), OK);
}

/**
 * @tc.name: RemoveDeviceTest006
 * @tc.desc: remove record with userId and deviceId, and there are same hashKey record in log table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: mazhao
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record (Key:k0, device:0, userId:user0), (Key:k1, device:1, userId:user1),
     * (Key:k2, device:2, userId:2)
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    InsertRecord(recordNum);
    std::string userHead = "user";
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD_WRITE);
        SetDeviceId(key, std::to_string(i));
        ChangeUserId(std::to_string(i), userHead + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    }
    /**
     * @tc.steps: step2. Check three Log record whether exist or not;
     * * @tc.expected: step2. record exist
    */
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), OK);
    }
    /**
     * @tc.steps: step3. Make log table all users's hashKey become same hashKey '99', and the hashKey in syncTable
     *  where device is deviceId1 also become '99',remove data with FLAG_AND_DATA flag.
     * * @tc.expected: step3. remove OK
    */
    std::string deviceId1 = "1";
    std::string deviceId2 = "2";
    std::string deviceId0 = "0";
    DistributedDBCloudKvStoreTest::ChangeHashKey(deviceId1);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId1, "user1", ClearMode::FLAG_AND_DATA), OK);
    Key key1({'k', '1'});
    Value actualValue;
    // there are other users with same hash_key connect with this data in sync_data table, cant not remove the data.
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue), OK);
    EXPECT_EQ(CheckLogTable(deviceId1), OK); // match user2 and user0;
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId1, "user2", ClearMode::FLAG_AND_DATA), OK);
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue), OK);
    EXPECT_EQ(CheckLogTable(deviceId1), OK); // only user0 match the hash_key that same as device1.
    EXPECT_EQ(CheckFlag(key1, LogInfoFlag::FLAG_CLOUD_WRITE), OK); // flag still 0x100;
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId1, "user0", ClearMode::FLAG_AND_DATA), OK);
    // all log have been deleted, so data would also be deleted.
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue), NOT_FOUND);
    EXPECT_EQ(CheckLogTable(deviceId1), NOT_FOUND);
    EXPECT_EQ(CheckWaterMark(userHead + "0"), NOT_FOUND);
}

/**
 * @tc.name: RemoveDeviceTest007
 * @tc.desc: remove record with invalid deviceId and mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: mazhao
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Test removeDeviceData with invalid length deviceId.
     * * @tc.expected:
    */
    std::string deviceId = std::string(128, 'a');
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId, ClearMode::FLAG_AND_DATA), OK);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId, "user1", ClearMode::FLAG_AND_DATA), OK);

    std::string invaliDeviceId = std::string(129, 'a');
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(invaliDeviceId, ClearMode::FLAG_AND_DATA), INVALID_ARGS);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(invaliDeviceId, "user1", ClearMode::FLAG_AND_DATA), INVALID_ARGS);

    /**
     * @tc.steps: step2. Test removeDeviceData with invalid mode.
     * * @tc.expected:
    */
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId, ClearMode::CLEAR_SHARED_TABLE), NOT_SUPPORT);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId, "user1", ClearMode::CLEAR_SHARED_TABLE), NOT_SUPPORT);
}

/**
 * @tc.name: RemoveDeviceTest008
 * @tc.desc: remove record without mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record (Key:k0, device:0, userId:user0), (Key:k1, device:1, userId:user1),
     * (Key:k2, device:2, userId:2)
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    InsertRecord(recordNum);
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD);
        SetDeviceId(key, std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    }

    /**
     * @tc.steps: step2. Check three Log record whether exist or not;
     * * @tc.expected: step2. record exist
    */
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
    }
    /**
     * @tc.steps: step3. Remove data without mode.
     * * @tc.expected: step3. remove OK, there are not user record exist in log table.
    */
    for (int i = 0; i < recordNum; i++) {
        EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(std::to_string(i)), OK);
    }
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), NOT_FOUND);
    }
}

/**
 * @tc.name: RemoveDeviceTest009
 * @tc.desc: remove record without mode FLAG_AND_DATA.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest009, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record (Key:k0, device:0, userId:user0), (Key:k1, device:1, userId:user1),
     * (Key:k2, device:2, userId:2)
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    InsertRecord(recordNum);
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        SetFlag(key, LogInfoFlag::FLAG_CLOUD);
        SetDeviceId(key, std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    }

    /**
     * @tc.steps: step2. Check three Log record whether exist or not;
     * * @tc.expected: step2. record exist
    */
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), OK);
    }
    /**
     * @tc.steps: step3. Remove data without mode FLAG_AND_DATA.
     * * @tc.expected: step3. remove OK, there are not user record exist in log table.
    */
    for (int i = 0; i < recordNum; i++) {
        EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(std::to_string(i), ClearMode::FLAG_AND_DATA), OK);
    }
    for (int i = 0; i < recordNum; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value actualValue;
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        std::string deviceId = std::to_string(i);
        EXPECT_EQ(CheckLogTable(deviceId), NOT_FOUND);
    }
}

/**
 * @tc.name: RemoveDeviceTest010
 * @tc.desc: remove record with invalid mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest010, TestSize.Level0)
{
    std::string deviceId = std::string(128, 'a');
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId, "", ClearMode::FLAG_ONLY), INVALID_ARGS);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId, "", ClearMode::CLEAR_SHARED_TABLE), INVALID_ARGS);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId, "", ClearMode::FLAG_AND_DATA), INVALID_ARGS);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId, "", ClearMode::DEFAULT), OK);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData("", "", ClearMode::DEFAULT), OK);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData("", ClearMode::DEFAULT), OK);
}

/**
 * @tc.name: RemoveDeviceTest011
 * @tc.desc: remove record while conn is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest011, TestSize.Level0)
{
    const KvStoreNbDelegate::Option option = {true, true};
    KvStoreNbDelegate *kvDelegateInvalidPtrS1_ = nullptr;
    ASSERT_EQ(GetKvStore(kvDelegateInvalidPtrS1_, "RemoveDeviceTest011", option), OK);
    ASSERT_NE(kvDelegateInvalidPtrS1_, nullptr);
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(kvDelegateInvalidPtrS1_);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    EXPECT_EQ(kvDelegateInvalidPtrS1_->RemoveDeviceData("", ClearMode::FLAG_ONLY), DB_ERROR);
    EXPECT_EQ(kvDelegateInvalidPtrS1_->RemoveDeviceData("", "", ClearMode::FLAG_ONLY), DB_ERROR);
    EXPECT_EQ(g_mgr.CloseKvStore(kvDelegateInvalidPtrS1_), OK);
}

/**
 * @tc.name: RemoveDeviceTest012
 * @tc.desc: Test remove all data from other device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest012, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert three record, k0 from device sync, k1 from local write, k2 from cloud sync.
     * * @tc.expected: step1. insert successfully
    */
    int recordNum = 3;
    InsertRecord(recordNum);
    SetFlag({'k', '0'}, LogInfoFlag::FLAG_CLOUD);
    SetFlag({'k', '1'}, LogInfoFlag::FLAG_LOCAL);
    /**
     * @tc.steps: step2. Remove data from device sync and cloud sync, and remove log.
     * * @tc.expected: step2. All data and log are removed except data from local write.
    */
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(), OK);
    Value actualValue;
    Value expectValue = {'k', '1'};
    EXPECT_EQ(kvDelegatePtrS1_->Get({'k', '1'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    EXPECT_EQ(kvDelegatePtrS1_->Get({'k', '0'}, actualValue), NOT_FOUND);
    EXPECT_EQ(kvDelegatePtrS1_->Get({'k', '2'}, actualValue), NOT_FOUND);
}

/**
 * @tc.name: RemoveDeviceTest013
 * @tc.desc: remove log record with FLAG_ONLY and empty deviceId.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wangxiangdong
 */
HWTEST_F(DistributedDBCloudKvStoreTest, RemoveDeviceTest013, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Insert data
     * * @tc.expected: step1. insert successfully
    */
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    deviceB_->PutData(k1, v1, 1u, 0); // 1 is current timestamp
    /**
     * @tc.steps: step2. sync between devices
     * * @tc.expected: step2. insert successfully
    */
    deviceB_->Sync(SyncMode::SYNC_MODE_PUSH_ONLY, true);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(k1, actualValue), OK);
    EXPECT_EQ(actualValue, v1);
    ASSERT_EQ(kvDelegatePtrS1_->Put(k1, v1), OK);
    /**
     * @tc.steps: step3. sync with cloud
     * * @tc.expected: step3. OK
    */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue2;
    EXPECT_EQ(kvDelegatePtrS2_->Get(k1, actualValue2), OK);
    EXPECT_EQ(actualValue2, v1);
    EXPECT_EQ(kvDelegatePtrS2_->RemoveDeviceData("", ClearMode::FLAG_AND_DATA), OK);
    Value actualValue3;
    EXPECT_EQ(kvDelegatePtrS2_->Get(k1, actualValue3), NOT_FOUND);
    /**
     * @tc.steps: step4. sync between devices and check result
     * * @tc.expected: step4. OK
    */
    deviceB_->Sync(SyncMode::SYNC_MODE_PUSH_ONLY, true);
    Value actualValue4;
    EXPECT_EQ(kvDelegatePtrS2_->Get(k1, actualValue4), OK);
    EXPECT_EQ(actualValue4, v1);
}

/**
 * @tc.name: NormalSyncInvalid001
 * @tc.desc: Test normal push not sync and get cloud version.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, NormalSyncInvalid001, TestSize.Level0)
{
    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue), OK);
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    auto result = kvDelegatePtrS1_->GetCloudVersion("");
    EXPECT_EQ(result.first, NOT_FOUND);
}

/**
 * @tc.name: NormalSyncInvalid002
 * @tc.desc: Test normal push sync and use invalidDevice to get cloud version.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, NormalSyncInvalid002, TestSize.Level0)
{
    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue), OK);
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
        EXPECT_EQ(table.second.upLoadInfo.insertCount, 1u);
    }
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.downLoadInfo.total, 2u); // download 2 records
        EXPECT_EQ(table.second.downLoadInfo.insertCount, 2u); // download 2 records
    }
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    std::string invalidDevice = std::string(DBConstant::MAX_DEV_LENGTH + 1, '0');
    auto result = kvDelegatePtrS2_->GetCloudVersion(invalidDevice);
    EXPECT_EQ(result.first, INVALID_ARGS);
}

/**
 * @tc.name: NormalSyncInvalid003
 * @tc.desc: Test normal push sync for add data while conn is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, NormalSyncInvalid003, TestSize.Level0)
{
    const KvStoreNbDelegate::Option option = {true, true};
    KvStoreNbDelegate *kvDelegateInvalidPtrS1_ = nullptr;
    ASSERT_EQ(GetKvStore(kvDelegateInvalidPtrS1_, "NormalSyncInvalid003", option), OK);
    ASSERT_NE(kvDelegateInvalidPtrS1_, nullptr);
    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(kvDelegateInvalidPtrS1_->Put(key, expectValue), OK);
    kvDelegateInvalidPtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    BlockSync(kvDelegateInvalidPtrS1_, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
        EXPECT_EQ(table.second.upLoadInfo.insertCount, 1u);
    }
    Value actualValue;
    EXPECT_EQ(kvDelegateInvalidPtrS1_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(kvDelegateInvalidPtrS1_);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    kvDelegateInvalidPtrS1_->SetGenCloudVersionCallback(nullptr);
    auto result = kvDelegateInvalidPtrS1_->GetCloudVersion("");
    EXPECT_EQ(result.first, DB_ERROR);
    EXPECT_EQ(g_mgr.CloseKvStore(kvDelegateInvalidPtrS1_), OK);
}

/**
 * @tc.name: NormalSyncInvalid004
 * @tc.desc: Test normal push sync use GetDeviceEntries while conn is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, NormalSyncInvalid004, TestSize.Level0)
{
    const KvStoreNbDelegate::Option option = {true, true};
    KvStoreNbDelegate *kvDelegateInvalidPtrS2_ = nullptr;
    ASSERT_EQ(GetKvStore(kvDelegateInvalidPtrS2_, "NormalSyncInvalid004", option), OK);
    ASSERT_NE(kvDelegateInvalidPtrS2_, nullptr);
    /**
     * @tc.steps: step1. store1 put (k1,v1) store2 put (k2,v2)
     * @tc.expected: step1. both put ok
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    kvDelegateInvalidPtrS2_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    Key key2 = {'k', '2'};
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, expectValue1), OK);
    ASSERT_EQ(kvDelegateInvalidPtrS2_->Put(key2, expectValue2), OK);
    /**
     * @tc.steps: step2. both store1 and store2 sync while conn is nullptr
     * @tc.expected: step2. both sync ok, and store2 got (k1,v1) store1 not exist (k2,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    LOGW("Store1 sync end");
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegateInvalidPtrS2_, OK, g_CloudSyncoption);
    LOGW("Store2 sync end");
    Value actualValue;
    EXPECT_EQ(kvDelegateInvalidPtrS2_->Get(key1, actualValue), OK);

    /**
     * @tc.steps: step3. use GetDeviceEntries while conn is nullptr
     * @tc.expected: step3. DB_ERROR
     */
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(kvDelegateInvalidPtrS2_);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    std::vector<Entry> entries;
    EXPECT_EQ(kvDelegateInvalidPtrS2_->GetDeviceEntries(std::string("DEVICES_A"), entries), DB_ERROR);
    EXPECT_EQ(entries.size(), 0u); // 1 record
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    EXPECT_EQ(actualValue, expectValue1);
    EXPECT_EQ(kvDelegatePtrS1_->Get(key2, actualValue), NOT_FOUND);

    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    kvDelegateInvalidPtrS2_->SetGenCloudVersionCallback(nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(kvDelegateInvalidPtrS2_), OK);
}

/**
 * @tc.name: NormalSyncInvalid005
 * @tc.desc: Test normal sync with invalid parm.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, NormalSyncInvalid005, TestSize.Level0)
{
    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue), OK);
    auto devices = g_CloudSyncoption.devices;
    EXPECT_EQ(kvDelegatePtrS1_->Sync(devices, SyncMode::SYNC_MODE_CLOUD_MERGE, nullptr), NOT_SUPPORT);
    Query query = Query::Select().Range({}, {});
    EXPECT_EQ(kvDelegatePtrS1_->Sync(devices, SyncMode::SYNC_MODE_CLOUD_MERGE, nullptr, query, true), NOT_SUPPORT);
    auto mode = g_CloudSyncoption.mode;
    EXPECT_EQ(kvDelegatePtrS1_->Sync(devices, mode, nullptr, query, true), NOT_SUPPORT);
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(kvDelegatePtrS1_);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption, DB_ERROR);
    EXPECT_EQ(kvDelegatePtrS1_->Sync(devices, mode, nullptr), DB_ERROR);
    EXPECT_EQ(kvDelegatePtrS1_->Sync(devices, mode, nullptr, query, true), DB_ERROR);
}

/**
 * @tc.name: NormalSyncInvalid006
 * @tc.desc: Test normal sync set cloudDB while cloudDB is empty and conn is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, NormalSyncInvalid006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set cloudDB while cloudDB is empty
     * @tc.expected: step1. INVALID_ARGS
     */
    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudDB(cloudDbs), INVALID_ARGS);
    /**
     * @tc.steps: step2. set cloudDB while conn is nullptr
     * @tc.expected: step2. DB_ERROR
     */
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(kvDelegatePtrS1_);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    cloudDbs[USER_ID] = virtualCloudDb_;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudDB(cloudDbs), DB_ERROR);
}

/**
 * @tc.name: NormalSyncInvalid007
 * @tc.desc: Test normal sync set cloudDb schema while conn is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, NormalSyncInvalid007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set cloudDB schema while conn is nullptr
     * @tc.expected: step1. DB_ERROR
     */
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(kvDelegatePtrS1_);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    std::map<std::string, DataBaseSchema> schemas;
    schemas[USER_ID] = GetDataBaseSchema(true);
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudDbSchema(schemas), DB_ERROR);
}

/**
 * @tc.name: NormalSyncInvalid008
 * @tc.desc: Test SetCloudSyncConfig with invalid parm.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvStoreTest, NormalSyncInvalid008, TestSize.Level0)
{
    /**
     * @tc.steps: step1. SetCloudSyncConfig with invalid maxUploadCount.
     * @tc.expected: step1. INVALID_ARGS
     */
    CloudSyncConfig config;
    int maxUploadCount = 0;
    config.maxUploadCount = maxUploadCount;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), INVALID_ARGS);
    maxUploadCount = 2001;
    config.maxUploadCount = maxUploadCount;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), INVALID_ARGS);
    maxUploadCount = 50;
    config.maxUploadCount = maxUploadCount;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), OK);

    /**
     * @tc.steps: step2. SetCloudSyncConfig with invalid maxUploadSize.
     * @tc.expected: step2. INVALID_ARGS
     */
    int maxUploadSize = 1023;
    config.maxUploadSize = maxUploadSize;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), INVALID_ARGS);
    maxUploadSize = 128 * 1024 * 1024 + 1;
    config.maxUploadSize = maxUploadSize;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), INVALID_ARGS);
    maxUploadSize = 10240;
    config.maxUploadSize = maxUploadSize;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), OK);

    /**
     * @tc.steps: step3. SetCloudSyncConfig with invalid maxRetryConflictTimes.
     * @tc.expected: step3. INVALID_ARGS
     */
    int maxRetryConflictTimes = -2;
    config.maxRetryConflictTimes = maxRetryConflictTimes;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), INVALID_ARGS);
    maxRetryConflictTimes = 2;
    config.maxRetryConflictTimes = maxRetryConflictTimes;
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), OK);

    /**
     * @tc.steps: step4. SetCloudSyncConfig while conn is nullptr
     * @tc.expected: step4. DB_ERROR
     */
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(kvDelegatePtrS1_);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    EXPECT_EQ(kvDelegatePtrS1_->SetCloudSyncConfig(config), DB_ERROR);
}

/**
 * @tc.name: ConflictSync001
 * @tc.desc: test upload delete with version conflict error under merge mode, then success after retry.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBCloudKvStoreTest, ConflictSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set the retry count to 2
     * @tc.expected: step1. ok.
     */
    CloudSyncConfig config;
    config.maxRetryConflictTimes = 2;
    kvDelegatePtrS1_->SetCloudSyncConfig(config);
    /**
     * @tc.steps: step2. Put 2 records and set flag to cloud
     * @tc.expected: step2. ok.
     */
    InsertRecord(2);
    SetFlag({'k', '0'}, LogInfoFlag::FLAG_CLOUD);
    SetFlag({'k', '1'}, LogInfoFlag::FLAG_CLOUD);
    /**
     * @tc.steps: step3. delete {k1, v1}
     * @tc.expected: step3. ok.
     */
    kvDelegatePtrS1_->Delete(KEY_1);
    /**
     * @tc.steps: step4. Set CLOUD_VERSION_CONFLICT when upload, and do sync
     * @tc.expected: step4. OK.
     */
    int recordIndex = 0;
    virtualCloudDb_->ForkInsertConflict([&recordIndex](const std::string &tableName, VBucket &extend, VBucket &record,
        vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        recordIndex++;
        if (recordIndex == 1) { // set 1st record return CLOUD_VERSION_CONFLICT
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_VERSION_CONFLICT);
            return CLOUD_VERSION_CONFLICT;
        }
        return OK;
    });
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps: step5. Check last process
     * @tc.expected: step5. ok.
     */
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
        EXPECT_EQ(table.second.upLoadInfo.successCount, 1u);
        EXPECT_EQ(table.second.upLoadInfo.failCount, 0u);
        EXPECT_EQ(table.second.upLoadInfo.deleteCount, 1u);
    }
    virtualCloudDb_->ForkInsertConflict(nullptr);
}
}
