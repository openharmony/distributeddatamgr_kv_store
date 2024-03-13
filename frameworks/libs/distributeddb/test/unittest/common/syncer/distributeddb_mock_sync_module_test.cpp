/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "db_common.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "message.h"
#include "mock_auto_launch.h"
#include "mock_communicator.h"
#include "mock_kv_sync_interface.h"
#include "mock_meta_data.h"
#include "mock_remote_executor.h"
#include "mock_single_ver_data_sync.h"
#include "mock_single_ver_kv_syncer.h"
#include "mock_single_ver_state_machine.h"
#include "mock_sync_engine.h"
#include "mock_sync_task_context.h"
#include "mock_time_sync.h"
#include "remote_executor_packet.h"
#include "single_ver_data_sync_utils.h"
#include "single_ver_kv_syncer.h"
#include "single_ver_relational_sync_task_context.h"
#include "virtual_communicator_aggregator.h"
#include "virtual_single_ver_sync_db_Interface.h"
#ifdef DATA_SYNC_CHECK_003
#include "virtual_relational_ver_sync_db_interface.h"
#endif

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
class TestKvDb {
public:
    ~TestKvDb()
    {
        LOGI("~TestKvDb");
    }
    void Initialize(ISyncInterface *syncInterface)
    {
        syncer_.Initialize(syncInterface, true);
        syncer_.EnableAutoSync(true);
    }
    void LocalChange()
    {
        syncer_.LocalDataChanged(static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
    }
    int Close()
    {
        return syncer_.Close(true);
    }
private:
    SyncerProxy syncer_;
};
class TestInterface : public TestKvDb, public VirtualSingleVerSyncDBInterface, public RefObject {
public:
    TestInterface() {}
    ~TestInterface()
    {
        TestKvDb::Close();
    }
    void Initialize()
    {
        TestKvDb::Initialize(this);
    }
    void TestLocalChange()
    {
        TestKvDb::LocalChange();
    }
    void TestSetIdentifier(std::vector<uint8_t> &identifierVec)
    {
        VirtualSingleVerSyncDBInterface::SetIdentifier(identifierVec);
    }

    void IncRefCount() override
    {
        RefObject::IncObjRef(this);
    }

