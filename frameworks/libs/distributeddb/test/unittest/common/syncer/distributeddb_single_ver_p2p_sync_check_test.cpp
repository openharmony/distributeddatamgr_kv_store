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

#include "ability_sync.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate_impl.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"
#include "process_system_api_adapter_impl.h"
#include "single_ver_data_packet.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string STORE_ID = "kv_stroe_sync_check_test";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";
    const int LOCAL_WATER_MARK_NOT_INIT = 0xaa;
    const int EIGHT_HUNDRED = 800;
    const int NORMAL_SYNC_SEND_REQUEST_CNT = 3;
    const int TWO_CNT = 2;
    const int SLEEP_MILLISECONDS = 500;
    const int TEN_SECONDS = 10;
    const int THREE_HUNDRED = 300;
    const int WAIT_30_SECONDS = 30000;
    const int WAIT_40_SECONDS = 40000;
    const int TIMEOUT_6_SECONDS = 6000;

    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreConfig g_config;
    DistributedDBToolsUnitTest g_tool;
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate* g_kvDelegatePtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    KvVirtualDevice* g_deviceB = nullptr;
    KvVirtualDevice* g_deviceC = nullptr;
    VirtualSingleVerSyncDBInterface *g_syncInterfaceB = nullptr;
    VirtualSingleVerSyncDBInterface *g_syncInterfaceC = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvDelegatePtr));
#ifndef LOW_LEVEL_MEM_DEV
    const int KEY_LEN = 20; // 20 Bytes
    const int VALUE_LEN = 4 * 1024 * 1024; // 4MB
    const int ENTRY_NUM = 2; // 16 entries
#endif

class DistributedDBSingleVerP2PSyncCheckTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void CancelTestInit(DeviceSyncOption &option, std::vector<Entry> &entries, const uint32_t mtuSize);
    void CancelTestEnd(std::vector<Entry> &entries, const uint32_t mtuSize);
};

void DistributedDBSingleVerP2PSyncCheckTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);

    string dir = g_testDir + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);

    std::shared_ptr<ProcessSystemApiAdapterImpl> g_adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
}

void DistributedDBSingleVerP2PSyncCheckTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

void DistributedDBSingleVerP2PSyncCheckTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B and C, and get a KvStoreNbDelegate as deviceA
     */
    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = SecurityLabel::S3;
    option.secOption.securityFlag = SecurityFlag::SECE;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    g_syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(g_syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, g_syncInterfaceB), E_OK);
    SecurityOption virtualOption;
    virtualOption.securityLabel = option.secOption.securityLabel;
    virtualOption.securityFlag = option.secOption.securityFlag;
    g_syncInterfaceB->SetSecurityOption(virtualOption);

    g_deviceC = new (std::nothrow) KvVirtualDevice(DEVICE_C);
    ASSERT_TRUE(g_deviceC != nullptr);
    g_syncInterfaceC = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(g_syncInterfaceC != nullptr);
    ASSERT_EQ(g_deviceC->Initialize(g_communicatorAggregator, g_syncInterfaceC), E_OK);
    g_syncInterfaceC->SetSecurityOption(virtualOption);
    RuntimeContext::GetInstance()->ClearAllDeviceTimeInfo();
}

void DistributedDBSingleVerP2PSyncCheckTest::TearDown(void)
{
    /**
     * @tc.teardown: Release device A, B, C
     */
    if (g_kvDelegatePtr != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
        g_kvDelegatePtr = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(STORE_ID);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    if (g_deviceC != nullptr) {
        delete g_deviceC;
        g_deviceC = nullptr;
    }
    if (g_communicatorAggregator != nullptr) {
        g_communicatorAggregator->RegOnDispatch(nullptr);
    }
}

/**
 * @tc.name: sec option check Sync 001
 * @tc.desc: if sec option not equal, forbid sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wangchuanqing
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SecOptionCheck001, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    ASSERT_TRUE(g_syncInterfaceB != nullptr);
    ASSERT_TRUE(g_syncInterfaceC != nullptr);
    SecurityOption secOption{SecurityLabel::S4, SecurityFlag::ECE};
    g_syncInterfaceB->SetSecurityOption(secOption);
    g_syncInterfaceC->SetSecurityOption(secOption);

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return SECURITY_OPTION_CHECK_ERROR.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == SECURITY_OPTION_CHECK_ERROR);
    }
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
}

/**
 * @tc.name: sec option check Sync 002
 * @tc.desc: if sec option not equal, forbid sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wangchuanqing
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SecOptionCheck002, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    ASSERT_TRUE(g_syncInterfaceC != nullptr);
    SecurityOption secOption{SecurityLabel::S4, SecurityFlag::ECE};
    g_syncInterfaceC->SetSecurityOption(secOption);
    secOption.securityLabel = SecurityLabel::S3;
    secOption.securityFlag = SecurityFlag::SECE;
    g_syncInterfaceB->SetSecurityOption(secOption);

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return SECURITY_OPTION_CHECK_ERROR.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        if (pair.first == DEVICE_B) {
            EXPECT_TRUE(pair.second == OK);
        } else {
            EXPECT_TRUE(pair.second == SECURITY_OPTION_CHECK_ERROR);
        }
    }
    VirtualDataItem item;
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value);
}

/**
 * @tc.name: sec option check Sync 003
 * @tc.desc: if sec option equal, check not pass, forbid sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SecOptionCheck003, TestSize.Level1)
{
    auto adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter);
    adapter->ForkCheckDeviceSecurityAbility([](const std::string &, const SecurityOption &) {
        return false;
    });
    /**
     * @tc.steps: step1. record packet
     * @tc.expected: step1. sync should failed in source.
     */
    std::atomic<int> messageCount = 0;
    g_communicatorAggregator->RegOnDispatch([&messageCount](const std::string &, Message *) {
        messageCount++;
    });
    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return SECURITY_OPTION_CHECK_ERROR.
     */
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(messageCount, 4); // 4 = 2 time sync + 2 ability sync
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == SECURITY_OPTION_CHECK_ERROR);
    }
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: sec option check Sync 004
 * @tc.desc: memory db not check device security
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SecOptionCheck004, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;
    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = SecurityLabel::NOT_SET;
    option.isMemoryDb = true;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);

    auto adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter);
    adapter->ForkCheckDeviceSecurityAbility([](const std::string &, const SecurityOption &) {
        return false;
    });
    adapter->ForkGetSecurityOption([](const std::string &, SecurityOption &securityOption) {
        securityOption.securityLabel = NOT_SET;
        return OK;
    });
    g_syncInterfaceB->ForkGetSecurityOption([](SecurityOption &) {
        return -E_NOT_SUPPORT;
    });

    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    EXPECT_EQ(status, OK);
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    adapter->ForkCheckDeviceSecurityAbility(nullptr);
    adapter->ForkGetSecurityOption(nullptr);
    g_syncInterfaceB->ForkGetSecurityOption(nullptr);
}

