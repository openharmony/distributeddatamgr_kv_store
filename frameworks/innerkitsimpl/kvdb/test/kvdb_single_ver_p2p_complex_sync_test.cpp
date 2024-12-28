/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "mock_sync_task_context.h"
#include "platform_specific.h"
#include "single_ver_data_sync.h"
#include "single_ver_kv_sync_task_context.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    class TestSingleVerKvSyncTask : public SingleVerKvSyncTaskContext {
    public:
        TestSingleVerKvSyncTask() = default;
    };
    string g_testDir;
    const string STORE_ID = "kv_stroe_complex_sync_test";
    const int WAIT_TIME = 1000;
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
        "w_timestamp INT," \
        "modify_time INT," \
        "create_time INT" \
        ");";

    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreConfig g_config;
    KvDBToolsUnitTest g_tool;
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    KvStoreNbDelegate* g_kvPtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAggre = nullptr;
    KvDevice *g_deviceB = nullptr;
    KvDevice *g_deviceC = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = bind(&KvDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvPtr));

    void PullSyncTest()
    {
        DBStatus res = OK;
        std::vector<std::string> devicesVec;
        devicesVec.push_back(g_deviceB->GetDeviceId());

        Key key = {'1'};
        Key key2 = {'2'};
        Value value = {'1'};
        g_deviceB->PutData(key, value, 0, 0);
        g_deviceB->PutData(key2, value, 1, 0);

        std::map<std::string, DBStatus> res;
        res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PULL_ONLY, res);
        EXCEPT_TRUE(res == OK);

        EXCEPT_TRUE(res.size() == devicesVec.size());
        for (const auto &pair : res) {
            LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
            EXPECT_TRUE(pair.second == OK);
        }
        Value value3;
        EXPECT_EQ(g_kvPtr->Get(key, value3), OK);
        EXPECT_EQ(value3, value);
        EXPECT_EQ(g_kvPtr->Get(key2, value3), OK);
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
        EXPECT_TRUE(g_kvPtr->PutBatch(entries) == OK);
        for (const auto &entry : entries) {
            Value resvalue;
            EXPECT_TRUE(g_kvPtr->Get(entry.key, resvalue) == OK);
            EXPECT_TRUE(resvalue == entry.value);
        }
        for (int i = 0; i < totalSize / 2; i++) { // 2: Half of the total
            g_kvPtr->Delete(entries[i].key);
            Value resvalue;
            EXPECT_TRUE(g_kvPtr->Get(entries[i].key, resvalue) == NOT_FOUND);
        }
        for (int i = totalSize / 2; i < totalSize; i++) {
            Value value = entries[i].value;
            value.push_back('x');
            EXPECT_TRUE(g_kvPtr->Put(entries[i].key, value) == OK);
            Value resvalue;
            EXPECT_TRUE(g_kvPtr->Get(entries[i].key, resvalue) == OK);
            EXPECT_TRUE(resvalue == value);
        }
    }

    void DataSync005()
    {
        ASSERT_NE(g_communicatorAggre, nullptr);
        SingleVerDataSync *sync = new (std::nothrow) SingleVerDataSync();
        EXCEPT_TRUE(sync != nullptr);
        sync->SendSaveDataNotifyPacket(nullptr, 0, 0, 0, TIME_SYNC_MESSAGE);
        EXPECT_EQ(g_communicatorAggre->GetOnlineDevices().size(), 3u); // 3 online dev
        delete sync;
    }

    void DataSync008()
    {
        SingleVerDataSync *sync = new (std::nothrow) SingleVerDataSync();
        EXCEPT_TRUE(sync != nullptr);
        auto context = new (std::nothrow) MockSyncTaskContext();
        sync->PutDataMsg(nullptr);
        bool isNeedHandle = false;
        bool isContinue = false;
        EXPECT_EQ(sync->MoveNextDataMsg(context, isNeedHandle, isContinue), nullptr);
        EXPECT_EQ(isNeedHandle, false);
        EXPECT_EQ(isContinue, false);
        delete sync;
        delete context;
    }

    void ReSetWaterDogTest001()
    {
        /**
         * @tc.steps: step1. put 10 key/value
         * @tc.expected: step1, put return OK.
         */
        for (int i = 0; i < 5; i++) { // put 5 key
            Key key = KvDBToolsUnitTest::GetRandPrefixKey({'a', 'b'}, 1024); // rand num 1024 for test
            Value value;
            KvDBToolsUnitTest::GetRandomKeyValue(value, 10 * 50 * 1024u); // 10 * 50 * 1024 = 500k
            EXPECT_EQ(g_kvPtr->Put(key, value), OK);
        }
        /**
         * @tc.steps: step2. SetDeviceMtuSize
         * @tc.expected: step2, return OK.
         */
        g_communicatorAggre->SetDeviceMtuSize(DEVICE_A, 50 * 1024u); // 50 * 1024u = 50k
        g_communicatorAggre->SetDeviceMtuSize(DEVICE_B, 50 * 1024u); // 50 * 1024u = 50k
        /**
         * @tc.steps: step3. deviceA,deviceB sync to each other at same time
         * @tc.expected: step3. sync should return OK.
         */
        EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PULL_ONLY, true), E_OK);
        g_communicatorAggre->SetDeviceMtuSize(DEVICE_A, 5 * 1024u * 1024u); // 5 * 1024u * 1024u = 5m
        g_communicatorAggre->SetDeviceMtuSize(DEVICE_B, 5 * 1024u * 1024u); // 5 * 1024u * 1024u = 5m
    }
}

class KvDBSingleVerP2PComplexSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBSingleVerP2PComplexSyncTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    KvDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);

    string dir = g_testDir + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }

    g_communicatorAggre = new (std::nothrow) VirtualCommunicatorAggregator();
    EXCEPT_TRUE(g_communicatorAggre != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggre);
}

