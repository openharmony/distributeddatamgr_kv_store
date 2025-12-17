/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "data_transformer.h"
#include "data_value.h"
#include "db_errno.h"
#include "distributeddb/result_set.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "relational_result_set_impl.h"
#include "relational_row_data.h"
#include "relational_row_data_impl.h"
#include "relational_row_data_set.h"
#include "relational_store_sqlite_ext.h"
#include "types_export.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
string g_storePath;
string g_tableName { "data" };

const vector<uint8_t> BLOB_VALUE { 'a', 'b', 'l', 'o', 'b', '\0', 'e', 'n', 'd' };
const double DOUBLE_VALUE = 1.123456;  // 1.123456 for test
const int64_t INT64_VALUE = 123456;  // 123456 for test
const std::string STR_VALUE = "I'm a string.";

DataValue g_blobValue;
DataValue g_doubleValue;
DataValue g_int64Value;
DataValue g_nullValue;
DataValue g_strValue;

void InitGlobalValue()
{
    Blob *blob = new (std::nothrow) Blob();
    blob->WriteBlob(BLOB_VALUE.data(), BLOB_VALUE.size());
    g_blobValue.Set(blob);

    g_doubleValue = DOUBLE_VALUE;
    g_int64Value = INT64_VALUE;
    g_strValue = STR_VALUE;
}

void CreateDBAndTable()
{
    sqlite3 *db = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        LOGE("open db failed:%d", errCode);
        sqlite3_close(db);
        return;
    }

    const string sql =
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    char *zErrMsg = nullptr;
    errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg);
    if (errCode != SQLITE_OK) {
        LOGE("sql error:%s", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_close(db);
}
}

class DistributedDBRelationalResultSetTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalResultSetTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    InitGlobalValue();
    g_storePath = g_testDir + "/relationalResultSetTest.db";
    LOGI("The test db is:%s", g_testDir.c_str());
}

void DistributedDBRelationalResultSetTest::TearDownTestCase(void) {}

void DistributedDBRelationalResultSetTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    CreateDBAndTable();
}

void DistributedDBRelationalResultSetTest::TearDown(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

/**
 * @tc.name: SerializeAndDeserialize
 * @tc.desc: Serialize and deserialize.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalResultSetTest, SerializeAndDeserialize, TestSize.Level1)
{
    RowData data1 {g_strValue, g_int64Value, g_doubleValue, g_blobValue, g_nullValue};
    RowData data2 {g_nullValue, g_blobValue, g_doubleValue, g_int64Value, g_strValue};  // data1.reverse()

    auto rowDataImpl1 = new (std::nothrow) RelationalRowDataImpl(std::move(data1));
    auto rowDataImpl2 = new (std::nothrow) RelationalRowDataImpl(std::move(data2));

    /**
     * @tc.steps: step1. Create a row data set which contains two row data;
     * @tc.expected: OK.
     */
    RelationalRowDataSet rowDataSet1;
    rowDataSet1.SetColNames({"column 1", "column 2", "column 3", "column 4", "column 5"});
    rowDataSet1.Insert(rowDataImpl1);
    rowDataSet1.Insert(rowDataImpl2);

    /**
     * @tc.steps: step2. Serialize the row data set;
     * @tc.expected: Serialize OK.
     */
    auto bufferLength = rowDataSet1.CalcLength();
    vector<uint8_t> buffer1(bufferLength);
    Parcel parcel1(buffer1.data(), buffer1.size());
    ASSERT_EQ(rowDataSet1.Serialize(parcel1), E_OK);
    ASSERT_FALSE(parcel1.IsError());

    /**
     * @tc.steps: step3. Deserialize the row data set;
     * @tc.expected: Deserialize OK.
     */
    vector<uint8_t> buffer2 = buffer1;
    Parcel parcel2(buffer2.data(), buffer2.size());
    RelationalRowDataSet rowDataSet2;
    ASSERT_EQ(rowDataSet2.DeSerialize(parcel2), E_OK);
    ASSERT_FALSE(parcel2.IsError());

    /**
     * @tc.steps: step4. Compare the deserialized row data set with the original;
     * @tc.expected: Compare OK. They are the same.
     */
    std::vector<std::string> colNames {"column 1", "column 2", "column 3", "column 4", "column 5"};
    EXPECT_EQ(rowDataSet2.GetColNames(), colNames);

    // test the second row. if the second row ok, the first row is likely ok
    StorageType type = StorageType::STORAGE_TYPE_NONE;
    EXPECT_EQ(rowDataSet2.Get(1)->GetType(0, type), E_OK);
    EXPECT_EQ(type, StorageType::STORAGE_TYPE_NULL);

    vector<uint8_t> desBlob;
    EXPECT_EQ(rowDataSet2.Get(1)->Get(1, desBlob), E_OK);
    EXPECT_EQ(desBlob, BLOB_VALUE);

    double desDoub;
    EXPECT_EQ(rowDataSet2.Get(1)->Get(2, desDoub), E_OK);
    EXPECT_EQ(desDoub, DOUBLE_VALUE);

    int64_t desInt64;
    EXPECT_EQ(rowDataSet2.Get(1)->Get(3, desInt64), E_OK);
    EXPECT_EQ(desInt64, INT64_VALUE);

    std::string desStr;
    EXPECT_EQ(rowDataSet2.Get(1)->Get(4, desStr), E_OK);
    EXPECT_EQ(desStr, STR_VALUE);
}

