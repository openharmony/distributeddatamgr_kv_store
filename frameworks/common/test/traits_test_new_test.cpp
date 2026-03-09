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
#include "traits.h"

#include <deque>
#include <list>
#include <unordered_set>
#include <variant>
#include <complex>
#include <functional>
#include <stack>
#include <queue>
#include <valarray>
#include <forward_list>
#include <gtest/gtest.h>
using namespace testing::ext;
using namespace OHOS;
namespace OHOS::Test {
class TraitsTestNew : public testing::Test {
public:
    class From {
    public:
        From() { }
    };
    class Convertible {
    public:
        explicit Convertible(const From &) {};
        Convertible() { }
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
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() { }
};

HWTEST_F(TraitsTestNew, same_index_of_v001, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<int32_t, int32_t, double, std::vector<uint8_t>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::string, int32_t, double, std::vector<uint8_t>>;
    ASSERT_EQ(index, 3);
    index = Traits::same_index_of_v<std::string>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_index_of_v002, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<double, int32_t, double, std::vector<uint8_t>>;
    ASSERT_EQ(index, 1);
    index = Traits::same_index_of_v<std::vector<uint8_t>, int32_t, double, std::vector<uint8_t>>;
    ASSERT_EQ(index, 2);
    index = Traits::same_index_of_v<int64_t>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_index_of_v003, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<uint32_t, int32_t, double, uint32_t>;
    ASSERT_EQ(index, 2);
    index = Traits::same_index_of_v<int16_t, int16_t, double, float>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<float>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_index_of_v004, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<char, int, char, float>;
    ASSERT_EQ(index, 1);
    index = Traits::same_index_of_v<uint8_t, uint8_t, uint16_t, uint32_t>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<uint16_t>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_index_of_v005, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<int64_t, int32_t, int64_t, std::string>;
    ASSERT_EQ(index, 1);
    index = Traits::same_index_of_v<bool, bool, double, float>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::string>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_in_v001, TestSize.Level0)
{
    auto exist = Traits::same_in_v<int32_t, int32_t, double, std::vector<uint8_t>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::string, int32_t, double, std::vector<uint8_t>>;
    ASSERT_FALSE(exist);
    exist = Traits::same_in_v<std::string>;
    ASSERT_FALSE(exist);
}

HWTEST_F(TraitsTestNew, same_in_v002, TestSize.Level0)
{
    auto exist = Traits::same_in_v<double, int32_t, double, std::vector<uint8_t>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::vector<uint8_t>, int32_t, double, std::vector<uint8_t>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<int64_t>;
    ASSERT_FALSE(exist);
}

HWTEST_F(TraitsTestNew, same_in_v003, TestSize.Level0)
{
    auto exist = Traits::same_in_v<uint32_t, int32_t, double, uint32_t>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<int16_t, int16_t, double, float>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<float>;
    ASSERT_FALSE(exist);
}

HWTEST_F(TraitsTestNew, same_in_v004, TestSize.Level0)
{
    auto exist = Traits::same_in_v<char, int, char, float>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<uint8_t, uint8_t, uint16_t, uint32_t>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<uint16_t>;
    ASSERT_FALSE(exist);
}

HWTEST_F(TraitsTestNew, same_in_v005, TestSize.Level0)
{
    auto exist = Traits::same_in_v<int64_t, int32_t, int64_t, std::string>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<bool, bool, double, float>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::string>;
    ASSERT_FALSE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v001, TestSize.Level0)
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

HWTEST_F(TraitsTestNew, convertible_index_of_v002, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<int64_t, int32_t, int16_t, double>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<double, int, float, double>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<float, int, double>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<int64_t>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v003, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<uint32_t, uint16_t, uint8_t, int>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<int32_t, int16_t, int8_t, int32_t>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<int64_t, int32_t, int16_t>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<uint32_t>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v004, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::string, const char *, std::string_view, std::vector<char>>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<std::string, std::string_view, const char *>;
    ASSERT_EQ(index, 1);
    index = Traits::convertible_index_of_v<std::string, std::vector<char>>;
    ASSERT_EQ(index, 1);
    index = Traits::convertible_index_of_v<std::string>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v005, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<double, float, int, double>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<long double, double, float>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<float, int, char>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<double>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_in_v001, TestSize.Level0)
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

HWTEST_F(TraitsTestNew, convertible_in_v002, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<int64_t, int32_t, int16_t, double>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<double, int, float, double>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<float, int, double>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<int64_t>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, convertible_in_v003, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<uint32_t, uint16_t, uint8_t, int>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<int32_t, int16_t, int8_t, int32_t>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<int64_t, int32_t, int16_t>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<uint32_t>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, convertible_in_v004, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::string, const char *, std::string_view, std::vector<char>>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<std::string, std::string_view, const char *>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<std::string, std::vector<char>>;
    ASSERT_FALSE(convertible);
    convertible = Traits::convertible_in_v<std::string>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, convertible_in_v005, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<double, float, int, double>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<long double, double, float>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<float, int, char>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<double>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v001, TestSize.Level0)
{
    std::variant<std::monostate, int64_t, bool, double, std::string, std::vector<uint8_t>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 6);
    std::variant<int64_t> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    ASSERT_EQ(size, 1);
}

HWTEST_F(TraitsTestNew, variant_size_of_v002, TestSize.Level0)
{
    std::variant<int, double, std::string> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 3);
    std::variant<bool> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    ASSERT_EQ(size, 1);
}

