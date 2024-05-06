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
#include <thread>
#include "db_common.h"
#include "db_errno.h"
#include "distributeddb_communicator_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "endian_convert.h"
#include "log_print.h"
#include "runtime_config.h"
#include "thread_pool_test_stub.h"
#include "virtual_communicator_aggregator.h"

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    EnvHandle g_envDeviceA;
    EnvHandle g_envDeviceB;
    EnvHandle g_envDeviceC;

static void HandleConnectChange(OnOfflineDevice &onlines, const std::string &target, bool isConnect)
{
    if (isConnect) {
        onlines.onlineDevices.insert(target);
        onlines.latestOnlineDevice = target;
        onlines.latestOfflineDevice.clear();
    } else {
        onlines.onlineDevices.erase(target);
        onlines.latestOnlineDevice.clear();
        onlines.latestOfflineDevice = target;
    }
}

class DistributedDBCommunicatorTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBCommunicatorTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Create and init CommunicatorAggregator and AdapterStub
     */
    LOGI("[UT][Test][SetUpTestCase] Enter.");
    bool errCode = SetUpEnv(g_envDeviceA, DEVICE_NAME_A);
    ASSERT_EQ(errCode, true);
    errCode = SetUpEnv(g_envDeviceB, DEVICE_NAME_B);
    ASSERT_EQ(errCode, true);
    DoRegTransformFunction();
    CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(false);
}

void DistributedDBCommunicatorTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Finalize and release CommunicatorAggregator and AdapterStub
     */
    LOGI("[UT][Test][TearDownTestCase] Enter.");
    std::this_thread::sleep_for(std::chrono::seconds(7)); // Wait 7 s to make sure all thread quiet and memory released
    TearDownEnv(g_envDeviceA);
    TearDownEnv(g_envDeviceB);
    CommunicatorAggregator::EnableCommunicatorNotFoundFeedback(true);
}

void DistributedDBCommunicatorTest::SetUp()
{
    DistributedDBUnitTest::DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBCommunicatorTest::TearDown()
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
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorTest, CommunicatorManagement001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. alloc communicator A using label A
     * @tc.expected: step1. alloc return OK.
     */
    int errorNo = E_OK;
    ICommunicator *commA = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    EXPECT_EQ(errorNo, E_OK);
    EXPECT_NE(commA, nullptr);

    /**
     * @tc.steps: step2. alloc communicator B using label B
     * @tc.expected: step2. alloc return OK.
     */
    errorNo = E_OK;
    ICommunicator *commB = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_B, errorNo);
    EXPECT_EQ(errorNo, E_OK);
    EXPECT_NE(commA, nullptr);

    /**
     * @tc.steps: step3. alloc communicator C using label A
     * @tc.expected: step3. alloc return not OK.
     */
    errorNo = E_OK;
    ICommunicator *commC = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    EXPECT_NE(errorNo, E_OK);
    EXPECT_EQ(commC, nullptr);

    /**
     * @tc.steps: step4. release communicator A and communicator B
     */
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commA);
    commA = nullptr;
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commB);
    commB = nullptr;

    /**
     * @tc.steps: step5. alloc communicator D using label A
     * @tc.expected: step5. alloc return OK.
     */
    errorNo = E_OK;
    ICommunicator *commD = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    EXPECT_EQ(errorNo, E_OK);
    EXPECT_NE(commD, nullptr);

    /**
     * @tc.steps: step6. release communicator D
     */
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commD);
    commD = nullptr;
}

static void ConnectWaitDisconnect()
{
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
}

/**
 * @tc.name: Online And Offline 001
 * @tc.desc: Test functionality triggered by physical devices online and offline
 * @tc.type: FUNC
 * @tc.require: AR000BVRNS AR000CQE0H
 * @tc.author: wudongxing
 */
HWTEST_F(DistributedDBCommunicatorTest, OnlineAndOffline001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. device A alloc communicator AA using label A and register callback
     * @tc.expected: step1. no callback.
     */
    int errorNo = E_OK;
    ICommunicator *commAA = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commAA);
    OnOfflineDevice onlineForAA;
    commAA->RegOnConnectCallback([&onlineForAA](const std::string &target, bool isConnect) {
        HandleConnectChange(onlineForAA, target, isConnect);}, nullptr);
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));

    /**
     * @tc.steps: step2. connect device A with device B and then disconnect
     * @tc.expected: step2. no callback.
     */
    ConnectWaitDisconnect();
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));

    /**
     * @tc.steps: step3. device B alloc communicator BB using label B and register callback
     * @tc.expected: step3. no callback.
     */
    ICommunicator *commBB = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_B, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commBB);
    OnOfflineDevice onlineForBB;
    commBB->RegOnConnectCallback([&onlineForBB](const std::string &target, bool isConnect) {
        HandleConnectChange(onlineForBB, target, isConnect);}, nullptr);
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));

    /**
     * @tc.steps: step4. connect device A with device B and then disconnect
     * @tc.expected: step4. no callback.
     */
    ConnectWaitDisconnect();
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));

    /**
     * @tc.steps: step5. device B alloc communicator BA using label A and register callback
     * @tc.expected: step5. no callback.
     */
    ICommunicator *commBA = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commBA);
    OnOfflineDevice onlineForBA;
    commBA->RegOnConnectCallback([&onlineForBA](const std::string &target, bool isConnect) {
        HandleConnectChange(onlineForBA, target, isConnect);}, nullptr);
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBA.onlineDevices.size(), static_cast<size_t>(0));

    /**
     * @tc.steps: step6. connect device A with device B
     * @tc.expected: step6. communicator AA has callback of device B online;
     *                      communicator BA has callback of device A online;
     *                      communicator BB no callback
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBA.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForAA.latestOnlineDevice, DEVICE_NAME_B);
    EXPECT_EQ(onlineForBA.latestOnlineDevice, DEVICE_NAME_A);

    /**
     * @tc.steps: step7. disconnect device A with device B
     * @tc.expected: step7. communicator AA has callback of device B offline;
     *                      communicator BA has callback of device A offline;
     *                      communicator BB no callback
     */
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForAA.latestOfflineDevice, DEVICE_NAME_B);
    EXPECT_EQ(onlineForBA.latestOfflineDevice, DEVICE_NAME_A);

    // Clean up
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(commBB);
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(commBA);
}

