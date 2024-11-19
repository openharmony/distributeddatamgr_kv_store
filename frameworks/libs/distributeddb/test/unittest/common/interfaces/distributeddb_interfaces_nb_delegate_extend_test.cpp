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
#include "db_constant.h"
#include "db_errno.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "platform_specific.h"
#include "process_system_api_adapter_impl.h"
#include "runtime_context.h"
#include "sqlite_single_ver_natural_store.h"
#include "storage_engine_manager.h"
#ifdef DB_DEBUG_ENV
#include "system_time.h"
#endif // DB_DEBUG_ENV
#include "kv_store_result_set_impl.h"
#include "kv_store_nb_delegate_impl.h"
#include "kv_virtual_device.h"
#include "virtual_communicator_aggregator.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    // define some variables to init a KvStoreDelegateManager object.
    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    string g_testDir;
    KvStoreConfig g_config;
    Key g_keyPrefix = {'A', 'B', 'C'};
    const int RESULT_SET_COUNT = 9;
    const int RESULT_SET_INIT_POS = -1;
    uint8_t g_testDict[RESULT_SET_COUNT] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

    // define the g_kvNbDelegateCallback, used to get some information when open a kv store.
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate *g_kvNbDelegatePtr = nullptr;
#ifndef OMIT_MULTI_VER
    KvStoreDelegate *g_kvDelegatePtr = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreDelegateCallback, placeholders::_1,
        placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvDelegatePtr));
#endif // OMIT_MULTI_VER
    const int OBSERVER_SLEEP_TIME = 100;
    const int BATCH_PRESET_SIZE_TEST = 10;
    const int DIVIDE_BATCH_PRESET_SIZE = 5;
    const int VALUE_OFFSET = 5;

    const int DEFAULT_KEY_VALUE_SIZE = 10;

    const int CON_PUT_THREAD_NUM = 4;
    const int PER_THREAD_PUT_NUM = 100;

    const std::string DEVICE_B = "deviceB";
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    KvVirtualDevice *g_deviceB = nullptr;
    VirtualSingleVerSyncDBInterface *g_syncInterfaceB = nullptr;

    // the type of g_kvNbDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvNbDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback, placeholders::_1,
        placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvNbDelegatePtr));

    enum LockState {
        UNLOCKED = 0,
        LOCKED
    };

    void InitResultSet()
    {
        Key testKey;
        Value testValue;
        for (int i = 0; i < RESULT_SET_COUNT; i++) {
            testKey.clear();
            testValue.clear();
            // set key
            testKey = g_keyPrefix;
            testKey.push_back(g_testDict[i]);
            // set value
            testValue.push_back(g_testDict[i]);
            // insert entry
            EXPECT_EQ(g_kvNbDelegatePtr->Put(testKey, testValue), OK);
        }
    }

    void InitVirtualDevice(const std::string &devId, KvVirtualDevice *&devices,
        VirtualSingleVerSyncDBInterface *&syncInterface)
    {
        devices = new (std::nothrow) KvVirtualDevice(devId);
        ASSERT_TRUE(devices != nullptr);
        syncInterface = new (std::nothrow) VirtualSingleVerSyncDBInterface();
        ASSERT_TRUE(syncInterface != nullptr);
        ASSERT_EQ(devices->Initialize(g_communicatorAggregator, syncInterface), E_OK);
    }

class DistributedDBInterfacesNBDelegateExtendTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBInterfacesNBDelegateExtendTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBInterfacesNBDelegateExtendTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);

    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBInterfacesNBDelegateExtendTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    g_kvDelegateStatus = INVALID_ARGS;
    g_kvNbDelegatePtr = nullptr;
#ifndef OMIT_MULTI_VER
    g_kvDelegatePtr = nullptr;
#endif // OMIT_MULTI_VER
}

