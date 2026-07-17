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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cloud/cloud_storage_utils.h"
#include "data_donation_sql_generator.h"
#include "distributeddb_data_donation_schema_json.h"
#include "rdb_general_ut.h"
#include "relational_store_client_utils.h"
#include "sqlite_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
string g_storePath;
const string STORE_ID = STORE_ID_1;
const string DB_SUFFIX = ".db";
class DataDonationSqlGeneratorTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
    static UtDateBaseSchemaInfo GetTestSchema();
    void PrepareTestData(sqlite3 *db, int64_t count);
    static UtDateBaseSchemaInfo GetJsonFileSchema();
    void PrepareJsonFileData(sqlite3 *db, int64_t count);
    void PrepareMutiRelationData(sqlite3 *db, int64_t count);
    void InsertDataWithoutMainTable(sqlite3 *db, int64_t count);
    void UpdateJsonFileData(sqlite3 *db, int64_t begin, int64_t count);
    void DeleteJsonFileData(sqlite3 *db, int64_t begin, int64_t count);
    void DeleteFromNonKeyoutTable(sqlite3 *db, int64_t begin, int64_t count);
    std::string InitMatrixFile();

protected:
    DataDonationSqlGenerator generator_;

    sqlite3 *db = nullptr;
    static constexpr size_t MAX_SLOT_NUM = 100;
    static constexpr size_t MATRIX_FILE_SLOT_SIZE = sizeof(uint64_t);
    static constexpr size_t MATRIX_FILE_SIZE = MAX_SLOT_NUM * MATRIX_FILE_SLOT_SIZE;
};

void DataDonationSqlGeneratorTest::SetUp()
{
    RDBGeneralUt::SetUp();
    g_storePath = BasicUnitTest::GetTestDir() + "/" + STORE_ID + DB_SUFFIX;
    LOGD("Test db is %s", g_storePath.c_str());
    db = RelationalTestUtils::CreateDataBase(g_storePath);
    ASSERT_NE(db, nullptr);
}

void DataDonationSqlGeneratorTest::TearDown()
{
    RDBGeneralUt::TearDown();
}

std::string DataDonationSqlGeneratorTest::InitMatrixFile()
{
    std::string matrixFilePath = GetTestDir() + "/matrixFile";

    int fd = open(matrixFilePath.c_str(), O_RDWR | O_CREAT, 0660);
    if (fd == -1) {
        return "";
    }

    int ret = ftruncate(fd, MATRIX_FILE_SIZE);
    close(fd);
    if (ret != 0) {
        unlink(matrixFilePath.c_str());
        return "";
    }
    return matrixFilePath;
}

UtDateBaseSchemaInfo DataDonationSqlGeneratorTest::GetTestSchema()
{
    UtDateBaseSchemaInfo info;
    
    UtTableSchemaInfo tableA;
    tableA.name = "A";
    
    UtFieldInfo idFieldA;
    idFieldA.field.colName = "id";
    idFieldA.field.type = TYPE_INDEX<int64_t>;
    idFieldA.field.primary = true;
    tableA.fieldInfo.push_back(idFieldA);
    
    UtFieldInfo nameFieldA;
    nameFieldA.field.colName = "name";
    nameFieldA.field.type = TYPE_INDEX<std::string>;
    nameFieldA.field.primary = false;
    tableA.fieldInfo.push_back(nameFieldA);
    
    info.tablesInfo.push_back(tableA);
    
    UtTableSchemaInfo tableB;
    tableB.name = "B";
    
    UtFieldInfo idFieldB;
    idFieldB.field.colName = "id";
    idFieldB.field.type = TYPE_INDEX<int64_t>;
    idFieldB.field.primary = true;
    tableB.fieldInfo.push_back(idFieldB);
    
    UtFieldInfo nameFieldB;
    nameFieldB.field.colName = "name";
    nameFieldB.field.type = TYPE_INDEX<std::string>;
    nameFieldB.field.primary = false;
    tableB.fieldInfo.push_back(nameFieldB);
    
    info.tablesInfo.push_back(tableB);
    
    UtTableSchemaInfo tableC;
    tableC.name = "C";
    
    UtFieldInfo idFieldC;
    idFieldC.field.colName = "id";
    idFieldC.field.type = TYPE_INDEX<int64_t>;
    idFieldC.field.primary = true;
    tableC.fieldInfo.push_back(idFieldC);
    
    UtFieldInfo ageFieldC;
    ageFieldC.field.colName = "age";
    ageFieldC.field.type = TYPE_INDEX<int64_t>;
    ageFieldC.field.primary = false;
    tableC.fieldInfo.push_back(ageFieldC);
    
    info.tablesInfo.push_back(tableC);
    
    return info;
}

