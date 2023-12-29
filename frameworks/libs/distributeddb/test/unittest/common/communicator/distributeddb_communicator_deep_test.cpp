/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <new>
#include <thread>
#include "db_errno.h"
#include "distributeddb_communicator_common.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "network_adapter.h"
#include "message.h"
#include "mock_process_communicator.h"
#include "serial_buffer.h"

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;

namespace {
    EnvHandle g_envDeviceA;
    EnvHandle g_envDeviceB;
    EnvHandle g_envDeviceC;
    ICommunicator *g_commAA = nullptr;
    ICommunicator *g_commAB = nullptr;
    ICommunicator *g_commBB = nullptr;
    ICommunicator *g_commBC = nullptr;
    ICommunicator *g_commCC = nullptr;
    ICommunicator *g_commCA = nullptr;
}

class DistributedDBCommunicatorDeepTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBCommunicatorDeepTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Create and init CommunicatorAggregator and AdapterStub
     */
    LOGI("[UT][DeepTest][SetUpTestCase] Enter.");
    bool errCode = SetUpEnv(g_envDeviceA, DEVICE_NAME_A);
    ASSERT_EQ(errCode, true);
    errCode = SetUpEnv(g_envDeviceB, DEVICE_NAME_B);
    ASSERT_EQ(errCode, true);
    errCode = SetUpEnv(g_envDeviceC, DEVICE_NAME_C);
    ASSERT_EQ(errCode, true);
    DoRegTransformFunction();
    CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(false);
}

void DistributedDBCommunicatorDeepTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Finalize and release CommunicatorAggregator and AdapterStub
     */
    LOGI("[UT][DeepTest][TearDownTestCase] Enter.");
    std::this_thread::sleep_for(std::chrono::seconds(7)); // Wait 7 s to make sure all thread quiet and memory released
    TearDownEnv(g_envDeviceA);
    TearDownEnv(g_envDeviceB);
    TearDownEnv(g_envDeviceC);
    CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(true);
}

namespace {
void AllocAllCommunicator()
{
    int errorNo = E_OK;
    g_commAA = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_commAA);
    g_commAB = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_B, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_commAB);
    g_commBB = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_B, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_commBB);
    g_commBC = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_C, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_commBC);
    g_commCC = g_envDeviceC.commAggrHandle->AllocCommunicator(LABEL_C, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_commCC);
    g_commCA = g_envDeviceC.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(g_commCA);
}

void ReleaseAllCommunicator()
{
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(g_commAA);
    g_commAA = nullptr;
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(g_commAB);
    g_commAB = nullptr;
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(g_commBB);
    g_commBB = nullptr;
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(g_commBC);
    g_commBC = nullptr;
    g_envDeviceC.commAggrHandle->ReleaseCommunicator(g_commCC);
    g_commCC = nullptr;
    g_envDeviceC.commAggrHandle->ReleaseCommunicator(g_commCA);
    g_commCA = nullptr;
}
}

void DistributedDBCommunicatorDeepTest::SetUp()
{
    DistributedDBUnitTest::DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: Alloc communicator AA, AB, BB, BC, CC, CA
     */
    AllocAllCommunicator();
}

void DistributedDBCommunicatorDeepTest::TearDown()
{
    /**
     * @tc.teardown: Release communicator AA, AB, BB, BC, CC, CA
     */
    ReleaseAllCommunicator();
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait 200 ms to make sure all thread quiet
}

/**
 * @tc.name: WaitAndRetrySend 001
 * @tc.desc: Test send retry semantic
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, WaitAndRetrySend001, TestSize.Level2)
{
    // Preset
    Message *msgForBB = nullptr;
    g_commBB->RegOnMessageCallback([&msgForBB](const std::string &srcTarget, Message *inMsg) {
        msgForBB = inMsg;
    }, nullptr);
    Message *msgForCA = nullptr;
    g_commCA->RegOnMessageCallback([&msgForCA](const std::string &srcTarget, Message *inMsg) {
        msgForCA = inMsg;
    }, nullptr);

    /**
     * @tc.steps: step1. connect device A with device B
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceC.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait 200 ms to make sure quiet

    /**
     * @tc.steps: step2. device A simulate send retry
     */
    g_envDeviceA.adapterHandle->SimulateSendRetry(DEVICE_NAME_B);

    /**
     * @tc.steps: step3. device A send message to device B using communicator AB
     * @tc.expected: step3. communicator BB received no message
     */
    Message *msgForAB = BuildRegedTinyMessage();
    ASSERT_NE(msgForAB, nullptr);
    SendConfig conf = {true, false, 0};
    int errCode = g_commAB->SendMessage(DEVICE_NAME_B, msgForAB, conf);
    EXPECT_EQ(errCode, E_OK);

    Message *msgForAA = BuildRegedTinyMessage();
    ASSERT_NE(msgForAA, nullptr);
    errCode = g_commAA->SendMessage(DEVICE_NAME_C, msgForAA, conf);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait 100 ms
    EXPECT_EQ(msgForBB, nullptr);
    EXPECT_NE(msgForCA, nullptr);
    delete msgForCA;
    msgForCA = nullptr;

    /**
     * @tc.steps: step4. device A simulate sendable feedback
     * @tc.expected: step4. communicator BB received the message
     */
    g_envDeviceA.adapterHandle->SimulateSendRetryClear(DEVICE_NAME_B);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait 100 ms
    EXPECT_NE(msgForBB, nullptr);
    delete msgForBB;
    msgForBB = nullptr;

    // CleanUp
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
}