/**
 * @tc.name: sec option check Sync 005
 * @tc.desc: if sec option equal, check not pass, forbid sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SecOptionCheck005, TestSize.Level1)
{
    auto adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter);
    g_syncInterfaceB->ForkGetSecurityOption([](SecurityOption &option) {
        option.securityLabel = NOT_SET;
        return E_OK;
    });
    adapter->ForkGetSecurityOption([](const std::string &, SecurityOption &securityOption) {
        securityOption.securityLabel = NOT_SET;
        return OK;
    });

    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    EXPECT_EQ(status, OK);
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == SECURITY_OPTION_CHECK_ERROR);
    }

    adapter->ForkCheckDeviceSecurityAbility(nullptr);
    adapter->ForkGetSecurityOption(nullptr);
    g_syncInterfaceB->ForkGetSecurityOption(nullptr);
}

/**
 * @tc.name: sec option check Sync 006
 * @tc.desc: memory db not check device security
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SecOptionCheck006, TestSize.Level1)
{
    ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    ASSERT_EQ(g_mgr.DeleteKvStore(STORE_ID), OK);
    g_kvDelegatePtr = nullptr;
    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = SecurityLabel::S1;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);

    auto adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter);
    adapter->ForkCheckDeviceSecurityAbility([](const std::string &, const SecurityOption &) {
        return true;
    });
    adapter->ForkGetSecurityOption([](const std::string &, SecurityOption &securityOption) {
        securityOption.securityLabel = S1;
        return OK;
    });
    g_syncInterfaceB->ForkGetSecurityOption([](SecurityOption &option) {
        option.securityLabel = SecurityLabel::S0;
        return E_OK;
    });

    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    EXPECT_EQ(status, OK);
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(std::make_shared<ProcessSystemApiAdapterImpl>());
    g_syncInterfaceB->ForkGetSecurityOption(nullptr);
}

/**
 * @tc.name: sec option check Sync 007
 * @tc.desc: sync should send security option
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SecOptionCheck007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. fork check device security ability
     * @tc.expected: step1. check param option should be S3 SECE.
     */
    auto adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter);
    adapter->ForkCheckDeviceSecurityAbility([](const std::string &, const SecurityOption &option) {
        EXPECT_EQ(option.securityLabel, SecurityLabel::S3);
        EXPECT_EQ(option.securityFlag, SecurityFlag::SECE);
        return true;
    });
    /**
     * @tc.steps: step2. sync twice
     * @tc.expected: step2. sync success.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    auto status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

/**
 * @tc.name: SecOptionCheck008
 * @tc.desc: pull compress sync when check device ability fail
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SecOptionCheck008, TestSize.Level1)
{
    auto adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter);
    auto deviceB = g_deviceB->GetDeviceId();
    adapter->ForkCheckDeviceSecurityAbility([deviceB](const std::string &dev, const SecurityOption &) {
        if (dev != "real_device") {
            return true;
        }
        return false;
    });
    g_syncInterfaceB->ForkGetSecurityOption([](SecurityOption &option) {
        option.securityLabel = SecurityLabel::S3;
        option.securityFlag = SecurityFlag::SECE;
        return E_OK;
    });
    g_syncInterfaceB->SetCompressSync(true);
    std::vector<std::string> devices;
    devices.push_back(deviceB);
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    EXPECT_EQ(status, OK);
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_EQ(pair.second, SECURITY_OPTION_CHECK_ERROR);
    }

    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(std::make_shared<ProcessSystemApiAdapterImpl>());
    g_syncInterfaceB->ForkGetSecurityOption(nullptr);
    g_syncInterfaceB->SetCompressSync(false);
}

/**
 * @tc.name: SyncProcess001
 * @tc.desc: sync process pull mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenghuitao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncProcess001, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceB deviceC put bigData
     */
    std::vector<Entry> entries;
    const int dataCount = 10;
    DistributedDBUnitTest::GenerateNumberEntryVector(dataCount, entries);

    for (uint32_t i = 0; i < entries.size(); i++) {
        if (i % 2 == 0) {
            g_deviceB->PutData(entries[i].key, entries[i].value, 0, 0);
        } else {
            g_deviceC->PutData(entries[i].key, entries[i].value, 0, 0);
        }
    }

    /**
     * @tc.steps: step2. deviceA call pull sync
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DeviceSyncProcess> syncProcessMap;
    DeviceSyncOption option;
    option.devices = devices;
    option.mode = SYNC_MODE_PULL_ONLY;
    option.isQuery = false;
    option.isWait = false;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, option, syncProcessMap);
    EXPECT_EQ(status, DBStatus::OK);

    /**
     * @tc.expected: step3. onProcess should be called, DeviceA have all bigData
     */
    for (const auto &entry : entries) {
        Value value;
        EXPECT_EQ(g_kvDelegatePtr->Get(entry.key, value), DBStatus::OK);
        EXPECT_EQ(value, entry.value);
    }

    for (const auto &entry : syncProcessMap) {
        LOGD("[SyncProcess001] dev %s, status %d, totalCount %u, finishedCount %u", entry.first.c_str(),
            entry.second.errCode, entry.second.pullInfo.total, entry.second.pullInfo.finishedCount);
        EXPECT_EQ(entry.second.errCode, OK);
        EXPECT_EQ(entry.second.process, ProcessStatus::FINISHED);
        EXPECT_EQ(entry.second.pullInfo.total, static_cast<uint32_t>(dataCount / 2));
        EXPECT_EQ(entry.second.pullInfo.finishedCount, static_cast<uint32_t>(dataCount / 2));
        ASSERT_TRUE(entry.second.syncId > 0);
    }
}

/**
 * @tc.name: SyncProcess002
 * @tc.desc: sync process pull mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenghuitao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncProcess002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. deviceA put bigData
     */
    std::vector<Entry> entries;
    const int dataCount = 10;
    DistributedDBUnitTest::GenerateNumberEntryVector(dataCount, entries);

    for (uint32_t i = 0; i < entries.size(); i++) {
        g_kvDelegatePtr->Put(entries[i].key, entries[i].value);
    }

    /**
     * @tc.steps: step2. virtual deviceB call pull sync
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DeviceSyncProcess> syncProcessMap;
    DeviceSyncOption option;
    option.mode = SYNC_MODE_PULL_ONLY;
    option.isQuery = false;
    option.isWait = true;
    uint32_t processCount = 0;
    std::vector<ProcessStatus> statuses = {ProcessStatus::PREPARED, ProcessStatus::PROCESSING, ProcessStatus::FINISHED};
    DeviceSyncProcessCallback onProcess =
        [&](const std::map<std::string, DeviceSyncProcess> &processMap) {
            syncProcessMap = processMap;
            for (const auto &entry : processMap) {
                LOGD("[SyncProcess002-onProcess] dev %s, status %d, process %d", entry.first.c_str(),
                    entry.second.errCode, entry.second.process);
                EXPECT_EQ(entry.second.errCode, DBStatus::OK);
                EXPECT_EQ(entry.second.process, statuses[processCount]);
                // total and finishedCount must be greater than 0 when processing
                if (entry.second.process == ProcessStatus::PROCESSING) {
                    EXPECT_TRUE(entry.second.pullInfo.total > 0);
                    EXPECT_TRUE(entry.second.pullInfo.finishedCount > 0);
                }
            }
            processCount++;
        };
    int status = g_deviceB->Sync(option, onProcess);
    EXPECT_EQ(status, E_OK);

    /**
     * @tc.expected: step3. onProcess should be called, DeviceB have all bigData
     */
    for (const auto &entry : entries) {
        VirtualDataItem item;
        EXPECT_EQ(g_deviceB->GetData(entry.key, item), E_OK);
        EXPECT_EQ(item.value, entry.value);
    }

    for (const auto &entry : syncProcessMap) {
        LOGD("[SyncProcess002] dev %s, status %d, totalCount %u, finishedCount %u", entry.first.c_str(),
            entry.second.errCode, entry.second.pullInfo.total, entry.second.pullInfo.finishedCount);
        EXPECT_EQ(entry.second.errCode, DBStatus::OK);
        EXPECT_EQ(entry.second.process, ProcessStatus::FINISHED);
        EXPECT_EQ(entry.second.pullInfo.total, static_cast<uint32_t>(dataCount));
        EXPECT_EQ(entry.second.pullInfo.finishedCount, static_cast<uint32_t>(dataCount));
        ASSERT_TRUE(entry.second.syncId > 0);
    }
}

/**
 * @tc.name: SyncProcess003
 * @tc.desc: sync process pull mode with QUERY.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenghuitao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncProcess003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. deviceA put bigData
     */
    std::vector<Entry> entries;
    const int dataCount = 10;
    DistributedDBUnitTest::GenerateNumberEntryVector(dataCount, entries);

    for (uint32_t i = 0; i < entries.size(); i++) {
        g_kvDelegatePtr->Put(entries[i].key, entries[i].value);
    }

    /**
     * @tc.steps: step2. virtual deviceB call pull sync
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DeviceSyncProcess> syncProcessMap;
    DeviceSyncOption option;
    option.mode = SYNC_MODE_PULL_ONLY;
    option.isQuery = true;
    option.isWait = true;
    option.query = Query::Select().Limit(5);
    DeviceSyncProcessCallback onProcess =
        [&syncProcessMap, this](const std::map<std::string, DeviceSyncProcess> &processMap) {
            syncProcessMap = processMap;
        };
    int status = g_deviceB->Sync(option, onProcess);
    EXPECT_EQ(status, E_OK);

    /**
     * @tc.expected: step3. onProcess should be called, DeviceB have all bigData
     */
    for (const auto &entry : std::vector<Entry>(entries.begin(), entries.begin() + 5)) {
        VirtualDataItem item;
        EXPECT_EQ(g_deviceB->GetData(entry.key, item), E_OK);
        EXPECT_EQ(item.value, entry.value);
    }

    for (const auto &entry : syncProcessMap) {
        LOGD("[SyncProcess003] dev %s, status %d, totalCount %u, finishedCount %u", entry.first.c_str(),
            entry.second.errCode, entry.second.pullInfo.total, entry.second.pullInfo.finishedCount);
        EXPECT_EQ(entry.second.errCode, DBStatus::OK);
        EXPECT_EQ(entry.second.process, ProcessStatus::FINISHED);
        EXPECT_EQ(entry.second.pullInfo.total, 5u);
        EXPECT_EQ(entry.second.pullInfo.finishedCount, 5u);
        ASSERT_TRUE(entry.second.syncId > 0);
    }
}

