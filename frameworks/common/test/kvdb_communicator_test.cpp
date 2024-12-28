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

#include <gtest/gtest.h>
#include <thread>
#include "db_common.h"
#include "db_errno.h"
#include "kvdb_communicator_common.h"
#include "kvdb_data_generate_unit_test.h"
#include "kvdb_tools_unit_test.h"
#include "endian_convert.h"
#include "log_print.h"
#include "runtime_confinig.h"
#include "thread_pool_test_stub.h"
#include "virtual_communicator_aggregator.h"

using namespace std;
using namespace testing::ext;
using namespace KvDB;
using namespace KvDBUnitTest;

namespace {
    EnvHandle g_envDevice11;
    EnvHandle g_envDevice22;
    EnvHandle g_envDeviceC;

static void HandleConnectChange(OnOfflineDevice &ondeviceTest, const std::string &tar, bool isConnect)
{
    if (isConnect) {
        ondeviceTest.lineDevices.insert(tar);
        ondeviceTest.latestOnlineDevice = tar;
        ondeviceTest.latestOfflineDevice.clear();
    } else {
        ondeviceTest.lineDevices.erase(tar);
        ondeviceTest.latestOnlineDevice.clear();
        ondeviceTest.latestOfflineDevice = tar;
    }
}

class KvDBCommunicatorTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBCommunicatorTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Create and init CommunicatorAggregator and AdapterStub
     */
    LOGI("[UT][Test][SetUpTestCase] Enter.");
    bool errCodeTest = SetUpE(g_envDevice11, DEVICE_NAME_C);
    ASSERT_EQ(errCodeTest, true);
    errCodeTest = SetUpE(g_envDevice22, DEVICE_NAME_C);
    ASSERT_EQ(errCodeTest, true);
    DoRegTransformFunction();
    CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(false);
}

void KvDBCommunicatorTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Finalize and release CommunicatorAggregator and AdapterStub
     */
    LOGI("[UT][Test][TearDownTestCase] Enter.");
    std::this_thread::sleep_for(std::chrono::seconds(7)); // Wait 7 s to make sure all thread quiet and memory released
    TearDownEnv(g_envDevice11);
    TearDownEnv(g_envDevice22);
    CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(true);
}

void KvDBCommunicatorTest::SetUp()
{
    KvDBUnitTest::KvDBToolsUnitTest::PrintTestCaseInfo();
}

void KvDBCommunicatorTest::TearDown()
{
    /**
     * @tc.teardown: Wait 100 ms to make sure all thread quiet
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait 100 ms
}

/**
 * @tc.name: Communicator Management 001
 * @tc.desc: Test alloc and release communicator
 * @tc.type: FUNC
 * @tc.require: AR000BVDGG AR000CQE0L
 */
HWTEST_F(KvDBCommunicatorTest, CommunicatorManagement001Test, TestSize.Level1)
{
    int errorFlag = E_OK;
    ICommunicator *commit1 = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    ASSERT_EQ(errorFlag, E_OK);
    EXPECT_NE(commit1, nullptr);

    errorFlag = E_OK;
    ICommunicator *commit2 = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_B, errorFlag);
    ASSERT_EQ(errorFlag, E_OK);
    EXPECT_NE(commit1, nullptr);

    errorFlag = E_OK;
    ICommunicator *commC = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    EXPECT_NE(errorFlag, E_OK);
    ASSERT_EQ(commC, nullptr);

    g_envDevice11.commit1Handle->ReleaseCommunicator(commit1);
    commit1 = nullptr;
    g_envDevice11.commit1Handle->ReleaseCommunicator(commit2);
    commit2 = nullptr;

    errorFlag = E_OK;
    ICommunicator *commDD = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    ASSERT_EQ(errorFlag, E_OK);
    EXPECT_NE(commDD, nullptr);

    g_envDevice11.commit1Handle->ReleaseCommunicator(commDD);
    commDD = nullptr;
}

/**
 * @tc.name: Communicator Management 002
 * @tc.desc: Test alloc and release communicator with different userId
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(KvDBCommunicatorTest, CommunicatorManagement002Test, TestSize.Level1)
{
    int errorFlag = E_OK;
    ICommunicator *commit1 = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag, USER_ID_11);
    commit1->Activate(USER_ID_11);
    ASSERT_EQ(errorFlag, E_OK);
    EXPECT_NE(commit1, nullptr);

    errorFlag = E_OK;
    ICommunicator *commit2 = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag, USER_ID_22);
    commit2->Activate(USER_ID_22);
    ASSERT_EQ(errorFlag, E_OK);
    EXPECT_NE(commit1, nullptr);

    errorFlag = E_OK;
    ICommunicator *commC = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag, USER_ID_11);
    EXPECT_NE(errorFlag, E_OK);
    ASSERT_EQ(commC, nullptr);

    g_envDevice11.commit1Handle->ReleaseCommunicator(commit1, USER_ID_11);
    commit1 = nullptr;
    g_envDevice11.commit1Handle->ReleaseCommunicator(commit2, USER_ID_22);
    commit2 = nullptr;
}

static void ConnectWaitDisconnect()
{
    AdapterStub::ConnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    AdapterStub::DisconnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
}

/**
 * @tc.name: Online And Offline 001
 * @tc.desc: Test functionality triggered by physical deviceTests online and offline
 * @tc.type: FUNC
 * @tc.require: AR000BVRNS AR000CQE0H
 */
