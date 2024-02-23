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
#include "runtime_config.h"
#include "single_ver_data_sync.h"
#include "single_ver_kv_sync_task_context.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    const string STORE_ID = "kv_stroe_sync_test";
    const int64_t TIME_OFFSET = 5000000;
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

    void CalculateDataTest(uint32_t itemCount, uint32_t keySize, uint32_t valueSize)
    {
        for (uint32_t i = 0; i < itemCount; i++) {
            std::vector<uint8_t> prefixKey = {'a', 'b', 'c'};
            Key key = DistributedDBToolsUnitTest::GetRandPrefixKey(prefixKey, keySize);
            Value value;
            DistributedDBToolsUnitTest::GetRandomKeyValue(value, valueSize);
            EXPECT_EQ(g_kvDelegatePtr->Put(key, value), OK);
        }
        size_t dataSize = g_kvDelegatePtr->GetSyncDataSize(DEVICE_B);
        uint32_t expectedDataSize = (valueSize + keySize);
        uint32_t externalSize = 70u;
        uint32_t serialHeadLen = 8u;
        LOGI("expectedDataSize=%u, v=%u", expectedDataSize, externalSize);
        uint32_t maxDataSize = 1024u * 1024u;
        if (itemCount * expectedDataSize >= maxDataSize) {
            EXPECT_EQ(static_cast<uint32_t>(dataSize), maxDataSize);
            return;
        }
        ASSERT_GE(static_cast<uint32_t>(dataSize), itemCount * expectedDataSize);
        ASSERT_LE(static_cast<uint32_t>(dataSize), serialHeadLen + itemCount * (expectedDataSize + externalSize));
    }

class DistributedDBSingleVerP2PSimpleSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerP2PSimpleSyncTest::SetUpTestCase(void)
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

void DistributedDBSingleVerP2PSimpleSyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerP2PSimpleSyncTest::SetUp(void)
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

void DistributedDBSingleVerP2PSimpleSyncTest::TearDown(void)
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

void CheckWatermark(const std::string &dev, KvStoreNbDelegate *kvDelegatePtr, WatermarkInfo expectInfo,
    bool sendEqual = true, bool receiveEqual = true)
{
    auto [status, watermarkInfo] = kvDelegatePtr->GetWatermarkInfo(dev);
    EXPECT_EQ(status, OK);
    if (sendEqual) {
        EXPECT_EQ(watermarkInfo.sendMark, expectInfo.sendMark);
    } else {
        EXPECT_NE(watermarkInfo.sendMark, expectInfo.sendMark);
    }
    if (receiveEqual) {
        EXPECT_EQ(watermarkInfo.receiveMark, expectInfo.receiveMark);
    } else {
        EXPECT_NE(watermarkInfo.receiveMark, expectInfo.receiveMark);
    }
}

/**
 * @tc.name: Normal Sync 001
 * @tc.desc: Test normal push sync for add data.
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S SR000CQE0B
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync001, TestSize.Level1)
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

    WatermarkInfo info;
    CheckWatermark(g_deviceB->GetDeviceId(), g_kvDelegatePtr, info);
    CheckWatermark(g_deviceC->GetDeviceId(), g_kvDelegatePtr, info);
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);
    CheckWatermark(g_deviceB->GetDeviceId(), g_kvDelegatePtr, info, false);
    CheckWatermark(g_deviceC->GetDeviceId(), g_kvDelegatePtr, info, false);

    /**
     * @tc.expected: step2. onComplete should be called, DeviceB,C have {k1,v1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value);
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value == value);
}

/**
 * @tc.name: Normal Sync 002
 * @tc.desc: Test normal push sync for update data.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync002, TestSize.Level1)
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

    /**
     * @tc.steps: step2. deviceA put {k1, v2}
     */
    Value value2;
    value2.push_back('2');
    status = g_kvDelegatePtr->Put(key, value2);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceB,C have {k1,v2}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    g_deviceC->GetData(key, item);
    EXPECT_TRUE(item.value == value2);
    g_deviceB->GetData(key, item);
    EXPECT_TRUE(item.value == value2);
}

/**
 * @tc.name: Normal Sync 003
 * @tc.desc: Test normal push sync for delete data.
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync003, TestSize.Level1)
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

    /**
     * @tc.steps: step2. deviceA delete k1
     */
    status = g_kvDelegatePtr->Delete(key);
    ASSERT_TRUE(status == OK);
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    /**
     * @tc.expected: step3. onComplete should be called, DeviceB,C have {k1, delete}
     */
    VirtualDataItem item;
    Key hashKey;
    DistributedDBToolsUnitTest::CalcHash(key, hashKey);
    EXPECT_EQ(g_deviceB->GetData(hashKey, item), -E_NOT_FOUND);
    EXPECT_EQ(g_deviceC->GetData(hashKey, item), -E_NOT_FOUND);
}