    void DecRefCount() override
    {
        RefObject::DecObjRef(this);
    }
};

namespace {
using State = SingleVerSyncStateMachine::State;
const uint32_t MESSAGE_COUNT = 10u;
const uint32_t EXECUTE_COUNT = 2u;
void Init(MockSingleVerStateMachine &stateMachine, MockSyncTaskContext &syncTaskContext, MockCommunicator &communicator,
    VirtualSingleVerSyncDBInterface &dbSyncInterface)
{
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    ASSERT_EQ(metadata->Initialize(&dbSyncInterface), E_OK);
    (void)syncTaskContext.Initialize("device", &dbSyncInterface, metadata, &communicator);
    (void)stateMachine.Initialize(&syncTaskContext, &dbSyncInterface, metadata, &communicator);
}

void Init(MockSingleVerStateMachine &stateMachine, MockSyncTaskContext *syncTaskContext,
    MockCommunicator &communicator, VirtualSingleVerSyncDBInterface *dbSyncInterface)
{
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    ASSERT_EQ(metadata->Initialize(dbSyncInterface), E_OK);
    (void)syncTaskContext->Initialize("device", dbSyncInterface, metadata, &communicator);
    (void)stateMachine.Initialize(syncTaskContext, dbSyncInterface, metadata, &communicator);
}

#ifdef RUN_AS_ROOT
void ChangeTime(int sec)
{
    timeval time;
    gettimeofday(&time, nullptr);
    time.tv_sec += sec;
    settimeofday(&time, nullptr);
}
#endif

int BuildRemoteQueryMsg(DistributedDB::Message *&message)
{
    auto packet = RemoteExecutorRequestPacket::Create();
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    message = new (std::nothrow) DistributedDB::Message(static_cast<uint32_t>(MessageId::REMOTE_EXECUTE_MESSAGE));
    if (message == nullptr) {
        RemoteExecutorRequestPacket::Release(packet);
        return -E_OUT_OF_MEMORY;
    }
    message->SetMessageType(TYPE_REQUEST);
    packet->SetNeedResponse();
    message->SetExternalObject(packet);
    return E_OK;
}

void ConstructPacel(Parcel &parcel, uint32_t conditionCount, const std::string &key, const std::string &value)
{
    parcel.WriteUInt32(RemoteExecutorRequestPacket::REQUEST_PACKET_VERSION_V2); // version
    parcel.WriteUInt32(1); // flag
    parcel.WriteInt(1); // current_version
    parcel.WriteInt(1); // opcode
    parcel.WriteString("sql"); // sql
    parcel.WriteInt(1); // bandArgs_
    parcel.WriteString("condition");
    parcel.EightByteAlign();

    parcel.WriteUInt32(conditionCount);
    if (key.empty()) {
        return;
    }
    parcel.WriteString(key);
    parcel.WriteString(value);
}

void StateMachineCheck013()
{
    MockSingleVerStateMachine stateMachine;
    auto *syncTaskContext = new (std::nothrow) MockSyncTaskContext();
    auto *dbSyncInterface = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_NE(syncTaskContext, nullptr);
    EXPECT_NE(dbSyncInterface, nullptr);
    if (dbSyncInterface == nullptr) {
        RefObject::KillAndDecObjRef(syncTaskContext);
        return;
    }
    MockCommunicator communicator;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    int count = 0;
    EXPECT_CALL(*syncTaskContext, Clear()).WillRepeatedly([&count]() {
        count++;
    });
    syncTaskContext->RegForkGetDeviceIdFunc([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep 2s
    });
    auto token = new VirtualContinueToken();
    syncTaskContext->SetContinueToken(static_cast<void *>(token));
    RefObject::KillAndDecObjRef(syncTaskContext);
    delete dbSyncInterface;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // sleep 5s and wait for task exist
    EXPECT_EQ(count, 1);
}

void AutoLaunchCheck001()
{
    MockAutoLaunch mockAutoLaunch;
    /**
     * @tc.steps: step1. put AutoLaunchItem in cache to simulate a connection was auto launched
     */
    std::string id = "TestAutoLaunch";
    std::string userId = "userId";
    AutoLaunchItem item;
    mockAutoLaunch.SetAutoLaunchItem(id, userId, item);
    EXPECT_CALL(mockAutoLaunch, TryCloseConnection(_)).WillOnce(Return());
    /**
     * @tc.steps: step2. send close signal to simulate a connection was unused in 1 min
     * @tc.expected: 10 thread try to close the connection and one thread close success
     */
    const int loopCount = 10;
    int finishCount = 0;
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::condition_variable cv;
    for (int i = 0; i < loopCount; i++) {
        std::thread t = std::thread([&finishCount, &mockAutoLaunch, &id, &userId, &mutex, &cv] {
            mockAutoLaunch.CallExtConnectionLifeCycleCallbackTask(id, userId);
            finishCount++;
            if (finishCount == loopCount) {
                std::unique_lock<std::mutex> lockInner(mutex);
                cv.notify_one();
            }
        });
        t.detach();
    }
    cv.wait(lock, [&finishCount, &loopCount]() { return finishCount == loopCount; });
}

void AbilitySync004()
{
    /**
     * @tc.steps: step1. set table TEST is permitSync
     */
    auto *context = new (std::nothrow) SingleVerKvSyncTaskContext();
    ASSERT_NE(context, nullptr);
    /**
     * @tc.steps: step2. test context recv dbAbility in diff thread
     */
    const int loopCount = 1000;
    std::atomic<int> finishCount = 0;
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::condition_variable cv;
    for (int i = 0; i < loopCount; i++) {
        std::thread t = std::thread([&] {
            DbAbility dbAbility;
            context->SetDbAbility(dbAbility);
            finishCount++;
            if (finishCount == loopCount) {
                cv.notify_one();
            }
        });
        t.detach();
    }
    cv.wait(lock, [&]() { return finishCount == loopCount; });
    EXPECT_EQ(context->GetRemoteCompressAlgoStr(), "none");
    RefObject::KillAndDecObjRef(context);
}

void SyncLifeTest001()
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(virtualCommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    VirtualSingleVerSyncDBInterface *syncDBInterface = new VirtualSingleVerSyncDBInterface();
    ASSERT_NE(syncDBInterface, nullptr);
    EXPECT_EQ(syncer->Initialize(syncDBInterface, true), -E_INVALID_ARGS);
    syncer->EnableAutoSync(true);
    for (int i = 0; i < 1000; i++) { // trigger 1000 times auto sync check
        syncer->LocalDataChanged(static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
    }
    EXPECT_EQ(virtualCommunicatorAggregator->GetOnlineDevices().size(), 0u);
    syncer = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete syncDBInterface;
}

void SyncLifeTest002()
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(virtualCommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    const std::string DEVICE_B = "deviceB";
    VirtualSingleVerSyncDBInterface *syncDBInterface = new VirtualSingleVerSyncDBInterface();
    ASSERT_NE(syncDBInterface, nullptr);
    std::string userId = "userid_0";
    std::string storeId = "storeId_0";
    std::string appId = "appid_0";
    std::string identifier = KvStoreDelegateManager::GetKvStoreIdentifier(userId, appId, storeId);
    std::vector<uint8_t> identifierVec(identifier.begin(), identifier.end());
    syncDBInterface->SetIdentifier(identifierVec);
    for (int i = 0; i < 100; i++) { // run 100 times
        EXPECT_EQ(syncer->Initialize(syncDBInterface, true), E_OK);
        syncer->EnableAutoSync(true);
        virtualCommunicatorAggregator->OnlineDevice(DEVICE_B);
        std::thread writeThread([syncer]() {
            syncer->LocalDataChanged(
                static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
        });
        std::thread closeThread([syncer, &syncDBInterface]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            EXPECT_EQ(syncer->Close(true), E_OK);
        });
        closeThread.join();
        writeThread.join();
    }
    syncer = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete syncDBInterface;
}

void SyncLifeTest003()
{
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(virtualCommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    TestInterface *syncDBInterface = new TestInterface();
    ASSERT_NE(syncDBInterface, nullptr);
    const std::string DEVICE_B = "deviceB";
    std::string userId = "userId_0";
    std::string storeId = "storeId_0";
    std::string appId = "appId_0";
    std::string identifier = KvStoreDelegateManager::GetKvStoreIdentifier(userId, appId, storeId);
    std::vector<uint8_t> identifierVec(identifier.begin(), identifier.end());
    syncDBInterface->TestSetIdentifier(identifierVec);
    syncDBInterface->Initialize();
    virtualCommunicatorAggregator->OnlineDevice(DEVICE_B);
    syncDBInterface->TestLocalChange();
    virtualCommunicatorAggregator->OfflineDevice(DEVICE_B);
    EXPECT_EQ(syncDBInterface->Close(), E_OK);
    RefObject::KillAndDecObjRef(syncDBInterface);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    RuntimeContext::GetInstance()->StopTaskPool();
}

void MockRemoteQuery002()
{
    MockRemoteExecutor *executor = new (std::nothrow) MockRemoteExecutor();
    ASSERT_NE(executor, nullptr);
    EXPECT_EQ(executor->CallResponseFailed(0, 0, 0, "DEVICE"), -E_BUSY);
    RefObject::KillAndDecObjRef(executor);
}

void SyncerCheck001()
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    EXPECT_EQ(syncer->SetSyncRetry(true), -E_NOT_INIT);
    syncer = nullptr;
}

void TimeSync001()
{
    auto *communicator = new(std::nothrow) MockCommunicator();
    ASSERT_NE(communicator, nullptr);
    auto *storage = new(std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_NE(storage, nullptr);
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();

    EXPECT_CALL(*communicator, SendMessage(_, _, _, _)).WillRepeatedly(Return(DB_ERROR));
    const int loopCount = 100;
    const int timeDriverMs = 100;
    for (int i = 0; i < loopCount; ++i) {
        MockTimeSync timeSync;
        EXPECT_EQ(timeSync.Initialize(communicator, metadata, storage, "DEVICES_A"), E_OK);
        EXPECT_CALL(timeSync, SyncStart).WillRepeatedly(Return(E_OK));
        timeSync.ModifyTimer(timeDriverMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeDriverMs));
        timeSync.Close();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    metadata = nullptr;
    delete storage;
    delete communicator;
}

class DistributedDBMockSyncModuleTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBMockSyncModuleTest::SetUpTestCase(void)
{
}

void DistributedDBMockSyncModuleTest::TearDownTestCase(void)
{
}

void DistributedDBMockSyncModuleTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBMockSyncModuleTest::TearDown(void)
{
}

/**
 * @tc.name: StateMachineCheck001
 * @tc.desc: Test machine do timeout when has same timerId.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck001, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    TimerId expectId = 0;
    TimerId actualId = expectId;
    EXPECT_CALL(syncTaskContext, GetTimerId()).WillOnce(Return(expectId));
    EXPECT_CALL(stateMachine, SwitchStateAndStep(_)).WillOnce(Return());

    stateMachine.CallStepToTimeout(actualId);
}

/**
 * @tc.name: StateMachineCheck002
 * @tc.desc: Test machine do timeout when has diff timerId.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck002, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    TimerId expectId = 0;
    TimerId actualId = 1;
    EXPECT_CALL(syncTaskContext, GetTimerId()).WillOnce(Return(expectId));
    EXPECT_CALL(stateMachine, SwitchStateAndStep(_)).Times(0);

    stateMachine.CallStepToTimeout(actualId);
}

/**
 * @tc.name: StateMachineCheck003
 * @tc.desc: Test machine exec next task when queue not empty.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck003, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    syncTaskContext.SetLastRequestSessionId(1u);
    EXPECT_CALL(syncTaskContext, IsTargetQueueEmpty()).WillRepeatedly(Return(false));
    EXPECT_CALL(syncTaskContext, Clear()).WillRepeatedly([&syncTaskContext]() {
        syncTaskContext.SetLastRequestSessionId(0u);
    });
    EXPECT_CALL(syncTaskContext, MoveToNextTarget()).WillRepeatedly(Return());
    EXPECT_CALL(syncTaskContext, IsCurrentSyncTaskCanBeSkipped()).WillOnce(Return(true)).WillOnce(Return(false));
    // we expect machine don't change context status when queue not empty
    EXPECT_CALL(syncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_CALL(stateMachine, PrepareNextSyncTask()).WillOnce(Return(E_OK));
    EXPECT_CALL(syncTaskContext, SetTaskExecStatus(_)).Times(0);

    EXPECT_EQ(stateMachine.CallExecNextTask(), E_OK);
    EXPECT_EQ(syncTaskContext.GetLastRequestSessionId(), 0u);
}

/**
 * @tc.name: StateMachineCheck004
 * @tc.desc: Test machine deal time sync ack failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck004, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    ASSERT_NE(message, nullptr);
    message->SetMessageType(TYPE_RESPONSE);
    message->SetSessionId(1u);
    EXPECT_CALL(syncTaskContext, GetRequestSessionId()).WillRepeatedly(Return(1u));
    EXPECT_EQ(stateMachine.CallTimeMarkSyncRecv(message), -E_INVALID_ARGS);
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_INVALID_ARGS);
    delete message;
}

/**
 * @tc.name: StateMachineCheck005
 * @tc.desc: Test machine recv errCode.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck005, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    EXPECT_CALL(stateMachine, SwitchStateAndStep(_)).WillRepeatedly(Return());
    EXPECT_CALL(syncTaskContext, GetRequestSessionId()).WillRepeatedly(Return(0u));

    std::initializer_list<int> testCode = {-E_DISTRIBUTED_SCHEMA_CHANGED, -E_DISTRIBUTED_SCHEMA_NOT_FOUND};
    for (int errCode : testCode) {
        stateMachine.DataRecvErrCodeHandle(0, errCode);
        EXPECT_EQ(syncTaskContext.GetTaskErrCode(), errCode);
        stateMachine.CallDataAckRecvErrCodeHandle(errCode, true);
        EXPECT_EQ(syncTaskContext.GetTaskErrCode(), errCode);
    }
    EXPECT_CALL(syncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    stateMachine.DataRecvErrCodeHandle(0, -E_NOT_PERMIT);
}

/**
 * @tc.name: StateMachineCheck006
 * @tc.desc: Test machine exec next task when queue not empty to empty.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck006, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    syncTaskContext.CallSetSyncMode(QUERY_PUSH);
    EXPECT_CALL(syncTaskContext, IsTargetQueueEmpty())
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(syncTaskContext, IsCurrentSyncTaskCanBeSkipped())
        .WillRepeatedly(Return(syncTaskContext.CallIsCurrentSyncTaskCanBeSkipped()));
    EXPECT_CALL(syncTaskContext, MoveToNextTarget()).WillOnce(Return());
    // we expect machine don't change context status when queue not empty
    EXPECT_CALL(syncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_CALL(syncTaskContext, SetTaskExecStatus(_)).WillOnce(Return());
    EXPECT_CALL(syncTaskContext, Clear()).WillRepeatedly(Return());

    EXPECT_EQ(stateMachine.CallExecNextTask(), -E_NO_SYNC_TASK);
}

/**
 * @tc.name: StateMachineCheck007
 * @tc.desc: Test machine DoSaveDataNotify in another thread.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck007, TestSize.Level3)
{
    MockSingleVerStateMachine stateMachine;
    uint8_t callCount = 0;
    EXPECT_CALL(stateMachine, DoSaveDataNotify(_, _, _))
        .WillRepeatedly([&callCount](uint32_t sessionId, uint32_t sequenceId, uint32_t inMsgId) {
            (void) sessionId;
            (void) sequenceId;
            (void) inMsgId;
            callCount++;
            std::this_thread::sleep_for(std::chrono::seconds(4)); // sleep 4s
        });
    stateMachine.CallStartSaveDataNotify(0, 0, 0);
    std::this_thread::sleep_for(std::chrono::seconds(5)); // sleep 5s
    stateMachine.CallStopSaveDataNotify();
    // timer is called once in 2s, we sleep 5s timer call twice
    EXPECT_EQ(callCount, 2);
    std::this_thread::sleep_for(std::chrono::seconds(10)); // sleep 10s to wait all thread exit
}

/**
 * @tc.name: StateMachineCheck008
 * @tc.desc: test machine process when last sync task send packet failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck008, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    syncTaskContext.CallCommErrHandlerFuncInner(-E_PERIPHERAL_INTERFACE_FAIL, 1u);
    EXPECT_EQ(syncTaskContext.IsCommNormal(), true);
}

/**
 * @tc.name: StateMachineCheck009
 * @tc.desc: test machine process when last sync task send packet failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck009, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    stateMachine.CallSwitchMachineState(SingleVerSyncStateMachine::Event::START_SYNC_EVENT); // START_SYNC_EVENT
    stateMachine.CommErrAbort(1u);
    EXPECT_EQ(stateMachine.GetCurrentState(), State::TIME_SYNC);
}

/**
 * @tc.name: StateMachineCheck010
 * @tc.desc: test machine process when error happened in response pull.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck010, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    EXPECT_CALL(stateMachine, SwitchStateAndStep(_)).WillOnce(Return());
    stateMachine.CallResponsePullError(-E_BUSY, false);
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: StateMachineCheck011
 * @tc.desc: test machine process when error happened in response pull.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck011, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    syncTaskContext.CallSetTaskExecStatus(SyncTaskContext::RUNNING);
    EXPECT_CALL(syncTaskContext, GetRequestSessionId()).WillOnce(Return(1u));
    syncTaskContext.ClearAllSyncTask();
    EXPECT_EQ(syncTaskContext.IsCommNormal(), false);
}

/**
 * @tc.name: StateMachineCheck012
 * @tc.desc: Verify Ability LastNotify AckReceive callback.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck012, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    EXPECT_CALL(stateMachine, SwitchStateAndStep(_)).WillOnce(Return());
    DistributedDB::Message msg(ABILITY_SYNC_MESSAGE);
    msg.SetMessageType(TYPE_NOTIFY);
    AbilitySyncAckPacket packet;
    packet.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    packet.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    packet.SetAckCode(-E_BUSY);
    msg.SetCopiedObject(packet);
    EXPECT_EQ(stateMachine.ReceiveMessageCallback(&msg), E_OK);
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: StateMachineCheck013
 * @tc.desc: test kill syncTaskContext.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck013, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(StateMachineCheck013());
}

/**
 * @tc.name: StateMachineCheck014
 * @tc.desc: test machine stop save notify without start.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck014, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    stateMachine.CallStopSaveDataNotify();
    EXPECT_EQ(stateMachine.GetSaveDataNotifyRefCount(), 0);
}

/**
 * @tc.name: DataSyncCheck001
 * @tc.desc: Test dataSync recv error ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, DataSyncCheck001, TestSize.Level1)
{
    SingleVerDataSync dataSync;
    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    message->SetErrorNo(E_FEEDBACK_COMMUNICATOR_NOT_FOUND);
    EXPECT_EQ(dataSync.AckPacketIdCheck(message), true);
    delete message;
}

/**
 * @tc.name: DataSyncCheck002
 * @tc.desc: Test dataSync recv notify ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, DataSyncCheck002, TestSize.Level1)
{
    SingleVerDataSync dataSync;
    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    message->SetMessageType(TYPE_NOTIFY);
    EXPECT_EQ(dataSync.AckPacketIdCheck(message), true);
    delete message;
}
#ifdef DATA_SYNC_CHECK_003
/**
 * @tc.name: DataSyncCheck003
 * @tc.desc: Test dataSync recv notify ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, DataSyncCheck003, TestSize.Level1)
{
    MockSingleVerDataSync mockDataSync;
    MockSyncTaskContext mockSyncTaskContext;
    auto mockMetadata = std::make_shared<MockMetadata>();
    SyncTimeRange dataTimeRange = {1, 0, 1, 0};
    mockDataSync.CallUpdateSendInfo(dataTimeRange, &mockSyncTaskContext);

    VirtualRelationalVerSyncDBInterface storage;
    MockCommunicator communicator;
    std::shared_ptr<Metadata> metadata = std::static_pointer_cast<Metadata>(mockMetadata);
    mockDataSync.Initialize(&storage, &communicator, metadata, "deviceId");

    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    DataAckPacket packet;
    message->SetSequenceId(1);
    message->SetCopiedObject(packet);
    mockSyncTaskContext.SetQuerySync(true);

    EXPECT_CALL(*mockMetadata, GetLastQueryTime(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(*mockMetadata, SetLastQueryTime(_, _, _))
        .WillOnce([&dataTimeRange](
                    const std::string &queryIdentify, const std::string &deviceId, const Timestamp &timestamp) {
            EXPECT_EQ(timestamp, dataTimeRange.endTime);
            return E_OK;
    });
    EXPECT_CALL(mockSyncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_EQ(mockDataSync.TryContinueSync(&mockSyncTaskContext, message), -E_FINISHED);
    delete message;
}
#endif

/**
 * @tc.name: DataSyncCheck004
 * @tc.desc: Test dataSync do ability sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, DataSyncCheck004, TestSize.Level1)
{
    MockSingleVerDataSync dataSync;
    auto *message = new (std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    message->SetMessageType(TYPE_NOTIFY);
    auto *context = new (std::nothrow) SingleVerKvSyncTaskContext();
    ASSERT_NE(context, nullptr);
    auto *communicator = new (std::nothrow) VirtualCommunicator("DEVICE", nullptr);
    ASSERT_NE(communicator, nullptr);
    dataSync.SetCommunicatorHandle(communicator);
    EXPECT_EQ(dataSync.CallDoAbilitySyncIfNeed(context, message, false), -E_NEED_ABILITY_SYNC);
    delete message;
    RefObject::KillAndDecObjRef(context);
    dataSync.SetCommunicatorHandle(nullptr);
    RefObject::KillAndDecObjRef(communicator);
}

/**
 * @tc.name: AutoLaunchCheck001
 * @tc.desc: Test autoLaunch close connection.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AutoLaunchCheck001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(AutoLaunchCheck001());
}

/**
 * @tc.name: AutoLaunchCheck002
 * @tc.desc: Test autoLaunch receive diff userId.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AutoLaunchCheck002, TestSize.Level1)
{
    MockAutoLaunch mockAutoLaunch;
    std::string id = "identify";
    std::string userId0 = "USER0";
    std::string userId1 = "USER1";
    AutoLaunchItem item;
    item.propertiesPtr = std::make_shared<KvDBProperties>();
    mockAutoLaunch.SetWhiteListItem(id, userId0, item);
    bool ext = false;
    EXPECT_EQ(mockAutoLaunch.CallGetAutoLaunchItemUid(id, userId1, ext), userId0);
    EXPECT_EQ(ext, false);
    mockAutoLaunch.ClearWhiteList();
}

/**
 * @tc.name: SyncDataSync001
 * @tc.desc: Test request start when RemoveDeviceDataIfNeed failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncDataSync001, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    MockSingleVerDataSync dataSync;

    EXPECT_CALL(dataSync, RemoveDeviceDataIfNeed(_)).WillRepeatedly(Return(-E_BUSY));
    EXPECT_EQ(dataSync.CallRequestStart(&syncTaskContext, PUSH), -E_BUSY);
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: SyncDataSync002
 * @tc.desc: Test pull request start when RemoveDeviceDataIfNeed failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncDataSync002, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    MockSingleVerDataSync dataSync;

    EXPECT_CALL(dataSync, RemoveDeviceDataIfNeed(_)).WillRepeatedly(Return(-E_BUSY));
    EXPECT_EQ(dataSync.CallPullRequestStart(&syncTaskContext), -E_BUSY);
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: SyncDataSync003
 * @tc.desc: Test call RemoveDeviceDataIfNeed in diff thread.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncDataSync003, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    MockSingleVerDataSync dataSync;

    VirtualSingleVerSyncDBInterface storage;
    MockCommunicator communicator;
    std::shared_ptr<MockMetadata> mockMetadata = std::make_shared<MockMetadata>();
    std::shared_ptr<Metadata> metadata = std::static_pointer_cast<Metadata>(mockMetadata);
    metadata->Initialize(&storage);
    const std::string deviceId = "deviceId";
    dataSync.Initialize(&storage, &communicator, metadata, deviceId);
    syncTaskContext.SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    syncTaskContext.Initialize(deviceId, &storage, metadata, &communicator);
    syncTaskContext.EnableClearRemoteStaleData(true);

    /**
     * @tc.steps: step1. set diff db createtime for rebuild label in meta
     */
    metadata->SetDbCreateTime(deviceId, 1, true); // 1 is old db createTime
    metadata->SetDbCreateTime(deviceId, 2, true); // 1 is new db createTime

    DistributedDB::Key k1 = {'k', '1'};
    DistributedDB::Value v1 = {'v', '1'};
    DistributedDB::Key k2 = {'k', '2'};
    DistributedDB::Value v2 = {'v', '2'};

    /**
     * @tc.steps: step2. call RemoveDeviceDataIfNeed in diff thread and then put data
     */
    std::thread thread1([&]() {
        (void)dataSync.CallRemoveDeviceDataIfNeed(&syncTaskContext);
        storage.PutDeviceData(deviceId, k1, v1);
        LOGD("PUT FINISH");
    });
    std::thread thread2([&]() {
        (void)dataSync.CallRemoveDeviceDataIfNeed(&syncTaskContext);
        storage.PutDeviceData(deviceId, k2, v2);
        LOGD("PUT FINISH");
    });
    thread1.join();
    thread2.join();

    DistributedDB::Value actualValue;
    storage.GetDeviceData(deviceId, k1, actualValue);
    EXPECT_EQ(v1, actualValue);
    storage.GetDeviceData(deviceId, k2, actualValue);
    EXPECT_EQ(v2, actualValue);
}

/**
 * @tc.name: AbilitySync001
 * @tc.desc: Test abilitySync abort when recv error.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AbilitySync001, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    AbilitySync abilitySync;

    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    AbilitySyncAckPacket packet;
    packet.SetAckCode(-E_BUSY);
    message->SetCopiedObject(packet);
    EXPECT_EQ(abilitySync.AckRecv(message, &syncTaskContext), -E_BUSY);
    delete message;
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: AbilitySync002
 * @tc.desc: Test abilitySync abort when save meta failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AbilitySync002, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    AbilitySync abilitySync;
    MockCommunicator comunicator;
    VirtualSingleVerSyncDBInterface syncDBInterface;
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    metaData->Initialize(&syncDBInterface);
    abilitySync.Initialize(&comunicator, &syncDBInterface, metaData, "deviceId");

    /**
     * @tc.steps: step1. set AbilitySyncAckPacket ackCode is E_OK for pass the ack check
     */
    DistributedDB::Message *message = new (std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    AbilitySyncAckPacket packet;
    packet.SetAckCode(E_OK);
    // should set version less than 108 avoid ability sync with 2 packet
    packet.SetSoftwareVersion(SOFTWARE_VERSION_RELEASE_7_0);
    message->SetCopiedObject(packet);
    /**
     * @tc.steps: step2. set syncDBInterface busy for save data return -E_BUSY
     */
    syncDBInterface.SetBusy(true);
    EXPECT_CALL(syncTaskContext, GetSchemaSyncStatus(_)).Times(0);
    EXPECT_EQ(abilitySync.AckRecv(message, &syncTaskContext), -E_BUSY);
    delete message;
    EXPECT_EQ(syncTaskContext.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: AbilitySync002
 * @tc.desc: Test abilitySync when offline.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AbilitySync003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set table TEST is permitSync
     */
    SingleVerRelationalSyncTaskContext *context = new (std::nothrow) SingleVerRelationalSyncTaskContext();
    ASSERT_NE(context, nullptr);
    RelationalSyncStrategy strategy;
    const std::string tableName = "TEST";
    strategy[tableName] = {true, true, true};
    context->SetRelationalSyncStrategy(strategy, true);
    QuerySyncObject query;
    query.SetTableName(tableName);
    /**
     * @tc.steps: step2. set table is need reset ability sync but it still permit sync
     */
    EXPECT_EQ(context->GetSchemaSyncStatus(query).first, true);
    /**
     * @tc.steps: step3. set table is schema change now it don't permit sync
     */
    context->SchemaChange();
    EXPECT_EQ(context->GetSchemaSyncStatus(query).first, false);
    RefObject::KillAndDecObjRef(context);
}

/**
 * @tc.name: AbilitySync004
 * @tc.desc: Test abilitySync when offline.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AbilitySync004, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(AbilitySync004());
}

/**
 * @tc.name: SyncLifeTest001
 * @tc.desc: Test syncer alive when thread still exist.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncLifeTest001, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(SyncLifeTest001());
}

/**
 * @tc.name: SyncLifeTest002
 * @tc.desc: Test autosync when thread still exist.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncLifeTest002, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(SyncLifeTest002());
}

/**
 * @tc.name: SyncLifeTest003
 * @tc.desc: Test syncer localdatachange when store is destructor
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncLifeTest003, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(SyncLifeTest003());
}

/**
 * @tc.name: SyncLifeTest004
 * @tc.desc: Test syncer remote data change.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncLifeTest004, TestSize.Level3)
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    auto syncDBInterface = new MockKvSyncInterface();
    int incRefCount = 0;
    EXPECT_CALL(*syncDBInterface, IncRefCount()).WillRepeatedly([&incRefCount]() {
        incRefCount++;
    });
    EXPECT_CALL(*syncDBInterface, DecRefCount()).WillRepeatedly(Return());
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    syncDBInterface->SetIdentifier(identifier);
    syncer->Initialize(syncDBInterface, true);
    syncer->EnableAutoSync(true);
    incRefCount = 0;
    syncer->RemoteDataChanged("");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(incRefCount, 2); // refCount is 2
    syncer = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete syncDBInterface;
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: SyncLifeTest005
 * @tc.desc: Test syncer remote device offline.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncLifeTest005, TestSize.Level3)
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    auto syncDBInterface = new MockKvSyncInterface();
    int incRefCount = 0;
    int dbInfoCount = 0;
    EXPECT_CALL(*syncDBInterface, IncRefCount()).WillRepeatedly([&incRefCount]() {
        incRefCount++;
    });
    EXPECT_CALL(*syncDBInterface, DecRefCount()).WillRepeatedly(Return());
    EXPECT_CALL(*syncDBInterface, GetDBInfo(_)).WillRepeatedly([&dbInfoCount](DBInfo &) {
        dbInfoCount++;
    });
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    syncDBInterface->SetIdentifier(identifier);
    syncer->Initialize(syncDBInterface, true);
    syncer->Close(true);
    incRefCount = 0;
    dbInfoCount = 0;
    syncer->RemoteDeviceOffline("dev");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(incRefCount, 1); // refCount is 1 when remote dev offline
    EXPECT_EQ(dbInfoCount, 0); // dbInfoCount is 0 when remote dev offline
    syncer = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete syncDBInterface;
}

/**
 * @tc.name: MessageScheduleTest001
 * @tc.desc: Test MessageSchedule stop timer when no message.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, MessageScheduleTest001, TestSize.Level1)
{
    MockSyncTaskContext *context = new MockSyncTaskContext();
    ASSERT_NE(context, nullptr);
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    bool last = false;
    context->OnLastRef([&last]() {
        last = true;
    });
    SingleVerDataMessageSchedule schedule;
    bool isNeedHandle = false;
    bool isNeedContinue = false;
    schedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    RefObject::KillAndDecObjRef(context);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(last);
}

/**
 * @tc.name: SyncEngineTest001
 * @tc.desc: Test SyncEngine receive message when closing.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncEngineTest001, TestSize.Level1)
{
    std::unique_ptr<MockSyncEngine> enginePtr = std::make_unique<MockSyncEngine>();
    EXPECT_CALL(*enginePtr, CreateSyncTaskContext())
        .WillRepeatedly(Return(new (std::nothrow) SingleVerKvSyncTaskContext()));
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    MockKvSyncInterface syncDBInterface;
    EXPECT_CALL(syncDBInterface, IncRefCount()).WillRepeatedly(Return());
    EXPECT_CALL(syncDBInterface, DecRefCount()).WillRepeatedly(Return());
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    syncDBInterface.SetIdentifier(identifier);
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    metaData->Initialize(&syncDBInterface);
    ASSERT_NE(virtualCommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    ISyncEngine::InitCallbackParam param = { nullptr, nullptr, nullptr };
    enginePtr->Initialize(&syncDBInterface, metaData, param);
    auto communicator =
        static_cast<VirtualCommunicator *>(virtualCommunicatorAggregator->GetCommunicator("real_device"));
    RefObject::IncObjRef(communicator);
    std::thread thread1([&]() {
        if (communicator == nullptr) {
            return;
        }
        for (int count = 0; count < 100; count++) { // loop 100 times
            auto *message = new (std::nothrow) DistributedDB::Message();
            ASSERT_NE(message, nullptr);
            message->SetMessageId(LOCAL_DATA_CHANGED);
            message->SetErrorNo(E_FEEDBACK_UNKNOWN_MESSAGE);
            communicator->CallbackOnMessage("src", message);
        }
    });
    std::thread thread2([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        enginePtr->Close();
    });
    thread1.join();
    thread2.join();

    LOGD("FINISHED");
    RefObject::KillAndDecObjRef(communicator);
    communicator = nullptr;
    enginePtr = nullptr;
    metaData = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    virtualCommunicatorAggregator = nullptr;
}

/**
 * @tc.name: SyncEngineTest002
 * @tc.desc: Test SyncEngine add sync operation.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncEngineTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare env
     */
    auto *enginePtr = new (std::nothrow) MockSyncEngine();
    ASSERT_NE(enginePtr, nullptr);
    EXPECT_CALL(*enginePtr, CreateSyncTaskContext())
        .WillRepeatedly([]() {
            return new (std::nothrow) SingleVerKvSyncTaskContext();
        });
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    MockKvSyncInterface syncDBInterface;
    int syncInterfaceRefCount = 1;
    EXPECT_CALL(syncDBInterface, IncRefCount()).WillRepeatedly([&syncInterfaceRefCount]() {
        syncInterfaceRefCount++;
    });
    EXPECT_CALL(syncDBInterface, DecRefCount()).WillRepeatedly([&syncInterfaceRefCount]() {
        syncInterfaceRefCount--;
    });
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    syncDBInterface.SetIdentifier(identifier);
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    ASSERT_NE(virtualCommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    ISyncEngine::InitCallbackParam param = { nullptr, nullptr, nullptr };
    enginePtr->Initialize(&syncDBInterface, metaData, param);
    /**
     * @tc.steps: step2. add sync operation for DEVICE_A and DEVICE_B. It will create two context for A and B
     */
    std::vector<std::string> devices = {
        "DEVICES_A", "DEVICES_B"
    };
    const int syncId = 1;
    auto operation = new (std::nothrow) SyncOperation(syncId, devices, 0, nullptr, false);
    if (operation != nullptr) {
        enginePtr->AddSyncOperation(operation);
    }
    /**
     * @tc.steps: step3. abort machine and both context will be released
     */
    syncInterfaceRefCount = 0;
    enginePtr->AbortMachineIfNeed(syncId);
    EXPECT_EQ(syncInterfaceRefCount, 0);
    enginePtr->Close();

    RefObject::KillAndDecObjRef(enginePtr);
    enginePtr = nullptr;
    RefObject::KillAndDecObjRef(operation);

    metaData = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    virtualCommunicatorAggregator = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: SyncEngineTest003
 * @tc.desc: Test SyncEngine add block sync operation.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncEngineTest003, TestSize.Level1)
{
    auto *enginePtr = new (std::nothrow) MockSyncEngine();
    ASSERT_NE(enginePtr, nullptr);
    std::vector<std::string> devices = {
        "DEVICES_A", "DEVICES_B"
    };
    const int syncId = 1;
    auto operation = new (std::nothrow) SyncOperation(syncId, devices, 0, nullptr, true);
    ASSERT_NE(operation, nullptr);
    operation->Initialize();
    enginePtr->AddSyncOperation(operation);
    for (const auto &device: devices) {
        EXPECT_EQ(operation->GetStatus(device), static_cast<int>(SyncOperation::OP_BUSY_FAILURE));
    }
    RefObject::KillAndDecObjRef(operation);
    RefObject::KillAndDecObjRef(enginePtr);
}

/**
* @tc.name: remote query packet 001
* @tc.desc: Test RemoteExecutorRequestPacket Serialization And DeSerialization
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBMockSyncModuleTest, RemoteQueryPacket001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create remoteExecutorRequestPacket
     */
    RemoteExecutorRequestPacket packet;
    std::map<std::string, std::string> extraCondition = { { "test", "testsql" } };
    packet.SetExtraConditions(extraCondition);
    packet.SetNeedResponse();
    packet.SetVersion(SOFTWARE_VERSION_RELEASE_6_0);

    /**
     * @tc.steps: step2. serialization to parcel
     */
    std::vector<uint8_t> buffer(packet.CalculateLen());
    Parcel parcel(buffer.data(), buffer.size());
    ASSERT_EQ(packet.Serialization(parcel), E_OK);
    ASSERT_FALSE(parcel.IsError());

    /**
     * @tc.steps: step3. deserialization from parcel
     */
    RemoteExecutorRequestPacket targetPacket;
    Parcel targetParcel(buffer.data(), buffer.size());
    ASSERT_EQ(targetPacket.DeSerialization(targetParcel), E_OK);
    ASSERT_FALSE(parcel.IsError());

    /**
     * @tc.steps: step4. check packet is equal
     */
    EXPECT_EQ(packet.GetVersion(), targetPacket.GetVersion());
    EXPECT_EQ(packet.GetFlag(), targetPacket.GetFlag());
}

/**
* @tc.name: remote query packet 002
* @tc.desc: Test RemoteExecutorAckPacket Serialization And DeSerialization
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBMockSyncModuleTest, RemoteQueryPacket002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create remoteExecutorRequestPacket
     */
    RemoteExecutorAckPacket packet;
    packet.SetLastAck();
    packet.SetAckCode(-E_INTERNAL_ERROR);
    packet.SetVersion(SOFTWARE_VERSION_RELEASE_6_0);

    /**
     * @tc.steps: step2. serialization to parcel
     */
    std::vector<uint8_t> buffer(packet.CalculateLen());
    Parcel parcel(buffer.data(), buffer.size());
    ASSERT_EQ(packet.Serialization(parcel), E_OK);
    ASSERT_FALSE(parcel.IsError());

    /**
     * @tc.steps: step3. deserialization from parcel
     */
    RemoteExecutorAckPacket targetPacket;
    Parcel targetParcel(buffer.data(), buffer.size());
    ASSERT_EQ(targetPacket.DeSerialization(targetParcel), E_OK);
    ASSERT_FALSE(parcel.IsError());

    /**
     * @tc.steps: step4. check packet is equal
     */
    EXPECT_EQ(packet.GetVersion(), targetPacket.GetVersion());
    EXPECT_EQ(packet.GetFlag(), targetPacket.GetFlag());
    EXPECT_EQ(packet.GetAckCode(), targetPacket.GetAckCode());
}

/**
* @tc.name: remote query packet 003
* @tc.desc: Test RemoteExecutorRequestPacket Serialization with invalid args
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangshijie
*/
HWTEST_F(DistributedDBMockSyncModuleTest, RemoteQueryPacket003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. check E_INVALID_ARGS
     */
    RemoteExecutorRequestPacket packet;
    packet.SetNeedResponse();
    packet.SetVersion(SOFTWARE_VERSION_RELEASE_6_0);

    std::vector<uint8_t> buffer(packet.CalculateLen());
    Parcel parcel(buffer.data(), buffer.size());

    ASSERT_EQ(packet.Serialization(parcel), E_OK);
    std::map<std::string, std::string> extraCondition = { { "test", "testsql" } };
    packet.SetExtraConditions(extraCondition);
    EXPECT_EQ(packet.Serialization(parcel), -E_INVALID_ARGS);

    std::string sql = "testsql";
    for (uint32_t i = 0; i < DBConstant::MAX_CONDITION_COUNT; i++) {
        extraCondition[std::to_string(i)] = sql;
    }
    packet.SetExtraConditions(extraCondition);

    std::vector<uint8_t> buffer2(packet.CalculateLen());
    Parcel parcel2(buffer2.data(), buffer2.size());
    Parcel targetParcel2(buffer2.data(), buffer2.size());
    EXPECT_EQ(packet.Serialization(parcel2), -E_INVALID_ARGS);

    extraCondition.erase("0");
    extraCondition.erase("1");
    extraCondition.erase("2");
    std::string bigKey(DBConstant::MAX_CONDITION_KEY_LEN + 1, 'a');
    extraCondition[bigKey] = sql;
    packet.SetExtraConditions(extraCondition);
    std::vector<uint8_t> buffer3(packet.CalculateLen());
    Parcel parcel3(buffer3.data(), buffer3.size());
    EXPECT_EQ(packet.Serialization(parcel3), -E_INVALID_ARGS);

    std::string bigValue(DBConstant::MAX_CONDITION_VALUE_LEN + 1, 'a');
    extraCondition["1"] = bigValue;
    packet.SetExtraConditions(extraCondition);
    std::vector<uint8_t> buffer4(packet.CalculateLen());
    Parcel parcel4(buffer4.data(), buffer4.size());
    EXPECT_EQ(packet.Serialization(parcel4), -E_INVALID_ARGS);
}

/**
* @tc.name: remote query packet 004
* @tc.desc: Test RemoteExecutorRequestPacket Deserialization with invalid args
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangshijie
*/
HWTEST_F(DistributedDBMockSyncModuleTest, RemoteQueryPacket004, TestSize.Level1)
{
    RemoteExecutorRequestPacket packet;
    packet.SetNeedResponse();
    packet.SetVersion(SOFTWARE_VERSION_RELEASE_6_0);

    std::vector<uint8_t> buffer(packet.CalculateLen());
    RemoteExecutorRequestPacket targetPacket;
    Parcel targetParcel(buffer.data(), 3); // 3 is invalid len for deserialization
    EXPECT_EQ(targetPacket.DeSerialization(targetParcel), -E_INVALID_ARGS);

    std::vector<uint8_t> buffer1(1024); // 1024 is buffer len for serialization
    Parcel parcel(buffer1.data(), buffer1.size());
    ConstructPacel(parcel, DBConstant::MAX_CONDITION_COUNT + 1, "", "");
    Parcel desParcel(buffer1.data(), buffer1.size());
    EXPECT_EQ(targetPacket.DeSerialization(desParcel), -E_INVALID_ARGS);

    Parcel parcel2(buffer1.data(), buffer1.size());
    std::string bigKey(DBConstant::MAX_CONDITION_KEY_LEN + 1, 'a');
    ConstructPacel(parcel2, 1, bigKey, "");
    Parcel desParcel2(buffer1.data(), buffer1.size());
    EXPECT_EQ(targetPacket.DeSerialization(desParcel2), -E_INVALID_ARGS);

    Parcel parcel3(buffer1.data(), buffer1.size());
    std::string bigValue(DBConstant::MAX_CONDITION_VALUE_LEN + 1, 'a');
    ConstructPacel(parcel3, 1, "1", bigValue);
    Parcel desParcel3(buffer1.data(), buffer1.size());
    EXPECT_EQ(targetPacket.DeSerialization(desParcel3), -E_INVALID_ARGS);

    Parcel parcel4(buffer1.data(), buffer1.size());
    ConstructPacel(parcel4, 1, "1", "1");
    Parcel desParcel4(buffer1.data(), buffer1.size());
    EXPECT_EQ(targetPacket.DeSerialization(desParcel4), E_OK);

    Parcel parcel5(buffer1.data(), buffer1.size());
    ConstructPacel(parcel5, 0, "", "");
    Parcel desParcel5(buffer1.data(), buffer1.size());
    EXPECT_EQ(targetPacket.DeSerialization(desParcel5), E_OK);
}

/**
 * @tc.name: SingleVerKvEntryTest001
 * @tc.desc: Test SingleVerKvEntry Serialize and DeSerialize.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SingleVerKvEntryTest001, TestSize.Level1)
{
    std::vector<SingleVerKvEntry *> kvEntries;
    size_t len = 0u;
    for (size_t i = 0; i < DBConstant::MAX_NORMAL_PACK_ITEM_SIZE + 1; ++i) {
        auto entryPtr = new GenericSingleVerKvEntry();
        kvEntries.push_back(entryPtr);
        len += entryPtr->CalculateLen(SOFTWARE_VERSION_CURRENT);
        len = BYTE_8_ALIGN(len);
    }
    std::vector<uint8_t> srcData(len, 0);
    Parcel parcel(srcData.data(), srcData.size());
    EXPECT_EQ(GenericSingleVerKvEntry::SerializeDatas(kvEntries, parcel, SOFTWARE_VERSION_CURRENT), E_OK);
    parcel = Parcel(srcData.data(), srcData.size());
    EXPECT_EQ(GenericSingleVerKvEntry::DeSerializeDatas(kvEntries, parcel), 0);
    SingleVerKvEntry::Release(kvEntries);
}

/**
* @tc.name: mock remote query 001
* @tc.desc: Test RemoteExecutor receive msg when closing
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBMockSyncModuleTest, MockRemoteQuery001, TestSize.Level3)
{
    MockRemoteExecutor *executor = new (std::nothrow) MockRemoteExecutor();
    ASSERT_NE(executor, nullptr);
    uint32_t count = 0u;
    EXPECT_CALL(*executor, ParseOneRequestMessage).WillRepeatedly(
        [&count](const std::string &device, DistributedDB::Message *inMsg) {
            std::this_thread::sleep_for(std::chrono::seconds(5)); // mock one msg execute 5 s
            count++;
    });
    EXPECT_CALL(*executor, IsPacketValid).WillRepeatedly(Return(true));
    for (uint32_t i = 0; i < MESSAGE_COUNT; i++) {
        DistributedDB::Message *message = nullptr;
        EXPECT_EQ(BuildRemoteQueryMsg(message), E_OK);
        executor->ReceiveMessage("DEVICE", message);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    executor->Close();
    EXPECT_EQ(count, EXECUTE_COUNT);
    RefObject::KillAndDecObjRef(executor);
}

/**
* @tc.name: mock remote query 002
* @tc.desc: Test RemoteExecutor response failed when closing
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangqiquan
*/
HWTEST_F(DistributedDBMockSyncModuleTest, MockRemoteQuery002, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(MockRemoteQuery002());
}

/**
 * @tc.name: SyncTaskContextCheck001
 * @tc.desc: test context check task can be skipped in push mode.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncTaskContextCheck001, TestSize.Level1)
{
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    (void)syncTaskContext.Initialize("device", &dbSyncInterface, metadata, &communicator);
    syncTaskContext.SetLastFullSyncTaskStatus(SyncOperation::Status::OP_FINISHED_ALL);
    syncTaskContext.CallSetSyncMode(static_cast<int>(SyncModeType::PUSH));
    EXPECT_EQ(syncTaskContext.CallIsCurrentSyncTaskCanBeSkipped(), true);
}

/**
 * @tc.name: SyncTaskContextCheck002
 * @tc.desc: test context check task can be skipped in push mode.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncTaskContextCheck002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create context and operation
     */
    auto syncTaskContext = new(std::nothrow) MockSyncTaskContext();
    ASSERT_NE(syncTaskContext, nullptr);
    auto operation = new SyncOperation(1u, {}, static_cast<int>(SyncModeType::QUERY_PUSH), nullptr, false);
    ASSERT_NE(operation, nullptr);
    QuerySyncObject querySyncObject;
    operation->SetQuery(querySyncObject);
    syncTaskContext->SetSyncOperation(operation);
    syncTaskContext->SetLastFullSyncTaskStatus(SyncOperation::Status::OP_FAILED);
    syncTaskContext->CallSetSyncMode(static_cast<int>(SyncModeType::QUERY_PUSH));
    EXPECT_CALL(*syncTaskContext, IsTargetQueueEmpty()).WillRepeatedly(Return(false));

