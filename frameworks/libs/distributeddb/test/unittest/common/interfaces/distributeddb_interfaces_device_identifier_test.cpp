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
#include <thread>

#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "distributeddb_data_generate_unit_test.h"
#include "kv_store_nb_delegate_impl.h"
#include "platform_specific.h"
#include "process_system_api_adapter_impl.h"
#include "sqlite_single_ver_natural_store.h"
#include "sqlite_single_ver_natural_store_connection.h"
#include "storage_engine_manager.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    KvStoreDelegateManager g_mgr(APP_ID, USER_ID);
    KvStoreConfig g_config;
    KvDBProperties g_property;
    KvStoreNbDelegate *g_kvNbDelegatePtr = nullptr;
    DBStatus g_kvDelegateStatus = INVALID_ARGS;
    SQLiteSingleVerNaturalStore *g_store = nullptr;
    DistributedDB::SQLiteSingleVerNaturalStoreConnection *g_connection = nullptr;
    const string STORE_ID = STORE_ID_SYNC;
    const int TIME_LAG = 100;
    const std::string DEVICE_ID_1 = "ABC";
    auto g_kvNbDelegateCallback = bind(&DistributedDBToolsUnitTest::KvStoreNbDelegateCallback,
        placeholders::_1, placeholders::_2, std::ref(g_kvDelegateStatus), std::ref(g_kvNbDelegatePtr));
}

class DistributedDBDeviceIdentifierTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBDeviceIdentifierTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_config.dataDir = g_testDir;
    g_mgr.SetKvStoreConfig(g_config);

    string dir = g_testDir + STORE_ID + "/" + DBConstant::SINGLE_SUB_DIR;
    DIR *dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }
    g_property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    g_property.SetStringProp(KvDBProperties::STORE_ID, STORE_ID);
    g_property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE);
}

void DistributedDBDeviceIdentifierTest::TearDownTestCase(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + STORE_ID + "/" + DBConstant::SINGLE_SUB_DIR) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_LAG));
}

void DistributedDBDeviceIdentifierTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    KvStoreNbDelegate::Option option = {true, false, false};
    g_mgr.GetKvStore(STORE_ID, option, g_kvNbDelegateCallback);
    EXPECT_TRUE(g_kvDelegateStatus == OK);
    ASSERT_TRUE(g_kvNbDelegatePtr != nullptr);

    g_store = new (std::nothrow) SQLiteSingleVerNaturalStore;
    ASSERT_NE(g_store, nullptr);
    ASSERT_EQ(g_store->Open(g_property), E_OK);

    int erroCode = E_OK;
    g_connection = static_cast<SQLiteSingleVerNaturalStoreConnection *>(g_store->GetDBConnection(erroCode));
    ASSERT_NE(g_connection, nullptr);
    g_store->DecObjRef(g_store);
    EXPECT_EQ(erroCode, E_OK);
}

void DistributedDBDeviceIdentifierTest::TearDown(void)
{
    if (g_connection != nullptr) {
        g_connection->Close();
    }

    g_store = nullptr;

    if (g_kvNbDelegatePtr != nullptr) {
        EXPECT_EQ(g_mgr.CloseKvStore(g_kvNbDelegatePtr), OK);
        g_kvNbDelegatePtr = nullptr;
        EXPECT_TRUE(g_mgr.DeleteKvStore(STORE_ID) == OK);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME_LAG));
}

/**
  * @tc.name: DeviceIdentifier001
  * @tc.desc: Set pragma to be GET_DEVICE_IDENTIFIER_OF_ENTRY,
  * set Key to be null and origDevice to be false, expect return INVALID_ARGS.
  * @tc.type: FUNC
  * @tc.require: AR000D08KV
  * @tc.author: maokeheng
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, DeviceIdentifier001, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Sync 1K data from DEVICE_B into database, with Key= KEY_1, and Value = VALUE_1.
     * @tc.expected: step1. Expect return true.
     */
    g_kvNbDelegatePtr->Put(KEY_1, VALUE_1);

    /**
     * @tc.steps:step2. Set PragmaCmd to be GET_DEVICE_IDENTIFIER_OF_ENTRY, and set input key to be null
     * @tc.expected: step2. Expect return INVALID_ARGS.
     */
    Key keyNull;
    PragmaEntryDeviceIdentifier param;
    param.key = keyNull;
    param.origDevice = false;
    PragmaData input = static_cast<void *>(&param);
    EXPECT_EQ(g_kvNbDelegatePtr->Pragma(GET_DEVICE_IDENTIFIER_OF_ENTRY, input), INVALID_ARGS);
}