#define REG_CONNECT_CALLBACK(communicator, online) \
{ \
    communicator->RegOnConnectCallback([&online](const std::string &target, bool isConnect) { \
        HandleConnectChange(online, target, isConnect); \
    }, nullptr); \
}

#define CONNECT_AND_WAIT(waitTime) \
{ \
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle); \
    std::this_thread::sleep_for(std::chrono::milliseconds(waitTime)); \
}

/**
 * @tc.name: Online And Offline 002
 * @tc.desc: Test functionality triggered by alloc and release communicator
 * @tc.type: FUNC
 * @tc.require: AR000BVRNT AR000CQE0I
 * @tc.author: wudongxing
 */
HWTEST_F(DistributedDBCommunicatorTest, OnlineAndOffline002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. connect device A with device B
     */
    CONNECT_AND_WAIT(200); // Sleep 200 ms

    /**
     * @tc.steps: step2. device A alloc communicator AA using label A and register callback
     * @tc.expected: step2. no callback.
     */
    int errorNo = E_OK;
    ICommunicator *commAA = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commAA);
    OnOfflineDevice onlineForAA;
    REG_CONNECT_CALLBACK(commAA, onlineForAA);
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));

    /**
     * @tc.steps: step3. device B alloc communicator BB using label B and register callback
     * @tc.expected: step3. no callback.
     */
    ICommunicator *commBB = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_B, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commBB);
    OnOfflineDevice onlineForBB;
    REG_CONNECT_CALLBACK(commBB, onlineForBB);
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));

    /**
     * @tc.steps: step4. device B alloc communicator BA using label A and register callback
     * @tc.expected: step4. communicator AA has callback of device B online;
     *                      communicator BA has callback of device A online;
     *                      communicator BB no callback.
     */
    ICommunicator *commBA = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commBA);
    OnOfflineDevice onlineForBA;
    REG_CONNECT_CALLBACK(commBA, onlineForBA);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBA.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForAA.latestOnlineDevice, DEVICE_NAME_B);
    EXPECT_EQ(onlineForBA.latestOnlineDevice, DEVICE_NAME_A);

    /**
     * @tc.steps: step5. device A alloc communicator AB using label B and register callback
     * @tc.expected: step5. communicator AB has callback of device B online;
     *                      communicator BB has callback of device A online;
     */
    ICommunicator *commAB = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_B, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commAB);
    OnOfflineDevice onlineForAB;
    REG_CONNECT_CALLBACK(commAB, onlineForAB);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(onlineForAB.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForAB.latestOnlineDevice, DEVICE_NAME_B);
    EXPECT_EQ(onlineForBB.latestOnlineDevice, DEVICE_NAME_A);

    /**
     * @tc.steps: step6. device A release communicator AA
     * @tc.expected: step6. communicator BA has callback of device A offline;
     *                      communicator AB and BB no callback;
     */
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(onlineForBA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForAB.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForBA.latestOfflineDevice, DEVICE_NAME_A);

    /**
     * @tc.steps: step7. device B release communicator BA
     * @tc.expected: step7. communicator AB and BB no callback;
     */
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(commBA);
    EXPECT_EQ(onlineForAB.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(1));

    /**
     * @tc.steps: step8. device B release communicator BB
     * @tc.expected: step8. communicator AB has callback of device B offline;
     */
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(commBB);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(onlineForAB.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForAB.latestOfflineDevice, DEVICE_NAME_B);

    // Clean up
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAB);
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
}

