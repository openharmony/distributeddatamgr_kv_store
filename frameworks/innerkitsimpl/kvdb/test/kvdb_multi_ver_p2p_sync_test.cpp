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
#ifndef OMIT_MULTI_VER
#include <gtest/gtest.h>
#include <thread>

#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "ikvdb_connection.h"
#include "kv_store_delegate.h"
#include "kvdb_manager.h"
#include "kvdb_pragma.h"
#include "log_print.h"
#include "meta_data.h"
#include "multi_ver_data_sync.h"
#include "platform_specific.h"
#include "sync_types.h"
#include "time_sync.h"
#include "virtual_multi_ver_sync_db_interface.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace KvDBUnitTest;
using namespace std;

#ifndef LOW_LEVEL_MEM_DEV
namespace {
    string g_testDir;
    const string STORE_ID = "kv_store_sync_test";
    const string STORE_ID_A = "kv_store_sync_test_a";
    const string STORE_ID_B = "kv_store_sync_test_b";
    const int WAIT_TIME_1 = 100;
    const int WAIT_TIME_2 = 300;
    const int WAIT_LONG_TIME = 200;
    const int WAIT_LIMIT_TIME = 400;
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";
    const int LIMIT_KEY_SIZE = 2048;
    constexpr int BIG_VALUE_SIZE = 2048 + 1; // > 1K
    constexpr int LIMIT_VALUE_SIZE = 4 * 2048 * 2048; // 4M
    KvStoreDelegateManager g_mgr("sync_test", "sync_test");
    KvStoreConfig g_config;
    KvStoreDelegate::Option g_option;

    // define the g_kvDelegateCallback, used to get some information when open a kv store.
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreDelegate *g_kvPtr = nullptr;
    KvStoreConnection *g_connectionA;
    KvStoreConnection *g_connectionC;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    KvVirtualDevice *g_deviceB = nullptr;
    KvVirtualDevice *g_deviceC = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = bind(&KvDBToolsUnitTest::KvStoreDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvPtr));

    KvStoreConnection *GetConnection(const std::string &dir, const std::string &storeId, int errCode)
    {
        KvDBProperties properties;
        properties.SetStringProp(KvDBProperties::USER_ID, "sync_test");
        properties.SetStringProp(KvDBProperties::APP_ID, "sync_test");
        properties.SetStringProp(KvDBProperties::STORE_ID, storeId);
        std::string identifier = DBCommon::TransferHashString("sync_test-sync_test-" + storeId);

        properties.SetStringProp(KvDBProperties::IDENTIFIER_DATA, identifier);
        std::string identifierDir = DBCommon::TransferStringToHex(identifier);
        properties.SetStringProp(KvDBProperties::IDENTIFIER_DIR, identifierDir);
        properties.SetStringProp(KvDBProperties::DATA_DIR, dir);
        properties.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::MULTI_VER_TYPE_SQLITE);
        properties.SetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, true);
        errCode = E_OK;
        auto conn = KvDBManager::GetDatabaseConnection(properties, errCode);
        if (errCode != E_OK) {
            LOGE("[KvdbMultiVerP2PSyncTes] db create failed path, err %d", errCode);
            return nullptr;
        }
        return static_cast<KvStoreConnection *>(conn);
    }

    int GetData(IKvDBConnection *conn, const Key &key, Value &value)
    {
        IKvDBSnapshot *snap = nullptr;
        int errCode = conn->GetSnapshot(snap);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = snap->Get(key, value);
        conn->ReleaseSnapshot(snap);
        return errCode;
    }
}

class KvDBMultiVerP2PSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBMultiVerP2PSyncTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    KvDBToolsUnitTest::TestDirInit(g_testDir);
    string dir = g_testDir + "/commitstore";
    g_config.dataDir = dir;
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }
    g_mgr.SetKvStoreConfig(g_config);
    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    EXCEPT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void KvDBMultiVerP2PSyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (KvDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }

    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
    g_communicatorAggregator = nullptr;
}

void KvDBMultiVerP2PSyncTest::SetUp(void)
{
    KvDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B and C
     */
    g_communicatorAggregator->Disable();
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    EXCEPT_TRUE(g_deviceB != nullptr);
    VirtualMultiVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualMultiVerSyncDBInterface;
    EXCEPT_TRUE(syncInterfaceB != nullptr);
    EXCEPT_EQ(syncInterfaceB->Initialize(DEVICE_B), E_OK);
    EXCEPT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);

    g_deviceC = new (std::nothrow) KvVirtualDevice(DEVICE_C);
    EXCEPT_TRUE(g_deviceC != nullptr);
    VirtualMultiVerSyncDBInterface *syncInterface = new (std::nothrow) VirtualMultiVerSyncDBInterface;
    EXCEPT_TRUE(syncInterface != nullptr);
    EXCEPT_EQ(syncInterface->Initialize(DEVICE_C), E_OK);
    EXCEPT_EQ(g_deviceC->Initialize(g_communicatorAggregator, syncInterface), E_OK);
    g_communicatorAggregator->Enable();

    auto permissionCheckTestCallback = [] (const std::string &userId, const std::string &appId,
                                const std::string &storeId, const std::string &deviceId,
                                uint8_t flag) -> bool {
                                return true;
                                };
    EXPECT_EQ(g_mgr.SetPermissionCheckTestCallback(permissionCheckTestCallback), OK);
}

void KvDBMultiVerP2PSyncTest::TearDown(void)
{
    /**
     * @tc.teardown: Release device A, B, C, connectionA and connectionB
     */
    if (g_kvPtr != nullptr) {
        EXCEPT_EQ(g_mgr.CloseKvStore(g_kvPtr), OK);
        g_kvPtr = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(STORE_ID);
        LOGD("delete kv store status %d", status);
        EXCEPT_TRUE(status == OK);
    }
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    if (g_deviceC != nullptr) {
        delete g_deviceC;
        g_deviceC = nullptr;
    }
    if (g_connectionA != nullptr) {
        g_connectionA->Close();
        EXCEPT_EQ(g_mgr.DeleteKvStore(STORE_ID_A), OK);
        g_connectionA = nullptr;
    }
    if (g_connectionC != nullptr) {
        g_connectionC->Close();
        EXCEPT_EQ(g_mgr.DeleteKvStore(STORE_ID_B), OK);
        g_connectionC = nullptr;
    }
    PermissionCheckTestCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckTestCallback(nullCallback), OK);
}

static DBStatus GetData(KvStoreDelegate *kvStore, const Key &key, Value &value)
{
    KvStoreSnapshotDelegate *snapShot = nullptr;
    DBStatus statusTmp;
    kvStore->GetKvStoreSnapshot(nullptr,
        [&statusTmp, &snapShot](DBStatus status, KvStoreSnapshotDelegate *snap) {
        statusTmp = status;
        snapShot = snap;
        });
    if (statusTmp != E_OK) {
        return statusTmp;
    }
    snapShot->Get(key, [&statusTmp, &value](DBStatus status, const Value &outValue) {
        statusTmp = status;
        value = outValue;
    });
    if (statusTmp == OK) {
        LOGD("[KvdbMultiVerP2PSyncTes] GetData key %c, value = %c", key[0], value[0]);
    }
    kvStore->ReleaseKvStoreSnapshot(snapShot);
    return statusTmp;
}

