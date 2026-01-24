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
#include "process_communicator_test_stub.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBKvMultiUserSyncTest : public KVGeneralUt {
protected:
    void PrepareData();
    void SyncStep1();
    void SyncStep2();
    void SyncStep3();
    static void SetTargetUserId(const std::string &deviceId, const std::string &userId);
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    static constexpr const char *USER_ID_1 = "USER_ID_1";
    static constexpr const char *USER_ID_2 = "USER_ID_2";
    static constexpr const char *USER_ID_3 = "USER_ID_3";
};

void CheckData(KvStoreNbDelegate *delegate, const Key &key, const Value &expectValue)
{
    Value actualValue;
    if (expectValue.empty()) {
        EXPECT_EQ(delegate->Get(key, actualValue), NOT_FOUND);
    } else {
        EXPECT_EQ(delegate->Get(key, actualValue), OK);
        EXPECT_EQ(actualValue, expectValue);
    }
}

void DistributedDBKvMultiUserSyncTest::SetTargetUserId(const std::string &deviceId, const std::string &userId)
{
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    ASSERT_NE(communicatorAggregator, nullptr);
    auto virtualCommunicatorAggregator = static_cast<VirtualCommunicatorAggregator*>(communicatorAggregator);
    auto communicator = static_cast<VirtualCommunicator*>(virtualCommunicatorAggregator->GetCommunicator(deviceId));
    ASSERT_NE(communicator, nullptr);
    communicator->SetTargetUserId(userId);
}

void DistributedDBKvMultiUserSyncTest::PrepareData()
{
    KvStoreNbDelegate::Option option;
    option.syncDualTupleMode = true;
    SetOption(option);

    StoreInfo storeInfo1 = {USER_ID_1, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);

    StoreInfo storeInfo2 = {USER_ID_2, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_A), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);

    EXPECT_EQ(store1->Put(KEY_1, VALUE_1), OK);
    EXPECT_EQ(store1->Put(KEY_2, VALUE_2), OK);
    EXPECT_EQ(store1->Put(KEY_3, VALUE_3), OK);
    EXPECT_EQ(store2->Put(KEY_4, VALUE_4), OK);

    CheckData(store1, KEY_1, VALUE_1);
    CheckData(store1, KEY_2, VALUE_2);
    CheckData(store1, KEY_3, VALUE_3);
    CheckData(store2, KEY_4, VALUE_4);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo2), E_OK);
}

void DistributedDBKvMultiUserSyncTest::SyncStep1()
{
    StoreInfo storeInfo1 = {USER_ID_1, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);

    StoreInfo storeInfo3 = {USER_ID_1, STORE_ID_2, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo3, DEVICE_B), E_OK);
    auto store3 = GetDelegate(storeInfo3);
    ASSERT_NE(store3, nullptr);

    SetTargetUserId(DEVICE_A, USER_ID_1);
    SetTargetUserId(DEVICE_B, USER_ID_1);
    BlockPush(storeInfo1, storeInfo3);

    CheckData(store3, KEY_1, VALUE_1);
    CheckData(store3, KEY_2, VALUE_2);
    CheckData(store3, KEY_3, VALUE_3);

    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo3), E_OK);
}

void DistributedDBKvMultiUserSyncTest::SyncStep2()
{
    StoreInfo storeInfo2 = {USER_ID_2, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_A), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);

    StoreInfo storeInfo4 = {USER_ID_2, STORE_ID_2, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo4, DEVICE_B), E_OK);
    auto store4 = GetDelegate(storeInfo4);
    ASSERT_NE(store4, nullptr);

    SetTargetUserId(DEVICE_A, USER_ID_2);
    SetTargetUserId(DEVICE_B, USER_ID_2);
    BlockPush(storeInfo2, storeInfo4);

    CheckData(store4, KEY_1, {});
    CheckData(store4, KEY_2, {});
    CheckData(store4, KEY_3, {});
    CheckData(store4, KEY_4, VALUE_4);

    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo2), E_OK);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo4), E_OK);
}

