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
    const std::string DEVICE_D = "real_device";
    const std::string DEVICE_E = "deviceB";
    const std::string DEVICE_F = "deviceC";
    const std::string CREATE_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS sync_data(" \
        "key         BLOB NOT NULL," \
        "value       BLOB," \
        "timestamp   INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "device      BLOB," \
        "hash_key    BLOB PRIMARY KEY NOT NULL," \
        "w_timestamp INT," \
        "modify_time INT," \
        "create_time INT" \
        ");";

    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreConfig g_config;
    KvDBToolsUnitTest g_tool;
    DBStatus g_Status = INVALID_ARGS;
    KvStoreNbDelegate* g_kvPtr = nullptr;
    VirtualCommunicatorAggregator* g_communicatorAgg = nullptr;
    KvDevice *g_deviceE = nullptr;
    KvDevice *g_deviceF = nullptr;

    // the type of g_kvDelegateCallback is function<void(DBStatus, KvStoreDelegate*)>
    auto g_kvDelegateCallback = bind(&KvDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_Status), std::ref(g_kvPtr));

    void PushSyncTest()
    {
        DBStatus res = OK;
        std::vector<std::string> devicesVec;
        devicesVec.push_back(g_deviceE->GetDeviceId());

        Key key = {'1'};
        Key key2 = {'2'};
        Value value = {'1'};
        g_deviceE->PutData(key, value, 0, 0);
        g_deviceE->PutData(key2, value, 1, 0);

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

    void CrudInnerTest()
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

    void DataCheck001()
    {
        ASSERT_NE(g_communicatorAgg, nullptr);
        SingleVerDataSync *sync = new (std::nothrow) SingleVerDataSync();
        EXCEPT_TRUE(sync != nullptr);
        sync->SendSaveDataNotifyPacket(nullptr, 0, 0, 0, TIME_SYNC_MESSAGE);
        EXPECT_EQ(g_communicatorAgg->GetOnlineDevices().size(), 3u); // 3 online dev
        delete sync;
    }

    void DataCheck002()
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

    void ReSetLimiteTest001()
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
        g_communicatorAgg->SetDeviceMtuSize(DEVICE_D, 50 * 1024u); // 50 * 1024u = 50k
        g_communicatorAgg->SetDeviceMtuSize(DEVICE_E, 50 * 1024u); // 50 * 1024u = 50k
        /**
         * @tc.steps: step3. deviceA,deviceB sync to each other at same time
         * @tc.expected: step3. sync should return OK.
         */
        EXPECT_EQ(g_deviceE->Sync(DistributedDB::SYNC_MODE_PULL_ONLY, true), E_OK);
        g_communicatorAgg->SetDeviceMtuSize(DEVICE_D, 5 * 1024u * 1024u); // 5 * 1024u * 1024u = 5m
        g_communicatorAgg->SetDeviceMtuSize(DEVICE_E, 5 * 1024u * 1024u); // 5 * 1024u * 1024u = 5m
    }
}
/**
  * @tc.name: InterceptData001
  * @tc.desc: test intercept receive data when sync
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, InterceptData001, TestSize.Level0)
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
                Key newKey = {'2'};
                int errCode = data.ModifyKey(i, newKey);
                if (errCode != OK) {
                    return errCode;
                }
                Value newValue = {'3'};
                errCode = data.ModifyValue(i, newValue);
                if (errCode != OK) {
                    return errCode;
                }
            }
            return E_OK;
        }
    );
    Key key = {'1'};
    Value value = {'1'};
    g_deviceE->PutData(key, value, 1u, 0); // 1 is timestamp
    /**
     * @tc.steps: step2. device A sync to device B and check data
     * @tc.expected: step2. interface return ok
     */
    std::vector<std::string> devicesVec = { g_deviceE->GetDeviceId() };
    std::map<std::string, DBStatus> res;
    EXCEPT_TRUE(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PULL_ONLY, res) == OK);
    EXCEPT_TRUE(res.size() == devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_EQ(pair.second, OK);
    }
    Value actual;
    EXPECT_EQ(g_kvPtr->Get(key, actual), NOT_FOUND);
    key = {'2'};
    EXPECT_EQ(g_kvPtr->Get(key, actual), OK);
    value = {'3'};
    EXPECT_EQ(actual, value);
}

