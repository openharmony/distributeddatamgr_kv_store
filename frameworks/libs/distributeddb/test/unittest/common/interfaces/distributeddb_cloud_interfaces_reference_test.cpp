/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "cloud_db_types.h"
#include "db_common.h"
#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "relational_store_manager.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    constexpr const char *DB_SUFFIX = ".db";
    constexpr const char *STORE_ID = "Relational_Store_ID";
    std::string g_testDir;
    std::string g_dbDir;
    std::string g_storePath;
    DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
    RelationalStoreDelegate *g_delegate = nullptr;

    class DistributedDBCloudInterfacesReferenceTest : public testing::Test {
    public:
        static void SetUpTestCase(void);
        static void TearDownTestCase(void);
        void SetUp();
        void TearDown();
    };

    void DistributedDBCloudInterfacesReferenceTest::SetUpTestCase(void)
    {
        DistributedDBToolsUnitTest::TestDirInit(g_testDir);
        LOGD("Test dir is %s", g_testDir.c_str());
        g_dbDir = g_testDir + "/";
        g_storePath = g_dbDir + STORE_ID + DB_SUFFIX;
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    }

    void DistributedDBCloudInterfacesReferenceTest::TearDownTestCase(void)
    {
    }

    void DistributedDBCloudInterfacesReferenceTest::SetUp(void)
    {
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, "PRAGMA journal_mode=WAL;"), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

        DBStatus status = g_mgr.OpenStore(g_storePath, STORE_ID, {}, g_delegate);
        EXPECT_EQ(status, OK);
        ASSERT_NE(g_delegate, nullptr);
    }

    void DistributedDBCloudInterfacesReferenceTest::TearDown(void)
    {
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), OK);
        g_delegate = nullptr;
        DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir);
    }

    /**
     * @tc.name: SetReferenceTest001
     * @tc.desc: Test empty args for set reference interface
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest001, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. call SetReference with empty TableReferenceProperty
         * @tc.expected: step1. Return INVALID_ARGS.
         */
        EXPECT_EQ(g_delegate->SetReference({}), OK);
        TableReferenceProperty tableReferenceProperty;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), INVALID_ARGS);
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        tableReferenceProperty.sourceTableName = sourceTableName;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), INVALID_ARGS);
        tableReferenceProperty.sourceTableName = "";
        tableReferenceProperty.targetTableName = targetTableName;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), INVALID_ARGS);
        tableReferenceProperty.targetTableName = "";
        std::map<std::string, std::string> columns;
        columns["col1"] = "col2";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), INVALID_ARGS);

        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        tableReferenceProperty.columns = {};
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), INVALID_ARGS);
        tableReferenceProperty.sourceTableName = "";
        tableReferenceProperty.targetTableName = targetTableName;
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), INVALID_ARGS);
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = "";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), INVALID_ARGS);
    }

    /**
     * @tc.name: SetReferenceTest002
     * @tc.desc: Test set two reference for same two tables
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest002, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. call SetReference with two TableReferenceProperty which sourceTableName and targetTableName
         * are both same
         * @tc.expected: step1. Return INVALID_ARGS.
         */
        TableReferenceProperty tableReferenceProperty;
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["col1"] = "col2";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty, tableReferenceProperty}), INVALID_ARGS);

        TableReferenceProperty tableReferenceProperty2;
        tableReferenceProperty2.sourceTableName = "sourceTableName1";
        tableReferenceProperty2.targetTableName = targetTableName;
        tableReferenceProperty2.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty, tableReferenceProperty2, tableReferenceProperty}),
            INVALID_ARGS);
    }

    /**
     * @tc.name: SetReferenceTest003
     * @tc.desc: Test simple circular dependency
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest003, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. call SetReference with circular dependency(A->B->A)
         * @tc.expected: step1. Return INVALID_ARGS.
         */
        TableReferenceProperty referenceAB;
        referenceAB.sourceTableName = "ta";
        referenceAB.targetTableName = "tb";
        std::map<std::string, std::string> columns;
        columns["col1"] = "col2";
        referenceAB.columns = columns;

        TableReferenceProperty referenceBA;
        referenceBA.sourceTableName = "tb";
        referenceBA.targetTableName = "ta";
        referenceBA.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({referenceAB, referenceBA}), INVALID_ARGS);

        /**
         * @tc.steps:step2. call SetReference with circular dependency(A->B->C->A)
         * @tc.expected: step1. Return INVALID_ARGS.
         */
        TableReferenceProperty referenceBC;
        referenceBC.sourceTableName = "tb";
        referenceBC.targetTableName = "tc";
        referenceBC.columns = columns;

        TableReferenceProperty referenceCA;
        referenceCA.sourceTableName = "tc";
        referenceCA.targetTableName = "ta";
        referenceCA.columns = columns;

        EXPECT_EQ(g_delegate->SetReference({referenceAB, referenceBC, referenceCA}), INVALID_ARGS);
        EXPECT_EQ(g_delegate->SetReference({referenceCA, referenceAB, referenceBC}), INVALID_ARGS);
    }

    /**
     * @tc.name: SetReferenceTest004
     * @tc.desc: Test complicated circular dependency
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest004, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. call SetReference with complicated dependency
         * @tc.expected: step1. Return INVALID_ARGS.
         */
        TableReferenceProperty referenceAB;
        referenceAB.sourceTableName = "ta";
        referenceAB.targetTableName = "tb";
        std::map<std::string, std::string> columns;
        columns["col1"] = "col2";
        referenceAB.columns = columns;

        TableReferenceProperty referenceDE;
        referenceDE.sourceTableName = "td";
        referenceDE.targetTableName = "te";
        referenceDE.columns = columns;

        TableReferenceProperty referenceAC;
        referenceAC.sourceTableName = "ta";
        referenceAC.targetTableName = "tc";
        referenceAC.columns = columns;

        TableReferenceProperty referenceEF;
        referenceEF.sourceTableName = "te";
        referenceEF.targetTableName = "tf";
        referenceEF.columns = columns;

        TableReferenceProperty referenceBD;
        referenceBD.sourceTableName = "tb";
        referenceBD.targetTableName = "td";
        referenceBD.columns = columns;

        TableReferenceProperty referenceFC;
        referenceFC.sourceTableName = "tf";
        referenceFC.targetTableName = "tc";
        referenceFC.columns = columns;

        EXPECT_EQ(g_delegate->SetReference({referenceAB, referenceDE, referenceAC, referenceEF, referenceBD,
            referenceFC}), DISTRIBUTED_SCHEMA_NOT_FOUND);

        TableReferenceProperty referenceFA;
        referenceFA.sourceTableName = "tf";
        referenceFA.targetTableName = "ta";
        referenceFA.columns = columns;
        EXPECT_EQ(g_delegate->SetReference(
            {referenceAB, referenceDE, referenceAC, referenceEF, referenceBD, referenceFC, referenceFA}), INVALID_ARGS);
    }

    /**
     * @tc.name: SetReferenceTest005
     * @tc.desc: Test table name is case insensitive
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest005, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. call SetReference with two TableReferenceProperty which sourceTableName and targetTableName
         * are both same
         * @tc.expected: step1. Return INVALID_ARGS.
         */
        TableReferenceProperty tableReferenceProperty;
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["col1"] = "col2";
        tableReferenceProperty.columns = columns;

        TableReferenceProperty tableReferenceProperty2;
        tableReferenceProperty2.sourceTableName = "SourCeTable";
        tableReferenceProperty2.targetTableName = "TARGETTABLE";
        tableReferenceProperty2.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty, tableReferenceProperty2}), INVALID_ARGS);
    }

    /**
     * @tc.name: SetReferenceTest006
     * @tc.desc: Test reference table doesn't create distributed table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest006, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. set reference with table doesn't exists
         * @tc.expected: step1. Return DISTRIBUTED_SCHEMA_NOT_FOUND.
         */
        TableReferenceProperty tableReferenceProperty;
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["col1"] = "col2";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), DISTRIBUTED_SCHEMA_NOT_FOUND);

        /**
         * @tc.steps:step2. set reference with table doesn't create distributed table
         * @tc.expected: step2. Return DISTRIBUTED_SCHEMA_NOT_FOUND.
         */
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int);create table " + targetTableName + "(id int);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), DISTRIBUTED_SCHEMA_NOT_FOUND);

        /**
         * @tc.steps:step3. set reference with one table doesn't create distributed table
         * @tc.expected: step3. Return DISTRIBUTED_SCHEMA_NOT_FOUND.
         */
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName), OK);
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), DISTRIBUTED_SCHEMA_NOT_FOUND);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    /**
     * @tc.name: SetReferenceTest007
     * @tc.desc: Test reference table doesn't create cloud sync distributed table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest007, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table in device mode
         * @tc.expected: step1. ok.
         */
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int);create table " + targetTableName + "(id int);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(targetTableName), OK);

        /**
         * @tc.steps:step2. set reference with column doesn't exists
         * @tc.expected: step2. Return INVALID_ARGS.
         */
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["id"] = "id";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), DISTRIBUTED_SCHEMA_NOT_FOUND);

        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    /**
     * @tc.name: SetReferenceTest008
     * @tc.desc: Test reference col doesn't exists table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest008, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table
         * @tc.expected: step1. ok.
         */
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int);create table " + targetTableName + "(id int);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(targetTableName, DistributedDB::CLOUD_COOPERATION), OK);

        /**
         * @tc.steps:step2. set reference with column doesn't exists
         * @tc.expected: step2. Return INVALID_ARGS.
         */
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["col1"] = "col2";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), INVALID_ARGS);

        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    void CheckResult(sqlite3 *db)
    {
        int count = 0;
        std::function<int (sqlite3_stmt *)> bindCallback = [] (sqlite3_stmt *bindStmt) {
            Key key;
            DBCommon::StringToVector("relational_schema", key);
            int errCode = SQLiteUtils::BindBlobToStatement(bindStmt, 1, key, false);
            return errCode;
        };
        std::string sql = "select value from " + DBConstant::RELATIONAL_PREFIX + "metadata where key = ?;";
        int errCode = RelationalTestUtils::ExecSql(db, sql, bindCallback, [&count] (sqlite3_stmt *stmt) {
            std::string schemaStr;
            std::vector<uint8_t> value;
            EXPECT_EQ(SQLiteUtils::GetColumnBlobValue(stmt, 0, value), E_OK);
            DBCommon::VectorToString(value, schemaStr);
            RelationalSchemaObject obj;
            EXPECT_EQ(obj.ParseFromSchemaString(schemaStr), E_OK);
            count++;
            return OK;
        });
        EXPECT_EQ(errCode, E_OK);
        EXPECT_EQ(count, 1);
    }

    void NormalSetReferenceTest(bool multipleTable)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table
         * @tc.expected: step1. ok.
         */
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int);create table " + targetTableName + "(id int);";
        if (multipleTable) {
            sql += "create table t3(key int, value int);create table t4(key int, value int);";
        }
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(targetTableName, DistributedDB::CLOUD_COOPERATION), OK);
        if (multipleTable) {
            EXPECT_EQ(g_delegate->CreateDistributedTable("t3", DistributedDB::CLOUD_COOPERATION), OK);
            EXPECT_EQ(g_delegate->CreateDistributedTable("t4", DistributedDB::CLOUD_COOPERATION), OK);
        }

        /**
         * @tc.steps:step2. set reference
         * @tc.expected: step2. Return OK.
         */
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["id"] = "id";
        tableReferenceProperty.columns = columns;
        std::vector<TableReferenceProperty> vec;
        vec.emplace_back(tableReferenceProperty);
        if (multipleTable) {
            TableReferenceProperty reference;
            reference.sourceTableName = "t3";
            reference.targetTableName = "t4";
            reference.columns["key"] = "key";
            reference.columns["value"] = "value";
            vec.emplace_back(reference);
        }
        EXPECT_EQ(g_delegate->SetReference(vec), OK);

        /**
         * @tc.steps:step3. parse schema in db
         * @tc.expected: step3. Return OK.
         */
        CheckResult(db);

        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    /**
     * @tc.name: SetReferenceTest009
     * @tc.desc: Test normal function for SetReference interface with one table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest009, TestSize.Level0)
    {
        NormalSetReferenceTest(false);
    }

    /**
     * @tc.name: SetReferenceTest010
     * @tc.desc: Test normal function for SetReference interface with two table
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest010, TestSize.Level0)
    {
        NormalSetReferenceTest(true);
    }

    void ReferenceChangeTest(bool isTableEmpty)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table
         * @tc.expected: step1. ok.
         */
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int, value text);create table " +
            targetTableName + "(id int, value text);create table t3 (id int, value text);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(targetTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable("t3", DistributedDB::CLOUD_COOPERATION), OK);

        if (!isTableEmpty) {
            sql = "insert into " + sourceTableName + " values(1, 'zhangsan');";
            EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        }

        /**
         * @tc.steps:step2. set reference
         * @tc.expected: step2. Return OK or PROPERTY_CHANGED.
         */
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["id"] = "id";
        tableReferenceProperty.columns = columns;
        if (isTableEmpty) {
            EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);
        } else {
            EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);
        }

        /**
         * @tc.steps:step3. set reference again with different table reference
         * @tc.expected: step3. Return OK or PROPERTY_CHANGED.
         */
        tableReferenceProperty.targetTableName = "t3";
        if (isTableEmpty) {
            EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);
        } else {
            EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);
        }

        /**
         * @tc.steps:step4. set reference again with different column reference
         * @tc.expected: step4. Return OK or PROPERTY_CHANGED.
         */
        columns["id"] = "value";
        tableReferenceProperty.columns = columns;
        if (isTableEmpty) {
            EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);
        } else {
            EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);
        }

        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
    }

    /**
     * @tc.name: SetReferenceTest011
     * @tc.desc: Test table reference change when table is empty
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest011, TestSize.Level0)
    {
        ReferenceChangeTest(true);
    }

    /**
     * @tc.name: SetReferenceTest012
     * @tc.desc: Test table reference change when table is not empty
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest012, TestSize.Level0)
    {
        ReferenceChangeTest(false);
    }

    /**
     * @tc.name: SetReferenceTest013
     * @tc.desc: Test table set reference is case insensitive
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest013, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table
         * @tc.expected: step1. ok.
         */
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int, value text);create table " +
            targetTableName + "(id int, value text);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(targetTableName, DistributedDB::CLOUD_COOPERATION), OK);
        sql = "insert into " + sourceTableName + " values(1, 'zhangsan');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

        /**
         * @tc.steps:step2. set reference
         * @tc.expected: step2. Return PROPERTY_CHANGED.
         */
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["id"] = "id";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);

        /**
         * @tc.steps:step3. set reference with same table name, but case different
         * @tc.expected: step3. Return OK.
         */
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = "Targettable";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);

        /**
         * @tc.steps:step4. set reference with same column name, but case different(value different)
         * @tc.expected: step4. Return OK.
         */
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        columns["id"] = "ID";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);

        /**
         * @tc.steps:step5. set reference with same column name, but case different(key different)
         * @tc.expected: step5. Return OK.
         */
        std::map<std::string, std::string> columns2;
        columns2["ID"] = "ID";
        tableReferenceProperty.columns = columns2;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);

        /**
         * @tc.steps:step6. set reference with column size not equal
         * @tc.expected: step6. Return PROPERTY_CHANGED.
         */
        columns2["ID"] = "ID";
        columns2["value"] = "value";
        tableReferenceProperty.columns = columns2;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);
    }

    /**
    * @tc.name: SetReferenceTest014
    * @tc.desc: Test table set reference with multi columns
    * @tc.type: FUNC
    * @tc.require:
    * @tc.author: zhangshjie
    */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest014, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table
         * @tc.expected: step1. ok.
         */
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int, value text, name text);create table " +
            targetTableName + "(id int, value text, name text);create table t3 (id int, value text);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(targetTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable("t3", DistributedDB::CLOUD_COOPERATION), OK);
        sql = "insert into " + sourceTableName + " values(1, 'zhangsan', 'test');insert into " + targetTableName +
            " values(2, 'lisi', 'test2');insert into t3 values(3, 'wangwu');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

        /**
         * @tc.steps:step2. set reference
         * @tc.expected: step2. Return PROPERTY_CHANGED.
         */
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["id"] = "id";
        columns["value"] = "value";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);

        /**
         * @tc.steps:step3. set reference with multi columns
         * @tc.expected: step3. Return PROPERTY_CHANGED.
         */
        std::map<std::string, std::string> columns2;
        columns2["id"] = "id";
        columns2["name"] = "name";
        tableReferenceProperty.columns = columns2;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);

        /**
         * @tc.steps:step4. set reference with multi reference property
         * @tc.expected: step4. Return PROPERTY_CHANGED.
         */
        TableReferenceProperty tableReferenceProperty2;
        tableReferenceProperty2.sourceTableName = sourceTableName;
        tableReferenceProperty2.targetTableName = "t3";
        std::map<std::string, std::string> columns3;
        columns3["id"] = "id";
        tableReferenceProperty2.columns = columns3;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty, tableReferenceProperty2}), PROPERTY_CHANGED);

        /**
         * @tc.steps:step5. set reference with one reference property
         * @tc.expected: step5. Return PROPERTY_CHANGED.
         */
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);
    }

    void SetCloudSchema(RelationalStoreDelegate *delegate)
    {
        TableSchema tableSchema;
        Field field1 = { "id", TYPE_INDEX<int64_t>, true, false };
        Field field2 = { "value", TYPE_INDEX<std::string>, false, true };
        Field field3 = { "name", TYPE_INDEX<std::string>, false, true };

        tableSchema = { "sourceTable", "src_shared", { field1, field2, field3} };
        DataBaseSchema dbSchema;
        dbSchema.tables.push_back(tableSchema);
        tableSchema = { "targetTable", "dst_shared", { field1, field2, field3} };
        dbSchema.tables.push_back(tableSchema);

        EXPECT_EQ(delegate->SetCloudDbSchema(dbSchema), OK);
    }

    /**
    * @tc.name: SetReferenceTest015
    * @tc.desc: Test reference change of shared table
    * @tc.type: FUNC
    * @tc.require:
    * @tc.author: zhangshjie
    */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest015, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table
         * @tc.expected: step1. ok.
         */
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int, value text, name text);create table " +
            targetTableName + "(id int, value text, name text);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(targetTableName, DistributedDB::CLOUD_COOPERATION), OK);
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = sourceTableName;
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["id"] = "id";
        columns["value"] = "value";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);

        /**
         * @tc.steps:step2. set cloud db schema, insert data into shared table
         * @tc.expected: step2. ok.
         */
        SetCloudSchema(g_delegate);
        sql = "insert into src_shared values(1, 'zhangsan', 'test', 'aa', 'bb');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

        /**
         * @tc.steps:step3. clear reference
         * @tc.expected: step3. return PROPERTY_CHANGED.
         */
        EXPECT_EQ(g_delegate->SetReference({}), PROPERTY_CHANGED);
    }

    /**
    * @tc.name: SetReferenceTest016
    * @tc.desc: Test set reference after some table was dropped
    * @tc.type: FUNC
    * @tc.require:
    * @tc.author: zhangshjie
    */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest016, TestSize.Level0)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table, then set reference t1->t2
         * @tc.expected: step1. return ok.
         */
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table t1(id int, value text);create table t2(id int, value text);" \
            "create table t3(id int, value text);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable("t1", DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable("t2", DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable("t3", DistributedDB::CLOUD_COOPERATION), OK);
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = "t1";
        tableReferenceProperty.targetTableName = "t2";
        std::map<std::string, std::string> columns;
        columns["id"] = "id";
        columns["value"] = "value";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);

        /**
         * @tc.steps:step2. drop table t1, then reopen store
         * @tc.expected: step2. return ok.
         */
        sql = "drop table t1;";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), OK);
        g_delegate = nullptr;
        EXPECT_EQ(g_mgr.OpenStore(g_storePath, STORE_ID, {}, g_delegate), OK);
        ASSERT_NE(g_delegate, nullptr);

        /**
         * @tc.steps:step3. set reference t2->t1
         * @tc.expected: step3. return ok.
         */
        tableReferenceProperty.sourceTableName = "t2";
        tableReferenceProperty.targetTableName = "t3";
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), OK);
    }

    /**
     * @tc.name: SetReferenceTest017
     * @tc.desc: Test log table is case insensitive in set reference interface
     * @tc.type: FUNC
     * @tc.require:
     * @tc.author: zhangshjie
     */
    HWTEST_F(DistributedDBCloudInterfacesReferenceTest, SetReferenceTest017, TestSize.Level1)
    {
        /**
         * @tc.steps:step1. prepare table and distributed table
         * @tc.expected: step1. ok.
         */
        std::string sourceTableName = "sourceTable";
        std::string targetTableName = "targetTable";
        sqlite3 *db = RelationalTestUtils::CreateDataBase(g_storePath);
        ASSERT_NE(db, nullptr);
        std::string sql = "create table " + sourceTableName + "(id int, value text);create table " +
            targetTableName + "(id int, value text);";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(sourceTableName, DistributedDB::CLOUD_COOPERATION), OK);
        EXPECT_EQ(g_delegate->CreateDistributedTable(targetTableName, DistributedDB::CLOUD_COOPERATION), OK);
        sql = "insert into " + sourceTableName + " values(1, 'zhangsan');";
        EXPECT_EQ(RelationalTestUtils::ExecSql(db, sql), SQLITE_OK);
        EXPECT_EQ(sqlite3_close_v2(db), SQLITE_OK);

        /**
         * @tc.steps:step2. set reference with same table name, but case mismatch
         * @tc.expected: step2. Return PROPERTY_CHANGED.
         */
        TableReferenceProperty tableReferenceProperty;
        tableReferenceProperty.sourceTableName = "SourceTable";
        tableReferenceProperty.targetTableName = targetTableName;
        std::map<std::string, std::string> columns;
        columns["id"] = "id";
        tableReferenceProperty.columns = columns;
        EXPECT_EQ(g_delegate->SetReference({tableReferenceProperty}), PROPERTY_CHANGED);
    }
}
