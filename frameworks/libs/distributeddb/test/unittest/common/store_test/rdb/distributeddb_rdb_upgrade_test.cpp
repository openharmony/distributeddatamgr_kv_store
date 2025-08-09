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
class DistributedDBRDBUpgradeTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    void InitUpgradeDelegate();
    void InitNoJsonTrackerSchema();
    static constexpr const char *DEVICE_SYNC_TABLE = "DEVICE_SYNC_TABLE";
};

void DistributedDBRDBUpgradeTest::SetUp()
{
    RDBGeneralUt::SetUp();
}

void DistributedDBRDBUpgradeTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBRDBUpgradeTest::InitUpgradeDelegate()
{
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, false}, true}, {{"name1", TYPE_INDEX<std::string>, false, true}, false},
        {{"name2", TYPE_INDEX<std::string>, false, true}, false}
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {{.name = DEVICE_SYNC_TABLE, .fieldInfo = filedInfo}}
    };
    RDBGeneralUt::SetSchemaInfo(info1, schemaInfo);
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "dev1"), E_OK);
}

void DistributedDBRDBUpgradeTest::InitNoJsonTrackerSchema()
{
    std::string keyStr = "relational_tracker_schema";
    Key key(keyStr.begin(), keyStr.end());
    std::string schemaStr = R"({"SCHEMA_TYPE":"TRACKER","TABLES":[
        {"NAME": "DEVICE_SYNC_TABLE","EXTEND_NAME": "id",
        "TRACKER_NAMES": ["id","name1","name2"],"TRACKER_ACTION": false}]})";
    Value value(schemaStr.begin(), schemaStr.end());
    ASSERT_EQ(PutMetaData(GetStoreInfo1(), key, value), E_OK);
}

/**
 * @tc.name: UpgradeTracker001
 * @tc.desc: Test rdb upgrade extend field from no json to json.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBUpgradeTest, UpgradeTracker001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    ASSERT_NO_FATAL_FAILURE(InitUpgradeDelegate());
    auto info1 = GetStoreInfo1();
    EXPECT_EQ(SetTrackerTables(info1, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert local data and log, low version schema.
     * @tc.expected: step2. Ok
     */
    InsertLocalDBData(0, 1, info1);
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) + " SET extend_field=1";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    ASSERT_NO_FATAL_FAILURE(InitNoJsonTrackerSchema());
    EXPECT_EQ(CloseDelegate(info1), E_OK);
    ASSERT_NO_FATAL_FAILURE(InitUpgradeDelegate());
    /**
     * @tc.steps: step3. Set tracker table and create distributed table.
     * @tc.expected: step3. Ok
     */
    auto store = GetDelegate(info1);
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(CreateDistributedTable(info1, DEVICE_SYNC_TABLE), E_OK);
    EXPECT_EQ(CountTableData(info1, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE),
        " json_valid(extend_field) = 0"), 0);
    EXPECT_EQ(SetTrackerTables(info1, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step4. Check log extend field.
     * @tc.expected: step4. Ok
     */
    EXPECT_EQ(CountTableData(info1, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE),
        " json_type(extend_field) = 'object'"), 1);
    EXPECT_EQ(CountTableData(info1, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE),
        " json_valid(extend_field) = 0"), 0);
}

/**
 * @tc.name: UpgradeTracker002
 * @tc.desc: Test rdb upgrade extend field is no json format.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBUpgradeTest, UpgradeTracker002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Init delegate and set tracker schema.
     * @tc.expected: step1. Ok
     */
    ASSERT_NO_FATAL_FAILURE(InitUpgradeDelegate());
    auto info1 = GetStoreInfo1();
    EXPECT_EQ(SetTrackerTables(info1, {DEVICE_SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Insert local data and log update extend_field to empty str.
     * @tc.expected: step2. Ok
     */
    InsertLocalDBData(0, 1, info1);
    std::string sql = "UPDATE " + DBCommon::GetLogTableName(DEVICE_SYNC_TABLE) + " SET extend_field=''";
    EXPECT_EQ(ExecuteSQL(sql, info1), E_OK);
    EXPECT_EQ(CountTableData(info1, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE),
        " json_valid(extend_field) = 0"), 1);
    /**
     * @tc.steps: step3. Set tracker again and check log.
     * @tc.expected: step3. Ok
     */
    EXPECT_EQ(SetTrackerTables(info1, {DEVICE_SYNC_TABLE}), E_OK);
    EXPECT_EQ(CountTableData(info1, DBCommon::GetLogTableName(DEVICE_SYNC_TABLE),
        " json_valid(extend_field) = 0"), 0);
}
}