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
#include <atomic>
#include <cstdint>
#include <gtest/gtest.h>
#include <thread>

#include "db_constant.h"
#include "db_common.h"
#include "distributeddb_storage_single_ver_natural_store_testcase.h"
#include "rd_single_ver_natural_store.h"
#include "rd_single_ver_natural_store_connection.h"
#include "rd_single_ver_storage_executor.h"
#include "kvdb_pragma.h"
#include "storage_engine_manager.h"
#include "grd_api_manager.h"
#include "grd_base/grd_db_api.h"
#include "grd_base/grd_type_export.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    string g_databaseName;
    string g_identifier;
    KvDBProperties g_property;

    RdSingleVerNaturalStore *g_store = nullptr;
    RdSingleVerNaturalStoreConnection *g_connection = nullptr;
    RdSingleVerStorageExecutor *g_handle = nullptr;
    RdSingleVerStorageExecutor *g_nullHandle = nullptr;
}

class DistributedDBStorageRdSingleVerNaturalExecutorTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBStorageRdSingleVerNaturalExecutorTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGI("DistributedDBStorageRdSingleVerNaturalExecutorTest dir is %s", g_testDir.c_str());
    std::string oriIdentifier = APP_ID + "-" + USER_ID + "-" + "TestGeneralNBExecutor";
    std::string identifier = DBCommon::TransferHashString(oriIdentifier);
    g_identifier = DBCommon::TransferStringToHex(identifier);

    g_databaseName = "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::MAINDB_DIR + "/" +
        DBConstant::SINGLE_VER_DATA_STORE + DBConstant::DB_EXTENSION;
    g_property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    g_property.SetStringProp(KvDBProperties::STORE_ID, "TestGeneralNBExecutor");
    g_property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, g_identifier);
    g_property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_RD_KERNAL);
}

void DistributedDBStorageRdSingleVerNaturalExecutorTest::TearDownTestCase(void)
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBStorageRdSingleVerNaturalExecutorTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR);
    g_store = new (std::nothrow) RdSingleVerNaturalStore;
    ASSERT_NE(g_store, nullptr);
    ASSERT_EQ(g_store->Open(g_property), E_OK);

    int erroCode = E_OK;
    g_connection = static_cast<RdSingleVerNaturalStoreConnection *>(g_store->GetDBConnection(erroCode));
    ASSERT_NE(g_connection, nullptr);
    RefObject::DecObjRef(g_store);
    EXPECT_EQ(erroCode, E_OK);

    g_handle = static_cast<RdSingleVerStorageExecutor *>(
        g_store->GetHandle(true, erroCode, OperatePerm::NORMAL_PERM));
    ASSERT_EQ(erroCode, E_OK);
    ASSERT_NE(g_handle, nullptr);

    g_nullHandle = new (nothrow) RdSingleVerStorageExecutor(nullptr, false);
    ASSERT_NE(g_nullHandle, nullptr);
}

void DistributedDBStorageRdSingleVerNaturalExecutorTest::TearDown(void)
{
    if (g_nullHandle != nullptr) {
        delete g_nullHandle;
        g_nullHandle = nullptr;
    }
    if (g_store != nullptr) {
        g_store->ReleaseHandle(g_handle);
    }
    if (g_connection != nullptr) {
        g_connection->Close();
        g_connection = nullptr;
    }
    g_store = nullptr;
    g_handle = nullptr;
}

/**
  * @tc.name: InvalidParam001
  * @tc.desc: Get Kv Data with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, InvalidParam001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The Data type is invalid
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    Timestamp timestamp = 0;
    Key key;
    Value value;
    int type = static_cast<int>(SingleVerDataType::SYNC_TYPE);
    EXPECT_EQ(g_nullHandle->GetKvData(SingleVerDataType(type + 1), key, value, timestamp), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. The key is empty
     * @tc.expected: step2. Expect -E_INVALID_ARGS
     */
    EXPECT_EQ(g_handle->GetKvData(SingleVerDataType(type), key, value, timestamp), -E_INVALID_ARGS);

    /**
     * @tc.steps: step3. The db is null
     * @tc.expected: step3. Expect -E_INVALID_DB
     */
    EXPECT_EQ(g_nullHandle->GetKvData(SingleVerDataType(type), KEY_1, value, timestamp), -E_INVALID_DB);
}

