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
#include "sqlite_single_ver_storage_executor_sql.h"

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

    const char * const ADD_SYNC = "ALTER TABLE sync_data ADD column version INT";
    const char * const DROP_CREATE = "ALTER TABLE sync_data DROP column create_time";
    const char * const DROP_MODIFY = "ALTER TABLE sync_data DROP column modify_time";
    const char * const ADD_LOCAL = "ALTER TABLE local_data ADD column flag INT";
    const char * const INSERT_SQL = "INSERT INTO sync_data VALUES('a', 'b', 1, 2, '', '', 'efdef', 100 , 1);";
    const int SQL_STATE_ERR = -1;
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
    LOGI("DistributedDBStorageSQLiteSingleVerNaturalExecutorTest dir is %s", g_testDir.c_str());
    std::string oriIdentifier = APP_ID + "-" + USER_ID + "-" + "TestGeneralNBExecutor";
    std::string identifier = DBCommon::TransferHashString(oriIdentifier);
    g_identifier = DBCommon::TransferStringToHex(identifier);

    g_databaseName = "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::MAINDB_DIR + "/" +
        DBConstant::SINGLE_VER_DATA_STORE + DBConstant::DB_EXTENSION;
    g_property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    g_property.SetStringProp(KvDBProperties::STORE_ID, "TestGeneralNBExecutor");
    g_property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, g_identifier);
    g_property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);
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
    EXPECT_EQ(g_nullHandle->GetEntries(false, SingleVerDataType::LOCAL_TYPE_SQLITE, KEY_1, entries), -E_INVALID_DB);

    /**
     * @tc.steps: step3. This key does not exist
     * @tc.expected: step3. Expect -E_NOT_FOUND
     */
    Key key;
    EXPECT_EQ(g_handle->GetEntries(false, SingleVerDataType::LOCAL_TYPE_SQLITE, KEY_1, entries), -E_NOT_FOUND);
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
    EXPECT_EQ(g_nullHandle->OpenResultSet(object, count), -E_INVALID_DB);
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
  * @tc.desc: Fail to call function with Invalid condition
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
    EXPECT_EQ(g_nullHandle->ForceCheckPoint(), -E_INVALID_DB);
    EXPECT_EQ(g_nullHandle->CheckIntegrity(), -E_INVALID_DB);
}

