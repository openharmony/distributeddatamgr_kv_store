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

#ifndef OMIT_JSON
#include <algorithm>
#include <gtest/gtest.h>
#include "db_errno.h"
#include "distributeddb_tools_unit_test.h"
#include "json_object.h"

using namespace std;
using namespace testing::ext;
using namespace DistributedDB;

namespace {
    const int MAX_DEPTH_FOR_TEST = 10;
    const int STRING1_DEPTH = 12;
    const int STRING3_DEPTH = 6;

    // nest depth = 12 and valid.
    const string JSON_STRING1 = "{\"#14\":[[{\"#11\":{\"#8\":[{\"#5\":[[{\"#2\":[{\"#0\":\"value_\"},\"value_\"],"
        "\"#3\":\"value_\"},\"value_\"],\"value_\"],\"#6\":\"value_\"},\"value_\"],\"#9\":\"value_\"},"
        "\"#12\":\"value_\"},\"value_\"],\"value_\"],\"#15\":{\"#18\":{\"#16\":\"value_\"},\"#19\":\"value_\"}}";

    // nest depth = 12 and invalid happens in nest depth = 2.
    const string JSON_STRING2 = "{\"#17\":[\"just for mistake pls.[{\"#14\":[[{\"#11\":{\"#8\":{\"#5\":[{\"#2\":"
        "{\"#0\":\"value_\"},\"#3\":\"value_\"},\"value_\"],\"#6\":\"value_\"},\"#9\":\"value_\"},"
        "\"#12\":\"value_\"},\"value_\"],\"value_\"],\"#15\":\"value_\"},\"value_\"],\"value_\"],"
        "\"#18\":{\"#21\":{\"#19\":\"value_\"},\"#22\":\"value_\"}}";

    // nest depth = 6 and valid.
    const string JSON_STRING3 = "{\"#5\":[{\"#2\":[[{\"#0\":\"value_\"},\"value_\"],\"value_\"],\"#3\":\"value_\"},"
        "\"value_\"],\"#6\":{\"#7\":\"value_\",\"#8\":\"value_\"}}";

    // nest depth = 6 and invalid happens in nest depth = 3.
    const string JSON_STRING4 = "{\"#6\":[{\"#3\":\"just for mistake pls.[{\"#0\":[\"value_\"],\"#1\":\"value_\"},"
        "\"value_\"],\"#4\":\"value_\"},\"value_\"],\"#7\":{\"#8\":\"value_\",\"#9\":\"value_\"}}";

    // nest depth = 15 and invalid happens in nest depth = 11.
    const string JSON_STRING5 = "{\"#35\":[{\"#29\":{\"#23\":{\"#17\":{\"#11\":{\"#8\":[{\"#5\":[{\"#2\":"
        "\"just for mistake pls.[[[{\"#0\":\"value_\"},\"value_\"],\"value_\"],\"value_\"],\"#3\":\"value_\"},"
        "\"value_\"],\"#6\":\"value_\"},\"value_\"],\"#9\":\"value_\"},\"#12\":{\"#13\":\"value_\","
        "\"#14\":\"value_\"}},\"#18\":{\"#19\":\"value_\",\"#20\":\"value_\"}},\"#24\":{\"#25\":\"value_\","
        "\"#26\":\"value_\"}},\"#30\":{\"#31\":\"value_\",\"#32\":\"value_\"}},\"value_\"],\"#36\":"
        "{\"#37\":[\"value_\"],\"#38\":\"value_\"}}";

    uint32_t g_oriMaxNestDepth = 0;
}

class DistributedDBJsonPrecheckUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown() {};
};

void DistributedDBJsonPrecheckUnitTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Specifies a maximum nesting depth of 10.
     */
    g_oriMaxNestDepth = JsonObject::SetMaxNestDepth(MAX_DEPTH_FOR_TEST);
}

void DistributedDBJsonPrecheckUnitTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Reset nesting depth to origin value.
     */
    JsonObject::SetMaxNestDepth(g_oriMaxNestDepth);
}