/**
 * @tc.name: Put
 * @tc.desc: Test put into result set
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalResultSetTest, Put, TestSize.Level1)
{
    RowData data1 {g_strValue, g_int64Value, g_doubleValue, g_blobValue, g_nullValue};
    RowData data2 {g_nullValue, g_blobValue, g_doubleValue, g_int64Value, g_strValue};  // data1.reverse()
    RowData data3 {g_strValue, g_int64Value, g_doubleValue, g_blobValue, g_nullValue};

    auto rowDataImpl1 = new (std::nothrow) RelationalRowDataImpl(std::move(data1));
    auto rowDataImpl2 = new (std::nothrow) RelationalRowDataImpl(std::move(data2));
    auto rowDataImpl3 = new (std::nothrow) RelationalRowDataImpl(std::move(data3));
    /**
     * @tc.steps: step1. Create 2 row data set which contains 3 row data totally;
     * @tc.expected: OK.
     */
    RelationalRowDataSet rowDataSet1;
    rowDataSet1.Insert(rowDataImpl1);

    RelationalRowDataSet rowDataSet2;
    rowDataSet2.SetRowData({ rowDataImpl2, rowDataImpl3 });

    /**
     * @tc.steps: step2. Put row data set;
     * @tc.expected: The count is in expect.
     */
    RelationalResultSetImpl resultSet;
    resultSet.Put("", 1, std::move(rowDataSet2));
    resultSet.Put("", 2, std::move(rowDataSet1));
    EXPECT_EQ(resultSet.GetCount(), 3);
}

