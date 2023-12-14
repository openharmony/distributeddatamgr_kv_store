/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifdef USE_RD_KERNEL
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>

#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kvdb_manager.h"
#include "platform_specific.h"
#include "process_system_api_adapter_impl.h"
#include "runtime_context.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    enum {
        SCHEMA_TYPE1 = 1,
        SCHEMA_TYPE2
    };
    // define some variables to init a KvStoreDelegateManager object.
    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    string g_testDir;
    KvStoreConfig g_config;

    DBStatus g_kvDelegateStatus = INVALID_ARGS;

    KvStoreNbDelegate *g_kvNbDelegatePtr = nullptr;
    auto g_kvNbDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvNbDelegatePtr));
    static const char *g_syncMMapFile = "./share_mem";
#ifndef OMIT_JSON
    void GenerateValidSchemaString(std::string &string, int num = SCHEMA_TYPE1)
    {
        switch (num) {
            case SCHEMA_TYPE1:
                string = "{\"SCHEMA_VERSION\":\"1.0\","
                         "\"SCHEMA_MODE\":\"STRICT\","
                         "\"SCHEMA_DEFINE\":{"
                         "\"field_name1\":\"BOOL\","
                         "\"field_name2\":{"
                         "\"field_name3\":\"INTEGER, NOT NULL\","
                         "\"field_name4\":\"LONG, DEFAULT 100\","
                         "\"field_name5\":\"DOUBLE, NOT NULL, DEFAULT 3.14\","
                         "\"field_name6\":\"STRING, NOT NULL, DEFAULT '3.1415'\","
                         "\"field_name7\":[],"
                         "\"field_name8\":{}"
                         "}"
                         "},"
                         "\"SCHEMA_INDEXES\":[\"$.field_name1\", \"$.field_name2.field_name6\"]}";
                break;
            case SCHEMA_TYPE2:
                string = "{\"SCHEMA_VERSION\":\"1.0\","
                         "\"SCHEMA_MODE\":\"STRICT\","
                         "\"SCHEMA_DEFINE\":{"
                         "\"field_name1\":\"LONG, DEFAULT 100\","
                         "\"field_name2\":{"
                         "\"field_name3\":\"INTEGER, NOT NULL\","
                         "\"field_name4\":\"LONG, DEFAULT 100\","
                         "\"field_name5\":\"DOUBLE, NOT NULL, DEFAULT 3.14\","
                         "\"field_name6\":\"STRING, NOT NULL, DEFAULT '3.1415'\""
                         "}"
                         "},"
                         "\"SCHEMA_INDEXES\":[\"$.field_name1\", \"$.field_name2.field_name6\"]}";
                break;
            default:
                return;
        }
    }
#endif
}

class DistributedDBInterfacesDatabaseRdKernelTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBInterfacesDatabaseRdKernelTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