void KvDBSingleVerP2PComplexSyncTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (KvDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void KvDBSingleVerP2PComplexSyncTest::SetUp(void)
{
    KvDBToolsUnitTest::PrintTestCaseInfo();
    /**
     * @tc.setup: create virtual device B and C, and get a KvStoreNbDelegate as deviceA
     */
    KvStoreNbDelegate::Option op;
    g_mgr.GetKvStore(STORE_ID, op, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvDelegateStatus == OK);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    g_deviceB = new (std::nothrow) KvDevice(DEVICE_B);
    EXCEPT_TRUE(g_deviceB != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceA = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    EXCEPT_TRUE(syncInterfaceA != nullptr);
    EXCEPT_EQ(g_deviceB->Initialize(g_communicatorAggre, syncInterfaceA), E_OK);

    g_deviceC = new (std::nothrow) KvDevice(DEVICE_C);
    EXCEPT_TRUE(g_deviceC != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    EXCEPT_TRUE(syncInterfaceB != nullptr);
    EXCEPT_EQ(g_deviceC->Initialize(g_communicatorAggre, syncInterfaceB), E_OK);

    auto permissionCheckCallback = [] (const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, uint8_t flag) -> bool {
            return true;
        };
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(permissionCheckCallback), OK);
}

void KvDBSingleVerP2PComplexSyncTest::TearDown(void)
{
    /**
     * @tc.teardown: Release device A, B, C
     */
    if (g_kvPtr != nullptr) {
        EXCEPT_EQ(g_mgr.CloseKvStore(g_kvPtr), OK);
        g_kvPtr = nullptr;
        DBStatus res = g_mgr.DeleteKvStore(STORE_ID);
        LOGD("delete kv store res %d", res);
        EXCEPT_TRUE(res == OK);
    }
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    if (g_deviceC != nullptr) {
        delete g_deviceC;
        g_deviceC = nullptr;
    }
    PermissionCheckCallbackV2 callback;
    EXPECT_EQ(g_mgr.SetPermissionCheckCallback(callback), OK);
}

/**
  * @tc.name: SaveDataNotify001
  * @tc.desc: Test SaveDataNotify function, delay < 30s should sync ok, > 36 should timeout
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: xushaohua
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SaveDataNotify001, TestSize.Level3)
{
    DBStatus res = OK;
    const int waitFiveSeconds = 5000;
    const int waitThirtySeconds = 30000;
    const int waitThirtySixSeconds = 36000;
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    res = g_kvPtr->Put(key, value);
    EXCEPT_TRUE(res == OK);

    /**
     * @tc.steps: step2. deviceB set sava data dely 5s
     */
    g_deviceB->SetSaveDataDelayTime(waitFiveSeconds);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK. onComplete should be called, deviceB sync success.
     */
    std::map<std::string, DBStatus> res;
    res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res);
    EXCEPT_TRUE(res == OK);
    EXCEPT_TRUE(res.size() == devicesVec.size());
    EXCEPT_TRUE(res[DEVICE_B] == OK);
}

/**
  * @tc.name: SaveDataNotify001
  * @tc.desc: Test SaveDataNotify function, delay < 30s should sync ok, > 36 should timeout
  * @tc.type: FUNC
  * @tc.require: AR000D4876
  * @tc.author: xushaohua
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SaveDataNotify100, TestSize.Level3)
{
    /**
     * @tc.steps: step4. deviceB set sava data dely 30s and put {k1, v1}
     */
    g_deviceB->SetSaveDataDelayTime(waitThirtySeconds);
    res = g_kvPtr->Put(key, value);
    EXCEPT_TRUE(res == OK);
     /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK. onComplete should be called, deviceB sync success.
     */
    res.clear();
    res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res);
    EXCEPT_TRUE(res == OK);
    EXCEPT_TRUE(res.size() == devicesVec.size());
    EXCEPT_TRUE(res[DEVICE_B] == OK);

    /**
     * @tc.steps: step4. deviceB set sava data dely 36s and put {k1, v1}
     */
    g_deviceB->SetSaveDataDelayTime(waitThirtySixSeconds);
    res = g_kvPtr->Put(key, value);
    EXCEPT_TRUE(res == OK);
    /**
     * @tc.steps: step5. deviceA call sync and wait
     * @tc.expected: step5. sync should return OK. onComplete should be called, deviceB sync TIME_OUT.
     */
    res.clear();
    res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res);
    EXCEPT_TRUE(res == OK);
    EXCEPT_TRUE(res.size() == devicesVec.size());
    EXCEPT_TRUE(res[DEVICE_B] == TIME_OUT);
}

/**
  * @tc.name: SametimeSync001
  * @tc.desc: Test 2 device sync with each other
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SametimeSync001, TestSize.Level3)
{
    DBStatus res = OK;
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    int responseCount = 0;
    int requestCount = 0;
    Key key = {'1'};
    Value value = {'1'};
    /**
     * @tc.steps: step1. make sure deviceB send pull firstly and response_pull secondly
     * @tc.expected: step1. deviceA put data when finish push task. put data should return OK.
     */
    g_communicatorAggre->RegOnDispatch([&responseCount, &requestCount, &key, &value](
        const std::string &target, DistributedDB::Message *message) {
        if (target != "real_device" || message->GetMessageId() != DATA_SYNC_MESSAGE) {
            return;
        }

        if (message->GetMessageType() == TYPE_RESPONSE) {
            responseCount++;
            if (responseCount == 1) { // 1 is the ack which B response A's push task
                EXPECT_EQ(g_kvPtr->Put(key, value), DBStatus::OK);
                std::sleep_for(std::chrono::seconds(1));
            } else if (responseCount == 2) { // 2 is the ack which B response A's response_pull task
                message->SetErrorNo(E_FEEDBACK_COMMUNICATOR_NOT_FOUND);
            }
        } else if (message->GetMessageType() == TYPE_REQUEST) {
            requestCount++;
            if (requestCount == 1) { // 1 is A push task
                std::sleep_for(std::chrono::seconds(2)); // sleep 2 sec
            }
        }
    });
    /**
     * @tc.steps: step2. deviceA,deviceB sync to each other at same time
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> res;
    std::thread subThread([]{
        g_deviceB->Sync(DistributedDB::SYNC_MODE_PULL_ONLY, true);
    });
    res = g_tool.SyncTest(g_kvPtr, devicesVec, DistributedDB::SYNC_MODE_PUSH_PULL, res);
    subThread.join();
    g_communicatorAggre->RegOnDispatch(nullptr);

    EXPECT_TRUE(res == OK);
    EXCEPT_TRUE(res.size() == devicesVec.size());
    EXPECT_TRUE(res[DEVICE_B] == OK);
    Value actual;
    g_kvPtr->Get(key, actual);
    EXPECT_EQ(actual, value);
}

/**
  * @tc.name: SametimeSync002
  * @tc.desc: Test 2 device sync with each other with water error
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SametimeSync002, TestSize.Level3)
{
    DBStatus res = OK;
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    g_kvPtr->Put({'k', '1'}, {'v', '1'});
    /**
     * @tc.steps: step1. make sure deviceA push data failed and increase water mark
     * @tc.expected: step1. deviceA push failed with timeout
     */
    g_communicatorAggre->RegOnDispatch([](const std::string &target, DistributedDB::Message *msg) {
        ASSERT_NE(msg, nullptr);
        if (target == DEVICE_B && msg->GetMessageId() == QUERY_SYNC_MESSAGE) {
            msg->SetMessageId(INVALID_MESSAGE_ID);
        }
    });
    std::map<std::string, DBStatus> res;
    auto callback = [&res](const std::map<std::string, DBStatus> &map) {
        res = map;
    };
    Query sql = Query::Select().PrefixKey({'k', '1'});
    EXPECT_EQ(g_kvPtr->Sync(devicesVec, DistributedDB::SYNC_MODE_PUSH_ONLY, callback, sql, true), OK);
    EXCEPT_TRUE(res.size() == devicesVec.size());
    EXPECT_TRUE(res[DEVICE_B] == TIME_OUT);
    /**
     * @tc.steps: step2. A push to B with query2, sleep 1s for waiting step3
     * @tc.expected: step2. sync should return OK.
     */
    g_communicatorAggre->RegOnDispatch([](const std::string &target, DistributedDB::Message *msg) {
        ASSERT_NE(msg, nullptr);
        if (target == DEVICE_B && msg->GetMessageId() == QUERY_SYNC_MESSAGE) {
            std::sleep_for(std::chrono::seconds(1));
        }
    });
    std::thread subThread([&devicesVec] {
        std::map<std::string, DBStatus> res;
        auto callback = [&res](const std::map<std::string, DBStatus> &map) {
            res = map;
        };
        Query sql = Query::Select().PrefixKey({'k', '2'});
        LOGD("Begin PUSH");
        EXPECT_EQ(g_kvPtr->Sync(devicesVec, DistributedDB::SYNC_MODE_PUSH_ONLY, callback, sql, true), OK);
        EXCEPT_TRUE(res.size() == devicesVec.size());
        EXPECT_TRUE(res[DEVICE_B] == OK);
    });
    /**
     * @tc.steps: step3. B pull to A when A is in push task
     * @tc.expected: step3. sync should return OP_FINISHED_ALL.
     */
    std::sleep_for(std::chrono::milliseconds(100));
    std::map<std::string, int> virtualResult;
    g_deviceB->Sync(DistributedDB::SYNC_MODE_PULL_ONLY, sql,
        [&virtualResult](const std::map<std::string, int> &map) {
            virtualResult = map;
        }, true);
    EXPECT_TRUE(res == OK);
    EXCEPT_EQ(virtualResult.size(), devicesVec.size());
    EXPECT_EQ(virtualResult[DEVICE_A], SyncOperation::OP_FINISHED_ALL);
    g_communicatorAggre->RegOnDispatch(nullptr);
    subThread.join();
}

