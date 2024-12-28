/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * kv under the License is kv on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <new>
#include <thread>
#include "db_errno.h"
#include "kvdb_communicator_common.h"
#include "kvdb_tools_unit_test.h"
#include "log_print.h"
#include "network_adapterTest.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "message.h"
#include "mock_process_communicator.h"
#include "serial_buffer.h"

using namespace std;
using namespace testing::ext;
using namespace KvDB;

namespace {
    EnvHandle g_envDevice1Test;
    EnvHandle g_envDevice2Test;
    EnvHandle g_envDevice3Test;
    CommunicatorTest *g_comm11 = nullptr;
    CommunicatorTest *g_comm12 = nullptr;
    CommunicatorTest *g_comm13 = nullptr;
    CommunicatorTest *g_comm14 = nullptr;
    CommunicatorTest *g_comm15 = nullptr;
    CommunicatorTest *g_comm16 = nullptr;
}

class KvDBCommunicatorDeepTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBCommunicatorDeepTest::SetUpTestCase(void)
{
    LOGI("[UT][DeepTest][SetUpTestCase] Enter.");
    bool errCodeTest = SetUpEnv(g_envDevice1Test, DEVICE_NAME_A);
    EXPECT_EQ(errCodeTest, true);
    errCodeTest = SetUpEnv(g_envDevice2Test, DEVICE_NAME_B);
    EXPECT_EQ(errCodeTest, true);
    errCodeTest = SetUpEnv(g_envDevice3Test, DEVICE_NAME_C);
    EXPECT_EQ(errCodeTest, true);
    DoRegTransformFunction();
    CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(false);
}

void KvDBCommunicatorDeepTest::TearDownTestCase(void)
{
    LOGI("[UT][DeepTest][TearDownTestCase] Enter.");
    std::this_thread::sleep_for(std::chrono::seconds(7)); // Wait 7 s to make sure all thread quiet
    TearDownEnv(g_envDevice1Test);
    TearDownEnv(g_envDevice2Test);
    TearDownEnv(g_envDevice3Test);
    CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(true);
}

namespace {
void AllocAllCommunicator()
{
    int errorNo = E_OK;
    g_comm11 = g_envDevice1Test.commAggrHandle->AllocCommunicator(LABEL1, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_comm11, "");
    g_comm12 = g_envDevice1Test.commAggrHandle->AllocCommunicator(LABEL2, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_comm12, "");
    g_comm13 = g_envDevice2Test.commAggrHandle->AllocCommunicator(LABEL2, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_comm13, "");
    g_comm14 = g_envDevice2Test.commAggrHandle->AllocCommunicator(LABEL3, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_comm14, "");
    g_comm15 = g_envDevice3Test.commAggrHandle->AllocCommunicator(LABEL3, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_comm15, "");
    g_comm16 = g_envDevice3Test.commAggrHandle->AllocCommunicator(LABEL1, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_comm16, "");
}

void ReleaseAllCommunicator()
{
    g_envDevice1Test.commAggrHandle->ReleaseCommunicator(g_comm11);
    g_comm11 = nullptr;
    g_envDevice1Test.commAggrHandle->ReleaseCommunicator(g_comm12);
    g_comm12 = nullptr;
    g_envDevice2Test.commAggrHandle->ReleaseCommunicator(g_comm13);
    g_comm13 = nullptr;
    g_envDevice2Test.commAggrHandle->ReleaseCommunicator(g_comm14);
    g_comm14 = nullptr;
    g_envDevice3Test.commAggrHandle->ReleaseCommunicator(g_comm15);
    g_comm15 = nullptr;
    g_envDevice3Test.commAggrHandle->ReleaseCommunicator(g_comm16);
    g_comm16 = nullptr;
}
}

void KvDBCommunicatorDeepTest::SetUp()
{
    KvDBUnitTest::KvDBToolsUnitTest::PrintTestCaseInfo();
    AllocAllCommunicator();
}

void KvDBCommunicatorDeepTest::TearDown()
{
    ReleaseAllCommunicator();
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait 200 ms to make sure all thread quiet
}

