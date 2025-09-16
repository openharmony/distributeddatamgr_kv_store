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

#include "kv_general_ut.h"

namespace DistributedDB {
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

class DistributedDBKVDataStatusTest : public KVGeneralUt {
public:
    void SetUp() override;
protected:
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    static constexpr const char *DEVICE_C = "DEVICE_C";
    static const uint32_t INVALID_DATA_OPERATOR = 0;
};

void DistributedDBKVDataStatusTest::SetUp()
{
    KVGeneralUt::SetUp();
    auto storeInfo1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_A), E_OK);
    auto storeInfo2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo2, DEVICE_B), E_OK);
}

/**
 * @tc.name: OperateDataStatus001
 * @tc.desc: Test sync from dev1 to dev2 after operate valid data status.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBKVDataStatusTest, OperateDataStatus001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync to dev2
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockPush(storeInfo1, storeInfo2);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. dev1 modify deviceId and operate valid data status
     * @tc.expected: step2. OK.
     */
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_C), E_OK);
    store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::UPDATE_TIME) |
                                        static_cast<uint32_t>(DataOperator::RESET_UPLOAD_CLOUD)), OK);
    /**
     * @tc.steps: step3. dev1 sync to dev2
     * @tc.expected: step3. sync should return OK and dev2 exist (k,v).
     */
    BlockPush(storeInfo1, storeInfo2);
    std::vector<Entry> entries;
    EXPECT_EQ(KVGeneralUt::GetDeviceEntries(store2, std::string(DEVICE_C), false, entries), OK);
    EXPECT_EQ(entries.size(), 1u); // 1 record
    EXPECT_EQ(entries[0].value, expectValue);
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}

/**
 * @tc.name: OperateDataStatus002
 * @tc.desc: Test sync from dev1 to dev2 after operate invalid data status.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBKVDataStatusTest, OperateDataStatus002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync to dev2
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockPush(storeInfo1, storeInfo2);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. dev1 modify deviceId and operate invalid data status
     * @tc.expected: step2. OK.
     */
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_C), E_OK);
    store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1->OperateDataStatus(INVALID_DATA_OPERATOR), OK);
    /**
     * @tc.steps: step3. dev1 sync to dev2
     * @tc.expected: step3. sync should return OK and dev2 get entries by device return NOT_FOUND.
     */
    BlockPush(storeInfo1, storeInfo2);
    std::vector<Entry> entries;
    EXPECT_EQ(KVGeneralUt::GetDeviceEntries(store2, std::string(DEVICE_C), false, entries), NOT_FOUND);
    EXPECT_EQ(entries.size(), 0u); // 0 record
}

/**
 * @tc.name: OperateDataStatus003
 * @tc.desc: Test sync from dev1 to dev2 after operate data status RESET_UPLOAD_CLOUD.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBKVDataStatusTest, OperateDataStatus003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync to dev2
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockPush(storeInfo1, storeInfo2);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. dev1 modify deviceId and operate data status RESET_UPLOAD_CLOUD
     * @tc.expected: step2. OK.
     */
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_C), E_OK);
    store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::RESET_UPLOAD_CLOUD)), OK);
    /**
     * @tc.steps: step3. dev1 sync to dev2
     * @tc.expected: step3. sync should return OK and dev2 get entries by device return NOT_FOUND.
     */
    BlockPush(storeInfo1, storeInfo2);
    std::vector<Entry> entries;
    EXPECT_EQ(KVGeneralUt::GetDeviceEntries(store2, std::string(DEVICE_C), false, entries), NOT_FOUND);
    EXPECT_EQ(entries.size(), 0u); // 0 record
}

/**
 * @tc.name: OperateDataStatus004
 * @tc.desc: Test sync with delete data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVDataStatusTest, OperateDataStatus004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync to dev2
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockPush(storeInfo1, storeInfo2);
    EXPECT_EQ(store1->Delete({'k'}), OK);
    BlockPush(storeInfo1, storeInfo2);
    /**
     * @tc.steps: step2. dev1 modify deviceId and operate data status RESET_UPLOAD_CLOUD
     * @tc.expected: step2. OK.
     */
    ASSERT_EQ(KVGeneralUt::CloseDelegate(storeInfo1), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo1, DEVICE_C), E_OK);
    store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::RESET_UPLOAD_CLOUD)), OK);
    /**
     * @tc.steps: step3. dev1 sync to dev2
     * @tc.expected: step3. sync should return OK and dev2 get entries by device return NOT_FOUND.
     */
    BlockPush(storeInfo1, storeInfo2);
    std::vector<Entry> entries;
    EXPECT_EQ(KVGeneralUt::GetDeviceEntries(store2, std::string(DEVICE_C), false, entries), NOT_FOUND);
    EXPECT_EQ(entries.size(), 0u); // 0 record
}