/**
  * @tc.name: UpdateKey001
  * @tc.desc: test update key can effect local data and sync data, without delete data
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, UpdateKey001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. device A set sync data (k1, v1) local data (k2, v2) (k3, v3) and delete (k4, v4)
     * @tc.expected: step1. put data return ok
     */
    Key k1 = {'k', '1'};
    Value v1 = {'v', '1'};
    g_deviceE->PutData(k1, v1, 1, 0);
    EXCEPT_EQ(g_deviceE->Sync(SyncMode::SYNC_MODE_PUSH_ONLY, true), E_OK);
    Value actual;
    EXPECT_EQ(g_kvPtr->Get(k1, actual), OK);
    EXPECT_EQ(v1, actual);
    Key k2 = {'k', '2'};
    Value v2 = {'v', '2'};
    Key k3 = {'k', '3'};
    Value v3 = {'v', '3'};
    Key k4 = {'k', '4'};
    Value v4 = {'v', '4'};
    EXPECT_EQ(g_kvPtr->Put(k2, v2), OK);
    EXPECT_EQ(g_kvPtr->Put(k3, v3), OK);
    EXPECT_EQ(g_kvPtr->Put(k4, v4), OK);
    EXPECT_EQ(g_kvPtr->Delete(k4), OK);
    /**
     * @tc.steps: step2. device A update key and set
     * @tc.expected: step2. put data return ok
     */
    DBStatus res = g_kvPtr->UpdateKey([](const Key &originKey, Key &newKey) {
        newKey = originKey;
        newKey.push_back('0');
    });
    EXPECT_EQ(res, OK);
    k1.push_back('0');
    k2.push_back('0');
    k3.push_back('0');
    EXPECT_EQ(g_kvPtr->Get(k1, actual), OK);
    EXPECT_EQ(v1, actual);
    EXPECT_EQ(g_kvPtr->Get(k2, actual), OK);
    EXPECT_EQ(v2, actual);
    EXPECT_EQ(g_kvPtr->Get(k3, actual), OK);
    EXPECT_EQ(v3, actual);
}

/**
  * @tc.name: MetaBusy001
  * @tc.desc: test sync normal when update water mark busy
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, MetaBusy001, TestSize.Level1)
{
    EXCEPT_TRUE(g_kvPtr != nullptr);
    Key key = {'1'};
    Value value = {'1'};
    EXPECT_EQ(g_kvPtr->Put(key, value), OK);
    std::vector<std::string> devicesVec = { g_deviceE->GetDeviceId() };
    std::map<std::string, DBStatus> res;
    EXCEPT_EQ(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res), OK);
    EXCEPT_EQ(res.size(), devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    value = {'2'};
    EXPECT_EQ(g_kvPtr->Put(key, value), OK);
    g_deviceE->SetSaveDataCallback([] () {
        RuntimeContext::GetInstance()->ScheduleTask([]() {
            g_deviceE->EraseWaterMark("real_device");
        });
        std::sleep_for(std::chrono::seconds(1));
    });
    EXPECT_EQ(g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res), OK);
    EXPECT_EQ(res.size(), devicesVec.size());
    for (const auto &pair : res) {
        LOGD("dev %s, res %d", pair.first.c_str(), pair.second);
        EXPECT_TRUE(pair.second == OK);
    }
    g_deviceE->SetSaveDataCallback(nullptr);
    RuntimeContext::GetInstance()->StopTaskPool();
}

/**
 * @tc.name: TestErrCodePassthrough001
 * @tc.desc: Test ErrCode Passthrough when sync comm fail
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, TestErrCodePassthrough001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. device put data.
     * @tc.expected: step1. sync return OK.
     */
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceE->GetDeviceId());
    devicesVec.push_back(g_deviceF->GetDeviceId());
    Key key1 = {'1'};
    Value value1 = {'1'};
    EXCEPT_EQ(g_kvPtr->Put(key1, value1), OK);

    /**
     * @tc.steps: step2. call sync and mock commErrCode is E_BASE(positive number).
     * @tc.expected: step2. return COMM_FAILURE.
     */
    g_communicatorAgg->MockCommErrCode(E_BASE);
    std::map<std::string, DBStatus> res;
    DBStatus res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res);
    EXCEPT_EQ(res, OK);
    for (const auto &pair : res) {
        LOGD("dev %s, res %d, expectStatus %d", pair.first.c_str(), pair.second, E_BASE);
        EXPECT_EQ(pair.second, COMM_FAILURE);
    }

    /**
     * @tc.steps: step3. call sync and mock commErrCode is -E_BASE(negative number).
     * @tc.expected: step3. return -E_BASE.
     */
    g_communicatorAgg->MockCommErrCode(-E_BASE);
    res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res);
    EXCEPT_EQ(res, OK);
    for (const auto &pair : res) {
        LOGD("dev %s, res %d, expectStatus %d", pair.first.c_str(), pair.second, COMM_FAILURE);
        EXPECT_EQ(pair.second, static_cast<DBStatus>(-E_BASE));
    }

    /**
     * @tc.steps: step4. call sync and mock commErrCode is INT_MAX.
     * @tc.expected: step4. return COMM_FAILURE.
     */
    g_communicatorAgg->MockCommErrCode(INT_MAX);
    res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res);
    EXCEPT_EQ(res, OK);
    for (const auto &pair : res) {
        LOGD("dev %s, res %d, expectStatus %d", pair.first.c_str(), pair.second, INT_MAX);
        EXPECT_EQ(pair.second, COMM_FAILURE);
    }

    /**
     * @tc.steps: step5. call sync and mock commErrCode is -INT_MAX.
     * @tc.expected: step5. return -INT_MAX.
     */
    g_communicatorAgg->MockCommErrCode(-INT_MAX);
    res = g_tool.SyncTest(g_kvPtr, devicesVec, SYNC_MODE_PUSH_ONLY, res);
    EXCEPT_EQ(res, OK);
    for (const auto &pair : res) {
        LOGD("dev %s, res %d, expectStatus %d", pair.first.c_str(), pair.second, -INT_MAX);
        EXPECT_EQ(pair.second, -INT_MAX);
    }
    g_communicatorAgg->MockCommErrCode(E_OK);
}