/**
 * @tc.name: EmptyResultSet
 * @tc.desc: Empty result set.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalResultSetTest, EmptyResultSet, TestSize.Level1)
{
    ResultSet *resultSet = new (std::nothrow) RelationalResultSetImpl;
    ASSERT_NE(resultSet, nullptr);

    EXPECT_EQ(resultSet->GetCount(), 0);        // 0 row data
    EXPECT_EQ(resultSet->GetPosition(), -1);    // the init position=-1
    EXPECT_FALSE(resultSet->MoveToFirst());     // move fail. position=-1
    EXPECT_FALSE(resultSet->MoveToLast());      // move fail. position=-1
    EXPECT_FALSE(resultSet->MoveToNext());      // move fail. position=-1
    EXPECT_FALSE(resultSet->MoveToPrevious());  // move fail. position=-1
    EXPECT_FALSE(resultSet->Move(1));           // move fail. position=-1
    EXPECT_FALSE(resultSet->MoveToPosition(0)); // move fail. position=1
    EXPECT_FALSE(resultSet->IsFirst());         // position=1, not the first one
    EXPECT_FALSE(resultSet->IsLast());          // position=1, not the last one
    EXPECT_TRUE(resultSet->IsBeforeFirst());    // empty result set, always true
    EXPECT_TRUE(resultSet->IsAfterLast());      // empty result set, always true
    EXPECT_FALSE(resultSet->IsClosed());        // not closed
    Entry entry;
    EXPECT_EQ(resultSet->GetEntry(entry), DBStatus::NOT_SUPPORT);   // for relational result set, not support get entry.
    ResultSet::ColumnType columnType;
    EXPECT_EQ(resultSet->GetColumnType(0, columnType), DBStatus::NOT_FOUND);       // the invalid position
    int columnIndex = -1;
    EXPECT_EQ(resultSet->GetColumnIndex("", columnIndex), DBStatus::NOT_FOUND);     // empty result set
    int64_t value = 0;
    EXPECT_EQ(resultSet->Get(0, value), DBStatus::NOT_FOUND);  // the invalid position
    std::map<std::string, VariantData> data;
    EXPECT_EQ(resultSet->GetRow(data), DBStatus::NOT_FOUND);   // the invalid position
    delete resultSet;
}

/**
 * @tc.name: NormalResultSet
 * @tc.desc: Normal result set.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalResultSetTest, NormalResultSet, TestSize.Level1)
{
    auto *resultSet = new (std::nothrow) RelationalResultSetImpl;
    ASSERT_NE(resultSet, nullptr);

    /**
     * @tc.steps: step1. Create a result set which contains two row data;
     * @tc.expected: OK.
     */
    RelationalRowDataSet rowDataSet1;
    EXPECT_EQ(rowDataSet1.Insert(new (std::nothrow) RelationalRowDataImpl({g_strValue, g_int64Value})), E_OK);
    RelationalRowDataSet rowDataSet2;
    rowDataSet2.SetColNames({"column 1", "column 2"});
    EXPECT_EQ(rowDataSet2.Insert(new (std::nothrow) RelationalRowDataImpl({g_nullValue, g_blobValue})), E_OK);

    EXPECT_EQ(resultSet->Put("", 2, std::move(rowDataSet2)), E_OK);  // the second one
    EXPECT_EQ(resultSet->Put("", 1, std::move(rowDataSet1)), E_OK);  // the first one

    /**
     * @tc.steps: step2. Check the result set;
     * @tc.expected: All the interface is running in expect.
     */
    EXPECT_EQ(resultSet->GetCount(), 2);        // two row data
    EXPECT_EQ(resultSet->GetPosition(), -1);    // the init position=-1
    EXPECT_TRUE(resultSet->MoveToFirst());      // move ok. position=0
    EXPECT_TRUE(resultSet->MoveToLast());       // move ok. position=1
    EXPECT_FALSE(resultSet->MoveToNext());      // move fail. position=2
    EXPECT_TRUE(resultSet->MoveToPrevious());   // move ok. position=1
    EXPECT_FALSE(resultSet->Move(1));           // move fail. position=2
    EXPECT_TRUE(resultSet->MoveToPosition(0));  // move ok. position=0
    EXPECT_TRUE(resultSet->IsFirst());          // position=0, the first one
    EXPECT_FALSE(resultSet->IsLast());          // position=0, not the last one
    EXPECT_FALSE(resultSet->IsBeforeFirst());   // position=0, not before the first
    EXPECT_FALSE(resultSet->IsAfterLast());     // position=0, not after the last
    EXPECT_FALSE(resultSet->IsClosed());        // not closed

    Entry entry;
    EXPECT_EQ(resultSet->GetEntry(entry), DBStatus::NOT_SUPPORT);   // for relational result set, not support get entry.

    ResultSet::ColumnType columnType;
    EXPECT_EQ(resultSet->GetColumnType(0, columnType), DBStatus::OK);
    EXPECT_EQ(columnType, ResultSet::ColumnType::STRING);

    std::vector<std::string> expectCols { "column 1", "column 2" };
    std::vector<std::string> colNames;
    resultSet->GetColumnNames(colNames);
    EXPECT_EQ(colNames, expectCols);

    int columnIndex = -1;
    EXPECT_EQ(resultSet->GetColumnIndex("", columnIndex), DBStatus::NONEXISTENT);  // the invalid column name
    EXPECT_EQ(resultSet->GetColumnIndex("column 1", columnIndex), DBStatus::OK);
    EXPECT_EQ(columnIndex, 0);

    int64_t value = 0;
    EXPECT_EQ(resultSet->Get(1, value), DBStatus::OK);
    EXPECT_EQ(value, INT64_VALUE);

    std::map<std::string, VariantData> data;
    EXPECT_EQ(resultSet->GetRow(data), DBStatus::OK);
    EXPECT_EQ(std::get<std::string>(data["column 1"]), STR_VALUE);
    EXPECT_EQ(std::get<int64_t>(data["column 2"]), INT64_VALUE);
    delete resultSet;
}