/**
  * @tc.name: InvalidParam011
  * @tc.desc: Change executor state to operate data
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
    EXPECT_EQ(g_handle->PutKvData(SingleVerDataType::LOCAL_TYPE_SQLITE, key, value, timestamp, nullptr), E_OK);

    /**
     * @tc.steps: step2. Get sqlite3 handle,then create executor for state CACHE_ATTACH_MAIN
     * @tc.expected: step2. Expect not null
     */
    sqlite3 *sqlHandle = nullptr;
    std::string dbPath = g_testDir + g_databaseName;
    OpenDbProperties property = {dbPath, false, false};
    EXPECT_EQ(SQLiteUtils::OpenDatabase(property, sqlHandle), E_OK);
    ASSERT_NE(sqlHandle, nullptr);

    auto executor = std::make_unique<SQLiteSingleVerStorageExecutor>(
        sqlHandle, false, false, ExecutorState::CACHE_ATTACH_MAIN);
    ASSERT_NE(executor, nullptr);

    /**
     * @tc.steps: step3. The singleVerNaturalStoreCommitNotifyData is null,delete
     * @tc.expected: step3. Expect SQL_STATE_ERR
     */
    EXPECT_EQ(executor->DeleteLocalKvData(key, nullptr, value, timestamp), SQL_STATE_ERR);

    /**
     * @tc.steps: step4. Update sync_data table and insert a sync data
     * @tc.expected: step4. Expect E_OK
     */
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(sqlHandle, DROP_MODIFY) == E_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(sqlHandle, DROP_CREATE) == E_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(sqlHandle, ADD_SYNC) == E_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(sqlHandle, INSERT_SQL) == E_OK);
    std::vector<DataItem> vec;
    uint64_t version = 0u;
    EXPECT_EQ(executor->GetMinVersionCacheData(vec, version), E_OK);
    EXPECT_EQ(executor->GetMaxVersionInCacheDb(version), E_OK);
    std::string hashDev = DBCommon::TransferHashString("device1");
    EXPECT_EQ(executor->RemoveDeviceDataInCacheMode(hashDev, true, 0u), E_OK);
    sqlite3_close_v2(sqlHandle);
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
    EXPECT_EQ(g_nullHandle->PrepareForSavingData(SingleVerDataType::LOCAL_TYPE_SQLITE), -E_INVALID_DB);

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
    option.dataType = IOption::SYNC_DATA + 1;
    EXPECT_EQ(emptyConn->PutBatch(option, entries), -E_NOT_SUPPORT);
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
     * @tc.expected: step1. Expect SQL_STATE_ERR
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
    EXPECT_EQ(g_connection->PutBatch(option, entries), SQL_STATE_ERR);
    std::vector<Key> keys;
    keys.push_back(KEY_1);
    EXPECT_EQ(g_connection->DeleteBatch(option, keys), SQL_STATE_ERR);

    /**
     * @tc.steps: step2.Change to LOCAL_DATA option
     * @tc.expected: step2. Expect SQL_STATE_ERR
     */
    option.dataType = IOption::LOCAL_DATA;
    EXPECT_EQ(g_connection->PutBatch(option, entries), SQL_STATE_ERR);
    EXPECT_EQ(g_connection->DeleteBatch(option, keys), SQL_STATE_ERR);

    /**
     * @tc.steps: step3. Table sync_data adds a column to make the num of cols equal to the cacheDB
     * @tc.expected: step3. Expect E_OK
     */
    sqlite3 *db;
    ASSERT_TRUE(sqlite3_open_v2((g_testDir + g_databaseName).c_str(),
        &db, SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, DROP_MODIFY) == E_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, DROP_CREATE) == E_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, ADD_SYNC) == E_OK);
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
  * @tc.name: ConnectionTest005
  * @tc.desc: Failed to Get entries,value and count
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ConnectionTest005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the db is null
     * @tc.expected: step1. Expect -E_INVALID_DB, Get return -E_NOT_INIT
     */
    g_store->ReleaseHandle(g_handle);
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    Query query = Query::Select();
    std::unique_ptr<SQLiteSingleVerNaturalStoreConnection> emptyConn =
        std::make_unique<SQLiteSingleVerNaturalStoreConnection>(nullptr);
    ASSERT_NE(emptyConn, nullptr);
    int count;
    EXPECT_EQ(emptyConn->GetCount(option, query, count), -E_INVALID_DB);
    std::vector<Entry> entries;
    EXPECT_EQ(emptyConn->GetEntries(option, query, entries), -E_INVALID_DB);
    Value value;
    EXPECT_EQ(emptyConn->Get(option, KEY_1, value), -E_NOT_INIT);

    /**
     * @tc.steps: step2. the dataType is not SYNC_DATA
     * @tc.expected: step2. Expect -E_NOT_SUPPORT
     */
    option.dataType = IOption::SYNC_DATA + 1;
    EXPECT_EQ(emptyConn->GetCount(option, query, count), -E_NOT_SUPPORT);
    EXPECT_EQ(emptyConn->GetEntries(option, query, entries), -E_NOT_SUPPORT);
    EXPECT_EQ(emptyConn->Get(option, KEY_1, value), -E_NOT_SUPPORT);

    /**
     * @tc.steps: step3. get in transaction
     * @tc.expected: step3. Expect GetEntries -E_NOT_FOUND, GetCount E_OK
     */
    option.dataType = IOption::SYNC_DATA;
    g_connection->StartTransaction();
    EXPECT_EQ(g_connection->GetEntries(option, query, entries), -E_NOT_FOUND);
    EXPECT_EQ(g_connection->GetCount(option, query, count), E_OK);
    g_connection->RollBack();

    /**
     * @tc.steps: step4. change the storageEngine to cacheDB
     * @tc.expected: step4. Expect -E_EKEYREVOKED
     */
    int errCode = E_OK;
    SQLiteSingleVerStorageEngine *storageEngine =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    EXPECT_EQ(g_connection->GetCount(option, query, count), -E_EKEYREVOKED);
    EXPECT_EQ(g_connection->GetEntries(option, query, entries), -E_EKEYREVOKED);
    EXPECT_EQ(g_connection->Get(option, KEY_1, value), -E_EKEYREVOKED);
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
    auto emptyConn = std::make_unique<SQLiteSingleVerNaturalStoreConnection>(nullptr);
    PragmaEntryDeviceIdentifier identifier = {.key = KEY_1};
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_GET_DEVICE_IDENTIFIER_OF_ENTRY, &identifier), -E_NOT_INIT);
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_EXEC_CHECKPOINT, nullptr), -E_NOT_INIT);
    EXPECT_EQ(emptyConn->CheckIntegrity(), -E_NOT_INIT);

    int limit = 0;
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_SET_MAX_LOG_LIMIT, &limit), -E_INVALID_DB);
    EXPECT_EQ(emptyConn->Pragma(PRAGMA_RM_DEVICE_DATA, nullptr), -E_INVALID_DB);
    CipherPassword pw;
    EXPECT_EQ(emptyConn->Import("/a.b", pw), -E_INVALID_DB);
    EXPECT_EQ(emptyConn->Export("/a.b", pw), -E_INVALID_DB);
    DatabaseLifeCycleNotifier notifier;
    EXPECT_EQ(emptyConn->RegisterLifeCycleCallback(notifier), -E_INVALID_DB);

    EXPECT_EQ(emptyConn->SetConflictNotifier(0, nullptr), -E_INVALID_ARGS);
    KvDBConflictAction func = [&](const KvDBCommitNotifyData &data) {};
    EXPECT_EQ(emptyConn->SetConflictNotifier(0, func), -E_INVALID_DB);
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
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ExecutorCache001, TestSize.Level1)
{
    g_handle->SetAttachMetaMode(true);
    std::set<std::string> devices;
    EXPECT_EQ(g_handle->GetExistsDevicesFromMeta(devices), SQL_STATE_ERR);
    EXPECT_EQ(g_handle->DeleteMetaDataByPrefixKey(KEY_1), SQL_STATE_ERR);
    std::vector<Key> keys;
    EXPECT_EQ(g_handle->DeleteMetaData(keys), SQL_STATE_ERR);
    EXPECT_EQ(g_handle->PrepareForSavingCacheData(SingleVerDataType::LOCAL_TYPE_SQLITE), SQL_STATE_ERR);
    std::string hashDev = DBCommon::TransferHashString("device1");
    EXPECT_EQ(g_handle->RemoveDeviceDataInCacheMode(hashDev, true, 0u), SQL_STATE_ERR);
    Timestamp timestamp;
    EXPECT_EQ(g_handle->GetMaxTimestampDuringMigrating(timestamp), -E_NOT_INIT);
    EXPECT_EQ(g_handle->ResetForSavingCacheData(SingleVerDataType::LOCAL_TYPE_SQLITE), E_OK);
}