void DistributedDBInterfacesNBDelegateExtendTest::TearDown(void)
{
    if (g_kvNbDelegatePtr != nullptr) {
        g_mgr.CloseKvStore(g_kvNbDelegatePtr);
        g_kvNbDelegatePtr = nullptr;
    }
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

#ifdef DB_DEBUG_ENV
/**
  * @tc.name: TimeChangeWithCloseStoreTest001
  * @tc.desc: Test close store with time changed
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, TimeChangeWithCloseStoreTest001, TestSize.Level3)
{
    KvStoreDelegateManager mgr(APP_ID, USER_ID);
    mgr.SetKvStoreConfig(g_config);

    std::atomic<bool> isFinished(false);

    std::vector<std::thread> slowThreads;
    for (int i = 0; i < 10; i++) { // 10: thread to slow donw system
        std::thread th([&isFinished]() {
            while (!isFinished) {
                // pass
            }
        });
        slowThreads.emplace_back(std::move(th));
    }

    std::thread th([&isFinished]() {
        int timeChangedCnt = 0;
        while (!isFinished.load()) {
            OS::SetOffsetBySecond(100 - timeChangedCnt++ * 2); // 100 2 : fake system time change
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100: wait for a while
        }
    });

    for (int i = 0; i < 100; i++) { // run 100 times
        const KvStoreNbDelegate::Option option = {true, false, false};
        mgr.GetKvStore(STORE_ID_1, option, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        EXPECT_EQ(g_kvDelegateStatus, OK);
        EXPECT_EQ(mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        g_kvNbDelegatePtr = nullptr;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000: wait for a while
    isFinished.store(true);
    th.join();
    for (auto &it : slowThreads) {
        it.join();
    }
    EXPECT_EQ(mgr.DeleteKvStore(STORE_ID_1), OK);
}

/**
  * @tc.name: TimeChangeWithCloseStoreTest002
  * @tc.desc: Test close store with time changed
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, TimeChangeWithCloseStoreTest002, TestSize.Level3)
{
    KvStoreDelegateManager mgr(APP_ID, USER_ID);
    mgr.SetKvStoreConfig(g_config);

    const KvStoreNbDelegate::Option option = {true, false, false};
    mgr.GetKvStore(STORE_ID_1, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    const int threadPoolMax = 10;
    for (int i = 0; i < threadPoolMax; ++i) {
        (void) RuntimeContext::GetInstance()->ScheduleTask([]() {
            std::this_thread::sleep_for(std::chrono::seconds(10)); // sleep 10s for block thread pool
        });
    }
    OS::SetOffsetBySecond(100); // 100 2 : fake system time change
    std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s for time tick

    EXPECT_EQ(mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvNbDelegatePtr = nullptr;

    EXPECT_EQ(mgr.DeleteKvStore(STORE_ID_1), OK);
    RuntimeContext::GetInstance()->StopTaskPool(); // stop all async task
}

/**
  * @tc.name: TimeChangeWithCloseStoreTest003
  * @tc.desc: Test store close with timechange listener
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, TimeChangeWithCloseStoreTest003, TestSize.Level3)
{
    /**
     * @tc.steps:step1. Create database.
     * @tc.expected: step1. Returns a non-null kvstore.
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore("TimeChangeWithCloseStoreTest003", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    std::shared_ptr<bool> timeChange = std::make_shared<bool>(false);
    int errCode = E_OK;
    auto *listener = RuntimeContext::GetInstance()->RegisterTimeChangedLister([timeChange](void *) {
        std::this_thread::sleep_for(std::chrono::seconds(10)); // block close store 10s
        *timeChange = true;
    }, nullptr, errCode);
    /**
     * @tc.steps:step2. Block time change 10s and trigger time change.
     * @tc.expected: step2. close store cost time > 5s.
     */
    ASSERT_EQ(errCode, E_OK);
    OS::SetOffsetBySecond(100); // 100 : fake system time change
    std::this_thread::sleep_for(std::chrono::seconds(1)); // wait 1s for time change
    Timestamp beginTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(beginTime);
    ASSERT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    Timestamp endTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(endTime);
    if (*timeChange) {
        EXPECT_GE(static_cast<int>(endTime - beginTime), 5 * 1000 * 1000); // 5 * 1000 * 1000 = 5s
    }
    listener->Drop(true);
    OS::SetOffsetBySecond(-100); // -100 : fake system time change
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore("TimeChangeWithCloseStoreTest003"), OK);
}

/**
  * @tc.name: TimeChangeWithDataChangeTest001
  * @tc.desc: Test change data and change time
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: liaoyonghuang
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, TimeChangeWithDataChangeTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create local database.
     * @tc.expected: step1. Returns a non-null kvstore.
     */
    KvStoreNbDelegate::Option option;
    option.localOnly = true;
    g_mgr.GetKvStore("TimeChangeWithDataChangeTest001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    /**
     * @tc.steps:step2. Set 100s time offset, put{k1, v1}. Then Set 0s time offset, put {k1, v2}.
     * @tc.expected: step2. Get {k1, v2} form DB.
     */
    OS::SetOffsetBySecond(100); // 100 : fake system time change
    g_kvNbDelegatePtr->Put(KEY_1, VALUE_1);
    OS::SetOffsetBySecond(0);
    g_kvNbDelegatePtr->Put(KEY_1, VALUE_2);
    Value expectValue = VALUE_2;
    Value actualValue;
    g_kvNbDelegatePtr->Get(KEY_1, actualValue);
    EXPECT_EQ(expectValue, actualValue);

    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore("TimeChangeWithDataChangeTest001"), OK);
}
#endif // DB_DEBUG_ENV