/**
 * @tc.name: DatabaseOnlineCallback001
 * @tc.desc: check database res notify online callback
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S SR000CQE0B
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DatabaseOnlineCallback001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. SetStoreStatusNotifier
     * @tc.expected: step1. SetStoreStatusNotifier ok
     */
    std::string targetDev = "DEVICE_X";
    bool isOk = false;
    auto databaseStatusNotify = [targetDev, &isOk] (const std::string &userId,
        const std::string &appId, const std::string &storeId, const std::string &deviceId, bool onlineStatus) -> void {
        if (userId == USER_ID && appId == APP_ID && storeId == STORE_ID && deviceId == targetDev &&
            onlineStatus == true) {
            isOk = true;
        }};
    g_mgr.SetStoreStatusNotifier(databaseStatusNotify);
    /**
     * @tc.steps: step2. trigger device online
     * @tc.expected: step2. check callback ok
     */
    g_communicatorAggre->OnlineDevice(targetDev);
    std::sleep_for(std::chrono::milliseconds(WAIT_TIME / 20));
    EXPECT_EQ(isOk, true);
    StoreStatusNotifier callback;
    g_mgr.SetStoreStatusNotifier(callback);
}

/**
 * @tc.name: DatabaseOfflineCallback001
 * @tc.desc: check database res notify online callback
 * @tc.type: FUNC
 * @tc.require: AR000CQS3S SR000CQE0B
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DatabaseOfflineCallback001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. SetStoreStatusNotifier
     * @tc.expected: step1. SetStoreStatusNotifier ok
     */
    std::string targetDev = "DEVICE_X";
    bool isOk = false;
    auto databaseStatusNotify = [targetDev, &isOk] (const std::string &userId,
        const std::string &appId, const std::string &storeId, const std::string &deviceId, bool onlineStatus) -> void {
        if (userId == USER_ID && appId == APP_ID && storeId == STORE_ID && deviceId == targetDev &&
            onlineStatus == false) {
            isOk = true;
        }};
    g_mgr.SetStoreStatusNotifier(databaseStatusNotify);
    /**
     * @tc.steps: step2. trigger device offline
     * @tc.expected: step2. check callback ok
     */
    g_communicatorAggre->OfflineDevice(targetDev);
    std::sleep_for(std::chrono::milliseconds(WAIT_TIME / 20));
    EXPECT_EQ(isOk, true);
    StoreStatusNotifier callback;
    g_mgr.SetStoreStatusNotifier(callback);
}

/**
  * @tc.name: CloseSync001
  * @tc.desc: Test 2 delegate close when sync
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, CloseSync001, TestSize.Level3)
{
    DBStatus res = OK;
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. make sure A sync start
     */
    bool sleep = false;
    g_communicatorAggre->RegOnDispatch([&sleep](const std::string &target, DistributedDB::Message *msg) {
        if (!sleep) {
            sleep = true;
            std::sleep_for(std::chrono::seconds(2)); // sleep 2s for waiting close db
        }
    });

    KvStoreNbDelegate* kvDelegatePtrA = nullptr;
    KvStoreNbDelegate::Option op;
    g_mgr.GetKvStore(STORE_ID, op, [&res, &kvDelegatePtrA](DBStatus s, KvStoreNbDelegate *delegate) {
        res = s;
        kvDelegatePtrA = delegate;
    });
    EXPECT_EQ(res, OK);
    EXPECT_NE(kvDelegatePtrA, nullptr);

    Key key = {'k'};
    Value value = {'v'};
    kvDelegatePtrA->Put(key, value);
    std::map<std::string, DBStatus> res;
    auto callback = [&res](const std::map<std::string, DBStatus>& statusMap) {
        res = statusMap;
    };
    /**
     * @tc.steps: step2. deviceA sync and then close
     * @tc.expected: step2. sync should abort and data don't exist in B
     */
    std::thread thread([&kvDelegatePtrA]() {
        std::sleep_for(std::chrono::seconds(1)); // sleep 1s for waiting sync start
        EXPECT_EQ(g_mgr.CloseKvStore(kvDelegatePtrA), OK);
    });
    EXPECT_EQ(kvDelegatePtrA->Sync(devicesVec, SYNC_MODE_PUSH_ONLY, callback, false), OK);
    LOGD("Sync finish");
    thread.join();
    std::sleep_for(std::chrono::seconds(5)); // sleep 5s for waiting sync finish
    EXPECT_EQ(res.size(), 0u);
    VirtualDataItem actual;
    EXPECT_EQ(g_deviceB->GetData(key, actual), -E_NOT_FOUND);
    g_communicatorAggre->RegOnDispatch(nullptr);
}

