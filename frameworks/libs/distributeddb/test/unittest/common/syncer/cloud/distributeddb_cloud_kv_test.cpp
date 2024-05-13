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

#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_virtual_device.h"
#include "kv_store_nb_delegate.h"
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
class DistributedDBCloudKvTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    void InsertRecord(int num);
    void SetDeviceId(const Key &key, const std::string &deviceId);
    void SetFlag(const Key &key, bool isCloudFlag);
    int CheckFlag(const Key &key, bool isCloudFlag);
    int CheckLogTable(const std::string &deviceId);
    int CheckWaterMark(const std::string &key);
    int ChangeUserId(const std::string &deviceId, const std::string &wantUserId);
    int ChangeHashKey(const std::string &deviceId);
protected:
    void GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId, int securityLabel = NOT_SET);
    void CloseKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId);
    void BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus,
        SyncMode mode = SyncMode::SYNC_MODE_CLOUD_MERGE, int expectSyncResult = OK);
    static DataBaseSchema GetDataBaseSchema();
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    KvStoreConfig config_;
    KvStoreNbDelegate* kvDelegatePtrS1_ = nullptr;
    KvStoreNbDelegate* kvDelegatePtrS2_ = nullptr;
    SyncProcess lastProcess_;
    VirtualCommunicatorAggregator *communicatorAggregator_ = nullptr;
    KvVirtualDevice *deviceB_ = nullptr;
};

void DistributedDBCloudKvTest::SetUpTestCase()
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }

    string dir = g_testDir + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }
}

void DistributedDBCloudKvTest::TearDownTestCase()
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBCloudKvTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    config_.dataDir = g_testDir;
    /**
     * @tc.setup: create virtual device B and C, and get a KvStoreNbDelegate as deviceA
     */
    virtualCloudDb_ = std::make_shared<VirtualCloudDb>();
    g_mgr.SetKvStoreConfig(config_);
    GetKvStore(kvDelegatePtrS1_, STORE_ID_1);
    // set aggregator after get store1, only store2 can sync with p2p
    communicatorAggregator_ = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(communicatorAggregator_ != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(communicatorAggregator_);
    GetKvStore(kvDelegatePtrS2_, STORE_ID_2);

    deviceB_ = new (std::nothrow) KvVirtualDevice("DEVICE_B");
    ASSERT_TRUE(deviceB_ != nullptr);
    auto syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(deviceB_->Initialize(communicatorAggregator_, syncInterfaceB), E_OK);
}

void DistributedDBCloudKvTest::TearDown()
{
    CloseKvStore(kvDelegatePtrS1_, STORE_ID_1);
    CloseKvStore(kvDelegatePtrS2_, STORE_ID_2);
    virtualCloudDb_ = nullptr;
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }

    if (deviceB_ != nullptr) {
        delete deviceB_;
        deviceB_ = nullptr;
    }

    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    communicatorAggregator_ = nullptr;
}

void DistributedDBCloudKvTest::BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus,
    SyncMode mode, int expectSyncResult)
{
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    SyncProcess last;
    auto callback =
        [expectDBStatus, &last, &cv, &dataMutex, &finish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                EXPECT_EQ(item.second.errCode, expectDBStatus);
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    finish = true;
                    last = item.second;
                }
                cv.notify_one();
            }
        }
    };
    CloudSyncOption option;
    option.mode = mode;
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    EXPECT_EQ(delegate->Sync(option, callback), expectSyncResult);
    if (expectSyncResult == OK) {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        cv.wait(uniqueLock, [&finish]() {
            return finish;
        });
    }
    lastProcess_ = last;
}

DataBaseSchema DistributedDBCloudKvTest::GetDataBaseSchema()
{
    DataBaseSchema schema;
    TableSchema tableSchema;
    tableSchema.name = "sync_data";
    Field field;
    field.colName = "key";
    field.type = TYPE_INDEX<std::string>;
    field.primary = true;
    tableSchema.fields.push_back(field);
    field.colName = "device";
    field.primary = false;
    tableSchema.fields.push_back(field);
    field.colName = "oridevice";
    tableSchema.fields.push_back(field);
    field.colName = "value";
    tableSchema.fields.push_back(field);
    field.colName = "device_create_time";
    field.type = TYPE_INDEX<int64_t>;
    tableSchema.fields.push_back(field);
    schema.tables.push_back(tableSchema);
    return schema;
}

