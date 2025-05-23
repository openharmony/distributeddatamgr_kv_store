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

#include "db_errno.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "iprocess_system_api_adapter.h"
#include "log_print.h"
#include "process_system_api_adapter_impl.h"
#include "runtime_context.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    const std::string DATA_FILE_PATH = "/data/test/";
    SecurityOption g_option = {0, 0};
    const std::string DEV_ID = "devId";
    std::shared_ptr<ProcessSystemApiAdapterImpl> g_adapter;
    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    string g_testDir;
    KvStoreConfig g_config;

    // define the g_kvDelegateCallback, used to get some information when open a kv store.
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate *g_kvNbDelegatePtr = nullptr;
    auto g_kvNbDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvNbDelegatePtr));
}

class RuntimeContextProcessSystemApiAdapterImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
};

void RuntimeContextProcessSystemApiAdapterImplTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Get an adapter
     */
    g_adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    EXPECT_TRUE(g_adapter != nullptr);
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
}

void RuntimeContextProcessSystemApiAdapterImplTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void RuntimeContextProcessSystemApiAdapterImplTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    g_adapter->ResetAdapter();
}

/**
 * @tc.name: SetSecurityOption001
 * @tc.desc: Set SecurityOption.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, SetSecurityOption001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. call SetSecurityOption to set SecurityOption before set g_adapter
     * @tc.expected: step1. function return E_NOT_SUPPORT
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    int errCode = RuntimeContext::GetInstance()->SetSecurityOption(g_testDir, g_option);
    EXPECT_TRUE(errCode == -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. call SetSecurityOption to set SecurityOption after set g_adapter
     * @tc.expected: step2. function return E_OK
     */
    EXPECT_TRUE(g_adapter != nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    errCode = RuntimeContext::GetInstance()->SetSecurityOption(g_testDir, g_option);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: GetSecurityOption001
 * @tc.desc: Get SecurityOption.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, GetSecurityOption001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. call GetSecurityOption to get SecurityOption before set g_adapter
     * @tc.expected: step1. function return E_NOT_SUPPORT
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    int errCode = RuntimeContext::GetInstance()->GetSecurityOption(DATA_FILE_PATH, g_option);
    EXPECT_TRUE(errCode == -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. call GetSecurityOption to get SecurityOption after set g_adapter
     * @tc.expected: step2. function return E_OK
     */
    EXPECT_TRUE(g_adapter != nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    errCode = RuntimeContext::GetInstance()->GetSecurityOption(DATA_FILE_PATH, g_option);
    EXPECT_TRUE(errCode == E_OK);
}

/**
 * @tc.name: RegisterLockStatusLister001
 * @tc.desc: Register a listener.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, RegisterLockStatusLister001, TestSize.Level1)
{
    int errCode = E_OK;
    bool lockStatus = false;
    auto onEventFunction1 = [&lockStatus](void *isLock) {
        LOGI("lock status 1 changed %d", *(static_cast<bool *>(isLock)));
        lockStatus = *(static_cast<bool *>(isLock));
    };

    auto onEventFunction2 = [&lockStatus](void *isLock) {
        LOGI("lock status 2 changed %d", *(static_cast<bool *>(isLock)));
        lockStatus = *(static_cast<bool *>(isLock));
    };
    /**
     * @tc.steps: step1. call RegisterLockStatusLister to register a listener before set adapter
     * @tc.expected: step1. function return ok
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    NotificationChain::Listener *listener =
        RuntimeContext::GetInstance()->RegisterLockStatusLister(onEventFunction1, errCode);
    EXPECT_NE(listener, nullptr);
    EXPECT_EQ(errCode, E_OK);

    /**
     * @tc.steps: step2. call RegisterLockStatusLister to register a listener after set g_adapter
     * @tc.expected: step2. function return a not null listener
     */
    EXPECT_TRUE(g_adapter != nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);

    auto listener1 = RuntimeContext::GetInstance()->RegisterLockStatusLister(onEventFunction1, errCode);
    EXPECT_TRUE(errCode == E_OK);
    EXPECT_NE(listener1, nullptr);
    listener1->Drop();

    /**
     * @tc.steps: step3. call SetLockStatus to change lock status
     * @tc.expected: step3. the listener's callback should be called
     */
    g_adapter->SetLockStatus(false);
    EXPECT_TRUE(!lockStatus);

    /**
     * @tc.steps: step4. call RegisterLockStatusLister to register another listener after set g_adapter
     * @tc.expected: step4. function return a not null listener
     */
    listener->Drop();
    listener = RuntimeContext::GetInstance()->RegisterLockStatusLister(onEventFunction2, errCode);
    EXPECT_NE(listener, nullptr);
    listener->Drop();
}

/**
 * @tc.name: IsAccessControlled001
 * @tc.desc: Get Access Lock Status.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, IsAccessControlled001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. call IsAccessControlled to get Access lock status before set g_adapter
     * @tc.expected: step1. function return true
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    bool isLocked = RuntimeContext::GetInstance()->IsAccessControlled();
    EXPECT_FALSE(isLocked);

    /**
     * @tc.steps: step2. IsAccessControlled to get Access lock status after set g_adapter
     * @tc.expected: step2. function return false
     */
    EXPECT_TRUE(g_adapter != nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    isLocked = RuntimeContext::GetInstance()->IsAccessControlled();
    EXPECT_TRUE(!isLocked);
}

/**
 * @tc.name: CheckDeviceSecurityAbility001
 * @tc.desc: Check device security ability.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, CheckDeviceSecurityAbility001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. call CheckDeviceSecurityAbility to check device security ability before set g_adapter
     * @tc.expected: step1. function return true
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    bool isSupported = RuntimeContext::GetInstance()->CheckDeviceSecurityAbility(DEV_ID, g_option);
    EXPECT_TRUE(isSupported);

    /**
     * @tc.steps: step2. IsAccessControlled to check device security ability after set g_adapter
     * @tc.expected: step2. function return true
     */
    EXPECT_TRUE(g_adapter != nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    isSupported = RuntimeContext::GetInstance()->CheckDeviceSecurityAbility(DEV_ID, g_option);
    EXPECT_TRUE(isSupported);
}

namespace {
void FuncCheckDeviceSecurityAbility()
{
    RuntimeContext::GetInstance()->CheckDeviceSecurityAbility("", SecurityOption());
    return;
}

void CheckDeviceSecurityAbility002()
{
    g_config.dataDir = g_testDir;
    EXPECT_EQ(g_mgr.SetKvStoreConfig(g_config), OK);

    EXPECT_EQ(RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter), E_OK);
    g_adapter->SetNeedCreateDb(true);

    const std::string storeId = "CheckDeviceSecurityAbility002";
    std::thread t1(FuncCheckDeviceSecurityAbility);
    std::thread t2([&]() {
        for (int i = 0; i < 100; i++) { // open close 100 times
            LOGI("open store!!");
            KvStoreNbDelegate::Option option1 = {true, false, false};
            g_mgr.GetKvStore(storeId, option1, g_kvNbDelegateCallback);
            EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        }
    });

    t1.join();
    t2.join();
}
}

