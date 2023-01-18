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
#include "single_ver_data_sync.h"
#include "single_ver_kv_sync_task_context.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    class TestSingleVerKvSyncTaskContext : public SingleVerKvSyncTaskContext {
    public:
        TestSingleVerKvSyncTaskContext() = default;
    };
    string g_testDir;
    const string STORE_ID = "kv_stroe_sync_test";
    const int64_t TIME_OFFSET = 5000000;
    const int WAIT_TIME = 1000;
    const int WAIT_5_SECONDS = 5000;
    const int WAIT_30_SECONDS = 30000;
    const int WAIT_36_SECONDS = 36000;
    const std::string DEVICE_A = "real_device";
    const std::string DEVICE_B = "deviceB";
    const std::string DEVICE_C = "deviceC";
    const std::string CREATE_SYNC_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS sync_data(" \
        "key         BLOB NOT NULL," \
        "value       BLOB," \
        "timestamp   INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "device      BLOB," \
        "ori_device  BLOB," \
        "hash_key    BLOB PRIMARY KEY NOT NULL," \
        "w_timestamp INT);";

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

    void PullSyncTest()
    {
        DBStatus status = OK;
        std::vector<std::string> devices;
        devices.push_back(g_deviceB->GetDeviceId());

        Key key = {'1'};
        Key key2 = {'2'};
        Value value = {'1'};
        g_deviceB->PutData(key, value, 0, 0);
        g_deviceB->PutData(key2, value, 1, 0);

        std::map<std::string, DBStatus> result;
        status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
        ASSERT_TRUE(status == OK);

        ASSERT_TRUE(result.size() == devices.size());
        for (const auto &pair : result) {
            LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
        Value value3;
        EXPECT_EQ(g_kvDelegatePtr->Get(key, value3), OK);
        EXPECT_EQ(value3, value);
        EXPECT_EQ(g_kvDelegatePtr->Get(key2, value3), OK);
        EXPECT_EQ(value3, value);
    }

    void CrudTest()
    {
        vector<Entry> entries;
        int totalSize = 10;
        for (int i = 0; i < totalSize; i++) {
            Entry entry;
            entry.key.push_back(i);
            entry.value.push_back('2');
            entries.push_back(entry);
        }
        EXPECT_TRUE(g_kvDelegatePtr->PutBatch(entries) == OK);
        for (const auto &entry : entries) {
            Value resultvalue;
            EXPECT_TRUE(g_kvDelegatePtr->Get(entry.key, resultvalue) == OK);
            EXPECT_TRUE(resultvalue == entry.value);
        }
        for (int i = 0; i < totalSize / 2; i++) {
            g_kvDelegatePtr->Delete(entries[i].key);
            Value resultvalue;
            EXPECT_TRUE(g_kvDelegatePtr->Get(entries[i].key, resultvalue) == NOT_FOUND);
        }
        for (int i = totalSize / 2; i < totalSize; i++) {
            Value value = entries[i].value;
            value.push_back('x');
            EXPECT_TRUE(g_kvDelegatePtr->Put(entries[i].key, value) == OK);
            Value resultvalue;
            EXPECT_TRUE(g_kvDelegatePtr->Get(entries[i].key, resultvalue) == OK);
            EXPECT_TRUE(resultvalue == value);
        }
    }
}

class DistributedDBSingleVerP2PSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerP2PSyncTest::SetUpTestCase(void)
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

void DistributedDBSingleVerP2PSyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerP2PSyncTest::SetUp(void)
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
                                return true;};
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
}

void DistributedDBSingleVerP2PSyncTest::TearDown(void)
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
 * @tc.name: Normal Sync 001
 * @tc.desc: Test normal push sync for add data.
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S SR000CQE0B
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync001, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync002, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync003, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync004, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync005, TestSize.Level2)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync006, TestSize.Level2)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync007, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync008, TestSize.Level2)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync009, TestSize.Level2)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, NormalSync010, TestSize.Level1)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(DEVICE_A);
    devices.push_back(g_deviceB->GetDeviceId());

    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    ASSERT_EQ(result.size(), devices.size());
    EXPECT_EQ(result[DEVICE_A], INVALID_ARGS);
    EXPECT_EQ(result[DEVICE_B], OK);
}