void TestRemoteRestart()
{
    /**
     * @tc.steps: step1. connect device D with device E
     */
    EnvHandle envDeviceD;
    EnvHandle envDeviceE;
    SetUpEnv(envDeviceD, "DEVICE_D");
    SetUpEnv(envDeviceE, "DEVICE_E");

    /**
     * @tc.steps: step2. device D alloc communicator DD using label A and register callback
     */
    int errorNo = E_OK;
    ICommunicator *commDD = envDeviceD.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commDD);
    OnOfflineDevice onlineForDD;
    REG_CONNECT_CALLBACK(commDD, onlineForDD);

    /**
     * @tc.steps: step3. device E alloc communicator EE using label A and register callback
     */
    ICommunicator *commEE = envDeviceE.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commEE);
    OnOfflineDevice onlineForEE;
    REG_CONNECT_CALLBACK(commEE, onlineForEE);
    /**
     * @tc.steps: step4. deviceD connect to deviceE
     * @tc.expected: step4. both communicator has callback;
     */
    AdapterStub::ConnectAdapterStub(envDeviceD.adapterHandle, envDeviceE.adapterHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(onlineForDD.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForEE.onlineDevices.size(), static_cast<size_t>(1));

    /**
     * @tc.steps: step5. device E restart
     */
    envDeviceE.commAggrHandle->ReleaseCommunicator(commEE);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    TearDownEnv(envDeviceE);
    SetUpEnv(envDeviceE, "DEVICE_E");

    commEE = envDeviceE.commAggrHandle->AllocCommunicator(LABEL_A, errorNo);
    ASSERT_NOT_NULL_AND_ACTIVATE(commEE);
    REG_CONNECT_CALLBACK(commEE, onlineForEE);
    onlineForEE.onlineDevices.clear();
    /**
     * @tc.steps: step6. deviceD connect to deviceE again
     * @tc.expected: step6. communicatorE has callback;
     */
    AdapterStub::ConnectAdapterStub(envDeviceD.adapterHandle, envDeviceE.adapterHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(onlineForEE.onlineDevices.size(), static_cast<size_t>(1));
    // Clean up and disconnect
    envDeviceD.commAggrHandle->ReleaseCommunicator(commDD);
    envDeviceE.commAggrHandle->ReleaseCommunicator(commEE);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    AdapterStub::DisconnectAdapterStub(envDeviceD.adapterHandle, envDeviceE.adapterHandle);
    TearDownEnv(envDeviceD);
    TearDownEnv(envDeviceE);
}
/**
 * @tc.name: Online And Offline 003
 * @tc.desc: Test functionality triggered by remote restart
 * @tc.type: FUNC
 * @tc.require: AR000CQE0I
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorTest, OnlineAndOffline003, TestSize.Level1)
{
    TestRemoteRestart();
}

/**
 * @tc.name: Online And Offline 004
 * @tc.desc: Test functionality triggered by remote restart with thread pool
 * @tc.type: FUNC
 * @tc.require: AR000CQE0I
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBCommunicatorTest, OnlineAndOffline004, TestSize.Level1)
{
    auto threadPool = std::make_shared<ThreadPoolTestStub>();
    RuntimeContext::GetInstance()->SetThreadPool(threadPool);
    TestRemoteRestart();
    RuntimeContext::GetInstance()->SetThreadPool(nullptr);
}

/**
 * @tc.name: Report Device Connect Change 001
 * @tc.desc: Test CommunicatorAggregator support report device connect change event
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorTest, ReportDeviceConnectChange001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. device A and device B register connect callback to CommunicatorAggregator
     */
    OnOfflineDevice onlineForA;
    int errCode = g_envDeviceA.commAggrHandle->RegOnConnectCallback(
        [&onlineForA](const std::string &target, bool isConnect) {
            HandleConnectChange(onlineForA, target, isConnect);
        }, nullptr);
    EXPECT_EQ(errCode, E_OK);
    OnOfflineDevice onlineForB;
    errCode = g_envDeviceB.commAggrHandle->RegOnConnectCallback(
        [&onlineForB](const std::string &target, bool isConnect) {
            HandleConnectChange(onlineForB, target, isConnect);
        }, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. connect device A with device B
     * @tc.expected: step2. device A callback B online; device B callback A online;
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    EXPECT_EQ(onlineForA.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForB.onlineDevices.size(), static_cast<size_t>(1));
    EXPECT_EQ(onlineForA.latestOnlineDevice, DEVICE_NAME_B);
    EXPECT_EQ(onlineForB.latestOnlineDevice, DEVICE_NAME_A);

    /**
     * @tc.steps: step3. connect device A with device B
     * @tc.expected: step3. device A callback B offline; device B callback A offline;
     */
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    EXPECT_EQ(onlineForA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForB.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForA.latestOfflineDevice, DEVICE_NAME_B);
    EXPECT_EQ(onlineForB.latestOfflineDevice, DEVICE_NAME_A);

    // Clean up
    g_envDeviceA.commAggrHandle->RegOnConnectCallback(nullptr, nullptr);
    g_envDeviceB.commAggrHandle->RegOnConnectCallback(nullptr, nullptr);
}

namespace {
LabelType ToLabelType(uint64_t commLabel)
{
    uint64_t netOrderLabel = HostToNet(commLabel);
    uint8_t *eachByte = reinterpret_cast<uint8_t *>(&netOrderLabel);
    std::vector<uint8_t> realLabel(COMM_LABEL_LENGTH, 0);
    for (int i = 0; i < static_cast<int>(sizeof(uint64_t)); i++) {
        realLabel[i] = eachByte[i];
    }
    return realLabel;
}
}

/**
 * @tc.name: Report Communicator Not Found 001
 * @tc.desc: Test CommunicatorAggregator support report communicator not found event
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorTest, ReportCommunicatorNotFound001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. device B register communicator not found callback to CommunicatorAggregator
     */
    std::vector<LabelType> lackLabels;
    int errCode = g_envDeviceB.commAggrHandle->RegCommunicatorLackCallback(
        [&lackLabels](const LabelType &commLabel, const std::string &userId)->int {
            lackLabels.push_back(commLabel);
            return -E_NOT_FOUND;
        }, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. connect device A with device B
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    /**
     * @tc.steps: step3. device A alloc communicator AA using label A and send message to B
     * @tc.expected: step3. device B callback that label A not found.
     */
    ICommunicator *commAA = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_A, errCode);
    ASSERT_NOT_NULL_AND_ACTIVATE(commAA);
    Message *msgForAA = BuildRegedTinyMessage();
    ASSERT_NE(msgForAA, nullptr);
    SendConfig conf = {true, false, 0};
    errCode = commAA->SendMessage(DEVICE_NAME_B, msgForAA, conf);
    EXPECT_EQ(errCode, E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(lackLabels.size(), static_cast<size_t>(1));
    EXPECT_EQ(lackLabels[0], ToLabelType(LABEL_A));

    /**
     * @tc.steps: step4. device B alloc communicator BA using label A and register message callback
     * @tc.expected: step4. communicator BA will not receive message.
     */
    ICommunicator *commBA = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_A, errCode);
    ASSERT_NE(commBA, nullptr);
    Message *recvMsgForBA = nullptr;
    commBA->RegOnMessageCallback([&recvMsgForBA](const std::string &srcTarget, Message *inMsg) {
        recvMsgForBA = inMsg;
    }, nullptr);
    commBA->Activate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    EXPECT_EQ(recvMsgForBA, nullptr);

    // Clean up
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(commBA);
    g_envDeviceB.commAggrHandle->RegCommunicatorLackCallback(nullptr, nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
}

#define DO_SEND_MESSAGE(src, dst, label, session) \
{ \
    Message *msgFor##src##label = BuildRegedTinyMessage(); \
    ASSERT_NE(msgFor##src##label, nullptr); \
    msgFor##src##label->SetSessionId(session); \
    SendConfig conf = {true, false, 0}; \
    errCode = comm##src##label->SendMessage(DEVICE_NAME_##dst, msgFor##src##label, conf); \
    EXPECT_EQ(errCode, E_OK); \
}

#define DO_SEND_GIANT_MESSAGE(src, dst, label, size) \
{ \
    Message *msgFor##src##label = BuildRegedGiantMessage(size); \
    ASSERT_NE(msgFor##src##label, nullptr); \
    SendConfig conf = {false, false, 0}; \
    errCode = comm##src##label->SendMessage(DEVICE_NAME_##dst, msgFor##src##label, conf); \
    EXPECT_EQ(errCode, E_OK); \
}

#define ALLOC_AND_SEND_MESSAGE(src, dst, label, session) \
    ICommunicator *comm##src##label = g_envDevice##src.commAggrHandle->AllocCommunicator(LABEL_##label, errCode); \
    ASSERT_NOT_NULL_AND_ACTIVATE(comm##src##label); \
    DO_SEND_MESSAGE(src, dst, label, session)

#define REG_MESSAGE_CALLBACK(src, label) \
    string srcTargetFor##src##label; \
    Message *recvMsgFor##src##label = nullptr; \
    comm##src##label->RegOnMessageCallback( \
        [&srcTargetFor##src##label, &recvMsgFor##src##label](const std::string &srcTarget, Message *inMsg) { \
        srcTargetFor##src##label = srcTarget; \
        recvMsgFor##src##label = inMsg; \
    }, nullptr);

/**
 * @tc.name: ReDeliver Message 001
 * @tc.desc: Test CommunicatorAggregator support redeliver message
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorTest, ReDeliverMessage001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. device B register communicator not found callback to CommunicatorAggregator
     */
    std::vector<LabelType> lackLabels;
    int errCode = g_envDeviceB.commAggrHandle->RegCommunicatorLackCallback(
        [&lackLabels](const LabelType &commLabel, const std::string &userId)->int {
            lackLabels.push_back(commLabel);
            return E_OK;
        }, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. connect device A with device B
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    /**
     * @tc.steps: step3. device A alloc communicator AA using label A and send message to B
     * @tc.expected: step3. device B callback that label A not found.
     */
    ALLOC_AND_SEND_MESSAGE(A, B, A, 100); // session id 100
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(lackLabels.size(), static_cast<size_t>(1));
    EXPECT_EQ(lackLabels[0], ToLabelType(LABEL_A));

    /**
     * @tc.steps: step4. device A alloc communicator AB using label B and send message to B
     * @tc.expected: step4. device B callback that label B not found.
     */
    ALLOC_AND_SEND_MESSAGE(A, B, B, 200); // session id 200
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(lackLabels.size(), static_cast<size_t>(2));
    EXPECT_EQ(lackLabels[1], ToLabelType(LABEL_B)); // 1 for second element

    /**
     * @tc.steps: step5. device B alloc communicator BA using label A and register message callback
     * @tc.expected: step5. communicator BA will receive message.
     */
    ICommunicator *commBA = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_A, errCode);
    ASSERT_NE(commBA, nullptr);
    REG_MESSAGE_CALLBACK(B, A);
    commBA->Activate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    EXPECT_EQ(srcTargetForBA, DEVICE_NAME_A);
    ASSERT_NE(recvMsgForBA, nullptr);
    EXPECT_EQ(recvMsgForBA->GetSessionId(), 100U); // session id 100
    delete recvMsgForBA;
    recvMsgForBA = nullptr;

    /**
     * @tc.steps: step6. device B alloc communicator BB using label B and register message callback
     * @tc.expected: step6. communicator BB will receive message.
     */
    ICommunicator *commBB = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_B, errCode);
    ASSERT_NE(commBB, nullptr);
    REG_MESSAGE_CALLBACK(B, B);
    commBB->Activate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    EXPECT_EQ(srcTargetForBB, DEVICE_NAME_A);
    ASSERT_NE(recvMsgForBB, nullptr);
    EXPECT_EQ(recvMsgForBB->GetSessionId(), 200U); // session id 200
    delete recvMsgForBB;
    recvMsgForBB = nullptr;

    // Clean up
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAB);
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(commBA);
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(commBB);
    g_envDeviceB.commAggrHandle->RegCommunicatorLackCallback(nullptr, nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
}

/**
 * @tc.name: ReDeliver Message 002
 * @tc.desc: Test CommunicatorAggregator support redeliver message by order
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorTest, ReDeliverMessage002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. device C create CommunicatorAggregator and initialize
     */
    bool step1 = SetUpEnv(g_envDeviceC, DEVICE_NAME_C);
    ASSERT_EQ(step1, true);

    /**
     * @tc.steps: step2. device B register communicator not found callback to CommunicatorAggregator
     */
    int errCode = g_envDeviceB.commAggrHandle->RegCommunicatorLackCallback([](const LabelType &commLabel,
        const std::string &userId)->int {
        return E_OK;
    }, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step3. connect device A with device B, then device B with device C
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::ConnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    /**
     * @tc.steps: step4. device A alloc communicator AA using label A and send message to B
     */
    ALLOC_AND_SEND_MESSAGE(A, B, A, 100); // session id 100
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    /**
     * @tc.steps: step5. device C alloc communicator CA using label A and send message to B
     */
    ALLOC_AND_SEND_MESSAGE(C, B, A, 200); // session id 200
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    DO_SEND_MESSAGE(A, B, A, 300); // session id 300
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    DO_SEND_MESSAGE(C, B, A, 400); // session id 400
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    /**
     * @tc.steps: step6. device B alloc communicator BA using label A and register message callback
     * @tc.expected: step6. communicator BA will receive message in order of sessionid 100, 200, 300, 400.
     */
    ICommunicator *commBA = g_envDeviceB.commAggrHandle->AllocCommunicator(LABEL_A, errCode);
    ASSERT_NE(commBA, nullptr);
    std::vector<std::pair<std::string, Message *>> msgCallbackForBA;
    commBA->RegOnMessageCallback([&msgCallbackForBA](const std::string &srcTarget, Message *inMsg) {
        msgCallbackForBA.push_back({srcTarget, inMsg});
    }, nullptr);
    commBA->Activate();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms
    ASSERT_EQ(msgCallbackForBA.size(), static_cast<size_t>(4)); // total 4 callback
    EXPECT_EQ(msgCallbackForBA[0].first, DEVICE_NAME_A); // the 0 order element
    EXPECT_EQ(msgCallbackForBA[1].first, DEVICE_NAME_C); // the 1 order element
    EXPECT_EQ(msgCallbackForBA[2].first, DEVICE_NAME_A); // the 2 order element
    EXPECT_EQ(msgCallbackForBA[3].first, DEVICE_NAME_C); // the 3 order element
    for (uint32_t i = 0; i < msgCallbackForBA.size(); i++) {
        EXPECT_EQ(msgCallbackForBA[i].second->GetSessionId(), static_cast<uint32_t>((i + 1) * 100)); // 1 sessionid 100
        delete msgCallbackForBA[i].second;
        msgCallbackForBA[i].second = nullptr;
    }

    // Clean up
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    g_envDeviceC.commAggrHandle->ReleaseCommunicator(commCA);
    g_envDeviceB.commAggrHandle->ReleaseCommunicator(commBA);
    g_envDeviceB.commAggrHandle->RegCommunicatorLackCallback(nullptr, nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    AdapterStub::DisconnectAdapterStub(g_envDeviceB.adapterHandle, g_envDeviceC.adapterHandle);
    TearDownEnv(g_envDeviceC);
}

/**
 * @tc.name: ReDeliver Message 003
 * @tc.desc: For observe memory in unusual scenario
 * @tc.type: FUNC
 * @tc.require: AR000DR9KV
 * @tc.author: xiaozhenjian
 */
HWTEST_F(DistributedDBCommunicatorTest, ReDeliverMessage003, TestSize.Level2)
{
    /**
     * @tc.steps: step1. device B register communicator not found callback to CommunicatorAggregator
     */
    int errCode = g_envDeviceB.commAggrHandle->RegCommunicatorLackCallback([](const LabelType &commLabel,
        const std::string &userId)->int {
        return E_OK;
    }, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. connect device A with device B
     */
    AdapterStub::ConnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    /**
     * @tc.steps: step3. device A alloc communicator AA,AB,AC using label A,B,C
     */
    ICommunicator *commAA = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_A, errCode);
    ASSERT_NOT_NULL_AND_ACTIVATE(commAA);
    ICommunicator *commAB = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_B, errCode);
    ASSERT_NOT_NULL_AND_ACTIVATE(commAB);
    ICommunicator *commAC = g_envDeviceA.commAggrHandle->AllocCommunicator(LABEL_C, errCode);
    ASSERT_NOT_NULL_AND_ACTIVATE(commAC);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep 100 ms

    /**
     * @tc.steps: step4. device A Continuously send tiny message to B using communicator AA,AB,AC
     */
    for (int turn = 0; turn < 11; turn++) { // Total 11 turns
        DO_SEND_MESSAGE(A, B, A, 0);
        DO_SEND_MESSAGE(A, B, B, 0);
        DO_SEND_MESSAGE(A, B, C, 0);
    }

    /**
     * @tc.steps: step5. device A Continuously send giant message to B using communicator AA,AB,AC
     */
    for (int turn = 0; turn < 5; turn++) { // Total 5 turns
        DO_SEND_GIANT_MESSAGE(A, B, A, (3 * 1024 * 1024)); // 3 MBytes, 1024 is scale
        DO_SEND_GIANT_MESSAGE(A, B, B, (6 * 1024 * 1024)); // 6 MBytes, 1024 is scale
        DO_SEND_GIANT_MESSAGE(A, B, C, (7 * 1024 * 1024)); // 7 MBytes, 1024 is scale
    }
    DO_SEND_GIANT_MESSAGE(A, B, A, (30 * 1024 * 1024)); // 30 MBytes, 1024 is scale

    /**
     * @tc.steps: step6. wait a long time then send last frame
     */
    for (int sec = 0; sec < 15; sec++) { // Total 15 s
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep 1 s
        LOGI("[UT][Test][ReDeliverMessage003] Sleep and wait=%d.", sec);
    }
    DO_SEND_MESSAGE(A, B, A, 0);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep 1 s

    // Clean up
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAB);
    g_envDeviceA.commAggrHandle->ReleaseCommunicator(commAC);
    g_envDeviceB.commAggrHandle->RegCommunicatorLackCallback(nullptr, nullptr);
    AdapterStub::DisconnectAdapterStub(g_envDeviceA.adapterHandle, g_envDeviceB.adapterHandle);
}

