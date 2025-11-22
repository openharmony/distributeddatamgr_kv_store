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
#include "res_finalizer.h"
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

    const Key schemaKey(DBConstant::RELATIONAL_SCHEMA_KEY,
        DBConstant::RELATIONAL_SCHEMA_KEY + strlen(DBConstant::RELATIONAL_SCHEMA_KEY));
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

void UpdateDBData(sqlite3 *db, const std::string &tableName, int key, bool isChange)
{
    std::string value = isChange ? std::to_string(key + 1) : std::to_string(key);
    std::string sql = "UPDATE " + tableName + " SET int_field1 = " + value + ";";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
}

void SetProcessFlag(sqlite3 *db, int key, uint32_t flag)
{
    std::string sql = "UPDATE naturalbase_rdb_aux_KNOWLEDGE_TABLE_log SET flag=flag|" + std::to_string(flag) +
        " WHERE data_key=" + std::to_string(key) + ";";
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), E_OK);
}

void CheckFlagAndCursor(sqlite3 *db, const std::string &tableName, int key, uint32_t flag, int64_t cursor)
{
    std::string sql = "SELECT flag, cursor FROM naturalbase_rdb_aux_KNOWLEDGE_TABLE_log WHERE data_key=" +
        std::to_string(key) + ";";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    ASSERT_EQ(errCode, E_OK);

    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("Reset stmt failed when check column type %d", ret);
        }
    });

    errCode = SQLiteUtils::StepWithRetry(stmt);
    ASSERT_EQ(errCode, SQLiteUtils::MapSQLiteErrno(SQLITE_ROW));

    int64_t realFlag = sqlite3_column_int64(stmt, 0);
    EXPECT_EQ(realFlag, static_cast<int64_t>(flag));

    int64_t realCursor = sqlite3_column_int64(stmt, 1);
    EXPECT_EQ(realCursor, cursor);
}

std::string GetOldTriggerSql()
{
    std::string updateTrigger = R"(CREATE TRIGGER IF NOT EXISTS naturalbase_rdb_KNOWLEDGE_TABLE_ON_UPDATE AFTER UPDATE
        ON 'KNOWLEDGE_TABLE'
        FOR EACH ROW BEGIN
        UPDATE naturalbase_rdb_aux_metadata SET value=value+1 WHERE
            key=x'6e61747572616c626173655f7264625f6175785f637572736f725f6b6e6f776c656467655f7461626c65' AND  CASE WHEN
            ((NEW.int_field1 IS NOT OLD.int_field1) OR (NEW.int_field2 IS NOT OLD.int_field2)) THEN 4 ELSE 0 END;
        UPDATE naturalbase_rdb_aux_KNOWLEDGE_TABLE_log SET timestamp=get_raw_sys_time(), device='', flag=0x02,
            extend_field = json_object('id', NEW.id), cursor = CASE WHEN ((NEW.int_field1 IS NOT OLD.int_field1) OR
            (NEW.int_field2 IS NOT OLD.int_field2)) THEN (SELECT value FROM naturalbase_rdb_aux_metadata WHERE
            key=x'6e61747572616c626173655f7264625f6175785f637572736f725f6b6e6f776c656467655f7461626c65')
            ELSE cursor END  WHERE data_key = OLD._rowid_;
        SELECT client_observer('KNOWLEDGE_TABLE', OLD._rowid_, 1,  CASE WHEN
            ((NEW.int_field1 IS NOT OLD.int_field1) OR (NEW.int_field2 IS NOT OLD.int_field2))
            THEN 4 ELSE 0 END);END;)";
    return updateTrigger;
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