static int CreateBufferThenAddIntoScheduler(SendTaskScheduler &scheduler, const std::string &dstTarget, Priority inPrio)
{
    SerialBuffer *eachBuff = new (std::nothrow) SerialBuffer();
    if (eachBuff == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    int errCode = eachBuff->AllocBufferByTotalLength(100, 0); // 100 totallen without header
    if (errCode != E_OK) {
        delete eachBuff;
        eachBuff = nullptr;
        return errCode;
    }
    SendTask task{eachBuff, dstTarget, nullptr, 0u};
    errCode = scheduler.AddSendTaskIntoSchedule(task, inPrio);
    if (errCode != E_OK) {
        delete eachBuff;
        eachBuff = nullptr;
        return errCode;
    }
    return E_OK;
}

/**
 * @tc.name: SendSchedule 001
 * @tc.desc: Test schedule in Priority order than in send order
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, SendSchedule001, TestSize.Level2)
{
    // Preset
    SendTaskScheduler scheduler;
    scheduler.Initialize();

    /**
     * @tc.steps: step1. Add low priority target A buffer to schecduler
     */
    int errCode = CreateBufferThenAddIntoScheduler(scheduler, DEVICE_NAME_A, Priority::LOW);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. Add low priority target B buffer to schecduler
     */
    errCode = CreateBufferThenAddIntoScheduler(scheduler, DEVICE_NAME_B, Priority::LOW);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step3. Add normal priority target B buffer to schecduler
     */
    errCode = CreateBufferThenAddIntoScheduler(scheduler, DEVICE_NAME_B, Priority::NORMAL);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step4. Add normal priority target C buffer to schecduler
     */
    errCode = CreateBufferThenAddIntoScheduler(scheduler, DEVICE_NAME_C, Priority::NORMAL);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step5. Add high priority target C buffer to schecduler
     */
    errCode = CreateBufferThenAddIntoScheduler(scheduler, DEVICE_NAME_C, Priority::HIGH);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step6. Add high priority target A buffer to schecduler
     */
    errCode = CreateBufferThenAddIntoScheduler(scheduler, DEVICE_NAME_A, Priority::HIGH);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step7. schedule out buffers one by one
     * @tc.expected: step7. the order is: high priority target C
     *                                    high priority target A
     *                                    normal priority target B
     *                                    normal priority target C
     *                                    low priority target A
     *                                    low priority target B
     */
    SendTask outTask;
    SendTaskInfo outTaskInfo;
    uint32_t totalLength = 0;
    // high priority target C
    errCode = scheduler.ScheduleOutSendTask(outTask, outTaskInfo, totalLength);
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(outTask.dstTarget, DEVICE_NAME_C);
    EXPECT_EQ(outTaskInfo.taskPrio, Priority::HIGH);
    scheduler.FinalizeLastScheduleTask();
    // high priority target A
    errCode = scheduler.ScheduleOutSendTask(outTask, outTaskInfo, totalLength);
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(outTask.dstTarget, DEVICE_NAME_A);
    EXPECT_EQ(outTaskInfo.taskPrio, Priority::HIGH);
    scheduler.FinalizeLastScheduleTask();
    // normal priority target B
    errCode = scheduler.ScheduleOutSendTask(outTask, outTaskInfo, totalLength);
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(outTask.dstTarget, DEVICE_NAME_B);
    EXPECT_EQ(outTaskInfo.taskPrio, Priority::NORMAL);
    scheduler.FinalizeLastScheduleTask();
    // normal priority target C
    errCode = scheduler.ScheduleOutSendTask(outTask, outTaskInfo, totalLength);
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(outTask.dstTarget, DEVICE_NAME_C);
    EXPECT_EQ(outTaskInfo.taskPrio, Priority::NORMAL);
    scheduler.FinalizeLastScheduleTask();
    // low priority target A
    errCode = scheduler.ScheduleOutSendTask(outTask, outTaskInfo, totalLength);
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(outTask.dstTarget, DEVICE_NAME_A);
    EXPECT_EQ(outTaskInfo.taskPrio, Priority::LOW);
    scheduler.FinalizeLastScheduleTask();
    // low priority target B
    errCode = scheduler.ScheduleOutSendTask(outTask, outTaskInfo, totalLength);
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(outTask.dstTarget, DEVICE_NAME_B);
    EXPECT_EQ(outTaskInfo.taskPrio, Priority::LOW);
    scheduler.FinalizeLastScheduleTask();
}