#ifndef LOW_LEVEL_MEM_DEV
/**
 * @tc.name: BigDataSync001
 * @tc.desc: big data sync push mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wangchuanqing
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, BigDataSync001, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put 16 bigData
     */
    std::vector<Entry> entries;
    std::vector<Key> keys;
    DistributedDBUnitTest::GenerateRecords(ENTRY_NUM, entries, keys, KEY_LEN, VALUE_LEN);
    for (const auto &entry : entries) {
        status = g_kvDelegatePtr->Put(entry.key, entry.value);
        ASSERT_TRUE(status == OK);
    }

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step2. onComplete should be called, DeviceB,C have {k1,v1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    for (const auto &entry : entries) {
        item.value.clear();
        g_deviceB->GetData(entry.key, item);
        EXPECT_TRUE(item.value == entry.value);
        item.value.clear();
        g_deviceC->GetData(entry.key, item);
        EXPECT_TRUE(item.value == entry.value);
    }
}

/**
 * @tc.name: BigDataSync002
 * @tc.desc: big data sync pull mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wangchuanqing
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, BigDataSync002, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA deviceB put bigData
     */
    std::vector<Entry> entries;
    std::vector<Key> keys;
    DistributedDBUnitTest::GenerateRecords(ENTRY_NUM, entries, keys, KEY_LEN, VALUE_LEN);

    for (uint32_t i = 0; i < entries.size(); i++) {
        if (i % 2 == 0) {
            g_deviceB->PutData(entries[i].key, entries[i].value, 0, 0);
        } else {
            g_deviceC->PutData(entries[i].key, entries[i].value, 0, 0);
        }
    }

    /**
     * @tc.steps: step3. deviceA call pull sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceA have all bigData
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    for (const auto &entry : entries) {
        Value value;
        EXPECT_EQ(g_kvDelegatePtr->Get(entry.key, value), OK);
        EXPECT_EQ(value, entry.value);
    }
}

/**
 * @tc.name: BigDataSync003
 * @tc.desc: big data sync pushAndPull mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: wangchuanqing
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, BigDataSync003, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA deviceB put bigData
     */
    std::vector<Entry> entries;
    std::vector<Key> keys;
    DistributedDBUnitTest::GenerateRecords(ENTRY_NUM, entries, keys, KEY_LEN, VALUE_LEN);

    for (uint32_t i = 0; i < entries.size(); i++) {
        if (i % 3 == 0) { // 0 3 6 9 12 15 for deivec B
            g_deviceB->PutData(entries[i].key, entries[i].value, 0, 0);
        } else if (i % 3 == 1) { // 1 4 7 10 13 16 for device C
            g_deviceC->PutData(entries[i].key, entries[i].value, 0, 0);
        } else { // 2 5 8 11 14 for device A
            status = g_kvDelegatePtr->Put(entries[i].key, entries[i].value);
            ASSERT_TRUE(status == OK);
        }
    }

    /**
     * @tc.steps: step3. deviceA call pushAndpull sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceA have all bigData
     * deviceB and deviceC has deviceA data
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    VirtualDataItem item;
    for (uint32_t i = 0; i < entries.size(); i++) {
        Value value;
        EXPECT_EQ(g_kvDelegatePtr->Get(entries[i].key, value), OK);
        EXPECT_EQ(value, entries[i].value);

        if (i % 3 == 2) { // 2 5 8 11 14 for device A
        item.value.clear();
        g_deviceB->GetData(entries[i].key, item);
        EXPECT_TRUE(item.value == entries[i].value);
        item.value.clear();
        g_deviceC->GetData(entries[i].key, item);
        EXPECT_TRUE(item.value == entries[i].value);
        }
    }
}
#endif

void DistributedDBSingleVerP2PSyncCheckTest::CancelTestInit(DeviceSyncOption &option, std::vector<Entry> &entries,
    const uint32_t mtuSize)
{
    option.devices.push_back(g_deviceB->GetDeviceId());
    option.devices.push_back(g_deviceC->GetDeviceId());
    option.mode = SYNC_MODE_PULL_ONLY;
    option.isQuery = false;
    option.isWait = false;

    std::vector<Key> keys;
    const uint32_t entriesSize = 14000u;
    const int keySize = 20;
    DistributedDBUnitTest::GenerateRecords(entriesSize, entries, keys, keySize, mtuSize);
    for (uint32_t i = 0; i < entries.size(); i++) {
        if (i % option.devices.size() == 0) {
            g_deviceB->PutData(entries[i].key, entries[i].value, 0, 0);
        } else {
            g_deviceC->PutData(entries[i].key, entries[i].value, 0, 0);
        }
    }

    g_communicatorAggregator->SetDeviceMtuSize("real_device", mtuSize);
    g_communicatorAggregator->SetDeviceMtuSize(DEVICE_C, mtuSize);
    g_communicatorAggregator->SetDeviceMtuSize(DEVICE_B, mtuSize);
}

void DistributedDBSingleVerP2PSyncCheckTest::CancelTestEnd(std::vector<Entry> &entries, const uint32_t mtuSize)
{
    size_t syncSuccCount = 0;
    for (uint32_t i = 0; i < entries.size(); i++) {
        Value value;
        if (g_kvDelegatePtr->Get(entries[i].key, value) == OK) {
            syncSuccCount++;
            EXPECT_EQ(value, entries[i].value);
        }
    }
    EXPECT_GT(syncSuccCount, static_cast<size_t>(0));
    EXPECT_LT(syncSuccCount, entries.size());
    uint32_t mtu = 5u;
    g_communicatorAggregator->SetDeviceMtuSize("real_device", mtu * mtuSize * mtuSize);
    g_communicatorAggregator->SetDeviceMtuSize(DEVICE_C, mtu * mtuSize * mtuSize);
    g_communicatorAggregator->SetDeviceMtuSize(DEVICE_B, mtu * mtuSize * mtuSize);
    g_communicatorAggregator->RegBeforeDispatch(nullptr);
}

/**
 * @tc.name: CancelSyncProcess001
 * @tc.desc: cancel data sync process pull mode.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lijun
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncProcessCancel001, TestSize.Level1)
{
    DeviceSyncOption option;
    std::vector<Entry> entries;
    const uint32_t mtuSize = 8u;
    /**
     * @tc.steps: step1. deviceB deviceC put data
    */
    CancelTestInit(option, entries, mtuSize);
    uint32_t syncId;
    std::mutex tempMutex;
    bool isFirst = true;
    g_communicatorAggregator->RegBeforeDispatch([&](const std::string &dstTarget, const Message *msg) {
        if (dstTarget == "real_device" && msg->GetMessageType() == TYPE_REQUEST &&
            msg->GetMessageId() == DATA_SYNC_MESSAGE) {
            tempMutex.lock();
            if (isFirst == true) {
                isFirst = false;
                /**
                * @tc.steps: step3. cancel sync
                * @tc.expected: step3. should return OK.
                */
                ASSERT_TRUE(g_kvDelegatePtr->CancelSync(syncId) == OK);
                tempMutex.unlock();
                std::this_thread::sleep_for(std::chrono::seconds(1));
                return;
            }
            tempMutex.unlock();
        }
    });

    std::mutex cancelMtx;
    std::condition_variable cancelCv;
    bool cancelFinished = false;

    DeviceSyncProcessCallback onProcess = [&](const std::map<std::string, DeviceSyncProcess> &processMap) {
        bool isAllCancel = true;
        for (auto &process : processMap) {
            syncId = process.second.syncId;
            if (process.second.errCode != COMM_FAILURE) {
                isAllCancel = false;
            }
        }
        if (isAllCancel) {
            std::unique_lock<std::mutex> lock(cancelMtx);
            cancelFinished = true;
            cancelCv.notify_all();
        }
    };
    /**
     * @tc.steps: step2. deviceA call pull sync
     * @tc.expected: step2. sync should return OK.
     */
    ASSERT_TRUE(g_kvDelegatePtr->Sync(option, onProcess) == OK);

    // Wait onProcess complete.
    {
        std::unique_lock<std::mutex> lock2(cancelMtx);
        cancelCv.wait(lock2, [&cancelFinished]() {return cancelFinished;});
    }
    // Wait until all the packets arrive.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    /**
     * @tc.steps: step4. Cancel abnormal syncId.
     * @tc.expected: step4. return NOT_FOUND.
     */
    ASSERT_TRUE(g_kvDelegatePtr->CancelSync(syncId) == NOT_FOUND);
    ASSERT_TRUE(g_kvDelegatePtr->CancelSync(0) == NOT_FOUND);
    ASSERT_TRUE(g_kvDelegatePtr->CancelSync(4294967295) == NOT_FOUND); // uint32_t max value 4294967295
    CancelTestEnd(entries, mtuSize);
}

