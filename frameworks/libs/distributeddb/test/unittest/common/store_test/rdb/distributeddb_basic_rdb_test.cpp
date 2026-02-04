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
#include "sqlite_relational_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
const std::string g_deviceA = "dev1";
const std::string g_deviceB = "dev2";
const std::string g_deviceC = "dev3";

class DistributedDBBasicRDBTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table, bool noPk = false);
    void PrepareRemoveDataStore(StoreInfo &info1, StoreInfo &info2, StoreInfo &info3, int count);
protected:
    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    static constexpr const char *CLOUD_SYNC_TABLE = "CLOUD_SYNC_TABLE";
};

void DistributedDBBasicRDBTest::SetUp()
{
    RDBGeneralUt::SetUp();
}

void DistributedDBBasicRDBTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

UtDateBaseSchemaInfo DistributedDBBasicRDBTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE));
    return info;
}

UtTableSchemaInfo DistributedDBBasicRDBTest::GetTableSchema(const std::string &table, bool noPk)
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = table;
    UtFieldInfo field;
    field.field.colName = "id";
    field.field.type = TYPE_INDEX<int64_t>;
    if (!noPk) {
        field.field.primary = true;
    }
    tableSchema.fieldInfo.push_back(field);
    return tableSchema;
}

void DistributedDBBasicRDBTest::PrepareRemoveDataStore(StoreInfo &info1, StoreInfo &info2, StoreInfo &info3, int count)
{
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info3, g_deviceC), E_OK);
    /**
    * @tc.steps: step1. dev1 insert data
    * @tc.expected: step1. Ok
    */
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    InsertLocalDBData(count, 0, info3);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);

    /**
    * @tc.steps: step2. create distributed tables and sync to dev2
    * @tc.expected: step2. Ok
    */
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info3, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId("dev1");
    BlockPush(info1, info2, g_defaultTable1);
    BasicUnitTest::SetLocalDeviceId("dev3");
    BlockPush(info3, info2, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count*2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
}

/**
 * @tc.name: InitDelegateExample001
 * @tc.desc: Test InitDelegate interface of RDBGeneralUt.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBBasicRDBTest, InitDelegateExample001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Call InitDelegate interface with default data.
     * @tc.expected: step1. Ok
     */
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    DataBaseSchema actualSchemaInfo = RDBGeneralUt::GetSchema(info1);
    ASSERT_EQ(actualSchemaInfo.tables.size(), 2u);
    EXPECT_EQ(actualSchemaInfo.tables[0].name, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CloseDelegate(info1), E_OK);

    /**
     * @tc.steps: step2. Call twice InitDelegate interface with the set data.
     * @tc.expected: step2. Ok
     */
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, false}, true}, {{"name", TYPE_INDEX<std::string>, false, true}, false},
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {{.name = DEVICE_SYNC_TABLE, .fieldInfo = filedInfo}}
    };
    RDBGeneralUt::SetSchemaInfo(info1, schemaInfo);
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);

    StoreInfo info2 = {USER_ID, APP_ID, STORE_ID_2};
    schemaInfo = {
        .tablesInfo = {
            {.name = DEVICE_SYNC_TABLE, .fieldInfo = filedInfo},
            {.name = CLOUD_SYNC_TABLE, .fieldInfo = filedInfo},
        }
    };
    RDBGeneralUt::SetSchemaInfo(info2, schemaInfo);
    EXPECT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    actualSchemaInfo = RDBGeneralUt::GetSchema(info2);
    ASSERT_EQ(actualSchemaInfo.tables.size(), schemaInfo.tablesInfo.size());
    EXPECT_EQ(actualSchemaInfo.tables[1].name, CLOUD_SYNC_TABLE);
    TableSchema actualTableInfo = RDBGeneralUt::GetTableSchema(info2, CLOUD_SYNC_TABLE);
    EXPECT_EQ(actualTableInfo.fields.size(), filedInfo.size());
}

