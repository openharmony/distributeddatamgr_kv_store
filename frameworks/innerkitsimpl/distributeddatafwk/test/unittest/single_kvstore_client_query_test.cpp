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

#define LOG_TAG "SingleKvStoreClientQueryTest"

#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "distributed_kv_data_manager.h"
#include "types.h"
#include "gtest/gtest.h"
namespace {
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class SingleKvStoreClientQueryTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();

    static std::shared_ptr<SingleKvStore> singleKvStore;
    static Status statusGetKvStore;
};

static constexpr const char *VALID_SCHEMA_STRICT_DEFINE = "{\"SCHEMA_VERSION\":\"1.0\","
                                                          "\"SCHEMA_MODE\":\"STRICT\","
                                                          "\"SCHEMA_SKIPSIZE\":0,"
                                                          "\"SCHEMA_DEFINE\":{"
                                                              "\"name\":\"INTEGER, NOT NULL\""
                                                          "},"
                                                          "\"SCHEMA_INDEXES\":[\"$.name\"]}";
std::shared_ptr<SingleKvStore> SingleKvStoreClientQueryTest::singleKvStore = nullptr;
Status SingleKvStoreClientQueryTest::statusGetKvStore = Status::ERROR;
static constexpr int32_t INVALID_NUMBER = -1;
static constexpr uint32_t MAX_QUERY_LENGTH = 1024;