/**
  * @tc.name: ExecutorCache002
  * @tc.desc: Fail to call func in each ExecutorState
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ExecutorCache002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. In MAINDB
     * @tc.expected: step1. Expect not E_OK
     */
    NotifyMigrateSyncData syncData;
    DataItem dataItem;
    std::vector<DataItem> items;
    items.push_back(dataItem);
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_INVALID_DB);
    uint64_t version;
    EXPECT_EQ(g_handle->GetMinVersionCacheData(items, version), -E_INVALID_ARGS);
    EXPECT_EQ(g_handle->GetMaxVersionInCacheDb(version), -E_INVALID_ARGS);
    EXPECT_EQ(g_handle->MigrateLocalData(), -E_INVALID_ARGS);

    /**
     * @tc.steps: step2. Change executor to CACHE_ATTACH_MAIN
     * @tc.expected: step2. Expect SQL_STATE_ERR
     */
    sqlite3 *sqlHandle = nullptr;
    EXPECT_EQ(SQLiteUtils::OpenDatabase({g_testDir + g_databaseName, false, false}, sqlHandle), E_OK);
    ASSERT_NE(sqlHandle, nullptr);
    auto executor = std::make_unique<SQLiteSingleVerStorageExecutor>(
        sqlHandle, false, false, ExecutorState::CACHE_ATTACH_MAIN);
    ASSERT_NE(executor, nullptr);
    EXPECT_EQ(executor->MigrateSyncDataByVersion(0u, syncData, items), SQL_STATE_ERR);
    EXPECT_EQ(executor->GetMinVersionCacheData(items, version), SQL_STATE_ERR);
    EXPECT_EQ(executor->GetMaxVersionInCacheDb(version), SQL_STATE_ERR);
    EXPECT_EQ(executor->MigrateLocalData(), SQL_STATE_ERR);
    executor->SetAttachMetaMode(true);
    EXPECT_EQ(executor->MigrateSyncDataByVersion(0u, syncData, items), SQL_STATE_ERR);

    /**
     * @tc.steps: step3. Change executor to MAIN_ATTACH_CACHE
     */
    auto executor2 = std::make_unique<SQLiteSingleVerStorageExecutor>(
        sqlHandle, false, false, ExecutorState::MAIN_ATTACH_CACHE);
    ASSERT_NE(executor2, nullptr);
    items.clear();
    EXPECT_EQ(executor2->MigrateSyncDataByVersion(0u, syncData, items), -E_INVALID_ARGS);
    items.push_back(dataItem);
    EXPECT_EQ(executor2->MigrateSyncDataByVersion(0u, syncData, items), SQL_STATE_ERR);
    EXPECT_EQ(executor2->GetMinVersionCacheData(items, version), SQL_STATE_ERR);
    EXPECT_EQ(executor2->GetMaxVersionInCacheDb(version), SQL_STATE_ERR);
    EXPECT_EQ(executor2->MigrateLocalData(), SQL_STATE_ERR);
    sqlite3_close_v2(sqlHandle);
    sqlHandle = nullptr;
}

