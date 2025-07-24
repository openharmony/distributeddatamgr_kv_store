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

#include "rdb_general_ut.h"

namespace DistributedDB {
    using namespace testing::ext;
    using namespace DistributedDB;
    using namespace DistributedDBUnitTest;

class DistributedDBRDBMultiUserSyncTest : public RDBGeneralUt {
public:
    void SetUp() override;
protected:
    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    static constexpr const char *DEVICE_A = "DEVICE_A";
    static constexpr const char *DEVICE_B = "DEVICE_B";
    static constexpr const char *USER_ID_1 = "userId1";
    static constexpr const char *USER_ID_2 = "userId2";
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table, bool noPk = false);
    static void SetTargetUserId(const std::string &deviceId, const std::string &userId);
};

void DistributedDBRDBMultiUserSyncTest::SetUp()
{
    RDBGeneralUt::SetUp();
    RelationalStoreDelegate::Option option;
    option.syncDualTupleMode = true;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
}

UtDateBaseSchemaInfo DistributedDBRDBMultiUserSyncTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE));
    return info;
}

UtTableSchemaInfo DistributedDBRDBMultiUserSyncTest::GetTableSchema(const std::string &table, bool noPk)
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = table;
    UtFieldInfo fieldId;
    fieldId.field.colName = "id";
    fieldId.field.type = TYPE_INDEX<int64_t>;
    if (!noPk) {
        fieldId.field.primary = true;
    }
    tableSchema.fieldInfo.push_back(fieldId);
    UtFieldInfo fieldName;
    fieldName.field.colName = "name";
    fieldName.field.type = TYPE_INDEX<std::string>;
    tableSchema.fieldInfo.push_back(fieldName);
    return tableSchema;
}

void DistributedDBRDBMultiUserSyncTest::SetTargetUserId(const std::string &deviceId, const std::string &userId)
{
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    RuntimeContext::GetInstance()->GetCommunicatorAggregator(communicatorAggregator);
    ASSERT_NE(communicatorAggregator, nullptr);
    auto virtualCommunicatorAggregator = static_cast<VirtualCommunicatorAggregator*>(communicatorAggregator);
    auto communicator = static_cast<VirtualCommunicator*>(virtualCommunicatorAggregator->GetCommunicator(deviceId));
    ASSERT_NE(communicator, nullptr);
    communicator->SetTargetUserId(userId);
}

HWTEST_F(DistributedDBRDBMultiUserSyncTest, NormalSyncTest001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Prepare db and data.
     * @tc.expected: step1. Ok
     */
    StoreInfo deviceAStore1 = {USER_ID_1, APP_ID, STORE_ID_1};
    StoreInfo deviceAStore2 = {USER_ID_2, APP_ID, STORE_ID_2};
    StoreInfo deviceBStore = {USER_ID_1, APP_ID, STORE_ID_3};

    SetSchemaInfo(deviceAStore1, GetDefaultSchema());
    SetSchemaInfo(deviceAStore2, GetDefaultSchema());
    SetSchemaInfo(deviceBStore, GetDefaultSchema());

    ASSERT_EQ(BasicUnitTest::InitDelegate(deviceAStore1, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(deviceAStore2, DEVICE_A), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(deviceBStore, DEVICE_B), E_OK);

    ASSERT_EQ(SetDistributedTables(deviceAStore1, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(deviceAStore2, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(deviceBStore, {DEVICE_SYNC_TABLE}), E_OK);

    InsertLocalDBData(0, 1, deviceAStore1);
    InsertLocalDBData(0, 1, deviceAStore2);
    sqlite3 *dbA2 = GetSqliteHandle(deviceAStore2);
    std::string updateSql = "update " + std::string(DEVICE_SYNC_TABLE) + " set name = 'new_name' where id = 0;";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(dbA2, updateSql), E_OK);

    ASSERT_EQ(RDBGeneralUt::CloseDelegate(deviceAStore1), OK);
    ASSERT_EQ(RDBGeneralUt::CloseDelegate(deviceAStore2), OK);
    /**
     * @tc.steps: step2. Sync from db2 of deviceA to deviceB.
     * @tc.expected: step2. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(deviceAStore2, DEVICE_A), E_OK);
    SetTargetUserId(DEVICE_A, USER_ID_1);
    SetTargetUserId(DEVICE_B, USER_ID_2);
    BlockPush(deviceAStore2, deviceBStore, DEVICE_SYNC_TABLE);
    ASSERT_EQ(RDBGeneralUt::CloseDelegate(deviceAStore2), OK);
    /**
     * @tc.steps: step3. Sync from db1 of deviceA to deviceB.
     * @tc.expected: step3. Ok
     */
    ASSERT_EQ(BasicUnitTest::InitDelegate(deviceAStore1, DEVICE_A), E_OK);
    SetTargetUserId(DEVICE_A, USER_ID_1);
    SetTargetUserId(DEVICE_B, USER_ID_1);
    BlockPush(deviceAStore1, deviceBStore, DEVICE_SYNC_TABLE);
    ASSERT_EQ(RDBGeneralUt::CloseDelegate(deviceAStore1), OK);
    /**
     * @tc.steps: step4. Check data in deviceB.
     * @tc.expected: step4. Ok
     */
    std::string checkSql = "select count(*) from " + std::string(DEVICE_SYNC_TABLE) + " where name = 'new_name';";
    sqlite3 *dbB = GetSqliteHandle(deviceBStore);
    int actualCount = 0;
    ASSERT_EQ(SQLiteUtils::GetCountBySql(dbB, checkSql, actualCount), E_OK);
    EXPECT_EQ(actualCount, 1);
    ASSERT_EQ(RDBGeneralUt::CloseDelegate(deviceBStore), OK);
}
}