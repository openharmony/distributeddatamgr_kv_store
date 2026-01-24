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

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBKVCompressTest : public KVGeneralUt {
public:
    void SetUp() override;
protected:
    void PrepareEnv(bool isNeedCompressOnSync);
    void TriggerPutAndSync();
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
};

void DistributedDBKVCompressTest::SetUp()
{
    KVGeneralUt::SetUp();
}

void DistributedDBKVCompressTest::PrepareEnv(bool isNeedCompressOnSync)
{
    KVGeneralUt::CloseAllDelegate();
    KvStoreNbDelegate::Option option;
    option.isNeedCompressOnSync = isNeedCompressOnSync;
    option.compressionRate = 100; // compress rate is 100
    SetOption(option);
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_B), E_OK);
}

void DistributedDBKVCompressTest::TriggerPutAndSync()
{
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value expectValue(DBConstant::MAX_VALUE_SIZE, 'v');
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockPush(storeInfo1, storeInfo2);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}

/**
 * @tc.name: SyncTest001
 * @tc.desc: Test sync with compress.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVCompressTest, SyncTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Open store with compress
     * @tc.expected: step1. Open ok
     */
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(true));
    /**
     * @tc.steps: step2. dev1 put (k,v) and sync to dev2
     * @tc.expected: step2. sync should return OK and dev2 exist (k,v).
     */
    auto size1 = GetAllSendMsgSize();
    ASSERT_NO_FATAL_FAILURE(TriggerPutAndSync());
    auto size2 = GetAllSendMsgSize();
    /**
     * @tc.steps: step3. Open store without compress
     * @tc.expected: step3. Open ok
     */
    ASSERT_NO_FATAL_FAILURE(PrepareEnv(false));
    /**
     * @tc.steps: step4. dev1 put (k,v) and sync to dev2
     * @tc.expected: step4. sync should return OK and dev2 exist (k,v).
     */
    auto size3 = GetAllSendMsgSize();
    ASSERT_NO_FATAL_FAILURE(TriggerPutAndSync());
    auto size4 = GetAllSendMsgSize();
    EXPECT_LE(size2 - size1, size4 - size3);
}
}
#endif