/**
 * @tc.name: PushFinishedNotify 001
 * @tc.desc: Test remote device push finished notify function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, PushFinishedNotify001, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA call SetRemotePushFinishedNotify
     * @tc.expected: step1. set should return OK.
     */
    int pushfinishedFlag = 0;
    DBStatus status = g_kvDelegatePtr->SetRemotePushFinishedNotify(
        [&pushfinishedFlag](const RemotePushNotifyInfo &info) {
            EXPECT_TRUE(info.deviceId == DEVICE_B);
            pushfinishedFlag = 1;
    });
    ASSERT_EQ(status, OK);

    /**
     * @tc.steps: step2. deviceB put k2, v2, and deviceA pull from deviceB
     * @tc.expected: step2. deviceA can not receive push finished notify
     */
    EXPECT_EQ(g_kvDelegatePtr->Put(KEY_2, VALUE_2), OK);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    EXPECT_TRUE(status == OK);
    EXPECT_EQ(pushfinishedFlag, 0);
    pushfinishedFlag = 0;

    /**
     * @tc.steps: step3. deviceB put k3, v3, and deviceA push and pull to deviceB
     * @tc.expected: step3. deviceA can not receive push finished notify
     */
    EXPECT_EQ(g_kvDelegatePtr->Put(KEY_3, VALUE_3), OK);
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    EXPECT_TRUE(status == OK);
    EXPECT_EQ(pushfinishedFlag, 0);
    pushfinishedFlag = 0;

    /**
     * @tc.steps: step4. deviceA call SetRemotePushFinishedNotify to reset notify
     * @tc.expected: step4. set should return OK.
     */
    status = g_kvDelegatePtr->SetRemotePushFinishedNotify([&pushfinishedFlag](const RemotePushNotifyInfo &info) {
        EXPECT_TRUE(info.deviceId == DEVICE_B);
        pushfinishedFlag = 2;
    });
    ASSERT_EQ(status, OK);

    /**
     * @tc.steps: step5. deviceA call SetRemotePushFinishedNotify set null to unregist
     * @tc.expected: step5. set should return OK.
     */
    status = g_kvDelegatePtr->SetRemotePushFinishedNotify(nullptr);
    ASSERT_EQ(status, OK);
}

namespace {
void RegOnDispatchWithDelayAck(bool &errCodeAck, bool &afterErrAck)
{
    // just delay the busy ack
    g_communicatorAggregator->RegOnDispatch([&errCodeAck, &afterErrAck](const std::string &dev, Message *inMsg) {
        if (dev != g_deviceB->GetDeviceId()) {
            return;
        }
        auto *packet = inMsg->GetObject<DataAckPacket>();
        if (packet != nullptr && packet->GetRecvCode() == -E_BUSY) {
            errCodeAck = true;
            while (!afterErrAck) {
            }
            LOGW("NOW SEND BUSY ACK");
        } else if (errCodeAck) {
            afterErrAck = true;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}

void RegOnDispatchWithOffline(bool &offlineFlag, bool &invalid, condition_variable &conditionOffline)
{
    g_communicatorAggregator->RegOnDispatch([&offlineFlag, &invalid, &conditionOffline](
                                                const std::string &dev, Message *inMsg) {
        auto *packet = inMsg->GetObject<DataAckPacket>();
        if (dev != DEVICE_B) {
            if (packet != nullptr && (packet->GetRecvCode() == LOCAL_WATER_MARK_NOT_INIT)) {
                offlineFlag = true;
                conditionOffline.notify_all();
                LOGW("[Dispatch] NOTIFY OFFLINE");
                std::this_thread::sleep_for(std::chrono::microseconds(EIGHT_HUNDRED));
            }
        } else if (!invalid && inMsg->GetMessageType() == TYPE_REQUEST) {
            LOGW("[Dispatch] NOW INVALID THIS MSG");
            inMsg->SetMessageType(TYPE_INVALID);
            inMsg->SetMessageId(INVALID_MESSAGE_ID);
            invalid = true;
        }
    });
}

void RegOnDispatchWithInvalidMsg(bool &invalid)
{
    g_communicatorAggregator->RegOnDispatch([&invalid](
        const std::string &dev, Message *inMsg) {
        if (dev == DEVICE_B && !invalid && inMsg->GetMessageType() == TYPE_REQUEST) {
            LOGW("[Dispatch] NOW INVALID THIS MSG");
            inMsg->SetMessageType(TYPE_INVALID);
            inMsg->SetMessageId(INVALID_MESSAGE_ID);
            invalid = true;
        }
    });
}

void PrepareEnv(vector<std::string> &devices, Key &key, Query &query)
{
    /**
     * @tc.steps: step1. ensure the watermark is no zero and finish timeSync and abilitySync
     * @tc.expected: step1. should return OK.
     */
    Value value = {'1'};
    std::map<std::string, DBStatus> result;
    ASSERT_TRUE(g_kvDelegatePtr->Put(key, value) == OK);

    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, DistributedDB::SYNC_MODE_PUSH_ONLY, result, query);
    EXPECT_TRUE(status == OK);
    ASSERT_TRUE(result[g_deviceB->GetDeviceId()] == OK);
}

void Sync(KvStoreNbDelegate *kvDelegatePtr, vector<std::string> &devices, const DBStatus &targetStatus)
{
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(kvDelegatePtr, devices, DistributedDB::SYNC_MODE_PUSH_ONLY, result);
    EXPECT_TRUE(status == OK);
    for (const auto &deviceId : devices) {
        ASSERT_TRUE(result[deviceId] == targetStatus);
    }
}

void Sync(vector<std::string> &devices, const DBStatus &targetStatus)
{
    Sync(g_kvDelegatePtr, devices, targetStatus);
}

void SyncWithQuery(vector<std::string> &devices, const Query &query, const SyncMode &mode,
    const DBStatus &targetStatus)
{
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, mode, result, query);
    EXPECT_TRUE(status == OK);
    for (const auto &deviceId : devices) {
        if (targetStatus == COMM_FAILURE) {
            // If syncTaskContext of deviceB is scheduled to be executed first, ClearAllSyncTask is
            // invoked when OfflineHandleByDevice is triggered, and SyncOperation::Finished() is triggered in advance.
            // The returned status is COMM_FAILURE.
            // If syncTaskContext of deviceB is not executed first, the error code is transparently transmitted.
            EXPECT_TRUE((result[deviceId] == static_cast<DBStatus>(-E_PERIPHERAL_INTERFACE_FAIL)) ||
                (result[deviceId] == COMM_FAILURE));
        } else {
            ASSERT_EQ(result[deviceId], targetStatus);
        }
    }
}

void SyncWithQuery(vector<std::string> &devices, const Query &query, const DBStatus &targetStatus)
{
    SyncWithQuery(devices, query, DistributedDB::SYNC_MODE_PUSH_ONLY, targetStatus);
}

void SyncWithDeviceOffline(vector<std::string> &devices, Key &key, const Query &query)
{
    Value value = {'2'};
    ASSERT_TRUE(g_kvDelegatePtr->Put(key, value) == OK);

    /**
     * @tc.steps: step2. invalid the sync msg
     * @tc.expected: step2. should return TIME_OUT.
     */
    SyncWithQuery(devices, query, TIME_OUT);

    /**
     * @tc.steps: step3. device offline when sync
     * @tc.expected: step3. should return COMM_FAILURE.
     */
    SyncWithQuery(devices, query, COMM_FAILURE);
}

void PrepareWaterMarkError(std::vector<std::string> &devices, Query &query)
{
    /**
     * @tc.steps: step1. prepare data
     */
    devices.push_back(g_deviceB->GetDeviceId());
    g_deviceB->Online();

    Key key = {'1'};
    query = Query::Select().PrefixKey(key);
    PrepareEnv(devices, key, query);

    /**
     * @tc.steps: step2. query sync and set queryWaterMark
     * @tc.expected: step2. should return OK.
     */
    Value value = {'2'};
    ASSERT_TRUE(g_kvDelegatePtr->Put(key, value) == OK);
    SyncWithQuery(devices, query, OK);

    /**
     * @tc.steps: step3. sync and invalid msg for set local device waterMark
     * @tc.expected: step3. should return TIME_OUT.
     */
    bool invalidMsg = false;
    RegOnDispatchWithInvalidMsg(invalidMsg);
    value = {'3'};
    ASSERT_TRUE(g_kvDelegatePtr->Put(key, value) == OK);
    Sync(devices, TIME_OUT);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

void RegOnDispatchWithoutDataPacket(std::atomic<int> &messageCount, bool calResponse = false)
{
    g_communicatorAggregator->RegOnDispatch([calResponse, &messageCount](const std::string &dev, Message *msg) {
        if (msg->GetMessageId() != TIME_SYNC_MESSAGE && msg->GetMessageId() != ABILITY_SYNC_MESSAGE) {
            return;
        }
        if (dev != DEVICE_B || (!calResponse && msg->GetMessageType() != TYPE_REQUEST)) {
            return;
        }
        messageCount++;
    });
}

void ReOpenDB()
{
    ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;
    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = SecurityLabel::S3;
    option.secOption.securityFlag = SecurityFlag::SECE;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
}
}

/**
 * @tc.name: AckSessionCheck 001
 * @tc.desc: Test ack session check function.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, AckSessionCheck001, TestSize.Level3)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceB sync to deviceA just for timeSync and abilitySync
     * @tc.expected: step1. should return OK.
     */
    ASSERT_TRUE(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true) == E_OK);

