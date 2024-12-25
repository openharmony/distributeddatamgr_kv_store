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

#include <functional>

#include "log_print.h"
#include "pool.h"
#include "gtest/gtest.h"
#include <mutex>

using namespace testing::ext;
using namespace OHOS;
namespace OHOS::Test {
static constexpr uint32_t MIN_TEST = 1;
static constexpr uint32_t CAPA_TEST = 3;
class PoolUnittest : public testing::Test {
public:
    struct Node {
        int node;
        bool operator==(Node &other)
        {
            return node == other.node;
        }
    };

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static Pool<PoolUnittest::Node> poolTest;
};

Pool<PoolUnittest::Node> PoolUnittest::poolTest = Pool<PoolUnittest::Node>(CAPA_TEST, MIN_TEST);

void PoolUnittest::SetUpTestCase(void) { }

void PoolUnittest::TearDownTestCase(void) { }

void PoolUnittest::SetUp(void) { }

void PoolUnittest::TearDown(void)
{
    auto close = [](std::shared_ptr<PoolUnittest::Node> data) {
        poolTest.Idle(data);
        poolTest.Release(data);
    };
    poolTest.Clean(close);
}

/**
 * @tc.name: Get_001
 * @tc.desc: test the std::shared_ptr<T> Get(bool isForce = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Get_001Test, TestSize.Level1)
{
    int flag = 0;
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_EQ(result, nullptr);
}

/**
 * @tc.name: Get_002
 * @tc.desc: test the std::shared_ptr<T> Get(bool isForce = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Get_002Test, TestSize.Level1)
{
    int flag = 0;
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get(true);
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_EQ(result, nullptr);
}

/**
 * @tc.name: Release_001
 * @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Release_001Test, TestSize.Level1)
{
    int flag = 0;
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);

    poolTest.Idle(result);
    auto retRelease = poolTest.Release(result);
    ASSERT_EQ(retRelease, true);
}

/**
 * @tc.name: Release_002
 * @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Release_002Test, TestSize.Level1)
{
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);

    poolTest.Idle(result);
    auto retRelease = poolTest.Release(result);
    ASSERT_EQ(retRelease, false);
}

/**
 * @tc.name: Release_003
 * @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Release_003Test, TestSize.Level1)
{
    int flag = 0;
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);

    poolTest.Idle(result);
    auto retRelease = poolTest.Release(result);
    ASSERT_EQ(retRelease, true);
}

/**
 * @tc.name: Release_004
 * @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Release_004Test, TestSize.Level1)
{
    int flag = 0;
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get(true);
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_EQ(result, nullptr);

    poolTest.Idle(result);
    auto retRelease = poolTest.Release(result);
    ASSERT_EQ(retRelease, false);
}

/**
 * @tc.name: Release_005
 * @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Release_005Test, TestSize.Level1)
{
    int flag = 0;
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);

    auto data = std::make_shared<PoolUnittest::Node>();
    poolTest.Idle(result);
    auto retRelease = poolTest.Release(data);
    ASSERT_EQ(retRelease, false);
}

/**
 * @tc.name: Release_006
 * @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Release_006Test, TestSize.Level1)
{
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);

    poolTest.Idle(result);
    auto retRelease = poolTest.Release(result, true);
    ASSERT_EQ(retRelease, true);
}

/**
 * @tc.name: Release_007
 * @tc.desc: test the int32_t Release(std::shared_ptr<T> data, bool force = false) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Release_007Test, TestSize.Level1)
{
    auto result = nullptr;
    auto retRelease = poolTest.Release(result, true);
    ASSERT_EQ(retRelease, false);
}

/**
 * @tc.name: Idle_001
 * @tc.desc: test the  void Idle(std::shared_ptr<T> data) function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Idle_001Test, TestSize.Level1)
{
    int flag = 0;
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);

    poolTest.Idle(result);
    auto retRelease = poolTest.Release(result);
    ASSERT_EQ(retRelease, true);
}

/**
 * @tc.name: Clean_001
 * @tc.desc: test the int32_t Clean(std::function<void(std::shared_ptr<T>)> close) noexcept function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PoolUnittest, Clean_001Test, TestSize.Level1)
{
    int flag = 0;
    auto result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);
    result->node = flag++;

    result = poolTest.Get();
    ASSERT_NE(result, nullptr);

    auto close = [](std::shared_ptr<PoolUnittest::Node> data) {
        poolTest.Idle(data);
        poolTest.Release(data);
    };
    auto retClean = poolTest.Clean(close);

    ASSERT_EQ(retClean, true);
}
} // namespace OHOS::Test
