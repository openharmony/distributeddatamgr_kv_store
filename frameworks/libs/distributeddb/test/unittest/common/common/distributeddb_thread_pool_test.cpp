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
#include "distributeddb_tools_unit_test.h"
#include "mock_thread_pool.h"

#include <gtest/gtest.h>

#include "log_print.h"
#include "runtime_context.h"

using namespace testing::ext;
using namespace testing;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;

namespace {
const int MAX_TIMER_COUNT = 5;
const int ONE_HUNDRED_MILLISECONDS = 100;
const int ONE_SECOND = 1000;

class DistributedDBThreadPoolTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    std::shared_ptr<MockThreadPool> threadPoolPtr_ = nullptr;
    class TimerWatcher {
    public:
        TimerWatcher() = default;

        ~TimerWatcher()
        {
            SafeExit();
        }

        void AddCount()
        {
            std::lock_guard<std::mutex> autoLock(countLock_);
            count_++;
        }

        void DecCount()
        {
            {
                std::lock_guard<std::mutex> autoLock(countLock_);
                count_--;
            }
            countCv_.notify_one();
        }

        void SafeExit()
        {
            std::unique_lock<std::mutex> uniqueLock(countLock_);
            countCv_.wait(uniqueLock, [this]() {
                return count_ <= 0;
            });
        }
    private:
        std::mutex countLock_;
        int count_ = 0;
        std::condition_variable countCv_;
    };
};

void DistributedDBThreadPoolTest::SetUpTestCase()
{
}

void DistributedDBThreadPoolTest::TearDownTestCase()
{
}

void DistributedDBThreadPoolTest::SetUp()
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    if (threadPoolPtr_ == nullptr) {
        threadPoolPtr_ = std::make_shared<MockThreadPool>();
    }
    RuntimeContext::GetInstance()->SetThreadPool(std::dynamic_pointer_cast<IThreadPool>(threadPoolPtr_));
}

void DistributedDBThreadPoolTest::TearDown()
{
    RuntimeContext::GetInstance()->SetThreadPool(nullptr);
    threadPoolPtr_ = nullptr;
}

void CallScheduleTaskOnce()
{
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([]() {
        LOGI("Task running");
    });
    EXPECT_EQ(errCode, E_OK);
}

void MockSchedule(const std::shared_ptr<MockThreadPool> &threadPoolPtr,
    const std::shared_ptr<DistributedDBThreadPoolTest::TimerWatcher> &watcher, std::atomic<TaskId> &taskId)
{
    ASSERT_NE(watcher, nullptr);
    EXPECT_CALL(*threadPoolPtr, Execute(_, _)).
        WillRepeatedly([watcher, &taskId](const Task &task, Duration time) {
        watcher->AddCount();
        std::thread workingThread = std::thread([task, time, watcher]() {
            std::this_thread::sleep_for(time);
            task();
            watcher->DecCount();
        });
        workingThread.detach();
        TaskId currentId = taskId++;
        return currentId;
    });
}

void MockRemove(const std::shared_ptr<MockThreadPool> &threadPoolPtr, bool removeRes, int &removeCount)
{
    EXPECT_CALL(*threadPoolPtr, Remove).WillRepeatedly([removeRes, &removeCount](const TaskId &taskId, bool) {
        LOGI("Call remove task %" PRIu64, taskId);
        removeCount++;
        return removeRes;
    });
}

void SetTimer(int &timerCount, TimerId &timer, int &finalizeCount, int timeOut = ONE_HUNDRED_MILLISECONDS)
{
    int errCode = RuntimeContext::GetInstance()->SetTimer(timeOut, [&timerCount](TimerId timerId) {
        LOGI("Timer %" PRIu64 " running", timerId);
        timerCount++;
        if (timerCount < MAX_TIMER_COUNT) { // max timer count is 5
            return E_OK;
        }
        return -E_END_TIMER;
    }, [&finalizeCount, timer]() {
        finalizeCount++;
        LOGI("Timer %" PRIu64" finalize", timer + 1);
    }, timer);
    EXPECT_EQ(errCode, E_OK);
}

void SetTimer(int &timerCount, TimerId &timer)
{
    int errCode = RuntimeContext::GetInstance()->SetTimer(ONE_HUNDRED_MILLISECONDS, [&timerCount](TimerId timerId) {
        LOGI("Timer %" PRIu64 " running", timerId);
        timerCount++;
        if (timerCount < MAX_TIMER_COUNT) { // max timer count is 5
            return E_OK;
        }
        return -E_END_TIMER;
    }, nullptr, timer);
    EXPECT_EQ(errCode, E_OK);
}