/**
  * @tc.name: ResultSetLimitTest001
  * @tc.desc: Get result set over limit
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: lianhuix
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, ResultSetLimitTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create database.
     * @tc.expected: step1. Returns a non-null kvstore.
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore("ResultSetLimitTest001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);

    /**
     * @tc.steps:step2. Put the random entry into the database.
     * @tc.expected: step2. Returns OK.
     */
    EXPECT_EQ(g_kvNbDelegatePtr->Put(KEY_1, VALUE_1), OK);
    EXPECT_EQ(g_kvNbDelegatePtr->Put(KEY_2, VALUE_2), OK);
    EXPECT_EQ(g_kvNbDelegatePtr->Put(KEY_3, VALUE_3), OK);

    /**
     * @tc.steps:step3. Get the resultset overlimit.
     * @tc.expected: step3. In limit returns OK, else return OVER_MAX_LIMITS.
     */
    std::vector<KvStoreResultSet *> dataResultSet;
    for (int i = 0; i < 8; i++) { // 8: max result set count
        KvStoreResultSet *resultSet = nullptr;
        EXPECT_EQ(g_kvNbDelegatePtr->GetEntries(Key{}, resultSet), OK);
        dataResultSet.push_back(resultSet);
        EXPECT_NE(resultSet, nullptr);
    }

    KvStoreResultSet *resultSet = nullptr;
    EXPECT_EQ(g_kvNbDelegatePtr->GetEntries(Key{}, resultSet), OVER_MAX_LIMITS);
    EXPECT_EQ(resultSet, nullptr);
    if (resultSet != nullptr) {
        EXPECT_EQ(g_kvNbDelegatePtr->CloseResultSet(resultSet), OK);
    }

    /**
     * @tc.steps:step4. Close result set and store.
     * @tc.expected: step4. Returns OK.
     */
    for (auto it : dataResultSet) {
        EXPECT_EQ(g_kvNbDelegatePtr->CloseResultSet(it), OK);
    }

    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("ResultSetLimitTest001"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: LocalStore001
  * @tc.desc: Test get kv store with localOnly
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, LocalStore001, TestSize.Level1)
{
    KvStoreDelegateManager mgr(APP_ID, USER_ID);
    mgr.SetKvStoreConfig(g_config);

    /**
     * @tc.steps:step1. Create database with localOnly.
     * @tc.expected: step1. Returns a non-null store.
     */
    KvStoreNbDelegate::Option option = {true, false, false};
    option.localOnly = true;
    DBStatus openStatus = DBStatus::DB_ERROR;
    KvStoreNbDelegate *openDelegate = nullptr;
    mgr.GetKvStore(STORE_ID_1, option, [&openStatus, &openDelegate](DBStatus status, KvStoreNbDelegate *delegate) {
        openStatus = status;
        openDelegate = delegate;
    });
    ASSERT_TRUE(openDelegate != nullptr);
    EXPECT_EQ(openStatus, OK);
    /**
     * @tc.steps:step2. call sync and put/get interface.
     * @tc.expected: step2. sync return NOT_ACTIVE.
     */
    DBStatus actionStatus = openDelegate->Sync({}, SyncMode::SYNC_MODE_PUSH_ONLY, nullptr);
    EXPECT_EQ(actionStatus, DBStatus::NOT_ACTIVE);
    Key key = {'k'};
    Value expectValue = {'v'};
    EXPECT_EQ(openDelegate->Put(key, expectValue), OK);
    Value actualValue;
    EXPECT_EQ(openDelegate->Get(key, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);

    int pragmaData = 1;
    auto input = static_cast<PragmaData>(&pragmaData);
    EXPECT_EQ(openDelegate->Pragma(SET_SYNC_RETRY, input), NOT_SUPPORT);

    EXPECT_EQ(mgr.CloseKvStore(openDelegate), OK);
    EXPECT_EQ(mgr.DeleteKvStore(STORE_ID_1), OK);
    openDelegate = nullptr;
}

/**
  * @tc.name: LocalStore002
  * @tc.desc: Test get kv store different local mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, LocalStore002, TestSize.Level1)
{
    KvStoreDelegateManager mgr(APP_ID, USER_ID);
    mgr.SetKvStoreConfig(g_config);

    /**
     * @tc.steps:step1. Create database with localOnly.
     * @tc.expected: step1. Returns a non-null store.
     */
    KvStoreNbDelegate::Option option = {true, false, false};
    option.localOnly = true;
    DBStatus openStatus = DBStatus::DB_ERROR;
    KvStoreNbDelegate *localDelegate = nullptr;
    mgr.GetKvStore(STORE_ID_1, option, [&openStatus, &localDelegate](DBStatus status, KvStoreNbDelegate *delegate) {
        openStatus = status;
        localDelegate = delegate;
    });
    ASSERT_TRUE(localDelegate != nullptr);
    EXPECT_EQ(openStatus, OK);
    /**
     * @tc.steps:step2. Create database without localOnly.
     * @tc.expected: step2. Returns a null store.
     */
    option.localOnly = false;
    KvStoreNbDelegate *syncDelegate = nullptr;
    mgr.GetKvStore(STORE_ID_1, option, [&openStatus, &syncDelegate](DBStatus status, KvStoreNbDelegate *delegate) {
        openStatus = status;
        syncDelegate = delegate;
    });
    EXPECT_EQ(syncDelegate, nullptr);
    EXPECT_EQ(openStatus, INVALID_ARGS);
    EXPECT_EQ(mgr.CloseKvStore(localDelegate), OK);
    EXPECT_EQ(mgr.DeleteKvStore(STORE_ID_1), OK);
    localDelegate = nullptr;
}

