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
#include "relational_store_client.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBRDBKnowledgeTest : public RDBGeneralUt {
public:
    void SetUp() override;
protected:
    static UtDateBaseSchemaInfo GetDefaultSchema();
    static UtTableSchemaInfo GetTableSchema(const std::string &table);
    static KnowledgeSourceSchema GetKnowledgeSchema();
    static constexpr const char *KNOWLEDGE_TABLE = "KNOWLEDGE_TABLE";
    static constexpr const char *SYNC_TABLE = "SYNC_TABLE";
    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
};

void DistributedDBRDBKnowledgeTest::SetUp()
{
    RDBGeneralUt::SetUp();
    SetSchemaInfo(info1_, GetDefaultSchema());
    InitDatabase(info1_);
}

UtDateBaseSchemaInfo DistributedDBRDBKnowledgeTest::GetDefaultSchema()
{
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(KNOWLEDGE_TABLE));
    return info;
}

UtTableSchemaInfo DistributedDBRDBKnowledgeTest::GetTableSchema(const std::string &table)
{
    UtTableSchemaInfo tableSchema;
    tableSchema.name = table;
    UtFieldInfo field;
    field.field.colName = "id";
    field.field.type = TYPE_INDEX<int64_t>;
    field.field.primary = true;
    tableSchema.fieldInfo.push_back(field);
    field.field.primary = false;
    field.field.colName = "int_field1";
    tableSchema.fieldInfo.push_back(field);
    field.field.colName = "int_field2";
    tableSchema.fieldInfo.push_back(field);
    field.field.colName = "int_field3";
    tableSchema.fieldInfo.push_back(field);
    return tableSchema;
}

KnowledgeSourceSchema DistributedDBRDBKnowledgeTest::GetKnowledgeSchema()
{
    KnowledgeSourceSchema schema;
    schema.extendColNames.insert("id");
    schema.knowledgeColNames.insert("int_field1");
    schema.knowledgeColNames.insert("int_field2");
    schema.tableName = SYNC_TABLE;
    return schema;
}

/**
 * @tc.name: SetKnowledge001
 * @tc.desc: Test set knowledge schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBKnowledgeTest, SetKnowledge001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set knowledge source schema and clean deleted data.
     * @tc.expected: step1. Ok
     */
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    KnowledgeSourceSchema schema;
    schema.tableName = KNOWLEDGE_TABLE;
    schema.extendColNames.insert("id");
    schema.knowledgeColNames.insert("int_field1");
    schema.knowledgeColNames.insert("int_field2");
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), OK);
    EXPECT_EQ(CleanDeletedData(db, schema.tableName, 0u), OK);
    /**
     * @tc.steps: step2. Clean deleted data after insert one data.
     * @tc.expected: step2. Ok
     */
    EXPECT_EQ(InsertLocalDBData(0, 1, info1_), E_OK);
    EXPECT_EQ(CleanDeletedData(db, schema.tableName, 0u), OK);
    /**
     * @tc.steps: step3. Clean deleted data after delete one data.
     * @tc.expected: step3. Ok
     */
    std::string sql = std::string("DELETE FROM ").append(KNOWLEDGE_TABLE).append(" WHERE 1=1");
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    EXPECT_EQ(CleanDeletedData(db, schema.tableName, 10u), OK); // delete which cursor less than 10
}

/**
 * @tc.name: SetKnowledge002
 * @tc.desc: Test set knowledge schema with invalid args.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBKnowledgeTest, SetKnowledge002, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set knowledge source schema and clean deleted data with null db.
     * @tc.expected: step1. INVALID_ARGS
     */
    KnowledgeSourceSchema schema;
    schema.tableName = KNOWLEDGE_TABLE;
    schema.extendColNames.insert("id");
    schema.knowledgeColNames.insert("int_field1");
    schema.knowledgeColNames.insert("int_field2");
    EXPECT_EQ(SetKnowledgeSourceSchema(nullptr, schema), INVALID_ARGS);
    EXPECT_EQ(CleanDeletedData(nullptr, schema.tableName, 0u), INVALID_ARGS);
    /**
     * @tc.steps: step2. Set knowledge source schema and clean deleted data with null db.
     * @tc.expected: step2. INVALID_ARGS
     */
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    schema.tableName = "UNKNOWN_TABLE";
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), INVALID_ARGS);
    EXPECT_EQ(CleanDeletedData(db, schema.tableName, 0u), OK);
}

/**
 * @tc.name: SetKnowledge003
 * @tc.desc: Test set knowledge schema after create distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBKnowledgeTest, SetKnowledge003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create distributed table.
     * @tc.expected: step1. Ok
     */
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(KNOWLEDGE_TABLE));
    info.tablesInfo.push_back(GetTableSchema(SYNC_TABLE));
    SetSchemaInfo(info1_, info);
    ASSERT_EQ(DistributedDB::RDBGeneralUt::InitDelegate(info1_), E_OK);
    ASSERT_EQ(CreateDistributedTable(info1_, SYNC_TABLE), E_OK);
    /**
     * @tc.steps: step2. Set knowledge source schema.
     * @tc.expected: step2. INVALID_ARGS
     */
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(SetKnowledgeSourceSchema(db, GetKnowledgeSchema()), INVALID_ARGS);
}

/**
 * @tc.name: SetKnowledge004
 * @tc.desc: Test set knowledge schema after create tracker table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBKnowledgeTest, SetKnowledge004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create tracker table.
     * @tc.expected: step1. Ok
     */
    UtDateBaseSchemaInfo info;
    info.tablesInfo.push_back(GetTableSchema(SYNC_TABLE));
    info.tablesInfo.push_back(GetTableSchema(KNOWLEDGE_TABLE));
    SetSchemaInfo(info1_, info);
    ASSERT_EQ(DistributedDB::RDBGeneralUt::InitDelegate(info1_), E_OK);
    ASSERT_EQ(DistributedDB::RDBGeneralUt::SetTrackerTables(info1_, {SYNC_TABLE}), E_OK);
    /**
     * @tc.steps: step2. Set knowledge source schema.
     * @tc.expected: step2. INVALID_ARGS
     */
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(SetKnowledgeSourceSchema(db, GetKnowledgeSchema()), INVALID_ARGS);
}
}