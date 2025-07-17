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
class DistributedDBBasicKVTest : public KVGeneralUt {
public:
    void SetUp() override;
};

void DistributedDBBasicKVTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
    auto storeInfo3 = GetStoreInfo3();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo3, "dev3"), E_OK);
}

/**
 * @tc.name: ExampleSync001
 * @tc.desc: Test sync from dev1 to dev2.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicKVTest, ExampleSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync to dev2
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockPush(storeInfo1, storeInfo2);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. dev2's schemaVersion and softwareVersion both equal to dev1's meta
     * @tc.expected: step2. version both equal.
     */
    auto [errCode, version] = GetRemoteSoftwareVersion(storeInfo1, "dev2", DBConstant::DEFAULT_USER);
    EXPECT_EQ(errCode, OK);
    EXPECT_EQ(version, static_cast<uint64_t>(SOFTWARE_VERSION_CURRENT));
    std::tie(errCode, version) = GetRemoteSchemaVersion(storeInfo1, "dev2", DBConstant::DEFAULT_USER);
    EXPECT_EQ(errCode, OK);
    EXPECT_NE(version, 0);
    uint64_t store2SchemaVersion = 0;
    std::tie(errCode, store2SchemaVersion) = GetLocalSchemaVersion(storeInfo2);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(version, store2SchemaVersion);
}

/**
 * @tc.name: WhitelistKvGet001
 * @tc.desc: Test kv get interface for whitelist.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xd
 */
HWTEST_F(DistributedDBBasicKVTest, WhitelistKvGet001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set whitelist appId, put (k,v)
     * @tc.expected: step1. get (k,v) result.
     */
    auto storeInfo3 = GetStoreInfo3();
    auto store3 = GetDelegate(storeInfo3);
    ASSERT_NE(store3, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store3->Put({'k'}, expectValue), OK);
    Value actualValue;
    EXPECT_EQ(store3->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. with transaction, set whitelist appId, put (k,v)
     * @tc.expected: step2. get (k,v) result.
     */
    store3->StartTransaction();
    EXPECT_EQ(store3->Put({'k', '2'}, expectValue), OK);
    Value actualValue2;
    EXPECT_EQ(store3->Get({'k', '2'}, actualValue2), OK);
    EXPECT_EQ(actualValue2, expectValue);
    store3->Commit();
    /**
     * @tc.steps: step3. do not set whitelist appId, put (k,v)
     * @tc.expected: step3. get (k,v) result.
     */
    auto storeInfo2 = GetStoreInfo2();
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2->Put({'k'}, expectValue), OK);
    Value actualValue3;
    EXPECT_EQ(store2->Get({'k'}, actualValue3), OK);
    EXPECT_EQ(actualValue3, expectValue);
}
} // namespace DistributedDB