void DistributedDBInterfacesDatabaseRdKernelTest::TearDownTestCase(void)
{
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

void DistributedDBInterfacesDatabaseRdKernelTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBInterfacesDatabaseRdKernelTest::TearDown(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

/**
  * @tc.name: GetKvStore003
  * @tc.desc: Get kv store through different SecurityOption, abnormal or normal.
  * @tc.type: FUNC
  * @tc.require: AR000EV1G2
  * @tc.author: liuwenkai
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Obtain the kvStore through the GetKvStore interface of the delegate manager
     *  using the parameter secOption(abnormal).
     * @tc.expected: step1. Returns a null kvstore and error code is not OK.
     */
    std::shared_ptr<IProcessSystemApiAdapter> g_adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(g_adapter);
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    int abnormalNum = -100;
    option.secOption.securityLabel = abnormalNum;
    option.secOption.securityFlag = abnormalNum;
    g_mgr.GetKvStore("distributed_getkvstore_003", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
    EXPECT_TRUE(g_kvDelegateStatus != OK);

    /**
     * @tc.steps: step2. Obtain the kvStore through the GetKvStore interface of the delegate manager
     *  using the parameter secOption(normal).
     * @tc.expected: step2. Returns a non-null kvstore and error code is OK.
     */
    option.secOption.securityLabel = S3;
    option.secOption.securityFlag = 0;
    g_mgr.GetKvStore("distributed_getkvstore_003", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    KvStoreNbDelegate *kvNbDelegatePtr1 = g_kvNbDelegatePtr;

    /**
     * @tc.steps: step3. Obtain the kvStore through the GetKvStore interface of the delegate manager
     *  using the parameter secOption(normal but not same as last).
     * @tc.expected: step3. Returns a null kvstore and error code is not OK.
     */
    option.secOption.securityLabel = S3;
    option.secOption.securityFlag = 1;
    g_mgr.GetKvStore("distributed_getkvstore_003", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
    EXPECT_TRUE(g_kvDelegateStatus != OK);

    /**
    * @tc.steps: step4. Obtain the kvStore through the GetKvStore interface of the delegate manager
    *  using the parameter secOption(normal and same as last).
    * @tc.expected: step4. Returns a non-null kvstore and error code is OK.
    */
    option.secOption.securityLabel = S3;
    option.secOption.securityFlag = 0;
    g_mgr.GetKvStore("distributed_getkvstore_003", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);

    string retStoreId = g_kvNbDelegatePtr->GetStoreId();
    EXPECT_TRUE(retStoreId == "distributed_getkvstore_003");

    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(kvNbDelegatePtr1), OK);
    g_kvNbDelegatePtr = nullptr;
    EXPECT_TRUE(g_mgr.DeleteKvStore("distributed_getkvstore_003") == OK);
}

/**
  * @tc.name: GetKvStore004
  * @tc.desc: Get kv store parameters with Observer and Notifier, then trigger callback.
  * @tc.type: FUNC
  * @tc.require: AR000EV1G2
  * @tc.author: liuwenkai
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Obtain the kvStore through the GetKvStore interface of the delegate manager
     *  using the parameter observer, notifier, key.
     * @tc.expected: step1. Returns a non-null kvstore and error code is OK.
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    KvStoreObserverUnitTest *observer = new (std::nothrow) KvStoreObserverUnitTest;
    ASSERT_NE(observer, nullptr);
    Key key;
    Value value1;
    Value value2;
    key.push_back(1);
    value1.push_back(1);
    value2.push_back(2);
    option.key = key;
    option.observer = observer;
    option.mode = OBSERVER_CHANGES_NATIVE;
    int sleepTime = 100;
    g_mgr.GetKvStore("distributed_getkvstore_004", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);

    /**
     * @tc.steps: step2. Put(k1,v1) to db and check the observer info.
     * @tc.expected: step2. Put successfully and trigger notifier callback.
     */
    EXPECT_TRUE(g_kvNbDelegatePtr->Put(key, value1) == OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    LOGI("observer count:%lu", observer->GetCallCount());
    EXPECT_TRUE(observer->GetCallCount() == 1);

    /**
     * @tc.steps: step3. put(k1,v2) to db and check the observer info.
     * @tc.expected: step3. put successfully
     */
    EXPECT_TRUE(g_kvNbDelegatePtr->Put(key, value2) == OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    LOGI("observer count:%lu", observer->GetCallCount());
    EXPECT_TRUE(observer->GetCallCount() == 2);

    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvNbDelegatePtr = nullptr;
    delete observer;
    observer = nullptr;
    EXPECT_TRUE(g_mgr.DeleteKvStore("distributed_getkvstore_004") == OK);
}

/**
  * @tc.name: GetKvStore005
  * @tc.desc: Get kv store parameters with rd shared mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open db with share mode and readOnly is false
     * @tc.expected: step1. open ok
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.rdconfig.readOnly = false;
    g_mgr.GetKvStore("distributed_getkvstore_005", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    /**
     * @tc.steps: step2. Put into the database some data.
     * @tc.expected: step2. Put returns OK.
     */
    std::vector<Key> keys;
    std::vector<Key> values;
    uint32_t totalSize = 20u; // 20 items
    for (size_t i = 0; i < totalSize; i++) {
        Entry entry;
        DistributedDBToolsUnitTest::GetRandomKeyValue(entry.key, static_cast<uint32_t>(i + 1));
        DistributedDBToolsUnitTest::GetRandomKeyValue(entry.value);
        EXPECT_EQ(g_kvNbDelegatePtr->Put(entry.key, entry.value), OK);
        keys.push_back(entry.key);
        values.push_back(entry.value);
    }
    /**
     * @tc.steps: step3. Get kv ok
     * @tc.expected: step3. returns OK
     */
    for (size_t i = 0; i < keys.size(); i++) {
        Value value;
        EXPECT_EQ(g_kvNbDelegatePtr->Get(keys[i], value), OK);
        EXPECT_TRUE(value == values[i]);
    }
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
}

/**
  * @tc.name: GetKvStore006
  * @tc.desc: Get kv store parameters with rd shared mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open db with share mode and readOnly is true
     * @tc.expected: step1. open ok
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.rdconfig.readOnly = false;
    /**
     * @tc.steps: step2. write db handle write data and close
     * @tc.expected: step2. close ok
     */
    g_mgr.GetKvStore("distributed_getkvstore_006", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    Entry entry = {{'1', '2'}};
    EXPECT_EQ(g_kvNbDelegatePtr->Put(entry.key, entry.value), OK);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);

    /**
     * @tc.steps: step3. read db handle open
     * @tc.expected: step3. open ok
     */
    option.rdconfig.readOnly = true;
    g_mgr.GetKvStore("distributed_getkvstore_006", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    /**
     * @tc.steps: step4. read db handle put entry
     * @tc.expected: step4. return not ok
     */
    EXPECT_EQ(g_kvNbDelegatePtr->Put(entry.key, entry.value), NO_PERMISSION);
    EXPECT_EQ(g_kvNbDelegatePtr->PutBatch({entry}), NO_PERMISSION);
    EXPECT_EQ(g_kvNbDelegatePtr->Delete(entry.key), NO_PERMISSION);
    EXPECT_EQ(g_kvNbDelegatePtr->DeleteBatch({entry.key}), NO_PERMISSION);
    /**
     * @tc.steps: step5. read db handle get entry and close
     * @tc.expected: step5. return not ok
     */
    Value value;
    EXPECT_EQ(g_kvNbDelegatePtr->Get(entry.key, value), OK);
    EXPECT_TRUE(value == entry.value);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
}

/**
  * @tc.name: GetKvStore007
  * @tc.desc: Get kv store parameters with wrong rd shared mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open db with not share mode and readOnly is true
     * @tc.expected: step1. open not ok
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.rdconfig.readOnly = true;
    g_mgr.GetKvStore("distributed_getkvstore_007", option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, INVALID_ARGS);
    ASSERT_EQ(g_kvNbDelegatePtr, nullptr);
}

/**
  * @tc.name: GetKvStore008
  * @tc.desc: Get kv store parameters with use Integrity check
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open db and readOnly is false and IntegrityCheck is true
     * @tc.expected: step1. open ok
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.isNeedIntegrityCheck = true;
    option.isNeedRmCorruptedDb = true;
    g_mgr.GetKvStore("distributed_getkvstore_009", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);

    /**
     * @tc.steps: step2. make db damaged and open db
     * @tc.expected: step2. open ok
     */
    std::string identifier = KvStoreDelegateManager::GetKvStoreIdentifier(USER_ID, APP_ID,
        "distributed_getkvstore_009");
    std::string dbFliePath = DistributedDBToolsUnitTest::GetKvNbStoreDirectory(identifier,
        "single_ver/main/gen_natural_store.db", g_testDir);
    DistributedDBToolsUnitTest::ModifyDatabaseFile(dbFliePath);

    g_mgr.GetKvStore("distributed_getkvstore_009", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
}

/**
  * @tc.name: GetKvStore009
  * @tc.desc: Get kv store parameters use Integrity check and RmCorruptedDb
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore009, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open db and readOnly is false and RmCorruptedDb false
     * @tc.expected: step1. open ok
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.isNeedIntegrityCheck = true;
    g_mgr.GetKvStore("distributed_getkvstore_010", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);

    g_mgr.GetKvStore("distributed_getkvstore_010", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
}

/**
  * @tc.name: GetKvStore010
  * @tc.desc: Get kv store parameters when crcCheck is true and not create db
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore010, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open db
     * @tc.expected: step1. open not ok
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.createIfNecessary = false;
    option.isNeedIntegrityCheck = true;
    option.rdconfig.readOnly = true;
    g_mgr.GetKvStore("distributed_getkvstore_011", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == INVALID_ARGS);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
}

/**
  * @tc.name: GetKvStore013
  * @tc.desc: Get kv store parameters in readOnly mode and RmCorruptedDb is true
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhuwentao
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStore013, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open db and readOnly is true and RmCorruptedDb true
     * @tc.expected: step1. open ok
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.isNeedIntegrityCheck = true;
    option.isNeedRmCorruptedDb = true;
    option.rdconfig.readOnly = true;
    g_mgr.GetKvStore("distributed_getkvstore_013", option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == INVALID_ARGS);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
}

/**
  * @tc.name: GetKvStoreWithInvalidOption001
  * @tc.desc: Get kv store with invalid option.storageEngineType
  * @tc.type:
  * @tc.require:
  * @tc.author: wanyi
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetKvStoreWithInvalidOption001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get kv store with invalid null option.storageEngineType
     * @tc.expected: step1. INVALID_ARGS
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = "";
    g_mgr.GetKvStore("GetKvStoreWithInvalidOption001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == INVALID_ARGS);

    /**
     * @tc.steps: step2. Get kv store with invalid option.storageEngineType
     * @tc.expected: step2. INVALID_ARGS
     */
    option.storageEngineType = "invalidType";
    g_mgr.GetKvStore("GetKvStoreWithInvalidOption001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == INVALID_ARGS);
}

/**
  * @tc.name: RepeatCloseKvStore001
  * @tc.desc: Close the kv store repeatedly and check the database.
  * @tc.type: FUNC
  * @tc.require: AR000C2F0C AR000CQDV7
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, RepeatCloseKvStore001, TestSize.Level2)
{
    /**
     * @tc.steps: step1. Obtain the kvStore through the GetKvStore interface of
     *  the delegate manager using the parameter createIfNecessary(true)
     * @tc.expected: step1. Returns a non-null kvstore and error code is OK.
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    g_mgr.GetKvStore("RepeatCloseKvStore_001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    static const size_t totalSize = 50;

    /**
     * @tc.steps: step2. Put into the database some data.
     * @tc.expected: step2. Put returns OK.
     */
    std::vector<Key> keys;
    for (size_t i = 0; i < totalSize; i++) {
        Entry entry;
        DistributedDBToolsUnitTest::GetRandomKeyValue(entry.key, static_cast<uint32_t>(i + 1));
        DistributedDBToolsUnitTest::GetRandomKeyValue(entry.value);
        EXPECT_EQ(g_kvNbDelegatePtr->Put(entry.key, entry.value), OK);
        keys.push_back(entry.key);
    }

    /**
     * @tc.steps: step3. Delete the data from the database, and close the database, reopen the database and
     *  get the data.
     * @tc.expected: step3. Delete returns OK, Close returns OK and Get returns NOT_FOUND.
     */
    for (size_t i = 0; i < keys.size(); i++) {
        Value value;
        EXPECT_EQ(g_kvNbDelegatePtr->Delete(keys[i]), OK);
        EXPECT_EQ(g_kvNbDelegatePtr->Get(keys[i], value), NOT_FOUND);
        EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        g_mgr.GetKvStore("RepeatCloseKvStore_001", option, g_kvNbDelegateCallback);
        EXPECT_EQ(g_kvNbDelegatePtr->Get(keys[i], value), NOT_FOUND);
    }
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    /**
     * @tc.steps: step4. Delete the kvstore created before.
     * @tc.expected: step4. Delete returns OK.
     */
    EXPECT_EQ(g_mgr.DeleteKvStore("RepeatCloseKvStore_001"), OK);
}
#ifndef OMIT_JSON
/**
  * @tc.name: CreatKvStoreWithSchema001
  * @tc.desc: Create non-memory KvStore with schema, check if create success.
  * @tc.type: FUNC
  * @tc.require: AR000DR9K2
  * @tc.author: weifeng
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, CreatKvStoreWithSchema001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. create a new db(non-memory, non-encrypt), with valid schema;
     * @tc.expected: step1. Returns a null kvstore and error code is NOT_SUPPORT.
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    GenerateValidSchemaString(option.schema);
    g_mgr.GetKvStore("CreatKvStoreWithSchema_001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == NOT_SUPPORT);
    g_kvNbDelegatePtr = nullptr;
}
#endif
/**
  * @tc.name: OpenKvStoreWithStoreOnly001
  * @tc.desc: open the kv store with the option that createDirByStoreIdOnly is true.
  * @tc.type: FUNC
  * @tc.require: AR000DR9K2
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, OpenKvStoreWithStoreOnly001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open the kv store with the option that createDirByStoreIdOnly is true.
     * @tc.expected: step1. Returns OK.
     */
    KvStoreNbDelegate::Option option;
    option.createDirByStoreIdOnly = true;
    option.storageEngineType = GAUSSDB_RD;
    g_mgr.GetKvStore("StoreOnly001", option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    auto kvStorePtr = g_kvNbDelegatePtr;
    /**
     * @tc.steps: step2. open the same store with the option that createDirByStoreIdOnly is false.
     * @tc.expected: step2. Returns NOT OK.
     */
    option.createDirByStoreIdOnly = false;
    g_kvNbDelegatePtr = nullptr;
    g_mgr.GetKvStore("StoreOnly001", option, g_kvNbDelegateCallback);
    EXPECT_EQ(g_kvDelegateStatus, INVALID_ARGS);
    /**
     * @tc.steps: step3. close the kvstore and delete the kv store;
     * @tc.expected: step3. Returns OK.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(kvStorePtr), OK);
    kvStorePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore("StoreOnly001"), OK);
}

/**
  * @tc.name: GetDBWhileOpened001
  * @tc.desc: open the kv store with the option that createDirByStoreIdOnly is true.
  * @tc.type: FUNC
  * @tc.require: AR000E8S2V
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, GetDBWhileOpened001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get the connection.
     * @tc.expected: step1. Returns OK.
     */
    KvDBProperties property;
    std::string storeId = "openTest";
    std::string origId = USER_ID + "-" + APP_ID + "-" + storeId;
    std::string identifier = DBCommon::TransferHashString(origId);
    std::string hexDir = DBCommon::TransferStringToHex(identifier);
    property.SetStringProp(KvDBProperties::IDENTIFIER_DATA, identifier);
    property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, hexDir);
    property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    property.SetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, true);
    property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_RD_KERNAL);
    property.SetBoolProp(KvDBProperties::MEMORY_MODE, false);
    property.SetBoolProp(KvDBProperties::ENCRYPTED_MODE, false);
    property.SetBoolProp(KvDBProperties::CREATE_DIR_BY_STORE_ID_ONLY, true);
    property.SetStringProp(KvDBProperties::APP_ID, APP_ID);
    property.SetStringProp(KvDBProperties::USER_ID, USER_ID);
    property.SetStringProp(KvDBProperties::APP_ID, storeId);

    int errCode = E_OK;
    auto connection1 = KvDBManager::GetDatabaseConnection(property, errCode, false);
    EXPECT_EQ(errCode, E_OK);
    /**
     * @tc.steps: step2. Get the connection with the para: isNeedIfOpened is false.
     * @tc.expected: step2. Returns -E_ALREADY_OPENED.
     */
    auto connection2 = KvDBManager::GetDatabaseConnection(property, errCode, false);
    EXPECT_EQ(errCode, -E_ALREADY_OPENED);
    EXPECT_EQ(connection2, nullptr);

    /**
     * @tc.steps: step3. Get the connection with the para: isNeedIfOpened is true.
     * @tc.expected: step3. Returns E_OK.
     */
    auto connection3 = KvDBManager::GetDatabaseConnection(property, errCode, true);
    EXPECT_EQ(errCode, OK);
    EXPECT_NE(connection3, nullptr);

    KvDBManager::ReleaseDatabaseConnection(connection1);
    KvDBManager::ReleaseDatabaseConnection(connection3);
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
}
namespace {
    void OpenCloseDatabase(const std::string &storeId)
    {
        KvStoreNbDelegate::Option option;
        option.storageEngineType = GAUSSDB_RD;
        DBStatus status;
        KvStoreNbDelegate *delegate = nullptr;
        auto nbDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
            placeholders::_1, placeholders::_2, std::ref(status), std::ref(delegate));
        int totalNum = 0;
        for (size_t i = 0; i < 100; i++) { // cycle 100 times.
            g_mgr.GetKvStore(storeId, option, nbDelegateCallback);
            if (delegate != nullptr) {
                totalNum++;
            }
            g_mgr.CloseKvStore(delegate);
            delegate = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        LOGD("Succeed %d times", totalNum);
    }

