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

#include <gtest/gtest.h>
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

class DataDonationSqlGeneratorTest : public RDBGeneralUt {
public:
    void SetUp() override;
    void TearDown() override;
    static UtDateBaseSchemaInfo GetTestSchema();
    void PrepareTestData(const StoreInfo &info, int64_t count);
    static UtDateBaseSchemaInfo GetJsonFileSchema();
    void PrepareJsonFileData(const StoreInfo &info, int64_t count);
    void UpdateJsonFileData(const StoreInfo &info, int64_t begin, int64_t count);
    void DeleteJsonFileData(const StoreInfo &info, int64_t begin, int64_t count);

protected:
    DataDonationSqlGenerator generator_;
};

void DataDonationSqlGeneratorTest::SetUp()
{
    RDBGeneralUt::SetUp();
}

void DataDonationSqlGeneratorTest::TearDown()
{
    RDBGeneralUt::TearDown();
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
    return info;
}

void DataDonationSqlGeneratorTest::PrepareTestData(const StoreInfo &info, int64_t count)
{
    for (int64_t i = 0; i < count; ++i) {
        std::string sqlA = "INSERT INTO A VALUES(" + std::to_string(i) + ", 'name_A_" + std::to_string(i) + "')";
        EXPECT_EQ(ExecuteSQL(sqlA, info), E_OK);
        
        std::string sqlB = "INSERT INTO B VALUES(" + std::to_string(i) + ", 'name_B_" + std::to_string(i) + "')";
        EXPECT_EQ(ExecuteSQL(sqlB, info), E_OK);
        
        std::string sqlC = "INSERT INTO C VALUES(" + std::to_string(i) + ", " + std::to_string(i + 18) + ")";
        EXPECT_EQ(ExecuteSQL(sqlC, info), E_OK);
    }
}

void DataDonationSqlGeneratorTest::PrepareJsonFileData(const StoreInfo &info, int64_t count)
{
    for (int64_t i = 0; i < count; ++i) {
        std::string sqlA = "INSERT INTO TableA VALUES(" + std::to_string(i) + ", " + std::to_string(i) +
            ", " + "'title_" + std::to_string(i) + "')";
        EXPECT_EQ(ExecuteSQL(sqlA, info), E_OK);
        
        std::string sqlB = "INSERT INTO TableB VALUES(" + std::to_string(i) + ", " +
            std::to_string(i) + ", " + "'cate_" + std::to_string(i) + "')";
        EXPECT_EQ(ExecuteSQL(sqlB, info), E_OK);
    }
}

void DataDonationSqlGeneratorTest::UpdateJsonFileData(const StoreInfo &info, int64_t begin, int64_t count)
{
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string sqlA = "UPDATE TableA SET title = 'x' where id = " + std::to_string(i);
        EXPECT_EQ(ExecuteSQL(sqlA, info), E_OK);
        
        std::string sqlB = "UPDATE TableB SET category_id = 'x' where id = " + std::to_string(i);
        EXPECT_EQ(ExecuteSQL(sqlB, info), E_OK);
    }
}

