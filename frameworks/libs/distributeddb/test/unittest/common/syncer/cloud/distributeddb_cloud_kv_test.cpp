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
class DistributedDBCloudKvTest : public testing::Test {
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

void DistributedDBCloudKvTest::SetUpTestCase()
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
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

void DistributedDBCloudKvTest::TearDown()
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

void DistributedDBCloudKvTest::BlockSync(KvStoreNbDelegate *delegate, DBStatus expectDBStatus, CloudSyncOption option,
    DBStatus expectSyncResult)
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

DataBaseSchema DistributedDBCloudKvTest::GetDataBaseSchema(bool invalidSchema)
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


DBStatus DistributedDBCloudKvTest::GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId,
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

void DistributedDBCloudKvTest::SyncAndGetProcessInfo(KvStoreNbDelegate *delegate, CloudSyncOption option)
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

bool DistributedDBCloudKvTest::CheckUserSyncInfo(const vector<std::string> users, const vector<DBStatus> userStatus,
    const vector<Info> userExpectInfo)
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
 * @tc.name: SubUser001
 * @tc.desc: Test get and delete db with sub user.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, SubUser001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Get db with subUser
     * @tc.expected: step1.ok
     */
    KvStoreDelegateManager manager(APP_ID, USER_ID, "subUser");
    KvStoreConfig subUserConfig = config_;
    subUserConfig.dataDir = config_.dataDir + "/subUser";
    OS::MakeDBDirectory(subUserConfig.dataDir);
    manager.SetKvStoreConfig(subUserConfig);
    KvStoreNbDelegate::Option option;
    DBStatus openRet = OK;
    KvStoreNbDelegate* kvDelegatePtrS3_ = nullptr;
    manager.GetKvStore(STORE_ID_1, option,
        [&openRet, &kvDelegatePtrS3_](DBStatus status, KvStoreNbDelegate *openDelegate) {
        openRet = status;
        kvDelegatePtrS3_ = openDelegate;
    });
    EXPECT_EQ(openRet, OK);
    ASSERT_NE(kvDelegatePtrS3_, nullptr);
    /**
     * @tc.steps: step2. close and delete db
     * @tc.expected: step2.ok
     */
    ASSERT_EQ(manager.CloseKvStore(kvDelegatePtrS3_), OK);
    DBStatus status = manager.DeleteKvStore(STORE_ID_1);
    EXPECT_EQ(status, OK);
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
    auto result = kvDelegatePtrS2_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (const auto &item : result.second) {
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
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    kvDelegatePtrS2_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
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
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    LOGW("Store1 sync end");
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    LOGW("Store2 sync end");
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue), OK);
    std::vector<Entry> entries;
    EXPECT_EQ(kvDelegatePtrS2_->GetDeviceEntries(std::string("DEVICES_A"), entries), OK);
    EXPECT_EQ(entries.size(), 1u); // 1 record
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    EXPECT_EQ(actualValue, expectValue1);
    EXPECT_EQ(kvDelegatePtrS1_->Get(key2, actualValue), NOT_FOUND);
    /**
     * @tc.steps: step3. store1 sync again
     * @tc.expected: step3. sync ok store1 got (k2,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    LOGW("Store1 sync end");
    EXPECT_EQ(kvDelegatePtrS1_->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue2);
    auto result = kvDelegatePtrS2_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (const auto &item : result.second) {
        EXPECT_EQ(DBBase64Utils::DecodeIfNeed(item.first), item.first);
    }
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    kvDelegatePtrS2_->SetGenCloudVersionCallback(nullptr);
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
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue2);
    /**
     * @tc.steps: step2. store1 sync again
     * @tc.expected: step2. sync ok and store1 got (k1,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
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
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. store1 delete (k1,v1) and both sync
     * @tc.expected: step2. both put ok
     */
    ASSERT_EQ(kvDelegatePtrS1_->Delete(key), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
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
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    for (const auto &process : lastProcess_.tableProcess) {
        EXPECT_EQ(process.second.upLoadInfo.insertCount, 60u); // sync 60 records
    }
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    for (const auto &process : lastProcess_.tableProcess) {
        EXPECT_EQ(process.second.downLoadInfo.insertCount, 60u); // sync 60 records
    }
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
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
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
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    ASSERT_EQ(kvDelegatePtrS2_->Put(k4, v3), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Delete(k2), OK);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
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
    CloudSyncOption option;
    option.mode = SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH;
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS2_, OK, option);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
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
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    ASSERT_EQ(kvDelegatePtrS2_->Put(k1, v2), OK);
    ASSERT_EQ(kvDelegatePtrS2_->Put(k3, v2), OK);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100ms
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
}

