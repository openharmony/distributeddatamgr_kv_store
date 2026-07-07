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

#include "distributeddb_tools_unit_test.h"

#include <gtest/gtest.h>

#include "db_errno.h"
#include "log_print.h"
#include "schema_utils.h"
#include "data_donation_schema.h"
#include "distributeddb_data_donation_schema_json.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

class DistributedDBDataDonationSchemaUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {}
    static void TearDownTestCase(void)
    {}
    void SetUp();
    void TearDown();
    DataDonationSchema ddSchema;
    void PrintRelationPathInfo(const DataDonationSchema::DdRelationsPath &path);
};

void DistributedDBDataDonationSchemaUnitTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    int errCode = ddSchema.Init(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON);
    ASSERT_EQ(errCode, E_OK);
}

void DistributedDBDataDonationSchemaUnitTest::TearDown(void)
{
}

void DistributedDBDataDonationSchemaUnitTest::PrintRelationPathInfo(const DataDonationSchema::DdRelationsPath &path)
{
    LOGI("[%s path]:", path.table.c_str());
    int i = 0;
    for (auto relation = path.relations.begin(); relation != path.relations.end(); ++relation) {
        LOGI("[%d]:", i++);
        LOGI("local   %s:%s", relation->key.localField.table.c_str(), relation->key.localField.field.c_str());
        LOGI("foreign %s:%s", relation->key.foreignField.table.c_str(), relation->key.foreignField.field.c_str());
        LOGI("key1    %s:%s", relation->localField.table.c_str(), relation->localField.field.c_str());
        LOGI("key2    %s:%s", relation->foreignField.table.c_str(), relation->foreignField.field.c_str());
    }
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4GetAll_001, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath();
    ASSERT_NE(path.table, "");
    ASSERT_FALSE(path.relations.empty());
    PrintRelationPathInfo(path);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4TableA_002, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableA");
    ASSERT_NE(path.table, "");
    ASSERT_FALSE(path.relations.empty());
    PrintRelationPathInfo(path);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4TableB_003, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableB");
    ASSERT_NE(path.table, "");
    ASSERT_FALSE(path.relations.empty());
    PrintRelationPathInfo(path);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4TableC_004, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableC");
    ASSERT_NE(path.table, "");
    ASSERT_FALSE(path.relations.empty());
    PrintRelationPathInfo(path);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4TableD_005, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableD");
    ASSERT_NE(path.table, "");
    ASSERT_FALSE(path.relations.empty());
    PrintRelationPathInfo(path);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4TableE_006, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableE");
    ASSERT_NE(path.table, "");
    ASSERT_FALSE(path.relations.empty());
    PrintRelationPathInfo(path);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4TableF_007, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableF");
    ASSERT_EQ(path.table, "");
    ASSERT_TRUE(path.relations.empty());
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4TableG_008, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableG");
    ASSERT_EQ(path.table, "");
    ASSERT_TRUE(path.relations.empty());
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetRelationPath4TableH_009, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableH");
    ASSERT_EQ(path.table, "");
    ASSERT_TRUE(path.relations.empty());
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_NeedWakeup_010, TestSize.Level0)
{
    DataDonationSchema::DdTrigger trigger;
    trigger.table = "TableA";
    DataDonationSchema::DdCondition condition;
    trigger.fields.insert({"KeyId", condition});
    ASSERT_EQ(ddSchema.NeedWakeup(trigger), true);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_NoNeedWakeup_011, TestSize.Level0)
{
    DataDonationSchema::DdTrigger trigger;
    trigger.table = "TableG";
    DataDonationSchema::DdCondition condition;
    condition.enable = true;
    condition.field = {"TableG", "F45"};
    condition.value = 1;
    trigger.fields.insert({"F45", condition});
    ASSERT_EQ(ddSchema.NeedWakeup(trigger), false);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_ConditionNeedWakeup_012, TestSize.Level0)
{
    DataDonationSchema::DdTrigger trigger;
    trigger.table = "TableE";
    DataDonationSchema::DdCondition condition;
    condition.enable = true;
    condition.field = {"TableA", "filter"};
    condition.value = 1;
    trigger.fields.insert({"F44", condition});
    ASSERT_EQ(ddSchema.NeedWakeup(trigger), true);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_ConditionNoNeedWakeup_013, TestSize.Level0)
{
    DataDonationSchema::DdTrigger trigger;
    trigger.table = "TableE";
    DataDonationSchema::DdCondition condition;
    condition.enable = true;
    condition.field = {"TableA", "filter"};
    condition.value = 2;
    trigger.fields.insert({"F44", condition});
    ASSERT_EQ(ddSchema.NeedWakeup(trigger), false);
}

