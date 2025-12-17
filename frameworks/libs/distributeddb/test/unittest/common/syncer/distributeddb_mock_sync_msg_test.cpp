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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#ifdef RUN_AS_ROOT
#include <sys/time.h>
#endif
#include <thread>

#include "ability_sync.h"
#include "distributeddb_tools_unit_test.h"
#include "mock_communicator.h"
#include "mock_kv_sync_interface.h"
#include "mock_meta_data.h"
#include "mock_single_ver_data_sync.h"
#include "mock_single_ver_sync_engine.h"
#include "mock_sync_task_context.h"
#include "virtual_relational_ver_sync_db_interface.h"
#include "virtual_single_ver_sync_db_Interface.h"

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBMockSyncMsgTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBMockSyncMsgTest::SetUpTestCase(void)
{
}

void DistributedDBMockSyncMsgTest::TearDownTestCase(void)
{
}

void DistributedDBMockSyncMsgTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBMockSyncMsgTest::TearDown(void)
{
}

/**
 * @tc.name: EnableClearRemoteStaleDataTest001
 * @tc.desc: Test EnableClearRemoteStaleData when syncTaskContextMap_ is not empty and context != nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, EnableClearRemoteStaleDataTest001, TestSize.Level0)
{
    auto enginePtr = std::make_unique<MockSingleVerSyncEngine>();
    ASSERT_NE(enginePtr, nullptr);
    EXPECT_CALL(*enginePtr, CreateSyncTaskContext(_))
        .WillOnce(Return(new (std::nothrow) SingleVerKvSyncTaskContext()));
    
    MockKvSyncInterface syncInterface;
    auto *context = enginePtr->CreateSyncTaskContext(syncInterface);
    ASSERT_NE(context, nullptr);

    DeviceSyncTarget devTarget("dev", "user");
    std::map<DeviceSyncTarget, ISyncTaskContext *> syncTaskContextMap = {{devTarget, context}};
    enginePtr->SetSyncTaskContextMap(syncTaskContextMap);
    enginePtr->EnableClearRemoteStaleData(true);
    enginePtr->Close();
}

/**
 * @tc.name: EnableClearRemoteStaleDataTest002
 * @tc.desc: Test EnableClearRemoteStaleData when syncTaskContextMap_ is not empty and context == nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, EnableClearRemoteStaleDataTest002, TestSize.Level0)
{
    auto enginePtr = std::make_unique<MockSingleVerSyncEngine>();
    ASSERT_NE(enginePtr, nullptr);
    EXPECT_CALL(*enginePtr, CreateSyncTaskContext(_))
        .WillOnce(Return(nullptr));
    
    MockKvSyncInterface syncInterface;
    ISyncTaskContext *context = enginePtr->CreateSyncTaskContext(syncInterface);
    ASSERT_EQ(context, nullptr);

    DeviceSyncTarget devTarget("dev", "user");
    std::map<DeviceSyncTarget, ISyncTaskContext *> syncTaskContextMap = {{devTarget, context}};
    enginePtr->SetSyncTaskContextMap(syncTaskContextMap);
    enginePtr->EnableClearRemoteStaleData(true);
    enginePtr->Close();
}

/**
 * @tc.name: StartAutoSubscribeTimer
 * @tc.desc: Test StartAutoSubscribeTimer when subscribeTimerId_ > 0.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, StartAutoSubscribeTimer, TestSize.Level0)
{
    auto enginePtr = std::make_unique<MockSingleVerSyncEngine>();
    ASSERT_NE(enginePtr, nullptr);
    MockKvSyncInterface syncInterface;
    EXPECT_CALL(syncInterface, IsSupportSubscribe()).WillRepeatedly(Return(E_OK));

    EXPECT_EQ(enginePtr->StartAutoSubscribeTimer(syncInterface), E_OK);
    EXPECT_EQ(enginePtr->StartAutoSubscribeTimer(syncInterface), -E_INTERNAL_ERROR);

    enginePtr->Close();
}

/**
 * @tc.name: SubscribeTimeOutCallbackIsNull
 * @tc.desc: Test SubscribeTimeOut when Callback is Null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, SubscribeTimeOutCallbackIsNull, TestSize.Level0)
{
    std::unique_ptr<MockSingleVerSyncEngine> enginePtr = std::make_unique<MockSingleVerSyncEngine>();
    ISyncEngine::InitCallbackParam callbackParam = { nullptr, nullptr, nullptr };
    enginePtr->SetQueryAutoSyncCallbackNull(false, callbackParam);
    TimerId id = 0;
    EXPECT_EQ(enginePtr->SubscribeTimeOut(id), E_OK);
}

/**
 * @tc.name: SubscribeTimeOutCallbackIsNull
 * @tc.desc: Test when allqueries is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, SubscribeTimeOutAllQueriesEmpty, TestSize.Level0)
{
    std::unique_ptr<MockSingleVerSyncEngine> enginePtr = std::make_unique<MockSingleVerSyncEngine>();
    ISyncEngine::InitCallbackParam callbackParam = { nullptr, nullptr, nullptr };
    enginePtr->SetQueryAutoSyncCallbackNull(true, callbackParam);
    enginePtr->InitSubscribeManager();
    TimerId id = 0;
    std::map<std::string, std::vector<QuerySyncObject>> emptyQueries;
    enginePtr->GetAllUnFinishSubQueries(emptyQueries);

    ASSERT_TRUE(emptyQueries.empty());
    EXPECT_EQ(enginePtr->SubscribeTimeOut(id), E_OK);
}

/**
 * @tc.name: SubscribeTimeOutCallbackIsNull
 * @tc.desc: Test SubscribeTimeOut when Callback is Null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, SubscribeTimeOutNormal, TestSize.Level0)
{
    std::unique_ptr<MockSingleVerSyncEngine> enginePtr = std::make_unique<MockSingleVerSyncEngine>();
    ISyncEngine::InitCallbackParam callbackParam = { nullptr, nullptr, nullptr };

    enginePtr->InitSubscribeManager();
    enginePtr->SetQueryAutoSyncCallbackNull(true, callbackParam);
    
    SubscribeManager subManager;
    std::string device = "device_A";
    QuerySyncObject querySyncObj(Query::Select().PrefixKey({'a', static_cast<uint8_t>('a')}));
    ASSERT_TRUE(subManager.ReserveLocalSubscribeQuery(device, querySyncObj) == E_OK);
    ASSERT_TRUE(subManager.ActiveLocalSubscribeQuery(device, querySyncObj) == E_OK);

    std::vector<QuerySyncObject> subscribeQueries;
    subManager.GetLocalSubscribeQueries(device, subscribeQueries);
    EXPECT_EQ(subscribeQueries.size(), 1);

    enginePtr->PutUnfinishedSubQueries(device, subscribeQueries);
    TimerId id = 0;
    std::map<std::string, std::vector<QuerySyncObject>> Queries;
    enginePtr->GetAllUnFinishSubQueries(Queries);
    ASSERT_FALSE(Queries.empty());
    EXPECT_EQ(enginePtr->SubscribeTimeOut(id), E_OK);
}

/**
 * @tc.name: ControlCmdAckRecvSubManagerIsNull
 * @tc.desc: Test ControlCmdAckRecv when SubManager is Null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, ControlCmdAckRecvSubManagerIsNull, TestSize.Level0)
{
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;

    std::shared_ptr<SubscribeManager> subManager = nullptr;
    mockSyncTaskContext.SetSubscribeManager(subManager);
    EXPECT_TRUE(mockSyncTaskContext.GetSubscribeManager() == nullptr);

    int ret = mockDataSync.ControlCmdAckRecv(&mockSyncTaskContext, nullptr);
    EXPECT_EQ(ret, -E_INVALID_ARGS);
}

/**
 * @tc.name: ControlCmdAckRecvSubPacketIsNull
 * @tc.desc: Test ControlCmdAck when packet is Null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, ControlCmdAckRecvPacketIsNull, TestSize.Level0)
{
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;
    auto subManager = std::make_shared<SubscribeManager>();
    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    mockSyncTaskContext.SetSubscribeManager(subManager);
    
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->GetObject<SubscribeRequest>(), nullptr);
    EXPECT_NE(mockSyncTaskContext.GetSubscribeManager(), nullptr);
    int ret = mockDataSync.ControlCmdAckRecv(&mockSyncTaskContext, message);
    EXPECT_EQ(ret, -E_INVALID_ARGS);
    delete message;
}

/**
 * @tc.name: SendControlAckSendError
 * @tc.desc: Test SendControlAck when send() return Error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, SendControlAckSendError, TestSize.Level0)
{
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;
    VirtualRelationalVerSyncDBInterface storage;
    auto communicator = new (std::nothrow) MockCommunicator();
    ASSERT_NE(communicator, nullptr);
    EXPECT_CALL(*communicator, SendMessage(_,_,_,_)).WillOnce(Return(-E_NOT_INIT));

    auto mockMetadata = std::make_shared<MockMetadata>();
    std::shared_ptr<Metadata> metadata = std::static_pointer_cast<Metadata>(mockMetadata);
    mockDataSync.Initialize(&storage, communicator, metadata, "deviceId");
    
    DistributedDB::Message *msg = new (std::nothrow) DistributedDB::Message(TYPE_NOTIFY);
    ASSERT_NE(msg, nullptr);
    msg->SetMessageType(TYPE_NOTIFY);
    AbilitySyncAckPacket packet;
    msg->SetCopiedObject(packet);

    const SubscribeRequest *object = msg->GetObject<SubscribeRequest>();
    uint32_t controlCmdType = object->GetcontrolCmdType();

    int ret = mockDataSync.CallSendControlAck(&mockSyncTaskContext, msg, -E_NOT_REGISTER, controlCmdType);
    EXPECT_EQ(ret, -E_NOT_INIT);

    delete communicator;
    delete msg;
}

/**
 * @tc.name: TestControlCmdRequestRecvPre
 * @tc.desc: Test when packet is null or context->GetRemoteSoftwareVersion() <= SOFTWARE_VERSION_BASE.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, TestControlCmdRequestRecvPre, TestSize.Level0)
{
    /**
     * @tc.step: 1. init.
     */
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;
    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    ASSERT_NE(message, nullptr);
    /**
     * @tc.case: 1. when packet is null.
     */
    EXPECT_EQ(message->GetObject<SubscribeRequest>(), nullptr);
    EXPECT_EQ(mockDataSync.CallControlCmdRequestRecvPre(&mockSyncTaskContext, message), -E_INVALID_ARGS);
    /**
     * @tc.case: 2. RemoteSoftwareVersion() <= SOFTWARE_VERSION_BASE.
     */
    mockSyncTaskContext.SetRemoteSoftwareVersion(SOFTWARE_VERSION_BASE);
    message->SetMessageType(TYPE_NOTIFY);
    auto packet = new (std::nothrow) SubscribeRequest;
    packet->SetPacketHead(E_OK, SOFTWARE_VERSION_CURRENT, INVALID_CONTROL_CMD, 1);
    message->SetCopiedObject(*packet);
    EXPECT_NE(message->GetObject<SubscribeRequest>(), nullptr);
    auto communicator = new (std::nothrow) MockCommunicator();
    ASSERT_NE(communicator, nullptr);
    mockDataSync.SetCommunicatorHandle(communicator);
    VirtualRelationalVerSyncDBInterface storage;
    auto mockMetadata = std::make_shared<MockMetadata>();
    std::shared_ptr<Metadata> metadata = std::static_pointer_cast<Metadata>(mockMetadata);
    mockDataSync.Initialize(&storage, communicator, metadata, "deviceId");

    int ret1 = mockDataSync.CallControlCmdRequestRecvPre(&mockSyncTaskContext, message);
    int ret2 = mockDataSync.CallDoAbilitySyncIfNeed(&mockSyncTaskContext, message, true);
    EXPECT_EQ(ret1, ret2);
    /**
     * @tc.case: 3. controlCmdType >= ControlCmdType::INVALID_CONTROL_CMD.
     */
    EXPECT_CALL(*communicator, SendMessage(_, _, _, _)).WillOnce(Return(-E_NOT_INIT));
    mockSyncTaskContext.SetRemoteSoftwareVersion(SOFTWARE_VERSION_BASE + 1);
    EXPECT_EQ(mockDataSync.CallControlCmdRequestRecvPre(&mockSyncTaskContext, message), -E_WAIT_NEXT_MESSAGE);
    
    mockDataSync.SetCommunicatorHandle(nullptr);
    RefObject::KillAndDecObjRef(communicator);
    delete message;
    delete packet;
}