/**
 * @tc.name: NormalSync010
 * @tc.desc: Test normal push sync for add data with different user.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync010, TestSize.Level0)
{
    // add <k1, v1>, sync to cloud with user1
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, expectValue1), OK);
    CloudSyncOption option;
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS1_, OK, option);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
    }

    // add <k2, v2>, sync to cloud with user2
    Key key2 = {'k', '2'};
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key2, expectValue2), OK);
    option.users.clear();
    option.users.push_back(USER_ID_2);
    BlockSync(kvDelegatePtrS1_, OK, option);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 2u);
    }

    option.users.clear();
    option.users.push_back(USER_ID);
    option.users.push_back(USER_ID_2);
    BlockSync(kvDelegatePtrS2_, OK, option);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.downLoadInfo.total, 2u);
    }
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue1);
    Value actualValue2;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key2, actualValue2), OK);
    EXPECT_EQ(actualValue2, expectValue2);
}

/**
 * @tc.name: NormalSync011
 * @tc.desc: Do not synchronize when security label is S4.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync011, TestSize.Level0)
{
    std::shared_ptr<ProcessSystemApiAdapterImpl> g_adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    KvStoreNbDelegate* kvDelegatePtrS3_ = nullptr;

    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = S4;
    EXPECT_EQ(GetKvStore(kvDelegatePtrS3_, STORE_ID_3, option), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS3_, OK, g_CloudSyncoption, SECURITY_OPTION_CHECK_ERROR);
    CloseKvStore(kvDelegatePtrS3_, STORE_ID_3);
}

/**
 * @tc.name: NormalSync012
 * @tc.desc: Test normal push sync with memory db.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync012, TestSize.Level0)
{
    KvStoreNbDelegate *memoryDB1 = nullptr;
    KvStoreNbDelegate::Option option1;
    option1.isMemoryDb = true;
    GetKvStore(memoryDB1, STORE_ID_3, option1);
    ASSERT_NE(memoryDB1, nullptr);
    KvStoreNbDelegate *memoryDB2 = nullptr;
    KvStoreNbDelegate::Option option2;
    option2.isMemoryDb = true;
    GetKvStore(memoryDB2, STORE_ID_4, option2);
    EXPECT_NE(memoryDB2, nullptr);
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    EXPECT_EQ(memoryDB1->Put(key1, expectValue1), OK);
    BlockSync(memoryDB1, OK, g_CloudSyncoption);
    BlockSync(memoryDB2, OK, g_CloudSyncoption);
    EXPECT_EQ(g_mgr.CloseKvStore(memoryDB1), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(memoryDB2), OK);
}

/**
 * @tc.name: NormalSync013
 * @tc.desc: Test the wrong schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync013, TestSize.Level0)
{
    KvStoreNbDelegate* kvDelegatePtrS3_ = nullptr;
    KvStoreNbDelegate::Option option;
    EXPECT_EQ(GetKvStore(kvDelegatePtrS3_, STORE_ID_3, option, true), INVALID_SCHEMA);
    CloseKvStore(kvDelegatePtrS3_, STORE_ID_3);
}

/**
 * @tc.name: NormalSync014
 * @tc.desc: Test sync after user change.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync014, TestSize.Level1)
{
    /**
     * @tc.steps: step1. kvDelegatePtrS1_ put and sync data (k1, v1)
     * @tc.expected: step1.ok
     */
    g_mgr.SetSyncActivationCheckCallback([] (const std::string &userId, const std::string &appId,
        const std::string &storeId)-> bool {
        return true;
    });
    KvStoreNbDelegate* kvDelegatePtrS3_ = nullptr;
    KvStoreNbDelegate::Option option;
    option.syncDualTupleMode = true;
    GetKvStore(kvDelegatePtrS3_, STORE_ID_3, option);
    Key key = {'k', '1'};
    Value value = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps: step2. Set sync block time 2s, and change user in sync block time
     * @tc.expected: step2. Sync return  USER_CHANGED.
     */
    virtualCloudDb_->SetBlockTime(2000); // 2000ms
    std::thread thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // sleep for 1000ms
        g_mgr.SetSyncActivationCheckCallback([] (const std::string &userId, const std::string &appId,
            const std::string &storeId)-> bool {
            return false;
        });
        RuntimeContext::GetInstance()->NotifyUserChanged();
    });
    BlockSync(kvDelegatePtrS3_, USER_CHANGED, g_CloudSyncoption);
    thread.join();
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
    auto option = g_CloudSyncoption;
    auto action = static_cast<uint32_t>(LockAction::INSERT) | static_cast<uint32_t>(LockAction::UPDATE)
        | static_cast<uint32_t>(LockAction::DELETE) | static_cast<uint32_t>(LockAction::DOWNLOAD);
    option.lockAction = static_cast<LockAction>(action);
    BlockSync(kvDelegatePtrS1_, OK, option);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
    }
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}

/**
 * @tc.name: NormalSync016
 * @tc.desc: Device A and device B have the same key data,
 *           and then devices B and A perform cloud synchronization sequentially.
 *           Finally, device A updates the data and performs cloud synchronization.
 *           Test if there is new data inserted into the cloud database.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync016, TestSize.Level0)
{
    Key key = {'k', '1'};
    Value value1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value1), OK);
    Value value2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS2_->Put(key, value2), OK);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);

    Value value3 = {'v', '3'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value3), OK);
    virtualCloudDb_->SetInsertHook([](VBucket &record) {
        for (auto &recordData : record) {
            std::string insertKey = "key";
            Type insertValue = "k1";
            EXPECT_FALSE(recordData.first == insertKey && recordData.second == insertValue);
        }
    });
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    virtualCloudDb_->SetInsertHook(nullptr);
}

/**
 * @tc.name: NormalSync017
 * @tc.desc: Test duplicate addition, deletion, and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync017, TestSize.Level0)
{
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    ASSERT_EQ(kvDelegatePtrS1_->Delete(key), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
}

/**
 * @tc.name: NormalSync018
 * @tc.desc: Test putBatch and sync with memory db.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync018, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Get two Memory DB.
     * @tc.expected: step1 OK.
     */
    KvStoreNbDelegate *memoryDB1 = nullptr;
    KvStoreNbDelegate::Option option1;
    option1.isMemoryDb = true;
    GetKvStore(memoryDB1, STORE_ID_3, option1);
    ASSERT_NE(memoryDB1, nullptr);
    KvStoreNbDelegate *memoryDB2 = nullptr;
    KvStoreNbDelegate::Option option2;
    option2.isMemoryDb = true;
    GetKvStore(memoryDB2, STORE_ID_4, option2);
    EXPECT_NE(memoryDB2, nullptr);

    /**
     * @tc.steps:step2. put 301 records and sync to cloud.
     * @tc.expected: step2 OK.
     */
    vector<Entry> entries;
    int count = 301; // put 301 records.
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        entries.push_back({key, value});
    }
    EXPECT_EQ(memoryDB1->PutBatch(entries), OK);
    BlockSync(memoryDB1, OK, g_CloudSyncoption);

    /**
     * @tc.steps:step3. Sync from cloud and check values.
     * @tc.expected: step3 OK.
     */
    BlockSync(memoryDB2, OK, g_CloudSyncoption);
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value expectValue(valueStr.begin(), valueStr.end());
        Value actualValue;
        EXPECT_EQ(memoryDB2->Get(key, actualValue), OK);
        EXPECT_EQ(actualValue, expectValue);
    }
    EXPECT_EQ(g_mgr.CloseKvStore(memoryDB1), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(memoryDB2), OK);
}

