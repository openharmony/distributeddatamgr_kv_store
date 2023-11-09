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

#include "db_constant.h"
#include "distributeddb_storage_rd_single_ver_natural_store_testcase.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    DistributedDB::KvStoreConfig g_config;
    std::string g_testDir;
    const std::string MEM_URL = "file:31?mode=memory&cache=shared";
    DistributedDB::RdSingleVerNaturalStore *g_store = nullptr;
    DistributedDB::RdSingleVerNaturalStoreConnection *g_connection = nullptr;
}

class DistributedDBStorageMemoryRdSingleVerNaturalStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBStorageMemoryRdSingleVerNaturalStoreTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    // IDENTIFIER_DIR is 31
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/31/" + DBConstant::SINGLE_SUB_DIR);
}

void DistributedDBStorageMemoryRdSingleVerNaturalStoreTest::TearDownTestCase(void) {}

void DistributedDBStorageMemoryRdSingleVerNaturalStoreTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    KvDBProperties property;
    property.SetStringProp(KvDBProperties::DATA_DIR, g_testDir);
    property.SetStringProp(KvDBProperties::STORE_ID, "TestGeneralNB");
    property.SetStringProp(KvDBProperties::IDENTIFIER_DIR, "31");
    property.SetIntProp(KvDBProperties::DATABASE_TYPE, KvDBProperties::SINGLE_VER_TYPE_RD_KERNAL);
    g_store = new (std::nothrow) RdSingleVerNaturalStore;
    ASSERT_NE(g_store, nullptr);
    ASSERT_EQ(g_store->Open(property), E_OK);

    int erroCode = E_OK;
    g_connection = static_cast<RdSingleVerNaturalStoreConnection *>(g_store->GetDBConnection(erroCode));
    ASSERT_NE(g_connection, nullptr);
    g_store->DecObjRef(g_store);
    EXPECT_EQ(erroCode, E_OK);
}

void DistributedDBStorageMemoryRdSingleVerNaturalStoreTest::TearDown(void)
{
    if (g_connection != nullptr) {
        g_connection->Close();
    }

    g_store = nullptr;
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir + "/31/" + DBConstant::SINGLE_SUB_DIR);
}

/**
  * @tc.name: SyncDatabaseOperate001
  * @tc.desc: To test the function of inserting data of the local device in the synchronization database.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBStorageMemoryRdSingleVerNaturalStoreTest, SyncDatabaseOperate001, TestSize.Level1)
{
    /**
     * @tc.steps: step1/2. Set Ioption to the local data and insert a record of key1 and value1.
     * @tc.expected: step1/2. Return OK.
     */
    /**
     * @tc.steps: step3. Set Ioption to the local data and obtain the value of key1.
     *  Check whether the value is the same as the value of value1.
     * @tc.expected: step3. The obtained value and value2 are the same.
     */
    /**
     * @tc.steps: step4. Ioption Set this parameter to the local data. Insert key1.
     *  The value cannot be empty. value2(!=value1)
     * @tc.expected: step4. Return OK.
     */
    /**
     * @tc.steps: step5. Set Ioption to the local data, GetMetaData to obtain the value of key1,
     *  and check whether the value is the same as the value of value2.
     * @tc.expected: step5. The obtained value and value2 are the same.
     */
    /**
     * @tc.steps: step6. The Ioption parameter is set to the local data.
     *  The data record whose key is empty and value is not empty is inserted.
     * @tc.expected: step6. Return E_INVALID_DATA.
     */
    /**
     * @tc.steps: step7. Set Ioption to the local data, insert data
     *  whose key2(!=key1) is not empty, and value is empty.
     * @tc.expected: step7. Return OK.
     */
    /**
     * @tc.steps: step8. Set option to local data, obtain the value of key2,
     *  and check whether the value is empty.
     * @tc.expected: step8. Return OK, value is empty.
     */
    /**
     * @tc.steps: step9. Ioption Set the local data.
     *  Insert the data whose key size is 1024 and value size is 4Mb.
     * @tc.expected: step9. Return OK.
     */
    /**
     * @tc.steps: step10/11. Set Ioption to the local data and insert data items
     *  whose value is greater than 4Mb or key is bigger than 1Kb
     * @tc.expected: step10/11. Return E_INVALID_ARGS.
     */
    DistributedDBStorageRdSingleVerNaturalStoreTestCase::SyncDatabaseOperate001(g_store, g_connection);
}