/**
 * @tc.name: TestSubscribeRequestRecvPre
 * @tc.desc: Test when controlCmdType != ControlCmdType::SUBSCRIBE_QUERY_CMD.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, TestSubscribeRequestRecvPre, TestSize.Level0)
{
    /**
     * @tc.step: 1. init.
     */
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;
    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    auto packet = new (std::nothrow) SubscribeRequest;
    EXPECT_NE(message, nullptr);
    EXPECT_NE(packet, nullptr);
    packet->SetPacketHead(E_OK, SOFTWARE_VERSION_CURRENT, INVALID_CONTROL_CMD, 1);
    message->SetCopiedObject(*packet);

    EXPECT_EQ(mockDataSync.CallSubscribeRequestRecvPre(&mockSyncTaskContext, packet, message), E_OK);
    mockDataSync.SetCommunicatorHandle(nullptr);
    delete message;
    delete packet;
}

/**
 * @tc.name: TestSubscribeRequestRecv
 * @tc.desc: Test when packet is null or submanage is null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, TestSubscribeRequestRecv, TestSize.Level0)
{
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;
    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    /**
     * @tc.case: 1. when packet is null.
     */
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->GetObject<SubscribeRequest>(), nullptr);
    EXPECT_EQ(mockDataSync.CallSubscribeRequestRecv(&mockSyncTaskContext, message), -E_INVALID_ARGS);
    /**
     * @tc.case: 2. subscribeManager == nullptr.
     */
    auto packet = new (std::nothrow) SubscribeRequest;
    packet->SetPacketHead(100, SOFTWARE_VERSION_CURRENT, INVALID_CONTROL_CMD, 1);
    message->SetCopiedObject(*packet);
    VirtualRelationalVerSyncDBInterface storage;
    auto communicator = new (std::nothrow) MockCommunicator();
    auto mockMetadata = std::make_shared<MockMetadata>();
    std::shared_ptr<Metadata> metadata = std::static_pointer_cast<Metadata>(mockMetadata);
    mockDataSync.Initialize(&storage, communicator, metadata, "deviceId");
    EXPECT_CALL(*communicator, SendMessage(_, _, _, _)).WillRepeatedly(Return(-E_NOT_INIT));
    EXPECT_EQ(mockDataSync.CallSubscribeRequestRecv(&mockSyncTaskContext, message), -E_INVALID_ARGS);
    /**
     * @tc.case: 2. storage_->AddSubscribe is not E_OK.
     */
    auto subManager = std::make_shared<SubscribeManager>();
    mockSyncTaskContext.SetSubscribeManager(subManager);
    EXPECT_NE(mockSyncTaskContext.GetSubscribeManager(), nullptr);
    EXPECT_EQ(mockDataSync.CallSubscribeRequestRecv(&mockSyncTaskContext, message), -E_NOT_SUPPORT);
    delete message;
    delete packet;
    delete communicator;
}