/**
  * @tc.name: DeviceIdentifier002
  * @tc.desc: Set pragma to be GET_DEVICE_IDENTIFIER_OF_ENTRY,
  * set Key to be null and origDevice to be true, expect return INVALID_ARGS.
  * @tc.type: FUNC
  * @tc.require: AR000D08KV
  * @tc.author: maokeheng
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, DeviceIdentifier002, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Sync 1K data from DEVICE_B into database, with Key= KEY_1, and Value = VALUE_1.
     * @tc.expected: step1. Expect return true.
     */
    g_kvNbDelegatePtr->Put(KEY_1, VALUE_1);

    /**
     * @tc.steps:step2. Set PragmaCmd to be GET_DEVICE_IDENTIFIER_OF_ENTRY, and set input key to be null
     * @tc.expected: step2. Expect return INVALID_ARGS.
     */
    Key keyNull;
    PragmaEntryDeviceIdentifier param;
    param.key = keyNull;
    param.origDevice = true;
    PragmaData input = static_cast<void *>(&param);
    EXPECT_EQ(g_kvNbDelegatePtr->Pragma(GET_DEVICE_IDENTIFIER_OF_ENTRY, input), INVALID_ARGS);
}

/**
  * @tc.name: DeviceIdentifier003
  * @tc.desc: Set pragma to be GET_DEVICE_IDENTIFIER_OF_ENTRY and origDevice to be false.
  * Check if a non-existing key will return NOT_FOUND.
  * @tc.type: FUNC
  * @tc.require: AR000D08KV
  * @tc.author: maokeheng
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, DeviceIdentifier003, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Sync 1K data from DEVICE_B into database, with Key= KEY_1, and Value = VALUE_1.
     * @tc.expected: step1. Expect return true.
     */
    g_kvNbDelegatePtr->Put(KEY_1, VALUE_1);
    /**
     * @tc.steps:step2. Set PragmaCmd to be GET_DEVICE_IDENTIFIER_OF_ENTRY, and set Key= Key_2
     * @tc.expected: step2. Expect return NOT_FOUND.
     */
    PragmaEntryDeviceIdentifier param;
    param.key = KEY_2;
    param.origDevice = false;
    PragmaData input = static_cast<void *>(&param);
    EXPECT_EQ(g_kvNbDelegatePtr->Pragma(GET_DEVICE_IDENTIFIER_OF_ENTRY, input), NOT_FOUND);
}

/**
  * @tc.name: DeviceIdentifier004
  * @tc.desc: Set pragma to be GET_DEVICE_IDENTIFIER_OF_ENTRY and origDevice to be true.
  * Check if a non-existing key will return NOT_FOUND.
  * @tc.type: FUNC
  * @tc.require: AR000D08KV
  * @tc.author: maokeheng
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, DeviceIdentifier004, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Sync 1K data from DEVICE_B into database, with Key= KEY_1, and Value = VALUE_1.
     * @tc.expected: step1. Expect return true.
     */
    g_kvNbDelegatePtr->Put(KEY_1, VALUE_1);
    /**
     * @tc.steps:step2. Set PragmaCmd to be GET_DEVICE_IDENTIFIER_OF_ENTRY, and set Key= Key_2
     * @tc.expected: step2. Expect return NOT_FOUND.
     */
    PragmaEntryDeviceIdentifier param;
    param.key = KEY_2;
    param.origDevice = true;
    PragmaData input = static_cast<void *>(&param);
    EXPECT_EQ(g_kvNbDelegatePtr->Pragma(GET_DEVICE_IDENTIFIER_OF_ENTRY, input), NOT_FOUND);
}

/**
  * @tc.name: DeviceIdentifier005
  * @tc.desc: Set pragma to be GET_DEVICE_IDENTIFIER_OF_ENTRY and origDevice to be false. check if returns OK.
  * @tc.type: FUNC
  * @tc.require: AR000D08KV
  * @tc.author: maokeheng
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, DeviceIdentifier005, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Sync 1K data from DEVICE_B into database, with Key= KEY_1, and Value = VALUE_1.
     * @tc.expected: step1. Expect return true.
     */
    g_kvNbDelegatePtr->Put(KEY_1, VALUE_1);
    /**
     * @tc.steps:step2. Set PragmaCmd = GET_DEVICE_IDENTIFIER_OF_ENTRY, Key= Key_1, origDevice = false.
     * @tc.expected: step2. Expect return deviceIdentifier is the same as deviceIdentifier of DEVICE_B.
     */
    PragmaEntryDeviceIdentifier param;
    param.key = KEY_1;
    param.origDevice = false;
    PragmaData input = static_cast<void *>(&param);
    EXPECT_EQ(g_kvNbDelegatePtr->Pragma(GET_DEVICE_IDENTIFIER_OF_ENTRY, input), OK);
}