/**
 * @tc.name: Normal Sync 004
 * @tc.desc: Test normal pull sync for add data.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync004, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    WatermarkInfo info;
    CheckWatermark(g_deviceB->GetDeviceId(), g_kvDelegatePtr, info);
    CheckWatermark(g_deviceC->GetDeviceId(), g_kvDelegatePtr, info);
    /**
     * @tc.steps: step1. deviceB put {k1, v1}
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
    CheckWatermark(g_deviceB->GetDeviceId(), g_kvDelegatePtr, info, true, false);
    CheckWatermark(g_deviceC->GetDeviceId(), g_kvDelegatePtr, info, true, false);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceA have {k1, VALUE_1}, {K2. VALUE_2}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    Value value3;
    EXPECT_EQ(g_kvDelegatePtr->Get(key, value3), OK);
    EXPECT_EQ(value3, value);
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, value3), OK);
    EXPECT_EQ(value3, value2);
}

/**
 * @tc.name: Normal Sync 005
 * @tc.desc: Test normal pull sync for update data.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM SR000CQE10
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync005, TestSize.Level2)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}, {k2, v2} t1
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    status = g_kvDelegatePtr->Put(key1, value1);
    ASSERT_TRUE(status == OK);
    Key key2 = {'2'};
    Value value2 = {'2'};
    status = g_kvDelegatePtr->Put(key2, value2);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step2. deviceB put {k1, v3} t2, t2 > t1
     */
    Value value3;
    value3.push_back('3');
    g_deviceB->PutData(key1, value3,
        TimeHelper::GetSysCurrentTime() + g_deviceB->GetLocalTimeOffset() + TIME_OFFSET, 0);

    /**
     * @tc.steps: step3. deviceC put {k2, v4} t2, t4 < t1
     */
    Value value4;
    value4.push_back('4');
    g_deviceC->PutData(key2, value4,
        TimeHelper::GetSysCurrentTime() + g_deviceC->GetLocalTimeOffset() - TIME_OFFSET, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    /**
     * @tc.steps: step4. deviceA call pull sync
     * @tc.expected: step4. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step4. onComplete should be called, DeviceA have {k1, v3}, {k2. v2}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    Value value5;
    g_kvDelegatePtr->Get(key1, value5);
    EXPECT_TRUE(value5 == value3);
    g_kvDelegatePtr->Get(key2, value5);
    EXPECT_TRUE(value5 == value2);
}

/**
 * @tc.name: Normal Sync 006
 * @tc.desc: Test normal pull sync for delete data.
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync006, TestSize.Level2)
{
    /**
     * @tc.steps: step1. deviceA put {k1, v1}, {k2, v2} t1
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    DBStatus status = g_kvDelegatePtr->Put(key1, value1);
    ASSERT_TRUE(status == OK);
    Key key2 = {'2'};
    Value value2 = {'2'};
    status = g_kvDelegatePtr->Put(key2, value2);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step2. deviceA put {k1, delete} t2, t2 <t1
     */
    Key hashKey1;
    DistributedDBToolsUnitTest::CalcHash(key1, hashKey1);
    g_deviceB->PutData(hashKey1, value1,
        TimeHelper::GetSysCurrentTime() + g_deviceB->GetLocalTimeOffset() + TIME_OFFSET, 1);

    /**
     * @tc.steps: step3. deviceA put {k1, delete} t3, t3 < t1
     */
    Key hashKey2;
    DistributedDBToolsUnitTest::CalcHash(key2, hashKey2);
    g_deviceC->PutData(hashKey2, value1,
        TimeHelper::GetSysCurrentTime() + g_deviceC->GetLocalTimeOffset() - TIME_OFFSET, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    /**
     * @tc.steps: step4. deviceA call pull sync
     * @tc.expected: step4. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step4. onComplete should be called, DeviceA have {k2. v2} don't have k1
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    Value value5;
    g_kvDelegatePtr->Get(key1, value5);
    EXPECT_TRUE(value5.empty());
    g_kvDelegatePtr->Get(key2, value5);
    EXPECT_TRUE(value5 == value2);
}

/**
 * @tc.name: Normal Sync 007
 * @tc.desc: Test normal push_pull sync for add data.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync007, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    status = g_kvDelegatePtr->Put(key1, value1);
    EXPECT_TRUE(status == OK);

    /**
     * @tc.steps: step1. deviceB put {k2, v2}
     */
    Key key2 = {'2'};
    Value value2 = {'2'};
    g_deviceB->PutData(key2, value2, 0, 0);

    /**
     * @tc.steps: step1. deviceB put {k3, v3}
     */
    Key key3 = {'3'};
    Value value3 = {'3'};
    g_deviceC->PutData(key3, value3, 0, 0);

    /**
     * @tc.steps: step4. deviceA call push_pull sync
     * @tc.expected: step4. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    ASSERT_TRUE(status == OK);

    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    /**
     * @tc.expected: step4. onComplete should be called, DeviceA have {k1. v1}, {k2, v2}, {k3, v3}
     *     deviceB received {k1. v1}, don't received k3, deviceC received {k1. v1}, don't received k2
     */
    Value value4;
    g_kvDelegatePtr->Get(key2, value4);
    EXPECT_TRUE(value4 == value2);
    g_kvDelegatePtr->Get(key3, value4);
    EXPECT_TRUE(value4 == value3);

    VirtualDataItem item1;
    g_deviceB->GetData(key1, item1);
    EXPECT_TRUE(item1.value == value1);
    item1.value.clear();
    g_deviceB->GetData(key3, item1);
    EXPECT_TRUE(item1.value.empty());

    VirtualDataItem item2;
    g_deviceC->GetData(key1, item2);
    EXPECT_TRUE(item2.value == value1);
    item2.value.clear();
    g_deviceC->GetData(key2, item2);
    EXPECT_TRUE(item2.value.empty());
}

/**
 * @tc.name: Normal Sync 008
 * @tc.desc: Test normal push_pull sync for update data.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync008, TestSize.Level2)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}, {k2, v2} t1
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    status = g_kvDelegatePtr->Put(key1, value1);
    ASSERT_TRUE(status == OK);

    Key key2 = {'2'};
    Value value2 = {'2'};
    status = g_kvDelegatePtr->Put(key2, value2);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step2. deviceB put {k1, v3} t2, t2 > t1
     */
    Value value3 = {'3'};
    g_deviceB->PutData(key1, value3,
        TimeHelper::GetSysCurrentTime() + g_deviceB->GetLocalTimeOffset() + TIME_OFFSET, 0);

    /**
     * @tc.steps: step3. deviceB put {k1, v4} t3, t4 <t1
     */
    Value value4 = {'4'};
    g_deviceC->PutData(key2, value4,
        TimeHelper::GetSysCurrentTime() + g_deviceC->GetLocalTimeOffset() - TIME_OFFSET, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));

    /**
     * @tc.steps: step4. deviceA call push_pull sync
     * @tc.expected: step4. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    ASSERT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    /**
     * @tc.expected: step4. onComplete should be called, DeviceA have {k1. v3}, {k2, v2}
     *     deviceB have {k1. v3}, deviceC have {k2. v2}
     */
    Value value5;
    g_kvDelegatePtr->Get(key1, value5);
    EXPECT_EQ(value5, value3);
    g_kvDelegatePtr->Get(key2, value5);
    EXPECT_EQ(value5, value2);

    VirtualDataItem item1;
    g_deviceB->GetData(key1, item1);
    EXPECT_TRUE(item1.value == value3);
    item1.value.clear();
    g_deviceB->GetData(key2, item1);
    EXPECT_TRUE(item1.value == value2);

    VirtualDataItem item2;
    g_deviceC->GetData(key2, item2);
    EXPECT_TRUE(item2.value == value2);
}