/**
 * @tc.name: NormalSync019
 * @tc.desc: Test dataItem has same time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync019, TestSize.Level0)
{
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS2_->Put(k1, v1), OK);
    deviceB_->Sync(SyncMode::SYNC_MODE_PULL_ONLY, true);

    VirtualDataItem dataItem;
    deviceB_->GetData(k1, dataItem);
    EXPECT_EQ(dataItem.timestamp, dataItem.writeTimestamp);
}

/**
 * @tc.name: NormalSync020
 * @tc.desc: Test sync with two users.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync020, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Inserts a piece of data.
     * @tc.expected: step1 OK.
     */
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(k1, v1), OK);
    /**
     * @tc.steps:step2. sync with two users.
     * @tc.expected: step2 OK.
     */
    CloudSyncOption option;
    option.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    option.users.push_back(USER_ID);
    option.users.push_back(USER_ID_2);
    option.devices.push_back("cloud");
    BlockSync(kvDelegatePtrS1_, OK, option);
    /**
     * @tc.steps:step3. Check upLoadInfo.batchIndex of two users.
     * @tc.expected: Both users have a upLoadInfo.batchIndex of 1.
     */
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.batchIndex, 1u);
    }
}

/**
 * @tc.name: NormalSync021
 * @tc.desc: Test Get Func to get cloudVersion.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync021, TestSize.Level0)
{
    /**
     * @tc.steps:step1. store2 GetCloudVersion.
     * @tc.expected: step1 OK.
     */
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
    }
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    auto result = kvDelegatePtrS2_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "1");
    }
    /**
     * @tc.steps:step2. store2 GetCloudVersion.
     * @tc.expected: step2 NOT_FOUND.
     */
    Key keyB;
    Value actualValueB;
    std::string deviceB = DBCommon::TransferStringToHex(DBCommon::TransferHashString("DEVICE_B"));
    std::string versionDeviceBStr = "naturalbase_cloud_version_" + deviceB;
    const char *buffer = versionDeviceBStr.c_str();
    for (uint32_t i = 0; i < versionDeviceBStr.size(); i++) {
        keyB.emplace_back(buffer[i]);
    }
    EXPECT_EQ(kvDelegatePtrS2_->Get(keyB, actualValueB), NOT_FOUND);
}

/**
 * @tc.name: NormalSync022
 * @tc.desc: Test Cloud sync without schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync022, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Get Memory DB.
     * @tc.expected: step1 OK.
     */
    KvStoreNbDelegate *memoryDB1 = nullptr;
    KvStoreNbDelegate::Option option;
    option.isMemoryDb = true;
    DBStatus openRet = OK;
    g_mgr.GetKvStore(STORE_ID_4, option, [&openRet, &memoryDB1](DBStatus status, KvStoreNbDelegate *openDelegate) {
        openRet = status;
        memoryDB1 = openDelegate;
    });
    EXPECT_EQ(openRet, OK);
    ASSERT_NE(memoryDB1, nullptr);
    /**
     * @tc.steps:step2. Sync without cloud schema.
     * @tc.expected: step2 SCHEMA_MISMATCH.
     */
    BlockSync(memoryDB1, OK, g_CloudSyncoption, CLOUD_ERROR);
    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
    cloudDbs[USER_ID] = virtualCloudDb_;
    cloudDbs[USER_ID_2] = virtualCloudDb2_;
    memoryDB1->SetCloudDB(cloudDbs);
    BlockSync(memoryDB1, OK, g_CloudSyncoption, SCHEMA_MISMATCH);
    EXPECT_EQ(g_mgr.CloseKvStore(memoryDB1), OK);
}

/**
 * @tc.name: NormalSync023
 * @tc.desc: Test normal local delete before cloud delete.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync023, TestSize.Level0)
{
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(k1, v1), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100ms
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    ASSERT_EQ(kvDelegatePtrS2_->Delete(k1), OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep 100ms
    ASSERT_EQ(kvDelegatePtrS1_->Delete(k1), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
}

/**
 * @tc.name: NormalSync024
 * @tc.desc: Test duplicate addition, deletion, and sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync024, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Device A inserts data and synchronizes, then Device B synchronizes.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step2. Device A deletes data and synchronizes, then Device B synchronizes.
     * @tc.expected: step2 OK.
     */
    ASSERT_EQ(kvDelegatePtrS1_->Delete(key), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step3. Device B inserts data and synchronizes it.
     * @tc.expected: step3 OK.
     */
    int insertNum = 0;
    virtualCloudDb_->SetInsertHook([&insertNum](VBucket &record) {
        insertNum++;
    });
    ASSERT_EQ(kvDelegatePtrS2_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    EXPECT_TRUE(insertNum > 0);
    virtualCloudDb_->SetInsertHook(nullptr);
}

/**
 * @tc.name: NormalSync025
 * @tc.desc: Test merge sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync025, TestSize.Level0)
{
    virtualCloudDb_->SetBlockTime(1000); // block query 1000ms
    auto option = g_CloudSyncoption;
    option.merge = false;
    EXPECT_EQ(kvDelegatePtrS1_->Sync(option, nullptr), OK);
    option.merge = true;
    EXPECT_EQ(kvDelegatePtrS1_->Sync(option, nullptr), OK);
    BlockSync(kvDelegatePtrS1_, CLOUD_SYNC_TASK_MERGED, option);
}

/**
 * @tc.name: NormalSync026
 * @tc.desc: Test delete when sync mode DEVICE_COLLABORATION.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync026, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create a database with the DEVICE_COLLABORATION mode on device1.
     * @tc.expected: step1 OK.
     */
    KvStoreNbDelegate* kvDelegatePtrS3_ = nullptr;
    KvStoreNbDelegate::Option option;
    option.conflictResolvePolicy = DEVICE_COLLABORATION;
    EXPECT_EQ(GetKvStore(kvDelegatePtrS3_, STORE_ID_3, option), OK);
    /**
     * @tc.steps:step2. put 1 record and sync.
     * @tc.expected: step2 OK.
     */
    Key key = {'k'};
    Value expectValue1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS3_->Put(key, expectValue1), OK);
    BlockSync(kvDelegatePtrS3_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step3. Update this record on device2.
     * @tc.expected: step3 OK.
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    ASSERT_EQ(kvDelegatePtrS1_->Delete(key), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step4. device1 sync.
     * @tc.expected: step4. The record was not covered by the cloud and cloud was covered.
     */
    BlockSync(kvDelegatePtrS3_, OK, g_CloudSyncoption);
    Value actualValue1;
    EXPECT_EQ(kvDelegatePtrS3_->Get(key, actualValue1), OK);
    EXPECT_EQ(actualValue1, expectValue1);
    CloseKvStore(kvDelegatePtrS3_, STORE_ID_3);
}

/**
 * @tc.name: NormalSync027
 * @tc.desc: Test lock failed during data insert.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync027, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put 1 record and sync.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    /**
     * @tc.steps:step2. Lock cloud DB, and do sync.
     * @tc.expected: step2 sync failed due to lock fail, and failCount is 1.
     */
    virtualCloudDb_->Lock();
    BlockSync(kvDelegatePtrS1_, CLOUD_LOCK_ERROR, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 1u);
        EXPECT_EQ(table.second.upLoadInfo.successCount, 0u);
        EXPECT_EQ(table.second.upLoadInfo.failCount, 1u);
    }
    virtualCloudDb_->UnLock();
}

