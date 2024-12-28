
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

#include "distributeddb_data_generate_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "messages.h"
#include "meta_data.h"
#include "ref_object.h"
#include "single_ver_sync_engine.h"
#include "version.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string ANTI_DOS_STORE_ID = "anti_dos_sync_test";
#ifndef RELATIONAL_STORE
    const int NUM = 108;
#else
    const int NUM = 120;
#endif
    const int WAIT_LONG_TIME = 1000;
    const int WAIT_SHORT_TIME = 200;
    const int TEST_ONE = 2;
    const int TEST_TWO = 10;
    const int TEST_THREE_THREAD = 100;
    const int TEST_THREE_OUTDATA = 2048;
    const int TEST_THREE_DATATIEM = 1000;
    const int LIMIT_QUEUE_CACHE_SIZE = 2048 * 1024;
    const int DEFAULT_CACHE_SIZE = 160 * 2048 * 1024; // Initial the default cache size of queue as 160MB
    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreConfig g_config;
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate* g_kvDelegatePtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    std::shared_ptr<Metadata> g_meta = nullptr;
    SingleVerSyncEngine *g_synerEngine = nullptr;
    VirtualCommunicator *g_communicator = nullptr;
    KvVerSyncDBInterface *g_interface = nullptr;

    auto g_kvDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvDelegatePtr));
}

class KvdbAntiDosSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvdbAntiDosSyncTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and VirtualCommunicatorAggregator.
     */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    EXCEPT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void KvdbAntiDosSyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release VirtualCommunicatorAggregator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void KvdbAntiDosSyncTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create VirtualCommunicator, VirtualSingleVerSyncDBInterface, SyncEngine,
     * and set maximum cache of queue.
     */
    const std::string remoteDeviceId = "real_device";
    KvStoreNbDelegate::Option option = {true};
    g_mgr.GetKvStore(ANTI_DOS_STORE_ID, option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvDelegateStatus == OK);
    EXCEPT_TRUE(g_kvDelegatePtr != nullptr);
    g_interface = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    EXCEPT_TRUE(g_interface != nullptr);
    std::vector<uint8_t> identifier(COMM_LABEL_LENGTH, 1u);
    g_interface->SetIdentifier(identifier);
    g_meta = std::make_shared<Metadata>();
    int errCodeMetaData = g_meta->Initialize(g_interface);
    EXCEPT_TRUE(errCodeMetaData == E_OK);
    g_synerEngine = new (std::nothrow) SingleVerSyncEngine();
    EXCEPT_TRUE(g_synerEngine != nullptr);
    ISyncEngine::InitCallbackParam param = { nullptr, nullptr, nullptr };
    int errCodeSyncEngine = g_synerEngine->Initialize(g_interface, g_meta, param);
    EXCEPT_TRUE(errCodeSyncEngine == E_OK);
    g_communicator = static_cast<VirtualCommunicator *>(g_communicatorAggregator->GetCommunicator(remoteDeviceId));
    EXCEPT_TRUE(g_communicator != nullptr);
    g_synerEngine->SetMaxQueueCacheSize(LIMIT_QUEUE_CACHE_SIZE);
}

void KvdbAntiDosSyncTest::TearDown(void)
{
    /**
     * @tc.teardown: Release VirtualCommunicator, VirtualSingleVerSyncDBInterface and SyncEngine.
     */
    if (g_communicator != nullptr) {
        g_communicator->KillObj();
        g_communicator = nullptr;
    }
    if (g_synerEngine != nullptr) {
        g_synerEngine->SetMaxQueueCacheSize(DEFAULT_CACHE_SIZE);
        auto syncEngine = g_synerEngine;
        g_synerEngine->OnKill([syncEngine]() { syncEngine->Close(); });
        RefObject::KillAndDecObjRef(g_synerEngine);
        g_synerEngine = nullptr;
    }
    g_meta = nullptr;
    if (g_interface != nullptr) {
        delete g_interface;
        g_interface = nullptr;
    }
    if (g_kvDelegatePtr != nullptr) {
        g_mgr.CloseKvStore(g_kvDelegatePtr);
        g_kvDelegatePtr = nullptr;
    }
    g_mgr.DeleteKvStore(ANTI_DOS_STORE_ID);
}