/**
  * @tc.name: DeviceIdentifier006
  * @tc.desc: Set pragma to be GET_DEVICE_IDENTIFIER_OF_ENTRY and origDevice to be true. check if returns OK.
  * @tc.type: FUNC
  * @tc.require: AR000D08KV
  * @tc.author: maokeheng
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, DeviceIdentifier006, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Sync 1K data from DEVICE_B into database, with Key= KEY_1, and Value = VALUE_1.
     * @tc.expected: step1. Expect return true.
     */
    g_kvNbDelegatePtr->Put(KEY_1, VALUE_1);
    /**
     * @tc.steps:step2. Set PragmaCmd = GET_DEVICE_IDENTIFIER_OF_ENTRY, Key= Key_1, origDevice = false.
     * @tc.expected: step2. Expect return deviceIdentifier is the same as deviceIdentifier of DEVICE_B.
     */
    PragmaEntryDeviceIdentifier param;
    param.key = KEY_1;
    param.origDevice = true;
    PragmaData input = static_cast<void *>(&param);
    EXPECT_EQ(g_kvNbDelegatePtr->Pragma(GET_DEVICE_IDENTIFIER_OF_ENTRY, input), OK);
}

/**
  * @tc.name: DeviceIdentifier007
  * @tc.desc: Set pragma to be GET_IDENTIFIER_OF_DEVICE. check if empty deviceID returns INVALID_ARGS.
  * @tc.type: FUNC
  * @tc.require: AR000D08KV
  * @tc.author: maokeheng
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, DeviceIdentifier007, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Set PragmaCmd = GET_IDENTIFIER_OF_DEVICE, deviceID= NULL.
     * @tc.expected: step1. Expect return INVALID_ARGS.
     */
    PragmaDeviceIdentifier param;
    param.deviceID = "";
    PragmaData input = static_cast<void *>(&param);
    EXPECT_EQ(g_kvNbDelegatePtr->Pragma(GET_IDENTIFIER_OF_DEVICE, input), INVALID_ARGS);
}

/**
  * @tc.name: DeviceIdentifier008
  * @tc.desc: Set pragma to be GET_IDENTIFIER_OF_DEVICE. check if deviceIdentifier matches deviceID.
  * @tc.type: FUNC
  * @tc.require: AR000D08KV
  * @tc.author: maokeheng
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, DeviceIdentifier008, TestSize.Level1)
{
    /**
     * @tc.steps:step1. Set PragmaCmd = GET_IDENTIFIER_OF_DEVICE, deviceID = DEVICE_ID_1
     * @tc.expected: step1. Expect return deviceIdentifier is the same as deviceIdentifier of DEVICE_ID_1.
     */
    PragmaDeviceIdentifier param;
    param.deviceID = DEVICE_ID_1;
    PragmaData input = static_cast<void *>(&param);
    EXPECT_EQ(g_kvNbDelegatePtr->Pragma(GET_IDENTIFIER_OF_DEVICE, input), OK);
    EXPECT_EQ(param.deviceIdentifier, DBCommon::TransferHashString(DEVICE_ID_1));
}

/**
  * @tc.name: ErrDbTest001
  * @tc.desc: Initialize check when the database is not opened
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, ErrDbTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Initialize an empty db and register nullptr callback
     * @tc.expected: step1. Expect return not nullptr
     */
    SQLiteSingleVerNaturalStore *errStore = new (std::nothrow) SQLiteSingleVerNaturalStore;
    ASSERT_NE(errStore, nullptr);

    /**
     * @tc.steps: step2. Get db handle
     * @tc.expected: step2. Expect return nullptr
     */
    int errCode;
    EXPECT_EQ(errStore->GetHandle(false, errCode), nullptr);

    /**
     * @tc.steps: step3. Check db integrity
     * @tc.expected: step3. Expect return -E_INVALID_DB
     */
    EXPECT_EQ(errStore->CheckIntegrity(), -E_INVALID_DB);

    /**
     * @tc.steps: step4. Check cache db mode
     * @tc.expected: step4. Expect return false
     */
    EXPECT_FALSE(errStore->IsCacheDBMode());
    EXPECT_FALSE(errStore->IsExtendedCacheDBMode());

    errStore->KillAndDecObjRef(errStore);
}