/**
 * @tc.name: NormalSync028
 * @tc.desc: Test multi user sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync028, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put 1 record and sync.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, value), OK);
    auto option = g_CloudSyncoption;
    option.users = {USER_ID, USER_ID_2};
    BlockSync(kvDelegatePtrS1_, OK, option);
    option.users = {USER_ID_2};
    BlockSync(kvDelegatePtrS2_, OK, option);
    option.users = {USER_ID, USER_ID_2};
    BlockSync(kvDelegatePtrS2_, OK, option);
    EXPECT_EQ(lastProcess_.tableProcess[USER_ID_2].downLoadInfo.total, 0u);
}

/**
 * @tc.name: NormalSync029
 * @tc.desc: Test lock failed during data insert.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync029, TestSize.Level3)
{
    /**
     * @tc.steps:step1. block virtual cloud.
     */
    virtualCloudDb_->SetBlockTime(1000); // block 1s
    /**
     * @tc.steps:step2. Call sync and let it run with block cloud.
     * @tc.expected: step2 sync ok.
     */
    auto option = g_CloudSyncoption;
    std::mutex finishMutex;
    std::condition_variable cv;
    std::vector<std::map<std::string, SyncProcess>> processRes;
    auto callback = [&processRes, &finishMutex, &cv](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item : process) {
            if (item.second.process != ProcessStatus::FINISHED) {
                return;
            }
        }
        {
            std::lock_guard<std::mutex> autoLock(finishMutex);
            processRes.push_back(process);
        }
        cv.notify_all();
    };
    EXPECT_EQ(kvDelegatePtrS1_->Sync(option, callback), OK);
    /**
     * @tc.steps:step3. USER0 sync first and USER2 sync second.
     * @tc.expected: step3 sync ok and USER2 task has been merged.
     */
    option.merge = true;
    option.users = {USER_ID};
    EXPECT_EQ(kvDelegatePtrS1_->Sync(option, callback), OK);
    option.users = {USER_ID_2};
    EXPECT_EQ(kvDelegatePtrS1_->Sync(option, callback), OK);
    /**
     * @tc.steps:step4. Wait all task finished.
     * @tc.expected: step4 Second sync task has USER0 and USER2.
     */
    virtualCloudDb_->SetBlockTime(0); // cancel block for test case run quickly
    std::unique_lock<std::mutex> uniqueLock(finishMutex);
    cv.wait_for(uniqueLock, std::chrono::milliseconds(DBConstant::MAX_TIMEOUT), [&processRes]() {
        return processRes.size() >= 3; // call sync 3 times
    });
    EXPECT_EQ(processRes[processRes.size() - 1].size(), 2u); // check the last process has 2 user
}

/**
 * @tc.name: NormalSync030
 * @tc.desc: Test sync while set null cloudDB
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync030, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create a database.
     * @tc.expected: step1 OK.
     */
    KvStoreNbDelegate* kvDelegatePtrS3_ = nullptr;
    KvStoreNbDelegate::Option option;
    DBStatus ret = OK;
    g_mgr.GetKvStore(STORE_ID_3, option, [&ret, &kvDelegatePtrS3_](DBStatus status, KvStoreNbDelegate *openDelegate) {
        ret = status;
        kvDelegatePtrS3_ = openDelegate;
    });
    EXPECT_EQ(ret, OK);
    ASSERT_NE(kvDelegatePtrS3_, nullptr);
    /**
     * @tc.steps:step2. Put {k, v}.
     * @tc.expected: step2 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS3_->Put(key, value), OK);
    /**
     * @tc.steps:step3. Set null cloudDB.
     * @tc.expected: step3 CLOUD_ERROR.
     */
    BlockSync(kvDelegatePtrS3_, OK, g_CloudSyncoption, CLOUD_ERROR);
    std::map<std::string, std::shared_ptr<ICloudDb>> cloudDbs;
    cloudDbs[USER_ID] = nullptr;
    cloudDbs[USER_ID_2] = virtualCloudDb2_;
    EXPECT_EQ(kvDelegatePtrS3_->SetCloudDB(cloudDbs), INVALID_ARGS);
    /**
     * @tc.steps:step4. Sync while set null cloudDB.
     * @tc.expected: step4 CLOUD_ERROR.
     */
    BlockSync(kvDelegatePtrS3_, OK, g_CloudSyncoption, CLOUD_ERROR);
    EXPECT_EQ(g_mgr.CloseKvStore(kvDelegatePtrS3_), OK);
}