/**
 * @tc.name: Anti Dos attack Sync 001
 * @tc.desc: Whether function run normally when the amount of messages is lower than the maximum of threads
 *   and the whole length of messages is lower than the maximum size of queue.
 * @tc.type: FUNC
 * @tc.require: AR000D08KU
 * @tc.author: yiguang
 */
HWTEST_F(KvdbAntiDosSyncTest, AntiDosAttackSync001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. control MessageReceiveCallback to send messages, whose number is lower than
     *  the maximum of threads and length is lower than the maximum size of queue.
     */
    const std::string srcTarget = "001";
    std::vector<SendDataItem> outData;

    for (unsigned int index = 0; index < g_synerEngine->GetMaxExecNum() - TEST_ONE; index++) {
        DataRequestPacket *requestPacket = new (std::nothrow) DataRequestPacket;
        EXCEPT_TRUE(requestPacket != nullptr);
        Message *messages = new (std::nothrow) Message(DATA_SYNC_MESSAGE);
        EXCEPT_TRUE(messages != nullptr);

        GenericSingleVerKvEntry *kvEntry = new (std::nothrow) GenericSingleVerKvEntry();
        EXCEPT_TRUE(kvEntry != nullptr);
        outData.push_back(kvEntry);
        requestPacket->SetData(outData);
        requestPacket->SetSendCode(E_OK);
        requestPacket->SetVersion(SOFTWARE_VERSION_CURRENT);
        uint32_t sessionId = index;
        uint32_t sequenceId = index;
        messages->SetMessageType(TYPE_REQUEST);
        messages->SetTarget(srcTarget);
        int errCode = messages->SetExternalObject(requestPacket);
        EXCEPT_TRUE(errCode == E_OK);
        messages->SetSessionId(sessionId);
        messages->SetSequenceId(sequenceId);
        g_communicator->CallbackOnMessage(srcTarget, messages);
    /**
     * @tc.expected: step1. no messages was found to be enqueued and discarded.
     */
        EXPECT_TRUE(g_synerEngine->GetQueueCacheSize() == 0);
    }
    EXPECT_TRUE(g_synerEngine->GetDiscardMsgNum() == 0);
}

/**
 * @tc.name: Anti Dos attack Sync 002
 * @tc.desc: Check if the enqueued and dequeue are normal when the whole length of messages is lower than
 *  maximum size of queue.
 * @tc.type: FUNC
 * @tc.require: AR000D08KU
 * @tc.author: yiguang
 */