#ifdef USE_DISTRIBUTEDDB_DEVICE
/**
 * @tc.name: RdbSyncExample001
 * @tc.desc: Test insert data and sync from dev1 to dev2.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbSyncExample001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. dev1 insert data.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), 0);

    /**
     * @tc.steps: step2. create distributed tables and sync to dev1.
     * @tc.expected: step2. Ok
     */
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BlockPush(info1, info2, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), 2);

    /**
     * @tc.steps: step3. update name and sync to dev1.
     * @tc.expected: step3. Ok
     */
    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='update'";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    ASSERT_NO_FATAL_FAILURE(BlockPush(info1, info2, g_defaultTable1));
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1, "name='update'"), 2);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest001
 * @tc.desc: Local clean should NOT delete data for other device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbRemoveDataForOtherDevicesTest001, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    auto info2 = GetStoreInfo2(); // dev2 as B
    auto info3 = GetStoreInfo3(); // dev3 as C
    /**
     * @tc.steps: step1.  prepare remove info and data
     * @tc.expected: step1. Ok
     */
    int count = 2;
    PrepareRemoveDataStore(info1, info2, info3, count);

    /**
     * @tc.steps: step2. A local clean without sync-delete
     * @tc.expected: step2. Ok
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice", g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), 0);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);

    /**
     * @tc.steps: step3. Trigger another push sync C->B, B should keep data
     * @tc.expected: step3. Ok
     */
    BlockPush(info2, info1, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest002
 * @tc.desc: Local clean should NOT affect sync func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbRemoveDataForOtherDevicesTest002, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    auto info2 = GetStoreInfo2(); // dev2 as B
    auto info3 = GetStoreInfo3(); // dev3 as C
    /**
     * @tc.steps: step1.  prepare remove info and data
     * @tc.expected: step1. Ok
     */
    int count = 2;
    PrepareRemoveDataStore(info1, info2, info3, count);

    /**
     * @tc.steps: step2. A local clean without sync-delete
     * @tc.expected: step2. Ok
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice", g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), 0);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);

    /**
     * @tc.steps: step3. Trigger another push sync C->B, B should keep data
     * @tc.expected: step3. Ok
     */
    InsertLocalDBData(0, 2, info2);
    BlockPush(info2, info1, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info1, DBCommon::GetLogTableName(g_defaultTable1), g_deviceB), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest003
 * @tc.desc: Local clean with mulit table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbRemoveDataForOtherDevicesTest003, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    auto info2 = GetStoreInfo2(); // dev2 as B
    auto info3 = GetStoreInfo3(); // dev3 as C
    /**
     * @tc.steps: step1.  prepare remove info and data
     * @tc.expected: step1. Ok
     */
    int count = 2;
    PrepareRemoveDataStore(info1, info2, info3, count);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable2}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable2}), E_OK);
    BlockPush(info1, info2, g_defaultTable2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable2), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable2), count);
    /**
     * @tc.steps: step2. A local clean without sync-delete
     * @tc.expected: step2. Ok
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice", g_deviceC}}, {g_defaultTable2, {"localDevice", g_deviceA}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), 0);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable2), g_deviceA), count);

    /**
     * @tc.steps: step3. Trigger another push sync C->B, B should keep data
     * @tc.expected: step3. Ok
     */
    InsertLocalDBData(0, 2, info2);
    BlockPush(info2, info1, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info1, DBCommon::GetLogTableName(g_defaultTable1), g_deviceB), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest004
 * @tc.desc: Local clean with invalid args
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbRemoveDataForOtherDevicesTest004, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    auto info2 = GetStoreInfo2(); // dev2 as B
    /**
     * @tc.steps: step1.  prepare remove info and data
     * @tc.expected: step1. Ok
     */
    int count = 2;
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice", g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), NOT_SUPPORT);

    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId("dev1");
    BlockPush(info1, info2, g_defaultTable1);

    /**
     * @tc.steps: step2. A local clean with invalid args
     */
    BasicUnitTest::SetLocalDeviceId("localDevice");
    clearMap = {{g_defaultTable1 + "%log", {"localDevice", g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), INVALID_ARGS);
    clearMap = {{"", {"localDevice", g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), INVALID_ARGS);
    clearMap = {{"notExist", {"localDevice", g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), TABLE_NOT_FOUND);
    clearMap = {{g_defaultTable1, {g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), NOT_SUPPORT);
    clearMap = {{g_defaultTable1, {}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), INVALID_ARGS);
    clearMap = {{g_defaultTable1, {""}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), INVALID_ARGS);
    std::string longInvalid(129, 'a');
    clearMap = {{g_defaultTable1, {longInvalid}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), INVALID_ARGS);
    clearMap = {};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), INVALID_ARGS);
    clearMap = {{g_defaultTable2, {"localDevice", g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), NOT_SUPPORT);
    clearMap = {{g_defaultTable1, {"localDevice", g_deviceA, g_deviceB, "dev4"}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest005
 * @tc.desc: Local clean with invalid tableMode
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbRemoveDataForOtherDevicesTest005, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::SPLIT_BY_DEVICE;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto info2 = GetStoreInfo2(); // dev2 as B
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    /**
     * @tc.steps: step1.  prepare remove info and data
     * @tc.expected: step1. Ok
     */
    int count = 2;
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BlockPush(info1, info2, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DBCommon::GetDistributedTableName(g_deviceA, g_defaultTable1), ""), count);

    /**
     * @tc.steps: step2. A local clean with invalid tableMode
     * @tc.expected: step2. NOT_SUPPORT
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice"}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), NOT_SUPPORT);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DBCommon::GetDistributedTableName(g_deviceA, g_defaultTable1), ""), count);
}

#ifdef USE_DISTRIBUTEDDB_CLOUD
/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest006
 * @tc.desc: Local clean with invalid sync mode
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbRemoveDataForOtherDevicesTest006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. sync dev1 data to cloud.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 2);

    /**
     * @tc.steps: step2. A local clean with invalid sync mode
     * @tc.expected: step2. NOT_SUPPORT
     */
    auto delegateA = GetDelegate(info1);
    ASSERT_NE(delegateA, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice"}}};
    EXPECT_EQ(delegateA->RemoveExceptDeviceData(clearMap), NOT_SUPPORT);
}
#endif

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest007
 * @tc.desc: sync A->B->C->A, A call RemoveExceptDeviceData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbRemoveDataForOtherDevicesTest007, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto info2 = GetStoreInfo2(); // dev2 as B
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    auto info3 = GetStoreInfo3(); // dev3 as C
    ASSERT_EQ(BasicUnitTest::InitDelegate(info3, g_deviceC), E_OK);
    /**
     * @tc.steps: step1.  prepare remove info and data
     * @tc.expected: step1. Ok
     */
    int count = 2;
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info3, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId("dev1");
    BlockPush(info1, info2, g_defaultTable1);
    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='update2'";
    EXPECT_EQ(ExecuteSQL(sql, info2), E_OK);
    BasicUnitTest::SetLocalDeviceId("dev2");
    BlockPush(info2, info3, g_defaultTable1);
    sql = "UPDATE " + g_defaultTable1 + " SET name='update3'";
    EXPECT_EQ(ExecuteSQL(sql, info3), E_OK);
    BlockPush(info3, info1, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);

    /**
     * @tc.steps: step2. A local clean without sync-delete
     * @tc.expected: step2. Ok
     */
    auto delegateA = GetDelegate(info1);
    ASSERT_NE(delegateA, nullptr);
    BasicUnitTest::SetLocalDeviceId("dev1");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"dev1"}}};
    EXPECT_EQ(delegateA->RemoveExceptDeviceData(clearMap), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest008
 * @tc.desc: call RemoveExceptDeviceData with tracker table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbRemoveDataForOtherDevicesTest008, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    auto info2 = GetStoreInfo2(); // dev2 as B
    SetSchemaInfo(info1, GetDefaultSchema());
    SetSchemaInfo(info2, GetDefaultSchema());
    /**
     * @tc.steps: step1.  prepare remove info and data
     * @tc.expected: step1. Ok
     */
    int count = 2;
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DEVICE_SYNC_TABLE), count);

    /**
    * @tc.steps: step2. set tracker and distributed tables and sync to dev2
    * @tc.expected: step2. Ok
    */
    ASSERT_EQ(SetDistributedTables(info1, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {DEVICE_SYNC_TABLE}), E_OK);
    ASSERT_EQ(SetTrackerTables(info2, {DEVICE_SYNC_TABLE}), E_OK);
    BasicUnitTest::SetLocalDeviceId("dev1");
    BlockPush(info1, info2, DEVICE_SYNC_TABLE);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DEVICE_SYNC_TABLE), count);

    /**
     * @tc.steps: step3. B call RemoveExceptDeviceData with A kept, table is sync-delete
     * @tc.expected: step3. Ok
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("dev2");
    std::map<std::string, std::vector<std::string>> clearMap = {{DEVICE_SYNC_TABLE, {"dev2", "dev4"}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DEVICE_SYNC_TABLE), 0);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), g_deviceA), count);
}
#endif

#ifdef USE_DISTRIBUTEDDB_CLOUD
/**
 * @tc.name: RdbCloudSyncExample001
 * @tc.desc: Test cloud sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. sync dev1 data to cloud.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 2);
}

/**
 * @tc.name: RdbCloudSyncExample002
 * @tc.desc: Test cloud insert data and cloud sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyue
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. cloud insert data.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    EXPECT_EQ(RDBDataGenerator::InsertCloudDBData(0, 20, 0, RDBGeneralUt::GetSchema(info1), virtualCloudDb), OK);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 20);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 0);

    /**
     * @tc.steps: step2. cloud sync data to dev1.
     * @tc.expected: step2. Ok
     */
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 20);
}