UtDateBaseSchemaInfo DataDonationSqlGeneratorTest::GetJsonFileSchema()
{
    UtDateBaseSchemaInfo info;
    
    UtTableSchemaInfo tableA;
    tableA.name = "TableA";
    
    UtFieldInfo idFieldA;
    idFieldA.field.colName = "id";
    idFieldA.field.type = TYPE_INDEX<int64_t>;
    idFieldA.field.primary = true;
    tableA.fieldInfo.push_back(idFieldA);
    
    UtFieldInfo fileId;
    fileId.field.colName = "KeyId";
    fileId.field.type = TYPE_INDEX<std::int64_t>;
    tableA.fieldInfo.push_back(fileId);

    UtFieldInfo title;
    title.field.colName = "title";
    title.field.type = TYPE_INDEX<std::string>;
    tableA.fieldInfo.push_back(title);
    info.tablesInfo.push_back(tableA);

    UtFieldInfo cateId;
    cateId.field.colName = "category_id";
    cateId.field.type = TYPE_INDEX<std::string>;
    UtTableSchemaInfo tableB;
    tableB.name = "TableB";
    tableB.fieldInfo.push_back(idFieldA);
    tableB.fieldInfo.push_back(fileId);
    tableB.fieldInfo.push_back(cateId);
    info.tablesInfo.push_back(tableB);

    UtTableSchemaInfo tableC;
    tableC.name = "TableC";
    tableC.fieldInfo.push_back(idFieldA);
    tableC.fieldInfo.push_back(fileId);
    tableC.fieldInfo.push_back(cateId);
    info.tablesInfo.push_back(tableC);
    return info;
}

void DataDonationSqlGeneratorTest::PrepareTestData(sqlite3 *db, int64_t count)
{
    for (int64_t i = 0; i < count; ++i) {
        std::string sqlA = "INSERT INTO A VALUES(" + std::to_string(i) + ", 'name_A_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        
        std::string sqlB = "INSERT INTO B VALUES(" + std::to_string(i) + ", 'name_B_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
        
        std::string sqlC = "INSERT INTO C VALUES(" + std::to_string(i) + ", " + std::to_string(i + 18) + ")";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlC), E_OK);
    }
}

void DataDonationSqlGeneratorTest::PrepareJsonFileData(sqlite3 *db, int64_t count)
{
    for (int64_t i = 0; i < count; ++i) {
        std::string sqlA = "INSERT INTO TableA VALUES(" + std::to_string(i) + ", " + std::to_string(i) +
            ", " + "'title_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        
        std::string sqlB = "INSERT INTO TableB VALUES(" + std::to_string(i) + ", " +
            std::to_string(i) + ", " + "'cate_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
    }
}

void DataDonationSqlGeneratorTest::PrepareMutiRelationData(sqlite3 *db, int64_t count)
{
    for (int64_t i = 0; i < count; ++i) {
        std::string sqlA = "INSERT INTO TableA VALUES(" + std::to_string(i) + ", " + std::to_string(i) +
            ", " + "'title_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        int relationNum = 500;
        for (int64_t j = 0; j < relationNum; ++j) {
            std::string sqlB = "INSERT INTO TableB VALUES(" + std::to_string(i * relationNum + j) + ", " +
            std::to_string(i) + ", " + "'cate_" + std::to_string(i) + "')";
            EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
        }
    }
}

void DataDonationSqlGeneratorTest::InsertDataWithoutMainTable(sqlite3 *db, int64_t count)
{
    for (int64_t i = 0; i < count; ++i) {
        std::string sqlC = "INSERT INTO TableC VALUES(" + std::to_string(i) + ", " +
            std::to_string(i) + ", " + "'cate_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlC), E_OK);
    }
}

void DataDonationSqlGeneratorTest::DeleteFromNonKeyoutTable(sqlite3 *db, int64_t begin, int64_t count)
{
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string sqlC = "DELETE FROM TableC where id = " + std::to_string(i);
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlC), E_OK);
    }
}

void DataDonationSqlGeneratorTest::UpdateJsonFileData(sqlite3 *db, int64_t begin, int64_t count)
{
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string sqlA = "UPDATE TableA SET title = 'x' where id = " + std::to_string(i);
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        
        std::string sqlB = "UPDATE TableB SET category_id = 'x' where id = " + std::to_string(i);
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
    }
}

void DataDonationSqlGeneratorTest::DeleteJsonFileData(sqlite3 *db, int64_t begin, int64_t count)
{
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string sqlA = "DELETE FROM TableA where id = " + std::to_string(i);
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        
        std::string sqlB = "DELETE FROM TableB where id = " + std::to_string(i);
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
    }
}

