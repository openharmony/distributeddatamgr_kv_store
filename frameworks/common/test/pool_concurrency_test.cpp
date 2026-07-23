/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <atomic>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "pool.h"

using namespace testing::ext;

namespace OHOS::Test {
namespace {
constexpr uint32_t NODE_COUNT = 3;

struct TestNode {
    explicit TestNode(const std::string &) {}
};

void ReleaseNodes(const std::shared_ptr<Pool<TestNode>> &pool, const std::vector<std::shared_ptr<TestNode>> &nodes,
                  const std::shared_ptr<std::atomic<uint32_t>> &releaseCount)
{
    std::vector<std::thread> workers;
    for (const auto &node : nodes) {
        workers.emplace_back([pool, node, releaseCount]() {
            pool->Idle(node);
            if (pool->Release(node, true)) {
                releaseCount->fetch_add(1);
            }
        });
    }
    for (auto &worker : workers) {
        worker.join();
    }
}
} // namespace

/**
 * @tc.name: Clean_001
 * @tc.desc: test Clean snapshots nodes before concurrent Idle and Release operations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST(PoolConcurrencyTest, Clean_001, TestSize.Level1)
{
    auto pool = std::make_shared<Pool<TestNode>>(NODE_COUNT, 0, "pool_concurrency_test");
    std::vector<std::shared_ptr<TestNode>> nodes;
    for (uint32_t i = 0; i < NODE_COUNT; ++i) {
        auto node = pool->Get();
        ASSERT_NE(node, nullptr);
        nodes.push_back(node);
    }

    auto closeCount = std::make_shared<std::atomic<uint32_t>>(0);
    auto releaseCount = std::make_shared<std::atomic<uint32_t>>(0);
    auto close = [pool, nodes, closeCount, releaseCount](std::shared_ptr<TestNode>) {
        EXPECT_EQ(pool->Get(), nullptr);
        auto invocation = closeCount->fetch_add(1);
        if (invocation == 0) {
            ReleaseNodes(pool, nodes, releaseCount);
        }
    };

    EXPECT_TRUE(pool->Clean(close));
    EXPECT_EQ(closeCount->load(), NODE_COUNT);
    EXPECT_EQ(releaseCount->load(), NODE_COUNT);

    auto reusedNode = pool->Get();
    ASSERT_NE(reusedNode, nullptr);
    pool->Idle(reusedNode);
    EXPECT_TRUE(pool->Release(reusedNode, true));
}
} // namespace OHOS::Test
