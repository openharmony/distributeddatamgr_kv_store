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

#include <gtest/gtest.h>
#include "cloud_sync_log_table_manager.h"
#include "distributeddb_tools_unit_test.h"
#include "rdb_data_generator.h"
#include "relational_store_client.h"
#include "sqlite_relational_utils.h"
#include "table_info.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
    constexpr const char *DB_SUFFIX = ".db";
    constexpr const char *STORE_ID = "Relational_Store_ID";
    std::string g_dbDir;
    std::string g_testDir;
}

namespace {
class DistributedDBRDBKnowledgeClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
protected:
    static UtTableSchemaInfo GetTableSchema(const std::string &table);
    static void SaveSchemaToMetaTable(sqlite3 *db, const std::string tableName, const TableInfo &tableInfo);
    static void CreateDistributedTable(const std::vector<std::string> &tableNames,
        DistributedDB::TableSyncType tableSyncType);
    static constexpr const char *KNOWLEDGE_TABLE = "KNOWLEDGE_TABLE";
    static constexpr const char *SYNC_TABLE = "SYNC_TABLE";
};


void DistributedDBRDBKnowledgeClientTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    LOGD("Test dir is %s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

void DistributedDBRDBKnowledgeClientTest::TearDownTestCase(void)
{
}

void DistributedDBRDBKnowledgeClientTest::SetUp()
{
}

void DistributedDBRDBKnowledgeClientTest::TearDown()
{
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
}

UtTableSchemaInfo DistributedDBRDBKnowledgeClientTest::GetTableSchema(const std::string &table)
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

void DistributedDBRDBKnowledgeClientTest::SaveSchemaToMetaTable(sqlite3 *db, const std::string tableName,
    const TableInfo &tableInfo)
{
    RelationalSchemaObject schema;
    schema.SetTableMode(DistributedDB::DistributedTableMode::SPLIT_BY_DEVICE);
    schema.RemoveRelationalTable(tableName);
    schema.AddRelationalTable(tableInfo);

    const Key schemaKey(DBConstant::RELATIONAL_SCHEMA_KEY.begin(), DBConstant::RELATIONAL_SCHEMA_KEY.end());
    Value schemaVal;
    auto schemaStr = schema.ToSchemaString();
    EXPECT_FALSE(schemaStr.size() > SchemaConstant::SCHEMA_STRING_SIZE_LIMIT);
    DBCommon::StringToVector(schemaStr, schemaVal);
    EXPECT_EQ(SQLiteRelationalUtils::PutKvData(db, false, schemaKey, schemaVal), E_OK);
}

void DistributedDBRDBKnowledgeClientTest::CreateDistributedTable(const std::vector<std::string> &tableNames,
    DistributedDB::TableSyncType tableSyncType)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    for (const auto &tableName : tableNames) {
        TableInfo tableInfo;
        tableInfo.SetTableName(tableName);
        tableInfo.SetTableSyncType(tableSyncType);
        TrackerTable table111;
        tableInfo.SetTrackerTable(table111);
        DistributedTable distributedTable;
        distributedTable.tableName = tableName;
        tableInfo.SetDistributedTable(distributedTable);
        std::unique_ptr<SqliteLogTableManager> tableManager = std::make_unique<CloudSyncLogTableManager>();
        EXPECT_NE(tableManager, nullptr);
        EXPECT_EQ(SQLiteUtils::AnalysisSchema(db, tableName, tableInfo), E_OK);
        std::vector<FieldInfo> fieldInfos = tableInfo.GetFieldInfos();
        EXPECT_EQ(SQLiteRelationalUtils::CreateRelationalMetaTable(db), E_OK);
        EXPECT_EQ(SQLiteRelationalUtils::InitCursorToMeta(db, false, tableName), E_OK);
        EXPECT_EQ(tableManager->CreateRelationalLogTable(db, tableInfo), E_OK);
        EXPECT_EQ(tableManager->AddRelationalLogTableTrigger(db, tableInfo, ""), E_OK);
        SQLiteRelationalUtils::SetLogTriggerStatus(db, true);
        SaveSchemaToMetaTable(db, tableName, tableInfo);
    }
    EXPECT_EQ(sqlite3_close_v2(db), E_OK);
}

void InsertDBData(const std::string &tableName, int count, sqlite3 *db)
{
    for (int i = 1; i <= count; i++) {
        std::string sql = "INSERT INTO " + tableName + " VALUES(" + std::to_string(i) + ',' + std::to_string(i) + ',' +
            std::to_string(i) + ',' + std::to_string(i) + ");";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
    }
}

/**
 * @tc.name: SetKnowledge001
 * @tc.desc: Test set knowledge schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBKnowledgeClientTest, SetKnowledge001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set knowledge source schema and clean deleted data.
     * @tc.expected: step1. Ok
     */
    UtTableSchemaInfo tableInfo = GetTableSchema(KNOWLEDGE_TABLE);
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    RDBDataGenerator::InitTableWithSchemaInfo(tableInfo, *db);
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
    InsertDBData(schema.tableName, 1, db);
    EXPECT_EQ(CleanDeletedData(db, schema.tableName, 0u), OK);
    /**
     * @tc.steps: step3. Clean deleted data after delete one data.
     * @tc.expected: step3. Ok
     */
    std::string sql = std::string("DELETE FROM ").append(KNOWLEDGE_TABLE).append(" WHERE 1=1");
    EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    EXPECT_EQ(CleanDeletedData(db, schema.tableName, 10u), OK); // delete which cursor less than 10

    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
 * @tc.name: SetKnowledge002
 * @tc.desc: Test set knowledge schema with invalid args.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBKnowledgeClientTest, SetKnowledge002, TestSize.Level0)
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
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    schema.tableName = "UNKNOWN_TABLE";
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), INVALID_ARGS);
    EXPECT_EQ(CleanDeletedData(db, schema.tableName, 0u), OK);

    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
 * @tc.name: SetKnowledge003
 * @tc.desc: Test set knowledge schema after create distributed table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBKnowledgeClientTest, SetKnowledge003, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create distributed table.
     * @tc.expected: step1. Ok
     */
    UtTableSchemaInfo tableInfo1 = GetTableSchema(KNOWLEDGE_TABLE);
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    RDBDataGenerator::InitTableWithSchemaInfo(tableInfo1, *db);
    UtTableSchemaInfo tableInfo2 = GetTableSchema(SYNC_TABLE);
    RDBDataGenerator::InitTableWithSchemaInfo(tableInfo2, *db);
    CreateDistributedTable({SYNC_TABLE}, DistributedDB::CLOUD_COOPERATION);

    /**
     * @tc.steps: step2. Set knowledge source schema.
     * @tc.expected: step2. INVALID_ARGS
     */
    KnowledgeSourceSchema schema;
    schema.tableName = KNOWLEDGE_TABLE;
    schema.extendColNames.insert("id");
    schema.knowledgeColNames.insert("int_field1");
    schema.knowledgeColNames.insert("int_field2");
    schema.tableName = SYNC_TABLE;
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), INVALID_ARGS);

    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}
}
