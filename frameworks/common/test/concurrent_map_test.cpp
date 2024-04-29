/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "ConcurrentMap"

#include "concurrent_map.h"
#include "gtest/gtest.h"
using namespace testing::ext;
namespace OHOS::Test {
template<typename _Key, typename _Tp> using ConcurrentMap = OHOS::ConcurrentMap<_Key, _Tp>;
class ConcurrentMapTest : public testing::Test {
public:
    struct TestValue {
        std::string id;
        std::string name;
        std::string testCase;
    };
    static void SetUpTestCase(void) {}

    static void TearDownTestCase(void) {}

protected:
    void SetUp()
    {
        values_.Clear();
    }

    void TearDown()
    {
        values_.Clear();
    }

    ConcurrentMap<std::string, TestValue> values_;
};

/**
* @tc.name: EmplaceWithNone
* @tc.desc: test the bool Emplace() noexcept function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(ConcurrentMapTest, EmplaceWithNone, TestSize.Level0)
{
    values_.Emplace();
    auto it = values_.Find("");
    ASSERT_TRUE(it.first);
}

/**
* @tc.name: EmplaceWithFilter
* @tc.desc: test the function:
 * template<typename _Filter, typename... _Args>
 * typename std::enable_if<std::is_convertible_v<_Filter, filter_type>, bool>::type
 * Emplace(const _Filter &filter, _Args &&...args) noexcept
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(ConcurrentMapTest, EmplaceWithFilter, TestSize.Level0)
{
    auto success = values_.Emplace(
        [](const decltype(values_)::map_type &entries) {
            return (entries.find("test") == entries.end());
        },
        "test", TestValue{ "id", "name", "test case" });
    ASSERT_TRUE(success);
    success = values_.Emplace(
        [](const decltype(values_)::map_type &entries) {
            return (entries.find("test") == entries.end());
        },
        "test", TestValue{ "id", "name", "test case" });
    ASSERT_TRUE(!success);
    success = values_.Emplace(
        [](decltype(values_)::map_type &entries) {
            auto it = entries.find("test");
            if (it != entries.end()) {
                entries.erase(it);
                it = entries.end();
            }
            return (it == entries.end());
        },
        "test", TestValue{ "id", "name", "test case" });
    ASSERT_TRUE(success);
}

/**
* @tc.name: EmplaceWithArgs
* @tc.desc: test the function:
 * template<typename... _Args>
 * typename std::enable_if<!std::is_convertible_v<decltype(First<_Args...>()), filter_type>, bool>::type
 * Emplace(_Args &&...args) noexcept
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(ConcurrentMapTest, EmplaceWithArgs, TestSize.Level0)
{
    TestValue value = { "id", "name", "test case" };
    auto success = values_.Emplace("test", value);
    ASSERT_TRUE(success);
    auto it = values_.Find("test");
    ASSERT_TRUE(it.first);
    ASSERT_EQ(it.second.id, value.id);
    ASSERT_EQ(it.second.name, value.name);
    ASSERT_EQ(it.second.testCase, value.testCase);
    values_.Clear();
    success = values_.Emplace(std::piecewise_construct, std::forward_as_tuple("test"), std::forward_as_tuple(value));
    ASSERT_TRUE(success);
    it = values_.Find("test");
    ASSERT_TRUE(it.first);
    ASSERT_EQ(it.second.id, value.id);
    ASSERT_EQ(it.second.name, value.name);
    ASSERT_EQ(it.second.testCase, value.testCase);
}
} // namespace OHOS::Test