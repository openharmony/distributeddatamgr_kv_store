/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "distributeddb_tools_unit_test.h"
#include "sqlite_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char* DB_SUFFIX = ".db";
    constexpr const char* STORE_ID = "Relational_Store_ID";
    string g_testDir;
    string g_dbDir;
    const std::string NORMAL_CREATE_TABLE_SQL = "CREATE TABLE IF NOT EXISTS sync_data(" \
        "key         BLOB NOT NULL UNIQUE," \
        "value       BLOB," \
        "timestamp   INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "device      BLOB," \
        "ori_device  BLOB," \
        "hash_key    BLOB PRIMARY KEY NOT NULL," \
        "w_timestamp INT," \
        "UNIQUE(device, ori_device));" \
        "CREATE INDEX key_index ON sync_data (key, flag);";


    const std::string MULTI_PRIMARI_KEY_SQL = "CREATE TABLE IF NOT EXISTS sync_data(" \
        "key         BLOB NOT NULL UNIQUE," \
        "value       BLOB," \
        "timestamp   INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "device      BLOB," \
        "ori_device  BLOB," \
        "hash_key    BLOB NOT NULL," \
        "w_timestamp INT," \
        "PRIMARY KEY(hash_key, device)" \
        "UNIQUE(device, ori_device));" \
        "CREATE INDEX key_index ON sync_data (key, flag);";

    const std::string NO_PRIMARI_KEY_SQL = "CREATE TABLE IF NOT EXISTS t1 (a INT, b INT, c INT);";

    const std::string ASSET_AFFINITY_SQL = "CREATE TABLE IF NOT EXISTS t1 (a ASSET PRIMARY KEY, b ASSETS," \
        "c asset, d assets, e aSSet, f AssEtS, g passet, h asset0, j assets1, k assetsq, m ass2et, n asusets);";

class DistributedDBRelationalSchemaAnalysisTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalSchemaAnalysisTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    LOGI("The test db is:%s", g_testDir.c_str());
    g_dbDir = g_testDir + "/";
}

void DistributedDBRelationalSchemaAnalysisTest::TearDownTestCase(void)
{}

void DistributedDBRelationalSchemaAnalysisTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBRelationalSchemaAnalysisTest::TearDown(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    return;
}


namespace {
TableInfo AnalysisTable(const std::string &name, const std::string &sql)
{
    sqlite3 *db = RelationalTestUtils::CreateDataBase(g_dbDir + STORE_ID + DB_SUFFIX);
    EXPECT_NE(db, nullptr);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
    EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);

    TableInfo table;
    int errcode = SQLiteUtils::AnalysisSchema(db, name, table);
    EXPECT_EQ(errcode, E_OK);
    EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    return table;
}
}

