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
#include "traits.h"
#include <gtest/gtest.h>
using namespace testing::ext;
namespace OHOS::Test {
class TraitsUnitTest : public testing::Test {
public:
    class From {
    public:
        From() { }
    };
    class ConvertibleTest {
    public:
        // ConvertibleTest is auto convert type, do not add explicit to stop the type convert.
        ConvertibleTest() { }
        ConvertibleTest(ConvertibleTest &&) noexcept {};
        ConvertibleTest &operator=(ConvertibleTest &&) noexcept
        {
            return *this;
        }
        operator From()
        {
            return From();
        }
    };
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() { }
};

/**
 * @tc.name: same_index_of_v
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TraitsUnitTest, same_index_of_vTest, TestSize.Level0)
{
    auto flag = Traits::same_index_of_v<int32_t, int32_t, double, std::vector<uint8_t>>;
    EXPECT_EQ(flag, 0);
    flag = Traits::same_index_of_v<std::string, int32_t, double, std::vector<uint8_t>>;
    EXPECT_EQ(flag, 3);
    flag = Traits::same_index_of_v<std::string>;
    EXPECT_EQ(flag, 0);
}

/**
 * @tc.name: same_in_v
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TraitsUnitTest, same_in_vTest, TestSize.Level0)
{
    auto existTest = Traits::same_in_v<int32_t, int32_t, double, std::vector<uint8_t>>;
    EXPECT_TRUE(existTest);
    existTest = Traits::same_in_v<std::string, int32_t, double, std::vector<uint8_t>>;
    ASSERT_FALSE(existTest);
    existTest = Traits::same_in_v<std::string>;
    ASSERT_FALSE(existTest);
}

/**
 * @tc.name: convertible_index_of_v
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TraitsUnitTest, convertible_index_of_vTest, TestSize.Level0)
{
    auto flag = Traits::convertible_index_of_v<int32_t, int16_t, double, std::vector<uint8_t>>;
    EXPECT_EQ(flag, 0);
    flag = Traits::convertible_index_of_v<std::string, int16_t, const char *, std::vector<uint8_t>>;
    EXPECT_EQ(flag, 1);
    flag = Traits::convertible_index_of_v<std::string, int16_t, std::vector<uint8_t>>;
    EXPECT_EQ(flag, 2);
    flag = Traits::convertible_index_of_v<std::string>;
    EXPECT_EQ(flag, 0);
}

/**
 * @tc.name: convertible_in_v
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TraitsUnitTest, convertible_in_vTest, TestSize.Level0)
{
    auto convertibleTest = Traits::convertible_in_v<int32_t, int16_t, double, std::vector<uint8_t>>;
    EXPECT_TRUE(convertibleTest);
    convertibleTest = Traits::convertible_in_v<std::string, int16_t, const char *, std::vector<uint8_t>>;
    EXPECT_TRUE(convertibleTest);
    convertibleTest = Traits::convertible_in_v<std::string, int16_t, std::vector<uint8_t>>;
    ASSERT_FALSE(convertibleTest);
    convertibleTest = Traits::convertible_in_v<std::string>;
    ASSERT_FALSE(convertibleTest);
}

/**
 * @tc.name: variant_size_of_v
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TraitsUnitTest, variant_size_of_vTest, TestSize.Level0)
{
    std::variantTest<std::monostate, int64_t, bool, double, std::string, std::vector<uint8_t>> valueTest;
    auto size = Traits::variant_size_of_v<decltype(valueTest)>;
    EXPECT_EQ(size, 6);
    std::variantTest<int64_t> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    EXPECT_EQ(size, 1);
}

/**
 * @tc.name: variant_index_of_v
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TraitsUnitTest, variant_index_of_vTest, TestSize.Level0)
{
    std::variantTest<std::monostate, int64_t, bool, double, std::string, std::vector<uint8_t>> valueTest;
    auto flag = Traits::variant_index_of_v<std::monostate, decltype(valueTest)>;
    EXPECT_EQ(flag, 0);
    flag = Traits::variant_index_of_v<int64_t, decltype(valueTest)>;
    EXPECT_EQ(flag, 1);
    flag = Traits::variant_index_of_v<bool, decltype(valueTest)>;
    EXPECT_EQ(flag, 2);
    flag = Traits::variant_index_of_v<double, decltype(valueTest)>;
    EXPECT_EQ(flag, 3);
    flag = Traits::variant_index_of_v<std::string, decltype(valueTest)>;
    EXPECT_EQ(flag, 4);
    flag = Traits::variant_index_of_v<std::vector<uint8_t>, decltype(valueTest)>;
    EXPECT_EQ(flag, 5);
    flag = Traits::variant_index_of_v<char *, decltype(valueTest)>;
    EXPECT_EQ(flag, 6);
}

/**
 * @tc.name: get_if_same_type
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:

 */
HWTEST_F(TraitsUnitTest, get_if_same_typeTest, TestSize.Level0)
{
    // 1. When the _Tp is a type in the ..._Types, the get_if is equal to the std::get_if.
    std::variantTest<std::monostate, int64_t, double, const char *> valueTest;
    auto *nil = Traits::get_if<std::monostate>(&valueTest);
    ASSERT_NE(nil, nullptr);
    auto *number11 = Traits::get_if<int64_t>(&valueTest);
    EXPECT_EQ(number11, nullptr);
    valueTest = int64_t(1);
    number11 = Traits::get_if<int64_t>(&valueTest);
    ASSERT_NE(number11, nullptr);
    EXPECT_EQ(*number11, 1);
    valueTest = 1.5;
    auto *dVal = Traits::get_if<double>(&valueTest);
    ASSERT_NE(dVal, nullptr);
    ASSERT_DOUBLE_EQ(*dVal, 1.5);
    valueTest = "testTrait";
    auto *charPtr = Traits::get_if<const char *>(&valueTest);
    ASSERT_NE(charPtr, nullptr);
    EXPECT_TRUE(strcmp(*charPtr, "testTrait") == 0);
}
/**
 * @tc.name: get_if_convertible_type
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TraitsUnitTest, get_if_convertible_typeTest, TestSize.Level0)
{
    // 2. When the _Tp is not a type in the ..._Types but someone in the ...Types can convert to _Tp implicitly,
    //    the get_if will return it.
    std::variantTest<std::monostate, int64_t, double, const char *, From> valueTest;
    valueTest = int64_t(1);
    auto *fVal = Traits::get_if<double>(&valueTest);
    EXPECT_EQ(fVal, nullptr);

    valueTest = "testTrait";
    auto *strVal = Traits::get_if<std::string>(&valueTest);
    ASSERT_NE(strVal, nullptr);
    EXPECT_TRUE(strcmp(*strVal, "testTrait") == 0);

    valueTest = From();
    auto *toVal = Traits::get_if<ConvertibleTest>(&valueTest);
    ASSERT_NE(toVal, nullptr);

    std::variantTest<std::monostate, int64_t, double, const char *, ConvertibleTest> val2;
    val2 = ConvertibleTest();
    auto *fromVal = Traits::get_if<From>(&val2);
    ASSERT_NE(fromVal, nullptr);
}

/**
 * @tc.name: get_if_invalid_type
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(TraitsUnitTest, get_if_invalid_typeTest, TestSize.Level0)
{
    // 3. When the _Tp is not a type in the ..._Types and can't convert, the get_if will return nullptr.
    std::variantTest<std::monostate, int64_t, double, const char *> valueTest;
    auto *unknownTest = Traits::get_if<std::vector<uint8_t>>(&valueTest);
    EXPECT_EQ(unknownTest, nullptr);
    valueTest = int64_t(9);
    unknownTest = Traits::get_if<std::vector<uint8_t>>(&valueTest);
    EXPECT_EQ(unknownTest, nullptr);
    valueTest = 1.5;
    unknownTest = Traits::get_if<std::vector<uint8_t>>(&valueTest);
    EXPECT_EQ(unknownTest, nullptr);
    valueTest = "testTrait";
    unknownTest = Traits::get_if<std::vector<uint8_t>>(&valueTest);
    EXPECT_EQ(unknownTest, nullptr);
}
} // namespace OHOS::Test