    void FreqOpenClose001()
    {
        std::string storeId = "FrqOpenClose001";
        std::thread t1(OpenCloseDatabase, storeId);
        std::thread t2(OpenCloseDatabase, storeId);
        std::thread t3(OpenCloseDatabase, storeId);
        std::thread t4(OpenCloseDatabase, storeId);
        t1.join();
        t2.join();
        t3.join();
        t4.join();
        EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
    }

    void FreqOpenCloseDel001()
    {
        std::string storeId = "FrqOpenCloseDelete001";
        std::thread t1(OpenCloseDatabase, storeId);
        std::thread t2([&]() {
            for (int i = 0; i < 10000; i++) { // loop 10000 times
                DBStatus status = g_mgr.DeleteKvStore(storeId);
                LOGI("delete res %d", status);
                EXPECT_TRUE(status == OK || status == BUSY || status == NOT_FOUND);
            }
        });
        t1.join();
        t2.join();
    }
}

/**
  * @tc.name: FreqOpenCloseDel001
  * @tc.desc: Open/close/delete the kv store concurrently.
  * @tc.type: FUNC
  * @tc.require: AR000DR9K2
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, FreqOpenCloseDel001, TestSize.Level2)
{
    ASSERT_NO_FATAL_FAILURE(FreqOpenCloseDel001());
}

/**
  * @tc.name: FreqOpenClose001
  * @tc.desc: Open and close the kv store concurrently.
  * @tc.type: FUNC
  * @tc.require: AR000DR9K2
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, FreqOpenClose001, TestSize.Level2)
{
    ASSERT_NO_FATAL_FAILURE(FreqOpenClose001());
}

/**
  * @tc.name: CheckKvStoreDir001
  * @tc.desc: Delete the kv store with the option that createDirByStoreIdOnly is true.
  * @tc.type: FUNC
  * @tc.require: AR000CQDV7
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, CheckKvStoreDir001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. open the kv store with the option that createDirByStoreIdOnly is true.
     * @tc.expected: step1. Returns OK.
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.createDirByStoreIdOnly = true;
    const std::string storeId("StoreOnly002");
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    std::string testSubDir;
    EXPECT_EQ(KvStoreDelegateManager::GetDatabaseDir(storeId, testSubDir), OK);
    std::string dataBaseDir = g_testDir + "/" + testSubDir;
    EXPECT_GE(access(dataBaseDir.c_str(), F_OK), 0);

    /**
     * @tc.steps: step2. delete the kv store, and check the directory.
     * @tc.expected: step2. the directory is removed.
     */
    EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
    LOGI("[%s]", dataBaseDir.c_str());
    ASSERT_EQ(OS::CheckPathExistence(dataBaseDir), false);
}