/**
 * @tc.name: Limit Data Sync 001
 * @tc.desc: Test sync limit key and value data
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, LimitDataSync001, TestSize.Level1)
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
 * @tc.name: Device Offline Sync 001
 * @tc.desc: Test push sync when device offline
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DeviceOfflineSync001, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}, {k2, v2}, {k3 delete}, {k4,v2}
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    ASSERT_TRUE(g_kvDelegatePtr->Put(key1, value1) == OK);

    Key key2 = {'2'};
    Value value2 = {'2'};
    ASSERT_TRUE(g_kvDelegatePtr->Put(key2, value2) == OK);

    Key key3 = {'3'};
    Value value3 = {'3'};
    ASSERT_TRUE(g_kvDelegatePtr->Put(key3, value3) == OK);
    ASSERT_TRUE(g_kvDelegatePtr->Delete(key3) == OK);

    Key key4 = {'4'};
    Value value4 = {'4'};
    ASSERT_TRUE(g_kvDelegatePtr->Put(key4, value4) == OK);

    /**
     * @tc.steps: step2. deviceB offline
     */
    g_deviceB->Offline();

    /**
     * @tc.steps: step3. deviceA call pull sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceB status is timeout
     *     deviceC has {k1, v1}, {k2, v2}, {k3 delete}, {k4,v4}
     */
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        if (pair.first == DEVICE_B) {
            EXPECT_TRUE(pair.second == COMM_FAILURE);
        } else {
            EXPECT_TRUE(pair.second == OK);
        }
    }
    VirtualDataItem item;
    g_deviceC->GetData(key1, item);
    EXPECT_TRUE(item.value == value1);
    item.value.clear();
    g_deviceC->GetData(key2, item);
    EXPECT_TRUE(item.value == value2);
    item.value.clear();
    Key hashKey;
    DistributedDBToolsUnitTest::CalcHash(key3, hashKey);
    EXPECT_TRUE(g_deviceC->GetData(hashKey, item) == -E_NOT_FOUND);
    item.value.clear();
    g_deviceC->GetData(key4, item);
    EXPECT_TRUE(item.value == value4);
}

/**
 * @tc.name: Device Offline Sync 002
 * @tc.desc: Test pull sync when device offline
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DeviceOfflineSync002, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    devices.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceB put {k1, v1}
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    g_deviceB->PutData(key1, value1, 0, 0);

    /**
     * @tc.steps: step2. deviceB offline
     */
    g_deviceB->Offline();

    /**
     * @tc.steps: step3. deviceC put {k2, v2}, {k3, delete}, {k4, v4}
     */
    Key key2 = {'2'};
    Value value2 = {'2'};
    g_deviceC->PutData(key2, value2, 0, 0);

    Key key3 = {'3'};
    Value value3 = {'3'};
    g_deviceC->PutData(key3, value3, 0, 1);

    Key key4 = {'4'};
    Value value4 = {'4'};
    g_deviceC->PutData(key4, value4, 0, 0);

    /**
     * @tc.steps: step2. deviceA call pull sync
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    DBStatus status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    ASSERT_TRUE(status == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceB status is timeout
     *     deviceA has {k2, v2}, {k3 delete}, {k4,v4}
     */
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        if (pair.first == DEVICE_B) {
            EXPECT_TRUE(pair.second == COMM_FAILURE);
        } else {
            EXPECT_TRUE(pair.second == OK);
        }
    }

    Value value5;
    EXPECT_TRUE(g_kvDelegatePtr->Get(key1, value5) != OK);
    g_kvDelegatePtr->Get(key2, value5);
    EXPECT_EQ(value5, value2);
    EXPECT_TRUE(g_kvDelegatePtr->Get(key3, value5) != OK);
    g_kvDelegatePtr->Get(key4, value5);
    EXPECT_EQ(value5, value4);
}

/**
 * @tc.name: Auto Sync 001
 * @tc.desc: Verify auto sync enable function.
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: xushaohua
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, AutoSync001, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, AutoSync002, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, BlockSync001, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, BlockSync002, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, BlockSync003, TestSize.Level1)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, BlockSync004, TestSize.Level2)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, BlockSync005, TestSize.Level2)
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
    thread thread([devices](){
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncQueue001, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncQueue002, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncQueue003, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncQueue004, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncQueue005, TestSize.Level3)
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

    std::thread threadFirst([devices](){
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
    std::thread threadSecond([devices, &lockMutex, &conditionVar](){
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
  * @tc.name: PermissionCheck001
  * @tc.desc: deviceA PermissionCheck not pass test, SYNC_MODE_PUSH_ONLY
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: wangchuanqing
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck001, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck002, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck003, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck004, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck005, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck006, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck007, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck008, TestSize.Level3)
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
HWTEST_F(DistributedDBSingleVerP2PSyncTest, PermissionCheck009, TestSize.Level3)
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
  * @tc.name: SaveDataNotify001
  * @tc.desc: Test SaveDataNotify function, delay < 30s should sync ok, > 36 should timeout
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: xushaohua
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SaveDataNotify001, TestSize.Level3)
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
     * @tc.steps: step2. deviceB set sava data dely 5s
     */
    g_deviceB->SetSaveDataDelayTime(WAIT_5_SECONDS);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK. onComplete should be called, deviceB sync success.
     */
    std::map<std::string, DBStatus> result;
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    ASSERT_TRUE(result[DEVICE_B] == OK);

    /**
     * @tc.steps: step4. deviceB set sava data dely 30s and put {k1, v1}
     */
    g_deviceB->SetSaveDataDelayTime(WAIT_30_SECONDS);
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);
     /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK. onComplete should be called, deviceB sync success.
     */
    result.clear();
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    ASSERT_TRUE(result[DEVICE_B] == OK);

    /**
     * @tc.steps: step4. deviceB set sava data dely 36s and put {k1, v1}
     */
    g_deviceB->SetSaveDataDelayTime(WAIT_36_SECONDS);
    status = g_kvDelegatePtr->Put(key, value);
    ASSERT_TRUE(status == OK);
    /**
     * @tc.steps: step5. deviceA call sync and wait
     * @tc.expected: step5. sync should return OK. onComplete should be called, deviceB sync TIME_OUT.
     */
    result.clear();
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    ASSERT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    ASSERT_TRUE(result[DEVICE_B] == TIME_OUT);
}

