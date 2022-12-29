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
#include <cstdint>
#include <gtest/gtest.h>

#include "db_constant.h"
#include "db_common.h"
#include "distributeddb_storage_single_ver_natural_store_testcase.h"
#include "kvdb_pragma.h"
#include "storage_engine_manager.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    string g_databaseName;
    string g_identifier;
    KvDBProperties g_property;

    SQLiteSingleVerNaturalStore *g_store = nullptr;
    SQLiteSingleVerNaturalStoreConnection *g_connection = nullptr;
    SQLiteSingleVerStorageExecutor *g_handle = nullptr;
    SQLiteSingleVerStorageExecutor *g_nullHandle = nullptr;
}

class DistributedDBStorageSQLiteSingleVerNaturalExecutorTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBStorageSQLiteSingleVerNaturalExecutorTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("DistributedDBStorageSQLiteSingleVerNaturalExecutorTest dir is %s", g_testDir.c_str());
    std::string oriIdentifier = APP_ID + "-" + USER_ID + "-" + "TestGeneralNBExecutor";
    std::string identifier = DBCommon::TransferHashString(oriIdentifier);
    g_identifier = DBCommon::TransferStringToHex(identifier);

    g_databaseName = "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::MAINDB_DIR + "/" +
        DBConstant::SINGLE_VER_DATA_STORE + ".db";
    g_property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    g_property.SetStringProp(KvDBProperties::STORE_ID, "TestGeneralNBExecutor");
    g_property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, g_identifier);
    g_property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE);
}

void DistributedDBStorageSQLiteSingleVerNaturalExecutorTest::TearDownTestCase(void)
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR);
}

void DistributedDBStorageSQLiteSingleVerNaturalExecutorTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR);
    g_store = new (std::nothrow) SQLiteSingleVerNaturalStore;
    ASSERT_NE(g_store, nullptr);
    ASSERT_EQ(g_store->Open(g_property), E_OK);

    int erroCode = E_OK;
    g_connection = static_cast<SQLiteSingleVerNaturalStoreConnection *>(g_store->GetDBConnection(erroCode));
    ASSERT_NE(g_connection, nullptr);
    g_store->DecObjRef(g_store);
    EXPECT_EQ(erroCode, E_OK);

    g_handle = static_cast<SQLiteSingleVerStorageExecutor *>(
        g_store->GetHandle(true, erroCode, OperatePerm::NORMAL_PERM));
    ASSERT_EQ(erroCode, E_OK);
    ASSERT_NE(g_handle, nullptr);

    g_nullHandle = new (nothrow) SQLiteSingleVerStorageExecutor(nullptr, false, false);
    ASSERT_NE(g_nullHandle, nullptr);
}

void DistributedDBStorageSQLiteSingleVerNaturalExecutorTest::TearDown(void)
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
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam001, TestSize.Level1)
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
     * @tc.steps: step2. The db is null
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    EXPECT_EQ(g_nullHandle->GetKvData(SingleVerDataType(type), key, value, timestamp), -E_INVALID_DB);

    /**
     * @tc.steps: step3. The key is empty
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    EXPECT_EQ(g_handle->GetKvData(SingleVerDataType(type), key, value, timestamp), -E_INVALID_ARGS);
}

/**
  * @tc.name: InvalidParam002
  * @tc.desc: Put Kv Data with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The Data type is invalid
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    Value value;
    EXPECT_EQ(g_nullHandle->PutKvData(SingleVerDataType::SYNC_TYPE, KEY_1, value, 0, nullptr), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. The db is null
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    EXPECT_EQ(g_nullHandle->PutKvData(SingleVerDataType::META_TYPE, KEY_1, value, 0, nullptr), -E_INVALID_DB);

    /**
     * @tc.steps: step3. The key is null
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    Key key;
    EXPECT_EQ(g_handle->PutKvData(SingleVerDataType::META_TYPE, key, value, 0, nullptr), -E_INVALID_ARGS);
}

/**
  * @tc.name: InvalidParam003
  * @tc.desc: Get entries with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The Data type is invalid
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    vector<Entry> entries;
    EXPECT_EQ(g_nullHandle->GetEntries(false, SingleVerDataType::META_TYPE, KEY_1, entries), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. The db is null
     * @tc.expected: step2.. Expect -E_INVALID_DB
     */
    EXPECT_EQ(g_nullHandle->GetEntries(false, SingleVerDataType::LOCAL_TYPE, KEY_1, entries), -E_INVALID_DB);

    /**
     * @tc.steps: step3. This key does not exist
     * @tc.expected: step3. Expect -E_NOT_FOUND
     */
    Key key;
    EXPECT_EQ(g_handle->GetEntries(false, SingleVerDataType::LOCAL_TYPE, KEY_1, entries), -E_NOT_FOUND);
}