/**
  * @tc.name: InvalidParam002
  * @tc.desc: Put Kv Data check
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, InvalidParam002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. rd unsupport put kv data
     * @tc.expected: step1. Expect -E_NOT_SUPPORT
     */
    Value value;
    EXPECT_EQ(g_nullHandle->PutKvData(SingleVerDataType::SYNC_TYPE, KEY_1, value, 0, nullptr), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. rd unsupport put kv data
     * @tc.expected: step2. Expect -E_NOT_SUPPORT
     */
    EXPECT_EQ(g_nullHandle->PutKvData(SingleVerDataType::META_TYPE, KEY_1, value, 0, nullptr), -E_NOT_SUPPORT);
}

/**
  * @tc.name: InvalidParam005
  * @tc.desc: Test timestamp with Invalid condition (rd not support timestamp)
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, InvalidParam005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The db is null
     * @tc.expected: step1. Expect return 0
     */
    Timestamp timestamp = 0;
    g_nullHandle->InitCurrentMaxStamp(timestamp);
    EXPECT_EQ(timestamp, 0u);

    /**
     * @tc.steps: step2. Get timestamp when The db is null
     * @tc.expected: step2. Expect -E_NOT_SUPPORT
     */
    std::vector<DataItem> dataItems;
    Timestamp begin = 0;
    Timestamp end = INT64_MAX;
    DataSizeSpecInfo info;
    EXPECT_EQ(g_nullHandle->GetSyncDataByTimestamp(dataItems, sizeof("time"), begin, end, info), -E_NOT_SUPPORT);
    EXPECT_EQ(g_nullHandle->GetDeletedSyncDataByTimestamp(dataItems, sizeof("time"), begin, end, info),
        -E_NOT_SUPPORT);
}

/**
  * @tc.name: InvalidParam008
  * @tc.desc: Test transaction with Invalid condition (rd not support transcaction yet)
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, InvalidParam008, TestSize.Level1)
{
    EXPECT_EQ(g_nullHandle->StartTransaction(TransactType::DEFERRED), E_OK);
    EXPECT_EQ(g_nullHandle->Commit(), E_OK);
    EXPECT_EQ(g_nullHandle->Rollback(), E_OK);

    EXPECT_EQ(g_handle->StartTransaction(TransactType::DEFERRED), E_OK);
    EXPECT_EQ(g_handle->Reset(), -E_NOT_SUPPORT);
}

/**
  * @tc.name: InvalidParam009
  * @tc.desc: Get identifier with Invalid condition (rd not support)
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, InvalidParam009, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The parameter is null
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    EXPECT_EQ(g_nullHandle->GetDeviceIdentifier(nullptr), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. The db is null
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    PragmaEntryDeviceIdentifier identifier;
    EXPECT_EQ(g_nullHandle->GetDeviceIdentifier(&identifier), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step3. The identifier is empty
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    EXPECT_EQ(g_handle->GetDeviceIdentifier(&identifier), -E_NOT_SUPPORT);
}

/**
  * @tc.name: InvalidParam010
  * @tc.desc: Fail to call function with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, InvalidParam010, TestSize.Level1)
{
    vector<Key> keys;
    EXPECT_EQ(g_nullHandle->GetAllMetaKeys(keys), -E_NOT_SUPPORT);
    string devName;
    vector<Entry> entries;
    EXPECT_EQ(g_nullHandle->GetAllSyncedEntries(devName, entries), -E_NOT_SUPPORT);
    EXPECT_EQ(g_nullHandle->ForceCheckPoint(), -E_INVALID_DB);
    EXPECT_EQ(g_nullHandle->CheckIntegrity(), -E_NOT_SUPPORT);
}

/**
  * @tc.name: ConnectionTest001
  * @tc.desc: Failed to get the keys (rd not support yet)
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, ConnectionTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the dataType is error
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    IOption option;
    option.dataType = IOption::SYNC_DATA + 1;
    vector<Key> keys;
    EXPECT_EQ(g_connection->GetKeys(option, KEY_1, keys), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. Get keys in cacheDB state
     * @tc.expected: step2. Expect -E_EKEYREVOKED
     */
    int errCode = E_OK;
    RdSingleVerStorageEngine *storageEngine =
        static_cast<RdSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    option.dataType = IOption::LOCAL_DATA;
    EXPECT_EQ(g_connection->GetKeys(option, KEY_1, keys), -E_NOT_SUPPORT);
    storageEngine->Release();

    /**
     * @tc.steps: step3. Get keys in null db connection
     * @tc.expected: step3. Expect -E_NOT_INIT
     */
    std::unique_ptr<RdSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<RdSingleVerNaturalStoreConnection>(nullptr);
    ASSERT_NE(emptyConn, nullptr);
    EXPECT_EQ(emptyConn->GetKeys(option, KEY_1, keys), -E_NOT_SUPPORT);
}