HWTEST_F(TraitsTestNew, variant_size_of_v003, TestSize.Level0)
{
    std::variant<std::monostate, int, float, char, std::vector<int>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 5);
    std::variant<float, double> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_size_of_v004, TestSize.Level0)
{
    std::variant<uint8_t, uint16_t, uint32_t, uint64_t> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 4);
    std::variant<int8_t, int16_t, int32_t, int64_t> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    ASSERT_EQ(size, 4);
}

HWTEST_F(TraitsTestNew, variant_size_of_v005, TestSize.Level0)
{
    std::variant<std::string, std::string_view, const char *> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 3);
    std::variant<std::vector<uint8_t>, std::vector<int>> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v001, TestSize.Level0)
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

HWTEST_F(TraitsTestNew, variant_index_of_v002, TestSize.Level0)
{
    std::variant<int, double, std::string> value;
    auto index = Traits::variant_index_of_v<int, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<double, decltype(value)>;
    ASSERT_EQ(index, 1);
    index = Traits::variant_index_of_v<std::string, decltype(value)>;
    ASSERT_EQ(index, 2);
    index = Traits::variant_index_of_v<float, decltype(value)>;
    ASSERT_EQ(index, 3);
}

HWTEST_F(TraitsTestNew, variant_index_of_v003, TestSize.Level0)
{
    std::variant<std::monostate, int, float, char, std::vector<int>> value;
    auto index = Traits::variant_index_of_v<std::monostate, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<int, decltype(value)>;
    ASSERT_EQ(index, 1);
    index = Traits::variant_index_of_v<float, decltype(value)>;
    ASSERT_EQ(index, 2);
    index = Traits::variant_index_of_v<char, decltype(value)>;
    ASSERT_EQ(index, 3);
    index = Traits::variant_index_of_v<std::vector<int>, decltype(value)>;
    ASSERT_EQ(index, 4);
}

HWTEST_F(TraitsTestNew, variant_index_of_v004, TestSize.Level0)
{
    std::variant<uint8_t, uint16_t, uint32_t, uint64_t> value;
    auto index = Traits::variant_index_of_v<uint8_t, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<uint16_t, decltype(value)>;
    ASSERT_EQ(index, 1);
    index = Traits::variant_index_of_v<uint32_t, decltype(value)>;
    ASSERT_EQ(index, 2);
    index = Traits::variant_index_of_v<uint64_t, decltype(value)>;
    ASSERT_EQ(index, 3);
    index = Traits::variant_index_of_v<int32_t, decltype(value)>;
    ASSERT_EQ(index, 4);
}

HWTEST_F(TraitsTestNew, variant_index_of_v005, TestSize.Level0)
{
    std::variant<std::string, std::string_view, const char *> value;
    auto index = Traits::variant_index_of_v<std::string, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::string_view, decltype(value)>;
    ASSERT_EQ(index, 1);
    index = Traits::variant_index_of_v<const char *, decltype(value)>;
    ASSERT_EQ(index, 2);
    index = Traits::variant_index_of_v<char *, decltype(value)>;
    ASSERT_EQ(index, 3);
}