/**
 * @tc.name: Transaction Sync 001
 * @tc.desc: Verify put transaction sync function.
 * @tc.type: FUNC
 * @tc.require: AR000BVRO4 AR000CQE0K
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, TransactionSyncTest001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    /**
     * @tc.steps: step2. deviceB put {k1, v1}, {k2,v2} in a transaction
     */
    g_deviceB->StartTransaction();
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_2, KvDBUnitTest::VALUE_2), E_OK);
    g_deviceB->Commit();

    /**
     * @tc.steps: step3. deviceB online and wait for sync
     */
    g_deviceB->Online();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    /**
     * @tc.steps: step4. deviceC put {k3, v3}, {k4,v4} in a transaction
     */
    g_deviceC->StartTransaction();
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_3, KvDBUnitTest::VALUE_3), E_OK);
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_4, KvDBUnitTest::VALUE_4), E_OK);
    g_deviceC->Commit();

    /**
     * @tc.steps: step5. deviceC online for sync
     */
    g_deviceC->Online();

    /**
     * @tc.steps: step6. deviceC offline
     */
    g_deviceC->Offline();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    /**
     * @tc.expected: step6. deviceA have {k1, v1}, {k2, v2}, not have k3, k4
     */
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_1, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_1);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_2);

    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_3, value), NOT_FOUND);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_4, value), NOT_FOUND);
}

/**
 * @tc.name: Transaction Sync 002
 * @tc.desc: Verify delete transaction sync function.
 * @tc.type: FUNC
 * @tc.require: AR000BVRO4 AR000CQE0K
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, TransactionSyncTest002, TestSize.Level2)
{
    /**
     * @tc.steps: step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    /**
     * @tc.steps: step2. deviceB put {k1, v1}, {k2,v2} in a transaction
     */
    g_deviceB->StartTransaction();
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_2, KvDBUnitTest::VALUE_2), E_OK);
    g_deviceB->Commit();

    /**
     * @tc.steps: step3. deviceB online and wait for sync
     */
    g_deviceB->Online();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    /**
     * @tc.steps: step4. deviceC put {k3, v3}, and delete k3 in a transaction
     */
    g_deviceC->StartTransaction();
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_3, KvDBUnitTest::VALUE_3), E_OK);
    EXCEPT_EQ(g_deviceC->DeleteData(KvDBUnitTest::KEY_3), E_OK);
    g_deviceC->Commit();

    /**
     * @tc.steps: step5. deviceB online for sync
     */
    g_deviceC->Online();

    /**
     * @tc.steps: step6. deviceC offline
     */
    g_deviceC->Offline();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    /**
     * @tc.expected: step6. deviceA have {k1, v1}, {k2, v2}, not have k3, k4
     */
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_1, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_1);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_2);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_3, value), NOT_FOUND);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_3, value), NOT_FOUND);
}

/**
 * @tc.name: Transaction Sync 003
 * @tc.desc: Verify update transaction sync function.
 * @tc.type: FUNC
 * @tc.require: AR000BVRO4 AR000CQE0K
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, TransactionSyncTest003, TestSize.Level2)
{
    /**
     * @tc.steps: step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    /**
     * @tc.steps: step2. deviceB put {k1, v1}, {k2,v2} in a transaction
     */
    g_deviceB->StartTransaction();
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_2, KvDBUnitTest::VALUE_2), E_OK);
    g_deviceB->Commit();

    /**
     * @tc.steps: step3. deviceB online and wait for sync
     */
    g_deviceB->Online();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    /**
     * @tc.steps: step4. deviceC put {k3, v3}, and update {k3, v4} in a transaction
     */
    g_deviceC->StartTransaction();
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_3, KvDBUnitTest::VALUE_3), E_OK);
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_3, KvDBUnitTest::VALUE_4), E_OK);
    g_deviceC->Commit();

    /**
     * @tc.steps: step5. deviceB online for sync
     */
    g_deviceC->Online();

    /**
     * @tc.steps: step6. deviceC offline
     */
    g_deviceC->Offline();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    /**
     * @tc.expected: step6. deviceA have {k1, v1}, {k2, v2}, not have k3, k4
     */
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_2);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_3, value), NOT_FOUND);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_3, value), NOT_FOUND);
}

/**
 * @tc.name: meta 001
 * @tc.desc: Verify meta add and update function
 * @tc.type: FUNC
 * @tc.require: AR000CQE0P AR000CQE0S
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, meta001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a meta and use VirtualMultiVerSyncDBInterface to init
     * @tc.expected: step1. meta init ok
     */
    meta meta;
    VirtualMultiVerSyncDBInterface *syncInterface = new (std::nothrow) VirtualMultiVerSyncDBInterface;
    EXCEPT_TRUE(syncInterface != nullptr);
    EXPECT_EQ(syncInterface->Initialize("meta_test"), E_OK);
    EXPECT_EQ(meta.Initialize(syncInterface), E_OK);

    /**
     * @tc.steps: step2. call SaveTimeOffset to write t1.
     * @tc.expected: step2. SaveTimeOffset return ok
     */
    const TimeOffset timeOffsetA = 1024;
    EXPECT_EQ(meta.SaveTimeOffset(DEVICE_B, timeOffsetA), E_OK);
    TimeOffset timeOffsetB = 0;

    /**
     * @tc.steps: step3. call GetTimeOffset to read t2.
     * @tc.expected: step3. t1 == t2
     */
    meta.GetTimeOffset(DEVICE_B, timeOffsetB);
    EXPECT_EQ(timeOffsetA, timeOffsetB);

    /**
     * @tc.steps: step4. call SaveTimeOffset to write t3. t3 != t1
     * @tc.expected: step4. SaveTimeOffset return ok
     */
    const TimeOffset timeOffsetC = 2048;
    EXPECT_EQ(meta.SaveTimeOffset(DEVICE_B, timeOffsetC), E_OK);

    /**
     * @tc.steps: step5. call GetTimeOffset to read t2.
     * @tc.expected: step5. t4 == t3
     */
    TimeOffset timeOffsetD = 0;
    meta.GetTimeOffset(DEVICE_B, timeOffsetD);
    EXPECT_EQ(timeOffsetC, timeOffsetD);
    syncInterface->DeleteDatabase();
    delete syncInterface;
    syncInterface = nullptr;
}

/**
 * @tc.name: Isolation Sync 001
 * @tc.desc: Verify add sync isolation between different kvstore.
 * @tc.type: FUNC
 * @tc.require: AR000BVDGP
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, IsolationSync001, TestSize.Level2)
{
    int errCode = 0;

    /**
     * @tc.steps: step1. Get connectionA, connectionB from different kvstore,
     *     connectionB not in g_communicatorAggregator
     */
    g_communicatorAggregator->Disable();
    g_connectionC = GetConnection(g_config.dataDir, STORE_ID_B, errCode);
    EXCEPT_TRUE(g_connectionC != nullptr);
    g_communicatorAggregator->Enable();
    g_connectionA = GetConnection(g_config.dataDir, STORE_ID_A, errCode);
    EXCEPT_TRUE(g_connectionA != nullptr);

    /**
     * @tc.steps: step2. deviceB put {k1, v1}
     */
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);

    /**
     * @tc.steps: step3. connectionA pull from deviceB
     * @tc.expected: step3. Pragma OK, connectionA have {k1, v1} , connectionB don't have k1.
     */
    PragmaSync pragmaData(devicesVec, SYNC_MODE_PULL_ONLY, nullptr);
    EXCEPT_TRUE(g_connectionA->Pragma(PRAGMA_SYNC_devicesVec, &pragmaData) == E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));
    Value value;
    EXCEPT_EQ(GetData(g_connectionA, KvDBUnitTest::KEY_1, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_1);
    EXPECT_EQ(GetData(g_connectionC, KvDBUnitTest::KEY_1, value), -E_NOT_FOUND);
}