/**
 * @tc.name: Fragment 001
 * @tc.desc: Test fragmentation in send and receive
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, Fragment001, TestSize.Level2)
{
    // Preset
    Message *recvMsgForBB = nullptr;
    g_commBB->RegOnMessageCallback([&recvMsgForBB](const std::string &srcTarget, Message *inMsg) {
        recvMsgForBB = inMsg;
    }, nullptr);

    /**
     * @tc.steps: step1. connect device A with device B
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);

    /**
     * @tc.steps: step2. device A send message(registered and giant) to device B using communicator AB
     * @tc.expected: step2. communicator BB received the message
     */
    const uint32_t dataLength = 13 * 1024 * 1024; // 13 MB, 1024 is scale
    Message *sendMsgForAB = BuildRegedGiantMessage(dataLength);
    ASSERT_NE(sendMsgForAB, nullptr);
    SendConfig conf = {false, false, 0};
    int errCode = g_commAB->SendMessage(DEVICE_NAME_B, sendMsgForAB, conf);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(2600)); // Wait 2600 ms to make sure send done
    ASSERT_NE(recvMsgForBB, nullptr);
    ASSERT_EQ(recvMsgForBB->GetMessageId(), REGED_GIANT_MSG_ID);

    /**
     * @tc.steps: step3. Compare received data with send data
     * @tc.expected: step3. equal
     */
    Message *oriMsgForAB = BuildRegedGiantMessage(dataLength);
    ASSERT_NE(oriMsgForAB, nullptr);
    const RegedGiantObject *oriObjForAB = oriMsgForAB->GetObject<RegedGiantObject>();
    ASSERT_NE(oriObjForAB, nullptr);
    const RegedGiantObject *recvObjForBB = recvMsgForBB->GetObject<RegedGiantObject>();
    ASSERT_NE(recvObjForBB, nullptr);
    bool isEqual = RegedGiantObject::CheckEqual(*oriObjForAB, *recvObjForBB);
    EXPECT_EQ(isEqual, true);

    // CleanUp
    delete oriMsgForAB;
    oriMsgForAB = nullptr;
    delete recvMsgForBB;
    recvMsgForBB = nullptr;
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
}

/**
 * @tc.name: Fragment 002
 * @tc.desc: Test fragmentation in partial loss
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, Fragment002, TestSize.Level2)
{
    // Preset
    Message *recvMsgForCC = nullptr;
    g_commCC->RegOnMessageCallback([&recvMsgForCC](const std::string &srcTarget, Message *inMsg) {
        recvMsgForCC = inMsg;
    }, nullptr);

    /**
     * @tc.steps: step1. connect device B with device C
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Wait 200 ms to make sure quiet

    /**
     * @tc.steps: step2. device B simulate partial loss
     */
    g_envDeviceB.adapterHandle->SimulateSendPartialLoss();

    /**
     * @tc.steps: step3. device B send message(registered and giant) to device C using communicator BC
     * @tc.expected: step3. communicator CC not receive the message
     */
    uint32_t dataLength = 13 * 1024 * 1024; // 13 MB, 1024 is scale
    Message *sendMsgForBC = BuildRegedGiantMessage(dataLength);
    ASSERT_NE(sendMsgForBC, nullptr);
    SendConfig conf = {false, false, 0};
    int errCode = g_commBC->SendMessage(DEVICE_NAME_C, sendMsgForBC, conf);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(2600)); // Wait 2600 ms to make sure send done
    EXPECT_EQ(recvMsgForCC, nullptr);

    /**
     * @tc.steps: step4. device B not simulate partial loss
     */
    g_envDeviceB.adapterHandle->SimulateSendPartialLossClear();

    /**
     * @tc.steps: step5. device B send message(registered and giant) to device C using communicator BC
     * @tc.expected: step5. communicator CC received the message, the length equal to the one that is second send
     */
    dataLength = 17 * 1024 * 1024; // 17 MB, 1024 is scale
    Message *resendMsgForBC = BuildRegedGiantMessage(dataLength);
    ASSERT_NE(resendMsgForBC, nullptr);
    errCode = g_commBC->SendMessage(DEVICE_NAME_C, resendMsgForBC, conf);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(3400)); // Wait 3400 ms to make sure send done
    ASSERT_NE(recvMsgForCC, nullptr);
    ASSERT_EQ(recvMsgForCC->GetMessageId(), REGED_GIANT_MSG_ID);
    const RegedGiantObject *recvObjForCC = recvMsgForCC->GetObject<RegedGiantObject>();
    ASSERT_NE(recvObjForCC, nullptr);
    EXPECT_EQ(dataLength, recvObjForCC->rawData_.size());

    // CleanUp
    delete recvMsgForCC;
    recvMsgForCC = nullptr;
    AdapterStub::DisconnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
}