/**
 * @tc.name: UnsubscribeRequestRecvPacketIsNull
 * @tc.desc: Test when packet is null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, UnsubscribeRequestRecvPacketIsNull, TestSize.Level0)
{
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;
    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message(TYPE_NOTIFY);
    EXPECT_NE(message, nullptr);
    EXPECT_EQ(message->GetObject<SubscribeRequest>(), nullptr);
    int ret = mockDataSync.CallUnsubscribeRequestRecv(&mockSyncTaskContext, message);
    EXPECT_EQ(ret, -E_INVALID_ARGS);
    delete message;
}

/**
 * @tc.name: UnsubscribeRequestRecvManagerIsNull
 * @tc.desc: Test when manager is null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, UnsubscribeRequestRecvManagerIsNull, TestSize.Level1)
{
    MockSyncTaskContext *mockSyncTaskContext = new (std::nothrow) MockSyncTaskContext();
    EXPECT_NE(mockSyncTaskContext, nullptr);
    MockSingleVerDataSync mockDataSync;
    VirtualRelationalVerSyncDBInterface storage;
    auto communicator = new (std::nothrow) MockCommunicator();
    EXPECT_NE(communicator, nullptr);
    EXPECT_CALL(*communicator, SendMessage(_, _, _, _)).WillOnce(Return(-E_NOT_INIT));
    auto mockMetadata = std::make_shared<MockMetadata>();
    std::shared_ptr<Metadata> metadata = std::static_pointer_cast<Metadata>(mockMetadata);
    mockDataSync.Initialize(&storage, communicator, metadata, "deviceId");

    DistributedDB::Message *msg = new (std::nothrow) DistributedDB::Message(TYPE_NOTIFY);
    msg->SetMessageType(TYPE_NOTIFY);
    AbilitySyncAckPacket packet;
    msg->SetCopiedObject(packet);

    EXPECT_NE(msg->GetObject<SubscribeRequest>(), nullptr);
    EXPECT_EQ(mockSyncTaskContext->GetSubscribeManager(), nullptr);
    int ret = mockDataSync.CallUnsubscribeRequestRecv(mockSyncTaskContext, msg);
    EXPECT_EQ(ret, -E_INVALID_ARGS);
    delete mockSyncTaskContext;
    delete msg;
    delete communicator;
}

/**
 * @tc.name: QuerySyncCheckContextIsNull
 * @tc.desc: Test when context is null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBMockSyncMsgTest, QuerySyncCheckContextIsNull, TestSize.Level1)
{
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext *mockSyncTaskContext = nullptr;
    EXPECT_EQ(mockSyncTaskContext, nullptr);
    int ret = mockDataSync.CallQuerySyncCheck(mockSyncTaskContext);
    EXPECT_EQ(ret, -E_INVALID_ARGS);
}
}