HWTEST_F(KvDBCommunicatorTest, OnlineAndOffline001Test, TestSize.Level1)
{
    int errorFlag = E_OK;
    ICommunicator *commitAAAA = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    ASSERT_NOT_ACTIVATE(commitAAAA, "");
    OnOfflineDevice onlineForTestAA;
    commitAAAA->RegOnConnectCallback([&onlineForTestAA](const std::string &tar, bool isConnect) {
        HandleConnectChange(onlineForTestAA, tar, isConnect);}, nullptr);
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));

    ConnectWaitDisconnect();
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));

    ICommunicator *commit2C = g_envDevice22.commit1Handle->AllocCommunicator(LABEL_B, errorFlag);
    ASSERT_NOT_ACTIVATE(commit2C, "");
    OnOfflineDevice onlineForTestBB;
    commit2C->RegOnConnectCallback([&onlineForTestBB](const std::string &tar, bool isConnect) {
        HandleConnectChange(onlineForTestBB, tar, isConnect);}, nullptr);
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(0));
    ConnectWaitDisconnect();
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(0));
    ICommunicator *commit2D = g_envDevice22.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    ASSERT_NOT_ACTIVATE(commit2D, "");
    OnOfflineDevice onlineForTestBA;
    commit2D->RegOnConnectCallback([&onlineForTestBA](const std::string &tar, bool isConnect) {
        HandleConnectChange(onlineForTestBA, tar, isConnect);}, nullptr);
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));

    AdapterStub::ConnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(1));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestBA.latestOnlineDevice, DEVICE_NAME_C);

    AdapterStub::DisconnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestAA.latestOfflineDevice, DEVICE_NAME_C);
    ASSERT_EQ(onlineForTestBA.latestOfflineDevice, DEVICE_NAME_C);

    // Clean up
    g_envDevice11.commit1Handle->ReleaseCommunicator(commitAAAA);
    g_envDevice22.commit1Handle->ReleaseCommunicator(commit2C);
    g_envDevice22.commit1Handle->ReleaseCommunicator(commit2D);
}

/**
 * @tc.name: Online And Offline 002
 * @tc.desc: Test functionality triggered by alloc and release communicator
 * @tc.type: FUNC
 * @tc.require: AR000BVRNT AR000CQE0I
 */
HWTEST_F(KvDBCommunicatorTest, OnlineAndOffline002Test, TestSize.Level1)
{
    CONNECT_AND_WAIT(200); // Sleep 200 ms

    int errorFlag = E_OK;
    ICommunicator *commitAAAA = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    ASSERT_NOT_ACTIVATE(commitAAAA, "");
    OnOfflineDevice onlineForTestAA;
    REG_CONNECT_CALLBACK(commitAAAA, onlineForTestAA);
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));

    ICommunicator *commit2C = g_envDevice22.commit1Handle->AllocCommunicator(LABEL_B, errorFlag);
    ASSERT_NOT_ACTIVATE(commit2C, "");
    OnOfflineDevice onlineForTestBB;
    REG_CONNECT_CALLBACK(commit2C, onlineForTestBB);
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(0));
    ICommunicator *commit2D = g_envDevice22.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    ASSERT_NOT_ACTIVATE(commit2D, "");
    OnOfflineDevice onlineForTestBA;
    REG_CONNECT_CALLBACK(commit2D, onlineForTestBA);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(1));
    ASSERT_EQ(onlineForTestAA.latestOnlineDevice, DEVICE_NAME_C);
    ASSERT_EQ(onlineForTestBA.latestOnlineDevice, DEVICE_NAME_C);
    ICommunicator *commit1B = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_B, errorFlag);
    ASSERT_NOT_ACTIVATE(commit1B, "");
    OnOfflineDevice onlineForTestAB;
    REG_CONNECT_CALLBACK(commit1B, onlineForTestAB);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(onlineForTestAB.lineDevices.size(), static_cast<size_t>(1));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(1));
    g_envDevice11.commit1Handle->ReleaseCommunicator(commitAAAA);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(onlineForTestBA.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestAB.lineDevices.size(), static_cast<size_t>(1));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(1));
    ASSERT_EQ(onlineForTestBA.latestOfflineDevice, DEVICE_NAME_C);

    g_envDevice22.commit1Handle->ReleaseCommunicator(commit2C);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(onlineForTestAB.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestAB.latestOfflineDevice, DEVICE_NAME_C);
    // Clean up
    g_envDevice11.commit1Handle->ReleaseCommunicator(commit1B);
    AdapterStub::DisconnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
}

