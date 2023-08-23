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
#include "traits.h"

#include <gtest/gtest.h>
using namespace testing::ext;
namespace OHOS::Test {
class TraitsTest : public testing::Test {
public:
    class From {
    public:
        From() {}
    };
    class Convertible {
    public:
        // Convertible is auto convert type, do not add explicit to stop the type convert.
        Convertible(const From &) {};
        Convertible() {}
        Convertible(Convertible &&) noexcept {};
        Convertible &operator=(Convertible &&) noexcept
        {
            return *this;
        }
        operator From()
        {
            return From();
        }
    };
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown() {}
};

/**
* @tc.name: same_index_of_v
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, same_index_of_v, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<int32_t, int32_t, double, std::vector<uint8_t>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::string, int32_t, double, std::vector<uint8_t>>;
    ASSERT_EQ(index, 3);
    index = Traits::same_index_of_v<std::string>;
    ASSERT_EQ(index, 0);
}

/**
* @tc.name: same_in_v
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, same_in_v, TestSize.Level0)
{
    auto exist = Traits::same_in_v<int32_t, int32_t, double, std::vector<uint8_t>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::string, int32_t, double, std::vector<uint8_t>>;
    ASSERT_FALSE(exist);
    exist = Traits::same_in_v<std::string>;
    ASSERT_FALSE(exist);
}

/**
* @tc.name: convertible_index_of_v
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, convertible_index_of_v, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<int32_t, int16_t, double, std::vector<uint8_t>>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<std::string, int16_t, const char *, std::vector<uint8_t>>;
    ASSERT_EQ(index, 1);
    index = Traits::convertible_index_of_v<std::string, int16_t, std::vector<uint8_t>>;
    ASSERT_EQ(index, 2);
    index = Traits::convertible_index_of_v<std::string>;
    ASSERT_EQ(index, 0);
}

/**
* @tc.name: convertible_in_v
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, convertible_in_v, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<int32_t, int16_t, double, std::vector<uint8_t>>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<std::string, int16_t, const char *, std::vector<uint8_t>>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<std::string, int16_t, std::vector<uint8_t>>;
    ASSERT_FALSE(convertible);
    convertible = Traits::convertible_in_v<std::string>;
    ASSERT_FALSE(convertible);
}

/**
* @tc.name: variant_size_of_v
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, variant_size_of_v, TestSize.Level0)
{
    std::variant<std::monostate, int64_t, bool, double, std::string, std::vector<uint8_t>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 6);
    std::variant<int64_t> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    ASSERT_EQ(size, 1);
}

/**
* @tc.name: variant_index_of_v
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, variant_index_of_v, TestSize.Level0)
{
    std::variant<std::monostate, int64_t, bool, double, std::string, std::vector<uint8_t>> value;
    auto index = Traits::variant_index_of_v<std::monostate, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<int64_t, decltype(value)>;
    ASSERT_EQ(index, 1);
    index = Traits::variant_index_of_v<bool, decltype(value)>;
    ASSERT_EQ(index, 2);
    index = Traits::variant_index_of_v<double, decltype(value)>;
    ASSERT_EQ(index, 3);
    index = Traits::variant_index_of_v<std::string, decltype(value)>;
    ASSERT_EQ(index, 4);
    index = Traits::variant_index_of_v<std::vector<uint8_t>, decltype(value)>;
    ASSERT_EQ(index, 5);
    index = Traits::variant_index_of_v<char *, decltype(value)>;
    ASSERT_EQ(index, 6);
}

/**
* @tc.name: get_if_same_type
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, get_if_same_type, TestSize.Level0)
{
    // 1. When the _Tp is a type in the ..._Types, the get_if is equal to the std::get_if.
    std::variant<std::monostate, int64_t, double, const char *> value;
    auto *nil = Traits::get_if<std::monostate>(&value);
    ASSERT_NE(nil, nullptr);
    auto *number = Traits::get_if<int64_t>(&value);
    ASSERT_EQ(number, nullptr);
    value = int64_t(1);
    number = Traits::get_if<int64_t>(&value);
    ASSERT_NE(number, nullptr);
    ASSERT_EQ(*number, 1);
    value = 1.5;
    auto *dVal = Traits::get_if<double>(&value);
    ASSERT_NE(dVal, nullptr);
    ASSERT_DOUBLE_EQ(*dVal, 1.5);
    value = "test case";
    auto *charPtr = Traits::get_if<const char *>(&value);
    ASSERT_NE(charPtr, nullptr);
    ASSERT_TRUE(strcmp(*charPtr, "test case") == 0);
}
/**
* @tc.name: get_if_convertible_type
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, get_if_convertible_type, TestSize.Level0)
{
    // 2. When the _Tp is not a type in the ..._Types but someone in the ...Types can convert to _Tp implicitly,
    //    the get_if will return it.
    std::variant<std::monostate, int64_t, double, const char *, From> value;
    value = int64_t(1);
    auto *fVal = Traits::get_if<double>(&value);
    ASSERT_EQ(fVal, nullptr);

    value = "test case";
    auto *strVal = Traits::get_if<std::string>(&value);
    ASSERT_NE(strVal, nullptr);
    ASSERT_TRUE(strcmp(*strVal, "test case") == 0);

    value = From();
    auto *toVal = Traits::get_if<Convertible>(&value);
    ASSERT_NE(toVal, nullptr);

    std::variant<std::monostate, int64_t, double, const char *, Convertible> val2;
    val2 = Convertible();
    auto *fromVal = Traits::get_if<From>(&val2);
    ASSERT_NE(fromVal, nullptr);
}

/**
* @tc.name: get_if_invalid_type
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(TraitsTest, get_if_invalid_type, TestSize.Level0)
{
    // 3. When the _Tp is not a type in the ..._Types and can't convert, the get_if will return nullptr.
    std::variant<std::monostate, int64_t, double, const char *> value;
    auto *unknown = Traits::get_if<std::vector<uint8_t>>(&value);
    ASSERT_EQ(unknown, nullptr);
    value = int64_t(9);
    unknown = Traits::get_if<std::vector<uint8_t>>(&value);
    ASSERT_EQ(unknown, nullptr);
    value = 1.5;
    unknown = Traits::get_if<std::vector<uint8_t>>(&value);
    ASSERT_EQ(unknown, nullptr);
    value = "test case";
    unknown = Traits::get_if<std::vector<uint8_t>>(&value);
    ASSERT_EQ(unknown, nullptr);
}
} // namespace OHOS::Test