/**
 * @tc.name: OperateDataStatus005
 * @tc.desc: Test operate data status args.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVDataStatusTest, OperateDataStatus005, TestSize.Level0)
{
    auto store1 = GetDelegate(GetStoreInfo1());
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::UPDATE_TIME)), OK);
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::RESET_UPLOAD_CLOUD)), OK);
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::UPDATE_TIME) |
        static_cast<uint32_t>(DataOperator::RESET_UPLOAD_CLOUD)), OK);
    EXPECT_EQ(store1->OperateDataStatus(0), OK);
}

/**
 * @tc.name: OperateDataStatus006
 * @tc.desc: Test operate data status invalid args.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVDataStatusTest, OperateDataStatus006, TestSize.Level0)
{
    SQLiteSingleVerNaturalStore store;
    EXPECT_EQ(store.OperateDataStatus(static_cast<uint32_t>(DataOperator::UPDATE_TIME)), -E_INVALID_DB);
    SQLiteSingleVerStorageExecutor handle(nullptr, false, false);
    EXPECT_EQ(SQLiteSingleVerNaturalStore::OperateDataStatus(&handle, "", "", 0), -E_NOT_FOUND);
}

/**
 * @tc.name: OperateDataStatus007
 * @tc.desc: Test operate data status with empty db.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVDataStatusTest, OperateDataStatus007, TestSize.Level0)
{
    auto path = GetTestDir() + "/OperateDataStatus007.db";
    sqlite3 *db = RelationalTestUtils::CreateDataBase(path);
    ASSERT_NE(db, nullptr);
    std::string sql = std::string("CREATE TABLE IF NOT EXISTS ") + DBConstant::KV_SYNC_TABLE_NAME +
        "(key int primary key)";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    sql = "CREATE TABLE IF NOT EXISTS naturalbase_kv_aux_sync_data_log(key int primary key)";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    SQLiteSingleVerStorageExecutor handle(db, false, false);
    auto errCode = SQLiteSingleVerNaturalStore::OperateDataStatus(&handle, "", "",
        static_cast<uint32_t>(DataOperator::UPDATE_TIME));
    EXPECT_EQ(errCode, -1); // -1 is inner error
    errCode = SQLiteSingleVerNaturalStore::OperateDataStatus(&handle, "", "",
        static_cast<uint32_t>(DataOperator::RESET_UPLOAD_CLOUD));
    EXPECT_EQ(errCode, -1); // -1 is inner error
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
 * @tc.name: OperateDataStatus008
 * @tc.desc: Test operate data status when db was error.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBKVDataStatusTest, OperateDataStatus008, TestSize.Level0)
{
    auto info1 = GetStoreInfo1();
    auto store1 = GetDelegate(info1);
    ASSERT_NE(store1, nullptr);
    std::string dir;
    ASSERT_EQ(KvStoreDelegateManager::GetDatabaseDir(info1.storeId, info1.appId, info1.userId, dir), OK);
    auto path = GetTestDir() + "/" + dir + "/single_ver/main/gen_natural_store.db";
    sqlite3 *db = RelationalTestUtils::CreateDataBase(path);
    ASSERT_NE(db, nullptr);
    std::string sql = "DROP TABLE IF EXISTS sync_data";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    sql = "CREATE TABLE IF NOT EXISTS sync_data(key int primary key)";
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::UPDATE_TIME)), DB_ERROR);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

#ifdef USE_DISTRIBUTEDDB_CLOUD
/**
 * @tc.name: CloudOperateDataStatus001
 * @tc.desc: Test sync cloud after operate valid data status.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBKVDataStatusTest, CloudOperateDataStatus001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync cloud and dev2 sync cloud
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    ASSERT_EQ(SetCloud(store1), OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    ASSERT_EQ(SetCloud(store2), OK);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockCloudSync(storeInfo1, DEVICE_A);
    BlockCloudSync(storeInfo2, DEVICE_B);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. dev1 operate valid data status
     * @tc.expected: step2. OK.
     */
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::UPDATE_TIME) |
                                        static_cast<uint32_t>(DataOperator::RESET_UPLOAD_CLOUD)), OK);
    /**
     * @tc.steps: step3. dev1 modify deviceId and sync cloud and dev2 sync cloud
     * @tc.expected: step3. sync should return OK and dev2 exist (k,v).
     */
    BlockCloudSync(storeInfo1, DEVICE_C);
    BlockCloudSync(storeInfo2, DEVICE_B);
    std::vector<Entry> entries;
    EXPECT_EQ(KVGeneralUt::GetDeviceEntries(store2, std::string(DEVICE_C), false, entries), OK);
    EXPECT_EQ(entries.size(), 1u); // 1 record
    EXPECT_EQ(entries[0].value, expectValue);
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}

