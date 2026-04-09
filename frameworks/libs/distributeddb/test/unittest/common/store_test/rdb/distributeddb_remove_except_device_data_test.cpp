/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "relational_store_client.h"
#include "relational_store_client_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
const std::string g_deviceA = "dev1";
const std::string g_deviceB = "dev2";
const std::string g_deviceC = "dev3";

constexpr int MAX_DEVICE_NAME_LENGTH = 128;
constexpr int INVALID_DEVICE_NAME_LENGTH = 129;
constexpr int DEFAULT_TEST_COUNT = 2;
constexpr int SMALL_TEST_COUNT = 5;
constexpr int MEDIUM_TEST_COUNT = 10;
constexpr int LARGE_TEST_COUNT = 50;
constexpr int XLARGE_TEST_COUNT = 100;
constexpr int XXLARGE_TEST_COUNT = 200;
constexpr int CONDITION_PARAM_1 = 1;
constexpr int CONDITION_PARAM_2 = 2;
constexpr int CONDITION_PARAM_3 = 3;
constexpr int DEVICE_4_COUNT = 3;
constexpr int ZERO = 0;

class DistributedDBRemoveExceptDeviceDataTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table, bool noPk = false);
    void PrepareRemoveDataStore(StoreInfo &info1, StoreInfo &info2, StoreInfo &info3, int count);
protected:
    void ExecuteTest031();
    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
    int64_t changedRows_ = -1;
};

void DistributedDBRemoveExceptDeviceDataTest::SetUp()
{
    RDBGeneralUt::SetUp();
}

void DistributedDBRemoveExceptDeviceDataTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

UtDateBaseSchemaInfo DistributedDBRemoveExceptDeviceDataTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(DEVICE_SYNC_TABLE));
    return info;
}

UtTableSchemaInfo DistributedDBRemoveExceptDeviceDataTest::GetTableSchema(const std::string &table, bool noPk)
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

void DistributedDBRemoveExceptDeviceDataTest::PrepareRemoveDataStore(
    StoreInfo &info1, StoreInfo &info2, StoreInfo &info3, int count)
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
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count + count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
}