namespace {
    void SetUpEnv(const std::string &localDev, const std::string &supportDev,
        const std::shared_ptr<DBStatusAdapter> &adapter, bool isSupport, EnvHandle &envDevice)
    {
        std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
        adapter->SetDBInfoHandle(handle);
        adapter->SetRemoteOptimizeCommunication(supportDev, isSupport);
        SetUpEnv(envDevice, localDev, adapter);
    }

    void InitCommunicator(DBInfo &dbInfo, EnvHandle &envDevice, OnOfflineDevice &onlineCallback, ICommunicator *&comm)
    {
        dbInfo.userId = USER_ID;
        dbInfo.appId = APP_ID;
        dbInfo.storeId = STORE_ID_1;
        dbInfo.isNeedSync = true;
        dbInfo.syncDualTupleMode = false;
        std::string label = DBCommon::GenerateHashLabel(dbInfo);
        std::vector<uint8_t> commLabel(label.begin(), label.end());
        int errorNo = E_OK;
        comm = envDevice.commAggrHandle->AllocCommunicator(commLabel, errorNo);
        ASSERT_NOT_NULL_AND_ACTIVATE(comm);
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
HWTEST_F(DistributedDBCommunicatorTest, CommunicationOptimization001, TestSize.Level3)
{
    auto *pAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(pAggregator, nullptr);
    const std::string deviceA = "DEVICES_A";
    const std::string deviceB = "DEVICES_B";
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(pAggregator);
    /**
     * @tc.steps: step1. set up env
     */
    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    SetUpEnv(deviceA, deviceB, adapterA, true, envDeviceA);

    EnvHandle envDeviceB;
    std::shared_ptr<DBStatusAdapter> adapterB = std::make_shared<DBStatusAdapter>();
    SetUpEnv(deviceB, deviceA, adapterB, true, envDeviceB);

    /**
     * @tc.steps: step2. device alloc communicator using label and register callback
     */
    DBInfo dbInfo;
    ICommunicator *commAA = nullptr;
    OnOfflineDevice onlineForAA;
    InitCommunicator(dbInfo, envDeviceA, onlineForAA, commAA);

    ICommunicator *commBB = nullptr;
    OnOfflineDevice onlineForBB;
    InitCommunicator(dbInfo, envDeviceB, onlineForBB, commBB);

    /**
     * @tc.steps: step3. connect device A with device B
     */
    AdapterStub::ConnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);

    /**
     * @tc.steps: step4. wait for label exchange
     * @tc.expected: step4. both communicator has no callback;
     */
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));

    /**
     * @tc.steps: step5. both notify
     * @tc.expected: step5. both has callback;
     */
    RuntimeConfig::NotifyDBInfos({ deviceB }, { dbInfo });
    adapterA->NotifyDBInfos({ deviceB }, { dbInfo });
    adapterB->NotifyDBInfos({ deviceA }, { dbInfo });
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(1));

    dbInfo.isNeedSync = false;
    adapterA->NotifyDBInfos({ deviceB }, { dbInfo });
    adapterB->NotifyDBInfos({ deviceA }, { dbInfo });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));

    // Clean up and disconnect
    envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    envDeviceB.commAggrHandle->ReleaseCommunicator(commBB);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);

    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceB);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