HWTEST_F(DistributedDBRelationalResultSetTest, Test001, TestSize.Level1)
{
    auto *resultSet = new (std::nothrow) RelationalResultSetImpl;
    ASSERT_NE(resultSet, nullptr);

    /**
     * @tc.steps: step1. Create a result set which contains two row data;
     * @tc.expected: OK.
     */
    RelationalRowDataSet rowDataSet1;
    RowData rowData = {g_blobValue, g_doubleValue, g_int64Value, g_nullValue, g_strValue};
    EXPECT_EQ(rowDataSet1.Insert(new (std::nothrow) RelationalRowDataImpl(std::move(rowData))), E_OK);
    EXPECT_EQ(resultSet->Put("", 1, std::move(rowDataSet1)), E_OK);  // the first one

    EXPECT_EQ(resultSet->MoveToFirst(), true);
    ResultSet::ColumnType columnType;
    EXPECT_EQ(resultSet->GetColumnType(0, columnType), DBStatus::OK);
    EXPECT_EQ(columnType, ResultSet::ColumnType::BLOB);
    EXPECT_EQ(resultSet->GetColumnType(1, columnType), DBStatus::OK);
    EXPECT_EQ(columnType, ResultSet::ColumnType::DOUBLE);
    EXPECT_EQ(resultSet->GetColumnType(2, columnType), DBStatus::OK);
    EXPECT_EQ(columnType, ResultSet::ColumnType::INT64);
    EXPECT_EQ(resultSet->GetColumnType(3, columnType), DBStatus::OK);
    EXPECT_EQ(columnType, ResultSet::ColumnType::NULL_VALUE);
    EXPECT_EQ(resultSet->GetColumnType(4, columnType), DBStatus::OK);
    EXPECT_EQ(columnType, ResultSet::ColumnType::STRING);

    delete resultSet;
}