/**
  * @tc.name: InvalidParam004
  * @tc.desc: Get count with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The db is null
     * @tc.expected: step1. Expect -E_INVALID_DB
     */
    Query query = Query::Select().OrderBy("abc", false);
    QueryObject object(query);
    int count;
    EXPECT_EQ(g_nullHandle->GetCount(object, count), -E_INVALID_DB);

    /**
     * @tc.steps: step2. The query condition is invalid
     * @tc.expected: step2. Expect -E_NOT_SUPPORT
     */
    EXPECT_EQ(g_handle->GetCount(object, count), -E_NOT_SUPPORT);
}

/**
  * @tc.name: InvalidParam005
  * @tc.desc: Test timestamp with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam005, TestSize.Level1)
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
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    std::vector<DataItem> dataItems;
    Timestamp begin = 0;
    Timestamp end = INT64_MAX;
    DataSizeSpecInfo info;
    EXPECT_EQ(g_nullHandle->GetSyncDataByTimestamp(dataItems, sizeof("time"), begin, end, info), -E_INVALID_DB);
    EXPECT_EQ(g_nullHandle->GetDeletedSyncDataByTimestamp(dataItems, sizeof("time"), begin, end, info), -E_INVALID_DB);
}

/**
  * @tc.name: InvalidParam006
  * @tc.desc: Open and get resultSet with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The db is null
     * @tc.expected: step1. Expect -E_INVALID_DB
     */
    int count;
    EXPECT_EQ(g_nullHandle->OpenResultSet(KEY_1, count), -E_INVALID_DB);
    vector<int64_t> cache;
    Key key;
    EXPECT_EQ(g_nullHandle->OpenResultSetForCacheRowIdMode(KEY_1, cache, 0, count), -E_INVALID_DB);
    Query query = Query::Select();
    QueryObject object(query);
    EXPECT_EQ(g_nullHandle->OpenResultSetForCacheRowIdMode(object, cache, 0, count), -E_INVALID_DB);

    /**
     * @tc.steps: step2. Then get
     * @tc.expected: step2. Expect -E_RESULT_SET_STATUS_INVALID
     */
    Value value;
    EXPECT_EQ(g_nullHandle->GetNextEntryFromResultSet(key, value, false), -E_RESULT_SET_STATUS_INVALID);

    /**
     * @tc.steps: step3. The db is valid,open
     * @tc.expected: step3. Expect E_OK
     */
    EXPECT_EQ(g_handle->OpenResultSetForCacheRowIdMode(object, cache, 0, count), E_OK);

    /**
     * @tc.steps: step4. Then get
     * @tc.expected: step4. Expect -E_RESULT_SET_STATUS_INVALID
     */
    Entry entry;
    EXPECT_EQ(g_handle->GetEntryByRowId(0, entry), -E_UNEXPECTED_DATA);
}

/**
  * @tc.name: InvalidParam007
  * @tc.desc: Reload resultSet with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Reload,but the db is null
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    Key key;
    EXPECT_EQ(g_nullHandle->ReloadResultSet(key), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. Reload,but the key is empty
     * @tc.expected: step2. Expect -E_INVALID_ARGS
     */
    vector<int64_t> cache;
    EXPECT_EQ(g_handle->ReloadResultSet(key), -E_INVALID_ARGS);
    EXPECT_EQ(g_nullHandle->ReloadResultSetForCacheRowIdMode(key, cache, 0, 0), -E_INVALID_ARGS);

    /**
     * @tc.steps: step3. Reload by object,but the db is null
     * @tc.expected: step3. Expect -E_INVALID_QUERY_FORMAT
     */
    Query query = Query::Select();
    QueryObject object(query);
    EXPECT_EQ(g_nullHandle->ReloadResultSet(object), -E_INVALID_QUERY_FORMAT);
    EXPECT_EQ(g_nullHandle->ReloadResultSetForCacheRowIdMode(object, cache, 0, 0), -E_INVALID_QUERY_FORMAT);

    /**
     * @tc.steps: step4. Reload with the valid db
     * @tc.expected: step4. Expect E_OK
     */
    EXPECT_EQ(g_handle->ReloadResultSetForCacheRowIdMode(object, cache, 0, 0), E_OK);
}