HWTEST_F(TraitsTestNew, get_if_same_type001, TestSize.Level0)
{
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

HWTEST_F(TraitsTestNew, get_if_same_type002, TestSize.Level0)
{
    std::variant<int, double, std::string> value;
    auto *intPtr = Traits::get_if<int>(&value);
    ASSERT_NE(intPtr, nullptr);
    ASSERT_EQ(*intPtr, 0);
    auto *dblPtr = Traits::get_if<double>(&value);
    ASSERT_EQ(dblPtr, nullptr);
    value = 42;
    intPtr = Traits::get_if<int>(&value);
    ASSERT_NE(intPtr, nullptr);
    ASSERT_EQ(*intPtr, 42);
    value = 3.14;
    dblPtr = Traits::get_if<double>(&value);
    ASSERT_NE(dblPtr, nullptr);
    ASSERT_DOUBLE_EQ(*dblPtr, 3.14);
    value = std::string("hello");
    auto *strPtr = Traits::get_if<std::string>(&value);
    ASSERT_NE(strPtr, nullptr);
    ASSERT_EQ(*strPtr, "hello");
}

HWTEST_F(TraitsTestNew, get_if_same_type003, TestSize.Level0)
{
    std::variant<std::monostate, int, float, char, std::vector<int>> value;
    auto *nil = Traits::get_if<std::monostate>(&value);
    ASSERT_NE(nil, nullptr);
    auto *intPtr = Traits::get_if<int>(&value);
    ASSERT_EQ(intPtr, nullptr);
    value = 100;
    intPtr = Traits::get_if<int>(&value);
    ASSERT_NE(intPtr, nullptr);
    ASSERT_EQ(*intPtr, 100);
    value = 3.5f;
    auto *floatPtr = Traits::get_if<float>(&value);
    ASSERT_NE(floatPtr, nullptr);
    ASSERT_FLOAT_EQ(*floatPtr, 3.5f);
    value = 'a';
    auto *charPtr = Traits::get_if<char>(&value);
    ASSERT_NE(charPtr, nullptr);
    ASSERT_EQ(*charPtr, 'a');
}

HWTEST_F(TraitsTestNew, get_if_same_type004, TestSize.Level0)
{
    std::variant<uint8_t, uint16_t, uint32_t, uint64_t> value;
    auto *u8 = Traits::get_if<uint8_t>(&value);
    ASSERT_NE(u8, nullptr);
    ASSERT_EQ(*u8, 0);
    auto *u16 = Traits::get_if<uint16_t>(&value);
    ASSERT_EQ(u16, nullptr);
    value = uint16_t(65535);
    u16 = Traits::get_if<uint16_t>(&value);
    ASSERT_NE(u16, nullptr);
    ASSERT_EQ(*u16, 65535);
    value = uint32_t(4294967295);
    auto *u32 = Traits::get_if<uint32_t>(&value);
    ASSERT_NE(u32, nullptr);
    ASSERT_EQ(*u32, 4294967295);
    value = uint64_t(18446744073709551615ULL);
    auto *u64 = Traits::get_if<uint64_t>(&value);
    ASSERT_NE(u64, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_same_type005, TestSize.Level0)
{
    std::variant<std::string, std::string_view, const char *> value;
    auto *str = Traits::get_if<std::string>(&value);
    ASSERT_NE(str, nullptr);
    auto *view = Traits::get_if<std::string_view>(&value);
    ASSERT_EQ(view, nullptr);
    value = std::string("test");
    str = Traits::get_if<std::string>(&value);
    ASSERT_NE(str, nullptr);
    ASSERT_EQ(*str, "test");
    value = std::string_view("view");
    view = Traits::get_if<std::string_view>(&value);
    ASSERT_NE(view, nullptr);
    ASSERT_EQ(*view, "view");
    value = "cstr";
    auto *cstr = Traits::get_if<const char *>(&value);
    ASSERT_NE(cstr, nullptr);
    ASSERT_TRUE(strcmp(*cstr, "cstr") == 0);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type001, TestSize.Level0)
{
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

HWTEST_F(TraitsTestNew, get_if_convertible_type002, TestSize.Level0)
{
    std::variant<int, float, const char *> value;
    value = 42;
    auto *dbl = Traits::get_if<double>(&value);
    ASSERT_EQ(dbl, nullptr);
    value = 3.14f;
    auto *intVal = Traits::get_if<int>(&value);
    ASSERT_EQ(intVal, nullptr);
    value = "hello";
    auto *strVal = Traits::get_if<std::string>(&value);
    ASSERT_NE(strVal, nullptr);
    ASSERT_EQ(*strVal, "hello");
}

HWTEST_F(TraitsTestNew, get_if_convertible_type003, TestSize.Level0)
{
    std::variant<int32_t, int16_t, int8_t> value;
    value = int8_t(127);
    auto *int16 = Traits::get_if<int16_t>(&value);
    ASSERT_EQ(int16, nullptr);
    value = int16_t(32767);
    auto *int32 = Traits::get_if<int32_t>(&value);
    ASSERT_EQ(int32, nullptr);
    value = int32_t(2147483647);
    int32 = Traits::get_if<int32_t>(&value);
    ASSERT_NE(int32, nullptr);
    ASSERT_EQ(*int32, 2147483647);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type004, TestSize.Level0)
{
    std::variant<std::string_view, const char *> value;
    value = std::string_view("view test");
    auto *str = Traits::get_if<std::string>(&value);
    ASSERT_EQ(str, nullptr);
    value = "c string";
    str = Traits::get_if<std::string>(&value);
    ASSERT_NE(str, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type005, TestSize.Level0)
{
    std::variant<float, int, char> value;
    value = 42;
    auto *dbl = Traits::get_if<double>(&value);
    ASSERT_EQ(dbl, nullptr);
    auto *flt = Traits::get_if<float>(&value);
    ASSERT_EQ(flt, nullptr);
    value = 'a';
    flt = Traits::get_if<float>(&value);
    ASSERT_EQ(flt, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type001, TestSize.Level0)
{
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

HWTEST_F(TraitsTestNew, get_if_invalid_type002, TestSize.Level0)
{
    std::variant<int, double> value;
    auto *str = Traits::get_if<std::string>(&value);
    ASSERT_EQ(str, nullptr);
    value = 42;
    str = Traits::get_if<std::string>(&value);
    ASSERT_EQ(str, nullptr);
    value = 3.14;
    str = Traits::get_if<std::string>(&value);
    ASSERT_EQ(str, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type003, TestSize.Level0)
{
    std::variant<uint8_t, uint16_t> value;
    auto *u32 = Traits::get_if<uint32_t>(&value);
    ASSERT_EQ(u32, nullptr);
    value = uint8_t(255);
    u32 = Traits::get_if<uint32_t>(&value);
    ASSERT_EQ(u32, nullptr);
    value = uint16_t(65535);
    u32 = Traits::get_if<uint32_t>(&value);
    ASSERT_EQ(u32, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type004, TestSize.Level0)
{
    std::variant<std::string_view, const char *> value;
    auto *vec = Traits::get_if<std::vector<int>>(&value);
    ASSERT_EQ(vec, nullptr);
    value = std::string_view("test");
    vec = Traits::get_if<std::vector<int>>(&value);
    ASSERT_EQ(vec, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type005, TestSize.Level0)
{
    std::variant<float, bool> value;
    auto *dbl = Traits::get_if<double>(&value);
    ASSERT_EQ(dbl, nullptr);
    auto *intVal = Traits::get_if<int>(&value);
    ASSERT_EQ(intVal, nullptr);
    value = 3.14f;
    dbl = Traits::get_if<double>(&value);
    ASSERT_EQ(dbl, nullptr);
    intVal = Traits::get_if<int>(&value);
    ASSERT_EQ(intVal, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v006, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<void*, void*, int, double>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::nullptr_t, int, double, std::nullptr_t>;
    ASSERT_EQ(index, 2);
    index = Traits::same_index_of_v<std::array<int, 5>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_index_of_v007, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::vector<int>, std::vector<int>, std::string>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::deque<int>, std::vector<int>, std::deque<int>>;
    ASSERT_EQ(index, 1);
    index = Traits::same_index_of_v<std::list<int>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_in_v006, TestSize.Level0)
{
    auto exist = Traits::same_in_v<void*, void*, int, double>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::nullptr_t, int, double, std::nullptr_t>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::array<int, 5>>;
    ASSERT_FALSE(exist);
}

HWTEST_F(TraitsTestNew, same_in_v007, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::vector<int>, std::vector<int>, std::string>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::deque<int>, std::vector<int>, std::deque<int>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::list<int>>;
    ASSERT_FALSE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v006, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<void*, std::nullptr_t, int*>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<const void*, void*, int*>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<const char*, std::string>;
    ASSERT_EQ(index, 1);
    index = Traits::convertible_index_of_v<void*>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v007, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::shared_ptr<int>, std::shared_ptr<const int>, std::unique_ptr<int>>;
    ASSERT_EQ(index, 1);
    index = Traits::convertible_index_of_v<std::shared_ptr<const int>, std::shared_ptr<int>>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<std::unique_ptr<int>, std::shared_ptr<int>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, convertible_in_v006, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<void*, std::nullptr_t, int*>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<const void*, void*, int*>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<const char*, std::string>;
    ASSERT_FALSE(convertible);
    convertible = Traits::convertible_in_v<void*>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, convertible_in_v007, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::shared_ptr<int>, std::shared_ptr<const int>, std::unique_ptr<int>>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<std::shared_ptr<const int>, std::shared_ptr<int>>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<std::unique_ptr<int>, std::shared_ptr<int>>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v006, TestSize.Level0)
{
    std::variant<std::monostate, void*, const void*, int*> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 4);
    std::variant<void*> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    ASSERT_EQ(size, 1);
}

HWTEST_F(TraitsTestNew, variant_size_of_v007, TestSize.Level0)
{
    std::variant<std::shared_ptr<int>, std::unique_ptr<int>, std::weak_ptr<int>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 3);
    std::variant<std::vector<int>, std::list<int>> value2;
    size = Traits::variant_size_of_v<decltype(value2)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v006, TestSize.Level0)
{
    std::variant<std::monostate, void*, const void*, int*> value;
    auto index = Traits::variant_index_of_v<std::monostate, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<void*, decltype(value)>;
    ASSERT_EQ(index, 1);
    index = Traits::variant_index_of_v<const void*, decltype(value)>;
    ASSERT_EQ(index, 2);
    index = Traits::variant_index_of_v<int*, decltype(value)>;
    ASSERT_EQ(index, 3);
    index = Traits::variant_index_of_v<char*, decltype(value)>;
    ASSERT_EQ(index, 4);
}

HWTEST_F(TraitsTestNew, variant_index_of_v007, TestSize.Level0)
{
    std::variant<std::shared_ptr<int>, std::unique_ptr<int>, std::weak_ptr<int>> value;
    auto index = Traits::variant_index_of_v<std::shared_ptr<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::unique_ptr<int>, decltype(value)>;
    ASSERT_EQ(index, 1);
    index = Traits::variant_index_of_v<std::weak_ptr<int>, decltype(value)>;
    ASSERT_EQ(index, 2);
    index = Traits::variant_index_of_v<std::shared_ptr<double>, decltype(value)>;
    ASSERT_EQ(index, 3);
}

HWTEST_F(TraitsTestNew, get_if_same_type006, TestSize.Level0)
{
    std::variant<std::monostate, void*, const void*, int*> value;
    auto *nil = Traits::get_if<std::monostate>(&value);
    ASSERT_NE(nil, nullptr);
    auto *vp = Traits::get_if<void*>(&value);
    ASSERT_EQ(vp, nullptr);
    int x = 42;
    value = &x;
    auto *ip = Traits::get_if<int*>(&value);
    ASSERT_NE(ip, nullptr);
    ASSERT_EQ(*ip, &x);
}

HWTEST_F(TraitsTestNew, get_if_same_type007, TestSize.Level0)
{
    std::variant<std::shared_ptr<int>, std::unique_ptr<int>, std::weak_ptr<int>> value;
    auto *sp = Traits::get_if<std::shared_ptr<int>>(&value);
    ASSERT_NE(sp, nullptr);
    ASSERT_EQ(sp->get(), nullptr);
    value = std::make_shared<int>(100);
    sp = Traits::get_if<std::shared_ptr<int>>(&value);
    ASSERT_NE(sp, nullptr);
    ASSERT_EQ(**sp, 100);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type006, TestSize.Level0)
{
    std::variant<int*, const int*> value;
    int x = 42;
    value = &x;
    auto *cip = Traits::get_if<const int*>(&value);
    ASSERT_EQ(cip, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type007, TestSize.Level0)
{
    std::variant<std::shared_ptr<int>, std::shared_ptr<const int>> value;
    value = std::make_shared<int>(200);
    auto *csp = Traits::get_if<std::shared_ptr<const int>>(&value);
    ASSERT_EQ(csp, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type006, TestSize.Level0)
{
    std::variant<std::monostate, void*, const void*, int*> value;
    auto *str = Traits::get_if<std::string>(&value);
    ASSERT_EQ(str, nullptr);
    int x = 42;
    value = &x;
    str = Traits::get_if<std::string>(&value);
    ASSERT_EQ(str, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type007, TestSize.Level0)
{
    std::variant<std::shared_ptr<int>, std::unique_ptr<int>> value;
    auto *weak = Traits::get_if<std::weak_ptr<int>>(&value);
    ASSERT_NE(weak, nullptr);
    value = std::make_shared<int>(300);
    weak = Traits::get_if<std::weak_ptr<int>>(&value);
    ASSERT_NE(weak, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v008, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::pair<int, double>, std::pair<int, double>, std::string>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::tuple<int, double, char>, int, std::tuple<int, double, char>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v008, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::pair<int, double>, std::pair<int, double>, std::string>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::tuple<int, double, char>, int, std::tuple<int, double, char>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v008, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::pair<int, int>, std::pair<short, short>>;
    ASSERT_EQ(index, 0);
    index = Traits::convertible_index_of_v<std::pair<double, double>, std::pair<int, int>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_in_v008, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::pair<int, int>, std::pair<short, short>>;
    ASSERT_TRUE(convertible);
    convertible = Traits::convertible_in_v<std::pair<double, double>, std::pair<int, int>>;
    ASSERT_TRUE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v008, TestSize.Level0)
{
    std::variant<std::pair<int, double>, std::tuple<int, double, char>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v008, TestSize.Level0)
{
    std::variant<std::pair<int, double>, std::tuple<int, double, char>> value;
    auto index = Traits::variant_index_of_v<std::pair<int, double>, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::tuple<int, double, char>, decltype(value)>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, get_if_same_type008, TestSize.Level0)
{
    std::variant<std::pair<int, double>, std::tuple<int, double, char>> value;
    auto *pair = Traits::get_if<std::pair<int, double>>(&value);
    ASSERT_NE(pair, nullptr);
    value = std::make_tuple(1, 2.5, 'a');
    auto *tuple = Traits::get_if<std::tuple<int, double, char>>(&value);
    ASSERT_NE(tuple, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type008, TestSize.Level0)
{
    std::variant<std::pair<int, int>, std::pair<short, short>> value;
    value = std::make_pair(10, 20);
    auto *pair = Traits::get_if<std::pair<short, short>>(&value);
    ASSERT_EQ(pair, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type008, TestSize.Level0)
{
    std::variant<std::pair<int, double>, std::tuple<int, double, char>> value;
    auto *vec = Traits::get_if<std::vector<int>>(&value);
    ASSERT_EQ(vec, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v009, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::optional<int>, std::optional<int>, int>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::nullopt_t, int, std::nullopt_t>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v009, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::optional<int>, std::optional<int>, int>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::nullopt_t, int, std::nullopt_t>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v009, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<int, std::optional<int>>;
    ASSERT_EQ(index, 1);
    index = Traits::convertible_index_of_v<std::nullopt_t, int, std::nullopt_t>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, convertible_in_v009, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<int, std::optional<int>>;
    ASSERT_FALSE(convertible);
    convertible = Traits::convertible_in_v<std::nullopt_t, int, std::nullopt_t>;
    ASSERT_TRUE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v009, TestSize.Level0)
{
    std::variant<std::optional<int>, std::optional<double>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v009, TestSize.Level0)
{
    std::variant<std::optional<int>, std::optional<double>> value;
    auto index = Traits::variant_index_of_v<std::optional<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::optional<double>, decltype(value)>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, get_if_same_type009, TestSize.Level0)
{
    std::variant<std::optional<int>, std::optional<double>> value;
    auto *opt = Traits::get_if<std::optional<int>>(&value);
    ASSERT_NE(opt, nullptr);
    ASSERT_EQ(*opt, std::nullopt);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type009, TestSize.Level0)
{
    std::variant<std::optional<int>, int> value;
    value = 42;
    auto *opt = Traits::get_if<std::optional<int>>(&value);
    ASSERT_EQ(opt, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type009, TestSize.Level0)
{
    std::variant<std::optional<int>, std::optional<double>> value;
    auto *str = Traits::get_if<std::string>(&value);
    ASSERT_EQ(str, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v010, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::function<void()>, std::function<void()>, int>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::function<int(int)>, int, std::function<int(int)>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v010, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::function<void()>, std::function<void()>, int>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::function<int(int)>, int, std::function<int(int)>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v010, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::function<void()>, std::function<int()>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_in_v010, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::function<void()>, std::function<int()>>;
    ASSERT_TRUE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v010, TestSize.Level0)
{
    std::variant<std::function<void()>, std::function<int(int)>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v010, TestSize.Level0)
{
    std::variant<std::function<void()>, std::function<int(int)>> value;
    auto index = Traits::variant_index_of_v<std::function<void()>, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::function<int(int)>, decltype(value)>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, get_if_same_type010, TestSize.Level0)
{
    std::variant<std::function<void()>, std::function<int(int)>> value;
    auto *func = Traits::get_if<std::function<void()>>(&value);
    ASSERT_NE(func, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type010, TestSize.Level0)
{
    std::variant<std::function<void()>, std::function<int(int)>> value;
    auto *str = Traits::get_if<std::string>(&value);
    ASSERT_EQ(str, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v011, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::chrono::milliseconds, std::chrono::milliseconds, std::chrono::seconds>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::chrono::seconds, std::chrono::milliseconds, std::chrono::seconds>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v011, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::chrono::milliseconds, std::chrono::milliseconds, std::chrono::seconds>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::chrono::seconds, std::chrono::milliseconds, std::chrono::seconds>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v011, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::chrono::duration<int>, std::chrono::milliseconds>;
    ASSERT_EQ(index, 1);
    index = Traits::convertible_index_of_v<std::chrono::duration<int, std::milli>, std::chrono::milliseconds>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_in_v011, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::chrono::duration<int>, std::chrono::milliseconds>;
    ASSERT_FALSE(convertible);
    convertible = Traits::convertible_in_v<std::chrono::duration<int, std::milli>, std::chrono::milliseconds>;
    ASSERT_TRUE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v011, TestSize.Level0)
{
    std::variant<std::chrono::milliseconds, std::chrono::seconds> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v011, TestSize.Level0)
{
    std::variant<std::chrono::milliseconds, std::chrono::seconds> value;
    auto index = Traits::variant_index_of_v<std::chrono::milliseconds, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::chrono::seconds, decltype(value)>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, get_if_same_type011, TestSize.Level0)
{
    std::variant<std::chrono::milliseconds, std::chrono::seconds> value;
    auto *ms = Traits::get_if<std::chrono::milliseconds>(&value);
    ASSERT_NE(ms, nullptr);
    value = std::chrono::seconds(10);
    auto *sec = Traits::get_if<std::chrono::seconds>(&value);
    ASSERT_NE(sec, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type011, TestSize.Level0)
{
    std::variant<std::chrono::milliseconds, std::chrono::seconds> value;
    value = std::chrono::seconds(5);
    auto *ms = Traits::get_if<std::chrono::milliseconds>(&value);
    ASSERT_EQ(ms, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type011, TestSize.Level0)
{
    std::variant<std::chrono::milliseconds, std::chrono::seconds> value;
    auto *micro = Traits::get_if<std::chrono::microseconds>(&value);
    ASSERT_NE(micro, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v012, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::set<int>, std::set<int>, std::vector<int>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::map<int, double>, int, std::map<int, double>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v012, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::set<int>, std::set<int>, std::vector<int>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::map<int, double>, int, std::map<int, double>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v012, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::set<int>, std::unordered_set<int>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, convertible_in_v012, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::set<int>, std::unordered_set<int>>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v012, TestSize.Level0)
{
    std::variant<std::set<int>, std::map<int, double>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v012, TestSize.Level0)
{
    std::variant<std::set<int>, std::map<int, double>> value;
    auto index = Traits::variant_index_of_v<std::set<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::map<int, double>, decltype(value)>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, get_if_same_type012, TestSize.Level0)
{
    std::variant<std::set<int>, std::map<int, double>> value;
    auto *set = Traits::get_if<std::set<int>>(&value);
    ASSERT_NE(set, nullptr);
    value = std::map<int, double>{{1, 1.5}};
    auto *map = Traits::get_if<std::map<int, double>>(&value);
    ASSERT_NE(map, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type012, TestSize.Level0)
{
    std::variant<std::set<int>, std::map<int, double>> value;
    auto *vec = Traits::get_if<std::vector<int>>(&value);
    ASSERT_EQ(vec, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v013, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::reference_wrapper<int>, std::reference_wrapper<int>, int>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_in_v013, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::reference_wrapper<int>, std::reference_wrapper<int>, int>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v013, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<int, std::reference_wrapper<int>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, convertible_in_v013, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<int, std::reference_wrapper<int>>;
    ASSERT_TRUE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v013, TestSize.Level0)
{
    int x = 42;
    std::variant<std::reference_wrapper<int>, std::reference_wrapper<double>> value{std::ref(x)};
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v013, TestSize.Level0)
{
    int x = 42;
    std::variant<std::reference_wrapper<int>, std::reference_wrapper<double>> value{std::ref(x)};
    auto index = Traits::variant_index_of_v<std::reference_wrapper<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type013, TestSize.Level0)
{
    int x = 42;
    std::variant<std::reference_wrapper<int>, std::reference_wrapper<double>> value{std::ref(x)};
    auto *ref = Traits::get_if<std::reference_wrapper<int>>(&value);
    ASSERT_NE(ref, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type013, TestSize.Level0)
{
    int x = 42;
    std::variant<std::reference_wrapper<int>, std::reference_wrapper<double>> value{std::ref(x)};
    auto *intVal = Traits::get_if<int>(&value);
    ASSERT_EQ(intVal, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v014, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::complex<double>, std::complex<double>, std::complex<float>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::complex<float>, std::complex<double>, std::complex<float>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v014, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::complex<double>, std::complex<double>, std::complex<float>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::complex<float>, std::complex<double>, std::complex<float>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v014, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<double, std::complex<double>>;
    ASSERT_EQ(index, 1);
    index = Traits::convertible_index_of_v<float, std::complex<double>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, convertible_in_v014, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<double, std::complex<double>>;
    ASSERT_FALSE(convertible);
    convertible = Traits::convertible_in_v<float, std::complex<double>>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v014, TestSize.Level0)
{
    std::variant<std::complex<double>, std::complex<float>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v014, TestSize.Level0)
{
    std::variant<std::complex<double>, std::complex<float>> value;
    auto index = Traits::variant_index_of_v<std::complex<double>, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::complex<float>, decltype(value)>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, get_if_same_type014, TestSize.Level0)
{
    std::variant<std::complex<double>, std::complex<float>> value;
    auto *cd = Traits::get_if<std::complex<double>>(&value);
    ASSERT_NE(cd, nullptr);
    value = std::complex<float>(1.0f, 2.0f);
    auto *cf = Traits::get_if<std::complex<float>>(&value);
    ASSERT_NE(cf, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_convertible_type014, TestSize.Level0)
{
    std::variant<std::complex<double>, double> value;
    value = 3.14;
    auto *cd = Traits::get_if<std::complex<double>>(&value);
    ASSERT_EQ(cd, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type014, TestSize.Level0)
{
    std::variant<std::complex<double>, std::complex<float>> value;
    auto *intVal = Traits::get_if<int>(&value);
    ASSERT_EQ(intVal, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v015, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::bitset<8>, std::bitset<8>, std::bitset<16>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::bitset<16>, std::bitset<8>, std::bitset<16>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v015, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::bitset<8>, std::bitset<8>, std::bitset<16>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::bitset<16>, std::bitset<8>, std::bitset<16>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v015, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<uint32_t, std::bitset<32>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, convertible_in_v015, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<uint32_t, std::bitset<32>>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v015, TestSize.Level0)
{
    std::variant<std::bitset<8>, std::bitset<16>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v015, TestSize.Level0)
{
    std::variant<std::bitset<8>, std::bitset<16>> value;
    auto index = Traits::variant_index_of_v<std::bitset<8>, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::bitset<16>, decltype(value)>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, get_if_same_type015, TestSize.Level0)
{
    std::variant<std::bitset<8>, std::bitset<16>> value;
    auto *bs8 = Traits::get_if<std::bitset<8>>(&value);
    ASSERT_NE(bs8, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type015, TestSize.Level0)
{
    std::variant<std::bitset<8>, std::bitset<16>> value;
    auto *bs32 = Traits::get_if<std::bitset<32>>(&value);
    ASSERT_EQ(bs32, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v016, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::stack<int>, std::stack<int>, std::queue<int>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::queue<int>, std::stack<int>, std::queue<int>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v016, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::stack<int>, std::stack<int>, std::queue<int>>;
    ASSERT_TRUE(exist);
    exist = Traits::same_in_v<std::queue<int>, std::stack<int>, std::queue<int>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v016, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::deque<int>, std::stack<int>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, convertible_in_v016, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::deque<int>, std::stack<int>>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v016, TestSize.Level0)
{
    std::variant<std::stack<int>, std::queue<int>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v016, TestSize.Level0)
{
    std::variant<std::stack<int>, std::queue<int>> value;
    auto index = Traits::variant_index_of_v<std::stack<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
    index = Traits::variant_index_of_v<std::queue<int>, decltype(value)>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, get_if_same_type016, TestSize.Level0)
{
    std::variant<std::stack<int>, std::queue<int>> value;
    auto *stk = Traits::get_if<std::stack<int>>(&value);
    ASSERT_NE(stk, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type016, TestSize.Level0)
{
    std::variant<std::stack<int>, std::queue<int>> value;
    auto *dq = Traits::get_if<std::deque<int>>(&value);
    ASSERT_EQ(dq, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v017, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::priority_queue<int>, std::priority_queue<int>, std::vector<int>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_in_v017, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::priority_queue<int>, std::priority_queue<int>, std::vector<int>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, variant_size_of_v017, TestSize.Level0)
{
    std::variant<std::priority_queue<int>, std::vector<int>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v017, TestSize.Level0)
{
    std::variant<std::priority_queue<int>, std::vector<int>> value;
    auto index = Traits::variant_index_of_v<std::priority_queue<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type017, TestSize.Level0)
{
    std::variant<std::priority_queue<int>, std::vector<int>> value;
    auto *pq = Traits::get_if<std::priority_queue<int>>(&value);
    ASSERT_NE(pq, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type017, TestSize.Level0)
{
    std::variant<std::priority_queue<int>, std::vector<int>> value;
    auto *stk = Traits::get_if<std::stack<int>>(&value);
    ASSERT_EQ(stk, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v018, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::valarray<int>, std::valarray<int>, std::vector<int>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_in_v018, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::valarray<int>, std::valarray<int>, std::vector<int>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, variant_size_of_v018, TestSize.Level0)
{
    std::variant<std::valarray<int>, std::valarray<double>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v018, TestSize.Level0)
{
    std::variant<std::valarray<int>, std::valarray<double>> value;
    auto index = Traits::variant_index_of_v<std::valarray<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type018, TestSize.Level0)
{
    std::variant<std::valarray<int>, std::valarray<double>> value;
    auto *va = Traits::get_if<std::valarray<int>>(&value);
    ASSERT_NE(va, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type018, TestSize.Level0)
{
    std::variant<std::valarray<int>, std::valarray<double>> value;
    auto *vec = Traits::get_if<std::vector<int>>(&value);
    ASSERT_EQ(vec, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v019, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::array<int, 5>, std::array<int, 5>, std::array<int, 10>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::array<int, 10>, std::array<int, 5>, std::array<int, 10>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v019, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::array<int, 5>, std::array<int, 5>, std::array<int, 10>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v019, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::vector<int>, std::array<int, 5>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, convertible_in_v019, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::vector<int>, std::array<int, 5>>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v019, TestSize.Level0)
{
    std::variant<std::array<int, 5>, std::array<double, 10>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v019, TestSize.Level0)
{
    std::variant<std::array<int, 5>, std::array<double, 10>> value;
    auto index = Traits::variant_index_of_v<std::array<int, 5>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type019, TestSize.Level0)
{
    std::variant<std::array<int, 5>, std::array<double, 10>> value;
    auto *arr = Traits::get_if<std::array<int, 5>>(&value);
    ASSERT_NE(arr, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type019, TestSize.Level0)
{
    std::variant<std::array<int, 5>, std::array<double, 10>> value;
    auto *vec = Traits::get_if<std::vector<int>>(&value);
    ASSERT_EQ(vec, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v020, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::forward_list<int>, std::forward_list<int>, std::list<int>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::list<int>, std::forward_list<int>, std::list<int>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v020, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::forward_list<int>, std::forward_list<int>, std::list<int>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, variant_size_of_v020, TestSize.Level0)
{
    std::variant<std::forward_list<int>, std::list<int>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v020, TestSize.Level0)
{
    std::variant<std::forward_list<int>, std::list<int>> value;
    auto index = Traits::variant_index_of_v<std::forward_list<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type020, TestSize.Level0)
{
    std::variant<std::forward_list<int>, std::list<int>> value;
    auto *fl = Traits::get_if<std::forward_list<int>>(&value);
    ASSERT_NE(fl, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type020, TestSize.Level0)
{
    std::variant<std::forward_list<int>, std::list<int>> value;
    auto *vec = Traits::get_if<std::vector<int>>(&value);
    ASSERT_EQ(vec, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v021, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::unordered_set<int>, std::unordered_set<int>, std::set<int>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_in_v021, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::unordered_set<int>, std::unordered_set<int>, std::set<int>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, variant_size_of_v021, TestSize.Level0)
{
    std::variant<std::unordered_set<int>, std::unordered_map<int, double>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v021, TestSize.Level0)
{
    std::variant<std::unordered_set<int>, std::unordered_map<int, double>> value;
    auto index = Traits::variant_index_of_v<std::unordered_set<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type021, TestSize.Level0)
{
    std::variant<std::unordered_set<int>, std::unordered_map<int, double>> value;
    auto *us = Traits::get_if<std::unordered_set<int>>(&value);
    ASSERT_NE(us, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type021, TestSize.Level0)
{
    std::variant<std::unordered_set<int>, std::unordered_map<int, double>> value;
    auto *set = Traits::get_if<std::set<int>>(&value);
    ASSERT_EQ(set, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v022, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::multiset<int>, std::multiset<int>, std::set<int>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_in_v022, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::multiset<int>, std::multiset<int>, std::set<int>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, variant_size_of_v022, TestSize.Level0)
{
    std::variant<std::multiset<int>, std::multimap<int, double>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v022, TestSize.Level0)
{
    std::variant<std::multiset<int>, std::multimap<int, double>> value;
    auto index = Traits::variant_index_of_v<std::multiset<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type022, TestSize.Level0)
{
    std::variant<std::multiset<int>, std::multimap<int, double>> value;
    auto *ms = Traits::get_if<std::multiset<int>>(&value);
    ASSERT_NE(ms, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type022, TestSize.Level0)
{
    std::variant<std::multiset<int>, std::multimap<int, double>> value;
    auto *set = Traits::get_if<std::set<int>>(&value);
    ASSERT_EQ(set, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v023, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::unordered_multiset<int>, std::unordered_multiset<int>,
                                         std::unordered_set<int>>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, same_in_v023, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::unordered_multiset<int>, std::unordered_multiset<int>, std::unordered_set<int>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, variant_size_of_v023, TestSize.Level0)
{
    std::variant<std::unordered_multiset<int>, std::unordered_multimap<int, double>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v023, TestSize.Level0)
{
    std::variant<std::unordered_multiset<int>, std::unordered_multimap<int, double>> value;
    auto index = Traits::variant_index_of_v<std::unordered_multiset<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type023, TestSize.Level0)
{
    std::variant<std::unordered_multiset<int>, std::unordered_multimap<int, double>> value;
    auto *ums = Traits::get_if<std::unordered_multiset<int>>(&value);
    ASSERT_NE(ums, nullptr);
}

HWTEST_F(TraitsTestNew, get_if_invalid_type023, TestSize.Level0)
{
    std::variant<std::unordered_multiset<int>, std::unordered_multimap<int, double>> value;
    auto *set = Traits::get_if<std::unordered_set<int>>(&value);
    ASSERT_EQ(set, nullptr);
}

HWTEST_F(TraitsTestNew, same_index_of_v024, TestSize.Level0)
{
    auto index = Traits::same_index_of_v<std::unique_ptr<int>, std::unique_ptr<int>, std::shared_ptr<int>>;
    ASSERT_EQ(index, 0);
    index = Traits::same_index_of_v<std::shared_ptr<int>, std::unique_ptr<int>, std::shared_ptr<int>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, same_in_v024, TestSize.Level0)
{
    auto exist = Traits::same_in_v<std::unique_ptr<int>, std::unique_ptr<int>, std::shared_ptr<int>>;
    ASSERT_TRUE(exist);
}

HWTEST_F(TraitsTestNew, convertible_index_of_v024, TestSize.Level0)
{
    auto index = Traits::convertible_index_of_v<std::unique_ptr<int>, std::shared_ptr<int>>;
    ASSERT_EQ(index, 1);
}

HWTEST_F(TraitsTestNew, convertible_in_v024, TestSize.Level0)
{
    auto convertible = Traits::convertible_in_v<std::unique_ptr<int>, std::shared_ptr<int>>;
    ASSERT_FALSE(convertible);
}

HWTEST_F(TraitsTestNew, variant_size_of_v024, TestSize.Level0)
{
    std::variant<std::unique_ptr<int>, std::unique_ptr<double>> value;
    auto size = Traits::variant_size_of_v<decltype(value)>;
    ASSERT_EQ(size, 2);
}

HWTEST_F(TraitsTestNew, variant_index_of_v024, TestSize.Level0)
{
    std::variant<std::unique_ptr<int>, std::unique_ptr<double>> value;
    auto index = Traits::variant_index_of_v<std::unique_ptr<int>, decltype(value)>;
    ASSERT_EQ(index, 0);
}

HWTEST_F(TraitsTestNew, get_if_same_type024, TestSize.Level0)
{
    std::variant<std::unique_ptr<int>, std::unique_ptr<double>> value;
    auto *up = Traits::get_if<std::unique_ptr<int>>(&value);
    ASSERT_NE(up, nullptr);
    ASSERT_EQ(up->get(), nullptr);
}
} // namespace OHOS::Test