/**
 * @tc.name: Normal Sync 009
 * @tc.desc: Test normal push_pull sync for delete data.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync009, TestSize.Level2)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}, {k2, v2} t1
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    status = g_kvDelegatePtr->Put(key1, value1);
    ASSERT_TRUE(status == OK);

    Key key2 = {'2'};
    Value value2 = {'2'};
    status = g_kvDelegatePtr->Put(key2, value2);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps: step2. deviceB put {k1, delete} t2, t2 > t1
     */
    Key hashKey1;
    DistributedDBToolsUnitTest::CalcHash(key1, hashKey1);
    g_deviceB->PutData(hashKey1, value1,
        TimeHelper::GetSysCurrentTime() + g_deviceB->GetLocalTimeOffset() + TIME_OFFSET, 1);

    /**
     * @tc.steps: step3. deviceB put {k1, delete} t3, t2 < t1
     */
    Key hashKey2;
    DistributedDBToolsUnitTest::CalcHash(key2, hashKey2);
    g_deviceC->PutData(hashKey2, value2,
        TimeHelper::GetSysCurrentTime() + g_deviceC->GetLocalTimeOffset() - TIME_OFFSET, 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    /**
     * @tc.steps: step4. deviceA call push_pull sync
     * @tc.expected: step4. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step4. onComplete should be called, DeviceA have {k1. delete}, {k2, v2}
     *     deviceB have {k2. v2}, deviceC have {k2. v2}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    Value value3;
    g_kvDelegatePtr->Get(key1, value3);
    EXPECT_TRUE(value3.empty());
    value3.clear();
    g_kvDelegatePtr->Get(key2, value3);
    EXPECT_TRUE(value3 == value2);

    VirtualDataItem item1;
    g_deviceB->GetData(key2, item1);
    EXPECT_TRUE(item1.value == value2);

    VirtualDataItem item2;
    g_deviceC->GetData(key2, item2);
    EXPECT_TRUE(item2.value == value2);
}

/**
 * @tc.name: Normal Sync 010
 * @tc.desc: Test sync failed by invalid devices.
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync010, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    std::string invalidDev = std::string(DBConstant::MAX_DEV_LENGTH + 1, '0');
    devices.push_back(DEVICE_A);
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(invalidDev);

    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    ASSERT_EQ(result.size(), devices.size());
    EXPECT_EQ(result[DEVICE_A], INVALID_ARGS);
    EXPECT_EQ(result[invalidDev], INVALID_ARGS);
    EXPECT_EQ(result[DEVICE_B], OK);
}

/**
 * @tc.name: Normal Sync 011
 * @tc.desc: Test sync with translated id.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, NormalSync011, TestSize.Level0)
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

    /**
     * @tc.steps: step2. ori dev will be append test
     * @tc.expected: step2. sync should return OK.
     */
    RuntimeConfig::SetTranslateToDeviceIdCallback([](const std::string &oriDevId, const StoreInfo &) {
        std::string dev = oriDevId + "test";
        LOGI("translate %s to %s", oriDevId.c_str(), dev.c_str());
        return dev;
    });
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    ASSERT_TRUE(status == OK);
    RuntimeConfig::SetTranslateToDeviceIdCallback(nullptr);

    /**
     * @tc.expected: step2. onComplete should be called, Send watermark should not be zero
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    WatermarkInfo info;
    CheckWatermark(g_deviceB->GetDeviceId(), g_kvDelegatePtr, info, false);
    info = {};
    std::string checkDev = g_deviceB->GetDeviceId() + "test";
    CheckWatermark(checkDev, g_kvDelegatePtr, info, false);
}

/**
 * @tc.name: Limit Data Sync 001
 * @tc.desc: Test sync limit key and value data
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, LimitDataSync001, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    Key key1;
    Value value1;
    DistributedDBToolsUnitTest::GetRandomKeyValue(key1, DBConstant::MAX_KEY_SIZE + 1);
    DistributedDBToolsUnitTest::GetRandomKeyValue(value1, DBConstant::MAX_VALUE_SIZE + 1);

    Key key2;
    Value value2;
    DistributedDBToolsUnitTest::GetRandomKeyValue(key2, DBConstant::MAX_KEY_SIZE);
    DistributedDBToolsUnitTest::GetRandomKeyValue(value2, DBConstant::MAX_VALUE_SIZE);

    /**
     * @tc.steps: step1. deviceB put {k1, v1}, K1 > 1k, v1 > 4M
     */
    g_deviceB->PutData(key1, value1, 0, 0);

    /**
     * @tc.steps: step2. deviceB put {k2, v2}, K2 = 1k, v2 = 4M
     */
    g_deviceC->PutData(key2, value2, 0, 0);

    /**
     * @tc.steps: step3. deviceA call pull sync from device B
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called.
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        if (pair.first == g_deviceB->GetDeviceId()) {
            EXPECT_TRUE(pair.second != OK);
        } else {
            EXPECT_TRUE(pair.second == OK);
        }
    }

    /**
     * @tc.steps: step4. deviceA call pull sync from deviceC
     * @tc.expected: step4. sync should return OK.
     */
    devices.clear();
    result.clear();
    devices.push_back(g_deviceC->GetDeviceId());
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step4. onComplete should be called, DeviceA have {k2. v2}, don't have {k1, v1}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    // Get value from A
    Value valueRead;
    EXPECT_TRUE(g_kvDelegatePtr->Get(key1, valueRead) != OK);
    valueRead.clear();
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, valueRead), OK);
    EXPECT_TRUE(valueRead == value2);
}