/**
  * @tc.name: TestErrCodePassthrough002
  * @tc.desc: Test ErrCode Passthrough when sync time out and isDirectEnd is false
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: suyue
  */
HWTEST_F(KvDBSingleVerP2PComplexSyncTest, TestErrCodePassthrough002, TestSize.Level3)
{
    /**
     * @tc.steps: step1. device put data.
     * @tc.expected: step1. sync return OK.
     */
    std::vector<std::string> devicesVec;
    devicesVec.push_back(g_deviceE->GetDeviceId());
    EXCEPT_EQ(g_kvPtr->Put({'k', '1'}, {'v', '1'}), OK);

    /**
     * @tc.steps: step2. set messageId invalid and isDirectEnd is false
     * @tc.expected: step2. make sure deviceA push data failed due to timeout
     */
    g_communicatorAgg->RegOnDispatch([](const std::string &target, DistributedDB::Message *msg) {
        ASSERT_NE(msg, nullptr);
        if (target == DEVICE_E && msg->GetMessageId() == QUERY_SYNC_MESSAGE) {
            msg->SetMessageId(INVALID_MESSAGE_ID);
        }
    });
    g_communicatorAgg->MockDirectEndFlag(false);

    /**
     * @tc.steps: step3. call sync and mock errCode is E_BASE(positive number).
     * @tc.expected: step3. return TIME_OUT.
     */
    std::map<std::string, DBStatus> res;
    auto callback = [&res](const std::map<std::string, DBStatus> &map) {
        res = map;
    };
    Query sql = Query::Select().PrefixKey({'k', '1'});
    g_communicatorAgg->MockCommErrCode(E_BASE);
    EXPECT_EQ(g_kvPtr->Sync(devicesVec, DistributedDB::SYNC_MODE_PUSH_ONLY, callback, sql, true), OK);
    EXPECT_EQ(res.size(), devicesVec.size());
    EXPECT_EQ(res[DEVICE_E], TIME_OUT);

    /**
     * @tc.steps: step4. call sync and mock errCode is -E_BASE(negative number).
     * @tc.expected: step4. return -E_BASE.
     */
    g_communicatorAgg->MockCommErrCode(-E_BASE);
    EXPECT_EQ(g_kvPtr->Sync(devicesVec, DistributedDB::SYNC_MODE_PUSH_ONLY, callback, sql, true), OK);
    EXPECT_EQ(res.size(), devicesVec.size());
    EXPECT_EQ(res[DEVICE_E], -E_BASE);

    /**
     * @tc.steps: step5. call sync and mock errCode is E_OK(0).
     * @tc.expected: step5. return TIME_OUT.
     */
    g_communicatorAgg->MockCommErrCode(E_OK);
    EXPECT_EQ(g_kvPtr->Sync(devicesVec, DistributedDB::SYNC_MODE_PUSH_ONLY, callback, sql, true), OK);
    EXPECT_EQ(res.size(), devicesVec.size());
    EXPECT_EQ(res[DEVICE_E], TIME_OUT);

    /**
     * @tc.steps: step6. call sync and mock errCode is -INT_MAX.
     * @tc.expected: step6. return -INT_MAX.
     */
    g_communicatorAgg->MockCommErrCode(-INT_MAX);
    EXPECT_EQ(g_kvPtr->Sync(devicesVec, DistributedDB::SYNC_MODE_PUSH_ONLY, callback, sql, true), OK);
    EXPECT_EQ(res.size(), devicesVec.size());
    EXPECT_EQ(res[DEVICE_E], -INT_MAX);

    g_communicatorAgg->RegOnDispatch(nullptr);
    g_communicatorAgg->MockCommErrCode(E_OK);
    g_communicatorAgg->MockDirectEndFlag(true);
}