/**
  * @tc.name: ExecutorCache003
  * @tc.desc: Test different condition to attach db
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ExecutorCache003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Copy empty db, then attach
     */
    string cacheDir = g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR +
        "/" + DBConstant::CACHEDB_DIR + "/" + DBConstant::SINGLE_VER_CACHE_STORE + DBConstant::DB_EXTENSION;
    EXPECT_EQ(DBCommon::CopyFile(g_testDir + g_databaseName, cacheDir), E_OK);
    CipherPassword password;
    EXPECT_EQ(g_nullHandle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::INVALID), -E_INVALID_ARGS);
    EXPECT_EQ(g_nullHandle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::CACHEDB), -E_INVALID_DB);
    EXPECT_EQ(g_nullHandle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::ATTACHING), E_OK);
    EXPECT_EQ(g_handle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::MAINDB), E_OK);

    /**
     * @tc.steps: step2. Try migrate data after attaching cache
     * @tc.expected: step2. Expect SQL_STATE_ERR
     */
    NotifyMigrateSyncData syncData;
    DataItem dataItem;
    std::vector<DataItem> items;
    items.push_back(dataItem);
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), SQL_STATE_ERR);
}

/**
  * @tc.name: ExecutorCache004
  * @tc.desc: Test migrate after attaching
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ExecutorCache004, TestSize.Level1)
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
        CipherType::DEFAULT, password, cacheDir, EngineState::MAINDB), E_OK);

    /**
     * @tc.steps: step2. Migrate sync data but param incomplete
     */
    NotifyMigrateSyncData syncData;
    DataItem dataItem;
    std::vector<DataItem> items;
    items.push_back(dataItem);
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_INVALID_ARGS);
    Timestamp timestamp;
    EXPECT_EQ(g_handle->GetMaxTimestampDuringMigrating(timestamp), E_OK);
    items.front().neglect = true;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), SQL_STATE_ERR);
    items.front().neglect = false;
    items.front().flag = DataItem::REMOVE_DEVICE_DATA_FLAG;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_INVALID_ARGS);
    items.front().key = {'r', 'e', 'm', 'o', 'v', 'e'};
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), SQL_STATE_ERR);
    items.front().flag = DataItem::REMOVE_DEVICE_DATA_NOTIFY_FLAG;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), SQL_STATE_ERR);
    items.front().flag = DataItem::REMOTE_DEVICE_DATA_MISS_QUERY;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_INVALID_DB);
    string selectSync = "SELECT * FROM sync_data";
    Value value;
    value.assign(selectSync.begin(), selectSync.end());
    items.front().value = value;
    items.front().flag = DataItem::REMOVE_DEVICE_DATA_NOTIFY_FLAG;
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), SQL_STATE_ERR);
    EXPECT_EQ(g_handle->MigrateLocalData(), E_OK);

    /**
     * @tc.steps: step3. Attach maindb
     */
    EXPECT_EQ(g_handle->AttachMainDbAndCacheDb(
        CipherType::DEFAULT, password, cacheDir, EngineState::CACHEDB), E_OK);
    EXPECT_EQ(g_handle->MigrateLocalData(), E_OK);
    EXPECT_EQ(g_handle->MigrateSyncDataByVersion(0u, syncData, items), -E_BUSY);

    sqlite3 *db = nullptr;
    ASSERT_EQ(g_handle->GetDbHandle(db), E_OK);
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(SQLiteUtils::GetStatement(db, MIGRATE_UPDATE_DATA_TO_MAINDB_FROM_CACHEHANDLE, stmt), E_OK);
    int errCode = E_OK;
    EXPECT_EQ(SQLiteUtils::BindBlobToStatement(stmt, BIND_SYNC_UPDATE_HASH_KEY_INDEX, {}), E_OK);
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    EXPECT_EQ(errCode, E_OK);
}

