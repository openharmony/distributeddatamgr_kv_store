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

#define LOG_TAG "Pool"

#include <functional>
#include <mutex>

#include "gtest/gtest.h"
#include "executor.h"
#include "pool.h"
#include "log_print.h"

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class PoolTest : public testing::Test {
public:
    struct Node {
        int value;
        bool operator==(Node &other){
            return value == other.value;
        }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr uint32_t CAPABILITY_ = 3; // capability
    static constexpr uint32_t MIN_ = 1;    // min
    static Pool<PoolTest::Node> pool_;
};
Pool<PoolTest::Node> PoolTest::pool_ = Pool<PoolTest::Node>(CAPABILITY_, MIN_);

void PoolTest::SetUpTestCase(void)
{}

void PoolTest::TearDownTestCase(void)
{}

void PoolTest::SetUp(void)
{}

void PoolTest::TearDown(void)
{}

/**
* @tc.name: Get_001
* @tc.desc: test the std::shared_ptr<T> Get(bool isForce = false) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PoolTest, Get_001, TestSize.Level1)
{
    auto ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_EQ(ret, nullptr);

    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
        // Do nothing, just a placeholder for the close function.
    };
    auto Ret = pool_.Clean(close);
    EXPECT_EQ(Ret, true);
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
    auto ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get(true);
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_EQ(ret, nullptr);

    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
        // Do nothing, just a placeholder for the close function.
    };
    auto Ret = pool_.Clean(close);
    EXPECT_EQ(Ret, true);
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
    auto ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    pool_.Idle(ret);
    auto Ret = pool_.Release(ret);
    EXPECT_EQ(Ret, true);
    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
        // Do nothing, just a placeholder for the close function.
    };
    Ret = pool_.Clean(close);
    EXPECT_EQ(Ret, true);
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
    EXPECT_TRUE(ret != nullptr);
    pool_.Idle(ret);
    auto Ret = pool_.Release(ret);
    EXPECT_EQ(Ret, false);
    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
        // Do nothing, just a placeholder for the close function.
    };
    Ret = pool_.Clean(close);
    EXPECT_EQ(Ret, true);
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
    auto ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    pool_.Idle(ret);
    auto Ret = pool_.Release(ret);
    EXPECT_EQ(Ret, true);
    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
        // Do nothing, just a placeholder for the close function.
    };
    Ret = pool_.Clean(close);
    EXPECT_EQ(Ret, true);
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
    auto ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get(true);
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_EQ(ret, nullptr);
    pool_.Idle(ret);
    auto Ret = pool_.Release(ret);
    EXPECT_EQ(Ret, false);
    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
        // Do nothing, just a placeholder for the close function.
    };
    Ret = pool_.Clean(close);
    EXPECT_EQ(Ret, true);
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
    auto ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get(true);
    pool_.Idle(ret);
    auto Ret = pool_.Release(ret);
    EXPECT_EQ(Ret, true);
    ZLOGE("test_Idle passed.");
    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
        // Do nothing, just a placeholder for the close function.
    };
    Ret = pool_.Clean(close);
    EXPECT_EQ(Ret, true);
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
    auto ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);
    ret = pool_.Get();
    EXPECT_TRUE(ret != nullptr);

    auto close = [](std::shared_ptr<PoolTest::Node> data) {
        pool_.Idle(data);
        pool_.Release(data);
        // Do nothing, just a placeholder for the close function.
    };
    auto Ret = pool_.Clean(close);
    EXPECT_EQ(Ret, true);
}
} // namespace OHOS::Test
