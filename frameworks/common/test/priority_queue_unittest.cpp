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
#include "priority_queue.h"
#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>

#include <set>
#include <shared_mutex>
#include <string>
#include <thread>

#include <queue>

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS;
using TaskId = uint64_t;
using Task = std::function<void()>;
using Time = std::chrono::steady_clock::time_point;
using Duration = std::chrono::steady_clock::duration;

static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
static constexpr TaskId INVALID_TASK_ID_TEST = static_cast<uint64_t>(0);
static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();
static constexpr uint32_t SHORT_INTERVAL_TIME = 100; // ms

class PriorityQueueUnittest : public testing::Test {
public:
    struct TestTask {
        Duration interval = INVALID_INTERVAL;
        uint64_t times = UNLIMITED_TIMES;
        TaskId taskId = INVALID_TASK_ID_TEST;

        std::function<void()> exec = []() {};
        TestTask() = default;

        bool Valid() const
        {
            return taskId != INVALID_TASK_ID_TEST;
        }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static PriorityQueue<PriorityQueueUnittest::TestTask, Time, TaskId> priorityqueue;
    static PriorityQueue<PriorityQueueUnittest::TestTask, Time, TaskId>::PQMatrix pqMatrix;
};
using TestTask = PriorityQueueUnittest::TestTask;
PriorityQueue<TestTask, Time, TaskId> PriorityQueueUnittest::priorityqueue =
    PriorityQueue<TestTask, Time, TaskId>(TestTask());
PriorityQueue<TestTask, Time, TaskId>::PQMatrix PriorityQueueUnittest::pqMatrix =
    PriorityQueue<TestTask, Time, TaskId>::PQMatrix(TestTask(), INVALID_TASK_ID_TEST);

void PriorityQueueUnittest::SetUpTestCase(void) { }

void PriorityQueueUnittest::TearDownTestCase(void) { }

void PriorityQueueUnittest::SetUp(void) { }

void PriorityQueueUnittest::TearDown(void)
{
    priorityqueue.Clean();
}

/**
 * @tc.name: PQMatrix_001
 * @tc.desc: test the PQMatrix(_Tsk task, _Tid id) function.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, PQMatrix_001Test, TestSize.Level1)
{
    TestTask utestTask;
    auto id = utestTask.taskId;
    ASSERT_EQ(pqMatrix.id_, id);
}

/**
 * @tc.name: PushPopSize_001
 * @tc.desc: Invalid test task.
 * @tc.type: FUNC
 */HWTEST_F(PriorityQueueUnittest, PushPopSize_001Test, TestSize.Level1)
{
    TestTask utestTask;
    auto id = utestTask.taskId;
    auto taskTime = std::chrono::seconds(0);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + taskTime);
    ASSERT_EQ(result, false);

    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
    auto testPop = priorityqueue.Pop();
    ASSERT_EQ(testPop.taskId, INVALID_TASK_ID_TEST);
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: PushPopSize_002
 * @tc.desc: Testing a single task.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, PushPopSize_002Test, TestSize.Level1)
{
    TestTask utestTask;
    auto id = ++(utestTask.taskId);
    auto taskTime = std::chrono::seconds(0);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + taskTime);
    ASSERT_EQ(result, true);

    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 1u);
    auto testPop = priorityqueue.Pop();
    ASSERT_EQ(testPop.taskId, id);
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: PushPopSize_003
 * @tc.desc: Testing multiple tasks.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, PushPopSize_003Test, TestSize.Level1)
{
    TestTask utestTask;
    for (int i = 0; i < 10; ++i) {
        auto taskTime = std::chrono::seconds(0);
        auto id = ++(utestTask.taskId);
        auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + taskTime);
        ASSERT_EQ(result, true);
    }

    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 10u);
    auto testPop = priorityqueue.Pop();
    ASSERT_EQ(testPop.taskId, 1);
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 9u);
}

