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

#include <gtest/gtest.h>
#include "block_data.h"
#include "task_scheduler.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS;
using duration = std::chrono::steady_clock::duration;

class TaskSchedulerUnitTest : public testing::Test {
public:
    static constexpr uint32_t SHORT_INTERVAL_TIME = 100; // ms
    static constexpr uint32_t LONG_INTERVAL_TEST = 1;    // s
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() { }
};

/**
 * @tc.name: At
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, At001Test, TestSize.Level0)
{
    TaskScheduler testScheduler("atTest");
    auto expiredTimeTest =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    int dataTest = 10;
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, dataTest);
    auto atTaskId11 = testScheduler.At(expiredTimeTest, [blockDataTest]() {
        int dataTest = 11;
        blockDataTest->SetValue(dataTest);
    });
    EXPECT_EQ(blockDataTest->GetValue(), 11);
    blockDataTest->Clear();
    expiredTimeTest =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    auto atTaskId22 = testScheduler.At(expiredTimeTest, [blockDataTest]() {
        int dataTest = 12;
        blockDataTest->SetValue(dataTest);
    });
    EXPECT_EQ(blockDataTest->GetValue(), 12);
    ASSERT_NE(atTaskId11, atTaskId22);
}

/**
 * @tc.name: At
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, At002Test, TestSize.Level0)
{
    TaskScheduler testScheduler("atTesthrs562b65rhem2j.6u5re3h5r6gfegbr656y5kjtyh2reh65esrwe");
    auto expiredTimeTest =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    int data = 18;
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, data);
    auto atTask = testScheduler.At(expiredTime, [blockData]() {
        int data = 11;
        blockDataTest->SetValue(data);
    });
    auto atTaskId25 = Scheduler.At(expiredTime, [blockData]() {
        int dataTest = 12;
        blockDataTest->SetValue(dataTest);
    });
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    ASSERT_NE(blockDataTest->GetValue(), 11);
    blockDataTest->Clear();
    EXPECT_EQ(blockDataTest->GetValue(), 12);
    EXPECT_EQ(atTaskId11, atTaskId25);
}

/**
 * @tc.name: Every
 * @tc.desc:execute task periodically with duration
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, ExecuteDurationTest, TestSize.Level0)
{
    TaskScheduler testScheduler("everyTesthrbnberhki,gmpruwsgm3hjioprwetiupdfjserhyer");
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, 0);
    int dataTest = 0;
    testScheduler.Every(std::chrono::milliseconds(SHORT_INTERVAL_TIME), [blockDataTest, &dataTest]() {
        dataTest++;
        blockDataTest->SetValue(dataTest);
    });
    for (int i = 1; i < 10; ++i) {
        EXPECT_EQ(blockDataTest->GetValue(), i);
        blockDataTest->Clear(0);
    }
}

/**
 * @tc.name: Reset
 * @tc.desc: Reset before task execution and the task is tasks_.begin() or not
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, Reset1Test, TestSize.Level0)
{
    TaskScheduler testScheduler("reset1Testreset1Testreset1Testreset1Testreset1Testreset1Testreset1Test");
    auto expiredTimeTest =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    auto atTaskId11 = testScheduler.At(expiredTimeTest, []() {});
    EXPECT_EQ(atTaskId11, 1);
    expiredTimeTest += std::chrono::milliseconds(LONG_INTERVAL_TEST);
    auto atTaskId22 = testScheduler.At(expiredTimeTest, []() {});
    EXPECT_EQ(atTaskId22, 2);

    auto resetTaskId1 = testScheduler.Reset(atTaskId11, std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    EXPECT_EQ(resetTaskId1, atTaskId11);

    auto resetTaskId2 = testScheduler.Reset(atTaskId22, std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    EXPECT_EQ(resetTaskId2, atTaskId22);
}

/**
 * @tc.name: Reset
 * @tc.desc: Reset during task execution
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, Reset2Test, TestSize.Level0)
{
    TaskScheduler testScheduler("reset2Testreset2Testreset2Test");
    int dataTest = 10;
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, dataTest);
    auto expiredTimeTest = std::chrono::steady_clock::now();
    auto atId = testScheduler.At(expiredTimeTest, [blockDataTest]() {
        blockDataTest->GetValue();
    });
    EXPECT_EQ(atId, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    auto taskId = testScheduler.Reset(atId, std::chrono::milliseconds(0));
    EXPECT_EQ(taskId, TaskScheduler::INVALID_TASK_ID);
    blockDataTest->SetValue(dataTest);
}

/**
 * @tc.name: Reset
 * @tc.desc: Reset after task execution
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, Reset3Test, TestSize.Level0)
{
    TaskScheduler testScheduler("reset3Testreset3Testreset3Testreset3Test");
    auto expiredTimeTest = std::chrono::steady_clock::now();
    int dataTest = 10;
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, dataTest);
    auto atId = testScheduler.At(expiredTimeTest, [blockDataTest]() {
        blockDataTest->GetValue();
    });
    EXPECT_EQ(atId, 1);
    blockDataTest->SetValue(dataTest);
    blockDataTest->SetValue(dataTest);
    EXPECT_EQ(blockDataTest->GetValue(), dataTest);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    auto taskId = testScheduler.Reset(atId, std::chrono::milliseconds(0));
    EXPECT_EQ(taskId, TaskScheduler::INVALID_TASK_ID);
}

/**
 * @tc.name: Every
 * @tc.desc: execute task for some times periodically with duration.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangjiang
 */