/**
  * @tc.name: SametimeSync001
  * @tc.desc: Test 2 device sync with each other
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SametimeSync001, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    int responseCount = 0;
    int requestCount = 0;
    Key key = {'1'};
    Value value = {'1'};
    /**
     * @tc.steps: step1. make sure deviceB send pull firstly and response_pull secondly
     * @tc.expected: step1. deviceA put data when finish push task. put data should return OK.
     */
    g_communicatorAggregator->RegOnDispatch([&responseCount, &requestCount, &key, &value](
        const std::string &target, DistributedDB::Message *msg) {
        if (target == "real_device" && msg->GetMessageId() == DATA_SYNC_MESSAGE) {
            if (msg->GetMessageType() == TYPE_RESPONSE) {
                responseCount++;
                if (responseCount == 1) { // 1 is the ack which B response A's push task
                    EXPECT_EQ(g_kvDelegatePtr->Put(key, value), DBStatus::OK);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                } else if (responseCount == 2) { // 2 is the ack which B response A's response_pull task
                    msg->SetErrorNo(E_FEEDBACK_COMMUNICATOR_NOT_FOUND);
                }
            } if (msg->GetMessageType() == TYPE_REQUEST) {
                requestCount++;
                if (requestCount == 1) { // 1 is A push task
                    std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep 2 sec
                }
            }
        }
    });
    /**
     * @tc.steps: step2. deviceA,deviceB sync to each other at same time
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    std::thread subThread([]{
        g_deviceB->Sync(DistributedDB::SYNC_MODE_PULL_ONLY, true);
    });
    status = g_tool.SyncTest(g_kvDelegatePtr, devices, DistributedDB::SYNC_MODE_PUSH_PULL, result);
    subThread.join();
    g_communicatorAggregator->RegOnDispatch(nullptr);

    EXPECT_TRUE(status == OK);
    ASSERT_TRUE(result.size() == devices.size());
    EXPECT_TRUE(result[DEVICE_B] == OK);
    Value actualValue;
    g_kvDelegatePtr->Get(key, actualValue);
    EXPECT_EQ(actualValue, value);
}

/**
  * @tc.name: SametimeSync002
  * @tc.desc: Test 2 device sync with each other with water error
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SametimeSync002, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    g_kvDelegatePtr->Put({'k', '1'}, {'v', '1'});
    /**
     * @tc.steps: step1. make sure deviceA push data failed and increase water mark
     * @tc.expected: step1. deviceA push failed with timeout
     */
    g_communicatorAggregator->RegOnDispatch([](const std::string &target, DistributedDB::Message *msg) {
        ASSERT_NE(msg, nullptr);
        if (target == DEVICE_B && msg->GetMessageId() == QUERY_SYNC_MESSAGE) {
            msg->SetMessageId(INVALID_MESSAGE_ID);
        }
    });
    std::map<std::string, DBStatus> result;
    auto callback = [&result](const std::map<std::string, DBStatus> &map) {
        result = map;
    };
    Query query = Query::Select().PrefixKey({'k', '1'});
    EXPECT_EQ(g_kvDelegatePtr->Sync(devices, DistributedDB::SYNC_MODE_PUSH_ONLY, callback, query, true), OK);
    ASSERT_TRUE(result.size() == devices.size());
    EXPECT_TRUE(result[DEVICE_B] == TIME_OUT);
    /**
     * @tc.steps: step2. A push to B with query2, sleep 1s for waiting step3
     * @tc.expected: step2. sync should return OK.
     */
    g_communicatorAggregator->RegOnDispatch([](const std::string &target, DistributedDB::Message *msg) {
        ASSERT_NE(msg, nullptr);
        if (target == DEVICE_B && msg->GetMessageId() == QUERY_SYNC_MESSAGE) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    std::thread subThread([&devices] {
        std::map<std::string, DBStatus> result;
        auto callback = [&result](const std::map<std::string, DBStatus> &map) {
            result = map;
        };
        Query query = Query::Select().PrefixKey({'k', '2'});
        LOGD("Begin PUSH");
        EXPECT_EQ(g_kvDelegatePtr->Sync(devices, DistributedDB::SYNC_MODE_PUSH_ONLY, callback, query, true), OK);
        ASSERT_TRUE(result.size() == devices.size());
        EXPECT_TRUE(result[DEVICE_A] == OK);
    });
    /**
     * @tc.steps: step3. B pull to A when A is in push task
     * @tc.expected: step3. sync should return OP_FINISHED_ALL.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::map<std::string, int> virtualResult;
    g_deviceB->Sync(DistributedDB::SYNC_MODE_PULL_ONLY, query,
        [&virtualResult](const std::map<std::string, int> &map) {
            virtualResult = map;
        }, true);
    EXPECT_TRUE(status == OK);
    ASSERT_EQ(virtualResult.size(), devices.size());
    EXPECT_EQ(virtualResult[DEVICE_A], SyncOperation::OP_FINISHED_ALL);
    g_communicatorAggregator->RegOnDispatch(nullptr);
    subThread.join();
}

/**
 * @tc.name: DatabaseOnlineCallback001
 * @tc.desc: check database status notify online callback
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S SR000CQE0B
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DatabaseOnlineCallback001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. SetStoreStatusNotifier
     * @tc.expected: step1. SetStoreStatusNotifier ok
     */
    std::string targetDev = "DEVICE_X";
    bool isCheckOk = false;
    auto databaseStatusNotifyCallback = [targetDev, &isCheckOk] (std::string userId,
        std::string appId, std::string storeId, const std::string deviceId, bool onlineStatus) -> void {
        if (userId == USER_ID && appId == APP_ID && storeId == STORE_ID && deviceId == targetDev &&
            onlineStatus == true) {
            isCheckOk = true;
        }};
    g_mgr.SetStoreStatusNotifier(databaseStatusNotifyCallback);
    /**
     * @tc.steps: step2. trigger device online
     * @tc.expected: step2. check callback ok
     */
    g_communicatorAggregator->OnlineDevice(targetDev);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME / 20));
    EXPECT_EQ(isCheckOk, true);
    StoreStatusNotifier nullCallback;
    g_mgr.SetStoreStatusNotifier(nullCallback);
}

