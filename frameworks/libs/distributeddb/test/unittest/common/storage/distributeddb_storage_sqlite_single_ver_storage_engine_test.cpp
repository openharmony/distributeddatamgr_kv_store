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

#include <gtest/gtest.h>

#include "db_common.h"
#include "distributeddb_storage_single_ver_natural_store_testcase.h"
#include "storage_engine_manager.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    string g_databaseName;
    string g_identifier;
    string g_cacheDir;
    KvDBProperties g_property;

    SQLiteSingleVerNaturalStore *g_store = nullptr;
    SQLiteSingleVerNaturalStoreConnection *g_connection = nullptr;

    const char * const ADD_SYNC = "ALTER TABLE sync_data ADD column version INT";
    const char * const INSERT_SQL = "INSERT INTO sync_data VALUES('a', 'b', 1, 2, '', '', 'efdef', 100 , 1, 0, 0);";
    const int SQL_STATE_ERR = -1;

    void CopyCacheDb()
    {
        EXPECT_EQ(DBCommon::CopyFile(g_testDir + g_databaseName, g_cacheDir), E_OK);
        EXPECT_EQ(DBCommon::CopyFile(g_testDir + g_databaseName + "-wal", g_cacheDir + "-wal"), E_OK);
    }

    void GetStorageEngine(SQLiteSingleVerStorageEngine *&storageEngine)
    {
        int errCode;
        storageEngine =
            static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
        EXPECT_EQ(errCode, E_OK);
    }
}

class DistributedDBStorageSQLiteSingleVerStorageEngineTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBStorageSQLiteSingleVerStorageEngineTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGI("DistributedDBStorageSQLiteSingleVerStorageEngineTest dir is %s", g_testDir.c_str());
    std::string oriIdentifier = APP_ID + "-" + USER_ID + "-" + "TestGeneralNBStorageEngine";
    std::string identifier = DBCommon::TransferHashString(oriIdentifier);
    g_identifier = DBCommon::TransferStringToHex(identifier);

    g_databaseName = "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::MAINDB_DIR + "/" +
        DBConstant::SINGLE_VER_DATA_STORE + DBConstant::DB_EXTENSION;
    g_property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    g_property.SetStringProp(KvDBProperties::STORE_ID, "TestGeneralNBStorageEngine");
    g_property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, g_identifier);
    g_property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);
    g_cacheDir = g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR +
        "/" + DBConstant::CACHEDB_DIR + "/" + DBConstant::SINGLE_VER_CACHE_STORE + DBConstant::DB_EXTENSION;
}

void DistributedDBStorageSQLiteSingleVerStorageEngineTest::TearDownTestCase(void)
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR);
}

void DistributedDBStorageSQLiteSingleVerStorageEngineTest::SetUp(void)
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
}

void DistributedDBStorageSQLiteSingleVerStorageEngineTest::TearDown(void)
{
    if (g_connection != nullptr) {
        g_connection->Close();
        g_connection = nullptr;
    }
    g_store = nullptr;
}

/**
  * @tc.name: DataTest001
  * @tc.desc: Change engine state, execute migrate
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, DataTest001, TestSize.Level1)
{
    SQLiteSingleVerStorageEngine *storageEngine = nullptr;
    GetStorageEngine(storageEngine);
    ASSERT_NE(storageEngine, nullptr);
    CopyCacheDb();

    storageEngine->SetEngineState(EngineState::ENGINE_BUSY);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), -E_BUSY);
    storageEngine->SetEngineState(EngineState::ATTACHING);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), -E_NOT_SUPPORT);
    storageEngine->SetEngineState(EngineState::MAINDB);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), SQL_STATE_ERR);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), SQL_STATE_ERR);
    storageEngine->Release();
    storageEngine = nullptr;
}

/**
  * @tc.name: DataTest002
  * @tc.desc: Alter table, Change engine state, execute migrate
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: bty
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, DataTest002, TestSize.Level1)
{
    sqlite3 *db;
    ASSERT_TRUE(sqlite3_open_v2((g_testDir + g_databaseName).c_str(),
        &db, SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, ADD_SYNC) == E_OK);
    ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, INSERT_SQL) == E_OK);
    sqlite3_close_v2(db);
    CopyCacheDb();
    SQLiteSingleVerStorageEngine *storageEngine = nullptr;
    GetStorageEngine(storageEngine);
    ASSERT_NE(storageEngine, nullptr);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), -E_BUSY);
    storageEngine->Release();
    storageEngine = nullptr;
}