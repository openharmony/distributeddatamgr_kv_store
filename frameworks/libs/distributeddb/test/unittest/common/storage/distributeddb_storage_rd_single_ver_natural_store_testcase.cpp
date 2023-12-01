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
#include "distributeddb_storage_rd_single_ver_natural_store_testcase.h"

#include "generic_single_ver_kv_entry.h"
#include "runtime_context.h"
#include "time_helper.h"

using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    const int MAX_TEST_KEY_SIZE = 1024;     // 1K
    const int MAX_TEST_VAL_SIZE = 4194304; // 4M
}

/**
  * @tc.name: SyncDatabaseOperate001
  * @tc.desc: To test the function of inserting data of the local device in the synchronization database.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: huangboxin
  */
void DistributedDBStorageRdSingleVerNaturalStoreTestCase::SyncDatabaseOperate001(RdSingleVerNaturalStore *&store,
    RdSingleVerNaturalStoreConnection *&connection)
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    DataBaseCommonPutOperate(store, connection, option);
}

/**
  * @tc.name: SyncDatabaseOperate003
  * @tc.desc: test the delete operation in sync database.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: huangboxin
  */
void DistributedDBStorageRdSingleVerNaturalStoreTestCase::SyncDatabaseOperate003(RdSingleVerNaturalStore *&store,
    RdSingleVerNaturalStoreConnection *&connection)
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    DataBaseCommonDeleteOperate(store, connection, option);
}

/**
  * @tc.name: SyncDatabaseOperate005
  * @tc.desc: test the reading for sync database.
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: huangboxin
  */
void DistributedDBStorageRdSingleVerNaturalStoreTestCase::SyncDatabaseOperate005(RdSingleVerNaturalStore *&store,
    RdSingleVerNaturalStoreConnection *&connection)
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    DataBaseCommonGetOperate(store, connection, option);
}

/**
  * @tc.name: SyncDatabaseOperate006
  * @tc.desc: test the get entries for sync database
  * @tc.type: FUNC
  * @tc.require: AR000CCPOM
  * @tc.author: huangboxin
  */
void DistributedDBStorageRdSingleVerNaturalStoreTestCase::SyncDatabaseOperate006(RdSingleVerNaturalStore *&store,
    RdSingleVerNaturalStoreConnection *&connection)
{
    IOption option;
    option.dataType = IOption::SYNC_DATA;
    Key key1, key2, key3;
    Value value1, value2, value3;

    /**
     * @tc.steps: step2/3/4. Set Ioption to synchronous data.
     * Insert the data of key=keyPrefix + 'a', value1.
     * Insert the data of key=keyPrefix + 'c', value2.
     * Insert the data of key length=keyPrefix length - 1, value3.
     * @tc.expected: step2/3/4. Return E_NOT_FOUND.
     */
    DistributedDBToolsUnitTest::GetRandomKeyValue(key1, 30); // 30 as random size
    key3 = key2 = key1;
    key2.push_back('C');
    key3.pop_back();
    DistributedDBToolsUnitTest::GetRandomKeyValue(value1, 84); // 84 as random size
    DistributedDBToolsUnitTest::GetRandomKeyValue(value2, 101); // 101 as random size
    DistributedDBToolsUnitTest::GetRandomKeyValue(value3, 37); // 37 as random size
    EXPECT_EQ(connection->Put(option, key1, value1), E_OK);
    EXPECT_EQ(connection->Put(option, key2, value2), E_OK);
    EXPECT_EQ(connection->Put(option, key3, value3), E_OK);

    /**
     * @tc.steps: step5. Obtain all data whose prefixKey is keyPrefix.
     * @tc.expected: step5. Return OK. The number of obtained data records is 2.
     */
    std::vector<Entry> entriesRead;
    EXPECT_EQ(connection->GetEntries(option, key1, entriesRead), E_OK);
    EXPECT_EQ(entriesRead.size(), 2UL);

    /**
     * @tc.steps: step6. Obtain all data whose prefixKey is empty.
     * @tc.expected: step6. Return OK. The number of obtained data records is 3.
     */
    entriesRead.clear();
    Key emptyKey;
    EXPECT_EQ(connection->GetEntries(option, emptyKey, entriesRead), E_OK);
    EXPECT_EQ(entriesRead.size(), 3UL);

    /**
     * @tc.steps: step7. Obtain all data whose prefixKey is keyPrefix.
     * @tc.expected: step7. Return -E_NOT_SUPPORT.
     */
    option.dataType = IOption::LOCAL_DATA;
    EXPECT_EQ(connection->GetEntries(option, emptyKey, entriesRead), -E_NOT_SUPPORT);
}