/**
 * @tc.name: SingleTableTest001
 * @tc.desc: Test generating SQL for single table query (only main table).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, SingleTableTest001, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"A", "id"};
    DataDonationSchema::DdField outputA = {"A", "KeyId"};
    DataDonationSchema::DdField tableB = {"B", "id"};
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, {}};
    DataDonationSchema::DdRelationsPath path = {"A", {relationAB}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues;
    std::vector<std::pair<std::string, int64_t>> maxRowids = {{"A", 50000}};
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.KeyId AS [A.KeyId], A._rowid_ AS [A._rowid_] FROM A WHERE A._rowid_ <= 50000"
        " ORDER BY A._rowid_ ASC LIMIT 1000");
}

/**
 * @tc.name: TwoTableJoinTest001
 * @tc.desc: Test generating SQL for two table join query.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, TwoTableJoinTest001, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"A", "id"};
    DataDonationSchema::DdField outputA = {"A", "KeyId"};
    DataDonationSchema::DdField tableB = {"B", "id"};
    DataDonationSchema::DdField outputB = {"B", "KeyId"};
    DataDonationSchema::DdField tableC = {"C", "id"};
    DataDonationSchema::DdField outputC = {"C", "name"};
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdForeignKey tableBC = {tableB, tableC};
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, outputB};
    DataDonationSchema::DdRelation relationBC = {tableBC, {}, outputC};
    DataDonationSchema::DdRelationsPath path = {"A", {relationAB, relationBC}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues = {{"A", 555}, {"B", 100}, {"C", 500}};
    std::vector<std::pair<std::string, int64_t>> maxRowids = {{"A", 50000}, {"B", 2000}, {"C", 4000}};
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.KeyId AS [A.KeyId], B.KeyId AS [B.KeyId], C.name AS [C.name],"
        " A._rowid_ AS [A._rowid_], B._rowid_ AS [B._rowid_], C._rowid_ AS [C._rowid_]"
        " FROM A LEFT JOIN B ON A.id = B.id LEFT JOIN C ON B.id = C.id"
        " WHERE ((A._rowid_ > 555) OR (A._rowid_ = 555 AND B._rowid_ > 100)"
        " OR (A._rowid_ = 555 AND B._rowid_ = 100 AND C._rowid_ > 500))"
        " AND A._rowid_ <= 50000 AND (B._rowid_ IS NULL OR B._rowid_ <= 2000)"
        " AND (C._rowid_ IS NULL OR C._rowid_ <= 4000)"
        " ORDER BY A._rowid_ ASC LIMIT 1000");
}

/**
 * @tc.name: MultiTableJoinTest001
 * @tc.desc: Test generating SQL for multi table join query (A -> B -> C -> D).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, MultiTableJoinTest001, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"A", "id"};
    DataDonationSchema::DdField outputA = {"A", "name"};
    DataDonationSchema::DdField tableB = {"B", "id"};
    DataDonationSchema::DdField tableC = {"C", "id"};
    DataDonationSchema::DdField tableD = {"D", "id"};
    DataDonationSchema::DdField outputD = {"D", "age"};
    
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdForeignKey tableBC = {tableB, tableC};
    DataDonationSchema::DdForeignKey tableCD = {tableC, tableD};
    
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, {}};
    DataDonationSchema::DdRelation relationBC = {tableBC, {}, {}};
    DataDonationSchema::DdRelation relationCD = {tableCD, {}, outputD};
    
    DataDonationSchema::DdRelationsPath path = {"A", {relationAB, relationBC, relationCD}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues = {{"A", 100}, {"B", 50}, {"C", 200}, {"D", 300}};
    std::vector<std::pair<std::string, int64_t>> maxRowids = {{"A", 50000}, {"B", 3000}, {"C", 2000}, {"D", 1000}};
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.name AS [A.name], D.age AS [D.age],"
        " A._rowid_ AS [A._rowid_], B._rowid_ AS [B._rowid_], C._rowid_ AS [C._rowid_], D._rowid_ AS [D._rowid_]"
        " FROM A LEFT JOIN B ON A.id = B.id LEFT JOIN C ON B.id = C.id LEFT JOIN D ON C.id = D.id"
        " WHERE ((A._rowid_ > 100) OR (A._rowid_ = 100 AND B._rowid_ > 50)"
        " OR (A._rowid_ = 100 AND B._rowid_ = 50 AND C._rowid_ > 200)"
        " OR (A._rowid_ = 100 AND B._rowid_ = 50 AND C._rowid_ = 200 AND D._rowid_ > 300))"
        " AND A._rowid_ <= 50000 AND (B._rowid_ IS NULL OR B._rowid_ <= 3000)"
        " AND (C._rowid_ IS NULL OR C._rowid_ <= 2000)"
        " AND (D._rowid_ IS NULL OR D._rowid_ <= 1000)"
        " ORDER BY A._rowid_ ASC LIMIT 1000");
}

/**
 * @tc.name: EmptyForeignFieldTest001
 * @tc.desc: Test generating SQL when foreignField is empty (only query main table).
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, EmptyForeignFieldTest001, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"A", "id"};
    DataDonationSchema::DdField outputA = {"A", "KeyId"};
    DataDonationSchema::DdField tableB = {"B", "id"};
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, {}};
    DataDonationSchema::DdRelationsPath path = {"A", {relationAB}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues;
    std::vector<std::pair<std::string, int64_t>> maxRowids = {{"A", 50000}};
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.KeyId AS [A.KeyId], A._rowid_ AS [A._rowid_] FROM A WHERE A._rowid_ <= 50000"
        " ORDER BY A._rowid_ ASC LIMIT 1000");
}

/**
 * @tc.name: PaginationTest001
 * @tc.desc: Test generating SQL with different pagination parameters.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, PaginationTest001, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"A", "id"};
    DataDonationSchema::DdField outputA = {"A", "KeyId"};
    DataDonationSchema::DdField tableB = {"B", "id"};
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, {}};
    DataDonationSchema::DdRelationsPath path = {"A", {relationAB}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues = {{"A", 1000}};
    std::vector<std::pair<std::string, int64_t>> maxRowids = {{"A", 50000}};
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.KeyId AS [A.KeyId], A._rowid_ AS [A._rowid_] FROM A WHERE ((A._rowid_ > 1000))"
        " AND A._rowid_ <= 50000 ORDER BY A._rowid_ ASC LIMIT 1000");
}

/**
 * @tc.name: InvalidPathTest001
 * @tc.desc: Test generating SQL with empty table name.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, InvalidPathTest001, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"A", "id"};
    DataDonationSchema::DdField outputA = {"A", "KeyId"};
    DataDonationSchema::DdField tableB = {"B", "id"};
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, {}};
    DataDonationSchema::DdRelationsPath path = {"", {relationAB}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues;
    std::vector<std::pair<std::string, int64_t>> maxRowids;
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: InvalidPathTest002
 * @tc.desc: Test generating SQL with empty relations.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, InvalidPathTest002, TestSize.Level0)
{
    DataDonationSchema::DdRelationsPath path = {"A", {}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues;
    std::vector<std::pair<std::string, int64_t>> maxRowids;
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: InvalidPathTest003
 * @tc.desc: Test generating SQL with invalid local field.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, InvalidPathTest003, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"", "id"};
    DataDonationSchema::DdField outputA = {"A", "KeyId"};
    DataDonationSchema::DdField tableB = {"B", "id"};
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, {}};
    DataDonationSchema::DdRelationsPath path = {"A", {relationAB}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues;
    std::vector<std::pair<std::string, int64_t>> maxRowids;
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: InvalidPathTest004
 * @tc.desc: Test generating SQL with invalid foreign field in key.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, InvalidPathTest004, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"A", "id"};
    DataDonationSchema::DdField outputA = {"A", "KeyId"};
    DataDonationSchema::DdField tableB = {"", "id"};
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, {}};
    DataDonationSchema::DdRelationsPath path = {"A", {relationAB}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues;
    std::vector<std::pair<std::string, int64_t>> maxRowids;
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, -E_INVALID_ARGS);
}

/**
 * @tc.name: MultipleFieldsTest001
 * @tc.desc: Test generating SQL with multiple fields to query.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, MultipleFieldsTest001, TestSize.Level0)
{
    DataDonationSchema::DdField tableA = {"A", "id"};
    DataDonationSchema::DdField outputA = {"A", "name"};
    DataDonationSchema::DdField tableB = {"B", "id"};
    DataDonationSchema::DdField tableC = {"C", "id"};
    DataDonationSchema::DdField outputCName = {"C", "name"};
    DataDonationSchema::DdField outputCAge = {"C", "age"};
    
    DataDonationSchema::DdForeignKey tableAB = {tableA, tableB};
    DataDonationSchema::DdForeignKey tableBC = {tableB, tableC};
    
    DataDonationSchema::DdRelation relationAB = {tableAB, outputA, {}};
    DataDonationSchema::DdRelation relationBC1 = {tableBC, {}, outputCName};
    
    DataDonationSchema::DdRelationsPath path = {"A", {relationAB, relationBC1}};

    std::string sql;
    std::vector<std::pair<std::string, int64_t>> cursorValues;
    std::vector<std::pair<std::string, int64_t>> maxRowids = {{"A", 50000}, {"B", 2000}, {"C", 4000}};
    int errCode = generator_.GenerateQuerySql(path, cursorValues, maxRowids, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.name AS [A.name], C.name AS [C.name],"
        " A._rowid_ AS [A._rowid_], B._rowid_ AS [B._rowid_], C._rowid_ AS [C._rowid_]"
        " FROM A LEFT JOIN B ON A.id = B.id LEFT JOIN C ON B.id = C.id"
        " WHERE A._rowid_ <= 50000 AND (B._rowid_ IS NULL OR B._rowid_ <= 2000)"
        " AND (C._rowid_ IS NULL OR C._rowid_ <= 4000)"
        " ORDER BY A._rowid_ ASC LIMIT 1000");
}

/**
 * @tc.name: SetSubscribeCursorBasicTest001
 * @tc.desc: Test basic functionality of SetSubscribeCursor interface.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, SetSubscribeCursorBasicTest001, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetTestSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    const int64_t dataCount = 10;
    PrepareTestData(db, dataCount);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBStatus status = delegate->SetSubscribeCursor(cursorIn);
    EXPECT_EQ(status, OK);
}

/**
 * @tc.name: SetSubscribeCursorNotSupportTest001
 * @tc.desc: Test SetSubscribeCursor interface returns OK when queryType is GET_ALL.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, SetSubscribeCursorNotSupportTest001, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetTestSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;
    
    DBStatus status = delegate->SetSubscribeCursor(cursorIn);
    EXPECT_EQ(status, OK);
}

/**
 * @tc.name: SetSubscribeCursorDifferentCursorTest001
 * @tc.desc: Test SetSubscribeCursor interface with different cursor values.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, SetSubscribeCursorDifferentCursorTest001, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetTestSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    const int64_t dataCount = 10;
    PrepareTestData(db, dataCount);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 100;
    DBStatus status = delegate->SetSubscribeCursor(cursorIn);
    EXPECT_EQ(status, OK);
    
    cursorIn.cursor = 100;
    status = delegate->SetSubscribeCursor(cursorIn);
    EXPECT_EQ(status, OK);
}

HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData001, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    const int64_t dataCount = CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL + 1;
    PrepareJsonFileData(db, dataCount);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;
    
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    int64_t incId = 0;
    DBStatus status = DBStatus::OK;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        EXPECT_EQ(status, dataOut.size() < CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL ? SUBSCRIBE_QUERY_END : OK);
        totalRecords = totalRecords + static_cast<int64_t>(dataOut.size());
        cursorIn = cursorOut;
        for (const auto &vbucket : dataOut) {
            int64_t opType = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::SUB_DATA_OP_TYPE, vbucket, opType), E_OK);
            int64_t fileId = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket("TableA.KeyId", vbucket, fileId), E_OK);
            EXPECT_EQ(fileId, incId++);
            EXPECT_EQ(opType, static_cast<int64_t>(SubDataOpType::OP_INSERT));
        }
    } while (status == OK);
    EXPECT_EQ(totalRecords, dataCount);
    EXPECT_EQ(delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut), INVALID_ARGS);
}

HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData002, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);
    const int64_t dataCount = 501;
    PrepareJsonFileData(db, dataCount);

    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    DBStatus status = DBStatus::OK;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        EXPECT_EQ(status, dataOut.size() < CloudDbConstant::SUBSCRIBE_QUERY_LIMIT ? SUBSCRIBE_QUERY_END : OK);
        totalRecords = totalRecords + static_cast<int64_t>(dataOut.size());
        cursorIn = cursorOut;
        for (const auto &vbucket : dataOut) {
            int64_t opType = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::SUB_DATA_OP_TYPE, vbucket, opType), E_OK);
            EXPECT_EQ(opType, static_cast<int64_t>(SubDataOpType::OP_INSERT));
        }
        EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
    } while (status == OK);
    EXPECT_EQ(totalRecords, 2 * dataCount);
}

HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData003, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);
    const int64_t dataCount = 501;
    PrepareJsonFileData(db, dataCount);

    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 100;
    
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    DBStatus status = DBStatus::OK;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        totalRecords = totalRecords + static_cast<int64_t>(dataOut.size());
        cursorIn = cursorOut;
        for (const auto &vbucket : dataOut) {
            int64_t opType = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::SUB_DATA_OP_TYPE, vbucket, opType), E_OK);
            EXPECT_EQ(opType, static_cast<int64_t>(SubDataOpType::OP_INSERT));
        }
        EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
    } while (status == OK);
    EXPECT_EQ(totalRecords, dataCount * 2);
}

HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData004, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);
    int64_t dataCount = 501;
    int64_t updCnt = 100;
    PrepareJsonFileData(db, dataCount);
    UpdateJsonFileData(db, 0, updCnt);
    DeleteJsonFileData(db, 100, updCnt);

    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    int idx = 0;
    DBStatus status = DBStatus::OK;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        totalRecords = totalRecords + static_cast<int64_t>(dataOut.size());
        cursorIn = cursorOut;
        for (const auto &vbucket : dataOut) {
            int64_t opType = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::SUB_DATA_OP_TYPE, vbucket, opType), E_OK);
            idx++;
        }
        EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
    } while (status == OK);
    EXPECT_EQ(totalRecords, dataCount * 2);
}

HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData005, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);
    int64_t dataCount = 100;
    int64_t updCnt = 100;
    PrepareJsonFileData(db, dataCount);
    DeleteJsonFileData(db, 0, updCnt);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    int idx = 0;
    DBStatus status = DBStatus::OK;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        EXPECT_TRUE(status == SUBSCRIBE_QUERY_END || status == OK);
        totalRecords = totalRecords + static_cast<int64_t>(dataOut.size());
        cursorIn = cursorOut;
        for (const auto &vbucket : dataOut) {
            int64_t opType = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::SUB_DATA_OP_TYPE, vbucket, opType), E_OK);
            EXPECT_EQ(opType, static_cast<int64_t>(SubDataOpType::OP_DELETE));
            idx++;
        }
        EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
    } while (status == OK);

    EXPECT_EQ(status, SUBSCRIBE_QUERY_END);
    EXPECT_EQ(totalRecords, dataCount * 2);
}

/**
 * @tc.name: ClientSchemaParseTest001
 * @tc.desc: Test binlog parse schema.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, ClientSchemaParseTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. set schema on service side.
     * @tc.expected: step1. OK.
     */
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);

    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);

    /**
     * @tc.steps:step2. parse schema on client side.
     * @tc.expected: step2. OK.
     */
    std::string dbPath = BasicUnitTest::GetTestDir() + "/" + STORE_ID_1 + ".db";
    MonitorTablesConfig *monitorConfig = DataDonationUtils::BinlogSchemaGet(dbPath.c_str());
    EXPECT_NE(monitorConfig, nullptr);
    EXPECT_EQ(monitorConfig->tableCount, 9);

    DataDonationUtils::FreeMonitorConfig(monitorConfig);
}

