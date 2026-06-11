/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include "db_errno.h"
#include "log_print.h"
#include "schema_utils.h"
#include "unique_queue.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

const static int BATCH_NUM = 100;
class DistributedDBUniqueQueue : public testing::Test {
public:
    static void SetUpTestCase(void)
    {}
    static void TearDownTestCase(void)
    {}
    void SetUp();
    void TearDown();
    struct TestUniqueHash {
        size_t operator ()(const int key) const
        {
            return std::hash<int>{}(key);
        }
    };

    class TestUniqueQueue : public UniqueQueue<int, TestUniqueHash, std::equal_to<int>> {};
    TestUniqueQueue queue;
    void PushBatchTest(size_t num = BATCH_NUM, int ret = E_OK);
    void ReadBatchTest(size_t readStart = 0, size_t maxNum = 1, bool success = true);
    void AdvanceFrontTest(size_t maxNum = 1, bool noLog = true);
};

void DistributedDBUniqueQueue::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    queue.Init(queue.INIT_CAP, 0);
}

void DistributedDBUniqueQueue::TearDown(void)
{}

void DistributedDBUniqueQueue::PushBatchTest(size_t num, int ret)
{
    static const int batchNum = BATCH_NUM;
    size_t loop = num / batchNum;
    size_t last = num % batchNum;
    int errCode = E_OK;
    int data[batchNum]{};
    for (size_t i = 0; i < loop; i++) {
        for (size_t j = 0; j < batchNum; j++) {
            data[j] = (j + batchNum * i);
        }
        errCode = queue.PushBatch(data, batchNum);
    }
    for (size_t k = 0; k < last; k++) {
        data[k] = (k + batchNum * loop);
    }
    errCode = queue.PushBatch(data, last);
    ASSERT_EQ(errCode, ret);
}

void DistributedDBUniqueQueue::ReadBatchTest(size_t readStart, size_t maxNum, bool success)
{
    static const int batchNum = BATCH_NUM;
    std::vector<int> data;
    data.reserve(batchNum);
    (void)queue.TryInitCursor(readStart);
    int readNum = queue.ReadBatch(data, maxNum);
    if (success) {
        ASSERT_EQ(readNum, maxNum);
    } else {
        ASSERT_NE(readNum, maxNum);
    }
}

void DistributedDBUniqueQueue::AdvanceFrontTest(size_t maxNum, bool noLog)
{
    queue.AdvanceFront(maxNum);
}

HWTEST_F(DistributedDBUniqueQueue, FunctionTest_PushBatch_001, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
}

HWTEST_F(DistributedDBUniqueQueue, FunctionTest_ReadBatch_002, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
    ReadBatchTest(0, 100);
}

HWTEST_F(DistributedDBUniqueQueue, FunctionTest_AdvanceFront_003, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
    ReadBatchTest(0, 100);
    AdvanceFrontTest(100);
}

HWTEST_F(DistributedDBUniqueQueue, FunctionTest_ConflictWithReadCache_004, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
    size_t len1 = queue.QueueSize();
    ReadBatchTest(0, 100);
    PushBatchTest(100, E_OK);
    size_t len2 = queue.QueueSize();
    ASSERT_EQ(len2, len1 + 100);
    AdvanceFrontTest(100);
    size_t len3 = queue.QueueSize();
    ASSERT_EQ(len3, 100);
    ReadBatchTest(100, 100);
    AdvanceFrontTest(100);
    size_t len4 = queue.QueueSize();
    ASSERT_EQ(len4, 0);
}

HWTEST_F(DistributedDBUniqueQueue, FunctionTest_UpdateRemainRead_005, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
    size_t len1 = queue.QueueSize();
    PushBatchTest(100, E_OK);
    size_t len2 = queue.QueueSize();
    ASSERT_EQ(len2, len1);
    ReadBatchTest(0, 100);
    AdvanceFrontTest(100);
    size_t len3 = queue.QueueSize();
    ASSERT_EQ(len3, 0);
}

HWTEST_F(DistributedDBUniqueQueue, DfxTest_PushFull_001, TestSize.Level0)
{
    PushBatchTest(queue.MAX_CAP - 1, E_OK);
    ASSERT_EQ(queue.IsFull(), true);
}

HWTEST_F(DistributedDBUniqueQueue, DfxTest_PushBatchOutRange_002, TestSize.Level0)
{
    PushBatchTest(queue.MAX_CAP, E_MAX_LIMITS);
    ASSERT_EQ(queue.IsFull(), false);
}

HWTEST_F(DistributedDBUniqueQueue, DfxTest_ReadBatchBigger_003, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
    ReadBatchTest(0, 101, false);
}

HWTEST_F(DistributedDBUniqueQueue, DfxTest_ReadBatchSmallerFailed_004, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
    ReadBatchTest(0, 99);
}

HWTEST_F(DistributedDBUniqueQueue, DfxTest_AdvanceFrontBiggerFailed_005, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
    ReadBatchTest(0, 100);
    AdvanceFrontTest(101, false);
}

HWTEST_F(DistributedDBUniqueQueue, DfxTest_AdvanceFrontSmaller_006, TestSize.Level0)
{
    PushBatchTest(100, E_OK);
    ReadBatchTest(0, 100);
    AdvanceFrontTest(99);
}