/**
  * @tc.name: CommunicationOptimization002
  * @tc.desc: Test notify with isSupport true and can offline by device change.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, CommunicationOptimization002, TestSize.Level3)
{
    auto *pAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(pAggregator, nullptr);
    const std::string deviceA = "DEVICES_A";
    const std::string deviceB = "DEVICES_B";
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(pAggregator);
    /**
     * @tc.steps: step1. set up env
     */
    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    SetUpEnv(deviceA, deviceB, adapterA, true, envDeviceA);

    EnvHandle envDeviceB;
    std::shared_ptr<DBStatusAdapter> adapterB = std::make_shared<DBStatusAdapter>();
    SetUpEnv(deviceB, deviceA, adapterB, true, envDeviceB);

    /**
     * @tc.steps: step2. device alloc communicator using label and register callback
     */
    DBInfo dbInfo;
    ICommunicator *commAA = nullptr;
    OnOfflineDevice onlineForAA;
    InitCommunicator(dbInfo, envDeviceA, onlineForAA, commAA);
    /**
     * @tc.steps: step3. connect device A with device B
     */
    AdapterStub::ConnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    /**
     * @tc.steps: step5. A notify remote
     * @tc.expected: step5. A has callback;
     */
    adapterA->NotifyDBInfos({ deviceB }, { dbInfo });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(1));

    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));

    // Clean up and disconnect
    envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceB);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