/**
  * @tc.name: ConnectionTest002
  * @tc.desc: Push and delete on empty connect
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, ConnectionTest002, TestSize.Level1)
{
    std::unique_ptr<RdSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<RdSingleVerNaturalStoreConnection>(nullptr);
    IOption option = {IOption::SYNC_DATA};
    std::vector<Entry> entries;
    EXPECT_EQ(emptyConn->PutBatch(option, entries), -E_INVALID_DB);
    std::vector<Key> keys;
    EXPECT_EQ(emptyConn->DeleteBatch(option, keys), -E_INVALID_DB);
    option.dataType = IOption::SYNC_DATA;
    EXPECT_EQ(emptyConn->PutBatch(option, entries), -E_INVALID_DB);
    EXPECT_EQ(emptyConn->DeleteBatch(option, keys), -E_INVALID_DB);
    option.dataType = IOption::SYNC_DATA + 1;
    EXPECT_EQ(emptyConn->PutBatch(option, entries), -E_NOT_SUPPORT);
}

/**
  * @tc.name: PragmaTest001
  * @tc.desc: Calling Pragma incorrectly
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the parameter is null
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    // Rd Pragma only support check point for now
    EXPECT_EQ(g_connection->Pragma(PRAGMA_RESULT_SET_CACHE_MAX_SIZE, nullptr), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_RESULT_SET_CACHE_MODE, nullptr), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_SET_AUTO_LIFE_CYCLE, nullptr), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_UNPUBLISH_SYNC, nullptr), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_PUBLISH_LOCAL, nullptr), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_GET_DEVICE_IDENTIFIER_OF_ENTRY, nullptr), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_SET_MAX_LOG_LIMIT, nullptr), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_GET_IDENTIFIER_OF_DEVICE, nullptr), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. the option is invalid
     * @tc.expected: step2. Expect -E_INVALID_ARGS
     */
    std::unique_ptr<RdSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<RdSingleVerNaturalStoreConnection>(nullptr);
    ASSERT_NE(emptyConn, nullptr);
    SecurityOption option = {S3, SECE};
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_TRIGGER_TO_MIGRATE_DATA, &option), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step3. the size is invalid
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    int size = 0;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_RESULT_SET_CACHE_MAX_SIZE, &size), -E_NOT_SUPPORT);
    size = 1;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_RESULT_SET_CACHE_MAX_SIZE, &size), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step4. the mode is invalid
     * @tc.expected: step4. Expect -E_INVALID_ARGS
     */
    ResultSetCacheMode mode = ResultSetCacheMode(2); // 2 is invalid mode
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_RESULT_SET_CACHE_MODE, &mode), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step5. the db is null
     * @tc.expected: step5. Expect -E_INVALID_DB
     */
    int time = 6000; // 6000 is random
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_SET_AUTO_LIFE_CYCLE, &time), -E_NOT_SUPPORT);
}

