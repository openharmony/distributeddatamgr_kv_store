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
#include "process_system_api_adapter_impl.h"
#include "single_ver_utils.h"
#include "storage_engine_manager.h"
#include "virtual_sqlite_storage_engine.h"

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

    void PrepareEnv()
    {
        sqlite3 *db;
        ASSERT_TRUE(sqlite3_open_v2((g_testDir + g_databaseName).c_str(),
            &db, SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK);
        ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, ADD_SYNC) == E_OK);
        ASSERT_TRUE(SQLiteUtils::ExecuteRawSQL(db, INSERT_SQL) == E_OK);
        sqlite3_close_v2(db);
    }

    OpenDbProperties GetProperties(bool isMem, const SecurityOption &option, bool createIfNecessary)
    {
        OpenDbProperties properties;
        properties.uri = DistributedDB::GetDatabasePath(g_property);
        properties.createIfNecessary = createIfNecessary;
        properties.subdir = DistributedDB::GetSubDirPath(g_property);
        properties.isMemDb = isMem;
        properties.securityOpt = option;
        return properties;
    }

    int InitVirtualEngine(const std::shared_ptr<DistributedDB::VirtualSingleVerStorageEngine> &engine,
        bool isMem, const SecurityOption &option, bool createIfNecessary, const StorageEngineAttr &poolSize)
    {
        auto properties = GetProperties(isMem, option, createIfNecessary);
        return engine->InitSQLiteStorageEngine(poolSize, properties, "");
    }

    std::pair<int, std::shared_ptr<DistributedDB::VirtualSingleVerStorageEngine>> GetVirtualEngineWithSecurity(
        uint32_t maxRead, bool isMem, const SecurityOption &option, bool createIfNecessary)
    {
        std::pair<int, std::shared_ptr<DistributedDB::VirtualSingleVerStorageEngine>> res;
        auto &[errCode, engine] = res;
        engine = std::make_shared<DistributedDB::VirtualSingleVerStorageEngine>();
        StorageEngineAttr poolSize = {1, 1, 1, maxRead}; // at most 1 write.
        errCode = InitVirtualEngine(engine, isMem, option, createIfNecessary, poolSize);
        return res;
    }

    std::pair<int, std::shared_ptr<DistributedDB::VirtualSingleVerStorageEngine>> GetVirtualEngine(uint32_t maxRead = 1,
        bool isMem = false, bool createIfNecessary = false)
    {
        return GetVirtualEngineWithSecurity(maxRead, isMem, {}, createIfNecessary);
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
    RefObject::DecObjRef(g_store);
    EXPECT_EQ(erroCode, E_OK);
}

void DistributedDBStorageSQLiteSingleVerStorageEngineTest::TearDown(void)
{
    if (g_connection != nullptr) {
        g_connection->Close();
        g_connection = nullptr;
    }
    g_store = nullptr;
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(nullptr);
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

/**
  * @tc.name: DataTest003
  * @tc.desc: Test invalid SQLiteSingleVerNaturalStore and invalid SQLiteSingleVerNaturalStoreConnection
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caihaoting
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, DataTest003, TestSize.Level0)
{
    /**
     * @tc.steps::step1. init invalid SQLiteSingleVerNaturalStore and invalid SQLiteSingleVerNaturalStoreConnection
     * @tc.expected: step1. return OK.
     */
    SQLiteSingleVerNaturalStore *invalidStore = nullptr;
    SQLiteSingleVerNaturalStoreConnection *invalidConnection = nullptr;
    ASSERT_EQ(invalidStore, nullptr);
    invalidConnection = new (std::nothrow) SQLiteSingleVerNaturalStoreConnection(invalidStore);
    ASSERT_NE(invalidConnection, nullptr);
    /**
     * @tc.steps:step2. test RegisterObserver with invalid SQLiteSingleVerNaturalStore
     * @tc.expected: step2. return -E_INVALID_CONNECTION.
     */
    int errCode = E_OK;
    Key key;
    key.push_back('a');
    KvDBObserverAction func = [&](const KvDBCommitNotifyData &data) {};
    invalidConnection->RegisterObserver(
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_LOCAL_PUT_EVENT), key, func,
        errCode);
    EXPECT_EQ(errCode, -E_INVALID_CONNECTION);
    /**
     * @tc.steps:step3. test UnRegisterObserver with invalid SQLiteSingleVerNaturalStore
     * @tc.expected: step3. return -E_INVALID_CONNECTION.
     */
    auto observerHandle = g_connection->RegisterObserver(
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_LOCAL_PUT_EVENT), key, func,
        errCode);
    EXPECT_EQ(errCode, E_OK);
    errCode = invalidConnection->UnRegisterObserver(observerHandle);
    EXPECT_EQ(errCode, -E_INVALID_CONNECTION);
    /**
     * @tc.steps:step4. test GetSecurityOption with invalid SQLiteSingleVerNaturalStore
     * @tc.expected: step4. return -E_INVALID_CONNECTION.
     */
    int securityLabel = NOT_SET;
    int securityFlag = ECE;
    errCode = invalidConnection->GetSecurityOption(securityLabel, securityFlag);
    EXPECT_EQ(errCode, -E_INVALID_CONNECTION);
    /**
     * @tc.steps:step5. test Close with invalid SQLiteSingleVerNaturalStore
     * @tc.expected: step5. return -E_INVALID_CONNECTION.
     */
    errCode = invalidConnection->Close();
    EXPECT_EQ(errCode, -E_INVALID_CONNECTION);
    /**
     * @tc.steps:step6. delete invalid SQLiteSingleVerNaturalStoreConnection
     * @tc.expected: step6. return OK.
     */
    delete invalidConnection;
    invalidConnection = nullptr;
    ASSERT_EQ(invalidConnection, nullptr);
}

