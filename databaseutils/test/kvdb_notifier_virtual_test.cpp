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
#define LOG_TAG "KVDBNotifierVirtualTest"

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "kvdb_notifier_client.h"
#include "kvdb_sync_callback.h"
#include "kv_store_observer.h"
#include "kvdb_notifier_stub.h"
#include "message_parcel.h"
#include "types.h"
#include "dev_manager.h"
#include "dds_trace.h"
#include "store_util.h"

using namespace OHOS::DistributedKv;
using namespace testing;
using namespace testing::ext;
namespace OHOS::Test {
class KVDBNotifierClientVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KVDBNotifierClientVirtualTest::SetUpTestCase(void)
{}

void KVDBNotifierClientVirtualTest::TearDownTestCase(void)
{}

void KVDBNotifierClientVirtualTest::SetUp(void)
{}

void KVDBNotifierClientVirtualTest::TearDown(void)
{}

class MockDevManager : public DevManager {
public:
    string ToUUID(const string &device) override {
        if (device == "device1") {
            return "uuid1";
        }
        return "";
    }
};

class MockKvStoreSyncCallback : public KvStoreSyncCallback {
public:
    void SyncCompleted(const map<string, Status> &results) override {
        syncCompletedCalled = true;
    }
    void SyncCompleted(const map<string, Status> &results, uint64_t sequenceId) override {
        syncCompletedWithIdCalled = true;
    }
    bool syncCompletedCalled = false;
    bool syncCompletedWithIdCalled = false;
};

class MockKvStoreObserver : public KvStoreObserver {
public:
    void OnSwitchChange(const SwitchNotification &notification) override {
        onSwitchChangeCalled = true;
    }
    bool onSwitchChangeCalled = false;
};

class MockAsyncDetail : public AsyncDetail {
public:
    void operator()(ProgressDetail &&detail) override {
        asyncDetailCalled = true;
    }
    bool asyncDetailCalled = false;
};

class KVDBNotifierClientTest : public testing::Test {
protected:
    void SetUp() override {
        devManager = make_shared<MockDevManager>();
        notifierClient = make_unique<KVDBNotifierClient>();
    }