#ifdef USE_DISTRIBUTEDDB_DEVICE
/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest001
 * @tc.desc: Local clean should NOT delete data for other device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest001, TestSize.Level1)
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
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    /**
     * @tc.steps: step2. A local clean without sync-delete
     * @tc.expected: step2. Ok
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice", g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), ZERO);
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
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest002, TestSize.Level1)
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
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    /**
     * @tc.steps: step2. A local clean without sync-delete
     * @tc.expected: step2. Ok
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice", g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);

    /**
     * @tc.steps: step3. Trigger another push sync C->B, B should keep data
     * @tc.expected: step3. Ok
     */
    InsertLocalDBData(0, DEFAULT_TEST_COUNT, info2);
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
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest003, TestSize.Level1)
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
    constexpr int count = DEFAULT_TEST_COUNT;
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
    std::map<std::string, std::vector<std::string>> clearMap =
        {{g_defaultTable1, {"localDevice", g_deviceC}}, {g_defaultTable2, {"localDevice", g_deviceA}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info3, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable2), g_deviceA), count);

    /**
     * @tc.steps: step3. Trigger another push sync C->B, B should keep data
     * @tc.expected: step3. Ok
     */
    InsertLocalDBData(0, DEFAULT_TEST_COUNT, info2);
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
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest004, TestSize.Level1)
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
    constexpr int count = DEFAULT_TEST_COUNT;
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice", g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), NOT_SUPPORT);

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
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), INVALID_ARGS);
    clearMap = {{"", {"localDevice", g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), INVALID_ARGS);
    clearMap = {{"notExist", {"localDevice", g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), TABLE_NOT_FOUND);
    clearMap = {{g_defaultTable1, {""}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), INVALID_ARGS);
    std::string longInvalid(INVALID_DEVICE_NAME_LENGTH, 'a');
    clearMap = {{g_defaultTable1, {longInvalid}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), INVALID_ARGS);
    clearMap = {{g_defaultTable2, {"localDevice", g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), NOT_SUPPORT);
    clearMap = {{g_defaultTable1, {"localDevice", g_deviceA, g_deviceB, "dev4"}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), count);
    clearMap = {{g_defaultTable1, {g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    clearMap = {{g_defaultTable1, {}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    clearMap = {};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest005
 * @tc.desc: Local clean with invalid tableMode
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest005, TestSize.Level1)
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
    constexpr int count = DEFAULT_TEST_COUNT;
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BlockPush(info1, info2, g_defaultTable1);
    std::string distributedTableName = DBCommon::GetDistributedTableName(g_deviceA, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, distributedTableName, ""), count);

    /**
     * @tc.steps: step2. A local clean with invalid tableMode
     * @tc.expected: step2. NOT_SUPPORT
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice"}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), NOT_SUPPORT);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, distributedTableName, ""), count);
}

#ifdef USE_DISTRIBUTEDDB_CLOUD
/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest006
 * @tc.desc: Local clean with invalid sync mode
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest006, TestSize.Level1)
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
    InsertLocalDBData(0, DEFAULT_TEST_COUNT, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), DEFAULT_TEST_COUNT);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}, TableSyncType::CLOUD_COOPERATION), E_OK);
    RDBGeneralUt::SetCloudDbConfig(info1);
    Query query = Query::Select().FromTable({g_defaultTable1});
    RDBGeneralUt::CloudBlockSync(info1, query);
    EXPECT_EQ(RDBGeneralUt::GetCloudDataCount(g_defaultTable1), DEFAULT_TEST_COUNT);

    /**
     * @tc.steps: step2. A local clean with invalid sync mode
     * @tc.expected: step2. NOT_SUPPORT
     */
    auto delegateA = GetDelegate(info1);
    ASSERT_NE(delegateA, nullptr);
    BasicUnitTest::SetLocalDeviceId("localDevice");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"localDevice"}}};
    EXPECT_EQ(delegateA->RemoveExceptDeviceData(clearMap, changedRows_), NOT_SUPPORT);
}
#endif

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest007
 * @tc.desc: sync A->B->C->A, A call RemoveExceptDeviceData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest007, TestSize.Level1)
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
    constexpr int count = DEFAULT_TEST_COUNT;
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
    EXPECT_EQ(delegateA->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest008
 * @tc.desc: call RemoveExceptDeviceData with tracker table
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xiefengzhu
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest008, TestSize.Level1)
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
    constexpr int count = DEFAULT_TEST_COUNT;
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
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DEVICE_SYNC_TABLE), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE), g_deviceA), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest009
 * @tc.desc: sync A->B, B update and call RemoveExceptDeviceData
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest009, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto info2 = GetStoreInfo2(); // dev2 as B
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    /**
     * @tc.steps: step1.  prepare remove info and data
     * @tc.expected: step1. Ok
     */
    constexpr int count = 1;
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);
    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='update2'";
    EXPECT_EQ(ExecuteSQL(sql, info2), E_OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    /**
     * @tc.steps: step2. A local clean without mismatch data
     * @tc.expected: step2. Ok
     */
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    EXPECT_EQ(delegateB->RemoveExceptDeviceData({}, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DBCommon::GetLogTableName(g_defaultTable1), "flag&0x02!=0"), count);
    /**
     * @tc.steps: step3. Change flag to local
     * @tc.expected: step3. Ok
     */
    auto db = GetSqliteHandle(info2);
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.content.flag = LogFlag::REMOTE;
    updateOption.condition.dataCondition = {
        .sql = "name=?",
        .args = {std::string("update2")}
    };
    ASSERT_EQ(UpdateDataLog(db, updateOption), OK);
    EXPECT_EQ(delegateB->RemoveExceptDeviceData({}, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DBCommon::GetLogTableName(g_defaultTable1)), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest010
 * @tc.desc: Test RemoveExceptDeviceData with empty table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest010, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"dev1"}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest011
 * @tc.desc: Test RemoveExceptDeviceData with all devices kept.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest011, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEVICE_4_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("dev2");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceA, g_deviceC, g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count + count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest012
 * @tc.desc: Test RemoveExceptDeviceData with single device kept.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest012, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEVICE_4_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("dev2");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceA}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest013
 * @tc.desc: Test RemoveExceptDeviceData with duplicate device names.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest013, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("dev2");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceA, g_deviceA, g_deviceA}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest014
 * @tc.desc: Test RemoveExceptDeviceData with non-existent device.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest014, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId("dev2");
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"not_exist_device"}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest015
 * @tc.desc: Test RemoveExceptDeviceData with very long device name.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest015, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    constexpr int count = DEFAULT_TEST_COUNT;
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    std::string longDevice(MAX_DEVICE_NAME_LENGTH, 'a');
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {longDevice}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest016
 * @tc.desc: Test RemoveExceptDeviceData with special characters in device name.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest016, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    constexpr int count = DEFAULT_TEST_COUNT;
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    std::string specialDevice = "device-with_special.chars@123";
    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {specialDevice}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest017
 * @tc.desc: Test RemoveExceptDeviceData with single data record.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest017, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    constexpr int count = 1;
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {"local"}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest018
 * @tc.desc: Test RemoveExceptDeviceData while sync is in progress.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest018, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEVICE_4_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), ZERO);

    BlockPush(info3, info2, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest019
 * @tc.desc: Test RemoveExceptDeviceData and then sync from removed device.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest019, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceB, g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);

    BlockPush(info3, info2, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest020
 * @tc.desc: Test RemoveExceptDeviceData with multiple tables.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest020, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable2}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable2}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable2), count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {
        {g_defaultTable1, {g_deviceB, g_deviceC}},
        {g_defaultTable2, {g_deviceB, g_deviceA}}
    };
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable2), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable2), g_deviceA), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest021
 * @tc.desc: Test RemoveExceptDeviceData with modified data on one device.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest021, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceB, g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceA), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest022
 * @tc.desc: Test RemoveExceptDeviceData with modified data on multiple devices.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest022, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    std::string sql = "UPDATE " + g_defaultTable1 + " SET name='dev2_update'";
    EXPECT_EQ(ExecuteSQL(sql, info2), E_OK);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceB, g_deviceA}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count + count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), ZERO);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest023
 * @tc.desc: Test RemoveExceptDeviceData keeps data from one device.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest023, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceB, g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest025
 * @tc.desc: Test RemoveExceptDeviceData performance with large dataset.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest025, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = XLARGE_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest026
 * @tc.desc: Test RemoveExceptDeviceData with multiple tables and large dataset.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest026, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = LARGE_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable2}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable2}), E_OK);
    BlockPush(info1, info2, g_defaultTable2);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable2), count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {
        {g_defaultTable1, {g_deviceC}},
        {g_defaultTable2, {g_deviceA}}
    };
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable2), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest027
 * @tc.desc: Test RemoveExceptDeviceData with data from three devices.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest027, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int DEVICE_4_COUNT = 3;
    PrepareRemoveDataStore(info1, info2, info3, DEVICE_4_COUNT);

    StoreInfo info4Real = {USER_ID, APP_ID, STORE_ID_1 + "_4"};
    ASSERT_EQ(BasicUnitTest::InitDelegate(info4Real, "dev4"), E_OK);
    InsertLocalDBData(0, DEVICE_4_COUNT, info4Real);
    ASSERT_EQ(SetDistributedTables(info4Real, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId("dev4");
    BlockPush(info4Real, info2, g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), DEVICE_4_COUNT + DEVICE_4_COUNT);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceB, g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), DEVICE_4_COUNT);
    std::string logTableName = DBCommon::GetLogTableName(g_defaultTable1);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, logTableName, g_deviceC), DEVICE_4_COUNT);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest028
 * @tc.desc: Test RemoveExceptDeviceData with rapid consecutive calls.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest028, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);

    for (int i = ZERO; i < SMALL_TEST_COUNT; i++) {
        std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceC}}};
        EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    }
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest029
 * @tc.desc: Test RemoveExceptDeviceData memory efficiency.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest029, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = XXLARGE_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest030
 * @tc.desc: Test RemoveExceptDeviceData with alternating keep/remove pattern.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest030, TestSize.Level4)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    auto info3 = GetStoreInfo3();
    constexpr int count = DEFAULT_TEST_COUNT;
    PrepareRemoveDataStore(info1, info2, info3, count);

    auto delegateB = GetDelegate(info2);
    ASSERT_NE(delegateB, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);

    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceB, g_deviceA}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableDataByDev(info2, DBCommon::GetLogTableName(g_defaultTable1), g_deviceC), ZERO);

    clearMap = {{g_defaultTable1, {g_deviceB}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), ZERO);

    PrepareRemoveDataStore(info1, info2, info3, count);
    clearMap = {{g_defaultTable1, {g_deviceB, g_deviceA, g_deviceC}}};
    EXPECT_EQ(delegateB->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count + count);
}

