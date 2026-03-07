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

#include "priority_queue.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS;
using TaskIdNew = uint64_t;
using TaskNew = std::function<void()>;
using DurationNew = std::chrono::steady_clock::duration;
using TimeNew = std::chrono::steady_clock::time_point;
static constexpr DurationNew INVALID_INTERVAL_NEW = std::chrono::milliseconds(0);
static constexpr uint64_t UNLIMITED_TIMES_NEW = std::numeric_limits<uint64_t>::max();
static constexpr TaskIdNew INVALID_TASK_ID_NEW = static_cast<uint64_t>(0);
static constexpr uint32_t SHORT_INTERVAL_NEW = 100;
class PriorityQueueTestNew : public testing::Test {
public:
    struct TestTaskNew {
        std::function<void()> exec = []() {};
        DurationNew interval = INVALID_INTERVAL_NEW;
        uint64_t times = UNLIMITED_TIMES_NEW;
        TaskIdNew taskId = INVALID_TASK_ID_NEW;
        TestTaskNew() = default;

        bool Valid() const
        {
            return taskId != INVALID_TASK_ID_NEW;
        }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static PriorityQueue<PriorityQueueTestNew::TestTaskNew, TimeNew, TaskIdNew> priorityqueueNew_;
    static PriorityQueue<PriorityQueueTestNew::TestTaskNew, TimeNew, TaskIdNew>::PQMatrix pqMatrixNew;
};
using TestTaskNew = PriorityQueueTestNew::TestTaskNew;
PriorityQueue<TestTaskNew, TimeNew, TaskIdNew> PriorityQueueTestNew::priorityqueueNew_ =
    PriorityQueue<TestTaskNew, TimeNew, TaskIdNew>(TestTaskNew());
PriorityQueue<TestTaskNew, TimeNew, TaskIdNew>::PQMatrix PriorityQueueTestNew::pqMatrixNew =
    PriorityQueue<TestTaskNew, TimeNew, TaskIdNew>::PQMatrix(TestTaskNew(), INVALID_TASK_ID_NEW);

void PriorityQueueTestNew::SetUpTestCase(void) { }

void PriorityQueueTestNew::TearDownTestCase(void) { }

void PriorityQueueTestNew::SetUp(void) { }

void PriorityQueueTestNew::TearDown(void)
{
    priorityqueueNew_.Clean();
}

/**
 * @tc.name: PQMatrixNew001
 * @tc.desc: test PQMatrix function
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PQMatrixNew001, TestSize.Level1)
{
    TestTaskNew testTask;
    EXPECT_EQ(pqMatrixNew.id_, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: PushPopSizeNew001
 * @tc.desc: Invalid test task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushPopSizeNew001, TestSize.Level1)
{
    TestTaskNew testTask;
    auto id = testTask.taskId;
    auto timely = std::chrono::seconds(0);
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
    EXPECT_EQ(ret, false);
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, INVALID_TASK_ID_NEW);
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: PushPopSizeNew002
 * @tc.desc: Testing a single task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushPopSizeNew002, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto timely = std::chrono::seconds(0);
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 1u);
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, id);
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: PushPopSizeNew003
 * @tc.desc: Testing multiple tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushPopSizeNew003, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 10u);
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, 1);
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 9u);
}

/**
 * @tc.name: PushPopSizeNew004
 * @tc.desc: Test delay task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushPopSizeNew004, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.times = 1;
    for (int i = 0; i < 5; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 6);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 5; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 10u);
    for (int i = 0; i < 5; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_EQ(retPop.taskId, i + 1);
    }
    for (int i = 0; i < 5; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_EQ(retPop.taskId, i + 6);
    }
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: PushPopSizeNew005
 * @tc.desc: Test delay task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushPopSizeNew005, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.times = 1;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW * 10);
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 1u);
    auto delayA = std::chrono::steady_clock::now();
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, id);
    auto delayB = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(delayB - delayA).count();
    auto delayms = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();

    EXPECT_LT(diff, delayms * 2);
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: FindNew001
 * @tc.desc: Invalid test task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FindNew001, TestSize.Level1)
{
    TestTaskNew testTask;
    auto id = testTask.taskId;
    auto timely = std::chrono::seconds(0);
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
    EXPECT_EQ(ret, false);
    auto retFind = priorityqueueNew_.Find(id);
    EXPECT_EQ(retFind.taskId, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: FindNew002
 * @tc.desc: test priority_queue Find function
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FindNew002, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 10u);
    auto retFind = priorityqueueNew_.Find(5);
    EXPECT_EQ(retFind.taskId, 5);
    retFind = priorityqueueNew_.Find(20);
    EXPECT_EQ(retFind.taskId, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: UpdateNew001
 * @tc.desc: Invalid test task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateNew001, TestSize.Level1)
{
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    TestTaskNew testTask;
    testTask.times = 3;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
    auto retUpdate = priorityqueueNew_.Update(id, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
 * @tc.name: UpdateNew002
 * @tc.desc: Test normal tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateNew002, TestSize.Level1)
{
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    TestTaskNew testTask;
    testTask.times = 3;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retUpdate = priorityqueueNew_.Update(id, updater);
    EXPECT_EQ(retUpdate, true);
}

/**
 * @tc.name: UpdateNew003
 * @tc.desc: Test running tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateNew003, TestSize.Level1)
{
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    TestTaskNew testTask;
    testTask.times = 3;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueueNew_.Pop();
    auto retUpdate = priorityqueueNew_.Update(retPop.taskId, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
 * @tc.name: UpdateNew004
 * @tc.desc: Test running tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateNew004, TestSize.Level1)
{
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { true, TimeNew() };
    };
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    TestTaskNew testTask;
    testTask.times = 3;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueueNew_.Pop();
    auto retUpdate = priorityqueueNew_.Update(retPop.taskId, updater);
    EXPECT_EQ(retUpdate, true);
}

/**
 * @tc.name: UpdateNew005
 * @tc.desc: Test running and finish tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateNew005, TestSize.Level1)
{
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    TestTaskNew testTask;
    testTask.times = 3;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueueNew_.Pop();
    priorityqueueNew_.Finish(id);
    auto retUpdate = priorityqueueNew_.Update(retPop.taskId, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
 * @tc.name: UpdateNew006
 * @tc.desc: Test running and finish tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateNew006, TestSize.Level1)
{
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { true, TimeNew() };
    };
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    TestTaskNew testTask;
    testTask.times = 3;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueueNew_.Pop();
    priorityqueueNew_.Finish(id);
    auto retUpdate = priorityqueueNew_.Update(retPop.taskId, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
 * @tc.name: RemoveNew001
 * @tc.desc: Invalid test task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, RemoveNew001, TestSize.Level1)
{
    TestTaskNew testTask;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
    auto retRemove = priorityqueueNew_.Remove(id, false);
    EXPECT_EQ(retRemove, false);
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: RemoveNew002
 * @tc.desc: Single and don't wait test task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, RemoveNew002, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 1u);
    auto retRemove = priorityqueueNew_.Remove(id, false);
    EXPECT_EQ(retRemove, true);
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: RemoveNew003
 * @tc.desc: Single and wait test task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, RemoveNew003, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 1u);
    priorityqueueNew_.Finish(id);
    auto retRemove = priorityqueueNew_.Remove(id, true);
    EXPECT_EQ(retRemove, true);
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: CleanNew001
 * @tc.desc: Testing a single task
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, CleanNew001, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.taskId = 1;
    auto timely = std::chrono::seconds(0);
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 1u);
    priorityqueueNew_.Clean();
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: CleanNew002
 * @tc.desc: Testing multiple tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, CleanNew002, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 10u);
    priorityqueueNew_.Clean();
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: FinishNew001
 * @tc.desc: test priority_queue Finish function
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FinishNew001, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 1u);
    priorityqueueNew_.Finish(id);
    auto retRemove = priorityqueueNew_.Remove(id, true);
    EXPECT_EQ(retRemove, true);
    retSize = priorityqueueNew_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
 * @tc.name: MultiplePushTest
 * @tc.desc: test multiple push operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, MultiplePushTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 50; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 50u);
}

/**
 * @tc.name: MultiplePopTest
 * @tc.desc: test multiple pop operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, MultiplePopTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 30; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 30; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_EQ(retPop.taskId, i + 1);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: MultipleFindTest
 * @tc.desc: test multiple find operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, MultipleFindTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 40; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 40; ++i) {
        auto retFind = priorityqueueNew_.Find(i + 1);
        EXPECT_EQ(retFind.taskId, i + 1);
    }
}

/**
 * @tc.name: MultipleUpdateTest
 * @tc.desc: test multiple update operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, MultipleUpdateTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 25; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    for (int i = 0; i < 25; ++i) {
        auto retUpdate = priorityqueueNew_.Update(i + 1, updater);
        EXPECT_EQ(retUpdate, true);
    }
}

/**
 * @tc.name: MultipleRemoveTest
 * @tc.desc: test multiple remove operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, MultipleRemoveTest, TestSize.Level1)
{
    TestTaskNew testTask;
    std::vector<TaskIdNew> ids;
    for (int i = 0; i < 20; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
        ids.push_back(id);
    }
    for (auto id : ids) {
        auto retRemove = priorityqueueNew_.Remove(id, false);
        EXPECT_EQ(retRemove, true);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: MultipleFinishTest
 * @tc.desc: test multiple finish operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, MultipleFinishTest, TestSize.Level1)
{
    TestTaskNew testTask;
    std::vector<TaskIdNew> ids;
    for (int i = 0; i < 15; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
        ids.push_back(id);
    }
    for (auto id : ids) {
        priorityqueueNew_.Finish(id);
        auto retRemove = priorityqueueNew_.Remove(id, true);
        EXPECT_EQ(retRemove, true);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: PushPopCycleTest
 * @tc.desc: test push pop cycle
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushPopCycleTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int round = 0; round < 5; ++round) {
        for (int i = 0; i < 10; ++i) {
            auto timely = std::chrono::seconds(0);
            testTask.taskId = static_cast<TaskIdNew>(round * 10 + i + 1);
            auto id = testTask.taskId;
            auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
            EXPECT_EQ(ret, true);
        }
        for (int i = 0; i < 10; ++i) {
            auto retPop = priorityqueueNew_.Pop();
            EXPECT_NE(retPop.taskId, INVALID_TASK_ID_NEW);
        }
    }
}

/**
 * @tc.name: FindNotFoundTest
 * @tc.desc: test find for non-existent tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FindNotFoundTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retFind = priorityqueueNew_.Find(100);
    EXPECT_EQ(retFind.taskId, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: UpdateNotFoundTest
 * @tc.desc: test update for non-existent tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateNotFoundTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    auto retUpdate = priorityqueueNew_.Update(100, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
 * @tc.name: RemoveNotFoundTest
 * @tc.desc: test remove for non-existent tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, RemoveNotFoundTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    auto retRemove = priorityqueueNew_.Remove(100, false);
    EXPECT_EQ(retRemove, false);
}

/**
 * @tc.name: FinishNotFoundTest
 * @tc.desc: test finish for non-existent tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FinishNotFoundTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    priorityqueueNew_.Finish(100);
    EXPECT_EQ(priorityqueueNew_.Size(), 10u);
}

/**
 * @tc.name: CleanEmptyTest
 * @tc.desc: test clean on empty queue
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, CleanEmptyTest, TestSize.Level1)
{
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
    priorityqueueNew_.Clean();
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: PopEmptyTest
 * @tc.desc: test pop on empty queue
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PopEmptyTest, TestSize.Level1)
{
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: SizeAfterCleanTest
 * @tc.desc: test size after clean
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, SizeAfterCleanTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 20; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 20u);
    priorityqueueNew_.Clean();
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: FindAfterPopTest
 * @tc.desc: test find after pop
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FindAfterPopTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 15; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, 1);
    auto retFind = priorityqueueNew_.Find(1);
    EXPECT_EQ(retFind.taskId, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: UpdateAfterPopTest
 * @tc.desc: test update after pop
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateAfterPopTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, 1);
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    auto retUpdate = priorityqueueNew_.Update(1, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
 * @tc.name: RemoveAfterPopTest
 * @tc.desc: test remove after pop
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, RemoveAfterPopTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 8; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, 1);
    auto retRemove = priorityqueueNew_.Remove(1, false);
    EXPECT_EQ(retRemove, false);
}

/**
 * @tc.name: FinishAfterPopTest
 * @tc.desc: test finish after pop
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FinishAfterPopTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 12; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, 1);
    priorityqueueNew_.Finish(1);
}

/**
 * @tc.name: PushPopFindTest
 * @tc.desc: test push pop find sequence
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushPopFindTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 18; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 9; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_EQ(retPop.taskId, i + 1);
    }
    for (int i = 10; i < 18; ++i) {
        auto retFind = priorityqueueNew_.Find(i);
        EXPECT_EQ(retFind.taskId, i);
    }
}

/**
 * @tc.name: LargeScaleTest
 * @tc.desc: test large scale operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, LargeScaleTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 100; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 100u);
    for (int i = 0; i < 100; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_EQ(retPop.taskId, i + 1);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: DelayTest
 * @tc.desc: test delay tasks
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, DelayTest, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.times = 1;
    for (int i = 0; i < 10; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW * (i + 1));
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 10; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_NE(retPop.taskId, INVALID_TASK_ID_NEW);
    }
}

/**
 * @tc.name: TimesTest
 * @tc.desc: test task execution times
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, TimesTest, TestSize.Level1)
{
    TestTaskNew testTask;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    testTask.taskId = 1;
    auto id = testTask.taskId;
    

    auto rescheduleUpdater = [delay](TestTaskNew &task) {
        if (task.times > 0) {
            task.times--;
            return std::pair<bool, TimeNew> { true, std::chrono::steady_clock::now() + delay };
        }
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    

    PriorityQueue<TestTaskNew, TimeNew, TaskIdNew> testQueue(testTask, rescheduleUpdater);
    
    testTask.times = 5;
    auto ret = testQueue.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    

    auto retPop = testQueue.Pop();
    EXPECT_EQ(retPop.taskId, id);
    

    for (int i = 0; i < 4; ++i) {
        testQueue.Finish(id);
        auto retPopNext = testQueue.Pop();
        EXPECT_EQ(retPopNext.taskId, id);
    }
}

/**
 * @tc.name: MultipleTimesTest
 * @tc.desc: test multiple tasks with different times
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, MultipleTimesTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        testTask.times = i + 1;
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
}

/**
 * @tc.name: EmptyPopTest
 * @tc.desc: test pop when queue becomes empty
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, EmptyPopTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 5; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 5; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_EQ(retPop.taskId, i + 1);
    }
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: FindAfterRemoveTest
 * @tc.desc: test find after remove
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FindAfterRemoveTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 15; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 15; ++i) {
        auto retFind = priorityqueueNew_.Find(i + 1);
        EXPECT_EQ(retFind.taskId, i + 1);
        auto retRemove = priorityqueueNew_.Remove(i + 1, false);
        EXPECT_EQ(retRemove, true);
    }
    auto retFind = priorityqueueNew_.Find(1);
    EXPECT_EQ(retFind.taskId, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: CleanAfterPushTest
 * @tc.desc: test clean after push
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, CleanAfterPushTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 25; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 25u);
    priorityqueueNew_.Clean();
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: UpdateIntervalTest
 * @tc.desc: test update with interval
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateIntervalTest, TestSize.Level1)
{
    TestTaskNew testTask;
    testTask.times = 3;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { true, std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL_NEW * 2) };
    };
    auto retUpdate = priorityqueueNew_.Update(id, updater);
    EXPECT_EQ(retUpdate, true);
}

/**
 * @tc.name: RemoveWithFinishTest
 * @tc.desc: test remove with finish
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, RemoveWithFinishTest, TestSize.Level1)
{
    TestTaskNew testTask;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
    testTask.taskId = 1;
    auto id = testTask.taskId;
    auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    
    // Test case 1: Remove from pending state
    auto retRemove1 = priorityqueueNew_.Remove(id, false);
    EXPECT_EQ(retRemove1, true);
    
    // Test case 2: Push, pop and finish
    ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, id);
    priorityqueueNew_.Finish(id);
    
    // Test case 3: Push again and remove from pending state
    ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retRemove2 = priorityqueueNew_.Remove(id, false);
    EXPECT_EQ(retRemove2, true);
}

/**
 * @tc.name: PushConcurrentTest
 * @tc.desc: test concurrent push operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushConcurrentTest, TestSize.Level1)
{
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &counter, i]() {
            TestTaskNew testTask;
            testTask.taskId = static_cast<TaskIdNew>(i + 1);
            auto timely = std::chrono::seconds(0);
            auto id = testTask.taskId;
            auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
            if (ret) {
                counter++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(counter, 10);
}

/**
 * @tc.name: PopConcurrentTest
 * @tc.desc: test concurrent pop operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PopConcurrentTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, &counter]() {
            auto retPop = priorityqueueNew_.Pop();
            if (retPop.taskId != INVALID_TASK_ID_NEW) {
                counter++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(counter, 10);
}

/**
 * @tc.name: FindConcurrentTest
 * @tc.desc: test concurrent find operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FindConcurrentTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 15; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (int i = 0; i < 15; ++i) {
        threads.emplace_back([this, i, &counter]() {
            auto retFind = priorityqueueNew_.Find(i + 1);
            if (retFind.taskId == i + 1) {
                counter++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(counter, 15);
}

/**
 * @tc.name: UpdateConcurrentTest
 * @tc.desc: test concurrent update operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, UpdateConcurrentTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 10; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    auto updater = [](TestTaskNew &) {
        return std::pair<bool, TimeNew> { false, TimeNew() };
    };
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i, updater, &counter]() {
            auto retUpdate = priorityqueueNew_.Update(i + 1, updater);
            if (retUpdate) {
                counter++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(counter, 10);
}

/**
 * @tc.name: RemoveConcurrentTest
 * @tc.desc: test concurrent remove operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, RemoveConcurrentTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 12; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    std::vector<std::thread> threads;
    std::atomic<int> counter = 0;
    for (int i = 0; i < 12; ++i) {
        threads.emplace_back([this, i, &counter]() {
            auto retRemove = priorityqueueNew_.Remove(i + 1, false);
            if (retRemove) {
                counter++;
            }
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(counter, 12);
}

/**
 * @tc.name: FinishConcurrentTest
 * @tc.desc: test concurrent finish operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FinishConcurrentTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 8; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL_NEW);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([this, i]() {
            priorityqueueNew_.Finish(i + 1);
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 8u);
}

/**
 * @tc.name: CleanConcurrentTest
 * @tc.desc: test concurrent clean operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, CleanConcurrentTest, TestSize.Level1)
{
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this]() {
            priorityqueueNew_.Clean();
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: StressTest
 * @tc.desc: stress test all operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, StressTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < 20; ++i) {
            auto timely = std::chrono::seconds(0);
            testTask.taskId = static_cast<TaskIdNew>(round * 20 + i + 1);
            auto id = testTask.taskId;
            auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
            EXPECT_EQ(ret, true);
        }
        for (int i = 0; i < 20; ++i) {
            auto retPop = priorityqueueNew_.Pop();
            EXPECT_NE(retPop.taskId, INVALID_TASK_ID_NEW);
        }
    }
}

/**
 * @tc.name: PushPopFindUpdateTest
 * @tc.desc: test push pop find update combination
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, PushPopFindUpdateTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 20; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 10; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_EQ(retPop.taskId, i + 1);
    }
    for (int i = 11; i <= 20; ++i) {
        auto retFind = priorityqueueNew_.Find(i);
        EXPECT_EQ(retFind.taskId, i);
        auto updater = [](TestTaskNew &) {
            return std::pair<bool, TimeNew> { false, TimeNew() };
        };
        auto retUpdate = priorityqueueNew_.Update(i, updater);
        EXPECT_EQ(retUpdate, true);
    }
}

/**
 * @tc.name: MixedOperationsTest
 * @tc.desc: test mixed operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, MixedOperationsTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 15; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retFind = priorityqueueNew_.Find(1);
    EXPECT_EQ(retFind.taskId, 1);
    auto retPop = priorityqueueNew_.Pop();
    EXPECT_EQ(retPop.taskId, 1);
    auto retFind2 = priorityqueueNew_.Find(1);
    EXPECT_EQ(retFind2.taskId, INVALID_TASK_ID_NEW);
}

/**
 * @tc.name: SizeConsistencyTest
 * @tc.desc: test size consistency
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, SizeConsistencyTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 30; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
        EXPECT_EQ(priorityqueueNew_.Size(), static_cast<size_t>(i + 1));
    }
    for (int i = 0; i < 30; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_EQ(priorityqueueNew_.Size(), static_cast<size_t>(30 - i - 1));
    }
}

/**
 * @tc.name: EmptyAfterAllPopTest
 * @tc.desc: test empty after all pop
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, EmptyAfterAllPopTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int i = 0; i < 25; ++i) {
        auto timely = std::chrono::seconds(0);
        testTask.taskId = static_cast<TaskIdNew>(i + 1);
        auto id = testTask.taskId;
        auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 25; ++i) {
        auto retPop = priorityqueueNew_.Pop();
        EXPECT_NE(retPop.taskId, INVALID_TASK_ID_NEW);
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

/**
 * @tc.name: FinalComprehensiveTest
 * @tc.desc: comprehensive test with all operations
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueTestNew, FinalComprehensiveTest, TestSize.Level1)
{
    TestTaskNew testTask;
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < 10; ++i) {
            auto timely = std::chrono::seconds(0);
            testTask.taskId = static_cast<TaskIdNew>(round * 10 + i + 1);
            auto id = testTask.taskId;
            auto ret = priorityqueueNew_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
            EXPECT_EQ(ret, true);
        }
        for (int i = 0; i < 10; ++i) {
            auto retPop = priorityqueueNew_.Pop();
            EXPECT_EQ(retPop.taskId, round * 10 + i + 1);
        }
    }
    EXPECT_EQ(priorityqueueNew_.Size(), 0u);
}

} // namespace OHOS::Test