/**
 * @tc.name: NormalSync031
 * @tc.desc: Test sync with error local device
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync031, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put 1 record and sync.
     * @tc.expected: step1 OK.
     */
    Key key = {'k'};
    Value value = {'v'};
    ASSERT_EQ(kvDelegatePtrS2_->Put(key, value), OK);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step2. Set local devices error and sync.
     * @tc.expected: step2 sync fail.
     */
    communicatorAggregator_->MockGetLocalDeviceRes(-E_CLOUD_ERROR);
    BlockSync(kvDelegatePtrS1_, CLOUD_ERROR, g_CloudSyncoption);
    communicatorAggregator_->MockGetLocalDeviceRes(E_OK);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.downLoadInfo.total, 0u);
        EXPECT_EQ(table.second.downLoadInfo.failCount, 0u);
        EXPECT_EQ(table.second.upLoadInfo.total, 0u);
    }
}

/**
 * @tc.name: NormalSync032
 * @tc.desc: Test some record upload fail in 1 batch.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync032, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put 10 records.
     * @tc.expected: step1 ok.
     */
    vector<Entry> entries;
    int count = 10; // put 10 records.
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        entries.push_back({key, value});
    }
    EXPECT_EQ(kvDelegatePtrS1_->PutBatch(entries), OK);
    /**
     * @tc.steps:step2. sync and set the last record upload fail.
     * @tc.expected: step2 sync fail and upLoadInfo.failCount is 1.
     */
    int uploadFailId = 0;
    virtualCloudDb_->ForkInsertConflict([&uploadFailId](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        uploadFailId++;
        if (uploadFailId == 10) { // 10 is the last record
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_ERROR);
            return CLOUD_ERROR;
        }
        return OK;
    });
    BlockSync(kvDelegatePtrS1_, CLOUD_ERROR, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 10u);
        EXPECT_EQ(table.second.upLoadInfo.successCount, 9u);
        EXPECT_EQ(table.second.upLoadInfo.insertCount, 9u);
        EXPECT_EQ(table.second.upLoadInfo.failCount, 1u);
    }
    virtualCloudDb_->ForkUpload(nullptr);
}

/**
 * @tc.name: NormalSync033
 * @tc.desc: test sync with different operation type and check upLoadInfo
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync033, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put local records {k1, v1} {k2, v2} and sync to cloud.
     * @tc.expected: step1 ok.
     */
    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    kvDelegatePtrS1_->Put(key1, value1);
    Key key2 = {'k', '2'};
    Value value2 = {'v', '2'};
    kvDelegatePtrS1_->Put(key2, value2);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step2. put {k3, v3}, delete {k1, v1}, and put {k2, v3}
     * @tc.expected: step2 ok.
     */
    Key key3 = {'k', '3'};
    Value value3 = {'v', '3'};
    kvDelegatePtrS1_->Put(key3, value3);
    kvDelegatePtrS1_->Delete(key1);
    kvDelegatePtrS1_->Put(key2, value3);
    /**
     * @tc.steps:step3. sync and check upLoadInfo
     * @tc.expected: step3 ok.
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 3u);
        EXPECT_EQ(table.second.upLoadInfo.batchIndex, 3u);
        EXPECT_EQ(table.second.upLoadInfo.successCount, 3u);
        EXPECT_EQ(table.second.upLoadInfo.insertCount, 1u);
        EXPECT_EQ(table.second.upLoadInfo.deleteCount, 1u);
        EXPECT_EQ(table.second.upLoadInfo.updateCount, 1u);
        EXPECT_EQ(table.second.upLoadInfo.failCount, 0u);
    }
}

/**
 * @tc.name: NormalSync034
 * @tc.desc: test sync data which size > maxUploadSize.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync034, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put data which size > maxUploadSize.
     * @tc.expected: step1 ok.
     */
    CloudSyncConfig config;
    int maxUploadSize = 1024000;
    int maxUploadCount = 40;
    config.maxUploadSize = maxUploadSize;
    config.maxUploadCount = maxUploadCount;
    kvDelegatePtrS1_->SetCloudSyncConfig(config);
    Key key = {'k'};
    Value value = {'v'};
    value.insert(value.end(), maxUploadSize, '0');
    kvDelegatePtrS1_->Put(key, value);
    /**
     * @tc.steps:step2. put entries which count > maxUploadCount.
     * @tc.expected: step2 ok.
     */
    vector<Entry> entries;
    for (int i = 0; i < maxUploadCount; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        entries.push_back({key, value});
    }
    EXPECT_EQ(kvDelegatePtrS1_->PutBatch(entries), OK);
    /**
     * @tc.steps:step3. sync and check upLoadInfo.batchIndex.
     * @tc.expected: step3 ok.
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.batchIndex, 2u);
    }
}

/**
 * @tc.name: NormalSync035
 * @tc.desc:test sync after export and import
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync035, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put local records {k1, v1} and sync to cloud.
     * @tc.expected: step1 ok.
     */
    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, value1), OK);
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    Value actualValue1;
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue1), OK);
    EXPECT_EQ(actualValue1, value1);
    auto result = kvDelegatePtrS1_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "1");
    }
    /**
     * @tc.steps:step2. export and import.
     * @tc.expected: step2 export and import ok.
     */
    std::string singleExportFileName = g_testDir + "/NormalSync034.$$";
    CipherPassword passwd;
    EXPECT_EQ(kvDelegatePtrS1_->Export(singleExportFileName, passwd), OK);
    EXPECT_EQ(kvDelegatePtrS1_->Import(singleExportFileName, passwd), OK);
    /**
     * @tc.steps:step3. put {k1, v2} and sync to cloud.
     * @tc.expected: step3 ok.
     */
    Value value2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, value2), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    Value actualValue2;
    EXPECT_EQ(kvDelegatePtrS1_->Get(key1, actualValue2), OK);
    EXPECT_EQ(actualValue2, value2);
    result = kvDelegatePtrS1_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "11");
    }
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
}