/**
  * @tc.name: ErrDbTest002
  * @tc.desc: Register check when the database is not opened
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, ErrDbTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Initialize an empty db
     * @tc.expected: step1. Expect return not nullptr
     */
    std::unique_ptr<SQLiteSingleVerNaturalStore> errStore = std::make_unique<SQLiteSingleVerNaturalStore>();
    ASSERT_NE(errStore, nullptr);

    /**
     * @tc.steps: step2. Check register nullptr callback
     * @tc.expected: step2. Expect return E_OK
     */
    EXPECT_EQ(errStore->RegisterLifeCycleCallback(nullptr), E_OK);

    /**
     * @tc.steps: step3. Check register Conflict type
     * @tc.expected: step3. Expect return -E_NOT_SUPPORT
     */
    RegisterFuncType funcType = OBSERVER_SINGLE_VERSION_NS_PUT_EVENT;
    EXPECT_EQ(errStore->TransConflictTypeToRegisterFunctionType(0, funcType), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step4. Check register Observer type
     * @tc.expected: step4. Expect return -E_NOT_SUPPORT
     */
    EXPECT_EQ(errStore->TransObserverTypeToRegisterFunctionType(0, funcType), -E_NOT_SUPPORT);
}

/**
  * @tc.name: ErrDbTest003
  * @tc.desc: Export and Import check when the database is not opened
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, ErrDbTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Initialize an empty db
     * @tc.expected: step1. Expect return not nullptr
     */
    std::unique_ptr<SQLiteSingleVerNaturalStore> errStore = std::make_unique<SQLiteSingleVerNaturalStore>();
    ASSERT_NE(errStore, nullptr);

    /**
     * @tc.steps: step2. Export
     * @tc.expected: step2. Expect return -E_INVALID_DB
     */
    CipherPassword passwd;
    EXPECT_EQ(errStore->Export(g_testDir, passwd), -E_INVALID_DB);

    /**
     * @tc.steps: step3. Import
     * @tc.expected: step3. Expect return -E_INVALID_DB
     */
    EXPECT_EQ(errStore->Import(g_testDir, passwd), -E_INVALID_DB);

    /**
     * @tc.steps: step4. Reset key
     * @tc.expected: step4. Expect return -E_INVALID_DB
     */
    EXPECT_EQ(errStore->Rekey(passwd), -E_INVALID_DB);
}

/**
  * @tc.name: ErrDbTest004
  * @tc.desc: Check the interface of operation data when the database is not opened
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, ErrDbTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Initialize an empty db
     * @tc.expected: step1. Expect return not nullptr
     */
    std::unique_ptr<SQLiteSingleVerNaturalStore> errStore = std::make_unique<SQLiteSingleVerNaturalStore>();
    ASSERT_NE(errStore, nullptr);

    /**
     * @tc.steps: step2. Get all meta keys
     * @tc.expected: step2. Expect return -E_INVALID_DB
     */
    std::vector<Key> keys;
    EXPECT_EQ(errStore->GetAllMetaKeys(keys), -E_INVALID_DB);

    /**
     * @tc.steps: step3. Delete meta data if the Key is empty
     * @tc.expected: step3. Expect return -E_INVALID_ARGS
     */
    Key key;
    keys.push_back(key);
    EXPECT_EQ(errStore->DeleteMetaData(keys), -E_INVALID_ARGS);

    /**
     * @tc.steps: step4. Delete meta data if the Key is not empty
     * @tc.expected: step4. Expect return -E_INVALID_DB
     */
    keys.front().push_back('A');
    EXPECT_EQ(errStore->DeleteMetaData(keys), -E_INVALID_DB);

    /**
     * @tc.steps: step5. put meta data
     * @tc.expected: step5. Expect return -E_INVALID_DB
     */
    Value values;
    EXPECT_EQ(errStore->PutMetaData(keys.front(), values), -E_INVALID_DB);
}