/**
  * @tc.name: ExecutorTest001
  * @tc.desc: Test find executor after disable engine
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest001, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    SQLiteSingleVerStorageEngine *storageEngine = nullptr;
    GetStorageEngine(storageEngine);
    ASSERT_NE(storageEngine, nullptr);
    EXPECT_EQ(storageEngine->TryToDisable(false), E_OK);
    int errCode = E_OK;
    EXPECT_EQ(storageEngine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode), nullptr);
    storageEngine->Release();
    storageEngine = nullptr;
}

/**
  * @tc.name: ExecutorTest002
  * @tc.desc: Test find executor success
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest002, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    SQLiteSingleVerStorageEngine *storageEngine = nullptr;
    GetStorageEngine(storageEngine);
    int errCode = E_OK;
    auto executor1 = storageEngine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode);
    EXPECT_NE(executor1, nullptr);
    auto executor2 = storageEngine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode);
    EXPECT_NE(executor2, nullptr);
    storageEngine->Recycle(executor1);
    storageEngine->Recycle(executor2);
    EXPECT_EQ(storageEngine->FindExecutor(false, OperatePerm::DISABLE_PERM, errCode), nullptr);
    storageEngine->Release();
    storageEngine = nullptr;
}

/**
  * @tc.name: ExecutorTest003
  * @tc.desc: Test find executor abnormal
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest003, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    auto [errCode, engine] = GetVirtualEngine();
    ASSERT_EQ(errCode, E_OK);
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 0);
    EXPECT_NE(executor1, nullptr);
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 0);
    EXPECT_EQ(executor2, nullptr);
    engine->Recycle(executor1);
    engine->Release();
}

/**
  * @tc.name: ExecutorTest004
  * @tc.desc: Test find executor abnormal
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest004, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    auto [errCode, engine] = GetVirtualEngine(3); // max read is 3
    ASSERT_EQ(errCode, E_OK);
    auto mockFunc = [](bool, StorageExecutor *&handle) {
        handle = nullptr;
        return -E_EKEYREVOKED;
    };
    auto executor0 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 0);
    EXPECT_NE(executor0, nullptr);
    /**
     * @tc.steps:step1. create new handle with mock func
     * @tc.expected: step1. create failed.
     */
    engine->ForkNewExecutorMethod(mockFunc);
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 0);
    EXPECT_EQ(executor1, nullptr);
    EXPECT_EQ(errCode, -E_BUSY);
    /**
     * @tc.steps:step2. create new handle with normal func
     * @tc.expected: step2. create ok.
     */
    engine->ForkNewExecutorMethod(nullptr);
    executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 0);
    EXPECT_NE(executor1, nullptr);
    /**
     * @tc.steps:step3. create new handle with mock func
     * @tc.expected: step3. create failed.
     */
    engine->ForkNewExecutorMethod([](bool, StorageExecutor *&handle) {
        handle = nullptr;
        return -E_BUSY;
    });
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 0);
    EXPECT_EQ(executor2, nullptr);
    engine->Recycle(executor1);
    engine->Recycle(executor0);
    engine->Release();
}

/**
  * @tc.name: ExecutorTest005
  * @tc.desc: Test find executor abnormal
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest005, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    auto [errCode, engine] = GetVirtualEngine(2); // max read is 2
    ASSERT_EQ(errCode, E_OK);
    engine->SetEnhance(true);
    /**
     * @tc.steps:step1. create new handle without mock func
     * @tc.expected: step1. create ok.
     */
    auto executor0 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 0);
    EXPECT_NE(executor0, nullptr);
    /**
     * @tc.steps:step2. create new handle with mock func, mock func will release executor
     * @tc.expected: step2. create fail.
     */
    engine->ForkNewExecutorMethod([executor = executor0, enginePtr = engine](bool, StorageExecutor *&handle) {
        StorageExecutor *executorPtr = executor;
        enginePtr->Recycle(executorPtr);
        handle = nullptr;
        return -E_EKEYREVOKED;
    });
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 0);
    EXPECT_EQ(executor1, nullptr);
    engine->Release();
    engine->ForkNewExecutorMethod(nullptr);
}

