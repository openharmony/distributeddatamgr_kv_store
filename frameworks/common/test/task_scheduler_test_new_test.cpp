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

#define LOG_TAG "TaskSchedulerNew"

#include "task_scheduler.h"
#include <chrono>
#include <gtest/gtest.h>

#include "block_data.h"
namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS;
using Duration = std::chrono::steady_clock::duration;
class TaskSchedulerNewTest : public testing::Test {
public:
    static constexpr uint32_t shortInterval = 100;
    static constexpr uint32_t longInterval = 1;
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() { }
};

HWTEST_F(TaskSchedulerNewTest, At001, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest001");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 10;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 11;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 11);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 12;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 12);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At002, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest002");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 20;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 21;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 21);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 22;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 22);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At003, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest003");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 30;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 31;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 31);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 32;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 32);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At004, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest004");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 40;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 41;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 41);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 42;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 42);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At005, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest005");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 50;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 51;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 51);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 52;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 52);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At006, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest006");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 60;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 61;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 61);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 62;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 62);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At007, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest007");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 70;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 71;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 71);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 72;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 72);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At008, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest008");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 80;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 81;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 81);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 82;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 82);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At009, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest009");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 90;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 91;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 91);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 92;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 92);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At010, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest010");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 100;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 101;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 101);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 102;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 102);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At011, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest011");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 110;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 111;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 111);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 112;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 112);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At012, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest012");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 120;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 121;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 121);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 122;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 122);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At013, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest013");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 130;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 131;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 131);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 132;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 132);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At014, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest014");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 140;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 141;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 141);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 142;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 142);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At015, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest015");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 150;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 151;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 151);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 152;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 152);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At016, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest016");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 160;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 161;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 161);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 162;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 162);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At017, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest017");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 170;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 171;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 171);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 172;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 172);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At018, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest018");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 180;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 181;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 181);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 182;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 182);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At019, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest019");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 190;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 191;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 191);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 192;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 192);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At020, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest020");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 200;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 201;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 201);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 202;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 202);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At021, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest021");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 210;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 211;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 211);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 212;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 212);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At022, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest022");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 220;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 221;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 221);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 222;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 222);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At023, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest023");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 230;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 231;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 231);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 232;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 232);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At024, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest024");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 240;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 241;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 241);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 242;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 242);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At025, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest025");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 250;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 251;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 251);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 252;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 252);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At026, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest026");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 260;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 261;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 261);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 262;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 262);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At027, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest027");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 270;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 271;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 271);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 272;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 272);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At028, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest028");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 280;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 281;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 281);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 282;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 282);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At029, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest029");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 290;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 291;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 291);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 292;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 292);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At030, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest030");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 300;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 301;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 301);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 302;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 302);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At031, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest031");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 310;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 311;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 311);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 312;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 312);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At032, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest032");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 320;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 321;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 321);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 322;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 322);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At033, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest033");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 330;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 331;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 331);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 332;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 332);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At034, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest034");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 340;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 341;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 341);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 342;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 342);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At035, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest035");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 350;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 351;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 351);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 352;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 352);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At036, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest036");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 360;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 361;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 361);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 362;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 362);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At037, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest037");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 370;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 371;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 371);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 372;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 372);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At038, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest038");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 380;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 381;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 381);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 382;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 382);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At039, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest039");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 390;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 391;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 391);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 392;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 392);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At040, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest040");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 400;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 401;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 401);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 402;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 402);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At041, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest041");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 410;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 411;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 411);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 412;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 412);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At042, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest042");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 420;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 421;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 421);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 422;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 422);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At043, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest043");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 430;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 431;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 431);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 432;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 432);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At044, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest044");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 440;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 441;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 441);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 442;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 442);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At045, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest045");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 450;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 451;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 451);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 452;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 452);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At046, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest046");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 460;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 461;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 461);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 462;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 462);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At047, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest047");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 470;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 471;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 471);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 472;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 472);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At048, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest048");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 480;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 481;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 481);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 482;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 482);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At049, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest049");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 490;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 491;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 491);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 492;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 492);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At050, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest050");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 500;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 501;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 501);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 502;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 502);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At051, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest051");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 510;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 511;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 511);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 512;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 512);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At052, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest052");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 520;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 521;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 521);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 522;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 522);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At053, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest053");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 530;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 531;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 531);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 532;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 532);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At054, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest054");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 540;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 541;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 541);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 542;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 542);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At055, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest055");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 550;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 551;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 551);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 552;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 552);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At056, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest056");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 560;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 561;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 561);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 562;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 562);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At057, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest057");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 570;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 571;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 571);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 572;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 572);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At058, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest058");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 580;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 581;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 581);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 582;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 582);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At059, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest059");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 590;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 591;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 591);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 592;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 592);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At060, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest060");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 600;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 601;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 601);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 602;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 602);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At061, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest061");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 610;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 611;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 611);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 612;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 612);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At062, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest062");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 620;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 621;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 621);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 622;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 622);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At063, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest063");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 630;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 631;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 631);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 632;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 632);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At064, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest064");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 640;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 641;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 641);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 642;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 642);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At065, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest065");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 650;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 651;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 651);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 652;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 652);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At066, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest066");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 660;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 661;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 661);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 662;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 662);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At067, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest067");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 670;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 671;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 671);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 672;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 672);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At068, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest068");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 680;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 681;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 681);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 682;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 682);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At069, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest069");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 690;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 691;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 691);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 692;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 692);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At070, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest070");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 700;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 701;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 701);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 702;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 702);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At071, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest071");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 710;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 711;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 711);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 712;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 712);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At072, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest072");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 720;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 721;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 721);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 722;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 722);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At073, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest073");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 730;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 731;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 731);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 732;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 732);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At074, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest074");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 740;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 741;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 741);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 742;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 742);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At075, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest075");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 750;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 751;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 751);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 752;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 752);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At076, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest076");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 760;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 761;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 761);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 762;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 762);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At077, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest077");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 770;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 771;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 771);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 772;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 772);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At078, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest078");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 780;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 781;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 781);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 782;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 782);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At079, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest079");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 790;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 791;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 791);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 792;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 792);
    ASSERT_NE(atTaskId1, atTaskId2);
}

HWTEST_F(TaskSchedulerNewTest, At080, TestSize.Level0)
{
    TaskScheduler taskScheduler("atTest080");
    auto expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    int testData = 800;
    auto blockData = std::make_shared<BlockData<int>>(longInterval, testData);
    auto atTaskId1 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 801;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 801);
    blockData->Clear();
    expiredTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(shortInterval);
    auto atTaskId2 = taskScheduler.At(expiredTime, [blockData]() {
        int testData = 802;
        blockData->SetValue(testData);
    });
    ASSERT_EQ(blockData->GetValue(), 802);
    ASSERT_NE(atTaskId1, atTaskId2);
}
} // namespace OHOS::Test