/**
 * @tc.name: ResultSetTest001
 * @tc.desc: Test get rowData and close
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, ResultSetTest001, TestSize.Level0)
{
    auto *resultSet = new (std::nothrow) RelationalResultSetImpl;
    ASSERT_NE(resultSet, nullptr);

    /**
     * @tc.steps: step1. Create a result set which contains five row data;
     * @tc.expected: OK.
     */
    RelationalRowDataSet rowDataSet1;
    RowData rowData = {g_blobValue, g_doubleValue, g_int64Value, g_nullValue, g_strValue};
    rowDataSet1.SetColNames({"column 1", "column 2", "column 3", "column 4", "column 5"});
    EXPECT_EQ(rowDataSet1.Insert(new (std::nothrow) RelationalRowDataImpl(std::move(rowData))), E_OK);
    EXPECT_EQ(resultSet->Put("", 1, std::move(rowDataSet1)), E_OK);  // the first one
    EXPECT_EQ(resultSet->MoveToFirst(), true);

    /**
     * @tc.steps: step2. Check data
     * @tc.expected: OK.
     */
    std::map<std::string, VariantData> data;
    EXPECT_EQ(resultSet->GetRow(data), DBStatus::OK);
    EXPECT_EQ(std::get<std::vector<uint8_t>>(data["column 1"]), BLOB_VALUE);
    EXPECT_EQ(std::get<double>(data["column 2"]), DOUBLE_VALUE);
    EXPECT_EQ(std::get<int64_t>(data["column 3"]), INT64_VALUE);
    EXPECT_EQ(std::get<std::string>(data["column 5"]), STR_VALUE);

    /**
     * @tc.steps: step3. Check close twice
     * @tc.expected: OK.
     */
    resultSet->Close();
    EXPECT_EQ(resultSet->GetCount(), 0);
    resultSet->Close();
    EXPECT_EQ(resultSet->GetCount(), 0);

    delete resultSet;
}

/**
 * @tc.name: ResultSetTest002
 * @tc.desc: Test get data in null resultSet
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: chenchaohao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, ResultSetTest002, TestSize.Level0)
{
    auto *resultSet = new (std::nothrow) RelationalResultSetImpl;
    ASSERT_NE(resultSet, nullptr);

    /**
     * @tc.steps: step1. check whether status is not found when get null resultSet
     * @tc.expected: OK.
     */
    std::vector<uint8_t> blob;
    EXPECT_EQ(resultSet->Get(0, blob), DBStatus::NOT_FOUND);
    std::string strValue = "";
    EXPECT_EQ(resultSet->Get(0, strValue), DBStatus::NOT_FOUND);
    int64_t intValue = 0;
    EXPECT_EQ(resultSet->Get(0, intValue), DBStatus::NOT_FOUND);
    double doubleValue = 0.0;
    EXPECT_EQ(resultSet->Get(0, doubleValue), DBStatus::NOT_FOUND);
    bool isNull = true;
    EXPECT_EQ(resultSet->IsColumnNull(0, isNull), DBStatus::NOT_FOUND);

    delete resultSet;
}

