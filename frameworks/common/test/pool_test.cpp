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

#include <functional>
#include <mutex>

#include "gtest/gtest.h"
#include "pool.h"
#include "log_print.h"

using namespace testing::ext;
using namespace OHOS;
namespace OHOS::Test {
class PoolTest : public testing::Test {
public:
    struct Node {
        int value;
        bool operator==(Node &other)
        {
            return value == other.value;
        }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr uint32_t CAPABILITY_TEST = 3; // capability
    static constexpr uint32_t MIN_TEST = 1;    // min
    static Pool<PoolTest::Node> pool_;
};
Pool<PoolTest::Node> PoolTest::pool_ = Pool<PoolTest::Node>(CAPABILITY_TEST, MIN_TEST);

void PoolTest::SetUpTestCase(void)
{}

void PoolTest::TearDownTestCase(void)
{}

void PoolTest::SetUp(void)
{}

void PoolTest::TearDown(void)
{
    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
    };
    pool_.Clean(close);
}

/**
* @tc.name: Get_001
* @tc.desc: test the std::shared_ptr<T> Get(bool isForce = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Get_001, TestSize.Level1)
{
    int index = 0;
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_EQ(ret, nullptr);
}

/**
* @tc.name: Get_002
* @tc.desc: test the std::shared_ptr<T> Get(bool isForce = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Get_002, TestSize.Level1)
{
    int index = 0;
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get(true);
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_EQ(ret, nullptr);
}

/**
* @tc.name: Release_001
* @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Release_001, TestSize.Level1)
{
    int index = 0;
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);

    pool_.Idle(ret);
    auto retRelease = pool_.Release(ret);
    EXPECT_EQ(retRelease, true);
}

/**
* @tc.name: Release_002
* @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Release_002, TestSize.Level1)
{
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);

    pool_.Idle(ret);
    auto retRelease = pool_.Release(ret);
    EXPECT_EQ(retRelease, false);
}

/**
* @tc.name: Release_003
* @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Release_003, TestSize.Level1)
{
    int index = 0;
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);

    pool_.Idle(ret);
    auto retRelease = pool_.Release(ret);
    EXPECT_EQ(retRelease, true);
}

/**
* @tc.name: Release_004
* @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Release_004, TestSize.Level1)
{
    int index = 0;
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get(true);
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_EQ(ret, nullptr);

    pool_.Idle(ret);
    auto retRelease = pool_.Release(ret);
    EXPECT_EQ(retRelease, false);
}

/**
* @tc.name: Release_005
* @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Release_005, TestSize.Level1)
{
    int index = 0;
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);

    auto data = std::make_shared<PoolTest::Node>();
    pool_.Idle(ret);
    auto retRelease = pool_.Release(data);
    EXPECT_EQ(retRelease, false);
}

/**
* @tc.name: Release_006
* @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Release_006, TestSize.Level1)
{
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);

    pool_.Idle(ret);
    auto retRelease = pool_.Release(ret, true);
    EXPECT_EQ(retRelease, true);
}

/**
* @tc.name: Release_007
* @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Release_007, TestSize.Level1)
{
    auto ret = nullptr;
    auto retRelease = pool_.Release(ret, true);
    EXPECT_EQ(retRelease, false);
}

/**
* @tc.name: Idle_001
* @tc.desc: test the  void Idle(std::shared_ptr<T> data) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Idle_001, TestSize.Level1)
{
    int index = 0;
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);

    pool_.Idle(ret);
    auto retRelease = pool_.Release(ret);
    EXPECT_EQ(retRelease, true);
}

/**
* @tc.name: Clean_001
* @tc.desc: test the int32_t Clean(std::function<void(std::shared_ptr<T>)> close) noexcept function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Clean_001, TestSize.Level1)
{
    int index = 0;
    auto ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;

    ret = pool_.Get();
    EXPECT_NE(ret, nullptr);

    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
    };
    auto retClean = pool_.Clean(close);
    EXPECT_EQ(retClean, true);
}
} // namespace OHOS::Test
