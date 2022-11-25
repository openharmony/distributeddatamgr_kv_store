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

#include <gtest/gtest.h>

#include "distributeddb_data_generate_unit_test.h"
#include "kvdb_manager.h"
#include "native_sqlite.h"
#include "sqlite_local_kvdb.h"
#include "sqlite_local_kvdb_connection.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    SQLiteLocalKvDBConnection *g_connection = nullptr;
    SQLiteLocalKvDB *g_invalidDb = nullptr;
    SQLiteLocalKvDBConnection *g_invalidConnection = nullptr;

    void CreateConnInInvalidDb()
    {
        ASSERT_EQ(g_invalidDb, nullptr);
        g_invalidConnection = new (std::nothrow) SQLiteLocalKvDBConnection(g_invalidDb);
        ASSERT_NE(g_invalidConnection, nullptr);
    }

    void CloseConnInInvalidDb()
    {
        if (g_invalidConnection != nullptr) {
            delete g_invalidConnection;
            g_invalidConnection = nullptr;
        }
    }

    void CreateConnInNewDb()
    {
        g_invalidDb = new (std::nothrow) SQLiteLocalKvDB();
        ASSERT_NE(g_invalidDb, nullptr);
        int errCode;
        g_invalidConnection = static_cast<SQLiteLocalKvDBConnection *>(g_invalidDb->GetDBConnection(errCode));
        ASSERT_NE(g_invalidConnection, nullptr);
        RefObject::DecObjRef(g_invalidDb);
    }

    void CloseConnInNewDb()
    {
        if (g_invalidConnection != nullptr) {
            EXPECT_EQ(g_invalidConnection->Close(), E_OK);
            g_invalidConnection = nullptr;
            g_invalidDb = nullptr;
        }
        if (g_invalidDb != nullptr) {
            delete g_invalidDb;
            g_invalidDb = nullptr;
        }
    }

    void TestFunc(const KvDBCommitNotifyData &data)
    {
    }
}

class DistributedDBStorageDataConnectionTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBStorageDataConnectionTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
}

void DistributedDBStorageDataConnectionTest::TearDownTestCase(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBStorageDataConnectionTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    KvDBProperties properties;
    properties.SetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, true);
    properties.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::LOCAL_TYPE);
    properties.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    properties.SetStringProp(KvDBProperties::STORE_ID, "test");
    properties.SetStringProp(KvDBProperties::IDENTIFIER_DIR, "test");

    int errCode = E_OK;
    g_connection = static_cast<SQLiteLocalKvDBConnection *>(KvDBManager::GetDatabaseConnection(properties, errCode));
    EXPECT_EQ(errCode, E_OK);
    ASSERT_NE(g_connection, nullptr);
}

void DistributedDBStorageDataConnectionTest::TearDown(void)
{
    if (g_connection != nullptr) {
        g_connection->Close();
        g_connection = nullptr;
    }
    CloseConnInInvalidDb();
    CloseConnInNewDb();
    return;
}

/**
  * @tc.name: ConnectionTest001
  * @tc.desc: Get value from abnormal connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageDataConnectionTest, ConnectionTest001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a connection when the db is null
     * @tc.expected: step1. Expect not null
     */
    CreateConnInInvalidDb();

    /**
     * @tc.steps: step2 Get value
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    IOption option;
    Value value;
    EXPECT_EQ(g_invalidConnection->Get(option, KEY_3, value), -E_INVALID_DB);
    CloseConnInInvalidDb();

    /**
     * @tc.steps: step3. Use db to create a new connection, but db is not open
     * @tc.expected: step3. Expect not null
     */
    CreateConnInNewDb();

    /**
     * @tc.steps: step4. Get value
     * @tc.expected: step4. Expect -E_INVALID_DB
     */
    EXPECT_EQ(g_invalidConnection->Get(option, KEY_3, value), -E_INVALID_DB);
    CloseConnInNewDb();
}

