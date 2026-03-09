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

#include <functional>
#include <mutex>
#include <thread>

#include "log_print.h"
#include "pool.h"
#include "gtest/gtest.h"

using namespace testing::ext;
using namespace OHOS;
namespace OHOS::Test {
static constexpr uint32_t CAPABILITY_TEST_NEW = 3;
static constexpr uint32_t MIN_TEST_NEW = 1;
static constexpr uint32_t SMALL_POOL_SIZE = 2;
static constexpr uint32_t MEDIUM_POOL_SIZE = 3;
static constexpr uint32_t LARGE_ITERATIONS = 100;
static constexpr uint32_t MEDIUM_ITERATIONS = 50;
static constexpr uint32_t SMALL_ITERATIONS = 10;
static constexpr uint32_t TINY_ITERATIONS = 3;
static constexpr uint32_t THREAD_COUNT_HIGH = 20;
static constexpr uint32_t THREAD_COUNT_MEDIUM = 10;
static constexpr uint32_t ROUND_COUNT_LOW = 3;
static constexpr uint32_t ROUND_COUNT_MEDIUM = 5;
static constexpr uint32_t ROUND_COUNT_HIGH = 10;
static constexpr uint32_t ROUND_COUNT_VERY_HIGH = 20;
class PoolTestNew : public testing::Test {
public:
    struct Node {
        int value;
        bool operator==(Node &other)
        {
            return value == other.value;
        }
        explicit Node(const std::string &threadName = "pool_test_new") {};
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    static Pool<PoolTestNew::Node> poolNew_;

    static void ProcessNode(std::shared_ptr<Node> node, std::atomic<int>& counter)
    {
        if (node != nullptr) {
            counter++;
            poolNew_.Idle(node);
            poolNew_.Release(node);
        }
    }

    void ConcurrentGetOperation(std::atomic<int>& counter)
    {
        const int RETRY_TIMES = 10;
        for (int j = 0; j < RETRY_TIMES; ++j) {
            auto ret = poolNew_.Get();
            ProcessNode(ret, counter);
        }
    }
    