/**
  * @tc.name: CommunicationOptimization003
  * @tc.desc: Test notify with isSupport false.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, CommunicationOptimization003, TestSize.Level3)
{
    auto *pAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(pAggregator, nullptr);
    const std::string deviceA = "DEVICES_A";
    const std::string deviceB = "DEVICES_B";
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(pAggregator);
    /**
     * @tc.steps: step1. set up env
     */
    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    SetUpEnv(deviceA, deviceB, adapterA, false, envDeviceA);
    EnvHandle envDeviceB;
    std::shared_ptr<DBStatusAdapter> adapterB = std::make_shared<DBStatusAdapter>();
    SetUpEnv(deviceB, deviceA, adapterB, false, envDeviceB);
    /**
     * @tc.steps: step2. device alloc communicator using label and register callback
     */
    DBInfo dbInfo;
    ICommunicator *commAA = nullptr;
    OnOfflineDevice onlineForAA;
    InitCommunicator(dbInfo, envDeviceA, onlineForAA, commAA);
    ICommunicator *commBB = nullptr;
    OnOfflineDevice onlineForBB;
    InitCommunicator(dbInfo, envDeviceB, onlineForBB, commBB);
    /**
     * @tc.steps: step3. connect device A with device B
     */
    AdapterStub::ConnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);
    /**
     * @tc.steps: step4. wait for label exchange
     * @tc.expected: step4. both communicator has no callback;
     */
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));
    /**
     * @tc.steps: step5. A notify remote
     * @tc.expected: step5. B has no callback;
     */
    adapterA->NotifyDBInfos({ deviceB }, { dbInfo });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(0));
    /**
     * @tc.steps: step6. A notify local
     * @tc.expected: step6. B has no callback;
     */
    dbInfo.isNeedSync = false;
    onlineForAA.onlineDevices.clear();
    adapterA->NotifyDBInfos({ deviceA }, { dbInfo });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(onlineForAA.onlineDevices.size(), static_cast<size_t>(0));
    // Clean up and disconnect
    envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    envDeviceB.commAggrHandle->ReleaseCommunicator(commBB);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);
    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceB);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

