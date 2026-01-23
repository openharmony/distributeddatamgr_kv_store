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
#ifdef USE_DISTRIBUTEDDB_CLOUD
using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
class DistributedDBRDBSqliteUtilsTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
protected:
    static constexpr const char *CLOUD_SYNC_TABLE_A = "CLOUD_SYNC_TABLE_A";
    void InitTables(const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitSchema(const StoreInfo &info, const std::string &table = CLOUD_SYNC_TABLE_A);
    void InitDistributedTable(const std::vector<std::string> &tables = {CLOUD_SYNC_TABLE_A});
    TableSchema GetTableSchema(const StoreInfo &info, const std::string &table) const;
    std::vector<TableSchema> GetTableSchemaVectors() const;

    StoreInfo info1_ = {USER_ID, APP_ID, STORE_ID_1};
};

void DistributedDBRDBSqliteUtilsTest::SetUp()
{
    RDBGeneralUt::SetUp();
    // create db first
    EXPECT_EQ(BasicUnitTest::InitDelegate(info1_, "dev1"), E_OK);
    EXPECT_NO_FATAL_FAILURE(InitTables());
    InitSchema(info1_);
    InitDistributedTable();
}

void DistributedDBRDBSqliteUtilsTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

void DistributedDBRDBSqliteUtilsTest::InitTables(const std::string &table)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + table + "("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "intCol INTEGER, stringCol1 TEXT, stringCol2 asset)";
    EXPECT_EQ(ExecuteSQL(sql, info1_), E_OK);
}

void DistributedDBRDBSqliteUtilsTest::InitSchema(const StoreInfo &info, const std::string &table)
{
    const std::vector<UtFieldInfo> filedInfo = {
        {{"id", TYPE_INDEX<int64_t>, true, false}, false},
        {{"intCol", TYPE_INDEX<int64_t>, false, true}, false},
        {{"stringCol1", TYPE_INDEX<std::string>, false, true}, false}
    };
    UtDateBaseSchemaInfo schemaInfo = {
        .tablesInfo = {
            {.name = table, .sharedTableName = table + "_shared", .fieldInfo = filedInfo, }
        }
    };
    RDBGeneralUt::SetSchemaInfo(info, schemaInfo);
}

void DistributedDBRDBSqliteUtilsTest::InitDistributedTable(const std::vector<std::string> &tables)
{
    RDBGeneralUt::SetCloudDbConfig(info1_);
    ASSERT_EQ(SetDistributedTables(info1_, tables, TableSyncType::CLOUD_COOPERATION), E_OK);
}

TableSchema DistributedDBRDBSqliteUtilsTest::GetTableSchema(const StoreInfo &info, const std::string &table) const
{
    auto dbSchema = GetSchema(info);
    for (const auto &tableSchema : dbSchema.tables) {
        if (tableSchema.name == table) {
            return tableSchema;
        }
    }
    TableSchema schema;
    schema.name = table;
    return schema;
}