/**
 * @tc.name: ClientSchemaParseError001
 * @tc.desc: Test binlog parse schema on error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, ClientSchemaParseError001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. parse schema when db path is null.
     * @tc.expected: step1. return nullptr.
     */
    MonitorTablesConfig *monitorConfig = DataDonationUtils::BinlogSchemaGet(nullptr);
    EXPECT_EQ(monitorConfig, nullptr);

    /**
     * @tc.steps:step2. parse schema when db schema is not set.
     * @tc.expected: step2. return nullptr.
     */
    std::string dbPath = BasicUnitTest::GetTestDir() + "/" + STORE_ID_1 + ".db";
    monitorConfig = DataDonationUtils::BinlogSchemaGet(dbPath.c_str());
    EXPECT_EQ(monitorConfig, nullptr);

    /**
     * @tc.steps:step3. parse schema when db path invalid.
     * @tc.expected: step3. return nullptr.
     */
    std::string invalidDbPath = "not_a_path";
    monitorConfig = DataDonationUtils::BinlogSchemaGet(invalidDbPath.c_str());
    EXPECT_EQ(monitorConfig, nullptr);
    DataDonationUtils::FreeMonitorConfig(monitorConfig);
}

/**
 * @tc.name: QueryBinlogSubscribeData006
 * @tc.desc: Test QuerySubscribeOutput interface with not enabled binlog.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData006, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_NE(status, OK);
}

/**
 * @tc.name: QueryBinlogSubscribeData007
 * @tc.desc: Test QuerySubscribeOutput GET_NEW when no data in main table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData007, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Set subscribe schema.
     * @tc.expected: step1. OK.
     */
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);

    /**
     * @tc.steps:step2. Insert 10 data to table C only, main table is empty
     * @tc.expected: step2. OK.
     */
    const int64_t dataCount = 10;
    InsertDataWithoutMainTable(db, dataCount);

    /**
     * @tc.steps:step3. Query using GET_NEW
     * @tc.expected: step3. data out is empty.
     */
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(status, SUBSCRIBE_QUERY_END);
    EXPECT_TRUE(dataOut.empty());
}