/**
  * @tc.name: CloseSync002
  * @tc.desc: Test 1 delegate close when in time sync
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, CloseSync002, TestSize.Level3)
{
    /**
     * @tc.steps: step1. invalid time sync packet from A
     */
    g_communicatorAggre->RegOnDispatch([](const std::string &target, DistributedDB::Message *msg) {
        ASSERT_NE(msg, nullptr);
        if (target == DEVICE_B && msg->GetMessageId() == TIME_SYNC_MESSAGE && msg->GetMessageType() == TYPE_REQUEST) {
            msg->SetMessageId(INVALID_MESSAGE_ID);
            LOGD("Message is invalid");
        }
    });
    Timestamp time;
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    g_deviceB->PutData({'k'}, {'v'}, time, 0);

    /**
     * @tc.steps: step2. B PUSH to A and A close after 1s
     * @tc.expected: step2. A closing time cost letter than 4s
     */
    std::thread closingThread([]() {
        std::sleep_for(std::chrono::seconds(1));
        LOGD("Begin Close");
        Timestamp begin;
        (void)OS::GetCurrentSysTimeInMicrosecond(begin);
        EXCEPT_EQ(g_mgr.CloseKvStore(g_kvPtr), OK);
        Timestamp end;
        (void)OS::GetCurrentSysTimeInMicrosecond(end);
        EXPECT_LE(static_cast<int>(end - begin), 4 * 1000 * 1000); // waiting 4 * 1000 * 1000 us
        LOGD("End Close");
    });
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    closingThread.join();

    /**
     * @tc.steps: step3. remove db
     * @tc.expected: step3. remove ok
     */
    g_kvPtr = nullptr;
    DBStatus res = g_mgr.DeleteKvStore(STORE_ID);
    LOGD("delete kv store res %d", res);
    EXCEPT_TRUE(res == OK);
    g_communicatorAggre->RegOnDispatch(nullptr);
}

/**
  * @tc.name: OrderbyWriteTimeSync001
  * @tc.desc: sync sql with order by writeTime
  * @tc.type: FUNC
  * @tc.require: AR000H5VLO
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, OrderbyWriteTimeSync001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. deviceA subscribe sql with order by write time
     * * @tc.expected: step1. interface return not support
    */
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    Query sql = Query::Select().PrefixKey({'k'}).OrderByWriteTime(true);
    EXPECT_EQ(g_kvPtr->Sync(devicesVec, DistributedDB::SYNC_MODE_PUSH_ONLY, nullptr, sql, true), NOT_SUPPORT);
}


/**
 * @tc.name: Device Offline Sync 001
 * @tc.desc: Test push sync when device offline
 * @tc.type: FUNC
 * @tc.require: AR000CCPOM
 * @tc.author: xushaohua
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DeviceOfflineSync001, TestSize.Level1)
{
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    devicesVec.push_back(g_deviceC->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}, {k2, v2}, {k3 delete}, {k4,v2}
     */
    Key key1 = {'1'};
    Value value1 = {'1'};
    EXCEPT_TRUE(g_kvPtr->Put(key1, value1) == OK);

    Key key2 = {'2'};
    Value value2 = {'2'};
    EXCEPT_TRUE(g_kvPtr->Put(key2, value2) == OK);

    Key key3 = {'3'};
    Value value3 = {'3'};
    EXCEPT_TRUE(g_kvPtr->Put(key3, value3) == OK);
    EXCEPT_TRUE(g_kvPtr->Delete(key3) == OK);

    Key key4 = {'4'};
    Value value4 = {'4'};
    EXCEPT_TRUE(g_kvPtr->Put(key4, value4) == OK);

    /**
     * @tc.steps: step2. deviceB offline
     */
    g_deviceB->Offline();

    /**
     * @tc.steps: step3. deviceA call pull sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> res;
    DBStatus res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res);
    EXCEPT_TRUE(res == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceB res is timeout
     *     deviceC has {k1, v1}, {k2, v2}, {k3 delete}, {k4,v4}
     */
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        if (pair.first == DEVICE_B) {
            // If syncTaskContext of deviceB is scheduled to be executed first, ClearAllSyncTask is
            // invoked when OfflineHandleByDevice is triggered, and SyncOperation::Finished() is triggered in advance.
            // The returned res is COMM_FAILURE
            EXPECT_TRUE((pair.second == static_cast<DBStatus>(-E_PERIPHERAL_INTERFACE_FAIL)) ||
                (pair.second == COMM_FAILURE));
        } else {
            EXPECT_EQ(pair.second, OK);
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
    KvDBToolsUnitTest::CalcHash(key3, hashKey);
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
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DeviceOfflineSync002, TestSize.Level1)
{
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    devicesVec.push_back(g_deviceC->GetDeviceId());

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
    std::map<std::string, DBStatus> res;
    DBStatus res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PULL_ONLY, res);
    EXCEPT_TRUE(res == OK);

    /**
     * @tc.expected: step3. onComplete should be called, DeviceB res is timeout
     *     deviceA has {k2, v2}, {k3 delete}, {k4,v4}
     */
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        if (pair.first == DEVICE_B) {
            // If syncTaskContext of deviceB is scheduled to be executed first, ClearAllSyncTask is
            // invoked when OfflineHandleByDevice is triggered, and SyncOperation::Finished() is triggered in advance.
            // The returned res is COMM_FAILURE
            EXPECT_TRUE((pair.second == static_cast<DBStatus>(-E_PERIPHERAL_INTERFACE_FAIL)) ||
                (pair.second == COMM_FAILURE));
        } else {
            EXPECT_EQ(pair.second, OK);
        }
    }

    Value value6;
    EXPECT_TRUE(g_kvPtr->Get(key1, value6) != OK);
    g_kvPtr->Get(key2, value6);
    EXPECT_EQ(value6, value2);
    EXPECT_TRUE(g_kvPtr->Get(key3, value6) != OK);
    g_kvPtr->Get(key4, value6);
    EXPECT_EQ(value6, value4);
}

