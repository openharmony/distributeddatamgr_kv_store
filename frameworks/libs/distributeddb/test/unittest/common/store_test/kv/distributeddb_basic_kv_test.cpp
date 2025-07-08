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
} // namespace DistributedDB