void DistributedDBKvMultiUserSyncTest::SyncStep3()
{
    StoreInfo storeInfo1 = {USER_ID_1, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);

    StoreInfo storeInfo4 = {USER_ID_2, STORE_ID_2, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo4, DEVICE_B), E_OK);
    auto store4 = GetDelegate(storeInfo4);
    ASSERT_NE(store4, nullptr);

    SetTargetUserId(DEVICE_A, USER_ID_2);
    SetTargetUserId(DEVICE_B, USER_ID_1);
    BlockPush(storeInfo1, storeInfo4);

    CheckData(store4, KEY_1, VALUE_1);
    CheckData(store4, KEY_2, VALUE_2);
    CheckData(store4, KEY_3, VALUE_3);
    CheckData(store4, KEY_4, VALUE_4);

    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo4), E_OK);
}

/**
 * @tc.name: NormalSyncTest001
 * @tc.desc: Test normal sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBKvMultiUserSyncTest, NormalSyncTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. (devA, user1) put 3 records, (devA, user2) put 1 record
     * @tc.expected: step1. OK.
     */
    PrepareData();
    /**
     * @tc.steps: step2. (devA, user1) sync to (devB, user1)
     * @tc.expected: step2. OK.
     */
    SyncStep1();
    /**
     * @tc.steps: step3. (devA, user2) put 3 records, (devB, user2) put 1 record
     * @tc.expected: step3. OK.
     */
    SyncStep2();
    /**
     * @tc.steps: step4. (devA, user1) put 3 records, (devB, user2) put 1 record
     * @tc.expected: step4. OK.
     */
    SyncStep3();
}

/**
 * @tc.name: NormalSyncTest002
 * @tc.desc: Test sync with low version target.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBKvMultiUserSyncTest, NormalSyncTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init process communicator
     * @tc.expected: step1. OK.
     */
    KvStoreNbDelegate::Option option;
    option.syncDualTupleMode = true;
    SetOption(option);
    std::shared_ptr<ProcessCommunicatorTestStub> processCommunicator =
        std::make_shared<ProcessCommunicatorTestStub>();
    processCommunicator->SetDataHeadInfo({DBStatus::LOW_VERSION_TARGET, 0u});
    SetProcessCommunicator(processCommunicator);

    /**
     * @tc.steps: step2. (devA, user1) put 1 record and sync to (devB, user2)
     * @tc.expected: step2. OK.
     */
    StoreInfo storeInfo1 = {USER_ID_1, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1->Put(KEY_1, VALUE_1), OK);

    StoreInfo storeInfo2 = {USER_ID_2, STORE_ID_2, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_B), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);

    SetTargetUserId(DEVICE_A, USER_ID_2);
    SetTargetUserId(DEVICE_B, USER_ID_1);
    UserInfo userInfo = {.receiveUser = USER_ID_2, .sendUser = USER_ID_1};
    processCommunicator->SetDataUserInfo({{userInfo}});
    BlockPush(storeInfo1, storeInfo2);

    CheckData(store2, KEY_1, VALUE_1);

    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo2), E_OK);

    /**
     * @tc.steps: step3. (devA, user1) put 1 record and sync to (devB, user3)
     * @tc.expected: step3. OK.
     */
    StoreInfo storeInfo3 = {USER_ID_3, STORE_ID_2, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo3, DEVICE_B), E_OK);
    auto store3 = GetDelegate(storeInfo3);
    ASSERT_NE(store3, nullptr);

    EXPECT_EQ(store1->Put(KEY_2, VALUE_2), OK);

    SetTargetUserId(DEVICE_A, USER_ID_3);
    SetTargetUserId(DEVICE_B, USER_ID_1);
    userInfo = {.receiveUser = USER_ID_3, .sendUser = USER_ID_1};
    processCommunicator->SetDataUserInfo({{userInfo}});
    BlockPush(storeInfo1, storeInfo3);

    CheckData(store3, KEY_2, VALUE_2);

    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo3), E_OK);
}