/**
 * @tc.name: DatabaseOfflineCallback001
 * @tc.desc: check database status notify online callback
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S SR000CQE0B
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DatabaseOfflineCallback001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. SetStoreStatusNotifier
     * @tc.expected: step1. SetStoreStatusNotifier ok
     */
    std::string targetDev = "DEVICE_X";
    bool isCheckOk = false;
    auto databaseStatusNotifyCallback = [targetDev, &isCheckOk] (std::string userId,
        std::string appId, std::string storeId, const std::string deviceId, bool onlineStatus) -> void {
        if (userId == USER_ID && appId == APP_ID && storeId == STORE_ID && deviceId == targetDev &&
            onlineStatus == false) {
            isCheckOk = true;
        }};
    g_mgr.SetStoreStatusNotifier(databaseStatusNotifyCallback);
    /**
     * @tc.steps: step2. trigger device offline
     * @tc.expected: step2. check callback ok
     */
    g_communicatorAggregator->OfflineDevice(targetDev);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME / 20));
    EXPECT_EQ(isCheckOk, true);
    StoreStatusNotifier nullCallback;
    g_mgr.SetStoreStatusNotifier(nullCallback);
}

/**
  * @tc.name: CloseSync001
  * @tc.desc: Test 2 delegate close when sync
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, CloseSync001, TestSize.Level3)
{
    DBStatus status = OK;
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. make sure A sync start
     */
    bool sleep = false;
    g_communicatorAggregator->RegOnDispatch([&sleep](const std::string &target, DistributedDB::Message *msg) {
        if (!sleep) {
            sleep = true;
            std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep 2s for waiting close db
        }
    });

    KvStoreNbDelegate* kvDelegatePtrA = nullptr;
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore(STORE_ID, option, [&status, &kvDelegatePtrA](DBStatus s, KvStoreNbDelegate *delegate) {
        status = s;
        kvDelegatePtrA = delegate;
    });
    EXPECT_EQ(status, OK);
    EXPECT_NE(kvDelegatePtrA, nullptr);

    Key key = {'k'};
    Value value = {'v'};
    kvDelegatePtrA->Put(key, value);
    std::map<std::string, DBStatus> result;
    auto callback = [&result](const std::map<std::string, DBStatus>& statusMap) {
        result = statusMap;
    };
    /**
     * @tc.steps: step2. deviceA sync and then close
     * @tc.expected: step2. sync should abort and data don't exist in B
     */
    std::thread closeThread([&kvDelegatePtrA]() {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s for waiting sync start
        EXPECT_EQ(g_mgr.CloseKvStore(kvDelegatePtrA), OK);
    });
    EXPECT_EQ(kvDelegatePtrA->Sync(devices, SYNC_MODE_PUSH_ONLY, callback, false), OK);
    LOGD("Sync finish");
    closeThread.join();
    std::this_thread::sleep_for(std::chrono::seconds(5)); // sleep 5s for waiting sync finish
    EXPECT_EQ(result.size(), 0u);
    VirtualDataItem actualValue;
    EXPECT_EQ(g_deviceB->GetData(key, actualValue), -E_NOT_FOUND);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
  * @tc.name: CloseSync002
  * @tc.desc: Test 1 delegate close when in time sync
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, CloseSync002, TestSize.Level3)
{
    /**
     * @tc.steps: step1. invalid time sync packet from A
     */
    g_communicatorAggregator->RegOnDispatch([](const std::string &target, DistributedDB::Message *msg) {
        ASSERT_NE(msg, nullptr);
        if (target == DEVICE_B && msg->GetMessageId() == TIME_SYNC_MESSAGE && msg->GetMessageType() == TYPE_REQUEST) {
            msg->SetMessageId(INVALID_MESSAGE_ID);
            LOGD("Message is invalid");
        }
    });
    Timestamp currentTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    g_deviceB->PutData({'k'}, {'v'}, currentTime, 0);

    /**
     * @tc.steps: step2. B PUSH to A and A close after 1s
     * @tc.expected: step2. A closing time cost letter than 4s
     */
    std::thread closingThread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        LOGD("Begin Close");
        Timestamp beginTime;
        (void)OS::GetCurrentSysTimeInMicrosecond(beginTime);
        ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
        Timestamp endTime;
        (void)OS::GetCurrentSysTimeInMicrosecond(endTime);
        EXPECT_LE(static_cast<int>(endTime - beginTime), 4 * 1000 * 1000); // waiting 4 * 1000 * 1000 us
        LOGD("End Close");
    });
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    closingThread.join();

    /**
     * @tc.steps: step3. remove db
     * @tc.expected: step3. remove ok
     */
    g_kvDelegatePtr = nullptr;
    DBStatus status = g_mgr.DeleteKvStore(STORE_ID);
    LOGD("delete kv store status %d", status);
    ASSERT_TRUE(status == OK);
    g_communicatorAggregator->RegOnDispatch(nullptr);
}