/**
 * @tc.name: WaitAndRetrySend 001
 * @tc.desc: Test send retry semantic
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 */
HWTEST_F(KvDBCommunicatorDeepTest, WaitAndRetrySend001Virtual, TestSize.Level2)
{
    // Preset
    Message *msgForTest1 = nullptr;
    g_comm13->RegOnMessageCallback([&msgForTest1](const std::string &srcTarget, Message *inMsg) {
        msgForTest1 = inMsg;
    }, nullptr);
    Message *msgForTest2 = nullptr;
    g_comm16->RegOnMessageCallback([&msgForTest2](const std::string &srcTarget, Message *inMsg) {
        msgForTest2 = inMsg;
    }, nullptr);

    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice3Test.adaHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait 200 ms to make sure quiet

    g_envDevice1Test.adaHandle->SimulateSendRetry(DEVICE_NAME_B);

    Message *msgForTest3 = BuildRegedTinyMessage();
    ASSERT_NE(msgForTest3, nullptr);
    SendConfig confin = {true, false, 0};
    int errCodeTest = g_comm12->SendMessage(DEVICE_NAME_B, msgForTest3, confin);
    EXPECT_EQ(errCodeTest, E_OK);

    Message *msgForTest4 = BuildRegedTinyMessage();
    ASSERT_NE(msgForTest4, nullptr);
    errCodeTest = g_comm11->SendMessage(DEVICE_NAME_C, msgForTest4, confin);
    EXPECT_EQ(errCodeTest, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait 100 ms
    EXPECT_EQ(msgForTest1, nullptr);
    EXPECT_NE(msgForTest2, nullptr);
    delete msgForTest2;
    msgForTest2 = nullptr;

    g_envDevice1Test.adaHandle->SimulateSendRetryClear(DEVICE_NAME_B);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait 100 ms
    EXPECT_NE(msgForTest1, nullptr);
    delete msgForTest1;
    msgForTest1 = nullptr;

    // CleanUp
    AdapterStub::DisconnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
}

static int CreateBufferThenAddIntoScheduler(SendTaskScheduler &schedulerTest, const std::string &dstTarget)
{
    SerialBuffer *serBuff = new (std::nothrow) SerialBuffer();
    if (serBuff == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    int errCodeTest = serBuff->AllocBufferByTotalLength(100, 0); // 100 totallen without header
    if (errCodeTest != E_OK) {
        delete serBuff;
        serBuff = nullptr;
        return errCodeTest;
    }
    SendTask task{serBuff, dstTarget, nullptr, 0u};
    errCodeTest = schedulerTest.AddSendTaskIntoSchedule(task, inPrio);
    if (errCodeTest != E_OK) {
        delete serBuff;
        serBuff = nullptr;
        return errCodeTest;
    }
    return E_OK;
}

/**
 * @tc.name: SendSchedule 001
 * @tc.desc: Test schedule in Priority order than in send order
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 */
HWTEST_F(KvDBCommunicatorDeepTest, SendSchedule001Virtual, TestSize.Level2)
{
    // Preset
    SendTaskScheduler schedulerTest;
    schedulerTest.Initialize();
    int errCodeTest = CreateBufferThenAddIntoScheduler(schedulerTest, DEVICE_NAME_A, Priority::LOW);
    EXPECT_EQ(errCodeTest, E_OK);

    errCodeTest = CreateBufferThenAddIntoScheduler(schedulerTest, DEVICE_NAME_B, Priority::LOW);
    EXPECT_EQ(errCodeTest, E_OK);

    errCodeTest = CreateBufferThenAddIntoScheduler(schedulerTest, DEVICE_NAME_B, Priority::NORMAL);
    EXPECT_EQ(errCodeTest, E_OK);

    errCodeTest = CreateBufferThenAddIntoScheduler(schedulerTest, DEVICE_NAME_C, Priority::NORMAL);
    EXPECT_EQ(errCodeTest, E_OK);

    errCodeTest = CreateBufferThenAddIntoScheduler(schedulerTest, DEVICE_NAME_C, Priority::HIGH);
    EXPECT_EQ(errCodeTest, E_OK);

    errCodeTest = CreateBufferThenAddIntoScheduler(schedulerTest, DEVICE_NAME_A, Priority::HIGH);
    EXPECT_EQ(errCodeTest, E_OK);

    SendTask outTaskTest;
    SendTaskInfo outTaskTestInfo;
    uint32_t totalLen = 0;
    // high priority target C
    errCodeTest = schedulerTest.ScheduleOutSendTask(outTaskTest, outTaskTestInfo, totalLen);
    EXPECT_EQ(errCodeTest, E_OK);
    EXPECT_EQ(outTaskTest.dstTarget, DEVICE_NAME_C);
    EXPECT_EQ(outTaskTestInfo.taskPrio, Priority::HIGH);
    schedulerTest.FinalizeLastScheduleTask();
    // high priority target A
    errCodeTest = schedulerTest.ScheduleOutSendTask(outTaskTest, outTaskTestInfo, totalLen);
    EXPECT_EQ(errCodeTest, E_OK);
    EXPECT_EQ(outTaskTest.dstTarget, DEVICE_NAME_A);
    EXPECT_EQ(outTaskTestInfo.taskPrio, Priority::HIGH);
    schedulerTest.FinalizeLastScheduleTask();
    // normal priority target B
    errCodeTest = schedulerTest.ScheduleOutSendTask(outTaskTest, outTaskTestInfo, totalLen);
    EXPECT_EQ(errCodeTest, E_OK);
    EXPECT_EQ(outTaskTest.dstTarget, DEVICE_NAME_B);
    EXPECT_EQ(outTaskTestInfo.taskPrio, Priority::NORMAL);
    schedulerTest.FinalizeLastScheduleTask();
    // normal priority target C
    errCodeTest = schedulerTest.ScheduleOutSendTask(outTaskTest, outTaskTestInfo, totalLen);
    EXPECT_EQ(errCodeTest, E_OK);
    EXPECT_EQ(outTaskTest.dstTarget, DEVICE_NAME_C);
    EXPECT_EQ(outTaskTestInfo.taskPrio, Priority::NORMAL);
    schedulerTest.FinalizeLastScheduleTask();
    // low priority target A
    errCodeTest = schedulerTest.ScheduleOutSendTask(outTaskTest, outTaskTestInfo, totalLen);
    EXPECT_EQ(errCodeTest, E_OK);
    EXPECT_EQ(outTaskTest.dstTarget, DEVICE_NAME_A);
    EXPECT_EQ(outTaskTestInfo.taskPrio, Priority::LOW);
    schedulerTest.FinalizeLastScheduleTask();
    // low priority target B
    errCodeTest = schedulerTest.ScheduleOutSendTask(outTaskTest, outTaskTestInfo, totalLen);
    EXPECT_EQ(errCodeTest, E_OK);
    EXPECT_EQ(outTaskTest.dstTarget, DEVICE_NAME_B);
    EXPECT_EQ(outTaskTestInfo.taskPrio, Priority::LOW);
    schedulerTest.FinalizeLastScheduleTask();
}

/**
 * @tc.name: Fragment 001
 * @tc.desc: Test fragmentation in send and receive
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 */
HWTEST_F(KvDBCommunicatorDeepTest, Fragment001Virtual, TestSize.Level2)
{
    // Preset
    Message *recvMsgFor11 = nullptr;
    g_comm13->RegOnMessageCallback([&recvMsgFor11](const std::string &srcTarget, Message *inMsg) {
        recvMsgFor11 = inMsg;
    }, nullptr);

    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);

    const uint32_t dataLen = 13 * 1024 * 1024; // 13 MB, 1024 is scale
    Message *sendMsgTestFor12 = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(sendMsgTestFor12, nullptr);
    SendConfig confin = {false, false, 0};
    int errCodeTest = g_comm12->SendMessage(DEVICE_NAME_B, sendMsgTestFor12, confin);
    EXPECT_EQ(errCodeTest, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(2600)); // Wait 2600 ms to make sure send done
    ASSERT_NE(recvMsgFor11, nullptr);
    EXPECT_EQ(recvMsgFor11->GetMessageId(), REGED_GIANT_MSG_ID);

    Message *oriMsgFor11 = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(oriMsgFor11, nullptr);
    const RegedGiantObject *oriObjFor11 = oriMsgFor11->GetObject<RegedGiantObject>();
    ASSERT_NE(oriObjFor11, nullptr);
    const RegedGiantObject *recvObjFor11 = recvMsgFor11->GetObject<RegedGiantObject>();
    ASSERT_NE(recvObjFor11, nullptr);
    bool isEqual = RegedGiantObject::CheckEqual(*oriObjFor11, *recvObjFor11);
    EXPECT_EQ(isEqual, true);

    // CleanUp
    delete oriMsgFor11;
    oriMsgFor11 = nullptr;
    delete recvMsgFor11;
    recvMsgFor11 = nullptr;
    AdapterStub::DisconnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
}

/**
 * @tc.name: Fragment 002
 * @tc.desc: Test fragmentation in partial loss
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 */
HWTEST_F(KvDBCommunicatorDeepTest, Fragment002Virtual, TestSize.Level2)
{
    // Preset
    Message *recvMsgFor33 = nullptr;
    g_comm15->RegOnMessageCallback([&recvMsgFor33](const std::string &srcTarget, Message *inMsg) {
        recvMsgFor33 = inMsg;
    }, nullptr);

    AdapterStub::ConnectAdapterStub(g_envDevice2Test.adaHandle, g_envDevice3Test.adaHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait 200 ms to make sure quiet

    g_envDevice2Test.adaHandle->SimulateSendPartialLoss();

    uint32_t dataLen = 13 * 1024 * 1024; // 13 MB, 1024 is scale
    Message *sendMsgTestForBC = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(sendMsgTestForBC, nullptr);
    SendConfig confin = {false, false, 0};
    g_envDevice2Test.adaHandle->SimulateSendPartialLossClear();

    dataLen = 18 * 1024 * 1024; // 17 MB, 1024 is scale
    Message *resendMsgTestForBC = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(resendMsgTestForBC, nullptr);
    errCodeTest = g_comm14->SendMessage(DEVICE_NAME_C, resendMsgTestForBC, confin);
    EXPECT_EQ(errCodeTest, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(3400)); // Wait 3400 ms to make sure send done
    ASSERT_NE(recvMsgFor33, nullptr);
    EXPECT_EQ(recvMsgFor33->GetMessageId(), REGED_GIANT_MSG_ID);
    const RegedGiantObject *recvObjFor33 = recvMsgFor33->GetObject<RegedGiantObject>();
    ASSERT_NE(recvObjFor33, nullptr);
    EXPECT_EQ(dataLen, recvObjFor33->rawData_.size());

    // CleanUp
    delete recvMsgFor33;
    recvMsgFor33 = nullptr;
    AdapterStub::DisconnectAdapterStub(g_envDevice2Test.adaHandle, g_envDevice3Test.adaHandle);
}

/**
 * @tc.name: Fragment 003
 * @tc.desc: Test fragmentation simultaneously
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 */
HWTEST_F(KvDBCommunicatorDeepTest, Fragment003Virtual, TestSize.Level3)
{
    // Preset
    std::atomic<int> count {0};
    OnMessageCallback callbackTest = [&count](const std::string &srcTarget, Message *inMsg) {
        delete inMsg;
        inMsg = nullptr;
        count.fetch_add(1, std::memory_order_seq_cst);
    };
    g_comm13->RegOnMessageCallback(callbackTest, nullptr);
    g_comm14->RegOnMessageCallback(callbackTest, nullptr);

    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    AdapterStub::ConnectAdapterStub(g_envDevice2Test.adaHandle, g_envDevice3Test.adaHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(400)); // Wait 400 ms to make sure quiet

    g_envDevice1Test.adaHandle->SimulateSendBlock();
    g_envDevice3Test.adaHandle->SimulateSendBlock();

    uint32_t dataLen = 23 * 1024 * 1024; // 23 MB, 1024 is scale
    Message *sendMsgTestFor12 = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(sendMsgTestFor12, nullptr);
    SendConfig confin = {false, false, 0};
    int errCodeTest = g_comm12->SendMessage(DEVICE_NAME_B, sendMsgTestFor12, confin);
    EXPECT_EQ(errCodeTest, E_OK);

    Message *sendMsgTestFor33 = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(sendMsgTestFor33, nullptr);
    errCodeTest = g_comm15->SendMessage(DEVICE_NAME_B, sendMsgTestFor33, confin);
    EXPECT_EQ(errCodeTest, E_OK);

    g_envDevice1Test.adaHandle->SimulateSendBlockClear();
    g_envDevice3Test.adaHandle->SimulateSendBlockClear();
    std::this_thread::sleep_for(std::chrono::milliseconds(9200)); // Wait 9200 ms to make sure send done
    EXPECT_EQ(count, 2); // 2 combined message received

    // CleanUp
    AdapterStub::DisconnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    AdapterStub::DisconnectAdapterStub(g_envDevice2Test.adaHandle, g_envDevice3Test.adaHandle);
}

/**
 * @tc.name: Fragment 004
 * @tc.desc: Test fragmentation in send and receive when rate limit
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 */
HWTEST_F(KvDBCommunicatorDeepTest, Fragment004Virtual, TestSize.Level2)
{
    /**
     * @tc.steps: step1. connect device A with device B
     */
    Message *recvMsgFor11 = nullptr;
    g_comm13->RegOnMessageCallback([&recvMsgFor11](const std::string &srcTarget, Message *inMsg) {
        recvMsgFor11 = inMsg;
    }, nullptr);
    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    std::atomic<int> count = 0;
    g_envDevice1Test.adaHandle->ForkSendBytes([&count]() {
        count++;
        if (count % 3 == 0) { // retry each 3 packet
            return -E_WAIT_RETRY;
        }
        return E_OK;
    });
    const uint32_t dataLen = 15 * 1024 * 1024; // 13 MB, 1024 is scale
    Message *sendMsgTest = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(sendMsgTest, nullptr);
    SendConfig confin = {false, false, 0};
    int errCodeTest = g_comm12->SendMessage(DEVICE_NAME_B, sendMsgTest, confin);
    EXPECT_EQ(errCodeTest, E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait 1s to make sure send done
    g_envDevice1Test.adaHandle->SimulateSendRetry(DEVICE_NAME_B);
    g_envDevice1Test.adaHandle->SimulateSendRetryClear(DEVICE_NAME_B);
    int reTimes = 5;
    while (recvMsgFor11 == nullptr && reTimes > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        reTimes--;
    }
    ASSERT_NE(recvMsgFor11, nullptr);
    EXPECT_EQ(recvMsgFor11->GetMessageId(), REGED_GIANT_MSG_ID);
    Message *oriMsgFor11 = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(oriMsgFor11, nullptr);
    auto *recvObjFor11 = recvMsgFor11->GetObject<RegedGiantObject>();
    ASSERT_NE(recvObjFor11, nullptr);
    auto *oriObjFor11 = oriMsgFor11->GetObject<RegedGiantObject>();
    ASSERT_NE(oriObjFor11, nullptr);
    bool isEqual = RegedGiantObject::CheckEqual(*oriObjFor11, *recvObjFor11);
    EXPECT_EQ(isEqual, true);
    g_envDevice1Test.adaHandle->ForkSendBytes(nullptr);

    // CleanUp
    AdapterStub::DisconnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    delete oriMsgFor11;
    oriMsgFor11 = nullptr;
    delete recvMsgFor11;
    recvMsgFor11 = nullptr;
}

namespace {
void ClearPreviousTestCaseInfluence()
{
    ReleaseAllCommunicator();
    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    AdapterStub::ConnectAdapterStub(g_envDevice2Test.adaHandle, g_envDevice3Test.adaHandle);
    AdapterStub::ConnectAdapterStub(g_envDevice3Test.adaHandle, g_envDevice1Test.adaHandle);
    std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait 10 s to make sure all thread quiet
    AdapterStub::DisconnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    AdapterStub::DisconnectAdapterStub(g_envDevice2Test.adaHandle, g_envDevice3Test.adaHandle);
    AdapterStub::DisconnectAdapterStub(g_envDevice3Test.adaHandle, g_envDevice1Test.adaHandle);
    AllocAllCommunicator();
}
}

/**
 * @tc.name: ReliableOnline 001
 * @tc.desc: Test device online reliability
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ AR000CQE0N
 */
HWTEST_F(KvDBCommunicatorDeepTest, ReliableOnline001Virtual, TestSize.Level2)
{
    // Preset
    ClearPreviousTestCaseInfluence();
    std::atomic<int> count {0};
    OnConnectCallback callbackTest = [&count](const std::string &target, bool isConnect) {
        if (isConnect) {
            count.fetch_add(1, std::memory_order_seq_cst);
        }
    };
    g_comm11->RegOnConnectCallback(callbackTest, nullptr);
    g_comm12->RegOnConnectCallback(callbackTest, nullptr);
    g_comm13->RegOnConnectCallback(callbackTest, nullptr);
    g_comm14->RegOnConnectCallback(callbackTest, nullptr);
    g_comm15->RegOnConnectCallback(callbackTest, nullptr);
    g_comm16->RegOnConnectCallback(callbackTest, nullptr);

    g_envDevice1Test.adaHandle->SimulateSendTotalLoss();
    g_envDevice2Test.adaHandle->SimulateSendTotalLoss();
    g_envDevice3Test.adaHandle->SimulateSendTotalLoss();

    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    AdapterStub::ConnectAdapterStub(g_envDevice2Test.adaHandle, g_envDevice3Test.adaHandle);
    AdapterStub::ConnectAdapterStub(g_envDevice3Test.adaHandle, g_envDevice1Test.adaHandle);

    std::this_thread::sleep_for(std::chrono::seconds(7)); // Wait 7 s to make sure quiet
    EXPECT_EQ(count, 0); // no online callbackTest received

    g_envDevice1Test.adaHandle->SimulateSendTotalLossClear();
    g_envDevice2Test.adaHandle->SimulateSendTotalLossClear();
    g_envDevice3Test.adaHandle->SimulateSendTotalLossClear();
    std::this_thread::sleep_for(std::chrono::seconds(7)); // Wait 7 s to make sure send done
    EXPECT_EQ(count, 6); // 6 online callbackTest received in total

    // CleanUp
    AdapterStub::DisconnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    AdapterStub::DisconnectAdapterStub(g_envDevice2Test.adaHandle, g_envDevice3Test.adaHandle);
    AdapterStub::DisconnectAdapterStub(g_envDevice3Test.adaHandle, g_envDevice1Test.adaHandle);
}

/**
 * @tc.name: NetworkAdapter001
 * @tc.desc: Test networkAdapter start func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 */
HWTEST_F(KvDBCommunicatorDeepTest, NetworkAdapter001Virtual, TestSize.Level1)
{
    auto proCommunicate = std::make_shared<MockProcessCommunicator>();
    EXPECT_CALL(*proCommunicate, Stop()).WillRepeatedly(testing::Return(OK));
    auto adapterTest = std::make_shared<NetworkAdapter>("");
    EXPECT_EQ(adapterTest->StartAdapter(), -E_INVALID);

    adapterTest = std::make_shared<NetworkAdapter>("labelTest");
    EXPECT_EQ(adapterTest->StartAdapter(), -E_INVALID);

    adapterTest = std::make_shared<NetworkAdapter>("labelTest", proCommunicate);
    EXPECT_CALL(*proCommunicate, Start).WillRepeatedly(testing::Return(DB_OK));
    EXPECT_EQ(adapterTest->StartAdapter(), -E_PERIPHERAL_INTERFACE_FAIL);

    EXPECT_CALL(*proCommunicate, Start).WillRepeatedly(testing::Return(OK));
    EXPECT_CALL(*proCommunicate, RegOnReceive).WillRepeatedly(testing::Return(DB_OK));
    EXPECT_EQ(adapterTest->StartAdapter(), -E_PERIPHERAL_INTERFACE_FAIL);
    EXPECT_CALL(*proCommunicate, RegOnReceive).WillRepeatedly(testing::Return(OK));
    EXPECT_CALL(*proCommunicate, RegOnDeviceChange).WillRepeatedly(testing::Return(DB_OK));
    EXPECT_EQ(adapterTest->StartAdapter(), -E_PERIPHERAL_INTERFACE_FAIL);

    EXPECT_CALL(*proCommunicate, RegOnDeviceChange).WillRepeatedly(testing::Return(OK));
    EXPECT_CALL(*proCommunicate, GetLocalDeviceInfos).WillRepeatedly([]() {
        DeviceInfos deviceInfosItem;
        deviceInfosItem.identifier = "DEVICES_TEST"; // local is deviceA
        return deviceInfosItem;
    });
    EXPECT_CALL(*proCommunicate, GetRemoteOnlineDeviceInfosList).WillRepeatedly([]() {
        std::vector<DeviceInfos> res;
        DeviceInfos deviceInfosItem;
        deviceInfosItem.identifier = "DEVICES_TEST"; // search local is deviceA
        res.push_back(deviceInfosItem);
        deviceInfosItem.identifier = "DEVICES_VIRTUAL"; // search remote is deviceB
        res.push_back(deviceInfosItem);
        return res;
    });
    EXPECT_CALL(*proCommunicate, IsSameProcessLabelStartedOnPeerDevice).WillRepeatedly([](const DeviceInfos &) {
        return false;
    });
    EXPECT_EQ(adapterTest->StartAdapter(), E_OK);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: NetworkAdapter002
 * @tc.desc: Test networkAdapter get mtu func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 */
HWTEST_F(KvDBCommunicatorDeepTest, NetworkAdapter002Virtual, TestSize.Level1)
{
    auto proCommunicate = std::make_shared<MockProcessCommunicator>();
    auto adapterTest = std::make_shared<NetworkAdapter>("labelTest", proCommunicate);

    EXPECT_CALL(*proCommunicate, GetMtuSize).WillRepeatedly([]() {
        return 0u;
    });
    EXPECT_EQ(adapterTest->GetMtuSize(), DBConstant::MIN_MTU_SIZE);

    EXPECT_CALL(*proCommunicate, GetMtuSize).WillRepeatedly([]() {
        return 2 * DBConstant::MAX_MTU_SIZE;
    });
    EXPECT_EQ(adapterTest->GetMtuSize(), DBConstant::MIN_MTU_SIZE);
    adapterTest = std::make_shared<NetworkAdapter>("labelTest", proCommunicate);
    EXPECT_EQ(adapterTest->GetMtuSize(), DBConstant::MAX_MTU_SIZE);
}

/**
 * @tc.name: NetworkAdapter003
 * @tc.desc: Test networkAdapter get timeout func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 */
HWTEST_F(KvDBCommunicatorDeepTest, NetworkAdapter003Virtual, TestSize.Level1)
{
    auto proCommunicate = std::make_shared<MockProcessCommunicator>();
    auto adapterTest = std::make_shared<NetworkAdapter>("labelTest", proCommunicate);

    EXPECT_CALL(*proCommunicate, GetTimeout).WillRepeatedly([]() {
        return 0u;
    });
    EXPECT_EQ(adapterTest->GetTimeout(), DBConstant::MIN_TIMEOUT);

    EXPECT_CALL(*proCommunicate, GetTimeout).WillRepeatedly([]() {
        return 2 * DBConstant::MAX_TIMEOUT;
    });
    EXPECT_EQ(adapterTest->GetTimeout(), DBConstant::MAX_TIMEOUT);
}

/**
 * @tc.name: NetworkAdapter004
 * @tc.desc: Test networkAdapter send bytes func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 */
HWTEST_F(KvDBCommunicatorDeepTest, NetworkAdapter004Virtual, TestSize.Level1)
{
    auto proCommunicate = std::make_shared<MockProcessCommunicator>();
    auto adapterTest = std::make_shared<NetworkAdapter>("labelTest", proCommunicate);

    EXPECT_CALL(*proCommunicate, SendData).WillRepeatedly([](const DeviceInfos &, const uint8_t *, uint32_t) {
        return OK;
    });

    auto data = std::make_shared<uint8_t>(1u);
    EXPECT_EQ(adapterTest->SendBytes("DEVICES_VIRTUAL", nullptr, 1, 0), -E_INVALID);
    EXPECT_EQ(adapterTest->SendBytes("DEVICES_VIRTUAL", data.get(), 0, 0), -E_INVALID);

    EXPECT_EQ(adapterTest->SendBytes("DEVICES_VIRTUAL", data.get(), 1, 0), E_OK);
    RuntimeContext::GetInstance()->StopTaskPool();
}

namespace {
void InitAdapter(const std::shared_ptr<NetworkAdapter> &adapterTest,
    const std::shared_ptr<MockProcessCommunicator> &proCommunicate,
    OnDataReceive &onDataRe, OnDeviceChange &onDataChange)
{
    EXPECT_CALL(*proCommunicate, Stop).WillRepeatedly([]() {
        return OK;
    });
    EXPECT_CALL(*proCommunicate, Start).WillRepeatedly([](const std::string &) {
        return OK;
    });
    EXPECT_CALL(*proCommunicate, RegOnReceive).WillRepeatedly(
        [&onDataRe](const OnDataReceive &callbackTest) {
            onDataRe = callbackTest;
            return OK;
    });
    EXPECT_CALL(*proCommunicate, RegOnDeviceChange).WillRepeatedly(
        [&onDataChange](const OnDeviceChange &callbackTest) {
            onDataChange = callbackTest;
            return OK;
    });
    EXPECT_CALL(*proCommunicate, GetRemoteOnlineDeviceInfosList).WillRepeatedly([]() {
        std::vector<DeviceInfos> res;
        return res;
    });
    EXPECT_CALL(*proCommunicate, IsSameProcessLabelStartedOnPeerDevice).WillRepeatedly([](const DeviceInfos &) {
        return false;
    });
    EXPECT_EQ(adapterTest->StartAdapter(), E_OK);
}
}
/**
 * @tc.name: NetworkAdapter005
 * @tc.desc: Test networkAdapter receive data func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 */
HWTEST_F(KvDBCommunicatorDeepTest, NetworkAdapter005Virtual, TestSize.Level1)
{
    auto proCommunicate = std::make_shared<MockProcessCommunicator>();
    auto adapterTest = std::make_shared<NetworkAdapter>("labelTest", proCommunicate);
    OnDataReceive onDataRe;
    OnDeviceChange onChange;
    InitAdapter(adapterTest, proCommunicate, onDataRe, onChange);
    ASSERT_NE(onDataRe, nullptr);

    auto data = std::make_shared<uint8_t>(1);
    DeviceInfos deviceInfosItem;
    onDataRe(deviceInfosItem, nullptr, 1);
    onDataRe(deviceInfosItem, data.get(), 0);
    EXPECT_CALL(*proCommunicate, CheckAndGetDataHeadInfo).WillRepeatedly(
        [](const uint8_t *, uint32_t, uint32_t &, std::vector<std::string> &) {
        return NO_PERMISSION;
    });
    onDataRe(deviceInfosItem, data.get(), 1);
    EXPECT_CALL(*proCommunicate, CheckAndGetDataHeadInfo).WillRepeatedly(
        [](const uint8_t *, uint32_t, uint32_t &, std::vector<std::string> &userIds) {
            userIds.emplace_back("1");
            return OK;
    });
    onDataRe(deviceInfosItem, data.get(), 1);
    adapterTest->RegBytesReceiveCallback([](const std::string &, uint32_t, const std::string &) {
    }, nullptr);
    onDataRe(deviceInfosItem, data.get(), 1);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: NetworkAdapter006
 * @tc.desc: Test networkAdapter device change func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 */
HWTEST_F(KvDBCommunicatorDeepTest, NetworkAdapter006Virtual, TestSize.Level1)
{
    auto proCommunicate = std::make_shared<MockProcessCommunicator>();
    auto adapterTest = std::make_shared<NetworkAdapter>("labelTest", proCommunicate);
    OnDataReceive onDataRe;
    OnDeviceChange onChange;
    InitAdapter(adapterTest, proCommunicate, onDataRe, onChange);
    ASSERT_NE(onChange, nullptr);
    DeviceInfos deviceInfosItem;

    onChange(deviceInfosItem, true);

    EXPECT_CALL(*proCommunicate, IsSameProcessLabelStartedOnPeerDevice).WillRepeatedly([](const DeviceInfos &) {
        return true;
    });
    onChange(deviceInfosItem, true);
    adapterTest->RegTargetChangeCallback([](const std::string &, bool) {
    }, nullptr);
    onChange(deviceInfosItem, false);
    onChange(deviceInfosItem, true);
    EXPECT_CALL(*proCommunicate, SendData).WillRepeatedly([](const DeviceInfos &, const uint8_t *, uint32_t) {
        return DB_OK;
    });
    EXPECT_CALL(*proCommunicate, IsSameProcessLabelStartedOnPeerDevice).WillRepeatedly([](const DeviceInfos &) {
        return false;
    });
    auto data = std::make_shared<uint8_t>(1);
    EXPECT_EQ(adapterTest->SendBytes("", data.get(), 1, 0), static_cast<int>(DB_OK));
    RuntimeContext::GetInstance()->StopTaskPool();
    EXPECT_EQ(adapterTest->IsDeviceOnline(""), false);
    ExtendInfo extinfo;
    EXPECT_EQ(adapterTest->GetExtendHeaderHandle(extinfo), nullptr);
}

/**
 * @tc.name: NetworkAdapter007
 * @tc.desc: Test networkAdapter recv invalid head length
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 */
HWTEST_F(KvDBCommunicatorDeepTest, NetworkAdapter007Virtual, TestSize.Level1)
{
    auto proCommunicate = std::make_shared<MockProcessCommunicator>();
    auto adapterTest = std::make_shared<NetworkAdapter>("NetworkAdapter007", proCommunicate);
    OnDataReceive onDataRe;
    OnDeviceChange onChange;
    InitAdapter(adapterTest, proCommunicate, onDataRe, onChange);
    ASSERT_NE(onChange, nullptr);
    EXPECT_CALL(*proCommunicate, CheckAndGetDataHeadInfo).WillOnce([](const uint8_t *, uint32_t, uint32_t &headLen,
        std::vector<std::string> &) {
        headLen = UINT32_MAX;
        return OK;
    });
    int callReceiveCount = 0;
    int res =
        adapterTest->RegBytesReceiveCallback([&callReceiveCount](const std::string &, const uint8_t *, uint32_t,
        const std::string &) {
        callReceiveCount++;
    }, nullptr);
    EXPECT_EQ(res, E_OK);
    std::vector<uint8_t> data = { 1u };
    DeviceInfos deviceInfosItem;
    onDataRe(deviceInfosItem, data.data(), 1u);
    EXPECT_EQ(callReceiveCount, 0);
}

/**
 * @tc.name: RetrySendExceededLimit001
 * @tc.desc: Test send result when the number of retry times exceeds the limit
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KvDBCommunicatorDeepTest, RetrySendExceededLimit001Virtual, TestSize.Level2)
{
    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    std::atomic<int> count = 0;
    g_envDevice1Test.adaHandle->ForkSendBytes([&count]() {
        count++;
        return -E_WAIT_RETRY;
    });
    std::vector<std::pair<int, bool>> sendResult;
    auto sendResultNotifier = [&sendResult](int result, bool isDirectEnd) {
        sendResult.push_back(std::pair<int, bool>(result, isDirectEnd));
    };
    const uint32_t dataLen = 13 * 1024 * 1024; // 13 MB, 1024 is scale
    Message *sendMsgTest = BuildRegedGiantMessage(dataLen);
    ASSERT_NE(sendMsgTest, nullptr);
    SendConfig confin = {false, false, 0};
    int errCodeTest = g_comm12->SendMessage(DEVICE_NAME_B, sendMsgTest, confin, sendResultNotifier);
    EXPECT_EQ(errCodeTest, E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait 1s to make sure send done
    g_envDevice1Test.adaHandle->SimulateSendRetry(DEVICE_NAME_B);
    g_envDevice1Test.adaHandle->SimulateSendRetryClear(DEVICE_NAME_B, -E_BASE);
    int reTimes = 5;
    while ((count < 4) && (reTimes > 0)) { // Wait to make sure retry exceeds the limit
        std::this_thread::sleep_for(std::chrono::seconds(3));
        reTimes--;
    }
    EXPECT_EQ(sendResult.size(), static_cast<size_t>(1)); // only one callbackTest result notification
    EXPECT_EQ(sendResult[0].first, -E_BASE); // index 0 retry fail
    EXPECT_EQ(sendResult[0].second, false);
    g_envDevice1Test.adaHandle->ForkSendBytes(nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
}

/**
 * @tc.name: RetrySendExceededLimit002
 * @tc.desc: Test multi thread call SendableCallback when the number of retry times exceeds the limit
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KvDBCommunicatorDeepTest, RetrySendExceededLimit002Virtual, TestSize.Level2)
{
    AdapterStub::ConnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
    std::atomic<int> count = 0;
    g_envDevice1Test.adaHandle->ForkSendBytes([&count]() {
        count++;
        return -E_WAIT_RETRY;
    });
    std::vector<std::pair<int, bool>> sendResult;
    auto sendResultNotifier = [&sendResult](int result, bool isDirectEnd) {
        sendResult.push_back(std::pair<int, bool>(result, isDirectEnd));
    };

    std::vector<std::thread> threads;
    int threadNum = 3;
    threads.reserve(threadNum);
    for (int n = 0; n < threadNum; n++) {
        threads.emplace_back([&]() {
            g_envDevice1Test.adaHandle->SimulateTriggerSendableCallback(DEVICE_NAME_B, -E_BASE);
        });
    }
    for (std::thread &t : threads) {
        t.join();
    }

    int reTimes = 5;
    while ((count < 4) && (reTimes > 0)) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        reTimes--;
    }
    EXPECT_EQ(sendResult.size(), static_cast<size_t>(1));
    EXPECT_EQ(sendResult[0].first, -E_BASE);
    EXPECT_EQ(sendResult[0].second, false);
    g_envDevice1Test.adaHandle->ForkSendBytes(nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDevice1Test.adaHandle, g_envDevice2Test.adaHandle);
}
