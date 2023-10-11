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

#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "relational_store_manager.h"
#include "runtime_config.h"
#include "virtual_relational_ver_sync_db_interface.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char *DB_SUFFIX = ".db";
    constexpr const char *STORE_ID = "Relational_Store_ID";
    std::string g_testDir;
    std::string g_dbDir;
    std::string g_storePath;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreDelegate *g_delegate = nullptr;

    class DistributedDBInterfacesRelationalObserverTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    };

    void DistributedDBInterfacesRelationalObserverTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        LOGD("Test dir is %s", g_testDir.c_str());
        g_dbDir = g_testDir + "/";
        g_storePath = g_dbDir + STORE_ID + DB_SUFFIX;
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    }

    void DistributedDBInterfacesRelationalObserverTest::TearDownTestCase(void)
    {
    }

    void DistributedDBInterfacesRelationalObserverTest::SetUp(void)
    {
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

        DBStatus status = g_mgr.OpenStore(g_storePath, STORE_ID, {}, g_delegate);
        EXPECT_EQ(status, OK);
        ASSERT_NE(g_delegate, nullptr);
    }

    void DistributedDBInterfacesRelationalObserverTest::TearDown(void)
    {
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), OK);
        g_delegate = nullptr;
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    }

    /**
     * @tc.name: RelationalObserverTest001
     * @tc.desc: Test invalid args for register unregister observer
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBInterfacesRelationalObserverTest, RegisterObserverTest001, TestSize.Level0)
    {
        EXPECT_EQ(g_delegate->RegisterObserver(nullptr), INVALID_ARGS);
        EXPECT_EQ(g_delegate->UnRegisterObserver(nullptr), INVALID_ARGS);
    }

    /**
     * @tc.name: RelationalObserverTest002
     * @tc.desc: Test register same observer twice
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBInterfacesRelationalObserverTest, RegisterObserverTest002, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. register observer1 twice
         * @tc.expected: step1. Return OK for first, return ALREADY_SET for second.
         */
        RelationalStoreObserverUnitTest *observer1 = new (std::nothrow) RelationalStoreObserverUnitTest();
        EXPECT_NE(observer1, nullptr);
        EXPECT_EQ(g_delegate->RegisterObserver(observer1), OK);
        EXPECT_EQ(g_delegate->RegisterObserver(observer1), ALREADY_SET);

        /**
         * @tc.steps:step2. unregister observer1 twice
         * @tc.expected: step2. Return OK for first, return NOT_FOUND for second.
         */
        EXPECT_EQ(g_delegate->UnRegisterObserver(observer1), OK);
        EXPECT_EQ(g_delegate->UnRegisterObserver(observer1), NOT_FOUND);

        /**
         * @tc.steps:step3. register observer1 again
         * @tc.expected: step3. Return OK.
         */
        EXPECT_EQ(g_delegate->RegisterObserver(observer1), OK);

        delete observer1;
        observer1 = nullptr;
    }

    /**
     * @tc.name: RelationalObserverTest003
     * @tc.desc: Test register observer over limit
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBInterfacesRelationalObserverTest, RegisterObserverTest003, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. register DBConstant::MAX_OBSERVER_COUNT + 1 observer
         * @tc.expected: step1. Return OK for first DBConstant::MAX_OBSERVER_COUNT, return OVER_MAX_LIMITS for
         * DBConstant::MAX_OBSERVER_COUNT + 1.
         */
        RelationalStoreObserverUnitTest *observerArr[DBConstant::MAX_OBSERVER_COUNT + 1] = { nullptr };
        for (int i = 0; i <= DBConstant::MAX_OBSERVER_COUNT; i++) {
            observerArr[i] = new (std::nothrow) RelationalStoreObserverUnitTest();
            EXPECT_NE(observerArr[i], nullptr);
            if (i < DBConstant::MAX_OBSERVER_COUNT) {
                EXPECT_EQ(g_delegate->RegisterObserver(observerArr[i]), OK);
            } else {
                EXPECT_EQ(g_delegate->RegisterObserver(observerArr[i]), OVER_MAX_LIMITS);
            }
        }

        /**
         * @tc.steps:step2. unregister observer1, than register again
         * @tc.expected: step2. Return OK
         */
        EXPECT_EQ(g_delegate->UnRegisterObserver(observerArr[0]), OK);
        EXPECT_EQ(g_delegate->RegisterObserver(observerArr[0]), OK);

        /**
         * @tc.steps:step3. call UnRegisterObserver() unregister all observer
         * @tc.expected: step3. Return OK
         */
        EXPECT_EQ(g_delegate->UnRegisterObserver(), OK);

        /**
         * @tc.steps:step4. register DBConstant::MAX_OBSERVER_COUNT + 1 observer
         * @tc.expected: step4. Return OK for first DBConstant::MAX_OBSERVER_COUNT, return OVER_MAX_LIMITS for
         * DBConstant::MAX_OBSERVER_COUNT + 1.
         */
        for (int i = 0; i <= DBConstant::MAX_OBSERVER_COUNT; i++) {
            if (i < DBConstant::MAX_OBSERVER_COUNT) {
                EXPECT_EQ(g_delegate->RegisterObserver(observerArr[i]), OK);
            } else {
                EXPECT_EQ(g_delegate->RegisterObserver(observerArr[i]), OVER_MAX_LIMITS);
            }
        }

        for (int i = 0; i <= DBConstant::MAX_OBSERVER_COUNT; i++) {
            delete observerArr[i];
            observerArr[i] = nullptr;
        }
    }

    /**
     * @tc.name: RelationalObserverTest004
     * @tc.desc: Test register observer over limit when OpenStore will register one observer
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBInterfacesRelationalObserverTest, RegisterObserverTest004, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. open store with observer
         * @tc.expected: step1. Return OK
         */
        RelationalStoreDelegate *delegate = nullptr;
        RelationalStoreObserverUnitTest *observer = new (std::nothrow) RelationalStoreObserverUnitTest();
        EXPECT_NE(observer, nullptr);
        RelationalStoreDelegate::Option option;
        option.observer = observer;
        EXPECT_EQ(g_mgr.OpenStore(g_storePath, STORE_ID, option, delegate), OK);
        ASSERT_NE(delegate, nullptr);

        /**
         * @tc.steps:step2. register DBConstant::MAX_OBSERVER_COUNT observer
         * @tc.expected: step2. Return OK for first DBConstant::MAX_OBSERVER_COUNT - 1, return OVER_MAX_LIMITS for
         * DBConstant::MAX_OBSERVER_COUNT because already register one observer when open store.
         */
        RelationalStoreObserverUnitTest *observerArr[DBConstant::MAX_OBSERVER_COUNT] = { nullptr };
        for (int i = 0; i < DBConstant::MAX_OBSERVER_COUNT; i++) {
            observerArr[i] = new (std::nothrow) RelationalStoreObserverUnitTest();
            EXPECT_NE(observerArr[i], nullptr);
            if (i < DBConstant::MAX_OBSERVER_COUNT - 1) {
                EXPECT_EQ(delegate->RegisterObserver(observerArr[i]), OK);
            } else {
                EXPECT_EQ(delegate->RegisterObserver(observerArr[i]), OVER_MAX_LIMITS);
            }
        }

        /**
         * @tc.steps:step3. close store
         * @tc.expected: step3. Return OK
         */
        EXPECT_EQ(g_mgr.CloseStore(delegate), OK);
        delegate = nullptr;
        delete observer;
        observer = nullptr;
        for (int i = 0; i < DBConstant::MAX_OBSERVER_COUNT; i++) {
            delete observerArr[i];
            observerArr[i] = nullptr;
        }
    }

    /**
     * @tc.name: RelationalObserverTest005
     * @tc.desc: Test register observer with two delegate
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBInterfacesRelationalObserverTest, RegisterObserverTest005, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. register DBConstant::MAX_OBSERVER_COUNT + 1 observer
         * @tc.expected: step1. Return OK for first DBConstant::MAX_OBSERVER_COUNT, return OVER_MAX_LIMITS for
         * DBConstant::MAX_OBSERVER_COUNT + 1.
         */
        RelationalStoreObserverUnitTest *observerArr[DBConstant::MAX_OBSERVER_COUNT + 1] = {nullptr};
        for (int i = 0; i <= DBConstant::MAX_OBSERVER_COUNT; i++) {
            observerArr[i] = new(std::nothrow) RelationalStoreObserverUnitTest();
            EXPECT_NE(observerArr[i], nullptr);
            if (i < DBConstant::MAX_OBSERVER_COUNT) {
                EXPECT_EQ(g_delegate->RegisterObserver(observerArr[i]), OK);
            } else {
                EXPECT_EQ(g_delegate->RegisterObserver(observerArr[i]), OVER_MAX_LIMITS);
            }
        }

        /**
         * @tc.steps:step2. register DBConstant::MAX_OBSERVER_COUNT + 1 observer with another delegate
         * @tc.expected: step2. Return OK for first DBConstant::MAX_OBSERVER_COUNT, return OVER_MAX_LIMITS for
         * DBConstant::MAX_OBSERVER_COUNT + 1.
         */
        RelationalStoreDelegate *delegate = nullptr;
        EXPECT_EQ(g_mgr.OpenStore(g_storePath, STORE_ID, {}, delegate), OK);
        ASSERT_NE(delegate, nullptr);
        RelationalStoreObserverUnitTest *observerArr1[DBConstant::MAX_OBSERVER_COUNT + 1] = {nullptr};
        for (int i = 0; i <= DBConstant::MAX_OBSERVER_COUNT; i++) {
            observerArr1[i] = new(std::nothrow) RelationalStoreObserverUnitTest();
            EXPECT_NE(observerArr1[i], nullptr);
            if (i < DBConstant::MAX_OBSERVER_COUNT) {
                EXPECT_EQ(delegate->RegisterObserver(observerArr1[i]), OK);
            } else {
                EXPECT_EQ(delegate->RegisterObserver(observerArr1[i]), OVER_MAX_LIMITS);
            }
        }

        /**
         * @tc.steps:step3. close store and delete observer
         * @tc.expected: step3. Return OK
         */
        EXPECT_EQ(g_mgr.CloseStore(delegate), OK);
        delegate = nullptr;
        for (int i = 0; i <= DBConstant::MAX_OBSERVER_COUNT; i++) {
            delete observerArr[i];
            observerArr[i] = nullptr;
            delete observerArr1[i];
            observerArr1[i] = nullptr;
        }
    }
}