    /**
     * @tc.steps: step2. deviceA StartTransaction for prevent other sync action deviceB sync will fail
     * @tc.expected: step2. should return OK.
     */
    ASSERT_TRUE(g_kvDelegatePtr->StartTransaction() == OK);

    bool errCodeAck = false;
    bool afterErrAck = false;
    RegOnDispatchWithDelayAck(errCodeAck, afterErrAck);

    Key key = {'1'};
    Value value = {'1'};
    Timestamp currentTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_TRUE(g_deviceB->PutData(key, value, currentTime, 0) == E_OK);
    EXPECT_TRUE(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true) == E_OK);

    Value outValue;
    EXPECT_TRUE(g_kvDelegatePtr->Get(key, outValue) == NOT_FOUND);

    /**
     * @tc.steps: step3. release the writeHandle and try again, sync success
     * @tc.expected: step3. should return OK.
     */
    EXPECT_TRUE(g_kvDelegatePtr->Commit() == OK);
    EXPECT_TRUE(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true) == E_OK);

    EXPECT_TRUE(g_kvDelegatePtr->Get(key, outValue) == OK);
    EXPECT_EQ(outValue, value);
}

/**
 * @tc.name: AckSafeCheck001
 * @tc.desc: Test ack session check filter all bad ack in device offline scene.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, AckSafeCheck001, TestSize.Level3)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    g_deviceB->Online();

    Key key = {'1'};
    Query query = Query::Select().PrefixKey(key);
    PrepareEnv(devices, key, query);

    std::condition_variable conditionOnline;
    std::condition_variable conditionOffline;
    bool onlineFlag = false;
    bool invalid = false;
    bool offlineFlag = false;
    thread subThread([&onlineFlag, &conditionOnline, &offlineFlag, &conditionOffline]() {
        LOGW("[Dispatch] NOW DEVICES IS OFFLINE");
        std::mutex offlineMtx;
        std::unique_lock<std::mutex> lck(offlineMtx);
        conditionOffline.wait(lck, [&offlineFlag]{ return offlineFlag; });
        g_deviceB->Offline();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        g_deviceB->Online();
        onlineFlag = true;
        conditionOnline.notify_all();
        LOGW("[Dispatch] NOW DEVICES IS ONLINE");
    });
    subThread.detach();

    RegOnDispatchWithOffline(offlineFlag, invalid, conditionOffline);

    SyncWithDeviceOffline(devices, key, query);

    std::mutex onlineMtx;
    std::unique_lock<std::mutex> lck(onlineMtx);
    conditionOnline.wait(lck, [&onlineFlag]{ return onlineFlag; });

    /**
     * @tc.steps: step4. sync again if has problem it will sync never end
     * @tc.expected: step4. should return OK.
     */
    SyncWithQuery(devices, query, OK);
}

/**
 * @tc.name: WaterMarkCheck001
 * @tc.desc: Test waterMark work correct in lost package scene.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, WaterMarkCheck001, TestSize.Level1)
{
    std::vector<std::string> devices;
    Query query = Query::Select();
    PrepareWaterMarkError(devices, query);

    /**
     * @tc.steps: step4. sync again see it work correct
     * @tc.expected: step4. should return OK.
     */
    SyncWithQuery(devices, query, OK);
}

/**
 * @tc.name: WaterMarkCheck002
 * @tc.desc: Test pull work correct in error waterMark scene.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, WaterMarkCheck002, TestSize.Level1)
{
    std::vector<std::string> devices;
    Query query = Query::Select();
    PrepareWaterMarkError(devices, query);

    /**
     * @tc.steps: step4. sync again see it work correct
     * @tc.expected: step4. should return OK.
     */
    Key key = {'2'};
    ASSERT_TRUE(g_kvDelegatePtr->Put(key, {}) == OK);
    query = Query::Select();
    SyncWithQuery(devices, query, DistributedDB::SYNC_MODE_PULL_ONLY, OK);

    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(key, item), -E_NOT_FOUND);
}

void RegOnDispatchToGetSyncCount(int &sendRequestCount, int sleepMs = 0)
{
    g_communicatorAggregator->RegOnDispatch([sleepMs, &sendRequestCount](
            const std::string &dev, Message *inMsg) {
        if (dev == DEVICE_B && inMsg->GetMessageType() == TYPE_REQUEST) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            sendRequestCount++;
            LOGD("sendRequestCount++...");
        }
    });
}

void TestDifferentSyncMode(SyncMode mode)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    DBStatus status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    int sendRequestCount = 0;
    RegOnDispatchToGetSyncCount(sendRequestCount);

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, mode, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step2. onComplete should be called, DeviceB have {k1,v1}, send request message 3 times
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value);

    EXPECT_EQ(sendRequestCount, NORMAL_SYNC_SEND_REQUEST_CNT);

    /**
     * @tc.steps: step3. reset sendRequestCount to 0, deviceA call sync and wait again without any change in db
     * @tc.expected: step3. sync should return OK, and sendRequestCount should be 1, because this merge can not
     * be skipped
     */
    sendRequestCount = 0;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);
    EXPECT_EQ(sendRequestCount, 1);
}

/**
 * @tc.name: PushSyncMergeCheck001
 * @tc.desc: Test push sync task merge, task can not be merged when the two sync task is not in the queue
 * at the same time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncMergeCheck001, TestSize.Level1)
{
    TestDifferentSyncMode(SYNC_MODE_PUSH_ONLY);
}

/**
 * @tc.name: PushSyncMergeCheck002
 * @tc.desc: Test push_pull sync task merge, task can not be merged when the two sync task is not in the queue
 * at the same time.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncMergeCheck002, TestSize.Level1)
{
    TestDifferentSyncMode(SYNC_MODE_PUSH_PULL);
}

void PrepareForSyncMergeTest(std::vector<std::string> &devices, int &sendRequestCount)
{
    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    DBStatus status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    RegOnDispatchToGetSyncCount(sendRequestCount, SLEEP_MILLISECONDS);

    /**
     * @tc.steps: step2. deviceA call sync and don't wait
     * @tc.expected: step2. sync should return OK.
     */
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [&sendRequestCount, devices, key, value](const std::map<std::string, DBStatus>& statusMap) {
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &pair : statusMap) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
        VirtualDataItem item;
        g_deviceB->GetData(key, item);
        EXPECT_EQ(item.value, value);
        EXPECT_EQ(sendRequestCount, NORMAL_SYNC_SEND_REQUEST_CNT);

        // reset sendRequestCount to 0
        sendRequestCount = 0;
    });
    ASSERT_TRUE(status == OK);
}

/**
 * @tc.name: PushSyncMergeCheck003
 * @tc.desc: Test push sync task merge, task can not be merged when there is change in db since last push sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncMergeCheck003, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    int sendRequestCount = 0;
    PrepareForSyncMergeTest(devices, sendRequestCount);

    /**
     * @tc.steps: step3. deviceA call sync and don't wait
     * @tc.expected: step3. sync should return OK.
     */
    Key key = {'1'};
    Value value = {'2'};
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [&sendRequestCount, devices, key, value, this](const std::map<std::string, DBStatus>& statusMap) {
        /**
         * @tc.expected: when the second sync task return, sendRequestCount should be 1, because this merge can not be
         * skipped, but it is no need to do time sync and ability sync, only need to do data sync
         */
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &pair : statusMap) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
        VirtualDataItem item;
        g_deviceB->GetData(key, item);
        EXPECT_EQ(item.value, value);
    });
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step4. deviceA put {k1, v2}
     */
    while (sendRequestCount < TWO_CNT) {
        std::this_thread::sleep_for(std::chrono::milliseconds(THREE_HUNDRED));
    }
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);
    // wait for the second sync task finish
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
    EXPECT_EQ(sendRequestCount, 1);
}

/**
 * @tc.name: PushSyncMergeCheck004
 * @tc.desc: Test push sync task merge, task can be merged when there is no change in db since last push sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncMergeCheck004, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    int sendRequestCount = 0;
    PrepareForSyncMergeTest(devices, sendRequestCount);

    /**
     * @tc.steps: step3. deviceA call sync and don't wait
     * @tc.expected: step3. sync should return OK.
     */
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [devices, this](const std::map<std::string, DBStatus>& statusMap) {
        /**
         * @tc.expected: when the second sync task return, sendRequestCount should be 0, because this merge can  be
         * skipped
         */
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &pair : statusMap) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
    });
    ASSERT_TRUE(status == OK);
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
    EXPECT_EQ(sendRequestCount, 0);
}

