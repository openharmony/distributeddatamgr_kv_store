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
#include "task_executor.h"

#include <gtest/gtest.h>

#include "block_data.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS;
using duration = std::chrono::steady_clock::duration;
class TaskExecutorTest : public testing::Test {
public:
    static constexpr uint32_t SHORT_INTERVAL = 100; // ms
    static constexpr uint32_t LONG_INTERVAL = 1;    // s
    TaskExecutor &taskExecutor = TaskExecutor::GetInstance();
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown() {}
};

/**
* @tc.name: Execute
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: CRJ
*/
HWTEST_F(TaskExecutorTest, Execute, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, testData);
    auto atTaskId1 = taskExecutor.Execute(
        [blockData]() {
            int testData = 11;
            blockData->SetValue(testData);
        },
        expiredTime);
    ASSERT_EQ(blockData->GetValue(), 11);
    blockData->Clear();
    auto atTaskId2 = taskExecutor.Execute(
        [blockData]() {
            int testData = 12;
            blockData->SetValue(testData);
        },
        expiredTime);
    ASSERT_EQ(blockData->GetValue(), 12);
    ASSERT_NE(atTaskId1, atTaskId2);
}

/**
* @tc.name: Schedule
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: CRJ
*/
HWTEST_F(TaskExecutorTest, Schedule, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    int testData = 10;
    auto taskId = taskExecutor.Schedule(
        [&testData] {
            testData++;
        },
        expiredTime);
    ASSERT_NE(taskId, TaskExecutor::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(testData, 20);
}

/**
* @tc.name: MultiSchedule
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: CRJ
*/
HWTEST_F(TaskExecutorTest, MultiSchedule, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(0, testData);
    auto task = [&blockData] {
        auto a = blockData->GetValue() + 1;
        blockData->SetValue(a);
    };
    TaskExecutor::TaskId id;
    for (int i = 0; i < 10; ++i) {
        auto temp = taskExecutor.Schedule(task, expiredTime, std::chrono::seconds(0), 10);
        ASSERT_NE(temp, id);
        id = temp;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(blockData->GetValue(), 110);
}
/**
* @tc.name: Remove
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: CRJ
*/
HWTEST_F(TaskExecutorTest, Remove, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(0, testData);
    auto task = [&blockData] {
        auto a = blockData->GetValue() + 1;
        blockData->SetValue(a);
    };
    auto temp = taskExecutor.Schedule(task, expiredTime * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(blockData->GetValue(), 11);
    ASSERT_TRUE(taskExecutor.Remove(temp));
    std::this_thread::sleep_for(expiredTime * 4);
    ASSERT_EQ(blockData->GetValue(), 11);
}
/**
* @tc.name: Reset
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
* @tc.author: CRJ
*/
HWTEST_F(TaskExecutorTest, Reset, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(0, testData);
    auto task = [&blockData] {
        auto a = blockData->GetValue() + 1;
        blockData->SetValue(a);
    };
    auto temp = taskExecutor.Schedule(task, expiredTime * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(blockData->GetValue(), 11);
    auto res = taskExecutor.Reset(temp, expiredTime * 5);
    ASSERT_EQ(res, temp);
    std::this_thread::sleep_for(expiredTime * 2);
    ASSERT_EQ(blockData->GetValue(), 11);
    std::this_thread::sleep_for(expiredTime * 4);
    ASSERT_EQ(blockData->GetValue(), 12);
    taskExecutor.Remove(res);
}
//auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, testData);
} // namespace OHOS::Test