void TestRemoteRestart()
{
    EnvHandle envDeviceD;
    EnvHandle envDeviceE;
    SetUpE(envDeviceD, "DEVICE_DONE");
    SetUpE(envDeviceE, "DEVICE_EX");

    int errorFlag = E_OK;
    ICommunicator *commDDD = envDeviceD.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    ASSERT_NOT_ACTIVATE(commDDD, "");
    OnOfflineDevice onlineForTestDD;
    OnOfflineDevice onlineForTestEE;
    REG_CONNECT_CALLBACK(commEE, onlineForTestEE);
    AdapterStub::ConnectAdapterStub(envDeviceD.adapterTestHandle, envDeviceE.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(onlineForTestDD.lineDevices.size(), static_cast<size_t>(1));
    ASSERT_EQ(onlineForTestEE.lineDevices.size(), static_cast<size_t>(1));
    envDeviceE.commit1Handle->ReleaseCommunicator(commEE);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    TearDownEnv(envDeviceE);
    SetUpE(envDeviceE, "DEVICE_EX");
    commEE = envDeviceE.commit1Handle->AllocCommunicator(LABEL_Q, errorFlag);
    ASSERT_NOT_ACTIVATE(commEE, "");
    REG_CONNECT_CALLBACK(commEE, onlineForTestEE);
    onlineForTestEE.lineDevices.clear();
    AdapterStub::ConnectAdapterStub(envDeviceD.adapterTestHandle, envDeviceE.adapterTestHandle);
    while (onlineForTestEE.lineDevices.size() != 1 && reTryTimes > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        reTryTimes--;
    }
    ASSERT_EQ(onlineForTestEE.lineDevices.size(), static_cast<size_t>(1));
    // Clean up and disconnect
    envDeviceD.commit1Handle->ReleaseCommunicator(commDDD);
    envDeviceE.commit1Handle->ReleaseCommunicator(commEE);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AdapterStub::DisconnectAdapterStub(envDeviceD.adapterTestHandle, envDeviceE.adapterTestHandle);
    TearDownEnv(envDeviceD);
    TearDownEnv(envDeviceE);
}
/**
 * @tc.name: Online And Offline 003
 * @tc.desc: Test functionality triggered by remote restart
 * @tc.type: FUNC
 * @tc.require: AR000CQE0I
 */
HWTEST_F(KvDBCommunicatorTest, OnlineAndOffline003Test, TestSize.Level1)
{
    TestRemoteRestart();
}

/**
 * @tc.name: Online And Offline 004
 * @tc.desc: Test functionality triggered by remote restart with thread pool
 * @tc.type: FUNC
 * @tc.require: AR000CQE0I
 */
HWTEST_F(KvDBCommunicatorTest, OnlineAndOffline004Test, TestSize.Level1)
{
    auto threadPoolTest = std::make_shared<ThreadPoolTestStub>();
    RuntimeContext::GetInstance()->SetThreadPool(threadPoolTest);
    TestRemoteRestart();
    RuntimeContext::GetInstance()->SetThreadPool(nullptr);
}

/**
 * @tc.name: Report Device Connect Change 001
 * @tc.desc: Test CommunicatorAggregator support report deviceTest connect change event
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 */
HWTEST_F(KvDBCommunicatorTest, ReportDeviceConnectChange001Test, TestSize.Level1)
{
    /**
     * @tc.steps: step1. deviceTest A and deviceTest B register connect callback to CommunicatorAggregator
     */
    OnOfflineDevice onlineForTestA;
    int errCodeTest = g_envDevice11.commit1Handle->RegOnConnectCallback(
        [&onlineForTestA](const std::string &tar, bool isConnect) {
            HandleConnectChange(onlineForTestA, tar, isConnect);
        }, nullptr);
    ASSERT_EQ(errCodeTest, E_OK);
    OnOfflineDevice onlineForTestB;
    errCodeTest = g_envDevice22.commit1Handle->RegOnConnectCallback(
        [&onlineForTestB](const std::string &tar, bool isConnect) {
            HandleConnectChange(onlineForTestB, tar, isConnect);
        }, nullptr);
    ASSERT_EQ(errCodeTest, E_OK);

    AdapterStub::ConnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(onlineForTestA.lineDevices.size(), static_cast<size_t>(1));
    ASSERT_EQ(onlineForTestB.lineDevices.size(), static_cast<size_t>(1));
    ASSERT_EQ(onlineForTestA.latestOnlineDevice, DEVICE_NAME_C);
    ASSERT_EQ(onlineForTestB.latestOnlineDevice, DEVICE_NAME_C);
    AdapterStub::DisconnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(onlineForTestA.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestB.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestA.latestOfflineDevice, DEVICE_NAME_C);
    ASSERT_EQ(onlineForTestB.latestOfflineDevice, DEVICE_NAME_C);

    // Clean up
    g_envDevice11.commit1Handle->RegOnConnectCallback(nullptr, nullptr);
    g_envDevice22.commit1Handle->RegOnConnectCallback(nullptr, nullptr);
}

namespace {
LabelType ToLabelType(uint64_t commLabel)
{
    uint64_t netOrderLabel = HostToNet(commLabel);
    uint8_t *eByte = reinterpret_cast<uint8_t *>(&netOrderLabel);
    std::vector<uint8_t> realLabel(COMM_LABEL_LENGTH, 0);
    for (int i = 0; i < static_cast<int>(sizeof(uint64_t)); i++) {
        realLabel[i] = eByte[i];
    }
    return realLabel;
}
}

/**
 * @tc.name: Report Communicator Not Found 001
 * @tc.desc: Test CommunicatorAggregator support report communicator not found event
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 */
HWTEST_F(KvDBCommunicatorTest, ReportCommunicatorNotFound001Test, TestSize.Level1)
{
    /**
     * @tc.steps: step1. deviceTest B register communicator not found callback to CommunicatorAggregator
     */
    std::vector<LabelType> lackLabelItem;
    int errCodeTest = g_envDevice22.commit1Handle->RegCommunicatorLackCallback(
        [&lackLabelItem](const LabelType &commLabel, const std::string &userId)->int {
            lackLabelItem.push_back(commLabel);
            return -E_NOT_FOUND;
        }, nullptr);
    ASSERT_EQ(errCodeTest, E_OK);

    AdapterStub::ConnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    ICommunicator *commitAAAA = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errCodeTest);
    ASSERT_NOT_ACTIVATE(commitAAAA, "");
    Message *msg = BuildRegedTinyMessage();
    ASSERT_NE(msg, nullptr);
    SendConfig confin = {true, false, 0};
    errCodeTest = commitAAAA->SendMessage(DEVICE_NAME_C, msg, confin);
    ASSERT_EQ(errCodeTest, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(lackLabelItem.size(), static_cast<size_t>(1));
    ASSERT_EQ(lackLabelItem[0], ToLabelType(LABEL_Q));

    ICommunicator *commit2D = g_envDevice22.commit1Handle->AllocCommunicator(LABEL_Q, errCodeTest);
    ASSERT_NE(commit2D, nullptr);
    Message *recvMsgForBC = nullptr;
    commit2D->RegOnMessageCallback([&recvMsgForBC](const std::string &srcTar, Message *inMess) {
        recvMsgForBC = inMess;
    }, nullptr);
    commit2D->Activate(USER_ID_11);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(recvMsgForBC, nullptr);

    // Clean up
    g_envDevice11.commit1Handle->ReleaseCommunicator(commitAAAA);
    g_envDevice22.commit1Handle->ReleaseCommunicator(commit2D);
    g_envDevice22.commit1Handle->RegCommunicatorLackCallback(nullptr, nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
}

/**
 * @tc.name: ReDeliver Message 001
 * @tc.desc: Test CommunicatorAggregator support redeliver message
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 */
HWTEST_F(KvDBCommunicatorTest, ReDeliverMessage001Test, TestSize.Level1)
{
    std::vector<LabelType> lackLabelItem;
    int errCodeTest = g_envDevice22.commit1Handle->RegCommunicatorLackCallback(
        [&lackLabelItem](const LabelType &commLabel, const std::string &userId)->int {
            lackLabelItem.push_back(commLabel);
            return E_OK;
        }, nullptr);
    ASSERT_EQ(errCodeTest, E_OK);
    AdapterStub::ConnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ALLOC_AND_SEND_MESSAGE(A, B, A, 100); // session id 100
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(lackLabelItem.size(), static_cast<size_t>(1));
    ASSERT_EQ(lackLabelItem[0], ToLabelType(LABEL_Q));
    ICommunicator *commit2D = g_envDevice22.commit1Handle->AllocCommunicator(LABEL_Q, errCodeTest);
    ASSERT_NE(commit2D, nullptr);
    REG_MESSAGE_CALLBACK(B, A);
    commit2D->Activate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(srcTarForBA, DEVICE_NAME_C);
    ASSERT_NE(recvMsgForBC, nullptr);
    ASSERT_EQ(recvMsgForBC->GetSessionId(), 100U); // session id 100
    delete recvMsgForBC;
    recvMsgForBC = nullptr;
    ICommunicator *commit2C = g_envDevice22.commit1Handle->AllocCommunicator(LABEL_B, errCodeTest);
    ASSERT_NE(commit2C, nullptr);
    REG_MESSAGE_CALLBACK(B, B);
    commit2C->Activate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(srcTarForBB, DEVICE_NAME_C);
    ASSERT_NE(recvMsgForBB, nullptr);
    ASSERT_EQ(recvMsgForBB->GetSessionId(), 200U); // session id 200
    delete recvMsgForBB;
    recvMsgForBB = nullptr;
    // Clean up
    g_envDevice11.commit1Handle->ReleaseCommunicator(commitAAAA);
    g_envDevice11.commit1Handle->ReleaseCommunicator(commit1B);
    g_envDevice22.commit1Handle->ReleaseCommunicator(commit2D);
    g_envDevice22.commit1Handle->ReleaseCommunicator(commit2C);
    g_envDevice22.commit1Handle->RegCommunicatorLackCallback(nullptr, nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
}

/**
 * @tc.name: ReDeliver Message 002
 * @tc.desc: Test CommunicatorAggregator support redeliver message by order
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 */
HWTEST_F(KvDBCommunicatorTest, ReDeliverMessage002Test, TestSize.Level1)
{
    bool step1 = SetUpE(g_envDeviceC, DEVICE_NAME_C);
    ASSERT_EQ(step1, true);

    int errCodeTest = g_envDevice22.commit1Handle->RegCommunicatorLackCallback([](const LabelType &commLabel,
        const std::string &userId)->int {
        return E_OK;
    }, nullptr);
    ASSERT_EQ(errCodeTest, E_OK);
    AdapterStub::ConnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    AdapterStub::ConnectAdapterStub(g_envDevice22.adapterTestHandle, g_envDeviceC.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ALLOC_AND_SEND_MESSAGE(A, B, A, 100); // session id 100
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    DO_SEND_MESS(C, B, A, 400); // session id 400
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ICommunicator *commit2D = g_envDevice22.commit1Handle->AllocCommunicator(LABEL_Q, errCodeTest);
    ASSERT_NE(commit2D, nullptr);
    std::vector<std::pair<std::string, Message *>> msgCallbackForBA;
    commit2D->RegOnMessageCallback([&msgCallbackForBA](const std::string &srcTar, Message *inMess) {
        msgCallbackForBA.push_back({srcTar, inMess});
    }, nullptr);
    commit2D->Activate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(msgCallbackForBA.size(), static_cast<size_t>(4)); // total 4 callback
    ASSERT_EQ(msgCallbackForBA[0].first, DEVICE_NAME_C); // the 0 order element
    ASSERT_EQ(msgCallbackForBA[1].first, DEVICE_NAME_C); // the 1 order element
    ASSERT_EQ(msgCallbackForBA[2].first, DEVICE_NAME_C); // the 2 order element
    ASSERT_EQ(msgCallbackForBA[3].first, DEVICE_NAME_C); // the 3 order element
    for (uint32_t i = 0; i < msgCallbackForBA.size(); i++) {
        ASSERT_EQ(msgCallbackForBA[i].second->GetSessionId(), static_cast<uint32_t>((i + 1) * 100)); // 1 sessionid 100
        delete msgCallbackForBA[i].second;
        msgCallbackForBA[i].second = nullptr;
    }
    // Clean up
    g_envDevice11.commit1Handle->ReleaseCommunicator(commitAAAA);
    g_envDeviceC.commit1Handle->ReleaseCommunicator(commCA);
    g_envDevice22.commit1Handle->ReleaseCommunicator(commit2D);
    g_envDevice22.commit1Handle->RegCommunicatorLackCallback(nullptr, nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    AdapterStub::DisconnectAdapterStub(g_envDevice22.adapterTestHandle, g_envDeviceC.adapterTestHandle);
    TearDownEnv(g_envDeviceC);
}

/**
 * @tc.name: ReDeliver Message 003
 * @tc.desc: For observe memory in unusual scenario
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 */
HWTEST_F(KvDBCommunicatorTest, ReDeliverMessage003Test, TestSize.Level2)
{
    int errCodeTest = g_envDevice22.commit1Handle->RegCommunicatorLackCallback([](const LabelType &commLabel,
        const std::string &userId)->int {
        return E_OK;
    }, nullptr);
    ASSERT_EQ(errCodeTest, E_OK);

    AdapterStub::ConnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ICommunicator *commitAAAA = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_Q, errCodeTest);
    ASSERT_NOT_ACTIVATE(commitAAAA, "");
    ICommunicator *commit1B = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_B, errCodeTest);
    ASSERT_NOT_ACTIVATE(commit1B, "");
    ICommunicator *commit1C = g_envDevice11.commit1Handle->AllocCommunicator(LABEL_C, errCodeTest);
    ASSERT_NOT_ACTIVATE(commit1C, "");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    /**
     * @tc.steps: step4. deviceTest A Continuously send tiny message to B using communicator AA,AB,AC
     */
    for (int turn = 0; turn < 11; turn++) { // Total 11 turns
        DO_SEND_MESS(A, B, A, 0);
        DO_SEND_MESS(A, B, B, 0);
        DO_SEND_MESS(A, B, C, 0);
    }

    for (int turn = 0; turn < 5; turn++) { // Total 5 turns
        DO_SEND_GIANT_MESSAGE(A, B, A, (3 * 1024 * 1024)); // 3 MBytes, 1024 is scale
        DO_SEND_GIANT_MESSAGE(A, B, B, (6 * 1024 * 1024)); // 6 MBytes, 1024 is scale
        DO_SEND_GIANT_MESSAGE(A, B, C, (7 * 1024 * 1024)); // 7 MBytes, 1024 is scale
    }
    DO_SEND_GIANT_MESSAGE(A, B, A, (30 * 1024 * 1024)); // 30 MBytes, 1024 is scale
    for (int sec = 0; sec < 15; sec++) { // Total 15 s
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep 1 s
        LOGI("[UT][Test][ReDeliverMessage003] Sleep and wait=%d.", sec);
    }
    DO_SEND_MESS(A, B, A, 0);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep 1 s

    // Clean up
    g_envDevice11.commit1Handle->ReleaseCommunicator(commitAAAA);
    g_envDevice11.commit1Handle->ReleaseCommunicator(commit1B);
    g_envDevice11.commit1Handle->ReleaseCommunicator(commit1C);
    g_envDevice22.commit1Handle->RegCommunicatorLackCallback(nullptr, nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDevice11.adapterTestHandle, g_envDevice22.adapterTestHandle);
}

namespace {
    void SetUpE(const std::string &localDev, const std::string &supportDev,
        const std::shared_ptr<DBStatusAdapter> &adapterTest, bool isSupport, EnvHandle &envDevice)
    {
        std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
        adapterTest->SetDBInfoHandle(handle);
        adapterTest->SetRemoteOptimizeCommunication(supportDev, isSupport);
        SetUpE(envDevice, localDev, adapterTest);
    }

    void InitCommunicator(DBInfo &dbInfoTest, EnvHandle &envDevice, OnOfflineDevice &onlineCallback)
    {
        dbInfoTest.userId = USER_ID;
        dbInfoTest.appId = APP_ID;
        dbInfoTest.storeId = STORE_ID_1;
        dbInfoTest.isNeedSync = true;
        dbInfoTest.syncDualTupleMode = false;
        std::string label = DBCommon::GenerateHashLabel(dbInfoTest);
        std::vector<uint8_t> commLabel(label.begin(), label.end());
        int errorFlag = E_OK;
        comm = envDevice.commit1Handle->AllocCommunicator(commLabel, errorFlag);
        ASSERT_NOT_ACTIVATE(comm, "");
        REG_CONNECT_CALLBACK(comm, onlineCallback);
    }
}

/**
  * @tc.name: CommunicationOptimization001
  * @tc.desc: Test notify with isSupport true.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBCommunicatorTest, CommunicationOptimization001Test, TestSize.Level3)
{
    auto *pAggregatorTest = new VirtualCommunicatorAggregator();
    ASSERT_NE(pAggregatorTest, nullptr);
    const std::string deviceTestA = "DEVICES_A";
    const std::string deviceTestB = "DEVICES_B";
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(pAggregatorTest);

    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    SetUpE(deviceTestA, deviceTestB, adapterTestA, true, envDeviceA);
    EnvHandle envDeviceBTest;
    std::shared_ptr<DBStatusAdapter> adapterTestB = std::make_shared<DBStatusAdapter>();
    SetUpE(deviceTestB, deviceTestA, adapterTestB, true, envDeviceBTest);

    DBInfo dbInfoTest;
    ICommunicator *commitAAAA = nullptr;
    OnOfflineDevice onlineForTestAA;
    InitCommunicator(dbInfoTest, envDeviceA, onlineForTestAA, commitAAAA);
    ICommunicator *commit2C = nullptr;
    OnOfflineDevice onlineForTestBB;
    InitCommunicator(dbInfoTest, envDeviceBTest, onlineForTestBB, commit2C);

    AdapterStub::ConnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(0));

    RuntimeConfig::NotifyDBInfos({ deviceTestB }, { dbInfoTest });
    adapterTestA->NotifyDBInfos({ deviceTestB }, { dbInfoTest });
    adapterTestB->NotifyDBInfos({ deviceTestA }, { dbInfoTest });
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(1));

    dbInfoTest.isNeedSync = false;
    adapterTestA->NotifyDBInfos({ deviceTestB }, { dbInfoTest });

    // Clean up and disconnect
    envDeviceA.commit1Handle->ReleaseCommunicator(commitAAAA);
    envDeviceBTest.commit1Handle->ReleaseCommunicator(commit2C);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);
    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceBTest);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

/**
  * @tc.name: CommunicationOptimization002
  * @tc.desc: Test notify with isSupport true and can offline by deviceTest change.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBCommunicatorTest, CommunicationOptimization002Test, TestSize.Level3)
{
    auto *pAggregatorTest = new VirtualCommunicatorAggregator();
    ASSERT_NE(pAggregatorTest, nullptr);
    const std::string deviceTestA = "DEVICES_A";
    const std::string deviceTestB = "DEVICES_B";
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(pAggregatorTest);

    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    SetUpE(deviceTestA, deviceTestB, adapterTestA, true, envDeviceA);

    EnvHandle envDeviceBTest;
    std::shared_ptr<DBStatusAdapter> adapterTestB = std::make_shared<DBStatusAdapter>();
    SetUpE(deviceTestB, deviceTestA, adapterTestB, true, envDeviceBTest);

    DBInfo dbInfoTest;
    ICommunicator *commitAAAA = nullptr;
    OnOfflineDevice onlineForTestAA;
    InitCommunicator(dbInfoTest, envDeviceA, onlineForTestAA, commitAAAA);

    AdapterStub::ConnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    adapterTestA->NotifyDBInfos({ deviceTestB }, { dbInfoTest });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(1));
    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));
    // Clean up and disconnect
    envDeviceA.commit1Handle->ReleaseCommunicator(commitAAAA);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceBTest);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

/**
  * @tc.name: CommunicationOptimization003
  * @tc.desc: Test notify with isSupport false.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  */
HWTEST_F(KvDBCommunicatorTest, CommunicationOptimization003Test, TestSize.Level3)
{
    auto *pAggregatorTest = new VirtualCommunicatorAggregator();
    ASSERT_NE(pAggregatorTest, nullptr);
    const std::string deviceTestA = "DEVICES_A";
    const std::string deviceTestB = "DEVICES_B";
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(pAggregatorTest);

    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    SetUpE(deviceTestA, deviceTestB, adapterTestA, false, envDeviceA);
    EnvHandle envDeviceBTest;
    std::shared_ptr<DBStatusAdapter> adapterTestB = std::make_shared<DBStatusAdapter>();
    SetUpE(deviceTestB, deviceTestA, adapterTestB, false, envDeviceBTest);

    DBInfo dbInfoTest;
    ICommunicator *commitAAAA = nullptr;
    OnOfflineDevice onlineForTestAA;
    InitCommunicator(dbInfoTest, envDeviceA, onlineForTestAA, commitAAAA);
    ICommunicator *commit2C = nullptr;
    OnOfflineDevice onlineForTestBB;
    InitCommunicator(dbInfoTest, envDeviceBTest, onlineForTestBB, commit2C);

    AdapterStub::ConnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);

    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(0));

    adapterTestA->NotifyDBInfos({ deviceTestB }, { dbInfoTest });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(0));

    dbInfoTest.isNeedSync = false;
    onlineForTestAA.lineDevices.clear();
    adapterTestA->NotifyDBInfos({ deviceTestA }, { dbInfoTest });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(onlineForTestAA.lineDevices.size(), static_cast<size_t>(0));
    // Clean up and disconnect
    envDeviceA.commit1Handle->ReleaseCommunicator(commitAAAA);
    envDeviceBTest.commit1Handle->ReleaseCommunicator(commit2C);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);
    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceBTest);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