/**
 * @tc.name: Fragment 003
 * @tc.desc: Test fragmentation simultaneously
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, Fragment003, TestSize.Level3)
{
    // Preset
    std::atomic<int> count {0};
    OnMessageCallback callback = [&count](const std::string &srcTarget, Message *inMsg) {
        delete inMsg;
        inMsg = nullptr;
        count.fetch_add(1, std::memory_order_seq_cst);
    };
    g_commBB->RegOnMessageCallback(callback, nullptr);
    g_commBC->RegOnMessageCallback(callback, nullptr);

    /**
     * @tc.steps: step1. connect device A with device B, then device B with device C
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::ConnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(400)); // Wait 400 ms to make sure quiet

    /**
     * @tc.steps: step2. device A and device C simulate send block
     */
    g_envDeviceA.adapterHandle->SimulateSendBlock();
    g_envDeviceC.adapterHandle->SimulateSendBlock();

    /**
     * @tc.steps: step3. device A send message(registered and giant) to device B using communicator AB
     */
    uint32_t dataLength = 23 * 1024 * 1024; // 23 MB, 1024 is scale
    Message *sendMsgForAB = BuildRegedGiantMessage(dataLength);
    ASSERT_NE(sendMsgForAB, nullptr);
    SendConfig conf = {false, false, 0};
    int errCode = g_commAB->SendMessage(DEVICE_NAME_B, sendMsgForAB, conf);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step4. device C send message(registered and giant) to device B using communicator CC
     */
    Message *sendMsgForCC = BuildRegedGiantMessage(dataLength);
    ASSERT_NE(sendMsgForCC, nullptr);
    errCode = g_commCC->SendMessage(DEVICE_NAME_B, sendMsgForCC, conf);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step5. device A and device C not simulate send block
     * @tc.expected: step5. communicator BB and BV received the message
     */
    g_envDeviceA.adapterHandle->SimulateSendBlockClear();
    g_envDeviceC.adapterHandle->SimulateSendBlockClear();
    std::this_thread::sleep_for(std::chrono::milliseconds(9200)); // Wait 9200 ms to make sure send done
    EXPECT_EQ(count, 2); // 2 combined message received

    // CleanUp
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::DisconnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
}

/**
 * @tc.name: Fragment 004
 * @tc.desc: Test fragmentation in send and receive when rate limit
 * @tc.type: FUNC
 * @tc.require: AR000BVDGI AR000CQE0M
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, Fragment004, TestSize.Level2)
{
    /**
     * @tc.steps: step1. connect device A with device B
     */
    Message *recvMsgForBB = nullptr;
    g_commBB->RegOnMessageCallback([&recvMsgForBB](const std::string &srcTarget, Message *inMsg) {
        recvMsgForBB = inMsg;
    }, nullptr);
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::atomic<int> count = 0;
    g_envDeviceA.adapterHandle->ForkSendBytes([&count]() {
        count++;
        if (count % 3 == 0) { // retry each 3 packet
            return -E_WAIT_RETRY;
        }
        return E_OK;
    });
    /**
     * @tc.steps: step2. device A send message(registered and giant) to device B using communicator AB
     * @tc.expected: step2. communicator BB received the message
     */
    const uint32_t dataLength = 13 * 1024 * 1024; // 13 MB, 1024 is scale
    Message *sendMsg = BuildRegedGiantMessage(dataLength);
    ASSERT_NE(sendMsg, nullptr);
    SendConfig conf = {false, false, 0};
    int errCode = g_commAB->SendMessage(DEVICE_NAME_B, sendMsg, conf);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait 1s to make sure send done
    g_envDeviceA.adapterHandle->SimulateSendRetry(DEVICE_NAME_B);
    g_envDeviceA.adapterHandle->SimulateSendRetryClear(DEVICE_NAME_B);
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait 5s to make sure send done
    ASSERT_NE(recvMsgForBB, nullptr);
    ASSERT_EQ(recvMsgForBB->GetMessageId(), REGED_GIANT_MSG_ID);
    /**
     * @tc.steps: step3. Compare received data with send data
     * @tc.expected: step3. equal
     */
    Message *oriMsgForAB = BuildRegedGiantMessage(dataLength);
    ASSERT_NE(oriMsgForAB, nullptr);
    auto *recvObjForBB = recvMsgForBB->GetObject<RegedGiantObject>();
    ASSERT_NE(recvObjForBB, nullptr);
    auto *oriObjForAB = oriMsgForAB->GetObject<RegedGiantObject>();
    ASSERT_NE(oriObjForAB, nullptr);
    bool isEqual = RegedGiantObject::CheckEqual(*oriObjForAB, *recvObjForBB);
    EXPECT_EQ(isEqual, true);
    g_envDeviceA.adapterHandle->ForkSendBytes(nullptr);

    // CleanUp
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    delete oriMsgForAB;
    oriMsgForAB = nullptr;
    delete recvMsgForBB;
    recvMsgForBB = nullptr;
}