void DistributedDBRemoveExceptDeviceDataTest::ExecuteTest031()
{
    constexpr int count = DEFAULT_TEST_COUNT;
    auto info1 = GetStoreInfo1(); // dev1 as A
    auto info2 = GetStoreInfo2(); // dev2 as B
    /**
     * @tc.steps: step2. A delete data and B update data
     * @tc.expected: step2. Ok
     */
    std::string sql = "DELETE FROM " + g_defaultTable1 ;
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    sql = "DELETE FROM " + DBCommon::GetLogTableName(g_defaultTable1);
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    sql = "UPDATE " + g_defaultTable1 + " SET name='update2'";
    EXPECT_EQ(ExecuteSQL(sql, info2), E_OK);
    BlockPush(info2, info1, g_defaultTable1);
    /**
     * @tc.steps: step3. A local clean data
     * @tc.expected: step3. Data exist because of create by it self
     */
    auto delegateA = GetDelegate(info1);
    ASSERT_NE(delegateA, nullptr);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    std::map<std::string, std::vector<std::string>> clearMap = {{g_defaultTable1, {g_deviceA}}};
    EXPECT_EQ(delegateA->RemoveExceptDeviceData(clearMap, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    /**
     * @tc.steps: step4. A change ori_device to dev2 and remove again
     * @tc.expected: step4. Remove success
     */
    auto db = GetSqliteHandle(info1);
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.content.oriDevice = g_deviceB;
    updateOption.condition.logCondition = {
        .sql = "ori_device=?",
        .args = {std::string("")}
    };
    ASSERT_EQ(UpdateDataLog(db, updateOption), OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    EXPECT_EQ(delegateA->RemoveExceptDeviceData({}, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), ZERO);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, DBCommon::GetLogTableName(g_defaultTable1)), ZERO);
    /**
     * @tc.steps: step5. B change ori_device to dev2 and remove
     * @tc.expected: step5. Remove success but data still exists
     */
    updateOption.tableName = g_defaultTable1;
    updateOption.content.oriDevice = g_deviceB;
    updateOption.content.flag = LogFlag::REMOTE;
    updateOption.condition.logCondition = {
        .sql = "ori_device=?",
        .args = {std::string(g_deviceA)}
    };
    db = GetSqliteHandle(info2);
    ASSERT_EQ(UpdateDataLog(db, updateOption), OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceB);
    auto delegateB = GetDelegate(info2);
    EXPECT_EQ(delegateB->RemoveExceptDeviceData({}, changedRows_), OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, g_defaultTable1), count);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DBCommon::GetLogTableName(g_defaultTable1)), count);
}

