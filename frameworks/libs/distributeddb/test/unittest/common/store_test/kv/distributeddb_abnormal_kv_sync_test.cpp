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

class DistributedDBAbnormalKVSyncTest : public KVGeneralUt {
public:
    void SetUp() override;
};

void DistributedDBAbnormalKVSyncTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
}

/**
 * @tc.name: SyncWithInvalidSoftwareVersionTest001
 * @tc.desc: Test ability sync after software version is invalid.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBAbnormalKVSyncTest, SyncWithInvalidSoftwareVersionTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. open store and sync first
     * @tc.expected: step1. sync ok.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    BlockPush(storeInfo1, storeInfo2);
    /**
     * @tc.steps: step2. set version invalid and sync again after reopen store
     * @tc.expected: step2. sync ok.
     */
    ASSERT_EQ(SetRemoteSoftwareVersion(storeInfo1, "dev2", DBConstant::DEFAULT_USER, INT32_MAX), OK);
    KVGeneralUt::CloseDelegate(storeInfo1);
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    std::atomic<int> msgCount = 0;
    RegBeforeDispatch([&msgCount](const std::string &dev, const Message *inMsg) {
        if (dev != "dev2" || inMsg->GetMessageId() != ABILITY_SYNC_MESSAGE) {
            return;
        }
        msgCount++;
    });
    BlockPush(storeInfo1, storeInfo2);
    RegBeforeDispatch(nullptr);
    EXPECT_GT(msgCount, 0);
}
}