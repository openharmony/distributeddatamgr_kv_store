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

#include <condition_variable>
#include <gtest/gtest.h>
#include <thread>

#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"
#include "relational_store_manager.h"
#include "runtime_config.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    std::shared_ptr<std::string> g_testDir = nullptr;
    VirtualCommunicatorAggregator* g_commAggregator = nullptr;
    const std::string DEVICE_A = "real_device";
    const std::string DEVICE_B = "deviceB";
    const std::string KEY_INSTANCE_ID = "INSTANCE_ID";
    KvVirtualDevice *g_deviceB = nullptr;
    DistributedDBToolsUnitTest g_tool;

    DBStatus OpenDelegate(const std::string &dlpPath, KvStoreNbDelegate *&ptr,
        KvStoreDelegateManager &mgr, bool syncDualTupleMode = false)
    {
        if (g_testDir == nullptr) {
            return DB_ERROR;
        }
        std::string dbPath = *g_testDir + dlpPath;
        KvStoreConfig storeConfig;
        storeConfig.dataDir = dbPath;
        OS::MakeDBDirectory(dbPath);
        mgr.SetKvStoreConfig(storeConfig);

        string dir = dbPath + "/single_ver";
        DIR* dirTmp = opendir(dir.c_str());
        if (dirTmp == nullptr) {
            OS::MakeDBDirectory(dir);
        } else {
            closedir(dirTmp);
        }

        KvStoreNbDelegate::Option option;
        option.syncDualTupleMode = syncDualTupleMode;
        DBStatus status = OK;
        mgr.GetKvStore(STORE_ID_1, option, [&ptr, &status](DBStatus status, KvStoreNbDelegate *delegate) {
            ptr = delegate;
            status = status;
        });
        return status;
    }

    DBStatus OpenDelegate(const std::string &dlpPath, RelationalStoreDelegate *&rdbDelegatePtr,
        RelationalStoreManager &mgr)
    {
        if (g_testDir == nullptr) {
            return DB_ERROR;
        }
        std::string dbDir = *g_testDir + dlpPath;
        OS::MakeDBDirectory(dbDir);
        std::string dbPath = dbDir + "/test.db";
        auto db = RelationalTestUtils::CreateDataBase(dbPath);
        EXPECT_EQ(sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
        db = nullptr;
        RelationalStoreDelegate::Option option;
        return mgr.OpenStore(dbPath, STORE_ID_1, option, rdbDelegatePtr);
    }

    void CloseDelegate(KvStoreNbDelegate *&ptr, KvStoreDelegateManager &mgr, std::string storeID)
    {
        if (ptr == nullptr) {
            return;
        }
        EXPECT_EQ(mgr.CloseKvStore(ptr), OK);
        ptr = nullptr;
        EXPECT_EQ(mgr.DeleteKvStore(storeID), OK);
    }

    void CloseDelegate(RelationalStoreDelegate *&ptr, RelationalStoreManager &mgr)
    {
        if (ptr == nullptr) {
            return;
        }
        EXPECT_EQ(mgr.CloseStore(ptr), OK);
        ptr = nullptr;
    }
}

class KvDBSingleVerDLPTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void KvDBSingleVerDLPTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    std::string dir;
    DistributedDBToolsUnitTest::TestDirInit(dir);
    if (g_testDir == nullptr) {
        g_testDir = std::make_shared<std::string>(dir);
    }

    g_commAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_commAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_commAggregator);
}

void KvDBSingleVerDLPTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (g_testDir != nullptr && DistributedDBToolsUnitTest::RemoveTestDbFiles(*g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void KvDBSingleVerDLPTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    ASSERT_TRUE(g_deviceB != nullptr);
    VirtualSingleVerSyncDBInterface *syncInterface = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    ASSERT_TRUE(syncInterface != nullptr);
    ASSERT_EQ(g_deviceB->Initialize(g_commAggregator, syncInterface), E_OK);
}

void KvDBSingleVerDLPTest::TearDown(void)
{
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
    PermissionCheckCallbackV3 nullCallback = nullptr;
    RuntimeConfig::SetPermissionCheckCallback(nullCallback);
    SyncActivationCheckCallbackV2 active = nullptr;
    RuntimeConfig::SetSyncActivationCheckCallback(active);
    RuntimeConfig::SetPermissionConditionCallback(nullptr);
}

/**
 * @tc.name: SameDelegateTest001
 * @tc.desc: Test kv delegate open with diff instanceID.
 * @tc.type: FUNC
 * @tc.require: SR000H0JSC
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBSingleVerDLPTest, SameDelegateTest001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);

    KvStoreDelegateManager mgr2(APP_ID, USER_ID, INSTANCE_ID_2);
    KvStoreNbDelegate *ptr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp2", ptr2, mgr2), OK);
    ASSERT_NE(ptr2, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    ptr1->Put(key1, value1);
    Key key2 = {'k', '2'};
    Value value2 = {'v', '2'};
    ptr2->Put(key2, value2);

    Value value;
    EXPECT_EQ(ptr1->Get(key1, value), OK);
    EXPECT_EQ(value1, value);
    EXPECT_EQ(ptr2->Get(key2, value), OK);
    EXPECT_EQ(value2, value);

    EXPECT_EQ(ptr1->Get(key2, value), NOT_FOUND);
    EXPECT_EQ(ptr2->Get(key1, value), NOT_FOUND);

    CloseDelegate(ptr1, mgr1, STORE_ID_1);
    CloseDelegate(ptr2, mgr2, STORE_ID_1);
}

/**
 * @tc.name: SameDelegateTest002
 * @tc.desc: Test rdb delegate open with diff instanceID.
 * @tc.type: FUNC
 * @tc.require: SR000H0JSC
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBSingleVerDLPTest, SameDelegateTest002, TestSize.Level1)
{
    RelationalStoreManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    RelationalStoreDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);

    RelationalStoreManager mgr2(APP_ID, USER_ID, INSTANCE_ID_2);
    RelationalStoreDelegate *ptr2 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp2", ptr2, mgr2), OK);
    ASSERT_NE(ptr2, nullptr);

    CloseDelegate(ptr1, mgr1);
    CloseDelegate(ptr2, mgr2);
}

/**
 * @tc.name: DlpDelegateCRUDTest001
 * @tc.desc: Test dlp delegate crud function.
 * @tc.type: FUNC
 * @tc.require: SR000H0JSC
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBSingleVerDLPTest, DlpDelegateCRUDTest001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    ptr1->Put(key1, value1);

    Value value;
    EXPECT_EQ(ptr1->Get(key1, value), OK);
    EXPECT_EQ(value1, value);

    Value value2 = {'v', '2'};
    ptr1->Put(key1, value2);
    EXPECT_EQ(ptr1->Get(key1, value), OK);
    EXPECT_EQ(value2, value);

    EXPECT_EQ(ptr1->Delete(key1), OK);
    EXPECT_EQ(ptr1->Get(key1, value), NOT_FOUND);

    CloseDelegate(ptr1, mgr1, STORE_ID_1);
}

/**
 * @tc.name: SandboxDelegateSync001
 * @tc.desc: Test dlp delegate sync function.
 * @tc.type: FUNC
 * @tc.require: SR000H0JSC
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBSingleVerDLPTest, SandboxDelegateSync001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    RuntimeConfig::SetPermissionCheckCallback([](const PermissionCheckParam &param, uint8_t flag) {
        if ((flag & PermissionCheckFlag::CHECK_FLAG_RECEIVE) != 0) {
            bool status = false;
            if (param.extraConditions.find(KEY_INSTANCE_ID) != param.extraConditions.end()) {
                status = param.extraConditions.at(KEY_INSTANCE_ID) == std::to_string(INSTANCE_ID_1);
            }
            return status;
        }
        if (param.userId != USER_ID || param.appId != APP_ID || param.storeId != STORE_ID_1 ||
            (param.instanceId != INSTANCE_ID_1 && param.instanceId != 0)) {
            return false;
        }
        return true;
    });
    RuntimeConfig::SetPermissionConditionCallback([](const PermissionConditionParam &param) {
        std::map<std::string, std::string> status;
        status.emplace(KEY_INSTANCE_ID, std::to_string(INSTANCE_ID_1));
        return status;
    });

    KvStoreNbDelegate *ptr1 = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", ptr1, mgr1), OK);
    ASSERT_NE(ptr1, nullptr);

    Key key1 = {'k', '1'};
    Value value1 = {'v', '1'};
    ptr1->Put(key1, value1);

    std::map<std::string, DBStatus> status;
    DBStatus status = g_tool.SyncTest(ptr1, { DEVICE_B }, SYNC_MODE_PUSH_ONLY, status);
    EXPECT_TRUE(status == OK);
    EXPECT_EQ(status[DEVICE_B], OK);

    CloseDelegate(ptr1, mgr1, STORE_ID_1);
}

/**
 * @tc.name: SandboxDelegateSync002
 * @tc.desc: Test dlp delegate sync active if callback return true.
 * @tc.type: FUNC
 * @tc.require: SR000H0JSC
 * @tc.author: zhangqiquan
 */
HWTEST_F(KvDBSingleVerDLPTest, SandboxDelegateSync002, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    RuntimeConfig::SetSyncActivationCheckCallback([](const ActivationCheckParam &param) {
        if (param.userId == USER_ID && param.appId == APP_ID && param.storeId == STORE_ID_1 &&
            param.instanceId == INSTANCE_ID_1) {
            return true;
        }
        return false;
    });

    KvStoreNbDelegate *ptr = nullptr;
    EXPECT_EQ(OpenDelegate("/dlp1", ptr, mgr1, true), OK);
    ASSERT_NE(ptr, nullptr);

    std::map<std::string, DBStatus> status;
    DBStatus status = g_tool.SyncTest(ptr, { DEVICE_B }, SYNC_MODE_PUSH_ONLY, status);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(status[DEVICE_B], OK);

    CloseDelegate(ptr, mgr1, STORE_ID_1);
}