/**
  * @tc.name: ConnectionTest002
  * @tc.desc: Delete and clear from abnormal connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageDataConnectionTest, ConnectionTest002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Use db to create a new connection, but db is not open
     * @tc.expected: step1. Expect not null
     */
    CreateConnInNewDb();

    /**
     * @tc.steps: step2. Clear and delete
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    IOption option;
    Value value;
    EXPECT_EQ(g_invalidConnection->Delete(option, KEY_3), -E_INVALID_DB);
    EXPECT_EQ(g_invalidConnection->Clear(option), -E_INVALID_DB);
    CloseConnInNewDb();
}

/**
  * @tc.name: ConnectionTest003
  * @tc.desc: Get entries from abnormal connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageDataConnectionTest, ConnectionTest003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a connection when the db is null
     * @tc.expected: step1. Expect not null
     */
    CreateConnInInvalidDb();

    /**
     * @tc.steps: step2. Get entries
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    IOption option;
    std::vector<Entry> entry;
    Key key(DBConstant::MAX_KEY_SIZE + 1, 'w');
    EXPECT_EQ(g_invalidConnection->GetEntries(option, key, entry), -E_INVALID_DB);
    CloseConnInInvalidDb();

    /**
     * @tc.steps: step3. Key is over size
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    CreateConnInNewDb();
    EXPECT_EQ(g_invalidConnection->GetEntries(option, key, entry), -E_INVALID_ARGS);

    /**
     * @tc.steps: step4. Key is normal, but the db not open
     * @tc.expected: step4. Expect -E_INVALID_DB
     */
    EXPECT_EQ(g_invalidConnection->GetEntries(option, KEY_3, entry), -E_INVALID_DB);
    CloseConnInNewDb();
}

/**
  * @tc.name: ConnectionTest004
  * @tc.desc: Put batch from abnormal connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageDataConnectionTest, ConnectionTest004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a connection when the db is null
     * @tc.expected: step1. Expect not null
     */
    CreateConnInInvalidDb();

    /**
     * @tc.steps: step2. Put empty entries
     * @tc.expected: step2. Expect -E_INVALID_ARGS
     */
    IOption option;
    std::vector<Entry> entry;
    EXPECT_EQ(g_invalidConnection->PutBatch(option, entry), -E_INVALID_ARGS);

    /**
     * @tc.steps: step3. Entries is normal, but the db is null
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    Value value;
    Entry ent = {KEY_3, value};
    entry.push_back(ent);
    EXPECT_EQ(g_invalidConnection->PutBatch(option, entry), -E_INVALID_ARGS);
    CloseConnInInvalidDb();

    /**
     * @tc.steps: step4. The db is not open
     * @tc.expected: step4. Expect -E_INVALID_DB
     */
    CreateConnInNewDb();
    EXPECT_EQ(g_invalidConnection->PutBatch(option, entry), -E_INVALID_DB);
    CloseConnInNewDb();
}

/**
  * @tc.name: ConnectionTest005
  * @tc.desc: Delete batch from abnormal connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageDataConnectionTest, ConnectionTest005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a connection when the db is null
     * @tc.expected: step1. Expect not null
     */
    CreateConnInInvalidDb();

    /**
     * @tc.steps: step2. delete, but the keys is empty
     * @tc.expected: step2. Expect -E_INVALID_ARGS
     */
    IOption option;
    std::vector<Key> keys;
    EXPECT_EQ(g_invalidConnection->DeleteBatch(option, keys), -E_INVALID_ARGS);

    /**
     * @tc.steps: step3. The key is over size
     * @tc.expected: step3. Expect -E_INVALID_ARGS
     */
    Key key(DBConstant::MAX_KEY_SIZE + 1, 'w');
    keys.push_back(key);
    EXPECT_EQ(g_invalidConnection->DeleteBatch(option, keys), -E_INVALID_ARGS);

    /**
     * @tc.steps: step4. key is normal , but the db is null
     * @tc.expected: step4. Expect -E_INVALID_DB
     */
    keys.clear();
    keys.push_back(KEY_3);
    EXPECT_EQ(g_invalidConnection->DeleteBatch(option, keys), -E_INVALID_DB);
    CloseConnInInvalidDb();
}

