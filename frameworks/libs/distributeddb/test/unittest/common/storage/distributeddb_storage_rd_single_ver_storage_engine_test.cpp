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
    g_store->DecObjRef(g_store);
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
#endif // USE_RD_KERNEL