/**
  * @tc.name: PutSync001
  * @tc.desc: put data and sync at same time
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, PutSync001, TestSize.Level3)
{
    /**
     * @tc.steps:step1. Create database with localOnly.
     * @tc.expected: step1. Returns a non-null store.
     */
    KvStoreDelegateManager mgr(APP_ID, USER_ID);
    mgr.SetKvStoreConfig(g_config);
    const KvStoreNbDelegate::Option option = {true, false, false};
    mgr.GetKvStore(STORE_ID_1, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    /**
     * @tc.steps:step2. Put data async.
     * @tc.expected: step2. Always returns OK.
     */
    std::atomic<bool> finish = false;
    std::thread putThread([&finish]() {
        while (!finish) {
            EXPECT_EQ(g_kvNbDelegatePtr->Put(KEY_1, VALUE_1), OK);
        }
    });
    /**
     * @tc.steps:step3. Call sync async.
     * @tc.expected: step3. Always returns OK.
     */
    std::thread syncThread([]() {
        std::vector<std::string> devices;
        devices.emplace_back("");
        Key key = {'k'};
        for (int i = 0; i < 100; ++i) { // sync 100 times
            Query query = Query::Select().PrefixKey(key);
            DBStatus status = g_kvNbDelegatePtr->Sync(devices, SYNC_MODE_PULL_ONLY, nullptr, query, true);
            EXPECT_EQ(status, OK);
        }
    });
    syncThread.join();
    finish = true;
    putThread.join();
    EXPECT_EQ(mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(mgr.DeleteKvStore(STORE_ID_1), OK);
}

/**
  * @tc.name: UpdateKey001
  * @tc.desc: Test update key
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, UpdateKey001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create database.
     * @tc.expected: step1. Returns a non-null kvstore.
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore("UpdateKey001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    /**
     * @tc.steps:step2. Put (k1, v1) into the database.
     * @tc.expected: step2. Returns OK.
     */
    Key k1 = {'k', '1'};
    EXPECT_EQ(g_kvNbDelegatePtr->Put(k1, VALUE_1), OK);
    /**
     * @tc.steps:step3. Update (k1, v1) to (k10, v1).
     * @tc.expected: step3. Returns OK and get k1 return not found.
     */
    g_kvNbDelegatePtr->UpdateKey([](const Key &originKey, Key &newKey) {
        newKey = originKey;
        newKey.push_back('0');
    });
    Value actualValue;
    EXPECT_EQ(g_kvNbDelegatePtr->Get(k1, actualValue), NOT_FOUND);
    k1.push_back('0');
    EXPECT_EQ(g_kvNbDelegatePtr->Get(k1, actualValue), OK);
    EXPECT_EQ(actualValue, VALUE_1);
    /**
     * @tc.steps:step4. Close store.
     * @tc.expected: step4. Returns OK.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("UpdateKey001"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: UpdateKey002
  * @tc.desc: Test update key with transaction
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, UpdateKey002, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create database.
     * @tc.expected: step1. Returns a non-null kvstore.
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore("UpdateKey002", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    /**
     * @tc.steps:step2. Put (k1, v1) into the database .
     * @tc.expected: step2. Returns OK.
     */
    Key k1 = {'k', '1'};
    EXPECT_EQ(g_kvNbDelegatePtr->Put(k1, VALUE_1), OK);
    g_kvNbDelegatePtr->StartTransaction();
    /**
     * @tc.steps:step3. Update (k1, v1) to (k10, v1).
     * @tc.expected: step3. Returns OK and get k1 return not found.
     */
    g_kvNbDelegatePtr->UpdateKey([](const Key &originKey, Key &newKey) {
        newKey = originKey;
        newKey.push_back('0');
    });
    Value actualValue;
    EXPECT_EQ(g_kvNbDelegatePtr->Get(k1, actualValue), NOT_FOUND);
    Key k10 = {'k', '1', '0'};
    EXPECT_EQ(g_kvNbDelegatePtr->Get(k10, actualValue), OK);
    EXPECT_EQ(actualValue, VALUE_1);
    /**
     * @tc.steps:step5. Rollback Transaction.
     * @tc.expected: step5. k10 not exist in db.
     */
    g_kvNbDelegatePtr->Rollback();
    EXPECT_EQ(g_kvNbDelegatePtr->Get(k10, actualValue), NOT_FOUND);
    EXPECT_EQ(g_kvNbDelegatePtr->Get(k1, actualValue), OK);
    /**
     * @tc.steps:step5. Commit transaction.
     * @tc.expected: step5. data exist in db.
     */
    g_kvNbDelegatePtr->StartTransaction();
    g_kvNbDelegatePtr->UpdateKey([](const Key &originKey, Key &newKey) {
        newKey = originKey;
        newKey.push_back('0');
    });
    g_kvNbDelegatePtr->Commit();
    EXPECT_EQ(g_kvNbDelegatePtr->Get(k10, actualValue), OK);
    /**
     * @tc.steps:step6. Close store.
     * @tc.expected: step6. Returns OK.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("UpdateKey002"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: UpdateKey003
  * @tc.desc: Test update key with invalid args
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, UpdateKey003, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create database.
     * @tc.expected: step1. Returns a non-null kvstore.
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore("UpdateKey003", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    /**
     * @tc.steps:step2. Put (k1, v1) into the database .
     * @tc.expected: step2. Returns OK.
     */
    Key k1 = {'k', '1'};
    Key k2 = {'k', '2'};
    EXPECT_EQ(g_kvNbDelegatePtr->Put(k1, VALUE_1), OK);
    EXPECT_EQ(g_kvNbDelegatePtr->Put(k2, VALUE_1), OK);
    /**
     * @tc.steps:step3. Update key with nullptr or invalid key.
     * @tc.expected: step3. Returns INVALID_ARGS.
     */
    EXPECT_EQ(g_kvNbDelegatePtr->UpdateKey(nullptr), INVALID_ARGS);
    DBStatus status = g_kvNbDelegatePtr->UpdateKey([](const Key &originKey, Key &newKey) {
        newKey.clear();
    });
    EXPECT_EQ(status, INVALID_ARGS);
    status = g_kvNbDelegatePtr->UpdateKey([](const Key &originKey, Key &newKey) {
        newKey.assign(2048u, '0'); // 2048 is invalid len
    });
    EXPECT_EQ(status, INVALID_ARGS);
    status = g_kvNbDelegatePtr->UpdateKey([](const Key &originKey, Key &newKey) {
        newKey = {'k', '3'};
    });
    EXPECT_EQ(status, CONSTRAINT);
    /**
     * @tc.steps:step4. Close store.
     * @tc.expected: step4. Returns OK.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("UpdateKey003"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: BlockTimer001
  * @tc.desc: Test open close function with block timer
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, BlockTimer001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Create database.
     * @tc.expected: step1. Returns a non-null store.
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore("BlockTimer001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    /**
     * @tc.steps:step2. Create block timer.
     * @tc.expected: step2. create ok.
     */
    TimerId timerId = 0u;
    bool timerFinalize = false;
    std::condition_variable cv;
    std::mutex finalizeMutex;
    bool triggerTimer = false;
    std::condition_variable triggerCv;
    std::mutex triggerMutex;
    int errCode = RuntimeContext::GetInstance()->SetTimer(1, [&triggerTimer, &triggerCv, &triggerMutex](TimerId id) {
        {
            std::lock_guard<std::mutex> autoLock(triggerMutex);
            triggerTimer = true;
        }
        triggerCv.notify_all();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return -E_END_TIMER;
    }, [&timerFinalize, &finalizeMutex, &cv]() {
        {
            std::lock_guard<std::mutex> autoLock(finalizeMutex);
            timerFinalize = true;
        }
        cv.notify_all();
    }, timerId);
    ASSERT_EQ(errCode, E_OK);
    {
        std::unique_lock<std::mutex> uniqueLock(triggerMutex);
        triggerCv.wait(uniqueLock, [&triggerTimer]() {
            return triggerTimer;
        });
    }
    /**
     * @tc.steps:step3. Close store.
     * @tc.expected: step3. Returns OK.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    std::unique_lock<std::mutex> uniqueLock(finalizeMutex);
    EXPECT_TRUE(timerFinalize);
    cv.wait(uniqueLock, [&timerFinalize]() {
        return timerFinalize;
    });
    EXPECT_EQ(g_mgr.DeleteKvStore("BlockTimer001"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: MigrateDeadLockTest0011
  * @tc.desc: Test the will not be deadlock in migration.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, MigrateDeadLockTest001, TestSize.Level2)
{
    std::shared_ptr<ProcessSystemApiAdapterImpl> g_adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    /**
     * @tc.steps:step1. Get the nb delegate.
     * @tc.expected: step1. Get results OK and non-null delegate.
     */
    KvStoreNbDelegate::Option option = {true, false, false};
    option.secOption = {S3, SECE};
    std::string storeId = "distributed_nb_delegate_test";
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);

    KvDBProperties property;
    property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    property.SetStringProp(KvDBProperties::STORE_ID, storeId);
    property.SetIntProp(KvDBProperties::SECURITY_LABEL, S3);
    property.SetIntProp(KvDBProperties::SECURITY_FLAG, SECE);

    std::string identifier = DBCommon::GenerateIdentifierId(storeId, APP_ID, USER_ID);
    property.SetStringProp(KvDBProperties::IDENTIFIER_DATA, DBCommon::TransferHashString(identifier));
    property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);

    int errCode;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);

    /**
     * @tc.steps:step2. create cache db
     * @tc.expected: step2. operation ok
     */
    std::string cacheDir =  g_testDir + "/" + DBCommon::TransferStringToHex(DBCommon::TransferHashString(identifier)) +
        "/" + DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::CACHEDB_DIR;
    std::string cacheDB = cacheDir + "/" + DBConstant::SINGLE_VER_CACHE_STORE + DBConstant::DB_EXTENSION;
    EXPECT_EQ(OS::CreateFileByFileName(cacheDB), E_OK);

    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);

    std::this_thread::sleep_for(std::chrono::seconds(3)); // 3 is sleep seconds
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
    g_kvNbDelegatePtr = nullptr;
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