/**
 * @tc.name: Isolation Sync 002
 * @tc.desc: Verify update sync isolation between different kvstore.
 * @tc.type: FUNC
 * @tc.require: AR000BVDGP
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, IsolationSync002, TestSize.Level2)
{
    int errCode = 0;

    /**
     * @tc.steps: step1. Get connectionA, connectionB from different kvstore,
     *     connectionB not in g_communicatorAggregator
     */
    g_communicatorAggregator->Disable();
    g_connectionC = GetConnection(g_config.dataDir, STORE_ID_B, errCode);
    EXCEPT_TRUE(g_connectionC != nullptr);
    g_communicatorAggregator->Enable();
    g_connectionA = GetConnection(g_config.dataDir, STORE_ID_A, errCode);
    EXCEPT_TRUE(g_connectionA != nullptr);

    /**
     * @tc.steps: step2. deviceB put {k1, v1} and update {k1, v2}
     */
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_2), E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    /**
     * @tc.steps: step3. connectionA pull from deviceB
     * @tc.expected: step3. Pragma OK, connectionA have {k1, v2} , connectionB don't have k1.
     */
    PragmaSync pragmaData(devicesVec, SYNC_MODE_PULL_ONLY, nullptr);
    EXCEPT_TRUE(g_connectionA->Pragma(PRAGMA_SYNC_devicesVec, &pragmaData) == E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    Value value;
    EXPECT_EQ(GetData(g_connectionA, KvDBUnitTest::KEY_1, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_2);
    EXPECT_EQ(GetData(g_connectionC, KvDBUnitTest::KEY_1, value), -E_NOT_FOUND);
}

/**
 * @tc.name: Isolation Sync 003
 * @tc.desc: Verify delete sync isolation between different kvstore.
 * @tc.type: FUNC
 * @tc.require: AR000BVDGP
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, IsolationSync003, TestSize.Level2)
{
    int errCode = 0;

    /**
     * @tc.steps: step1. Get connectionA, connectionB from different kvstore,
     *     connectionB not in g_communicatorAggregator, connectionB put {k1,v1}
     */
    g_communicatorAggregator->Disable();
    g_connectionC = GetConnection(g_config.dataDir, STORE_ID_B, errCode);
    EXCEPT_TRUE(g_connectionC != nullptr);
    IOption option;
    EXCEPT_EQ(g_connectionC->Put(option, KEY_1, VALUE_1), E_OK);
    g_communicatorAggregator->Enable();
    g_connectionA = GetConnection(g_config.dataDir, STORE_ID_A, errCode);
    EXCEPT_TRUE(g_connectionA != nullptr);

    /**
     * @tc.steps: step2. deviceB put {k1, v1} and delete k1
     */
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);
    EXCEPT_EQ(g_deviceB->DeleteData(KvDBUnitTest::KEY_1), E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    /**
     * @tc.steps: step3. connectionA pull from deviceB
     * @tc.expected: step3. Pragma OK, connectionA don't have k1, connectionB have {k1.v1}
     */
    LOGD("[KvdbMultiVerP2PSyncTes] start sync");
    PragmaSync pragmaData(devicesVec, SYNC_MODE_PULL_ONLY, nullptr);
    EXCEPT_TRUE(g_connectionA->Pragma(PRAGMA_SYNC_devicesVec, &pragmaData) == E_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));

    Value value;
    EXPECT_EQ(GetData(g_connectionA, KvDBUnitTest::KEY_1, value), -E_NOT_FOUND);
    EXPECT_EQ(GetData(g_connectionC, KvDBUnitTest::KEY_1, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_1);
}

static void SetTimeSyncPacketField(TimeSyncPacket &packet, Timestamp sourceBegin, Timestamp sourceEnd,
    Timestamp targetBegin, Timestamp targetEnd)
{
    packet.SetSourceTimeBegin(sourceBegin);
    packet.SetSourceTimeEnd(sourceEnd);
    packet.SetTargetTimeBegin(targetBegin);
    packet.SetTargetTimeEnd(targetEnd);
}

static bool IsTimeSyncPacketisEqual(const TimeSyncPacket &packetA, const TimeSyncPacket &packetC)
{
    bool isEqual = true;
    isEqual = packetA.GetSourceTimeBegin() == packetC.GetSourceTimeBegin() ? isEqual : false;
    isEqual = packetA.GetSourceTimeEnd() == packetC.GetSourceTimeEnd() ? isEqual : false;
    isEqual = packetA.GetTargetTimeBegin() == packetC.GetTargetTimeBegin() ? isEqual : false;
    isEqual = packetA.GetTargetTimeEnd() == packetC.GetTargetTimeEnd() ? isEqual : false;
    return isEqual;
}

/**
 * @tc.name: Timesync Packet 001
 * @tc.desc: Verify TimeSyncPacket Serialization and DeSerialization
 * @tc.type: FUNC
 * @tc.require: AR000BVRNU AR000CQE0J
 * @tc.author: xiaozhenjian
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, TimeSyncPacket001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create TimeSyncPacket packetA aand packetC
     */
    TimeSyncPacket packetA;
    TimeSyncPacket packetC;
    SetTimeSyncPacketField(packetA, 1, 2, 3, 4); // 1, 2, 3, 4 is four field for time sync packet
    SetTimeSyncPacketField(packetC, 5, 4, 3, 2); // 2, 3, 4, 5 is four field for time sync packet
    Message oriMsgA;
    Message oriMsgC;
    oriMsgA.SetCopiedObject(packetA);
    oriMsgA.SetMessageId(TIME_SYNC_MESSAGE);
    oriMsgA.SetMessageType(TYPE_REQUEST);
    oriMsgC.SetCopiedObject(packetC);
    oriMsgC.SetMessageId(TIME_SYNC_MESSAGE);
    oriMsgC.SetMessageType(TYPE_RESPONSE);

    /**
     * @tc.steps: step2. Serialization packetA to bufferA
     */
    uint32_t lenA = TimeSync::CalculateLen(&oriMsgA);
    vector<uint8_t> bufferA;
    bufferA.resize(lenA);
    int ret = TimeSync::Serialization(bufferA.data(), lenA, &oriMsgA);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step3. Serialization packetC to bufferC
     */
    uint32_t lenB = TimeSync::CalculateLen(&oriMsgC);
    vector<uint8_t> bufferC;
    bufferC.resize(lenB);
    ret = TimeSync::Serialization(bufferC.data(), lenB, &oriMsgC);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step4. DeSerialization bufferA to outPktA
     * @tc.expected: step4. packetA == outPktA
     */
    Message outMsg;
    outMsg.SetMessageId(TIME_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_REQUEST);
    ret = TimeSync::DeSerialization(bufferA.data(), lenA, &outMsg);
    EXCEPT_EQ(ret, E_OK);
    const TimeSyncPacket *outPktA = outMsg.GetObject<TimeSyncPacket>();
    ASSERT_NE(outPktA, nullptr);
    EXPECT_EQ(IsTimeSyncPacketisEqual(packetA, *outPktA), true);

    /**
     * @tc.steps: step5. DeSerialization bufferA to outPktA
     * @tc.expected: step5. packetC == outPktB  outPktB != outPktA
     */
    Message outMsgB;
    outMsgB.SetMessageId(TIME_SYNC_MESSAGE);
    outMsgB.SetMessageType(TYPE_RESPONSE);
    ret = TimeSync::DeSerialization(bufferC.data(), lenB, &outMsgB);
    EXCEPT_EQ(ret, E_OK);
    const TimeSyncPacket *outPktB = outMsgB.GetObject<TimeSyncPacket>();
    ASSERT_NE(outPktB, nullptr);
    EXPECT_EQ(IsTimeSyncPacketisEqual(packetC, *outPktB), true);
    EXPECT_EQ(IsTimeSyncPacketisEqual(*outPktA, *outPktB), false);
}