void RegOnDispatchWithInvalidMsgAndCnt(int &sendRequestCount, int sleepMs, bool &invalid)
{
    g_communicatorAggregator->RegOnDispatch([&sendRequestCount, sleepMs, &invalid](
        const std::string &dev, Message *inMsg) {
        if (dev == DEVICE_B && !invalid && inMsg->GetMessageType() == TYPE_REQUEST) {
            inMsg->SetMessageType(TYPE_INVALID);
            inMsg->SetMessageId(INVALID_MESSAGE_ID);
            sendRequestCount++;
            invalid = true;
            LOGW("[Dispatch]invalid THIS MSG, sendRequestCount = %d", sendRequestCount);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        }
    });
}

/**
 * @tc.name: PushSyncMergeCheck005
 * @tc.desc: Test push sync task merge, task cannot be merged when the last push sync is failed
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, SyncMergeCheck005, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    int sendRequestCount = 0;
    bool invalid = false;
    RegOnDispatchWithInvalidMsgAndCnt(sendRequestCount, SLEEP_MILLISECONDS, invalid);

    /**
     * @tc.steps: step2. deviceA call sync and don't wait
     * @tc.expected: step2. sync should return TIME_OUT.
     */
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [&sendRequestCount, devices, this](const std::map<std::string, DBStatus>& statusMap) {
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &deviceId : devices) {
            ASSERT_EQ(statusMap.at(deviceId), TIME_OUT);
        }
    });
    EXPECT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call sync and don't wait
     * @tc.expected: step3. sync should return OK.
     */
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [key, value, &sendRequestCount, devices, this](const std::map<std::string, DBStatus>& statusMap) {
        /**
         * @tc.expected: when the second sync task return, sendRequestCount should be 3, because this merge can not be
         * skipped, deviceB should have {k1, v1}.
         */
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &pair : statusMap) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_EQ(pair.second, OK);
        }
        VirtualDataItem item;
        g_deviceB->GetData(key, item);
        EXPECT_EQ(item.value, value);
    });
    ASSERT_TRUE(status == OK);
    while (sendRequestCount < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(THREE_HUNDRED));
    }
    sendRequestCount = 0;
    RegOnDispatchToGetSyncCount(sendRequestCount, SLEEP_MILLISECONDS);

    // wait for the second sync task finish
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
    EXPECT_EQ(sendRequestCount, NORMAL_SYNC_SEND_REQUEST_CNT);
}

void PrePareForQuerySyncMergeTest(bool isQuerySync, std::vector<std::string> &devices,
    Key &key, Value &value, int &sendRequestCount)
{
    DBStatus status = OK;
    /**
     * @tc.steps: step1. deviceA put {k1, v1}...{k10, v10}
     */
    Query query = Query::Select().PrefixKey(key);
    const int dataSize = 10;
    for (int i = 0; i < dataSize; i++) {
        key.push_back(i);
        value.push_back(i);
        status = g_kvDelegatePtr->Put(key, value);
        ASSERT_TRUE(status == OK);
        key.pop_back();
        value.pop_back();
    }

    RegOnDispatchToGetSyncCount(sendRequestCount, SLEEP_MILLISECONDS);
    /**
     * @tc.steps: step2. deviceA call query sync and don't wait
     * @tc.expected: step2. sync should return OK.
     */
    auto completeCallBack = [&sendRequestCount, &key, &value, dataSize, devices]
        (const std::map<std::string, DBStatus>& statusMap) {
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &pair : statusMap) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_EQ(pair.second, OK);
        }
        // when first sync finish, DeviceB have {k1,v1}, {k3,v3}, {k5,v5} .. send request message 3 times
        VirtualDataItem item;
        for (int i = 0; i < dataSize; i++) {
            key.push_back(i);
            value.push_back(i);
            g_deviceB->GetData(key, item);
            EXPECT_EQ(item.value, value);
            key.pop_back();
            value.pop_back();
        }
        EXPECT_EQ(sendRequestCount, NORMAL_SYNC_SEND_REQUEST_CNT);
        // reset sendRequestCount to 0
        sendRequestCount = 0;
    };
    if (isQuerySync) {
        status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY, completeCallBack, query, false);
    } else {
        status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY, completeCallBack);
    }
    ASSERT_TRUE(status == OK);
}

/**
 * @tc.name: QuerySyncMergeCheck001
 * @tc.desc: Test query push sync task merge, task can be merged when there is no change in db since last query sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, QuerySyncMergeCheck001, TestSize.Level3)
{
    std::vector<std::string> devices;
    int sendRequestCount = 0;
    devices.push_back(g_deviceB->GetDeviceId());

    Key key {'1'};
    Value value {'1'};
    Query query = Query::Select().PrefixKey(key);
    PrePareForQuerySyncMergeTest(true, devices, key, value, sendRequestCount);

    /**
     * @tc.steps: step3. deviceA call query sync and don't wait
     * @tc.expected: step3. sync should return OK.
     */
    DBStatus status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [devices, this](const std::map<std::string, DBStatus>& statusMap) {
        /**
         * @tc.expected: when the second sync task return, sendRequestCount should be 0, because this merge can be
         * skipped because there is no change in db since last query sync
         */
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &pair : statusMap) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
    }, query, false);
    ASSERT_TRUE(status == OK);
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
    EXPECT_EQ(sendRequestCount, 0);
}

/**
 * @tc.name: QuerySyncMergeCheck002
 * @tc.desc: Test query push sync task merge, task can not be merged when there is change in db since last sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, QuerySyncMergeCheck002, TestSize.Level3)
{
    std::vector<std::string> devices;
    int sendRequestCount = 0;
    devices.push_back(g_deviceB->GetDeviceId());

    Key key {'1'};
    Value value {'1'};
    Query query = Query::Select().PrefixKey(key);
    PrePareForQuerySyncMergeTest(true, devices, key, value, sendRequestCount);

    /**
     * @tc.steps: step3. deviceA call query sync and don't wait
     * @tc.expected: step3. sync should return OK.
     */
    Value value3{'3'};
    DBStatus status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [&sendRequestCount, devices, key, value3, this](const std::map<std::string, DBStatus>& statusMap) {
        /**
         * @tc.expected: when the second sync task return, sendRequestCount should be 1, because this merge can not be
         * skipped when there is change in db since last query sync, deviceB have {k1, v1'}
         */
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &pair : statusMap) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
        VirtualDataItem item;
        g_deviceB->GetData(key, item);
        EXPECT_TRUE(item.value == value3);
        EXPECT_EQ(sendRequestCount, 1);
        }, query, false);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step4. deviceA put {k1, v1'}
     * @tc.steps: step4. reset sendRequestCount to 0, deviceA call sync and wait
     * @tc.expected: step4. sync should return OK, and sendRequestCount should be 1, because this merge can not
     * be skipped
     */
    while (sendRequestCount < TWO_CNT) {
        std::this_thread::sleep_for(std::chrono::milliseconds(THREE_HUNDRED));
    }
    g_kvDelegatePtr->Put(key, value3);
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
}

/**
 * @tc.name: QuerySyncMergeCheck003
 * @tc.desc: Test query push sync task merge, task can not be merged when then query id is different
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, QuerySyncMergeCheck003, TestSize.Level3)
{
    std::vector<std::string> devices;
    int sendRequestCount = 0;
    devices.push_back(g_deviceB->GetDeviceId());

    Key key {'1'};
    Value value {'1'};
    PrePareForQuerySyncMergeTest(true, devices, key, value, sendRequestCount);

    /**
     * @tc.steps: step3.  deviceA call another query sync
     * @tc.expected: step3. sync should return OK.
     */
    Key key2 = {'2'};
    Value value2 = {'2'};
    DBStatus status = g_kvDelegatePtr->Put(key2, value2);
    ASSERT_TRUE(status == OK);
    Query query2 = Query::Select().PrefixKey(key2);
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [&sendRequestCount, key2, value2, devices, this](const std::map<std::string, DBStatus>& statusMap) {
        /**
         * @tc.expected: when the second sync task return, sendRequestCount should be 1, because this merge can not be
         * skipped, deviceB have {k2,v2}
         */
        ASSERT_TRUE(statusMap.size() == devices.size());
        for (const auto &pair : statusMap) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
        VirtualDataItem item;
        g_deviceB->GetData(key2, item);
        EXPECT_TRUE(item.value == value2);
        EXPECT_EQ(sendRequestCount, 1);
        }, query2, false);
    ASSERT_TRUE(status == OK);
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
}