/**
  * @tc.name: ErrDbTest005
  * @tc.desc: Test sync data when the database is not opened
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, ErrDbTest005, TestSize.Level1)
{
    std::unique_ptr<SQLiteSingleVerNaturalStore> errStore = std::make_unique<SQLiteSingleVerNaturalStore>();
    ASSERT_NE(errStore, nullptr);
    ContinueToken token = nullptr;
    std::vector<DataItem> dataItems;
    DataSizeSpecInfo info = {DBConstant::MAX_SYNC_BLOCK_SIZE + 1, 0};
    EXPECT_EQ(errStore->GetSyncDataNext(dataItems, token, info), -E_INVALID_ARGS);
    info.blockSize = 0;
    EXPECT_EQ(errStore->GetSyncDataNext(dataItems, token, info), -E_INVALID_ARGS);
    errStore->ReleaseContinueToken(token);
}

/**
  * @tc.name: StorageEngineTest001
  * @tc.desc: Test cache db remove data
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, StorageEngineTest001, TestSize.Level1)
{
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(CACHEDB);
    EXPECT_EQ(g_store->RemoveDeviceData("device1", false), -1);
    storageEngine->Release();
}

/**
  * @tc.name: StorageEngineTest002
  * @tc.desc: Test the interface of Dump
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, StorageEngineTest002, TestSize.Level1)
{
    std::string exportFileName = g_testDir + "/" + STORE_ID + ".dump";
    OS::FileHandle fd;
    EXPECT_EQ(OS::OpenFile(exportFileName, fd), E_OK);
    g_store->Dump(fd.handle);
    OS::CloseFile(fd);
    OS::RemoveDBDirectory(exportFileName);
}

/**
  * @tc.name: StorageEngineTest003
  * @tc.desc: Test the accuracy of CacheRecordVersion
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, StorageEngineTest003, TestSize.Level1)
{
    uint64_t curVersion = g_store->GetCacheRecordVersion();
    g_store->IncreaseCacheRecordVersion();
    EXPECT_EQ(g_store->GetCacheRecordVersion(), curVersion + 1);
    EXPECT_EQ(g_store->GetAndIncreaseCacheRecordVersion(), curVersion + 1);
    EXPECT_EQ(g_store->GetCacheRecordVersion(), curVersion + 2);

    curVersion = 0;
    std::unique_ptr<SQLiteSingleVerNaturalStore> store2 = std::make_unique<SQLiteSingleVerNaturalStore>();
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2->GetCacheRecordVersion(), curVersion);
    store2->IncreaseCacheRecordVersion();
    EXPECT_EQ(store2->GetCacheRecordVersion(), curVersion);
}

/**
  * @tc.name: StorageEngineTest004
  * @tc.desc: Modify parameter initialization StorageEngine
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, StorageEngineTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get storageEngine
     * @tc.expected: step1. Expect return E_OK
     */
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);

    /**
     * @tc.steps: step2. Set the wrong min write num
     * @tc.expected: step2. Expect -E_INVALID_ARGS
     */
    StorageEngineAttr poolSize = {17, 1, 1, 1};  // 17 means the maximum value is exceeded, 1 is the normal value
    OpenDbProperties option = storageEngine->GetOpenOption();
    EXPECT_EQ(storageEngine->InitSQLiteStorageEngine(poolSize, option), -E_INVALID_ARGS);

    /**
     * @tc.steps: step3. Set the correct min write num
     * @tc.expected: step3. Expect E_OK
     */
    poolSize.minWriteNum = 1;
    EXPECT_EQ(storageEngine->InitSQLiteStorageEngine(poolSize, option), E_OK);

    /**
     * @tc.steps: step4. Change the Engine State
     * @tc.expected: step4. Expect E_OK
     */
    EngineState engineState = {CACHEDB};
    storageEngine->SetEngineState(engineState);
    EXPECT_EQ(storageEngine->InitSQLiteStorageEngine(poolSize, option), E_OK);

    storageEngine->Release();
}