/**
  * @tc.name: CommunicationOptimization004
  * @tc.desc: Test notify with isSupport false and it be will changed by communication.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBCommunicatorTest, CommunicationOptimization004Test, TestSize.Level3)
{
    const std::string deviceTestA = "DEVICES_A";
    const std::string deviceTestB = "DEVICES_B";
    /**
     * @tc.steps: step1. set up env
     */
    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    SetUpE(deviceTestA, deviceTestB, adapterTestA, false, envDeviceA);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(envDeviceA.commit1Handle);
    EnvHandle envDeviceBTest;
    std::shared_ptr<DBStatusAdapter> adapterTestB = std::make_shared<DBStatusAdapter>();
    SetUpE(deviceTestB, deviceTestA, adapterTestB, false, envDeviceBTest);
    DBInfo dbInfoTest;
    ICommunicator *commitAAAA = nullptr;
    OnOfflineDevice onlineForTestAA;
    InitCommunicator(dbInfoTest, envDeviceA, onlineForTestAA, commitAAAA);
    ICommunicator *commit2C = nullptr;
    OnOfflineDevice onlineForTestBB;
    InitCommunicator(dbInfoTest, envDeviceBTest, onlineForTestBB, commit2C);
    /**
     * @tc.steps: step3. connect deviceTest A with deviceTest B
     */
    ASSERT_EQ(adapterTestA->IsSupport(deviceTestB), false);
    AdapterStub::ConnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(adapterTestA->IsSupport(deviceTestB), true);
    // Clean up and disconnect
    envDeviceA.commit1Handle->ReleaseCommunicator(commitAAAA);
    envDeviceBTest.commit1Handle->ReleaseCommunicator(commit2C);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    envDeviceA.commit1Handle = nullptr;
    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceBTest);
}

