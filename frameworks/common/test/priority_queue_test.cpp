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

using namespace testing::ext;
using namespace OHOS;
namespace OHOS::Test {
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
* @tc.desc: test the priority_queue _Tsk Pop() function.
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
* @tc.name: Pop_001
* @tc.desc: test the priority_queue _Tsk Pop() function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Pop_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
    priorityqueue_.Pop();
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Pop_002
* @tc.desc: test the priority_queue _Tsk Pop() function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Pop_002, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    EXPECT_EQ(retPop.taskId, id);
}

/**
* @tc.name: Pop_003
* @tc.desc: test the priority_queue _Tsk Pop() function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Pop_003, TestSize.Level1)
{
    TestTask testTask;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
    auto retPop = priorityqueue_.Pop();
    EXPECT_EQ(retPop.taskId, id);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Push_001
* @tc.desc: test the priority_queue bool Push(_Tsk tsk, _Tid id, _Tme tme) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Push_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
}

/**
* @tc.name: Push_002
* @tc.desc: test the priority_queue bool Push(_Tsk tsk, _Tid id, _Tme tme) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Push_002, TestSize.Level1)
{
    TestTask testTask;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: Push_003
* @tc.desc: test the priority_queue bool Push(_Tsk tsk, _Tid id, _Tme tme) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Push_003, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    id = ++(testTask.taskId);
    delay = std::chrono::milliseconds(SHORT_INTERVAL * 2);
    ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
}

/**
* @tc.name: Size_001
* @tc.desc: test the priority_queue size_t Size() function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Size_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Size_002
* @tc.desc: test the priority_queue size_t Size() function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Size_002, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
}

/**
* @tc.name: Size_003
* @tc.desc: test the priority_queue size_t Size() function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Size_003, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    id = ++(testTask.taskId);
    delay = std::chrono::milliseconds(SHORT_INTERVAL * 2);
    ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 2u);
}

/**
* @tc.name: Find_001
* @tc.desc: test the priority_queue _Tsk Find(_Tid id) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Find_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = testTask.taskId;
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
    auto retFind = priorityqueue_.Find(id);
    EXPECT_EQ(retFind.taskId, id);
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
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retFind = priorityqueue_.Find(id);
    EXPECT_EQ(retFind.taskId, id);
}

/**
* @tc.name: Update_001
* @tc.desc: test the priority_queue bool Update(_Tid id, TskUpdater updater) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_001, TestSize.Level1)
{
    auto updater_ = [](TestTask &) { return std::pair{false, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = testTask.taskId;
    testTask.interval = delay * 2;
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, false);
    auto retUpdate = priorityqueue_.Update(id, updater_);
    EXPECT_EQ(retUpdate, false);
}

/**
* @tc.name: Update_002
* @tc.desc: test the priority_queue bool Update(_Tid id, TskUpdater updater) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_002, TestSize.Level1)
{
    auto updater_ = [](TestTask &) { return std::pair{false, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    testTask.interval = delay * 2;
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retUpdate = priorityqueue_.Update(id, updater_);
    EXPECT_EQ(retUpdate, true);
}

/**
* @tc.name: Update_003
* @tc.desc: test the priority_queue bool Update(_Tid id, TskUpdater updater) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_003, TestSize.Level1)
{
    auto updater_ = [](TestTask &) { return std::pair{false, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    testTask.interval = delay * 2;
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    auto retUpdate = priorityqueue_.Update(retPop.taskId, updater_);
    EXPECT_EQ(retUpdate, false);
}

/**
* @tc.name: Update_004
* @tc.desc: test the priority_queue bool Update(_Tid id, TskUpdater updater) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_004, TestSize.Level1)
{
    auto updater_ = [](TestTask &) { return std::pair{true, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    testTask.interval = delay * 2;
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    auto retUpdate = priorityqueue_.Update(retPop.taskId, updater_);
    EXPECT_EQ(retUpdate, true);
}

/**
* @tc.name: Update_005
* @tc.desc: test the priority_queue bool Update(_Tid id, TskUpdater updater) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_005, TestSize.Level1)
{
    auto updater_ = [](TestTask &) { return std::pair{false, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    testTask.interval = delay * 2;
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    priorityqueue_.Finish(id);
    auto retUpdate = priorityqueue_.Update(retPop.taskId, updater_);
    EXPECT_EQ(retUpdate, false);
}

/**
* @tc.name: Update_006
* @tc.desc: test the priority_queue bool Update(_Tid id, TskUpdater updater) function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Update_006, TestSize.Level1)
{
    auto updater_ = [](TestTask &) { return std::pair{true, Time()};};
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    TestTask testTask;
    testTask.times = 3;
    auto id = ++(testTask.taskId);
    testTask.interval = delay * 2;
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retPop = priorityqueue_.Pop();
    priorityqueue_.Finish(id);
    auto retUpdate = priorityqueue_.Update(retPop.taskId, updater_);
    EXPECT_EQ(retUpdate, false);
}

/**
* @tc.name: Remove_001
* @tc.desc: test the priority_queue bool Remove(_Tid id, bool wait) function.
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
* @tc.desc: test the priority_queue bool Remove(_Tid id, bool wait) function.
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
* @tc.desc: test the priority_queue bool Remove(_Tid id, bool wait) function.
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
* @tc.desc: test the priority_queue void Clean() function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Clean_001, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 1u);
    priorityqueue_.Clean();
    retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 0u);
}

/**
* @tc.name: Clean_002
* @tc.desc: test the priority_queue void Clean() function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
*/
HWTEST_F(PriorityQueueTest, Clean_002, TestSize.Level1)
{
    TestTask testTask;
    auto id = ++(testTask.taskId);
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL);
    auto ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    id = ++(testTask.taskId);
    delay = std::chrono::milliseconds(SHORT_INTERVAL * 2);
    ret = priorityqueue_.Push(testTask, id, std::chrono::steady_clock::now() + delay);
    EXPECT_EQ(ret, true);
    auto retSize = priorityqueue_.Size();
    EXPECT_EQ(retSize, 2u);
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