/**
 * @tc.name: Limit Data Sync 002
 * @tc.desc: Test PutBatch with invalid entries and then call sync.
 * @tc.type: FUNC
 * @tc.require: DTS2024012914038
 * @tc.author: mazhao
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, LimitDataSync002, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());
    Key legalKey;
    DistributedDBToolsUnitTest::GetRandomKeyValue(legalKey, DBConstant::MAX_KEY_SIZE); // 1K
    Value legalValue;
    DistributedDBToolsUnitTest::GetRandomKeyValue(legalValue, DBConstant::MAX_VALUE_SIZE); // 4M
    Value emptyValue; // 0k
    vector<Entry> illegalEntrys; // size is 512M + 1KB
    for (int i = 0; i < 127; i++) { // 127 * (legalValue + legalKey) is equal to 508M + 127KB < 512M.
        illegalEntrys.push_back({legalKey, legalValue});
    }
    for (int i = 0; i < 3970; i++) { // 3970 * legalKey is equal to 3970KB.
        illegalEntrys.push_back({legalKey, emptyValue});
    }
    /**
     * @tc.steps: step1. PutBatch with invalid entries inside which total length of the key and valud is more than 512M
     * @tc.expected: step1. PutBatch should return INVALID_ARGS.
     */
    EXPECT_EQ(g_kvDelegatePtr->PutBatch(illegalEntrys), INVALID_ARGS);
    /**
     * @tc.steps: step2. deviceA call push_pull sync
     * @tc.expected: step2. sync return OK and all statuses is OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    ASSERT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        printf("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
}

/**
 * @tc.name: Auto Sync 001
 * @tc.desc: Verify auto sync enable function.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, AutoSync001, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. enable auto sync
     * @tc.expected: step1, Pragma return OK.
     */
    bool autoSync = true;
    PragmaData data = static_cast<PragmaData>(&autoSync);
    DBStatus status = g_kvDelegatePtr->Pragma(AUTO_SYNC, data);
    ASSERT_EQ(status, OK);

    /**
     * @tc.steps: step2. deviceA put {k1, v1}, {k2, v2}
     */
    ASSERT_TRUE(g_kvDelegatePtr->Put(KEY_1, VALUE_1) == OK);
    ASSERT_TRUE(g_kvDelegatePtr->Put(KEY_2, VALUE_2) == OK);

    /**
     * @tc.steps: step3. sleep for data sync
     * @tc.expected: step3. deviceB,C has {k1, v1}, {k2, v2}
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    VirtualDataItem item;
    g_deviceB->GetData(KEY_1, item);
    EXPECT_EQ(item.value, VALUE_1);
    g_deviceB->GetData(KEY_2, item);
    EXPECT_EQ(item.value, VALUE_2);
    g_deviceC->GetData(KEY_1, item);
    EXPECT_EQ(item.value, VALUE_1);
    g_deviceC->GetData(KEY_2, item);
    EXPECT_EQ(item.value, VALUE_2);
}

/**
 * @tc.name: Auto Sync 002
 * @tc.desc: Verify auto sync disable function.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, AutoSync002, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. disable auto sync
     * @tc.expected: step1, Pragma return OK.
     */
    bool autoSync = false;
    PragmaData data = static_cast<PragmaData>(&autoSync);
    DBStatus status = g_kvDelegatePtr->Pragma(AUTO_SYNC, data);
    ASSERT_EQ(status, OK);

    /**
     * @tc.steps: step2. deviceB put {k1, v1}, deviceC put {k2, v2}
     */
    g_deviceB->PutData(KEY_1, VALUE_1, 0, 0);
    g_deviceC->PutData(KEY_2, VALUE_2, 0, 0);

    /**
     * @tc.steps: step3. sleep for data sync
     * @tc.expected: step3. deviceA don't have k1, k2.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    Value value3;
    EXPECT_TRUE(g_kvDelegatePtr->Get(KEY_1, value3) == NOT_FOUND);
    EXPECT_TRUE(g_kvDelegatePtr->Get(KEY_2, value3) == NOT_FOUND);
}

/**
 * @tc.name: Block Sync 001
 * @tc.desc: Verify block push sync function.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, BlockSync001, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    g_kvDelegatePtr->Put(KEY_1, VALUE_1);

    /**
     * @tc.steps: step2. deviceA call block push sync to deviceB & deviceC.
     * @tc.expected: step2. Sync return OK, devices status OK, deviceB & deivceC has {k1, v1}.
     */
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, true);
    ASSERT_EQ(status, OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item1;
    EXPECT_EQ(g_deviceB->GetData(KEY_1, item1), OK);
    EXPECT_EQ(item1.value, VALUE_1);
    VirtualDataItem item2;
    EXPECT_EQ(g_deviceC->GetData(KEY_1, item2), OK);
    EXPECT_EQ(item2.value, VALUE_1);
}

