/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "DataQueryVirtualTest"

#include "data_query.h"
#include "kv_types_util.h"
#include <string>
#include "types.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

using namespace OHOS::DistributedKv;
using namespace testing;
using namespace testing::ext;
namespace OHOS::Test {
class DataQueryVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DataQueryVirtualTest::SetUpTestCase(void)
{}

void DataQueryVirtualTest::TearDownTestCase(void)
{}

void DataQueryVirtualTest::SetUp(void)
{}

void DataQueryVirtualTest::TearDown(void)
{}

class MockQuery {
public:
    MOCK_METHOD2(Range, void(const std::vector<uint8_t>&, const std::vector<uint8_t>&));
    MOCK_METHOD1(IsNull, void(const std::string&));
    MOCK_METHOD1(IsNotNull, void(const std::string&));
};

/**
 * @tc.name: Reset
 * @tc.desc:
 * @tc.type: Reset test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, Reset, TestSize.Level0)
{
    ZLOGI("ResetTest begin.");
    DataQuery queryReset;
    queryReset.EqualTo("field1", 123);
    queryReset.Reset();
    EXPECT_EQ(queryReset.str_, "");
    EXPECT_FALSE(queryReset.hasKeys_);
    EXPECT_FALSE(queryReset.hasPrefix_);
    EXPECT_EQ(queryReset.deviceId_, "");
    EXPECT_EQ(queryReset.prefix_, "");
}

/**
 * @tc.name: EqualToInteger
 * @tc.desc:
 * @tc.type: EqualToInteger test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, EqualToInteger, TestSize.Level0)
{
    ZLOGI("EqualToInteger begin.");
    DataQuery queryEqualToInteger;
    queryEqualToInteger.EqualTo("field1", 123);
    EXPECT_EQ(queryEqualToInteger.str_,
        std::string(DataQuery::EQUAL_TO) + " " + DataQuery::TYPE_INTEGER + " field1 123");
}

/**
 * @tc.name: EqualToLong
 * @tc.desc:
 * @tc.type: EqualToLong test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, EqualToLong, TestSize.Level0)
{
    ZLOGI("EqualToLong begin.");
    DataQuery queryEqualToLong;
    queryEqualToLong.EqualTo("field1", 123456789012345LL);
    EXPECT_EQ(queryEqualToLong.str_,
        std::string(DataQuery::EQUAL_TO) + " " + DataQuery::TYPE_LONG + " field1 123456789012345");
}

/**
 * @tc.name: EqualToDouble
 * @tc.desc:
 * @tc.type: EqualToDouble test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, EqualToDouble, TestSize.Level0)
{
    ZLOGI("EqualToDouble begin.");
    DataQuery queryEqualToDouble;
    queryEqualToDouble.EqualTo("field1", 123.45);
    EXPECT_EQ(queryEqualToDouble.str_,
        std::string(DataQuery::EQUAL_TO) + " " + DataQuery::TYPE_DOUBLE + " field1 123.45");
}

/**
 * @tc.name: EqualToString
 * @tc.desc:
 * @tc.type: EqualToString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, EqualToString, TestSize.Level0)
{
    ZLOGI("EqualToString begin.");
    DataQuery queryEqualToString;
    queryEqualToString.EqualTo("field1", "testValue");
    EXPECT_EQ(queryEqualToString.str_,
        std::string(DataQuery::EQUAL_TO) + " " + DataQuery::TYPE_STRING + " field1 testValue");
}

/**
 * @tc.name: EqualToBooleanTrue
 * @tc.desc:
 * @tc.type: EqualToBooleanTrue test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, EqualToBooleanTrue, TestSize.Level0)
{
    ZLOGI("EqualToBooleanTrue begin.");
    DataQuery queryEqualToBooleanTrue;
    queryEqualToBooleanTrue.EqualTo("field1", true);
    EXPECT_EQ(queryEqualToBooleanTrue.str_,
        std::string(DataQuery::EQUAL_TO) + " " + DataQuery::TYPE_BOOLEAN + " field1 true");
}

/**
 * @tc.name: EqualToBooleanFalse
 * @tc.desc:
 * @tc.type: EqualToBooleanFalse test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, EqualToBooleanFalse, TestSize.Level0)
{
    ZLOGI("EqualToBooleanFalse begin.");
    DataQuery queryEqualToBooleanFalse;
    queryEqualToBooleanFalse.EqualTo("field1", false);
    EXPECT_EQ(queryEqualToBooleanFalse.str_,
    std::string(DataQuery::EQUAL_TO) + " " + DataQuery::TYPE_BOOLEAN + " field1 false");
}

/**
 * @tc.name: NotEqualToInteger
 * @tc.desc:
 * @tc.type: NotEqualToInteger test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotEqualToInteger, TestSize.Level0)
{
    ZLOGI("NotEqualToInteger begin.");
    DataQuery queryNotEqualToInteger;
    queryNotEqualToInteger.NotEqualTo("field1", 123);
    EXPECT_EQ(queryNotEqualToInteger.str_,
        std::string(DataQuery::NOT_EQUAL_TO) + " " + DataQuery::TYPE_INTEGER + " field1 123");
}

/**
 * @tc.name: NotEqualToLong
 * @tc.desc:
 * @tc.type: NotEqualToLong test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotEqualToLong, TestSize.Level0)
{
    ZLOGI("NotEqualToLong begin.");
    DataQuery queryNotEqualToLong;
    queryNotEqualToLong.NotEqualTo("field1", 123456789012345LL);
    EXPECT_EQ(queryNotEqualToLong.str_,
        std::string(DataQuery::NOT_EQUAL_TO) + " " + DataQuery::TYPE_LONG + " field1 123456789012345");
}

/**
 * @tc.name: NotEqualToDouble
 * @tc.desc:
 * @tc.type: NotEqualToDouble test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotEqualToDouble, TestSize.Level0)
{
    ZLOGI("NotEqualToDouble begin.");
    DataQuery queryNotEqualToDouble;
    queryNotEqualToDouble.NotEqualTo("field1", 123.45);
    EXPECT_EQ(queryNotEqualToDouble.str_,
        std::string(DataQuery::NOT_EQUAL_TO) + " " + DataQuery::TYPE_DOUBLE + " field1 123.45");
}

/**
 * @tc.name: NotEqualToString
 * @tc.desc:
 * @tc.type: NotEqualToString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotEqualToString, TestSize.Level0)
{
    ZLOGI("NotEqualToString begin.");
    DataQuery queryNotEqualToString;
    queryNotEqualToString.NotEqualTo("field1", "testValue");
    EXPECT_EQ(queryNotEqualToString.str_,
        std::string(DataQuery::NOT_EQUAL_TO) + " " + DataQuery::TYPE_STRING + " field1 testValue");
}

/**
 * @tc.name: NotEqualToBooleanTrue
 * @tc.desc:
 * @tc.type: NotEqualToBooleanTrue test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotEqualToBooleanTrue, TestSize.Level0)
{
    ZLOGI("NotEqualToBooleanTrue begin.");
    DataQuery queryNotEqualToBooleanTrue;
    queryNotEqualToBooleanTrue.NotEqualTo("field1", true);
    EXPECT_EQ(queryNotEqualToBooleanTrue.str_,
        std::string(DataQuery::NOT_EQUAL_TO) + " " + DataQuery::TYPE_BOOLEAN + " field1 true");
}

/**
 * @tc.name: NotEqualToBooleanFalse
 * @tc.desc:
 * @tc.type: NotEqualToBooleanFalse test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotEqualToBooleanFalse, TestSize.Level0)
{
    ZLOGI("NotEqualToBooleanFalse begin.");
    DataQuery queryNotEqualToBooleanFalse;
    queryNotEqualToBooleanFalse.NotEqualTo("field1", false);
    EXPECT_EQ(queryNotEqualToBooleanFalse.str_,
        std::string(DataQuery::NOT_EQUAL_TO) + " " + DataQuery::TYPE_BOOLEAN + " field1 false");
}

/**
 * @tc.name: GreaterThanInteger
 * @tc.desc:
 * @tc.type: GreaterThanInteger test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GreaterThanInteger, TestSize.Level0)
{
    ZLOGI("GreaterThanInteger begin.");
    DataQuery queryNotEqualToBooleanFalse;
    queryGreaterThanInteger.GreaterThan("field1", 123);
    EXPECT_EQ(queryGreaterThanInteger.str_,
        std::string(DataQuery::GREATER_THAN) + " " + DataQuery::TYPE_INTEGER + " field1 123");
}

/**
 * @tc.name: GreaterThanLong
 * @tc.desc:
 * @tc.type: GreaterThanLong test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GreaterThanLong, TestSize.Level0)
{
    ZLOGI("GreaterThanLong begin.");
    DataQuery queryGreaterThanLong;
    queryGreaterThanLong.GreaterThan("field1", 123456789012345LL);
    EXPECT_EQ(queryGreaterThanLong.str_,
        std::string(DataQuery::GREATER_THAN) + " " + DataQuery::TYPE_LONG + " field1 123456789012345");
}

/**
 * @tc.name: GreaterThanDouble
 * @tc.desc:
 * @tc.type: GreaterThanDouble test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GreaterThanDouble, TestSize.Level0)
{
    ZLOGI("GreaterThanDouble begin.");
    DataQuery queryGreaterThanDouble;
    queryGreaterThanDouble.GreaterThan("field1", 123.45);
    EXPECT_EQ(queryGreaterThanDouble.str_,
        std::string(DataQuery::GREATER_THAN) + " " + DataQuery::TYPE_DOUBLE + " field1 123.45");
}

/**
 * @tc.name: GreaterThanString
 * @tc.desc:
 * @tc.type: GreaterThanString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GreaterThanString, TestSize.Level0)
{
    ZLOGI("GreaterThanString begin.");
    DataQuery queryGreaterThanString;
    queryGreaterThanString.GreaterThan("field1", "testValue");
    EXPECT_EQ(queryGreaterThanString.str_,
        std::string(DataQuery::GREATER_THAN) + " " + DataQuery::TYPE_STRING + " field1 testValue");
}

/**
 * @tc.name: LessThanInteger
 * @tc.desc:
 * @tc.type: LessThanInteger test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, LessThanInteger, TestSize.Level0)
{
    ZLOGI("LessThanInteger begin.");
    DataQuery queryLessThanInteger;
    queryLessThanInteger.LessThan("field1", 123);
    EXPECT_EQ(queryLessThanInteger.str_,
        std::string(DataQuery::LESS_THAN) + " " + DataQuery::TYPE_INTEGER + " field1 123");
}

/**
 * @tc.name: LessThanLong
 * @tc.desc:
 * @tc.type: LessThanLong test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, LessThanLong, TestSize.Level0)
{
    ZLOGI("GreaterThanLong begin.");
    DataQuery queryLessThanLong;
    queryLessThanLong.LessThan("field1", 123456789012345LL);
    EXPECT_EQ(queryLessThanLong.str_,
        std::string(DataQuery::LESS_THAN) + " " + DataQuery::TYPE_LONG + " field1 123456789012345");
}

/**
 * @tc.name: LessThanDouble
 * @tc.desc:
 * @tc.type: LessThanDouble test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, LessThanDouble, TestSize.Level0)
{
    ZLOGI("LessThanDouble begin.");
    DataQuery queryLessThanDouble;
    queryLessThanDouble.LessThan("field1", 123.45);
    EXPECT_EQ(queryLessThanDouble.str_,
        std::string(DataQuery::LESS_THAN) + " " + DataQuery::TYPE_DOUBLE + " field1 123.45");
}

/**
 * @tc.name: LessThanString
 * @tc.desc:
 * @tc.type: LessThanString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, LessThanString, TestSize.Level0)
{
    ZLOGI("LessThanString begin.");
    DataQuery queryLessThanString;
    queryLessThanString.LessThan("field1", "testValue");
    EXPECT_EQ(queryLessThanString.str_,
        std::string(DataQuery::LESS_THAN) + " " + DataQuery::TYPE_STRING + " field1 testValue");
}

/**
 * @tc.name: GreaterThanOrEqualToInteger
 * @tc.desc:
 * @tc.type: GreaterThanOrEqualToInteger test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GreaterThanOrEqualToInteger, TestSize.Level0)
{
    ZLOGI("LessThanInteger begin.");
    DataQuery queryGreaterThanOrEqualToInteger;
    queryGreaterThanOrEqualToInteger.GreaterThanOrEqualTo("field1", 123);
    EXPECT_EQ(queryGreaterThanOrEqualToInteger.str_,
        std::string(DataQuery::GREATER_THAN_OR_EQUAL_TO) + " " + DataQuery::TYPE_INTEGER + " field1 123");
}

/**
 * @tc.name: GreaterThanOrEqualToLong
 * @tc.desc:
 * @tc.type: GreaterThanOrEqualToLong test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GreaterThanOrEqualToLong, TestSize.Level0)
{
    ZLOGI("GreaterThanLong begin.");
    DataQuery queryGreaterThanOrEqualToLong;
    queryGreaterThanOrEqualToLong.GreaterThanOrEqualTo("field1", 123456789012345LL);
    EXPECT_EQ(queryGreaterThanOrEqualToLong.str_,
        std::string(DataQuery::GREATER_THAN_OR_EQUAL_TO) + " " + DataQuery::TYPE_LONG + " field1 123456789012345");
}

/**
 * @tc.name: GreaterThanOrEqualToDouble
 * @tc.desc:
 * @tc.type: GreaterThanOrEqualToDouble test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GreaterThanOrEqualToDouble, TestSize.Level0)
{
    ZLOGI("LessThanDouble begin.");
    DataQuery queryGreaterThanOrEqualToDouble;
    queryGreaterThanOrEqualToDouble.GreaterThanOrEqualTo("field1", 123.45);
    EXPECT_EQ(queryGreaterThanOrEqualToDouble.str_,
        std::string(DataQuery::GREATER_THAN_OR_EQUAL_TO) + " " + DataQuery::TYPE_DOUBLE + " field1 123.45");
}

/**
 * @tc.name: GreaterThanOrEqualToString
 * @tc.desc:
 * @tc.type: GreaterThanOrEqualToString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, GreaterThanOrEqualToString, TestSize.Level0)
{
    ZLOGI("LessThanString begin.");
    DataQuery queryGreaterThanOrEqualToString;
    queryGreaterThanOrEqualToString.GreaterThanOrEqualTo("field1", "testValue");
    EXPECT_EQ(queryGreaterThanOrEqualToString.str_,
        std::string(DataQuery::GREATER_THAN_OR_EQUAL_TO) + " " + DataQuery::TYPE_STRING + " field1 testValue");
}

/**
 * @tc.name: LessThanOrEqualToInteger
 * @tc.desc:
 * @tc.type: LessThanOrEqualToInteger test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, LessThanOrEqualToInteger, TestSize.Level0)
{
    ZLOGI("LessThanOrEqualToInteger begin.");
    DataQuery queryLessThanOrEqualToInteger;
    queryLessThanOrEqualToInteger.LessThanOrEqualTo("field1", 123);
    EXPECT_EQ(queryLessThanOrEqualToInteger.str_,
        std::string(DataQuery::LESS_THAN_OR_EQUAL_TO) + " " + DataQuery::TYPE_INTEGER + " field1 123");
}

/**
 * @tc.name: LessThanOrEqualToLong
 * @tc.desc:
 * @tc.type: LessThanOrEqualToLong test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, LessThanOrEqualToLong, TestSize.Level0)
{
    ZLOGI("LessThanOrEqualToLong begin.");
    DataQuery queryLessThanOrEqualToLong;
    queryLessThanOrEqualToLong.LessThanOrEqualTo("field1", 123456789012345LL);
    EXPECT_EQ(queryLessThanOrEqualToLong.str_,
        std::string(DataQuery::LESS_THAN_OR_EQUAL_TO) + " " + DataQuery::TYPE_LONG + " field1 123456789012345");
}

/**
 * @tc.name: LessThanOrEqualToDouble
 * @tc.desc:
 * @tc.type: LessThanOrEqualToDouble test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, LessThanOrEqualToDouble, TestSize.Level0)
{
    ZLOGI("LessThanOrEqualToDouble begin.");
    DataQuery queryLessThanOrEqualToDouble;
    queryLessThanOrEqualToDouble.LessThanOrEqualTo("field1", 123.45);
    EXPECT_EQ(queryLessThanOrEqualToDouble.str_,
        std::string(DataQuery::LESS_THAN_OR_EQUAL_TO) + " " + DataQuery::TYPE_DOUBLE + " field1 123.45");
}

/**
 * @tc.name: LessThanOrEqualToString
 * @tc.desc:
 * @tc.type: LessThanOrEqualToString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, LessThanOrEqualToString, TestSize.Level0)
{
    ZLOGI("LessThanOrEqualToString begin.");
    DataQuery queryLessThanOrEqualToString;
    queryLessThanOrEqualToString.LessThanOrEqualTo("field1", "testValue");
    EXPECT_EQ(queryLessThanOrEqualToString.str_,
        std::string(DataQuery::LESS_THAN_OR_EQUAL_TO) + " " + DataQuery::TYPE_STRING + " field1 testValue");
}

/**
 * @tc.name: Between_ValidInputs_CallsRange
 * @tc.desc:
 * @tc.type: Between_ValidInputs_CallsRange test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, Between_ValidInputs_CallsRange, TestSize.Level0)
{
    ZLOGI("Between_ValidInputs_CallsRange begin.");
    MockQuery mockQuery;
    DataQuery queryBetween(&mockQuery);

    EXPECT_CALL(mockQuery, Range(ElementsAre('a', 'b'), ElementsAre('c', 'd')));

    queryBetween.Between("ab", "cd");
}

/**
 * @tc.name: IsNull_ValidField_UpdatesStringAndCallsIsNull
 * @tc.desc:
 * @tc.type: IsNull_ValidField_UpdatesStringAndCallsIsNull test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, IsNull_ValidField_UpdatesStringAndCallsIsNull, TestSize.Level0)
{
    ZLOGI("IsNull_ValidField_UpdatesStringAndCallsIsNull begin.");
    MockQuery mockQuery;
    DataQuery queryIsNull(&mockQuery);

    EXPECT_CALL(mockQuery, IsNull("field"));

    queryIsNull.IsNull("field");
}

/**
 * @tc.name: IsNull_InvalidField_DoesNotUpdateStringOrCallIsNull
 * @tc.desc:
 * @tc.type: IsNull_InvalidField_DoesNotUpdateStringOrCallIsNull test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, IsNull_InvalidField_DoesNotUpdateStringOrCallIsNull, TestSize.Level0)
{
    ZLOGI("IsNull_InvalidField_DoesNotUpdateStringOrCallIsNull begin.");
    MockQuery mockQuery;
    DataQuery queryIsNull(&mockQuery);

    EXPECT_CALL(mockQuery, IsNull(_)).Times(0);

    queryIsNull.IsNull("invalid field");
}

/**
 * @tc.name: IsNotNull_ValidField_UpdatesStringAndCallsIsNotNull
 * @tc.desc:
 * @tc.type: IsNotNull_ValidField_UpdatesStringAndCallsIsNotNull test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, IsNotNull_ValidField_UpdatesStringAndCallsIsNotNull, TestSize.Level0)
{
    ZLOGI("IsNotNull_ValidField_UpdatesStringAndCallsIsNotNull begin.");
    MockQuery mockQuery;
    DataQuery queryIsNotNull(&mockQuery);

    EXPECT_CALL(mockQuery, IsNotNull("field"));

    queryIsNotNull.IsNotNull("field");
}

/**
 * @tc.name: IsNotNull_InvalidField_DoesNotUpdateStringOrCallIsNotNull
 * @tc.desc:
 * @tc.type: IsNotNull_InvalidField_DoesNotUpdateStringOrCallIsNotNull test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, IsNotNull_InvalidField_DoesNotUpdateStringOrCallIsNotNull, TestSize.Level0)
{
    ZLOGI("IsNotNull_InvalidField_DoesNotUpdateStringOrCallIsNotNull begin.");
    MockQuery mockQuery;
    DataQuery queryIsNotNull(&mockQuery);

    EXPECT_CALL(mockQuery, IsNotNull(_)).Times(0);

    query.IsNotNull("invalid field");
}

/**
 * @tc.name: InInteger
 * @tc.desc:
 * @tc.type: InInteger test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, InInteger, TestSize.Level0)
{
    ZLOGI("InInteger begin.");
    DataQuery queryInInteger;
    queryInInteger.In("field1", 123);
    EXPECT_EQ(queryInInteger.str_,
        std::string(DataQuery::IN) + " " + DataQuery::TYPE_INTEGER + " field1 123");
}

/**
 * @tc.name: InLong
 * @tc.desc:
 * @tc.type: InLong test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, InLong, TestSize.Level0)
{
    ZLOGI("InLong begin.");
    DataQuery queryInLong;
    queryInLong.In("field1", 123456789012345LL);
    EXPECT_EQ(queryInLong.str_,
        std::string(DataQuery::IN) + " " + DataQuery::TYPE_LONG + " field1 123456789012345");
}

/**
 * @tc.name: InDouble
 * @tc.desc:
 * @tc.type: InDouble test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, InDouble, TestSize.Level0)
{
    ZLOGI("InDouble begin.");
    DataQuery queryInDouble;
    queryInDouble.In("field1", 123.45);
    EXPECT_EQ(queryInDouble.str_,
        std::string(DataQuery::IN) + " " + DataQuery::TYPE_DOUBLE + " field1 123.45");
}

/**
 * @tc.name: InString
 * @tc.desc:
 * @tc.type: InString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, InString, TestSize.Level0)
{
    ZLOGI("InString begin.");
    DataQuery queryInString;
    queryInString.In("field1", "testValue");
    EXPECT_EQ(queryInString.str_,
        std::string(DataQuery::IN) + " " + DataQuery::TYPE_STRING + " field1 testValue");
}

/**
 * @tc.name: NotInInteger
 * @tc.desc:
 * @tc.type: NotInInteger test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotInInteger, TestSize.Level0)
{
    ZLOGI("NotInInteger begin.");
    DataQuery queryNotInInteger;
    queryNotInInteger.NotIn("field1", 123);
    EXPECT_EQ(queryNotInInteger.str_,
        std::string(DataQuery::NOT_IN) + " " + DataQuery::TYPE_INTEGER + " field1 123");
}

/**
 * @tc.name: NotInLong
 * @tc.desc:
 * @tc.type: NotInLong test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotInLong, TestSize.Level0)
{
    ZLOGI("NotInLong begin.");
    DataQuery queryNotInLong;
    queryNotInLong.NotIn("field1", 123456789012345LL);
    EXPECT_EQ(queryNotInLong.str_,
        std::string(DataQuery::NOT_IN) + " " + DataQuery::TYPE_LONG + " field1 123456789012345");
}

/**
 * @tc.name: NotInDouble
 * @tc.desc:
 * @tc.type: NotInDouble test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotInDouble, TestSize.Level0)
{
    ZLOGI("InDouble begin.");
    DataQuery queryNotInDouble;
    queryNotInDouble.NotIn("field1", 123.45);
    EXPECT_EQ(queryNotInDouble.str_,
        std::string(DataQuery::NOT_IN) + " " + DataQuery::TYPE_DOUBLE + " field1 123.45");
}

/**
 * @tc.name: NotInString
 * @tc.desc:
 * @tc.type: NotInString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, NotInString, TestSize.Level0)
{
    ZLOGI("NotInString begin.");
    DataQuery queryNotInString;
    queryNotInString.NotIn("field1", "testValue");
    EXPECT_EQ(queryNotInString.str_,
        std::string(DataQuery::NOT_IN) + " " + DataQuery::TYPE_STRING + " field1 testValue");
}

/**
 * @tc.name: Like_ValidFieldAndValue_AppendsToQuery
 * @tc.desc:
 * @tc.type: Like_ValidFieldAndValue_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, Like_ValidFieldAndValue_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("Like_ValidFieldAndValue_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryLikeMock = std::make_shared<DataQuery>();
    DataQuery queryLike;
    EXPECT_CALL(*queryLikeMock, Like("field", "value")).Times(1);
    queryLike.Like("field", "value");
    EXPECT_EQ(queryLike.ToString(), std::string(DataQuery::LIKE) + " field value");
}

/**
 * @tc.name: Unlike_ValidFieldAndValue_AppendsToQuery
 * @tc.desc:
 * @tc.type: Unlike_ValidFieldAndValue_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, Unlike_ValidFieldAndValue_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("Unlike_ValidFieldAndValue_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryUnlikeMock = std::make_shared<DataQuery>();
    DataQuery queryUnlike;
    EXPECT_CALL(*queryUnlikeMock, NotLike("field", "value")).Times(1);
    queryUnlike.Unlike("field", "value");
    EXPECT_EQ(queryUnlike.ToString(), std::string(DataQuery::NOT_LIKE) + " field1 testValue");
}

/**
 * @tc.name: And_AppendsAndToQuery
 * @tc.desc:
 * @tc.type: And_AppendsAndToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, And_AppendsAndToQuery, TestSize.Level0)
{
    ZLOGI("And_AppendsAndToQuery begin.");
    std::shared_ptr<DataQuery> queryAndMock = std::make_shared<DataQuery>();
    DataQuery queryAnd;
    EXPECT_CALL(*queryAndMock, And()).Times(1);
    queryAnd.And();
    EXPECT_EQ(queryAnd.ToString(), " AND");
}

/**
 * @tc.name: Or_AppendsOrToQuery
 * @tc.desc:
 * @tc.type: Or_AppendsOrToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, Or_AppendsOrToQuery, TestSize.Level0)
{
    ZLOGI("Or_AppendsOrToQuery begin.");
    std::shared_ptr<DataQuery> queryOrMock = std::make_shared<DataQuery>();
    DataQuery queryOr;
    EXPECT_CALL(*queryOrMock, Or()).Times(1);
    queryOr.Or();
    EXPECT_EQ(queryOr.ToString(), " OR");
}

/**
 * @tc.name: OrderByAsc_ValidField_AppendsToQuery
 * @tc.desc:
 * @tc.type: OrderByAsc_ValidField_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, OrderByAsc_ValidField_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("OrderByAsc_ValidField_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryOrderByAscMock = std::make_shared<DataQuery>();
    DataQuery queryOrderByAsc;
    EXPECT_CALL(*queryOrderByAscMock, OrderBy("field")).Times(1);
    queryOrderByAsc.OrderByAsc("field");
    EXPECT_EQ(queryOrderByAsc.ToString(), " ORDER BY ASC field");
}

/**
 * @tc.name: OrderByDesc_ValidField_AppendsToQuery
 * @tc.desc:
 * @tc.type: OrderByDesc_ValidField_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, OrderByDesc_ValidField_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("OrderByDesc_ValidField_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryOrderByDescMock = std::make_shared<DataQuery>();
    DataQuery queryOrderByDesc;
    EXPECT_CALL(*queryOrderByDescMock, OrderBy("field", false)).Times(1);
    queryOrderByDesc.OrderByDesc("field");
    EXPECT_EQ(queryOrderByDesc.ToString(), " ORDER BY DESC field");
}

/**
 * @tc.name: OrderByWriteTime_Ascending_AppendsToQuery
 * @tc.desc:
 * @tc.type: OrderByWriteTime_Ascending_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, OrderByWriteTime_Ascending_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("OrderByWriteTime_Ascending_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryOrderByWriteTimeMock = std::make_shared<DataQuery>();
    DataQuery queryOrderByWriteTime;
    EXPECT_CALL(*queryOrderByWriteTimeMock, OrderByWriteTime(true)).Times(1);
    queryOrderByWriteTime.OrderByWriteTime(true);
    EXPECT_EQ(queryOrderByWriteTime.ToString(), " ORDER BY WRITE_TIME ASC");
}

/**
 * @tc.name: OrderByWriteTime_Descending_AppendsToQuery
 * @tc.desc:
 * @tc.type: OrderByWriteTime_Descending_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, OrderByWriteTime_Descending_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("OrderByWriteTime_Descending_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryOrderByWriteTimeMock = std::make_shared<DataQuery>();
    DataQuery queryOrderByWriteTime;
    EXPECT_CALL(*queryMock, OrderByWriteTime(false)).Times(1);
    queryOrderByWriteTime.OrderByWriteTime(false);
    EXPECT_EQ(queryOrderByWriteTime.ToString(), " ORDER BY WRITE_TIME DESC");
}

/**
 * @tc.name: Limit_ValidNumberAndOffset_AppendsToQuery
 * @tc.desc:
 * @tc.type: Limit_ValidNumberAndOffset_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, Limit_ValidNumberAndOffset_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("Limit_ValidNumberAndOffset_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryLimitMock = std::make_shared<DataQuery>();
    DataQuery queryLimit;
    EXPECT_CALL(*queryLimitMock, Limit(10, 0)).Times(1);
    queryLimit.Limit(10, 0);
    EXPECT_EQ(queryLimit.ToString(), " LIMIT 10 0");
}

/**
 * @tc.name: BeginGroup_AppendsBeginGroupToQuery
 * @tc.desc:
 * @tc.type: BeginGroup_AppendsBeginGroupToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, BeginGroup_AppendsBeginGroupToQuery, TestSize.Level0)
{
    ZLOGI("BeginGroup_AppendsBeginGroupToQuery begin.");
    std::shared_ptr<DataQuery> queryBeginGroupMock = std::make_shared<DataQuery>();
    DataQuery queryBeginGroup;
    EXPECT_CALL(*queryBeginGroupMock, BeginGroup()).Times(1);
    queryBeginGroup.BeginGroup();
    EXPECT_EQ(queryBeginGroup.ToString(), " (");
}

/**
 * @tc.name: EndGroup_AppendsEndGroupToQuery
 * @tc.desc:
 * @tc.type: EndGroup_AppendsEndGroupToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, EndGroup_AppendsEndGroupToQuery, TestSize.Level0)
{
    ZLOGI("EndGroup_AppendsEndGroupToQuery begin.");
    std::shared_ptr<DataQuery> queryEndGroupMock = std::make_shared<DataQuery>();
    DataQuery queryEndGroup;
    EXPECT_CALL(*queryEndGroupMock, EndGroup()).Times(1);
    queryEndGroup.EndGroup();
    EXPECT_EQ(queryEndGroup.ToString(), " )");
}

/**
 * @tc.name: KeyPrefix_ValidPrefix_AppendsToQuery
 * @tc.desc:
 * @tc.type: KeyPrefix_ValidPrefix_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, KeyPrefix_ValidPrefix_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("KeyPrefix_ValidPrefix_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryKeyPrefixMock = std::make_shared<DataQuery>();
    DataQuery queryKeyPrefix;
    EXPECT_CALL(*queryKeyPrefixMock, KeyPrefix("prefix")).Times(1);
    queryKeyPrefix.KeyPrefix("prefix");
    EXPECT_EQ(queryKeyPrefix.ToString(), " KEY_PREFIX prefix");
}

/**
 * @tc.name: DeviceId_ValidDeviceId_AppendsToQuery
 * @tc.desc:
 * @tc.type: DeviceId_ValidDeviceId_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, DeviceId_ValidDeviceId_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("DeviceId_ValidDeviceId_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryDeviceIdMock = std::make_shared<DataQuery>();
    DataQuery queryDeviceId;
    EXPECT_CALL(*queryDeviceIdMock, DeviceId("deviceId")).Times(1);
    queryDeviceId.DeviceId("deviceId");
    EXPECT_EQ(queryDeviceId.ToString(), " DEVICE_ID deviceId");
}

/**
 * @tc.name: SetSuggestIndex_ValidIndex_AppendsToQuery
 * @tc.desc:
 * @tc.type: SetSuggestIndex_ValidIndex_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, SetSuggestIndex_ValidIndex_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("DeviceId_ValidDeviceId_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> querySetSuggestIndexMock = std::make_shared<DataQuery>();
    DataQuery querySetSuggestIndex;
    EXPECT_CALL(*querySetSuggestIndexMock, SuggestIndex("index")).Times(1);
    querySetSuggestIndex.SetSuggestIndex("index");
    EXPECT_EQ(querySetSuggestIndex.ToString(), " SUGGEST_INDEX index");
}

/**
 * @tc.name: InKeys_ValidKeys_AppendsToQuery
 * @tc.desc:
 * @tc.type: InKeys_ValidKeys_AppendsToQuery test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, InKeys_ValidKeys_AppendsToQuery, TestSize.Level0)
{
    ZLOGI("InKeys_ValidKeys_AppendsToQuery begin.");
    std::shared_ptr<DataQuery> queryInKeysMock = std::make_shared<DataQuery>();
    DataQuery queryInKeys;
    std::vector<std::string> keys = {"key1", "key2"};
    EXPECT_CALL(*queryInKeysMock, InKeys(keys)).Times(1);
    queryInKeys.InKeys(keys);
    EXPECT_EQ(queryInKeys.ToString(), " IN_KEYS START_IN key1 key2 END_IN");
}

/**
 * @tc.name: ToString_QueryTooLong_ReturnsEmptyString
 * @tc.desc:
 * @tc.type: ToString_QueryTooLong_ReturnsEmptyString test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, ToString_QueryTooLong_ReturnsEmptyString, TestSize.Level0)
{
    ZLOGI("ToString_QueryTooLong_ReturnsEmptyString begin.");
    DataQuery queryToString;
    queryToString.str_.assign(MAX_QUERY_LENGTH + 1, 'a');
    EXPECT_EQ(queryToString.ToString(), "");
}

/**
 * @tc.name: AppendCommon_ValidInput_CorrectAppending
 * @tc.desc:
 * @tc.type: AppendCommon_ValidInput_CorrectAppending test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, AppendCommon_ValidInput_CorrectAppending, TestSize.Level0)
{
    ZLOGI("AppendCommon_ValidInput_CorrectAppending begin.");
    DataQuery queryAppendCommon;
    string keyword = "key";
    string fieldType = "type";
    string field = "field";
    int value = 123;
    queryAppendCommon.AppendCommon(keyword, fieldType, field, value);
    EXPECT_EQ(queryAppendCommon.GetQuery(), " key type field 123");
}

/**
 * @tc.name: AppendCommonString_ValidInput_CorrectAppending
 * @tc.desc:
 * @tc.type: AppendCommonString_ValidInput_CorrectAppending test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, AppendCommonString_ValidInput_CorrectAppending, TestSize.Level0)
{
    ZLOGI("AppendCommonString_ValidInput_CorrectAppending begin.");
    DataQuery queryAppendCommonString;
    string keyword = "key";
    string fieldType = "type";
    string field = "field";
    string value = "value";
    queryAppendCommonString.AppendCommonString(keyword, fieldType, field, value);
    EXPECT_EQ(queryAppendCommonString.GetQuery(), " key type field value");
}

/**
 * @tc.name: AppendCommonBoolean_ValidInput_CorrectAppending
 * @tc.desc:
 * @tc.type: AppendCommonBoolean_ValidInput_CorrectAppending test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, AppendCommonBoolean_ValidInput_CorrectAppending, TestSize.Level0)
{
    ZLOGI("AppendCommonBoolean_ValidInput_CorrectAppending begin.");
    DataQuery queryAppendCommonBoolean;
    string keyword = "key";
    string fieldType = "type";
    string field = "field";
    bool value = true;
    queryAppendCommonBoolean.AppendCommonBoolean(keyword, fieldType, field, value);
    EXPECT_EQ(queryAppendCommonBoolean.GetQuery(), " key type field true");
}

/**
 * @tc.name: AppendCommonList_ValidInput_CorrectAppending
 * @tc.desc:
 * @tc.type: AppendCommonList_ValidInput_CorrectAppending test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, AppendCommonList_ValidInput_CorrectAppending, TestSize.Level0)
{
    ZLOGI("AppendCommonList_ValidInput_CorrectAppending begin.");
    DataQuery queryAppendCommonList;
    string keyword = "key";
    string fieldType = "type";
    string field = "field";
    vector<int> valueList = {1, 2, 3};
    queryAppendCommonList.AppendCommonList(keyword, fieldType, field, valueList);
    EXPECT_EQ(queryAppendCommonList.GetQuery(), " key type field ( 1 2 3 )");
}

/**
 * @tc.name: AppendCommonListString_ValidInput_CorrectAppending
 * @tc.desc:
 * @tc.type: AppendCommonListString_ValidInput_CorrectAppending test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, AppendCommonListString_ValidInput_CorrectAppending, TestSize.Level0)
{
    ZLOGI("AppendCommonListString_ValidInput_CorrectAppending begin.");
    DataQuery queryAppendCommonListString;
    string keyword = "key";
    string fieldType = "type";
    string field = "field";
    vector<string> valueList = {"value1", "value2"};
    queryAppendCommonListString.AppendCommonListString(keyword, fieldType, field, valueList);
    EXPECT_EQ(queryAppendCommonListString.GetQuery(), " key type field ( value1 value2 )");
}

/**
 * @tc.name: EscapeSpace_ValidInput_CorrectEscaping
 * @tc.desc:
 * @tc.type: EscapeSpace_ValidInput_CorrectEscaping test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, EscapeSpace_ValidInput_CorrectEscaping, TestSize.Level0)
{
    ZLOGI("EscapeSpace_ValidInput_CorrectEscaping begin.");
    DataQuery queryEscapeSpace;
    string input = "field with space";
    queryEscapeSpace.EscapeSpace(input);
    EXPECT_EQ(input, "field\\ with\\ space");
}

/**
 * @tc.name: ValidateField_ValidField_ReturnsTrue
 * @tc.desc:
 * @tc.type: ValidateField_ValidField_ReturnsTrue test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, ValidateField_ValidField_ReturnsTrue, TestSize.Level0)
{
    ZLOGI("ValidateField_ValidField_ReturnsTrue begin.");
    DataQuery queryValidateField;
    string field = "validField";
    EXPECT_TRUE(queryValidateField.ValidateField(field));
}

/**
 * @tc.name: ValidateField_EmptyField_ReturnsFalse
 * @tc.desc:
 * @tc.type: ValidateField_EmptyField_ReturnsFalse test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, ValidateField_EmptyField_ReturnsFalse, TestSize.Level0)
{
    ZLOGI("ValidateField_EmptyField_ReturnsFalse begin.");
    DataQuery queryValidateField;
    string field = "";
    EXPECT_FALSE(queryValidateField.ValidateField(field));
}

/**
 * @tc.name: ValidateField_FieldWithSpecialChar_ReturnsFalse
 * @tc.desc:
 * @tc.type: ValidateField_FieldWithSpecialChar_ReturnsFalse test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, ValidateField_FieldWithSpecialChar_ReturnsFalse, TestSize.Level0)
{
    ZLOGI("ValidateField_FieldWithSpecialChar_ReturnsFalse begin.");
    DataQuery queryValidateField;
    string field = "field with special*char";
    EXPECT_FALSE(queryValidateField.ValidateField(field));
}

/**
 * @tc.name: BasicToString_ValidInput_CorrectConversion
 * @tc.desc:
 * @tc.type: BasicToString_ValidInput_CorrectConversion test function
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(DataQueryVirtualTest, BasicToString_ValidInput_CorrectConversion, TestSize.Level0)
{
    ZLOGI("BasicToString_ValidInput_CorrectConversion begin.");
    DataQuery queryBasicToString;
    int value = 456;
    string result = queryBasicToString.BasicToString(value);
    EXPECT_EQ(result, "456");
}
} // namespace OHOS::Test