/**
 * @tc.name: SetKnowledge004
 * @tc.desc: Test set knowledge schema with sort column.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBRDBKnowledgeClientTest, SetKnowledge004, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set knowledge source schema when sort column has type int.
     * @tc.expected: step1. Ok
     */
    UtTableSchemaInfo tableInfo = GetTableSchema(KNOWLEDGE_TABLE);

    // add a non-integer field
    UtFieldInfo field;
    field.field.colName = "string_field";
    field.field.type = TYPE_INDEX<std::string>;
    field.field.primary = false;
    tableInfo.fieldInfo.push_back(field);

    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    ASSERT_NE(db, nullptr);
    RDBDataGenerator::InitTableWithSchemaInfo(tableInfo, *db);
    KnowledgeSourceSchema schema;
    schema.tableName = KNOWLEDGE_TABLE;
    schema.columnsToVerify = {};
    schema.extendColNames.insert("id");
    schema.knowledgeColNames.insert("int_field1");
    schema.knowledgeColNames.insert("int_field2");
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), OK);

    /**
     * @tc.steps: step2. Set knowledge source schema when sort column not exist.
     * @tc.expected: step2. INVALID_ARGS
     */
    schema.columnsToVerify = {{"processSequence", {"not_exist"}}};
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), INVALID_ARGS);

    /**
     * @tc.steps: step3. Set knowledge source schema when sort column is not integer.
     * @tc.expected: step3. INVALID_ARGS
     */
    schema.columnsToVerify = {{"processSequence", {"string_field"}}};
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), INVALID_ARGS);

    /**
     * @tc.steps: step4. Set knowledge source schema when sort column empty.
     * @tc.expected: step4. OK
     */
    schema.columnsToVerify = {{"processSequence", {"", "id", "int_field1"}}};
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), OK);

    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
 * @tc.name: SetKnowledge005
 * @tc.desc: Test knowledge table update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBRDBKnowledgeClientTest, SetKnowledge005, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set knowledge source schema
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

    /**
     * @tc.steps: step2. insert data and set flag to processed.
     * @tc.expected: step2. Ok
     */
    uint32_t flag = static_cast<uint32_t>(LogInfoFlag::FLAG_KNOWLEDGE_INVERTED_WRITE);
    InsertDBData(schema.tableName, 1, db);
    SetProcessFlag(db, 1, flag);
    uint32_t expectFlag = static_cast<uint32_t>(LogInfoFlag::FLAG_KNOWLEDGE_INVERTED_WRITE) |
        static_cast<uint32_t>(LogInfoFlag::FLAG_LOCAL);
    int64_t expectCursor = 1;
    CheckFlagAndCursor(db, schema.tableName, 1, expectFlag, expectCursor);

    /**
     * @tc.steps: step3. update data with same content, flag and cursor unchanged
     * @tc.expected: step3. Ok
     */
    UpdateDBData(db, schema.tableName, 1, false);
    CheckFlagAndCursor(db, schema.tableName, 1, expectFlag, expectCursor);

    /**
     * @tc.steps: step4. update data with different content, flag and cursor changed
     * @tc.expected: step4. Ok
     */
    UpdateDBData(db, schema.tableName, 1, true);
    expectFlag = static_cast<uint32_t>(LogInfoFlag::FLAG_LOCAL);
    expectCursor = 2;
    CheckFlagAndCursor(db, schema.tableName, 1, expectFlag, expectCursor);

    /**
     * @tc.steps: step5. set knowledge schema again when has new trigger.
     * @tc.expected: step5. Ok
     */
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), OK);

    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}

/**
 * @tc.name: SetKnowledge006
 * @tc.desc: Test set knowledge table schema when trigger needs update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suyuchen
 */
HWTEST_F(DistributedDBRDBKnowledgeClientTest, SetKnowledge006, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Set knowledge source schema
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

    /**
     * @tc.steps: step2. set to old trigger
     * @tc.expected: step2. Ok
     */
    std::string dropTrigger = "DROP TRIGGER IF EXISTS naturalbase_rdb_" + std::string(KNOWLEDGE_TABLE) + "_ON_UPDATE;";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db, dropTrigger), E_OK);

    std::string createOldTrigger = GetOldTriggerSql();
    ASSERT_EQ(RelationalTestUtils::ExecSql(db, createOldTrigger), E_OK);

    /**
     * @tc.steps: step3. insert data and update data with same content
     * @tc.expected: step3. Ok, flag changed
     */
    uint32_t flag = static_cast<uint32_t>(LogInfoFlag::FLAG_KNOWLEDGE_INVERTED_WRITE);
    InsertDBData(schema.tableName, 1, db);
    SetProcessFlag(db, 1, flag);
    UpdateDBData(db, schema.tableName, 1, false);
    CheckFlagAndCursor(db, schema.tableName, 1, static_cast<uint32_t>(LogInfoFlag::FLAG_LOCAL), 1);

    /**
     * @tc.steps: step4. Set knowledge cursor and set knowledge source schema again
     * @tc.expected: step4. Ok, flag is updated
     */
    std::string setCursor = std::string("INSERT OR REPLACE INTO naturalbase_rdb_aux_metadata (key, value) VALUES") +
        " ('knowledge_cursor_" + std::string(KNOWLEDGE_TABLE) + "', 1)";
    ASSERT_EQ(RelationalTestUtils::ExecSql(db, setCursor), E_OK);
    EXPECT_EQ(SetKnowledgeSourceSchema(db, schema), OK);

    uint32_t expectFlag = static_cast<uint32_t>(LogInfoFlag::FLAG_KNOWLEDGE_INVERTED_WRITE) |
        static_cast<uint32_t>(LogInfoFlag::FLAG_LOCAL) |
        static_cast<uint32_t>(LogInfoFlag::FLAG_KNOWLEDGE_VECTOR_WRITE);
    int64_t expectCursor = 1;
    CheckFlagAndCursor(db, schema.tableName, 1, expectFlag, expectCursor);

    /**
     * @tc.steps: step5. update data with same content, flag and cursor unchanged
     * @tc.expected: step5. Ok
     */
    UpdateDBData(db, schema.tableName, 1, false);
    CheckFlagAndCursor(db, schema.tableName, 1, expectFlag, expectCursor);

    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
}
}