    const int loopCount = 1000;
    /**
     * @tc.steps: step2. loop 1000 times for writing data into lastQuerySyncTaskStatusMap_ async
     */
    std::thread writeThread([&syncTaskContext]() {
        for (int i = 0; i < loopCount; ++i) {
            syncTaskContext->SaveLastPushTaskExecStatus(static_cast<int>(SyncOperation::Status::OP_FAILED));
        }
    });
    /**
     * @tc.steps: step3. loop 100000 times for clear lastQuerySyncTaskStatusMap_ async
     */
    std::thread clearThread([&syncTaskContext]() {
        for (int i = 0; i < 100000; ++i) { // loop 100000 times
            syncTaskContext->ResetLastPushTaskStatus();
        }
    });
    /**
     * @tc.steps: step4. loop 1000 times for read data from lastQuerySyncTaskStatusMap_ async
     */
    std::thread readThread([&syncTaskContext]() {
        for (int i = 0; i < loopCount; ++i) {
            EXPECT_EQ(syncTaskContext->CallIsCurrentSyncTaskCanBeSkipped(), false);
        }
    });
    writeThread.join();
    clearThread.join();
    readThread.join();
    RefObject::KillAndDecObjRef(operation);
    syncTaskContext->SetSyncOperation(nullptr);
    RefObject::KillAndDecObjRef(syncTaskContext);
}

/**
 * @tc.name: SyncTaskContextCheck003
 * @tc.desc: test context call on sync task add.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncTaskContextCheck003, TestSize.Level1)
{
    auto *syncTaskContext = new (std::nothrow) MockSyncTaskContext();
    syncTaskContext->CallSetTaskExecStatus(DistributedDB::ISyncTaskContext::RUNNING);
    int callCount = 0;
    std::condition_variable cv;
    std::mutex countMutex;
    syncTaskContext->RegOnSyncTask([&callCount, &countMutex, &cv]() {
        {
            std::lock_guard<std::mutex> autoLock(countMutex);
            callCount++;
        }
        cv.notify_one();
        return E_OK;
    });
    EXPECT_EQ(syncTaskContext->AddSyncTarget(nullptr), -E_INVALID_ARGS);
    auto target = new (std::nothrow) SingleVerSyncTarget();
    ASSERT_NE(target, nullptr);
    target->SetTaskType(ISyncTarget::REQUEST);
    EXPECT_EQ(syncTaskContext->AddSyncTarget(target), E_OK);
    {
        std::unique_lock<std::mutex> lock(countMutex);
        cv.wait_for(lock, std::chrono::seconds(5), [&callCount]() { // wait 5s
            return callCount > 0;
        });
    }
    EXPECT_EQ(callCount, 1);
    RefObject::KillAndDecObjRef(syncTaskContext);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: SyncTaskContextCheck004
 * @tc.desc: test context add sync task should not cancel current task.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncTaskContextCheck004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create context and target
     */
    auto *syncTaskContext = new (std::nothrow) MockSyncTaskContext();
    ASSERT_NE(syncTaskContext, nullptr);
    int beforeRetryTime = syncTaskContext->GetRetryTime();
    auto target = new (std::nothrow) SingleVerSyncTarget();
    ASSERT_NE(target, nullptr);
    target->SetTaskType(ISyncTarget::REQUEST);
    syncTaskContext->SetAutoSync(true);
    /**
     * @tc.steps: step2. add target
     * @tc.expected: retry time should not be changed
     */
    EXPECT_EQ(syncTaskContext->AddSyncTarget(target), E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    int afterRetryTime = syncTaskContext->GetRetryTime();
    EXPECT_EQ(beforeRetryTime, afterRetryTime);
    /**
     * @tc.steps: step3. release resource
     */
    RefObject::KillAndDecObjRef(syncTaskContext);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: SyncTaskContextCheck005
 * @tc.desc: test context call get query id async.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncTaskContextCheck005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare context
     */
    auto context = new (std::nothrow) SingleVerRelationalSyncTaskContext();
    ASSERT_NE(context, nullptr);
    SingleVerSyncStateMachine stateMachine;
    VirtualCommunicator communicator("device", nullptr);
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    ASSERT_EQ(metadata->Initialize(&dbSyncInterface), E_OK);
    (void)context->Initialize("device", &dbSyncInterface, metadata, &communicator);
    (void)stateMachine.Initialize(context, &dbSyncInterface, metadata, &communicator);

    for (int i = 0; i < 100; ++i) { // 100 sync target
        auto target = new (std::nothrow) SingleVerSyncTarget();
        ASSERT_NE(target, nullptr);
        target->SetTaskType(ISyncTarget::RESPONSE);
        EXPECT_EQ(context->AddSyncTarget(target), E_OK);
    }
    std::thread clearThread([context]() {
        for (int i = 0; i < 100; ++i) { // clear 100 times
            context->Clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    std::thread getThread([context]() {
        for (int i = 0; i < 100; ++i) { // get 100 times
            (void) context->GetDeleteSyncId();
            (void) context->GetQuerySyncId();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    std::thread copyThread([context]() {
        for (int i = 0; i < 100; ++i) { // move 100 times
            (void) context->MoveToNextTarget();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    clearThread.join();
    getThread.join();
    copyThread.join();
    context->Clear();
    EXPECT_EQ(context->GetDeleteSyncId(), "");
    EXPECT_EQ(context->GetQuerySyncId(), "");
    RefObject::KillAndDecObjRef(context);
}

/**
 * @tc.name: SyncTaskContextCheck006
 * @tc.desc: test context call get query id async.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncTaskContextCheck006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare context
     */
    auto context = new (std::nothrow) SingleVerRelationalSyncTaskContext();
    ASSERT_NE(context, nullptr);
    auto communicator = new (std::nothrow) VirtualCommunicator("device", nullptr);
    ASSERT_NE(communicator, nullptr);
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    ASSERT_EQ(metadata->Initialize(&dbSyncInterface), E_OK);
    (void)context->Initialize("device", &dbSyncInterface, metadata, communicator);
    /**
     * @tc.steps: step2. add sync target into context
     */
    auto target = new (std::nothrow) SingleVerSyncTarget();
    ASSERT_NE(target, nullptr);
    target->SetTaskType(ISyncTarget::RESPONSE);
    EXPECT_EQ(context->AddSyncTarget(target), E_OK);
    /**
     * @tc.steps: step3. move to next target
     * @tc.expected: retry time will be reset to zero
     */
    context->SetRetryTime(AUTO_RETRY_TIMES);
    context->MoveToNextTarget();
    EXPECT_EQ(context->GetRetryTime(), 0);
    context->Clear();
    RefObject::KillAndDecObjRef(context);
    RefObject::KillAndDecObjRef(communicator);
}
#ifdef RUN_AS_ROOT
/**
 * @tc.name: TimeChangeListenerTest001
 * @tc.desc: Test RegisterTimeChangedLister.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, TimeChangeListenerTest001, TestSize.Level1)
{
    SingleVerKVSyncer syncer;
    VirtualSingleVerSyncDBInterface syncDBInterface;
    KvDBProperties dbProperties;
    dbProperties.SetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, true);
    syncDBInterface.SetDbProperties(dbProperties);
    EXPECT_EQ(syncer.Initialize(&syncDBInterface, false), -E_NO_NEED_ACTIVE);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s wait for time tick
    const std::string LOCAL_TIME_OFFSET_KEY = "localTimeOffset";
    std::vector<uint8_t> key;
    DBCommon::StringToVector(LOCAL_TIME_OFFSET_KEY, key);
    std::vector<uint8_t> beforeOffset;
    EXPECT_EQ(syncDBInterface.GetMetaData(key, beforeOffset), E_OK);
    ChangeTime(2); // increase 2s
    std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s wait for time tick
    std::vector<uint8_t> afterOffset;
    EXPECT_EQ(syncDBInterface.GetMetaData(key, afterOffset), E_OK);
    EXPECT_NE(beforeOffset, afterOffset);
    ChangeTime(-2); // decrease 2s
}
#endif

/**
 * @tc.name: TimeChangeListenerTest002
 * @tc.desc: Test TimeChange.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, TimeChangeListenerTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create syncer without activation
     */
    MockSingleVerKVSyncer syncer;
    VirtualSingleVerSyncDBInterface syncDBInterface;
    KvDBProperties dbProperties;
    dbProperties.SetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, true);
    syncDBInterface.SetDbProperties(dbProperties);
    /**
     * @tc.steps: step2. trigger time change logic and reopen syncer at the same time
     * @tc.expected: no crash here
     */
    const int loopCount = 100;
    std::thread timeChangeThread([&syncer]() {
        for (int i = 0; i < loopCount; ++i) {
            int64_t changeOffset = 1;
            syncer.CallRecordTimeChangeOffset(&changeOffset);
        }
    });
    for (int i = 0; i < loopCount; ++i) {
        EXPECT_EQ(syncer.Initialize(&syncDBInterface, false), -E_NO_NEED_ACTIVE);
        EXPECT_EQ(syncer.Close(true), -E_NOT_INIT);
    }
    timeChangeThread.join();
}

/**
 * @tc.name: SyncerCheck001
 * @tc.desc: Test syncer call set sync retry before init.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncerCheck001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(SyncerCheck001());
}

/**
 * @tc.name: SyncerCheck002
 * @tc.desc: Test syncer call get timestamp with close and open.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncerCheck002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create context and syncer
     */
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    auto virtualCommunicatorAggregator = new(std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_NE(virtualCommunicatorAggregator, nullptr);
    auto syncDBInterface = new VirtualSingleVerSyncDBInterface();
    ASSERT_NE(syncDBInterface, nullptr);
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    syncDBInterface->SetIdentifier(identifier);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    /**
     * @tc.steps: step2. get timestamp by syncer over and over again
     */
    std::atomic<bool> finish = false;
    std::thread t([&finish, &syncer]() {
        while (!finish) {
            (void) syncer->GetTimestamp();
        }
    });
    /**
     * @tc.steps: step3. re init syncer over and over again
     * @tc.expected: step3. dont crash here.
     */
    for (int i = 0; i < 100; ++i) { // loop 100 times
        syncer->Initialize(syncDBInterface, false);
        syncer->Close(true);
    }
    finish = true;
    t.join();
    delete syncDBInterface;
    syncer = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: SyncerCheck003
 * @tc.desc: Test syncer query auto sync.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, DISABLE_SyncerCheck003, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    InternalSyncParma param;
    auto *engine = new(std::nothrow) MockSyncEngine();
    ASSERT_NE(engine, nullptr);
    auto *storage = new(std::nothrow) MockKvSyncInterface();
    ASSERT_NE(storage, nullptr);
    int incCount = 0;
    int decCount = 0;
    EXPECT_CALL(*storage, IncRefCount).WillRepeatedly([&incCount]() {
        incCount++;
    });
    EXPECT_CALL(*storage, DecRefCount).WillRepeatedly([&decCount]() {
        decCount++;
    });
    syncer.Init(engine, storage, true);
    syncer.CallQueryAutoSync(param);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s
    RuntimeContext::GetInstance()->StopTaskPool();
    EXPECT_EQ(incCount, 1);
    EXPECT_EQ(decCount, 1);
    RefObject::KillAndDecObjRef(engine);
    delete storage;
    syncer.Init(nullptr, nullptr, false);
}

/**
 * @tc.name: SyncerCheck004
 * @tc.desc: Test syncer call status check.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncerCheck004, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    EXPECT_EQ(syncer.CallStatusCheck(), -E_BUSY);
}

/**
 * @tc.name: SyncerCheck005
 * @tc.desc: Test syncer call erase watermark without storage.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncerCheck005, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    syncer.SetMetadata(metadata);
    EXPECT_EQ(syncer.EraseDeviceWaterMark("", true), -E_NOT_INIT);
    syncer.SetMetadata(nullptr);
}

/**
 * @tc.name: SyncerCheck006
 * @tc.desc: Test syncer call init with busy.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncerCheck006, TestSize.Level1)
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    auto syncDBInterface = new VirtualSingleVerSyncDBInterface();
    ASSERT_NE(syncDBInterface, nullptr);
    syncDBInterface->SetBusy(true);
    EXPECT_EQ(syncer->Initialize(syncDBInterface, false), -E_BUSY);
    syncDBInterface->SetBusy(true);
    syncer = nullptr;
    delete syncDBInterface;
}

/**
 * @tc.name: SyncerCheck007
 * @tc.desc: Test syncer get sync data size without syncer lock.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncerCheck007, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    auto mockMeta = std::make_shared<MockMetadata>();
    auto metadata = std::static_pointer_cast<Metadata>(mockMeta);
    EXPECT_CALL(*mockMeta, GetLocalWaterMark).WillRepeatedly([&syncer](const DeviceID &, uint64_t &) {
        syncer.TestSyncerLock();
    });
    syncer.SetMetadata(metadata);
    auto syncDBInterface = new VirtualSingleVerSyncDBInterface();
    ASSERT_NE(syncDBInterface, nullptr);
    syncer.Init(nullptr, syncDBInterface, true);
    size_t size = 0u;
    EXPECT_EQ(syncer.GetSyncDataSize("device", size), E_OK);
    syncer.SetMetadata(nullptr);
    delete syncDBInterface;
}

/**
 * @tc.name: SyncerCheck008
 * @tc.desc: Test syncer call set sync retry before init.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncerCheck008, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    auto syncDBInterface = new(std::nothrow) MockKvSyncInterface();
    ASSERT_NE(syncDBInterface, nullptr);
    auto engine = new (std::nothrow) MockSyncEngine();
    ASSERT_NE(engine, nullptr);
    engine->InitSubscribeManager();
    syncer.SetSyncEngine(engine);
    int incRefCount = 0;
    int decRefCount = 0;
    EXPECT_CALL(*syncDBInterface, GetDBInfo(_)).WillRepeatedly([](DBInfo &) {
    });
    EXPECT_CALL(*syncDBInterface, IncRefCount()).WillRepeatedly([&incRefCount]() {
        incRefCount++;
    });
    EXPECT_CALL(*syncDBInterface, DecRefCount()).WillRepeatedly([&decRefCount]() {
        decRefCount++;
    });
    DBInfo info;
    QuerySyncObject querySyncObject;
    std::shared_ptr<DBInfoHandleTest> handleTest = std::make_shared<DBInfoHandleTest>();
    RuntimeContext::GetInstance()->SetDBInfoHandle(handleTest);
    RuntimeContext::GetInstance()->RecordRemoteSubscribe(info, "DEVICE", querySyncObject);

    syncer.CallTriggerAddSubscribeAsync(syncDBInterface);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    RuntimeContext::GetInstance()->StopTaskPool();
    RuntimeContext::GetInstance()->SetDBInfoHandle(nullptr);
    syncer.SetSyncEngine(nullptr);

    EXPECT_EQ(incRefCount, 1);
    EXPECT_EQ(decRefCount, 1);
    RefObject::KillAndDecObjRef(engine);
    delete syncDBInterface;
}

/**
 * @tc.name: SessionId001
 * @tc.desc: Test syncer call set sync retry before init.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SessionId001, TestSize.Level1)
{
    auto context = new(std::nothrow) MockSyncTaskContext();
    ASSERT_NE(context, nullptr);
    const uint32_t sessionIdMaxValue = 0x8fffffffu;
    context->SetLastRequestSessionId(sessionIdMaxValue);
    EXPECT_LE(context->CallGenerateRequestSessionId(), sessionIdMaxValue);
    RefObject::KillAndDecObjRef(context);
}

/**
 * @tc.name: TimeSync001
 * @tc.desc: Test syncer call set sync retry before init.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, TimeSync001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(TimeSync001());
}

/**
 * @tc.name: TimeSync002
 * @tc.desc: Test syncer call set sync retry before init.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, TimeSync002, TestSize.Level1)
{
    auto *storage = new(std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_NE(storage, nullptr);
    auto *communicator = new(std::nothrow) MockCommunicator();
    ASSERT_NE(communicator, nullptr);
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();

    MockTimeSync timeSync;
    EXPECT_CALL(timeSync, SyncStart).WillRepeatedly(Return(E_OK));
    EXPECT_EQ(timeSync.Initialize(communicator, metadata, storage, "DEVICES_A"), E_OK);
    const int loopCount = 100;
    const int timeDriverMs = 10;
    for (int i = 0; i < loopCount; ++i) {
        timeSync.ModifyTimer(timeDriverMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeDriverMs));
        timeSync.CallResetTimer();
    }
    timeSync.Close();
    EXPECT_EQ(timeSync.CallIsClosed(), true);
    metadata = nullptr;
    delete storage;
    RefObject::KillAndDecObjRef(communicator);
}

/**
 * @tc.name: TimeSync003
 * @tc.desc: Test time sync cal system offset.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, TimeSync003, TestSize.Level0)
{
    TimeSyncPacket timeSyncInfo;
    const TimeOffset requestOffset = 100; // 100 is request offset
    timeSyncInfo.SetRequestLocalOffset(requestOffset);
    timeSyncInfo.SetResponseLocalOffset(0);
    timeSyncInfo.SetSourceTimeBegin(requestOffset);
    const TimeOffset rtt = 100;
    timeSyncInfo.SetTargetTimeBegin(rtt/2); // 2 is half of rtt
    timeSyncInfo.SetTargetTimeEnd(rtt/2 + 1); // 2 is half of rtt
    timeSyncInfo.SetSourceTimeEnd(requestOffset + rtt + 1);
    auto [offset, actualRtt] = MockTimeSync::CalCalculateTimeOffset(timeSyncInfo);
    EXPECT_EQ(MockTimeSync::CallCalculateRawTimeOffset(timeSyncInfo, offset), 0); // 0 is virtual delta time
    EXPECT_EQ(actualRtt, rtt);
}

/**
 * @tc.name: SyncContextCheck001
 * @tc.desc: Test context time out logic.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncContextCheck001, TestSize.Level1)
{
    auto context = new (std::nothrow) MockSyncTaskContext();
    ASSERT_NE(context, nullptr);
    context->SetTimeoutCallback([context](TimerId id) {
        EXPECT_EQ(id, 1u);
        EXPECT_EQ(context->GetUseCount(), 0);
        return E_OK;
    });
    EXPECT_EQ(context->CallTimeout(1u), E_OK);
    RefObject::KillAndDecObjRef(context);
}

/**
 * @tc.name: SingleVerDataSyncUtils001
 * @tc.desc: Test translate item got diff timestamp.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SingleVerDataSyncUtils001, TestSize.Level1)
{
    MockSyncTaskContext context;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    (void)context.Initialize("device", &dbSyncInterface, metadata, &communicator);

    std::vector<SendDataItem> data;
    for (int i = 0; i < 2; ++i) { // loop 2 times
        data.push_back(new(std::nothrow) GenericSingleVerKvEntry());
        data[i]->SetTimestamp(UINT64_MAX);
    }
    SingleVerDataSyncUtils::TransSendDataItemToLocal(&context, "", data);
    EXPECT_NE(data[0]->GetTimestamp(), data[1]->GetTimestamp());
    SingleVerKvEntry::Release(data);
}

/**
 * @tc.name: SyncTimerResetTest001
 * @tc.desc: Test it will retrurn ok when sync with a timer already exists.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBMockSyncModuleTest, SyncTimerResetTest001, TestSize.Level1) {
    MockSingleVerStateMachine stateMachine;
    MockSyncTaskContext syncTaskContext;
    MockCommunicator communicator;
    VirtualSingleVerSyncDBInterface dbSyncInterface;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);

    EXPECT_EQ(stateMachine.CallStartWatchDog(), E_OK);
    EXPECT_EQ(stateMachine.CallPrepareNextSyncTask(), E_OK);
    stateMachine.CallStopWatchDog();
}
}