/**
  * @tc.name: EncryptedAlgoUpgrade001
  * @tc.desc: Test upgrade encrypted db can sync normally
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, EncryptedAlgoUpgrade001, TestSize.Level3)
{
    /**
     * @tc.steps: step1. clear db
     * * @tc.expected: step1. interface return ok
    */
    if (g_kvPtr != nullptr) {
        EXCEPT_EQ(g_mgr.CloseKvStore(g_kvPtr), OK);
        g_kvPtr = nullptr;
        DBStatus res = g_mgr.DeleteKvStore(STORE_ID);
        LOGD("delete kv store res %d", res);
        EXCEPT_TRUE(res == OK);
    }

    CipherPassword passWord;
    std::vector<uint8_t> passwdVect = {'p', 's', 'd', '1'};
    passWord.SetValue(passwdVect.data(), passwdVect.size());
    /**
     * @tc.steps: step2. open old db by sql
     * * @tc.expected: step2. interface return ok
    */
    std::string identifier = DBCommon::GenerateIdentifierId(STORE_ID, APP_ID, USER_ID);
    std::string hashDir = DBCommon::TransferHashString(identifier);
    std::string hexHashDir = DBCommon::TransferStringToHex(hashDir);
    std::string dbPath = g_testDir + "/" + hexHashDir + "/single_ver";
    EXCEPT_TRUE(DBCommon::CreateDirectory(g_testDir + "/" + hexHashDir) == E_OK);
    EXCEPT_TRUE(DBCommon::CreateDirectory(dbPath) == E_OK);
    std::vector<std::string> dbDir {DBConstant::MAINDB_DIR, DBConstant::METADB_DIR, DBConstant::CACHEDB_DIR};
    for (const auto &item : dbDir) {
        EXCEPT_TRUE(DBCommon::CreateDirectory(dbPath + "/" + item) == E_OK);
    }
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    sqlite3 *db;
    std::string uri = dbPath + "/" + DBConstant::MAINDB_DIR + "/" + DBConstant::SINGLE_VER_DATA_STORE + ".db";
    EXCEPT_TRUE(sqlite3_open_v2(uri.c_str(), &db, flag, nullptr) == SQLITE_OK);
    SQLiteUtils::SetKeyInner(db, CipherType::AES_256_GCM, passWord, DBConstant::DEFAULT_ITER_TIMES);
    /**
     * @tc.steps: step3. create table and close
     * * @tc.expected: step3. interface return ok
    */
    EXCEPT_TRUE(SQLiteUtils::ExecuteRawSQL(db, CREATE_SYNC_TABLE_SQL) == E_OK);
    sqlite3_close_v2(db);
    db = nullptr;
    LOGI("create old db success");
    /**
     * @tc.steps: step4. get kvstore
     * * @tc.expected: step4. interface return ok
    */
    KvStoreNbDelegate::Option op;
    op.isEncryptedDb = true;
    op.cipher = CipherType::AES_256_GCM;
    op.passWord = passWord;
    g_mgr.GetKvStore(STORE_ID, op, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvDelegateStatus == OK);
    EXCEPT_TRUE(g_kvPtr != nullptr);
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
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, RemoveDeviceData002, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    /**
     * @tc.steps: step1. sync deviceB data to A and check data
     * * @tc.expected: step1. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    Value value = {'1'};
    Timestamp time;
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key1, value, time, 0), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key2, value, time, 0), E_OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    Value actual;
    EXPECT_EQ(g_kvPtr->Get(key1, actual), OK);
    EXPECT_EQ(actual, value);
    actual.clear();
    EXPECT_EQ(g_kvPtr->Get(key2, actual), OK);
    EXPECT_EQ(actual, value);
    /**
     * @tc.steps: step2. call RemoveDeviceData
     * * @tc.expected: step2. interface return ok
    */
    g_kvPtr->RemoveDeviceData(g_deviceB->GetDeviceId());
    EXPECT_EQ(g_kvPtr->Get(key1, actual), NOT_FOUND);
    EXPECT_EQ(g_kvPtr->Get(key2, actual), NOT_FOUND);
    /**
     * @tc.steps: step3. sync to device A again and check data
     * * @tc.expected: step3. sync ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    actual.clear();
    EXPECT_EQ(g_kvPtr->Get(key1, actual), OK);
    EXPECT_EQ(actual, value);
    actual.clear();
    EXPECT_EQ(g_kvPtr->Get(key2, actual), OK);
    EXPECT_EQ(actual, value);
}

/**
  * @tc.name: DataSync001
  * @tc.desc: Test Data Sync when Initialize
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DataSync001, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    EXCEPT_TRUE(dataSync != nullptr);
    std::shared_ptr<Metadata> inMetadata = nullptr;
    std::string deviceId;
    Message message;
    VirtualSingleVerSyncDBInterface tmpInterface;
    VirtualCommunicator tmpCommunicator(deviceId, g_communicatorAggre);
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
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DataSync002, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    EXCEPT_TRUE(dataSync != nullptr);
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
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DataSync003, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    EXCEPT_TRUE(dataSync != nullptr);
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
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DataSync004, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    EXCEPT_TRUE(dataSync != nullptr);
    Message message;
    TestSingleVerKvSyncTask tmpContext;
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
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DataSync005, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(DataSync005());
}

/**
  * @tc.name: DataSync006
  * @tc.desc: Test control start with invalid param in DataSync Class
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DataSync006, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    EXCEPT_TRUE(dataSync != nullptr);
    TestSingleVerKvSyncTask tmpContext;
    EXPECT_EQ(dataSync->ControlCmdStart(nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(dataSync->ControlCmdStart(&tmpContext), -E_INVALID_ARGS);
    std::shared_ptr<SubscribeManager> subManager = std::make_shared<SubscribeManager>();
    tmpContext.SetSubscribeManager(subManager);
    tmpContext.SetMode(SyncModeType::INVALID_MODE);
    EXPECT_EQ(dataSync->ControlCmdStart(&tmpContext), -E_INVALID_ARGS);
    std::set<Key> Keys = {{'a'}, {'b'}};
    Query sql = Query::Select().InKeys(Keys);
    QuerySyncObject innerQuery(sql);
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
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DataSync007, TestSize.Level1)
{
    SingleVerDataSync *dataSync = new (std::nothrow) SingleVerDataSync();
    EXCEPT_TRUE(dataSync != nullptr);
    Message message;
    ControlRequestPacket packet;
    TestSingleVerKvSyncTask tmpContext;
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
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DataSync008, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(DataSync008());
}

/**
 * @tc.name: SyncRetry001
 * @tc.desc: use sync retry sync use push
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SyncRetry001, TestSize.Level3)
{
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. set sync retry
     * @tc.expected: step1, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvPtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step2. deviceA put {k1, v1}, {k2, v2}
     */
    EXCEPT_TRUE(g_kvPtr->Put(KEY_1, VALUE_1) == OK);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> res;
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res) == OK);

    /**
     * @tc.expected: step4. onComplete should be called, and res is time_out
     */
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: SyncRetry002
 * @tc.desc: use sync retry sync use pull
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SyncRetry002, TestSize.Level3)
{
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE, 4u);
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. set sync retry
     * @tc.expected: step1, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvPtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step2. deviceA call sync and wait
     * @tc.expected: step2. sync should return OK.
     */
    std::map<std::string, DBStatus> res;
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PULL_ONLY, res) == OK);

    /**
     * @tc.expected: step3. onComplete should be called, and res is time_out
     */
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == TIME_OUT);
    }
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: SyncRetry003
 * @tc.desc: use sync retry sync use push by compress
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SyncRetry003, TestSize.Level3)
{
    if (g_kvPtr != nullptr) {
        EXCEPT_EQ(g_mgr.CloseKvStore(g_kvPtr), OK);
        g_kvPtr = nullptr;
    }
    /**
     * @tc.steps: step1. open db use Compress
     * @tc.expected: step1, Pragma return OK.
     */
    KvStoreNbDelegate::Option op;
    op.isNeedCompressOnSync = true;
    op.compressionRate = 70;
    g_mgr.GetKvStore(STORE_ID, op, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvDelegateStatus == OK);
    EXCEPT_TRUE(g_kvPtr != nullptr);

    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. set sync retry
     * @tc.expected: step2, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvPtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step3. deviceA put {k1, v1}, {k2, v2}
     */
    EXCEPT_TRUE(g_kvPtr->Put(KEY_1, VALUE_1) == OK);

    /**
     * @tc.steps: step4. deviceA call sync and wait
     * @tc.expected: step4. sync should return OK.
     */
    std::map<std::string, DBStatus> res;
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res) == OK);

    /**
     * @tc.expected: step5. onComplete should be called, and res is time_out
     */
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: SyncRetry004
 * @tc.desc: use sql sync retry sync use push
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SyncRetry004, TestSize.Level3)
{
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. set sync retry
     * @tc.expected: step1, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvPtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step2. deviceA put {k1, v1}, {k2, v2}
     */
    for (int i = 0; i < 5; i++) {
        Key key = KvDBToolsUnitTest::GetRandPrefixKey({'a', 'b'}, 128); // rand num 1024 for test
        Value value;
        KvDBToolsUnitTest::GetRandomKeyValue(value, 256u);
        EXPECT_EQ(g_kvPtr->Put(key, value), OK);
    }

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> res;
    std::vector<uint8_t> prefixKey({'a', 'b'});
    Query sql = Query::Select().PrefixKey(prefixKey);
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res, sql) == OK);

    /**
     * @tc.expected: step4. onComplete should be called, and res is time_out
     */
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: SyncRetry005
 * @tc.desc: use sync retry sync use pull by compress
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, SyncRetry005, TestSize.Level3)
{
    if (g_kvPtr != nullptr) {
        EXCEPT_EQ(g_mgr.CloseKvStore(g_kvPtr), OK);
        g_kvPtr = nullptr;
    }
    /**
     * @tc.steps: step1. open db use Compress
     * @tc.expected: step1, Pragma return OK.
     */
    KvStoreNbDelegate::Option op;
    op.isNeedCompressOnSync = true;
    g_mgr.GetKvStore(STORE_ID, op, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvDelegateStatus == OK);
    EXCEPT_TRUE(g_kvPtr != nullptr);

    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step2. set sync retry
     * @tc.expected: step2, Pragma return OK.
     */
    int pragmaData = 1;
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvPtr->Pragma(SET_SYNC_RETRY, input) == OK);

    /**
     * @tc.steps: step3. deviceA call sync and wait
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> res;
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PULL_ONLY, res) == OK);

    /**
     * @tc.expected: step4. onComplete should be called, and res is time_out
     */
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_EQ(pair.second, OK);
    }
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, UNKNOW_MESSAGE);
}

