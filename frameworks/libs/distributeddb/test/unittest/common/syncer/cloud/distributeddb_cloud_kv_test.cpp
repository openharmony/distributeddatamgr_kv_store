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
#include "kv_store_nb_delegate.h"
#include "platform_specific.h"
#include "virtual_cloud_db.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
class DistributedDBCloudKvTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
protected:
    void GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId);
    void CloseKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId);
    static void BlockSync(KvStoreNbDelegate *delegate, DBStatus expect);
    static DataBaseSchema GetDataBaseSchema();
    std::shared_ptr<VirtualCloudDb> virtualCloudDb_ = nullptr;
    KvStoreConfig config_;
    KvStoreNbDelegate* kvDelegatePtrS1_ = nullptr;
    KvStoreNbDelegate* kvDelegatePtrS2_ = nullptr;
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
    GetKvStore(kvDelegatePtrS2_, STORE_ID_2);
}

void DistributedDBCloudKvTest::TearDown()
{
    CloseKvStore(kvDelegatePtrS1_, STORE_ID_1);
    CloseKvStore(kvDelegatePtrS2_, STORE_ID_2);
    virtualCloudDb_ = nullptr;
}

void DistributedDBCloudKvTest::BlockSync(KvStoreNbDelegate *delegate, DBStatus expect)
{
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    auto callback = [expect, &cv, &dataMutex, &finish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                EXPECT_EQ(item.second.errCode, expect);
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    finish = true;
                }
                cv.notify_one();
            }
        }
    };
    CloudSyncOption option;
    option.users.push_back(USER_ID);
    option.devices.push_back("cloud");
    ASSERT_EQ(delegate->Sync(option, callback), OK);
    std::unique_lock<std::mutex> uniqueLock(dataMutex);
    cv.wait(uniqueLock, [&finish]() {
        return finish;
    });
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

void DistributedDBCloudKvTest::GetKvStore(KvStoreNbDelegate *&delegate, const std::string &storeId)
{
    KvStoreNbDelegate::Option option;
    DBStatus openRet = OK;
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
    BlockSync(kvDelegatePtrS1_, OK);
    BlockSync(kvDelegatePtrS2_, OK);
    Value actualValue;
    EXPECT_EQ(kvDelegatePtrS2_->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
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
}