/**
  * @tc.name: CommunicationOptimization004
  * @tc.desc: Test notify with isSupport false and it be will changed by communication.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, CommunicationOptimization004, TestSize.Level3)
{
    const std::string deviceA = "DEVICES_A";
    const std::string deviceB = "DEVICES_B";
    /**
     * @tc.steps: step1. set up env
     */
    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    SetUpEnv(deviceA, deviceB, adapterA, false, envDeviceA);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(envDeviceA.commAggrHandle);
    EnvHandle envDeviceB;
    std::shared_ptr<DBStatusAdapter> adapterB = std::make_shared<DBStatusAdapter>();
    SetUpEnv(deviceB, deviceA, adapterB, false, envDeviceB);
    /**
     * @tc.steps: step2. device alloc communicator using label and register callback
     */
    DBInfo dbInfo;
    ICommunicator *commAA = nullptr;
    OnOfflineDevice onlineForAA;
    InitCommunicator(dbInfo, envDeviceA, onlineForAA, commAA);
    ICommunicator *commBB = nullptr;
    OnOfflineDevice onlineForBB;
    InitCommunicator(dbInfo, envDeviceB, onlineForBB, commBB);
    /**
     * @tc.steps: step3. connect device A with device B
     */
    EXPECT_EQ(adapterA->IsSupport(deviceB), false);
    AdapterStub::ConnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(adapterA->IsSupport(deviceB), true);
    // Clean up and disconnect
    envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    envDeviceB.commAggrHandle->ReleaseCommunicator(commBB);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    envDeviceA.commAggrHandle = nullptr;
    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceB);
}

/**
  * @tc.name: CommunicationOptimization005
  * @tc.desc: Test notify with isSupport false and send label exchange.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, CommunicationOptimization005, TestSize.Level3)
{
    const std::string deviceA = "DEVICES_A";
    const std::string deviceB = "DEVICES_B";
    /**
     * @tc.steps: step1. set up env
     */
    EnvHandle envDeviceA;
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    handle->SetLocalIsSupport(false);
    adapterA->SetDBInfoHandle(handle);
    SetUpEnv(envDeviceA, deviceA, adapterA);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(envDeviceA.commAggrHandle);
    EnvHandle envDeviceB;
    SetUpEnv(envDeviceB, deviceB, nullptr);
    /**
     * @tc.steps: step2. connect device A with device B
     */
    EXPECT_EQ(adapterA->IsSupport(deviceB), false);
    AdapterStub::ConnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step3. device alloc communicator using label and register callback
     */
    DBInfo dbInfo;
    ICommunicator *commAA = nullptr;
    OnOfflineDevice onlineForAA;
    InitCommunicator(dbInfo, envDeviceA, onlineForAA, commAA);
    ICommunicator *commBB = nullptr;
    OnOfflineDevice onlineForBB;
    InitCommunicator(dbInfo, envDeviceB, onlineForBB, commBB);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(onlineForBB.onlineDevices.size(), static_cast<size_t>(1));

    // Clean up and disconnect
    envDeviceA.commAggrHandle->ReleaseCommunicator(commAA);
    envDeviceB.commAggrHandle->ReleaseCommunicator(commBB);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    AdapterStub::DisconnectAdapterStub(envDeviceA.adapterHandle, envDeviceB.adapterHandle);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    envDeviceA.commAggrHandle = nullptr;
    TearDownEnv(envDeviceA);
    TearDownEnv(envDeviceB);
}