/**
  * @tc.name: InvalidParam008
  * @tc.desc: Test transaction with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam008, TestSize.Level1)
{
    EXPECT_EQ(g_nullHandle->StartTransaction(TransactType::DEFERRED), -E_INVALID_DB);
    EXPECT_EQ(g_nullHandle->Commit(), -E_INVALID_DB);
    EXPECT_EQ(g_nullHandle->Rollback(), -E_INVALID_DB);

    EXPECT_EQ(g_handle->StartTransaction(TransactType::DEFERRED), E_OK);
    EXPECT_EQ(g_handle->Reset(), E_OK);
}

/**
  * @tc.name: InvalidParam009
  * @tc.desc: Get identifier with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam009, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The parameter is null
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    EXPECT_EQ(g_nullHandle->GetDeviceIdentifier(nullptr), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. The db is null
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    PragmaEntryDeviceIdentifier identifier;
    EXPECT_EQ(g_nullHandle->GetDeviceIdentifier(&identifier), -E_INVALID_DB);

    /**
     * @tc.steps: step3. The identifier is empty
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    EXPECT_EQ(g_handle->GetDeviceIdentifier(&identifier), -E_INVALID_ARGS);
}

/**
  * @tc.name: InvalidParam010
  * @tc.desc: Get meta key with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam010, TestSize.Level1)
{
    vector<Key> keys;
    EXPECT_EQ(g_nullHandle->GetAllMetaKeys(keys), -E_INVALID_DB);

    string devName;
    vector<Entry> entries;
    EXPECT_EQ(g_nullHandle->GetAllSyncedEntries(devName, entries), -E_INVALID_DB);
}

/**
  * @tc.name: InvalidParam011
  * @tc.desc:
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidParam011, TestSize.Level1)
{
    /**
     * @tc.steps: step1. put local kv data
     * @tc.expected: step1. Expect E_OK
     */
    Key key = KEY_1;
    Value value;
    Timestamp timestamp = 0;
    EXPECT_EQ(g_handle->PutKvData(SingleVerDataType::LOCAL_TYPE, key, value, timestamp, nullptr), E_OK);

    /**
     * @tc.steps: step2. Get sqlite3 handle,then create executor for state CACHE_ATTACH_MAIN
     * @tc.expected: step2. Expect not null
     */
    sqlite3 *sqlHandle = nullptr;
    std::string dbPath = g_testDir + g_databaseName;
    OpenDbProperties property = {dbPath, false, false};
    EXPECT_EQ(SQLiteUtils::OpenDatabase(property, sqlHandle), E_OK);
    EXPECT_NE(sqlHandle, nullptr);
    auto executor = new (std::nothrow) SQLiteSingleVerStorageExecutor(
        sqlHandle, false, false, ExecutorState::CACHE_ATTACH_MAIN);
    EXPECT_NE(executor, nullptr);

    /**
     * @tc.steps: step3. The singleVerNaturalStoreCommitNotifyData is null,delete
     * @tc.expected: step3. Expect -1
     */
    EXPECT_EQ(executor->DeleteLocalKvData(key, nullptr, value, timestamp), -1); // -1 is covert from sqlite error code
    sqlite3_close_v2(sqlHandle);
    delete executor;
    sqlHandle = nullptr;
}

/**
  * @tc.name: InvalidSync001
  * @tc.desc: Save sync data with Invalid condition
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, InvalidSync001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. The saveSyncStatements_ is not prepare
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    DataItem item;
    DeviceInfo info;
    Timestamp time = 0;
    EXPECT_EQ(g_handle->SaveSyncDataItem(item, info, time, nullptr, false), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. Try to prepare when the db is null
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    EXPECT_EQ(g_nullHandle->PrepareForSavingData(SingleVerDataType::LOCAL_TYPE), -E_INVALID_DB);

    /**
     * @tc.steps: step3. The data item key is empty
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    EXPECT_EQ(g_handle->PrepareForSavingData(SingleVerDataType::SYNC_TYPE), E_OK);
    EXPECT_EQ(g_handle->SaveSyncDataItem(item, info, time, nullptr, false), -E_INVALID_ARGS);

    /**
     * @tc.steps: step4. The committedData is null
     * @tc.expected: step4. Expect return E_OK
     */
    item.key = KEY_1;
    EXPECT_EQ(g_handle->SaveSyncDataItem(item, info, time, nullptr, false), E_OK);

    /**
     * @tc.steps: step5. Into EraseSyncData
     * @tc.expected: step5. Expect return E_OK
     */
    SingleVerNaturalStoreCommitNotifyData data;
    item.writeTimestamp = 1;
    item.flag = DataItem::REMOTE_DEVICE_DATA_MISS_QUERY;
    EXPECT_EQ(g_handle->SaveSyncDataItem(item, info, time, &data, true), E_OK);
}