void DistributedDBCloudKvTest::GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId, int securityLabel)
{
    KvStoreNbDelegate::Option option;
    DBStatus openRet = OK;
    option.secOption.securityLabel = securityLabel;
    g_mgr.GetKvStore(storeId, option, [&openRet, &delegate](DBStatus status, KvStoreNbDelegate *openDelegate) {
        openRet = status;
        delegate = openDelegate;
    });
    EXPECT_EQ(openRet, OK);
    EXPECT_NE(delegate, nullptr);

    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
    cloudDbs[USER_ID] = virtualCloudDb_;
    delegate->SetCloudDB(cloudDbs);
    std::map<std::string, DataBaseSchema> schemas;
    schemas[USER_ID] = GetDataBaseSchema();
    delegate->SetCloudDbSchema(schemas);
}

void DistributedDBCloudKvTest::CloseKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId)
{
    if (delegate != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(delegate), OK);
        delegate = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(storeId);
        LOGD("delete kv store status %d store %s", status, storeId.c_str());
        ASSERT_EQ(status, OK);
    }
}

/**
 * @tc.name: NormalSync001
 * @tc.desc: Test normal push sync for add data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync001, TestSize.Level0)
{
    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue), OK);
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    BlockSync(kvDelegatePtrS1_, OK);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
    }
    BlockSync(kvDelegatePtrS2_, OK);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    auto result = kvDelegatePtrS2_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "1");
    }
}

/**
 * @tc.name: NormalSync002
 * @tc.desc: Test normal push pull sync for add data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. store1 put (k1,v1) store2 put (k2,v2)
     * @tc.expected: step1. both put ok
     */
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    Key key2 = {'k', '2'};
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, expectValue1), OK);
    ASSERT_EQ(kvDelegatePtrS2_->Put(key2, expectValue2), OK);
    /**
     * @tc.steps: step2. both store1 and store2 sync
     * @tc.expected: step2. both sync ok, and store2 got (k1,v1) store1 not exist (k2,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK);
    LOGW("Store1 sync end");
    BlockSync(kvDelegatePtrS2_, OK);
    LOGW("Store2 sync end");
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue1);
    EXPECT_EQ(kvDelegatePtrS1_->Get(key2, actualValue), NOT_FOUND);
    /**
     * @tc.steps: step3. store1 sync again
     * @tc.expected: step3. sync ok store1 got (k2,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK);
    LOGW("Store1 sync end");
    EXPECT_EQ(kvDelegatePtrS1_->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue2);
}

/**
 * @tc.name: NormalSync003
 * @tc.desc: Test normal pull sync for update data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. store1 put (k1,v1) store2 put (k1,v2)
     * @tc.expected: step1. both put ok
     */
    Key key = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue1), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS2_->Put(key, expectValue2), OK);
    /**
     * @tc.steps: step2. both store1 and store2 sync
     * @tc.expected: step2. both sync ok and store2 got (k1,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK);
    BlockSync(kvDelegatePtrS2_, OK);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue2);
    /**
     * @tc.steps: step2. store1 sync again
     * @tc.expected: step2. sync ok and store1 got (k1,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK);
    EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue2);
}

/**
 * @tc.name: NormalSync004
 * @tc.desc: Test normal push sync for delete data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. store1 put (k1,v1) and both sync
     * @tc.expected: step1. put ok and both sync ok
     */
    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue), OK);
    BlockSync(kvDelegatePtrS1_, OK);
    BlockSync(kvDelegatePtrS2_, OK);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. store1 delete (k1,v1) and both sync
     * @tc.expected: step2. both put ok
     */
    ASSERT_EQ(kvDelegatePtrS1_->Delete(key), OK);
    BlockSync(kvDelegatePtrS1_, OK);
    BlockSync(kvDelegatePtrS2_, OK);
    actualValue.clear();
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), NOT_FOUND);
    EXPECT_NE(actualValue, expectValue);
}