/**
 * @tc.name: AnalysisTable001
 * @tc.desc: Analysis with table which has one primary key
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, AnalysisTable001, TestSize.Level1)
{
    TableInfo table = AnalysisTable("sync_data", NORMAL_CREATE_TABLE_SQL);
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
}

/**
 * @tc.name: AnalysisTable002
 * @tc.desc: Analysis with table which has multi primary key
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, AnalysisTable002, TestSize.Level1)
{
    TableInfo table = AnalysisTable("sync_data", MULTI_PRIMARI_KEY_SQL);
    EXPECT_EQ(table.GetPrimaryKey().size(), 2u);
}

/**
 * @tc.name: AnalysisTable003
 * @tc.desc: Analysis with table which has no primary key
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, AnalysisTable003, TestSize.Level1)
{
    TableInfo table = AnalysisTable("t1", NO_PRIMARI_KEY_SQL);
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
}

/**
 * @tc.name: AnalysisTable003
 * @tc.desc: Analysis with table which has no primary key
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: xulianhui
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, AnalysisTable004, TestSize.Level1)
{
    TableInfo table = AnalysisTable("t1", "create table t1(a integer primary key autoincrement, b integer);");
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
    EXPECT_EQ(table.GetAutoIncrement(), true);
    table = AnalysisTable("t2", "create table t2(a integer primary key autoincrement , b integer);");
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
    EXPECT_EQ(table.GetAutoIncrement(), true);
    table = AnalysisTable("t3", "create table t3(a integer, b integer primary key autoincrement);");
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
    EXPECT_EQ(table.GetAutoIncrement(), true);
    table = AnalysisTable("t4", "create table t4(a integer, b integer primary key autoincrement );");
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
    EXPECT_EQ(table.GetAutoIncrement(), true);

    table = AnalysisTable("t5", "create table t5(a_autoincrement integer, b integer primary key);");
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
    EXPECT_EQ(table.GetAutoIncrement(), false);

    table = AnalysisTable("t6_autoincrement", "create table t6_autoincrement(a integer, b integer primary key);");
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
    EXPECT_EQ(table.GetAutoIncrement(), false);
}

/**
 * @tc.name: AnalysisTable005
 * @tc.desc: Analysis with table with asset or assets affinity
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangshijie
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, AnalysisTable005, TestSize.Level1)
{
    TableInfo table = AnalysisTable("t1", ASSET_AFFINITY_SQL);
    EXPECT_EQ(table.GetPrimaryKey().size(), 1u);
    std::vector<FieldInfo> fieldInfos = table.GetFieldInfos();
    EXPECT_EQ(fieldInfos.size(), 12u); // 12 is column count
    int index = 1;
    for (const auto &field : fieldInfos) {
        if (index < 7) { // 7 is field index
            EXPECT_EQ(field.GetStorageType(), StorageType::STORAGE_TYPE_BLOB); // asset and assets affinity blob
        } else {
            EXPECT_EQ(field.GetStorageType(), StorageType::STORAGE_TYPE_NULL); // asset and assets affinity blob
        }
        index++;
    }
}

/**
 * @tc.name: SetDistributedSchema001
 * @tc.desc: Set distributed schema
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
  */
HWTEST_F(DistributedDBRelationalSchemaAnalysisTest, SetDistributedSchema001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Create a DistributedSchema;
     */
    RelationalSchemaObject schema;
    DistributedSchema distributedSchema;
    distributedSchema.version = SOFTWARE_VERSION_CURRENT;
    DistributedTable table;
    DistributedField field;
    field.colName = "field1";
    field.isP2pSync = true;
    field.isSpecified = true;
    table.fields.push_back(field);
    field.colName = "field2";
    field.isP2pSync = false;
    field.isSpecified = false;
    table.fields.push_back(field);
    table.tableName = "table1";
    distributedSchema.tables.push_back(table);
    table.tableName = "table2";
    distributedSchema.tables.push_back(table);
    schema.SetDistributedSchema(distributedSchema);
    /**
     * @tc.steps: step2. DistributedSchema to json string and parse it;
     * @tc.expected: DistributedSchema is same.
     */
    auto schemaStr = schema.ToSchemaString();
    RelationalSchemaObject parseSchemaObj;
    EXPECT_EQ(parseSchemaObj.ParseFromSchemaString(schemaStr), E_OK);
    auto parseSchema = parseSchemaObj.GetDistributedSchema();
    EXPECT_EQ(parseSchema.version, distributedSchema.version);
    ASSERT_EQ(parseSchema.tables.size(), distributedSchema.tables.size());
    for (size_t i = 0; i < parseSchema.tables.size(); ++i) {
        EXPECT_EQ(parseSchema.tables[i].tableName, distributedSchema.tables[i].tableName);
        ASSERT_EQ(parseSchema.tables[i].fields.size(), distributedSchema.tables[i].fields.size());
        for (size_t j = 0; j < parseSchema.tables[i].fields.size(); ++j) {
            EXPECT_EQ(parseSchema.tables[i].fields[j].colName, distributedSchema.tables[i].fields[j].colName);
            EXPECT_EQ(parseSchema.tables[i].fields[j].isP2pSync, distributedSchema.tables[i].fields[j].isP2pSync);
        }
    }
}
}
#endif // RELATIONAL_STORE