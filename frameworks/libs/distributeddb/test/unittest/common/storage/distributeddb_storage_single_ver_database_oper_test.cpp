/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "single_ver_database_oper.h"
#include "storage_engine_manager.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace testing;

namespace {
class DistributedDBStorageSingleVerDatabaseOperTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
protected:
    std::string testDir_;
    SQLiteSingleVerNaturalStore *singleVerNaturalStore_ = nullptr;
    SQLiteSingleVerStorageEngine *singleVerStorageEngine_ = nullptr;
};

void DistributedDBStorageSingleVerDatabaseOperTest::SetUpTestCase(void)
{
}

void DistributedDBStorageSingleVerDatabaseOperTest::TearDownTestCase(void)
{
}

void DistributedDBStorageSingleVerDatabaseOperTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    DistributedDBToolsUnitTest::TestDirInit(testDir_);
    singleVerNaturalStore_ = new (std::nothrow) SQLiteSingleVerNaturalStore();
    ASSERT_NE(singleVerNaturalStore_, nullptr);
    KvDBProperties property;
    property.SetStringProp(KvDBProperties::DATA_DIR, "");
    property.SetStringProp(KvDBProperties::STORE_ID, "TestDatabaseOper");
    property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, "TestDatabaseOper");
    property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);
    int errCode = E_OK;
    singleVerStorageEngine_ =
        static_cast<SQLiteSingleVerStorageEngine *>(StorageEngineManager::GetStorageEngine(property, errCode));
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(singleVerStorageEngine_, nullptr);
}

void DistributedDBStorageSingleVerDatabaseOperTest::TearDown(void)
{
    delete singleVerNaturalStore_;
    singleVerNaturalStore_ = nullptr;
    singleVerStorageEngine_ = nullptr;
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(testDir_) != E_OK) {
        LOGE("rm test db files error.");
    }
}

/**
 * @tc.name: DatabaseOperationTest001
 * @tc.desc: test Rekey, Import and Export interface in abnormal condition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, DatabaseOperationTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create singleVerDatabaseOper with nullptr;
     * @tc.expected: OK.
     */
    std::unique_ptr<SingleVerDatabaseOper> singleVerDatabaseOper =
        std::make_unique<SingleVerDatabaseOper>(singleVerNaturalStore_, nullptr);
    ASSERT_NE(singleVerDatabaseOper, nullptr);

    /**
     * @tc.steps: step2. Test Rekey, Import and Export interface;
     * @tc.expected: OK.
     */
    CipherPassword passwd;
    EXPECT_EQ(singleVerDatabaseOper->Rekey(passwd), -E_INVALID_DB);
    EXPECT_EQ(singleVerDatabaseOper->Import("", passwd), -E_INVALID_DB);
    EXPECT_EQ(singleVerDatabaseOper->Export("", passwd), -E_INVALID_DB);
    KvDBProperties properties;
    EXPECT_EQ(singleVerDatabaseOper->RekeyRecover(properties), -E_INVALID_ARGS);
    EXPECT_EQ(singleVerDatabaseOper->ClearImportTempFile(properties), -E_INVALID_ARGS);
    EXPECT_EQ(singleVerDatabaseOper->ClearExportedTempFiles(properties), -E_INVALID_ARGS);
}

/**
 * @tc.name: DatabaseOperationTest002
 * @tc.desc: test Import and Export interface in abnormal condition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBStorageSingleVerDatabaseOperTest, DatabaseOperationTest002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create singleVerDatabaseOper;
     * @tc.expected: OK.
     */
    std::unique_ptr<SingleVerDatabaseOper> singleVerDatabaseOper =
        std::make_unique<SingleVerDatabaseOper>(singleVerNaturalStore_, singleVerStorageEngine_);
    ASSERT_NE(singleVerDatabaseOper, nullptr);

    /**
     * @tc.steps: step2. Test Import and Export interface;
     * @tc.expected: OK.
     */
    CipherPassword passwd;
    EXPECT_EQ(singleVerDatabaseOper->Export("", passwd), -E_NOT_INIT);
    singleVerDatabaseOper->SetLocalDevId("device");
    EXPECT_EQ(singleVerDatabaseOper->Export("", passwd), -E_INVALID_ARGS);
}
}