/**
 * @tc.name: SerializeTest001
 * @tc.desc: Test Serialize func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, SerializeTest001, TestSize.Level0)
{
    DataValue dataValue;
    dataValue.ResetValue();
    RowData rowData;
    rowData.push_back(dataValue);
    RelationalRowDataImpl row(std::move(rowData));
    Parcel parcel(nullptr, 0);
    EXPECT_EQ(row.Serialize(parcel), -E_PARSE_FAIL);
}

/**
 * @tc.name: SerializeTest002
 * @tc.desc: Test Serialize func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, SerializeTest002, TestSize.Level0)
{
    std::string largeString(DBConstant::MAX_SET_VALUE_SIZE + 1, 'a');
    DataValue dataValue;
    dataValue.SetText(largeString);
    RowData rowData;
    rowData.push_back(dataValue);
    RelationalRowDataImpl row(std::move(rowData));
    uint32_t parcelSize = DBConstant::MAX_SET_VALUE_SIZE;
    uint8_t* buffer = new(std::nothrow) uint8_t[parcelSize];
    ASSERT_NE(buffer, nullptr);
    Parcel parcel(buffer, parcelSize);
    EXPECT_EQ(row.Serialize(parcel), -E_PARSE_FAIL);
    delete[] buffer;
}

/**
 * @tc.name: DeSerializeTest001
 * @tc.desc: Test DeSerialize func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, DeSerializeTest001, TestSize.Level0)
{
    uint8_t buffer[100] = {0};
    Parcel parcelA(buffer, 100);
    EXPECT_FALSE(parcelA.IsError());
    uint32_t size = 1;
    (void)parcelA.WriteUInt32(size);
    EXPECT_FALSE(parcelA.IsError());
    uint32_t type = static_cast<uint32_t>(StorageType::STORAGE_TYPE_NONE);
    (void)parcelA.WriteUInt32(type);
    EXPECT_FALSE(parcelA.IsError());
    RelationalRowDataImpl row;
    Parcel parcelB(buffer, 100);
    EXPECT_EQ(row.DeSerialize(parcelB), -E_PARSE_FAIL);
}

/**
 * @tc.name: DeSerializeTest002
 * @tc.desc: Test DeSerialize func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, DeSerializeTest002, TestSize.Level0)
{
    /**
     *  @tc.steps: step1. Set totalLen as the critical condition.
     *  satisfies the write condition (parcelLen is less than totalLen), but does not meet the byte alignment condition
     *  parcelLen is 12 after writing, after byte alignment it becomes 16, which is greater than totalLen.
     */
    uint8_t buffer[12] = {0}; // Set totalLen as 12
    Parcel parcelA(buffer, 12);
    uint32_t size = 1;
    (void)parcelA.WriteUInt32(size);
    EXPECT_FALSE(parcelA.IsError());
    uint32_t type = static_cast<uint32_t>(StorageType::STORAGE_TYPE_NULL);
    (void)parcelA.WriteUInt32(type);
    EXPECT_FALSE(parcelA.IsError());
    RelationalRowDataImpl row;
    Parcel parcelB(buffer, 12);
    EXPECT_EQ(row.DeSerialize(parcelB), -E_PARSE_FAIL);
}

/**
 * @tc.name: GetTypeTest001
 * @tc.desc: Test GetType func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, GetTypeTest001, TestSize.Level0)
{
    DistributedDB::RowData data;
    DistributedDB::DataValue value;
    value.SetText("test");
    data.emplace_back(std::move(value));
    DistributedDB::RelationalRowDataImpl row(std::move(data));
    DistributedDB::StorageType type;
    EXPECT_EQ(row.GetType(-1, type), -E_NONEXISTENT);
    EXPECT_EQ(row.GetType(3, type), -E_NONEXISTENT);
}

/**
 * @tc.name: GetTest001
 * @tc.desc: Test Get func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, GetTest001, TestSize.Level0)
{
    DistributedDB::RowData data;
    DistributedDB::DataValue value;
    value.SetText("test");
    data.emplace_back(std::move(value));
    DistributedDB::RelationalRowDataImpl row(std::move(data));
    int64_t intValue;
    double doubleValue;
    std::string stringValueResult;
    std::vector<uint8_t> blobValueResult;
    EXPECT_EQ(row.Get(-1, intValue), -E_NONEXISTENT);
    EXPECT_EQ(row.Get(1, intValue), -E_NONEXISTENT);
    EXPECT_EQ(row.Get(-1, doubleValue), -E_NONEXISTENT);
    EXPECT_EQ(row.Get(1, doubleValue), -E_NONEXISTENT);
    EXPECT_EQ(row.Get(-1, stringValueResult), -E_NONEXISTENT);
    EXPECT_EQ(row.Get(1, stringValueResult), -E_NONEXISTENT);
    EXPECT_EQ(row.Get(-1, blobValueResult), -E_NONEXISTENT);
    EXPECT_EQ(row.Get(1, blobValueResult), -E_NONEXISTENT);
}

/**
 * @tc.name: GetTest002
 * @tc.desc: Test Get func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, GetTest002, TestSize.Level0)
{
    DistributedDB::RowData data;
    DistributedDB::DataValue value;
    value.SetText("test");
    data.emplace_back(std::move(value));
    DistributedDB::RelationalRowDataImpl row(std::move(data));
    int64_t intValue;
    EXPECT_EQ(row.Get(0, intValue), -E_TYPE_MISMATCH);
    double doubleValue;
    EXPECT_EQ(row.Get(0, doubleValue), -E_TYPE_MISMATCH);
}

/**
 * @tc.name: GetTest003
 * @tc.desc: Test Get func
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: tiansimiao
 */