/**
  * @tc.name: SyncDatabaseOperate003
  * @tc.desc: test the delete operation in sync database.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBStorageMemoryRdSingleVerNaturalStoreTest, SyncDatabaseOperate003, TestSize.Level1)
{
    /**
     * @tc.steps: step2. Set Ioption to the local data and delete the data whose key is key1 (empty).
     * @tc.expected: step2. Return E_INVALID_ARGS.
     */
    /**
     * @tc.steps: step3. Set Ioption to the local data, insert non-null key1, and non-null value1 data.
     * @tc.expected: step3. Return E_OK.
     */
    /**
     * @tc.steps: step4. Set Ioption to the local data, obtain the value of key1,
     *  and check whether the value is the same as that of value1.
     * @tc.expected: step4. Return E_OK. The obtained value is the same as the value of value1.
     */
    /**
     * @tc.steps: step5. Set Ioption to the local data and delete the data whose key is key1.
     * @tc.expected: step5. Return E_OK.
     */
    /**
     * @tc.steps: step5. Set Ioption to the local data and obtain the value of Key1.
     * @tc.expected: step5. Return E_NOT_FOUND.
     */
    DistributedDBStorageRdSingleVerNaturalStoreTestCase::SyncDatabaseOperate003(g_store, g_connection);
}

/**
  * @tc.name: SyncDatabaseOperate005
  * @tc.desc: test the reading for sync database.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBStorageMemoryRdSingleVerNaturalStoreTest, SyncDatabaseOperate005, TestSize.Level1)
{
    /**
     * @tc.steps: step2. Set Ioption to the local data and delete the data whose key is key1 (empty).
     * @tc.expected: step2. Return E_INVALID_ARGS.
     */
    /**
     * @tc.steps: step3. Set Ioption to the local data, insert non-null key1, and non-null value1 data.
     * @tc.expected: step3. Return E_OK.
     */
    /**
     * @tc.steps: step4. Set Ioption to the local data, obtain the value of key1,
     *  and check whether the value is the same as that of value1.
     * @tc.expected: step4. Return E_OK. The obtained value is the same as the value of value1.
     */
    /**
     * @tc.steps: step5. Set Ioption to the local data and obtain the value data of Key1.
     *  Check whether the value is the same as the value of value2.
     * @tc.expected: step4. Return E_OK, and the value is the same as the value of value2.
     */
    /**
     * @tc.steps: step5. The Ioption is set to the local.
     *  The data of the key1 and value2(!=value1) is inserted.
     * @tc.expected: step4. Return E_OK.
     */
    DistributedDBStorageRdSingleVerNaturalStoreTestCase::SyncDatabaseOperate005(g_store, g_connection);
}

/**
  * @tc.name: SyncDatabaseOperate006
  * @tc.desc: test the get entries for sync database
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: wangbingquan
  */
HWTEST_F(DistributedDBStorageMemoryRdSingleVerNaturalStoreTest, SyncDatabaseOperate006, TestSize.Level1)
{
    /**
     * @tc.steps: step2/3/4. Set Ioption to synchronous data.
     * Insert the data of key=keyPrefix + 'a', value1.
     * Insert the data of key=keyPrefix + 'c', value2.
     * Insert the data of key length=keyPrefix length - 1, value3.
     * @tc.expected: step2/3/4. Return E_NOT_FOUND.
     */
    /**
     * @tc.steps: step5. Obtain all data whose prefixKey is keyPrefix.
     * @tc.expected: step5. Return OK. The number of obtained data records is 2.
     */
    /**
     * @tc.steps: step6. Obtain all data whose prefixKey is empty.
     * @tc.expected: step6. Return OK. The number of obtained data records is 3.
     */
    /**
     * @tc.steps: step7. Obtain all data whose prefixKey is keyPrefix.
     * @tc.expected: step7. Return E_NOT_SUPPORT.
     */
    DistributedDBStorageRdSingleVerNaturalStoreTestCase::SyncDatabaseOperate006(g_store, g_connection);
}
#endif // USE_RD_KERNEL