/**
 * @tc.name: ReSetWatchDogTest001
 * @tc.desc: trigger resetWatchDog while pull
 * @tc.type: FUNC
 * @tc.require: AR000CKRTD AR000CQE0E
 * @tc.author: zhuwentao
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, ReSetWaterDogTest001, TestSize.Level3)
{
    ASSERT_NO_FATAL_FAILURE(ReSetWaterDogTest001());
}

/**
  * @tc.name: RebuildSync001
  * @tc.desc: rebuild db and sync again
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, RebuildSync001, TestSize.Level3)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    /**
     * @tc.steps: step1. sync deviceB data to A and check data
     * * @tc.expected: step1. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    Value value = {'1'};
    Timestamp time;
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key1, value, time, 0), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key2, value, time, 0), E_OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    std::sleep_for(std::chrono::seconds(1));

    Value actual;
    EXPECT_EQ(g_kvPtr->Get(key1, actual), OK);
    EXPECT_EQ(actual, value);
    actual.clear();
    EXPECT_EQ(g_kvPtr->Get(key2, actual), OK);
    EXPECT_EQ(actual, value);
    /**
     * @tc.steps: step2. delete db and rebuild
     * * @tc.expected: step2. interface return ok
    */
    g_mgr.CloseKvStore(g_kvPtr);
    g_kvPtr = nullptr;
    EXCEPT_TRUE(g_mgr.DeleteKvStore(STORE_ID) == OK);
    KvStoreNbDelegate::Option op;
    g_mgr.GetKvStore(STORE_ID, op, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvDelegateStatus == OK);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    /**
     * @tc.steps: step3. sync to device A again
     * * @tc.expected: step3. sync ok
    */
    value = {'2'};
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key1, value, time, 0), E_OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    std::sleep_for(std::chrono::seconds(1));
    /**
     * @tc.steps: step4. check data in device A
     * * @tc.expected: step4. check ok
    */
    actual.clear();
    EXPECT_EQ(g_kvPtr->Get(key1, actual), OK);
    EXPECT_EQ(actual, value);
}