void DataDonationSqlGeneratorTest::DeleteJsonFileData(const StoreInfo &info, int64_t begin, int64_t count)
{
    for (int64_t i = begin; i < begin + count; ++i) {
        std::string sqlA = "DELETE FROM TableA where id = " + std::to_string(i);
        EXPECT_EQ(ExecuteSQL(sqlA, info), E_OK);
        
        std::string sqlB = "DELETE FROM TableB where id = " + std::to_string(i);
        EXPECT_EQ(ExecuteSQL(sqlB, info), E_OK);
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
    int errCode = generator_.GenerateQuerySql(path, 0, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.KeyId AS [A.KeyId] FROM A LIMIT 100 OFFSET 0");
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
    int errCode = generator_.GenerateQuerySql(path, 555, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.KeyId AS [A.KeyId], B.KeyId AS [B.KeyId], C.name AS [C.name] FROM A LEFT JOIN B"
        " ON A.id = B.id LEFT JOIN C ON B.id = C.id LIMIT 100 OFFSET 555");
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
    int errCode = generator_.GenerateQuerySql(path, 100, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.name AS [A.name], D.age AS [D.age] FROM A LEFT JOIN B ON A.id = B.id LEFT JOIN C"
        " ON B.id = C.id LEFT JOIN D ON C.id = D.id LIMIT 100 OFFSET 100");
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
    int errCode = generator_.GenerateQuerySql(path, 0, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.KeyId AS [A.KeyId] FROM A LIMIT 100 OFFSET 0");
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
    int errCode = generator_.GenerateQuerySql(path, 1000, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.KeyId AS [A.KeyId] FROM A LIMIT 100 OFFSET 1000");
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
    int errCode = generator_.GenerateQuerySql(path, 0, sql);
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
    int errCode = generator_.GenerateQuerySql(path, 0, sql);
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
    int errCode = generator_.GenerateQuerySql(path, 0, sql);
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
    int errCode = generator_.GenerateQuerySql(path, 0, sql);
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
    int errCode = generator_.GenerateQuerySql(path, 0, sql);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(sql, "SELECT A.name AS [A.name], C.name AS [C.name] FROM A LEFT JOIN B"
        " ON A.id = B.id LEFT JOIN C ON B.id = C.id LIMIT 100 OFFSET 0");
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
    PrepareTestData(storeInfo, dataCount);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    
    DBSubscribeCur cursorIn;
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
    
    DBSubscribeCur cursorIn;
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
    PrepareTestData(storeInfo, dataCount);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);
    
    DBSubscribeCur cursorIn;
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
    
    const int64_t dataCount = 501;
    PrepareJsonFileData(storeInfo, dataCount);
    
    auto delegate = GetDelegate(storeInfo);
    ASSERT_NE(delegate, nullptr);
    EXPECT_EQ(delegate->SetBinlogEnabled(true), OK);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCur cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;
    
    DBSubscribeCur cursorOut;
    std::vector<VBucket> dataOut;
    int64_t totalRecords = 0;
    int64_t incId = 0;
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
            int64_t fileId = 0;
            EXPECT_EQ(CloudStorageUtils::GetValueFromVBucket("TableA.KeyId", vbucket, fileId), E_OK);
            EXPECT_EQ(fileId, incId++);
            EXPECT_EQ(opType, static_cast<int64_t>(SubDataOpType::OP_INSERT));
        }
    } while (status == OK);
    EXPECT_EQ(totalRecords, dataCount);
}

HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData002, TestSize.Level0)
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

    const int64_t dataCount = 501;
    PrepareJsonFileData(storeInfo, dataCount);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCur cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBSubscribeCur cursorOut;
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

    auto db = GetSqliteHandle(storeInfo);
    ASSERT_NE(db, nullptr);
    ASSERT_EQ(SQLiteUtils::SetBinlogEnabled(db, true), E_OK);

    const int64_t dataCount = 501;
    PrepareJsonFileData(storeInfo, dataCount);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCur cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 100;
    
    DBSubscribeCur cursorOut;
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

    auto db = GetSqliteHandle(storeInfo);
    ASSERT_NE(db, nullptr);
    ASSERT_EQ(SQLiteUtils::SetBinlogEnabled(db, true), E_OK);

    int64_t dataCount = 501;
    int64_t updCnt = 100;
    PrepareJsonFileData(storeInfo, dataCount);
    UpdateJsonFileData(storeInfo, 0, updCnt);
    DeleteJsonFileData(storeInfo, 100, updCnt);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCur cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBSubscribeCur cursorOut;
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
    EXPECT_EQ(totalRecords, (dataCount * 2 - updCnt) + updCnt * 2 + updCnt);
}

HWTEST_F(DataDonationSqlGeneratorTest, QueryBinlogSubscribeData005, TestSize.Level0)
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

    int64_t dataCount = 100;
    int64_t updCnt = 100;
    PrepareJsonFileData(storeInfo, dataCount);
    DeleteJsonFileData(storeInfo, 0, updCnt);

    EXPECT_EQ(delegate->SetSubscribeSchema(DataDonationSchemaJsonTest::DATA_DONATION_SCHEMA_JSON), DBStatus::OK);
    
    DBSubscribeCur cursorIn;
    cursorIn.queryType = SubQueryType::GET_NEW;
    cursorIn.cursor = 0;
    
    DBSubscribeCur cursorOut;
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
    MonitorTablesConfig *monitorConfig = RelationalStoreClientUtils::BinlogSchemaGet(dbPath.c_str());
    EXPECT_NE(monitorConfig, nullptr);

    for (int i = 0; i < monitorConfig->tableCount; i++) {
        MonitorTableCol table = monitorConfig->tables[i];
        for (int j = 0; j < table.colCount; j++) {
            free(table.cols[j]);
            table.cols[j] = nullptr;
        }
        free(table.cols);
    }
    free(monitorConfig->tables);
    free(monitorConfig);
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
    MonitorTablesConfig *monitorConfig = RelationalStoreClientUtils::BinlogSchemaGet(nullptr);
    EXPECT_EQ(monitorConfig, nullptr);

    /**
     * @tc.steps:step2. parse schema when db schema is not set.
     * @tc.expected: step2. return nullptr.
     */
    std::string dbPath = BasicUnitTest::GetTestDir() + "/" + STORE_ID_1 + ".db";
    monitorConfig = RelationalStoreClientUtils::BinlogSchemaGet(dbPath.c_str());
    EXPECT_EQ(monitorConfig, nullptr);

    /**
     * @tc.steps:step3. parse schema when db path invalid.
     * @tc.expected: step3. return nullptr.
     */
    std::string invalidDbPath = "not_a_path";
    monitorConfig = RelationalStoreClientUtils::BinlogSchemaGet(invalidDbPath.c_str());
    EXPECT_EQ(monitorConfig, nullptr);
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
    
    DBSubscribeCur cursorIn;
    cursorIn.queryType = SubQueryType::GET_ALL;
    cursorIn.cursor = 0;
    DBSubscribeCur cursorOut;
    std::vector<VBucket> dataOut;
    DBStatus status = delegate->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_NE(status, OK);
}
}