    void ConcurrentReleaseOperation(std::atomic<int>& counter)
    {
        const int RETRY_TIMES = 5;
        for (int j = 0; j < RETRY_TIMES; ++j) {
            auto ret = poolNew_.Get();
            ProcessNode(ret, counter);
        }
    }
};
Pool<PoolTestNew::Node> PoolTestNew::poolNew_ =
    Pool<PoolTestNew::Node>(CAPABILITY_TEST_NEW, MIN_TEST_NEW, "pool_test_new");

void PoolTestNew::SetUpTestCase(void) { }

void PoolTestNew::TearDownTestCase(void) { }

void PoolTestNew::SetUp(void) { }

void PoolTestNew::TearDown(void)
{
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    poolNew_.Clean(close);
}

/**
 * @tc.name: GetNew001
 * @tc.desc: test the std::shared_ptr Get(bool isForce) function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetNew001, TestSize.Level1)
{
    int index = 0;
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name: GetNew002
 * @tc.desc: test the std::shared_ptr Get(bool isForce) function with force
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetNew002, TestSize.Level1)
{
    int index = 0;
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get(true);
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name: ReleaseNew001
 * @tc.desc: test the int32_t Release function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNew001, TestSize.Level1)
{
    int index = 0;
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, true);
}

/**
 * @tc.name: ReleaseNew002
 * @tc.desc: test the int32_t Release function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNew002, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, false);
}

/**
 * @tc.name: ReleaseNew003
 * @tc.desc: test the int32_t Release function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNew003, TestSize.Level1)
{
    int index = 0;
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, true);
}

/**
 * @tc.name: ReleaseNew004
 * @tc.desc: test the int32_t Release function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNew004, TestSize.Level1)
{
    int index = 0;
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get(true);
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_EQ(ret, nullptr);
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, false);
}

/**
 * @tc.name: ReleaseNew005
 * @tc.desc: test the int32_t Release function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNew005, TestSize.Level1)
{
    int index = 0;
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    auto data = std::make_shared<PoolTestNew::Node>();
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(data);
    EXPECT_EQ(retRelease, false);
}

/**
 * @tc.name: ReleaseNew006
 * @tc.desc: test the int32_t Release function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNew006, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret, true);
    EXPECT_EQ(retRelease, true);
}

/**
 * @tc.name: ReleaseNew007
 * @tc.desc: test the int32_t Release function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNew007, TestSize.Level1)
{
    auto ret = nullptr;
    auto retRelease = poolNew_.Release(ret, true);
    EXPECT_EQ(retRelease, false);
}

/**
 * @tc.name: IdleNew001
 * @tc.desc: test the void Idle function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, IdleNew001, TestSize.Level1)
{
    int index = 0;
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, true);
}

/**
 * @tc.name: CleanNew001
 * @tc.desc: test the int32_t Clean function
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanNew001, TestSize.Level1)
{
    int index = 0;
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = index++;
    ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    auto retClean = poolNew_.Clean(close);
    EXPECT_EQ(retClean, true);
}

/**
 * @tc.name: GetMultipleTest
 * @tc.desc: test multiple get operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetMultipleTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto ret = poolNew_.Get();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name: IdleMultipleTest
 * @tc.desc: test multiple idle operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, IdleMultipleTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
    }
    // Only the first (CAPABILITY_TEST_NEW - MIN_TEST_NEW) releases will return true
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto retRelease = poolNew_.Release(nodes[i]);
        bool expected = (i < (CAPABILITY_TEST_NEW - MIN_TEST_NEW));
        EXPECT_EQ(retRelease, expected);
    }
}

/**
 * @tc.name: ReleaseMultipleTest
 * @tc.desc: test multiple release operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseMultipleTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
    }
    // Only the first (CAPABILITY_TEST_NEW - MIN_TEST_NEW) releases will return true
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto retRelease = poolNew_.Release(nodes[i]);
        bool expected = (i < (CAPABILITY_TEST_NEW - MIN_TEST_NEW));
        EXPECT_EQ(retRelease, expected);
    }
}

/**
 * @tc.name: CleanMultipleTest
 * @tc.desc: test clean operation with multiple nodes
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanMultipleTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto close = [&nodes](std::shared_ptr<PoolTestNew::Node> data) {
        for (auto &node : nodes) {
            if (node == data) {
                poolNew_.Idle(node);
                poolNew_.Release(node);
            }
        }
    };
    auto retClean = poolNew_.Clean(close);
    EXPECT_EQ(retClean, true);
}

/**
 * @tc.name: GetAfterReleaseTest
 * @tc.desc: test get after release operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetAfterReleaseTest, TestSize.Level1)
{
    for (int round = 0; round < 3; ++round) {
        std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
        for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
            auto ret = poolNew_.Get();
            EXPECT_NE(ret, nullptr);
            ret->value = i;
            nodes.push_back(ret);
        }
        for (auto &node : nodes) {
            poolNew_.Idle(node);
            poolNew_.Release(node);
        }
    }
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: GetWithForceTest
 * @tc.desc: test get with force parameter
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetWithForceTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto ret = poolNew_.Get(true);
    EXPECT_NE(ret, nullptr);
    ret->value = 100;
    poolNew_.Idle(ret);
    poolNew_.Release(ret);
}

/**
 * @tc.name: ReleaseNotIdleTest
 * @tc.desc: test release without idle operation
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNotIdleTest, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = 1;
    // When current_ equals min_, Release returns false
    auto retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, false);
    // Get another node to increase current_
    auto ret2 = poolNew_.Get();
    EXPECT_NE(ret2, nullptr);
    poolNew_.Idle(ret);
    // Now current_ is 2 and min_ is 1, so Release should return true
    retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, true);
    poolNew_.Release(ret2);
}

/**
 * @tc.name: ReleaseWithForceTest
 * @tc.desc: test release with force parameter
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseWithForceTest, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = 1;
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret, true);
    EXPECT_EQ(retRelease, true);
}

/**
 * @tc.name: GetNullPtrTest
 * @tc.desc: test get returns nullptr when pool is full
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetNullPtrTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW + 1; ++i) {
        auto ret = poolNew_.Get();
        if (i < CAPABILITY_TEST_NEW) {
            EXPECT_NE(ret, nullptr);
            ret->value = i;
            nodes.push_back(ret);
        } else {
            EXPECT_EQ(ret, nullptr);
        }
    }
}

/**
 * @tc.name: IdleAllTest
 * @tc.desc: test idle all acquired nodes
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, IdleAllTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
    }
    // Only the first (CAPABILITY_TEST_NEW - MIN_TEST_NEW) releases will return true
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto retRelease = poolNew_.Release(nodes[i]);
        bool expected = (i < (CAPABILITY_TEST_NEW - MIN_TEST_NEW));
        EXPECT_EQ(retRelease, expected);
    }
}

/**
 * @tc.name: CleanAllTest
 * @tc.desc: test clean all nodes from pool
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanAllTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    int cleanedCount = 0;
    auto close = [&cleanedCount](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
        cleanedCount++;
    };
    auto retClean = poolNew_.Clean(close);
    EXPECT_EQ(retClean, true);
    EXPECT_EQ(cleanedCount, CAPABILITY_TEST_NEW);
}

/**
 * @tc.name: GetReuseTest
 * @tc.desc: test get reuses released nodes
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetReuseTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i * 10;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
        poolNew_.Release(node);
    }
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
    }
}

/**
 * @tc.name: ConcurrentGetTest
 * @tc.desc: test concurrent get operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ConcurrentGetTest, TestSize.Level1)
{
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        threads.emplace_back([this, &counter]() {
            auto ret = poolNew_.Get();
            if (ret != nullptr) {
                counter++;
                poolNew_.Idle(ret);
                poolNew_.Release(ret);
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(counter, CAPABILITY_TEST_NEW);
}

/**
 * @tc.name: ConcurrentReleaseTest
 * @tc.desc: test concurrent release operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ConcurrentReleaseTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (auto &node : nodes) {
        threads.emplace_back([this, node, &counter]() {
            poolNew_.Idle(node);
            auto retRelease = poolNew_.Release(node);
            if (retRelease) {
                counter++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    // Only (CAPABILITY_TEST_NEW - MIN_TEST_NEW) releases will succeed
    EXPECT_EQ(counter, CAPABILITY_TEST_NEW - MIN_TEST_NEW);
}

/**
 * @tc.name: GetSequenceTest
 * @tc.desc: test get operations in sequence
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetSequenceTest, TestSize.Level1)
{
    for (int i = 0; i < CAPABILITY_TEST_NEW * 3; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: ReleaseNullTest
 * @tc.desc: test release with null pointer
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNullTest, TestSize.Level1)
{
    std::shared_ptr<PoolTestNew::Node> nullNode = nullptr;
    auto retRelease = poolNew_.Release(nullNode);
    EXPECT_EQ(retRelease, false);
    retRelease = poolNew_.Release(nullNode, true);
    EXPECT_EQ(retRelease, false);
}

/**
 * @tc.name: IdleNullTest
 * @tc.desc: test idle with null pointer
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, IdleNullTest, TestSize.Level1)
{
    std::shared_ptr<PoolTestNew::Node> nullNode = nullptr;
    poolNew_.Idle(nullNode);
    auto retRelease = poolNew_.Release(nullNode);
    EXPECT_EQ(retRelease, false);
}

/**
 * @tc.name: GetAfterCleanTest
 * @tc.desc: test get after clean operation
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetAfterCleanTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    poolNew_.Clean(close);
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
    }
}

/**
 * @tc.name: MultipleGetReleaseTest
 * @tc.desc: test multiple get and release cycles
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, MultipleGetReleaseTest, TestSize.Level1)
{
    for (int round = 0; round < 5; ++round) {
        std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
        for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
            auto ret = poolNew_.Get();
            EXPECT_NE(ret, nullptr);
            ret->value = round * 100 + i;
            nodes.push_back(ret);
        }
        for (auto &node : nodes) {
            poolNew_.Idle(node);
            poolNew_.Release(node);
        }
    }
}

/**
 * @tc.name: GetForceWhenFullTest
 * @tc.desc: test get with force when pool is full
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetForceWhenFullTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto ret = poolNew_.Get();
    EXPECT_EQ(ret, nullptr);
    ret = poolNew_.Get(true);
    EXPECT_NE(ret, nullptr);
    poolNew_.Idle(ret);
    poolNew_.Release(ret);
}

/**
 * @tc.name: ReleaseForceTest
 * @tc.desc: test release with force parameter
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseForceTest, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = 1;
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret, true);
    EXPECT_EQ(retRelease, true);
}

/**
 * @tc.name: MultipleGetForceTest
 * @tc.desc: test multiple get with force operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, MultipleGetForceTest, TestSize.Level1)
{
    for (int i = 0; i < 10; ++i) {
        auto ret = poolNew_.Get(true);
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: GetStressTest
 * @tc.desc: stress test get operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetStressTest, TestSize.Level1)
{
    for (int i = 0; i < LARGE_ITERATIONS; ++i) {
        auto ret = poolNew_.Get();
        if (ret != nullptr) {
            ret->value = i;
            poolNew_.Idle(ret);
            poolNew_.Release(ret);
        }
    }
}

/**
 * @tc.name: ReleaseStressTest
 * @tc.desc: stress test release operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseStressTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
        poolNew_.Release(node);
    }
    const int RETRY_TIMES = 50;
    for (int i = 0; i < RETRY_TIMES; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: CleanStressTest
 * @tc.desc: stress test clean operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanStressTest, TestSize.Level1)
{
    for (int round = 0; round < ROUND_COUNT_HIGH; ++round) {
        std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
        for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
            auto ret = poolNew_.Get();
            EXPECT_NE(ret, nullptr);
            ret->value = i;
            nodes.push_back(ret);
        }
        auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
            poolNew_.Idle(data);
            poolNew_.Release(data);
        };
        poolNew_.Clean(close);
    }
}

/**
 * @tc.name: GetAfterMultipleReleaseTest
 * @tc.desc: test get after multiple release operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetAfterMultipleReleaseTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        poolNew_.Idle(nodes[i]);
        poolNew_.Release(nodes[i]);
    }
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: IdleReleaseSequenceTest
 * @tc.desc: test idle and release sequence
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, IdleReleaseSequenceTest, TestSize.Level1)
{
    // Get all nodes first
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    // Then idle and release them
    for (auto &node : nodes) {
        poolNew_.Idle(node);
    }
    // Only the first (CAPABILITY_TEST_NEW - MIN_TEST_NEW) releases will return true
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto retRelease = poolNew_.Release(nodes[i]);
        bool expected = (i < (CAPABILITY_TEST_NEW - MIN_TEST_NEW));
        EXPECT_EQ(retRelease, expected);
    }
}

/**
 * @tc.name: GetIdleReleaseTest
 * @tc.desc: test get idle release cycle
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetIdleReleaseTest, TestSize.Level1)
{
    for (int round = 0; round < ROUND_COUNT_LOW; ++round) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = round;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: MultiplePoolTest
 * @tc.desc: test multiple pool instances
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, MultiplePoolTest, TestSize.Level1)
{
    Pool<PoolTestNew::Node> pool1(SMALL_POOL_SIZE, MIN_TEST_NEW, "pool1_new");
    Pool<PoolTestNew::Node> pool2(SMALL_POOL_SIZE, MIN_TEST_NEW, "pool2_new");
    auto ret1 = pool1.Get();
    auto ret2 = pool2.Get();
    EXPECT_NE(ret1, nullptr);
    EXPECT_NE(ret2, nullptr);
    ret1->value = 100;
    ret2->value = 200;
    pool1.Idle(ret1);
    pool2.Idle(ret2);
    pool1.Release(ret1);
    pool2.Release(ret2);
}

/**
 * @tc.name: GetFromMultiplePoolsTest
 * @tc.desc: test get from multiple pool instances
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetFromMultiplePoolsTest, TestSize.Level1)
{
    Pool<PoolTestNew::Node> pool1(MEDIUM_POOL_SIZE, MIN_TEST_NEW, "pool1_new");
    Pool<PoolTestNew::Node> pool2(MEDIUM_POOL_SIZE, MIN_TEST_NEW, "pool2_new");
    for (int i = 0; i < MEDIUM_POOL_SIZE; ++i) {
        auto ret1 = pool1.Get();
        auto ret2 = pool2.Get();
        EXPECT_NE(ret1, nullptr);
        EXPECT_NE(ret2, nullptr);
        pool1.Idle(ret1);
        pool2.Idle(ret2);
        pool1.Release(ret1);
        pool2.Release(ret2);
    }
}

/**
 * @tc.name: CleanPartialTest
 * @tc.desc: test clean with partial nodes
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanPartialTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW - 1; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    int cleanedCount = 0;
    auto close = [&cleanedCount](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
        cleanedCount++;
    };
    poolNew_.Clean(close);
    EXPECT_EQ(cleanedCount, CAPABILITY_TEST_NEW - 1);
}

/**
 * @tc.name: GetFullPoolTest
 * @tc.desc: test get when pool is full
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetFullPoolTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto ret = poolNew_.Get();
    EXPECT_EQ(ret, nullptr);
    for (auto &node : nodes) {
        poolNew_.Idle(node);
        poolNew_.Release(node);
    }
}

/**
 * @tc.name: GetEmptyPoolTest
 * @tc.desc: test get when pool is empty
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetEmptyPoolTest, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    poolNew_.Idle(ret);
    poolNew_.Release(ret);
}

/**
 * @tc.name: ReleaseEmptyPoolTest
 * @tc.desc: test release when pool has no nodes
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseEmptyPoolTest, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    poolNew_.Idle(ret);
    poolNew_.Release(ret);
    auto retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, false);
}

/**
 * @tc.name: GetReuseAfterCleanTest
 * @tc.desc: test get reuses nodes after clean
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetReuseAfterCleanTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    poolNew_.Clean(close);
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: IdleReleaseMultipleTest
 * @tc.desc: test idle and release multiple times
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, IdleReleaseMultipleTest, TestSize.Level1)
{
    for (int round = 0; round < ROUND_COUNT_HIGH; ++round) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = round;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: GetReleaseCycleTest
 * @tc.desc: test get release cycle
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetReleaseCycleTest, TestSize.Level1)
{
    for (int round = 0; round < ROUND_COUNT_VERY_HIGH; ++round) {
        std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
        for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
            auto ret = poolNew_.Get();
            EXPECT_NE(ret, nullptr);
            ret->value = i;
            nodes.push_back(ret);
        }
        for (auto &node : nodes) {
            poolNew_.Idle(node);
            poolNew_.Release(node);
        }
    }
}

/**
 * @tc.name: GetForceConcurrentTest
 * @tc.desc: test concurrent get with force
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetForceConcurrentTest, TestSize.Level1)
{
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (int i = 0; i < THREAD_COUNT_MEDIUM; ++i) {
        threads.emplace_back([this, &counter]() {
            auto ret = poolNew_.Get(true);
            if (ret != nullptr) {
                counter++;
                poolNew_.Idle(ret);
                poolNew_.Release(ret);
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_GT(counter, 0);
}

/**
 * @tc.name: CleanAllNodesTest
 * @tc.desc: test clean all nodes
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanAllNodesTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    int cleanedCount = 0;
    auto close = [&cleanedCount](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
        cleanedCount++;
    };
    poolNew_.Clean(close);
    EXPECT_EQ(cleanedCount, CAPABILITY_TEST_NEW);
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: GetConcurrentDifferentThreads
 * @tc.desc: test concurrent get from different threads
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetConcurrentDifferentThreads, TestSize.Level1)
{
    std::vector<std::thread> threads;
    std::atomic<int> successCount = 0;
    std::atomic<int> failCount = 0;
    for (int i = 0; i < THREAD_COUNT_HIGH; ++i) {
        threads.emplace_back([this, &successCount, &failCount]() {
            auto ret = poolNew_.Get();
            if (ret != nullptr) {
                successCount++;
                poolNew_.Idle(ret);
                poolNew_.Release(ret);
            } else {
                failCount++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_GT(successCount, 0);
}

/**
 * @tc.name: ReleaseConcurrentDifferentThreads
 * @tc.desc: test concurrent release from different threads
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseConcurrentDifferentThreads, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    std::vector<std::thread> threads;
    std::atomic<int> successCount = 0;
    for (auto &node : nodes) {
        threads.emplace_back([this, node, &successCount]() {
            poolNew_.Idle(node);
            auto retRelease = poolNew_.Release(node);
            if (retRelease) {
                successCount++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    // Only (CAPABILITY_TEST_NEW - MIN_TEST_NEW) releases will succeed
    EXPECT_EQ(successCount, CAPABILITY_TEST_NEW - MIN_TEST_NEW);
}

/**
 * @tc.name: GetAfterPartialCleanTest
 * @tc.desc: test get after partial clean
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetAfterPartialCleanTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    // First, get CAPABILITY_TEST_NEW nodes (should succeed)
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        if (ret != nullptr) {
            ret->value = i;
        }
        nodes.push_back(ret);
    }
    // Then try to get one more node (should fail as pool is at capacity)
    auto extraRet = poolNew_.Get();
    EXPECT_EQ(extraRet, nullptr);
    
    // Release half of the nodes
    for (size_t i = 0; i < nodes.size() / 2; ++i) {
        poolNew_.Idle(nodes[i]);
        poolNew_.Release(nodes[i]);
    }
    // Now we should be able to get new nodes again
    for (size_t i = 0; i < nodes.size() / 2; ++i) {
        auto ret = poolNew_.Get();
        // After releasing half the nodes, we should be able to get new ones
        EXPECT_NE(ret, nullptr);
        if (ret != nullptr) {
            ret->value = static_cast<int>(i + CAPABILITY_TEST_NEW);
        }
    }
}

/**
 * @tc.name: MultipleCleanTest
 * @tc.desc: test multiple clean operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, MultipleCleanTest, TestSize.Level1)
{
    for (int round = 0; round < ROUND_COUNT_MEDIUM; ++round) {
        std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
        for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
            auto ret = poolNew_.Get();
            EXPECT_NE(ret, nullptr);
            ret->value = i;
            nodes.push_back(ret);
        }
        auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
            poolNew_.Idle(data);
            poolNew_.Release(data);
        };
        poolNew_.Clean(close);
    }
}

/**
 * @tc.name: GetMixedTest
 * @tc.desc: test get with mixed parameters
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetMixedTest, TestSize.Level1)
{
    auto ret1 = poolNew_.Get();
    EXPECT_NE(ret1, nullptr);
    auto ret2 = poolNew_.Get();
    EXPECT_NE(ret2, nullptr);
    auto ret3 = poolNew_.Get();
    EXPECT_NE(ret3, nullptr);
    auto ret4 = poolNew_.Get(true);
    EXPECT_NE(ret4, nullptr);
    poolNew_.Idle(ret1);
    poolNew_.Release(ret1);
    poolNew_.Idle(ret2);
    poolNew_.Release(ret2);
    poolNew_.Idle(ret3);
    poolNew_.Release(ret3);
    poolNew_.Idle(ret4);
    poolNew_.Release(ret4);
}

/**
 * @tc.name: ReleaseMultipleTimesTest
 * @tc.desc: test release same node multiple times
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseMultipleTimesTest, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = 1;
    poolNew_.Idle(ret);
    auto retRelease1 = poolNew_.Release(ret, true);
    EXPECT_EQ(retRelease1, true);
    // The node is removed after first release, so second should return false
    auto retRelease2 = poolNew_.Release(ret);
    EXPECT_EQ(retRelease2, false);
}

/**
 * @tc.name: GetReleaseSequenceTest
 * @tc.desc: test get release sequence
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetReleaseSequenceTest, TestSize.Level1)
{
    for (int i = 0; i < MEDIUM_ITERATIONS; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: CleanNoNodesTest
 * @tc.desc: test clean when no nodes
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanNoNodesTest, TestSize.Level1)
{
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    auto retClean = poolNew_.Clean(close);
    EXPECT_EQ(retClean, true);
}

/**
 * @tc.name: GetForceFullPoolTest
 * @tc.desc: test get with force on full pool
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetForceFullPoolTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto ret = poolNew_.Get();
    EXPECT_EQ(ret, nullptr);
    ret = poolNew_.Get(true);
    EXPECT_NE(ret, nullptr);
    for (auto &node : nodes) {
        poolNew_.Idle(node);
    }
    for (auto &node : nodes) {
        poolNew_.Release(node);
    }
}

/**
 * @tc.name: IdleReleaseFullPoolTest
 * @tc.desc: test idle and release on full pool
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, IdleReleaseFullPoolTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
    }
    // Only the first (CAPABILITY_TEST_NEW - MIN_TEST_NEW) releases will return true
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto retRelease = poolNew_.Release(nodes[i]);
        bool expected = (i < (CAPABILITY_TEST_NEW - MIN_TEST_NEW));
        EXPECT_EQ(retRelease, expected);
    }
}

/**
 * @tc.name: GetAfterAllReleaseTest
 * @tc.desc: test get after all releases
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetAfterAllReleaseTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
        poolNew_.Release(node);
    }
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: GetConcurrentStressTest
 * @tc.desc: stress test concurrent get operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetConcurrentStressTest, TestSize.Level1)
{
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (int i = 0; i < 50; ++i) {
        threads.emplace_back([this, &counter]() {
            ConcurrentGetOperation(counter);
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_GT(counter, 0);
}

/**
 * @tc.name: ReleaseConcurrentStressTest
 * @tc.desc: stress test concurrent release operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseConcurrentStressTest, TestSize.Level1)
{
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([this, &counter]() {
            ConcurrentReleaseOperation(counter);
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_GT(counter, 0);
}

/**
 * @tc.name: GetSequenceMultiplePoolsTest
 * @tc.desc: test get sequence from multiple pools
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetSequenceMultiplePoolsTest, TestSize.Level1)
{
    Pool<PoolTestNew::Node> pool1(3, 1, "pool1_new");
    Pool<PoolTestNew::Node> pool2(3, 1, "pool2_new");
    for (int i = 0; i < 10; ++i) {
        auto ret1 = pool1.Get();
        auto ret2 = pool2.Get();
        EXPECT_NE(ret1, nullptr);
        EXPECT_NE(ret2, nullptr);
        pool1.Idle(ret1);
        pool2.Idle(ret2);
        pool1.Release(ret1);
        pool2.Release(ret2);
    }
}

/**
 * @tc.name: CleanMultiplePoolsTest
 * @tc.desc: test clean multiple pools
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanMultiplePoolsTest, TestSize.Level1)
{
    Pool<PoolTestNew::Node> pool1(3, 1, "pool1_new");
    Pool<PoolTestNew::Node> pool2(3, 1, "pool2_new");
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes1;
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes2;
    for (int i = 0; i < 3; ++i) {
        auto ret1 = pool1.Get();
        auto ret2 = pool2.Get();
        nodes1.push_back(ret1);
        nodes2.push_back(ret2);
    }
    auto close1 = [&pool1](std::shared_ptr<PoolTestNew::Node> data) {
        pool1.Idle(data);
        pool1.Release(data);
    };
    auto close2 = [&pool2](std::shared_ptr<PoolTestNew::Node> data) {
        pool2.Idle(data);
        pool2.Release(data);
    };
    pool1.Clean(close1);
    pool2.Clean(close2);
}

/**
 * @tc.name: GetWithDifferentValuesTest
 * @tc.desc: test get with different values
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetWithDifferentValuesTest, TestSize.Level1)
{
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i * 100;
    }
}

/**
 * @tc.name: ReleaseWithDifferentValuesTest
 * @tc.desc: test release with different values
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseWithDifferentValuesTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i * 100;
        nodes.push_back(ret);
    }
    for (size_t i = 0; i < nodes.size(); ++i) {
        EXPECT_EQ(nodes[i]->value, static_cast<int>(i) * 100);
        poolNew_.Idle(nodes[i]);
    }
    for (auto &node : nodes) {
        poolNew_.Release(node);
    }
}

/**
 * @tc.name: GetCleanGetTest
 * @tc.desc: test get clean get sequence
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetCleanGetTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    poolNew_.Clean(close);
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
    }
}

/**
 * @tc.name: GetIdleGetTest
 * @tc.desc: test get idle get sequence
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetIdleGetTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
    }
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        poolNew_.Idle(ret);
        poolNew_.Release(ret);
    }
}

/**
 * @tc.name: ReleaseGetTest
 * @tc.desc: test release get sequence
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseGetTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (auto &node : nodes) {
        poolNew_.Idle(node);
        poolNew_.Release(node);
    }
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
    }
}

/**
 * @tc.name: GetNullForceTest
 * @tc.desc: test get null with force
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetNullForceTest, TestSize.Level1)
{
    std::shared_ptr<PoolTestNew::Node> nullNode = nullptr;
    EXPECT_EQ(nullNode, nullptr);
}

/**
 * @tc.name: GetIdleNullTest
 * @tc.desc: test idle null node
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetIdleNullTest, TestSize.Level1)
{
    std::shared_ptr<PoolTestNew::Node> nullNode = nullptr;
    poolNew_.Idle(nullNode);
}

/**
 * @tc.name: ReleaseNullForceTest
 * @tc.desc: test release null with force
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, ReleaseNullForceTest, TestSize.Level1)
{
    std::shared_ptr<PoolTestNew::Node> nullNode = nullptr;
    auto retRelease = poolNew_.Release(nullNode, true);
    EXPECT_EQ(retRelease, false);
}

/**
 * @tc.name: MultipleGetNullTest
 * @tc.desc: test multiple get null checks
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, MultipleGetNullTest, TestSize.Level1)
{
    for (int i = 0; i < 5; ++i) {
        auto ret = poolNew_.Get();
        // First CAPABILITY_TEST_NEW (3) calls should succeed
        if (i < CAPABILITY_TEST_NEW) {
            EXPECT_NE(ret, nullptr);
        } else {
            // Later calls should fail if pool is full
            EXPECT_EQ(ret, nullptr);
        }
    }
}

/**
 * @tc.name: GetReleaseDifferentOrderTest
 * @tc.desc: test get release in different order
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetReleaseDifferentOrderTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    for (int i = CAPABILITY_TEST_NEW - 1; i >= 0; --i) {
        poolNew_.Idle(nodes[i]);
        poolNew_.Release(nodes[i]);
    }
}

/**
 * @tc.name: GetRandomReleaseTest
 * @tc.desc: test get random release
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetRandomReleaseTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    std::vector<int> indices = {0, 2, 1};
    for (int idx : indices) {
        poolNew_.Idle(nodes[idx]);
        poolNew_.Release(nodes[idx]);
    }
}

/**
 * @tc.name: GetIdleReleaseForceTest
 * @tc.desc: test get idle release with force
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetIdleReleaseForceTest, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = 1;
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret, true);
    EXPECT_EQ(retRelease, true);
}

/**
 * @tc.name: GetIdleReleaseNoForceTest
 * @tc.desc: test get idle release without force
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetIdleReleaseNoForceTest, TestSize.Level1)
{
    auto ret = poolNew_.Get();
    EXPECT_NE(ret, nullptr);
    ret->value = 1;
    // Get another node to increase current_ above min_
    auto ret2 = poolNew_.Get();
    EXPECT_NE(ret2, nullptr);
    poolNew_.Idle(ret);
    auto retRelease = poolNew_.Release(ret);
    EXPECT_EQ(retRelease, true);
    poolNew_.Release(ret2);
}

/**
 * @tc.name: CleanEmptyPoolTest
 * @tc.desc: test clean on empty pool
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, CleanEmptyPoolTest, TestSize.Level1)
{
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    auto retClean = poolNew_.Clean(close);
    EXPECT_EQ(retClean, true);
}

/**
 * @tc.name: GetCleanConcurrentTest
 * @tc.desc: test concurrent get and clean
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, GetCleanConcurrentTest, TestSize.Level1)
{
    std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
    for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
        auto ret = poolNew_.Get();
        EXPECT_NE(ret, nullptr);
        ret->value = i;
        nodes.push_back(ret);
    }
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    std::thread cleanThread([this, close]() {
        poolNew_.Clean(close);
    });
    std::thread getThread([this]() {
for (int i = 0; i < SMALL_ITERATIONS; ++i) {
            auto ret = poolNew_.Get();
            if (ret != nullptr) {
                poolNew_.Idle(ret);
                poolNew_.Release(ret);
            }
        }
    });
    cleanThread.join();
    getThread.join();
}

/**
 * @tc.name: FinalComprehensiveTest
 * @tc.desc: comprehensive test with all operations
 * @tc.type: FUNC
 */
HWTEST_F(PoolTestNew, FinalComprehensiveTest, TestSize.Level1)
{
    for (int round = 0; round < 3; ++round) {
        std::vector<std::shared_ptr<PoolTestNew::Node>> nodes;
        for (int i = 0; i < CAPABILITY_TEST_NEW; ++i) {
            auto ret = poolNew_.Get();
            EXPECT_NE(ret, nullptr);
            ret->value = round * 100 + i;
            nodes.push_back(ret);
        }
        for (auto &node : nodes) {
            poolNew_.Idle(node);
            poolNew_.Release(node);
        }
    }
    auto close = [](std::shared_ptr<PoolTestNew::Node> data) {
        poolNew_.Idle(data);
        poolNew_.Release(data);
    };
    poolNew_.Clean(close);
}

} // namespace OHOS::Test
