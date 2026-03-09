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
#include <gtest/gtest.h>

#include "block_data.h"
#include "executor_pool.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS;
using duration = std::chrono::steady_clock::duration;
class ExecutorPoolTestNew : public testing::Test {
public:
    struct Data {
        std::mutex mutex_;
        int data = 0;
        void Add()
        {
            std::lock_guard<std::mutex> lockGuard(mutex_);
            data++;
        }
    };
    static constexpr uint32_t SHORT_INTERVAL = 80; // ms
    static constexpr uint32_t LONG_INTERVAL = 2;    // s
    static std::shared_ptr<ExecutorPool> executorPoolNew_;
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void)
    {
        executorPoolNew_ = nullptr;
    };
    void SetUp() {};
    void TearDown() { }
};
std::shared_ptr<ExecutorPool> ExecutorPoolTestNew::executorPoolNew_ =
    std::make_shared<ExecutorPool>(16, 6);

/**
 * @tc.name: ExecuteNew
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(LONG_INTERVAL, testData);
    auto atTaskId1 = executorPoolNew_->Schedule(expiredTime, [blockData]() {
        int testData = 11;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 11);
    blockData->Clear();
    auto atTaskId2 = executorPoolNew_->Schedule(expiredTime, [blockData]() {
        int testData = 12;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 12);
    ASSERT_NE(atTaskId1, atTaskId2);
}

/**
 * @tc.name: ScheduleNew
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    auto testData = std::make_shared<Data>();
    auto taskId = executorPoolNew_->Schedule(
        [testData] {
            testData->Add();
        },
        expiredTime);
    ASSERT_NE(taskId, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 2));
    ASSERT_GT(testData->data, 1);
    executorPoolNew_->Remove(taskId);
}

/**
 * @tc.name: MultiScheduleNew
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew, TestSize.Level0)
{
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    std::set<ExecutorPool::TaskId> ids;
    for (int i = 0; i < 3; ++i) {
        auto id = executorPoolNew_->Schedule(task, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 3), 2);
        ASSERT_EQ(ids.count(id), 0);
        ids.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(data->data, 6);
    for (auto id : ids) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    auto temp = executorPoolNew_->Schedule(task, expiredTime * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(data->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(temp));
    std::this_thread::sleep_for(expiredTime * 4);
    ASSERT_EQ(data->data, 1);
}

/**
 * @tc.name: ResetNew
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew, TestSize.Level0)
{
    auto expiredTime = std::chrono::milliseconds(SHORT_INTERVAL);
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    // Use single-shot task
    auto temp = executorPoolNew_->Schedule(task, expiredTime * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 2));
    ASSERT_EQ(data->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(temp, std::chrono::milliseconds(expiredTime * 4)), temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 5));
    ASSERT_EQ(data->data, 2);
    executorPoolNew_->Remove(temp);
}

/**
 * @tc.name: MaxEqualsOneNew
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew, TestSize.Level0)
{
    auto executors = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> testNum = 1;
    auto delayTask = [&testNum] {
        testNum++;
    };
    auto task = [&testNum] {
        testNum += 2;
    };
    executors->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 2), delayTask);
    ASSERT_EQ(testNum, 1);
    executors->Execute(task);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL));
    ASSERT_EQ(testNum, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 2));
    ASSERT_EQ(testNum, 4);
}

/**
 * @tc.name: RemoveInExecuteTaskNew
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew, TestSize.Level0)
{
    auto executors = std::make_shared<ExecutorPool>(1, 1);
    auto taskId = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> flag = 0;
    taskId = executors->Schedule(
        [executors, &taskId, &flag]() {
            flag++;
            executors->Remove(taskId, false);
            taskId = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL),
        12);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskId, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(flag, 1);
}

/**
 * @tc.name: ExecuteNew2
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew2, TestSize.Level0)
{
    auto delayPeriod = std::chrono::milliseconds(SHORT_INTERVAL * 2);
    int startValue = 20;
    auto blockValue = std::make_shared<BlockData<int>>(LONG_INTERVAL, startValue);
    auto taskHandle1 = executorPoolNew_->Schedule(delayPeriod, [blockValue]() {
        int newVal = 25;
        blockValue->SetValue(newVal);
    });
    ASSERT_EQ(blockValue->GetValue(), 25);
    blockValue->Clear();
    auto taskHandle2 = executorPoolNew_->Schedule(delayPeriod, [blockValue]() {
        int newVal = 30;
        blockValue->SetValue(newVal);
    });
    ASSERT_EQ(blockValue->GetValue(), 30);
    ASSERT_NE(taskHandle1, taskHandle2);
}

/**
 * @tc.name: ScheduleNew2
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew2, TestSize.Level0)
{
    auto executionDelay = std::chrono::milliseconds(SHORT_INTERVAL * 2);
    auto dataCounter = std::make_shared<Data>();
    auto scheduledId = executorPoolNew_->Schedule(
        [dataCounter] {
            dataCounter->Add();
        },
        executionDelay);
    ASSERT_NE(scheduledId, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 4));
    ASSERT_GT(dataCounter->data, 1);
    executorPoolNew_->Remove(scheduledId);
}

/**
 * @tc.name: MultiScheduleNew2
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew2, TestSize.Level0)
{
    auto accumulator = std::make_shared<Data>();
    auto operation = [accumulator] {
        accumulator->Add();
    };
    std::set<ExecutorPool::TaskId> taskHandles;
    for (int j = 0; j < 5; ++j) {
        auto id = executorPoolNew_->Schedule(operation, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(taskHandles.count(id), 0);
        taskHandles.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(accumulator->data, 10);
    for (auto id : taskHandles) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew2
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew2, TestSize.Level0)
{
    auto timeDelay = std::chrono::milliseconds(SHORT_INTERVAL * 3);
    auto valueCounter = std::make_shared<Data>();
    auto workTask = [valueCounter] {
        valueCounter->Add();
    };
    auto scheduledWork = executorPoolNew_->Schedule(workTask, timeDelay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 2));
    ASSERT_EQ(valueCounter->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(scheduledWork));
    std::this_thread::sleep_for(timeDelay * 5);
    ASSERT_EQ(valueCounter->data, 1);
}

/**
 * @tc.name: ResetNew2
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew2, TestSize.Level0)
{
    auto initialDelay = std::chrono::milliseconds(SHORT_INTERVAL * 2);
    auto dataAccumulator = std::make_shared<Data>();
    auto workFunction = [dataAccumulator] {
        dataAccumulator->Add();
    };
    // Use single-shot task
    auto taskIdentifier = executorPoolNew_->Schedule(workFunction, initialDelay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 3));
    ASSERT_EQ(dataAccumulator->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(taskIdentifier,
        std::chrono::milliseconds(initialDelay * 4)), taskIdentifier);
    std::this_thread::sleep_for(std::chrono::milliseconds(initialDelay * 5));
    ASSERT_EQ(dataAccumulator->data, 2);
    executorPoolNew_->Remove(taskIdentifier);
}

/**
 * @tc.name: MaxEqualsOneNew2
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew2, TestSize.Level0)
{
    auto threadExecutor = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> atomicValue = 2;
    auto delayedFunction = [&atomicValue] {
        atomicValue++;
    };
    auto immediateFunction = [&atomicValue] {
        atomicValue += 3;
    };
    threadExecutor->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 3), delayedFunction);
    ASSERT_EQ(atomicValue, 2);
    threadExecutor->Execute(immediateFunction);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 2));
    ASSERT_EQ(atomicValue, 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 3));
    ASSERT_EQ(atomicValue, 6);
}

/**
 * @tc.name: RemoveInExecuteTaskNew2
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew2, TestSize.Level0)
{
    auto poolInstance = std::make_shared<ExecutorPool>(1, 1);
    auto workTaskId = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> executionFlag = 0;
    workTaskId = poolInstance->Schedule(
        [poolInstance, &workTaskId, &executionFlag]() {
            executionFlag++;
            poolInstance->Remove(workTaskId, false);
            workTaskId = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 2),
        14);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(workTaskId, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(executionFlag, 1);
}

/**
 * @tc.name: ExecuteNew3
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew3, TestSize.Level0)
{
    auto executionTime = std::chrono::milliseconds(SHORT_INTERVAL * 3);
    int initialValue = 30;
    auto dataBlock = std::make_shared<BlockData<int>>(LONG_INTERVAL, initialValue);
    auto scheduledTask1 = executorPoolNew_->Schedule(executionTime, [dataBlock]() {
        int newVal = 35;
        dataBlock->SetValue(newVal);
    });
    ASSERT_EQ(dataBlock->GetValue(), 35);
    dataBlock->Clear();
    auto scheduledTask2 = executorPoolNew_->Schedule(executionTime, [dataBlock]() {
        int newVal = 40;
        dataBlock->SetValue(newVal);
    });
    ASSERT_EQ(dataBlock->GetValue(), 40);
    ASSERT_NE(scheduledTask1, scheduledTask2);
}

/**
 * @tc.name: ScheduleNew3
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew3, TestSize.Level0)
{
    auto delayTime = std::chrono::milliseconds(SHORT_INTERVAL * 3);
    auto counterData = std::make_shared<Data>();
    auto taskIdentifier = executorPoolNew_->Schedule(
        [counterData] {
            counterData->Add();
        },
        delayTime);
    ASSERT_NE(taskIdentifier, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 5));
    ASSERT_GT(counterData->data, 1);
    executorPoolNew_->Remove(taskIdentifier);
}

/**
 * @tc.name: MultiScheduleNew3
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew3, TestSize.Level0)
{
    auto dataAccumulator = std::make_shared<Data>();
    auto workOperation = [dataAccumulator] {
        dataAccumulator->Add();
    };
    std::set<ExecutorPool::TaskId> taskIds;
    for (int k = 0; k < 3; ++k) {
        auto id = executorPoolNew_->Schedule(workOperation, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(taskIds.count(id), 0);
        taskIds.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(dataAccumulator->data, 6);
    for (auto id : taskIds) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew3
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew3, TestSize.Level0)
{
    auto delayDuration = std::chrono::milliseconds(SHORT_INTERVAL * 4);
    auto valueStorage = std::make_shared<Data>();
    auto workOperation = [valueStorage] {
        valueStorage->Add();
    };
    auto scheduledTask = executorPoolNew_->Schedule(workOperation, delayDuration * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 3));
    ASSERT_EQ(valueStorage->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(scheduledTask));
    std::this_thread::sleep_for(delayDuration * 6);
    ASSERT_EQ(valueStorage->data, 1);
}

/**
 * @tc.name: ResetNew3
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew3, TestSize.Level0)
{
    auto initialDelay = std::chrono::milliseconds(SHORT_INTERVAL * 3);
    auto counterStorage = std::make_shared<Data>();
    auto executionFunction = [counterStorage] {
        counterStorage->Add();
    };
    // Use single-shot task
    auto workHandle = executorPoolNew_->Schedule(executionFunction, initialDelay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 3));
    ASSERT_EQ(counterStorage->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(workHandle,
        std::chrono::milliseconds(initialDelay * 4)), workHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(initialDelay * 5));
    ASSERT_EQ(counterStorage->data, 2);
    executorPoolNew_->Remove(workHandle);
}

/**
 * @tc.name: MaxEqualsOneNew3
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew3, TestSize.Level0)
{
    auto singleThreadPool = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> atomicCounter = 3;
    auto delayedExecution = [&atomicCounter] {
        atomicCounter++;
    };
    auto immediateExecution = [&atomicCounter] {
        atomicCounter += 4;
    };
    singleThreadPool->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 4), delayedExecution);
    ASSERT_EQ(atomicCounter, 3);
    singleThreadPool->Execute(immediateExecution);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 3));
    ASSERT_EQ(atomicCounter, 7);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 4));
    ASSERT_EQ(atomicCounter, 8);
}

/**
 * @tc.name: RemoveInExecuteTaskNew3
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew3, TestSize.Level0)
{
    auto executorInstance = std::make_shared<ExecutorPool>(1, 1);
    auto taskIdReference = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> statusFlag = 0;
    taskIdReference = executorInstance->Schedule(
        [executorInstance, &taskIdReference, &statusFlag]() {
            statusFlag++;
            executorInstance->Remove(taskIdReference, false);
            taskIdReference = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 3), 16);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskIdReference, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(statusFlag, 1);
}

/**
 * @tc.name: ExecuteNew4
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew4, TestSize.Level0)
{
    auto executionPeriod = std::chrono::milliseconds(SHORT_INTERVAL * 4);
    int startingValue = 40;
    auto sharedData = std::make_shared<BlockData<int>>(LONG_INTERVAL, startingValue);
    auto workItem1 = executorPoolNew_->Schedule(executionPeriod, [sharedData]() {
        int newVal = 45;
        sharedData->SetValue(newVal);
    });
    ASSERT_EQ(sharedData->GetValue(), 45);
    sharedData->Clear();
    auto workItem2 = executorPoolNew_->Schedule(executionPeriod, [sharedData]() {
        int newVal = 50;
        sharedData->SetValue(newVal);
    });
    ASSERT_EQ(sharedData->GetValue(), 50);
    ASSERT_NE(workItem1, workItem2);
}

/**
 * @tc.name: ScheduleNew4
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew4, TestSize.Level0)
{
    auto waitInterval = std::chrono::milliseconds(SHORT_INTERVAL * 4);
    auto sharedCounter = std::make_shared<Data>();
    auto handleId = executorPoolNew_->Schedule(
        [sharedCounter] {
            sharedCounter->Add();
        },
        waitInterval);
    ASSERT_NE(handleId, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 6));
    ASSERT_GT(sharedCounter->data, 1);
    executorPoolNew_->Remove(handleId);
}

/**
 * @tc.name: MultiScheduleNew4
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew4, TestSize.Level0)
{
    auto dataStore = std::make_shared<Data>();
    auto processData = [dataStore] {
        dataStore->Add();
    };
    std::set<ExecutorPool::TaskId> taskHandles;
    for (int m = 0; m < 6; ++m) {
        auto id = executorPoolNew_->Schedule(processData, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(taskHandles.count(id), 0);
        taskHandles.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(dataStore->data, 12);
    for (auto id : taskHandles) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew4
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew4, TestSize.Level0)
{
    auto executionDelay = std::chrono::milliseconds(SHORT_INTERVAL * 5);
    auto dataObject = std::make_shared<Data>();
    auto scheduledWork = [dataObject] {
        dataObject->Add();
    };
    auto workReference = executorPoolNew_->Schedule(scheduledWork, executionDelay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 4));
    ASSERT_EQ(dataObject->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(workReference));
    std::this_thread::sleep_for(executionDelay * 7);
    ASSERT_EQ(dataObject->data, 1);
}

/**
 * @tc.name: ResetNew4
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew4, TestSize.Level0)
{
    auto startDelay = std::chrono::milliseconds(SHORT_INTERVAL * 4);
    auto counterObject = std::make_shared<Data>();
    auto workerFunction = [counterObject] {
        counterObject->Add();
    };
    // Use single-shot task
    auto taskReference = executorPoolNew_->Schedule(workerFunction, startDelay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 5));
    ASSERT_EQ(counterObject->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(taskReference,
        std::chrono::milliseconds(startDelay * 4)), taskReference);
    std::this_thread::sleep_for(std::chrono::milliseconds(startDelay * 5));
    ASSERT_EQ(counterObject->data, 2);
    executorPoolNew_->Remove(taskReference);
}

/**
 * @tc.name: MaxEqualsOneNew4
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew4, TestSize.Level0)
{
    auto constrainedPool = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> sharedValue = 4;
    auto delayedTask = [&sharedValue] {
        sharedValue++;
    };
    auto immediateTask = [&sharedValue] {
        sharedValue += 5;
    };
    constrainedPool->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 5), delayedTask);
    ASSERT_EQ(sharedValue, 4);
    constrainedPool->Execute(immediateTask);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 4));
    ASSERT_EQ(sharedValue, 9);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 5));
    ASSERT_EQ(sharedValue, 10);
}

/**
 * @tc.name: RemoveInExecuteTaskNew4
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew4, TestSize.Level0)
{
    auto threadPool = std::make_shared<ExecutorPool>(1, 1);
    auto taskIdentity = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> flagState = 0;
    taskIdentity = threadPool->Schedule(
        [threadPool, &taskIdentity, &flagState]() {
            flagState++;
            threadPool->Remove(taskIdentity, false);
            taskIdentity = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 4), 18);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskIdentity, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(flagState, 1);
}

/**
 * @tc.name: ExecuteNew5
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew5, TestSize.Level0)
{
    auto scheduledTime = std::chrono::milliseconds(SHORT_INTERVAL * 5);
    int baseValue = 50;
    auto dataContainer = std::make_shared<BlockData<int>>(LONG_INTERVAL, baseValue);
    auto operation1 = executorPoolNew_->Schedule(scheduledTime, [dataContainer]() {
        int newVal = 55;
        dataContainer->SetValue(newVal);
    });
    ASSERT_EQ(dataContainer->GetValue(), 55);
    dataContainer->Clear();
    auto operation2 = executorPoolNew_->Schedule(scheduledTime, [dataContainer]() {
        int newVal = 60;
        dataContainer->SetValue(newVal);
    });
    ASSERT_EQ(dataContainer->GetValue(), 60);
    ASSERT_NE(operation1, operation2);
}

/**
 * @tc.name: ScheduleNew5
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew5, TestSize.Level0)
{
    auto delayAmount = std::chrono::milliseconds(SHORT_INTERVAL * 5);
    auto counterStore = std::make_shared<Data>();
    auto operationId = executorPoolNew_->Schedule(
        [counterStore] {
            counterStore->Add();
        },
        delayAmount);
    ASSERT_NE(operationId, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 7));
    ASSERT_GT(counterStore->data, 1);
    executorPoolNew_->Remove(operationId);
}

/**
 * @tc.name: MultiScheduleNew5
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew5, TestSize.Level0)
{
    auto accumulator = std::make_shared<Data>();
    auto processData = [accumulator] {
        accumulator->Add();
    };
    std::set<ExecutorPool::TaskId> operationIds;
    for (int n = 0; n < 7; ++n) {
        auto id = executorPoolNew_->Schedule(processData, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(operationIds.count(id), 0);
        operationIds.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(accumulator->data, 14);
    for (auto id : operationIds) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew5
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew5, TestSize.Level0)
{
    auto waitTime = std::chrono::milliseconds(SHORT_INTERVAL * 6);
    auto valueStore = std::make_shared<Data>();
    auto workFunction = [valueStore] {
        valueStore->Add();
    };
    auto taskId = executorPoolNew_->Schedule(workFunction, waitTime * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 5));
    ASSERT_EQ(valueStore->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(taskId));
    std::this_thread::sleep_for(waitTime * 8);
    ASSERT_EQ(valueStore->data, 1);
}

/**
 * @tc.name: ResetNew5
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew5, TestSize.Level0)
{
    auto initialDelay = std::chrono::milliseconds(SHORT_INTERVAL * 5);
    auto counter = std::make_shared<Data>();
    auto workTask = [counter] {
        counter->Add();
    };
    // Use single-shot task
    auto taskHandle = executorPoolNew_->Schedule(workTask, initialDelay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 6));
    ASSERT_EQ(counter->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(taskHandle,
        std::chrono::milliseconds(initialDelay * 4)), taskHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(initialDelay * 5));
    ASSERT_EQ(counter->data, 2);
    executorPoolNew_->Remove(taskHandle);
}

/**
 * @tc.name: MaxEqualsOneNew5
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew5, TestSize.Level0)
{
    auto singleExecutor = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> sharedCounter = 5;
    auto delayedOperation = [&sharedCounter] {
        sharedCounter++;
    };
    auto immediateOperation = [&sharedCounter] {
        sharedCounter += 6;
    };
    singleExecutor->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 6), delayedOperation);
    ASSERT_EQ(sharedCounter, 5);
    singleExecutor->Execute(immediateOperation);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 5));
    ASSERT_EQ(sharedCounter, 11);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(sharedCounter, 12);
}

/**
 * @tc.name: RemoveInExecuteTaskNew5
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew5, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(1, 1);
    auto taskId = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> flag = 0;
    taskId = executor->Schedule(
        [executor, &taskId, &flag]() {
            flag++;
            executor->Remove(taskId, false);
            taskId = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 5), 20);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskId, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(flag, 1);
}

/**
 * @tc.name: ExecuteNew6
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew6, TestSize.Level0)
{
    auto executionDelay = std::chrono::milliseconds(SHORT_INTERVAL * 6);
    int initialValue = 60;
    auto dataBlock = std::make_shared<BlockData<int>>(LONG_INTERVAL, initialValue);
    auto task1 = executorPoolNew_->Schedule(executionDelay, [dataBlock]() {
        int newVal = 65;
        dataBlock->SetValue(newVal);
    });
    ASSERT_EQ(dataBlock->GetValue(), 65);
    dataBlock->Clear();
    auto task2 = executorPoolNew_->Schedule(executionDelay, [dataBlock]() {
        int newVal = 70;
        dataBlock->SetValue(newVal);
    });
    ASSERT_EQ(dataBlock->GetValue(), 70);
    ASSERT_NE(task1, task2);
}

/**
 * @tc.name: ScheduleNew6
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew6, TestSize.Level0)
{
    auto waitTime = std::chrono::milliseconds(SHORT_INTERVAL * 6);
    auto counter = std::make_shared<Data>();
    auto id = executorPoolNew_->Schedule(
        [counter] {
            counter->Add();
        },
        waitTime);
    ASSERT_NE(id, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_GT(counter->data, 1);
    executorPoolNew_->Remove(id);
}

/**
 * @tc.name: MultiScheduleNew6
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew6, TestSize.Level0)
{
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    std::set<ExecutorPool::TaskId> ids;
    for (int i = 0; i < 8; ++i) {
        auto id = executorPoolNew_->Schedule(task, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(ids.count(id), 0);
        ids.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(data->data, 16);
    for (auto id : ids) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew6
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew6, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 7);
    auto data = std::make_shared<Data>();
    auto work = [data] {
        data->Add();
    };
    auto task = executorPoolNew_->Schedule(work, delay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(data->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(task));
    std::this_thread::sleep_for(delay * 9);
    ASSERT_EQ(data->data, 1);
}

/**
 * @tc.name: ResetNew6
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew6, TestSize.Level0)
{
    auto initialDelay = std::chrono::milliseconds(SHORT_INTERVAL * 6);
    auto data = std::make_shared<Data>();
    auto operation = [data] {
        data->Add();
    };
    // Use single-shot task
    auto task = executorPoolNew_->Schedule(operation, initialDelay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(data->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(task,
        std::chrono::milliseconds(initialDelay * 4)), task);
    std::this_thread::sleep_for(std::chrono::milliseconds(initialDelay * 5));
    ASSERT_EQ(data->data, 2);
    executorPoolNew_->Remove(task);
}

/**
 * @tc.name: MaxEqualsOneNew6
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew6, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> value = 6;
    auto delayed = [&value] {
        value++;
    };
    auto immediate = [&value] {
        value += 7;
    };
    executor->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 7), delayed);
    ASSERT_EQ(value, 6);
    executor->Execute(immediate);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(value, 14);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 7));
    ASSERT_EQ(value, 14);
}

/**
 * @tc.name: RemoveInExecuteTaskNew6
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew6, TestSize.Level0)
{
    auto executors = std::make_shared<ExecutorPool>(1, 1);
    auto taskId = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> flag = 0;
    taskId = executors->Schedule(
        [executors, &taskId, &flag]() {
            flag++;
            executors->Remove(taskId, false);
            taskId = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 6), 22);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskId, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(flag, 1);
}

/**
 * @tc.name: ExecuteNew7
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew7, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 7);
    int startValue = 70;
    auto data = std::make_shared<BlockData<int>>(LONG_INTERVAL, startValue);
    auto task1 = executorPoolNew_->Schedule(delay, [data]() {
        int newVal = 75;
        data->SetValue(newVal);
    });
    ASSERT_EQ(data->GetValue(), 75);
    data->Clear();
    auto task2 = executorPoolNew_->Schedule(delay, [data]() {
        int newVal = 80;
        data->SetValue(newVal);
    });
    ASSERT_EQ(data->GetValue(), 80);
    ASSERT_NE(task1, task2);
}

/**
 * @tc.name: ScheduleNew7
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew7, TestSize.Level0)
{
    auto wait = std::chrono::milliseconds(SHORT_INTERVAL * 7);
    auto counter = std::make_shared<Data>();
    auto id = executorPoolNew_->Schedule(
        [counter] {
            counter->Add();
        },
        wait);
    ASSERT_NE(id, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 9));
    ASSERT_GT(counter->data, 1);
    executorPoolNew_->Remove(id);
}

/**
 * @tc.name: MultiScheduleNew7
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew7, TestSize.Level0)
{
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    std::set<ExecutorPool::TaskId> ids;
    for (int i = 0; i < 9; ++i) {
        auto id = executorPoolNew_->Schedule(task, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(ids.count(id), 0);
        ids.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(data->data, 18);
    for (auto id : ids) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew7
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew7, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 8);
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    auto scheduled = executorPoolNew_->Schedule(task, delay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 7));
    ASSERT_EQ(data->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(scheduled));
    std::this_thread::sleep_for(delay * 10);
    ASSERT_EQ(data->data, 1);
}

/**
 * @tc.name: ResetNew7
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew7, TestSize.Level0)
{
    auto initial = std::chrono::milliseconds(SHORT_INTERVAL * 7);
    auto data = std::make_shared<Data>();
    auto operation = [data] {
        data->Add();
    };
    // Use single-shot task
    auto task = executorPoolNew_->Schedule(operation, initial * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(data->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(task,
        std::chrono::milliseconds(initial * 4)), task);
    std::this_thread::sleep_for(std::chrono::milliseconds(initial * 5));
    ASSERT_EQ(data->data, 2);
    executorPoolNew_->Remove(task);
}

/**
 * @tc.name: MaxEqualsOneNew7
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew7, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> value = 7;
    auto delayed = [&value] {
        value++;
    };
    auto immediate = [&value] {
        value += 8;
    };
    executor->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 8), delayed);
    ASSERT_EQ(value, 7);
    executor->Execute(immediate);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 7));
    ASSERT_EQ(value, 15);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(value, 16);
}

/**
 * @tc.name: RemoveInExecuteTaskNew7
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew7, TestSize.Level0)
{
    auto executors = std::make_shared<ExecutorPool>(1, 1);
    auto taskId = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> flag = 0;
    taskId = executors->Schedule(
        [executors, &taskId, &flag]() {
            flag++;
            executors->Remove(taskId, false);
            taskId = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 7), 24);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskId, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(flag, 1);
}

/**
 * @tc.name: ExecuteNew8
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew8, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 8);
    int start = 80;
    auto data = std::make_shared<BlockData<int>>(LONG_INTERVAL, start);
    auto task1 = executorPoolNew_->Schedule(delay, [data]() {
        int newVal = 85;
        data->SetValue(newVal);
    });
    ASSERT_EQ(data->GetValue(), 85);
    data->Clear();
    auto task2 = executorPoolNew_->Schedule(delay, [data]() {
        int newVal = 90;
        data->SetValue(newVal);
    });
    ASSERT_EQ(data->GetValue(), 90);
    ASSERT_NE(task1, task2);
}

/**
 * @tc.name: ScheduleNew8
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew8, TestSize.Level0)
{
    auto wait = std::chrono::milliseconds(SHORT_INTERVAL * 8);
    auto counter = std::make_shared<Data>();
    auto id = executorPoolNew_->Schedule(
        [counter] {
            counter->Add();
        },
        wait);
    ASSERT_NE(id, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_GT(counter->data, 1);
    executorPoolNew_->Remove(id);
}

/**
 * @tc.name: MultiScheduleNew8
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew8, TestSize.Level0)
{
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    std::set<ExecutorPool::TaskId> ids;
    for (int i = 0; i < 10; ++i) {
        auto id = executorPoolNew_->Schedule(task, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(ids.count(id), 0);
        ids.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(data->data, 20);
    for (auto id : ids) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew8
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew8, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 9);
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    auto scheduled = executorPoolNew_->Schedule(task, delay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(data->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(scheduled));
    std::this_thread::sleep_for(delay * 11);
    ASSERT_EQ(data->data, 1);
}

/**
 * @tc.name: ResetNew8
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew8, TestSize.Level0)
{
    auto initial = std::chrono::milliseconds(SHORT_INTERVAL * 8);
    auto data = std::make_shared<Data>();
    auto operation = [data] {
        data->Add();
    };
    // Use single-shot task
    auto task = executorPoolNew_->Schedule(operation, initial * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 9));
    ASSERT_EQ(data->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(task,
        std::chrono::milliseconds(initial * 4)), task);
    std::this_thread::sleep_for(std::chrono::milliseconds(initial * 5));
    ASSERT_EQ(data->data, 2);
    executorPoolNew_->Remove(task);
}

/**
 * @tc.name: MaxEqualsOneNew8
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew8, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> value = 8;
    auto delayed = [&value] {
        value++;
    };
    auto immediate = [&value] {
        value += 9;
    };
    executor->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 9), delayed);
    ASSERT_EQ(value, 8);
    executor->Execute(immediate);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 8));
    ASSERT_EQ(value, 17);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 9));
    ASSERT_EQ(value, 18);
}

/**
 * @tc.name: RemoveInExecuteTaskNew8
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew8, TestSize.Level0)
{
    auto executors = std::make_shared<ExecutorPool>(1, 1);
    auto taskId = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> flag = 0;
    taskId = executors->Schedule(
        [executors, &taskId, &flag]() {
            flag++;
            executors->Remove(taskId, false);
            taskId = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 8), 26);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskId, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(flag, 1);
}

/**
 * @tc.name: ExecuteNew9
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew9, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 9);
    int start = 90;
    auto data = std::make_shared<BlockData<int>>(LONG_INTERVAL, start);
    auto task1 = executorPoolNew_->Schedule(delay, [data]() {
        int newVal = 95;
        data->SetValue(newVal);
    });
    ASSERT_EQ(data->GetValue(), 95);
    data->Clear();
    auto task2 = executorPoolNew_->Schedule(delay, [data]() {
        int newVal = 100;
        data->SetValue(newVal);
    });
    ASSERT_EQ(data->GetValue(), 100);
    ASSERT_NE(task1, task2);
}

/**
 * @tc.name: ScheduleNew9
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew9, TestSize.Level0)
{
    auto wait = std::chrono::milliseconds(SHORT_INTERVAL * 9);
    auto counter = std::make_shared<Data>();
    auto id = executorPoolNew_->Schedule(
        [counter] {
            counter->Add();
        },
        wait);
    ASSERT_NE(id, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 11));
    ASSERT_GT(counter->data, 1);
    executorPoolNew_->Remove(id);
}

/**
 * @tc.name: MultiScheduleNew9
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew9, TestSize.Level0)
{
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    std::set<ExecutorPool::TaskId> ids;
    for (int i = 0; i < 11; ++i) {
        auto id = executorPoolNew_->Schedule(task, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(ids.count(id), 0);
        ids.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(data->data, 22);
    for (auto id : ids) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew9
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew9, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 10);
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    auto scheduled = executorPoolNew_->Schedule(task, delay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 9));
    ASSERT_EQ(data->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(scheduled));
    std::this_thread::sleep_for(delay * 12);
    ASSERT_EQ(data->data, 1);
}

/**
 * @tc.name: ResetNew9
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew9, TestSize.Level0)
{
    auto initial = std::chrono::milliseconds(SHORT_INTERVAL * 9);
    auto data = std::make_shared<Data>();
    auto operation = [data] {
        data->Add();
    };
    // Use single-shot task
    auto task = executorPoolNew_->Schedule(operation, initial * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(data->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(task,
        std::chrono::milliseconds(initial * 4)), task);
    std::this_thread::sleep_for(std::chrono::milliseconds(initial * 5));
    ASSERT_EQ(data->data, 2);
    executorPoolNew_->Remove(task);
}

/**
 * @tc.name: MaxEqualsOneNew9
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew9, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> value = 9;
    auto delayed = [&value] {
        value++;
    };
    auto immediate = [&value] {
        value += 10;
    };
    executor->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 10), delayed);
    ASSERT_EQ(value, 9);
    executor->Execute(immediate);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 9));
    ASSERT_EQ(value, 19);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 20));
    ASSERT_EQ(value, 20);
}

/**
 * @tc.name: RemoveInExecuteTaskNew9
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew9, TestSize.Level0)
{
    auto executors = std::make_shared<ExecutorPool>(1, 1);
    auto taskId = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> flag = 0;
    taskId = executors->Schedule(
        [executors, &taskId, &flag]() {
            flag++;
            executors->Remove(taskId, false);
            taskId = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 9), 28);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskId, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(flag, 1);
}

/**
 * @tc.name: ExecuteNew10
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ExecuteNew10, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 10);
    int start = 100;
    auto data = std::make_shared<BlockData<int>>(LONG_INTERVAL, start);
    auto task1 = executorPoolNew_->Schedule(delay, [data]() {
        int newVal = 105;
        data->SetValue(newVal);
    });
    ASSERT_EQ(data->GetValue(), 105);
    data->Clear();
    auto task2 = executorPoolNew_->Schedule(delay, [data]() {
        int newVal = 110;
        data->SetValue(newVal);
    });
    ASSERT_EQ(data->GetValue(), 110);
    ASSERT_NE(task1, task2);
}

/**
 * @tc.name: ScheduleNew10
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ScheduleNew10, TestSize.Level0)
{
    auto wait = std::chrono::milliseconds(SHORT_INTERVAL * 10);
    auto counter = std::make_shared<Data>();
    auto id = executorPoolNew_->Schedule(
        [counter] {
            counter->Add();
        },
        wait);
    ASSERT_NE(id, ExecutorPool::INVALID_TASK_ID);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 12));
    ASSERT_GT(counter->data, 1);
    executorPoolNew_->Remove(id);
}

/**
 * @tc.name: MultiScheduleNew10
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MultiScheduleNew10, TestSize.Level0)
{
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    std::set<ExecutorPool::TaskId> ids;
    for (int i = 0; i < 12; ++i) {
        auto id = executorPoolNew_->Schedule(task, std::chrono::seconds(0),
            std::chrono::milliseconds(SHORT_INTERVAL * 2), 2);
        ASSERT_EQ(ids.count(id), 0);
        ids.insert(id);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 10));
    ASSERT_EQ(data->data, 24);
    for (auto id : ids) {
        executorPoolNew_->Remove(id);
    }
}

/**
 * @tc.name: RemoveNew10
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveNew10, TestSize.Level0)
{
    auto delay = std::chrono::milliseconds(SHORT_INTERVAL * 11);
    auto data = std::make_shared<Data>();
    auto task = [data] {
        data->Add();
    };
    auto scheduled = executorPoolNew_->Schedule(task, delay * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 20));
    ASSERT_EQ(data->data, 1);
    ASSERT_TRUE(executorPoolNew_->Remove(scheduled));
    std::this_thread::sleep_for(delay * 13);
    ASSERT_EQ(data->data, 1);
}

/**
 * @tc.name: ResetNew10
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, ResetNew10, TestSize.Level0)
{
    auto initial = std::chrono::milliseconds(SHORT_INTERVAL * 10);
    auto data = std::make_shared<Data>();
    auto operation = [data] {
        data->Add();
    };
    // Use single-shot task
    auto task = executorPoolNew_->Schedule(operation, initial * 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 11));
    ASSERT_EQ(data->data, 1);
    ASSERT_EQ(executorPoolNew_->Reset(task,
        std::chrono::milliseconds(initial * 4)), task);
    std::this_thread::sleep_for(std::chrono::milliseconds(initial * 5));
    ASSERT_EQ(data->data, 2);
    executorPoolNew_->Remove(task);
}

/**
 * @tc.name: MaxEqualsOneNew10
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, MaxEqualsOneNew10, TestSize.Level0)
{
    auto executor = std::make_shared<ExecutorPool>(1, 0);
    std::atomic<int> value = 10;
    auto delayed = [&value] {
        value++;
    };
    auto immediate = [&value] {
        value += 11;
    };
    executor->Schedule(std::chrono::milliseconds(SHORT_INTERVAL * 11), delayed);
    ASSERT_EQ(value, 10);
    executor->Execute(immediate);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 20));
    ASSERT_EQ(value, 22);
    std::this_thread::sleep_for(std::chrono::milliseconds(SHORT_INTERVAL * 11));
    ASSERT_EQ(value, 22);
}

/**
 * @tc.name: RemoveInExecuteTaskNew10
 * @tc.desc: test remove task when task is running.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: NewTest
 */
HWTEST_F(ExecutorPoolTestNew, RemoveInExecuteTaskNew10, TestSize.Level0)
{
    auto executors = std::make_shared<ExecutorPool>(1, 1);
    auto taskId = ExecutorPool::INVALID_TASK_ID;
    std::atomic<int> flag = 0;
    taskId = executors->Schedule(
        [executors, &taskId, &flag]() {
            flag++;
            executors->Remove(taskId, false);
            taskId = ExecutorPool::INVALID_TASK_ID;
        },
        std::chrono::seconds(0), std::chrono::milliseconds(SHORT_INTERVAL * 10), 30);
    std::this_thread::sleep_for(std::chrono::seconds(LONG_INTERVAL));
    ASSERT_EQ(taskId, ExecutorPool::INVALID_TASK_ID);
    ASSERT_EQ(flag, 1);
}

} // namespace OHOS::Test