/**
  * @tc.name: OrderbyWriteTimeSync001
  * @tc.desc: sync query with order by writeTime
  * @tc.type: FUNC
  * @tc.require: AR000H5VLO
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, OrderbyWriteTimeSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. deviceA subscribe query with order by write time
     * * @tc.expected: step1. interface return not support
    */
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    Query query = Query::Select().PrefixKey({'k'}).OrderByWriteTime(true);
    EXPECT_EQ(g_kvDelegatePtr->Sync(devices, DistributedDB::SYNC_MODE_PUSH_ONLY, nullptr, query, true), NOT_SUPPORT);
}

/**
  * @tc.name: EncryptedAlgoUpgrade001
  * @tc.desc: Test upgrade encrypted db can sync normally
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, EncryptedAlgoUpgrade001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. clear db
     * * @tc.expected: step1. interface return ok
    */
    if (g_kvDelegatePtr != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
        g_kvDelegatePtr = nullptr;
        DBStatus status = g_mgr.DeleteKvStore(STORE_ID);
        LOGD("delete kv store status %d", status);
        ASSERT_TRUE(status == OK);
    }

    CipherPassword passwd;
    std::vector<uint8_t> passwdVect = {'p', 's', 'd', '1'};
    passwd.SetValue(passwdVect.data(), passwdVect.size());
    /**
     * @tc.steps: step2. open old db by sql
     * * @tc.expected: step2. interface return ok
    */
    std::string identifier = DBCommon::GenerateIdentifierId(STORE_ID, APP_ID, USER_ID);
    std::string hashDir = DBCommon::TransferHashString(identifier);
    std::string hexHashDir = DBCommon::TransferStringToHex(hashDir);
    std::string dbPath = g_testDir + "/" + hexHashDir + "/single_ver";
    ASSERT_TRUE(DBCommon::CreateDirectory(g_testDir + "/" + hexHashDir) == OK);
    ASSERT_TRUE(DBCommon::CreateDirectory(dbPath) == OK);
    std::vector<std::string> dbDir {DBConstant::MAINDB_DIR, DBConstant::METADB_DIR, DBConstant::CACHEDB_DIR};
    for (const auto &item : dbDir) {
        ASSERT_TRUE(DBCommon::CreateDirectory(dbPath + "/" + item) == OK);
    }
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    sqlite3 *db;
    std::string fileUrl = dbPath + "/" + DBConstant::MAINDB_DIR + "/" + DBConstant::SINGLE_VER_DATA_STORE + ".db";
    ASSERT_TRUE(sqlite3_open_v2(fileUrl.c_str(), &db, flag, nullptr) == SQLITE_OK);
    SQLiteUtils::SetKeyInner(db, CipherType::AES_256_GCM, passwd, DBConstant::DEFAULT_ITER_TIMES);
    /**
     * @tc.steps: step3. create table and close
     * * @tc.expected: step3. interface return ok
    */
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, CREATE_SYNC_TABLE_SQL) == E_OK);
    sqlite3_close_v2(db);
    db = nullptr;
    LOGI("create old db success");
    /**
     * @tc.steps: step4. get kvstore
     * * @tc.expected: step4. interface return ok
    */
    KvStoreNbDelegate::Option option;
    option.isEncryptedDb = true;
    option.cipher = CipherType::AES_256_GCM;
    option.passwd = passwd;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    /**
     * @tc.steps: step5. sync ok
     * * @tc.expected: step5. interface return ok
    */
    PullSyncTest();
    /**
     * @tc.steps: step5. crud ok
     * * @tc.expected: step5. interface return ok
    */
    CrudTest();
}