static MultiVerCommitNode MakeMultiVerCommitA()
{
    MultiVerCommitNode commit;
    commit.commitId = vector<uint8_t>(1, 11); // 1 is length, 11 is value
    commit.leftParent = vector<uint8_t>(2, 22); // 2 is length, 22 is value
    commit.rightParent = vector<uint8_t>(3, 33); // 3 is length, 33 is value
    commit.timestamp = 1; // 444 is value
    commit.version = 2; // 5555 is value
    commit.isLocal = 3; // 66666 is value
    commit.deviceInfo = "AAAAAA";
    return commit;
}

static MultiVerCommitNode MakeMultiVerCommitB()
{
    MultiVerCommitNode commit;
    commit.commitId = vector<uint8_t>(9, 99); // 9 is length, 99 is value
    commit.leftParent = vector<uint8_t>(8, 88); // 8 is length, 88 is value
    commit.rightParent = vector<uint8_t>(7, 77); // 7 is length, 77 is value
    commit.timestamp = 1; // 666 is value
    commit.version = 5; // 5555 is value
    commit.isLocal = 2; // 44444 is value
    commit.deviceInfo = "BBBBBB";
    return commit;
}

static MultiVerCommitNode MakeMultiVerCommitC()
{
    MultiVerCommitNode commit;
    commit.commitId = vector<uint8_t>(1, 99); // 1 is length, 99 is value
    commit.leftParent = vector<uint8_t>(2, 88); // 2 is length, 88 is value
    commit.rightParent = vector<uint8_t>(3, 77); // 3 is length, 77 is value
    commit.timestamp = 466; // 466 is value
    commit.version = 5555; // 5555 is value
    commit.isLocal = 66444; // 66444 is value
    commit.deviceInfo = "CCCCCC";
    return commit;
}

static bool IsMultiVerCommitisEqual(const MultiVerCommitNode &inCommitA, const MultiVerCommitNode &inCommitB)
{
    bool isEqual = true;
    isEqual = inCommitA.commitId == inCommitB.commitId ? isEqual : false;
    isEqual = inCommitA.leftParent == inCommitB.leftParent ? isEqual : false;
    isEqual = inCommitA.rightParent == inCommitB.rightParent ? isEqual : false;
    isEqual = inCommitA.timestamp == inCommitB.timestamp ? isEqual : false;
    isEqual = inCommitA.version == inCommitB.version ? isEqual : false;
    isEqual = inCommitA.isLocal == inCommitB.isLocal ? isEqual : false;
    isEqual = inCommitA.deviceInfo == inCommitB.deviceInfo ? isEqual : false;
    return isEqual;
}

static void MakeCommitHistorySyncRequestPacketA(CommitHistorySyncRequestPacket &packet)
{
    std::map<std::string, MultiVerCommitNode> comMap;
    comMap[string("A")] = MakeMultiVerCommitA();
    comMap[string("C")] = MakeMultiVerCommitC();
    packet.SetcomMap(comMap);
}

static void MakeCommitHistorySyncRequestpacketC(CommitHistorySyncRequestPacket &packet)
{
    std::map<std::string, MultiVerCommitNode> comMap;
    comMap[string("B")] = MakeMultiVerCommitB();
    comMap[string("C")] = MakeMultiVerCommitC();
    comMap[string("BB")] = MakeMultiVerCommitB();
    packet.SetcomMap(comMap);
}

static bool IsCommitHistorySyncRequestPacketisEqual(const CommitHistorySyncRequestPacket &packetA,
    const CommitHistorySyncRequestPacket &packetC)
{
    std::map<std::string, MultiVerCommitNode> comMapA;
    std::map<std::string, MultiVerCommitNode> comMapB;
    packetA.GetcomMap(comMapA);
    packetC.GetcomMap(comMapB);
    for (const auto &entry : comMapA) {
        if (comMapB.count(entry.first) == 0) {
            return false;
        }
        if (!IsMultiVerCommitisEqual(entry.second, comMapB[entry.first])) {
            return false;
        }
    }
    for (const auto &entry : comMapB) {
        if (comMapA.count(entry.first) == 0) {
            return false;
        }
        if (!IsMultiVerCommitisEqual(entry.second, comMapA[entry.first])) {
            return false;
        }
    }
    return true;
}

/**
 * @tc.name: Commit History Sync Request Packet 001
 * @tc.desc: Verify CommitHistorySyncRequestPacket Serialization and DeSerialization
 * @tc.type: FUNC
 * @tc.require: AR000BVRNU AR000CQE0J
 * @tc.author: xiaozhenjian
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, CommitHistorySyncRequestPacket001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create CommitHistorySyncRequestPacket packetA aand packetC
     */
    CommitHistorySyncRequestPacket packetA;
    CommitHistorySyncRequestPacket packetC;
    MakeCommitHistorySyncRequestPacketA(packetA);
    MakeCommitHistorySyncRequestpacketC(packetC);
    Message oriMsgA;
    Message oriMsgC;
    oriMsgA.SetCopiedObject(packetA);
    oriMsgA.SetMessageId(COMMIT_HISTORY_SYNC_MESSAGE);
    oriMsgA.SetMessageType(TYPE_REQUEST);
    oriMsgC.SetCopiedObject(packetC);
    oriMsgC.SetMessageId(COMMIT_HISTORY_SYNC_MESSAGE);
    oriMsgC.SetMessageType(TYPE_REQUEST);

    /**
     * @tc.steps: step2. Serialization packetA to bufferA
     */
    uint32_t lenA = CommitHistorySync::CalculateLen(&oriMsgA);
    vector<uint8_t> bufferA;
    bufferA.resize(lenA);
    int ret = CommitHistorySync::Serialization(bufferA.data(), lenA, &oriMsgA);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step3. Serialization packetC to bufferC
     */
    uint32_t lenB = CommitHistorySync::CalculateLen(&oriMsgC);
    vector<uint8_t> bufferC;
    bufferC.resize(lenB);
    ret = CommitHistorySync::Serialization(bufferC.data(), lenB, &oriMsgC);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step4. DeSerialization bufferA to outPktA
     * @tc.expected: step4. packetA == outPktA
     */
    Message outMsg;
    outMsg.SetMessageId(COMMIT_HISTORY_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_REQUEST);
    ret = CommitHistorySync::DeSerialization(bufferA.data(), lenA, &outMsg);
    EXCEPT_EQ(ret, E_OK);
    const CommitHistorySyncRequestPacket *outPktA = outMsg.GetObject<CommitHistorySyncRequestPacket>();
    ASSERT_NE(outPktA, nullptr);
    EXPECT_EQ(IsCommitHistorySyncRequestPacketisEqual(packetA, *outPktA), true);

    /**
     * @tc.steps: step5. DeSerialization bufferC to outPktB
     * @tc.expected: step5. packetC == outPktB, outPktB != outPktA
     */
    Message outMsgB;
    outMsgB.SetMessageId(COMMIT_HISTORY_SYNC_MESSAGE);
    outMsgB.SetMessageType(TYPE_REQUEST);
    ret = CommitHistorySync::DeSerialization(bufferC.data(), lenB, &outMsgB);
    EXCEPT_EQ(ret, E_OK);
    const CommitHistorySyncRequestPacket *outPktB = outMsgB.GetObject<CommitHistorySyncRequestPacket>();
    ASSERT_NE(outPktB, nullptr);
    EXPECT_EQ(IsCommitHistorySyncRequestPacketisEqual(packetC, *outPktB), true);
    EXPECT_EQ(IsCommitHistorySyncRequestPacketisEqual(*outPktA, *outPktB), false);
}