/**
  * @tc.name: ExecutorCache005
  * @tc.desc: Alter table then save data in cache mode
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, ExecutorCache005, TestSize.Level1)
{
    g_store->ReleaseHandle(g_handle);
    sqlite3 *db = nullptr;
    ASSERT_TRUE(sqlite3_open_v2((g_testDir + g_databaseName).c_str(),
        &db, SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, ADD_LOCAL) == E_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, ADD_SYNC) == E_OK);
    auto executor = std::make_unique<SQLiteSingleVerStorageExecutor>(db, false, false);
    ASSERT_NE(executor, nullptr);
    LocalDataItem item;
    EXPECT_EQ(executor->PutLocalDataToCacheDB(item), -E_INVALID_ARGS);
    item.hashKey = KEY_1;
    EXPECT_EQ(executor->PutLocalDataToCacheDB(item), -E_INVALID_ARGS);
    item.flag = DataItem::DELETE_FLAG;
    EXPECT_EQ(executor->PutLocalDataToCacheDB(item), E_OK);
    Query query = Query::Select();
    QueryObject object(query);
    DataItem dataItem;
    dataItem.flag = DataItem::REMOTE_DEVICE_DATA_MISS_QUERY;
    DeviceInfo info;
    Timestamp maxTime;
    EXPECT_EQ(executor->SaveSyncDataItemInCacheMode(dataItem, info, maxTime, 0, object), -E_INVALID_ARGS);
    dataItem.key = KEY_1;
    EXPECT_EQ(executor->SaveSyncDataItemInCacheMode(dataItem, info, maxTime, 0, object), E_OK);
    dataItem.flag = DataItem::DELETE_FLAG;
    EXPECT_EQ(executor->SaveSyncDataItemInCacheMode(dataItem, info, maxTime, 0, object), E_OK);
    sqlite3_close_v2(db);
}