/**
 * @tc.name: PushPopSize_004
 * @tc.desc: Test the delayTime task.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, PushPopSize_004Test, TestSize.Level1)
{
    TestTask utestTask;
    utestTask.times = 1;
    for (int i = 0; i < 5; ++i) {
        auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
        auto id = ++(utestTask.taskId);
        auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
        ASSERT_EQ(result, true);
    }

    for (int i = 0; i < 5; ++i) {
        auto taskTime = std::chrono::seconds(0);
        auto id = ++(utestTask.taskId);
        auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + taskTime);
        ASSERT_EQ(result, true);
    }

    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 10u);
    for (int i = 0; i < 5; ++i) {
        auto testPop = priorityqueue.Pop();
        ASSERT_EQ(testPop.taskId, i + 6);
    }

    for (int i = 0; i < 5; ++i) {
        auto testPop = priorityqueue.Pop();
        ASSERT_EQ(testPop.taskId, i + 1);
    }
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: PushPopSize_005
 * @tc.desc: Test the delayTime task.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, PushPopSize_005Test, TestSize.Level1)
{
    TestTask utestTask;
    utestTask.times = 1;
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    auto id = ++(utestTask.taskId);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 1u);
    auto dalayTimeA = std::chrono::steady_clock::now();
    auto testPop = priorityqueue.Pop();
    ASSERT_EQ(testPop.taskId, id);

    auto dalayTimeB = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(dalayTimeB - dalayTimeA).count();
    auto dalayTimems = std::chrono::duration_cast<std::chrono::milliseconds>(delayTime).count();
    EXPECT_LT(diff, dalayTimems * 1.5);
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: Find_001
 * @tc.desc: Invalid test task.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, Find_001Test, TestSize.Level1)
{
    TestTask utestTask;
    auto id = utestTask.taskId;
    auto taskTime = std::chrono::seconds(0);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + taskTime);
    ASSERT_EQ(result, false);
    auto retFind = priorityqueue.Find(id);
    ASSERT_EQ(retFind.taskId, INVALID_TASK_ID_TEST);
}

/**
 * @tc.name: Find_002
 * @tc.desc: test the priority_queue _Tsk Find(_Tid id) function.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, Find_002Test, TestSize.Level1)
{
    TestTask utestTask;
    for (int i = 0; i < 10; ++i) {
        auto taskTime = std::chrono::seconds(0);
        auto id = ++(utestTask.taskId);
        auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + taskTime);
        ASSERT_EQ(result, true);
    }

    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 10u);
    auto retFind = priorityqueue.Find(5);
    ASSERT_EQ(retFind.taskId, 5);
    retFind = priorityqueue.Find(20);
    ASSERT_EQ(retFind.taskId, INVALID_TASK_ID_TEST);
}

/**
 * @tc.name: Update_001
 * @tc.desc: Invalid test task.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, Update_001Test, TestSize.Level1)
{
    auto updateTest = [](TestTask &) {
        return std::pair { false, Time() };
    };
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    TestTask utestTask;
    utestTask.times = 3;
    auto id = utestTask.taskId;
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, false);
    auto retUpdate = priorityqueue.Update(id, updateTest);
    ASSERT_EQ(retUpdate, false);
}

/**
 * @tc.name: Update_002
 * @tc.desc: Test normal tasks.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, Update_002Test, TestSize.Level1)
{
    auto updateTest = [](TestTask &) {
        return std::pair { false, Time() };
    };
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    TestTask utestTask;
    utestTask.times = 3;
    auto id = ++(utestTask.taskId);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto retUpdate = priorityqueue.Update(id, updateTest);
    ASSERT_EQ(retUpdate, true);
}

/**
 * @tc.name: Update_003
 * @tc.desc: Test the running tasks.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, Update_003Test, TestSize.Level1)
{
    auto updateTest = [](TestTask &) {
        return std::pair { false, Time() };
    };
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    TestTask utestTask;
    utestTask.times = 3;
    auto id = ++(utestTask.taskId);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto testPop = priorityqueue.Pop();
    auto retUpdate = priorityqueue.Update(testPop.taskId, updateTest);
    ASSERT_EQ(retUpdate, false);
}

/**
 * @tc.name: Update_004
 * @tc.desc: Test the running tasks.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, Update_004Test, TestSize.Level1)
{
    auto updateTest = [](TestTask &) {
        return std::pair { true, Time() };
    };
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    TestTask utestTask;
    utestTask.times = 3;
    auto id = ++(utestTask.taskId);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto testPop = priorityqueue.Pop();
    auto retUpdate = priorityqueue.Update(testPop.taskId, updateTest);
    ASSERT_EQ(retUpdate, true);
}

/**
 * @tc.name: Update_005
 * @tc.desc: Test the running and finish tasks.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, Update_005Test, TestSize.Level1)
{
    auto updateTest = [](TestTask &) {
        return std::pair { false, Time() };
    };
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    TestTask utestTask;
    utestTask.times = 3;
    auto id = ++(utestTask.taskId);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto testPop = priorityqueue.Pop();
    priorityqueue.Finish(id);
    auto retUpdate = priorityqueue.Update(testPop.taskId, updateTest);
    ASSERT_EQ(retUpdate, false);
}

/**
 * @tc.name: Update_006
 * @tc.desc: Test the running and finish tasks.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PriorityQueueUnittest, Update_006Test, TestSize.Level1)
{
    auto updateTest = [](TestTask &) {
        return std::pair { true, Time() };
    };
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    TestTask utestTask;
    utestTask.times = 3;
    auto id = ++(utestTask.taskId);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto testPop = priorityqueue.Pop();
    priorityqueue.Finish(id);
    auto retUpdate = priorityqueue.Update(testPop.taskId, updateTest);
    ASSERT_EQ(retUpdate, false);
}

/**
 * @tc.name: Remove_001
 * @tc.desc: Invalid test task.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PriorityQueueUnittest, Remove_001Test, TestSize.Level1)
{
    TestTask utestTask;
    auto id = utestTask.taskId;
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, false);
    auto remove = priorityqueue.Remove(id, false);
    ASSERT_EQ(remove, false);
    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: Remove_002
 * @tc.desc: Single and don't wait test task.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PriorityQueueUnittest, Remove_002Test, TestSize.Level1)
{
    TestTask utestTask;
    auto id = ++(utestTask.taskId);
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 1u);
    auto remove = priorityqueue.Remove(id, false);
    ASSERT_EQ(remove, true);
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: Remove_003
 * @tc.desc: Single and wait test task.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PriorityQueueUnittest, Remove_003Test, TestSize.Level1)
{
    TestTask utestTask;
    auto id = ++(utestTask.taskId);
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 1u);
    priorityqueue.Finish(id);
    auto remove = priorityqueue.Remove(id, true);
    ASSERT_EQ(remove, true);
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: Clean_001
 * @tc.desc: Testing a single task.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PriorityQueueUnittest, Clean_001Test, TestSize.Level1)
{
    TestTask utestTask;
    auto taskTime = std::chrono::seconds(0);
    auto id = ++(utestTask.taskId);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + taskTime);
    ASSERT_EQ(result, true);
    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 1u);
    priorityqueue.Clean();
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: Clean_002
 * @tc.desc: Testing multiple tasks.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PriorityQueueUnittest, Clean_002Test, TestSize.Level1)
{
    TestTask utestTask;
    for (int i = 0; i < 10; ++i) {
        auto taskTime = std::chrono::seconds(0);
        auto id = ++(utestTask.taskId);
        auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + taskTime);
        ASSERT_EQ(result, true);
    }
    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 10u);
    priorityqueue.Clean();
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}

/**
 * @tc.name: Finish_001
 * @tc.desc: test the priority_queue void Finish(_Tid id) function.
 * @tc.type: FUNC
 */
HWTEST_F(PriorityQueueUnittest, FinishTest, TestSize.Level1)
{
    TestTask utestTask;
    auto id = ++(utestTask.taskId);
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    auto result = priorityqueue.Push(utestTask, id, std::chrono::steady_clock::now() + delayTime);
    ASSERT_EQ(result, true);
    auto queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 1u);
    priorityqueue.Finish(id);
    auto remove = priorityqueue.Remove(id, true);
    ASSERT_EQ(remove, true);
    queueSize = priorityqueue.Size();
    ASSERT_EQ(queueSize, 0u);
}
} // namespace OHOS::Test