/**
 * @tc.name: NormalSync005
 * @tc.desc: Test normal push sync for add data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync005, TestSize.Level1)
{
    for (int i = 0; i < 60; ++i) { // sync 60 records
        Key key = {'k'};
        Value expectValue = {'v'};
        key.push_back(static_cast<uint8_t>(i));
        expectValue.push_back(static_cast<uint8_t>(i));
        ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue), OK);
    }
    BlockSync(kvDelegatePtrS1_, OK);
    BlockSync(kvDelegatePtrS2_, OK);
}

/**
 * @tc.name: NormalSync006
 * @tc.desc: Test normal push sync with insert delete update.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync006, TestSize.Level0)
{
    Key k1 = {'k', '1'};
    Key k2 = {'k', '2'};
    Value v1 = {'v', '1'};
    Value v2 = {'v', '2'};
    Value v3 = {'v', '3'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(k1, v1), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Put(k2, v2), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Put(k2, v3), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Delete(k1), OK);
    BlockSync(kvDelegatePtrS1_, OK);
    BlockSync(kvDelegatePtrS2_, OK);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(k1, actualValue), NOT_FOUND);
    EXPECT_EQ(kvDelegatePtrS2_->Get(k2, actualValue), OK);
    EXPECT_EQ(actualValue, v3);
}

/**
 * @tc.name: NormalSync007
 * @tc.desc: Test normal push sync with download and upload.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync007, TestSize.Level0)
{
    Key k1 = {'k', '1'};
    Key k2 = {'k', '2'};
    Key k3 = {'k', '3'};
    Key k4 = {'k', '4'};
    Value v1 = {'v', '1'};
    Value v2 = {'v', '2'};
    Value v3 = {'v', '3'};
    ASSERT_EQ(kvDelegatePtrS2_->Put(k1, v1), OK);
    ASSERT_EQ(kvDelegatePtrS2_->Put(k2, v1), OK);
    ASSERT_EQ(kvDelegatePtrS2_->Put(k3, v1), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100ms
    ASSERT_EQ(kvDelegatePtrS1_->Put(k1, v2), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Put(k2, v2), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Put(k4, v2), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100ms
    BlockSync(kvDelegatePtrS1_, OK);
    ASSERT_EQ(kvDelegatePtrS2_->Put(k4, v3), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Delete(k2), OK);
    BlockSync(kvDelegatePtrS2_, OK);
}

/**
 * @tc.name: NormalSync008
 * @tc.desc: Test complex sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync008, TestSize.Level0)
{
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    deviceB_->PutData(k1, v1, 1u, 0); // 1 is current timestamp
    deviceB_->Sync(SyncMode::SYNC_MODE_PUSH_ONLY, true);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(k1, actualValue), OK);
    EXPECT_EQ(actualValue, v1);
    BlockSync(kvDelegatePtrS2_, OK, SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH);
    BlockSync(kvDelegatePtrS1_, OK);
    EXPECT_EQ(kvDelegatePtrS1_->Get(k1, actualValue), NOT_FOUND);
}

/**
 * @tc.name: NormalSync009
 * @tc.desc: Test normal push sync with download and upload.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync009, TestSize.Level0)
{
    Key k1 = {'k', '1'};
    Key k2 = {'k', '2'};
    Key k3 = {'k', '3'};
    Value v1 = {'v', '1'};
    Value v2 = {'v', '2'};
    Value v3 = {'v', '3'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(k1, v1), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Put(k2, v1), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Delete(k1), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100ms
    BlockSync(kvDelegatePtrS1_, OK);
    ASSERT_EQ(kvDelegatePtrS2_->Put(k1, v2), OK);
    ASSERT_EQ(kvDelegatePtrS2_->Put(k3, v2), OK);
    BlockSync(kvDelegatePtrS2_, OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100ms
    BlockSync(kvDelegatePtrS1_, OK);
}

/**
 * @tc.name: NormalSync010
 * @tc.desc: Do not synchronize when security label is S4.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync010, TestSize.Level0)
{
    std::shared_ptr<ProcessSystemApiAdapterImpl> g_adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    KvStoreNbDelegate* kvDelegatePtrS3_ = nullptr;

    GetKvStore(kvDelegatePtrS3_, STORE_ID_3, S4);
    BlockSync(kvDelegatePtrS1_, OK);
    BlockSync(kvDelegatePtrS2_, OK);
    BlockSync(kvDelegatePtrS3_, OK, SyncMode::SYNC_MODE_CLOUD_MERGE, SECURITY_OPTION_CHECK_ERROR);
    CloseKvStore(kvDelegatePtrS3_, STORE_ID_3);
}

/**
 * @tc.name: NormalSync015
 * @tc.desc: Test sync in all process.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync015, TestSize.Level0)
{
    Key key = {'k'};
    Value expectValue = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue), OK);
    BlockSync(kvDelegatePtrS1_, OK);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
    }
    BlockSync(kvDelegatePtrS2_, OK);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}

void DistributedDBCloudKvTest::SetFlag(const Key &key, bool isCloudFlag)
{
    sqlite3 *db_;
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    ASSERT_TRUE(sqlite3_open_v2(fileUrl.c_str(), &db_, flag, nullptr) == SQLITE_OK);
    int errCode = E_OK;
    std::string sql;
    if (isCloudFlag) {
        sql = "UPDATE sync_data SET flag=256 WHERE Key=?";
    } else {
        sql = "UPDATE sync_data SET flag=2 WHERE Key=?";
    }
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
    }
    ASSERT_EQ(errCode, E_OK);
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, true); // only one arg.
    ASSERT_EQ(errCode, E_OK);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
    }
    EXPECT_EQ(SQLiteUtils::StepWithRetry(statement), SQLiteUtils::MapSQLiteErrno(SQLITE_DONE));
    SQLiteUtils::ResetStatement(statement, true, errCode);
    EXPECT_EQ(errCode, E_OK);
    sqlite3_close_v2(db_);
}

int DistributedDBCloudKvTest::CheckFlag(const Key &key, bool isCloudFlag)
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
    if (isCloudFlag) {
        sql = "SELECT * FROM sync_data WHERE Key =? AND (flag=0x100)";
    } else {
        sql = "SELECT * FROM sync_data WHERE Key =? AND (flag=0x02)";
    }
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db_, sql, statement);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return NOT_FOUND;
    }
    std::vector<uint8_t> keyVec(key.begin(), key.end());
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, keyVec, true); // only one arg.
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

int DistributedDBCloudKvTest::CheckWaterMark(const std::string &user)
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

void DistributedDBCloudKvTest::SetDeviceId(const Key &key, const std::string &deviceId)
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

int DistributedDBCloudKvTest::CheckLogTable(const std::string &deviceId)
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
        "(SELECT hash_key FROM sync_data WHERE device =? AND (flag=0x100));";
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

int DistributedDBCloudKvTest::ChangeUserId(const std::string &deviceId, const std::string &wantUserId)
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
    std::vector<uint8_t> wantUserIdVec(wantUserId.begin(), wantUserId.end());
    errCode = SQLiteUtils::BindBlobToStatement(statement, bindIndex, wantUserIdVec, true); // only one arg.
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

int DistributedDBCloudKvTest::ChangeHashKey(const std::string &deviceId)
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

void DistributedDBCloudKvTest::InsertRecord(int num)
{
    for (int i = 0; i < num; i++) {
        Key key;
        key.push_back('k');
        key.push_back('0' + i);
        Value value;
        value.push_back('k');
        value.push_back('0' + i);
        ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
        BlockSync(kvDelegatePtrS1_, OK);
        SetFlag(key, true);
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
HWTEST_F(DistributedDBCloudKvTest, RemoveDeviceTest001, TestSize.Level0)
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
        SetFlag(key, true);
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
        EXPECT_EQ(CheckFlag(key, true), OK);
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
        EXPECT_EQ(CheckFlag(key, false), OK);
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
HWTEST_F(DistributedDBCloudKvTest, RemoveDeviceTest002, TestSize.Level0)
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
        SetFlag(key, true);
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
HWTEST_F(DistributedDBCloudKvTest, RemoveDeviceTest003, TestSize.Level0)
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
        SetFlag(key, true);
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
        EXPECT_EQ(CheckFlag(key, true), OK); // flag become 0x2;
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
    EXPECT_EQ(CheckFlag(key1, false), OK); // flag become 0x2;
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
HWTEST_F(DistributedDBCloudKvTest, RemoveDeviceTest004, TestSize.Level0)
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
        SetFlag(key, true);
        SetDeviceId(key, std::to_string(i));
        ChangeUserId(std::to_string(i), userHead + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
        EXPECT_EQ(CheckFlag(key, true), OK);
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
    EXPECT_EQ(CheckFlag(key0, false), OK); // flag become 0x2;
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
HWTEST_F(DistributedDBCloudKvTest, RemoveDeviceTest005, TestSize.Level0)
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
        SetFlag(key, true);
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
        EXPECT_EQ(CheckFlag(key, true), OK);
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
    EXPECT_EQ(CheckFlag(key0, false), OK); // flag become 0x2;
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
HWTEST_F(DistributedDBCloudKvTest, RemoveDeviceTest006, TestSize.Level0)
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
        SetFlag(key, true);
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
    DistributedDBCloudKvTest::ChangeHashKey(deviceId1);
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId1, "user1", ClearMode::FLAG_AND_DATA), OK);
    Key key1({'k', '1'});
    Value actualValue;
    // there are other users with same hash_key connect with this data in sync_data table, cant not remove the data.
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue), OK);
    EXPECT_EQ(CheckLogTable(deviceId1), OK); // match user2 and user0;
    EXPECT_EQ(kvDelegatePtrS1_->RemoveDeviceData(deviceId1, "user2", ClearMode::FLAG_AND_DATA), OK);
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue), OK);
    EXPECT_EQ(CheckLogTable(deviceId1), OK); // only user0 match the hash_key that same as device1.
    EXPECT_EQ(CheckFlag(key1, true), OK); // flag still 0x100;
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
HWTEST_F(DistributedDBCloudKvTest, RemoveDeviceTest007, TestSize.Level0)
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
}