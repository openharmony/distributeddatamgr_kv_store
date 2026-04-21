/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifdef USE_DISTRIBUTEDDB_CLOUD
#include "kv_general_ut.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBKvCloudQueryTest : public KVGeneralUt {
public:
    void SetUp() override;
protected:
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
};

void DistributedDBKvCloudQueryTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_B), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    ASSERT_EQ(SetCloud(store1), OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    ASSERT_EQ(SetCloud(store2), OK);
}

/**
 * @tc.name: PrefixKey001
 * @tc.desc: Test sync cloud with prefix key.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvCloudQueryTest, PrefixKey001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v)
     * @tc.expected: step1. put return OK.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k', '1'}, expectValue), OK);
    EXPECT_EQ(store1->Put({'k', '2'}, expectValue), OK);
    /**
     * @tc.steps: step2. dev1 sync with k1 to dev2
     * @tc.expected: step2. sync should return OK.
     */
    CloudSyncOption syncOption;
    syncOption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    syncOption.users.push_back(DistributedDBUnitTest::USER_ID);
    syncOption.devices.emplace_back("cloud");
    syncOption.query = Query::Select().PrefixKey({'k', '1'});
    syncOption.queryMode = QueryMode::UPLOAD_ONLY;
    EXPECT_NO_FATAL_FAILURE(BlockCloudSync(storeInfo1, DEVICE_A, syncOption));
    auto storeInfo2 = GetStoreInfo2();
    EXPECT_NO_FATAL_FAILURE(BlockCloudSync(storeInfo2, DEVICE_B));
    Value actualValue;
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2->Get({'k', '1'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    EXPECT_EQ(store2->Get({'k', '2'}, actualValue), NOT_FOUND);
}

/**
 * @tc.name: PrefixKey002
 * @tc.desc: Test sync cloud with empty prefix key.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvCloudQueryTest, PrefixKey002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v)
     * @tc.expected: step1. put return OK.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k', '1'}, expectValue), OK);
    EXPECT_EQ(store1->Put({'k', '2'}, expectValue), OK);
    /**
     * @tc.steps: step2. dev1 sync with k1 to dev2
     * @tc.expected: step2. sync should return OK.
     */
    CloudSyncOption syncOption;
    syncOption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    syncOption.users.push_back(DistributedDBUnitTest::USER_ID);
    syncOption.devices.emplace_back("cloud");
    syncOption.query = Query::Select().PrefixKey({});
    syncOption.queryMode = QueryMode::UPLOAD_ONLY;
    EXPECT_NO_FATAL_FAILURE(BlockCloudSync(storeInfo1, DEVICE_A, syncOption));
    auto storeInfo2 = GetStoreInfo2();
    EXPECT_NO_FATAL_FAILURE(BlockCloudSync(storeInfo2, DEVICE_B));
    Value actualValue;
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2->Get({'k', '1'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    EXPECT_EQ(store2->Get({'k', '2'}, actualValue), OK);
}

/**
 * @tc.name: PrefixKey003
 * @tc.desc: Test prefix key not work without UPLOAD_ONLY.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvCloudQueryTest, PrefixKey003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v)
     * @tc.expected: step1. put return OK.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k', '1'}, expectValue), OK);
    EXPECT_EQ(store1->Put({'k', '2'}, expectValue), OK);
    /**
     * @tc.steps: step2. dev1 sync with k1 to dev2
     * @tc.expected: step2. sync should return OK.
     */
    CloudSyncOption syncOption;
    syncOption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    syncOption.users.push_back(DistributedDBUnitTest::USER_ID);
    syncOption.devices.emplace_back("cloud");
    syncOption.query = Query::Select().PrefixKey({'k', '1'});
    syncOption.queryMode = QueryMode::UPLOAD_AND_DOWNLOAD;
    EXPECT_NO_FATAL_FAILURE(BlockCloudSync(storeInfo1, DEVICE_A, syncOption));
    auto storeInfo2 = GetStoreInfo2();
    EXPECT_NO_FATAL_FAILURE(BlockCloudSync(storeInfo2, DEVICE_B));
    Value actualValue;
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2->Get({'k', '1'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    EXPECT_EQ(store2->Get({'k', '2'}, actualValue), OK);
}

/**
 * @tc.name: PrefixKey004
 * @tc.desc: Test not support query operation.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvCloudQueryTest, PrefixKey004, TestSize.Level0)
{
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    CloudSyncOption syncOption;
    syncOption.mode = SyncMode::SYNC_MODE_CLOUD_MERGE;
    syncOption.users.push_back(DistributedDBUnitTest::USER_ID);
    syncOption.devices.emplace_back("cloud");
    syncOption.query = Query::Select().InKeys({{'k'}});
    syncOption.queryMode = QueryMode::UPLOAD_ONLY;
    EXPECT_NO_FATAL_FAILURE(BlockCloudSync(storeInfo1, DEVICE_A, syncOption, NOT_SUPPORT, OK));
}
}
#endif