/**
  * @tc.name: CommunicationOptimization005
  * @tc.desc: Test notify with isSupport false and send label exchange.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  */
HWTEST_F(KvDBCommunicatorTest, CommunicationOptimization005Test, TestSize.Level3)
{
    const std::string deviceTestA = "DEVICES_A";
    const std::string deviceTestB = "DEVICES_B";
    /**
     * @tc.steps: step1. set up env
     */
    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    handle->SetLocalIsSupport(false);
    adapterTestA->SetDBInfoHandle(handle);
    SetUpE(envDeviceA, deviceTestA, adapterTestA);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(envDeviceA.commit1Handle);
    EnvHandle envDeviceBTest;
    SetUpE(envDeviceBTest, deviceTestB, nullptr);
    ASSERT_EQ(adapterTestA->IsSupport(deviceTestB), false);
    AdapterStub::ConnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    DBInfo dbInfoTest;
    ICommunicator *commitAAAA = nullptr;
    OnOfflineDevice onlineForTestAA;
    InitCommunicator(dbInfoTest, envDeviceA, onlineForTestAA, commitAAAA);
    ICommunicator *commit2C = nullptr;
    OnOfflineDevice onlineForTestBB;
    InitCommunicator(dbInfoTest, envDeviceBTest, onlineForTestBB, commit2C);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(onlineForTestBB.lineDevices.size(), static_cast<size_t>(1));

    // Clean up and disconnect
    envDeviceA.commit1Handle->ReleaseCommunicator(commitAAAA);
    envDeviceBTest.commit1Handle->ReleaseCommunicator(commit2C);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterTestHandle, envDeviceBTest.adapterTestHandle);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    envDeviceA.commit1Handle = nullptr;
    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceBTest);
}