    shared_ptr<MockDevManager> devManager;
    unique_ptr<KVDBNotifierClient> notifierClient;
};

class KVDBNotifierStubVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KVDBNotifierStubVirtualTest::SetUpTestCase(void)
{}

void KVDBNotifierStubVirtualTest::TearDownTestCase(void)
{}

void KVDBNotifierStubVirtualTest::SetUp(void)
{}

void KVDBNotifierStubVirtualTest::TearDown(void)
{}

/**
 * @tc.name: SyncCompleted_CallbackFound_CallbackInvoked
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, SyncCompleted_CallbackFound_CallbackInvoked, TestSize.Level0)
{
    ZLOGI("SyncCompleted_CallbackFound_CallbackInvoked begin.");
    uint64_t sequenceId = 1;
    auto callback = make_shared<MockKvStoreSyncCallback>();
    notifierClient->AddSyncCallback(callback, sequenceId);

    notifierClient->SyncCompleted({}, sequenceId);

    EXPECT_TRUE(callback->syncCompletedCalled);
    EXPECT_TRUE(callback->syncCompletedWithIdCalled);
}

/**
 * @tc.name: SyncCompleted_CallbackNotFound_NoInvocation
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, SyncCompleted_CallbackNotFound_NoInvocation, TestSize.Level0)
{
    ZLOGI("SyncCompleted_CallbackNotFound_NoInvocation begin.");
    uint64_t sequenceId = 1;
    notifierClient->SyncCompleted({}, sequenceId);
    // No callback added, so no invocation should happen
}

/**
 * @tc.name: OnRemoteChange_DeviceToUUID_RemoteUpdated
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, OnRemoteChange_DeviceToUUID_RemoteUpdated, TestSize.Level0)
{
    ZLOGI("OnRemoteChange_DeviceToUUID_RemoteUpdated begin.");
    map<string, bool> mask = {{"device1", true}};
    notifierClient->OnRemoteChange(mask, static_cast<int32_t>(DataType::TYPE_STATICS));

    // Check if the remote is updated
    auto [exist, value] = notifierClient->remotes_.Find("uuid1");
    EXPECT_TRUE(exist);
    EXPECT_TRUE(value.first);
}

/**
 * @tc.name: IsChanged_DeviceExists_ReturnsCorrectStatus
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, IsChanged_DeviceExists_ReturnsCorrectStatus, TestSize.Level0)
{
    ZLOGI("IsChanged_DeviceExists_ReturnsCorrectStatus begin.");
    notifierClient->remotes_.InsertOrAssign("uuid1", make_pair(true, false));

    EXPECT_TRUE(notifierClient->IsChanged("uuid1", DataType::TYPE_STATICS));
    EXPECT_FALSE(notifierClient->IsChanged("uuid1", DataType::TYPE_DYNAMICAL));
}

/**
 * @tc.name: AddSyncCallback_NullCallback_NoAddition
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, AddSyncCallback_NullCallback_NoAddition, TestSize.Level0)
{
    ZLOGI("AddSyncCallback_NullCallback_NoAddition begin.");
    uint64_t sequenceId = 1;
    notifierClient->AddSyncCallback(nullptr, sequenceId);

    // No callback should be added
    EXPECT_FALSE(notifierClient->syncCallbackInfo_.Find(sequenceId).first);
}

/**
 * @tc.name: AddCloudSyncCallback_NullCallback_NoAddition
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, AddCloudSyncCallback_NullCallback_NoAddition, TestSize.Level0)
{
    ZLOGI("AddCloudSyncCallback_NullCallback_NoAddition begin.");
    uint64_t sequenceId = 1;
    notifierClient->AddCloudSyncCallback(sequenceId, nullptr);

    // No callback should be added
    EXPECT_FALSE(notifierClient->cloudSyncCallbacks_.Find(sequenceId).first);
}

/**
 * @tc.name: AddSwitchCallback_NullObserver_NoAddition
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, AddSwitchCallback_NullObserver_NoAddition, TestSize.Level0)
{
    ZLOGI("AddSwitchCallback_NullObserver_NoAddition begin.");
    notifierClient->AddSwitchCallback("appId", nullptr);

    // No observer should be added
    EXPECT_EQ(notifierClient->switchObservers_.Size(), 0);
}

/**
 * @tc.name: AddSwitchCallback_DuplicateObserver_NoAddition
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, AddSwitchCallback_DuplicateObserver_NoAddition, TestSize.Level0)
{
    ZLOGI("AddSwitchCallback_DuplicateObserver_NoAddition begin.");
    auto observer = make_shared<MockKvStoreObserver>();
    notifierClient->AddSwitchCallback("appId", observer);
    notifierClient->AddSwitchCallback("appId", observer);

    // Only one observer should be added
    EXPECT_EQ(notifierClient->switchObservers_.Size(), 1);
}

/**
 * @tc.name: OnSwitchChange_ObserversNotified
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierClientVirtualTest, OnSwitchChange_ObserversNotified, TestSize.Level0)
{
    ZLOGI("OnSwitchChange_ObserversNotified begin.");
    auto observer = make_shared<MockKvStoreObserver>();
    notifierClient->AddSwitchCallback("appId", observer);

    SwitchNotification notification;
    notifierClient->OnSwitchChange(notification);

    EXPECT_TRUE(observer->onSwitchChangeCalled);
}

/**
 * @tc.name: OnRemoteRequest_ValidCodeAndDescriptor_ShouldInvokeHandler
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierStubVirtualTest, OnRemoteRequest_ValidCodeAndDescriptor_ShouldInvokeHandler, TestSize.Level0)
{
    ZLOGI("OnRemoteRequest_ValidCodeAndDescriptor_ShouldInvokeHandler begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(KVDBNotifierStub::GetDescriptor());
    data.WriteInt32(1); // 假设这是有效的数据

    EXPECT_CALL(*stub, OnSyncCompleted(testing::Ref(data), testing::_)).WillOnce(testing::Return(ERR_NONE));

    int32_t result = stub->OnRemoteRequest(static_cast<uint32_t>(KVDBNotifierCode::SYNC_COMPLETED), data, reply, option);
    EXPECT_EQ(result, ERR_NONE);
}

/**
 * @tc.name: OnRemoteRequest_InvalidDescriptor_ShouldReturnError
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierStubVirtualTest, OnRemoteRequest_InvalidDescriptor_ShouldReturnError, TestSize.Level0)
{
    ZLOGI("OnRemoteRequest_InvalidDescriptor_ShouldReturnError begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(u"InvalidDescriptor");

    int32_t result = stub->OnRemoteRequest(static_cast<uint32_t>(KVDBNotifierCode::SYNC_COMPLETED), data, reply, option);
    EXPECT_EQ(result, -1);
}

/**
 * @tc.name: OnRemoteRequest_CodeOutOfBound_ShouldReturnError
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierStubVirtualTest, OnRemoteRequest_CodeOutOfBound_ShouldReturnError, TestSize.Level0)
{
    ZLOGI("OnRemoteRequest_CodeOutOfBound_ShouldReturnError begin.");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(KVDBNotifierStub::GetDescriptor());

    int32_t result = stub->OnRemoteRequest(static_cast<uint32_t>(KVDBNotifierCode::TRANS_BUTT), data, reply, option);
    EXPECT_EQ(result, IPCObjectStub::OnRemoteRequest(static_cast<uint32_t>(KVDBNotifierCode::TRANS_BUTT), data, reply, option));
}

/**
 * @tc.name: OnSyncCompleted_ValidData_ShouldInvokeSyncCompleted
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierStubVirtualTest, OnSyncCompleted_ValidData_ShouldInvokeSyncCompleted, TestSize.Level0)
{
    ZLOGI("OnSyncCompleted_ValidData_ShouldInvokeSyncCompleted begin.");
    MessageParcel data;
    MessageParcel reply;
    std::map<std::string, Status> results = {{"key", Status::SUCCESS}};
    uint64_t sequenceId = 12345;

    EXPECT_CALL(ITypesUtil, Unmarshal(testing::Ref(data), testing::Ref(results), testing::Ref(sequenceId)))
        .WillOnce(testing::Return(true));

    EXPECT_CALL(*stub, SyncCompleted(testing::_, sequenceId)).Times(1);

    int32_t result = stub->OnSyncCompleted(data, reply);
    EXPECT_EQ(result, ERR_NONE);
}

/**
 * @tc.name: OnSyncCompleted_InvalidData_ShouldReturnError
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: sql
 */
HWTEST_F(KVDBNotifierStubVirtualTest, OnSyncCompleted_InvalidData_ShouldReturnError, TestSize.Level0)
{
    ZLOGI("OnSyncCompleted_InvalidData_ShouldReturnError begin.");
    MessageParcel data;
    MessageParcel reply;
    std::map<std::string, Status> results;
    uint64_t sequenceId = 0;

    EXPECT_CALL(ITypesUtil, Unmarshal(testing::Ref(data), testing::Ref(results), testing::Ref(sequenceId)))
        .WillOnce(testing::Return(false));

    int32_t result = stub->OnSyncCompleted(data, reply);
    EXPECT_EQ(result, IPC_STUB_INVALID_DATA_ERR);
}
} // namespace OHOS::Test