/**
 * @tc.name: QueryBinlogSubscribeData008
 * @tc.desc: Test QuerySubscribeOutput GET_NEW when primaryKey not foreginKey.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData008, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Set subscribe schema.
     * @tc.expected: step1. OK.
     */
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);

    /**
     * @tc.steps:step2. Insert 10 data to table A and B
     * @tc.expected: step2. OK.
     */
    const int64_t dataCount = 10;
    PrepareJsonFileData(db, dataCount);

    /**
     * @tc.steps:step3. Query using GET_NEW
     * @tc.expected: step3. data out is not empty.
     */
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(status, SUBSCRIBE_QUERY_END);
    EXPECT_FALSE(dataOut.empty());
}

/**
 * @tc.name: QueryBinlogSubscribeData009
 * @tc.desc: Test QuerySubscribeOutput GET_NEW when data deleted.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData009, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    const int64_t dataCount = 10;
    for (int64_t i = 0; i < dataCount; ++i) {
        std::string sqlA = "INSERT INTO TableA VALUES(" + std::to_string(i) + ", " + std::to_string(i) +
            ", " + "'title_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        
        std::string sqlB = "DELETE FROM TableA WHERE id=" + std::to_string(i);
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
    }
    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = DBStatus::OK;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        EXPECT_EQ(status, dataOut.size() < CloudDbConstant::SUBSCRIBE_QUERY_LIMIT ? SUBSCRIBE_QUERY_END : OK);
        cursorIn = cursorOut;
        for (const auto &vbucket : dataOut) {
            int64_t opType = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket(CloudDbConstant::SUB_DATA_OP_TYPE, vbucket, opType), E_OK);
            EXPECT_EQ(opType, static_cast<int64_t>(SubDataOpType::OP_DELETE));
            int64_t val = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket("TableA.KeyId", vbucket, val), E_OK);
            Type cloudValue;
            EXPECT_EQ(CloudStorageUtils::GetTypeCaseInsensitive("TableB.Id", vbucket, cloudValue), true);
        }
        EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
    } while (status == OK);
}

/**
 * @tc.name: QueryBinlogSubscribeData010
 * @tc.desc: Test QuerySubscribeOutput GET_ALL when data deleted.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData010, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    const int64_t dataCount = CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL * 2;
    PrepareJsonFileData(db, dataCount);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;
    
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    DBStatus status = DBStatus::OK;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        totalRecords = totalRecords + static_cast<int64_t>(dataOut.size());
        cursorIn = cursorOut;
        if (totalRecords == CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL) {
            DeleteJsonFileData(db, 500, 500);
        }
    } while (status == OK);
    EXPECT_EQ(totalRecords, dataCount);
}

/**
 * @tc.name: QueryBinlogSubscribeData011
 * @tc.desc: Test QuerySubscribeOutput GET_NEW when data deleted from non-keyOut table.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData011, TestSize.Level0)
{
    /**
     * @tc.steps:step1. set binlog and schema.
     * @tc.expected: step1. OK.
     */
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);

    /**
     * @tc.steps:step2. Insert 10 data to 3 tables, then delete them.
     * @tc.expected: step2. OK.
     */
    const int64_t dataCount = 10;
    const int keyOutNum = 2;
    PrepareJsonFileData(db, dataCount);
    InsertDataWithoutMainTable(db, dataCount);
    DeleteJsonFileData(db, 0, dataCount);
    DeleteFromNonKeyoutTable(db, 0, dataCount);

    /**
     * @tc.steps:step3. Query using GET_NEW, only contain delete records for keyOut tables.
     * @tc.expected: step3. OK.
     */
    DBSubscribeCursor cursorIn = {.queryType = SubQueryType::GET_NEW, .cursor = 0};
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(status, SUBSCRIBE_QUERY_END);
    EXPECT_EQ(dataOut.size(), dataCount * keyOutNum);
}