namespace {
void ClearPreviousTestCaseInfluence()
{
    ReleaseAllCommunicator();
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::ConnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
    AdapterStub::ConnectAdapterStub(g_envDeviceC.adapterHandle, g_envDeviceA.adapterHandle);
    std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait 10 s to make sure all thread quiet
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::DisconnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
    AdapterStub::DisconnectAdapterStub(g_envDeviceC.adapterHandle, g_envDeviceA.adapterHandle);
    AllocAllCommunicator();
}
}

/**
 * @tc.name: ReliableOnline 001
 * @tc.desc: Test device online reliability
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ AR000CQE0N
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, ReliableOnline001, TestSize.Level2)
{
    // Preset
    ClearPreviousTestCaseInfluence();
    std::atomic<int> count {0};
    OnConnectCallback callback = [&count](const std::string &target, bool isConnect) {
        if (isConnect) {
            count.fetch_add(1, std::memory_order_seq_cst);
        }
    };
    g_commAA->RegOnConnectCallback(callback, nullptr);
    g_commAB->RegOnConnectCallback(callback, nullptr);
    g_commBB->RegOnConnectCallback(callback, nullptr);
    g_commBC->RegOnConnectCallback(callback, nullptr);
    g_commCC->RegOnConnectCallback(callback, nullptr);
    g_commCA->RegOnConnectCallback(callback, nullptr);

    /**
     * @tc.steps: step1. device A and device B and device C simulate send total loss
     */
    g_envDeviceA.adapterHandle->SimulateSendTotalLoss();
    g_envDeviceB.adapterHandle->SimulateSendTotalLoss();
    g_envDeviceC.adapterHandle->SimulateSendTotalLoss();

    /**
     * @tc.steps: step2. connect device A with device B, device B with device C, device C with device A
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::ConnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
    AdapterStub::ConnectAdapterStub(g_envDeviceC.adapterHandle, g_envDeviceA.adapterHandle);

    /**
     * @tc.steps: step3. wait a long time
     * @tc.expected: step3. no communicator received the online callback
     */
    std::this_thread::sleep_for(std::chrono::seconds(7)); // Wait 7 s to make sure quiet
    EXPECT_EQ(count, 0); // no online callback received

    /**
     * @tc.steps: step4. device A and device B and device C not simulate send total loss
     */
    g_envDeviceA.adapterHandle->SimulateSendTotalLossClear();
    g_envDeviceB.adapterHandle->SimulateSendTotalLossClear();
    g_envDeviceC.adapterHandle->SimulateSendTotalLossClear();
    std::this_thread::sleep_for(std::chrono::seconds(7)); // Wait 7 s to make sure send done
    EXPECT_EQ(count, 6); // 6 online callback received in total

    // CleanUp
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::DisconnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
    AdapterStub::DisconnectAdapterStub(g_envDeviceC.adapterHandle, g_envDeviceA.adapterHandle);
}