// @Real query sync-DATA table by key, judge is exist.
bool DistributedDBStorageRdSingleVerNaturalStoreTestCase::IsSqlinteExistKey(const std::vector<SyncData> &vecSyncData,
    const std::vector<uint8_t> &key)
{
    for (const auto &iter : vecSyncData) {
        if (key == iter.key) {
            return true;
        }
    }
    return false;
}

void DistributedDBStorageRdSingleVerNaturalStoreTestCase::DataBaseCommonPutOperate(RdSingleVerNaturalStore *&store,
    RdSingleVerNaturalStoreConnection *&connection, IOption option)
{
    Key key1;
    Value value1;

    /**
     * @tc.steps: step1/2. Set Ioption to the local data and insert a record of key1 and value1.
     * @tc.expected: step1/2. Return OK.
     */
    DistributedDBToolsUnitTest::GetRandomKeyValue(key1);
    DistributedDBToolsUnitTest::GetRandomKeyValue(value1);
    EXPECT_EQ(connection->Put(option, key1, value1), E_OK);

    /**
     * @tc.steps: step3. Set Ioption to the local data and obtain the value of key1.
     *  Check whether the value is the same as the value of value1.
     * @tc.expected: step3. The obtained value and value2 are the same.
     */
    Value valueRead;
    EXPECT_EQ(connection->Get(option, key1, valueRead), E_OK);
    EXPECT_EQ(DistributedDBToolsUnitTest::IsValueEqual(valueRead, value1), true);
    Value value2;
    DistributedDBToolsUnitTest::GetRandomKeyValue(value2, static_cast<int>(value1.size() + 3)); // 3 more for diff

    /**
     * @tc.steps: step4. Ioption Set this parameter to the local data. Insert key1.
     *  The value cannot be empty. value2(!=value1)
     * @tc.expected: step4. Return OK.
     */
    EXPECT_EQ(connection->Put(option, key1, value2), E_OK);

    /**
     * @tc.steps: step5. Set Ioption to the local data, GetMetaData to obtain the value of key1,
     *  and check whether the value is the same as the value of value2.
     * @tc.expected: step5. The obtained value and value2 are the same.
     */
    EXPECT_EQ(connection->Get(option, key1, valueRead), E_OK);
    EXPECT_EQ(DistributedDBToolsUnitTest::IsValueEqual(valueRead, value2), true);

    /**
     * @tc.steps: step6. The Ioption parameter is set to the local data.
     *  The data record whose key is empty and value is not empty is inserted.
     * @tc.expected: step6. Return E_INVALID_DATA.
     */
    Key emptyKey;
    Value emptyValue;
    EXPECT_EQ(connection->Put(option, emptyKey, value1), -E_INVALID_ARGS);

    /**
     * @tc.steps: step7. Set Ioption to the local data, insert data
     *  whose key2(!=key1) is not empty, and value is empty.
     * @tc.expected: step7. Return E_OK
     */
    Key key2;
    DistributedDBToolsUnitTest::GetRandomKeyValue(key2, static_cast<int>(key1.size() + 1));
    EXPECT_EQ(connection->Put(option, key2, emptyValue), E_OK);

    /**
     * @tc.steps: step8. Set option to local data, obtain the value of key2,
     *  and check whether the value is empty.
     * @tc.expected: step8. Return E_OK.
     */
    EXPECT_EQ(connection->Get(option, key2, valueRead), E_OK);

    /**
     * @tc.steps: step9. Ioption Set the local data.
     *  Insert the data whose key size is 1024 and value size is 4Mb + 1.
     * @tc.expected: step9. Return OK.
     */
    Key sizeKey;
    Value sizeValue;
    DistributedDBToolsUnitTest::GetRandomKeyValue(sizeKey, MAX_TEST_KEY_SIZE);
    DistributedDBToolsUnitTest::GetRandomKeyValue(sizeValue, MAX_TEST_VAL_SIZE + 1);
    EXPECT_EQ(connection->Put(option, sizeKey, sizeValue), -E_INVALID_ARGS);
    EXPECT_EQ(connection->Get(option, sizeKey, valueRead), -E_NOT_FOUND);

    /**
     * @tc.steps: step10/11. Set Ioption to the local data and insert data items
     *  whose value is greater than 4Mb or key is bigger than 1Kb
     * @tc.expected: step10/11. Return E_INVALID_ARGS.
     */
    sizeKey.push_back(std::rand()); // random size
    EXPECT_EQ(connection->Put(option, sizeKey, sizeValue), -E_INVALID_ARGS);
    sizeKey.pop_back();
    sizeValue.push_back(174); // 174 as random size
    EXPECT_EQ(connection->Put(option, sizeKey, sizeValue), -E_INVALID_ARGS);
}

