/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "distributeddb_tools_unit_test.h"
#include "sqlite_single_ver_storage_engine.h"
#include "virtual_sqlite_storage_engine.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    const int DELAY_TIME_MS = 100;
    std::string g_testDir;
    std::string g_databaseName;
    std::string g_identifier;
    KvDBProperties g_property;

    void PrepareEnv()
    {
        sqlite3 *db;
        ASSERT_TRUE(sqlite3_open_v2((g_testDir + g_databaseName).c_str(),
            &db, SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK);
        sqlite3_close_v2(db);
    }

    std::pair<int, std::shared_ptr<DistributedDB::VirtualSingleVerStorageEngine>> GetVirtualEngine(
        uint32_t maxRead = 2)
    {
        std::pair<int, std::shared_ptr<DistributedDB::VirtualSingleVerStorageEngine>> res;
        auto &[errCode, engine] = res;
        engine = std::make_shared<DistributedDB::VirtualSingleVerStorageEngine>();
        StorageEngineAttr poolSize = {1, 1, 1, maxRead}; // at most 1 write, maxRead read.
        OpenDbProperties option;
        option.uri = g_testDir + g_databaseName;
        option.createIfNecessary = false;
        option.isMemDb = false;
        option.subdir = g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR;
        errCode = engine->InitSQLiteStorageEngine(poolSize, option, "");
        return res;
    }
}

class DistributedDBStorageEngineDelayReleaseTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void DistributedDBStorageEngineDelayReleaseTest::SetUpTestCase()
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    std::string oriIdentifier = "app0-user0-TestStorageEngineDelayRelease";
    g_identifier = DBCommon::TransferHashString(oriIdentifier);
    g_databaseName = "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::MAINDB_DIR + "/" +
        DBConstant::SINGLE_VER_DATA_STORE + DBConstant::DB_EXTENSION;
    g_property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    g_property.SetStringProp(KvDBProperties::STORE_ID, "TestStorageEngineDelayRelease");
    g_property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, g_identifier);
    g_property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_SQLITE);
}

void DistributedDBStorageEngineDelayReleaseTest::TearDownTestCase()
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR);
}

void DistributedDBStorageEngineDelayReleaseTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + g_identifier + "/" + DBConstant::SINGLE_SUB_DIR);
    std::string identDir = g_testDir + "/" + g_identifier;
    std::string singleDir = identDir + "/" + DBConstant::SINGLE_SUB_DIR;
    std::string mainDir = singleDir + "/" + DBConstant::MAINDB_DIR;
    DBCommon::CreateDirectory(identDir);
    DBCommon::CreateDirectory(singleDir);
    DBCommon::CreateDirectory(mainDir);
    ASSERT_NO_FATAL_FAILURE(PrepareEnv());
}

void DistributedDBStorageEngineDelayReleaseTest::TearDown()
{
}

/**
 * @tc.name: DelayReleaseTest001
 * @tc.desc: When delayed release is disabled, excess read executor is deleted immediately.
 * @tc.type: FUNC
 */
HWTEST_F(DistributedDBStorageEngineDelayReleaseTest, DelayReleaseTest001, TestSize.Level0)
{
    auto [errCode, engine] = GetVirtualEngine(2); // maxRead is 2
    ASSERT_EQ(errCode, E_OK);
    int ret = E_OK;
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor1, nullptr);
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor2, nullptr);
    // Recycle both, no delayed release enabled, both should be put into idle list.
    engine->Recycle(executor1);
    engine->Recycle(executor2);
    // Still able to fetch executor again.
    auto executor3 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor3, nullptr);
    engine->Recycle(executor3);
    engine->Release();
}

/**
 * @tc.name: DelayReleaseTest002
 * @tc.desc: When delayed release enabled, excess read executor is reused instead of recreated.
 * @tc.type: FUNC
 */
HWTEST_F(DistributedDBStorageEngineDelayReleaseTest, DelayReleaseTest002, TestSize.Level0)
{
    auto [errCode, engine] = GetVirtualEngine(2); // maxRead is 2
    ASSERT_EQ(errCode, E_OK);
    engine->SetReadExecutorDelayRelease(true, DELAY_TIME_MS);
    int createCount = 0;
    engine->ForkNewExecutorMethod([&createCount](bool, StorageExecutor *&handle) {
        createCount++;
        handle = nullptr;
        return -E_BUSY;
    });
    int ret = E_OK;
    // First fetch should use the minRead executor (already created), no new create.
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor1, nullptr);
    EXPECT_EQ(createCount, 0);
    // Second fetch needs a new executor, creation fails -> null.
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_EQ(executor2, nullptr);
    engine->ForkNewExecutorMethod(nullptr);
    engine->Recycle(executor1);
    engine->Release();
}

/**
 * @tc.name: DelayReleaseTest003
 * @tc.desc: Delayed release keeps the excess executor alive within delay time, reuse avoids recreation.
 * @tc.type: FUNC
 */
HWTEST_F(DistributedDBStorageEngineDelayReleaseTest, DelayReleaseTest003, TestSize.Level4)
{
    auto [errCode, engine] = GetVirtualEngine(2); // maxRead is 2
    ASSERT_EQ(errCode, E_OK);
    engine->SetReadExecutorDelayRelease(true, DELAY_TIME_MS);
    int ret = E_OK;
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor1, nullptr);
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor2, nullptr);
    // Recycle both, one goes to idle list, the other goes to delayed release list.
    engine->Recycle(executor1);
    engine->Recycle(executor2);
    // Within delay time, reuse the delayed executor instead of creating a new one.
    int createCount = 0;
    engine->ForkNewExecutorMethod([&createCount](bool, StorageExecutor *&handle) {
        createCount++;
        handle = nullptr;
        return -E_BUSY;
    });
    auto executor3 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor3, nullptr);
    auto executor4 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor4, nullptr);
    EXPECT_EQ(createCount, 0);
    engine->ForkNewExecutorMethod(nullptr);
    engine->Recycle(executor3);
    engine->Recycle(executor4);
    engine->Release();
}