/**
  * @tc.name: ConnectionTest001
  * @tc.desc: Failed to get the keys
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ConnectionTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the dataType is error
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    IOption option;
    option.dataType = IOption::SYNC_DATA + 1;
    vector<Key> keys;
    EXPECT_EQ(g_connection->GetKeys(option, KEY_1, keys), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. Get keys in cacheDB state
     * @tc.expected: step2. Expect -E_EKEYREVOKED
     */
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    option.dataType = IOption::LOCAL_DATA;
    EXPECT_EQ(g_connection->GetKeys(option, KEY_1, keys), -E_EKEYREVOKED);
    storageEngine->Release();

    /**
     * @tc.steps: step3. Get keys in null db connection
     * @tc.expected: step3. Expect -E_NOT_INIT
     */
    std::unique_ptr<SQLiteSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<SQLiteSingleVerNaturalStoreConnection>(nullptr);
    ASSERT_NE(emptyConn, nullptr);
    EXPECT_EQ(emptyConn->GetKeys(option, KEY_1, keys), -E_NOT_INIT);
}

/**
  * @tc.name: ConnectionTest002
  * @tc.desc: Push and delete on empty connect
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ConnectionTest002, TestSize.Level1)
{
    std::unique_ptr<SQLiteSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<SQLiteSingleVerNaturalStoreConnection>(nullptr);
    IOption option = {IOption::LOCAL_DATA};
    std::vector<Entry> entries;
    EXPECT_EQ(emptyConn->PutBatch(option, entries), -E_INVALID_DB);
    std::vector<Key> keys;
    EXPECT_EQ(emptyConn->DeleteBatch(option, keys), -E_INVALID_DB);
    option.dataType = IOption::SYNC_DATA;
    EXPECT_EQ(emptyConn->PutBatch(option, entries), -E_INVALID_DB);
    EXPECT_EQ(emptyConn->DeleteBatch(option, keys), -E_INVALID_DB);
}

/**
  * @tc.name: ConnectionTest003
  * @tc.desc: Failed to Put and Delete
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ConnectionTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Only change the storageEngine to cacheDB
     * @tc.expected: step1. Expect -1
     */
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    IOption option = {IOption::SYNC_DATA};
    std::vector<Entry> entries;
    entries.push_back(ENTRY_1);
    g_store->ReleaseHandle(g_handle);
    EXPECT_EQ(g_connection->PutBatch(option, entries), -1); // -1 is sqlite error
    std::vector<Key> keys;
    keys.push_back(KEY_1);
    EXPECT_EQ(g_connection->DeleteBatch(option, keys), -1);

    /**
     * @tc.steps: step2.Change to LOCAL_DATA option
     * @tc.expected: step2. Expect -1
     */
    option.dataType = IOption::LOCAL_DATA;
    EXPECT_EQ(g_connection->PutBatch(option, entries), -1);
    EXPECT_EQ(g_connection->DeleteBatch(option, keys), -1);

    /**
     * @tc.steps: step3. Table sync_data adds a column to make the num of cols equal to the cacheDB
     * @tc.expected: step3. Expect E_OK
     */
    sqlite3 *db;
    ASSERT_TRUE(sqlite3_open_v2((g_testDir + g_databaseName).c_str(),
        &db, SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK);
    string addSync = "alter table sync_data add column version INT";
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, addSync) == E_OK);
    sqlite3_close_v2(db);
    option.dataType = IOption::SYNC_DATA;
    EXPECT_EQ(g_connection->PutBatch(option, entries), E_OK);
    storageEngine->Release();
}

