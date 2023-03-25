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

#include <condition_variable>
#include <gtest/gtest.h>
#include <thread>

#include "db_constant.h"
#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string STORE_ID = "kv_stroe_permission_sync_test";
    const int WAIT_TIME = 1000;
    const std::string DEVICE_A = "real_device";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";

    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreConfig g_config;
    DistributedDBToolsUnitTest g_tool;
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate* g_kvDelegatePtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
    KvVirtualDevice *g_deviceB = nullptr;
    KvVirtualDevice *g_deviceC = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvDelegatePtr));
}

class DistributedDBSingleVerP2PPermissionSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerP2PPermissionSyncTest::SetUpTestCase(void)
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
}

void DistributedDBSingleVerP2PPermissionSyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerP2PPermissionSyncTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B and C, and get a KvStoreNbDelegate as deviceA
     */
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);

    g_deviceC = new (std::nothrow) KvVirtualDevice(DEVICE_C);
    ASSERT_TRUE(g_deviceC != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceC = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceC != nullptr);
    ASSERT_EQ(g_deviceC->Initialize(g_communicatorAggregator, syncInterfaceC), E_OK);

    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            return true;
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
}

void DistributedDBSingleVerP2PPermissionSyncTest::TearDown(void)
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
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck001
  * @tc.desc: deviceA PermissionCheck not pass test, SYNC_MODE_PUSH_ONLY
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            if (flag & CHECK_FLAG_SEND) {
                LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
                return false;
            } else {
                LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
                return true;
            }
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step2. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called,
     * status == PERMISSION_CHECK_FORBID_SYNC, deviceB and deviceC do not have {k1, v1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
    }
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck002
  * @tc.desc: deviceA PermissionCheck not pass test, SYNC_MODE_PULL_ONLY
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck002, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            if (flag & CHECK_FLAG_RECEIVE) {
                LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
                return false;
            } else {
                LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
                return true;
            }
        };

    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);

    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step2. deviceB put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    g_deviceB->PutData(key, value, 0, 0);

    /**
     * @tc.steps: step2. deviceB put {k2, v2}
     */
    Key key2 = {'2'};
    Value value2 = {'2'};
    g_deviceC->PutData(key2, value2, 0, 0);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call pull sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called,
     * status == PERMISSION_CHECK_FORBID_SYNC, DeviceA do not have {k1, VALUE_1}, {K2. VALUE_2}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
    }
    Value value3;
    EXPECT_EQ(g_kvDelegatePtr->Get(key, value3), NOT_FOUND);
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, value3), NOT_FOUND);
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck003
  * @tc.desc: deviceA PermissionCheck not pass test, SYNC_MODE_PUSH_PULL
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck003, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            if (flag & (CHECK_FLAG_SEND | CHECK_FLAG_RECEIVE)) {
                LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
                return false;
            } else {
                LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
                return true;
            }
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);

    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step2. deviceA put {k1, v1}
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    DBStatus status = g_kvDelegatePtr->Put(key1, value1);
    EXPECT_TRUE(status == OK);

    /**
     * @tc.steps: step2. deviceB put {k2, v2}
     */
    Key key2 = {'2'};
    Value value2 = {'2'};
    g_deviceB->PutData(key2, value2, 0, 0);

    /**
     * @tc.steps: step2. deviceB put {k3, v3}
     */
    Key key3 = {'3'};
    Value value3 = {'3'};
    g_deviceC->PutData(key3, value3, 0, 0);

    /**
     * @tc.steps: step3. deviceA call push_pull sync
     * @tc.expected: step3. sync should return OK.
     * onComplete should be called, status == PERMISSION_CHECK_FORBID_SYNC
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    ASSERT_TRUE(status == OK);

    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
    }

    /**
     * @tc.expected: step3. DeviceA only have {k1. v1}
     *      DeviceB only have {k2. v2}, DeviceC only have {k3. v3}
     */
    Value value4;
    EXPECT_TRUE(g_kvDelegatePtr->Get(key2, value4) == NOT_FOUND);
    EXPECT_TRUE(g_kvDelegatePtr->Get(key3, value4) == NOT_FOUND);

    VirtualDataItem item1;
    g_deviceB->GetData(key1, item1);
    EXPECT_TRUE(item1.value.empty());
    g_deviceB->GetData(key3, item1);
    EXPECT_TRUE(item1.value.empty());

    VirtualDataItem item2;
    g_deviceC->GetData(key1, item2);
    EXPECT_TRUE(item1.value.empty());
    g_deviceC->GetData(key2, item2);
    EXPECT_TRUE(item2.value.empty());
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck004
  * @tc.desc: deviceB and deviceC PermissionCheck not pass test, SYNC_MODE_PUSH_ONLY
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck004, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            if (flag & CHECK_FLAG_RECEIVE) {
                LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
                return false;
            } else {
                LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
                return true;
            }
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step2. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called,
     * status == PERMISSION_CHECK_FORBID_SYNC, deviceB and deviceC do not have {k1, v1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
    }
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck005
  * @tc.desc: deviceB and deviceC PermissionCheck not pass test, SYNC_MODE_PULL_ONLY
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck005, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            if (flag & CHECK_FLAG_SEND) {
                LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
                return false;
            } else {
                LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
                return true;
            }
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);

    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step2. deviceB put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    g_deviceB->PutData(key, value, 0, 0);

    /**
     * @tc.steps: step2. deviceB put {k2, v2}
     */
    Key key2 = {'2'};
    Value value2 = {'2'};
    g_deviceC->PutData(key2, value2, 0, 0);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call pull sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called,
     * status == PERMISSION_CHECK_FORBID_SYNC, DeviceA do not have {k1, VALUE_1}, {K2. VALUE_2}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
    }
    Value value3;
    EXPECT_EQ(g_kvDelegatePtr->Get(key, value3), NOT_FOUND);
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, value3), NOT_FOUND);
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck006
  * @tc.desc: deviceA PermissionCheck deviceB not pass, deviceC pass
  * @tc.type: FUNC
  * @tc.require: AR000EJJOJ
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck006, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            if (deviceId == g_deviceB->GetDeviceId()) {
                LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
                return false;
            } else {
                LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
                return true;
            }
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step2. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called,
     * status == PERMISSION_CHECK_FORBID_SYNC, deviceB and deviceC do not have {k1, v1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        if (g_deviceB->GetDeviceId() == pair.first) {
            EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
        } else {
            EXPECT_TRUE(pair.second == OK);
        }
    }
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value == value);
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck007
  * @tc.desc: deviceA PermissionCheck, deviceB not pass, deviceC pass in SYNC_MODE_AUTO_PUSH
  * @tc.type: FUNC
  * @tc.require: AR000G3RLS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck007, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            if (deviceId == g_deviceC->GetDeviceId() &&
                (flag & (CHECK_FLAG_RECEIVE | CHECK_FLAG_AUTOSYNC))) {
                LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
                return false;
            } else {
                LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
                return true;
            }
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());
    /**
     * @tc.steps: step2. deviceA set auto sync
     */
    bool autoSync = true;
    PragmaData data = static_cast<PragmaData>(&autoSync);
    status = g_kvDelegatePtr->Pragma(AUTO_SYNC, data);
    ASSERT_EQ(status, OK);

    /**
     * @tc.steps: step3. deviceA put {k1, v1}, and sleep 1s
     */
    Key key = {'1'};
    Value value = {'1'};
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));

    /**
     * @tc.steps: step3. check value in device B and not in device C.
     */
    VirtualDataItem item;
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value.empty());
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value);
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
+  * @tc.name: PermissionCheck008
+  * @tc.desc: deviceA PermissionCheck, deviceB not pass, deviceC pass in SYNC_MODE_AUTO_PULL
+  * @tc.type: FUNC
+  * @tc.require: AR000G3RLS
+  * @tc.author: zhangqiquan
+  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck008, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            if (deviceId == g_deviceC->GetDeviceId() &&
                (flag & CHECK_FLAG_SPONSOR)) {
                LOGD("in RunPermissionCheck callback func, check not pass, flag:%d", flag);
                return false;
            } else {
                LOGD("in RunPermissionCheck callback func, check pass, flag:%d", flag);
                return true;
            }
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step2. deviceB put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    g_deviceB->PutData(key, value, 0, 0);

    /**
     * @tc.steps: step2. device put {k2, v2}
     */
    Key key2 = {'2'};
    Value value2 = {'2'};
    g_deviceC->PutData(key2, value2, 0, 0);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call push sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));

    /**
     * @tc.expected: step4. onComplete should be called,
     * status == PERMISSION_CHECK_FORBID_SYNC, deviceB and deviceC do not have {k1, v1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        if (g_deviceC->GetDeviceId() == pair.first) {
                EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
            } else {
                EXPECT_TRUE(pair.second == OK);
        }
    }
    /**
     * @tc.steps: step5. check value in device A
     */
    Value value4;
    EXPECT_TRUE(g_kvDelegatePtr->Get(key, value4) == OK);
    EXPECT_TRUE(g_kvDelegatePtr->Get(key2, value4) == NOT_FOUND);
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck009
  * @tc.desc: different runpermissioncheck call return different value
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck009, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    int count = 1;
    auto permissionCheckCallback = [&count] (const std::string &userId, const std::string &appId,
        const std::string &storeId, const std::string &deviceId, uint8_t flag) -> bool {
            (void)userId;
            (void)appId;
            (void)storeId;
            (void)deviceId;
            if (flag & CHECK_FLAG_SEND) {
                bool result = count % 2;
                LOGD("in RunPermissionCheck callback, check result:%d, flag:%d", result, flag);
                count++;
                return result;
            }
            LOGD("in RunPermissionCheck callback, check pass, flag:%d", flag);
            return true;
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
    std::vector<std::string> devices = {g_deviceB->GetDeviceId()};
    /**
     * @tc.steps: step2. deviceA call sync three times and not wait
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    ASSERT_TRUE(g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [&result](const std::map<std::string, DBStatus>& statusMap) {
            result = statusMap;
        }, false) == OK);
    std::map<std::string, DBStatus> result2;
    ASSERT_TRUE(g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [&result2](const std::map<std::string, DBStatus>& statusMap) {
            result2 = statusMap;
        }, false) == OK);
    std::map<std::string, DBStatus> result3;
    ASSERT_TRUE(g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY,
        [&result3](const std::map<std::string, DBStatus>& statusMap) {
            result3 = statusMap;
        }, false) == OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    /**
     * @tc.expected: step3. onComplete should be called,
     * status is : OK, PERMISSION_CHECK_FORBID_SYNC, OK
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        EXPECT_TRUE(pair.second == OK);
    }
    ASSERT_TRUE(result2.size() == devices.size());
    for (const auto &pair : result2) {
        EXPECT_TRUE(pair.second == PERMISSION_CHECK_FORBID_SYNC);
    }
    ASSERT_TRUE(result3.size() == devices.size());
    for (const auto &pair : result3) {
        EXPECT_TRUE(pair.second == OK);
    }
    PermissionCheckCallbackV2 nullCallback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}

/**
  * @tc.name: PermissionCheck010
  * @tc.desc: permission check cost lot of time and return false
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBSingleVerP2PPermissionSyncTest, PermissionCheck010, TestSize.Level3)
{
    /**
     * @tc.steps: step1. SetPermissionCheckCallback
     * @tc.expected: step1. return OK.
     */
    int count = 0;
    auto permissionCheckCallback = [&count] (const std::string &userId, const std::string &appId,
        const std::string &storeId, const std::string &deviceId, uint8_t flag) -> bool {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        count++;
        LOGD("check permission %d", count);
        return count > 1;
    };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
    /**
     * @tc.steps: step2. put (k1, v1)
     * @tc.expected: step2. return OK.
     */
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    EXPECT_EQ(g_kvDelegatePtr->Put(k1, v1), OK);
    /**
     * @tc.steps: step3. sync to DEVICE_B twice
     * @tc.expected: step3. return OK.
     */
    std::vector<std::string> devices;
    devices.push_back(DEVICE_B);
    EXPECT_TRUE(g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY, nullptr, false) == OK);
    EXPECT_TRUE(g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY, nullptr, true) == OK);
    /**
     * @tc.steps: step4. (k1, v1) exist in DeviceB
     * @tc.expected: step4. get return OK.
     */
    VirtualDataItem actualValue;
    EXPECT_EQ(g_deviceB->GetData(k1, actualValue), OK);
    EXPECT_EQ(v1, actualValue.value);
    PermissionCheckCallbackV2 nullCallback = nullptr;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(nullCallback), OK);
}