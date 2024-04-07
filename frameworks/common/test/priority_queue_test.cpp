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

#include <gtest/gtest.h>
#include <functional>
#include <queue>
#include <chrono>
#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <set>
#include <shared_mutex>
#include "priority_queue.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS;
using TaskId = uint64_t;
using Task = std::function<void()>;
using Duration = std::chrono::steady_clock::duration;
using Time = std::chrono::steady_clock::time_point;
static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();
static constexpr TaskId INVALID_TASK_ID = static_cast<uint64_t>(0l);
static constexpr uint32_t SHORT_INTERVAL = 100; // ms
class PriorityQueueTest : public testing::Test {
public:
    struct TestTask {
        std::function<void()> exec = []() {};
        Duration interval = INVALID_INTERVAL;
        uint64_t times = UNLIMITED_TIMES;
        TaskId taskId = INVALID_TASK_ID;
        TestTask() = default;

        bool Valid() const
        {
            return taskId != INVALID_TASK_ID;
        }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    static PriorityQueue<PriorityQueueTest::TestTask, Time, TaskId> priorityqueue_;
    static PriorityQueue<PriorityQueueTest::TestTask, Time, TaskId>::PQMatrix pqMatrix;
};
using TestTask = PriorityQueueTest::TestTask;
PriorityQueue<TestTask, Time, TaskId> PriorityQueueTest::priorityqueue_ =
PriorityQueue<TestTask, Time, TaskId>(TestTask());
PriorityQueue<TestTask, Time, TaskId>::PQMatrix PriorityQueueTest::pqMatrix =
PriorityQueue<TestTask, Time, TaskId>::PQMatrix(TestTask(), INVALID_TASK_ID);

void PriorityQueueTest::SetUpTestCase(void)
{
}

void PriorityQueueTest::TearDownTestCase(void)
{
}

void PriorityQueueTest::SetUp(void)
{
}

void PriorityQueueTest::TearDown(void)
{
    priorityqueue_.Clean();
}

/**
* @tc.name: PQMatrix_001
* @tc.desc: test the PQMatrix(_Tsk task, _Tid id) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, PQMatrix_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = testTask.taskId;
    EXPECT_EQ(pqMatrix.id_, id);
}

/**
* @tc.name: PushPopSize_001
* @tc.desc: Invalid test task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, PushPopSize_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = testTask.taskId;
    auto timely = std::chrono::seconds(0);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
    EXPECT_EQ(ret, false);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
    auto retPop = priorityqueue_.Pop();
    EXPECT_EQ(retPop.taskId, INVALID_TASK_ID);
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: PushPopSize_002
* @tc.desc: Testing a single task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, PushPopSize_002, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto timely = std::chrono::seconds(0);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
    auto retPop = priorityqueue_.Pop();
    EXPECT_EQ(retPop.taskId, id);
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: PushPopSize_003
* @tc.desc: Testing multiple tasks.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, PushPopSize_003, TestSize.Level1)
{
    TestTask testTask;
    for (int i = 0; i < 10; ++i) {
        auto timely = std::chrono::seconds(0);
        auto id = ++(testTask.taskId);
        auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 10u);
    auto retPop = priorityqueue_.Pop();
    EXPECT_EQ(retPop.taskId, 1);
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 9u);
}

/**
* @tc.name: PushPopSize_004
* @tc.desc: Test the delay task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, PushPopSize_004, TestSize.Level1)
{
    TestTask testTask;
    testTask.times = 1;
    for (int i = 0; i < 5; ++i) {
        auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
        auto id = ++(testTask.taskId);
        auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
        EXPECT_EQ(ret, true);
    }
    for (int i = 0; i < 5; ++i) {
        auto timely = std::chrono::seconds(0);
        auto id = ++(testTask.taskId);
        auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 10u);
    for (int i = 0; i < 5; ++i) {
        auto retPop = priorityqueue_.Pop();
        EXPECT_EQ(retPop.taskId, i+6);
    }
    for (int i = 0; i < 5; ++i) {
        auto retPop = priorityqueue_.Pop();
        EXPECT_EQ(retPop.taskId, i+1);
    }
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: PushPopSize_005
* @tc.desc: Test the delay task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, PushPopSize_005, TestSize.Level1)
{
    TestTask testTask;
    testTask.times = 1;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto id = ++(testTask.taskId);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
    auto delayA = std::chrono::steady_clock::now();
    auto retPop = priorityqueue_.Pop();
    EXPECT_EQ(retPop.taskId, id);
    auto delayB = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(delayB - delayA).count();
    auto delayms = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
    EXPECT_LT(diff, delayms * 1.5);
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Find_001
* @tc.desc: Invalid test task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Find_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = testTask.taskId;
    auto timely = std::chrono::seconds(0);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
    EXPECT_EQ(ret, false);
    auto retFind = priorityqueue_.Find(id);
    EXPECT_EQ(retFind.taskId, INVALID_TASK_ID);
}

/**
* @tc.name: Find_002
* @tc.desc: test the priority_queue _Tsk Find(_Tid id) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Find_002, TestSize.Level1)
{
    TestTask testTask;
    for (int i = 0; i < 10; ++i) {
        auto timely = std::chrono::seconds(0);
        auto id = ++(testTask.taskId);
        auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 10u);
    auto retFind = priorityqueue_.Find(5);
    EXPECT_EQ(retFind.taskId, 5);
    retFind = priorityqueue_.Find(20);
    EXPECT_EQ(retFind.taskId, INVALID_TASK_ID);
}

/**
* @tc.name: Update_001
* @tc.desc: Invalid test task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_001, TestSize.Level1)
{
    auto updater = [](TestTask &) { return std::pair{false, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = testTask.taskId;
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
    auto retUpdate = priorityqueue_.Update(id, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
* @tc.name: Update_002
* @tc.desc: Test normal tasks.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_002, TestSize.Level1)
{
    auto updater = [](TestTask &) { return std::pair{false, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retUpdate = priorityqueue_.Update(id, updater);
    EXPECT_EQ(retUpdate, true);
}

/**
* @tc.name: Update_003
* @tc.desc: Test the running tasks.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_003, TestSize.Level1)
{
    auto updater = [](TestTask &) { return std::pair{false, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    auto retUpdate = priorityqueue_.Update(retPop.taskId, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
* @tc.name: Update_004
* @tc.desc: Test the running tasks.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_004, TestSize.Level1)
{
    auto updater = [](TestTask &) { return std::pair{true, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    auto retUpdate = priorityqueue_.Update(retPop.taskId, updater);
    EXPECT_EQ(retUpdate, true);
}

/**
* @tc.name: Update_005
* @tc.desc: Test the running and finish tasks.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_005, TestSize.Level1)
{
    auto updater = [](TestTask &) { return std::pair{false, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    priorityqueue_.Finish(id);
    auto retUpdate = priorityqueue_.Update(retPop.taskId, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
* @tc.name: Update_006
* @tc.desc: Test the running and finish tasks.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_006, TestSize.Level1)
{
    auto updater = [](TestTask &) { return std::pair{true, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    priorityqueue_.Finish(id);
    auto retUpdate = priorityqueue_.Update(retPop.taskId, updater);
    EXPECT_EQ(retUpdate, false);
}

/**
* @tc.name: Remove_001
* @tc.desc: Invalid test task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Remove_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
    auto retRemove = priorityqueue_.Remove(id, false);
    EXPECT_EQ(retRemove, false);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Remove_002
* @tc.desc: Single and don't wait test task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Remove_002, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
    auto retRemove = priorityqueue_.Remove(id, false);
    EXPECT_EQ(retRemove, true);
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Remove_003
* @tc.desc: Single and wait test task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Remove_003, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
    priorityqueue_.Finish(id);
    auto retRemove = priorityqueue_.Remove(id, true);
    EXPECT_EQ(retRemove, true);
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Clean_001
* @tc.desc: Testing a single task.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Clean_001, TestSize.Level1)
{
    TestTask testTask;
    auto timely = std::chrono::seconds(0);
    auto id = ++(testTask.taskId);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
    priorityqueue_.Clean();
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Clean_002
* @tc.desc: Testing multiple tasks.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Clean_002, TestSize.Level1)
{
    TestTask testTask;
    for (int i = 0; i < 10; ++i) {
        auto timely = std::chrono::seconds(0);
        auto id = ++(testTask.taskId);
        auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + timely);
        EXPECT_EQ(ret, true);
    }
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 10u);
    priorityqueue_.Clean();
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Finish_001
* @tc.desc: test the priority_queue void Finish(_Tid id) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Finish_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
    priorityqueue_.Finish(id); // Marking Finish
    auto retRemove = priorityqueue_.Remove(id, true);
    EXPECT_EQ(retRemove, true);
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}
} // namespace OHOS::Test
