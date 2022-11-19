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
  * @tc.name: Destructor001
  * @tc.desc: Test the destructor of g_handle when the transaction is opened
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerNaturalExecutorTest, Destructor001, TestSize.Level1)
{
    g_handle->StartTransaction(TransactType::DEFERRED);
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