/**
 * @tc.name: QueryBinlogSubscribeData012
 * @tc.desc: Test QuerySubscribeOutput one mainTable data relation muti subTable data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData012, TestSize.Level0)
{
    /**
     * @tc.steps:step1. set binlog and schema.
     * @tc.expected: step1. OK.
     */
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);

    /**
     * @tc.steps:step2. Insert 5000 data to 2 tables.
     * @tc.expected: step2. OK.
     */
    const int64_t dataCount = 10;
    PrepareMutiRelationData(db, dataCount);

    /**
     * @tc.steps:step3. Query using GET_NEW, has muti relation data.
     * @tc.expected: step3. OK.
     */
    DBSubscribeCursor cursorIn = {.queryType = SubQueryType::GET_NEW, .cursor = 0};
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int donateCount = 0;
    DBStatus status;
    do {
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        cursorIn = cursorOut;
        EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
        donateCount = donateCount + dataOut.size();
        dataOut.clear();
    } while (status == OK);
    int expectCount = 5060;
    EXPECT_EQ(donateCount, expectCount);
}

/**
 * @tc.name: QueryWithSameCursorTest001
 * @tc.desc: Test QuerySubscribeOutput GET_NEW with same cursor twice.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryWithSameCursorTest001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Set subscribe schema.
     * @tc.expected: step1. OK.
     */
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);

    /**
     * @tc.steps:step2. Insert 101 data
     * @tc.expected: step2. OK.
     */
    const int64_t dataCount = 101;
    PrepareJsonFileData(db, dataCount);

    /**
     * @tc.steps:step3. Query using GET_NEW
     * @tc.expected: step3. OK.
     */
    DBSubscribeCursor cursorIn = {.queryType = SubQueryType::GET_NEW, .cursor = 0};
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(dataOut.size(), 100);
    EXPECT_EQ(cursorOut.cursor, 100);

    /**
     * @tc.steps:step4. Query again using same cursor
     * @tc.expected: step4. data is the same as first time.
     */
    dataOut.clear();
    cursorOut.cursor = 0;
    status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(status, OK);
    EXPECT_EQ(dataOut.size(), 100);
    EXPECT_EQ(cursorOut.cursor, 100);
}