static void MakeCommitHistorySyncAckPacketA(CommitHistorySyncAckPacket &packet)
{
    std::vector<MultiVerCommitNode> commitVec;
    commitVec.push_back(MakeMultiVerCommitA());
    commitVec.push_back(MakeMultiVerCommitC());
    packet.SetData(commitVec);
    packet.SetErrorCode(10086); // 10086 is errorcode
}

static void MakeCommitHistorySyncAckpacketC(CommitHistorySyncAckPacket &packet)
{
    std::vector<MultiVerCommitNode> commitVec;
    commitVec.push_back(MakeMultiVerCommitB());
    commitVec.push_back(MakeMultiVerCommitC());
    commitVec.push_back(MakeMultiVerCommitB());
    packet.SetData(commitVec);
    packet.SetErrorCode(10010); // 10010 is errorcode
}

static bool IsCommitSyncAckPacketisEqual(const CommitHistorySyncAckPacket &packetA,
    const CommitHistorySyncAckPacket &packetC)
{
    int errCodeA;
    int errCodeB;
    std::vector<MultiVerCommitNode> commitVecA;
    std::vector<MultiVerCommitNode> commitVecB;
    packetA.GetData(commitVecA);
    packetC.GetData(commitVecB);
    packetA.GetErrorCode(errCodeA);
    packetC.GetErrorCode(errCodeB);
    if (errCodeA != errCodeB) {
        return false;
    }
    if (commitVecA.size() != commitVecB.size()) {
        return false;
    }
    int count = 0;
    for (const auto &entry : commitVecA) {
        if (!IsMultiVerCommitisEqual(entry, commitVecB[count++])) {
            return false;
        }
    }
    return true;
}

/**
 * @tc.name: Commit History Sync Ack Packet 001
 * @tc.desc: Verify CommitHistorySyncAckPacket Serialization and DeSerialization
 * @tc.type: FUNC
 * @tc.require: AR000BVRNU AR000CQE0J
 * @tc.author: xiaozhenjian
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, CommitHistorySyncAckPacket001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create CommitHistorySyncAckPacket packetA aand packetC
     */
    CommitHistorySyncAckPacket packetA;
    CommitHistorySyncAckPacket packetC;
    MakeCommitHistorySyncAckPacketA(packetA);
    MakeCommitHistorySyncAckpacketC(packetC);
    Message oriMsgA;
    Message oriMsgC;
    oriMsgA.SetCopiedObject(packetA);
    oriMsgA.SetMessageId(COMMIT_HISTORY_SYNC_MESSAGE);
    oriMsgA.SetMessageType(TYPE_RESPONSE);
    oriMsgC.SetCopiedObject(packetC);
    oriMsgC.SetMessageId(COMMIT_HISTORY_SYNC_MESSAGE);
    oriMsgC.SetMessageType(TYPE_RESPONSE);

    /**
     * @tc.steps: step2. Serialization packetA to bufferA
     */
    uint32_t lenA = CommitHistorySync::CalculateLen(&oriMsgA);
    vector<uint8_t> bufferA;
    bufferA.resize(lenA);
    int ret = CommitHistorySync::Serialization(bufferA.data(), lenA, &oriMsgA);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step3. Serialization packetC to bufferC
     */
    uint32_t lenB = CommitHistorySync::CalculateLen(&oriMsgC);
    vector<uint8_t> bufferC;
    bufferC.resize(lenB);
    ret = CommitHistorySync::Serialization(bufferC.data(), lenB, &oriMsgC);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step4. DeSerialization bufferA to outPktA
     * @tc.expected: step4. packetA == outPktA
     */
    Message outMsg;
    outMsg.SetMessageId(COMMIT_HISTORY_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_RESPONSE);
    ret = CommitHistorySync::DeSerialization(bufferA.data(), lenA, &outMsg);
    EXCEPT_EQ(ret, E_OK);
    const CommitHistorySyncAckPacket *outPktA = outMsg.GetObject<CommitHistorySyncAckPacket>();
    ASSERT_NE(outPktA, nullptr);
    EXPECT_EQ(IsCommitSyncAckPacketisEqual(packetA, *outPktA), true);

    /**
     * @tc.steps: step5. DeSerialization bufferC to outPktB
     * @tc.expected: step5. packetC == outPktB, outPktB!= outPktA
     */
    Message outMsgB;
    outMsgB.SetMessageId(COMMIT_HISTORY_SYNC_MESSAGE);
    outMsgB.SetMessageType(TYPE_RESPONSE);
    ret = CommitHistorySync::DeSerialization(bufferC.data(), lenB, &outMsgB);
    EXCEPT_EQ(ret, E_OK);
    const CommitHistorySyncAckPacket *outPktB = outMsgB.GetObject<CommitHistorySyncAckPacket>();
    ASSERT_NE(outPktB, nullptr);
    EXPECT_EQ(IsCommitSyncAckPacketisEqual(packetC, *outPktB), true);
    EXPECT_EQ(IsCommitSyncAckPacketisEqual(*outPktA, *outPktB), false);
}

static bool IsMultiVerRequestPacketisEqual(const MultiRequestPacket &packetA,
    const MultiRequestPacket &packetC)
{
    MultiVerCommitNode commitA;
    MultiVerCommitNode commitC;
    packetA.GetCommit(commitA);
    packetC.GetCommit(commitC);
    return IsMultiVerCommitisEqual(commitA, commitC);
}