/**
  * @tc.name: InvalidQueryTest001
  * @tc.desc: Test GetEntries with range query filter by sqlite
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: mazhao
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, InvalidQueryTest001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Get the nb delegate.
     * @tc.expected: step1. Get results OK and non-null delegate.
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore("InvalidQueryTest001", option, g_kvNbDelegateCallback);
    std::vector<Entry> entries;
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);

    /**
     * @tc.steps: step2. Use range query conditions to obtain the resultset when use sqlite engine.
     * @tc.expected: step2. return NOT_SUPPORT.
     */
    KvStoreResultSet *resultSet = nullptr;
    Query inValidQuery = Query::Select().Range({}, {});
    EXPECT_EQ(g_kvNbDelegatePtr->GetEntries(inValidQuery, resultSet), NOT_SUPPORT);
    EXPECT_EQ(g_kvNbDelegatePtr->GetEntries(inValidQuery, entries), NOT_SUPPORT);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("InvalidQueryTest001"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: InvalidQueryTest002
  * @tc.desc: Test GetEntries with range query filter by sqlite while conn is nullptr.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caihaoting
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, InvalidQueryTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. initialize result set.
     * @tc.expected: step1. Success.
     */
    KvStoreNbDelegate::Option option = {true, false, false};
    g_mgr.GetKvStore("InvalidQueryTest002", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    InitResultSet();

    /**
     * @tc.steps: step2. get entries using result set while conn is nullptr.
     * @tc.expected: step2. DB_ERROR.
     */
    KvStoreResultSet *readResultSet = nullptr;
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(g_kvNbDelegatePtr);
    EXPECT_EQ(kvStoreImpl->Close(), OK);
    EXPECT_EQ(g_kvNbDelegatePtr->GetEntries(g_keyPrefix, readResultSet), DB_ERROR);
    ASSERT_TRUE(readResultSet == nullptr);

    std::vector<Entry> entries;
    Query query = Query::Select().PrefixKey({'a', 'c'});
    EXPECT_EQ(g_kvNbDelegatePtr->GetEntries(query, entries), DB_ERROR);
    EXPECT_EQ(entries.size(), 0UL);

    EXPECT_EQ(g_kvNbDelegatePtr->GetEntries(query, readResultSet), DB_ERROR);
    ASSERT_TRUE(readResultSet == nullptr);

    int count = -1;
    EXPECT_EQ(g_kvNbDelegatePtr->GetCount(query, count), DB_ERROR);
    EXPECT_EQ(count, -1);

    /**
     * @tc.steps: step3. close kvStore.
     * @tc.expected: step3. Success.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("InvalidQueryTest002"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: SyncRangeQuery001
  * @tc.desc: test sync query with range
  * @tc.type: FUNC
  * @tc.require: DTS2023112110763
  * @tc.author: mazhao
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, SyncRangeQuery001, TestSize.Level3)
{
    /**
     * @tc.steps:step1. Create database with localOnly.
     * @tc.expected: step1. Returns a non-null store.
     */
    InitVirtualDevice(DEVICE_B, g_deviceB, g_syncInterfaceB);
    KvStoreDelegateManager mgr(APP_ID, USER_ID);
    mgr.SetKvStoreConfig(g_config);
    const KvStoreNbDelegate::Option option = {true, false, false};
    mgr.GetKvStore(STORE_ID_1, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    /**
     * @tc.steps:step2. Construct invalid query with range, Call sync async.
     * @tc.expected: step2. returns NOT_SUPPORT.
     */
    std::vector<std::string> devices;
    devices.emplace_back(DEVICE_B);
    Query inValidQuery = Query::Select().Range({}, {});
    DBStatus status = g_kvNbDelegatePtr->Sync(devices, SYNC_MODE_PULL_ONLY, nullptr, inValidQuery, true);
    EXPECT_EQ(status, NOT_SUPPORT);
    EXPECT_EQ(mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(mgr.DeleteKvStore(STORE_ID_1), OK);
}

#ifndef USE_RD_KERNEL
/**
  * @tc.name: InvalidOption001
  * @tc.desc: Test get kv store use invalid options info func with rd, need execute in manual.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhujinlin
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, InvalidOption001, TestSize.Level3)
{
    /**
     * @tc.steps:step1. Get the nb delegate.
     * @tc.expected: step1. Get results OK and non-null delegate.
     */
    KvStoreNbDelegate::Option option = {true, false, false};
    option.storageEngineType = GAUSSDB_RD;
    option.rdconfig.pageSize = 64u;
    option.rdconfig.cacheSize = 4u * 1024u * 1024u;
    option.rdconfig.type = HASH;
    g_mgr.GetKvStore("InvalidOption001", option, g_kvNbDelegateCallback);
    ASSERT_EQ(g_kvNbDelegatePtr, nullptr);
    EXPECT_EQ(g_kvDelegateStatus, INVALID_ARGS);
    /**
     * @tc.steps:step2. Get the nb delegate.
     * @tc.expected: step2. Get results OK and non-null delegate.
     */
    option.rdconfig.cacheSize = (4u * 1024u * 1024u) - 64u;
    g_mgr.GetKvStore("InvalidOption001", option, g_kvNbDelegateCallback);
    ASSERT_NE(g_kvNbDelegatePtr, nullptr);
    EXPECT_EQ(g_kvDelegateStatus, OK);
    /**
     * @tc.steps:step3. Close and delete KV store
     * @tc.expected: step3. Returns OK.
     */
    g_mgr.CloseKvStore(g_kvNbDelegatePtr);
    EXPECT_EQ(g_mgr.DeleteKvStore("InvalidOption001"), OK);
    g_kvNbDelegatePtr = nullptr;
}
#endif

/**
  * @tc.name: OptionValidCheck001
  * @tc.desc: test validation of option mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, OptionModeValidCheck001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Get the nb delegate.
     * @tc.expected: step1. Get results OK and non-null delegate.
     */
    KvStoreNbDelegate::Option option = {true, false, false};
    KvStoreObserverUnitTest *observer = new KvStoreObserverUnitTest();
    ASSERT_TRUE(observer != nullptr);
    option.observer = observer;
    std::vector<int> invalidModeVec = {0, 5, 6, 7, 9, 16};
    std::string storeId = "OptionModeValidCheck001";
    for (size_t i = 0; i < invalidModeVec.size(); i++) {
        option.mode = invalidModeVec.at(i);
        g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
        EXPECT_EQ(g_kvDelegateStatus, INVALID_ARGS);
    }

    std::vector<int> validModeVec = {1, 2, 3, 4, 8};
    for (size_t i = 0; i < validModeVec.size(); i++) {
        option.mode = validModeVec.at(i);
        g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
        ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
        EXPECT_EQ(g_kvDelegateStatus, OK);
        EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
        g_kvNbDelegatePtr = nullptr;
    }

    delete observer;
}

/**
  * @tc.name: AbnormalKvStoreTest001
  * @tc.desc: Test KvStoreNbDelegateImpl interface while conn is nullptr.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: suyue
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, AbnormalKvStoreTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. GetKvStore for initialize g_kvNbDelegatePtr.
     * @tc.expected: step1. Success.
     */
    KvStoreNbDelegate::Option option = {true, false, false};
    g_mgr.GetKvStore("AbnormalKvStoreTest001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    InitResultSet();

    /**
     * @tc.steps: step2. test KvStoreNbDelegateImpl interface while conn is nullptr.
     * @tc.expected: step2. return DB_ERROR.
     */
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(g_kvNbDelegatePtr);
    EXPECT_EQ(kvStoreImpl->Close(), OK);

    const Key key = {0};
    EXPECT_EQ(kvStoreImpl->PublishLocal(key, true, true, nullptr), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->UnpublishToLocal(key, true, true), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->UnpublishToLocal({}, true, true), INVALID_ARGS);
    EXPECT_EQ(kvStoreImpl->RemoveDeviceData(""), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->CancelSync(0), DB_ERROR);
    bool autoSync = true;
    PragmaData data = static_cast<PragmaData>(&autoSync);
    EXPECT_EQ(kvStoreImpl->Pragma(AUTO_SYNC, data), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->SetConflictNotifier(0, nullptr), DB_ERROR);
    CipherPassword password;
    EXPECT_EQ(kvStoreImpl->Rekey(password), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->Export("", password, true), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->Import("", password), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->StartTransaction(), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->Commit(), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->Rollback(), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->CheckIntegrity(), DB_ERROR);
    SecurityOption securityOption;
    EXPECT_EQ(kvStoreImpl->GetSecurityOption(securityOption), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->SetRemotePushFinishedNotify(nullptr), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->SetEqualIdentifier("", {}), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->SetPushDataInterceptor(nullptr), DB_ERROR);

    /**
     * @tc.steps: step3. close kvStore.
     * @tc.expected: step3. Success.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("AbnormalKvStoreTest001"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: AbnormalKvStoreTest002
  * @tc.desc: Test KvStoreNbDelegateImpl interface while conn is nullptr.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: suyue
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, AbnormalKvStoreTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. GetKvStore for initialize g_kvNbDelegatePtr.
     * @tc.expected: step1. Success.
     */
    KvStoreNbDelegate::Option option = {true, false, false};
    g_mgr.GetKvStore("AbnormalKvStoreTest002", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    InitResultSet();

    /**
     * @tc.steps: step2. test KvStoreNbDelegateImpl interface while conn is nullptr.
     * @tc.expected: step2. return DB_ERROR.
     */
    auto kvStoreImpl = static_cast<KvStoreNbDelegateImpl *>(g_kvNbDelegatePtr);
    EXPECT_EQ(kvStoreImpl->Close(), OK);

    Query query;
    EXPECT_EQ(kvStoreImpl->SubscribeRemoteQuery({}, nullptr, query, true), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->UnSubscribeRemoteQuery({}, nullptr, query, true), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->RemoveDeviceData(), DB_ERROR);
    const Key key = {0};
    std::vector<Key> keys;
    EXPECT_EQ(kvStoreImpl->GetKeys(key, keys), DB_ERROR);
    uint32_t expectedVal = 0;
    EXPECT_EQ(kvStoreImpl->GetSyncDataSize(""), expectedVal);
    EXPECT_EQ(kvStoreImpl->UpdateKey(nullptr), DB_ERROR);
    const std::string device = "test";
    std::pair<DBStatus, WatermarkInfo> info = kvStoreImpl->GetWatermarkInfo(device);
    EXPECT_EQ(info.first, DB_ERROR);
    EXPECT_EQ(kvStoreImpl->GetTaskCount(), DB_ERROR);
    EXPECT_EQ(kvStoreImpl->SetReceiveDataInterceptor(nullptr), DB_ERROR);
    CloudSyncConfig config;
    EXPECT_EQ(kvStoreImpl->SetCloudSyncConfig(config), DB_ERROR);
    const IOption iOption;
    std::vector<Entry> entries;
    EXPECT_EQ(kvStoreImpl->GetEntries(key, entries), DB_ERROR);

    /**
     * @tc.steps: step3. close kvStore.
     * @tc.expected: step3. Success.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.DeleteKvStore("AbnormalKvStoreTest002"), OK);
    g_kvNbDelegatePtr = nullptr;
}

/**
  * @tc.name: AbnormalKvStoreResultSetTest
  * @tc.desc: Test KvStoreResultSetImpl interface when class para is nullptr.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: suyue
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, AbnormalKvStoreResultSetTest, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Call interfaces when class para is null.
     * @tc.expected: step1. return failInfo.
     */
    KvStoreResultSetImpl kvStoreObj(nullptr);
    EXPECT_EQ(kvStoreObj.GetCount(), 0);
    EXPECT_EQ(kvStoreObj.GetPosition(), INIT_POSITION);
    EXPECT_EQ(kvStoreObj.Move(0), false);
    EXPECT_EQ(kvStoreObj.MoveToPosition(0), false);
    EXPECT_EQ(kvStoreObj.MoveToFirst(), false);
    EXPECT_EQ(kvStoreObj.MoveToLast(), false);
    EXPECT_EQ(kvStoreObj.IsFirst(), false);
    EXPECT_EQ(kvStoreObj.IsLast(), false);
    EXPECT_EQ(kvStoreObj.IsBeforeFirst(), false);
    EXPECT_EQ(kvStoreObj.IsAfterLast(), false);
    std::vector<std::string> columnNames;
    kvStoreObj.GetColumnNames(columnNames);
    Entry entry;
    EXPECT_EQ(kvStoreObj.GetEntry(entry), DB_ERROR);
    EXPECT_EQ(kvStoreObj.IsClosed(), false);
    kvStoreObj.Close();

    /**
     * @tc.steps: step2. Call unsupported interfaces.
     * @tc.expected: step2. return NOT_SUPPORT.
     */
    std::string columnName;
    int columnIndex = 0;
    EXPECT_EQ(kvStoreObj.GetColumnIndex(columnName, columnIndex), NOT_SUPPORT);
    EXPECT_EQ(kvStoreObj.GetColumnName(columnIndex, columnName), NOT_SUPPORT);
    std::vector<uint8_t> vecVal;
    EXPECT_EQ(kvStoreObj.Get(columnIndex, vecVal), NOT_SUPPORT);
    std::string strVal;
    EXPECT_EQ(kvStoreObj.Get(columnIndex, strVal), NOT_SUPPORT);
    int64_t intVal;
    EXPECT_EQ(kvStoreObj.Get(columnIndex, intVal), NOT_SUPPORT);
    double doubleVal;
    EXPECT_EQ(kvStoreObj.Get(columnIndex, doubleVal), NOT_SUPPORT);
    bool isNull;
    EXPECT_EQ(kvStoreObj.IsColumnNull(columnIndex, isNull), NOT_SUPPORT);
    std::map<std::string, VariantData> data;
    EXPECT_EQ(kvStoreObj.GetRow(data), NOT_SUPPORT);
}

/**
  * @tc.name: AbnormalKvStoreTest003
  * @tc.desc: Test SqliteCloudKvStore interface when para is invalid.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: suyue
  */
HWTEST_F(DistributedDBInterfacesNBDelegateExtendTest, AbnormalKvStoreTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Call defaule interfaces.
     * @tc.expected: step1. return E_OK.
     */
    SqliteCloudKvStore kvStoreObj(nullptr);
    DataBaseSchema schema;
    EXPECT_EQ(kvStoreObj.SetCloudDbSchema(schema), E_OK);
    EXPECT_EQ(kvStoreObj.Commit(), E_OK);
    EXPECT_EQ(kvStoreObj.Rollback(), E_OK);
    const TableName tableName = "test";
    VBucket vBucket;
    EXPECT_EQ(kvStoreObj.FillCloudAssetForDownload(tableName, vBucket, true), E_OK);
    EXPECT_EQ(kvStoreObj.SetLogTriggerStatus(true), E_OK);
    QuerySyncObject query;
    EXPECT_EQ(kvStoreObj.CheckQueryValid(query), E_OK);
    ContinueToken continueStmtToken = nullptr;
    EXPECT_EQ(kvStoreObj.ReleaseCloudDataToken(continueStmtToken), E_OK);
    std::vector<QuerySyncObject> syncQuery;
    std::vector<std::string> users;
    EXPECT_EQ(kvStoreObj.GetCompensatedSyncQuery(syncQuery, users), E_OK);

    /**
     * @tc.steps: step2. Call interfaces when class para is null.
     * @tc.expected: step2. return failInfo.
     */
    DataInfoWithLog log;
    EXPECT_EQ(kvStoreObj.GetInfoByPrimaryKeyOrGid(tableName, vBucket, log, vBucket), -E_INTERNAL_ERROR);
    DownloadData downloadData;
    EXPECT_EQ(kvStoreObj.PutCloudSyncData(tableName, downloadData), -E_INTERNAL_ERROR);
    Timestamp timestamp = 0;
    int64_t count = 0;
    EXPECT_EQ(kvStoreObj.GetUploadCount(query, timestamp, true, true, count), -E_INTERNAL_ERROR);
    std::vector<Timestamp> timestampVec;
    EXPECT_EQ(kvStoreObj.GetAllUploadCount(query, timestampVec, true, true, count), -E_INTERNAL_ERROR);

    /**
     * @tc.steps: step3. Get and set Schema with different para when class para is null.
     * @tc.expected: step3. return failInfo.
     */
    TableSchema tableSchema;
    EXPECT_EQ(kvStoreObj.GetCloudTableSchema(tableName, tableSchema), -E_NOT_FOUND);
    CloudSyncData cloudDataResult;
    EXPECT_EQ(kvStoreObj.GetCloudDataNext(continueStmtToken, cloudDataResult), -E_INVALID_ARGS);
    std::map<std::string, DataBaseSchema> schemaMap;
    EXPECT_EQ(kvStoreObj.SetCloudDbSchema(schemaMap), -E_INVALID_SCHEMA);
    schema.tables = {tableSchema, tableSchema};
    schemaMap.insert(std::pair<std::string, DataBaseSchema>(tableName, schema));
    EXPECT_EQ(kvStoreObj.SetCloudDbSchema(schemaMap), -E_INVALID_SCHEMA);
    const std::string user = "user1";
    kvStoreObj.SetUser(user);
    EXPECT_EQ(kvStoreObj.GetCloudTableSchema(tableName, tableSchema), -E_SCHEMA_MISMATCH);
}
}