HWTEST_F(DistributedDBDataDonationSchemaUnitTest, Dfx_GetRelationPathFailed4InvalidTable_001, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("invalid_table");
    ASSERT_EQ(path.table, "");
    ASSERT_TRUE(path.relations.empty());
}

/**
 * @tc.name: FunctionTest_RootTableGetRelationPath_001
 * @tc.desc: Test GetRelationPath for a root table (table with no outgoing FK but with keyOut mapping).
 *           This tests the fix for MergeRelationsMaps which now handles root tables correctly.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_RootTableGetRelationPath_001, TestSize.Level0)
{
    // Use the existing ddSchema which is already working
    // The existing schema has TableA with keyOut and tables pointing to TableA
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath("TableA");
    // TableA should have valid path (it's the root table that other tables point to)
    EXPECT_EQ(path.table, "TableA");
    EXPECT_FALSE(path.relations.empty());
}

/**
 * @tc.name: FunctionTest_RootTableWithChildTables_001
 * @tc.desc: Test GetRelationPath for a root table with child tables having FK to it.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_RootTableWithChildTables_001, TestSize.Level0)
{
    // Use existing ddSchema - TableC has FK to TableA
    DataDonationSchema::DdRelationsPath &pathC = ddSchema.GetRelationPath("TableC");
    EXPECT_EQ(pathC.table, "TableC");
    EXPECT_FALSE(pathC.relations.empty());

    // TableB also has FK to TableA
    DataDonationSchema::DdRelationsPath &pathB = ddSchema.GetRelationPath("TableB");
    EXPECT_EQ(pathB.table, "TableB");
    EXPECT_FALSE(pathB.relations.empty());
}

/**
 * @tc.name: FunctionTest_GetDefaultRelationPath_001
 * @tc.desc: Test GetRelationPath() (default) returns proper path for keyOut without condition.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_GetDefaultRelationPath_001, TestSize.Level0)
{
    // Use existing ddSchema which has proper configuration
    DataDonationSchema::DdRelationsPath &path = ddSchema.GetRelationPath();
    EXPECT_NE(path.table, "");
    EXPECT_FALSE(path.relations.empty());
}

/**
 * @tc.name: FunctionTest_NeedWakeupForRootTable_001
 * @tc.desc: Test NeedWakeup returns correct value for root table triggers.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBDataDonationSchemaUnitTest, FunctionTest_NeedWakeupForRootTable_001, TestSize.Level0)
{
    // Test that NeedWakeup works for TableA (root table with no outgoing FK)
    DataDonationSchema::DdTrigger trigger;
    trigger.table = "TableA";
    DataDonationSchema::DdCondition condition;
    trigger.fields.insert({"KeyId", condition});
    EXPECT_TRUE(ddSchema.NeedWakeup(trigger));

    // Test with condition that matches
    condition.enable = true;
    condition.field = {"TableA", "filter"};
    condition.value = 1;
    trigger.fields.clear();
    trigger.fields.insert({"KeyId", condition});
    EXPECT_TRUE(ddSchema.NeedWakeup(trigger));
}

/**
 * @tc.name: DecodeSchemaErrorTest_001
 * @tc.desc: Test schema decoding when missing search config.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DistributedDBDataDonationSchemaUnitTest, DecodeSchemaErrorTest_001, TestSize.Level0)
{
    std::string schemaStr = R"({
        "dbSchema": [{
            "tables": [
                {
                    "tableName": "table1",
                    "fields": [{
                        "columnName": "column1",
                        "type": "Integer",
                        "primaryKey": true
                    }]
                }
            ]
        }],
        "searchConfig": {}
    })";

    DataDonationSchema schema;
    EXPECT_EQ(schema.Init(schemaStr), -E_INVALID_ARGS);
}