/**
 * @tc.name: MultiVerValueObject Request Packet 001
 * @tc.desc: Verify MultiRequestPacket Serialization and DeSerialization
 * @tc.type: FUNC
 * @tc.require: AR000BVRNU AR000CQE0J
 * @tc.author: xiaozhenjian
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, MultiVerRequestPacket001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create CommitHistorySyncAckPacket packetA aand packetC
     */
    MultiRequestPacket packetA;
    MultiRequestPacket packetC;
    MultiVerCommitNode commitA = MakeMultiVerCommitA();
    MultiVerCommitNode commitC = MakeMultiVerCommitB();
    packetA.SetCommit(commitA);
    packetC.SetCommit(commitC);
    Message oriMsgA;
    Message oriMsgC;
    oriMsgA.SetCopiedObject(packetA);
    oriMsgA.SetMessageId(MULTI_VER_DATA_SYNC_MESSAGE);
    oriMsgA.SetMessageType(TYPE_REQUEST);
    oriMsgC.SetCopiedObject(packetC);

    /**
     * @tc.steps: step2. Serialization packetA to bufferA
     */
    uint32_t lenA = MultiVerDataSync::CalculateLen(&oriMsgA);
    vector<uint8_t> bufferA;
    bufferA.resize(lenA);
    int ret = MultiVerDataSync::Serialization(bufferA.data(), lenA, &oriMsgA);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step3. Serialization packetC to bufferC
     */
    uint32_t lenB = MultiVerDataSync::CalculateLen(&oriMsgC);
    vector<uint8_t> bufferC;
    bufferC.resize(lenB);
    ret = MultiVerDataSync::Serialization(bufferC.data(), lenB, &oriMsgC);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step4. DeSerialization bufferA to outPktA
     * @tc.expected: step4. packetA == outPktA
     */
    Message outMsg;
    outMsg.SetMessageId(MULTI_VER_DATA_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_REQUEST);
    ret = MultiVerDataSync::DeSerialization(bufferA.data(), lenA, &outMsg);
    EXCEPT_EQ(ret, E_OK);
    const MultiRequestPacket *outPktA = outMsg.GetObject<MultiRequestPacket>();
    ASSERT_NE(outPktA, nullptr);
    EXPECT_EQ(IsMultiVerRequestPacketisEqual(packetA, *outPktA), true);

    /**
     * @tc.steps: step5. DeSerialization bufferC to outPktB
     * @tc.expected: step5. packetC == outPktB, outPktB!= outPktA
     */
    Message outMsgB;
    outMsgB.SetMessageId(MULTI_VER_DATA_SYNC_MESSAGE);
    outMsgB.SetMessageType(TYPE_REQUEST);
    ret = MultiVerDataSync::DeSerialization(bufferC.data(), lenB, &outMsgB);
    EXCEPT_EQ(ret, E_OK);
    const MultiRequestPacket *outPktB = outMsgB.GetObject<MultiRequestPacket>();
    ASSERT_NE(outPktB, nullptr);
    EXPECT_EQ(IsMultiVerRequestPacketisEqual(packetC, *outPktB), true);
    EXPECT_EQ(IsMultiVerRequestPacketisEqual(*outPktA, *outPktB), false);
}

static void MakeMultiVerAckPacketA(MultiVerAckPacket &packet)
{
    std::vector<std::vector<uint8_t>> entryVec;
    entryVec.push_back(vector<uint8_t>(2, 2)); // 111 is length, 11 is value
    entryVec.push_back(vector<uint8_t>(3, 3)); // 222 is length, 22 is value
    packet.SetData(entryVec);
    packet.SetErrorCode(5); // 333 is errorcode
}

static void MakeMultiVerAckpacketC(MultiVerAckPacket &packet)
{
    std::vector<std::vector<uint8_t>> entryVec;
    entryVec.push_back(vector<uint8_t>(4, 9)); // 999 is length, 99 is value
    entryVec.push_back(vector<uint8_t>(5,7)); // 888 is length, 88 is value
    packet.SetData(entryVec);
    packet.SetErrorCode(1); // 777 is errorcode
}

static bool IsMultiAckPacketisEqual(const MultiVerAckPacket &packetA, const MultiVerAckPacket &packetC)
{
    int errCodeA;
    int errCodeB;
    std::vector<std::vector<uint8_t>> entryVecA;
    std::vector<std::vector<uint8_t>> entryVecB;
    packetA.GetData(entryVecA);
    packetC.GetData(entryVecB);
    packetA.GetErrorCode(errCodeA);
    packetC.GetErrorCode(errCodeB);
    if (errCodeA != errCodeB) {
        return false;
    }
    if (entryVecA != entryVecB) {
        return false;
    }
    return true;
}

/**
 * @tc.name: MultiVerValueObject Ack Packet 001
 * @tc.desc: Verify MultiVerAckPacket Serialization and DeSerialization
 * @tc.type: FUNC
 * @tc.require: AR000BVRNU AR000CQE0J
 * @tc.author: xiaozhenjian
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, MultiVerAckPacket001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create MultiVerAckPacket packetA aand packetC
     */
    MultiVerAckPacket packetA;
    MultiVerAckPacket packetC;
    MakeMultiVerAckPacketA(packetA);
    MakeMultiVerAckpacketC(packetC);
    Message oriMsgA;
    Message oriMsgC;
    oriMsgA.SetCopiedObject(packetA);
    oriMsgA.SetMessageId(MULTI_VER_DATA_SYNC_MESSAGE);
    oriMsgA.SetMessageType(TYPE_RESPONSE);
    oriMsgC.SetCopiedObject(packetC);
    oriMsgC.SetMessageId(MULTI_VER_DATA_SYNC_MESSAGE);
    oriMsgC.SetMessageType(TYPE_RESPONSE);

    /**
     * @tc.steps: step2. Serialization packetA to bufferA
     */
    uint32_t lenA = MultiVerDataSync::CalculateLen(&oriMsgA);
    vector<uint8_t> bufferA;
    bufferA.resize(lenA);
    int ret = MultiVerDataSync::Serialization(bufferA.data(), lenA, &oriMsgA);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step3. Serialization packetC to bufferC
     */
    uint32_t lenB = MultiVerDataSync::CalculateLen(&oriMsgC);
    vector<uint8_t> bufferC;
    bufferC.resize(lenB);
    ret = MultiVerDataSync::Serialization(bufferC.data(), lenB, &oriMsgC);
    EXCEPT_EQ(ret, E_OK);

    /**
     * @tc.steps: step4. DeSerialization bufferA to outPktA
     * @tc.expected: step4. packetA == outPktA
     */
    Message outMsg;
    outMsg.SetMessageId(MULTI_VER_DATA_SYNC_MESSAGE);
    outMsg.SetMessageType(TYPE_RESPONSE);
    ret = MultiVerDataSync::DeSerialization(bufferA.data(), lenA, &outMsg);
    EXCEPT_EQ(ret, E_OK);
    const MultiVerAckPacket *outPktA = outMsg.GetObject<MultiVerAckPacket>();
    ASSERT_NE(outPktA, nullptr);
    EXPECT_EQ(IsMultiAckPacketisEqual(packetA, *outPktA), true);

    /**
     * @tc.steps: step5. DeSerialization bufferC to outPktB
     * @tc.expected: step5. packetC == outPktB, outPktB!= outPktA
     */
    Message outMsgB;
    outMsgB.SetMessageId(MULTI_VER_DATA_SYNC_MESSAGE);
    outMsgB.SetMessageType(TYPE_RESPONSE);
    ret = MultiVerDataSync::DeSerialization(bufferC.data(), lenB, &outMsgB);
    EXCEPT_EQ(ret, E_OK);
    const MultiVerAckPacket *outPktB = outMsgB.GetObject<MultiVerAckPacket>();
    ASSERT_NE(outPktB, nullptr);
    EXPECT_EQ(IsMultiAckPacketisEqual(packetC, *outPktB), true);
    EXPECT_EQ(IsMultiAckPacketisEqual(*outPktA, *outPktB), false);
}