/**
 * @tc.name: QueryBinlogSubscribeData013
 * @tc.desc: Test get ALL query where joined tables require continued querying
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData013, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    const int64_t dataCount = 2001;
    for (int64_t i = 0; i < dataCount; ++i) {
        std::string sqlA = "INSERT INTO TableA VALUES(" + std::to_string(i) + ", " + std::to_string(i) +
            ", " + "'title_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        std::string sqlB = "INSERT INTO TableB VALUES(" + std::to_string(i) + ", " +
            std::to_string(i % 2) + ", " + "'cate_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
    }
    for (int64_t i = 10000; i < 10000 + dataCount; ++i) {
        std::string sqlA = "INSERT INTO TableA VALUES(" + std::to_string(i) + ", " + std::to_string(i) +
            ", " + "'title_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        std::string sqlB = "INSERT INTO TableB VALUES(" + std::to_string(i) + ", " +
            std::to_string(22000 - i) + ", " + "'cate_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
    }
    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;

    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = DBStatus::OK;
    size_t totalRecords = 0;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        cursorIn = cursorOut;
        totalRecords = totalRecords + dataOut.size();
        EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
    } while (status == OK);
    size_t expectRecords = 6001;
    EXPECT_EQ(totalRecords, expectRecords);
}

/**
 * @tc.name: QueryBinlogSubscribeData014
 * @tc.desc: Test GET_ALL query without set cursor but after reopening the db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData014, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    const int64_t dataCount = CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL * 2;
    PrepareJsonFileData(db, dataCount);
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    DBStatus status = DBStatus::OK;
    int idx = 0;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        totalRecords = totalRecords + static_cast<int64_t>(dataOut.size());
        cursorIn = cursorOut;
        if (idx == 0) {
            CloseAllDelegate();
            ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
            delegate = GetDelegate(storeInfo);
            ASSERT_NE(delegate, nullptr);
            EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
            EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON),
                DBStatus::OK);
        }
        idx++;
    } while (status == OK);
    EXPECT_EQ(totalRecords, dataCount + CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL);
}

/**
 * @tc.name: QueryBinlogSubscribeData015
 * @tc.desc: Test GET_ALL query after reopening the db
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData015, TestSize.Level0)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    const int64_t dataCount = CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL * 3;
    PrepareJsonFileData(db, dataCount);
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    DBStatus status = DBStatus::OK;
    int idx = 0;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        totalRecords = totalRecords + static_cast<int64_t>(dataOut.size());
        cursorIn = cursorOut;
        if (idx == 1) {
            CloseAllDelegate();
            ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
            delegate = GetDelegate(storeInfo);
            ASSERT_NE(delegate, nullptr);
            EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
            EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON),
                DBStatus::OK);
        } else {
            EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
        }
        idx++;
    } while (status == OK);
    EXPECT_EQ(cursorOut.cursor, dataCount - 1);
    EXPECT_EQ(totalRecords, dataCount + CloudDbConstant::SUBSCRIBE_QUERY_LIMIT_GET_ALL);
}

/**
 * @tc.name: QueryBinlogSubscribeData016
 * @tc.desc: Test get ALL query where joined tables require continued querying
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData016, TestSize.Level2)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);
    const int64_t count = 2;
    for (int64_t i = 0; i < count; ++i) {
        std::string sqlA = "INSERT INTO TableA VALUES(" + std::to_string(i) + ", " + std::to_string(i) +
            ", " + "'title_" + std::to_string(i) + "')";
        EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlA), E_OK);
        int relationNum = 12000;
        for (int64_t j = 0;  j < relationNum; ++j) {
            std::string sqlB = "INSERT INTO TableB VALUES(" + std::to_string(i * relationNum + j) + ", " +
            std::to_string(i) + ", " + "'cate_" + std::to_string(i) + "')";
            EXPECT_EQ(SQLiteUtils::ExecuteRawSQL(db, sqlB), E_OK);
        }
    }
    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;

    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = DBStatus::OK;
    size_t totalRecords = 0;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        cursorIn = cursorOut;
        totalRecords = totalRecords + dataOut.size();
        EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
    } while (status == OK);
    size_t expectRecords = 24240;
    EXPECT_EQ(totalRecords, expectRecords);
}

/**
 * @tc.name: SetBinlogEnabled002
 * @tc.desc: Test disable binlog.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, SetBinlogEnabled002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Init delegate and disable binlog.
     * @tc.expected: step1. Return OK.
     */
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "device1"), E_OK);
    auto *delegate = GetDelegate(info1);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(false), OK);
    EXPECT_EQ(RDBGeneralUt::CloseDelegate(info1), E_OK);
}