/**
 * @tc.name: NetworkAdapter001
 * @tc.desc: Test networkAdapter start func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, NetworkAdapter001, TestSize.Level1)
{
    auto processCommunicator = std::make_shared<MockProcessCommunicator>();
    EXPECT_CALL(*processCommunicator, Stop()).WillRepeatedly(testing::Return(OK));
    /**
     * @tc.steps: step1. adapter start with empty label
     * @tc.expected: step1. start failed
     */
    auto adapter = std::make_shared<NetworkAdapter>("");
    EXPECT_EQ(adapter->StartAdapter(), -E_INVALID_ARGS);
    /**
     * @tc.steps: step2. adapter start with not empty label but processCommunicator is null
     * @tc.expected: step2. start failed
     */
    adapter = std::make_shared<NetworkAdapter>("label");
    EXPECT_EQ(adapter->StartAdapter(), -E_INVALID_ARGS);
    /**
     * @tc.steps: step3. processCommunicator start not ok
     * @tc.expected: step3. start failed
     */
    adapter = std::make_shared<NetworkAdapter>("label", processCommunicator);
    EXPECT_CALL(*processCommunicator, Start).WillRepeatedly(testing::Return(DB_ERROR));
    EXPECT_EQ(adapter->StartAdapter(), -E_PERIPHERAL_INTERFACE_FAIL);
    /**
     * @tc.steps: step4. processCommunicator reg not ok
     * @tc.expected: step4. start failed
     */
    EXPECT_CALL(*processCommunicator, Start).WillRepeatedly(testing::Return(OK));
    EXPECT_CALL(*processCommunicator, RegOnDataReceive).WillRepeatedly(testing::Return(DB_ERROR));
    EXPECT_EQ(adapter->StartAdapter(), -E_PERIPHERAL_INTERFACE_FAIL);
    EXPECT_CALL(*processCommunicator, RegOnDataReceive).WillRepeatedly(testing::Return(OK));
    EXPECT_CALL(*processCommunicator, RegOnDeviceChange).WillRepeatedly(testing::Return(DB_ERROR));
    EXPECT_EQ(adapter->StartAdapter(), -E_PERIPHERAL_INTERFACE_FAIL);
    /**
     * @tc.steps: step5. processCommunicator reg ok
     * @tc.expected: step5. start success
     */
    EXPECT_CALL(*processCommunicator, RegOnDeviceChange).WillRepeatedly(testing::Return(OK));
    EXPECT_CALL(*processCommunicator, GetLocalDeviceInfos).WillRepeatedly([]() {
        DeviceInfos deviceInfos;
        deviceInfos.identifier = "DEVICES_A"; // local is deviceA
        return deviceInfos;
    });
    EXPECT_CALL(*processCommunicator, GetRemoteOnlineDeviceInfosList).WillRepeatedly([]() {
        std::vector<DeviceInfos> res;
        DeviceInfos deviceInfos;
        deviceInfos.identifier = "DEVICES_A"; // search local is deviceA
        res.push_back(deviceInfos);
        deviceInfos.identifier = "DEVICES_B"; // search remote is deviceB
        res.push_back(deviceInfos);
        return res;
    });
    EXPECT_CALL(*processCommunicator, IsSameProcessLabelStartedOnPeerDevice).WillRepeatedly([](const DeviceInfos &) {
        return false;
    });
    EXPECT_EQ(adapter->StartAdapter(), E_OK);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: NetworkAdapter002
 * @tc.desc: Test networkAdapter get mtu func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, NetworkAdapter002, TestSize.Level1)
{
    auto processCommunicator = std::make_shared<MockProcessCommunicator>();
    auto adapter = std::make_shared<NetworkAdapter>("label", processCommunicator);
    /**
     * @tc.steps: step1. processCommunicator return 0 mtu
     * @tc.expected: step1. adapter will adjust to min mtu
     */
    EXPECT_CALL(*processCommunicator, GetMtuSize).WillRepeatedly([]() {
        return 0u;
    });
    EXPECT_EQ(adapter->GetMtuSize(), DBConstant::MIN_MTU_SIZE);
    /**
     * @tc.steps: step2. processCommunicator return 2 max mtu
     * @tc.expected: step2. adapter will return min mtu util re make
     */
    EXPECT_CALL(*processCommunicator, GetMtuSize).WillRepeatedly([]() {
        return 2 * DBConstant::MAX_MTU_SIZE;
    });
    EXPECT_EQ(adapter->GetMtuSize(), DBConstant::MIN_MTU_SIZE);
    adapter = std::make_shared<NetworkAdapter>("label", processCommunicator);
    EXPECT_EQ(adapter->GetMtuSize(), DBConstant::MAX_MTU_SIZE);
}