/**
 * @tc.name: NormalSync036
 * @tc.desc: test sync data with SetCloudSyncConfig.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync036, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put data and SetCloudSyncConfig.
     * @tc.expected: step1 ok.
     */
    CloudSyncConfig config;
    int maxUploadCount = 40;
    config.maxUploadCount = maxUploadCount;
    kvDelegatePtrS1_->SetCloudSyncConfig(config);
    Key key = {'k', '1'};
    Value value = {'v', '1'};
    kvDelegatePtrS1_->Put(key, value);
    /**
     * @tc.steps:step2. sync.
     * @tc.expected: step2 ok.
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
}

/**
 * @tc.name: NormalSync037
 * @tc.desc: test update sync data while local and cloud record device is same and not local device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync037, TestSize.Level0)
{
    /**
     * @tc.steps:step1. deviceA put {k1, v1} and sync to cloud.
     * @tc.expected: step1. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    Key key = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue1), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step2. deviceB sync to cloud.
     * @tc.expected: step2. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue1;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue1), OK);
    EXPECT_EQ(actualValue1, expectValue1);
    /**
     * @tc.steps:step3. deviceA put {k1, v2} and sync to cloud.
     * @tc.expected: step3. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue2), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step4. deviceB sync to cloud.
     * @tc.expected: step4. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue2;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue2), OK);
    EXPECT_EQ(actualValue2, expectValue2);
}

/**
 * @tc.name: NormalSync038
 * @tc.desc: test insert sync data while local and cloud record device is same and not local device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync038, TestSize.Level0)
{
    /**
     * @tc.steps:step1. deviceA put {k1, v1} and sync to cloud.
     * @tc.expected: step1. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, expectValue1), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step2. deviceB sync to cloud.
     * @tc.expected: step2. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue1;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue1), OK);
    EXPECT_EQ(actualValue1, expectValue1);
    /**
     * @tc.steps:step3. deviceA put {k2, v2} and sync to cloud.
     * @tc.expected: step3. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    Key key2 = {'k', '2'};
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key2, expectValue2), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step4. deviceB sync to cloud.
     * @tc.expected: step4. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    actualValue1.clear();
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue1), OK);
    EXPECT_EQ(actualValue1, expectValue1);
    Value actualValue2;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key2, actualValue2), OK);
    EXPECT_EQ(actualValue2, expectValue2);
}

/**
 * @tc.name: NormalSync039
 * @tc.desc: test delete sync data while local and cloud record device is same and not local device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync039, TestSize.Level0)
{
    /**
     * @tc.steps:step1. deviceA put {k1, v1} {k2, v2} and sync to cloud.
     * @tc.expected: step1. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, expectValue1), OK);
    Key key2 = {'k', '2'};
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key2, expectValue2), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step2. deviceB sync to cloud.
     * @tc.expected: step2. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue1;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue1), OK);
    EXPECT_EQ(actualValue1, expectValue1);
    Value actualValue2;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key2, actualValue2), OK);
    EXPECT_EQ(actualValue2, expectValue2);
    /**
     * @tc.steps:step3. deviceA delete {k2, v2} and sync to cloud.
     * @tc.expected: step3. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    ASSERT_EQ(kvDelegatePtrS1_->Delete(key2), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step4. deviceB sync to cloud.
     * @tc.expected: step4. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    actualValue1.clear();
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue1), OK);
    EXPECT_EQ(actualValue1, expectValue1);
    actualValue2.clear();
    EXPECT_EQ(kvDelegatePtrS2_->Get(key2, actualValue2), NOT_FOUND);
    EXPECT_NE(actualValue2, expectValue2);
}

/**
 * @tc.name: NormalSync040
 * @tc.desc: test update sync data while local and cloud record device is different and not local device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync040, TestSize.Level0)
{
    /**
     * @tc.steps:step1. deviceA put {k1, v1} and sync to cloud.
     * @tc.expected: step1. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    Key key = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue1), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step2. deviceB sync to cloud.
     * @tc.expected: step2. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue1;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue1), OK);
    EXPECT_EQ(actualValue1, expectValue1);
    /**
     * @tc.steps:step3. deviceA put {k1, v2} and sync to cloud.
     * @tc.expected: step3. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key, expectValue2), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps:step4. deviceB put {k1, v3} sync to cloud.
     * @tc.expected: step4. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    Value expectValue3 = {'v', '3'};
    ASSERT_EQ(kvDelegatePtrS2_->Put(key, expectValue3), OK);
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue2;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue2), OK);
    EXPECT_NE(actualValue2, expectValue2);
    EXPECT_EQ(actualValue2, expectValue3);
}

/**
 * @tc.name: NormalSync041
 * @tc.desc: Test concurrent sync and close DB.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync041, TestSize.Level1)
{
    /**
     * @tc.steps:step1. put data to cloud.
     * @tc.expected: step1 ok.
     */
    Key key = {'k', '1'};
    Value value = {'v', '1'};
    kvDelegatePtrS1_->Put(key, value);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);

    /**
     * @tc.steps:step2. sync and close DB concurrently.
     * @tc.expected: step2 ok.
     */
    KvStoreNbDelegate* kvDelegatePtrS3_ = nullptr;
    KvStoreNbDelegate::Option option;
    EXPECT_EQ(GetKvStore(kvDelegatePtrS3_, STORE_ID_3, option), OK);
    virtualCloudDb_->ForkQuery([](const std::string &tableName, VBucket &extend) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // sleep for 200ms
    });
    KvStoreDelegateManager &mgr = g_mgr;
    std::thread syncThread([&mgr, &kvDelegatePtrS3_]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // sleep for 100ms
        EXPECT_EQ(mgr.CloseKvStore(kvDelegatePtrS3_), OK);
    });
    EXPECT_EQ(kvDelegatePtrS3_->Sync(g_CloudSyncoption, nullptr), OK);
    syncThread.join();
}

