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
#include "mock_single_ver_kv_syncer.h"
#include "mock_sync_engine.h"

namespace DistributedDB {
using namespace testing::ext;
class DistributedDBKVNotifyTest : public KVGeneralUt {
public:
    void SetUp() override;
protected:
    void NotifySyncTest(SyncMode mode, int expectCount);
};

void DistributedDBKVNotifyTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, "dev1"), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, "dev2"), E_OK);
}

void DistributedDBKVNotifyTest::NotifySyncTest(SyncMode mode, int expectCount)
{
    /**
     * @tc.steps: step1. store1 register pull notify
     * @tc.expected: step1. register ok.
     */
    auto storeInfo1 = GetStoreInfo1();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    std::atomic<int> count = 0;
    int ret = store1->SetDeviceSyncNotify(DeviceSyncEvent::REMOTE_PULL_STARTED,
        [&count](const DeviceSyncNotifyInfo &info) {
        EXPECT_EQ(info.deviceId, "dev2");
        count++;
    });
    ASSERT_EQ(ret, OK);
    /**
     * @tc.steps: step2. store2 pull store1
     * @tc.expected: step2. pull ok and notify was triggered.
     */
    auto storeInfo2 = GetStoreInfo2();
    BlockDeviceSync(storeInfo2, storeInfo1, mode, OK);
    EXPECT_EQ(count, expectCount);
}

#ifdef USE_DISTRIBUTEDDB_DEVICE
/**
 * @tc.name: NotifySync001
 * @tc.desc: Test notify when pull sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVNotifyTest, NotifySync001, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(NotifySyncTest(SyncMode::SYNC_MODE_PULL_ONLY, 1));
}

/**
 * @tc.name: NotifySync002
 * @tc.desc: Test notify when push pull sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVNotifyTest, NotifySync002, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(NotifySyncTest(SyncMode::SYNC_MODE_PUSH_PULL, 1));
}

/**
 * @tc.name: NotifySync003
 * @tc.desc: Test notify when push sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVNotifyTest, NotifySync003, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(NotifySyncTest(SyncMode::SYNC_MODE_PUSH_ONLY, 0));
}

/**
 * @tc.name: NotifySync004
 * @tc.desc: Test cancel notify when pull sync.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVNotifyTest, NotifySync004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. store1 register pull notify
     * @tc.expected: step1. register ok.
     */
    auto storeInfo = GetStoreInfo1();
    auto store = GetDelegate(storeInfo);
    ASSERT_NE(store, nullptr);
    std::atomic<int> count = 0;
    int ret = store->SetDeviceSyncNotify(DeviceSyncEvent::REMOTE_PULL_STARTED,
        [&count](const DeviceSyncNotifyInfo &info) {
          EXPECT_EQ(info.deviceId, "dev2");
          count++;
        });
    ASSERT_EQ(ret, OK);
    /**
     * @tc.steps: step2. store2 pull store
     * @tc.expected: step2. pull ok and notify was triggered.
     */
    auto storeInfo2 = GetStoreInfo2();
    BlockDeviceSync(storeInfo2, storeInfo, SyncMode::SYNC_MODE_PULL_ONLY, OK);
    EXPECT_EQ(count, 1);
    /**
     * @tc.steps: step3. store2 pull store after store cancel notify
     * @tc.expected: step3. pull ok and notify was not triggered.
     */
    count = 0;
    ASSERT_EQ(store->SetDeviceSyncNotify(DeviceSyncEvent::REMOTE_PULL_STARTED, nullptr), OK);
    BlockDeviceSync(storeInfo2, storeInfo, SyncMode::SYNC_MODE_PULL_ONLY, OK);
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: NotifySync005
 * @tc.desc: Test GenericSyncer::SetDeviceSyncNotify when event != REMOTE_PULL_STARTED and not init.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBKVNotifyTest, NotifySync005, TestSize.Level0)
{
    MockSingleVerKVSyncer syncer;
    ASSERT_EQ(syncer.SetDeviceSyncNotify(static_cast<DeviceSyncEvent>(1), nullptr), -E_INVALID_ARGS);
    ASSERT_EQ(syncer.SetDeviceSyncNotify(DeviceSyncEvent::REMOTE_PULL_STARTED, nullptr), -E_NOT_INIT);
}
#endif

/**
 * @tc.name: GetExtendInfo001
 * @tc.desc: Test when syncInterface_ is null.
 * @tc.type: FUNC
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBKVNotifyTest, GetExtendInfo001, TestSize.Level0)
{
    MockSyncEngine mockSyncEngine;
    EXPECT_EQ(mockSyncEngine.GetSyncInterface(), nullptr);
    ExtendInfo extendInfo = mockSyncEngine.CallGetExtendInfo();
    EXPECT_EQ(extendInfo.appId, "");
}
}