/**
  * @tc.name: PragmaTest002
  * @tc.desc: Incorrect publishing and unPublishing
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the db is null
     * @tc.expected: step1. Expect -E_INVALID_DB
     */
    std::unique_ptr<RdSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<RdSingleVerNaturalStoreConnection>(nullptr);
    PragmaPublishInfo info;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_PUBLISH_LOCAL, &info), -E_NOT_SUPPORT);
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_UNPUBLISH_SYNC, &info), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. publish in transaction
     * @tc.expected: step2. Expect -E_NOT_SUPPORT
     */
    g_store->ReleaseHandle(g_handle);
    g_connection->StartTransaction();
    EXPECT_EQ(g_connection->Pragma(PRAGMA_PUBLISH_LOCAL, &info), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_UNPUBLISH_SYNC, &info), -E_NOT_SUPPORT);
    g_connection->RollBack();

    /**
     * @tc.steps: step3. publish in cacheDB
     * @tc.expected: step3. Expect -E_EKEYREVOKED
     */
    int errCode = E_OK;
    RdSingleVerStorageEngine *storageEngine =
        static_cast<RdSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_PUBLISH_LOCAL, &info), -E_NOT_SUPPORT);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_UNPUBLISH_SYNC, &info), -E_NOT_SUPPORT);
    g_connection->StartTransaction();
    g_connection->Commit();
    storageEngine->Release();
}

/**
  * @tc.name: PragmaTest003
  * @tc.desc: Failed to call function with empty connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaTest003, TestSize.Level1)
{
    auto emptyConn = std::make_unique<RdSingleVerNaturalStoreConnection>(nullptr);
    PragmaEntryDeviceIdentifier identifier = {.key = KEY_1};
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_GET_DEVICE_IDENTIFIER_OF_ENTRY, &identifier), -E_NOT_SUPPORT);
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_EXEC_CHECKPOINT, nullptr), -E_NOT_INIT);
    EXPECT_EQ(emptyConn->CheckIntegrity(), -E_NOT_INIT);

    int limit = 0;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_SET_MAX_LOG_LIMIT, &limit), -E_NOT_SUPPORT);
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_RM_DEVICE_DATA, nullptr), -E_NOT_SUPPORT);
    CipherPassword pw;
    EXPECT_EQ(emptyConn->Import("/a.b", pw), -E_INVALID_DB);
    EXPECT_EQ(emptyConn->Export("/a.b", pw), -E_INVALID_DB);
    DatabaseLifeCycleNotifier notifier;
    EXPECT_EQ(emptyConn->RegisterLifeCycleCallback(notifier), -E_NOT_SUPPORT);

    EXPECT_EQ(emptyConn->SetConflictNotifier(0, nullptr), -E_NOT_SUPPORT);
    KvDBConflictAction func = [&](const KvDBCommitNotifyData &data) {};
    EXPECT_EQ(emptyConn->SetConflictNotifier(0, func), -E_NOT_SUPPORT);
    IKvDBSnapshot *shot;
    EXPECT_EQ(emptyConn->GetSnapshot(shot), -E_NOT_SUPPORT);
}

/**
  * @tc.name: ExecutorCache001
  * @tc.desc: Fail to operate data
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, ExecutorCache001, TestSize.Level1)
{
    g_handle->SetAttachMetaMode(true);
    std::set<std::string> devices;
    EXPECT_EQ(g_handle->GetExistsDevicesFromMeta(devices), -E_NOT_SUPPORT);
    EXPECT_EQ(g_handle->DeleteMetaDataByPrefixKey(KEY_1), -E_NOT_SUPPORT);
    std::vector<Key> keys;
    EXPECT_EQ(g_handle->DeleteMetaData(keys), -E_NOT_SUPPORT);
    EXPECT_EQ(g_handle->PrepareForSavingCacheData(SingleVerDataType::LOCAL_TYPE_SQLITE), -E_NOT_SUPPORT);
    std::string hashDev = DBCommon::TransferHashString("device1");
    EXPECT_EQ(g_handle->RemoveDeviceDataInCacheMode(hashDev, true, 0u), -E_NOT_SUPPORT);
    Timestamp timestamp;
    EXPECT_EQ(g_handle->GetMaxTimestampDuringMigrating(timestamp), -E_NOT_SUPPORT);
    EXPECT_EQ(g_handle->ResetForSavingCacheData(SingleVerDataType::LOCAL_TYPE_SQLITE), -E_NOT_SUPPORT);
}

/**
  * @tc.name: ExecutorCache003
  * @tc.desc: Test different condition to attach db
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, ExecutorCache003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Copy empty db, then attach
     */
    string cacheDir = g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR +
        "/" + DBConstant::CACHEDB_DIR + "/" + DBConstant::SINGLE_VER_CACHE_STORE + DBConstant::DB_EXTENSION;
    EXPECT_EQ(DBCommon::CopyFile(g_testDir + g_databaseName, cacheDir), E_OK);
    CipherPassword password;
    EXPECT_EQ(g_nullHandle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::INVALID), -E_NOT_SUPPORT);
    EXPECT_EQ(g_nullHandle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::CACHEDB), -E_NOT_SUPPORT);
    EXPECT_EQ(g_nullHandle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::ATTACHING), -E_NOT_SUPPORT);
    EXPECT_EQ(g_handle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::MAINDB), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. Try migrate data after attaching cache
     * @tc.expected: step2. Expect SQL_STATE_ERR
     */
    NotifyMigrateSyncData syncData;
    DataItem dataItem;
    std::vector<DataItem> items;
    items.push_back(dataItem);
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
}