/**
  * @tc.name: ConnectionTest006
  * @tc.desc: Get snapshot from abnormal connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageDataConnectionTest, ConnectionTest006, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Create a connection when the db is null
     * @tc.expected: step1. Expect not null
     */
    CreateConnInInvalidDb();

    /**
     * @tc.steps: step2. the db is null
     * @tc.expected: step2. Expect -E_INVALID_DB
     */
    IKvDBSnapshot *snapshot = nullptr;
    EXPECT_EQ(g_invalidConnection->GetSnapshot(snapshot), -E_INVALID_DB);
    CloseConnInInvalidDb();

    /**
     * @tc.steps: step3. GetSnapshot after changing db operate perm to DISABLE_PERM
     * @tc.expected: step3. Expect -E_STALE
     */
    SQLiteLocalKvDB *localKvdb = new (std::nothrow) SQLiteLocalKvDB();
    ASSERT_NE(localKvdb, nullptr);
    EXPECT_EQ(localKvdb->TryToDisableConnection(OperatePerm::DISABLE_PERM), E_OK);
    int errCode;
    SQLiteLocalKvDBConnection *connection =
        static_cast<SQLiteLocalKvDBConnection *>(localKvdb->NewConnection(errCode));
    ASSERT_NE(connection, nullptr);
    EXPECT_EQ(connection->GetSnapshot(snapshot), -E_STALE);
    EXPECT_EQ(connection->Close(), E_OK);
    connection = nullptr;
    localKvdb = nullptr;

    /**
     * @tc.steps: step4. Get snapshot from normal db, then preClose
     * @tc.expected: step4. Expect -E_BUSY
     */
    EXPECT_EQ(g_connection->GetSnapshot(snapshot), E_OK);
    EXPECT_EQ(g_connection->PreClose(), -E_BUSY);
    g_connection->ReleaseSnapshot(snapshot);
}

/**
  * @tc.name: ConnectionTest007
  * @tc.desc: Test transaction from abnormal connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageDataConnectionTest, ConnectionTest007, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Start transaction when the db is null
     * @tc.expected: step1. Expect -E_INVALID_DB
     */
    CreateConnInInvalidDb();
    EXPECT_EQ(g_invalidConnection->StartTransaction(), -E_INVALID_DB);
    CloseConnInInvalidDb();

    /**
     * @tc.steps: step2. Start transaction and rollback
     * @tc.expected: step2. Expect true after StartTransaction, false after rollback
     */
    g_connection->StartTransaction();
    EXPECT_EQ(g_connection->IsTransactionStarted(), true);
    g_connection->RollBack();
    EXPECT_EQ(g_connection->IsTransactionStarted(), false);
}

/**
  * @tc.name: ConnectionTest008
  * @tc.desc: Export,import and rekey from abnormal connection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageDataConnectionTest, ConnectionTest008, TestSize.Level1)
{
    /**
     * @tc.steps: step1. the db is null
     * @tc.expected: step1. Expect -E_INVALID_DB
     */
    CreateConnInInvalidDb();
    CipherPassword passwd;
    string filePath = g_testDir + "/ExportTest.db";
    EXPECT_EQ(g_invalidConnection->Rekey(passwd), -E_INVALID_DB);
    EXPECT_EQ(g_invalidConnection->Export(filePath, passwd), -E_INVALID_DB);
    EXPECT_EQ(g_invalidConnection->Import(filePath, passwd), -E_INVALID_DB);
    CloseConnInInvalidDb();

    /**
     * @tc.steps: step2. Make OperatePerm not equal to NORMAL_PERM
     * @tc.expected: step2. Expect -E_BUSY
     */
    SQLiteLocalKvDB *localKvdb = new (std::nothrow) SQLiteLocalKvDB();
    ASSERT_NE(localKvdb, nullptr);
    EXPECT_EQ(localKvdb->TryToDisableConnection(OperatePerm::DISABLE_PERM), E_OK);
    int errCode;
    SQLiteLocalKvDBConnection *connection =
        static_cast<SQLiteLocalKvDBConnection *>(localKvdb->NewConnection(errCode));
    ASSERT_NE(connection, nullptr);
    EXPECT_EQ(connection->Import(filePath, passwd), -E_BUSY);
    EXPECT_EQ(connection->Close(), E_OK);
    connection = nullptr;
    localKvdb = nullptr;

    /**
     * @tc.steps: step3. Test in transaction
     * @tc.expected: step3. Expect -E_BUSY
     */
    g_connection->StartTransaction();
    EXPECT_EQ(g_connection->Rekey(passwd), -E_BUSY);
    EXPECT_EQ(g_connection->Import(filePath, passwd), -E_BUSY);
    g_connection->RollBack();

    /**
     * @tc.steps: step4. Test after Registering observer
     * @tc.expected: step4. Expect -E_BUSY
     */
    int result;
    Key key;
    key.push_back('a');
    KvDBObserverHandle* handle = g_connection->RegisterObserver(
        static_cast<unsigned int>(SQLITE_GENERAL_NS_LOCAL_PUT_EVENT), key, TestFunc, result);
    EXPECT_EQ(result, E_OK);
    EXPECT_NE(handle, nullptr);
    EXPECT_EQ(g_connection->Rekey(passwd), -E_BUSY);
    EXPECT_EQ(g_connection->Import(filePath, passwd), -E_BUSY);
    g_connection->UnRegisterObserver(handle);
}