/**
 * @tc.name: NetworkAdapter003
 * @tc.desc: Test networkAdapter get timeout func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, NetworkAdapter003, TestSize.Level1)
{
    auto processCommunicator = std::make_shared<MockProcessCommunicator>();
    auto adapter = std::make_shared<NetworkAdapter>("label", processCommunicator);
    /**
     * @tc.steps: step1. processCommunicator return 0 timeout
     * @tc.expected: step1. adapter will adjust to min timeout
     */
    EXPECT_CALL(*processCommunicator, GetTimeout).WillRepeatedly([]() {
        return 0u;
    });
    EXPECT_EQ(adapter->GetTimeout(), DBConstant::MIN_TIMEOUT);
    /**
     * @tc.steps: step2. processCommunicator return 2 max timeout
     * @tc.expected: step2. adapter will adjust to max timeout
     */
    EXPECT_CALL(*processCommunicator, GetTimeout).WillRepeatedly([]() {
        return 2 * DBConstant::MAX_TIMEOUT;
    });
    EXPECT_EQ(adapter->GetTimeout(), DBConstant::MAX_TIMEOUT);
}

/**
 * @tc.name: NetworkAdapter004
 * @tc.desc: Test networkAdapter send bytes func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, NetworkAdapter004, TestSize.Level1)
{
    auto processCommunicator = std::make_shared<MockProcessCommunicator>();
    auto adapter = std::make_shared<NetworkAdapter>("label", processCommunicator);

    EXPECT_CALL(*processCommunicator, SendData).WillRepeatedly([](const DeviceInfos &, const uint8_t *, uint32_t) {
        return OK;
    });
    /**
     * @tc.steps: step1. adapter send data with error param
     * @tc.expected: step1. adapter send failed
     */
    auto data = std::make_shared<uint8_t>(1u);
    EXPECT_EQ(adapter->SendBytes("DEVICES_B", nullptr, 1, 0), -E_INVALID_ARGS);
    EXPECT_EQ(adapter->SendBytes("DEVICES_B", data.get(), 0, 0), -E_INVALID_ARGS);
    /**
     * @tc.steps: step2. adapter send data with right param
     * @tc.expected: step2. adapter send ok
     */
    EXPECT_EQ(adapter->SendBytes("DEVICES_B", data.get(), 1, 0), E_OK);
    RuntimeContext::GetInstance()->StopTaskPool();
}