/**
 * @tc.name: CloudOperateDataStatus002
 * @tc.desc: Test sync cloud after operate invalid data status.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBKVDataStatusTest, CloudOperateDataStatus002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync cloud and dev2 sync cloud
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    ASSERT_EQ(SetCloud(store1), OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    ASSERT_EQ(SetCloud(store2), OK);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockCloudSync(storeInfo1, DEVICE_A);
    BlockCloudSync(storeInfo2, DEVICE_B);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. dev1 operate invalid data status
     * @tc.expected: step2. OK.
     */
    EXPECT_EQ(store1->OperateDataStatus(INVALID_DATA_OPERATOR), OK);
    /**
     * @tc.steps: step3. dev1 modify deviceId and sync cloud and dev2 sync cloud
     * @tc.expected: step3. sync should return OK and dev2 get entries by device return NOT_FOUND.
     */
    BlockCloudSync(storeInfo1, DEVICE_C);
    BlockCloudSync(storeInfo2, DEVICE_B);
    std::vector<Entry> entries;
    EXPECT_EQ(KVGeneralUt::GetDeviceEntries(store2, std::string(DEVICE_C), false, entries), NOT_FOUND);
    EXPECT_EQ(entries.size(), 0u); // 0 record
}

/**
 * @tc.name: CloudOperateDataStatus003
 * @tc.desc: Test sync cloud after operate data status RESET_UPLOAD_CLOUD.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caihaoting
 */
HWTEST_F(DistributedDBKVDataStatusTest, CloudOperateDataStatus003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 put (k,v) and sync cloud and dev2 sync cloud
     * @tc.expected: step1. sync should return OK and dev2 exist (k,v).
     */
    auto storeInfo1 = GetStoreInfo1();
    auto storeInfo2 = GetStoreInfo2();
    auto store1 = GetDelegate(storeInfo1);
    ASSERT_NE(store1, nullptr);
    ASSERT_EQ(SetCloud(store1), OK);
    auto store2 = GetDelegate(storeInfo2);
    ASSERT_NE(store2, nullptr);
    ASSERT_EQ(SetCloud(store2), OK);
    Value expectValue = {'v'};
    EXPECT_EQ(store1->Put({'k'}, expectValue), OK);
    BlockCloudSync(storeInfo1, DEVICE_A);
    BlockCloudSync(storeInfo2, DEVICE_B);
    Value actualValue;
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
    /**
     * @tc.steps: step2. dev1 operate data status RESET_UPLOAD_CLOUD
     * @tc.expected: step2. OK.
     */
    EXPECT_EQ(store1->OperateDataStatus(static_cast<uint32_t>(DataOperator::RESET_UPLOAD_CLOUD)), OK);
    /**
     * @tc.steps: step3. dev1 modify deviceId and sync cloud and dev2 sync cloud
     * @tc.expected: step3. sync should return OK and dev2 exist (k,v).
     */
    BlockCloudSync(storeInfo1, DEVICE_C);
    BlockCloudSync(storeInfo2, DEVICE_B);
    std::vector<Entry> entries;
    EXPECT_EQ(KVGeneralUt::GetDeviceEntries(store2, std::string(DEVICE_C), false, entries), OK);
    EXPECT_EQ(entries.size(), 1u); // 1 record
    EXPECT_EQ(entries[0].value, expectValue);
    EXPECT_EQ(store2->Get({'k'}, actualValue), OK);
    EXPECT_EQ(actualValue, expectValue);
}
#endif // USE_DISTRIBUTEDDB_CLOUD
} // namespace DistributedDB