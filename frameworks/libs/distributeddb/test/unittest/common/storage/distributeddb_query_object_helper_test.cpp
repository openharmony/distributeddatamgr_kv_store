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

#include <gtest/gtest.h>

#include "cloud/cloud_db_types.h"
#include "db_errno.h"
#include "get_query_info.h"
#include "log_print.h"
#include "query_object.h"
#include "query_sync_object.h"
#include "relational_store_manager.h"
#include "version.h"

using namespace testing::ext;
using namespace DistributedDB;

namespace {
const std::string VALID_SCHEMA_FULL_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
  "\"SCHEMA_MODE\":\"STRICT\","
  "\"SCHEMA_DEFINE\":{"
      "\"field_name1\":\"BOOL\","
      "\"field_name2\":{"
          "\"field_name3\":\"INTEGER, NOT NULL\","
          "\"field_name4\":\"LONG, DEFAULT 100\","
          "\"field_name5\":\"DOUBLE, NOT NULL, DEFAULT 3.14\","
          "\"field_name6\":\"STRING, NOT NULL, DEFAULT '3.1415'\","
          "\"field_name7\":[],"
          "\"field_name8\":{}"
      "}"
  "},"
  "\"SCHEMA_INDEXES\":[\"$.field_name1\", \"$.field_name2.field_name6\"]}";
const std::string TEST_FIELD_NAME = "$.field_name2.field_name6";

void GetQuerySql(const Query &query)
{
    QueryObject queryObj(query);

    SchemaObject schema;
    schema.ParseFromSchemaString(VALID_SCHEMA_FULL_DEFINE);
    queryObj.SetSchema(schema);

    int errCode = E_OK;
    SqliteQueryHelper helper = queryObj.GetQueryHelper(errCode);
    ASSERT_EQ(errCode, E_OK);
    EXPECT_EQ(errCode, E_OK);
    std::string sql;
    helper.GetQuerySql(sql, false);
    LOGD("[UNITTEST][sql] = [%s]", sql.c_str());
}

void CheckType(const Type &expectType, const Type &actualType)
{
    ASSERT_EQ(expectType.index(), actualType.index());
    if (expectType.index() != TYPE_INDEX<std::string>) {
        return;
    }
    std::string expectStr = std::get<std::string>(expectType);
    std::string actualStr = std::get<std::string>(actualType);
    EXPECT_EQ(expectStr, actualStr);
}

void CheckQueryNode(const QueryNode &expectNode, const QueryNode &actualNode)
{
    EXPECT_EQ(expectNode.type, actualNode.type);
    EXPECT_EQ(expectNode.fieldName, actualNode.fieldName);
    ASSERT_EQ(expectNode.fieldValue.size(), actualNode.fieldValue.size());
    for (size_t i = 0; i < expectNode.fieldValue.size(); ++i) {
        CheckType(expectNode.fieldValue[i], actualNode.fieldValue[i]);
    }
}

class DistributedDBQueryObjectHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBQueryObjectHelperTest::SetUpTestCase(void)
{
}

void DistributedDBQueryObjectHelperTest::TearDownTestCase(void)
{
}

void DistributedDBQueryObjectHelperTest::SetUp(void)
{
}

void DistributedDBQueryObjectHelperTest::TearDown(void)
{
}

/**
  * @tc.name: Query001
  * @tc.desc: Check the legal single query operation to see if the generated container is correct
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBQueryObjectHelperTest, Query001, TestSize.Level1)
{
    Query query1 = Query::Select().NotEqualTo(TEST_FIELD_NAME, 123); // random test data
    GetQuerySql(query1);

    Query query2 = Query::Select().EqualTo(TEST_FIELD_NAME, true);
    GetQuerySql(query2);

    Query query3 = Query::Select().GreaterThan(TEST_FIELD_NAME, 0);
    GetQuerySql(query3);

    Query query4 = Query::Select().LessThan(TEST_FIELD_NAME, INT_MAX);
    GetQuerySql(query4);

    Query query5 = Query::Select().GreaterThanOrEqualTo(TEST_FIELD_NAME, 1.56); // random test data
    GetQuerySql(query5);

    Query query6 = Query::Select().LessThanOrEqualTo(TEST_FIELD_NAME, 100); // random test data
    GetQuerySql(query6);

    std::string testValue = "employee.sun.yong";
    Query query7 = Query::Select().Like(TEST_FIELD_NAME, testValue);
    GetQuerySql(query7);

    Query query8 = Query::Select().NotLike(TEST_FIELD_NAME, "testValue");
    GetQuerySql(query8);

    std::vector<int> fieldValues{1, 1, 1};
    Query query9 = Query::Select().In(TEST_FIELD_NAME, fieldValues);
    GetQuerySql(query9);

    Query query10 = Query::Select().NotIn(TEST_FIELD_NAME, fieldValues);
    GetQuerySql(query10);

    Query query11 = Query::Select().OrderBy(TEST_FIELD_NAME, false);
    GetQuerySql(query11);

    Query query12 = Query::Select().Limit(1, 2);
    GetQuerySql(query12);

    Query query13 = Query::Select().IsNull(TEST_FIELD_NAME);
    GetQuerySql(query13);
}

/**
  * @tc.name: Query002
  * @tc.desc: Check for illegal query conditions can not get helper transfer to sql
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBQueryObjectHelperTest, Query002, TestSize.Level1)
{
    float testValue = 1.1;
    Query query = Query::Select().NotEqualTo(".test", testValue);
    QueryObject queryObj(query);
    SchemaObject schema;
    schema.ParseFromSchemaString(VALID_SCHEMA_FULL_DEFINE);
    queryObj.SetSchema(schema);
    int errCode = E_OK;
    SqliteQueryHelper helper = queryObj.GetQueryHelper(errCode); // invalid field name
    EXPECT_NE(errCode, E_OK);

    Query query1 = Query::Select().GreaterThan(TEST_FIELD_NAME, true); // bool compare
    QueryObject queryObj1(query1);
    queryObj1.SetSchema(schema);
    SqliteQueryHelper helper1 = queryObj1.GetQueryHelper(errCode);
    EXPECT_NE(errCode, E_OK);

    Query query2 = Query::Select().LessThan("$.field_name2.field_name4", true);
    QueryObject queryObj2(query2);
    queryObj2.SetSchema(schema);
    SqliteQueryHelper helper2 = queryObj2.GetQueryHelper(errCode);
    EXPECT_NE(errCode, E_OK);
}

/**
  * @tc.name: Query003
  * @tc.desc: Check combination condition transfer to sql
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: sunpeng
  */
HWTEST_F(DistributedDBQueryObjectHelperTest, Query003, TestSize.Level1)
{
    Query query = Query::Select().EqualTo(TEST_FIELD_NAME, true).And().GreaterThan(TEST_FIELD_NAME, 1);
    GetQuerySql(query);

    Query query1 = Query::Select().GreaterThan(TEST_FIELD_NAME, 1).OrderBy(TEST_FIELD_NAME);
    GetQuerySql(query1);
}

std::vector<std::vector<QueryNode>> Query004GetExpectNode()
{
    std::vector<std::vector<QueryNode>> expectNode;
    expectNode.push_back({
        QueryNode {
            .type = QueryNodeType::BEGIN_GROUP,
            .fieldName = "",
            .fieldValue = {}
        }, QueryNode {
            .type = QueryNodeType::EQUAL_TO,
            .fieldName = "field1",
            .fieldValue = {std::string("1")}
        }, QueryNode {
            .type = QueryNodeType::AND,
            .fieldName = "",
            .fieldValue = {}
        }, QueryNode {
            .type = QueryNodeType::EQUAL_TO,
            .fieldName = "field2",
            .fieldValue = {std::string("2")}
        }, QueryNode {
            .type = QueryNodeType::END_GROUP,
            .fieldName = "",
            .fieldValue = {}
        }, QueryNode {
            .type = QueryNodeType::OR,
            .fieldName = "",
            .fieldValue = {}
        }, QueryNode {
            .type = QueryNodeType::BEGIN_GROUP,
            .fieldName = "",
            .fieldValue = {}
        }, QueryNode {
            .type = QueryNodeType::EQUAL_TO,
            .fieldName = "field1",
            .fieldValue = {std::string("2")}
        }, QueryNode {
            .type = QueryNodeType::AND,
            .fieldName = "",
            .fieldValue = {}
        }, QueryNode {
            .type = QueryNodeType::EQUAL_TO,
            .fieldName = "field2",
            .fieldValue = {std::string("1")}
        }, QueryNode {
            .type = QueryNodeType::END_GROUP,
            .fieldName = "",
            .fieldValue = {}
        }});
    return expectNode;
}

std::vector<QueryNode> Query004GetTable2ExpectNode()
{
    return {
        QueryNode {
            .type = QueryNodeType::IN,
            .fieldName = "field3",
            .fieldValue = {std::string("1"), std::string("2"), std::string("3")}
        }
    };
}

/**
  * @tc.name: Query004
  * @tc.desc: Check query transform in serialize func
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: zhangqiquan
  */
HWTEST_F(DistributedDBQueryObjectHelperTest, Query004, TestSize.Level1)
{
    std::vector<std::string> inVal = {"1", "2", "3"};
    Query query = Query::Select().From("table1").BeginGroup().
        EqualTo("field1", "1").And().EqualTo("field2", "2").
        EndGroup().Or().BeginGroup().EqualTo("field1", "2").And().EqualTo("field2", "1").EndGroup().
        From("table2").In("field3", inVal);

    std::vector<QuerySyncObject> objectList = QuerySyncObject::GetQuerySyncObject(query);
    ASSERT_EQ(objectList.size(), 2u); // 2 tables

    std::vector<std::vector<QueryNode>> expectNode = Query004GetExpectNode();
    expectNode.emplace_back(Query004GetTable2ExpectNode());
    std::vector<std::vector<QueryNode>> actualNode;
    for (auto &object: objectList) {
        ASSERT_TRUE(object.IsContainQueryNodes());
        Bytes bytes;
        bytes.resize(object.CalculateParcelLen(SOFTWARE_VERSION_CURRENT));
        Parcel parcel(bytes.data(), bytes.size());
        EXPECT_EQ(object.SerializeData(parcel, SOFTWARE_VERSION_CURRENT), E_OK);
        DBStatus status;
        auto nodeList = RelationalStoreManager::ParserQueryNodes(bytes, status);
        EXPECT_EQ(status, OK);
        actualNode.emplace_back(nodeList);
    }

    ASSERT_EQ(actualNode.size(), expectNode.size());
    for (size_t i = 0; i < actualNode.size(); ++i) {
        for (size_t j = 0; j < actualNode[i].size(); ++j) {
            ASSERT_EQ(actualNode[i].size(), expectNode[i].size());
            LOGD("check %zu table %zu node", i, j);
            CheckQueryNode(expectNode[i][j], actualNode[i][j]);
        }
    }
}
}