/**
 * @tc.name: CompressionRate1
 * @tc.desc: Open the kv store with invalid compressionRate and open successfully.
 * @tc.type: FUNC
 * @tc.require: AR000G3QTT
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, CompressionRate1, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Open the kv store with the option that comressionRate is invalid.
     * @tc.expected: step1. Open kv store failed. Returns DB_ERROR.
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.compressionRate = 0; // 0 is invalid.
    const std::string storeId("CompressionRate1");
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == NOT_SUPPORT);

    g_mgr.CloseKvStore(g_kvNbDelegatePtr);
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), NOT_FOUND);
}

/**
 * @tc.name: CompressionRate2
 * @tc.desc: Open the kv store with again with different compression option.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lianhuix
 */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, CompressionRate2, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Open the kv store with the option that comressionRate is invalid.
     * @tc.expected: step1. Open kv store successfully. Returns OK.
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.compressionRate = 70; // 70 compression rate.
    const std::string storeId("CompressionRate1");
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr == nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == NOT_SUPPORT);

    /**
     * @tc.steps: step2. Open again with different compression option
     * @tc.expected: step2. Open kv store failed. Returns NOT_SUPPORT.
     */
    DBStatus status;
    KvStoreNbDelegate *delegate = nullptr;
    auto callback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(status), std::ref(delegate));

    option.compressionRate = 80; // 80 compression rate.
    g_mgr.GetKvStore(storeId, option, callback);
    ASSERT_TRUE(delegate == nullptr);
    EXPECT_TRUE(status == NOT_SUPPORT);

    option.isNeedCompressOnSync = false;
    option.compressionRate = 70; // 70 compression rate.
    g_mgr.GetKvStore(storeId, option, callback);
    ASSERT_TRUE(delegate == nullptr);
    EXPECT_TRUE(status == NOT_SUPPORT);

    /**
     * @tc.steps: step3. Close kv store
     * @tc.expected: step3. NOT_FOUND.
     */
    g_mgr.CloseKvStore(g_kvNbDelegatePtr);
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), NOT_FOUND);
}

HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, DataInterceptor1, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Open the kv store with the option that comressionRate is invalid.
     * @tc.expected: step1. Open kv store successfully. Returns OK.
     */
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    const std::string storeId("DataInterceptor1");
    g_mgr.GetKvStore(storeId, option, g_kvNbDelegateCallback);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);
    EXPECT_TRUE(g_kvDelegateStatus == OK);

    auto interceptorCallback = [] (InterceptedData &data, const std::string &sourceID,
        const std::string &targetId) -> int {
        return OK;
    };

    EXPECT_EQ(g_kvNbDelegatePtr->SetPushDataInterceptor(interceptorCallback), NOT_SUPPORT);

    g_mgr.CloseKvStore(g_kvNbDelegatePtr);
    g_kvNbDelegatePtr = nullptr;
    EXPECT_EQ(g_mgr.DeleteKvStore(storeId), OK);
}

int *GetProcessSyncMemory(int *fd)
{
    *fd = open(g_syncMMapFile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (*fd <= 0) {
        return NULL;
    }
    if (ftruncate(*fd, sizeof(int)) < 0) {
        return NULL;
    }
    int *step = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if (step == MAP_FAILED) {
        return NULL;
    }
    *step = 0;
    return step;
}

void ClearProcessSyncMemory(int fd, int *buffer)
{
    ASSERT_GE(munmap(buffer, sizeof(int)), 0);
    ASSERT_EQ(close(fd), 0);
    ASSERT_GE(unlink(g_syncMMapFile), 0);
}

void WaitCondition(int *X, int Y)
{
    while (*X != Y) {
        usleep(10000);  // sleep 10000 microseconds
    }
}

KvStoreNbDelegate::Option GetWriteOption()
{
    KvStoreNbDelegate::Option option;
    option.storageEngineType = GAUSSDB_RD;
    option.rdconfig.readOnly = false;
    return option;
}

/**
 * @tc.name: MultiProcessOpenDb001
 * @tc.desc: Test 2 processes concurrently start the same KvStoreNbDelegate
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: huangboxin
 */
HWTEST_F(DistributedDBInterfacesDatabaseRdKernelTest, MultiProcessOpenDb001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create shared variable memory to share variables
     * @tc.expected: step1. OK
     */
    int fd = 0;
    int *step = GetProcessSyncMemory(&fd);
    /**
     * @tc.steps: step2. use 2 process to get kv store
     * @tc.expected: step2. Returns OK and OVER_MAX_LIMITS
     */
    DBStatus result;
    KvStoreNbDelegate::Option writeOption = GetWriteOption();
    pid_t pid = fork();
    ASSERT_GE(pid, 0);
    if (pid == 0) {
        (*step)++;
        WaitCondition(step, 1);

        KvStoreDelegateManager mgr1(APP_ID, USER_ID);
        KvStoreNbDelegate *delegate1 = nullptr;
        mgr1.SetKvStoreConfig(g_config);
        auto delegateCallback1 = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(result), std::ref(delegate1));
        mgr1.GetKvStore("multiprocess_getkvstore_001", writeOption, delegateCallback1);
        ASSERT_TRUE(delegate1 != nullptr);

        if (result != OK) {
            EXPECT_EQ(result, OVER_MAX_LIMITS);
        }
        (*step)++;
        WaitCondition(step, 3);
        if (delegate1 != nullptr) {
            EXPECT_EQ(mgr1.CloseKvStore(delegate1), OK);
            delegate1 = nullptr;
        }
        // quit the child process
        exit(0);
    } else {
        WaitCondition(step, 1);

        KvStoreDelegateManager mgr2(APP_ID, USER_ID);
        KvStoreNbDelegate *delegate2 = nullptr;
        mgr2.SetKvStoreConfig(g_config);
        auto delegateCallback2 = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(result), std::ref(delegate2));
        mgr2.GetKvStore("multiprocess_getkvstore_001", writeOption, delegateCallback2);

        if (result != OK) {
            EXPECT_EQ(result, OVER_MAX_LIMITS);
        }
        (*step)++;
        WaitCondition(step, 3);
        if (delegate2 != nullptr) {
            EXPECT_EQ(mgr2.CloseKvStore(delegate2), OK);
            delegate2 = nullptr;
        }
    }
    waitpid(pid, NULL, 0);
    ClearProcessSyncMemory(fd, step);
}

#endif // USE_RD_KERNEL
