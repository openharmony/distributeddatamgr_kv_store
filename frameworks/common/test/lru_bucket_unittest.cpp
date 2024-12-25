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

#define LOG_TAG "LRUBucketUnitTest"

#include "lru_bucket.h"
#include "gtest/gtest.h"

namespace OHOS::Test {
using namespace testing::ext;
template <typename _Key, typename _Tp>
using LRUBucket = OHOS::LRUBucket<_Key, _Tp>;

class LRUBucketUnitTest : public testing::Test {
public:
    struct TestValueNode {
        std::string id;
        std::string name;
        std::string testCase;
    };
    static constexpr size_t TESTCAPACITY = 10;

    static void SetUpTestCase(void)
    {
    }

    static void TearDownTestCase(void)
    {
    }

protected:
    void SetUp()
    {
        bucket.ResetCapacity(0);
        bucket.ResetCapacity(TESTCAPACITY);
        for (size_t i = 0; i < TESTCAPACITY; ++i) {
            std::string key = std::string("unittest_") + std::to_string(i);
            TestValueNode valueTest = { key, key, "case" };
            bucket.Set(key, valueTest);
        }
    }

    void TearDown() { }

    LRUBucket<std::string, TestValueNode> bucket { TESTCAPACITY };
};

/**
 * @tc.name: insert
 * @tc.desc: Set the valueTest to the lru bucket, whose capacity is more than one.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, insertTest, TestSize.Level0)
{
    bucket.Set("unittest_10", { "unittest_10", "unittest_10", "case" });
    TestValueNode valueTest;
    EXPECT_TRUE(!bucket.Get("unittest_0", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_6", valueTest));
    EXPECT_TRUE(bucket.ResetCapacity(1));
    EXPECT_TRUE(bucket.Capacity() == 1);
    EXPECT_TRUE(bucket.Size() <= 1);
    EXPECT_TRUE(bucket.Get("unittest_6", valueTest));
}

/**
 * @tc.name: cap_one_insert
 * @tc.desc: Set the valueTest to the lru bucket, whose capacity is one.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, cap_one_insertTest, TestSize.Level0)
{
    bucket.ResetCapacity(1);
    for (size_t i = 0; i <= TESTCAPACITY; ++i) {
        std::string key = std::string("unittest_") + std::to_string(i);
        TestValueNode valueTest = { key, key, "find" };
        bucket.Set(key, valueTest);
    }
    TestValueNode valueTest;
    EXPECT_TRUE(!bucket.Get("unittest_0", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_10", valueTest));
}

/**
 * @tc.name: cap_zero_insert
 * @tc.desc: Set the valueTest to the lru bucket, whose capacity is zero.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, cap_zero_insertTest, TestSize.Level0)
{
    bucket.ResetCapacity(0);
    for (size_t i = 0; i <= TESTCAPACITY; ++i) {
        std::string key = std::string("unittest_") + std::to_string(i);
        TestValueNode valueTest = { key, key, "find" };
        bucket.Set(key, valueTest);
    }
    TestValueNode valueTest;
    EXPECT_TRUE(!bucket.Get("unittest_10", valueTest));
}

/**
 * @tc.name: find_head
 * @tc.desc: find the head element from the lru bucket.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, find_headTest, TestSize.Level0)
{
    TestValueNode valueTest;
    EXPECT_TRUE(bucket.ResetCapacity(1));
    EXPECT_TRUE(bucket.Capacity() == 1);
    EXPECT_TRUE(bucket.Size() <= 1);
    EXPECT_TRUE(bucket.Get("unittest_9", valueTest));
}

/**
 * @tc.name: find_tail
 * @tc.desc: find the tail element, then the element will move to head.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, find_tailTest, TestSize.Level0)
{
    TestValueNode valueTest;
    EXPECT_TRUE(bucket.Get("unittest_0", valueTest));
    EXPECT_TRUE(bucket.ResetCapacity(1));
    EXPECT_TRUE(bucket.Capacity() == 1);
    EXPECT_TRUE(bucket.Size() <= 1);
    EXPECT_TRUE(bucket.Get("unittest_0", valueTest));
}

/**
 * @tc.name: find_mid
 * @tc.desc: find the mid element, then the element will move to head.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, find_midTest, TestSize.Level0)
{
    TestValueNode valueTest;
    EXPECT_TRUE(bucket.Get("unittest_5", valueTest));
    EXPECT_TRUE(bucket.ResetCapacity(1));
    EXPECT_TRUE(bucket.Capacity() == 1);
    EXPECT_TRUE(bucket.Size() <= 1);
    EXPECT_TRUE(bucket.Get("unittest_5", valueTest));
}

/**
 * @tc.name: find_and_insert
 * @tc.desc: find the tail element, then the element will move to head.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, find_and_insertTest, TestSize.Level0)
{
    TestValueNode valueTest;
    if (!bucket.Get("MyTest", valueTest)) {
        bucket.Set("MyTest", { "MyTest", "MyTest", "case" });
    }
    EXPECT_TRUE(bucket.Get("MyTest", valueTest));

    if (!bucket.Get("unittest_0", valueTest)) {
        bucket.Set("unittest_0", { "unittest_0", "unittest_0", "case" });
    }
    EXPECT_TRUE(bucket.Get("unittest_0", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_5", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_4", valueTest));
    EXPECT_TRUE(!bucket.Get("unittest_1", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_2", valueTest));
}

/**
 * @tc.name: del_head
 * @tc.desc: delete the head element, then the next element will move to head.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, del_headTest, TestSize.Level0)
{
    TestValueNode valueTest;
    EXPECT_TRUE(bucket.Delete("unittest_9"));
    EXPECT_TRUE(!bucket.Get("unittest_9", valueTest));
    EXPECT_TRUE(bucket.ResetCapacity(1));
    EXPECT_TRUE(bucket.Capacity() == 1);
    EXPECT_TRUE(bucket.Size() <= 1);
    EXPECT_TRUE(bucket.Get("unittest_8", valueTest));
}

/**
 * @tc.name: del_head
 * @tc.desc: delete the tail element, then the lru chain keep valid.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, del_tailTest, TestSize.Level0)
{
    TestValueNode valueTest;
    EXPECT_TRUE(bucket.Delete("unittest_0"));
    EXPECT_TRUE(!bucket.Get("unittest_0", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_4", valueTest));
    EXPECT_TRUE(bucket.ResetCapacity(1));
    EXPECT_TRUE(bucket.Capacity() == 1);
    EXPECT_TRUE(bucket.Size() <= 1);
    EXPECT_TRUE(bucket.Get("unittest_4", valueTest));
}

/**
 * @tc.name: del_mid
 * @tc.desc: delete the mid element, then the lru chain keep valid.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, del_midTest, TestSize.Level0)
{
    TestValueNode valueTest;
    EXPECT_TRUE(bucket.Delete("unittest_5"));
    EXPECT_TRUE(!bucket.Get("unittest_5", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_4", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_6", valueTest));
    EXPECT_TRUE(bucket.ResetCapacity(2));
    EXPECT_TRUE(bucket.Capacity() == 2);
    EXPECT_TRUE(bucket.Size() <= 2);
    EXPECT_TRUE(bucket.Get("unittest_4", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_6", valueTest));
}

/**
 * @tc.name: del_mid
 * @tc.desc: the lru bucket has only one element, then delete it.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, cap_one_delTest, TestSize.Level0)
{
    TestValueNode valueTest;
    bucket.ResetCapacity(1);
    EXPECT_TRUE(bucket.Delete("unittest_9"));
    EXPECT_TRUE(!bucket.Get("unittest_9", valueTest));
}

/**
 * @tc.name: del_mid
 * @tc.desc: the lru bucket has no element.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, cap_zero_delTest, TestSize.Level0)
{
    TestValueNode valueTest;
    bucket.ResetCapacity(0);
    EXPECT_TRUE(!bucket.Delete("unittest_9"));
    EXPECT_TRUE(!bucket.Get("unittest_9", valueTest));
}

/**
 * @tc.name: update_one
 * @tc.desc: update the valueTest and the lru chain won't change.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, update_oneTest, TestSize.Level0)
{
    TestValueNode valueTest;
    EXPECT_TRUE(bucket.Update("unittest_4", { "unittest_4", "unittest_4", "update" }));
    EXPECT_TRUE(bucket.Get("unittest_4", valueTest));
    EXPECT_TRUE(valueTest.testCase == "update");
    EXPECT_TRUE(bucket.Update("unittest_9", { "unittest_9", "unittest_9", "update" }));
    EXPECT_TRUE(bucket.ResetCapacity(1));
    EXPECT_TRUE(bucket.Capacity() == 1);
    EXPECT_TRUE(bucket.Size() <= 1);
    EXPECT_TRUE(bucket.Get("unittest_4", valueTest));
    EXPECT_TRUE(!bucket.Get("unittest_9", valueTest));
}

/**
 * @tc.name: update_several
 * @tc.desc: update several values and the lru chain won't change.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(LRUBucketUnitTest, update_severalTest, TestSize.Level0)
{
    TestValueNode valueTest;
    std::map<std::string, TestValueNode> values = {
        { "unittest_2", { "unittest_2", "unittest_2", "update" } },
        { "unittest_3", { "unittest_3", "unittest_3", "update" } },
        { "unittest_6", { "unittest_6", "unittest_6", "update" } }
    };
    EXPECT_TRUE(bucket.Update(values));
    EXPECT_TRUE(bucket.ResetCapacity(3));
    EXPECT_TRUE(bucket.Capacity() == 3);
    EXPECT_TRUE(bucket.Size() <= 3);
    EXPECT_TRUE(!bucket.Get("unittest_2", valueTest));
    EXPECT_TRUE(!bucket.Get("unittest_3", valueTest));
    EXPECT_TRUE(!bucket.Get("unittest_6", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_9", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_8", valueTest));
    EXPECT_TRUE(bucket.Get("unittest_7", valueTest));
}
} // namespace OHOS::Test