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
#ifdef RUN_AS_ROOT
#include <sys/time.h>
#endif
#include <thread>

#include "db_common.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "db.h"
#include "mock_kv_sync_interface.h"
#include "mock_meta_data.h"
#include "mock_remote_executor.h"
#include "mock_single_ver_data_sync.h"
#include "mock_sync_engine.h"
#include "mock_sync_task_context.h"
#include "mock_time_sync.h"
#include "single_ver_data_sync_utils.h"
#include "single_ver_kv_syncer.h"
#include "virtual_relational_ver_sync_db_interface.h"
#include "virtual_single_ver_sync_db_Interface.h"

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
class AnotherTestKvDb {
public:
    ~AnotherTestKvDb()
    {
        LOGI("~TestKvDb");
    }
    void Initialize(IInterface *interface)
    {
        sync_.Initialize(interface, false);
        sync_.EnableAutoSync(false);
    }
    void LocalChangeEvent()
    {
        sync_.LocalDataChanged(static_cast<int>(NotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
    }
    int Close()
    {
        return sync_.Close(false);
    }
private:
    SyncerProxy sync_;
};
class TestInterface : public AnotherTestKvDb, public Interface, public RefObject {
public:
    TestInterface() {}
    ~TestInterface()
    {
        AnotherTestKvDb::Close();
    }
    void Initialize()
    {
        AnotherTestKvDb::Initialize(this);
    }
    void TestLocalChangeEvent()
    {
        AnotherTestKvDb::LocalChangeEvent();
    }
    void TestSetIdentifierEqual(std::vector<uint8_t> &identifierVec)
    {
        Interface::SetIdentifier(identifierVec);
    }

    void incRefCountSet() override
    {
        RefObject::IncObjRef(this);
    }

    void decRefCountSet() override
    {
        RefObject::DecObjRef(this);
    }
};

namespace {
using State = SingleVerSyncstateMachineer::State;
const uint32_t DBINFO_COUNT = 10u;
const uint32_t EXECUTE_COUNT = 2u;
void Init(MockSingleVerstateMachineer &stateMachineer, MockContext &Context, MockCommunicator &communicator,
    Interface &dbSyncInterface)
{
    std::shared_ptr<Metadata> meta = std::make_shared<Metadata>();
    EXCEPT_EQ(meta->Initialize(&dbSyncInterface), E_OK);
    (void)Context.Initialize("device", &dbSyncInterface, meta, &communicator);
    (void)stateMachineer.Initialize(&Context, &dbSyncInterface, meta, &communicator);
}

void Init(MockSingleVerstateMachineer &stateMachineer, MockContext *Context,
    MockCommunicator &communicator, Interface *dbSyncInterface)
{
    std::shared_ptr<Metadata> meta = std::make_shared<Metadata>();
    EXCEPT_EQ(meta->Initialize(dbSyncInterface), E_OK);
    (void)Context->Initialize("device", dbSyncInterface, meta, &communicator);
    (void)stateMachineer.Initialize(Context, dbSyncInterface, meta, &communicator);
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

int BuildRemoteQueryMsg(DistributedDB::db *&db)
{
    auto packet = RemoteExecutorReques::Create();
    if (packet == nullptr) {
        return -ERROR;
    }
    db = new (std::nothrow) DistributedDB::db(static_cast<uint32_t>(dbinfoId::REMOTE_EXECUTE_dbinfo));
    if (db == nullptr) {
        RemoteExecutorReques::Release(packet);
        return -ERROR;
    }
    db->SetdbinfoType(TYPE_REQUEST);
    packet->SetNeedResponse();
    db->SetExternalObject(packet);
    return E_OK;
}

void ConstructPacel(Parcel &parcel, uint32_t conditionCount, const std::string &key, const std::string &value)
{
    parcel.WriteUInt32(RemoteExecutorReques::REQUEST_PACKET_VERSION_V2); // version
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

void AbilitySyncTest004()
{
    /**
     * @tc.steps: step1. set table TEST is permitSync
     */
    auto *context = new (std::nothrow) SingleVerKvContext();
    ASSERT_NE(context, nullptr);
    /**
     * @tc.steps: step2. test context recv dbAbility in diff thread
     */
    const int loopCount = 1000;
    std::atomic<int> count = 0;
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::condition_variable cv;
    for (int i = 0; i < loopCount; i++) {
        std::thread t = std::thread([&context, &count, &cv] {
            DbAbility dbAbility;
            context->SetDbAbility(dbAbility);
            count++;
            if (count == loopCount) {
                cv.notify_one();
            }
        });
        t.detach();
    }
    cv.wait(lock, [&]() { return count == loopCount; });
    EXPECT_EQ(context->GetRemoteCompressAlgoStr(), "none");
    RefObject::KillAndDecObjRef(context);
}

void SyncModeTest001()
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *CommunicatorAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(CommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(CommunicatorAggregator);
    Interface *interface = new Interface();
    ASSERT_NE(interface, nullptr);
    EXPECT_EQ(syncer->Initialize(interface, true), -E_INVALID_ARGS);
    syncer->EnableAutoSync(true);
    for (int i = 0; i < 1000; i++) { // trigger 1000 times auto sync check
        syncer->LocalDataChanged(static_cast<int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT));
    }
    EXPECT_EQ(CommunicatorAggregator->GetOnlineDevices().size(), 0u);
    syncer = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete interface;
}

void SyncModeTest002()
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *CommunicatorAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(CommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(CommunicatorAggregator);
    Interface *interface = new Interface();
    ASSERT_NE(interface, nullptr);
    std::string userId = "userid_0";
    std::string storeId = "storeId_0";
    std::string appId = "appid_0";
    std::string identifier = KvStoreDelegateManager::GetKvStoreIdentifier(userId, appId, storeId);
    std::vector<uint8_t> identifierVec(identifier.begin(), identifier.end());
    interface->SetIdentifier(identifierVec);
    syncer = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete interface;
}

void SyncModeTest003()
{
    VirtualCommunicatorAggregator *CommunicatorAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(CommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(CommunicatorAggregator);
    TestInterface *interface = new TestInterface();
    ASSERT_NE(interface, nullptr);
    const std::string deviceB = "deviceB";
    std::string userId = "userId_0";
    std::string storeId = "storeId_0";
    std::string appId = "appId_0";
    std::string identifier = KvStoreDelegateManager::GetKvStoreIdentifier(userId, appId, storeId);
    std::vector<uint8_t> identifierVec(identifier.begin(), identifier.end());
    interface->TestSetIdentifierEqual(identifierVec);
    interface->Initialize();
    CommunicatorAggregator->OnlineDevice(deviceB);
    interface->TestLocalChangeEvent();
    CommunicatorAggregator->OfflineDevice(deviceB);
    EXPECT_EQ(interface->Close(), E_OK);
    RefObject::KillAndDecObjRef(interface);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    RuntimeContext::GetInstance()->StopTaskPool();
}

void MockRemoteQuery002()
{
    MockRemoteExecutor *mock = new (std::nothrow) MockRemoteExecutor();
    ASSERT_NE(mock, nullptr);
    EXPECT_EQ(mock->CallResponseFailed(0, 0, 0, "DEVICE"), -E_BUSY);
    RefObject::KillAndDecObjRef(mock);
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
    auto *tmp = new(std::nothrow) Interface();
    ASSERT_NE(tmp, nullptr);
    std::shared_ptr<Metadata> meta = std::make_shared<Metadata>();

    EXPECT_CALL(*communicator, Senddbinfo(_, _, _, _)).WillRepeatedly(Return(DB_ERROR));
    const int loopCount = 100;
    const int timeDriverMs = 100;
    for (int i = 0; i < loopCount; ++i) {
        MockTimeSync timeSync;
        EXPECT_EQ(timeSync.Initialize(communicator, meta, tmp, "DEVICES_A"), E_OK);
        EXPECT_CALL(timeSync, SyncStart).WillRepeatedly(Return(E_OK));
        timeSync.ModifyTimer(timeDriverMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeDriverMs));
        timeSync.Close();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    meta = nullptr;
    delete tmp;
    delete communicator;
}

class KvDBMockSyncModuleTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBMockSyncModuleTest::SetUpTestCase(void)
{
}

void KvDBMockSyncModuleTest::TearDownTestCase(void)
{
}

void KvDBMockSyncModuleTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCasedbinfo();
}

void KvDBMockSyncModuleTest::TearDown(void)
{
}

/**
 * @tc.name: stateMachineerCheck001
 * @tc.desc: Test machine do timeout when has same timerId.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck001, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);

    TimerId expectId = 0;
    TimerId actualId = expectId;
    EXPECT_CALL(Context, GetTimerId()).WillOnce(Return(expectId));
    EXPECT_CALL(stateMachineer, SwitchStateAndStep(_)).WillOnce(Return());

    stateMachineer.CallStepToTimeout(actualId);
}

/**
 * @tc.name: stateMachineerCheck002
 * @tc.desc: Test machine do timeout when has diff timerId.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck002, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);

    TimerId expectId = 0;
    TimerId actualId = 1;
    EXPECT_CALL(Context, GetTimerId()).WillOnce(Return(expectId));
    EXPECT_CALL(stateMachineer, SwitchStateAndStep(_)).Times(0);

    stateMachineer.CallStepToTimeout(actualId);
}

/**
 * @tc.name: stateMachineerCheck003
 * @tc.desc: Test machine exec next task when queue not empty.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck003, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);

    Context.SetLastRequestSessionId(1u);
    EXPECT_CALL(Context, IsTargetQueueEmpty()).WillRepeatedly(Return(false));
    EXPECT_CALL(Context, Clear()).WillRepeatedly([&Context]() {
        Context.SetLastRequestSessionId(0u);
    });
    EXPECT_CALL(Context, MoveToNextTarget(_)).WillRepeatedly(Return());
    EXPECT_CALL(Context, IsCurrentSyncTaskCanBeSkipped()).WillOnce(Return(true)).WillOnce(Return(false));
    // we expect machine don't change context status when queue not empty
    EXPECT_CALL(Context, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_CALL(stateMachineer, PrepareNextSyncTask()).WillOnce(Return(E_OK));
    EXPECT_CALL(Context, SetTaskExecStatus(_)).Times(0);

    EXPECT_EQ(stateMachineer.CallExecNextTask(), E_OK);
    EXPECT_EQ(Context.GetLastRequestSessionId(), 0u);
}

/**
 * @tc.name: stateMachineerCheck004
 * @tc.desc: Test machine deal time sync ack failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck004, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);

    DistributedDB::db *db = new (std::nothrow) DistributedDB::db();
    ASSERT_NE(db, nullptr);
    db->SetdbinfoType(TYPE_RESPONSE);
    db->SetSessionId(1u);
    EXPECT_CALL(Context, GetRequestSessionId()).WillRepeatedly(Return(1u));
    EXPECT_EQ(stateMachineer.CallTimeMarkSyncRecv(db), -E_INVALID_ARGS);
    EXPECT_EQ(Context.GetTaskErrCode(), -E_INVALID_ARGS);
    delete db;
}

/**
 * @tc.name: stateMachineerCheck005
 * @tc.desc: Test machine recv err.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck005, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);
    EXPECT_CALL(stateMachineer, SwitchStateAndStep(_)).WillRepeatedly(Return());
    EXPECT_CALL(Context, GetRequestSessionId()).WillRepeatedly(Return(0u));

    std::initializer_list<int> testCode = {-E_DISTRIBUTED_SCHEMA_CHANGED, -E_DISTRIBUTED_SCHEMA_NOT_FOUND};
    for (int err : testCode) {
        stateMachineer.DataRecvErrCodeHandle(0, err);
        EXPECT_EQ(Context.GetTaskErrCode(), err);
        stateMachineer.CallDataAckRecvErrCodeHandle(err, true);
        EXPECT_EQ(Context.GetTaskErrCode(), err);
    }
    EXPECT_CALL(Context, SetOperationStatus(_)).WillOnce(Return());
    stateMachineer.DataRecvErrCodeHandle(0, -E_NOT_PERMIT);
}

/**
 * @tc.name: stateMachineerCheck006
 * @tc.desc: Test machine exec next task when queue not empty to empty.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck006, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);

    Context.CallSetSyncMode(QUERY_PUSH);
    EXPECT_CALL(Context, IsTargetQueueEmpty())
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(Context, IsCurrentSyncTaskCanBeSkipped())
        .WillRepeatedly(Return(Context.CallIsCurrentSyncTaskCanBeSkipped()));
    EXPECT_CALL(Context, MoveToNextTarget(_)).WillOnce(Return());
    // we expect machine don't change context status when queue not empty
    EXPECT_CALL(Context, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_CALL(Context, SetTaskExecStatus(_)).WillOnce(Return());
    EXPECT_CALL(Context, Clear()).WillRepeatedly(Return());

    EXPECT_EQ(stateMachineer.CallExecNextTask(), -E_NO_SYNC_TASK);
}

/**
 * @tc.name: stateMachineerCheck007
 * @tc.desc: Test machine DoSaveDataNotify in another thread.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck007, TestSize.Level3)
{
    MockSingleVerstateMachineer stateMachineer;
    uint8_t callCount = 0;
    EXPECT_CALL(stateMachineer, DoSaveDataNotify(_, _, _))
        .WillRepeatedly([&callCount](uint32_t sessionId, uint32_t sequenceId, uint32_t inMsgId) {
            (void) sessionId;
            (void) sequenceId;
            (void) inMsgId;
            callCount++;
            std::this_thread::sleep_for(std::chrono::seconds(4)); // sleep 4s
        });
    stateMachineer.CallStartSaveDataNotify(0, 0, 0);
    std::this_thread::sleep_for(std::chrono::seconds(5)); // sleep 5s
    stateMachineer.CallStopSaveDataNotify();
    // timer is called once in 2s, we sleep 5s timer call twice
    EXPECT_EQ(callCount, 2);
    std::this_thread::sleep_for(std::chrono::seconds(10)); // sleep 10s to wait all thread exit
}

/**
 * @tc.name: stateMachineerCheck008
 * @tc.desc: test machine process when last sync task send packet failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck008, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);
    Context.CallCommErrHandlerFuncInner(-E_PERIPHERAL_INTERFACE_FAIL, 1u);
    EXPECT_EQ(Context.IsCommNormal(), true);
}

/**
 * @tc.name: stateMachineerCheck009
 * @tc.desc: test machine process when last sync task send packet failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck009, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);
    stateMachineer.CallSwitchMachineState(SingleVerSyncstateMachineer::Event::START_SYNC_EVENT); // START_SYNC_EVENT
    stateMachineer.CommErrAbort(1u);
    EXPECT_EQ(stateMachineer.GetCurrentState(), State::TIME_SYNC);
}

/**
 * @tc.name: stateMachineerCheck010
 * @tc.desc: test machine process when error happened in response pull.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck010, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);
    EXPECT_CALL(stateMachineer, SwitchStateAndStep(_)).WillOnce(Return());
    stateMachineer.CallResponsePullError(-E_BUSY, false);
    EXPECT_EQ(Context.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: stateMachineerCheck011
 * @tc.desc: test machine process when error happened in response pull.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck011, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);
    Context.CallSetTaskExecStatus(Context::RUNNING);
    EXPECT_CALL(Context, GetRequestSessionId()).WillOnce(Return(1u));
    Context.ClearAllSyncTask();
    EXPECT_EQ(Context.IsCommNormal(), false);
}

/**
 * @tc.name: stateMachineerCheck012
 * @tc.desc: Verify Ability LastNotify AckReceive callback.
 * @tc.type: FUNC
 * @tc.require: AR000DR9K4
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck012, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    Init(stateMachineer, Context, communicator, dbSyncInterface);
    EXPECT_CALL(stateMachineer, SwitchStateAndStep(_)).WillOnce(Return());
    DistributedDB::db msg(ABILITY_SYNC_dbinfo);
    msg.SetdbinfoType(TYPE_NOTIFY);
    AbilitySyncTestAckPacket packet;
    packet.SetProtocolVersion(ABILITY_SYNC_VERSION_V1);
    packet.SetSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    packet.SetAckCode(-E_BUSY);
    msg.SetCopiedObject(packet);
    EXPECT_EQ(stateMachineer.ReceivedbinfoCallback(&msg), E_OK);
    EXPECT_EQ(Context.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: stateMachineerCheck013
 * @tc.desc: test kill Context.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck013, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(stateMachineerCheck013());
}

/**
 * @tc.name: stateMachineerCheck014
 * @tc.desc: test machine stop save notify without start.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, stateMachineerCheck014, TestSize.Level1)
{
    MockSingleVerstateMachineer stateMachineer;
    stateMachineer.CallStopSaveDataNotify();
    EXPECT_EQ(stateMachineer.GetSaveDataNotifyRefCount(), 0);
}

/**
 * @tc.name: DataSyncCheckMock001
 * @tc.desc: Test sync recv error ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, DataSyncCheckMock001, TestSize.Level1)
{
    SingleVerDataSync sync;
    DistributedDB::db *db = new (std::nothrow) DistributedDB::db();
    ASSERT_TRUE(db != nullptr);
    db->SetErrorNo(E_FEEDBACK_COMMUNICATOR_NOT_FOUND);
    EXPECT_EQ(sync.AckPacketIdCheck(db), true);
    delete db;
}

/**
 * @tc.name: DataSyncCheckMock002
 * @tc.desc: Test sync recv notify ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, DataSyncCheckMock002, TestSize.Level1)
{
    SingleVerDataSync sync;
    DistributedDB::db *db = new (std::nothrow) DistributedDB::db();
    ASSERT_TRUE(db != nullptr);
    db->SetdbinfoType(TYPE_NOTIFY);
    EXPECT_EQ(sync.AckPacketIdCheck(db), true);
    delete db;
}
#ifdef DATA_SYNC_CHECK_003
/**
 * @tc.name: DataSyncCheckMock003
 * @tc.desc: Test sync recv notify ack.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, DataSyncCheckMock003, TestSize.Level1)
{
    MockSingleVerDataSync mockDataSync;
    MockContext mockContext;
    auto mockMetadata = std::make_shared<MockMetadata>();
    SyncTimeRange dataTimeRange = {1, 0, 1, 0};
    mockDataSync.CallUpdateSenddbinfo(dataTimeRange, &mockContext);

    VirtualRelationalVerSyncDBInterface tmp;
    MockCommunicator communicator;
    std::shared_ptr<Metadata> meta = std::static_pointer_cast<Metadata>(mockMetadata);
    mockDataSync.Initialize(&tmp, &communicator, meta, "devId");

    DistributedDB::db *db = new (std::nothrow) DistributedDB::db();
    ASSERT_TRUE(db != nullptr);
    DataAckPacket packet;
    db->SetSequenceId(1);
    db->SetCopiedObject(packet);
    mockContext.SetQuerySync(true);

    EXPECT_CALL(*mockMetadata, GetLastQueryTime(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(*mockMetadata, SetLastQueryTime(_, _, _))
        .WillOnce([&dataTimeRange](
                    const std::string &queryIdentify, const std::string &devId, const Timestamp &timestamp) {
            EXPECT_EQ(timestamp, dataTimeRange.endTime);
            return E_OK;
    });
    EXPECT_CALL(mockContext, SetOperationStatus(_)).WillOnce(Return());
    EXPECT_EQ(mockDataSync.TryContinueSync(&mockContext, db), -E_FINISHED);
    delete db;
}
#endif

/**
 * @tc.name: DataSyncCheckMock004
 * @tc.desc: Test sync do ability sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, DataSyncCheckMock004, TestSize.Level1)
{
    MockSingleVerDataSync sync;
    auto *db = new (std::nothrow) DistributedDB::db();
    ASSERT_TRUE(db != nullptr);
    db->SetdbinfoType(TYPE_NOTIFY);
    auto *context = new (std::nothrow) SingleVerKvContext();
    ASSERT_NE(context, nullptr);
    auto *communicator = new (std::nothrow) VirtualCommunicator("DEVICE", nullptr);
    ASSERT_NE(communicator, nullptr);
    sync.SetCommunicatorHandle(communicator);
    EXPECT_EQ(sync.CallDoAbilitySyncTestIfNeed(context, db, false), -E_NEED_ABILITY_SYNC);
    delete db;
    RefObject::KillAndDecObjRef(context);
    sync.SetCommunicatorHandle(nullptr);
    RefObject::KillAndDecObjRef(communicator);
}

/**
 * @tc.name: autoLaunchOptionCheck001
 * @tc.desc: Test autoLaunchOption close connection.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, autoLaunchOptionCheck001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(autoLaunchOptionCheck001());
}

/**
 * @tc.name: autoLaunchOptionCheck002
 * @tc.desc: Test autoLaunchOption receive diff userId.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, autoLaunchOptionCheck002, TestSize.Level1)
{
    autoLaunchOption autoLaunchOption;
    std::string id = "identify";
    std::string userId0 = "USER0";
    std::string userId1 = "USER1";
    autoLaunchOptionItem item;
    item.propertiesPtr = std::make_shared<KvDBProperties>();
    autoLaunchOption.SetWhiteListItem(id, userId0, item);
    bool ext = false;
    EXPECT_EQ(autoLaunchOption.CallGetautoLaunchOptionItemUid(id, userId1, ext), userId0);
    EXPECT_EQ(ext, false);
    autoLaunchOption.ClearWhiteList();
}

/**
 * @tc.name: SyncDataSync001
 * @tc.desc: Test request start when RemoveDeviceDataIfNeed failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncDataSync001, TestSize.Level1)
{
    MockContext Context;
    MockSingleVerDataSync sync;

    EXPECT_CALL(sync, RemoveDeviceDataIfNeed(_)).WillRepeatedly(Return(-E_BUSY));
    EXPECT_EQ(sync.CallRequestStart(&Context, PUSH), -E_BUSY);
    EXPECT_EQ(Context.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: SyncDataSync002
 * @tc.desc: Test pull request start when RemoveDeviceDataIfNeed failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncDataSync002, TestSize.Level1)
{
    MockContext Context;
    MockSingleVerDataSync sync;

    EXPECT_CALL(sync, RemoveDeviceDataIfNeed(_)).WillRepeatedly(Return(-E_BUSY));
    EXPECT_EQ(sync.CallPullRequestStart(&Context), -E_BUSY);
    EXPECT_EQ(Context.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: SyncDataSync003
 * @tc.desc: Test call RemoveDeviceDataIfNeed in diff thread.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncDataSync003, TestSize.Level1)
{
    MockContext Context;
    MockSingleVerDataSync sync;

    Interface tmp;
    MockCommunicator communicator;
    std::shared_ptr<MockMetadata> mockMetadata = std::make_shared<MockMetadata>();
    std::shared_ptr<Metadata> meta = std::static_pointer_cast<Metadata>(mockMetadata);
    meta->Initialize(&tmp);
    const std::string devId = "devId";
    sync.Initialize(&tmp, &communicator, meta, devId);
    Context.SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    Context.Initialize(devId, &tmp, meta, &communicator);
    Context.EnableClearRemoteStaleData(true);

    /**
     * @tc.steps: step1. set diff db createtime for rebuild label in meta
     */
    meta->SetDbCreateTime(devId, 1, true); // 1 is old db createTime
    meta->SetDbCreateTime(devId, 2, true); // 1 is new db createTime

    DistributedDB::Key k1 = {'k', '1'};
    DistributedDB::Value v1 = {'v', '1'};
    DistributedDB::Key k2 = {'k', '2'};
    DistributedDB::Value v2 = {'v', '2'};

    /**
     * @tc.steps: step2. call RemoveDeviceDataIfNeed in diff thread and then put data
     */
    std::thread thread1([&sync, &Context, &tmp, devId, k1, v1]() {
        (void)sync.CallRemoveDeviceDataIfNeed(&Context);
        tmp.PutDeviceData(devId, k1, v1);
        LOGD("PUT FINISH");
    });
    std::thread thread2([&sync, &Context, &tmp, devId, k2, v2]() {
        (void)sync.CallRemoveDeviceDataIfNeed(&Context);
        tmp.PutDeviceData(devId, k2, v2);
        LOGD("PUT FINISH");
    });
    thread1.join();
    thread2.join();

    DistributedDB::Value actualValue;
    tmp.GetDeviceData(devId, k1, actualValue);
    EXPECT_EQ(v1, actualValue);
    tmp.GetDeviceData(devId, k2, actualValue);
    EXPECT_EQ(v2, actualValue);
}

/**
 * @tc.name: AbilitySyncTest001
 * @tc.desc: Test AbilitySyncTest abort when recv error.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, AbilitySyncTest001, TestSize.Level1)
{
    MockContext Context;
    AbilitySyncTest AbilitySyncTest;

    DistributedDB::db *db = new (std::nothrow) DistributedDB::db();
    ASSERT_TRUE(db != nullptr);
    AbilitySyncTestAckPacket packet;
    packet.SetAckCode(-E_BUSY);
    db->SetCopiedObject(packet);
    EXPECT_EQ(AbilitySyncTest.AckRecv(db, &Context), -E_BUSY);
    delete db;
    EXPECT_EQ(Context.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: AbilitySyncTest002
 * @tc.desc: Test AbilitySyncTest abort when save meta failed.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, AbilitySyncTest002, TestSize.Level1)
{
    MockContext Context;
    AbilitySyncTest AbilitySyncTest;
    MockCommunicator comunicator;
    Interface interface;
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    metaData->Initialize(&interface);
    AbilitySyncTest.Initialize(&comunicator, &interface, metaData, "devId");

    /**
     * @tc.steps: step1. set AbilitySyncTestAckPacket ackCode is E_OK for pass the ack check
     */
    DistributedDB::db *db = new (std::nothrow) DistributedDB::db();
    ASSERT_TRUE(db != nullptr);
    AbilitySyncTestAckPacket packet;
    packet.SetAckCode(E_OK);
    // should set version less than 108 avoid ability sync with 2 packet
    packet.SetSoftwareVersion(SOFTWARE_VERSION_RELEASE_7_0);
    db->SetCopiedObject(packet);
    /**
     * @tc.steps: step2. set interface busy for save data return -E_BUSY
     */
    interface.SetBusy(true);
    EXPECT_CALL(Context, GetSchemaSyncStatus(_)).Times(0);
    EXPECT_EQ(AbilitySyncTest.AckRecv(db, &Context), -E_BUSY);
    delete db;
    EXPECT_EQ(Context.GetTaskErrCode(), -E_BUSY);
}

/**
 * @tc.name: AbilitySyncTest002
 * @tc.desc: Test AbilitySyncTest when offline.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, AbilitySyncTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set table TEST is permitSync
     */
    SingleVerRelationalContext *context = new (std::nothrow) SingleVerRelationalContext();
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
 * @tc.name: AbilitySyncTest004
 * @tc.desc: Test AbilitySyncTest when offline.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, AbilitySyncTest004, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(AbilitySyncTest004());
}

/**
 * @tc.name: SyncModeTest001
 * @tc.desc: Test syncer alive when thread still exist.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncModeTest001, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(SyncModeTest001());
}

/**
 * @tc.name: SyncModeTest002
 * @tc.desc: Test autosync when thread still exist.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncModeTest002, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(SyncModeTest002());
}

/**
 * @tc.name: SyncModeTest003
 * @tc.desc: Test syncer localdatachange when store is destructor
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncModeTest003, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(SyncModeTest003());
}

/**
 * @tc.name: SyncModeTest004
 * @tc.desc: Test syncer remote data change.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncModeTest004, TestSize.Level3)
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *CommunicatorAggregator = new VirtualCommunicatorAggregator();
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(CommunicatorAggregator);
    auto interface = new MockKvSyncInterface();
    int incRefCountSet = 0;
    EXPECT_CALL(*interface, incRefCountSet()).WillRepeatedly([&incRefCountSet]() {
        incRefCountSet++;
    });
    EXPECT_CALL(*interface, decRefCountSet()).WillRepeatedly(Return());
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    interface->SetIdentifier(identifier);
    syncer->Initialize(interface, true);
    syncer->EnableAutoSync(true);
    incRefCountSet = 0;
    syncer->RemoteDataChanged("");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(incRefCountSet, 2); // refCount is 2
    syncer = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete interface;
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: SyncModeTest005
 * @tc.desc: Test syncer remote device offline.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncModeTest005, TestSize.Level3)
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    VirtualCommunicatorAggregator *CommunicatorAggregator = new VirtualCommunicatorAggregator();
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(CommunicatorAggregator);
    auto interface = new MockKvSyncInterface();
    int incRefCountSet = 0;
    int dbdbinfoCount = 0;
    EXPECT_CALL(*interface, incRefCountSet()).WillRepeatedly([&incRefCountSet]() {
        incRefCountSet++;
    });
    EXPECT_CALL(*interface, decRefCountSet()).WillRepeatedly(Return());
    EXPECT_CALL(*interface, GetDBdbinfo(_)).WillRepeatedly([&dbdbinfoCount](DBdbinfo &) {
        dbdbinfoCount++;
    });
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    interface->SetIdentifier(identifier);
    syncer->Initialize(interface, true);
    syncer->Close(true);
    incRefCountSet = 0;
    dbdbinfoCount = 0;
    syncer->RemoteDeviceOffline("dev");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(incRefCountSet, 1); // refCount is 1 when remote dev offline
    EXPECT_EQ(dbdbinfoCount, 0); // dbdbinfoCount is 0 when remote dev offline
    syncer = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    delete interface;
}

/**
 * @tc.name: dbinfoScheduleTest001
 * @tc.desc: Test dbinfoSchedule stop timer when no db.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, dbinfoScheduleTest001, TestSize.Level1)
{
    MockContext *context = new MockContext();
    ASSERT_NE(context, nullptr);
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    bool last = true;
    context->OnLastRef([&last]() {
        last = true;
    });
    KvdbinfoSchedule schedule;
    bool isNeedHandle = true;
    bool isNeedContinue = true;
    schedule.MoveNextMsg(context, isNeedHandle, isNeedContinue);
    RefObject::KillAndDecObjRef(context);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(last);
}

/**
 * @tc.name: SyncInnerTest001
 * @tc.desc: Test SyncEngine receive db when closing.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncInnerTest001, TestSize.Level1)
{
    std::unique_ptr<MockSyncEngine> ptr = std::make_unique<MockSyncEngine>();
    EXPECT_CALL(*ptr, CreateContext(_))
        .WillRepeatedly(Return(new (std::nothrow) SingleVerKvContext()));
    VirtualCommunicatorAggregator *CommunicatorAggregator = new VirtualCommunicatorAggregator();
    MockKvSyncInterface interface;
    EXPECT_CALL(interface, incRefCountSet()).WillRepeatedly(Return());
    EXPECT_CALL(interface, decRefCountSet()).WillRepeatedly(Return());
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    interface.SetIdentifier(identifier);
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    metaData->Initialize(&interface);
    ASSERT_NE(CommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(CommunicatorAggregator);
    ISyncEngine::InitCallbackParam param = { nullptr, nullptr, nullptr };
    ptr->Initialize(&interface, metaData, param);
    auto communicator =
        static_cast<VirtualCommunicator *>(CommunicatorAggregator->GetCommunicator("real_device"));
    RefObject::IncObjRef(communicator);
    std::thread thread([&communicator]() {
        if (communicator == nullptr) {
            return;
        }
        for (int count = 0; count < 100; count++) { // loop 100 times
            auto *db = new (std::nothrow) DistributedDB::db();
            ASSERT_NE(db, nullptr);
            db->SetdbinfoId(LOCAL_DATA_CHANGED);
            db->SetErrorNo(E_FEEDBACK_UNKNOWN_dbinfo);
            communicator->CallbackOndbinfo("src", db);
        }
    });
    thread.join();
    LOGD("FINISHED");
    RefObject::KillAndDecObjRef(communicator);
    communicator = nullptr;
    ptr = nullptr;
    metaData = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    CommunicatorAggregator = nullptr;
}

/**
 * @tc.name: SyncInnerTest002
 * @tc.desc: Test SyncEngine add sync oper.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncInnerTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare env
     */
    auto *ptr = new (std::nothrow) MockSyncEngine();
    ASSERT_NE(ptr, nullptr);
    EXPECT_CALL(*ptr, CreateContext(_))
        .WillRepeatedly([](const ISyncInterface &) {
            return new (std::nothrow) SingleVerKvContext();
        });
    VirtualCommunicatorAggregator *CommunicatorAggregator = new VirtualCommunicatorAggregator();
    MockKvSyncInterface interface;
    int syncInterfaceRefCount = 1;
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    interface.SetIdentifier(identifier);
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    ASSERT_NE(CommunicatorAggregator, nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(CommunicatorAggregator);
    ISyncEngine::InitCallbackParam param = { nullptr, nullptr, nullptr };
    ptr->Initialize(&interface, metaData, param);
    /**
     * @tc.steps: step2. add sync oper for DEVICE_A and DEVICE_B. It will create two context for A and B
     */

    metaData = nullptr;
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    CommunicatorAggregator = nullptr;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: SyncInnerTest004
 * @tc.desc: Test SyncEngine alloc failed with null tmp.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncInnerTest004, TestSize.Level0)
{
    auto *ptr = new (std::nothrow) MockSyncEngine();
    ASSERT_NE(ptr, nullptr);
    int err = E_OK;
    auto *context = ptr->CallGetContext("dev", err);
    EXPECT_EQ(context, nullptr);
    EXPECT_EQ(err, -E_INVALID_DB);
    RefObject::KillAndDecObjRef(ptr);
}

/**
 * @tc.name: SyncInnerTest005
 * @tc.desc: Test alloc communicator with userId, test set and release equal identifier.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncInnerTest005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Get communicator aggregator and set callback to check userId.
     * @tc.expected: step1. ok
     */
    std::unique_ptr<MockSyncEngine> ptr = std::make_unique<MockSyncEngine>();
    MockKvSyncInterface interface;
    KvDBProperties kvDBProperties;
    std::string userId1 = "user_1";
    kvDBProperties.SetStringProp(DBProperties::USER_ID, userId1);
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    interface.SetIdentifier(identifier);
    interface.SetDbProperties(kvDBProperties);
    std::shared_ptr<Metadata> metaData = std::make_shared<Metadata>();
    metaData->Initialize(&interface);
    VirtualCommunicatorAggregator *CommunicatorAggregator = new VirtualCommunicatorAggregator();
    EXPECT_EQ(ptr->SetEqualIdentifier(DBCommon::TransferHashString("LABEL"), { "DEVICE" }), E_OK);
    ptr->Close();
    CommunicatorAggregator->SetReleaseCommunicatorCallback(nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    CommunicatorAggregator = nullptr;
}

/**
* @tc.name: remote query packet 001
* @tc.desc: Test RemoteExecutorReques Serialization And DeSerialization
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangqiquan
*/
HWTEST_F(KvDBMockSyncModuleTest, RemoteQueryPacket001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create RemoteExecutorReques
     */
    RemoteExecutorReques packet;
    std::map<std::string, std::string> ConditionMap = { { "test", "testsql" } };
    packet.SetExtraConditions(ConditionMap);
    packet.SetNeedResponse();
    packet.SetVersion(SOFTWARE_VERSION_RELEASE_6_0);

    /**
     * @tc.steps: step2. serialization to parcel
     */
    std::vector<uint8_t> buf(packet.CalculateLen());
    Parcel parcel(buf.data(), buf.size());
    EXCEPT_EQ(packet.Serialization(parcel), E_OK);
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
HWTEST_F(KvDBMockSyncModuleTest, RemoteQueryPacket002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create RemoteExecutorReques
     */
    RemoteExecutorAckPacket packet;
    packet.SetLastAck();
    packet.SetAckCode(-E_INTERNAL_ERROR);
    packet.SetVersion(SOFTWARE_VERSION_RELEASE_6_0);

    /**
     * @tc.steps: step2. serialization to parcel
     */
    std::vector<uint8_t> buf(packet.CalculateLen());
    Parcel parcel(buf.data(), buf.size());
    EXCEPT_EQ(packet.Serialization(parcel), E_OK);
    ASSERT_FALSE(parcel.IsError());

    /**
     * @tc.steps: step3. deserialization from parcel
     */
    RemoteExecutorAckPacket targetPacket;
    Parcel targetParcel(buf.data(), buf.size());
    EXCEPT_EQ(targetPacket.DeSerialization(targetParcel), E_OK);
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
* @tc.desc: Test RemoteExecutorReques Serialization with invalid args
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangshijie
*/
HWTEST_F(KvDBMockSyncModuleTest, RemoteQueryPacket003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. check E_INVALID_ARGS
     */
    RemoteExecutorReques packet;
    packet.SetNeedResponse();
    packet.SetVersion(SOFTWARE_VERSION_RELEASE_6_0);

    std::vector<uint8_t> buf(packet.CalculateLen());
    Parcel parcel(buf.data(), buf.size());

    EXCEPT_EQ(packet.Serialization(parcel), E_OK);
    std::map<std::string, std::string> ConditionMap = { { "test", "testsql" } };
    packet.SetExtraConditions(ConditionMap);
    EXPECT_EQ(packet.Serialization(parcel), -E_INVALID_ARGS);

    std::vector<uint8_t> buffer2(packet.CalculateLen());
    Parcel parcel2(buffer2.data(), buffer2.size());
    Parcel targetParcel2(buffer2.data(), buffer2.size());
    EXPECT_EQ(packet.Serialization(parcel2), -E_INVALID_ARGS);

    ConditionMap.erase("0");
    ConditionMap.erase("1");
    ConditionMap.erase("2");
    std::string bigKey(DBConstant::MAX_CONDITION_KEY_LEN + 1, 'a');
    ConditionMap[bigKey] = sql;
    packet.SetExtraConditions(ConditionMap);
    std::vector<uint8_t> buffer3(packet.CalculateLen());
    Parcel parcel3(buffer3.data(), buffer3.size());
    EXPECT_EQ(packet.Serialization(parcel3), -E_INVALID_ARGS);

    std::string bigValue(DBConstant::MAX_CONDITION_VALUE_LEN + 1, 'a');
    ConditionMap["1"] = bigValue;
    packet.SetExtraConditions(ConditionMap);
    std::vector<uint8_t> buffer4(packet.CalculateLen());
    Parcel parcel4(buffer4.data(), buffer4.size());
    EXPECT_EQ(packet.Serialization(parcel4), -E_INVALID_ARGS);
}

/**
* @tc.name: remote query packet 004
* @tc.desc: Test RemoteExecutorReques Deserialization with invalid args
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangshijie
*/
HWTEST_F(KvDBMockSyncModuleTest, RemoteQueryPacket004, TestSize.Level1)
{
    RemoteExecutorReques packet;
    packet.SetNeedResponse();
    packet.SetVersion(SOFTWARE_VERSION_RELEASE_6_0);

    std::vector<uint8_t> buf(packet.CalculateLen());
    RemoteExecutorReques targetPacket;
    Parcel targetParcel(buf.data(), 3); // 3 is invalid len for deserialization
    EXPECT_EQ(targetPacket.DeSerialization(targetParcel), -E_INVALID_ARGS);

    Parcel parcel2(buffer.data(), buffer.size());
    std::string bigKey(DBConstant::MAX_CONDITION_KEY_LEN + 1, 'a');
    ConstructPacel(parcel2, 1, bigKey, "");
    Parcel desParcel2(buffer.data(), buffer.size());
    EXPECT_EQ(targetPacket.DeSerialization(desParcel2), -E_INVALID_ARGS);

    Parcel parcel3(buffer.data(), buffer.size());
    std::string bigValue(DBConstant::MAX_CONDITION_VALUE_LEN + 1, 'a');
    ConstructPacel(parcel3, 1, "1", bigValue);
    Parcel desParcel3(buffer.data(), buffer.size());
    EXPECT_EQ(targetPacket.DeSerialization(desParcel3), -E_INVALID_ARGS);

    Parcel parcel5(buffer.data(), buffer.size());
    ConstructPacel(parcel5, 0, "", "");
    Parcel desParcel5(buffer.data(), buffer.size());
    EXPECT_EQ(targetPacket.DeSerialization(desParcel5), E_OK);
}

/**
 * @tc.name: SingleVerKvEntryTest001
 * @tc.desc: Test SingleVerKvEntry Serialize and DeSerialize.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SingleVerKvEntryTest001, TestSize.Level1)
{
    std::vector<SingleVerKvEntry *> kvEntries;
    size_t len = 0u;
    for (size_t i = 0; i < DBConstant::MAX_NORMAL_PACK_ITEM_SIZE + 1; ++i) {
        auto entryPtr = new GenericSingleVerKvEntry();
        kvEntries.push_back(entryPtr);
        len += entryPtr->CalculateLen(SOFTWARE_VERSION_CURRENT);
        len = BYTE_8_ALIGN(len);
    }
    std::vector<uint8_t> data(len, 0);
    Parcel parcel(data.data(), data.size());
    EXPECT_EQ(GenericSingleVerKvEntry::SerializeDatas(kvEntries, parcel, SOFTWARE_VERSION_CURRENT), E_OK);
    parcel = Parcel(data.data(), data.size());
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
HWTEST_F(KvDBMockSyncModuleTest, MockRemoteQuery001, TestSize.Level3)
{
    MockRemoteExecutor *mock = new (std::nothrow) MockRemoteExecutor();
    ASSERT_NE(mock, nullptr);
    uint32_t count = 0u;
    EXPECT_CALL(*mock, ParseOneRequestdbinfo).WillRepeatedly(
        [&count](const std::string &device, DistributedDB::db *inMsg) {
            std::this_thread::sleep_for(std::chrono::seconds(5)); // mock one msg execute 5 s
            count++;
    });
    EXPECT_CALL(*mock, IsPacketValid).WillRepeatedly(Return(true));
    for (uint32_t i = 0; i < DBINFO_COUNT; i++) {
        DistributedDB::db *db = nullptr;
        EXPECT_EQ(BuildRemoteQueryMsg(db), E_OK);
        mock->Receivedbinfo("DEVICE", db);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    mock->Close();
    EXPECT_EQ(count, EXECUTE_COUNT);
    RefObject::KillAndDecObjRef(mock);
}

/**
* @tc.name: mock remote query 002
* @tc.desc: Test RemoteExecutor response failed when closing
* @tc.type: FUNC
* @tc.require: AR000GK58G
* @tc.author: zhangqiquan
*/
HWTEST_F(KvDBMockSyncModuleTest, MockRemoteQuery002, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(MockRemoteQuery002());
}

/**
 * @tc.name: ContextCheck001
 * @tc.desc: test context check task can be skipped in push mode.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, ContextCheck001, TestSize.Level1)
{
    MockContext Context;
    MockCommunicator communicator;
    Interface dbSyncInterface;
    std::shared_ptr<Metadata> meta = std::make_shared<Metadata>();
    Context.CallSetSyncMode(static_cast<int>(SyncModeType::PUSH));
    EXPECT_EQ(Context.CallIsCurrentSyncTaskCanBeSkipped(), true);
}

/**
 * @tc.name: ContextCheck002
 * @tc.desc: test context check task can be skipped in push mode.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, ContextCheck002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create context and oper
     */
    auto Context = new(std::nothrow) MockContext();
    ASSERT_NE(Context, nullptr);
    oper->SetQuery(querySyncObject);
    Context->SetSyncOperation(oper);
    Context->SetLastFullSyncTaskStatus(SyncOperation::Status::OP_FAILED);
    Context->CallSetSyncMode(static_cast<int>(SyncModeType::QUERY_PUSH));
    EXPECT_CALL(*Context, IsTargetQueueEmpty()).WillRepeatedly(Return(false));

    const int loopCount = 1000;
    std::thread readThread([&Context]() {
        for (int i = 0; i < loopCount; ++i) {
            EXPECT_EQ(Context->CallIsCurrentSyncTaskCanBeSkipped(), false);
        }
    });
    writeThread.join();
    clearThread.join();
    readThread.join();
    RefObject::KillAndDecObjRef(oper);
    Context->SetSyncOperation(nullptr);
    RefObject::KillAndDecObjRef(Context);
}

/**
 * @tc.name: ContextCheck003
 * @tc.desc: test context call on sync task add.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, ContextCheck003, TestSize.Level1)
{
    auto *Context = new (std::nothrow) MockContext();
    Context->CallSetTaskExecStatus(DistributedDB::IContext::RUNNING);
    int callCount = 0;
    std::condition_variable cv;
    std::mutex Mutex;
    EXPECT_EQ(Context->AddSyncTarget(nullptr), -E_INVALID_ARGS);
    auto target = new (std::nothrow) SingleVerSyncTarget();
    ASSERT_NE(target, nullptr);
    target->SetTaskType(ISyncTarget::REQUEST);
    EXPECT_EQ(Context->AddSyncTarget(target), E_OK);
    {
        std::unique_lock<std::mutex> lock(Mutex);
        cv.wait_for(lock, std::chrono::seconds(5), [&callCount]() { // wait 5s
            return callCount > 0;
        });
    }
    EXPECT_EQ(callCount, 1);
    RefObject::KillAndDecObjRef(Context);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: ContextCheck004
 * @tc.desc: test context add sync task should not cancel current task.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, ContextCheck004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create context and target
     */
    auto *text = new (std::nothrow) MockContext();
    ASSERT_NE(text, nullptr);
    int beforeRetryTime = text->GetRetryTime();
    auto target = new (std::nothrow) SingleVerSyncTarget();
    ASSERT_NE(target, nullptr);
    target->SetTaskType(ISyncTarget::REQUEST);
    text->SetAutoSync(true);
    /**
     * @tc.steps: step2. add target
     * @tc.expected: retry time should not be changed
     */
    EXPECT_EQ(text->AddSyncTarget(target), E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    int afterRetryTime = text->GetRetryTime();
    EXPECT_EQ(beforeRetryTime, afterRetryTime);
    /**
     * @tc.steps: step3. release resource
     */
    RefObject::KillAndDecObjRef(text);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: ContextCheck005
 * @tc.desc: test context call get query id async.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, ContextCheck005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare context
     */
    auto context = new (std::nothrow) SingleVerRelationalContext();
    ASSERT_NE(context, nullptr);
    SingleVerSyncstateMachineer stateMachineer;
    VirtualCommunicator communicator("device", nullptr);
    Interface dbSync;
    std::shared_ptr<Metadata> meta = std::make_shared<Metadata>();
    EXCEPT_EQ(meta->Initialize(&dbSync), E_OK);
    (void)context->Initialize("device", &dbSync, meta, &communicator);
    (void)stateMachineer.Initialize(context, &dbSync, meta, &communicator);
    std::thread getThread([context]() {
        for (int i = 0; i < 100; ++i) { // get 100 times
            (void) context->GetDeleteSyncId();
            (void) context->GetQuerySyncId();
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
 * @tc.name: ContextCheck006
 * @tc.desc: test context call get query id async.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, ContextCheck006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. prepare context
     */
    auto context = new (std::nothrow) SingleVerRelationalContext();
    ASSERT_NE(context, nullptr);
    auto communicator = new (std::nothrow) VirtualCommunicator("device", nullptr);
    ASSERT_NE(communicator, nullptr);
    (void)context->Initialize("device", &dbSyncInterface, meta, communicator);
    /**
     * @tc.steps: step2. add sync target into context
     */
    auto target = new (std::nothrow) SingleVerSyncTarget();
    ASSERT_NE(target, nullptr);
    context->SetRetryTime(AUTO_RETRY_TIMES);
    context->MoveToNextTarget(DBConstant::MIN_TIMEOUT);
    EXPECT_EQ(context->GetRetryTime(), 0);
    context->Clear();
    RefObject::KillAndDecObjRef(context);
    RefObject::KillAndDecObjRef(communicator);
}

/**
 * @tc.name: ContextCheck007
 * @tc.desc: test get query sync id for field sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(KvDBMockSyncModuleTest, ContextCheck007, TestSize.Level0)
{
    /**
     * @tc.steps: step1. prepare context
     * @tc.expected: OK
     */
    auto context = new (std::nothrow) SingleVerRelationalContext();
    ASSERT_NE(context, nullptr);
    SingleVerSyncstateMachineer stateMachineer;
    VirtualCommunicator communicator("device", nullptr);
    VirtualRelationalVerSyncDBInterface Interface;
    std::shared_ptr<Metadata> meta = std::make_shared<Metadata>();
    EXCEPT_EQ(meta->Initialize(&Interface), E_OK);
    (void)context->Initialize("device", &Interface, meta, &communicator);
    (void)stateMachineer.Initialize(context, &Interface, meta, &communicator);
    /**
     * @tc.steps: step2. prepare table and query
     * @tc.expected: OK
     */
    Fielddbinfo field;
    field.SetFieldName("abc");
    field.SetColumnId(0);
    Tabledbinfo table;
    table.SetTableName("tableA");
    table.AddField(field);
    RelationalSchemaObject schemaObj;
    schemaObj.AddRelationalTable(table);
    Interface.SetSchemadbinfo(schemaObj);
    QuerySyncObject query;
    query.SetTableName("tableA");
    context->SetQuery(query);
    /**
     * @tc.steps: step3. get and check queryId
     * @tc.expected: OK
     */
    context->SetRemoteSoftwareVersion(SOFTWARE_VERSION_CURRENT);
    std::string expectQuerySyncId = DBCommon::TransferStringToHex(DBCommon::TransferHashString("abc"));
    std::string actualQuerySyncId = context->GetQuerySyncId();
    EXPECT_EQ(expectQuerySyncId, actualQuerySyncId);
    context->Clear();
    RefObject::KillAndDecObjRef(context);
}

#ifdef RUN_AS_ROOT
/**
 * @tc.name: TimeChangeListenerTest001
 * @tc.desc: Test RegisterTimeChangedLister.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, TimeChangeListenerTest001, TestSize.Level1)
{
    SingleVerKVSyncer syncer;
    Interface interface;
    KvDBProperties properties;
    properties.SetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, true);
    interface.SetDbProperties(properties);
    EXPECT_EQ(syncer.Initialize(&interface, false), -E_NO_NEED_ACTIVE);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s wait for time tick
    const std::string LOCAL_TIME_OFFSET_KEY = "localTimeOffset";
    std::vector<uint8_t> key;
    DBCommon::StringToVector(LOCAL_TIME_OFFSET_KEY, key);
    std::vector<uint8_t> beforeOffset;
    EXPECT_EQ(interface.GetMetaData(key, beforeOffset), E_OK);
    ChangeTime(2); // increase 2s
    std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s wait for time tick
    std::vector<uint8_t> afterOffset;
    EXPECT_EQ(interface.GetMetaData(key, afterOffset), E_OK);
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
HWTEST_F(KvDBMockSyncModuleTest, TimeChangeListenerTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create syncer without activation
     */
    MockSingleVerKVSyncer syncer;
    Interface interface;
    KvDBProperties properties;
    properties.SetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE, true);
    interface.SetDbProperties(properties);
    /**
     * @tc.steps: step2. trigger time change logic and reopen syncer at the same time
     * @tc.expected: no crash here
     */
    const int loopCount = 100;
    std::thread thread([&syncer]() {
        for (int i = 0; i < loopCount; ++i) {
            int64_t changeOffset = 1;
            syncer.CallRecordTimeChangeOffset(&changeOffset);
        }
    });
    for (int i = 0; i < loopCount; ++i) {
        EXPECT_EQ(syncer.Initialize(&interface, false), -E_NO_NEED_ACTIVE);
        EXPECT_EQ(syncer.Close(true), -E_NOT_INIT);
    }
    thread.join();
}

/**
 * @tc.name: SyncerCheck001
 * @tc.desc: Test syncer call set sync retry before init.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncerCheck001, TestSize.Level1)
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
HWTEST_F(KvDBMockSyncModuleTest, SyncerCheck002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create context and syncer
     */
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    auto CommunicatorAggregator = new(std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_NE(CommunicatorAggregator, nullptr);
    auto interface = new Interface();
    ASSERT_NE(interface, nullptr);
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    interface->SetIdentifier(identifier);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(CommunicatorAggregator);
    /**
     * @tc.steps: step2. get timestamp by syncer over and over again
     */
    std::atomic<bool> isfinish = false;
    std::thread t([&isfinish, &syncer]() {
        while (!isfinish) {
            (void) syncer->GetTimestamp();
        }
    });
    /**
     * @tc.steps: step3. re init syncer over and over again
     * @tc.expected: step3. dont crash here.
     */
    for (int i = 0; i < 100; ++i) { // loop 100 times
        syncer->Initialize(interface, false);
        syncer->Close(true);
    }
    isfinish = true;
    t.join();
    delete interface;
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
HWTEST_F(KvDBMockSyncModuleTest, DISABLE_SyncerCheck003, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    InternalSyncParma param;
    auto *engine = new(std::nothrow) MockSyncEngine();
    ASSERT_NE(engine, nullptr);
    auto *tmp = new(std::nothrow) MockKvSyncInterface();
    ASSERT_NE(tmp, nullptr);
    int incCount = 0;
    int decCount = 0;
    EXPECT_CALL(*tmp, incRefCountSet).WillRepeatedly([&incCount]() {
        incCount++;
    });
    EXPECT_CALL(*tmp, decRefCountSet).WillRepeatedly([&decCount]() {
        decCount++;
    });
    syncer.Init(engine, tmp, true);
    syncer.CallQueryAutoSync(param);
    std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep 2s
    RuntimeContext::GetInstance()->StopTaskPool();
    EXPECT_EQ(incCount, 1);
    EXPECT_EQ(decCount, 1);
    RefObject::KillAndDecObjRef(engine);
    delete tmp;
}

/**
 * @tc.name: SyncerCheck004
 * @tc.desc: Test syncer call status check.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncerCheck004, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    EXPECT_EQ(syncer.CallStatusCheck(), -E_BUSY);
}

/**
 * @tc.name: SyncerCheck005
 * @tc.desc: Test syncer call erase watermark without tmp.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncerCheck005, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    std::shared_ptr<Metadata> meta = std::make_shared<Metadata>();
    syncer.SetMetadata(meta);
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
HWTEST_F(KvDBMockSyncModuleTest, SyncerCheck006, TestSize.Level1)
{
    std::shared_ptr<SingleVerKVSyncer> syncer = std::make_shared<SingleVerKVSyncer>();
    auto interface = new Interface();
    ASSERT_NE(interface, nullptr);
    interface->SetBusy(false);
    EXPECT_EQ(syncer->Initialize(interface, false), -E_BUSY);
    interface->SetBusy(false);
    syncer = nullptr;
    delete interface;
}

/**
 * @tc.name: SyncerCheck007
 * @tc.desc: Test syncer get sync data size without syncer lock.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncerCheck007, TestSize.Level1)
{
    MockSingleVerKVSyncer KVSyncer;
    auto mockMeta = std::make_shared<MockMetadata>();
    auto meta = std::static_pointer_cast<Metadata>(mockMeta);
    EXPECT_CALL(*mockMeta, GetLocalWaterMark).WillRepeatedly([&KVSyncer](const DeviceID &, uint64_t &) {
        KVSyncer.TestSyncerLock();
    });
    KVSyncer.SetMetadata(meta);
    auto interface = new Interface();
    ASSERT_NE(interface, nullptr);
    KVSyncer.Init(nullptr, interface, false);
    size_t size = 0u;
    EXPECT_EQ(KVSyncer.GetSyncDataSize("device", size), E_OK);
    KVSyncer.SetMetadata(nullptr);
    delete interface;
}

/**
 * @tc.name: SyncerCheck008
 * @tc.desc: Test syncer call set sync retry before init.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBMockSyncModuleTest, SyncerCheck008, TestSize.Level1)
{
    MockSingleVerKVSyncer syncer;
    auto interface = new(std::nothrow) MockKvSyncInterface();
    ASSERT_NE(interface, nullptr);
    auto engine = new (std::nothrow) MockSyncEngine();
    ASSERT_NE(engine, nullptr);
    engine->InitSubscribeManager();
    syncer.SetSyncEngine(engine);
    int incRefCountSet = 0;
    int decRefCountSet = 0;
    EXPECT_CALL(*interface, GetDBdbinfo(_)).WillRepeatedly([](DBdbinfo &) {
    });
    EXPECT_CALL(*interface, incRefCountSet()).WillRepeatedly([&incRefCountSet]() {
        incRefCountSet++;
    });
    EXPECT_CALL(*interface, decRefCountSet()).WillRepeatedly([&decRefCountSet]() {
        decRefCountSet++;
    });
    DBdbinfo db;
    QuerySyncObject querySyncObject;
    std::shared_ptr<DBdbinfoHandleTest> handleTest = std::make_shared<DBdbinfoHandleTest>();
    RuntimeContext::GetInstance()->SetDBdbinfoHandle(handleTest);
    RuntimeContext::GetInstance()->RecordRemoteSubscribe(db, "DEVICE", querySyncObject);

    syncer.CallTriggerAddSubscribeAsync(interface);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    RuntimeContext::GetInstance()->StopTaskPool();
    RuntimeContext::GetInstance()->SetDBdbinfoHandle(nullptr);
    syncer.SetSyncEngine(nullptr);

    EXPECT_EQ(incRefCountSet, 1);
    EXPECT_EQ(decRefCountSet, 1);
    RefObject::KillAndDecObjRef(engine);
    delete interface;
}
}