/**
  * @tc.name: RemoveDeviceData002
  * @tc.desc: test remove device data before sync
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, RemoveDeviceData002, TestSize.Level1)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    /**
     * @tc.steps: step1. sync deviceB data to A and check data
     * * @tc.expected: step1. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    Value value = {'1'};
    Timestamp currentTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key1, value, currentTime, 0), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key2, value, currentTime, 0), E_OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    Value actualValue;
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    actualValue.clear();
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    /**
     * @tc.steps: step2. call RemoveDeviceData
     * * @tc.expected: step2. interface return ok
    */
    g_kvDelegatePtr->RemoveDeviceData(g_deviceB->GetDeviceId());
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), NOT_FOUND);
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, actualValue), NOT_FOUND);
    /**
     * @tc.steps: step3. sync to device A again and check data
     * * @tc.expected: step3. sync ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    actualValue.clear();
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    actualValue.clear();
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, value);
}

/**
  * @tc.name: DataSync001
  * @tc.desc: Test Data Sync when Initialize
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DataSync001, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    ASSERT_TRUE(dataSync != nullptr);
    std::shared_ptr<Metadata> inMetadata = nullptr;
    std::string deviceId;
    Message message;
    VirtualSingleVerSyncDBInterface tmpInterface;
    VirtualCommunicator tmpCommunicator(deviceId, g_communicatorAggregator);
    EXPECT_EQ(dataSync->Initialize(nullptr, nullptr, inMetadata, deviceId), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->Initialize(&tmpInterface, nullptr, inMetadata, deviceId), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->Initialize(&tmpInterface, &tmpCommunicator, inMetadata, deviceId), -E_INVALID_ARGS);
    delete dataSync;
}

/**
  * @tc.name: DataSync002
  * @tc.desc: Test active sync with invalid param in DataSync Class
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DataSync002, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    ASSERT_TRUE(dataSync != nullptr);
    Message message;
    EXPECT_EQ(dataSync->TryContinueSync(nullptr, &message), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->TryContinueSync(nullptr, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->PushStart(nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->PushPullStart(nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->PullRequestStart(nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->PullResponseStart(nullptr), -E_INVALID_ARGS);
    delete dataSync;
}

/**
  * @tc.name: DataSync003
  * @tc.desc: Test receive invalid request data packet in DataSync Class
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DataSync003, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    ASSERT_TRUE(dataSync != nullptr);
    uint64_t tmpMark = 0;
    Message message;
    EXPECT_EQ(dataSync->DataRequestRecv(nullptr, nullptr, tmpMark), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->DataRequestRecv(nullptr, &message, tmpMark), -E_INVALID_ARGS);
    delete dataSync;
}

/**
  * @tc.name: DataSync004
  * @tc.desc: Test receive invalid ack packet in DataSync Class
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DataSync004, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    ASSERT_TRUE(dataSync != nullptr);
    Message message;
    TestSingleVerKvSyncTaskContext tmpContext;
    EXPECT_EQ(dataSync->AckPacketIdCheck(nullptr), false);
    EXPECT_EQ(dataSync->AckPacketIdCheck(&message), false);
    EXPECT_EQ(dataSync->AckRecv(&tmpContext, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->AckRecv(nullptr, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->AckRecv(nullptr, &message), -E_INVALID_ARGS);
    delete dataSync;
}

/**
  * @tc.name: DataSync005
  * @tc.desc: Test receive invalid notify packet in DataSync Class
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DataSync005, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    ASSERT_TRUE(dataSync != nullptr);
    dataSync->SendSaveDataNotifyPacket(nullptr, 0, 0, 0, TIME_SYNC_MESSAGE);
    delete dataSync;
}

/**
  * @tc.name: DataSync006
  * @tc.desc: Test control start with invalid param in DataSync Class
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DataSync006, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    ASSERT_TRUE(dataSync != nullptr);
    TestSingleVerKvSyncTaskContext tmpContext;
    EXPECT_EQ(dataSync->ControlCmdStart(nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->ControlCmdStart(&tmpContext), -E_INVALID_ARGS);
    std::shared_ptr<SubscribeManager> subManager = std::make_shared<SubscribeManager>();
    tmpContext.SetSubscribeManager(subManager);
    tmpContext.SetMode(SyncModeType::INVALID_MODE);
    EXPECT_EQ(dataSync->ControlCmdStart(&tmpContext), -E_INVALID_ARGS);
    std::set<Key> Keys = {{'a'}, {'b'}};
    Query query = Query::Select().InKeys(Keys);
    QuerySyncObject innerQuery(query);
    tmpContext.SetQuery(innerQuery);
    tmpContext.SetMode(SyncModeType::SUBSCRIBE_QUERY);
    EXPECT_EQ(dataSync->ControlCmdStart(&tmpContext), -E_NOT_SUPPORT);
    delete dataSync;
    subManager = nullptr;
}

/**
  * @tc.name: DataSync007
  * @tc.desc: Test receive invalid control packet in DataSync Class
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DataSync007, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    ASSERT_TRUE(dataSync != nullptr);
    Message message;
    ControlRequestPacket packet;
    TestSingleVerKvSyncTaskContext tmpContext;
    EXPECT_EQ(dataSync->ControlCmdRequestRecv(nullptr, &message), -E_INVALID_ARGS);
    message.SetCopiedObject(packet);
    EXPECT_EQ(dataSync->ControlCmdRequestRecv(nullptr, &message), -E_INVALID_ARGS);
    delete dataSync;
}

/**
  * @tc.name: DataSync008
  * @tc.desc: Test pull null msg in dataQueue in DataSync Class
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, DataSync008, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    ASSERT_TRUE(dataSync != nullptr);
    dataSync->PutDataMsg(nullptr);
    delete dataSync;
}

/**
 * @tc.name: SyncRetry001
 * @tc.desc: use sync retry sync use push
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncRetry001, TestSize.Level3)
{
    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. set sync retry
     * @tc.expected: step1, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvDelegatePtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step2. deviceA put {k1, v1}, {k2, v2}
     */
    ASSERT_TRUE(g_kvDelegatePtr->Put(KEY_1, VALUE_1) == OK);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    ASSERT_TRUE(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result) == OK);

    /**
     * @tc.expected: step4. onComplete should be called, and status is time_out
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: SyncRetry002
 * @tc.desc: use sync retry sync use pull
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncRetry002, TestSize.Level3)
{
    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE, 4u);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. set sync retry
     * @tc.expected: step1, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvDelegatePtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    ASSERT_TRUE(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result) == OK);

    /**
     * @tc.expected: step3. onComplete should be called, and status is time_out
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == TIME_OUT);
    }
    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: SyncRetry003
 * @tc.desc: use sync retry sync use push by compress
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncRetry003, TestSize.Level3)
{
    if (g_kvDelegatePtr != nullptr) {
        ASSERT_EQ(g_mgr.CloseKvStore(g_kvDelegatePtr), OK);
        g_kvDelegatePtr = nullptr;
    }
    /**
     * @tc.steps: step1. open db use Compress
     * @tc.expected: step1, Pragma return OK.
     */
    KvStoreNbDelegate::Option option;
    option.isNeedCompressOnSync = true;
    option.compressionRate = 70;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);

    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. set sync retry
     * @tc.expected: step2, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvDelegatePtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step3. deviceA put {k1, v1}, {k2, v2}
     */
    ASSERT_TRUE(g_kvDelegatePtr->Put(KEY_1, VALUE_1) == OK);

    /**
     * @tc.steps: step4. deviceA call sync and wait
     * @tc.expected: step4. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    ASSERT_TRUE(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result) == OK);

    /**
     * @tc.expected: step5. onComplete should be called, and status is time_out
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: SyncRetry004
 * @tc.desc: use query sync retry sync use push
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, SyncRetry004, TestSize.Level3)
{
    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. set sync retry
     * @tc.expected: step1, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvDelegatePtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step2. deviceA put {k1, v1}, {k2, v2}
     */
    for (int i = 0; i < 5; i++) {
        Key key = DistributedDBToolsUnitTest::GetRandPrefixKey({'a', 'b'}, 128); // rand num 1024 for test
        Value value;
        DistributedDBToolsUnitTest::GetRandomKeyValue(value, 256u);
        EXPECT_EQ(g_kvDelegatePtr->Put(key, value), OK);
    }

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    std::vector<uint8_t> prefixKey({'a', 'b'});
    Query query = Query::Select().PrefixKey(prefixKey);
    ASSERT_TRUE(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query) == OK);

    /**
     * @tc.expected: step4. onComplete should be called, and status is time_out
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        LOGD("dev %s, status %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: ReSetWatchDogTest001
 * @tc.desc: trigger resetWatchDog while pull
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, ReSetWaterDogTest001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. put 10 key/value
     * @tc.expected: step1, put return OK.
     */
    for (int i = 0; i < 5; i++) {
        Key key = DistributedDBToolsUnitTest::GetRandPrefixKey({'a', 'b'}, 1024); // rand num 1024 for test
        Value value;
        DistributedDBToolsUnitTest::GetRandomKeyValue(value, 10 * 50 * 1024u);
        EXPECT_EQ(g_kvDelegatePtr->Put(key, value), OK);
    }
    /**
     * @tc.steps: step1. SetDeviceMtuSize
     * @tc.expected: step1, return OK.
     */
    g_communicatorAggregator->SetDeviceMtuSize(DEVICE_A, 50 * 1024u); // 4k
    g_communicatorAggregator->SetDeviceMtuSize(DEVICE_B, 50 * 1024u); // 4k
    /**
     * @tc.steps: step2. deviceA,deviceB sync to each other at same time
     * @tc.expected: step2. sync should return OK.
     */
    g_deviceB->Sync(DistributedDB::SYNC_MODE_PULL_ONLY, true);
    g_communicatorAggregator->SetDeviceMtuSize(DEVICE_A, 5 * 1024u * 1024u); // 4k
    g_communicatorAggregator->SetDeviceMtuSize(DEVICE_B, 5 * 1024u * 1024u); // 4k
}