/**
 * @tc.name: RdbCloudSyncExample004
 * @tc.desc: Test upload failed, when return FILE_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample004, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);

    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    virtualCloudDb->SetLocalAssetNotFound(true);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query, OK, LOCAL_ASSET_NOT_FOUND);
    EXPECT_EQ(RDBGeneralUt::GetAbnormalCount(g_defaultTable1, DBStatus::LOCAL_ASSET_NOT_FOUND), 2);

    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='update'";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    virtualCloudDb->SetLocalAssetNotFound(false);
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetAbnormalCount(g_defaultTable1, DBStatus::LOCAL_ASSET_NOT_FOUND), 0);
}

/**
 * @tc.name: RdbCloudSyncExample005
 * @tc.desc: Test upload when asset is abnormal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample005, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);

    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    virtualCloudDb->SetLocalAssetNotFound(true);
    // sync failed and local asset abnormal
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query, OK, LOCAL_ASSET_NOT_FOUND);
    EXPECT_EQ(RDBGeneralUt::GetAbnormalCount(g_defaultTable1, DBStatus::LOCAL_ASSET_NOT_FOUND), 2);

    virtualCloudDb->ClearAllData();
    query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 0);
    // insert new local assert
    std::string sql = "DELETE FROM " + g_defaultTable1;
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    query = Query::Select().FromTable({g_defaultTable1});
    InsertLocalDBData(0, 4, info1);
    virtualCloudDb->SetLocalAssetNotFound(false);
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 2);
}

/**
 * @tc.name: RdbCloudSyncExample006
 * @tc.desc: one table is normal and another is abnormal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample006, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    InsertLocalDBData(0, 2, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable2), 2);
    
    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    virtualCloudDb->SetLocalAssetNotFound(true);
    // sync failed and local asset abnormal
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1, g_defaultTable2}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query, OK, LOCAL_ASSET_NOT_FOUND);
    EXPECT_EQ(RDBGeneralUt::GetAbnormalCount(g_defaultTable1, DBStatus::LOCAL_ASSET_NOT_FOUND), 2);

    virtualCloudDb->ClearAllData();
    virtualCloudDb->SetLocalAssetNotFound(false);

    query = Query::Select().FromTable({g_defaultTable1, g_defaultTable2});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), 0);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable2), 2);
}

/**
 * @tc.name: RdbCloudSyncExample007
 * @tc.desc: sync when table have field "timestamp"
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: liaoyonghuang
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample007, TestSize.Level0)
{
    // step1: init local table
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, false}, false},
        {{"timestamp", TYPE_INDEX<int64_t>, false, true}, false},
    };
    std::string tableName = "test_table";
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = tableName, .fieldInfo = filedInfo}
        }
    };
    RDBGeneralUt::SetSchemaInfo(info1, schemaInfo);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    InsertLocalDBData(0, 30, info1);
    // step2: do sync
    ASSERT_EQ(SetDistributedTables(info1, {tableName}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({tableName});
    RDBGeneralUt::CloudBlockSync(info1, query);
}

/**
 * @tc.name: RdbCloudSyncExample008
 * @tc.desc: Test upload failed, when return SKIP_WHEN_CLOUD_SPACE_INSUFFICIENT
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbCloudSyncExample008, TestSize.Level0)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    InsertLocalDBData(0, 1, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), 1);

    std::shared_ptr<VirtualCloudDb> virtualCloudDb = RDBGeneralUt::GetVirtualCloudDb();
    ASSERT_NE(virtualCloudDb, nullptr);
    virtualCloudDb->SetUploadRecordStatus(DBStatus::SKIP_WHEN_CLOUD_SPACE_INSUFFICIENT);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query, OK, SKIP_WHEN_CLOUD_SPACE_INSUFFICIENT);

    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='update'";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    virtualCloudDb->SetUploadRecordStatus(DBStatus::OK);
    RDBGeneralUt::CloudBlockSync(info1, query, OK, OK);
}
#endif // USE_DISTRIBUTEDDB_CLOUD

/**
 * @tc.name: RdbUtilsTest001
 * @tc.desc: Test rdb utils execute actions.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBBasicRDBTest, RdbUtilsTest001, TestSize.Level0)
{
    std::vector<std::function<int()>> actions;
    /**
     * @tc.steps: step1. execute null actions no effect ret.
     * @tc.expected: step1. E_OK
     */
    actions.emplace_back(nullptr);
    EXPECT_EQ(SQLiteRelationalUtils::ExecuteListAction(actions), E_OK);
    /**
     * @tc.steps: step2. execute abort when action return error.
     * @tc.expected: step2. -E_INVALID_ARGS
     */
    actions.clear();
    actions.emplace_back([]() {
        return -E_INVALID_ARGS;
    });
    actions.emplace_back([]() {
        return -E_NOT_SUPPORT;
    });
    EXPECT_EQ(SQLiteRelationalUtils::ExecuteListAction(actions), -E_INVALID_ARGS);
}
}