/**
 * @tc.name: NormalSync042
 * @tc.desc: Test some record upload fail in 1 batch and get cloud version successfully.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync042, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put 10 records.
     * @tc.expected: step1 ok.
     */
    vector<Entry> entries;
    int count = 10; // put 10 records.
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        entries.push_back({key, value});
    }
    EXPECT_EQ(kvDelegatePtrS1_->PutBatch(entries), OK);
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    /**
     * @tc.steps:step2. sync and set the last record upload fail.
     * @tc.expected: step2 sync fail and upLoadInfo.failCount is 1.
     */
    int uploadFailId = 0;
    virtualCloudDb_->ForkInsertConflict([&uploadFailId](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        uploadFailId++;
        if (uploadFailId == 10) { // 10 is the last record
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_ERROR);
            return CLOUD_ERROR;
        }
        return OK;
    });
    BlockSync(kvDelegatePtrS1_, CLOUD_ERROR, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 10u);
        EXPECT_EQ(table.second.upLoadInfo.successCount, 9u);
        EXPECT_EQ(table.second.upLoadInfo.insertCount, 9u);
        EXPECT_EQ(table.second.upLoadInfo.failCount, 1u);
    }
    /**
     * @tc.steps:step3. get cloud version successfully.
     * @tc.expected: step3 OK.
     */
    auto result = kvDelegatePtrS1_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "1");
    }
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    virtualCloudDb_->ForkUpload(nullptr);
}

/**
 * @tc.name: NormalSync043
 * @tc.desc: Test some record upload fail in 1 batch and get cloud version failed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync043, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put 10 records.
     * @tc.expected: step1 ok.
     */
    vector<Entry> entries;
    int count = 10; // put 10 records.
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        entries.push_back({key, value});
    }
    EXPECT_EQ(kvDelegatePtrS1_->PutBatch(entries), OK);
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    /**
     * @tc.steps:step2. sync and set all record upload fail.
     * @tc.expected: step2 sync fail and upLoadInfo.failCount is 10.
     */
    virtualCloudDb_->ForkInsertConflict([](const std::string &tableName, VBucket &extend, VBucket &record,
        std::vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_ERROR);
        return CLOUD_ERROR;
    });
    BlockSync(kvDelegatePtrS1_, CLOUD_ERROR, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 10u);
        EXPECT_EQ(table.second.upLoadInfo.successCount, 0u);
        EXPECT_EQ(table.second.upLoadInfo.insertCount, 0u);
        EXPECT_EQ(table.second.upLoadInfo.failCount, 10u);
    }
    /**
     * @tc.steps:step3. get cloud version failed.
     * @tc.expected: step3 NOT_FOUND.
     */
    auto result = kvDelegatePtrS1_->GetCloudVersion("");
    EXPECT_EQ(result.first, NOT_FOUND);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "");
    }
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    virtualCloudDb_->ForkUpload(nullptr);
}

/**
 * @tc.name: NormalSync044
 * @tc.desc: Test RemoveDeviceData with FLAG_ONLY option
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangtao
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync044, TestSize.Level0)
{
    /**
     * @tc.steps:step1. store1 put (k1,v1) and (k2,v2)
     * @tc.expected: step1. both put ok
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    Key key2 = {'k', '2'};
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, expectValue1), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Put(key2, expectValue2), OK);
    /**
     * @tc.steps: step2. DEVICE_A with store1 sync and DEVICE_B with store2 sync
     * @tc.expected: step2. both sync ok, and store2 got (k1,v1) and (k2,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    LOGW("Store1 sync end");
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    LOGW("Store2 sync end");
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue1);
    EXPECT_EQ(kvDelegatePtrS2_->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue2);
    /**
     * @tc.steps: step3. store2 RevoveDeviceData with FLAG_ONLY option
     * @tc.expected: step3. store2 delete DEVICE_A's version CloudVersion data successfully
     */
    auto result = kvDelegatePtrS2_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "1");
    }
    EXPECT_EQ(kvDelegatePtrS2_->RemoveDeviceData("DEVICES_A", ClearMode::FLAG_ONLY), OK);
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    result = kvDelegatePtrS2_->GetCloudVersion("");
    EXPECT_EQ(result.first, NOT_FOUND);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "");
    }
}

/**
 * @tc.name: NormalSync045
 * @tc.desc: Test some record upload fail in 1 batch and extend size greater than record size
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangtao
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync045, TestSize.Level0)
{
    /**
     * @tc.steps:step1. put 10 records.
     * @tc.expected: step1 ok.
     */
    vector<Entry> entries;
    int count = 10; // put 10 records.
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        entries.push_back({key, value});
    }
    EXPECT_EQ(kvDelegatePtrS1_->PutBatch(entries), OK);
    /**
     * @tc.steps:step2. sync and add one empty extend as result
     * @tc.expected: step2 sync fail and upLoadInfo.failCount is 10. 1 batch failed.
     */
    std::atomic<int> missCount = -1;
    virtualCloudDb_->SetClearExtend(missCount);
    BlockSync(kvDelegatePtrS1_, CLOUD_ERROR, g_CloudSyncoption);
    for (const auto &table : lastProcess_.tableProcess) {
        EXPECT_EQ(table.second.upLoadInfo.total, 10u);
        EXPECT_EQ(table.second.upLoadInfo.successCount, 0u);
        EXPECT_EQ(table.second.upLoadInfo.insertCount, 0u);
        EXPECT_EQ(table.second.upLoadInfo.failCount, 10u);
    }
    virtualCloudDb_->ForkUpload(nullptr);
}

