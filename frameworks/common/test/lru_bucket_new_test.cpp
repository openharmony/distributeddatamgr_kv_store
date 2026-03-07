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

#define LOG_TAG "LRUBucketTestNew"

#include "lru_bucket.h"
#include "gtest/gtest.h"
namespace OHOS::Test {
using namespace testing::ext;
template <typename _Key, typename _Tp>
using LRUBucket = OHOS::LRUBucket<_Key, _Tp>;

class LRUBucketTestNew : public testing::Test {
public:
    struct TestValueAlpha {
        std::string identifier;
        std::string label;
        std::string scenario;
    };
    static constexpr size_t TEST_CAPACITY = 10;

    static void SetUpTestCase(void) { }

    static void TearDownTestCase(void) { }

protected:
    void SetUp()
    {
        bucketAlpha_.ResetCapacity(0);
        bucketAlpha_.ResetCapacity(TEST_CAPACITY);
        for (size_t i = 0; i < TEST_CAPACITY; ++i) {
            std::string key = std::string("alpha_") + std::to_string(i);
            TestValueAlpha value = { key, key, "alpha_case" };
            bucketAlpha_.Set(key, value);
        }
    }

    void TearDown() { }

    LRUBucket<std::string, TestValueAlpha> bucketAlpha_ { TEST_CAPACITY };
};

/**
 * @tc.name: insertNew
 * @tc.desc: Set the value to the lru bucket with capacity more than one
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, insertNew, TestSize.Level0)
{
    bucketAlpha_.Set("alpha_10", { "alpha_10", "alpha_10", "alpha_case" });
    TestValueAlpha value;
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_0", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_6", value));
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(1));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 1);
    ASSERT_TRUE(bucketAlpha_.Size() <= 1);
    ASSERT_TRUE(bucketAlpha_.Get("alpha_6", value));
}

/**
 * @tc.name: capOneInsertNew
 * @tc.desc: Set the value to the lru bucket with capacity one
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, capOneInsertNew, TestSize.Level0)
{
    bucketAlpha_.ResetCapacity(1);
    for (size_t i = 0; i <= TEST_CAPACITY; ++i) {
        std::string key = std::string("alpha_") + std::to_string(i);
        TestValueAlpha value = { key, key, "alpha_find" };
        bucketAlpha_.Set(key, value);
    }
    TestValueAlpha value;
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_0", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_10", value));
}

/**
 * @tc.name: capZeroInsertNew
 * @tc.desc: Set the value to the lru bucket with capacity zero
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, capZeroInsertNew, TestSize.Level0)
{
    bucketAlpha_.ResetCapacity(0);
    for (size_t i = 0; i <= TEST_CAPACITY; ++i) {
        std::string key = std::string("alpha_") + std::to_string(i);
        TestValueAlpha value = { key, key, "alpha_find" };
        bucketAlpha_.Set(key, value);
    }
    TestValueAlpha value;
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_10", value));
}

/**
 * @tc.name: findHeadNew
 * @tc.desc: find the head element from the lru bucket
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, findHeadNew, TestSize.Level0)
{
    TestValueAlpha value;
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(1));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 1);
    ASSERT_TRUE(bucketAlpha_.Size() <= 1);
    ASSERT_TRUE(bucketAlpha_.Get("alpha_9", value));
}

/**
 * @tc.name: findTailNew
 * @tc.desc: find the tail element then the element will move to head
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, findTailNew, TestSize.Level0)
{
    TestValueAlpha value;
    ASSERT_TRUE(bucketAlpha_.Get("alpha_0", value));
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(1));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 1);
    ASSERT_TRUE(bucketAlpha_.Size() <= 1);
    ASSERT_TRUE(bucketAlpha_.Get("alpha_0", value));
}

/**
 * @tc.name: findMidNew
 * @tc.desc: find the mid element then the element will move to head
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, findMidNew, TestSize.Level0)
{
    TestValueAlpha value;
    ASSERT_TRUE(bucketAlpha_.Get("alpha_5", value));
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(1));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 1);
    ASSERT_TRUE(bucketAlpha_.Size() <= 1);
    ASSERT_TRUE(bucketAlpha_.Get("alpha_5", value));
}

/**
 * @tc.name: findAndInsertNew
 * @tc.desc: find the tail element then the element will move to head
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, findAndInsertNew, TestSize.Level0)
{
    TestValueAlpha value;
    if (!bucketAlpha_.Get("AlphaTest", value)) {
        bucketAlpha_.Set("AlphaTest", { "AlphaTest", "AlphaTest", "alpha_case" });
    }
    ASSERT_TRUE(bucketAlpha_.Get("AlphaTest", value));
    if (!bucketAlpha_.Get("alpha_0", value)) {
        bucketAlpha_.Set("alpha_0", { "alpha_0", "alpha_0", "alpha_case" });
    }
    ASSERT_TRUE(bucketAlpha_.Get("alpha_0", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_5", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_4", value));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_1", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_2", value));
}

/**
 * @tc.name: delHeadNew
 * @tc.desc: delete the head element then the next element will move to head
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, delHeadNew, TestSize.Level0)
{
    TestValueAlpha value;
    ASSERT_TRUE(bucketAlpha_.Delete("alpha_9"));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_9", value));
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(1));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 1);
    ASSERT_TRUE(bucketAlpha_.Size() <= 1);
    ASSERT_TRUE(bucketAlpha_.Get("alpha_8", value));
}

/**
 * @tc.name: delTailNew
 * @tc.desc: delete the tail element then the lru chain keep valid
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, delTailNew, TestSize.Level0)
{
    TestValueAlpha value;
    ASSERT_TRUE(bucketAlpha_.Delete("alpha_0"));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_0", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_4", value));
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(1));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 1);
    ASSERT_TRUE(bucketAlpha_.Size() <= 1);
    ASSERT_TRUE(bucketAlpha_.Get("alpha_4", value));
}

/**
 * @tc.name: delMidNew
 * @tc.desc: delete the mid element then the lru chain keep valid
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, delMidNew, TestSize.Level0)
{
    TestValueAlpha value;
    ASSERT_TRUE(bucketAlpha_.Delete("alpha_5"));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_5", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_4", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_6", value));
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(2));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 2);
    ASSERT_TRUE(bucketAlpha_.Size() <= 2);
    ASSERT_TRUE(bucketAlpha_.Get("alpha_4", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_6", value));
}

/**
 * @tc.name: capOneDelNew
 * @tc.desc: the lru bucket has only one element then delete it
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, capOneDelNew, TestSize.Level0)
{
    TestValueAlpha value;
    bucketAlpha_.ResetCapacity(1);
    ASSERT_TRUE(bucketAlpha_.Delete("alpha_9"));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_9", value));
}

/**
 * @tc.name: capZeroDelNew
 * @tc.desc: the lru bucket has no element
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, capZeroDelNew, TestSize.Level0)
{
    TestValueAlpha value;
    bucketAlpha_.ResetCapacity(0);
    ASSERT_TRUE(!bucketAlpha_.Delete("alpha_9"));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_9", value));
}

/**
 * @tc.name: updateOneNew
 * @tc.desc: update the value and the lru chain will not change
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, updateOneNew, TestSize.Level0)
{
    TestValueAlpha value;
    ASSERT_TRUE(bucketAlpha_.Update("alpha_4", { "alpha_4", "alpha_4", "alpha_update" }));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_4", value));
    ASSERT_TRUE(value.scenario == "alpha_update");
    ASSERT_TRUE(bucketAlpha_.Update("alpha_9", { "alpha_9", "alpha_9", "alpha_update" }));
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(1));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 1);
    ASSERT_TRUE(bucketAlpha_.Size() <= 1);
    ASSERT_TRUE(bucketAlpha_.Get("alpha_4", value));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_9", value));
}

/**
 * @tc.name: updateSeveralNew
 * @tc.desc: update several values and the lru chain will not change
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, updateSeveralNew, TestSize.Level0)
{
    TestValueAlpha value;
    std::map<std::string, TestValueAlpha> values = {
        { "alpha_2", { "alpha_2", "alpha_2", "alpha_update" } },
        { "alpha_3", { "alpha_3", "alpha_3", "alpha_update" } },
        { "alpha_6", { "alpha_6", "alpha_6", "alpha_update" } }
    };
    ASSERT_TRUE(bucketAlpha_.Update(values));
    ASSERT_TRUE(bucketAlpha_.ResetCapacity(3));
    ASSERT_TRUE(bucketAlpha_.Capacity() == 3);
    ASSERT_TRUE(bucketAlpha_.Size() <= 3);
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_2", value));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_3", value));
    ASSERT_TRUE(!bucketAlpha_.Get("alpha_6", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_9", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_8", value));
    ASSERT_TRUE(bucketAlpha_.Get("alpha_7", value));
}

/**
 * @tc.name: containsWithChangeNew
 * @tc.desc: contains the key and change the lru position
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, containsWithChangeNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceAlpha0", "deviceAlpha1", "deviceAlpha2", "deviceAlpha3", "deviceAlpha4",
        "deviceAlpha5", "deviceAlpha6", "deviceAlpha7", "deviceAlpha8", "deviceAlpha9",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [exists0, changed0] = bucket.Contains("deviceAlpha0");
    ASSERT_TRUE(exists0);
    ASSERT_TRUE(changed0);
    auto [keyMemo0, valMemo0] = bucket.DumpMemento();
    std::vector<std::string> keyTag = {
        "deviceAlpha1", "deviceAlpha2", "deviceAlpha3", "deviceAlpha4", "deviceAlpha5",
        "deviceAlpha6", "deviceAlpha7", "deviceAlpha8", "deviceAlpha9", "deviceAlpha0",
    };
    std::vector<uint32_t> valTag = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    EXPECT_EQ(keyMemo0, keyTag);
    EXPECT_EQ(valMemo0, valTag);
    auto [exists, changed] = bucket.Contains("deviceAlpha5");
    ASSERT_TRUE(exists);
    ASSERT_TRUE(changed);
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    keyTag = {
        "deviceAlpha1", "deviceAlpha2", "deviceAlpha3", "deviceAlpha4", "deviceAlpha6",
        "deviceAlpha7", "deviceAlpha8", "deviceAlpha9", "deviceAlpha0", "deviceAlpha5",
    };
    valTag = { 1, 2, 3, 4, 6, 7, 8, 9, 0, 5 };
    EXPECT_EQ(keyMemo, keyTag);
    EXPECT_EQ(valMemo, valTag);
}

/**
 * @tc.name: containsWithChangeBaseFiveNew
 * @tc.desc: contains the key and change the lru position base five
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, containsWithChangeBaseFiveNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceBeta0", "deviceBeta1", "deviceBeta2", "deviceBeta3", "deviceBeta4",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [exists0, changed0] = bucket.Contains("deviceBeta0");
    ASSERT_TRUE(exists0);
    ASSERT_TRUE(changed0);
    auto [keyMemo0, valMemo0] = bucket.DumpMemento();
    std::vector<std::string> keyTag = {
        "deviceBeta1", "deviceBeta2", "deviceBeta3", "deviceBeta4", "deviceBeta0",
    };
    std::vector<uint32_t> valTag = { 1, 2, 3, 4, 0 };
    EXPECT_EQ(keyMemo0, keyTag);
    EXPECT_EQ(valMemo0, valTag);
    auto [exists1, changed1] = bucket.Contains("deviceBeta3");
    ASSERT_TRUE(exists1);
    ASSERT_TRUE(changed1);
    auto [keyMemo1, valMemo1] = bucket.DumpMemento();
    keyTag = {
        "deviceBeta1", "deviceBeta2", "deviceBeta4", "deviceBeta0", "deviceBeta3",
    };
    valTag = { 1, 2, 4, 0, 3 };
    EXPECT_EQ(keyMemo1, keyTag);
    EXPECT_EQ(valMemo1, valTag);
}

/**
 * @tc.name: containsWithNoChangeNew
 * @tc.desc: contains the key and not change the lru position
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, containsWithNoChangeNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceGamma0", "deviceGamma1", "deviceGamma2", "deviceGamma3", "deviceGamma4",
        "deviceGamma5", "deviceGamma6", "deviceGamma7", "deviceGamma8", "deviceGamma9",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [exists, changed] = bucket.Contains("deviceGamma9");
    ASSERT_TRUE(exists);
    ASSERT_FALSE(changed);
    auto [keyMemo, valueMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, keys);
    EXPECT_EQ(valueMemo, values);
}

/**
 * @tc.name: containsWithNoChangeBaseFiveNew
 * @tc.desc: contains the key and not change the lru position base five
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, containsWithNoChangeBaseFiveNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceDelta0", "deviceDelta1", "deviceDelta2", "deviceDelta3", "deviceDelta4",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [exists, changed] = bucket.Contains("deviceDelta4");
    ASSERT_TRUE(exists);
    ASSERT_FALSE(changed);
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, keys);
    EXPECT_EQ(valMemo, values);
}

/**
 * @tc.name: notContainsNew
 * @tc.desc: not contains the key
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, notContainsNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceEpsilon0", "deviceEpsilon1", "deviceEpsilon2", "deviceEpsilon3", "deviceEpsilon4",
        "deviceEpsilon5", "deviceEpsilon6", "deviceEpsilon7", "deviceEpsilon8", "deviceEpsilon9",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [exists, changed] = bucket.Contains("deviceEpsilon10");
    ASSERT_FALSE(exists);
    ASSERT_FALSE(changed);
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, keys);
    EXPECT_EQ(valMemo, values);
}

/**
 * @tc.name: notContainsBaseFiveNew
 * @tc.desc: not contains the key base five
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, notContainsBaseFiveNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceZeta0", "deviceZeta1", "deviceZeta2", "deviceZeta3", "deviceZeta4",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [exists, changed] = bucket.Contains("deviceZeta10");
    ASSERT_FALSE(exists);
    ASSERT_FALSE(changed);
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, keys);
    EXPECT_EQ(valMemo, values);
}

/**
 * @tc.name: notContainsAndSetNew
 * @tc.desc: not contains the key and set the new value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, notContainsAndSetNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceEta0", "deviceEta1", "deviceEta2", "deviceEta3", "deviceEta4",
        "deviceEta5", "deviceEta6", "deviceEta7", "deviceEta8", "deviceEta9",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [exists, changed] = bucket.Contains("deviceEta10");
    ASSERT_FALSE(exists);
    ASSERT_FALSE(changed);
    success = bucket.Set("deviceEta10", 10);
    ASSERT_TRUE(success);
    std::vector<std::string> keyTag = {
        "deviceEta1", "deviceEta2", "deviceEta3", "deviceEta4", "deviceEta5",
        "deviceEta6", "deviceEta7", "deviceEta8", "deviceEta9", "deviceEta10",
    };
    std::vector<uint32_t> valTag = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, keyTag);
    EXPECT_EQ(valMemo, valTag);
}

/**
 * @tc.name: notContainsAndSetBaseFiveNew
 * @tc.desc: not contains the key and set the new value base five
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, notContainsAndSetBaseFiveNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceTheta0", "deviceTheta1", "deviceTheta2", "deviceTheta3", "deviceTheta4",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [exists, changed] = bucket.Contains("deviceTheta10");
    ASSERT_FALSE(exists);
    ASSERT_FALSE(changed);
    success = bucket.Set("deviceTheta10", 10);
    ASSERT_TRUE(success);
    std::vector<std::string> result = {
        "deviceTheta0", "deviceTheta1", "deviceTheta2", "deviceTheta3", "deviceTheta4", "deviceTheta10",
    };
    std::vector<uint32_t> valResult = { 0, 1, 2, 3, 4, 10 };
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, result);
    EXPECT_EQ(valMemo, valResult);
    success = bucket.Set("deviceTheta4", 4);
    ASSERT_TRUE(success);
    result = {
        "deviceTheta0", "deviceTheta1", "deviceTheta2", "deviceTheta3", "deviceTheta10", "deviceTheta4",
    };
    valResult = { 0, 1, 2, 3, 10, 4 };
    auto [newKeyMemo, newValMemo] = bucket.DumpMemento();
    EXPECT_EQ(newKeyMemo, result);
    EXPECT_EQ(newValMemo, valResult);
}

/**
 * @tc.name: initializeNew
 * @tc.desc: use the Memento to init the lru
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, initializeNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceIota0", "deviceIota1", "deviceIota2", "deviceIota3", "deviceIota4",
        "deviceIota5", "deviceIota6", "deviceIota7", "deviceIota8", "deviceIota9",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, keys);
    EXPECT_EQ(valMemo, values);
}

/**
 * @tc.name: initializeNotEnoughNew
 * @tc.desc: use the Memento to init the lru not enough
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, initializeNotEnoughNew, TestSize.Level0)
{
    std::vector<std::string> keys = {
        "deviceKappa0", "deviceKappa1", "deviceKappa2", "deviceKappa3", "deviceKappa4",
    };
    std::vector<uint32_t> values = { 0, 1, 2, 3, 4 };
    LRUBucket<std::string, uint32_t> bucket { TEST_CAPACITY };
    auto success = bucket.Initialize({ keys, values });
    ASSERT_TRUE(success);
    EXPECT_EQ(5, bucket.Size());
    EXPECT_EQ(TEST_CAPACITY, bucket.Capacity());
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, keys);
    EXPECT_EQ(valMemo, values);
}

/**
 * @tc.name: multipleInsertTest
 * @tc.desc: test multiple insert operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, multipleInsertTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 20 };
    for (int i = 0; i < 50; ++i) {
        bucket.Set(i, i * 2);
    }
    EXPECT_EQ(bucket.Size(), 20);
    for (int i = 30; i < 50; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 2);
    }
}

/**
 * @tc.name: multipleDeleteTest
 * @tc.desc: test multiple delete operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, multipleDeleteTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 30 };
    for (int i = 0; i < 40; ++i) {
        bucket.Set(i, i * 3);
    }
    EXPECT_EQ(bucket.Size(), 30);
    for (int i = 10; i < 20; ++i) {
        ASSERT_TRUE(bucket.Delete(i));
    }
    EXPECT_EQ(bucket.Size(), 20);
    for (int i = 10; i < 20; ++i) {
        int value = 0;
        ASSERT_FALSE(bucket.Get(i, value));
    }
}

/**
 * @tc.name: multipleUpdateTest
 * @tc.desc: test multiple update operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, multipleUpdateTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 25 };
    for (int i = 0; i < 35; ++i) {
        bucket.Set(i, i * 4);
    }
    EXPECT_EQ(bucket.Size(), 25);
    std::map<int, int> updates;
    for (int i = 10; i < 25; ++i) {
        updates[i] = i * 5;
    }
    ASSERT_TRUE(bucket.Update(updates));
    for (int i = 10; i < 25; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 5);
    }
}

/**
 * @tc.name: capacityChangeTest
 * @tc.desc: test capacity changes
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, capacityChangeTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    for (int i = 0; i < 15; ++i) {
        bucket.Set(i, i);
    }
    EXPECT_EQ(bucket.Size(), 10);
    EXPECT_EQ(bucket.Capacity(), 10);
    bucket.ResetCapacity(5);
    EXPECT_EQ(bucket.Size(), 5);
    EXPECT_EQ(bucket.Capacity(), 5);
    bucket.ResetCapacity(20);
    for (int i = 15; i < 30; ++i) {
        bucket.Set(i, i);
    }
    EXPECT_EQ(bucket.Size(), 20);
    EXPECT_EQ(bucket.Capacity(), 20);
}

/**
 * @tc.name: stringKeyTest
 * @tc.desc: test lru bucket with string keys
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, stringKeyTest, TestSize.Level0)
{
    LRUBucket<std::string, int> bucket { 15 };
    for (int i = 0; i < 25; ++i) {
        std::string key = "str_key_" + std::to_string(i);
        bucket.Set(key, i * 6);
    }
    EXPECT_EQ(bucket.Size(), 15);
    for (int i = 10; i < 25; ++i) {
        std::string key = "str_key_" + std::to_string(i);
        int value = 0;
        ASSERT_TRUE(bucket.Get(key, value));
        EXPECT_EQ(value, i * 6);
    }
}

/**
 * @tc.name: emptyBucketTest
 * @tc.desc: test operations on empty bucket
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, emptyBucketTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    EXPECT_EQ(bucket.Size(), 0);
    EXPECT_EQ(bucket.Size(), 0);
    int value = 0;
    ASSERT_FALSE(bucket.Get(1, value));
    ASSERT_FALSE(bucket.Delete(1));
}

/**
 * @tc.name: fullBucketTest
 * @tc.desc: test operations on full bucket
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, fullBucketTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 5 };
    for (int i = 0; i < 5; ++i) {
        bucket.Set(i, i);
    }
    EXPECT_EQ(bucket.Size(), 5);
    EXPECT_GT(bucket.Size(), 0);
    bucket.Set(5, 5);
    int value = 0;
    ASSERT_FALSE(bucket.Get(0, value));
    ASSERT_TRUE(bucket.Get(5, value));
    EXPECT_EQ(value, 5);
}

/**
 * @tc.name: containsMultipleTest
 * @tc.desc: test contains operation multiple times
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, containsMultipleTest, TestSize.Level0)
{
    std::vector<std::string> keys = { "a", "b", "c", "d", "e" };
    std::vector<int> values = { 1, 2, 3, 4, 5 };
    LRUBucket<std::string, int> bucket { 10 };
    bucket.Initialize({ keys, values });
    for (const auto &key : keys) {
        auto [exists, changed] = bucket.Contains(key);
        ASSERT_TRUE(exists);
        ASSERT_TRUE(changed);
    }
}

/**
 * @tc.name: dumpMementoTest
 * @tc.desc: test dump memento operation
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, dumpMementoTest, TestSize.Level0)
{
    std::vector<std::string> keys = { "x", "y", "z" };
    std::vector<int> values = { 10, 20, 30 };
    LRUBucket<std::string, int> bucket { 10 };
    bucket.Initialize({ keys, values });
    auto [keyMemo, valMemo] = bucket.DumpMemento();
    EXPECT_EQ(keyMemo, keys);
    EXPECT_EQ(valMemo, values);
}

/**
 * @tc.name: largeScaleInsertTest
 * @tc.desc: test large scale insert operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, largeScaleInsertTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 100 };
    for (int i = 0; i < 1000; ++i) {
        bucket.Set(i, i * 7);
    }
    EXPECT_EQ(bucket.Size(), 100);
    for (int i = 900; i < 1000; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 7);
    }
}

/**
 * @tc.name: largeScaleDeleteTest
 * @tc.desc: test large scale delete operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, largeScaleDeleteTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 200 };
    for (int i = 0; i < 200; ++i) {
        bucket.Set(i, i);
    }
    EXPECT_EQ(bucket.Size(), 200);
    for (int i = 50; i < 150; ++i) {
        ASSERT_TRUE(bucket.Delete(i));
    }
    EXPECT_EQ(bucket.Size(), 100);
    for (int i = 50; i < 150; ++i) {
        int value = 0;
        ASSERT_FALSE(bucket.Get(i, value));
    }
}

/**
 * @tc.name: stressTest
 * @tc.desc: test stress operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, stressTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 50 };
    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < 100; ++i) {
            bucket.Set(round * 1000 + i, i);
        }
        for (int i = 50; i < 100; ++i) {
            int key = round * 1000 + i;
            // Only delete if the key exists (it might have been evicted due to LRU)
            bucket.Delete(key);
        }
    }
    // The final size should be 50 (the last 50 elements from the last round)
    EXPECT_LE(bucket.Size(), 50);
}

/**
 * @tc.name: intKeyValueTest
 * @tc.desc: test lru bucket with int key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, intKeyValueTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 20 };
    for (int i = 0; i < 30; ++i) {
        bucket.Set(i, i * 8);
    }
    for (int i = 10; i < 30; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 8);
    }
}

/**
 * @tc.name: stringKeyValueTest
 * @tc.desc: test lru bucket with string key and string value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, stringKeyValueTest, TestSize.Level0)
{
    LRUBucket<std::string, std::string> bucket { 15 };
    for (int i = 0; i < 25; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::string value = "value_" + std::to_string(i);
        bucket.Set(key, value);
    }
    for (int i = 10; i < 25; ++i) {
        std::string key = "key_" + std::to_string(i);
        std::string value = "";
        ASSERT_TRUE(bucket.Get(key, value));
        EXPECT_EQ(value, "value_" + std::to_string(i));
    }
}

/**
 * @tc.name: uint32KeyValueTest
 * @tc.desc: test lru bucket with uint32_t key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, uint32KeyValueTest, TestSize.Level0)
{
    LRUBucket<uint32_t, uint32_t> bucket { 25 };
    for (uint32_t i = 0; i < 40; ++i) {
        bucket.Set(i, i * 9);
    }
    for (uint32_t i = 15; i < 40; ++i) {
        uint32_t value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 9);
    }
}

/**
 * @tc.name: doubleKeyValueTest
 * @tc.desc: test lru bucket with double key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, doubleKeyValueTest, TestSize.Level0)
{
    LRUBucket<double, double> bucket { 18 };
    for (int i = 0; i < 30; ++i) {
        double key = i * 1.5;
        double value = i * 2.5;
        bucket.Set(key, value);
    }
    for (int i = 12; i < 30; ++i) {
        double key = i * 1.5;
        double value = 0.0;
        ASSERT_TRUE(bucket.Get(key, value));
        EXPECT_DOUBLE_EQ(value, i * 2.5);
    }
}

/**
 * @tc.name: boolKeyValueTest
 * @tc.desc: test lru bucket with bool key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, boolKeyValueTest, TestSize.Level0)
{
    LRUBucket<bool, bool> bucket { 4 };
    bucket.Set(true, false);
    bucket.Set(false, true);
    bool value = false;
    ASSERT_TRUE(bucket.Get(true, value));
    EXPECT_EQ(value, false);
    ASSERT_TRUE(bucket.Get(false, value));
    EXPECT_EQ(value, true);
}

/**
 * @tc.name: charKeyValueTest
 * @tc.desc: test lru bucket with char key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, charKeyValueTest, TestSize.Level0)
{
    LRUBucket<char, char> bucket { 10 };
    for (char c = 'A'; c <= 'Z'; ++c) {
        bucket.Set(c, c + 1);
    }
    for (char c = 'Q'; c <= 'Z'; ++c) {
        char value = '\0';
        ASSERT_TRUE(bucket.Get(c, value));
        EXPECT_EQ(value, c + 1);
    }
}

/**
 * @tc.name: longKeyValueTest
 * @tc.desc: test lru bucket with long key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, longKeyValueTest, TestSize.Level0)
{
    LRUBucket<long, long> bucket { 12 };
    for (long i = 0; i < 25; ++i) {
        bucket.Set(i, i * 10L);
    }
    for (long i = 13; i < 25; ++i) {
        long value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 10L);
    }
}

/**
 * @tc.name: floatKeyValueTest
 * @tc.desc: test lru bucket with float key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, floatKeyValueTest, TestSize.Level0)
{
    LRUBucket<float, float> bucket { 16 };
    for (int i = 0; i < 30; ++i) {
        float key = i * 1.2f;
        float value = i * 2.3f;
        bucket.Set(key, value);
    }
    for (int i = 14; i < 30; ++i) {
        float key = i * 1.2f;
        float value = 0.0f;
        ASSERT_TRUE(bucket.Get(key, value));
        EXPECT_FLOAT_EQ(value, i * 2.3f);
    }
}

/**
 * @tc.name: int64KeyValueTest
 * @tc.desc: test lru bucket with int64_t key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, int64KeyValueTest, TestSize.Level0)
{
    LRUBucket<int64_t, int64_t> bucket { 22 };
    for (int64_t i = 0; i < 40; ++i) {
        bucket.Set(i, i * 100LL);
    }
    for (int64_t i = 18; i < 40; ++i) {
        int64_t value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 100LL);
    }
}

/**
 * @tc.name: uint64KeyValueTest
 * @tc.desc: test lru bucket with uint64_t key and value
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, uint64KeyValueTest, TestSize.Level0)
{
    LRUBucket<uint64_t, uint64_t> bucket { 24 };
    for (uint64_t i = 0; i < 50; ++i) {
        bucket.Set(i, i * 200ULL);
    }
    for (uint64_t i = 26; i < 50; ++i) {
        uint64_t value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 200ULL);
    }
}

/**
 * @tc.name: sizeAfterOperationsTest
 * @tc.desc: test size after various operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, sizeAfterOperationsTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    EXPECT_EQ(bucket.Size(), 0);
    for (int i = 0; i < 15; ++i) {
        bucket.Set(i, i);
    }
    EXPECT_EQ(bucket.Size(), 10);
    bucket.Delete(14);
    EXPECT_EQ(bucket.Size(), 9);
    bucket.Delete(13);
    EXPECT_EQ(bucket.Size(), 8);
    bucket.Delete(12);
    EXPECT_EQ(bucket.Size(), 7);
}

/**
 * @tc.name: capacityTest
 * @tc.desc: test capacity operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, capacityTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 5 };
    EXPECT_EQ(bucket.Capacity(), 5);
    bucket.ResetCapacity(10);
    EXPECT_EQ(bucket.Capacity(), 10);
    bucket.ResetCapacity(3);
    EXPECT_EQ(bucket.Capacity(), 3);
}

/**
 * @tc.name: emptyAfterClearTest
 * @tc.desc: test empty after clearing bucket
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, emptyAfterClearTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    for (int i = 0; i < 15; ++i) {
        bucket.Set(i, i);
    }
    EXPECT_GT(bucket.Size(), 0);
    bucket.ResetCapacity(0);
    EXPECT_EQ(bucket.Size(), 0);
}

/**
 * @tc.name: getNotFoundTest
 * @tc.desc: test get operation for non-existent keys
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, getNotFoundTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    for (int i = 0; i < 10; ++i) {
        bucket.Set(i, i);
    }
    int value = 0;
    ASSERT_FALSE(bucket.Get(10, value));
    ASSERT_FALSE(bucket.Get(11, value));
    ASSERT_FALSE(bucket.Get(-1, value));
}

/**
 * @tc.name: deleteNotFoundTest
 * @tc.desc: test delete operation for non-existent keys
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, deleteNotFoundTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    for (int i = 0; i < 10; ++i) {
        bucket.Set(i, i);
    }
    ASSERT_FALSE(bucket.Delete(10));
    ASSERT_FALSE(bucket.Delete(20));
    ASSERT_FALSE(bucket.Delete(-5));
}

/**
 * @tc.name: updateNotFoundTest
 * @tc.desc: test update operation for non-existent keys
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, updateNotFoundTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    for (int i = 0; i < 10; ++i) {
        bucket.Set(i, i);
    }
    ASSERT_FALSE(bucket.Update(10, 100));
    ASSERT_FALSE(bucket.Update(15, 150));
}

/**
 * @tc.name: updateMultipleNotFoundTest
 * @tc.desc: test update multiple operation for non-existent keys
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, updateMultipleNotFoundTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    for (int i = 0; i < 10; ++i) {
        bucket.Set(i, i);
    }
    std::map<int, int> updates = {
        {10, 100},
        {11, 110},
        {12, 120}
    };
    ASSERT_FALSE(bucket.Update(updates));
}

/**
 * @tc.name: lruOrderTest
 * @tc.desc: test lru ordering after multiple accesses
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, lruOrderTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 5 };
    for (int i = 0; i < 5; ++i) {
        bucket.Set(i, i);
    }
    int value = 0;
    bucket.Get(0, value);
    bucket.Get(1, value);
    bucket.Set(5, 5);
    bucket.Set(6, 6);
    ASSERT_FALSE(bucket.Get(2, value));
    ASSERT_FALSE(bucket.Get(3, value));
    ASSERT_TRUE(bucket.Get(4, value));
}

/**
 * @tc.name: containsOrderTest
 * @tc.desc: test contains operation effect on order
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, containsOrderTest, TestSize.Level0)
{
    std::vector<std::string> keys = {"a", "b", "c", "d", "e"};
    std::vector<int> values = {1, 2, 3, 4, 5};
    LRUBucket<std::string, int> bucket { 5 };
    bucket.Initialize({ keys, values });
    // Access a and b to move them to the front
    int value = 0;
    bucket.Get("a", value);
    bucket.Get("b", value);
    // Add a new element which will evict the oldest element (c)
    bucket.Set("f", 6);
    // Add another new element which will evict the next oldest (d)
    bucket.Set("g", 7);
    // Now c and d should be evicted
    ASSERT_FALSE(bucket.Get("c", value));
    ASSERT_FALSE(bucket.Get("d", value));
    // e, a, b, f, g should be there
    ASSERT_TRUE(bucket.Get("e", value));
    EXPECT_EQ(value, 5);
}

/**
 * @tc.name: setOverwriteTest
 * @tc.desc: test set operation overwriting existing key
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, setOverwriteTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    bucket.Set(1, 100);
    bucket.Set(1, 200);
    int value = 0;
    ASSERT_TRUE(bucket.Get(1, value));
    EXPECT_EQ(value, 200);
}

/**
 * @tc.name: initializeOverwriteTest
 * @tc.desc: test initialize operation with existing data
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, initializeOverwriteTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    bucket.Set(1, 10);
    bucket.Set(2, 20);
    std::vector<int> keys = {3, 4, 5};
    std::vector<int> values = {30, 40, 50};
    bucket.Initialize({ keys, values });
    EXPECT_EQ(bucket.Size(), 3);
}

/**
 * @tc.name: negativeKeyTest
 * @tc.desc: test operations with negative keys
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, negativeKeyTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    for (int i = -10; i < 0; ++i) {
        bucket.Set(i, i * 2);
    }
    for (int i = -10; i < -5; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 2);
    }
}

/**
 * @tc.name: largeKeyTest
 * @tc.desc: test operations with large keys
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, largeKeyTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 5 };
    for (int i = 1000000; i < 1000010; ++i) {
        bucket.Set(i, i);
    }
    for (int i = 1000000; i < 1000005; ++i) {
        int value = 0;
        ASSERT_FALSE(bucket.Get(i, value));
    }
    for (int i = 1000005; i < 1000010; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i);
    }
}

/**
 * @tc.name: zeroCapacityTest
 * @tc.desc: test operations with zero capacity
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, zeroCapacityTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 0 };
    bucket.Set(1, 1);
    bucket.Set(2, 2);
    EXPECT_EQ(bucket.Size(), 0);
    int value = 0;
    ASSERT_FALSE(bucket.Get(1, value));
}

/**
 * @tc.name: veryLargeCapacityTest
 * @tc.desc: test operations with very large capacity
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, veryLargeCapacityTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 1000 };
    for (int i = 0; i < 500; ++i) {
        bucket.Set(i, i);
    }
    EXPECT_EQ(bucket.Size(), 500);
    for (int i = 0; i < 500; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i);
    }
}

/**
 * @tc.name: alternatingAccessTest
 * @tc.desc: test alternating access pattern
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, alternatingAccessTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 5 };
    for (int i = 0; i < 10; ++i) {
        bucket.Set(i, i);
    }
    int value = 0;
    for (int round = 0; round < 3; ++round) {
        bucket.Get(9, value);
        bucket.Get(8, value);
        bucket.Set(10 + round * 2, 10 + round * 2);
        bucket.Set(11 + round * 2, 11 + round * 2);
    }
}

/**
 * @tc.name: randomAccessTest
 * @tc.desc: test random access pattern
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, randomAccessTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 20 };
    for (int i = 0; i < 100; ++i) {
        bucket.Set(i, i);
    }
    int value = 0;
    for (int i = 80; i < 100; ++i) {
        ASSERT_TRUE(bucket.Get(i, value));
    }
}

/**
 * @tc.name: boundaryTest
 * @tc.desc: test boundary conditions
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, boundaryTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 1 };
    bucket.Set(1, 1);
    bucket.Set(2, 2);
    bucket.Set(3, 3);
    int value = 0;
    ASSERT_FALSE(bucket.Get(1, value));
    ASSERT_FALSE(bucket.Get(2, value));
    ASSERT_TRUE(bucket.Get(3, value));
    EXPECT_EQ(value, 3);
}

/**
 * @tc.name: finalComprehensiveTest
 * @tc.desc: comprehensive test with all operations
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, finalComprehensiveTest, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    EXPECT_EQ(bucket.Size(), 0);
    EXPECT_EQ(bucket.Capacity(), 10);
    EXPECT_EQ(bucket.Size(), 0);
    
    for (int i = 0; i < 20; ++i) {
        bucket.Set(i, i * 2);
    }
    
    EXPECT_EQ(bucket.Size(), 10);
    EXPECT_GT(bucket.Size(), 0);
    
    for (int i = 10; i < 20; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 2);
    }
    
    for (int i = 15; i < 20; ++i) {
        ASSERT_TRUE(bucket.Delete(i));
    }
    
    EXPECT_EQ(bucket.Size(), 5);
    
    std::map<int, int> updates = {{10, 100}, {11, 110}, {12, 120}, {13, 130}, {14, 140}};
    ASSERT_TRUE(bucket.Update(updates));
    
    for (int i = 10; i < 14; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, updates[i]);
    }
    
    bucket.ResetCapacity(3);
    EXPECT_EQ(bucket.Size(), 3);
    
    bucket.ResetCapacity(0);
    EXPECT_EQ(bucket.Size(), 0);
    EXPECT_EQ(bucket.Size(), 0);
}

/**
 * @tc.name: AdditionalTest01
 * @tc.desc: additional test case 1
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest01, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 5 };
    for (int i = 0; i < 10; ++i) {
        bucket.Set(i, i * 2);
    }
    EXPECT_EQ(bucket.Size(), 5);
    for (int i = 5; i < 10; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 2);
    }
}

/**
 * @tc.name: AdditionalTest02
 * @tc.desc: additional test case 2
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest02, TestSize.Level0)
{
    LRUBucket<std::string, int> bucket { 8 };
    for (int i = 0; i < 15; ++i) {
        std::string key = "key_" + std::to_string(i);
        bucket.Set(key, i * 3);
    }
    EXPECT_EQ(bucket.Size(), 8);
    for (int i = 7; i < 15; ++i) {
        std::string key = "key_" + std::to_string(i);
        int value = 0;
        ASSERT_TRUE(bucket.Get(key, value));
        EXPECT_EQ(value, i * 3);
    }
}

/**
 * @tc.name: AdditionalTest03
 * @tc.desc: additional test case 3
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest03, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 10 };
    for (int i = 0; i < 20; ++i) {
        bucket.Set(i, i * 4);
    }
    for (int i = 0; i < 20; ++i) {
        int value = 0;
        if (i >= 10) {
            ASSERT_TRUE(bucket.Get(i, value));
            EXPECT_EQ(value, i * 4);
        } else {
            ASSERT_FALSE(bucket.Get(i, value));
        }
    }
}

/**
 * @tc.name: AdditionalTest04
 * @tc.desc: additional test case 4
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest04, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 15 };
    for (int i = 0; i < 30; ++i) {
        bucket.Set(i, i * 5);
    }
    for (int i = 0; i < 30; ++i) {
        int value = 0;
        if (i >= 15) {
            ASSERT_TRUE(bucket.Get(i, value));
            EXPECT_EQ(value, i * 5);
        } else {
            ASSERT_FALSE(bucket.Get(i, value));
        }
    }
}

/**
 * @tc.name: AdditionalTest05
 * @tc.desc: additional test case 5
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest05, TestSize.Level0)
{
    LRUBucket<std::string, std::string> bucket { 12 };
    for (int i = 0; i < 25; ++i) {
        std::string key = "str_key_" + std::to_string(i);
        std::string value = "str_value_" + std::to_string(i);
        bucket.Set(key, value);
    }
    EXPECT_EQ(bucket.Size(), 12);
}

/**
 * @tc.name: AdditionalTest06
 * @tc.desc: additional test case 6
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest06, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 20 };
    for (int i = 0; i < 40; ++i) {
        bucket.Set(i, i * 6);
    }
    for (int i = 0; i < 40; ++i) {
        int value = 0;
        if (i >= 20) {
            ASSERT_TRUE(bucket.Get(i, value));
            EXPECT_EQ(value, i * 6);
        } else {
            ASSERT_FALSE(bucket.Get(i, value));
        }
    }
}

/**
 * @tc.name: AdditionalTest07
 * @tc.desc: additional test case 7
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest07, TestSize.Level0)
{
    LRUBucket<std::string, int> bucket { 25 };
    for (int i = 0; i < 50; ++i) {
        std::string key = "large_key_" + std::to_string(i);
        bucket.Set(key, i);
    }
    EXPECT_EQ(bucket.Size(), 25);
}

/**
 * @tc.name: AdditionalTest08
 * @tc.desc: additional test case 8
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest08, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 30 };
    for (int i = 0; i < 60; ++i) {
        bucket.Set(i, i * 7);
    }
    for (int i = 0; i < 30; ++i) {
        int value = 0;
        ASSERT_FALSE(bucket.Get(i, value));
    }
}

/**
 * @tc.name: AdditionalTest09
 * @tc.desc: additional test case 9
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest09, TestSize.Level0)
{
    LRUBucket<double, double> bucket { 10 };
    for (int i = 0; i < 20; ++i) {
        double key = i * 1.5;
        double value = i * 2.5;
        bucket.Set(key, value);
    }
    EXPECT_EQ(bucket.Size(), 10);
}

/**
 * @tc.name: AdditionalTest10
 * @tc.desc: additional test case 10
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest10, TestSize.Level0)
{
    LRUBucket<float, float> bucket { 15 };
    for (int i = 0; i < 30; ++i) {
        float key = i * 2.5f;
        float value = i * 3.5f;
        bucket.Set(key, value);
    }
    EXPECT_EQ(bucket.Size(), 15);
}

/**
 * @tc.name: AdditionalTest11
 * @tc.desc: additional test case 11
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest11, TestSize.Level0)
{
    LRUBucket<bool, bool> bucket { 8 };
    bucket.Set(true, false);
    bucket.Set(false, true);
    bool value = false;
    ASSERT_TRUE(bucket.Get(true, value));
    EXPECT_EQ(value, false);
    ASSERT_TRUE(bucket.Get(false, value));
    EXPECT_EQ(value, true);
}

/**
 * @tc.name: AdditionalTest12
 * @tc.desc: additional test case 12
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest12, TestSize.Level0)
{
    LRUBucket<char, char> bucket { 10 };
    for (char c = 'a'; c <= 'z'; ++c) {
        char value = c + 1;
        bucket.Set(c, value);
    }
    EXPECT_EQ(bucket.Size(), 10);
}

/**
 * @tc.name: AdditionalTest13
 * @tc.desc: additional test case 13
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest13, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 5 };
    for (int i = 0; i < 5; ++i) {
        bucket.Set(i, i * 10);
    }
    for (int i = 0; i < 5; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 10);
    }
    bucket.Set(5, 50);
    int value = 0;
    ASSERT_FALSE(bucket.Get(0, value));
    ASSERT_TRUE(bucket.Get(5, value));
    EXPECT_EQ(value, 50);
}

/**
 * @tc.name: AdditionalTest14
 * @tc.desc: additional test case 14
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest14, TestSize.Level0)
{
    LRUBucket<std::string, int> bucket { 6 };
    bucket.Set("test_key_1", 100);
    bucket.Set("test_key_2", 200);
    bucket.Set("test_key_3", 300);
    bucket.Set("test_key_4", 400);
    bucket.Set("test_key_5", 500);
    bucket.Set("test_key_6", 600);
    EXPECT_EQ(bucket.Size(), 6);
    int value = 0;
    ASSERT_TRUE(bucket.Get("test_key_1", value));
    EXPECT_EQ(value, 100);
}

/**
 * @tc.name: AdditionalTest15
 * @tc.desc: additional test case 15
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest15, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 8 };
    for (int i = 0; i < 16; ++i) {
        bucket.Set(i, i * 11);
    }
    for (int i = 8; i < 16; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 11);
    }
}

/**
 * @tc.name: AdditionalTest16
 * @tc.desc: additional test case 16
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest16, TestSize.Level0)
{
    LRUBucket<std::string, std::string> bucket { 10 };
    for (int i = 0; i < 20; ++i) {
        std::string key = "add_key_" + std::to_string(i);
        std::string value = "add_value_" + std::to_string(i);
        bucket.Set(key, value);
    }
    EXPECT_EQ(bucket.Size(), 10);
}

/**
 * @tc.name: AdditionalTest17
 * @tc.desc: additional test case 17
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest17, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 12 };
    for (int i = 0; i < 24; ++i) {
        bucket.Set(i, i * 12);
    }
    for (int i = 12; i < 24; ++i) {
        int value = 0;
        ASSERT_TRUE(bucket.Get(i, value));
        EXPECT_EQ(value, i * 12);
    }
}

/**
 * @tc.name: AdditionalTest18
 * @tc.desc: additional test case 18
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest18, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 15 };
    for (int i = 0; i < 30; ++i) {
        bucket.Set(i, i * 13);
    }
    for (int i = 0; i < 15; ++i) {
        int value = 0;
        ASSERT_FALSE(bucket.Get(i, value));
    }
}

/**
 * @tc.name: AdditionalTest19
 * @tc.desc: additional test case 19
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest19, TestSize.Level0)
{
    LRUBucket<std::string, int> bucket { 7 };
    for (int i = 0; i < 14; ++i) {
        std::string key = "case_key_" + std::to_string(i);
        bucket.Set(key, i * 14);
    }
    EXPECT_EQ(bucket.Size(), 7);
}

/**
 * @tc.name: AdditionalTest20
 * @tc.desc: additional test case 20
 * @tc.type: FUNC
 */
HWTEST_F(LRUBucketTestNew, AdditionalTest20, TestSize.Level0)
{
    LRUBucket<int, int> bucket { 9 };
    for (int i = 0; i < 18; ++i) {
        bucket.Set(i, i * 15);
    }
    for (int i = 0; i < 9; ++i) {
        int value = 0;
        ASSERT_FALSE(bucket.Get(i, value));
    }
}

} // namespace OHOS::Test