/**
 * @tc.name: Simple Data Sync 001
 * @tc.desc: Verify normal simple data sync function.
 * @tc.type: FUNC
 * @tc.require: AR000BVDGR
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, DataSync001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    /**
     * @tc.steps: step2. deviceB put {k1, v1}
     */
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);

    /**
     * @tc.steps: step4. deviceB put {k2, v2}
     */
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_2, KvDBUnitTest::VALUE_2), E_OK);

    /**
     * @tc.steps: step5. enable communicator and set deviceB,C online
     */
    g_deviceB->Online();
    g_deviceC->Online();

    /**
     * @tc.steps: step6. wait for sync
     * @tc.expected: step6. deviceA has {k1, v2} {k2, v2}
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_1, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_1);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_2);
}

/**
 * @tc.name: Big Data Sync 001
 * @tc.desc: Verify normal big data sync function.
 * @tc.type: FUNC
 * @tc.require: AR000BVDGR
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, BigDataSync001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    /**
     * @tc.steps: step2. deviceB put {k1, v1}, v1 size 1k
     */
    Value value1;
    KvDBToolsUnitTest::GetRandomValue(value1, BIG_VALUE_SIZE); // 1k +1
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, value1), E_OK);

    /**
     * @tc.steps: step4. deviceC put {k2, v2}, v2 size 1k
     */
    Value value2;
    KvDBToolsUnitTest::GetRandomValue(value2, BIG_VALUE_SIZE); // 1k +1
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_2, value2), E_OK);

    /**
     * @tc.steps: step5. set deviceB,C online
     */
    g_deviceB->Online();
    g_deviceC->Online();

    /**
     * @tc.steps: step5. wait 2s for sync
     * @tc.expected: step5. deviceA has {k1, v2} {k2, v2}
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_1, value), E_OK);
    EXPECT_EQ(value, value1);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), E_OK);
    EXPECT_EQ(value, value2);
}

/**
 * @tc.name: Limit Data Sync 001
 * @tc.desc: Verify normal limit data sync function.
 * @tc.type: FUNC
 * @tc.require: AR000BVDGR
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, LimitDataSync001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));
    /**
     * @tc.steps: step2. deviceB put {k1, v1}, k1 size 1k, v1 size 4M
     */
    Key key1;
    Value value1;
    KvDBToolsUnitTest::GetRandomValue(key1, LIMIT_KEY_SIZE);
    KvDBToolsUnitTest::GetRandomValue(value1, LIMIT_VALUE_SIZE);
    EXCEPT_EQ(g_deviceB->PutData(key1, value1), E_OK);

    /**
     * @tc.steps: step3. deviceC put {k2, v2}, k2 size 1k, v2 size 4M
     */
    Key key2;
    Value value2;
    KvDBToolsUnitTest::GetRandomValue(key2, LIMIT_KEY_SIZE);
    KvDBToolsUnitTest::GetRandomValue(value2, LIMIT_VALUE_SIZE);
    EXCEPT_EQ(g_deviceC->PutData(key2, value2), E_OK);

    /**
     * @tc.steps: step4. set deviceB,C online
     */
    g_deviceB->Online();
    g_deviceC->Online();

    /**
     * @tc.steps: step5. wait 30 for sync
     * @tc.expected: step5. deviceA has {k1, v2} {k2, v2}
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_LIMIT_TIME));
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, key1, value), E_OK);
    EXPECT_EQ(value, value1);
    EXPECT_EQ(GetData(g_kvPtr, key2, value), E_OK);
    EXPECT_EQ(value, value2);
}

/**
 * @tc.name: Multi Record 001
 * @tc.desc: Verify normal multi record sync function.
 * @tc.type: FUNC
 * @tc.require: AR000BVDGR
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, MultiRecord001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    /**
     * @tc.steps: step2. deviceB put {k1, v1}
     */
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);

    /**
     * @tc.steps: step4. deviceB put {k1, v2} v2 > 1K
     */
    Value value2;
    KvDBToolsUnitTest::GetRandomValue(value2, BIG_VALUE_SIZE); // 1k +1
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, value2), E_OK);

    /**
     * @tc.steps: step4. deviceB put {k2, v3}
     */
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_2, KvDBUnitTest::VALUE_3), E_OK);

    /**
     * @tc.steps: step5. deviceB put {k3, v3} and delete k3
     */
    EXCEPT_TRUE(g_deviceB->StartTransaction() == E_OK);
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_3, KvDBUnitTest::VALUE_3), E_OK);
    EXCEPT_EQ(g_deviceB->DeleteData(KvDBUnitTest::KEY_3), E_OK);
    EXCEPT_TRUE(g_deviceB->Commit() == E_OK);

    /**
     * @tc.steps: step6. deviceC put {k4, v4}
     */
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_4, KvDBUnitTest::VALUE_4), E_OK);

    /**
     * @tc.steps: step7. deviceB put {k4, v5} v2 > 1K
     */
    Value value5;
    KvDBToolsUnitTest::GetRandomValue(value5, BIG_VALUE_SIZE); // 1k +1
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_4, value5), E_OK);

    /**
     * @tc.steps: step8. deviceB put {k5, v6}
     */
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_5, KvDBUnitTest::VALUE_6), E_OK);

    /**
     * @tc.steps: step9. deviceB put {k6, v6} and delete k6
     */
    EXCEPT_TRUE(g_deviceC->StartTransaction() == E_OK);
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_6, KvDBUnitTest::VALUE_6), E_OK);
    EXCEPT_EQ(g_deviceC->DeleteData(KvDBUnitTest::KEY_6), E_OK);
    EXCEPT_TRUE(g_deviceC->Commit() == E_OK);

    /**
     * @tc.steps: step10. set deviceB,C online
     */
    g_deviceB->Online();
    g_deviceC->Online();

    /**
     * @tc.steps: step11. wait 5s for sync
     * @tc.expected: step11. deviceA has {k1, v2}, {k2, v3}, {k4, v5}, {k5, v6}
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_LONG_TIME));
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_1, value), E_OK);
    EXPECT_EQ(value, value2);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_3);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_4, value), E_OK);
    EXPECT_EQ(value, value5);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_5, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_6);
}

/**
 * @tc.name: Net Disconnect Sync 001
 * @tc.desc: Test exception sync when net disconnected.
 * @tc.type: FUNC
 * @tc.require: AR000BVDGR
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, NetDisconnectSync001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    devicesVec.push_back(g_deviceC->GetDeviceId());

    EXCEPT_TRUE(g_deviceB->StartTransaction() == E_OK);
    /**
     * @tc.steps: step2. deviceB put {k1, v1}
     */
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);

    /**
     * @tc.steps: step4. deviceB put {k1, v2} v2 > 1K
     */
    Value value2;
    KvDBToolsUnitTest::GetRandomValue(value2, 1024 + 1); // 1k +1
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, value2), E_OK);

    /**
     * @tc.steps: step4. deviceB put {k2, v3}
     */
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_2, KvDBUnitTest::VALUE_3), E_OK);

    /**
     * @tc.steps: step5. deviceB put {k3, v3} and delete k3
     */
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_3, KvDBUnitTest::VALUE_3), E_OK);
    EXCEPT_EQ(g_deviceB->DeleteData(KvDBUnitTest::KEY_3), E_OK);
    EXCEPT_TRUE(g_deviceB->Commit() == E_OK);

    /**
     * @tc.steps: step6. deviceB online and enable communicator
     */
    g_deviceB->Online();

    /**
     * @tc.steps: step7. disable communicator and wait 5s
     * @tc.expected: step7. deviceA has no key1, key2
     */
    g_communicatorAggregator->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_LONG_TIME + WAIT_LONG_TIME));

    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_1, value), NOT_FOUND);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), NOT_FOUND);

    EXCEPT_TRUE(g_deviceC->StartTransaction() == E_OK);
    /**
     * @tc.steps: step8. deviceC put {k4, v4}
     */
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_4, KvDBUnitTest::VALUE_4), E_OK);

    /**
     * @tc.steps: step9. deviceB put {k4, v5} v2 > 1K
     */
    Value value5;
    KvDBToolsUnitTest::GetRandomValue(value5, BIG_VALUE_SIZE); // 1k +1
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_4, value5), E_OK);

    /**
     * @tc.steps: step10. deviceB put {k5, v6}
     */
    EXCEPT_TRUE(g_deviceC->PutData(KvDBUnitTest::KEY_5, KvDBUnitTest::VALUE_6) == E_OK);

    /**
     * @tc.steps: step11. deviceB put {k6, v6} and delete k6
     */
    EXCEPT_TRUE(g_deviceC->PutData(KvDBUnitTest::KEY_6, KvDBUnitTest::VALUE_6) == E_OK);
    EXCEPT_TRUE(g_deviceC->DeleteData(KvDBUnitTest::KEY_6) == E_OK);
    EXCEPT_TRUE(g_deviceC->Commit() == E_OK);

    /**
     * @tc.steps: step12. deviceC online and enable communicator
     */
    g_communicatorAggregator->Enable();
    g_deviceC->Online();

    /**
     * @tc.steps: step13. wait 5s for sync
     * @tc.expected: step13. deviceA has {k4, v5}, {k5, v6}
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_LONG_TIME)); // wait 5s
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_4, value), E_OK);
    EXPECT_EQ(value, value5);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_5, value), E_OK);
    EXPECT_EQ(value, KvDBUnitTest::VALUE_6);
}

/**
  * @tc.name: SyncQueue006
  * @tc.desc: multi version not support sync queue
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(KvDBMultiVerP2PSyncTest, SyncQueue006, TestSize.Level3)
{
    /**
     * @tc.steps:step1. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);

    /**
     * @tc.steps:step2. Set PragmaCmd to be GET_QUEUED_SYNC_SIZE
     * @tc.expected: step2. Expect return NOT_SUPPORT.
     */
    int param;
    PragmaData input = static_cast<PragmaData>(&param);
    EXPECT_EQ(g_kvPtr->Pragma(GET_QUEUED_SYNC_SIZE, input), NOT_SUPPORT);
    EXPECT_EQ(g_kvPtr->Pragma(SET_QUEUED_SYNC_LIMIT, input), NOT_SUPPORT);
    EXPECT_EQ(g_kvPtr->Pragma(GET_QUEUED_SYNC_LIMIT, input), NOT_SUPPORT);
}