/**
* @tc.name: QuerySyncMergeCheck004
* @tc.desc: Test query push sync task merge, task can be merged when there is no change in db since last push sync
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhangshijie
*/
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, QuerySyncMergeCheck004, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    Key key {'1'};
    Value value {'1'};
    int sendRequestCount = 0;
    PrePareForQuerySyncMergeTest(false, devices, key, value, sendRequestCount);

    /**
     * @tc.steps: step3. deviceA call query sync without any change in db
     * @tc.expected: step3. sync should return OK, and sendRequestCount should be 0, because this merge can be skipped
     */
    Query query = Query::Select().PrefixKey(key);
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [devices, this](const std::map<std::string, DBStatus>& statusMap) {
            /**
             * @tc.expected step3: when the second sync task return, sendRequestCount should be 0, because this merge
             * can be skipped because there is no change in db since last push sync
             */
            ASSERT_TRUE(statusMap.size() == devices.size());
            for (const auto &pair : statusMap) {
                LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
                EXPECT_TRUE(pair.second == OK);
            }
        }, query, false);
    ASSERT_TRUE(status == OK);
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
    EXPECT_EQ(sendRequestCount, 0);
}

/**
  * @tc.name: GetDataNotify001
  * @tc.desc: Test GetDataNotify function, delay < 30s should sync ok, > 36 should timeout
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, GetDataNotify001, TestSize.Level3)
{
    ASSERT_NE(g_kvDelegatePtr, nullptr);
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    const std::string DEVICE_A = "real_device";
    /**
     * @tc.steps: step1. deviceB set get data delay 40s
     */
    g_deviceB->DelayGetSyncData(WAIT_40_SECONDS);
    g_communicatorAggregator->SetTimeout(DEVICE_A, TIMEOUT_6_SECONDS);

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK. onComplete should be called, deviceB sync TIME_OUT.
     */
    std::map<std::string, DBStatus> result;
    std::map<std::string, int> virtualRes;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, true);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(result.size(), devices.size());
    EXPECT_EQ(result[DEVICE_B], TIME_OUT);
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
    Query query = Query::Select();
    g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, query, [&virtualRes](std::map<std::string, int> resMap) {
        virtualRes = std::move(resMap);
    }, true);
    EXPECT_EQ(virtualRes.size(), devices.size());
    EXPECT_EQ(virtualRes[DEVICE_A], static_cast<int>(SyncOperation::OP_TIMEOUT));
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));

    /**
     * @tc.steps: step3. deviceB set get data delay 30s
     */
    g_deviceB->DelayGetSyncData(WAIT_30_SECONDS);
    /**
     * @tc.steps: step4. deviceA call sync and wait
     * @tc.expected: step4. sync should return OK. onComplete should be called, deviceB sync OK.
     */
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, true);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(result.size(), devices.size());
    EXPECT_EQ(result[DEVICE_B], OK);
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
    g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, query, [&virtualRes](std::map<std::string, int> resMap) {
        virtualRes = std::move(resMap);
    }, true);
    EXPECT_EQ(virtualRes.size(), devices.size());
    EXPECT_EQ(virtualRes[DEVICE_A], static_cast<int>(SyncOperation::OP_FINISHED_ALL));
    g_deviceB->DelayGetSyncData(0);
}

/**
  * @tc.name: GetDataNotify002
  * @tc.desc: Test GetDataNotify function, two device sync each other at same time
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, GetDataNotify002, TestSize.Level3)
{
    ASSERT_NE(g_kvDelegatePtr, nullptr);
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    const std::string DEVICE_A = "real_device";

    /**
     * @tc.steps: step1. deviceA sync first to finish time sync and ability sync
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, true);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(result.size(), devices.size());
    EXPECT_EQ(result[DEVICE_B], OK);
    /**
     * @tc.steps: step2. deviceB set get data delay 30s
     */
    g_deviceB->DelayGetSyncData(WAIT_30_SECONDS);

    /**
     * @tc.steps: step3. deviceB call sync and wait
     */
    std::thread asyncThread([]() {
        std::map<std::string, int> virtualRes;
        Query query = Query::Select();
        g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, query, [&virtualRes](std::map<std::string, int> resMap) {
                virtualRes = std::move(resMap);
            }, true);
    });

    /**
     * @tc.steps: step4. deviceA call sync and wait
     * @tc.expected: step4. sync should return OK. because notify timer trigger (30s - 1s)/2s => 15times
     */
    std::this_thread::sleep_for(std::chrono::seconds(1));
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, true);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(result.size(), devices.size());
    EXPECT_EQ(result[DEVICE_B], OK);
    asyncThread.join();
    std::this_thread::sleep_for(std::chrono::seconds(TEN_SECONDS));
}

/**
 * @tc.name: DelaySync001
 * @tc.desc: Test delay first packet will not effect data conflict
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, DelaySync001, TestSize.Level3)
{
    // B put (k, b) after A put (k, a)
    Key key = {'k'};
    Value aValue = {'a'};
    g_kvDelegatePtr->Put(key, aValue);
    std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s for data conflict
    Timestamp currentTime = TimeHelper::GetSysCurrentTime() + TimeHelper::BASE_OFFSET;
    Value bValue = {'b'};
    EXPECT_EQ(g_deviceB->PutData(key, bValue, currentTime, 0), E_OK);

    // delay time sync message, delay time/2 should greater than put sleep time
    g_communicatorAggregator->SetTimeout(DEVICE_B, DBConstant::MAX_TIMEOUT);
    g_communicatorAggregator->SetTimeout("real_device", DBConstant::MAX_TIMEOUT);
    g_communicatorAggregator->RegBeforeDispatch([](const std::string &dstTarget, const Message *msg) {
        if (dstTarget == DEVICE_B && msg->GetMessageId() == MessageId::TIME_SYNC_MESSAGE) {
            std::this_thread::sleep_for(std::chrono::seconds(3)); // sleep for 3s
        }
    });

    // A call sync and (k, b) in A
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, true);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(result.size(), devices.size());
    EXPECT_EQ(result[DEVICE_B], OK);

    Value actualValue;
    g_kvDelegatePtr->Get(key, actualValue);
    EXPECT_EQ(actualValue, bValue);
    g_communicatorAggregator->RegBeforeDispatch(nullptr);
}

/**
 * @tc.name: KVAbilitySyncOpt001
 * @tc.desc: check ability sync 2 packet
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVAbilitySyncOpt001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet
     * @tc.expected: step1. sync should failed in source.
     */
    std::atomic<int> messageCount = 0;
    g_communicatorAggregator->RegOnDispatch([&messageCount](const std::string &dev, Message *msg) {
        if (msg->GetMessageId() != ABILITY_SYNC_MESSAGE) {
            return;
        }
        messageCount++;
        EXPECT_GE(g_kvDelegatePtr->GetTaskCount(), 1);
    });
    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return SECURITY_OPTION_CHECK_ERROR.
     */
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(messageCount, 2); // 2 ability sync
    for (const auto &pair : result) {
        EXPECT_EQ(pair.second, OK);
    }
}

/**
 * @tc.name: KVAbilitySyncOpt002
 * @tc.desc: check get task count while conn is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVAbilitySyncOpt002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. record packet while conn is nullptr.
     * @tc.expected: step1. sync should failed in source and get task count return DB_ERROR.
     */
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(g_kvDelegatePtr);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    std::atomic<int> messageCount = 0;
    g_communicatorAggregator->RegOnDispatch([&messageCount](const std::string &dev, Message *msg) {
        if (msg->GetMessageId() != ABILITY_SYNC_MESSAGE) {
            return;
        }
        messageCount++;
        EXPECT_EQ(g_kvDelegatePtr->GetTaskCount(), DB_ERROR);
    });
}