void DistributedDBJsonPrecheckUnitTest::SetUp()
{
    DistributedDBUnitTest::DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

/**
 * @tc.name: Precheck Valid String 001
 * @tc.desc: json string is legal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: yiguang
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseValidString001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Check legal json string with nesting depth of 12.
     * @tc.expected: step1. return value = 12.
     */
    int errCode = E_OK;
    int stepOne = static_cast<int>(JsonObject::CalculateNestDepth(JSON_STRING1, errCode));
    EXPECT_TRUE(errCode == E_OK);
    EXPECT_TRUE(stepOne == STRING1_DEPTH);

    /**
     * @tc.steps: step2. Parsing of legal json string with nesting depth greater than 10 failed.
     * @tc.expected: step2. Parsing result failed.
     */
    JsonObject tempObj;
    int stepTwo = tempObj.Parse(JSON_STRING1);
    EXPECT_TRUE(stepTwo != E_OK);
}

/**
 * @tc.name: Precheck Valid String 002
 * @tc.desc: json string is legal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: yiguang
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseValidString002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Check legal json string with nesting depth of 6.
     * @tc.expected: step1. return value = 6.
     */
    int errCode = E_OK;
    int stepOne = static_cast<int>(JsonObject::CalculateNestDepth(JSON_STRING3, errCode));
    EXPECT_TRUE(errCode == E_OK);
    EXPECT_TRUE(stepOne == STRING3_DEPTH);

    /**
     * @tc.steps: step2. Parsing of legal json string with nesting depth less than 10 success.
     * @tc.expected: step2. Parsing result success.
     */
    JsonObject tempObj;
    int stepTwo = tempObj.Parse(JSON_STRING3);
    EXPECT_TRUE(stepTwo == E_OK);
}

/**
 * @tc.name: Precheck invalid String 001
 * @tc.desc: The json string has been detected illegal before exceeding the specified nesting depth.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: yiguang
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseInvalidString001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Parsing of illegal json string with nesting depth greater than 10 success.
     * @tc.expected: step1. Parsing result failed.
     */
    JsonObject tempObj;
    int stepOne = tempObj.Parse(JSON_STRING2);
    EXPECT_TRUE(stepOne != E_OK);
}

/**
 * @tc.name: Precheck invalid String 002
 * @tc.desc: The json string has been detected illegal before exceeding the specified nesting depth.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: yiguang
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseInvalidString002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Parsing of illegal json string with nesting depth less than 10 success.
     * @tc.expected: step1. Parsing result failed.
     */
    JsonObject tempObj;
    int stepOne = tempObj.Parse(JSON_STRING4);
    EXPECT_TRUE(stepOne != E_OK);
}

/**
 * @tc.name: Precheck invalid String 003
 * @tc.desc: The json string has been detected illegal before exceeding the specified nesting depth.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: yiguang
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseInvalidString003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. Detect illegal json string with nesting depth greater than 10.
     * @tc.expected: step1. return value > 10.
     */
    int errCode = E_OK;
    int stepOne = static_cast<int>(JsonObject::CalculateNestDepth(JSON_STRING5, errCode));
    EXPECT_TRUE(errCode == E_OK);
    EXPECT_TRUE(stepOne > MAX_DEPTH_FOR_TEST);

    /**
     * @tc.steps: step2. Parsing of illegal json string with nesting depth greater than 10 success.
     * @tc.expected: step2. Parsing result failed.
     */
    JsonObject tempObj;
    int stepTwo = tempObj.Parse(JSON_STRING5);
    EXPECT_TRUE(stepTwo != E_OK);
}

/**
 * @tc.name: ParseDuplicativeString001
 * @tc.desc: The json string has more than one field with same name.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseDuplicativeString001, TestSize.Level0)
{
    /**
     * @tc.steps: step1. Parse json.
     * @tc.expected: step1. return ok.
     */
    std::string json = R"({"field1":"123", "field2": true, "field1": 123})";
    JsonObject object;
    EXPECT_EQ(object.JsonObject::Parse(json), E_OK);
    /**
     * @tc.steps: step2. Check field.
     * @tc.expected: step2. field1 is 123, field2 is True.
     */
    FieldValue value;
    EXPECT_EQ(object.GetFieldValueByFieldPath({"field1"}, value), E_OK);
    EXPECT_EQ(value.integerValue, 123);
    EXPECT_EQ(object.GetFieldValueByFieldPath({"field2"}, value), E_OK);
    EXPECT_TRUE(value.boolValue);
}