/**
  * @tc.name: DbStatusAdapter001
  * @tc.desc: Test notify with isSupport false.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, DbStatusAdapter001, TestSize.Level1)
{
    auto *pAggregator = new VirtualCommunicatorAggregator();
    ASSERT_NE(pAggregator, nullptr);
    const std::string deviceA = "DEVICES_A";
    const std::string deviceB = "DEVICES_B";
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(pAggregator);

    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterA->SetDBInfoHandle(handle);
    adapterA->SetRemoteOptimizeCommunication(deviceB, true);
    std::string actualRemoteDevInfo;
    size_t remoteInfoCount = 0u;
    size_t localCount = 0u;
    DBInfo dbInfo;
    adapterA->NotifyDBInfos({ deviceA }, { dbInfo });

    dbInfo = {
        USER_ID,
        APP_ID,
        STORE_ID_1,
        true,
        false
    };
    adapterA->NotifyDBInfos({ deviceA }, { dbInfo });
    dbInfo.isNeedSync = false;
    adapterA->NotifyDBInfos({ deviceA }, { dbInfo });
    adapterA->NotifyDBInfos({ deviceB }, { dbInfo });

    size_t remoteChangeCount = 0u;
    adapterA->SetDBStatusChangeCallback(
        [&actualRemoteDevInfo, &remoteInfoCount](const std::string &devInfo, const std::vector<DBInfo> &dbInfos) {
            actualRemoteDevInfo = devInfo;
            remoteInfoCount = dbInfos.size();
        },
        [&localCount]() {
            localCount++;
        },
        [&remoteChangeCount, deviceB](const std::string &dev) {
            remoteChangeCount++;
            EXPECT_EQ(dev, deviceB);
        });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(actualRemoteDevInfo, deviceB);
    EXPECT_EQ(remoteInfoCount, 1u);
    adapterA->SetRemoteOptimizeCommunication(deviceB, false);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(remoteChangeCount, 1u);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

/**
  * @tc.name: DbStatusAdapter002
  * @tc.desc: Test adapter clear cache.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, DbStatusAdapter002, TestSize.Level1) {
    const std::string deviceB = "DEVICES_B";
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterA->SetDBInfoHandle(handle);
    adapterA->SetRemoteOptimizeCommunication(deviceB, true);
    EXPECT_TRUE(adapterA->IsSupport(deviceB));
    adapterA->SetDBInfoHandle(handle);
    adapterA->SetRemoteOptimizeCommunication(deviceB, false);
    EXPECT_FALSE(adapterA->IsSupport(deviceB));
}

/**
  * @tc.name: DbStatusAdapter003
  * @tc.desc: Test adapter get local dbInfo.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, DbStatusAdapter003, TestSize.Level1) {
    const std::string deviceB = "DEVICES_B";
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterA->SetDBInfoHandle(handle);
    handle->SetLocalIsSupport(true);
    std::vector<DBInfo> dbInfos;
    EXPECT_EQ(adapterA->GetLocalDBInfos(dbInfos), E_OK);
    handle->SetLocalIsSupport(false);
    EXPECT_EQ(adapterA->GetLocalDBInfos(dbInfos), E_OK);
    adapterA->SetDBInfoHandle(handle);
    EXPECT_EQ(adapterA->GetLocalDBInfos(dbInfos), -E_NOT_SUPPORT);
}

/**
  * @tc.name: DbStatusAdapter004
  * @tc.desc: Test adapter clear cache will get callback.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, DbStatusAdapter004, TestSize.Level1)
{
    const std::string deviceB = "DEVICES_B";
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterA->SetDBInfoHandle(handle);
    handle->SetLocalIsSupport(true);
    std::vector<DBInfo> dbInfos;
    DBInfo dbInfo = {
        USER_ID,
        APP_ID,
        STORE_ID_1,
        true,
        true
    };
    dbInfos.push_back(dbInfo);
    dbInfo.storeId = STORE_ID_2;
    dbInfos.push_back(dbInfo);
    dbInfo.storeId = STORE_ID_3;
    dbInfo.isNeedSync = false;
    dbInfos.push_back(dbInfo);
    adapterA->NotifyDBInfos({"dev"}, dbInfos);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    size_t notifyCount = 0u;
    adapterA->SetDBStatusChangeCallback([&notifyCount](const std::string &devInfo,
        const std::vector<DBInfo> &dbInfos) {
        LOGD("on callback");
        for (const auto &dbInfo: dbInfos) {
            EXPECT_FALSE(dbInfo.isNeedSync);
        }
        notifyCount = dbInfos.size();
    }, nullptr, nullptr);
    adapterA->SetDBInfoHandle(nullptr);
    EXPECT_EQ(notifyCount, 2u); // 2 dbInfo is need sync now it should be offline
}

/**
  * @tc.name: DbStatusAdapter005
  * @tc.desc: Test adapter is need auto sync.
  * @tc.type: FUNC
  * @tc.require: AR000HGD0B
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBCommunicatorTest, DbStatusAdapter005, TestSize.Level1)
{
    const std::string deviceB = "DEVICES_B";
    std::shared_ptr<DBStatusAdapter> adapterA = std::make_shared<DBStatusAdapter>();
    std::shared_ptr<DBInfoHandleTest> handle = std::make_shared<DBInfoHandleTest>();
    adapterA->SetDBInfoHandle(handle);
    handle->SetLocalIsSupport(true);
    EXPECT_EQ(adapterA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceB), true);
    handle->SetNeedAutoSync(false);
    EXPECT_EQ(adapterA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceB), false);
    handle->SetLocalIsSupport(false);
    EXPECT_EQ(adapterA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceB), false);
    adapterA->SetDBInfoHandle(handle);
    EXPECT_EQ(adapterA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceB), true);
    adapterA->SetDBInfoHandle(nullptr);
    EXPECT_EQ(adapterA->IsNeedAutoSync(USER_ID, APP_ID, STORE_ID_1, deviceB), true);
}
}