/**
  * @tc.name: ExecutorCache004
  * @tc.desc: Test migrate after attaching
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, ExecutorCache004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Copy normal db, attach cache
     * @tc.expected: step1. Expect E_OK
     */
    string cacheDir = g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR +
        "/" + DBConstant::CACHEDB_DIR + "/" + DBConstant::SINGLE_VER_CACHE_STORE + DBConstant::DB_EXTENSION;
    EXPECT_EQ(g_handle->ForceCheckPoint(), E_OK);
    EXPECT_EQ(DBCommon::CopyFile(g_testDir + g_databaseName, cacheDir), E_OK);
    CipherPassword password;
    EXPECT_EQ(g_handle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::MAINDB), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step2. Migrate sync data but param incomplete
     */
    NotifyMigrateSyncData syncData;
    DataItem dataItem;
    std::vector<DataItem> items;
    items.push_back(dataItem);
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
    Timestamp timestamp;
    EXPECT_EQ(g_handle->GetMaxTimestampDuringMigrating(timestamp), -E_NOT_SUPPORT);
    items.front().neglect = true;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
    items.front().neglect = false;
    items.front().flag = DataItem::REMOVE_DEVICE_DATA_FLAG;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
    items.front().key = {'r', 'e', 'm', 'o', 'v', 'e'};
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
    items.front().flag = DataItem::REMOVE_DEVICE_DATA_NOTIFY_FLAG;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
    items.front().flag = DataItem::REMOTE_DEVICE_DATA_MISS_QUERY;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
    string selectSync = "SELECT * FROM sync_data";
    Value value;
    value.assign(selectSync.begin(), selectSync.end());
    items.front().value = value;
    items.front().flag = DataItem::REMOVE_DEVICE_DATA_NOTIFY_FLAG;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
    EXPECT_EQ(g_handle->MigrateLocalData(), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step3. Attach maindb
     */
    EXPECT_EQ(g_handle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::CACHEDB), -E_NOT_SUPPORT);
    EXPECT_EQ(g_handle->MigrateLocalData(), -E_NOT_SUPPORT);
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_NOT_SUPPORT);
}