/**
 * @tc.name: NormalSync046
 * @tc.desc: Test RemoveDeviceData with FLAG_ONLY option and empty deviceName
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenghuitao
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync046, TestSize.Level0)
{
    /**
     * @tc.steps:step1. store1 put (k1,v1) and (k2,v2)
     * @tc.expected: step1. both put ok
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    kvDelegatePtrS1_->SetGenCloudVersionCallback([](const std::string &origin) {
        LOGW("origin is %s", origin.c_str());
        return origin + "1";
    });
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    Key key2 = {'k', '2'};
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, expectValue1), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Put(key2, expectValue2), OK);
    /**
     * @tc.steps: step2. DEVICE_A with store1 sync and DEVICE_B with store2 sync
     * @tc.expected: step2. both sync ok, and store2 got (k1,v1) and (k2,v2)
     */
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    LOGW("Store1 sync end");
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    LOGW("Store2 sync end");
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue1);
    EXPECT_EQ(kvDelegatePtrS2_->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue2);
    /**
     * @tc.steps: step3. store2 RevoveDeviceData with FLAG_ONLY option
     * @tc.expected: step3. store2 delete DEVICE_A's version CloudVersion data successfully
     */
    auto result = kvDelegatePtrS2_->GetCloudVersion("");
    EXPECT_EQ(result.first, OK);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "1");
    }
    EXPECT_EQ(kvDelegatePtrS2_->RemoveDeviceData("", ClearMode::FLAG_ONLY), OK);
    kvDelegatePtrS1_->SetGenCloudVersionCallback(nullptr);
    result = kvDelegatePtrS2_->GetCloudVersion("");
    EXPECT_EQ(result.first, NOT_FOUND);
    for (auto item : result.second) {
        EXPECT_EQ(item.second, "");
    }
}

/**
 * @tc.name: NormalSync047
 * @tc.desc: Test multi users sync when user1 sync fail.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync047, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put 20 records.
     * @tc.expected: step1. ok.
     */
    vector<Entry> entries;
    int count = 20; // put 20 records.
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        entries.push_back({key, value});
    }
    EXPECT_EQ(kvDelegatePtrS1_->PutBatch(entries), OK);

    /**
     * @tc.steps: step2. multi users sync and set user1 fail.
     * @tc.expected: step2. user1 sync fail and other user sync success.
     */
    int uploadFailId = 0;
    virtualCloudDb_->ForkInsertConflict([&uploadFailId](const std::string &tableName, VBucket &extend, VBucket &record,
        vector<VirtualCloudDb::CloudData> &cloudDataVec) {
        uploadFailId++;
        if (uploadFailId > 15) { // the first 15 records success
            extend[CloudDbConstant::ERROR_FIELD] = static_cast<int64_t>(DBStatus::CLOUD_ERROR);
            return CLOUD_ERROR;
        }
        return OK;
    });
    CloudSyncOption option;
    option.mode = SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH;
    option.users.push_back(USER_ID);
    option.users.push_back(USER_ID_2);
    option.devices.push_back("cloud");
    SyncAndGetProcessInfo(kvDelegatePtrS1_, option);

    vector<DBStatus> userStatus = {CLOUD_ERROR, OK};
    vector<Info> userExpectInfo = {{1u, 20u, 15u, 5u, 15u, 0u, 0u}, {1u, 20u, 20u, 0u, 20u, 0u, 0u}};
    EXPECT_TRUE(CheckUserSyncInfo(option.users, userStatus, userExpectInfo));
    virtualCloudDb_->ForkUpload(nullptr);
}

/**
 * @tc.name: NormalSync048
 * @tc.desc: test sync data while cloud delete on record and local do not have this record.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync048, TestSize.Level0)
{
    /**
     * @tc.steps: step1. deviceB put {k1, v1} {k2, v2} and sync to cloud
     * @tc.expected: step1. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    Key key1 = {'k', '1'};
    Value expectValue1 = {'v', '1'};
    Key key2 = {'k', '2'};
    Value expectValue2 = {'v', '2'};
    ASSERT_EQ(kvDelegatePtrS1_->Put(key1, expectValue1), OK);
    ASSERT_EQ(kvDelegatePtrS1_->Put(key2, expectValue2), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);

    /**
     * @tc.steps: step2. deviceB delete {k1, v1} and sync to cloud
     * @tc.expected: step2. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_B");
    ASSERT_EQ(kvDelegatePtrS1_->Delete(key1), OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);

    /**
     * @tc.steps: step3. deviceA sync to cloud
     * @tc.expected: step3. ok.
     */
    communicatorAggregator_->SetLocalDeviceId("DEVICES_A");
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    Value actualValue2;
    ASSERT_EQ(kvDelegatePtrS2_->Get(key2, actualValue2), OK);
    ASSERT_EQ(actualValue2, expectValue2);
}

/**
 * @tc.name: NormalSync049
 * @tc.desc: test sync data with invalid modify_time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBCloudKvTest, NormalSync049, TestSize.Level0)
{
    /**
     * @tc.steps: step1. put {k, v}
     * @tc.expected: step1. ok.
     */
    vector<Entry> entries;
    int count = 31; // put 31 records.
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        std::string valueStr = "v_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        Value value(valueStr.begin(), valueStr.end());
        entries.push_back({key, value});
    }
    kvDelegatePtrS1_->PutBatch(entries);
    /**
     * @tc.steps: step2. modify time in local and sync
     * @tc.expected: step2. ok.
     */
    sqlite3 *db_;
    uint64_t openFlag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    std::string fileUrl = g_testDir + "/" \
        "2d23c8a0ffadafcaa03507a4ec2290c83babddcab07c0e2945fbba93efc7eec0/single_ver/main/gen_natural_store.db";
    ASSERT_TRUE(sqlite3_open_v2(fileUrl.c_str(), &db_, openFlag, nullptr) == SQLITE_OK);
    std::string sql = "UPDATE sync_data SET modify_time = modify_time + modify_time where rowid>0";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db_, sql), E_OK);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    /**
     * @tc.steps: step3. put {k, v2} in another device
     * @tc.expected: step3. ok.
     */
    Value newValue = {'v'};
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        entries.push_back({key, newValue});
    }
    kvDelegatePtrS2_->PutBatch(entries);
    /**
     * @tc.steps: step4. sync and check data
     * @tc.expected: step4. ok.
     */
    BlockSync(kvDelegatePtrS2_, OK, g_CloudSyncoption);
    BlockSync(kvDelegatePtrS1_, OK, g_CloudSyncoption);
    Value actualValue;
    for (int i = 0; i < count; i++) {
        std::string keyStr = "k_" + std::to_string(i);
        Key key(keyStr.begin(), keyStr.end());
        EXPECT_EQ(kvDelegatePtrS1_->Get(key, actualValue), OK);
        EXPECT_EQ(actualValue, newValue);
    }
}
}