/**
 * @tc.name: KVSyncOpt001
 * @tc.desc: check time sync and ability sync once
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVSyncOpt001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount);
    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    Sync(devices, OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step3. reopen kv store
     * @tc.expected: step3. reopen OK.
     */
    ReOpenDB();
    /**
     * @tc.steps: step4. reopen kv store and sync again
     * @tc.expected: step4. reopen OK and sync success, no negotiation packet.
     */
    messageCount = 0;
    Sync(devices, OK);
    EXPECT_EQ(messageCount, 0);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: KVSyncOpt002
 * @tc.desc: check device time sync once
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVSyncOpt002, TestSize.Level1)
{
/**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount);
    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    Sync(devices, OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    // close kv store avoid packet dispatch error
    ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;
    ASSERT_EQ(g_mgr.DeleteKvStore(STORE_ID), OK);
    EXPECT_TRUE(RuntimeContext::GetInstance()->IsTimeTickMonitorValid());
    /**
     * @tc.steps: step3. open new kv store
     * @tc.expected: step3. open OK.
     */
    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = SecurityLabel::S3;
    option.secOption.securityFlag = SecurityFlag::SECE;
    KvStoreNbDelegate *delegate2 = nullptr;
    g_mgr.GetKvStore(STORE_ID_2, option, [&delegate2](DBStatus status, KvStoreNbDelegate *delegate) {
        delegate2 = delegate;
        EXPECT_EQ(status, OK);
    });
    /**
     * @tc.steps: step4. sync again
     * @tc.expected: step4. sync success, only ability sync packet.
     */
    messageCount = 0;
    Sync(delegate2, devices, OK);
    EXPECT_EQ(messageCount, 1); // 1 contain ability sync packet
    EXPECT_EQ(g_mgr.CloseKvStore(delegate2), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore(STORE_ID_2), OK);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: KVSyncOpt003
 * @tc.desc: check time sync and ability sync once
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVSyncOpt003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount);
    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    Sync(devices, OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step3. reopen kv store
     * @tc.expected: step3. reopen OK.
     */
    ReOpenDB();
    /**
     * @tc.steps: step4. reopen kv store and sync again
     * @tc.expected: step4. reopen OK and sync success, no negotiation packet.
     */
    messageCount = 0;
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 0);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: KVSyncOpt004
 * @tc.desc: check sync in keys after reopen db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVSyncOpt004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. deviceA call sync and wait
     * @tc.expected: step1. sync should return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    Sync(devices, OK);
    /**
     * @tc.steps: step2. reopen kv store
     * @tc.expected: step2. reopen OK.
     */
    ReOpenDB();
    /**
     * @tc.steps: step3. sync with in keys
     * @tc.expected: step3. sync OK.
     */
    std::map<std::string, DBStatus> result;
    std::set<Key> condition;
    condition.insert({'k'});
    Query query = Query::Select().InKeys(condition);
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, DistributedDB::SYNC_MODE_PUSH_ONLY, result, query);
    EXPECT_EQ(status, OK);
    for (const auto &deviceId : devices) {
        EXPECT_EQ(result[deviceId], OK);
    }
}

/**
 * @tc.name: KVSyncOpt005
 * @tc.desc: check record ability finish after receive ability sync
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVSyncOpt005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount, true);
    /**
     * @tc.steps: step2. deviceB call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 2); // DEV_A send negotiation 2 ack packet.
    /**
     * @tc.steps: step3. reopen kv store
     * @tc.expected: step3. reopen OK.
     */
    ReOpenDB();
    /**
     * @tc.steps: step4. reopen kv store and sync again
     * @tc.expected: step4. reopen OK and sync success, no negotiation packet.
     */
    messageCount = 0;
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 0);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: KVSyncOpt006
 * @tc.desc: check time sync and ability sync once after rebuild
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVSyncOpt006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount, true);
    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step3. rebuild kv store
     * @tc.expected: step3. rebuild OK.
     */
    ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;
    g_mgr.DeleteKvStore(STORE_ID);
    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = SecurityLabel::S3;
    option.secOption.securityFlag = SecurityFlag::SECE;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    /**
     * @tc.steps: step4. rebuild kv store and sync again
     * @tc.expected: step4. rebuild OK and sync success, re ability sync.
     */
    messageCount = 0;
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 1);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: KVSyncOpt007
 * @tc.desc: check re ability sync after import
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVSyncOpt007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount, true);
    /**
     * @tc.steps: step2. deviceB call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 2); // DEV_A send negotiation 2 ack packet.
    /**
     * @tc.steps: step3. export and import
     * @tc.expected: step3. export and import OK.
     */
    std::string singleExportFileName = g_testDir + "/KVSyncOpt007.$$";
    CipherPassword passwd;
    EXPECT_EQ(g_kvDelegatePtr->Export(singleExportFileName, passwd), OK);
    EXPECT_EQ(g_kvDelegatePtr->Import(singleExportFileName, passwd), OK);
    /**
     * @tc.steps: step4. reopen kv store and sync again
     * @tc.expected: step4. reopen OK and sync success, no negotiation packet.
     */
    messageCount = 0;
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 1); // DEV_A send negotiation 1 ack packet.
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: KVSyncOpt008
 * @tc.desc: check rebuild open store with NOT_SET.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tankaisheng
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVSyncOpt008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount, true);
    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step3. rebuild kv store
     * @tc.expected: step3. rebuild failed.
     */
    ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
    g_kvDelegatePtr = nullptr;
    g_mgr.DeleteKvStore(STORE_ID);
    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = SecurityLabel::NOT_SET;
    option.secOption.securityFlag = SecurityFlag::SECE;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == DBStatus::INVALID_ARGS);
    ASSERT_TRUE(g_kvDelegatePtr == nullptr);
}

/**
 * @tc.name: KVTimeChange001
 * @tc.desc: check time sync and ability sync once
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVTimeChange001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. record packet which send to B
     */
    std::atomic<int> messageCount = 0;
    RegOnDispatchWithoutDataPacket(messageCount);
    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    Sync(devices, OK);
    EXPECT_EQ(messageCount, 2); // 2 contain time sync request packet and ability sync packet
    /**
     * @tc.steps: step3. sync again
     * @tc.expected: step3. sync success, no negotiation packet.
     */
    messageCount = 0;
    Sync(devices, OK);
    EXPECT_EQ(messageCount, 0);
    /**
     * @tc.steps: step4. modify time offset and sync again
     * @tc.expected: step4. sync success, only time sync packet.
     */
    RuntimeContext::GetInstance()->NotifyTimestampChanged(100);
    RuntimeContext::GetInstance()->RecordAllTimeChange();
    RuntimeContext::GetInstance()->ClearAllDeviceTimeInfo();
    messageCount = 0;
    Sync(devices, OK);
    EXPECT_EQ(messageCount, 1); // 1 contain time sync request packet
    messageCount = 0;
    EXPECT_EQ(g_deviceB->Sync(SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(messageCount, 0);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
 * @tc.name: KVTimeChange002
 * @tc.desc: test NotifyTimestampChanged will not stuck when notify delegate with no metadata
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liuhongyang
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, KVTimeChange002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open a new store with STORE_ID_3
     * @tc.expected: step1. open success
     */
    KvStoreNbDelegate::Option option;
    option.secOption.securityLabel = SecurityLabel::S3;
    option.secOption.securityFlag = SecurityFlag::SECE;
    KvStoreNbDelegate *delegate2 = nullptr;
    g_mgr.GetKvStore(STORE_ID_3, option, [&delegate2](DBStatus status, KvStoreNbDelegate *delegate) {
        delegate2 = delegate;
        EXPECT_EQ(status, OK);
    });
    ASSERT_TRUE(delegate2 != nullptr);
    /**
     * @tc.steps: step2. STORE_ID_3 sync once so that it will be notified when time change
     * @tc.expected: step2. sync should return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    EXPECT_EQ(g_tool.SyncTest(delegate2, devices, SYNC_MODE_PULL_ONLY, result, true), OK);
    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    Sync(devices, OK);
    /**
     * @tc.steps: step4. call NotifyTimestampChanged
     * @tc.expected: step4. expect no deadlock
     */
    RuntimeContext::GetInstance()->NotifyTimestampChanged(100);
    /**
     * @tc.steps: step5. clean up the created db
     */
    ASSERT_EQ(g_mgr.CloseKvStore(delegate2), OK);
    delegate2 = nullptr;
    ASSERT_TRUE(g_mgr.DeleteKvStore(STORE_ID_3) == OK);
}

/**
 * @tc.name: InvalidSync001
 * @tc.desc: Test sync with empty tables
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBSingleVerP2PSyncCheckTest, InvalidSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. sync with empty tables
     * @tc.expected: step1. NOT_SUPPORT
     */
    Query query = Query::Select().FromTable({""});
    DBStatus callStatus = g_kvDelegatePtr->Sync({g_deviceB->GetDeviceId()}, SYNC_MODE_PUSH_ONLY, nullptr, query, true);
    EXPECT_EQ(callStatus, NOT_SUPPORT);

    /**
     * @tc.steps: step2. sync option with empty tables
     * @tc.expected: step2. NOT_SUPPORT
     */
    DeviceSyncOption option;
    option.devices = {g_deviceB->GetDeviceId()};
    option.isQuery = true;
    option.isWait = true;
    option.query = query;
    std::mutex cancelMtx;
    bool cancelFinished = false;
    DeviceSyncProcessCallback onProcess = [&](const std::map<std::string, DeviceSyncProcess> &processMap) {
        bool isAllCancel = true;
        for (auto &process : processMap) {
            if (process.second.errCode != COMM_FAILURE) {
                isAllCancel = false;
            }
        }
        if (isAllCancel) {
            std::unique_lock<std::mutex> lock(cancelMtx);
            cancelFinished = true;
        }
    };
    callStatus = OK;
    callStatus = g_kvDelegatePtr->Sync(option, onProcess);
    EXPECT_EQ(callStatus, NOT_SUPPORT);
}
}