void SingleKvStoreClientQueryTest::SetUpTestCase(void)
{
    std::string baseDir = "/data/service/el1/public/database/SingleKvStoreClientQueryTest";
    mkdir(baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SingleKvStoreClientQueryTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/SingleKvStoreClientQueryTest/key");
    (void)remove("/data/service/el1/public/database/SingleKvStoreClientQueryTest/kvdb");
    (void)remove("/data/service/el1/public/database/SingleKvStoreClientQueryTest");
}

void SingleKvStoreClientQueryTest::SetUp(void)
{}

void SingleKvStoreClientQueryTest::TearDown(void)
{}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is reset
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, TestQueryReset, TestSize.Level1)
{
    DataQuery query;
    EXPECT_TRUE(query.ToString().length() == 0);
    std::string str = "test value";
    query.EqualTo("$.test_field_name", str);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is equalTo, the field is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryEqualToInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.EqualTo("", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("$.test_field_name^", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("", (int64_t)100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("^", (int64_t)100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("$.^", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("", false);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("^$.test_field_name", false);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("", std::string("str"));
    EXPECT_TRUE(query.ToString().length() == 0);
    query.EqualTo("^^^^^^^", std::string("str"));
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is equalTo, the field is valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryEqualToValidField, TestSize.Level1)
{
    DataQuery query;
    query.EqualTo("$.test_field_name", 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.EqualTo("$.test_field_name", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.EqualTo("$.test_field_name", 1.23);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.EqualTo("$.test_field_name", false);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::string str = "";
    query.EqualTo("$.test_field_name", str);
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is notEqualTo, the field is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryNotEqualToValidField, TestSize.Level1)
{
    DataQuery query;
    query.NotEqualTo("", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("$.test_field_name^test", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("", (int64_t)100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("^$.test_field_name", (int64_t)100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("^", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("", false);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("^^", false);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("", std::string("test_value"));
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotEqualTo("$.test_field^_name", std::string("test_value"));
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is notEqualTo, the field is valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryNotEqualToInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.NotEqualTo("$.test_field_name", 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.NotEqualTo("$.test_field_name", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.NotEqualTo("$.test_field_name", 1.23);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.NotEqualTo("$.test_field_name", false);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::string str = "test value";
    query.NotEqualTo("$.test_field_name", str);
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is greaterThan, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryGreaterThanInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.GreaterThan("", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThan("$.^^", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThan("", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThan("^$.test_field_name", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThan("", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThan("^", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThan("", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThan("$.test_field_name^*%$#", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is greaterThan, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryGreaterThanValidField, TestSize.Level1)
{
    DataQuery query;
    query.GreaterThan("$.test_field_name", 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.GreaterThan("$.test_field_name", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.GreaterThan("$.test_field_name", 1.23);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.GreaterThan("$.test_field_name$$$", "test value");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is lessThan, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryLessThanInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.LessThan("", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThan("$.^", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThan("", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThan("^$.test_field_name", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThan("", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThan("^^^", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThan("", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThan("$.test_field_name^", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is lessThan, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryLessThanValidField, TestSize.Level1)
{
    DataQuery query;
    query.LessThan("$.test_field_name", 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.LessThan("$.test_field_name", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.LessThan("$.test_field_name", 1.23);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.LessThan("$.test_field_name", "test value");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is greaterThanOrEqualTo, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryGreaterThanOrEqualToInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.GreaterThanOrEqualTo("", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThanOrEqualTo("^$.test_field_name", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThanOrEqualTo("", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThanOrEqualTo("$.test_field_name^", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThanOrEqualTo("", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThanOrEqualTo("^$.^", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThanOrEqualTo("", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.GreaterThanOrEqualTo("^^=", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is greaterThanOrEqualTo, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryGreaterThanOrEqualToValidField, TestSize.Level1)
{
    DataQuery query;
    query.GreaterThanOrEqualTo("$.test_field_name", 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.GreaterThanOrEqualTo("$.test_field_name", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.GreaterThanOrEqualTo("$.test_field_name", 1.23);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.GreaterThanOrEqualTo("$.test_field_name", "test value");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is lessThanOrEqualTo, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryLessThanOrEqualToInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.LessThanOrEqualTo("", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThanOrEqualTo("^$.test_field_name", 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThanOrEqualTo("", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThanOrEqualTo("$.test_field_name^", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThanOrEqualTo("", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThanOrEqualTo("^", 1.23);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThanOrEqualTo("", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.LessThanOrEqualTo("678678^", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is lessThanOrEqualTo, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryLessThanOrEqualToValidField, TestSize.Level1)
{
    DataQuery query;
    query.LessThanOrEqualTo("$.test_field_name", 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.LessThanOrEqualTo("$.test_field_name", (int64_t) 100);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.LessThanOrEqualTo("$.test_field_name", 1.23);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.LessThanOrEqualTo("$.test_field_name", "test value");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is isNull, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryIsNullInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.IsNull("");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.IsNull("$.test^_field_name");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is isNull, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryIsNullValidField, TestSize.Level1)
{
    DataQuery query;
    query.IsNull("$.test_field_name");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is in, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryInInvalidField, TestSize.Level1)
{
    DataQuery query;
    std::vector<int> vectInt{ 10, 20, 30 };
    query.In("", vectInt);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.In("^", vectInt);
    EXPECT_TRUE(query.ToString().length() == 0);
    std::vector<int64_t> vectLong{ (int64_t) 100, (int64_t) 200, (int64_t) 300 };
    query.In("", vectLong);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.In("$.test_field_name^", vectLong);
    EXPECT_TRUE(query.ToString().length() == 0);
    std::vector<double> vectDouble{1.23, 2.23, 3.23};
    query.In("", vectDouble);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.In("$.^test_field_name", vectDouble);
    EXPECT_TRUE(query.ToString().length() == 0);
    std::vector<std::string> vectString{ "value 1", "value 2", "value 3" };
    query.In("", vectString);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.In("$.test_field_^name^", vectString);
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is in, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryInValidField, TestSize.Level1)
{
    DataQuery query;
    std::vector<int> vectInt{ 10, 20, 30 };
    query.In("$.test_field_name", vectInt);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::vector<int64_t> vectLong{ (int64_t) 100, (int64_t) 200, (int64_t) 300 };
    query.In("$.test_field_name", vectLong);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::vector<double> vectDouble{1.23, 2.23, 3.23};
    query.In("$.test_field_name", vectDouble);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::vector<std::string> vectString{ "value 1", "value 2", "value 3" };
    query.In("$.test_field_name", vectString);
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is notIn, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryNotInInvalidField, TestSize.Level1)
{
    DataQuery query;
    std::vector<int> vectInt{ 10, 20, 30 };
    query.NotIn("", vectInt);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotIn("$.^", vectInt);
    EXPECT_TRUE(query.ToString().length() == 0);
    std::vector<int64_t> vectLong{ (int64_t) 100, (int64_t) 200, (int64_t) 300 };
    query.NotIn("", vectLong);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotIn("^^", vectLong);
    EXPECT_TRUE(query.ToString().length() == 0);
    std::vector<double> vectDouble{ 1.23, 2.23, 3.23 };
    query.NotIn("", vectDouble);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotIn("^$.test_field_name", vectDouble);
    EXPECT_TRUE(query.ToString().length() == 0);
    std::vector<std::string> vectString{ "value 1", "value 2", "value 3" };
    query.NotIn("", vectString);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.NotIn("$.^", vectString);
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is notIn, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryNotInValidField, TestSize.Level1)
{
    DataQuery query;
    std::vector<int> vectInt{ 10, 20, 30 };
    query.NotIn("$.test_field_name", vectInt);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::vector<int64_t> vectLong{ (int64_t) 100, (int64_t) 200, (int64_t) 300 };
    query.NotIn("$.test_field_name", vectLong);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::vector<double> vectDouble{ 1.23, 2.23, 3.23 };
    query.NotIn("$.test_field_name", vectDouble);
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::vector<std::string> vectString{ "value 1", "value 2", "value 3" };
    query.NotIn("$.test_field_name", vectString);
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is like, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryLikeInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.Like("", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.Like("$.test_fi^eld_name", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is like, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryLikeValidField, TestSize.Level1)
{
    DataQuery query;
    query.Like("$.test_field_name", "test value");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is unlike, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryUnlikeInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.Unlike("", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.Unlike("$.^", "test value");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is unlike, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryUnlikeValidField, TestSize.Level1)
{
    DataQuery query;
    query.Unlike("$.test_field_name", "test value");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is and
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryAnd, TestSize.Level1)
{
    DataQuery query;
    query.Like("$.test_field_name1", "test value1");
    query.And();
    query.Like("$.test_field_name2", "test value2");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is or
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryOr, TestSize.Level1)
{
    DataQuery query;
    query.Like("$.test_field_name1", "test value1");
    query.Or();
    query.Like("$.test_field_name2", "test value2");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is orderByAsc, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryOrderByAscInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.OrderByAsc("");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.OrderByAsc("$.^");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is orderByAsc, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryOrderByAscValidField, TestSize.Level1)
{
    DataQuery query;
    query.OrderByAsc("$.test_field_name1");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is orderByDesc, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryOrderByDescInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.OrderByDesc("");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.OrderByDesc("$.test^_field_name1");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is orderByDesc, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryOrderByDescValidField, TestSize.Level1)
{
    DataQuery query;
    query.OrderByDesc("$.test_field_name1");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is limit, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryLimitInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.Limit(INVALID_NUMBER, 100);
    EXPECT_TRUE(query.ToString().length() == 0);
    query.Limit(10, INVALID_NUMBER);
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is limit, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryLimitValidField, TestSize.Level1)
{
    DataQuery query;
    query.Limit(10, 100);
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: query single kvStore by dataQuery, the predicate is notEqualTo
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, SingleKvStoreQueryNotEqualTo, TestSize.Level1)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .encrypt = true, .autoSync = true,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .schema =  VALID_SCHEMA_STRICT_DEFINE };
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/SingleKvStoreClientQueryTest";
    AppId appId = { "SingleKvStoreClientQueryTest" };
    StoreId storeId = { "SingleKvStoreClientQueryTestStoreId1" };
    statusGetKvStore = manager.GetSingleKvStore(options, appId, storeId, singleKvStore);
    EXPECT_NE(singleKvStore, nullptr) << "kvStorePtr is null.";
    singleKvStore->Put("test_key_1", "{\"name\":1}");
    singleKvStore->Put("test_key_2", "{\"name\":2}");
    singleKvStore->Put("test_key_3", "{\"name\":3}");

    DataQuery query;
    query.NotEqualTo("$.name", 3);
    std::vector<Entry> results;
    Status status1 = singleKvStore->GetEntries(query, results);
    ASSERT_EQ(status1, Status::SUCCESS);
    EXPECT_TRUE(results.size() == 2);
    results.clear();
    Status status2 = singleKvStore->GetEntries(query, results);
    ASSERT_EQ(status2, Status::SUCCESS);
    EXPECT_TRUE(results.size() == 2);

    std::shared_ptr<KvStoreResultSet> resultSet;
    Status status3 = singleKvStore->GetResultSet(query, resultSet);
    ASSERT_EQ(status3, Status::SUCCESS);
    EXPECT_TRUE(resultSet->GetCount() == 2);
    auto closeResultSetStatus = singleKvStore->CloseResultSet(resultSet);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS);
    Status status4 = singleKvStore->GetResultSet(query, resultSet);
    ASSERT_EQ(status4, Status::SUCCESS);
    EXPECT_TRUE(resultSet->GetCount() == 2);

    closeResultSetStatus = singleKvStore->CloseResultSet(resultSet);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS);

    int resultSize1;
    Status status5 = singleKvStore->GetCount(query, resultSize1);
    ASSERT_EQ(status5, Status::SUCCESS);
    EXPECT_TRUE(resultSize1 == 2);
    int resultSize2;
    Status status6 = singleKvStore->GetCount(query, resultSize2);
    ASSERT_EQ(status6, Status::SUCCESS);
    EXPECT_TRUE(resultSize2 == 2);

    singleKvStore->Delete("test_key_1");
    singleKvStore->Delete("test_key_2");
    singleKvStore->Delete("test_key_3");
    Status status = manager.CloseAllKvStore(appId);
    EXPECT_EQ(status, Status::SUCCESS);
    status = manager.DeleteAllKvStore(appId, options.baseDir);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: DataQuery
* @tc.desc: query single kvStore by dataQuery, the predicate is notEqualTo and equalTo
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, SingleKvStoreQueryNotEqualToAndEqualTo, TestSize.Level1)
{
    DistributedKvDataManager manager;
    Options options = { .createIfMissing = true, .encrypt = true, .autoSync = true,
                        .kvStoreType = KvStoreType::SINGLE_VERSION, .schema = VALID_SCHEMA_STRICT_DEFINE };
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/SingleKvStoreClientQueryTest";
    AppId appId = { "SingleKvStoreClientQueryTest" };
    StoreId storeId = { "SingleKvStoreClientQueryTestStoreId2" };
    statusGetKvStore = manager.GetSingleKvStore(options, appId, storeId, singleKvStore);
    EXPECT_NE(singleKvStore, nullptr) << "kvStorePtr is null.";
    singleKvStore->Put("test_key_1", "{\"name\":1}");
    singleKvStore->Put("test_key_2", "{\"name\":2}");
    singleKvStore->Put("test_key_3", "{\"name\":3}");

    DataQuery query;
    query.NotEqualTo("$.name", 3);
    query.And();
    query.EqualTo("$.name", 1);
    std::vector<Entry> results1;
    Status status1 = singleKvStore->GetEntries(query, results1);
    ASSERT_EQ(status1, Status::SUCCESS);
    EXPECT_TRUE(results1.size() == 1);
    std::vector<Entry> results2;
    Status status2 = singleKvStore->GetEntries(query, results2);
    ASSERT_EQ(status2, Status::SUCCESS);
    EXPECT_TRUE(results2.size() == 1);

    std::shared_ptr<KvStoreResultSet> resultSet;
    Status status3 = singleKvStore->GetResultSet(query, resultSet);
    ASSERT_EQ(status3, Status::SUCCESS);
    EXPECT_TRUE(resultSet->GetCount() == 1);
    auto closeResultSetStatus = singleKvStore->CloseResultSet(resultSet);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS);
    Status status4 = singleKvStore->GetResultSet(query, resultSet);
    ASSERT_EQ(status4, Status::SUCCESS);
    EXPECT_TRUE(resultSet->GetCount() == 1);

    closeResultSetStatus = singleKvStore->CloseResultSet(resultSet);
    ASSERT_EQ(closeResultSetStatus, Status::SUCCESS);

    int resultSize1;
    Status status5 = singleKvStore->GetCount(query, resultSize1);
    ASSERT_EQ(status5, Status::SUCCESS);
    EXPECT_TRUE(resultSize1 == 1);
    int resultSize2;
    Status status6 = singleKvStore->GetCount(query, resultSize2);
    ASSERT_EQ(status6, Status::SUCCESS);
    EXPECT_TRUE(resultSize2 == 1);

    singleKvStore->Delete("test_key_1");
    singleKvStore->Delete("test_key_2");
    singleKvStore->Delete("test_key_3");
    Status status = manager.CloseAllKvStore(appId);
    EXPECT_EQ(status, Status::SUCCESS);
    status = manager.DeleteAllKvStore(appId, options.baseDir);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: DataQuery
* @tc.desc: query group, the predicate is prefix, isNotNull, but field is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryGroupAbnormal, TestSize.Level1)
{
    DataQuery query;
    query.KeyPrefix("");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.KeyPrefix("prefix^");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.Reset();
    query.BeginGroup();
    query.IsNotNull("");
    EXPECT_TRUE(query.ToString().length() > 0);
    query.IsNotNull("^$.name");
    EXPECT_TRUE(query.ToString().length() > 0);
    query.EndGroup();
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: query group, the predicate is prefix, isNotNull.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryByGroupNormal, TestSize.Level1)
{
    DataQuery query;
    query.KeyPrefix("prefix");
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    query.BeginGroup();
    query.IsNotNull("$.name");
    query.EndGroup();
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is setSuggestIndex, the field is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: liuwenhui
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQuerySetSuggestIndexInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.SetSuggestIndex("");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.SetSuggestIndex("test_field^_name");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is setSuggestIndex, the field is valid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: liuwenhui
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQuerySetSuggestIndexValidField, TestSize.Level1)
{
    DataQuery query;
    query.SetSuggestIndex("test_field_name");
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is inKeys
* @tc.type: FUNC
* @tc.require:
* @tc.author: taoyuxin
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQuerySetInKeys, TestSize.Level1)
{
    DataQuery query;
    query.InKeys({});
    EXPECT_TRUE(query.ToString().length() == 0);
    query.InKeys({"test_field_name"});
    EXPECT_TRUE(query.ToString().length() > 0);
    query.InKeys({"test_field_name_hasKey"});
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::vector<std::string> keys { "test_field", "", "^test_field", "^", "test_field_name" };
    query.InKeys(keys);
    EXPECT_TRUE(query.ToString().length() > 0);
}

/**
* @tc.name: DataQuery
* @tc.desc:the predicate is deviceId, the field is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryDeviceIdInvalidField, TestSize.Level1)
{
    DataQuery query;
    query.DeviceId("");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.DeviceId("$$^");
    EXPECT_TRUE(query.ToString().length() == 0);
    query.DeviceId("device_id^");
    EXPECT_TRUE(query.ToString().length() == 0);
}

/**
* @tc.name: DataQuery
* @tc.desc: the predicate is valid deviceId, the field is valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(SingleKvStoreClientQueryTest, DataQueryDeviceIdValidField, TestSize.Level1)
{
    DataQuery query;
    query.DeviceId("device_id");
    EXPECT_TRUE(query.ToString().length() > 0);
    query.Reset();
    std::string deviceId = "";
    uint32_t i = 0;
    while (i < MAX_QUERY_LENGTH) {
        deviceId += "device";
        i++;
    }
    query.DeviceId(deviceId);
    EXPECT_TRUE(query.ToString().length() == 0);
}
} // namespace