/**
 * @tc.name: NormalSyncTest003
 * @tc.desc: deviceA:user1 put 2 records and sync to deviceB, deviceA:user1 delete 1 records, deviceA:user2 put
 * 2 records and sync to deviceB, deviceA:user1 sync to deviceB.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBKvMultiUserSyncTest, NormalSyncTest003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set option
     * @tc.expected: step1. OK.
     */
    KvStoreNbDelegate::Option option;
    option.syncDualTupleMode = true;
    option.conflictResolvePolicy = DEVICE_COLLABORATION;
    SetOption(option);

    /**
     * @tc.steps: step2. deviceA:user1 put 10 records and sync to deviceB, deviceA:user1 delete 1 records
     * @tc.expected: step2. OK.
     */
    StoreInfo storeInfo1 = {USER_ID_1, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1->Put(KEY_1, VALUE_1), OK);
    EXPECT_EQ(store1->Put(KEY_2, VALUE_2), OK);

    StoreInfo storeInfo3 = {USER_ID_3, STORE_ID_2, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo3, DEVICE_B), E_OK);
    auto store3 = GetDelegate(storeInfo3);
    ASSERT_NE(store3, nullptr);

    SetTargetUserId(DEVICE_A, USER_ID_3);
    SetTargetUserId(DEVICE_B, USER_ID_1);
    BlockPush(storeInfo1, storeInfo3);
    EXPECT_EQ(store1->Delete(KEY_1), OK);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);

    /**
     * @tc.steps: step3. deviceA:user2 put 2 records and sync to deviceB
     * @tc.expected: step3. OK.
     */
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);

    StoreInfo storeInfo2 = {USER_ID_2, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_A), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2->Put(KEY_1, VALUE_1), OK);
    EXPECT_EQ(store2->Put(KEY_2, VALUE_2), OK);

    SetTargetUserId(DEVICE_A, USER_ID_3);
    SetTargetUserId(DEVICE_B, USER_ID_2);
    BlockPush(storeInfo2, storeInfo3);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo2), E_OK);

    /**
     * @tc.steps: step4. deviceA:user1 sync to deviceB.
     * @tc.expected: step4. OK.
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    BlockPush(storeInfo1, storeInfo3);
    Value actualValue;
    EXPECT_EQ(store3->Get(KEY_1, actualValue), NOT_FOUND);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo3), E_OK);
}

/**
 * @tc.name: InvalidSync001
 * @tc.desc: Test sync with empty target user.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBKvMultiUserSyncTest, InvalidSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. (devA, user1) put 3 records, (devA, user2) put 1 record
     * @tc.expected: step1. OK.
     */
    KvStoreNbDelegate::Option option;
    option.syncDualTupleMode = true;
    SetOption(option);
    StoreInfo storeInfo1 = {USER_ID_1, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);

    StoreInfo storeInfo2 = {USER_ID_2, STORE_ID_1, APP_ID};
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_B), E_OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);

    EXPECT_EQ(store1->Put(KEY_1, VALUE_1), OK);
    EXPECT_EQ(store1->Put(KEY_2, VALUE_2), OK);
    EXPECT_EQ(store1->Put(KEY_3, VALUE_3), OK);
    EXPECT_EQ(store2->Put(KEY_4, VALUE_4), OK);

    CheckData(store1, KEY_1, VALUE_1);
    CheckData(store1, KEY_2, VALUE_2);
    CheckData(store1, KEY_3, VALUE_3);
    CheckData(store2, KEY_4, VALUE_4);
    /**
     * @tc.steps: step2. set empty target user and sync
     * @tc.expected: step2. return OK.
     */
    SetTargetUserId(DEVICE_A, "");
    BlockPush(storeInfo1, storeInfo2);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo2), E_OK);
}
}
#endif