/**
 * @tc.name: PermissionCheckTest001
 * @tc.desc: deviceA permission check not pass
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, PermissionCheckTest001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. SetPermissionCheckTestCallback
     * @tc.expected: step1. return OK.
     */
    auto PermissionCheckTestCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
                                        const std::string &deviceId, uint8_t flag) -> bool {
                                        if (flag & CHECK_FLAG_RECEIVE) {
                                            LOGD("in RunPermissionCheckTest callback func, check not pass, flag:%d", flag);
                                            return false;
                                        } else {
                                            LOGD("in RunPermissionCheckTest callback func, check pass, flag:%d", flag);
                                            return true;
                                        }
                                        };
    EXPECT_EQ(g_mgr.SetPermissionCheckTestCallback(PermissionCheckTestCallback), OK);

    /**
     * @tc.steps: step2. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    /**
     * @tc.steps: step3. deviceB put {k1, v1}
     */
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);

    /**
     * @tc.steps: step4. deviceC put {k2, v2}
     */
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_2, KvDBUnitTest::VALUE_2), E_OK);

    /**
     * @tc.steps: step5. enable communicator and set deviceB,C online
     */
    g_deviceB->Online();
    g_deviceC->Online();

    /**
     * @tc.steps: step6. wait for sync
     * @tc.expected: step6. deviceA do not has {k1, v2} {k2, v2}
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_1, value), NOT_FOUND);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), NOT_FOUND);
    PermissionCheckTestCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckTestCallback(nullCallback), OK);
}

/**
 * @tc.name: PermissionCheckTest002
 * @tc.desc: deviceB deviceC permission check not pass
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBMultiVerP2PSyncTest, PermissionCheckTest002, TestSize.Level2)
{
    /**
     * @tc.steps: step1. SetPermissionCheckTestCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckTestCallback = [] (const std::string &userId, const std::string &appId,
                                        const std::string &storeId, const std::string &deviceId,
                                        uint8_t flag) -> bool {
                                        if (flag & CHECK_FLAG_SEND) {
                                            LOGD("in RunPermissionCheckTest check not pass, flag:%d", flag);
                                            return false;
                                        } else {
                                            LOGD("in RunPermissionCheckTest check pass, flag:%d", flag);
                                            return true;
                                        }
                                        };
    EXPECT_EQ(g_mgr.SetPermissionCheckTestCallback(permissionCheckTestCallback), OK);

    /**
     * @tc.steps: step2. open a KvStoreNbDelegate as deviceA
     */
    g_mgr.GetKvStore(STORE_ID, g_option, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_1));

    /**
     * @tc.steps: step3. deviceB put {k1, v1}
     */
    EXCEPT_EQ(g_deviceB->PutData(KvDBUnitTest::KEY_1, KvDBUnitTest::VALUE_1), E_OK);

    /**
     * @tc.steps: step4. deviceC put {k2, v2}
     */
    EXCEPT_EQ(g_deviceC->PutData(KvDBUnitTest::KEY_2, KvDBUnitTest::VALUE_2), E_OK);

    /**
     * @tc.steps: step5. enable communicator and set deviceB,C online
     */
    g_deviceB->Online();
    g_deviceC->Online();

    /**
     * @tc.steps: step6. wait for sync
     * @tc.expected: step6. deviceA do not has {k1, v2} {k2, v2}
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME_2));
    Value value;
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_1, value), NOT_FOUND);
    EXPECT_EQ(GetData(g_kvPtr, KvDBUnitTest::KEY_2, value), NOT_FOUND);
    PermissionCheckTestCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckTestCallback(nullCallback), OK);
}
#endif
#endif // OMIT_MULTI_VER