/**
 * @tc.name:  Block Sync 002
 * @tc.desc: Verify block pull sync function.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, BlockSync002, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceB put {k1, v1}, deviceC put {k2, v2}
     */
    g_deviceB->PutData(KEY_1, VALUE_1, 0, 0);
    g_deviceC->PutData(KEY_2, VALUE_2, 0, 0);

    /**
     * @tc.steps: step2. deviceA call block pull and pull sync to deviceB & deviceC.
     * @tc.expected: step2. Sync return OK, devices status OK, deviceA has {k1, v1}, {k2, v2}
     */
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, true);
    ASSERT_EQ(status, OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    Value value3;
    EXPECT_TRUE(g_kvDelegatePtr->Get(KEY_1, value3) == OK);
    EXPECT_TRUE(value3 == VALUE_1);
    EXPECT_TRUE(g_kvDelegatePtr->Get(KEY_2, value3) == OK);
    EXPECT_TRUE(value3 == VALUE_2);
}

/**
 * @tc.name:  Block Sync 003
 * @tc.desc: Verify block push and pull sync function.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, BlockSync003, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    g_kvDelegatePtr->Put(KEY_1, VALUE_1);

    /**
     * @tc.steps: step2. deviceB put {k1, v1}, deviceB put {k2, v2}
     */
    g_deviceB->PutData(KEY_2, VALUE_2, 0, 0);
    g_deviceC->PutData(KEY_3, VALUE_3, 0, 0);

    /**
     * @tc.steps: step3. deviceA call block pull and pull sync to deviceB & deviceC.
     * @tc.expected: step3. Sync return OK, devices status OK, deviceA has {k1, v1}, {k2, v2} {k3, v3}
     *      deviceB has {k1, v1}, {k2. v2} , deviceC has {k1, v1}, {k3, v3}
     */
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result, true);
    ASSERT_EQ(status, OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }

    VirtualDataItem item1;
    g_deviceB->GetData(KEY_1, item1);
    EXPECT_TRUE(item1.value == VALUE_1);
    g_deviceB->GetData(KEY_2, item1);
    EXPECT_TRUE(item1.value == VALUE_2);

    VirtualDataItem item2;
    g_deviceC->GetData(KEY_1, item2);
    EXPECT_TRUE(item2.value == VALUE_1);
    g_deviceC->GetData(KEY_3, item2);
    EXPECT_TRUE(item2.value == VALUE_3);

    Value value3;
    EXPECT_TRUE(g_kvDelegatePtr->Get(KEY_1, value3) == OK);
    EXPECT_TRUE(value3 == VALUE_1);
    EXPECT_TRUE(g_kvDelegatePtr->Get(KEY_2, value3) == OK);
    EXPECT_TRUE(value3 == VALUE_2);
    EXPECT_TRUE(g_kvDelegatePtr->Get(KEY_3, value3) == OK);
    EXPECT_TRUE(value3 == VALUE_3);
}