/**
  * @tc.name: StorageEngineTest005
  * @tc.desc: Set adapter to check engine
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, StorageEngineTest005, TestSize.Level1)
{
    std::shared_ptr<ProcessSystemApiAdapterImpl> adapter = std::make_shared<ProcessSystemApiAdapterImpl>();
    EXPECT_TRUE(adapter != nullptr);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(adapter);
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_NE(storageEngine, nullptr);
    EXPECT_EQ(storageEngine->CheckEngineOption(g_property), E_OK);
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
}

/**
  * @tc.name: StorageEngineTest006
  * @tc.desc: Check the engine Option after setting the schema
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, StorageEngineTest006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. First parse the schema, then set the KvDBProperties
     * @tc.expected: step1. Expect return -E_SCHEMA_MISMATCH
     */
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_NE(storageEngine, nullptr);
    KvDBProperties prop;
    prop.SetStringProp(KvDBProperties::DATA_DIR, g_property.GetStringProp(KvDBProperties::DATA_DIR, ""));
    prop.SetStringProp(KvDBProperties::STORE_ID, g_property.GetStringProp(KvDBProperties::STORE_ID, ""));
    prop.SetIntProp(KvDBProperties::DATABASE_TYPE, g_property.GetIntProp(KvDBProperties::DATABASE_TYPE, 0));
    string schemaString =
        "{\"SCHEMA_VERSION\":\"1.0\","
        "\"SCHEMA_MODE\":\"STRICT\","
        "\"SCHEMA_DEFINE\":{"
        "\"field_name1\":\"BOOL\"},"
        "\"SCHEMA_INDEXES\":[\"$.field_name1\"]}";
    SchemaObject schema;
    schema.ParseFromSchemaString(schemaString);
    prop.SetSchema(schema);
    EXPECT_EQ(storageEngine->CheckEngineOption(prop), -E_SCHEMA_MISMATCH);

    /**
     * @tc.steps: step2. Set the error Schema for the option
     * @tc.expected: step2. Expect return -E_SCHEMA_MISMATCH
     */
    OpenDbProperties option = storageEngine->GetOpenOption();
    option.schema = "errorSchema";
    StorageEngineAttr poolSize = {1, 1, 1, 1};  // 1 is the valid size
    EXPECT_EQ(storageEngine->InitSQLiteStorageEngine(poolSize, option), E_OK);
    EXPECT_EQ(storageEngine->CheckEngineOption(prop), -E_SCHEMA_MISMATCH);

    /**
     * @tc.steps: step3. Set the correct Schema for the option
     * @tc.expected: step3. Expect return E_OK
     */
    option.schema = schemaString;
    EXPECT_EQ(storageEngine->InitSQLiteStorageEngine(poolSize, option), E_OK);
    EXPECT_EQ(storageEngine->CheckEngineOption(prop), E_OK);

    storageEngine->Release();
}

/**
  * @tc.name: StorageEngineTest007
  * @tc.desc: Export and import after changing engine state
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, StorageEngineTest007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get storage engine
     * @tc.expected: step1. Expect E_OK
     */
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);

    /**
     * @tc.steps: step2. Change engine state to cache db
     * @tc.expected: step2. Expect -E_NOT_SUPPORT
     */
    storageEngine->SetEngineState(CACHEDB);
    CipherPassword passwd;
    string filePath = g_testDir + "/store_export.db";
    EXPECT_EQ(g_store->Export(filePath, passwd), -E_NOT_SUPPORT);
    EXPECT_EQ(g_store->Import(filePath, passwd), -E_NOT_SUPPORT);
    EXPECT_EQ(g_store->Rekey(passwd), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step3. Change engine state to INVALID
     * @tc.expected: step3. Expect -E_BUSY
     */
    storageEngine->SetEngineState(INVALID);
    EXPECT_EQ(g_store->Export(filePath, passwd), -E_BUSY);
    EXPECT_EQ(g_store->Import(filePath, passwd), -E_BUSY);
    EXPECT_EQ(g_store->Rekey(passwd), -E_BUSY);
    storageEngine->Release();
}

/**
  * @tc.name: StorageEngineTest008
  * @tc.desc: add and remove sub after changing engine state
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBDeviceIdentifierTest, StorageEngineTest008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Get storage engine
     * @tc.expected: step1. Expect E_OK
     */
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);

    /**
     * @tc.steps: step2. Change engine state to cache db
     * @tc.expected: step2. Expect E_OK
     */
    storageEngine->SetEngineState(CACHEDB);
    Query query = Query::Select();
    QueryObject queryObj(query);
    std::string sub = "123";
    EXPECT_EQ(g_store->AddSubscribe(sub, queryObj, true), E_OK);

    /**
     * @tc.steps: step3. Remove subscribe from cache db
     * @tc.expected: step3. Expect -1
     */
    std::vector<std::string> subs;
    subs.push_back(sub);
    EXPECT_EQ(g_store->RemoveSubscribe(subs), -1);
    storageEngine->Release();
}