HWTEST_F(DistributedDBRelationalResultSetTest, GetTest003, TestSize.Level0)
{
    DistributedDB::RowData data;
    DistributedDB::DataValue value;
    int64_t intValue = 10;
    value.GetInt64(intValue);
    data.emplace_back(std::move(value));
    DistributedDB::RelationalRowDataImpl row(std::move(data));
    std::string stringValueResult;
    EXPECT_EQ(row.Get(0, stringValueResult), -E_TYPE_MISMATCH);
    std::vector<uint8_t> blobValueResult;
    EXPECT_EQ(row.Get(0, blobValueResult), -E_TYPE_MISMATCH);
}

/**
 * @tc.name: BinlogSupportTest001
 * @tc.desc: Test binlog support API return values as expected
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: hongyangliu
 */
HWTEST_F(DistributedDBRelationalResultSetTest, BinlogSupportTest001, TestSize.Level0)
{
    EXPECT_EQ(sqlite3_is_support_binlog(nullptr), SQLITE_ERROR);
    EXPECT_EQ(sqlite3_is_support_binlog(""), sqlite3_is_support_binlog(" "));
}

/**
 * @tc.name: CompressSupportTest001
 * @tc.desc: Test sqlite open with different vfs option return values as expected
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: hongyangliu
 */
HWTEST_F(DistributedDBRelationalResultSetTest, CompressSupportTest001, TestSize.Level0)
{
    sqlite3 *db = nullptr;
    uint32_t openOption = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    /**
     * @tc.steps: step1. sqlite open with both filepath and vfs as null
     * @tc.expected: no crash
     */
    ASSERT_NO_FATAL_FAILURE(sqlite3_open_v2(nullptr, &db, openOption, nullptr));
    sqlite3_close_v2(db);
    db = nullptr;
    /**
     * @tc.steps: step2. sqlite open with null filepath and random vfs
     * @tc.expected: no crash
     */
    ASSERT_NO_FATAL_FAILURE(sqlite3_open_v2(nullptr, &db, openOption, "non-exist"));
    sqlite3_close_v2(db);
    db = nullptr;
    /**
     * @tc.steps: step3. sqlite open with null filepath and compress vfs
     * @tc.expected: no crash
     */
    ASSERT_NO_FATAL_FAILURE(sqlite3_open_v2(nullptr, &db, openOption, "compressvfs"));
    sqlite3_close_v2(db);
    db = nullptr;
    /**
     * @tc.steps: step4. sqlite open with a regular file with no vfs
     * @tc.expected: open ok
     */
    EXPECT_EQ(sqlite3_open_v2(g_storePath.c_str(), &db, openOption, nullptr), SQLITE_OK);
    sqlite3_close_v2(db);
    db = nullptr;
    /**
     * @tc.steps: step5. sqlite open with a non-whitelist file using compress vfs
     * @tc.expected: open ok
     */
    std::string nonWhiteDb = g_testDir + "/notWhiteList.db";
    EXPECT_EQ(sqlite3_open_v2(nonWhiteDb.c_str(), &db, openOption, "compressvfs"), SQLITE_OK);
    sqlite3_close_v2(db);
    db = nullptr;
    /**
     * @tc.steps: step6. sqlite open with a whitelist file using compress vfs
     * @tc.expected: no crash
     */
    std::string whitelistDb = g_testDir + "/RdbTestNO_slave.db";
    ASSERT_NO_FATAL_FAILURE(sqlite3_open_v2(whitelistDb.c_str(), &db, openOption, "compressvfs"));
    sqlite3_close_v2(db);
    db = nullptr;
}
#endif