/**
 * @tc.name:  Block Sync 004
 * @tc.desc: Verify block sync function invalid args.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, BlockSync004, TestSize.Level2)
{
    std::vector<std::string> devices;

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    g_kvDelegatePtr->Put(KEY_1, VALUE_1);

    /**
     * @tc.steps: step2. deviceA call block push sync to deviceB & deviceC.
     * @tc.expected: step2. Sync return INVALID_ARGS
     */
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, true);
    EXPECT_EQ(status, INVALID_ARGS);

    /**
     * @tc.steps: step3. deviceB, deviceC offlinem and push deviceA sync to deviceB and deviceC.
     * @tc.expected: step3. Sync return OK, but the deviceB and deviceC are TIME_OUT
     */
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());
    g_deviceB->Offline();
    g_deviceC->Offline();

    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, true);
    EXPECT_EQ(status, OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == COMM_FAILURE);
    }
}

/**
 * @tc.name:  Block Sync 005
 * @tc.desc: Verify block sync function busy.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, BlockSync005, TestSize.Level2)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());
    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    g_kvDelegatePtr->Put(KEY_1, VALUE_1);

    /**
     * @tc.steps: step2. New a thread to deviceA call block push sync to deviceB & deviceC,
     *      but deviceB & C is blocked
     * @tc.expected: step2. Sync will be blocked util timeout, and then return OK
     */
    g_deviceB->Offline();
    g_deviceC->Offline();
    thread thread([devices]() {
        std::map<std::string, DBStatus> resultInner;
        DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, resultInner, true);
        EXPECT_EQ(status, OK);
    });
    thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    /**
     * @tc.steps: step3. sleep 1s and call sync.
     * @tc.expected: step3. Sync will return BUSY.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result, true);
    EXPECT_EQ(status, OK);
}

/**
  * @tc.name: SyncQueue001
  * @tc.desc: Invalid args check of Pragma GET_QUEUED_SYNC_SIZE SET_QUEUED_SYNC_LIMIT and
  * GET_QUEUED_SYNC_LIMIT, expect return INVALID_ARGS.
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, SyncQueue001, TestSize.Level3)
{
    /**
     * @tc.steps:step1. Set PragmaCmd to be GET_QUEUED_SYNC_SIZE, and set param to be null
     * @tc.expected: step1. Expect return INVALID_ARGS.
     */
    int *param = nullptr;
    PragmaData input = static_cast<PragmaData>(param);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_SIZE, input), INVALID_ARGS);

    /**
     * @tc.steps:step2. Set PragmaCmd to be SET_QUEUED_SYNC_LIMIT, and set param to be null
     * @tc.expected: step2. Expect return INVALID_ARGS.
     */
    input = static_cast<PragmaData>(param);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(SET_QUEUED_SYNC_LIMIT, input), INVALID_ARGS);

    /**
     * @tc.steps:step3. Set PragmaCmd to be GET_QUEUED_SYNC_LIMIT, and set param to be null
     * @tc.expected: step3. Expect return INVALID_ARGS.
     */
    input = static_cast<PragmaData>(param);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_LIMIT, input), INVALID_ARGS);

    /**
     * @tc.steps:step4. Set PragmaCmd to be SET_QUEUED_SYNC_LIMIT, and set param to be QUEUED_SYNC_LIMIT_MIN - 1
     * @tc.expected: step4. Expect return INVALID_ARGS.
     */
    int limit = DBConstant::QUEUED_SYNC_LIMIT_MIN - 1;
    input = static_cast<PragmaData>(&limit);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(SET_QUEUED_SYNC_LIMIT, input), INVALID_ARGS);

    /**
     * @tc.steps:step5. Set PragmaCmd to be SET_QUEUED_SYNC_LIMIT, and set param to be QUEUED_SYNC_LIMIT_MAX + 1
     * @tc.expected: step5. Expect return INVALID_ARGS.
     */
    limit = DBConstant::QUEUED_SYNC_LIMIT_MAX + 1;
    input = static_cast<PragmaData>(&limit);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(SET_QUEUED_SYNC_LIMIT, input), INVALID_ARGS);
}

/**
  * @tc.name: SyncQueue002
  * @tc.desc: Pragma GET_QUEUED_SYNC_LIMIT and SET_QUEUED_SYNC_LIMIT
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, SyncQueue002, TestSize.Level3)
{
    /**
     * @tc.steps:step1. Set PragmaCmd to be GET_QUEUED_SYNC_LIMIT,
     * @tc.expected: step1. Expect return OK, limit eq QUEUED_SYNC_LIMIT_DEFAULT.
     */
    int limit = 0;
    PragmaData input = static_cast<PragmaData>(&limit);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_LIMIT, input), OK);
    EXPECT_EQ(limit, DBConstant::QUEUED_SYNC_LIMIT_DEFAULT);

    /**
     * @tc.steps:step2. Set PragmaCmd to be SET_QUEUED_SYNC_LIMIT, and set param to be 50
     * @tc.expected: step2. Expect return OK.
     */
    limit = 50;
    input = static_cast<PragmaData>(&limit);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(SET_QUEUED_SYNC_LIMIT, input), OK);

    /**
     * @tc.steps:step3. Set PragmaCmd to be GET_QUEUED_SYNC_LIMIT,
     * @tc.expected: step3. Expect return OK, limit eq 50
     */
    limit = 0;
    input = static_cast<PragmaData>(&limit);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_LIMIT, input), OK);
    EXPECT_EQ(limit, 50);
}