/**
 * @tc.name: ScheduleTask001
 * @tc.desc: Test schedule task by thread pool
 * @tc.type: FUNC
 * @tc.require: AR000I0KU9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBThreadPoolTest, ScheduleTask001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. set thread pool and schedule task
     * @tc.expected: step1. thread pool execute task count is once.
     */
    ASSERT_NE(threadPoolPtr_, nullptr);
    int callCount = 0;
    std::thread workingThread;
    EXPECT_CALL(*threadPoolPtr_, Execute(_)).WillRepeatedly([&callCount, &workingThread](const Task &task) {
        callCount++;
        workingThread = std::thread([task]() {
            task();
        });
        return 1u; // task id is 1
    });
    ASSERT_NO_FATAL_FAILURE(CallScheduleTaskOnce());
    if (workingThread.joinable()) {
        workingThread.join();
    }
    EXPECT_EQ(callCount, 1);
    /**
     * @tc.steps: step2. reset thread pool and schedule task
     * @tc.expected: step2. thread pool execute task count is once.
     */
    RuntimeContext::GetInstance()->SetThreadPool(nullptr);
    callCount = 0;
    ASSERT_NO_FATAL_FAILURE(CallScheduleTaskOnce());
    if (workingThread.joinable()) {
        workingThread.join();
    }
    EXPECT_EQ(callCount, 0);
}

/**
 * @tc.name: SetTimer001
 * @tc.desc: Test set timer by thread pool
 * @tc.type: FUNC
 * @tc.require: AR000I0KU9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBThreadPoolTest, SetTimer001, TestSize.Level1)
{
    ASSERT_NE(threadPoolPtr_, nullptr);
    std::shared_ptr<TimerWatcher> watcher = std::make_shared<TimerWatcher>();
    std::atomic<TaskId> currentId = 1;
    ASSERT_NO_FATAL_FAILURE(MockSchedule(threadPoolPtr_, watcher, currentId));
    /**
     * @tc.steps: step1. set timer and record timer call count
     * @tc.expected: step1. call count is MAX_TIMER_COUNT and finalize once.
     */
    int timerCount = 0;
    TimerId timer = 0;
    int finalizeCount = 0;
    ASSERT_NO_FATAL_FAILURE(SetTimer(timerCount, timer, finalizeCount));
    /**
     * @tc.steps: step2. mock modify timer
     * @tc.expected: step2. can call modify timer when timer is runnning.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // sleep 150ms
    EXPECT_CALL(*threadPoolPtr_, Reset).WillOnce([&currentId](const TaskId &id, Duration modifyTime) {
        LOGI("call modify timer task is %" PRIu64, id);
        EXPECT_EQ(id, currentId - 1);
        Duration duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::milliseconds(ONE_HUNDRED_MILLISECONDS));
        EXPECT_EQ(modifyTime, duration);
        return id;
    });
    EXPECT_EQ(RuntimeContext::GetInstance()->ModifyTimer(timer, ONE_HUNDRED_MILLISECONDS), E_OK);
    /**
     * @tc.steps: step3. wait timer finished
     */
    watcher->SafeExit();
    EXPECT_EQ(timerCount, MAX_TIMER_COUNT);
    EXPECT_EQ(finalizeCount, 1);
}

/**
 * @tc.name: SetTimer002
 * @tc.desc: Test repeat setting timer
 * @tc.type: FUNC
 * @tc.require: AR000I0KU9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBThreadPoolTest, SetTimer002, TestSize.Level1)
{
    ASSERT_NE(threadPoolPtr_, nullptr);
    std::shared_ptr<TimerWatcher> watcher = std::make_shared<TimerWatcher>();
    std::atomic<TaskId> currentId = 1;
    ASSERT_NO_FATAL_FAILURE(MockSchedule(threadPoolPtr_, watcher, currentId));
    /**
     * @tc.steps: step1. set timer and record timer call count
     * @tc.expected: step1. call count is MAX_TIMER_COUNT and finalize once.
     */
    int timerCountArray[MAX_TIMER_COUNT] = {};
    int finalizeCountArray[MAX_TIMER_COUNT] = {};
    for (int i = 0; i < MAX_TIMER_COUNT; ++i) {
        TimerId timer;
        SetTimer(timerCountArray[i], timer, finalizeCountArray[i]);
    }
    /**
     * @tc.steps: step2. wait timer finished
     */
    watcher->SafeExit();
    for (int i = 0; i < MAX_TIMER_COUNT; ++i) {
        EXPECT_EQ(timerCountArray[i], MAX_TIMER_COUNT);
        EXPECT_EQ(finalizeCountArray[i], 1);
    }
}

