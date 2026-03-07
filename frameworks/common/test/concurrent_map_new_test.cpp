/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define LOG_TAG "ConcurrentMapNew"

#include "concurrent_map.h"
#include "gtest/gtest.h"
using namespace testing::ext;
namespace OHOS::Test {
template <typename _Key, typename _Tp>
using ConcurrentMap = OHOS::ConcurrentMap<_Key, _Tp>;
class ConcurrentMapTestNew : public testing::Test {
public:
    struct TestValueAlpha {
        std::string identifier;
        std::string label;
        std::string scenario;
    };
    static void SetUpTestCase(void) { }

    static void TearDownTestCase(void) { }

protected:
    void SetUp()
    {
        dataAlpha_.Clear();
        dataBeta_.Clear();
        dataGamma_.Clear();
    }

    void TearDown()
    {
        dataAlpha_.Clear();
        dataBeta_.Clear();
        dataGamma_.Clear();
    }

    ConcurrentMap<std::string, TestValueAlpha> dataAlpha_;
    ConcurrentMap<int, TestValueAlpha> dataBeta_;
    ConcurrentMap<uint32_t, TestValueAlpha> dataGamma_;
};

/**
 * @tc.name: EmplaceBasicOperation
 * @tc.desc: test basic emplace operation with empty parameters
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceBasicOperation, TestSize.Level0)
{
    dataAlpha_.Emplace();
    auto result = dataAlpha_.Find("");
    ASSERT_TRUE(result.first);
    dataAlpha_.Clear();
}

/**
 * @tc.name: EmplaceWithFilterCondition
 * @tc.desc: test emplace operation with custom filter condition
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithFilterCondition, TestSize.Level0)
{
    auto success = dataAlpha_.Emplace(
        [](const decltype(dataAlpha_)::map_type &entries) {
            return (entries.find("alpha") == entries.end());
        },
        "alpha", TestValueAlpha { "alpha_id", "alpha_name", "alpha_scenario" });
    ASSERT_TRUE(success);
    success = dataAlpha_.Emplace(
        [](const decltype(dataAlpha_)::map_type &entries) {
            return (entries.find("alpha") == entries.end());
        },
        "alpha", TestValueAlpha { "alpha_id", "alpha_name", "alpha_scenario" });
    ASSERT_TRUE(!success);
}

/**
 * @tc.name: EmplaceWithMultipleArguments
 * @tc.desc: test emplace operation with multiple arguments
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithMultipleArguments, TestSize.Level0)
{
    TestValueAlpha value = { "beta_id", "beta_name", "beta_scenario" };
    auto success = dataAlpha_.Emplace("beta", value);
    ASSERT_TRUE(success);
    auto result = dataAlpha_.Find("beta");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, value.identifier);
    ASSERT_EQ(result.second.label, value.label);
    ASSERT_EQ(result.second.scenario, value.scenario);
}

/**
 * @tc.name: EmplaceIntKeyTest
 * @tc.desc: test emplace operation with integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceIntKeyTest, TestSize.Level0)
{
    TestValueAlpha value = { "int_id", "int_name", "int_scenario" };
    auto success = dataBeta_.Emplace(100, value);
    ASSERT_TRUE(success);
    auto result = dataBeta_.Find(100);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, value.identifier);
}

/**
 * @tc.name: EmplaceUintKeyTest
 * @tc.desc: test emplace operation with unsigned integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceUintKeyTest, TestSize.Level0)
{
    TestValueAlpha value = { "uint_id", "uint_name", "uint_scenario" };
    auto success = dataGamma_.Emplace(200, value);
    ASSERT_TRUE(success);
    auto result = dataGamma_.Find(200);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.label, value.label);
}

/**
 * @tc.name: FindNonExistentKey
 * @tc.desc: test find operation with non-existent key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindNonExistentKey, TestSize.Level0)
{
    auto result = dataAlpha_.Find("nonexistent");
    ASSERT_FALSE(result.first);
    dataAlpha_.Clear();
}

/**
 * @tc.name: FindIntNonExistentKey
 * @tc.desc: test find operation with non-existent integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindIntNonExistentKey, TestSize.Level0)
{
    auto result = dataBeta_.Find(999);
    ASSERT_FALSE(result.first);
    dataBeta_.Clear();
}

/**
 * @tc.name: FindUintNonExistentKey
 * @tc.desc: test find operation with non-existent unsigned integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindUintNonExistentKey, TestSize.Level0)
{
    auto result = dataGamma_.Find(888);
    ASSERT_FALSE(result.first);
    dataGamma_.Clear();
}

/**
 * @tc.name: ClearEmptyMap
 * @tc.desc: test clear operation on empty map
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, ClearEmptyMap, TestSize.Level0)
{
    dataAlpha_.Clear();
    ASSERT_TRUE(dataAlpha_.Empty());
}

/**
 * @tc.name: ClearNonEmptyMap
 * @tc.desc: test clear operation on non-empty map
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, ClearNonEmptyMap, TestSize.Level0)
{
    TestValueAlpha value = { "clear_id", "clear_name", "clear_scenario" };
    dataAlpha_.Emplace("clear_key", value);
    ASSERT_FALSE(dataAlpha_.Empty());
    dataAlpha_.Clear();
    ASSERT_TRUE(dataAlpha_.Empty());
}

/**
 * @tc.name: SizeEmptyMap
 * @tc.desc: test size operation on empty map
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, SizeEmptyMap, TestSize.Level0)
{
    ASSERT_EQ(dataAlpha_.Size(), 0);
}

/**
 * @tc.name: SizeSingleElement
 * @tc.desc: test size operation with single element
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, SizeSingleElement, TestSize.Level0)
{
    TestValueAlpha value = { "size_id", "size_name", "size_scenario" };
    dataAlpha_.Emplace("size_key", value);
    ASSERT_EQ(dataAlpha_.Size(), 1);
}

/**
 * @tc.name: SizeMultipleElements
 * @tc.desc: test size operation with multiple elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, SizeMultipleElements, TestSize.Level0)
{
    for (int i = 0; i < 10; ++i) {
        std::string key = "key_" + std::to_string(i);
        TestValueAlpha value = { "id_" + std::to_string(i),
                                  "name_" + std::to_string(i),
                                  "scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    ASSERT_EQ(dataAlpha_.Size(), 10);
}

/**
 * @tc.name: IntSizeMultipleElements
 * @tc.desc: test size operation with multiple integer keyed elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntSizeMultipleElements, TestSize.Level0)
{
    for (int i = 0; i < 20; ++i) {
        TestValueAlpha value = { "int_id_" + std::to_string(i),
                                  "int_name_" + std::to_string(i),
                                  "int_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i, value);
    }
    ASSERT_EQ(dataBeta_.Size(), 20);
}

/**
 * @tc.name: UintSizeMultipleElements
 * @tc.desc: test size operation with multiple unsigned integer keyed elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintSizeMultipleElements, TestSize.Level0)
{
    for (uint32_t i = 0; i < 30; ++i) {
        TestValueAlpha value = { "uint_id_" + std::to_string(i),
                                  "uint_name_" + std::to_string(i),
                                  "uint_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i, value);
    }
    ASSERT_EQ(dataGamma_.Size(), 30);
}

/**
 * @tc.name: EmplaceDuplicateKey
 * @tc.desc: test emplace operation with duplicate key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceDuplicateKey, TestSize.Level0)
{
    TestValueAlpha value1 = { "dup_id_1", "dup_name_1", "dup_scenario_1" };
    TestValueAlpha value2 = { "dup_id_2", "dup_name_2", "dup_scenario_2" };
    auto success1 = dataAlpha_.Emplace("duplicate_key", value1);
    ASSERT_TRUE(success1);
    auto success2 = dataAlpha_.Emplace("duplicate_key", value2);
    ASSERT_FALSE(success2);
}

/**
 * @tc.name: EmplaceIntDuplicateKey
 * @tc.desc: test emplace operation with duplicate integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceIntDuplicateKey, TestSize.Level0)
{
    TestValueAlpha value1 = { "int_dup_id_1", "int_dup_name_1", "int_dup_scenario_1" };
    TestValueAlpha value2 = { "int_dup_id_2", "int_dup_name_2", "int_dup_scenario_2" };
    auto success1 = dataBeta_.Emplace(1000, value1);
    ASSERT_TRUE(success1);
    auto success2 = dataBeta_.Emplace(1000, value2);
    ASSERT_FALSE(success2);
}

/**
 * @tc.name: EmplaceUintDuplicateKey
 * @tc.desc: test emplace operation with duplicate unsigned integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceUintDuplicateKey, TestSize.Level0)
{
    TestValueAlpha value1 = { "uint_dup_id_1", "uint_dup_name_1", "uint_dup_scenario_1" };
    TestValueAlpha value2 = { "uint_dup_id_2", "uint_dup_name_2", "uint_dup_scenario_2" };
    auto success1 = dataGamma_.Emplace(2000, value1);
    ASSERT_TRUE(success1);
    auto success2 = dataGamma_.Emplace(2000, value2);
    ASSERT_FALSE(success2);
}

/**
 * @tc.name: FindAfterEmplace
 * @tc.desc: test find operation after emplace
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindAfterEmplace, TestSize.Level0)
{
    TestValueAlpha value = { "find_id", "find_name", "find_scenario" };
    dataAlpha_.Emplace("find_key", value);
    auto result = dataAlpha_.Find("find_key");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, value.identifier);
    ASSERT_EQ(result.second.label, value.label);
    ASSERT_EQ(result.second.scenario, value.scenario);
}

/**
 * @tc.name: FindIntAfterEmplace
 * @tc.desc: test find operation with integer key after emplace
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindIntAfterEmplace, TestSize.Level0)
{
    TestValueAlpha value = { "int_find_id", "int_find_name", "int_find_scenario" };
    dataBeta_.Emplace(500, value);
    auto result = dataBeta_.Find(500);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, value.identifier);
}

/**
 * @tc.name: FindUintAfterEmplace
 * @tc.desc: test find operation with unsigned integer key after emplace
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindUintAfterEmplace, TestSize.Level0)
{
    TestValueAlpha value = { "uint_find_id", "uint_find_name", "uint_find_scenario" };
    dataGamma_.Emplace(600, value);
    auto result = dataGamma_.Find(600);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.label, value.label);
}

/**
 * @tc.name: MultipleEmplaceAndFind
 * @tc.desc: test multiple emplace and find operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, MultipleEmplaceAndFind, TestSize.Level0)
{
    for (int i = 0; i < 50; ++i) {
        std::string key = "multi_key_" + std::to_string(i);
        TestValueAlpha value = { "multi_id_" + std::to_string(i),
                                  "multi_name_" + std::to_string(i),
                                  "multi_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    for (int i = 0; i < 50; ++i) {
        std::string key = "multi_key_" + std::to_string(i);
        auto result = dataAlpha_.Find(key);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.identifier, "multi_id_" + std::to_string(i));
    }
    ASSERT_EQ(dataAlpha_.Size(), 50);
}

/**
 * @tc.name: MultipleIntEmplaceAndFind
 * @tc.desc: test multiple emplace and find operations with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, MultipleIntEmplaceAndFind, TestSize.Level0)
{
    for (int i = 0; i < 60; ++i) {
        TestValueAlpha value = { "int_multi_id_" + std::to_string(i),
                                  "int_multi_name_" + std::to_string(i),
                                  "int_multi_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i * 10, value);
    }
    for (int i = 0; i < 60; ++i) {
        auto result = dataBeta_.Find(i * 10);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.label, "int_multi_name_" + std::to_string(i));
    }
    ASSERT_EQ(dataBeta_.Size(), 60);
}

/**
 * @tc.name: MultipleUintEmplaceAndFind
 * @tc.desc: test multiple emplace and find operations with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, MultipleUintEmplaceAndFind, TestSize.Level0)
{
    for (uint32_t i = 0; i < 70; ++i) {
        TestValueAlpha value = { "uint_multi_id_" + std::to_string(i),
                                  "uint_multi_name_" + std::to_string(i),
                                  "uint_multi_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i * 5, value);
    }
    for (uint32_t i = 0; i < 70; ++i) {
        auto result = dataGamma_.Find(i * 5);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.scenario, "uint_multi_scenario_" + std::to_string(i));
    }
    ASSERT_EQ(dataGamma_.Size(), 70);
}

/**
 * @tc.name: ClearAfterMultipleOperations
 * @tc.desc: test clear operation after multiple operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, ClearAfterMultipleOperations, TestSize.Level0)
{
    for (int i = 0; i < 80; ++i) {
        std::string key = "clear_multi_key_" + std::to_string(i);
        TestValueAlpha value = { "clear_multi_id_" + std::to_string(i),
                                  "clear_multi_name_" + std::to_string(i),
                                  "clear_multi_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    ASSERT_EQ(dataAlpha_.Size(), 80);
    dataAlpha_.Clear();
    ASSERT_TRUE(dataAlpha_.Empty());
    ASSERT_EQ(dataAlpha_.Size(), 0);
}

/**
 * @tc.name: ClearIntAfterMultipleOperations
 * @tc.desc: test clear operation after multiple operations with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, ClearIntAfterMultipleOperations, TestSize.Level0)
{
    for (int i = 0; i < 90; ++i) {
        TestValueAlpha value = { "clear_int_multi_id_" + std::to_string(i),
                                  "clear_int_multi_name_" + std::to_string(i),
                                  "clear_int_multi_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i * 2, value);
    }
    ASSERT_EQ(dataBeta_.Size(), 90);
    dataBeta_.Clear();
    ASSERT_TRUE(dataBeta_.Empty());
    ASSERT_EQ(dataBeta_.Size(), 0);
}

/**
 * @tc.name: ClearUintAfterMultipleOperations
 * @tc.desc: test clear operation after multiple operations with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, ClearUintAfterMultipleOperations, TestSize.Level0)
{
    for (uint32_t i = 0; i < 100; ++i) {
        TestValueAlpha value = { "clear_uint_multi_id_" + std::to_string(i),
                                  "clear_uint_multi_name_" + std::to_string(i),
                                  "clear_uint_multi_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i * 3, value);
    }
    ASSERT_EQ(dataGamma_.Size(), 100);
    dataGamma_.Clear();
    ASSERT_TRUE(dataGamma_.Empty());
    ASSERT_EQ(dataGamma_.Size(), 0);
}

/**
 * @tc.name: EmplacePiecewiseConstruct
 * @tc.desc: test emplace operation with piecewise construct
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplacePiecewiseConstruct, TestSize.Level0)
{
    TestValueAlpha value = { "piecewise_id", "piecewise_name", "piecewise_scenario" };
    dataAlpha_.Clear();
    auto success = dataAlpha_.Emplace(std::piecewise_construct,
                                       std::forward_as_tuple("piecewise_key"),
                                       std::forward_as_tuple(value));
    ASSERT_TRUE(success);
    auto result = dataAlpha_.Find("piecewise_key");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, value.identifier);
    ASSERT_EQ(result.second.label, value.label);
    ASSERT_EQ(result.second.scenario, value.scenario);
}

/**
 * @tc.name: EmplaceFilterWithErase
 * @tc.desc: test emplace with filter that erases existing entry
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceFilterWithErase, TestSize.Level0)
{
    auto success = dataAlpha_.Emplace(
        [](decltype(dataAlpha_)::map_type &entries) {
            auto it = entries.find("erase_key");
            if (it != entries.end()) {
                entries.erase(it);
                it = entries.end();
            }
            return (it == entries.end());
        },
        "erase_key", TestValueAlpha { "erase_id", "erase_name", "erase_scenario" });
    ASSERT_TRUE(success);
}

/**
 * @tc.name: FindAndUpdate
 * @tc.desc: test find operation and update value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindAndUpdate, TestSize.Level0)
{
    TestValueAlpha value1 = { "update_id_1", "update_name_1", "update_scenario_1" };
    dataAlpha_.Emplace("update_key", value1);
    auto result = dataAlpha_.Find("update_key");
    ASSERT_TRUE(result.first);
    TestValueAlpha value2 = { "update_id_2", "update_name_2", "update_scenario_2" };
    dataAlpha_.Clear();
    dataAlpha_.Emplace("update_key", value2);
    auto result2 = dataAlpha_.Find("update_key");
    ASSERT_TRUE(result2.first);
    ASSERT_EQ(result2.second.identifier, value2.identifier);
}

/**
 * @tc.name: LargeScaleEmplace
 * @tc.desc: test large scale emplace operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, LargeScaleEmplace, TestSize.Level0)
{
    for (int i = 0; i < 200; ++i) {
        std::string key = "large_key_" + std::to_string(i);
        TestValueAlpha value = { "large_id_" + std::to_string(i),
                                  "large_name_" + std::to_string(i),
                                  "large_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    ASSERT_EQ(dataAlpha_.Size(), 200);
}

/**
 * @tc.name: LargeScaleIntEmplace
 * @tc.desc: test large scale emplace operations with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, LargeScaleIntEmplace, TestSize.Level0)
{
    for (int i = 0; i < 300; ++i) {
        TestValueAlpha value = { "large_int_id_" + std::to_string(i),
                                  "large_int_name_" + std::to_string(i),
                                  "large_int_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i, value);
    }
    ASSERT_EQ(dataBeta_.Size(), 300);
}

/**
 * @tc.name: LargeScaleUintEmplace
 * @tc.desc: test large scale emplace operations with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, LargeScaleUintEmplace, TestSize.Level0)
{
    for (uint32_t i = 0; i < 400; ++i) {
        TestValueAlpha value = { "large_uint_id_" + std::to_string(i),
                                 "large_uint_name_" + std::to_string(i),
                                 "large_uint_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i, value);
    }
    ASSERT_EQ(dataGamma_.Size(), 400);
}

/**
 * @tc.name: EmptyCheckAfterOperations
 * @tc.desc: test empty check after various operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmptyCheckAfterOperations, TestSize.Level0)
{
    ASSERT_TRUE(dataAlpha_.Empty());
    TestValueAlpha value = { "empty_id", "empty_name", "empty_scenario" };
    dataAlpha_.Emplace("empty_key", value);
    ASSERT_FALSE(dataAlpha_.Empty());
    dataAlpha_.Clear();
    ASSERT_TRUE(dataAlpha_.Empty());
}

/**
 * @tc.name: IntEmptyCheckAfterOperations
 * @tc.desc: test empty check after various operations with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntEmptyCheckAfterOperations, TestSize.Level0)
{
    ASSERT_TRUE(dataBeta_.Empty());
    TestValueAlpha value = { "int_empty_id", "int_empty_name", "int_empty_scenario" };
    dataBeta_.Emplace(1, value);
    ASSERT_FALSE(dataBeta_.Empty());
    dataBeta_.Clear();
    ASSERT_TRUE(dataBeta_.Empty());
}

/**
 * @tc.name: UintEmptyCheckAfterOperations
 * @tc.desc: test empty check after various operations with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintEmptyCheckAfterOperations, TestSize.Level0)
{
    ASSERT_TRUE(dataGamma_.Empty());
    TestValueAlpha value = { "uint_empty_id", "uint_empty_name", "uint_empty_scenario" };
    dataGamma_.Emplace(1, value);
    ASSERT_FALSE(dataGamma_.Empty());
    dataGamma_.Clear();
    ASSERT_TRUE(dataGamma_.Empty());
}

/**
 * @tc.name: FindRandomKey
 * @tc.desc: test find operation with random keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindRandomKey, TestSize.Level0)
{
    for (int i = 0; i < 150; ++i) {
        std::string key = "random_key_" + std::to_string(i);
        TestValueAlpha value = { "random_id_" + std::to_string(i),
                                  "random_name_" + std::to_string(i),
                                  "random_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    int randomIndex = 75;
    std::string randomKey = "random_key_" + std::to_string(randomIndex);
    auto result = dataAlpha_.Find(randomKey);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "random_id_" + std::to_string(randomIndex));
}

/**
 * @tc.name: FindRandomIntKey
 * @tc.desc: test find operation with random integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindRandomIntKey, TestSize.Level0)
{
    for (int i = 0; i < 250; ++i) {
        TestValueAlpha value = { "random_int_id_" + std::to_string(i),
                                  "random_int_name_" + std::to_string(i),
                                  "random_int_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i * 3, value);
    }
    int randomIndex = 125;
    int randomKey = randomIndex * 3;
    auto result = dataBeta_.Find(randomKey);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.label, "random_int_name_" + std::to_string(randomIndex));
}

/**
 * @tc.name: FindRandomUintKey
 * @tc.desc: test find operation with random unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindRandomUintKey, TestSize.Level0)
{
    for (uint32_t i = 0; i < 350; ++i) {
        TestValueAlpha value = { "random_uint_id_" + std::to_string(i),
                                  "random_uint_name_" + std::to_string(i),
                                  "random_uint_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i * 7, value);
    }
    uint32_t randomIndex = 175;
    uint32_t randomKey = randomIndex * 7;
    auto result = dataGamma_.Find(randomKey);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.scenario, "random_uint_scenario_" + std::to_string(randomIndex));
}

/**
 * @tc.name: SizeAfterClear
 * @tc.desc: test size after clear operation
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, SizeAfterClear, TestSize.Level0)
{
    for (int i = 0; i < 180; ++i) {
        std::string key = "size_clear_key_" + std::to_string(i);
        TestValueAlpha value = { "size_clear_id_" + std::to_string(i),
                                  "size_clear_name_" + std::to_string(i),
                                  "size_clear_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    ASSERT_EQ(dataAlpha_.Size(), 180);
    dataAlpha_.Clear();
    ASSERT_EQ(dataAlpha_.Size(), 0);
}

/**
 * @tc.name: IntSizeAfterClear
 * @tc.desc: test size after clear operation with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntSizeAfterClear, TestSize.Level0)
{
    for (int i = 0; i < 280; ++i) {
        TestValueAlpha value = { "int_size_clear_id_" + std::to_string(i),
                                  "int_size_clear_name_" + std::to_string(i),
                                  "int_size_clear_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i * 4, value);
    }
    ASSERT_EQ(dataBeta_.Size(), 280);
    dataBeta_.Clear();
    ASSERT_EQ(dataBeta_.Size(), 0);
}

/**
 * @tc.name: UintSizeAfterClear
 * @tc.desc: test size after clear operation with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintSizeAfterClear, TestSize.Level0)
{
    for (uint32_t i = 0; i < 380; ++i) {
        TestValueAlpha value = { "uint_size_clear_id_" + std::to_string(i),
                                  "uint_size_clear_name_" + std::to_string(i),
                                  "uint_size_clear_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i * 9, value);
    }
    ASSERT_EQ(dataGamma_.Size(), 380);
    dataGamma_.Clear();
    ASSERT_EQ(dataGamma_.Size(), 0);
}

/**
 * @tc.name: EmplaceWithLongString
 * @tc.desc: test emplace operation with long string values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithLongString, TestSize.Level0)
{
    std::string longIdentifier = "very_long_identifier_" + std::string(100, 'a');
    std::string longLabel = "very_long_label_" + std::string(100, 'b');
    std::string longScenario = "very_long_scenario_" + std::string(100, 'c');
    TestValueAlpha value = { longIdentifier, longLabel, longScenario };
    dataAlpha_.Emplace("long_string_key", value);
    auto result = dataAlpha_.Find("long_string_key");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, longIdentifier);
    ASSERT_EQ(result.second.label, longLabel);
    ASSERT_EQ(result.second.scenario, longScenario);
}

/**
 * @tc.name: EmplaceWithEmptyString
 * @tc.desc: test emplace operation with empty string values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithEmptyString, TestSize.Level0)
{
    TestValueAlpha value = { "", "", "" };
    dataAlpha_.Emplace("empty_string_key", value);
    auto result = dataAlpha_.Find("empty_string_key");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "");
    ASSERT_EQ(result.second.label, "");
    ASSERT_EQ(result.second.scenario, "");
}

/**
 * @tc.name: FindAfterMultipleClears
 * @tc.desc: test find operation after multiple clear operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindAfterMultipleClears, TestSize.Level0)
{
    TestValueAlpha value1 = { "multi_clear_id_1", "multi_clear_name_1", "multi_clear_scenario_1" };
    dataAlpha_.Emplace("multi_clear_key", value1);
    auto result1 = dataAlpha_.Find("multi_clear_key");
    ASSERT_TRUE(result1.first);
    dataAlpha_.Clear();
    TestValueAlpha value2 = { "multi_clear_id_2", "multi_clear_name_2", "multi_clear_scenario_2" };
    dataAlpha_.Emplace("multi_clear_key", value2);
    auto result2 = dataAlpha_.Find("multi_clear_key");
    ASSERT_TRUE(result2.first);
    ASSERT_EQ(result2.second.identifier, value2.identifier);
    dataAlpha_.Clear();
    auto result3 = dataAlpha_.Find("multi_clear_key");
    ASSERT_FALSE(result3.first);
}

/**
 * @tc.name: IntFindAfterMultipleClears
 * @tc.desc: test find operation with integer key after multiple clear operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntFindAfterMultipleClears, TestSize.Level0)
{
    TestValueAlpha value1 = { "int_multi_clear_id_1", "int_multi_clear_name_1", "int_multi_clear_scenario_1" };
    dataBeta_.Emplace(100, value1);
    auto result1 = dataBeta_.Find(100);
    ASSERT_TRUE(result1.first);
    dataBeta_.Clear();
    TestValueAlpha value2 = { "int_multi_clear_id_2", "int_multi_clear_name_2", "int_multi_clear_scenario_2" };
    dataBeta_.Emplace(100, value2);
    auto result2 = dataBeta_.Find(100);
    ASSERT_TRUE(result2.first);
    ASSERT_EQ(result2.second.label, value2.label);
    dataBeta_.Clear();
    auto result3 = dataBeta_.Find(100);
    ASSERT_FALSE(result3.first);
}

/**
 * @tc.name: UintFindAfterMultipleClears
 * @tc.desc: test find operation with unsigned integer key after multiple clear operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintFindAfterMultipleClears, TestSize.Level0)
{
    TestValueAlpha value1 = { "uint_multi_clear_id_1", "uint_multi_clear_name_1", "uint_multi_clear_scenario_1" };
    dataGamma_.Emplace(200, value1);
    auto result1 = dataGamma_.Find(200);
    ASSERT_TRUE(result1.first);
    dataGamma_.Clear();
    TestValueAlpha value2 = { "uint_multi_clear_id_2", "uint_multi_clear_name_2", "uint_multi_clear_scenario_2" };
    dataGamma_.Emplace(200, value2);
    auto result2 = dataGamma_.Find(200);
    ASSERT_TRUE(result2.first);
    ASSERT_EQ(result2.second.scenario, value2.scenario);
    dataGamma_.Clear();
    auto result3 = dataGamma_.Find(200);
    ASSERT_FALSE(result3.first);
}

/**
 * @tc.name: SizeConsistencyCheck
 * @tc.desc: test size consistency after various operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, SizeConsistencyCheck, TestSize.Level0)
{
    ASSERT_EQ(dataAlpha_.Size(), 0);
    for (int i = 0; i < 100; ++i) {
        std::string key = "consistency_key_" + std::to_string(i);
        TestValueAlpha value = { "consistency_id_" + std::to_string(i),
                                  "consistency_name_" + std::to_string(i),
                                  "consistency_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
        ASSERT_EQ(dataAlpha_.Size(), i + 1);
    }
    for (int i = 0; i < 50; ++i) {
        std::string key = "consistency_key_" + std::to_string(i);
        auto result = dataAlpha_.Find(key);
        ASSERT_TRUE(result.first);
    }
    ASSERT_EQ(dataAlpha_.Size(), 100);
    dataAlpha_.Clear();
    ASSERT_EQ(dataAlpha_.Size(), 0);
}

/**
 * @tc.name: IntSizeConsistencyCheck
 * @tc.desc: test size consistency with integer keys after various operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntSizeConsistencyCheck, TestSize.Level0)
{
    ASSERT_EQ(dataBeta_.Size(), 0);
    for (int i = 0; i < 120; ++i) {
        TestValueAlpha value = { "int_consistency_id_" + std::to_string(i),
                                  "int_consistency_name_" + std::to_string(i),
                                  "int_consistency_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i * 5, value);
        ASSERT_EQ(dataBeta_.Size(), i + 1);
    }
    for (int i = 0; i < 60; ++i) {
        int key = i * 5;
        auto result = dataBeta_.Find(key);
        ASSERT_TRUE(result.first);
    }
    ASSERT_EQ(dataBeta_.Size(), 120);
    dataBeta_.Clear();
    ASSERT_EQ(dataBeta_.Size(), 0);
}

/**
 * @tc.name: UintSizeConsistencyCheck
 * @tc.desc: test size consistency with unsigned integer keys after various operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintSizeConsistencyCheck, TestSize.Level0)
{
    ASSERT_EQ(dataGamma_.Size(), 0);
    for (uint32_t i = 0; i < 140; ++i) {
        TestValueAlpha value = { "uint_consistency_id_" + std::to_string(i),
                                  "uint_consistency_name_" + std::to_string(i),
                                  "uint_consistency_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i * 11, value);
        ASSERT_EQ(dataGamma_.Size(), i + 1);
    }
    for (uint32_t i = 0; i < 70; ++i) {
        uint32_t key = i * 11;
        auto result = dataGamma_.Find(key);
        ASSERT_TRUE(result.first);
    }
    ASSERT_EQ(dataGamma_.Size(), 140);
    dataGamma_.Clear();
    ASSERT_EQ(dataGamma_.Size(), 0);
}

/**
 * @tc.name: EmplaceWithSpecialCharacters
 * @tc.desc: test emplace operation with special characters in strings
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithSpecialCharacters, TestSize.Level0)
{
    TestValueAlpha value = { "id_with_underscore", "name-with-dash", "scenario.with.dot" };
    dataAlpha_.Emplace("special_char_key", value);
    auto result = dataAlpha_.Find("special_char_key");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "id_with_underscore");
    ASSERT_EQ(result.second.label, "name-with-dash");
    ASSERT_EQ(result.second.scenario, "scenario.with.dot");
}

/**
 * @tc.name: EmplaceWithNumericStrings
 * @tc.desc: test emplace operation with numeric strings
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithNumericStrings, TestSize.Level0)
{
    TestValueAlpha value = { "12345", "67890", "54321" };
    dataAlpha_.Emplace("numeric_string_key", value);
    auto result = dataAlpha_.Find("numeric_string_key");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "12345");
    ASSERT_EQ(result.second.label, "67890");
    ASSERT_EQ(result.second.scenario, "54321");
}

/**
 * @tc.name: FindWithCaseSensitive
 * @tc.desc: test find operation with case sensitive strings
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindWithCaseSensitive, TestSize.Level0)
{
    TestValueAlpha value = { "CaseSensitiveID", "casesensitivelabel", "CaseSensitiveScenario" };
    dataAlpha_.Emplace("CaseSensitiveKey", value);
    auto result1 = dataAlpha_.Find("CaseSensitiveKey");
    ASSERT_TRUE(result1.first);
    auto result2 = dataAlpha_.Find("casesensitivekey");
    ASSERT_FALSE(result2.first);
}

/**
 * @tc.name: MultipleMapsIndependent
 * @tc.desc: test multiple maps operate independently
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, MultipleMapsIndependent, TestSize.Level0)
{
    TestValueAlpha valueAlpha = { "alpha_id", "alpha_name", "alpha_scenario" };
    TestValueAlpha valueBeta = { "beta_id", "beta_name", "beta_scenario" };
    TestValueAlpha valueGamma = { "gamma_id", "gamma_name", "gamma_scenario" };
    
    dataAlpha_.Emplace("alpha_key", valueAlpha);
    dataBeta_.Emplace(1, valueBeta);
    dataGamma_.Emplace(1, valueGamma);
    
    ASSERT_EQ(dataAlpha_.Size(), 1);
    ASSERT_EQ(dataBeta_.Size(), 1);
    ASSERT_EQ(dataGamma_.Size(), 1);
    
    auto resultAlpha = dataAlpha_.Find("alpha_key");
    auto resultBeta = dataBeta_.Find(1);
    auto resultGamma = dataGamma_.Find(1);
    
    ASSERT_TRUE(resultAlpha.first);
    ASSERT_TRUE(resultBeta.first);
    ASSERT_TRUE(resultGamma.first);
    
    ASSERT_EQ(resultAlpha.second.identifier, "alpha_id");
    ASSERT_EQ(resultBeta.second.label, "beta_name");
    ASSERT_EQ(resultGamma.second.scenario, "gamma_scenario");
}

/**
 * @tc.name: EmplaceWithNegativeIntKey
 * @tc.desc: test emplace operation with negative integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithNegativeIntKey, TestSize.Level0)
{
    TestValueAlpha value = { "negative_id", "negative_name", "negative_scenario" };
    dataBeta_.Emplace(-100, value);
    auto result = dataBeta_.Find(-100);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "negative_id");
}

/**
 * @tc.name: EmplaceWithZeroIntKey
 * @tc.desc: test emplace operation with zero integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithZeroIntKey, TestSize.Level0)
{
    TestValueAlpha value = { "zero_id", "zero_name", "zero_scenario" };
    dataBeta_.Emplace(0, value);
    auto result = dataBeta_.Find(0);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.label, "zero_name");
}

/**
 * @tc.name: EmplaceWithLargeIntKey
 * @tc.desc: test emplace operation with large integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithLargeIntKey, TestSize.Level0)
{
    TestValueAlpha value = { "large_id", "large_name", "large_scenario" };
    dataBeta_.Emplace(999999, value);
    auto result = dataBeta_.Find(999999);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.scenario, "large_scenario");
}

/**
 * @tc.name: EmplaceWithLargeUintKey
 * @tc.desc: test emplace operation with large unsigned integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithLargeUintKey, TestSize.Level0)
{
    TestValueAlpha value = { "uint_large_id", "uint_large_name", "uint_large_scenario" };
    dataGamma_.Emplace(888888, value);
    auto result = dataGamma_.Find(888888);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "uint_large_id");
}

/**
 * @tc.name: EmplaceWithZeroUintKey
 * @tc.desc: test emplace operation with zero unsigned integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithZeroUintKey, TestSize.Level0)
{
    TestValueAlpha value = { "uint_zero_id", "uint_zero_name", "uint_zero_scenario" };
    dataGamma_.Emplace(0, value);
    auto result = dataGamma_.Find(0);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.label, "uint_zero_name");
}

/**
 * @tc.name: EmplaceUintMaxKey
 * @tc.desc: test emplace operation with maximum unsigned integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceUintMaxKey, TestSize.Level0)
{
    TestValueAlpha value = { "max_id", "max_name", "max_scenario" };
    uint32_t maxKey = UINT32_MAX;
    dataGamma_.Emplace(maxKey, value);
    auto result = dataGamma_.Find(maxKey);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.scenario, "max_scenario");
}

/**
 * @tc.name: MixedKeyTypes
 * @tc.desc: test operations with mixed key types
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, MixedKeyTypes, TestSize.Level0)
{
    TestValueAlpha value1 = { "mixed_id_1", "mixed_name_1", "mixed_scenario_1" };
    TestValueAlpha value2 = { "mixed_id_2", "mixed_name_2", "mixed_scenario_2" };
    TestValueAlpha value3 = { "mixed_id_3", "mixed_name_3", "mixed_scenario_3" };
    
    dataAlpha_.Emplace("mixed_string_key", value1);
    dataBeta_.Emplace(777, value2);
    dataGamma_.Emplace(666, value3);
    
    auto result1 = dataAlpha_.Find("mixed_string_key");
    auto result2 = dataBeta_.Find(777);
    auto result3 = dataGamma_.Find(666);
    
    ASSERT_TRUE(result1.first);
    ASSERT_TRUE(result2.first);
    ASSERT_TRUE(result3.first);
    
    ASSERT_EQ(result1.second.identifier, "mixed_id_1");
    ASSERT_EQ(result2.second.label, "mixed_name_2");
    ASSERT_EQ(result3.second.scenario, "mixed_scenario_3");
}

/**
 * @tc.name: EmptyStringKey
 * @tc.desc: test emplace operation with empty string key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmptyStringKey, TestSize.Level0)
{
    TestValueAlpha value = { "empty_key_id", "empty_key_name", "empty_key_scenario" };
    dataAlpha_.Emplace("", value);
    auto result = dataAlpha_.Find("");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "empty_key_id");
}

/**
 * @tc.name: VeryLongStringKey
 * @tc.desc: test emplace operation with very long string key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, VeryLongStringKey, TestSize.Level0)
{
    std::string veryLongKey = "very_long_key_" + std::string(500, 'x');
    TestValueAlpha value = { "very_long_key_id", "very_long_key_name", "very_long_key_scenario" };
    dataAlpha_.Emplace(veryLongKey, value);
    auto result = dataAlpha_.Find(veryLongKey);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.label, "very_long_key_name");
}

/**
 * @tc.name: SequentialIntKeys
 * @tc.desc: test emplace and find with sequential integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, SequentialIntKeys, TestSize.Level0)
{
    for (int i = 0; i < 500; ++i) {
        TestValueAlpha value = { "seq_id_" + std::to_string(i),
                                  "seq_name_" + std::to_string(i),
                                  "seq_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i, value);
    }
    for (int i = 0; i < 500; ++i) {
        auto result = dataBeta_.Find(i);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.identifier, "seq_id_" + std::to_string(i));
    }
    ASSERT_EQ(dataBeta_.Size(), 500);
}

/**
 * @tc.name: SequentialUintKeys
 * @tc.desc: test emplace and find with sequential unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, SequentialUintKeys, TestSize.Level0)
{
    for (uint32_t i = 0; i < 600; ++i) {
        TestValueAlpha value = { "uint_seq_id_" + std::to_string(i),
                                  "uint_seq_name_" + std::to_string(i),
                                  "uint_seq_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i, value);
    }
    for (uint32_t i = 0; i < 600; ++i) {
        auto result = dataGamma_.Find(i);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.label, "uint_seq_name_" + std::to_string(i));
    }
    ASSERT_EQ(dataGamma_.Size(), 600);
}

/**
 * @tc.name: ReverseIntKeys
 * @tc.desc: test emplace and find with reverse order integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, ReverseIntKeys, TestSize.Level0)
{
    for (int i = 300; i >= 0; --i) {
        TestValueAlpha value = { "rev_id_" + std::to_string(i),
                                  "rev_name_" + std::to_string(i),
                                  "rev_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i, value);
    }
    for (int i = 0; i <= 300; ++i) {
        auto result = dataBeta_.Find(i);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.scenario, "rev_scenario_" + std::to_string(i));
    }
    ASSERT_EQ(dataBeta_.Size(), 301);
}

/**
 * @tc.name: ReverseUintKeys
 * @tc.desc: test emplace and find with reverse order unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, ReverseUintKeys, TestSize.Level0)
{
    for (int i = 200; i >= 0; --i) {
        TestValueAlpha value = { "uint_rev_id_" + std::to_string(i),
                                  "uint_rev_name_" + std::to_string(i),
                                  "uint_rev_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(static_cast<uint32_t>(i), value);
    }
    for (uint32_t i = 0; i <= 200; ++i) {
        auto result = dataGamma_.Find(i);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.identifier, "uint_rev_id_" + std::to_string(i));
    }
    ASSERT_EQ(dataGamma_.Size(), 201);
}

/**
 * @tc.name: RandomAccessTest
 * @tc.desc: test random access pattern with string keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, RandomAccessTest, TestSize.Level0)
{
    std::vector<std::string> keys;
    for (int i = 0; i < 300; ++i) {
        std::string key = "random_access_key_" + std::to_string(i);
        TestValueAlpha value = { "random_access_id_" + std::to_string(i),
                                  "random_access_name_" + std::to_string(i),
                                  "random_access_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
        keys.push_back(key);
    }
    for (size_t i = 0; i < keys.size(); i += 3) {
        auto result = dataAlpha_.Find(keys[i]);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.label, "random_access_name_" + std::to_string(i));
    }
    ASSERT_EQ(dataAlpha_.Size(), 300);
}

/**
 * @tc.name: IntRandomAccessTest
 * @tc.desc: test random access pattern with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntRandomAccessTest, TestSize.Level0)
{
    std::vector<int> keys;
    for (int i = 0; i < 400; ++i) {
        TestValueAlpha value = { "int_random_access_id_" + std::to_string(i), "int_random_access_name_" + std::to_string(i), "int_random_access_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i * 13, value);
        keys.push_back(i * 13);
    }
    for (size_t i = 0; i < keys.size(); i += 7) {
        auto result = dataBeta_.Find(keys[i]);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.scenario, "int_random_access_scenario_" + std::to_string(i));
    }
    ASSERT_EQ(dataBeta_.Size(), 400);
}

/**
 * @tc.name: UintRandomAccessTest
 * @tc.desc: test random access pattern with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintRandomAccessTest, TestSize.Level0)
{
    std::vector<uint32_t> keys;
    for (uint32_t i = 0; i < 500; ++i) {
        TestValueAlpha value = { "uint_random_access_id_" + std::to_string(i), "uint_random_access_name_" + std::to_string(i), "uint_random_access_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i * 17, value);
        keys.push_back(i * 17);
    }
    for (size_t i = 0; i < keys.size(); i += 11) {
        auto result = dataGamma_.Find(keys[i]);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.identifier, "uint_random_access_id_" + std::to_string(i));
    }
    ASSERT_EQ(dataGamma_.Size(), 500);
}

/**
 * @tc.name: OverwriteWithClear
 * @tc.desc: test overwriting data with clear operation
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, OverwriteWithClear, TestSize.Level0)
{
    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < 50; ++i) {
            std::string key = "overwrite_key_" + std::to_string(i);
            TestValueAlpha value = { "overwrite_id_" + std::to_string(i) + "_round_" + std::to_string(round), "overwrite_name_" + std::to_string(i), "overwrite_scenario_" + std::to_string(i) };
            dataAlpha_.Emplace(key, value);
        }
        ASSERT_EQ(dataAlpha_.Size(), 50);
        dataAlpha_.Clear();
        ASSERT_EQ(dataAlpha_.Size(), 0);
    }
}

/**
 * @tc.name: IntOverwriteWithClear
 * @tc.desc: test overwriting data with clear operation using integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntOverwriteWithClear, TestSize.Level0)
{
    for (int round = 0; round < 15; ++round) {
        for (int i = 0; i < 60; ++i) {
            TestValueAlpha value = { "int_overwrite_id_" + std::to_string(i) + "_round_" + std::to_string(round), "int_overwrite_name_" + std::to_string(i), "int_overwrite_scenario_" + std::to_string(i) };
            dataBeta_.Emplace(i, value);
        }
        ASSERT_EQ(dataBeta_.Size(), 60);
        dataBeta_.Clear();
        ASSERT_EQ(dataBeta_.Size(), 0);
    }
}

/**
 * @tc.name: UintOverwriteWithClear
 * @tc.desc: test overwriting data with clear operation using unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintOverwriteWithClear, TestSize.Level0)
{
    for (int round = 0; round < 20; ++round) {
        for (uint32_t i = 0; i < 70; ++i) {
            TestValueAlpha value = { "uint_overwrite_id_" + std::to_string(i) + "_round_" + std::to_string(round), "uint_overwrite_name_" + std::to_string(i), "uint_overwrite_scenario_" + std::to_string(i) };
            dataGamma_.Emplace(i, value);
        }
        ASSERT_EQ(dataGamma_.Size(), 70);
        dataGamma_.Clear();
        ASSERT_EQ(dataGamma_.Size(), 0);
    }
}

/**
 * @tc.name: StressTestLargeScale
 * @tc.desc: test stress operations with large scale data
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, StressTestLargeScale, TestSize.Level0)
{
    for (int i = 0; i < 1000; ++i) {
        std::string key = "stress_key_" + std::to_string(i);
        TestValueAlpha value = { "stress_id_" + std::to_string(i), "stress_name_" + std::to_string(i), "stress_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    ASSERT_EQ(dataAlpha_.Size(), 1000);
    for (int i = 0; i < 1000; ++i) {
        std::string key = "stress_key_" + std::to_string(i);
        auto result = dataAlpha_.Find(key);
        ASSERT_TRUE(result.first);
    }
    dataAlpha_.Clear();
    ASSERT_TRUE(dataAlpha_.Empty());
}

/**
 * @tc.name: IntStressTestLargeScale
 * @tc.desc: test stress operations with large scale data using integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntStressTestLargeScale, TestSize.Level0)
{
    for (int i = 0; i < 500; ++i) {
        TestValueAlpha value = { "int_stress_id_" + std::to_string(i), "int_stress_name_" + std::to_string(i), "int_stress_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i, value);
    }
    ASSERT_EQ(dataBeta_.Size(), 500);
    for (int i = 0; i < 500; ++i) {
        auto result = dataBeta_.Find(i);
        ASSERT_TRUE(result.first);
    }
    dataBeta_.Clear();
    ASSERT_TRUE(dataBeta_.Empty());
}

/**
 * @tc.name: UintStressTestLargeScale
 * @tc.desc: test stress operations with large scale data using unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintStressTestLargeScale, TestSize.Level0)
{
    for (uint32_t i = 0; i < 100; ++i) {
        TestValueAlpha value = { "uint_stress_id_" + std::to_string(i), "uint_stress_name_" + std::to_string(i), "uint_stress_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i, value);
    }
    ASSERT_EQ(dataGamma_.Size(), 100);
    for (uint32_t i = 0; i < 100; ++i) {
        auto result = dataGamma_.Find(i);
        ASSERT_TRUE(result.first);
    }
    dataGamma_.Clear();
    ASSERT_TRUE(dataGamma_.Empty());
}

/**
 * @tc.name: FindWithSimilarKeys
 * @tc.desc: test find operation with similar keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindWithSimilarKeys, TestSize.Level0)
{
    TestValueAlpha value = { "similar_id", "similar_name", "similar_scenario" };
    dataAlpha_.Emplace("key_alpha", value);
    dataAlpha_.Emplace("key_beta", value);
    dataAlpha_.Emplace("key_gamma", value);
    
    auto result1 = dataAlpha_.Find("key_alpha");
    auto result2 = dataAlpha_.Find("key_beta");
    auto result3 = dataAlpha_.Find("key_gamma");
    
    ASSERT_TRUE(result1.first);
    ASSERT_TRUE(result2.first);
    ASSERT_TRUE(result3.first);
    
    auto result4 = dataAlpha_.Find("key_alph");
    auto result5 = dataAlpha_.Find("key_bet");
    auto result6 = dataAlpha_.Find("key_gam");
    
    ASSERT_FALSE(result4.first);
    ASSERT_FALSE(result5.first);
    ASSERT_FALSE(result6.first);
}

/**
 * @tc.name: IntFindWithSimilarKeys
 * @tc.desc: test find operation with similar integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntFindWithSimilarKeys, TestSize.Level0)
{
    TestValueAlpha value = { "int_similar_id", "int_similar_name", "int_similar_scenario" };
    dataBeta_.Emplace(1000, value);
    dataBeta_.Emplace(1001, value);
    dataBeta_.Emplace(1002, value);
    
    auto result1 = dataBeta_.Find(1000);
    auto result2 = dataBeta_.Find(1001);
    auto result3 = dataBeta_.Find(1002);
    
    ASSERT_TRUE(result1.first);
    ASSERT_TRUE(result2.first);
    ASSERT_TRUE(result3.first);
    
    auto result4 = dataBeta_.Find(999);
    auto result5 = dataBeta_.Find(1003);
    
    ASSERT_FALSE(result4.first);
    ASSERT_FALSE(result5.first);
}

/**
 * @tc.name: UintFindWithSimilarKeys
 * @tc.desc: test find operation with similar unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintFindWithSimilarKeys, TestSize.Level0)
{
    TestValueAlpha value = { "uint_similar_id", "uint_similar_name", "uint_similar_scenario" };
    dataGamma_.Emplace(2000, value);
    dataGamma_.Emplace(2001, value);
    dataGamma_.Emplace(2002, value);
    
    auto result1 = dataGamma_.Find(2000);
    auto result2 = dataGamma_.Find(2001);
    auto result3 = dataGamma_.Find(2002);
    
    ASSERT_TRUE(result1.first);
    ASSERT_TRUE(result2.first);
    ASSERT_TRUE(result3.first);
    
    auto result4 = dataGamma_.Find(1999);
    auto result5 = dataGamma_.Find(2003);
    
    ASSERT_FALSE(result4.first);
    ASSERT_FALSE(result5.first);
}

/**
 * @tc.name: ClearFindEmplaceSequence
 * @tc.desc: test sequence of clear, find, and emplace operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, ClearFindEmplaceSequence, TestSize.Level0)
{
    dataAlpha_.Clear();
    auto result1 = dataAlpha_.Find("seq_key");
    ASSERT_FALSE(result1.first);
    
    TestValueAlpha value = { "seq_id", "seq_name", "seq_scenario" };
    dataAlpha_.Emplace("seq_key", value);
    
    auto result2 = dataAlpha_.Find("seq_key");
    ASSERT_TRUE(result2.first);
    
    dataAlpha_.Clear();
    
    auto result3 = dataAlpha_.Find("seq_key");
    ASSERT_FALSE(result3.first);
    
    dataAlpha_.Emplace("seq_key", value);
    
    auto result4 = dataAlpha_.Find("seq_key");
    ASSERT_TRUE(result4.first);
}

/**
 * @tc.name: IntClearFindEmplaceSequence
 * @tc.desc: test sequence of clear, find, and emplace operations with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntClearFindEmplaceSequence, TestSize.Level0)
{
    dataBeta_.Clear();
    auto result1 = dataBeta_.Find(999);
    ASSERT_FALSE(result1.first);
    
    TestValueAlpha value = { "int_seq_id", "int_seq_name", "int_seq_scenario" };
    dataBeta_.Emplace(999, value);
    
    auto result2 = dataBeta_.Find(999);
    ASSERT_TRUE(result2.first);
    
    dataBeta_.Clear();
    
    auto result3 = dataBeta_.Find(999);
    ASSERT_FALSE(result3.first);
    
    dataBeta_.Emplace(999, value);
    
    auto result4 = dataBeta_.Find(999);
    ASSERT_TRUE(result4.first);
}

/**
 * @tc.name: UintClearFindEmplaceSequence
 * @tc.desc: test sequence of clear, find, and emplace operations with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintClearFindEmplaceSequence, TestSize.Level0)
{
    dataGamma_.Clear();
    auto result1 = dataGamma_.Find(888);
    ASSERT_FALSE(result1.first);
    
    TestValueAlpha value = { "uint_seq_id", "uint_seq_name", "uint_seq_scenario" };
    dataGamma_.Emplace(888, value);
    
    auto result2 = dataGamma_.Find(888);
    ASSERT_TRUE(result2.first);
    
    dataGamma_.Clear();
    
    auto result3 = dataGamma_.Find(888);
    ASSERT_FALSE(result3.first);
    
    dataGamma_.Emplace(888, value);
    
    auto result4 = dataGamma_.Find(888);
    ASSERT_TRUE(result4.first);
}

/**
 * @tc.name: VerifyAllFields
 * @tc.desc: test verification of all fields in value structure
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, VerifyAllFields, TestSize.Level0)
{
    TestValueAlpha value = { "verify_id_12345", "verify_name_abcde", "verify_scenario_xyz" };
    dataAlpha_.Emplace("verify_key", value);
    auto result = dataAlpha_.Find("verify_key");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "verify_id_12345");
    ASSERT_EQ(result.second.label, "verify_name_abcde");
    ASSERT_EQ(result.second.scenario, "verify_scenario_xyz");
}

/**
 * @tc.name: VerifyAllFieldsInt
 * @tc.desc: test verification of all fields with integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, VerifyAllFieldsInt, TestSize.Level0)
{
    TestValueAlpha value = { "verify_int_id_999", "verify_int_name_888", "verify_int_scenario_777" };
    dataBeta_.Emplace(555, value);
    auto result = dataBeta_.Find(555);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "verify_int_id_999");
    ASSERT_EQ(result.second.label, "verify_int_name_888");
    ASSERT_EQ(result.second.scenario, "verify_int_scenario_777");
}

/**
 * @tc.name: VerifyAllFieldsUint
 * @tc.desc: test verification of all fields with unsigned integer key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, VerifyAllFieldsUint, TestSize.Level0)
{
    TestValueAlpha value = { "verify_uint_id_444", "verify_uint_name_333", "verify_uint_scenario_222" };
    dataGamma_.Emplace(111, value);
    auto result = dataGamma_.Find(111);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "verify_uint_id_444");
    ASSERT_EQ(result.second.label, "verify_uint_name_333");
    ASSERT_EQ(result.second.scenario, "verify_uint_scenario_222");
}

/**
 * @tc.name: MultipleEmplaceWithSameValue
 * @tc.desc: test multiple emplace operations with same value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, MultipleEmplaceWithSameValue, TestSize.Level0)
{
    TestValueAlpha value = { "same_id", "same_name", "same_scenario" };
    for (int i = 0; i < 100; ++i) {
        std::string key = "same_value_key_" + std::to_string(i);
        dataAlpha_.Emplace(key, value);
    }
    ASSERT_EQ(dataAlpha_.Size(), 100);
    for (int i = 0; i < 100; ++i) {
        std::string key = "same_value_key_" + std::to_string(i);
        auto result = dataAlpha_.Find(key);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.identifier, "same_id");
    }
}

/**
 * @tc.name: IntMultipleEmplaceWithSameValue
 * @tc.desc: test multiple emplace operations with same value using integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntMultipleEmplaceWithSameValue, TestSize.Level0)
{
    TestValueAlpha value = { "int_same_id", "int_same_name", "int_same_scenario" };
    for (int i = 0; i < 150; ++i) {
        dataBeta_.Emplace(i, value);
    }
    ASSERT_EQ(dataBeta_.Size(), 150);
    for (int i = 0; i < 150; ++i) {
        auto result = dataBeta_.Find(i);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.label, "int_same_name");
    }
}

/**
 * @tc.name: UintMultipleEmplaceWithSameValue
 * @tc.desc: test multiple emplace operations with same value using unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintMultipleEmplaceWithSameValue, TestSize.Level0)
{
    TestValueAlpha value = { "uint_same_id", "uint_same_name", "uint_same_scenario" };
    for (uint32_t i = 0; i < 200; ++i) {
        dataGamma_.Emplace(i, value);
    }
    ASSERT_EQ(dataGamma_.Size(), 200);
    for (uint32_t i = 0; i < 200; ++i) {
        auto result = dataGamma_.Find(i);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.scenario, "uint_same_scenario");
    }
}

/**
 * @tc.name: EmptyAfterClearAndRefill
 * @tc.desc: test empty state after clear and refill operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmptyAfterClearAndRefill, TestSize.Level0)
{
    for (int i = 0; i < 50; ++i) {
        std::string key = "refill_key_" + std::to_string(i);
        TestValueAlpha value = { "refill_id_" + std::to_string(i), "refill_name_" + std::to_string(i), "refill_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    ASSERT_EQ(dataAlpha_.Size(), 50);
    dataAlpha_.Clear();
    ASSERT_TRUE(dataAlpha_.Empty());
    for (int i = 0; i < 30; ++i) {
        std::string key = "refill_new_key_" + std::to_string(i);
        TestValueAlpha value = { "refill_new_id_" + std::to_string(i), "refill_new_name_" + std::to_string(i), "refill_new_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    ASSERT_EQ(dataAlpha_.Size(), 30);
}

/**
 * @tc.name: IntEmptyAfterClearAndRefill
 * @tc.desc: test empty state after clear and refill operations with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntEmptyAfterClearAndRefill, TestSize.Level0)
{
    for (int i = 0; i < 60; ++i) {
        TestValueAlpha value = { "int_refill_id_" + std::to_string(i), "int_refill_name_" + std::to_string(i), "int_refill_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i, value);
    }
    ASSERT_EQ(dataBeta_.Size(), 60);
    dataBeta_.Clear();
    ASSERT_TRUE(dataBeta_.Empty());
    for (int i = 0; i < 40; ++i) {
        TestValueAlpha value = { "int_refill_new_id_" + std::to_string(i), "int_refill_new_name_" + std::to_string(i), "int_refill_new_scenario_" + std::to_string(i) };
        dataBeta_.Emplace(i, value);
    }
    ASSERT_EQ(dataBeta_.Size(), 40);
}

/**
 * @tc.name: UintEmptyAfterClearAndRefill
 * @tc.desc: test empty state after clear and refill operations with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintEmptyAfterClearAndRefill, TestSize.Level0)
{
    for (uint32_t i = 0; i < 70; ++i) {
        TestValueAlpha value = { "uint_refill_id_" + std::to_string(i), "uint_refill_name_" + std::to_string(i), "uint_refill_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i, value);
    }
    ASSERT_EQ(dataGamma_.Size(), 70);
    dataGamma_.Clear();
    ASSERT_TRUE(dataGamma_.Empty());
    for (uint32_t i = 0; i < 50; ++i) {
        TestValueAlpha value = { "uint_refill_new_id_" + std::to_string(i), "uint_refill_new_name_" + std::to_string(i), "uint_refill_new_scenario_" + std::to_string(i) };
        dataGamma_.Emplace(i, value);
    }
    ASSERT_EQ(dataGamma_.Size(), 50);
}

/**
 * @tc.name: FindWithPartialKey
 * @tc.desc: test find operation with partial key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindWithPartialKey, TestSize.Level0)
{
    TestValueAlpha value = { "partial_id", "partial_name", "partial_scenario" };
    dataAlpha_.Emplace("full_complete_key", value);
    auto result1 = dataAlpha_.Find("full_complete_key");
    ASSERT_TRUE(result1.first);
    auto result2 = dataAlpha_.Find("full");
    ASSERT_FALSE(result2.first);
    auto result3 = dataAlpha_.Find("key");
    ASSERT_FALSE(result3.first);
}

/**
 * @tc.name: FindKeyWithSpaces
 * @tc.desc: test find operation with keys containing spaces
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FindKeyWithSpaces, TestSize.Level0)
{
    TestValueAlpha value = { "space_id", "space_name", "space_scenario" };
    dataAlpha_.Emplace("key with spaces", value);
    auto result1 = dataAlpha_.Find("key with spaces");
    ASSERT_TRUE(result1.first);
    auto result2 = dataAlpha_.Find("keywithspaces");
    ASSERT_FALSE(result2.first);
}

/**
 * @tc.name: VerifyAfterMultipleEmplaceClear
 * @tc.desc: test verification after multiple emplace and clear cycles
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, VerifyAfterMultipleEmplaceClear, TestSize.Level0)
{
    for (int cycle = 0; cycle < 5; ++cycle) {
        for (int i = 0; i < 20; ++i) {
            std::string key = "cycle_" + std::to_string(cycle) + "_key_" + std::to_string(i);
            TestValueAlpha value = { "cycle_id_" + std::to_string(i), "cycle_name_" + std::to_string(i), "cycle_scenario_" + std::to_string(i) };
            dataAlpha_.Emplace(key, value);
        }
        ASSERT_EQ(dataAlpha_.Size(), 20);
        dataAlpha_.Clear();
        ASSERT_TRUE(dataAlpha_.Empty());
    }
}

/**
 * @tc.name: IntVerifyAfterMultipleEmplaceClear
 * @tc.desc: test verification after multiple emplace and clear cycles with integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, IntVerifyAfterMultipleEmplaceClear, TestSize.Level0)
{
    for (int cycle = 0; cycle < 8; ++cycle) {
        for (int i = 0; i < 25; ++i) {
            TestValueAlpha value = { "int_cycle_id_" + std::to_string(i), "int_cycle_name_" + std::to_string(i), "int_cycle_scenario_" + std::to_string(i) };
            dataBeta_.Emplace(i, value);
        }
        ASSERT_EQ(dataBeta_.Size(), 25);
        dataBeta_.Clear();
        ASSERT_TRUE(dataBeta_.Empty());
    }
}

/**
 * @tc.name: UintVerifyAfterMultipleEmplaceClear
 * @tc.desc: test verification after multiple emplace and clear cycles with unsigned integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, UintVerifyAfterMultipleEmplaceClear, TestSize.Level0)
{
    for (int cycle = 0; cycle < 10; ++cycle) {
        for (uint32_t i = 0; i < 30; ++i) {
            TestValueAlpha value = { "uint_cycle_id_" + std::to_string(i), "uint_cycle_name_" + std::to_string(i), "uint_cycle_scenario_" + std::to_string(i) };
            dataGamma_.Emplace(i, value);
        }
        ASSERT_EQ(dataGamma_.Size(), 30);
        dataGamma_.Clear();
        ASSERT_TRUE(dataGamma_.Empty());
    }
}

/**
 * @tc.name: EmplaceWithUnicodeCharacters
 * @tc.desc: test emplace operation with unicode characters
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, EmplaceWithUnicodeCharacters, TestSize.Level0)
{
    TestValueAlpha value = { "unicode_id_abc", "unicode_name_xyz", "unicode_scenario_123" };
    dataAlpha_.Emplace("unicode_key_test", value);
    auto result = dataAlpha_.Find("unicode_key_test");
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second.identifier, "unicode_id_abc");
}

/**
 * @tc.name: FinalComprehensiveTest
 * @tc.desc: comprehensive test with all operations combined
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentMapTestNew, FinalComprehensiveTest, TestSize.Level0)
{
    ASSERT_TRUE(dataAlpha_.Empty());
    ASSERT_EQ(dataAlpha_.Size(), 0);
    
    for (int i = 0; i < 100; ++i) {
        std::string key = "final_key_" + std::to_string(i);
        TestValueAlpha value = { "final_id_" + std::to_string(i), "final_name_" + std::to_string(i), "final_scenario_" + std::to_string(i) };
        dataAlpha_.Emplace(key, value);
    }
    
    ASSERT_EQ(dataAlpha_.Size(), 100);
    ASSERT_FALSE(dataAlpha_.Empty());
    
    for (int i = 0; i < 100; i += 2) {
        std::string key = "final_key_" + std::to_string(i);
        auto result = dataAlpha_.Find(key);
        ASSERT_TRUE(result.first);
        ASSERT_EQ(result.second.identifier, "final_id_" + std::to_string(i));
    }
    
    dataAlpha_.Clear();
    ASSERT_TRUE(dataAlpha_.Empty());
    ASSERT_EQ(dataAlpha_.Size(), 0);
}

} // namespace OHOS::Test