/**
  * @tc.name: SyncQueue003
  * @tc.desc: sync queue test
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, SyncQueue003, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps:step1. Set PragmaCmd to be GET_QUEUED_SYNC_SIZE,
     * @tc.expected: step1. Expect return OK, size eq 0.
     */
    int size;
    PragmaData input = static_cast<PragmaData>(&size);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_SIZE, input), OK);
    EXPECT_EQ(size, 0);

    /**
     * @tc.steps:step2. deviceA put {k1, v1}
     */
    status = g_kvDelegatePtr->Put(KEY_1, VALUE_1);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps:step3. deviceA sync SYNC_MODE_PUSH_ONLY
     */
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY, nullptr, false);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps:step4. deviceA put {k2, v2}
     */
    status = g_kvDelegatePtr->Put(KEY_2, VALUE_2);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps:step5. deviceA sync SYNC_MODE_PUSH_ONLY
     */
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY, nullptr, false);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps:step6. deviceB put {k3, v3}
     */
    g_deviceB->PutData(KEY_3, VALUE_3, 0, 0);

    /**
     * @tc.steps:step7. deviceA put {k4, v4}
     */
    status = g_kvDelegatePtr->Put(KEY_4, VALUE_4);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps:step8. deviceA sync SYNC_MODE_PUSH_PULL
     */
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_PULL, nullptr, false);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps:step9. Set PragmaCmd to be GET_QUEUED_SYNC_SIZE,
     * @tc.expected: step1. Expect return OK, 0 <= size <= 4
     */
    EXPECT_EQ(g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_SIZE, input), OK);
    ASSERT_TRUE((size >= 0) && (size <= 4));

    /**
     * @tc.steps:step10. deviceB put {k5, v5}
     */
    g_deviceB->PutData(KEY_5, VALUE_5, 0, 0);

    /**
     * @tc.steps:step11. deviceA call sync and wait
     * @tc.expected: step11. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step11. onComplete should be called, DeviceA,B,C have {k1,v1}~ {KEY_5,VALUE_5}
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        EXPECT_TRUE(pair.second == OK);
    }
    VirtualDataItem item;
    g_deviceB->GetData(KEY_1, item);
    EXPECT_TRUE(item.value == VALUE_1);
    g_deviceB->GetData(KEY_2, item);
    EXPECT_TRUE(item.value == VALUE_2);
    g_deviceB->GetData(KEY_3, item);
    EXPECT_TRUE(item.value == VALUE_3);
    g_deviceB->GetData(KEY_4, item);
    EXPECT_TRUE(item.value == VALUE_4);
    g_deviceB->GetData(KEY_5, item);
    EXPECT_TRUE(item.value == VALUE_5);
    Value value;
    EXPECT_EQ(g_kvDelegatePtr->Get(KEY_3, value), OK);
    EXPECT_EQ(VALUE_3, value);
    EXPECT_EQ(g_kvDelegatePtr->Get(KEY_5, value), OK);
    EXPECT_EQ(VALUE_5, value);
}

/**
  * @tc.name: SyncQueue004
  * @tc.desc: sync queue full test
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, SyncQueue004, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps:step1. deviceB C block
     */
    g_communicatorAggregator->SetBlockValue(true);

    /**
     * @tc.steps:step2. deviceA put {k1, v1}
     */
    status = g_kvDelegatePtr->Put(KEY_1, VALUE_1);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.steps:step3. deviceA sync QUEUED_SYNC_LIMIT_DEFAULT times
     * @tc.expected: step3. Expect return OK
     */
    for (int i = 0; i < DBConstant::QUEUED_SYNC_LIMIT_DEFAULT; i++) {
        status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY, nullptr, false);
        ASSERT_TRUE(status == OK);
    }

    /**
     * @tc.steps:step4. deviceA sync
     * @tc.expected: step4. Expect return BUSY
     */
    status = g_kvDelegatePtr->Sync(devices, SYNC_MODE_PUSH_ONLY, nullptr, false);
    ASSERT_TRUE(status == BUSY);
    g_communicatorAggregator->SetBlockValue(false);
}

/**
  * @tc.name: SyncQueue005
  * @tc.desc: block sync queue test
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, SyncQueue005, TestSize.Level3)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());
    /**
     * @tc.steps:step1. New a thread to deviceA call block push sync to deviceB & deviceC,
     *      but deviceB & C is offline
     * @tc.expected: step1. Sync will be blocked util timeout, and then return OK
     */
    g_deviceB->Offline();
    g_deviceC->Offline();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));

    /**
     * @tc.steps:step2. deviceA put {k1, v1}
     */
    g_kvDelegatePtr->Put(KEY_1, VALUE_1);

    std::mutex lockMutex;
    std::condition_variable conditionVar;

    std::thread threadFirst([devices]() {
        std::map<std::string, DBStatus> resultInner;
        DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, resultInner, true);
        EXPECT_EQ(status, OK);
    });
    threadFirst.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    /**
     * @tc.steps:step3. New a thread to deviceA call block push sync to deviceB & deviceC,
     *      but deviceB & C is offline
     * @tc.expected: step2. Sync will be blocked util timeout, and then return OK
     */
    std::thread threadSecond([devices, &lockMutex, &conditionVar]() {
        std::map<std::string, DBStatus> resultInner;
        DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, resultInner, true);
        EXPECT_EQ(status, OK);
        std::unique_lock<mutex> lockInner(lockMutex);
        conditionVar.notify_one();
    });
    threadSecond.detach();

    /**
     * @tc.steps:step4. Set PragmaCmd to be GET_QUEUED_SYNC_SIZE,
     * @tc.expected: step1. Expect return OK, size eq 0.
     */
    int size;
    PragmaData input = static_cast<PragmaData>(&size);
    EXPECT_EQ(g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_SIZE, input), OK);
    EXPECT_EQ(size, 0);

    /**
     * @tc.steps:step5. wait exit
     */
    std::unique_lock<mutex> lock(lockMutex);
    auto now = std::chrono::system_clock::now();
    conditionVar.wait_until(lock, now + 2 * INT8_MAX * 1000ms);
}