/**
 * @tc.name: SetTimer003
 * @tc.desc: Test set timer and finalize is null
 * @tc.type: FUNC
 * @tc.require: AR000I0KU9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBThreadPoolTest, SetTimer003, TestSize.Level1)
{
    ASSERT_NE(threadPoolPtr_, nullptr);
    std::shared_ptr<TimerWatcher> watcher = std::make_shared<TimerWatcher>();
    std::atomic<TaskId> currentId = 1;
    ASSERT_NO_FATAL_FAILURE(MockSchedule(threadPoolPtr_, watcher, currentId));
    /**
     * @tc.steps: step1. set timer and record timer call count
     * @tc.expected: step1. call count is MAX_TIMER_COUNT and finalize once.
     */
    int timerCount = 0;
    TimerId timer = 0;
    ASSERT_NO_FATAL_FAILURE(SetTimer(timerCount, timer));
    /**
     * @tc.steps: step3. wait timer finished
     */
    watcher->SafeExit();
    EXPECT_EQ(timerCount, MAX_TIMER_COUNT);
}

/**
 * @tc.name: SetTimer004
 * @tc.desc: Test remove timer function
 * @tc.type: FUNC
 * @tc.require: AR000I0KU9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBThreadPoolTest, SetTimer004, TestSize.Level2)
{
    ASSERT_NE(threadPoolPtr_, nullptr);
    std::shared_ptr<TimerWatcher> watcher = std::make_shared<TimerWatcher>();
    std::atomic<TaskId> currentId = 1;
    ASSERT_NO_FATAL_FAILURE(MockSchedule(threadPoolPtr_, watcher, currentId));
    int removeCount = 0;
    MockRemove(threadPoolPtr_, false, removeCount);
    /**
     * @tc.steps: step1. set timer and record timer call count
     * @tc.expected: step1. call count is MAX_TIMER_COUNT and finalize once.
     */
    int timerCount = 0;
    TimerId timer = 0;
    int finalizeCount = 0;
    ASSERT_NO_FATAL_FAILURE(SetTimer(timerCount, timer, finalizeCount, ONE_SECOND));
    /**
     * @tc.steps: step2. remove timer
     * @tc.expected: step2. call count is zero when timerId no exist.
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(ONE_HUNDRED_MILLISECONDS));
    RuntimeContext::GetInstance()->RemoveTimer(timer - 1, true);
    EXPECT_EQ(removeCount, 0);
    MockRemove(threadPoolPtr_, true, removeCount);
    RuntimeContext::GetInstance()->RemoveTimer(timer, true);
    EXPECT_EQ(removeCount, 1);
    /**
     * @tc.steps: step3. wait timer finished
     */
    watcher->SafeExit();
    EXPECT_EQ(timerCount, 0);
}

/**
 * @tc.name: SetTimer005
 * @tc.desc: Test repeat remove timer
 * @tc.type: FUNC
 * @tc.require: AR000I0KU9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBThreadPoolTest, SetTimer005, TestSize.Level1)
{
    ASSERT_NE(threadPoolPtr_, nullptr);
    std::shared_ptr<TimerWatcher> watcher = std::make_shared<TimerWatcher>();
    std::atomic<TaskId> currentId = 1;
    ASSERT_NO_FATAL_FAILURE(MockSchedule(threadPoolPtr_, watcher, currentId));
    int removeCount = 0;
    MockRemove(threadPoolPtr_, true, removeCount);
    /**
     * @tc.steps: step1. set timer and record timer call count
     * @tc.expected: step1. call count is MAX_TIMER_COUNT and finalize once.
     */
    int timerCountArray[MAX_TIMER_COUNT] = {};
    int finalizeCountArray[MAX_TIMER_COUNT] = {};
    TimerId timerIdArray[MAX_TIMER_COUNT] = {};
    for (int i = 0; i < MAX_TIMER_COUNT; ++i) {
        SetTimer(timerCountArray[i], timerIdArray[i], finalizeCountArray[i], ONE_SECOND);
    }
    /**
     * @tc.steps: step2. remove all timer
     */
    int sleepTime = MAX_TIMER_COUNT * ONE_SECOND - 2 * ONE_HUNDRED_MILLISECONDS;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    for (const auto &timerId : timerIdArray) {
        RuntimeContext::GetInstance()->RemoveTimer(timerId, true);
    }
    /**
     * @tc.steps: step3. wait timer finished
     */
    watcher->SafeExit();
    EXPECT_LE(removeCount, MAX_TIMER_COUNT);
    for (int i = 0; i < MAX_TIMER_COUNT; ++i) {
        EXPECT_LE(timerCountArray[i], MAX_TIMER_COUNT);
        EXPECT_EQ(finalizeCountArray[i], 1);
    }
}