/**
 * @tc.name: SetBinlogEnabled003
 * @tc.desc: Test enable then disable binlog.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, SetBinlogEnabled003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Init delegate, enable then disable binlog.
     * @tc.expected: step1. Both return OK.
     */
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "device1"), E_OK);
    auto *delegate = GetDelegate(info1);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(delegate->SetBinlogEnabled(false), OK);
    EXPECT_EQ(RDBGeneralUt::CloseDelegate(info1), E_OK);
}

/**
 * @tc.name: SetBinlogEnabled004
 * @tc.desc: Test repeatedly enable binlog.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, SetBinlogEnabled004, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Init delegate, enable binlog twice.
     * @tc.expected: step1. Both return OK.
     */
    StoreInfo info1 = {USER_ID, APP_ID, STORE_ID_1};
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "device1"), E_OK);
    auto *delegate = GetDelegate(info1);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    EXPECT_EQ(RDBGeneralUt::CloseDelegate(info1), E_OK);
}

/**
 * @tc.name: SetBinlogEnabled005
 * @tc.desc: Test enable binlog then insert data works normally.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, SetBinlogEnabled005, TestSize.Level1)
{
    /**
     * @tc.steps: step1. cloud insert data.
     * @tc.expected: step1. Ok
     */
    RelationalStoreDelegate::Option option;
    option.tableMode = DistributedTableMode::COLLABORATION;
    SetOption(option);
    auto info1 = GetStoreInfo1();
    ASSERT_EQ(BasicUnitTest::InitDelegate(info1, "device1"), E_OK);
    auto *delegate = GetDelegate(info1);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
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
 * @tc.name: QueryBinlogSubscribeData018
 * @tc.desc: Test query old cursor.
 * @tc.type: FUNC
 * @tc.author: test
 */
HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData018, TestSize.Level2)
{
    StoreInfo storeInfo = {USER_ID, APP_ID, STORE_ID_1};
    SetSchemaInfo(storeInfo, GetJsonFileSchema());
    ASSERT_EQ(BasicUnitTest::InitDelegate(storeInfo, "device1"), E_OK);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    auto db = GetSqliteHandle(storeInfo);
    ASSERT_NE(db, nullptr);
    ASSERT_EQ(SQLiteUtils::SetBinlogEnabled(db, true), E_OK);
    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    SetBinlogSchemaAndChangeCallback(db);
    const int64_t dataCount = 4000;
    PrepareJsonFileData(db, dataCount);

    DBSubscribeCursor cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBSubscribeCursor cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = DBStatus::OK;
    int idx = 0;
    do {
        dataOut = {};
        status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
        EXPECT_EQ(status, dataOut.size() < CloudDbConstant::SUBSCRIBE_QUERY_LIMIT ? SUBSCRIBE_QUERY_END : OK);
        cursorIn = cursorOut;
        idx++;
        if (idx == 2 || idx == 9 || idx == 17) {
            EXPECT_EQ(delegate->SetSubscribeCursor(cursorIn), OK);
        }
        if (idx == 27) {
            cursorIn.cursor -= 100;
        }
    } while (status == OK);
}
}