namespace {
void InitAdapter(const std::shared_ptr<NetworkAdapter> &adapter,
    const std::shared_ptr<MockProcessCommunicator> &processCommunicator,
    OnDataReceive &onDataReceive, OnDeviceChange &onDataChange)
{
    EXPECT_CALL(*processCommunicator, Stop).WillRepeatedly([]() {
        return OK;
    });
    EXPECT_CALL(*processCommunicator, Start).WillRepeatedly([](const std::string &) {
        return OK;
    });
    EXPECT_CALL(*processCommunicator, RegOnDataReceive).WillRepeatedly(
        [&onDataReceive](const OnDataReceive &callback) {
            onDataReceive = callback;
            return OK;
    });
    EXPECT_CALL(*processCommunicator, RegOnDeviceChange).WillRepeatedly(
        [&onDataChange](const OnDeviceChange &callback) {
            onDataChange = callback;
            return OK;
    });
    EXPECT_CALL(*processCommunicator, GetRemoteOnlineDeviceInfosList).WillRepeatedly([]() {
        std::vector<DeviceInfos> res;
        return res;
    });
    EXPECT_CALL(*processCommunicator, IsSameProcessLabelStartedOnPeerDevice).WillRepeatedly([](const DeviceInfos &) {
        return false;
    });
    EXPECT_EQ(adapter->StartAdapter(), E_OK);
}
}
/**
 * @tc.name: NetworkAdapter005
 * @tc.desc: Test networkAdapter receive data func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, NetworkAdapter005, TestSize.Level1)
{
    auto processCommunicator = std::make_shared<MockProcessCommunicator>();
    auto adapter = std::make_shared<NetworkAdapter>("label", processCommunicator);
    OnDataReceive onDataReceive;
    OnDeviceChange onDeviceChange;
    InitAdapter(adapter, processCommunicator, onDataReceive, onDeviceChange);
    ASSERT_NE(onDataReceive, nullptr);
    /**
     * @tc.steps: step1. adapter recv data with error param
     */
    auto data = std::make_shared<uint8_t>(1);
    DeviceInfos deviceInfos;
    onDataReceive(deviceInfos, nullptr, 1);
    onDataReceive(deviceInfos, data.get(), 0);
    /**
     * @tc.steps: step2. adapter recv data with no permission
     */
    EXPECT_CALL(*processCommunicator, CheckAndGetDataHeadInfo).WillRepeatedly(
        [](const uint8_t *, uint32_t, uint32_t &, std::vector<std::string> &) {
        return NO_PERMISSION;
    });
    onDataReceive(deviceInfos, data.get(), 1);
    EXPECT_CALL(*processCommunicator, CheckAndGetDataHeadInfo).WillRepeatedly(
        [](const uint8_t *, uint32_t, uint32_t &, std::vector<std::string> &userIds) {
            userIds.emplace_back("1");
            return OK;
    });
    /**
     * @tc.steps: step3. adapter recv data with no callback
     */
    onDataReceive(deviceInfos, data.get(), 1);
    adapter->RegBytesReceiveCallback([](const std::string &, const uint8_t *, uint32_t, const std::string &) {
    }, nullptr);
    onDataReceive(deviceInfos, data.get(), 1);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: NetworkAdapter006
 * @tc.desc: Test networkAdapter device change func
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, NetworkAdapter006, TestSize.Level1)
{
    auto processCommunicator = std::make_shared<MockProcessCommunicator>();
    auto adapter = std::make_shared<NetworkAdapter>("label", processCommunicator);
    OnDataReceive onDataReceive;
    OnDeviceChange onDeviceChange;
    InitAdapter(adapter, processCommunicator, onDataReceive, onDeviceChange);
    ASSERT_NE(onDeviceChange, nullptr);
    DeviceInfos deviceInfos;
    /**
     * @tc.steps: step1. onDeviceChange with no same process
     */
    onDeviceChange(deviceInfos, true);
    /**
     * @tc.steps: step2. onDeviceChange with same process
     */
    EXPECT_CALL(*processCommunicator, IsSameProcessLabelStartedOnPeerDevice).WillRepeatedly([](const DeviceInfos &) {
        return true;
    });
    onDeviceChange(deviceInfos, true);
    adapter->RegTargetChangeCallback([](const std::string &, bool) {
    }, nullptr);
    onDeviceChange(deviceInfos, false);
    /**
     * @tc.steps: step3. adapter send data with db_error
     * @tc.expected: step3. adapter send failed
     */
    onDeviceChange(deviceInfos, true);
    EXPECT_CALL(*processCommunicator, SendData).WillRepeatedly([](const DeviceInfos &, const uint8_t *, uint32_t) {
        return DB_ERROR;
    });
    EXPECT_CALL(*processCommunicator, IsSameProcessLabelStartedOnPeerDevice).WillRepeatedly([](const DeviceInfos &) {
        return false;
    });
    auto data = std::make_shared<uint8_t>(1);
    EXPECT_EQ(adapter->SendBytes("", data.get(), 1, 0), -E_PERIPHERAL_INTERFACE_FAIL);
    RuntimeContext::GetInstance()->StopTaskPool();
    EXPECT_EQ(adapter->IsDeviceOnline(""), false);
    ExtendInfo info;
    EXPECT_EQ(adapter->GetExtendHeaderHandle(info), nullptr);
}

/**
 * @tc.name: NetworkAdapter007
 * @tc.desc: Test networkAdapter recv invalid head length
 * @tc.type: FUNC
 * @tc.require: AR000BVDGJ
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorDeepTest, NetworkAdapter007, TestSize.Level1)
{
    auto processCommunicator = std::make_shared<MockProcessCommunicator>();
    auto adapter = std::make_shared<NetworkAdapter>("NetworkAdapter007", processCommunicator);
    OnDataReceive onDataReceive;
    OnDeviceChange onDeviceChange;
    InitAdapter(adapter, processCommunicator, onDataReceive, onDeviceChange);
    ASSERT_NE(onDeviceChange, nullptr);
    /**
     * @tc.steps: step1. CheckAndGetDataHeadInfo return invalid headLen
     * @tc.expected: step1. adapter check this len
     */
    EXPECT_CALL(*processCommunicator, CheckAndGetDataHeadInfo).WillOnce([](const uint8_t *, uint32_t, uint32_t &headLen,
        std::vector<std::string> &) {
        headLen = UINT32_MAX;
        return OK;
    });
    /**
     * @tc.steps: step2. Adapter ignore data because len is too large
     * @tc.expected: step2. BytesReceive never call
     */
    int callByteReceiveCount = 0;
    int res = adapter->RegBytesReceiveCallback([&callByteReceiveCount](const std::string &, const uint8_t *, uint32_t,
        const std::string &) {
        callByteReceiveCount++;
    }, nullptr);
    EXPECT_EQ(res, E_OK);
    std::vector<uint8_t> data = { 1u };
    DeviceInfos deviceInfos;
    onDataReceive(deviceInfos, data.data(), 1u);
    EXPECT_EQ(callByteReceiveCount, 0);
}