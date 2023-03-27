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
#include "mock_meta_data.h"
#include "mock_remote_executor.h"
#include "mock_single_ver_data_sync.h"
#include "mock_single_ver_state_machine.h"
#include "mock_sync_engine.h"
#include "mock_sync_task_context.h"
#include "mock_time_sync.h"
#include "remote_executor_packet.h"
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

namespace {
const uint32_t MESSAGE_COUNT = 10u;
const uint32_t EXECUTE_COUNT = 2u;
void Init(MockSingleVerStateMachine &stateMachine, MockSyncTaskContext &syncTaskContext,
    MockCommunicator &communicator, VirtualSingleVerSyncDBInterface &dbSyncInterface)
{
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
    (void)syncTaskContext.Initialize("device", &dbSyncInterface, metadata, &communicator);
    (void)stateMachine.Initialize(&syncTaskContext, &dbSyncInterface, metadata, &communicator);
}

void Init(MockSingleVerStateMachine &stateMachine, MockSyncTaskContext *syncTaskContext,
    MockCommunicator &communicator, VirtualSingleVerSyncDBInterface *dbSyncInterface)
{
    std::shared_ptr<Metadata> metadata = std::make_shared<Metadata>();
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

    EXPECT_CALL(stateMachine, PrepareNextSyncTask()).WillOnce(Return(E_OK));

    EXPECT_CALL(syncTaskContext, IsTargetQueueEmpty()).WillRepeatedly(Return(false));
    EXPECT_CALL(syncTaskContext, MoveToNextTarget()).WillRepeatedly(Return());
    EXPECT_CALL(syncTaskContext, IsCurrentSyncTaskCanBeSkipped())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    // we expect machine don't change context status when queue not empty
    EXPECT_CALL(syncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_CALL(syncTaskContext, SetTaskExecStatus(_)).Times(0);

    EXPECT_EQ(stateMachine.CallExecNextTask(), E_OK);
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

    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
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
    EXPECT_CALL(syncTaskContext, Clear()).WillOnce(Return());

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
    stateMachine.CallSwitchMachineState(1u); // START_SYNC_EVENT
    stateMachine.CommErrAbort(1u);
    EXPECT_EQ(stateMachine.GetCurrentState(), 1u);
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
 * @tc.name: StateMachineCheck013
 * @tc.desc: test kill syncTaskContext.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, StateMachineCheck013, TestSize.Level1)
{
    MockSingleVerStateMachine stateMachine;
    auto *syncTaskContext = new(std::nothrow) MockSyncTaskContext();
    auto *dbSyncInterface = new(std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_NE(syncTaskContext, nullptr);
    EXPECT_NE(dbSyncInterface, nullptr);
    if (dbSyncInterface == nullptr) {
        RefObject::KillAndDecObjRef(syncTaskContext);
        return;
    }
    MockCommunicator communicator;
    Init(stateMachine, syncTaskContext, communicator, dbSyncInterface);
    EXPECT_CALL(*syncTaskContext, Clear()).WillOnce(Return());
    syncTaskContext->RegForkGetDeviceIdFunc([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep 2s
    });
    int token = 1;
    int *tokenPtr = &token;
    syncTaskContext->SetContinueToken(tokenPtr);
    RefObject::KillAndDecObjRef(syncTaskContext);
    delete dbSyncInterface;
    std::this_thread::sleep_for(std::chrono::seconds(5)); // sleep 5s and wait for task exist
    tokenPtr = nullptr;
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
    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
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
    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
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

    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    DataAckPacket packet;
    message->SetSequenceId(1);
    message->SetCopiedObject(packet);
    mockSyncTaskContext.SetQuerySync(true);

    EXPECT_CALL(*mockMetadata, GetLastQueryTime(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(*mockMetadata, SetLastQueryTime(_, _, _)).WillOnce([&dataTimeRange](const std::string &queryIdentify,
        const std::string &deviceId, const Timestamp &timestamp) {
        EXPECT_EQ(timestamp, dataTimeRange.endTime);
        return E_OK;
    });
    EXPECT_CALL(mockSyncTaskContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_EQ(mockDataSync.TryContinueSync(&mockSyncTaskContext, message), -E_FINISHED);
    delete message;
}
#endif
/**
 * @tc.name: AutoLaunchCheck001
 * @tc.desc: Test autoLaunch close connection.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, AutoLaunchCheck001, TestSize.Level1)
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
    cv.wait(lock, [&finishCount, &loopCount]() {
        return finishCount == loopCount;
    });
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
    std::thread thread1([&syncTaskContext, &storage, &dataSync, &deviceId, &k1, &v1]() {
        (void)dataSync.CallRemoveDeviceDataIfNeed(&syncTaskContext);
        storage.PutDeviceData(deviceId, k1, v1);
        LOGD("PUT FINISH");
    });
    std::thread thread2([&syncTaskContext, &storage, &dataSync, &deviceId, &k2, &v2]() {
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

    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
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
    DistributedDB::Message *message = new(std::nothrow) DistributedDB::Message();
    ASSERT_TRUE(message != nullptr);
    AbilitySyncAckPacket packet;
    packet.SetAckCode(E_OK);
    packet.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    message->SetCopiedObject(packet);
    /**
     * @tc.steps: step2. set syncDBInterface busy for save data return -E_BUSY
     */
    syncDBInterface.SetBusy(true);
    SyncStrategy mockStrategy = {true, false, false};
    EXPECT_CALL(syncTaskContext, GetSyncStrategy(_)).WillOnce(Return(mockStrategy));
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
    context->SetRelationalSyncStrategy(strategy);
    QuerySyncObject query;
    query.SetTableName(tableName);
    /**
     * @tc.steps: step2. set table is need reset ability sync but it still permit sync
     */
    context->SetIsNeedResetAbilitySync(true);
    EXPECT_EQ(context->GetSyncStrategy(query).permitSync, true);
    /**
     * @tc.steps: step3. set table is schema change now it don't permit sync
     */
    context->SchemaChange();
    EXPECT_EQ(context->GetSyncStrategy(query).permitSync, false);
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
        std::thread t = std::thread([&context, &finishCount, &loopCount, &cv] {
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
    RefObject::KillAndDecObjRef(context);
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
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    VirtualSingleVerSyncDBInterface *syncDBInterface = new VirtualSingleVerSyncDBInterface();
    syncer->Initialize(syncDBInterface, true);
    syncer->EnableAutoSync(true);
    for (int i = 0; i < 1000; i++) { // trigger 1000 times auto sync check
        syncer->LocalDataChanged(SQLITE_GENERAL_NS_PUT_EVENT);
    }
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
    EXPECT_CALL(*enginePtr, CreateSyncTaskContext()).WillRepeatedly(Return(nullptr));
    VirtualCommunicatorAggregator *virtualCommunicatorAggregator = new VirtualCommunicatorAggregator();
    VirtualSingleVerSyncDBInterface syncDBInterface;
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    ASSERT_NE(virtualCommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(virtualCommunicatorAggregator);
    EXPECT_EQ(enginePtr->Initialize(nullptr, metaData, nullptr, nullptr, nullptr), -E_INVALID_ARGS);
    std::shared_ptr<Metadata> nullMetaData = nullptr;
    EXPECT_EQ(enginePtr->Initialize(&syncDBInterface, nullMetaData, nullptr, nullptr, nullptr), -E_INVALID_ARGS);
    enginePtr->Initialize(&syncDBInterface, metaData, nullptr, nullptr, nullptr);
    auto communicator =
        static_cast<VirtualCommunicator *>(virtualCommunicatorAggregator->GetCommunicator("real_device"));
    RefObject::IncObjRef(communicator);
    std::thread thread1([&communicator]() {
        if (communicator == nullptr) {
            return;
        }
        for (int count = 0; count < 100; count++) { // loop 100 times
            auto *message = new(std::nothrow) DistributedDB::Message();
            communicator->CallbackOnMessage("src", message);
        }
    });
    std::thread thread2([&enginePtr]() {
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
    MockRemoteExecutor *executor = new(std::nothrow) MockRemoteExecutor();
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
    MockRemoteExecutor *executor = new(std::nothrow) MockRemoteExecutor();
    ASSERT_NE(executor, nullptr);
    executor->CallResponseFailed(0, 0, 0, "DEVICE");
    RefObject::KillAndDecObjRef(executor);
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
 * @tc.name: TimeSync001
 * @tc.desc: Test syncer call set sync retry before init.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBMockSyncModuleTest, TimeSync001, TestSize.Level1)
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
        timeSync.Initialize(communicator, metadata, storage, "DEVICES_A");
        timeSync.ModifyTimer(timeDriverMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeDriverMs));
        timeSync.Close();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    metadata = nullptr;
    delete storage;
    delete communicator;
}