void DistributedDBStorageRdSingleVerNaturalStoreTestCase::DataBaseCommonDeleteOperate(RdSingleVerNaturalStore *&store,
    RdSingleVerNaturalStoreConnection *&connection, IOption option)
{
    /**
     * @tc.steps: step2. Set Ioption to the local data and delete the data whose key is key1 (empty).
     * @tc.expected: step2. Return E_INVALID_ARGS.
     */
    Key key1;
    EXPECT_EQ(connection->Delete(option, key1), -E_INVALID_ARGS);
    DistributedDBToolsUnitTest::GetRandomKeyValue(key1, MAX_TEST_KEY_SIZE + 1);
    EXPECT_EQ(connection->Delete(option, key1), -E_INVALID_ARGS);
    DistributedDBToolsUnitTest::GetRandomKeyValue(key1);
    EXPECT_EQ(connection->Delete(option, key1), E_OK);

    /**
     * @tc.steps: step3. Set Ioption to the local data, insert non-null key1, and non-null value1 data.
     * @tc.expected: step3. Return E_OK.
     */
    Value value1;
    DistributedDBToolsUnitTest::GetRandomKeyValue(value1);
    EXPECT_EQ(connection->Put(option, key1, value1), E_OK);

    /**
     * @tc.steps: step4. Set Ioption to the local data, obtain the value of key1,
     *  and check whether the value is the same as that of value1.
     * @tc.expected: step4. Return E_OK. The obtained value is the same as the value of value1.
     */
    Value valueRead;
    EXPECT_EQ(connection->Get(option, key1, valueRead), E_OK);
    EXPECT_EQ(DistributedDBToolsUnitTest::IsValueEqual(valueRead, value1), true);

    /**
     * @tc.steps: step5. Set Ioption to the local data and delete the data whose key is key1.
     * @tc.expected: step5. Return E_OK.
     */
    EXPECT_EQ(connection->Delete(option, key1), E_OK);

    /**
     * @tc.steps: step5. Set Ioption to the local data and obtain the value of Key1.
     * @tc.expected: step5. Return E_NOT_FOUND.
     */
    EXPECT_EQ(connection->Get(option, key1, valueRead), -E_NOT_FOUND);
}

void DistributedDBStorageRdSingleVerNaturalStoreTestCase::DataBaseCommonGetOperate(RdSingleVerNaturalStore *&store,
    RdSingleVerNaturalStoreConnection *&connection, IOption option)
{
    /**
     * @tc.steps: step2. Set Ioption to the local data and delete the data whose key is key1 (empty).
     * @tc.expected: step2. Return E_INVALID_ARGS.
     */
    Key key1;
    Value valueRead;
    // empty key
    EXPECT_EQ(connection->Get(option, key1, valueRead), -E_INVALID_ARGS);

    // invalid key
    DistributedDBToolsUnitTest::GetRandomKeyValue(key1, MAX_TEST_KEY_SIZE + 1);
    EXPECT_EQ(connection->Get(option, key1, valueRead), -E_INVALID_ARGS);

    // non-exist key
    DistributedDBToolsUnitTest::GetRandomKeyValue(key1, MAX_TEST_KEY_SIZE);
    EXPECT_EQ(connection->Get(option, key1, valueRead), -E_NOT_FOUND);

    /**
     * @tc.steps: step3. Set Ioption to the local data, insert non-null key1, and non-null value1 data.
     * @tc.expected: step3. Return E_OK.
     */
    Value value1;
    DistributedDBToolsUnitTest::GetRandomKeyValue(value1);
    EXPECT_EQ(connection->Put(option, key1, value1), E_OK);

    /**
     * @tc.steps: step4. Set Ioption to the local data, obtain the value of key1,
     *  and check whether the value is the same as that of value1.
     * @tc.expected: step4. Return E_OK. The obtained value is the same as the value of value1.
     */
    EXPECT_EQ(connection->Get(option, key1, valueRead), E_OK);
    EXPECT_EQ(DistributedDBToolsUnitTest::IsValueEqual(valueRead, value1), true);

    Key key2;
    DistributedDBToolsUnitTest::GetRandomKeyValue(key2);
    EXPECT_EQ(connection->Get(option, key2, valueRead), -E_NOT_FOUND);

    /**
     * @tc.steps: step5. Set Ioption to the local data and obtain the value data of Key1.
     *  Check whether the value is the same as the value of value2.
     * @tc.expected: step4. Return E_OK, and the value is the same as the value of value2.
     */
    Value value2;
    DistributedDBToolsUnitTest::GetRandomKeyValue(value2, value1.size() + 1);
    EXPECT_EQ(connection->Put(option, key1, value2), E_OK);

    /**
     * @tc.steps: step5. The Ioption is set to the local.
     *  The data of the key1 and value2(!=value1) is inserted.
     * @tc.expected: step4. Return E_OK.
     */
    EXPECT_EQ(connection->Get(option, key1, valueRead), E_OK);
    EXPECT_EQ(DistributedDBToolsUnitTest::IsValueEqual(valueRead, value2), true);
}
#endif // USE_RD_KERNEL