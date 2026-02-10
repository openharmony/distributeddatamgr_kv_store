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

#include "kv_general_ut.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBKvCloudSyncTest : public KVGeneralUt {
public:
    void SetUp() override;
protected:
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
};

void DistributedDBKvCloudSyncTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_B), E_OK);
}

#ifdef USE_DISTRIBUTEDDB_CLOUD
/**
 * @tc.name: CloudSyncAfterImport001
 * @tc.desc: Test sync cloud after import.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvCloudSyncTest, CloudSyncAfterImport001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. dev1 put (k,v)
     * @tc.expected: step1. put return OK.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    ASSERT_EQ(SetCloud(store1), OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    ASSERT_EQ(SetCloud(store2), OK);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    /**
     * @tc.steps: step2. dev2 export bak db
     * @tc.expected: step2. export should return OK.
     */
    std::string singleFileName = GetTestDir() + "/CloudSyncAfterImport001";
    CipherPassword pwd;
    store2->Export(singleFileName, pwd);
    /**
     * @tc.steps: step3. dev1 put (k,v) and sync cloud and dev2 sync cloud
     * @tc.expected: step3. sync should return OK and dev2 exist (k,v).
     */
    BlockCloudSync(storeInfo1, DEVICE_A);
    BlockCloudSync(storeInfo2, DEVICE_B);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step4. dev2 import bak db
     * @tc.expected: step4. import should return OK and dev2 not exist (k,v).
     */
    ASSERT_EQ(store2->Import(singleFileName, pwd), OK);
    EXPECT_EQ(store2->Get({'k'}, actualValue), NOT_FOUND);
    /**
     * @tc.steps: step5. dev2 sync cloud
     * @tc.expected: step5. sync should return OK and dev2 exist (k,v).
     */
    BlockCloudSync(storeInfo2, DEVICE_B);
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}

/**
 * @tc.name: CloudSyncAbnormal001
 * @tc.desc: Test sync cloud after cloud return 404.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKvCloudSyncTest, CloudSyncAbnormal001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set cloud return CLOUD_NOT_FOUND and cloud sync
     * @tc.expected: step1. sync return CLOUD_NOT_FOUND.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    ASSERT_EQ(SetCloud(store1), OK);
    SetActionStatus(CLOUD_ASSET_NOT_FOUND);
    EXPECT_NO_FATAL_FAILURE(BlockCloudSync(storeInfo1, DEVICE_A, OK, CLOUD_ASSET_NOT_FOUND));
    SetActionStatus(OK);
}

/**
 * @tc.name: CloudSyncTest001
 * @tc.desc: Test sync cloud when query return E_EXPIRED_CURSOR.
 * @tc.type: FUNC
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBKvCloudSyncTest, CloudSyncTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v)
     * @tc.expected: step1. put return OK.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    ASSERT_EQ(SetCloud(store1), OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    ASSERT_EQ(SetCloud(store2), OK);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    /**
     * @tc.steps: step2. sync and return E_EXPIRED_CURSOR when query
     * @tc.expected: step2. sync return OK.
    */
    std::atomic<int> count = 0;
    virtualCloudDb_->ForkAfterQueryResult([&count](VBucket &, std::vector<VBucket> &) {
        count++;
        return count == 1 ? DBStatus::EXPIRED_CURSOR : DBStatus::QUERY_END;
    });
    BlockCloudSync(storeInfo1, DEVICE_A);
    BlockCloudSync(storeInfo2, DEVICE_B);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}
#endif
} // namespace