/**
  * @tc.name: CalculateSyncData001
  * @tc.desc: Test sync data whose device never synced before
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, CalculateSyncData001, TestSize.Level3)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    size_t dataSize = g_kvDelegatePtr->GetSyncDataSize(DEVICE_B);
    uint32_t serialHeadLen = 8u;
    EXPECT_EQ(static_cast<uint32_t>(dataSize), 0u + serialHeadLen);
    uint32_t keySize = 256u;
    uint32_t valuesize = 1024u;
    uint32_t itemCount = 10u;
    CalculateDataTest(itemCount, keySize, valuesize);
}

/**
  * @tc.name: CalculateSyncData002
  * @tc.desc: Test sync data whose device synced before, but sync data is less than 1M
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, CalculateSyncData002, TestSize.Level3)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    Key key1 = {'1'};
    Value value1 = {'1'};
    EXPECT_EQ(g_kvDelegatePtr->Put(key1, value1), OK);

    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        EXPECT_TRUE(pair.second == OK);
    }

    uint32_t keySize = 256u;
    uint32_t valuesize = 512u;
    uint32_t itemCount = 20u;
    CalculateDataTest(itemCount, keySize, valuesize);
}

/**
  * @tc.name: CalculateSyncData003
  * @tc.desc: Test sync data whose device synced before, but sync data is larger than 1M
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, CalculateSyncData003, TestSize.Level3)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    Key key1 = {'1'};
    Value value1 = {'1'};
    EXPECT_EQ(g_kvDelegatePtr->Put(key1, value1), OK);

    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        EXPECT_TRUE(pair.second == OK);
    }
    uint32_t keySize = 256u;
    uint32_t valuesize = 1024u;
    uint32_t itemCount = 2048u;
    CalculateDataTest(itemCount, keySize, valuesize);
}

/**
  * @tc.name: CalculateSyncData004
  * @tc.desc: Test invalid device when call GetSyncDataSize interface
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, CalculateSyncData004, TestSize.Level3)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    std::string device;
    EXPECT_EQ(g_kvDelegatePtr->GetSyncDataSize(device), 0u);
}

/**
  * @tc.name: CalculateSyncData005
  * @tc.desc: Test CalculateSyncData and rekey Concurrently
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, CalculateSyncData005, TestSize.Level3)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    size_t dataSize = 0;
    Key key1 = {'1'};
    Value value1 = {'1'};
    EXPECT_EQ(g_kvDelegatePtr->Put(key1, value1), OK);
    std::thread thread1([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        CipherPassword passwd; // random password
        vector<uint8_t> passwdBuffer(10, 45);  // 10 and 45 as random password.
        passwd.SetValue(passwdBuffer.data(), passwdBuffer.size());
        g_kvDelegatePtr->Rekey(passwd);
    });
    std::thread thread2([&dataSize, &key1, &value1]() {
        dataSize = g_kvDelegatePtr->GetSyncDataSize(DEVICE_B);
        if (dataSize > 0) {
            uint32_t expectedDataSize = (key1.size() + value1.size());
            uint32_t externalSize = 70u;
            uint32_t serialHeadLen = 8u;
            ASSERT_GE(static_cast<uint32_t>(dataSize), expectedDataSize);
            ASSERT_LE(static_cast<uint32_t>(dataSize), serialHeadLen + expectedDataSize + externalSize);
        }
    });
    thread1.join();
    thread2.join();
}

/**
 * @tc.name: GetWaterMarkInfo001
 * @tc.desc: Test invalid dev for get water mark info.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerP2PSimpleSyncTest, GetWaterMarkInfo001, TestSize.Level0)
{
    std::string dev;
    auto res = g_kvDelegatePtr->GetWatermarkInfo(dev);
    EXPECT_EQ(res.first, INVALID_ARGS);
    EXPECT_EQ(res.second.sendMark, 0u);
    EXPECT_EQ(res.second.receiveMark, 0u);

    dev = std::string(DBConstant::MAX_DEV_LENGTH + 1, 'a');
    res = g_kvDelegatePtr->GetWatermarkInfo(dev);
    EXPECT_EQ(res.first, INVALID_ARGS);
    EXPECT_EQ(res.second.sendMark, 0u);
    EXPECT_EQ(res.second.receiveMark, 0u);
}
} // namespace