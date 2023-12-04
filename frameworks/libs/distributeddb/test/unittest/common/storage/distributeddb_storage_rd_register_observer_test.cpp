/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <thread>
#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "default_factory.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "ikvdb_factory.h"
#include "log_print.h"
#include "rd_single_ver_natural_store.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;

    RdSingleVerNaturalStore *g_singleVerNaturaStore = nullptr;
    IKvDBConnection *g_singleVerNaturaStoreConnection = nullptr;
    bool g_createFactory = false;

    Key g_emptyKey;
    string g_keyStr1 = "key_1";
    string g_keyStr2 = "key_2";
    string g_keyStr3 = "key_3";
    string g_keyStr4 = "key_4";
    string g_keyStr5 = "key_5";
    string g_keyStr6 = "key_6";

    string g_valueStr1 = "value_1";
    string g_valueStr2 = "value_2";
    string g_valueStr3 = "value_3";
    string g_valueStr4 = "value_4";
    string g_valueStr5 = "value_5";
    string g_valueStr6 = "value_6";
    string g_oldValueStr3 = "old_value_3";
    string g_oldValueStr4 = "old_value_4";

    list<Entry> g_emptyEntries;
    Entry g_entry0;
    Entry g_entry1;
    Entry g_entry2;
    Entry g_entry3;
    Entry g_entry4;
    Entry g_entry5;
    Entry g_entry6;
    Entry g_oldEntry3;
    Entry g_oldEntry4;

    bool g_testFuncCalled = false;
    list<Entry> g_insertedEntries;
    list<Entry> g_updatedEntries;
    list<Entry> g_deletedEntries;

    Entry TransferStrToKyEntry(const string &key, const string &value)
    {
        Entry entry;
        entry.key.resize(key.size());
        entry.key.assign(key.begin(), key.end());
        entry.value.resize(value.size());
        entry.value.assign(value.begin(), value.end());
        return entry;
    }

    void TestFunc(const KvDBCommitNotifyData &data)
    {
        g_testFuncCalled = true;
        int errCode;
        g_insertedEntries = data.GetInsertedEntries(errCode);
        ASSERT_EQ(errCode, E_OK);
        g_updatedEntries = data.GetUpdatedEntries(errCode);
        ASSERT_EQ(errCode, E_OK);
        g_deletedEntries = data.GetDeletedEntries(errCode);
        ASSERT_EQ(errCode, E_OK);
        LOGI("Insert:%zu, update:%zu, delete:%zu", g_insertedEntries.size(), g_updatedEntries.size(),
            g_deletedEntries.size());
        return;
    }
}

class DistributedDBStorageRdRegisterObserverTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBStorageRdRegisterObserverTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    if (IKvDBFactory::GetCurrent() == nullptr) {
        IKvDBFactory *factory = new (std::nothrow) DefaultFactory();
        ASSERT_NE(factory, nullptr);
        if (factory == nullptr) {
            LOGE("failed to new DefaultFactory!");
            return;
        }
        IKvDBFactory::Register(factory);
        g_createFactory = true;
    }
    // prepare test entries
    g_entry1 = TransferStrToKyEntry(g_keyStr1, g_valueStr1);
    g_entry2 = TransferStrToKyEntry(g_keyStr2, g_valueStr2);
    g_entry3 = TransferStrToKyEntry(g_keyStr3, g_valueStr3);
    g_entry4 = TransferStrToKyEntry(g_keyStr4, g_valueStr4);
    g_entry5 = TransferStrToKyEntry(g_keyStr5, g_valueStr5);
    g_entry6 = TransferStrToKyEntry(g_keyStr6, g_valueStr6);
    g_oldEntry3 = TransferStrToKyEntry(g_keyStr3, g_oldValueStr3);
    g_oldEntry4 = TransferStrToKyEntry(g_keyStr4, g_oldValueStr4);
}

void DistributedDBStorageRdRegisterObserverTest::TearDownTestCase(void)
{
    if (g_createFactory) {
        if (IKvDBFactory::GetCurrent() != nullptr) {
            delete IKvDBFactory::GetCurrent();
            IKvDBFactory::Register(nullptr);
        }
    }
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
}

void DistributedDBStorageRdRegisterObserverTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    IKvDBFactory *factory = IKvDBFactory::GetCurrent();
    ASSERT_NE(factory, nullptr);
    if (factory == nullptr) {
        LOGE("failed to get DefaultFactory!");
        return;
    }

    g_singleVerNaturaStore = new (std::nothrow) RdSingleVerNaturalStore();
    ASSERT_NE(g_singleVerNaturaStore, nullptr);
    if (g_singleVerNaturaStore == nullptr) {
        return;
    }

    KvDBProperties property;
    property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    property.SetStringProp(KvDBProperties::STORE_ID, "TestGeneralNB");
    property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, "TestGeneralNB");
    property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_RD_KERNAL);
    int errCode = g_singleVerNaturaStore->Open(property);
    ASSERT_EQ(errCode, E_OK);
    if (errCode != E_OK) {
        g_singleVerNaturaStore = nullptr;
        return;
    }

    g_singleVerNaturaStoreConnection = g_singleVerNaturaStore->GetDBConnection(errCode);
    ASSERT_EQ(errCode, E_OK);
    ASSERT_NE(g_singleVerNaturaStoreConnection, nullptr);
}

