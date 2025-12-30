/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifdef USE_DISTRIBUTEDDB_DEVICE
#include "kv_general_ut.h"
#include "kv_store_nb_delegate_impl.h"

namespace DistributedDB {
    using namespace testing::ext;
    using namespace DistributedDB;
    using namespace DistributedDBUnitTest;

class DistributedDBKvDeviceSyncTest : public KVGeneralUt {
public:
    void SetUp() override;
protected:
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    DistributedDBToolsUnitTest g_tool;
};

void DistributedDBKvDeviceSyncTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_B), E_OK);
}

/**
 * @tc.name: NormalSyncTest001
 * @tc.desc: Test normal sync without retry.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBKvDeviceSyncTest, NormalSyncTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v)
     * @tc.expected: step1. return OK
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key key = {'k'};
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put(key, expectValue), OK);
    /**
     * @tc.steps: step2. dev1 sync to dev2
     * @tc.expected: step2. return OK and dev2 exist (k,v)
     */
    DeviceSyncOption option;
    option.devices = {DEVICE_B};
    option.mode = SYNC_MODE_PUSH_ONLY;
    option.isRetry = false;
    std::map<std::string, DBStatus> syncRet;
    EXPECT_EQ(g_tool.SyncTest(store1, option, syncRet), OK);
    for (const auto &ret : syncRet) {
        EXPECT_EQ(ret.first, DEVICE_B);
        EXPECT_EQ(ret.second, OK);
    }
    auto storeInfo2 = GetStoreInfo2();
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value actualValue;
    EXPECT_EQ(store2->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}

/**
 * @tc.name: NormalSyncTest002
 * @tc.desc: Test normal sync with multiple devices with the same name.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBKvDeviceSyncTest, NormalSyncTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. dev1 put (k,v)
     * @tc.expected: step1. return OK
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key key = {'k'};
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put(key, expectValue), OK);
    /**
     * @tc.steps: step2. dev1 sync to dev2
     * @tc.expected: step2. return OK and dev2 exist (k,v)
     */
    DeviceSyncOption option;
    option.devices = {DEVICE_B, DEVICE_B, DEVICE_B};
    option.mode = SYNC_MODE_PUSH_ONLY;
    option.isRetry = false;
    std::map<std::string, DBStatus> syncRet;
    EXPECT_EQ(g_tool.SyncTest(store1, option, syncRet), OK);
    for (const auto &ret : syncRet) {
        EXPECT_EQ(ret.first, DEVICE_B);
        EXPECT_EQ(ret.second, OK);
    }
    auto storeInfo2 = GetStoreInfo2();
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value actualValue;
    EXPECT_EQ(store2->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}

/**
 * @tc.name: AbnormalKvSyncTest001
 * @tc.desc: Test sync with invalid args.
 * @tc.type: FUNC
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBKvDeviceSyncTest, AbnormalKvSyncTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. dev1 put (k,v)
     * @tc.expected: step1. return OK
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Key key = {'k'};
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put(key, expectValue), OK);
    /**
     * @tc.steps: step2. dev1 sync to dev2 with invalid mode
     * @tc.expected: step2. return NOT_SUPPORT
     */
    DeviceSyncOption option;
    option.devices = {DEVICE_B};
    option.mode = SYNC_MODE_CLOUD_MERGE;
    option.isRetry = false;
    std::map<std::string, DBStatus> syncRet;
    EXPECT_EQ(g_tool.SyncTest(store1, option, syncRet), NOT_SUPPORT);
    auto storeInfo2 = GetStoreInfo2();
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value actualValue;
    EXPECT_EQ(store2->Get(key, actualValue), NOT_FOUND);
    option.mode = SYNC_MODE_PUSH_PULL;
    /**
     * @tc.steps: step3. dev1 sync to dev2 with invalid query
     * @tc.expected: step3. return NOT_SUPPORT
     */
    option.isQuery = true;
    option.query = Query::Select().FromTable({"table1", "table2"});
    EXPECT_EQ(g_tool.SyncTest(store1, option, syncRet), NOT_SUPPORT);
    EXPECT_EQ(store2->Get(key, actualValue), NOT_FOUND);
    option.query = Query::Select().From("A").From("B");
    EXPECT_EQ(g_tool.SyncTest(store1, option, syncRet), NOT_SUPPORT);
    EXPECT_EQ(store2->Get(key, actualValue), NOT_FOUND);
    /**
     * @tc.steps: step4. dev1 sync to dev2 with empty devices
     * @tc.expected: step4. return NOT_SUPPORT
     */
    option.devices.clear();
    option.isQuery = false;
    EXPECT_EQ(g_tool.SyncTest(store1, option, syncRet), INVALID_ARGS);
    EXPECT_EQ(store2->Get(key, actualValue), NOT_FOUND);
    /**
     * @tc.steps: step5. dev1 sync to dev2 after close db
     * @tc.expected: step5. return DB_ERROR
     */
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(store1);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    EXPECT_EQ(g_tool.SyncTest(store1, option, syncRet), DB_ERROR);
    EXPECT_EQ(store2->Get(key, actualValue), NOT_FOUND);
}
}
#endif