/**
  * @tc.name: DbStatusAdapter001
  * @tc.desc: Test notify with isSupport false.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  */
HWTEST_F(KvDBCommunicatorTest, DbStatusAdapter001Test, TestSize.Level1)
{
    auto *pAggregatorTest = new VirtualCommunicatorAggregator();
    ASSERT_NE(pAggregatorTest, nullptr);
    const std::string deviceTestA = "DEVICES_A";
    const std::string deviceTestB = "DEVICES_B";
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(pAggregatorTest);
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterTestA->SetDBInfoHandle(handle);
    adapterTestA->SetRemoteOptimizeCommunication(deviceTestB, true);
    std::string actualRemoteDevInfo;
    size_t remoteInfoCount = 0u;
    size_t localCount = 0u;
    DBInfo dbInfoTest;
    adapterTestA->NotifyDBInfos({ deviceTestA }, { dbInfoTest });
    dbInfoTest = {
        USER_ID,
        APP_ID,
        STORE_ID_1,
        true,
        false
    };
    adapterTestA->NotifyDBInfos({ deviceTestA }, { dbInfoTest });
    dbInfoTest.isNeedSync = false;
    adapterTestA->NotifyDBInfos({ deviceTestA }, { dbInfoTest });
    adapterTestA->NotifyDBInfos({ deviceTestB }, { dbInfoTest });
    size_t remoteChangeCount = 0u;
    adapterTestA->SetDBStatusChangeCallback(
        [&actualRemoteDevInfo, &remoteInfoCount](const std::string &devInfo, const std::vector<DBInfo> &dbInfoTests) {
            actualRemoteDevInfo = devInfo;
            remoteInfoCount = dbInfoTests.size();
        },
        [&localCount]() {
            localCount++;
        },
        [&remoteChangeCount, deviceTestB](const std::string &dev) {
            remoteChangeCount++;
            ASSERT_EQ(dev, deviceTestB);
        });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(actualRemoteDevInfo, deviceTestB);
    ASSERT_EQ(remoteInfoCount, 1u);
    adapterTestA->SetRemoteOptimizeCommunication(deviceTestB, false);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(remoteChangeCount, 1u);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

/**
  * @tc.name: DbStatusAdapter002
  * @tc.desc: Test adapterTest clear cache.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  */
HWTEST_F(KvDBCommunicatorTest, DbStatusAdapter002Test, TestSize.Level1) {
    const std::string deviceTestB = "DEVICES_B";
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterTestA->SetDBInfoHandle(handle);
    adapterTestA->SetRemoteOptimizeCommunication(deviceTestB, true);
    EXPECT_TRUE(adapterTestA->IsSupport(deviceTestB));
    adapterTestA->SetDBInfoHandle(handle);
    adapterTestA->SetRemoteOptimizeCommunication(deviceTestB, false);
    ASSERT_FALSE(adapterTestA->IsSupport(deviceTestB));
}

/**
  * @tc.name: DbStatusAdapter003
  * @tc.desc: Test adapterTest get local dbInfoTest.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  */
HWTEST_F(KvDBCommunicatorTest, DbStatusAdapter003Test, TestSize.Level1) {
    const std::string deviceTestB = "DEVICES_B";
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterTestA->SetDBInfoHandle(handle);
    handle->SetLocalIsSupport(true);
    std::vector<DBInfo> dbInfoTests;
    ASSERT_EQ(adapterTestA->GetLocalDBInfos(dbInfoTests), E_OK);
    handle->SetLocalIsSupport(false);
    ASSERT_EQ(adapterTestA->GetLocalDBInfos(dbInfoTests), E_OK);
    adapterTestA->SetDBInfoHandle(handle);
    ASSERT_EQ(adapterTestA->GetLocalDBInfos(dbInfoTests), -E_NOT_SUPPORT);
}

/**
  * @tc.name: DbStatusAdapter004
  * @tc.desc: Test adapterTest clear cache will get callback.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  */
HWTEST_F(KvDBCommunicatorTest, DbStatusAdapter004Test, TestSize.Level1)
{
    const std::string deviceTestB = "DEVICES_B";
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterTestA->SetDBInfoHandle(handle);
    handle->SetLocalIsSupport(true);
    std::vector<DBInfo> dbInfoTests;
    DBInfo dbInfoTest = {
        USER_ID,
        APP_ID,
        STORE_ID_1,
        true,
        true
    };
    dbInfoTests.push_back(dbInfoTest);
    dbInfoTest.storeId = STORE_ID_2;
    dbInfoTests.push_back(dbInfoTest);
    dbInfoTest.storeId = STORE_ID_3;
    dbInfoTest.isNeedSync = false;
    dbInfoTests.push_back(dbInfoTest);
    adapterTestA->NotifyDBInfos({"dev"}, dbInfoTests);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    size_t notifyCount = 0u;
    adapterTestA->SetDBStatusChangeCallback([&notifyCount](const std::string &devInfo,
        const std::vector<DBInfo> &dbInfoTests) {
        LOGD("on callback");
        for (const auto &dbInfoTest: dbInfoTests) {
            ASSERT_FALSE(dbInfoTest.isNeedSync);
        }
        notifyCount = dbInfoTests.size();
    }, nullptr, nullptr);
    adapterTestA->SetDBInfoHandle(nullptr);
    ASSERT_EQ(notifyCount, 2u); // 2 dbInfoTest is need sync now it should be offline
}

/**
  * @tc.name: DbStatusAdapter005
  * @tc.desc: Test adapterTest is need auto sync.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  */
HWTEST_F(KvDBCommunicatorTest, DbStatusAdapter005Test, TestSize.Level1)
{
    const std::string deviceTestB = "DEVICES_B";
    std::shared_ptr<DBStatusAdapter> adapterTestA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterTestA->SetDBInfoHandle(handle);
    handle->SetLocalIsSupport(true);
    ASSERT_EQ(adapterTestA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceTestB), true);
    handle->SetNeedAutoSync(false);
    ASSERT_EQ(adapterTestA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceTestB), false);
    handle->SetLocalIsSupport(false);
    ASSERT_EQ(adapterTestA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceTestB), false);
    adapterTestA->SetDBInfoHandle(handle);
    ASSERT_EQ(adapterTestA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceTestB), true);
    adapterTestA->SetDBInfoHandle(nullptr);
    ASSERT_EQ(adapterTestA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceTestB), true);
}
}