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
#include <gtest/gtest.h>

#include "db_common.h"
#include "distributeddb_storage_rd_single_ver_natural_store_testcase.h"
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

    RdSingleVerNaturalStore *g_store = nullptr;
    RdSingleVerNaturalStoreConnection *g_connection = nullptr;

    void GetStorageEngine(RdSingleVerStorageEngine *&storageEngine)
    {
        int errCode;
        storageEngine =
            static_cast<RdSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(g_property, errCode));
        EXPECT_EQ(errCode, E_OK);
    }
}

class DistributedDBStorageRdSingleVerStorageEngineTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBStorageRdSingleVerStorageEngineTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGI("DistributedDBStorageRdSingleVerStorageEngineTest dir is %s", g_testDir.c_str());
    std::string oriIdentifier = APP_ID + "-" + USER_ID + "-" + "TestGeneralNBStorageEngine";
    std::string identifier = DBCommon::TransferHashString(oriIdentifier);
    g_identifier = DBCommon::TransferStringToHex(identifier);

    g_databaseName = "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::MAINDB_DIR + "/" +
        DBConstant::SINGLE_VER_DATA_STORE + DBConstant::DB_EXTENSION;
    g_property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    g_property.SetStringProp(KvDBProperties::STORE_ID, "TestGeneralNBStorageEngine");
    g_property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, g_identifier);
    g_property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_RD_KERNAL);
}

void DistributedDBStorageRdSingleVerStorageEngineTest::TearDownTestCase(void)
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR);
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBStorageRdSingleVerStorageEngineTest::SetUp(void)
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
}

void DistributedDBStorageRdSingleVerStorageEngineTest::TearDown(void)
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
HWTEST_F(DistributedDBStorageRdSingleVerStorageEngineTest, DataTest001, TestSize.Level1)
{
    RdSingleVerStorageEngine *storageEngine = nullptr;
    GetStorageEngine(storageEngine);
    ASSERT_NE(storageEngine, nullptr);

    storageEngine->SetEngineState(EngineState::ENGINE_BUSY);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), -E_NOT_SUPPORT);
    storageEngine->SetEngineState(EngineState::ATTACHING);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), -E_NOT_SUPPORT);
    storageEngine->SetEngineState(EngineState::MAINDB);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), -E_NOT_SUPPORT);
    storageEngine->SetEngineState(EngineState::CACHEDB);
    EXPECT_EQ(storageEngine->ExecuteMigrate(), -E_NOT_SUPPORT);
    storageEngine->Release();
    storageEngine = nullptr;
}

/**
  * @tc.name: StorageEnginePrintDbFileMsg001
  * @tc.desc: test when file is not exist
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: xiefengzhu
  */
HWTEST_F(DistributedDBStorageRdSingleVerStorageEngineTest, StorageEnginePrintDbFileMsg001, TestSize.Level1)
{
    // release and print db file msg when db file is not exist
    RdSingleVerStorageEngine *storageEngine1 = nullptr;
    GetStorageEngine(storageEngine1);
    ASSERT_NE(storageEngine1, nullptr);
    storageEngine1->Release();
    EXPECT_EQ(storageEngine1->GetEngineState(), EngineState::INVALID);
    storageEngine1 = nullptr;
    // release and print db file msg when db wal file is not exist
    RdSingleVerStorageEngine *storageEngine2 = nullptr;
    GetStorageEngine(storageEngine2);
    ASSERT_NE(storageEngine2, nullptr);

    std::string dbPath = "/tmp/test_db";
    storageEngine2->SetUri(dbPath);

    std::ofstream dbFile(dbPath);
    dbFile.close();
    storageEngine2->Release();
    EXPECT_EQ(storageEngine2->GetEngineState(), EngineState::INVALID);
    storageEngine2 = nullptr;
    // release and print db file msg when db shm file is not exist
    RdSingleVerStorageEngine *storageEngine3 = nullptr;
    GetStorageEngine(storageEngine3);
    ASSERT_NE(storageEngine3, nullptr);

    storageEngine3->SetUri(dbPath);
    std::string walPath = dbPath + "-wal";
    std::ofstream walFile(walPath);
    walFile.close();
    storageEngine3->Release();
    EXPECT_EQ(storageEngine3->GetEngineState(), EngineState::INVALID);
    storageEngine3 = nullptr;
    // release and print db file msg when db dwr file is not exist
    RdSingleVerStorageEngine *storageEngine4 = nullptr;
    GetStorageEngine(storageEngine4);
    ASSERT_NE(storageEngine4, nullptr);

    storageEngine4->SetUri(dbPath);
    std::string shmPath = dbPath + "-shm";
    std::ofstream shmFile(shmPath);
    shmFile.close();
    storageEngine4->Release();
    EXPECT_EQ(storageEngine4->GetEngineState(), EngineState::INVALID);
    storageEngine4 = nullptr;
    //remove temp file
    remove(dbPath.c_str());
    remove(walPath.c_str());
    remove(shmPath.c_str());
}
#endif // USE_RD_KERNEL