/**
  * @tc.name: ConnectionTest004
  * @tc.desc: Failed to GetResultSet
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ConnectionTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the db is null
     * @tc.expected: step1. Expect -E_INVALID_DB
     */
    g_store->ReleaseHandle(g_handle);
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    IKvDBResultSet *set = nullptr;
    Query query = Query::Select();
    std::unique_ptr<SQLiteSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<SQLiteSingleVerNaturalStoreConnection>(nullptr);
    EXPECT_EQ(emptyConn->GetResultSet(option, KEY_1, set), -E_INVALID_DB);
    emptyConn->ReleaseResultSet(set);

    /**
     * @tc.steps: step2. get in transaction
     * @tc.expected: step2. Expect -E_BUSY
     */
    g_connection->StartTransaction();
    EXPECT_EQ(g_connection->GetResultSet(option, query, set), -E_BUSY);
    g_connection->RollBack();

    /**
     * @tc.steps: step3. change the storageEngine to cacheDB
     * @tc.expected: step3. Expect -E_EKEYREVOKED
     */
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    EXPECT_EQ(g_connection->GetResultSet(option, query, set), -E_EKEYREVOKED);
    EXPECT_EQ(g_connection->GetResultSet(option, KEY_1, set), -E_EKEYREVOKED);
    storageEngine->Release();
}

/**
  * @tc.name: PragmaTest001
  * @tc.desc: Calling Pragma incorrectly
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, PragmaTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the parameter is null
     * @tc.expected: step1. Expect -E_INVALID_ARGS
     */
    EXPECT_EQ(g_connection->Pragma(PRAGMA_RESULT_SET_CACHE_MAX_SIZE, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_RESULT_SET_CACHE_MODE, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_SET_AUTO_LIFE_CYCLE, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_UNPUBLISH_SYNC, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_PUBLISH_LOCAL, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_GET_DEVICE_IDENTIFIER_OF_ENTRY, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_SET_MAX_LOG_LIMIT, nullptr), -E_INVALID_ARGS);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_GET_IDENTIFIER_OF_DEVICE, nullptr), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. the option is invalid
     * @tc.expected: step2. Expect -E_INVALID_ARGS
     */
    std::unique_ptr<SQLiteSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<SQLiteSingleVerNaturalStoreConnection>(nullptr);
    ASSERT_NE(emptyConn, nullptr);
    SecurityOption option = {S3, SECE};
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_TRIGGER_TO_MIGRATE_DATA, &option), -E_INVALID_CONNECTION);

    /**
     * @tc.steps: step3. the size is invalid
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    int size = 0;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_RESULT_SET_CACHE_MAX_SIZE, &size), -E_INVALID_ARGS);
    size = 1;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_RESULT_SET_CACHE_MAX_SIZE, &size), E_OK);

    /**
     * @tc.steps: step4. the mode is invalid
     * @tc.expected: step4. Expect -E_INVALID_ARGS
     */
    ResultSetCacheMode mode = ResultSetCacheMode(2); // 2 is invalid mode
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_RESULT_SET_CACHE_MODE, &mode), -E_INVALID_ARGS);

    /**
     * @tc.steps: step5. the db is null
     * @tc.expected: step5. Expect -E_INVALID_DB
     */
    int time = 6000; // 6000 is random
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_SET_AUTO_LIFE_CYCLE, &time), -E_INVALID_DB);
}

/**
  * @tc.name: PragmaTest002
  * @tc.desc: Incorrect publishing and unPublishing
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, PragmaTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the db is null
     * @tc.expected: step1. Expect -E_INVALID_DB
     */
    std::unique_ptr<SQLiteSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<SQLiteSingleVerNaturalStoreConnection>(nullptr);
    PragmaPublishInfo info;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_PUBLISH_LOCAL, &info), -E_INVALID_DB);
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_UNPUBLISH_SYNC, &info), -E_INVALID_DB);

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
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_PUBLISH_LOCAL, &info), -E_EKEYREVOKED);
    EXPECT_EQ(g_connection->Pragma(PRAGMA_UNPUBLISH_SYNC, &info), -E_EKEYREVOKED);
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
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, PragmaTest003, TestSize.Level1)
{
    std::unique_ptr<SQLiteSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<SQLiteSingleVerNaturalStoreConnection>(nullptr);
    PragmaEntryDeviceIdentifier identifier = {.key = KEY_1};
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_GET_DEVICE_IDENTIFIER_OF_ENTRY, &identifier), -E_NOT_INIT);
    EXPECT_EQ(emptyConn->CheckIntegrity(), -E_NOT_INIT);

    int limit = 0;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_SET_MAX_LOG_LIMIT, &limit), -E_INVALID_DB);
    CipherPassword pw;
    EXPECT_EQ(emptyConn->Import("/a.b", pw), -E_INVALID_DB);
    DatabaseLifeCycleNotifier notifier;
    EXPECT_EQ(emptyConn->RegisterLifeCycleCallback(notifier), -E_INVALID_DB);
}