HWTEST_F(TaskSchedulerUnitTest, EveryExecuteTimesTest, TestSize.Level0)
{
    TaskScheduler testScheduler("everyTimeseveryTimeseveryTimeseveryTimeseveryTimeseveryTimes");
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, 0);
    int dataTest = 0;
    int times = 5;
    auto taskId = testScheduler.Every(
        times, std::chrono::milliseconds(0), std::chrono::milliseconds(SHORT_INTERVAL_TIME),
        [blockDataTest, times, &dataTest]() {
            dataTest++;
            if (dataTest < times) {
                blockDataTest->Clear(dataTest);
                return;
            }
            blockDataTest->SetValue(dataTest);
        });
    EXPECT_EQ(blockDataTest->GetValue(), times);
    auto reId = testScheduler.Reset(taskId, std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    EXPECT_EQ(reId, TaskScheduler::INVALID_TASK_ID);
    EXPECT_EQ(blockDataTest->GetValue(), times);
}

/**
 * @tc.name: Remove
 * @tc.desc: remove task before execute.
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, RemoveBeforeExecuteTest, TestSize.Level0)
{
    TaskScheduler testScheduler("RemoveBeforeExecuteRemoveBefore");
    auto expiredTimeTest = std::chrono::steady_clock::now() + std::chrono::milliseconds(SHORT_INTERVAL_TIME);
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, 0);
    int dataTest = 0;
    auto taskId = testScheduler.At(
        expiredTimeTest,
        [blockDataTest, dataTest]() {
            int tmpDataTest = dataTest + 1;
            blockDataTest->SetValue(tmpDataTest);
        },
        std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    testScheduler.Remove(taskId);
    auto reId = testScheduler.Reset(taskId, std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    EXPECT_EQ(reId, TaskScheduler::INVALID_TASK_ID);
    EXPECT_EQ(blockDataTest->GetValue(), dataTest);
}

/**
 * @tc.name: Remove
 * @tc.desc: remove task during execute, and waiting.
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, RemoveWaitExecuteTest, TestSize.Level0)
{
    TaskScheduler testScheduler("RemoveWaitExecute");
    auto expiredTimeTest = std::chrono::steady_clock::now() + std::chrono::milliseconds(0);
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, 0);
    auto blockDataWaitTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, 0);
    int dataTest = 1;
    auto taskId = testScheduler.At(
        expiredTimeTest,
        [blockDataTest, blockDataWaitTest, &dataTest]() {
            blockDataTest->SetValue(dataTest);
            blockDataWaitTest->GetValue();
            int tmpDataTest = dataTest + 1;
            blockDataTest->SetValue(tmpDataTest);
        },
        std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    EXPECT_EQ(blockDataTest->GetValue(), dataTest);
    auto reId = testScheduler.Reset(taskId, std::chrono::milliseconds(SHORT_INTERVAL_TIME));
    EXPECT_EQ(taskId, reId);
    testScheduler.Remove(taskId, true);
    EXPECT_EQ(blockDataTest->GetValue(), dataTest + 1);
}

/**
 * @tc.name: Remove
 * @tc.desc: remove task during execute, but no wait.
 * @tc.type: FUNC
 */
HWTEST_F(TaskSchedulerUnitTest, RemoveNoWaitExecuteTest, TestSize.Level0)
{
    TaskScheduler testScheduler("RemoveNoWaitExecuteRemoveNoWaitExecuteRemoveNoWaitExecute");
    auto expiredTimeTest = std::chrono::steady_clock::now() + std::chrono::milliseconds(0);
    auto blockDataTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, 0);
    auto blockDataWaitTest = std::make_shared<BlockData<int>>(LONG_INTERVAL_TEST, 0);
    int dataTest = 1;
    auto taskId = testScheduler.At(expiredTimeTest, [blockDataTest, blockDataWaitTest, &dataTest]() {
        blockDataTest->SetValue(dataTest);
        blockDataWaitTest->GetValue();
        int tmpDataTest = dataTest + 1;
        blockDataTest->SetValue(tmpDataTest);
    });
    EXPECT_EQ(blockDataTest->GetValue(), dataTest);
    blockDataTest->Clear(0);
    testScheduler.Remove(taskId);
    blockDataWaitTest->SetValue(dataTest);
    EXPECT_EQ(blockDataTest->GetValue(), dataTest + 1);
}
} // namespace OHOS::Test