HWTEST_F(KvdbAntiDosSyncTest, AntiDosAttackSync002, TestSize.Level3)
{
    /**
     * @tc.steps: step1. set block in function DispatchMessage as true.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_SHORT_TIME));
    g_communicatorAggregator->SetBlockValue(true);

    /**
     * @tc.steps: step2. control MessageReceiveCallback to send suitable messages.
     */
    const std::string srcTarget = "001";

    for (unsigned int index = 0; index < g_synerEngine->GetMaxExecNum() + TEST_TWO; index++) {
        std::vector<SendDataItem> outData;
        DataRequestPacket *requestPacket = new (std::nothrow) DataRequestPacket;
        EXCEPT_TRUE(requestPacket != nullptr);
        Message *messages = new (std::nothrow) Message(DATA_SYNC_MESSAGE);
        EXCEPT_TRUE(messages != nullptr);

        GenericSingleVerKvEntry *kvEntry = new (std::nothrow) GenericSingleVerKvEntry();
        EXCEPT_TRUE(kvEntry != nullptr);
        outData.push_back(kvEntry);
        requestPacket->SetData(outData);
        requestPacket->SetSendCode(E_OK);
        requestPacket->SetVersion(SOFTWARE_VERSION_CURRENT);

        uint32_t sessionId = index;
        uint32_t sequenceId = index;
        messages->SetMessageType(TYPE_REQUEST);
        messages->SetTarget(srcTarget);
        int errCode = messages->SetExternalObject(requestPacket);
        EXCEPT_TRUE(errCode == E_OK);
        g_communicator->CallbackOnMessage(srcTarget, messages);
    }

    /**
     * @tc.expected: step2. all messages enter the queue.
     */
    EXPECT_TRUE(g_synerEngine->GetDiscardMsgNum() == 0);
    EXPECT_EQ(g_synerEngine->GetQueueCacheSize() / NUM, 0);

    /**
     * @tc.steps: step3. set block in function DispatchMessage as false after a period of time.
     */
    g_communicator->Disable();
    g_communicatorAggregator->SetBlockValue(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_LONG_TIME));

    /**
     * @tc.expected: step3. the queue is eventually empty and no messages is discarded.
     */
    EXPECT_TRUE(g_synerEngine->GetDiscardMsgNum() == 0);
    EXPECT_TRUE(g_synerEngine->GetQueueCacheSize() == 0);
}

/**
 * @tc.name: Anti Dos attack Sync 003
 * @tc.desc: Whether messages enter and drop when all threads hang.
 * @tc.type: FUNC
 * @tc.require: AR000D08KU
 * @tc.author: yiguang
 */
HWTEST_F(KvdbAntiDosSyncTest, AntiDosAttackSync003, TestSize.Level3)
{
    /**
     * @tc.steps: step1. set block in function DispatchMessage as true.
     */
    g_communicatorAggregator->SetBlockValue(true);
    g_communicator->SetRemoteVersion(0u);
    /**
     * @tc.steps: step2. control MessageReceiveCallback to send messages that are more than maximum size of queue.
     */
    const std::string srcTarget = "001";

    for (unsigned int index = 0; index < g_synerEngine->GetMaxExecNum() + TEST_THREE_THREAD; index++) {
        std::vector<SendDataItem> outData;
        DataRequestPacket *requestPacket = new (std::nothrow) DataRequestPacket;
        EXCEPT_TRUE(requestPacket != nullptr);
        Message *messages = new (std::nothrow) Message(DATA_SYNC_MESSAGE);
        EXCEPT_TRUE(messages != nullptr);
        for (int outIndex = 0; outIndex < TEST_THREE_OUTDATA; outIndex++) {
            GenericSingleVerKvEntry *kvEntry = new (std::nothrow) GenericSingleVerKvEntry();
            EXCEPT_TRUE(kvEntry != nullptr);
            outData.push_back(kvEntry);
        }
        requestPacket->SetData(outData);
        requestPacket->SetSendCode(E_OK);
        requestPacket->SetVersion(SOFTWARE_VERSION_CURRENT);

        uint32_t sessionId = index;
        uint32_t sequenceId = index;
        messages->SetMessageType(TYPE_REQUEST);
        const std::string target = srcTarget + std::to_string(index);
        messages->SetTarget(target);
        int errCode = messages->SetExternalObject(requestPacket);
        EXCEPT_TRUE(errCode == E_OK);
        messages->SetSessionId(sessionId);
        messages->SetSequenceId(sequenceId);
        g_communicator->CallbackOnMessage(target, messages);
    }

    /**
     * @tc.expected: step2. after part of messages are enqueued, the rest of the messages are discarded.
     */
    g_communicator->SetRemoteVersion(UINT16_MAX);
    EXPECT_TRUE(g_synerEngine->GetDiscardMsgNum() > 0);
    EXPECT_TRUE(g_synerEngine->GetQueueCacheSize() > 0);
    g_communicatorAggregator->SetBlockValue(false);
}