void DistributedDBStorageRdRegisterObserverTest::TearDown(void)
{
    if (g_singleVerNaturaStoreConnection != nullptr) {
        g_singleVerNaturaStoreConnection->Close();
    }
    std::string identifierName;
    g_singleVerNaturaStore->DecObjRef(g_singleVerNaturaStore);
    identifierName = DBCommon::TransferStringToHex("TestGeneralNB");
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/" + identifierName + "/" + DBConstant::SINGLE_SUB_DIR);
}

/**
  * @tc.name: RegisterObserver001
  * @tc.desc: Register a NULL pointer as an observer
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: liujialei
  */
HWTEST_F(DistributedDBStorageRdRegisterObserverTest, RegisterObserver001, TestSize.Level1)
{
    /**
     * @tc.steps: step1/2. Register a null pointer to subscribe to the database.
     * Check whether the registration is successful.
     * @tc.expected: step1/2. Returns INVALID_ARGS.
     */
    int result;
    KvDBObserverHandle* handle = g_singleVerNaturaStoreConnection->RegisterObserver(
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT), g_entry1.key,
        nullptr, result);
    EXPECT_EQ(result, -E_INVALID_ARGS);
    EXPECT_EQ(handle, nullptr);

    /**
     * @tc.steps: step3/4. UnRegister a null pointer to subscribe to the database.
     * Check whether the unregistration is successful.
     * @tc.expected: step3/4. Returns INVALID_ARGS.
     */
    result = g_singleVerNaturaStoreConnection->UnRegisterObserver(nullptr);
    EXPECT_EQ(result, -E_INVALID_ARGS);
    return;
}

/**
  * @tc.name: RegisterObserver010
  * @tc.desc: Register an observer for the local sync database change and the local database change of a specified key.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: liujialei
  */
HWTEST_F(DistributedDBStorageRdRegisterObserverTest, RegisterObserver010, TestSize.Level1)
{
    /**
     * @tc.steps: step1/2. Register an observer for the local sync database change
     *  and the local database change of a specified key. Check register result.
     * @tc.expected: step1/2. Returns E_NOT_SUPPORT.
     */
    int result;
    KvDBObserverHandle* handle = g_singleVerNaturaStoreConnection->RegisterObserver(
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT) |
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_LOCAL_PUT_EVENT),
        g_entry1.key, TestFunc, result);
    EXPECT_EQ(result, -E_NOT_SUPPORT);
    EXPECT_EQ(handle, nullptr);
    return;
}

/**
  * @tc.name: RegisterObserver011
  * @tc.desc: Register an observer for the remote sync database change and the local database change of a specified key
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: liujialei
  */
HWTEST_F(DistributedDBStorageRdRegisterObserverTest, RegisterObserver011, TestSize.Level1)
{
    /**
     * @tc.steps: step1/2. Register an observer for the remote sync database change
     *  and the local database change of a specified key. Check register result.
     * @tc.expected: step1/2. Returns E_NOT_SUPPORT.
     */
    int result;
    KvDBObserverHandle* handle = g_singleVerNaturaStoreConnection->RegisterObserver(
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_SYNC_EVENT) |
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_LOCAL_PUT_EVENT),
        g_entry1.key, TestFunc, result);
    EXPECT_EQ(result, -E_NOT_SUPPORT);
    EXPECT_EQ(handle, nullptr);
    return;
}

/**
  * @tc.name: RegisterObserver012
  * @tc.desc: Register an observer for the local sync database change and the local database change of any key.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: liujialei
  */
HWTEST_F(DistributedDBStorageRdRegisterObserverTest, RegisterObserver012, TestSize.Level1)
{
    /**
     * @tc.steps: step1/2. Register an observer for the local sync database change
     * and the local database change of any key. Check register result.
     * @tc.expected: step1/2. Returns E_NOT_SUPPORT.
     */
    int result;
    KvDBObserverHandle* handle = g_singleVerNaturaStoreConnection->RegisterObserver(
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_PUT_EVENT) |
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_LOCAL_PUT_EVENT),
        g_emptyKey, TestFunc, result);
    EXPECT_EQ(result, -E_NOT_SUPPORT);
    EXPECT_EQ(handle, nullptr);
    return;
}

/**
  * @tc.name: RegisterObserver013
  * @tc.desc: Register an observer for the remote sync database change and the local database change of any key.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: liujialei
  */
HWTEST_F(DistributedDBStorageRdRegisterObserverTest, RegisterObserver013, TestSize.Level1)
{
    /**
     * @tc.steps: step1/2. Register an observer for the remote sync database change
     *  and the local database change of any key. Check register result.
     * @tc.expected: step1/2. Returns E_NOT_SUPPORT.
     */
    int result;
    KvDBObserverHandle* handle = g_singleVerNaturaStoreConnection->RegisterObserver(
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_SYNC_EVENT) |
        static_cast<unsigned int>(SQLiteGeneralNSNotificationEventType::SQLITE_GENERAL_NS_LOCAL_PUT_EVENT),
        g_emptyKey, TestFunc, result);
    EXPECT_EQ(result, -E_NOT_SUPPORT);
    EXPECT_EQ(handle, nullptr);
    return;
}
#endif // USE_RD_KERNEL