/**
 * @tc.name: PutCloudGid001
 * @tc.desc: Test put cloud gid with abnormal params.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBSqliteUtilsTest, PutCloudGid001, TestSize.Level2)
{
    std::vector<VBucket> data;
    EXPECT_EQ(SQLiteRelationalUtils::PutCloudGid(nullptr, "", data), -E_INVALID_DB);
    VBucket row;
    data.push_back(row);
    row[CloudDbConstant::GID_FIELD] = Nil();
    data.push_back(row);
    row[CloudDbConstant::GID_FIELD] = std::string("str");
    data.push_back(row);
    EXPECT_EQ(SQLiteRelationalUtils::PutCloudGidInner(nullptr, data), -E_INVALID_ARGS);
}

/**
 * @tc.name: PutCloudGid002
 * @tc.desc: Test put cloud gid with abnormal table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBSqliteUtilsTest, PutCloudGid002, TestSize.Level2)
{
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    std::string sql = "CREATE TABLE IF NOT EXISTS " + DBCommon::GetTmpLogTableName(CLOUD_SYNC_TABLE_A) +
                      "(ID integer primary key autoincrement)";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    ASSERT_EQ(errCode, E_OK);
    std::vector<VBucket> data;
    EXPECT_EQ(SQLiteRelationalUtils::PutCloudGid(db, CLOUD_SYNC_TABLE_A, data), -1);
}

/**
 * @tc.name: GetOneBatchCloudNotExistRecord001
 * @tc.desc: Test get one batch record with abnormal params.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBSqliteUtilsTest, GetOneBatchCloudNotExistRecord001, TestSize.Level2)
{
    std::vector<SQLiteRelationalUtils::CloudNotExistRecord> record;
    EXPECT_EQ(SQLiteRelationalUtils::GetOneBatchCloudNotExistRecord(CLOUD_SYNC_TABLE_A, nullptr, record, "id"),
        -E_INVALID_DB);
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    std::string sql = "CREATE TABLE IF NOT EXISTS " + DBCommon::GetTmpLogTableName(CLOUD_SYNC_TABLE_A) +
                      "(ID integer primary key autoincrement, cloud_gid TEXT UNIQUE ON CONFLICT IGNORE)";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    ASSERT_EQ(errCode, E_OK);
    sql = std::string("INSERT INTO ") + CLOUD_SYNC_TABLE_A + " VALUES(1, 1, 'str1', 'asset')";
    errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    ASSERT_EQ(errCode, E_OK);
    sql = std::string("UPDATE ") + DBCommon::GetLogTableName(CLOUD_SYNC_TABLE_A) + " SET cloud_gid='gid'";
    errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    ASSERT_EQ(errCode, E_OK);
    RuntimeContext::GetInstance()->SetCloudTranslate(nullptr);
    EXPECT_EQ(SQLiteRelationalUtils::GetOneBatchCloudNotExistRecord(CLOUD_SYNC_TABLE_A, db, record, "stringCol2"),
        -E_NOT_INIT);
}

/**
 * @tc.name: SharedTable001
 * @tc.desc: Test check shared table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBSqliteUtilsTest, SharedTable001, TestSize.Level0)
{
    TableSchema oriTableA = GetTableSchema(info1_, CLOUD_SYNC_TABLE_A);
    const std::string sharedTableA = std::string(CLOUD_SYNC_TABLE_A) + "_shared";
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(nullptr, oriTableA, sharedTableA), -E_INVALID_DB);
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, oriTableA, sharedTableA), E_OK);
    const std::string sharedTableB = "CLOUD_SYNC_TABLE_B_shared";
    // check not exist table and got ok
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, oriTableA, sharedTableB), E_OK);
    TableSchema oriTableB;
    oriTableB.name = "CLOUD_SYNC_TABLE_B";
    // shared table exist but ori table not exist, got invalid args
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, oriTableB, sharedTableA), -E_INVALID_ARGS);
    // check not support table name
    std::string sql = "CREATE TABLE IF NOT EXISTS 'TABLE&'(ID integer primary key autoincrement)";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    const std::string sharedTableC = "TABLE&";
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, oriTableB, sharedTableC), -E_NOT_SUPPORT);
}

std::vector<TableSchema> DistributedDBRDBSqliteUtilsTest::GetTableSchemaVectors() const
{
    TableSchema schema1 = {
        .name = "TABLE3",
        .fields = {
            {
                .colName = std::string("UNKNOWN"),
                .type = -1,
                .nullable = true
            }
        }
    };
    TableSchema schema2 = {
        .name = "TABLE3",
        .fields = {
            {
                .colName = std::string("UNKNOWN"),
                .type = TYPE_INDEX<int64_t>,
                .nullable = true
            }
        }
    };
    TableSchema schema3 = {
        .name = "TABLE3",
        .fields = {
            {
                .colName = std::string("ID"),
                .type = TYPE_INDEX<int64_t>,
                .nullable = false
            }
        }
    };
    TableSchema schema4 = {
        .name = "TABLE3",
        .fields = {
            {
                .colName = std::string("ID"),
                .type = TYPE_INDEX<int64_t>,
                .nullable = true
            }
        }
    };
    std::vector<TableSchema> vecTables = {schema1, schema2, schema3, schema4};
    return vecTables;
}

/**
 * @tc.name: SharedTable002
 * @tc.desc: Test check invalid shared table.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBSqliteUtilsTest, SharedTable002, TestSize.Level0)
{
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    std::string sql = "CREATE TABLE IF NOT EXISTS TABLE1(ID INT primary key)";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    // check shared table without cloud_owner field, got invalid args
    TableSchema oriTable = GetTableSchema(info1_, CLOUD_SYNC_TABLE_A);
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, oriTable, "TABLE1"), -E_INVALID_ARGS);
    // check shared table without cloud_privilege field, got invalid args
    sql = "CREATE TABLE IF NOT EXISTS TABLE2(ID INT primary key, cloud_owner TEXT)";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, oriTable, "TABLE2"), -E_INVALID_ARGS);
    sql = "CREATE TABLE IF NOT EXISTS TABLE3(ID INT primary key, "
          "cloud_owner TEXT, cloud_privilege TEXT)";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    std::vector<TableSchema> vecTables = GetTableSchemaVectors();
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, vecTables[0], "TABLE3"), -E_INVALID_ARGS);
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, vecTables[1], "TABLE3"), -E_INVALID_ARGS);
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, vecTables[2], "TABLE3"), -E_INVALID_ARGS);
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, vecTables[3], "TABLE3"), E_OK);
}

/**
 * @tc.name: SharedTable003
 * @tc.desc: Test check shared table with view.
 * @tc.type: FUNC
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBRDBSqliteUtilsTest, SharedTable003, TestSize.Level0)
{
    auto db = GetSqliteHandle(info1_);
    ASSERT_NE(db, nullptr);
    std::string sql = "CREATE VIEW TEST AS SELECT * FROM sqlite_master";
    ASSERT_EQ(SQLiteUtils::ExecuteRawSQL(db, sql), E_OK);
    TableSchema table = {
        .name = "TEST",
        .fields = {
            {
                .colName = std::string("ID"),
                .type = TYPE_INDEX<int64_t>,
                .nullable = true
            }
        }
    };
    EXPECT_EQ(SQLiteRelationalUtils::CheckUserCreateSharedTable(db, table, "TEST"), -E_NOT_SUPPORT);
}
}
#endif