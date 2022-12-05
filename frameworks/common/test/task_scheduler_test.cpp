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

using namespace testing::ext;
using namespace OHOS;
using duration = std::chrono::steady_clock::duration;
class TaskSchedulerTest : public testing::Test {
public:
    static constexpr uint32_t SHORT_INTERVAL = 100;
    static constexpr uint32_t LONG_INTERVAL = 300;
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
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(TaskSchedulerTest, Every, TestSize.Level0)
{
    TaskScheduler taskScheduler("everyTest");
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, 0);
    taskScheduler.Every(std::chrono::milliseconds(SHORT_INTERVAL), [blockData]() {
        int testData = 1;
        blockData->SetValue(testData);
    });
    for (int i = 1; i < 5; ++i) {
        ASSERT_EQ(blockData->GetValue(), 1);
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
    expiredTime += std::chrono::milliseconds(LONG_INTERVAL);
    auto atTaskId2 = taskScheduler.At(expiredTime, []() {});

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
    blockData->SetValue(testData);
    blockData->SetValue(testData);
    ASSERT_EQ(blockData->GetValue(), testData);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL));
    auto resetTaskId = taskScheduler.Reset(atTaskId, std::chrono::milliseconds(0));
    ASSERT_EQ(resetTaskId, TaskScheduler::INVALID_TASK_ID);
}