/**
 * @tc.name: RdbRemoveDataForOtherDevicesTest031
 * @tc.desc: sync A->B->A, A rebuild and call RemoveExceptDeviceData
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RdbRemoveDataForOtherDevicesTest031, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1(); // dev1 as A
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    auto info2 = GetStoreInfo2(); // dev2 as B
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);
    /**
     * @tc.steps: step1. Prepare remove info and data
     * @tc.expected: step1. Ok
     */
    constexpr int count = DEFAULT_TEST_COUNT;
    InsertLocalDBData(0, count, info1);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info1, g_defaultTable1), count);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);
    ExecuteTest031();
}

// ============================================================================
// RelationalStoreClientUtils Test Cases
// ============================================================================

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog001
 * @tc.desc: Test UpdateDataLog with null database pointer
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog001, TestSize.Level1)
{
    UpdateOption option;
    option.tableName = g_defaultTable1;
    option.condition.logCondition = SelectCondition{"1=1", {}};
    option.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(nullptr, option);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog002
 * @tc.desc: Test UpdateDataLog with empty table name
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog002, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    UpdateOption updateOption;
    updateOption.tableName = "";
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog003
 * @tc.desc: Test UpdateDataLog with both log and data conditions (invalid)
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog003, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.condition.dataCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog004
 * @tc.desc: Test UpdateDataLog with no conditions (invalid)
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog004, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog005
 * @tc.desc: Test UpdateDataLog with no update content (invalid)
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog005, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    UpdateContent content;
    updateOption.content = content;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog006
 * @tc.desc: Test UpdateDataLog with invalid LogFlag value
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog006, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = static_cast<LogFlag>(999);

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog007
 * @tc.desc: Test UpdateDataLog with non-existent table
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog007, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    UpdateOption updateOption;
    updateOption.tableName = "non_existent_table";
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, -E_TABLE_NOT_FOUND);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog008
 * @tc.desc: Test UpdateDataLog with non-distributed table
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog008, TestSize.Level1)
{
    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    // Create a non-distributed table
    std::string sql = "CREATE TABLE non_distributed_table (id INTEGER PRIMARY KEY);";
    ASSERT_EQ(ExecuteSQL(sql, info), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    UpdateOption updateOption;
    updateOption.tableName = "non_distributed_table";
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    // Table not found in schema, should return -E_DISTRIBUTED_SCHEMA_NOT_FOUND
    EXPECT_EQ(errCode, -E_DISTRIBUTED_SCHEMA_NOT_FOUND);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog009
 * @tc.desc: Test UpdateDataLog to set LOCAL flag on log records
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog009, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    // Insert data and sync
    InsertLocalDBData(0, MEDIUM_TEST_COUNT, info1);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    sqlite3 *db = GetSqliteHandle(info2);
    ASSERT_NE(db, nullptr);

    // Update log to set LOCAL flag
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog010
 * @tc.desc: Test UpdateDataLog to remove LOCAL flag (REMOTE)
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog010, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    // Insert data and sync
    InsertLocalDBData(0, MEDIUM_TEST_COUNT, info1);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    sqlite3 *db = GetSqliteHandle(info2);
    ASSERT_NE(db, nullptr);

    // Update log to set LOCAL flag first
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = LogFlag::LOCAL;
    ASSERT_EQ(RelationalStoreClientUtils::UpdateDataLog(db, updateOption), E_OK);

    // Then remove LOCAL flag using REMOTE
    updateOption.content.flag = LogFlag::REMOTE;
    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog011
 * @tc.desc: Test UpdateDataLog to update ori_device
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog011, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    // Insert data and sync
    InsertLocalDBData(0, SMALL_TEST_COUNT, info1);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    sqlite3 *db = GetSqliteHandle(info2);
    ASSERT_NE(db, nullptr);

    // Update ori_device
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.oriDevice = g_deviceB;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog012
 * @tc.desc: Test UpdateDataLog with both flag and ori_device
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog012, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    // Insert data and sync
    InsertLocalDBData(0, SMALL_TEST_COUNT, info1);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    sqlite3 *db = GetSqliteHandle(info2);
    ASSERT_NE(db, nullptr);

    // Update both flag and ori_device
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"1=1", {}};
    updateOption.content.flag = LogFlag::LOCAL;
    updateOption.content.oriDevice = g_deviceC;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DBCommon::GetLogTableName(g_defaultTable1), "flag&0x02=0x02"),
        SMALL_TEST_COUNT);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog013
 * @tc.desc: Test UpdateDataLog with data condition
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog013, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    // Insert data and sync
    InsertLocalDBData(0, MEDIUM_TEST_COUNT, info1);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    sqlite3 *db = GetSqliteHandle(info2);
    ASSERT_NE(db, nullptr);

    // Update log based on data condition (only first 5 records)
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.dataCondition = SelectCondition{"id < ?", {static_cast<int64_t>(SMALL_TEST_COUNT)}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog014
 * @tc.desc: Test UpdateDataLog with parameterized log condition
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog014, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info1 = GetStoreInfo1();
    auto info2 = GetStoreInfo2();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, g_deviceA), E_OK);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info2, g_deviceB), E_OK);

    // Insert data and sync
    InsertLocalDBData(0, MEDIUM_TEST_COUNT, info1);
    ASSERT_EQ(SetDistributedTables(info1, {g_defaultTable1}), E_OK);
    ASSERT_EQ(SetDistributedTables(info2, {g_defaultTable1}), E_OK);
    BasicUnitTest::SetLocalDeviceId(g_deviceA);
    BlockPush(info1, info2, g_defaultTable1);

    sqlite3 *db = GetSqliteHandle(info2);
    ASSERT_NE(db, nullptr);

    // Update with parameterized condition
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"data_key IN (SELECT _rowid_ FROM " +
        g_defaultTable1 + " WHERE id < ?)", {static_cast<int64_t>(CONDITION_PARAM_3)}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(RDBGeneralUt::CountTableData(info2, DBCommon::GetLogTableName(g_defaultTable1), "flag&0x02=0x02"),
        CONDITION_PARAM_3);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog015
 * @tc.desc: Test UpdateDataLog with mismatched parameter count (invalid)
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog015, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    // Mismatched parameters: SQL has 1 placeholder but provide 2 args
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"1=1", {static_cast<int64_t>(CONDITION_PARAM_1),
        static_cast<int64_t>(CONDITION_PARAM_2)}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalStoreClientUtilsUpdateDataLog016
 * @tc.desc: Test UpdateDataLog with empty SQL in condition (invalid)
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsUpdateDataLog016, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    // Empty SQL in condition
    UpdateOption updateOption;
    updateOption.tableName = g_defaultTable1;
    updateOption.condition.logCondition = SelectCondition{"", {}};
    updateOption.content.flag = LogFlag::LOCAL;

    auto errCode = RelationalStoreClientUtils::UpdateDataLog(db, updateOption);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: RelationalStoreClientUtilsGetRDBSchema001
 * @tc.desc: Test GetRDBSchema with valid schema
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsGetRDBSchema001, TestSize.Level1)
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);

    auto info = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info, g_deviceA), E_OK);

    sqlite3 *db = GetSqliteHandle(info);
    ASSERT_NE(db, nullptr);

    auto [errCode, rdbSchema] = RelationalStoreClientUtils::GetRDBSchema(db, false);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: RelationalStoreClientUtilsGetRDBSchema002
 * @tc.desc: Test GetRDBSchema with null database pointer
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRemoveExceptDeviceDataTest, RelationalStoreClientUtilsGetRDBSchema002, TestSize.Level1)
{
    auto [errCode, rdbSchema] = RelationalStoreClientUtils::GetRDBSchema(nullptr, false);
    EXPECT_NE(errCode, E_OK);
}

#endif
} // namespace