/**
  * @tc.name: RebuildSync001
  * @tc.desc: rebuild db and sync again
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, RebuildSync001, TestSize.Level3)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    /**
     * @tc.steps: step1. sync deviceB data to A and check data
     * * @tc.expected: step1. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    Value value = {'1'};
    Timestamp currentTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key1, value, currentTime, 0), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key2, value, currentTime, 0), E_OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);

    Value actualValue;
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    actualValue.clear();
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    /**
     * @tc.steps: step2. delete db and rebuild
     * * @tc.expected: step2. interface return ok
    */
    g_mgr.CloseKvStore(g_kvDelegatePtr);
    g_kvDelegatePtr = nullptr;
    ASSERT_TRUE(g_mgr.DeleteKvStore(STORE_ID) == OK);
    KvStoreNbDelegate::Option option;
    g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
    ASSERT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    /**
     * @tc.steps: step3. sync to device A again
     * * @tc.expected: step3. sync ok
    */
    value = {'2'};
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key1, value, currentTime, 0), E_OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    /**
     * @tc.steps: step4. check data in device A
     * * @tc.expected: step4. check ok
    */
    actualValue.clear();
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, value);
}

/**
  * @tc.name: RebuildSync002
  * @tc.desc: test clear remote data when receive data
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, RebuildSync002, TestSize.Level1)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    /**
     * @tc.steps: step1. device A SET_WIPE_POLICY
     * * @tc.expected: step1. interface return ok
    */
    int pragmaData = 2; // 2 means enable
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvDelegatePtr->Pragma(SET_WIPE_POLICY, input) == OK);
    /**
     * @tc.steps: step2. sync deviceB data to A and check data
     * * @tc.expected: step2. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    Key key3 = {'3'};
    Key key4 = {'4'};
    Value value = {'1'};
    Timestamp currentTime;
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key1, value, currentTime, 0), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key2, value, currentTime, 0), E_OK);
    EXPECT_EQ(g_kvDelegatePtr->Put(key3, value), OK);
    /**
     * @tc.steps: step3. deviceA call pull sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> result;
    ASSERT_TRUE(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result) == OK);

    /**
     * @tc.expected: step4. onComplete should be called, check data
     */
    ASSERT_TRUE(result.size() == devices.size());
    for (const auto &pair : result) {
        EXPECT_TRUE(pair.second == OK);
    }
    Value actualValue;
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    /**
     * @tc.steps: step5. device B rebuild and put some data
     * * @tc.expected: step5. rebuild ok
    */
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterfaceB != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key3, value, currentTime, 0), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(currentTime);
    EXPECT_EQ(g_deviceB->PutData(key4, value, currentTime, 0), E_OK);
    /**
     * @tc.steps: step6. sync to device A again and check data
     * * @tc.expected: step6. sync ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(g_kvDelegatePtr->Get(key3, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    EXPECT_EQ(g_kvDelegatePtr->Get(key4, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), NOT_FOUND);
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, actualValue), NOT_FOUND);
}

/**
  * @tc.name: RebuildSync003
  * @tc.desc: test clear history data when receive ack
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, RebuildSync003, TestSize.Level1)
{
    ASSERT_TRUE(g_kvDelegatePtr != nullptr);
    /**
     * @tc.steps: step1. sync deviceB data to A and check data
     * * @tc.expected: step1. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    Key key3 = {'3'};
    Key key4 = {'4'};
    Value value = {'1'};
    EXPECT_EQ(g_deviceB->PutData(key1, value, 1u, 0), E_OK); // 1: timestamp
    EXPECT_EQ(g_deviceB->PutData(key2, value, 2u, 0), E_OK); // 2: timestamp
    EXPECT_EQ(g_kvDelegatePtr->Put(key3, value), OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_PULL, true), E_OK);
    Value actualValue;
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(key3, item), E_OK);
    EXPECT_EQ(item.value, value);
    /**
     * @tc.steps: step2. device B sync to device A,but make it failed
     * * @tc.expected: step2. interface return ok
    */
    EXPECT_EQ(g_deviceB->PutData(key4, value, 3u, 0), E_OK); // 3: timestamp
    g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_A, DATA_SYNC_MESSAGE);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    /**
     * @tc.steps: step3. device B set delay send time
     * * @tc.expected: step3. interface return ok
    */
    std::set<std::string> delayDevice = {DEVICE_B};
    g_communicatorAggregator->SetSendDelayInfo(3000u, DATA_SYNC_MESSAGE, 1u, 0u, delayDevice); // delay 3000ms one time
    /**
     * @tc.steps: step4. device A rebuilt, device B push data to A and set clear remote data mark into context after 1s
     * * @tc.expected: step4. interface return ok
    */
    std::thread thread1([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // wait 1s
        g_deviceB->SetClearRemoteStaleData(true);
        if (g_kvDelegatePtr != nullptr) {
            g_mgr.CloseKvStore(g_kvDelegatePtr);
            g_kvDelegatePtr = nullptr;
        }
        ASSERT_TRUE(g_mgr.DeleteKvStore(STORE_ID) == OK);
        KvStoreNbDelegate::Option option;
        g_mgr.GetKvStore(STORE_ID, option, g_kvDelegateCallback);
        ASSERT_TRUE(g_kvDelegateStatus == OK);
        ASSERT_TRUE(g_kvDelegatePtr != nullptr);
        std::map<std::string, DBStatus> result;
        std::vector<std::string> devices = {g_deviceB->GetDeviceId()};
        g_communicatorAggregator->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
        ASSERT_TRUE(g_tool.SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result) == OK);
    });
    /**
     * @tc.steps: step5. device B sync to A, make it clear history data and check data
     * * @tc.expected: step5. interface return ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    thread1.join();
    EXPECT_EQ(g_deviceB->GetData(key3, item), -E_NOT_FOUND);
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    EXPECT_EQ(g_kvDelegatePtr->Get(key2, actualValue), OK);
    EXPECT_EQ(actualValue, value);
    g_communicatorAggregator->ResetSendDelayInfo();
}

/**
  * @tc.name: GetSyncDataFail001
  * @tc.desc: test get sync data failed when sync
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBSingleVerP2PSyncTest, GetSyncDataFail001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. device B set get data errCode control and put some data
     * * @tc.expected: step1. interface return ok
    */
    g_deviceB->SetGetDataErrCode(1, -E_BUSY, true);
    Key key1 = {'1'};
    Value value = {'1'};
    EXPECT_EQ(g_deviceB->PutData(key1, value, 1u, 0), E_OK); // 1: timestamp
    /**
     * @tc.steps: step2. device B sync to device A and check data
     * * @tc.expected: step2. interface return ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    Value actualValue;
    EXPECT_EQ(g_kvDelegatePtr->Get(key1, actualValue), NOT_FOUND);
    g_deviceB->ResetDataControl();
}