/**
 * @tc.name: ParseString001
 * @tc.desc: Parse none obj json string.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseString001, TestSize.Level0)
{
    std::string nonJson = R"("field1":123)";
    JsonObject object;
    std::vector<uint8_t> data(nonJson.begin(), nonJson.end());
    EXPECT_EQ(object.Parse(data), -E_JSON_PARSE_FAIL);
    EXPECT_TRUE(object.ToString().empty());
}

/**
 * @tc.name: ParseString002
 * @tc.desc: Parse double and long.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseString002, TestSize.Level0)
{
    std::string json = R"({"field1":429496729500, "field2":11.1})";
    JsonObject object;
    std::vector<uint8_t> data(json.begin(), json.end());
    EXPECT_EQ(object.Parse(data), E_OK);
    FieldValue value;
    EXPECT_EQ(object.GetFieldValueByFieldPath({"field1"}, value), E_OK);
    EXPECT_EQ(value.longValue, 429496729500);
    EXPECT_EQ(object.GetFieldValueByFieldPath({"field2"}, value), E_OK);
    EXPECT_NE(value.doubleValue, 0);
}

/**
 * @tc.name: ParseString003
 * @tc.desc: Parse obj.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseString003, TestSize.Level0)
{
    std::string json = R"({"field1":42949672950, "field2":{"field3":123}})";
    JsonObject object;
    std::vector<uint8_t> data(json.begin(), json.end());
    EXPECT_EQ(object.Parse(data), E_OK);
    std::set<FieldPath> outSubPath;
    std::map<FieldPath, FieldType> outSubPathType;
    EXPECT_EQ(object.GetSubFieldPath({"field1"}, outSubPath), -E_NOT_SUPPORT);
    EXPECT_EQ(object.GetSubFieldPathAndType({"field1"}, outSubPathType), -E_NOT_SUPPORT);
    EXPECT_EQ(object.GetSubFieldPath({"field2"}, outSubPath), E_OK);
    EXPECT_EQ(object.GetSubFieldPathAndType({"field2"}, outSubPathType), E_OK);
    std::set<FieldPath> actualPath = {{"field2", "field3"}};
    EXPECT_EQ(outSubPath, actualPath);
    FieldPath inPath;
    EXPECT_EQ(object.GetSubFieldPath(inPath, outSubPath), E_OK);
    EXPECT_EQ(object.GetSubFieldPathAndType(inPath, outSubPathType), E_OK);
}

/**
 * @tc.name: ParseString004
 * @tc.desc: Parse array.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseString004, TestSize.Level0)
{
    std::string json = R"({"field1":["123"], "field3":"field3"})";
    JsonObject object;
    std::vector<uint8_t> data(json.begin(), json.end());
    EXPECT_EQ(object.Parse(data), E_OK);
    uint32_t size = 0u;
    EXPECT_EQ(object.GetArraySize({"field3"}, size), -E_NOT_SUPPORT);
    EXPECT_EQ(object.GetArraySize({"field1"}, size), E_OK);
    EXPECT_EQ(size, 1u);
    FieldPath inPath;
    EXPECT_EQ(object.GetArraySize(inPath, size), -E_NOT_SUPPORT);
    std::vector<std::vector<std::string>> content;
    EXPECT_EQ(object.GetArrayContentOfStringOrStringArray({"field1"}, content), E_OK);
    std::vector<std::vector<std::string>> actual = {{"123"}};
    EXPECT_EQ(content, actual);
    EXPECT_EQ(object.GetArrayContentOfStringOrStringArray(inPath, content), -E_NOT_SUPPORT);
}

/**
 * @tc.name: ParseString005
 * @tc.desc: Parse array.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, ParseString005, TestSize.Level0)
{
    std::string json = R"({"field1":["123", null, ["456", 789]], "field3":"field3"})";
    JsonObject object;
    EXPECT_EQ(object.Parse(json), E_OK);
    std::vector<std::vector<std::string>> content;
    EXPECT_EQ(object.GetArrayContentOfStringOrStringArray({"field1"}, content), -E_NOT_SUPPORT);

    json = R"({"field1":[["456", "789"], []], "field3":"field3"})";
    object = {};
    EXPECT_EQ(object.Parse(json), E_OK);
    EXPECT_EQ(object.GetArrayContentOfStringOrStringArray({"field1"}, content), E_OK);
    EXPECT_EQ(content.size(), 1u);
}

/**
 * @tc.name: BuildObj001
 * @tc.desc: Build json obj.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, BuildObj001, TestSize.Level0)
{
    JsonObject obj;
    FieldValue val;
    val.boolValue = true;
    EXPECT_EQ(obj.InsertField({"bool_field"}, FieldType::LEAF_FIELD_BOOL, val), E_OK);
    val.stringValue = "str";
    EXPECT_EQ(obj.InsertField({"str_field"}, FieldType::LEAF_FIELD_STRING, val), E_OK);
    val.integerValue = INT32_MAX;
    EXPECT_EQ(obj.InsertField({"int_field"}, FieldType::LEAF_FIELD_INTEGER, val), E_OK);
    val.longValue = INT64_MAX;
    EXPECT_EQ(obj.InsertField({"long_field"}, FieldType::LEAF_FIELD_LONG, val), E_OK);
    val.doubleValue = 3.1415;
    EXPECT_EQ(obj.InsertField({"double_field"}, FieldType::LEAF_FIELD_DOUBLE, val), E_OK);
    EXPECT_EQ(obj.InsertField({"null_field"}, FieldType::LEAF_FIELD_NULL, val), E_OK);
    EXPECT_EQ(obj.InsertField({"array_field"}, FieldType::LEAF_FIELD_ARRAY, val), -E_INVALID_ARGS);
    EXPECT_EQ(obj.InsertField({"obj_field"}, FieldType::LEAF_FIELD_OBJECT, val), E_OK);
    EXPECT_EQ(obj.InsertField({"array_field"}, FieldType::LEAF_FIELD_OBJECT, val), E_OK);
    EXPECT_EQ(obj.InsertField({"array_field", "array_inner"}, FieldType::LEAF_FIELD_OBJECT, val), E_OK);
    EXPECT_EQ(obj.InsertField({"array_field", "array_inner", "inner"}, FieldType::LEAF_FIELD_OBJECT, val), E_OK);
    EXPECT_EQ(obj.DeleteField({"obj_field"}), E_OK);
    LOGI("json is %s", obj.ToString().c_str());
}

/**
 * @tc.name: BuildObj002
 * @tc.desc: Build json obj.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, BuildObj002, TestSize.Level0)
{
    std::string json = R"({"array_field":[]})";
    JsonObject obj;
    EXPECT_EQ(obj.Parse(json), E_OK);
    FieldValue val;
    val.stringValue = "str";
    EXPECT_EQ(obj.InsertField({"array_field1"}, FieldType::LEAF_FIELD_STRING, val, true), E_OK);
    EXPECT_EQ(obj.InsertField({"array_field2"}, FieldType::LEAF_FIELD_STRING, val), E_OK);
    EXPECT_EQ(obj.InsertField({"array_field"}, FieldType::LEAF_FIELD_STRING, val, true), E_OK);
    EXPECT_EQ(obj.InsertField({"array_field"}, FieldType::LEAF_FIELD_STRING, val), E_OK);
    LOGI("json is %s", obj.ToString().c_str());
}

/**
 * @tc.name: InnerError001
 * @tc.desc: Inner error in json obj.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBJsonPrecheckUnitTest, InnerError001, TestSize.Level0)
{
    auto obj = CJsonObject::Parse("");
    std::vector<std::string> outArray;
    JsonObject jsonObject;
    EXPECT_EQ(jsonObject.GetStringArrayContentByCJsonValue(obj, outArray), -E_NOT_SUPPORT);
    std::string json = R"({"array_field":[123]})";
    obj = CJsonObject::Parse(json);
    EXPECT_EQ(jsonObject.GetStringArrayContentByCJsonValue(obj, outArray), -E_NOT_SUPPORT);

    FieldType type;
    EXPECT_EQ(jsonObject.GetFieldTypeByCJsonValue(obj, type), E_OK);
    EXPECT_EQ(type, FieldType::INTERNAL_FIELD_OBJECT);
    json = R"({})";
    obj = CJsonObject::Parse(json);
    EXPECT_EQ(jsonObject.GetFieldTypeByCJsonValue(obj, type), E_OK);
    EXPECT_EQ(type, FieldType::LEAF_FIELD_OBJECT);
}
#endif