/**
 * @tc.name: SetTimer006
 * @tc.desc: Test repeat remove timer when time action finished
 * @tc.type: FUNC
 * @tc.require: AR000I0KU9
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBThreadPoolTest, SetTimer006, TestSize.Level1)
{
    ASSERT_NE(threadPoolPtr_, nullptr);
    std::atomic<TaskId> currentId = 1;
    std::shared_ptr<TimerWatcher> watcher = std::make_shared<TimerWatcher>();
    ASSERT_NO_FATAL_FAILURE(MockSchedule(threadPoolPtr_, watcher, currentId));
    int removeCount = 0;
    MockRemove(threadPoolPtr_, false, removeCount);

    std::mutex dataMutex;
    std::set<TimerId> removeSet;
    std::set<TimerId> checkSet;
    std::set<TimerId> timerSet;
    for (int i = 0; i < 10; ++i) { // 10 timer
        TimerId id;
        int errCode = RuntimeContext::GetInstance()->SetTimer(1, [&dataMutex, &removeSet, &checkSet](TimerId timerId) {
            LOGI("Timer %" PRIu64 " running", timerId);
            std::lock_guard<std::mutex> autoLock(dataMutex);
            if (removeSet.find(timerId) != removeSet.end()) {
                EXPECT_TRUE(checkSet.find(timerId) == checkSet.end());
                checkSet.insert(timerId);
                return -E_END_TIMER;
            }
            return E_OK;
        }, nullptr, id);
        EXPECT_EQ(errCode, E_OK);
        timerSet.insert(id);
    }
    for (const auto &timer: timerSet) {
        RuntimeContext::GetInstance()->RemoveTimer(timer);
        LOGI("Timer %" PRIu64 " remove", timer);
        std::lock_guard<std::mutex> autoLock(dataMutex);
        removeSet.insert(timer);
    }
    watcher->SafeExit();
}

/**
 * @tc.name: TaskPool001
 * @tc.desc: Test TaskPool schedule task
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBThreadPoolTest, TaskPool001, TestSize.Level1)
{
    RuntimeContext::GetInstance()->SetThreadPool(nullptr);
    std::mutex dataMutex;
    std::condition_variable cv;
    int finishedTaskCount = 0;
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([&finishedTaskCount, &dataMutex, &cv]() {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s
        LOGD("exec task ok");
        {
            std::lock_guard<std::mutex> autoLock(dataMutex);
            finishedTaskCount++;
        }
        cv.notify_one();
    });
    EXPECT_EQ(errCode, E_OK);
    constexpr int execTaskCount = 2;
    for (int i = 0; i < execTaskCount; ++i) {
        errCode = RuntimeContext::GetInstance()->ScheduleQueuedTask("TaskPool",
            [i, &finishedTaskCount, &dataMutex, &cv]() {
            LOGD("exec task %d", i);
            {
                std::lock_guard<std::mutex> autoLock(dataMutex);
                finishedTaskCount++;
            }
            cv.notify_one();
        });
        EXPECT_EQ(errCode, E_OK);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // sleep 1s
    }
    {
        std::unique_lock<std::mutex> uniqueLock(dataMutex);
        LOGD("begin wait all task finished");
        cv.wait(uniqueLock, [&finishedTaskCount]() {
            return finishedTaskCount == execTaskCount + 1;
        });
        LOGD("end wait all task finished");
    }
    RuntimeContext::GetInstance()->StopTaskPool();
}
}