/**
  * @tc.name: ExecutorTest006
  * @tc.desc: Test find executor abnormal when get executor timeout and operate abort
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest006, TestSize.Level4)
{
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    auto [errCode, engine] = GetVirtualEngine(1); // max read is 1
    ASSERT_EQ(errCode, E_OK);
    /**
     * @tc.steps:step1. create new handle
     * @tc.expected: step1. create ok.
     */
    auto executor0 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode);
    EXPECT_NE(executor0, nullptr);
    /**
     * @tc.steps:step2. create new handle again
     * @tc.expected: step2. create failed by timeout.
     */
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 1); // max wait 1s
    EXPECT_EQ(executor1, nullptr);
    /**
     * @tc.steps:step3. create new handle again and async mark operate abort
     * @tc.expected: step3. create failed by operate abort.
     */
    std::thread t([enginePtr = engine]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        enginePtr->Abort();
    });
    executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode, false, 5); // max wait 5s
    EXPECT_EQ(executor1, nullptr);
    t.join();
    engine->Recycle(executor0);
    engine->Release();
}

/**
  * @tc.name: ExecutorTest007
  * @tc.desc: Test diff executor pool will not recycle others pool's executor
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest007, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    auto [errCode, engine1] = GetVirtualEngine(1); // max read is 1
    ASSERT_EQ(errCode, E_OK);
    auto [ret, engine2] = GetVirtualEngine(1); // max read is 1
    ASSERT_EQ(ret, E_OK);
    /**
     * @tc.steps:step1. create new handle
     * @tc.expected: step1. create ok.
     */
    auto executor1 = engine1->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode);
    EXPECT_NE(executor1, nullptr);
    /**
     * @tc.steps:step2. create new handle
     * @tc.expected: step2. create ok.
     */
    auto tmp = executor1;
    engine2->Recycle(tmp);
    engine1->Recycle(executor1);
    EXPECT_EQ(executor1, nullptr);
    engine1->Release();
    engine2->Release();
}

/**
  * @tc.name: ExecutorTest008
  * @tc.desc: Test mem executor pool
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest008, TestSize.Level0)
{
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    auto [errCode, engine1] = GetVirtualEngine(1, true); // max read is 1
    /**
     * @tc.steps:step1. create new handle
     * @tc.expected: step1. create ok.
     */
    auto executor1 = engine1->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode);
    EXPECT_NE(executor1, nullptr);
    engine1->Recycle(executor1);
    engine1->CallSetSQL({});
    engine1->Release();
}

/**
  * @tc.name: ExecutorTest009
  * @tc.desc: Test cache executor pool
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest009, TestSize.Level0)
{
    auto systemApi = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(systemApi);
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    SecurityOption option = {S3, SECE};
    auto [errCode, engine] = GetVirtualEngineWithSecurity(2, false, option, true); // max read is 2
    /**
     * @tc.steps:step1. create new handle
     * @tc.expected: step1. create ok.
     */
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode);
    EXPECT_NE(executor1, nullptr);
    /**
     * @tc.steps:step2. create new handle with fork open main failed
     * @tc.expected: step2. create failed because of read handle was not allowed.
     */
    engine->ForkOpenMainDatabaseMethod([executor1, enginePtr = engine](bool, sqlite3 *&db, OpenDbProperties &) {
        StorageExecutor *executor = executor1;
        enginePtr->Recycle(executor);
        db = nullptr;
        return -E_EKEYREVOKED;
    });
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, errCode);
    EXPECT_EQ(executor2, nullptr);
    engine->ForkOpenMainDatabaseMethod(nullptr);
    engine->Recycle(executor2);
    engine->Release();
}

/**
  * @tc.name: ExecutorTest010
  * @tc.desc: Test cache executor pool
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zqq
  */
HWTEST_F(DistributedDBStorageSQLiteSingleVerStorageEngineTest, ExecutorTest010, TestSize.Level0)
{
    auto systemApi = std::make_shared<ProcessSystemApiAdapterImpl>();
    RuntimeContext::GetInstance()->SetProcessSystemApiAdapter(systemApi);
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
    CopyCacheDb();
    SecurityOption option = {S3, SECE};
    auto [errCode, engine] = GetVirtualEngineWithSecurity(2, false, option, true); // max read is 2
    ASSERT_EQ(errCode, E_OK);
    /**
     * @tc.steps:step1. create new handle with create if necessary
     * @tc.expected: step1. create ok.
     */
    auto properties = GetProperties(false, option, true);
    auto [ret, handle] = engine->GetCacheHandle(properties);
    EXPECT_EQ(ret, E_OK);
    if (handle != nullptr) {
        EXPECT_EQ(sqlite3_close_v2(handle), SQLITE_OK);
    }
    /**
     * @tc.steps:step2. create new handle without create if necessary
     * @tc.expected: step2. create ok.
     */
    properties = GetProperties(false, option, false);
    std::tie(ret, handle) = engine->GetCacheHandle(properties);
    EXPECT_EQ(ret, E_OK);
    if (handle != nullptr) {
        EXPECT_EQ(sqlite3_close_v2(handle), SQLITE_OK);
    }
    /**
     * @tc.steps:step3. create new handle without create if necessary and dir was removed
     * @tc.expected: step3. create failed.
     */
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    properties.createIfNecessary = false;
    std::tie(ret, handle) = engine->GetCacheHandle(properties);
    EXPECT_EQ(ret, -E_INVALID_DB);
    engine->Release();
}