/**
 * @tc.name: DelayReleaseTest004
 * @tc.desc: After delay time elapses, the periodic timer releases the delayed executor.
 * @tc.type: FUNC
 */
HWTEST_F(DistributedDBStorageEngineDelayReleaseTest, DelayReleaseTest004, TestSize.Level4)
{
    auto [errCode, engine] = GetVirtualEngine(2); // maxRead is 2
    ASSERT_EQ(errCode, E_OK);
    engine->SetReadExecutorDelayRelease(true, DELAY_TIME_MS);
    int ret = E_OK;
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor1, nullptr);
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor2, nullptr);
    engine->Recycle(executor1);
    engine->Recycle(executor2);
    // Consume the idle executor so the next fetch has to rely on the delayed list being empty.
    auto executor3 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor3, nullptr);
    // Wait for the delay time to elapse so the periodic timer releases the delayed executor.
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_TIME_MS * 5)); // 5 * 100 ms
    int createCount = 0;
    engine->ForkNewExecutorMethod([&createCount](bool, StorageExecutor *&handle) {
        createCount++;
        handle = nullptr;
        return -E_BUSY;
    });
    // After the timer released the delayed executor, fetching needs a new one, creation fails -> null.
    auto executor4 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_EQ(executor4, nullptr);
    EXPECT_GT(createCount, 0);
    engine->ForkNewExecutorMethod(nullptr);
    engine->Recycle(executor3);
    engine->Release();
}

/**
 * @tc.name: DelayReleaseTest005
 * @tc.desc: Zero delay time boundary: the delayed executor expires immediately and is released.
 * @tc.type: FUNC
 */
HWTEST_F(DistributedDBStorageEngineDelayReleaseTest, DelayReleaseTest005, TestSize.Level0)
{
    auto [errCode, engine] = GetVirtualEngine(2); // maxRead is 2
    ASSERT_EQ(errCode, E_OK);
    engine->SetReadExecutorDelayRelease(true, 0); // boundary: zero delay time
    int ret = E_OK;
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor1, nullptr);
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor2, nullptr);
    engine->Recycle(executor1);
    engine->Recycle(executor2);
    // Consume the idle executor so the next fetch depends on the delayed list.
    auto executor3 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor3, nullptr);
    // Give the timer enough time to release the zero-delay executor.
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_TIME_MS * 5)); // 5 * 100 ms
    int createCount = 0;
    engine->ForkNewExecutorMethod([&createCount](bool, StorageExecutor *&handle) {
        createCount++;
        handle = nullptr;
        return -E_BUSY;
    });
    auto executor4 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_EQ(executor4, nullptr);
    EXPECT_GT(createCount, 0);
    engine->ForkNewExecutorMethod(nullptr);
    engine->Recycle(executor3);
    engine->Release();
}

/**
 * @tc.name: DelayReleaseTest006
 * @tc.desc: Large delay time boundary: the delayed executor remains reusable for a long period.
 * @tc.type: FUNC
 */
HWTEST_F(DistributedDBStorageEngineDelayReleaseTest, DelayReleaseTest006, TestSize.Level0)
{
    auto [errCode, engine] = GetVirtualEngine(2); // maxRead is 2
    ASSERT_EQ(errCode, E_OK);
    engine->SetReadExecutorDelayRelease(true, 60000); // boundary: very large delay time 60000 ms
    int ret = E_OK;
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor1, nullptr);
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor2, nullptr);
    engine->Recycle(executor1);
    engine->Recycle(executor2);
    // Consume the idle executor so the next fetch depends on the delayed list.
    auto executor3 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor3, nullptr);
    // Even after a wait well below the large delay, the delayed executor is reused, no new create.
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // sleep 50 ms
    int createCount = 0;
    engine->ForkNewExecutorMethod([&createCount](bool, StorageExecutor *&handle) {
        createCount++;
        handle = nullptr;
        return -E_BUSY;
    });
    auto executor4 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor4, nullptr);
    EXPECT_EQ(createCount, 0);
    engine->ForkNewExecutorMethod(nullptr);
    engine->Recycle(executor4);
    engine->Recycle(executor3);
    engine->Release();
}

/**
 * @tc.name: DelayReleaseTest007
 * @tc.desc: Toggling delayed release off stops further delayed release behavior and releases cleanly.
 * @tc.type: FUNC
 */
HWTEST_F(DistributedDBStorageEngineDelayReleaseTest, DelayReleaseTest007, TestSize.Level0)
{
    auto [errCode, engine] = GetVirtualEngine(2); // maxRead is 2
    ASSERT_EQ(errCode, E_OK);
    engine->SetReadExecutorDelayRelease(true, DELAY_TIME_MS);
    int ret = E_OK;
    auto executor1 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor1, nullptr);
    auto executor2 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor2, nullptr);
    engine->Recycle(executor1);
    engine->Recycle(executor2);
    // Disable delayed release; the pending timer is stopped.
    engine->SetReadExecutorDelayRelease(false, 0);
    // Fetching should still succeed through the normal idle/excess path.
    auto executor3 = engine->FindExecutor(false, OperatePerm::NORMAL_PERM, ret);
    EXPECT_NE(executor3, nullptr);
    engine->Recycle(executor3);
    engine->Release();
}