/**
  * @tc.name: MoveToTest001
  * @tc.desc: Test MoveTo and InnerMoveToHead func
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: tiansimiao
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, MoveToTest001, TestSize.Level1)
{
    int position = 1;
    int currPosition = -1;
    GRD_ResultSet *resultSet = nullptr;
    EXPECT_EQ(g_nullHandle->MoveTo(position, resultSet, currPosition), -E_INVALID_ARGS);
    currPosition = 0;
    EXPECT_EQ(g_nullHandle->MoveTo(position, resultSet, currPosition), -E_INVALID_ARGS);
}

/**
  * @tc.name: PragmaGetPageSize_Normal
  * @tc.desc: Call Pragma to query pageSize normally, should return a positive integer
  * @tc.type: FUNC
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaGetPageSize_Normal, TestSize.Level1)
{
    int pageSize = 0;
    int ret = g_connection->Pragma(PRAGMA_GET_PAGE_SIZE, &pageSize);
    EXPECT_EQ(ret, E_OK);
    EXPECT_GT(pageSize, 0);
    // Common pageSize enum values: 4 / 8 / 16 / 32 / 64 (unit: KB, depending on kernel compile config)
    EXPECT_TRUE(pageSize == 4 || pageSize == 8 || pageSize == 16 || pageSize == 32 || pageSize == 64);
}

/**
  * @tc.name: PragmaGetPageSize_NullParam
  * @tc.desc: Pass nullptr as parameter, should return -E_INVALID_ARGS
  * @tc.type: FUNC
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaGetPageSize_NullParam, TestSize.Level1)
{
    int ret = g_connection->Pragma(PRAGMA_GET_PAGE_SIZE, nullptr);
    EXPECT_EQ(ret, -E_INVALID_ARGS);
}

/**
  * @tc.name: PragmaGetPageSize_NullApi
  * @tc.desc: Mock GRD_GetConfig function pointer not loaded (Windows or SO missing), should return -E_INVALID_DATA
  * @tc.type: FUNC
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaGetPageSize_NullApi, TestSize.Level1)
{
    DocumentDB::GRD_APIInfo *apiInfo = DocumentDB::GetApiInfo();
    auto originalGetConfig = apiInfo->GetConfigApi;
    apiInfo->GetConfigApi = nullptr;

    int pageSize = -1; // Sentinel value, verify error path does not write back
    int ret = g_connection->Pragma(PRAGMA_GET_PAGE_SIZE, &pageSize);

    apiInfo->GetConfigApi = originalGetConfig;

    EXPECT_EQ(ret, -E_INVALID_DATA);
    EXPECT_EQ(pageSize, -1);
}

namespace {
    void ConcurrentReadPageSizeWorker(RdSingleVerNaturalStore *store,
        int iterations, std::atomic<int> &successCount, std::atomic<int> &firstPageSize)
    {
        int errCode = E_OK;
        auto *conn = static_cast<RdSingleVerNaturalStoreConnection *>(store->GetDBConnection(errCode));
        ASSERT_NE(conn, nullptr);
        ASSERT_EQ(errCode, E_OK);

        for (int j = 0; j < iterations; ++j) {
            int pageSize = 0;
            int ret = conn->Pragma(PRAGMA_GET_PAGE_SIZE, &pageSize);
            if (ret != E_OK || pageSize <= 0) {
                continue;
            }
            int expected = 0;
            if (firstPageSize.compare_exchange_strong(expected, pageSize)) {
            }
            if (pageSize == firstPageSize.load()) {
                successCount++;
            }
        }

        conn->Close();
    }
}

/**
  * @tc.name: PragmaGetPageSize_ConcurrentRead
  * @tc.desc: Concurrently call getPageSize from multiple threads, all should succeed and return the same value
  * @tc.type: FUNC
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaGetPageSize_ConcurrentRead, TestSize.Level1)
{
    if (g_store != nullptr && g_handle != nullptr) {
        g_store->ReleaseHandle(g_handle);
        g_handle = nullptr;
    }

    const int threadCount = 8;
    const int iterations = 200;
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    std::atomic<int> firstPageSize{0};

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(ConcurrentReadPageSizeWorker,
            g_store, iterations, std::ref(successCount), std::ref(firstPageSize));
    }

    for (auto &t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount.load(), threadCount * iterations);
    EXPECT_GT(firstPageSize.load(), 0);
}

namespace {
    void ConcurrentWriteWorker(RdSingleVerNaturalStore *store,
        int threadIndex, int iterations, std::atomic<int> &writeSuccess)
    {
        int errCode = E_OK;
        auto *conn = static_cast<RdSingleVerNaturalStoreConnection *>(store->GetDBConnection(errCode));
        ASSERT_NE(conn, nullptr);
        ASSERT_EQ(errCode, E_OK);

        IOption option = {IOption::SYNC_DATA};
        for (int j = 0; j < iterations; ++j) {
            Key key = {'k', static_cast<uint8_t>('0' + threadIndex), static_cast<uint8_t>('0' + j)};
            Value val = {'v', static_cast<uint8_t>('0' + threadIndex), static_cast<uint8_t>('0' + j)};
            std::vector<Entry> entries = {{key, val}};
            if (conn->PutBatch(option, entries) == E_OK) {
                writeSuccess++;
            }
        }

        conn->Close();
    }
}

/**
  * @tc.name: PragmaGetPageSize_ConcurrentWithWrite
  * @tc.desc: Concurrently call getPageSize and PutBatch, all operations should succeed without deadlock
  * @tc.type: FUNC
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaGetPageSize_ConcurrentWithWrite, TestSize.Level1)
{
    if (g_store != nullptr && g_handle != nullptr) {
        g_store->ReleaseHandle(g_handle);
        g_handle = nullptr;
    }

    const int readThreadCount = 4;
    const int writeThreadCount = 4;
    const int iterations = 50;
    std::vector<std::thread> threads;
    std::atomic<int> readSuccess{0};
    std::atomic<int> writeSuccess{0};
    std::atomic<int> firstPageSize{0};

    for (int i = 0; i < readThreadCount; ++i) {
        threads.emplace_back(ConcurrentReadPageSizeWorker,
            g_store, iterations, std::ref(readSuccess), std::ref(firstPageSize));
    }

    for (int i = 0; i < writeThreadCount; ++i) {
        threads.emplace_back(ConcurrentWriteWorker,
            g_store, i, iterations, std::ref(writeSuccess));
    }

    for (auto &t : threads) {
        t.join();
    }

    EXPECT_GT(readSuccess.load(), 0);
    EXPECT_GT(writeSuccess.load(), 0);
}

/**
  * @tc.name: OpenWithValidPageSize_CreateDb
  * @tc.desc: Create new DB with each valid pageSize (default/4/8/16/32/64 KB), verify it persists
  * @tc.type: FUNC
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, OpenWithValidPageSize_CreateDb, TestSize.Level1)
{
    TearDown();
    EXPECT_EQ(DistributedDBToolsUnitTest::RemoveTestDbFiles(
        g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR), 0);

    KvDBProperties defaultProp = g_property;
    RdSingleVerNaturalStore *defaultStore = new (std::nothrow) RdSingleVerNaturalStore();
    ASSERT_NE(defaultStore, nullptr);
    ASSERT_EQ(defaultStore->Open(defaultProp), E_OK);

    int errCode = E_OK;
    auto *defaultConn = static_cast<RdSingleVerNaturalStoreConnection *>(defaultStore->GetDBConnection(errCode));
    ASSERT_NE(defaultConn, nullptr);
    RefObject::DecObjRef(defaultStore);

    int defaultPageSize = 0;
    ASSERT_EQ(defaultConn->Pragma(PRAGMA_GET_PAGE_SIZE, &defaultPageSize), E_OK);
    ASSERT_GT(defaultPageSize, 0);
    defaultConn->Close();

    const std::vector<int> validPageSizes = {-1, 4, 8, 16, 32, 64};

    for (size_t idx = 0; idx < validPageSizes.size(); ++idx) {
        KvDBProperties prop = g_property;
        if (validPageSizes[idx] > 0) {
            prop.SetUIntProp(KvDBProperties::KVDB_PAGE_SIZE, static_cast<uint32_t>(validPageSizes[idx]));
        }

        RdSingleVerNaturalStore *store = new (std::nothrow) RdSingleVerNaturalStore();
        ASSERT_NE(store, nullptr);
        EXPECT_EQ(store->Open(prop), E_OK) << "pageSize=" << validPageSizes[idx];

        int errCode = E_OK;
        auto *conn = static_cast<RdSingleVerNaturalStoreConnection *>(store->GetDBConnection(errCode));
        ASSERT_NE(conn, nullptr);
        RefObject::DecObjRef(store);

        int actualPageSize = 0;
        EXPECT_EQ(conn->Pragma(PRAGMA_GET_PAGE_SIZE, &actualPageSize), E_OK) << "pageSize=" << validPageSizes[idx];
        EXPECT_EQ(actualPageSize, defaultPageSize) << "pageSize=" << validPageSizes[idx];

        conn->Close();
    }
}

/**
  * @tc.name: OpenWithInvalidPageSize_CreateDb
  * @tc.desc: Create new DB with invalid pageSize, Open should fail
  * @tc.type: FUNC
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, OpenWithInvalidPageSize_CreateDb, TestSize.Level1)
{
    const std::vector<int> invalidPageSizes = {0, 1, 2, 3, 5, 7, 65, 128};

    for (int badSize : invalidPageSizes) {
        std::string storeId = "TestGeneralNBExecutor_Invalid_" + std::to_string(badSize);
        std::string oriIdentifier = APP_ID + "-" + USER_ID + "-" + storeId;
        std::string identifier = DBCommon::TransferHashString(oriIdentifier);
        std::string hexIdentifier = DBCommon::TransferStringToHex(identifier);
        std::string dbDir = g_testDir + "/" + hexIdentifier + "/" + DBConstant::SINGLE_SUB_DIR;

        EXPECT_EQ(DistributedDBToolsUnitTest::RemoveTestDbFiles(dbDir), 0);

        KvDBProperties initProp = g_property;
        initProp.SetStringProp(KvDBProperties::STORE_ID, storeId);
        initProp.SetStringProp(KvDBProperties::IDENTIFIER_DIR, hexIdentifier);

        RdSingleVerNaturalStore *initStore = new (std::nothrow) RdSingleVerNaturalStore();
        ASSERT_NE(initStore, nullptr);
        EXPECT_EQ(initStore->Open(initProp), E_OK) << "init invalid pageSize=" << badSize;

        initStore->Close();
        RefObject::DecObjRef(initStore);

        KvDBProperties prop = g_property;
        prop.SetStringProp(KvDBProperties::STORE_ID, storeId);
        prop.SetStringProp(KvDBProperties::IDENTIFIER_DIR, hexIdentifier);
        prop.SetUIntProp(KvDBProperties::KVDB_PAGE_SIZE,
            static_cast<uint32_t>(badSize));

        RdSingleVerNaturalStore *store = new (std::nothrow) RdSingleVerNaturalStore();
        ASSERT_NE(store, nullptr);
        EXPECT_NE(store->Open(prop), E_OK) << "invalid pageSize=" << badSize;
        RefObject::DecObjRef(store);

        EXPECT_EQ(DistributedDBToolsUnitTest::RemoveTestDbFiles(dbDir), 0);
    }
}

/**
  * @tc.name: PragmaGetPageSize_InvalidType
  * @tc.desc: Mock GRD_GetConfig returns non-INTEGER type, should return -E_INVALID_DATA and not write back parameter
  * @tc.type: FUNC
  */
HWTEST_F(DistributedDBStorageRdSingleVerNaturalExecutorTest, PragmaGetPageSize_InvalidType, TestSize.Level1)
{
    DocumentDB::GRD_APIInfo *apiInfo = DocumentDB::GetApiInfo();
    auto originalGetConfig = apiInfo->GetConfigApi;

    apiInfo->GetConfigApi = [](GRD_DB *db, GRD_ConfigTypeE type) -> GRD_DbValueT {
        GRD_DbValueT result = {GRD_DB_DATATYPE_NULL, {0}};
        result.type = GRD_DB_DATATYPE_TEXT;
        return result;
    };

    int pageSize = -1; // Sentinel value
    int ret = g_connection->Pragma(PRAGMA_GET_PAGE_SIZE, &pageSize);

    apiInfo->GetConfigApi = originalGetConfig;

    EXPECT_EQ(ret, -E_INVALID_DATA);
    EXPECT_EQ(pageSize, -1);
}