/**
  * @tc.name: RebuildSync002
  * @tc.desc: test clear remote data when receive data
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, RebuildSync002, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());
    /**
     * @tc.steps: step1. device A SET_WIPE_POLICY
     * * @tc.expected: step1. interface return ok
    */
    int pragmaData = 2; // 2 means enable
    PragmaData input = static_cast<PragmaData>(&pragmaData);
    EXPECT_TRUE(g_kvPtr->Pragma(SET_WIPE_POLICY, input) == OK);
    /**
     * @tc.steps: step2. sync deviceB data to A and check data
     * * @tc.expected: step2. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    Key key3 = {'3'};
    Key key4 = {'4'};
    Value value = {'1'};
    Timestamp time;
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key1, value, time, 0), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key2, value, time, 0), E_OK);
    EXPECT_EQ(g_kvPtr->Put(key3, value), OK);
    /**
     * @tc.steps: step3. deviceA call pull sync
     * @tc.expected: step3. sync should return OK.
     */
    std::map<std::string, DBStatus> res;
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_PULL, res) == OK);

    /**
     * @tc.expected: step4. onComplete should be called, check data
     */
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        EXPECT_TRUE(pair.second == OK);
    }
    Value actual;
    EXPECT_EQ(g_kvPtr->Get(key1, actual), OK);
    EXPECT_EQ(actual, value);
    EXPECT_EQ(g_kvPtr->Get(key2, actual), OK);
    EXPECT_EQ(actual, value);
    /**
     * @tc.steps: step5. device B rebuild and put some data
     * * @tc.expected: step5. rebuild ok
    */
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    g_deviceB = new (std::nothrow) KvDevice(DEVICE_B);
    EXCEPT_TRUE(g_deviceB != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterfaceA = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    EXCEPT_TRUE(syncInterfaceA != nullptr);
    EXCEPT_EQ(g_deviceB->Initialize(g_communicatorAggre, syncInterfaceA), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key3, value, time, 0), E_OK);
    (void)OS::GetCurrentSysTimeInMicrosecond(time);
    EXPECT_EQ(g_deviceB->PutData(key4, value, time, 0), E_OK);
    /**
     * @tc.steps: step6. sync to device A again and check data
     * * @tc.expected: step6. sync ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(g_kvPtr->Get(key3, actual), OK);
    EXPECT_EQ(actual, value);
    EXPECT_EQ(g_kvPtr->Get(key4, actual), OK);
    EXPECT_EQ(actual, value);
    EXPECT_EQ(g_kvPtr->Get(key1, actual), NOT_FOUND);
    EXPECT_EQ(g_kvPtr->Get(key2, actual), NOT_FOUND);
}

/**
  * @tc.name: RebuildSync003
  * @tc.desc: test clear history data when receive ack
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, RebuildSync003, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
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
    EXPECT_EQ(g_kvPtr->Put(key3, value), OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_PULL, true), E_OK);
    Value actual;
    EXPECT_EQ(g_kvPtr->Get(key1, actual), OK);
    EXPECT_EQ(actual, value);
    EXPECT_EQ(g_kvPtr->Get(key2, actual), OK);
    EXPECT_EQ(actual, value);
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(key3, item), E_OK);
    EXPECT_EQ(item.value, value);
    /**
     * @tc.steps: step2. device B sync to device A,but make it failed
     * * @tc.expected: step2. interface return ok
    */
    EXPECT_EQ(g_deviceB->PutData(key4, value, 3u, 0), E_OK); // 3: timestamp
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_A, DATA_SYNC_MESSAGE);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    /**
     * @tc.steps: step4. device A rebuilt, device B push data to A and set clear remote data mark into context after 1s
     * * @tc.expected: step4. interface return ok
    */
    g_deviceB->SetClearRemoteStaleData(true);
    g_mgr.CloseKvStore(g_kvPtr);
    g_kvPtr = nullptr;
    EXCEPT_TRUE(g_mgr.DeleteKvStore(STORE_ID) == OK);
    KvStoreNbDelegate::Option op;
    g_mgr.GetKvStore(STORE_ID, op, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvDelegateStatus == OK);
    EXCEPT_TRUE(g_kvPtr != nullptr);
    std::map<std::string, DBStatus> res;
    std::vector<std::string> devicesVec = {g_deviceB->GetDeviceId()};
    g_communicatorAggre->SetDropMessageTypeByDevice(DEVICE_B, DATA_SYNC_MESSAGE);
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res) == OK);
    /**
     * @tc.steps: step5. device B sync to A, make it clear history data and check data
     * * @tc.expected: step5. interface return ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(g_deviceB->GetData(key3, item), -E_NOT_FOUND);
    EXPECT_EQ(g_kvPtr->Get(key1, actual), OK);
    EXPECT_EQ(actual, value);
    EXPECT_EQ(g_kvPtr->Get(key2, actual), OK);
    EXPECT_EQ(actual, value);
    g_communicatorAggre->ResetSendDelayInfo();
}

/**
  * @tc.name: RebuildSync004
  * @tc.desc: test WIPE_STALE_DATA mode when peers rebuilt db
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangtao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, RebuildSync004, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    /**
     * @tc.steps: step1. sync deviceB data to A and check data
     * * @tc.expected: step1. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    Key key3 = {'3'};
    Key key4 = {'4'};
    Value value = {'1'};
    EXPECT_EQ(g_kvPtr->Put(key1, value), OK);
    EXPECT_EQ(g_kvPtr->Put(key2, value), OK);
    EXPECT_EQ(g_kvPtr->Put(key3, value), OK);
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_PULL, true), E_OK);
    Value actual;
    EXPECT_EQ(g_kvPtr->Get(key1, actual), OK);
    EXPECT_EQ(actual, value);
    EXPECT_EQ(g_kvPtr->Get(key2, actual), OK);
    EXPECT_EQ(actual, value);
    EXPECT_EQ(g_kvPtr->Get(key3, actual), OK);
    EXPECT_EQ(actual, value);
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(key1, item), E_OK);
    EXPECT_EQ(item.value, value);
    EXPECT_EQ(g_deviceB->GetData(key2, item), E_OK);
    EXPECT_EQ(item.value, value);
    EXPECT_EQ(g_deviceB->GetData(key3, item), E_OK);
    EXPECT_EQ(item.value, value);

    /**
     * @tc.steps: step2. device A rebuilt, device B push data to A and set clear remote data mark into context after 1s
     * * @tc.expected: step2. interface return ok
    */
    g_deviceB->SetClearRemoteStaleData(true);
    EXPECT_EQ(g_deviceB->PutData(key4, value, 3u, 2), E_OK); // 3: timestamp

    VirtualDataItem item2;
    EXPECT_EQ(g_deviceB->GetData(key4, item2), E_OK);
    EXPECT_EQ(item2.value, value);
    KvStoreNbDelegate::Option op;
    g_mgr.GetKvStore(STORE_ID, op, g_kvDelegateCallback);
    EXCEPT_TRUE(g_kvDelegateStatus == OK);
    EXCEPT_TRUE(g_kvPtr != nullptr);

    /**
     * @tc.steps: step3. device B sync to A, make it clear history data and check data
     * * @tc.expected: step3. interface return ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true), E_OK);
    EXPECT_EQ(g_deviceB->GetData(key2, item), -E_NOT_FOUND);
    EXPECT_EQ(g_deviceB->GetData(key3, item), -E_NOT_FOUND);
    EXPECT_EQ(g_deviceB->GetData(key4, item2), E_OK);
    EXPECT_EQ(item2.value, value);
    EXPECT_EQ(g_kvPtr->Get(key4, actual), OK);
    EXPECT_EQ(actual, value);
}

/**
  * @tc.name: RemoveDeviceData001
  * @tc.desc: call rekey and removeDeviceData Concurrently
  * @tc.type: FUNC
  * @tc.require: AR000D487B
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, RemoveDeviceData001, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    /**
     * @tc.steps: step1. sync deviceB data to A
     * * @tc.expected: step1. interface return ok
    */
    Key key1 = {'1'};
    Key key2 = {'2'};
    g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_ONLY, true);

    Value actual;
    g_kvPtr->Get(key1, actual);
    EXPECT_EQ(actual, value);
    actual.clear();
    g_kvPtr->Get(key2, actual);
    EXPECT_EQ(actual, value);
    /**
     * @tc.steps: step2. call Rekey and RemoveDeviceData Concurrently
     * * @tc.expected: step2. interface return ok
    */
    std::thread thread1([]() {
        CipherPassword passwd3;
        std::vector<uint8_t> passwdVect = {'p', 's', 'd', 'z'};
        passwd3.SetValue(passwdVect.data(), passwdVect.size());
        g_kvPtr->Rekey(passwd3);
    });
    std::thread thread2([]() {
        g_kvPtr->RemoveDeviceData(g_deviceB->GetDeviceId());
    });
    thread1.join();
    thread2.join();
}

/**
  * @tc.name: DeviceOfflineSyncTask001
  * @tc.desc: Test sync task when device offline and close db Concurrently
  * @tc.type: FUNC
  * @tc.require: AR000HI2JS
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DeviceOfflineSyncTask001, TestSize.Level3)
{
    DBStatus res = OK;
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    EXCEPT_TRUE(g_kvPtr->Put(key, value) == OK);

    /**
     * @tc.steps: step2. deviceA set auto sync and put some key/value
     * @tc.expected: step2. interface should return OK.
     */
    bool autoSync = true;
    PragmaData data = static_cast<PragmaData>(&autoSync);
    res = g_kvPtr->Pragma(AUTO_SYNC, data);
    EXCEPT_EQ(res, OK);

    Key key1 = {'2'};
    Key key2 = {'3'};
    Key key3 = {'4'};
    EXCEPT_TRUE(g_kvPtr->Put(key, value) == OK);
    EXCEPT_TRUE(g_kvPtr->Put(key1, value) == OK);
    EXCEPT_TRUE(g_kvPtr->Put(key2, value) == OK);
    EXCEPT_TRUE(g_kvPtr->Put(key3, value) == OK);
    /**
     * @tc.steps: step3. device offline and close db Concurrently
     * @tc.expected: step3. interface should return OK.
     */
    std::thread thread1([]() {
        g_mgr.CloseKvStore(g_kvPtr);
        g_kvPtr = nullptr;
    });
    std::thread thread2([]() {
        g_deviceB->Offline();
    });
    thread1.join();
    thread2.join();
    std::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    EXCEPT_TRUE(g_mgr.DeleteKvStore(STORE_ID) == OK);
}