/**
 * @tc.name: CheckDeviceSecurityAbility002
 * @tc.desc: Check device security ability with getkvstore frequency.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, CheckDeviceSecurityAbility002, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckDeviceSecurityAbility002());
}

/**
 * @tc.name: SetSystemApiAdapterTest001
 * @tc.desc: Set SecurityOption.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, SetSystemApiAdapterTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. remove system api adapter
     * @tc.expected: step1. return false
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    EXPECT_FALSE(g_mgr.IsProcessSystemApiAdapterValid());

    /**
     * @tc.steps: step2. set g_adapter
     * @tc.expected: step2. return true
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    EXPECT_TRUE(g_mgr.IsProcessSystemApiAdapterValid());
}

/**
 * @tc.name: SecurityOptionUpgrade001
 * @tc.desc: Test upgrade security label from s1 to s3.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, SecurityOptionUpgrade001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set g_adapter and open with s1
     * @tc.expected: step1. return true
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    EXPECT_TRUE(g_mgr.IsProcessSystemApiAdapterValid());
    g_config.dataDir = g_testDir;
    EXPECT_EQ(g_mgr.SetKvStoreConfig(g_config), OK);

    const std::string storeId = "SecurityOptionUpgrade001";
    KvStoreNbDelegate::Option option = {true, false, false};
    option.secOption = { S1, ECE };
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    /**
     * @tc.steps: step2. re open with s3
     * @tc.expected: step2. open ok
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    option.secOption = { S3, SECE };
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    /**
     * @tc.steps: step3. re open with s4
     * @tc.expected: step3. open ok
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    option.secOption = { S4, SECE };
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

/**
 * @tc.name: SecurityOptionUpgrade002
 * @tc.desc: Test upgrade security label from NOT_SET to s3.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, SecurityOptionUpgrade002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set g_adapter and open with s1
     * @tc.expected: step1. return true
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(new(std::nothrow) VirtualCommunicatorAggregator);
    EXPECT_TRUE(g_mgr.IsProcessSystemApiAdapterValid());
    g_config.dataDir = g_testDir;
    EXPECT_EQ(g_mgr.SetKvStoreConfig(g_config), OK);

    const std::string storeId = "SecurityOptionUpgrade002";
    KvStoreNbDelegate::Option option = {true, false, false};
    option.secOption = { NOT_SET, ECE };
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    /**
     * @tc.steps: step2. re open with s3
     * @tc.expected: step2. open ok
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    option.secOption = { S3, SECE };
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

/**
 * @tc.name: SecurityOptionUpgrade003
 * @tc.desc: Test upgrade with error security label.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, SecurityOptionUpgrade003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set g_adapter and open with not set
     * @tc.expected: step1. return true
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(new(std::nothrow) VirtualCommunicatorAggregator);
    EXPECT_TRUE(g_mgr.IsProcessSystemApiAdapterValid());
    g_config.dataDir = g_testDir;
    EXPECT_EQ(g_mgr.SetKvStoreConfig(g_config), OK);

    const std::string storeId = "SecurityOptionUpgrade003";
    KvStoreNbDelegate::Option option = {true, false, false};
    option.secOption = { NOT_SET, ECE };
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    /**
     * @tc.steps: step2. re open with s3
     * @tc.expected: step2. open ok
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    auto existSecOpt = g_adapter->GetExistSecOpt();
    option.secOption = { S3, SECE };
    for (const auto &item : existSecOpt) {
        g_adapter->SetSecurityOption(item.first, option.secOption);
    }

    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

/**
 * @tc.name: SecurityOptionUpgrade004
 * @tc.desc: Test upgrade security label when set failed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(RuntimeContextProcessSystemApiAdapterImplTest, SecurityOptionUpgrade004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. set g_adapter and open with not set
     * @tc.expected: step1. return true
     */
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(new(std::nothrow) VirtualCommunicatorAggregator);
    EXPECT_TRUE(g_mgr.IsProcessSystemApiAdapterValid());
    g_config.dataDir = g_testDir;
    EXPECT_EQ(g_mgr.SetKvStoreConfig(g_config), OK);

    const std::string storeId = "SecurityOptionUpgrade004";
    KvStoreNbDelegate::Option option = {true, false, false};
    option.secOption = { S1, ECE };
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    /**
     * @tc.steps: step2. re open with s2 but get s3
     * @tc.expected: step2. open ok
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    auto existSecOpt = g_adapter->GetExistSecOpt();
    option.secOption = { S3, SECE };
    for (const auto &item : existSecOpt) {
        if (item.first.substr(item.first.size() - std::string(".db").size()) != std::string(".db")) {
            continue;
        }
        g_adapter->SetSecurityOption(item.first, option.secOption);
    }
    option.secOption = { S2, ECE };
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    existSecOpt = g_adapter->GetExistSecOpt();
    for (const auto &item : existSecOpt) {
        if (item.first.substr(item.first.size() - std::string(".db").size()) != std::string(".db")) {
            continue;
        }
        EXPECT_EQ(item.second.securityLabel, option.secOption.securityLabel);
    }
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}