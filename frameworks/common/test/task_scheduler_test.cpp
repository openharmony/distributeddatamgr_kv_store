/*
* Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "task_scheduler.h"

#include <gtest/gtest.h>

#include "block_data.h"
namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS;
using duration = std::chrono::steady_clock::duration;
class TaskSchedulerTest : public testing::Test {
public:
    static constexpr uint32_t SHORT_INTERVAL = 100; // ms
    static constexpr uint32_t LONG_INTERVAL = 1;    // s
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown() {}
};

/**
* @tc.name: At
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TaskSchedulerTest, At, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL);
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 11;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 11);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 12;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 12);
    ASSERT_NE(atTaskId1, atTaskId2);
}

/**
* @tc.name: Every
* @tc.desc:execute task periodically with duration
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TaskSchedulerTest, ExecuteDuration, TestSize.Level0)
{
    TaskScheduler taskScheduler("everyTest");
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, 0);
    int testData = 0;
    taskScheduler.Every(std::chrono::milliseconds(SHORT_INTERVAL), [blockData, &testData]() {
        testData++;
        blockData->SetValue(testData);
    });
    for (int i = 1; i < 10; ++i) {
        ASSERT_EQ(blockData->GetValue(), i);
        blockData->Clear(0);
    }
}

/**
* @tc.name: Reset
* @tc.desc: Reset before task execution and the task is tasks_.begin() or not
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TaskSchedulerTest, Reset1, TestSize.Level0)
{
    TaskScheduler taskScheduler("reset1Test");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL);
    auto atTaskId1 = taskScheduler.At(expiredTime, []() {});
    ASSERT_EQ(atTaskId1, 1);
    expiredTime += std::chrono::milliseconds(LONG_INTERVAL);
    auto atTaskId2 = taskScheduler.At(expiredTime, []() {});
    ASSERT_EQ(atTaskId2, 2);

    auto resetTaskId1 = taskScheduler.Reset(atTaskId1, std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(resetTaskId1, atTaskId1);

    auto resetTaskId2 = taskScheduler.Reset(atTaskId2, std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(resetTaskId2, atTaskId2);
}

/**
* @tc.name: Reset
* @tc.desc: Reset during task execution
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TaskSchedulerTest, Reset2, TestSize.Level0)
{
    TaskScheduler taskScheduler("reset2Test");
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, testData);
    auto expiredTime = std::chrono::steady_clock::now();
    auto atTaskId = taskScheduler.At(expiredTime, [blockData]() {
        blockData->GetValue();
    });
    ASSERT_EQ(atTaskId, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL));
    auto resetTaskId = taskScheduler.Reset(atTaskId, std::chrono::milliseconds(0));
    ASSERT_EQ(resetTaskId, TaskScheduler::INVALID_TASK_ID);
    blockData->SetValue(testData);
}

/**
* @tc.name: Reset
* @tc.desc: Reset after task execution
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TaskSchedulerTest, Reset3, TestSize.Level0)
{
    TaskScheduler taskScheduler("reset3Test");
    auto expiredTime = std::chrono::steady_clock::now();
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, testData);
    auto atTaskId = taskScheduler.At(expiredTime, [blockData]() {
        blockData->GetValue();
    });
    ASSERT_EQ(atTaskId, 1);
    blockData->SetValue(testData);
    blockData->SetValue(testData);
    ASSERT_EQ(blockData->GetValue(), testData);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL));
    auto resetTaskId = taskScheduler.Reset(atTaskId, std::chrono::milliseconds(0));
    ASSERT_EQ(resetTaskId, TaskScheduler::INVALID_TASK_ID);
}

/**
* @tc.name: Every
* @tc.desc: execute task for some times periodically with duration.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(TaskSchedulerTest, EveryExecuteTimes, TestSize.Level0)
{
    TaskScheduler taskScheduler("everyTimes");
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, 0);
    int testData = 0;
    int times = 5;
    auto taskId = taskScheduler.Every(times, std::chrono::milliseconds(0),
        std::chrono::milliseconds(SHORT_INTERVAL), [blockData, times, &testData]() {
        testData++;
        if (testData < times) {
            blockData->Clear(testData);
            return;
        }
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), times);
    auto resetId = taskScheduler.Reset(taskId, std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(resetId, TaskScheduler::INVALID_TASK_ID);
    ASSERT_EQ(blockData->GetValue(), times);
}

/**
* @tc.name: Remove
* @tc.desc: remove task before execute.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(TaskSchedulerTest, RemoveBeforeExecute, TestSize.Level0)
{
    TaskScheduler taskScheduler("RemoveBeforeExecute");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL);
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, 0);
    int testData = 0;
    auto taskId = taskScheduler.At(expiredTime, [blockData, testData]() {
        int tmpData = testData + 1;
        blockData->SetValue(tmpData);
    }, std::chrono::milliseconds(SHORT_INTERVAL));
    taskScheduler.Remove(taskId);
    auto resetId = taskScheduler.Reset(taskId, std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(resetId, TaskScheduler::INVALID_TASK_ID);
    ASSERT_EQ(blockData->GetValue(), testData);
}

/**
* @tc.name: Remove
* @tc.desc: remove task during execute, and waiting.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(TaskSchedulerTest, RemoveWaitExecute, TestSize.Level0)
{
    TaskScheduler taskScheduler("RemoveWaitExecute");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(0);
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL, 0);
    auto blockDataWait = std::make_shared<BlockData<int>>(LONG_INTERVAL, 0);
    int testData = 1;
    auto taskId = taskScheduler.At(expiredTime, [blockDataTest, blockDataWait, &testData]() {
        blockDataTest->SetValue(testData);
        blockDataWait->GetValue();
        int tmpData = testData + 1;
        blockDataTest->SetValue(tmpData);
    }, std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(blockDataTest->GetValue(), testData);
    auto resetId = taskScheduler.Reset(taskId, std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(taskId, resetId);
    taskScheduler.Remove(taskId, true);
    ASSERT_EQ(blockDataTest->GetValue(), testData + 1);
}

/**
* @tc.name: Remove
* @tc.desc: remove task during execute, but no wait.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(TaskSchedulerTest, RemoveNoWaitExecute, TestSize.Level0)
{
    TaskScheduler taskScheduler("RemoveNoWaitExecute");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(0);
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL, 0);
    auto blockDataWait = std::make_shared<BlockData<int>>(LONG_INTERVAL, 0);
    int testData = 1;
    auto taskId = taskScheduler.At(expiredTime, [blockDataTest, blockDataWait, &testData]() {
        blockDataTest->SetValue(testData);
        blockDataWait->GetValue();
        int tmpData = testData + 1;
        blockDataTest->SetValue(tmpData);
    });
    ASSERT_EQ(blockDataTest->GetValue(), testData);
    blockDataTest->Clear(0);
    taskScheduler.Remove(taskId);
    blockDataWait->SetValue(testData);
    ASSERT_EQ(blockDataTest->GetValue(), testData + 1);
}
} // namespace OHOS::Test