/**
  * @tc.name: DeviceOfflineSyncTask002
  * @tc.desc: Test sync task when autoSync and close db Concurrently
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DeviceOfflineSyncTask002, TestSize.Level3)
{
    DBStatus res = OK;
    g_deviceC->Offline();

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    EXCEPT_TRUE(g_kvPtr->Put(key, value) == OK);

    /**
     * @tc.steps: step2. deviceA set auto sync and put some key/value
     * @tc.expected: step2. interface should return OK.
     */
    bool autoSync = true;
    PragmaData data = static_cast<PragmaData>(&autoSync);
    res = g_kvPtr->Pragma(AUTO_SYNC, data);
    EXCEPT_EQ(res, OK);
    std::sleep_for(std::chrono::milliseconds(WAIT_TIME * 2));

    Key key1 = {'2'};
    Key key2 = {'3'};
    EXCEPT_TRUE(g_kvPtr->Put(key1, value) == OK);
    EXCEPT_TRUE(g_kvPtr->Put(key2, value) == OK);
    /**
     * @tc.steps: step3. close db
     * @tc.expected: step3. interface should return OK.
     */
    g_mgr.CloseKvStore(g_kvPtr);
    g_kvPtr = nullptr;
    EXCEPT_TRUE(g_mgr.DeleteKvStore(STORE_ID) == OK);
}

/**
  * @tc.name: DeviceOfflineSyncTask003
  * @tc.desc: Test sync task when device offline after call sync
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, DeviceOfflineSyncTask003, TestSize.Level3)
{
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceB->GetDeviceId());

    /**
     * @tc.steps: step1. deviceA put {k1, v1}
     */
    Key key = {'1'};
    Value value = {'1'};
    EXCEPT_TRUE(g_kvPtr->Put(key, value) == OK);
    /**
     * @tc.steps: step2. device offline after call sync
     * @tc.expected: step2. interface should return OK.
     */
    Query sql = Query::Select().PrefixKey(key);
    EXCEPT_TRUE(g_kvPtr->Sync(devicesVec, SYNC_MODE_PUSH_ONLY, nullptr, sql, false) == OK);
    std::sleep_for(std::chrono::milliseconds(15)); // wait for 15ms
    g_deviceB->Offline();
}

/**
  * @tc.name: GetSyncDataFail001
  * @tc.desc: test get sync data failed when sync
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, GetSyncDataFail001, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
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
    Value actual;
    EXPECT_EQ(g_kvPtr->Get(key1, actual), NOT_FOUND);
    g_deviceB->ResetDataControl();
}

/**
  * @tc.name: GetSyncDataFail004
  * @tc.desc: test get sync data E_EKEYREVOKED failed in push_and_pull sync
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, GetSyncDataFail004, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    /**
     * @tc.steps: step1. device B set get data errCode control and put some data
     * * @tc.expected: step1. interface return ok
    */
    g_deviceB->SetGetDataErrCode(2, -E_EKEYREVOKED, true);
    int totalSize = 4000u;
    std::vector<Entry> entries;
    std::vector<Key> keys;
    const int keyLen = 10; // 20 Bytes
    const int valueLen = 10; // 20 Bytes
    DistributedDBUnitTest::GenerateRecords(totalSize, entries, keys, keyLen, valueLen);
    uint32_t i = 1u;
    for (const auto &entry : entries) {
        EXPECT_EQ(g_deviceB->PutData(entry.key, entry.value, i, 0), E_OK);
        i++;
    }
    Key key = {'a', 'b', 'c'};
    Value value = {'1'};
    EXPECT_EQ(g_kvPtr->Put(key, value), OK);
    /**
     * @tc.steps: step2. device B sync to device A and check data
     * * @tc.expected: step2. interface return ok
    */
    EXPECT_EQ(g_deviceB->Sync(DistributedDB::SYNC_MODE_PUSH_PULL, true), E_OK);
    std::sleep_for(std::chrono::seconds(1));
    Value actual;
    for (int j = 1u; j <= totalSize; j++) {
        if (j > totalSize / 2) {
            EXPECT_EQ(g_kvPtr->Get(entries[j - 1].key, actual), NOT_FOUND);
        } else {
            EXPECT_EQ(g_kvPtr->Get(entries[j - 1].key, actual), OK);
        }
    }
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(key, item), E_OK);
    g_deviceB->ResetDataControl();
}

/**
  * @tc.name: InterceptDataFail001
  * @tc.desc: test intercept data failed when sync
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, InterceptDataFail001, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    /**
     * @tc.steps: step1. device A set intercept data errCode and put some data
     * * @tc.expected: step1. interface return ok
    */
    g_kvPtr->SetPushDataInterceptor(
        [](InterceptedData &data, const std::string &sourceID, const std::string &targetID) {
            int errCode = OK;
            auto entries = data.GetEntries();
            LOGD("====here111,size=%d", entries.size());
            for (size_t i = 0; i < entries.size(); i++) {
                Key newKey;
                errCode = data.ModifyKey(i, newKey);
                if (errCode != OK) {
                    break;
                }
            }
            return errCode;
        }
    );
    Key key = {'1'};
    Value value = {'1'};
    EXPECT_EQ(g_kvPtr->Put(key, value), OK);
    /**
     * @tc.steps: step2. device A sync to device B and check data
     * * @tc.expected: step2. interface return ok
    */
    std::vector<std::string> devicesVec = { g_deviceB->GetDeviceId() };
    std::map<std::string, DBStatus> res;
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res) == OK);
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == INTERCEPT_DATA_FAIL);
    }
    VirtualDataItem item;
    EXPECT_EQ(g_deviceB->GetData(key, item), -E_NOT_FOUND);
}

/**
  * @tc.name: InterceptDataFail002
  * @tc.desc: test intercept data failed when sync
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, InterceptDataFail002, TestSize.Level0)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    /**
     * @tc.steps: step1. device A set intercept data errCode and B put some data
     * @tc.expected: step1. interface return ok
     */
    g_kvPtr->SetReceiveDataInterceptor(
        [](InterceptedData &data, const std::string &sourceID, const std::string &targetID) {
            auto entries = data.GetEntries();
            LOGD("====on receive,size=%d", entries.size());
            for (size_t i = 0; i < entries.size(); i++) {
                Key newKey;
                int errCode = data.ModifyKey(i, newKey);
                if (errCode != OK) {
                    return errCode;
                }
            }
            return E_OK;
        }
    );
    Key key = {'1'};
    Value value = {'1'};
    g_deviceB->PutData(key, value, 1u, 0); // 1 is timestamp
    /**
     * @tc.steps: step2. device A sync to device B and check data
     * @tc.expected: step2. interface return ok
     */
    std::vector<std::string> devicesVec = { g_deviceB->GetDeviceId() };
    std::map<std::string, DBStatus> res;
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PULL_ONLY, res) == OK);
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_EQ(pair.second, INTERCEPT_DATA_FAIL);
    }
    Value actual;
    EXPECT_EQ(g_kvPtr->Get(key, actual), NOT_FOUND);
}
