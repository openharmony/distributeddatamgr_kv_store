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

#define LOG_TAG "ConcurrentStripedMapNew"

#include "concurrent_striped_map.h"

#include <future>
#include <thread>

#include "gtest/gtest.h"
using namespace testing::ext;
namespace OHOS::Test {
class ConcurrentStripedMapTestNew : public testing::Test {
public:
    static void SetUpTestCase(void) {}

    static void TearDownTestCase(void) {}

protected:
    void SetUp() {}

    void TearDown() {}
};

/**
 * @tc.name: ComputeBasicTest
 * @tc.desc: Test basic Compute method with lambda
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeBasicTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.Compute(10, [](auto key, int32_t &value) {
        value = 10;
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);
    int32_t value = 0;
    map.Compute(10, [&value](auto k, int32_t &v) {
        value = v;
        return false;
    });
    EXPECT_EQ(value, 10);
}

/**
 * @tc.name: ComputeNullActionNew
 * @tc.desc: Test Compute method with null action
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeNullActionNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.Compute(5, nullptr);
    EXPECT_FALSE(ret);
    EXPECT_EQ(map.Size(), 0);
}

/**
 * @tc.name: ComputeRemoveElementNew
 * @tc.desc: Test Compute method that removes an element
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeRemoveElementNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(15, [](auto key, int32_t &value) {
        value = 150;
        return true;
    });
    EXPECT_EQ(map.Size(), 1);
    bool ret = map.Compute(15, [](auto key, int32_t &value) {
        return false;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 0);
}

/**
 * @tc.name: ComputeIfPresentNullActionNew
 * @tc.desc: Test ComputeIfPresent method with null action
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentNullActionNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.ComputeIfPresent(8, nullptr);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ComputeIfPresentNonExistentKeyNew
 * @tc.desc: Test ComputeIfPresent method with non-existent key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentNonExistentKeyNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.ComputeIfPresent(88, [](auto key, int32_t &value) {
        value = 880;
        return true;
    });
    EXPECT_FALSE(ret);
    EXPECT_TRUE(map.Empty());
}

/**
 * @tc.name: ComputeIfPresentExistingKeyNew
 * @tc.desc: Test ComputeIfPresent method with existing key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentExistingKeyNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(99, [](auto key, int32_t &value) {
        value = 990;
        return true;
    });
    bool ret = map.ComputeIfPresent(99, [](auto key, int32_t &value) {
        EXPECT_EQ(value, 990);
        value = 991;
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);
    int32_t retrievedValue = 0;
    map.Compute(99, [&retrievedValue](auto key, int32_t &value) {
        retrievedValue = value;
        return true;
    });
    EXPECT_EQ(retrievedValue, 991);
}

/**
 * @tc.name: EraseIfNullActionNew
 * @tc.desc: Test EraseIf method with null action
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EraseIfNullActionNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(1, [](auto key, int32_t &value) {
        value = 1;
        return true;
    });
    map.Compute(2, [](auto key, int32_t &value) {
        value = 2;
        return true;
    });
    EXPECT_EQ(map.Size(), 2);
    size_t count = map.EraseIf(nullptr);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(map.Size(), 2);
}

/**
 * @tc.name: EraseIfWithConditionNew
 * @tc.desc: Test EraseIf method with a condition
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EraseIfWithConditionNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(11, [](auto key, int32_t &value) {
        value = 11;
        return true;
    });
    map.Compute(22, [](auto key, int32_t &value) {
        value = 22;
        return true;
    });
    map.Compute(33, [](auto key, int32_t &value) {
        value = 33;
        return true;
    });
    EXPECT_EQ(map.Size(), 3);
    size_t count = map.EraseIf([](const int32_t &key, int32_t &value) {
        return value < 20;
    });
    EXPECT_EQ(count, 1);
    EXPECT_EQ(map.Size(), 2);
}

/**
 * @tc.name: DoActionNullActionNew
 * @tc.desc: Test DoAction method with null action and various returns
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, DoActionNullActionNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    EXPECT_FALSE(map.DoAction(nullptr));
    EXPECT_EQ(map.Size(), 0);
    EXPECT_TRUE(map.DoAction([]() {
        return true;
    }));
    EXPECT_FALSE(map.DoAction([]() {
        return false;
    }));
}

/**
 * @tc.name: StringKeyAndValueTestNew
 * @tc.desc: Test ConcurrentStripedMap with string key and value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, StringKeyAndValueTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    bool ret = map.Compute("alpha_key", [](const std::string &key, std::string &value) {
        value = "alpha_value";
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);
    std::string retrievedValue = "";
    map.Compute("alpha_key", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return true;
    });
    EXPECT_EQ(retrievedValue, "alpha_value");
}

/**
 * @tc.name: StringKeyComputeIfPresentTestNew
 * @tc.desc: Test ComputeIfPresent with string key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, StringKeyComputeIfPresentTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    map.Compute("beta_key", [](const std::string &key, std::string &value) {
        value = "beta_value";
        return true;
    });
    bool ret = map.ComputeIfPresent("beta_key", [](const std::string &key, std::string &value) {
        value = "beta_modified_value";
        return true;
    });
    EXPECT_TRUE(ret);
    std::string retrievedValue = "";
    map.Compute("beta_key", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return true;
    });
    EXPECT_EQ(retrievedValue, "beta_modified_value");
}

struct TestObjectNew {
    int objectId;
    std::string objectName;

    TestObjectNew() : objectId(0), objectName("") {}
    TestObjectNew(int i, const std::string &n) : objectId(i), objectName(n) {}

    bool operator==(const TestObjectNew &other) const
    {
        return objectId == other.objectId && objectName == other.objectName;
    }
};

/**
 * @tc.name: CustomObjectTestNew
 * @tc.desc: Test ConcurrentStripedMap with custom object as value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, CustomObjectTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<int, TestObjectNew> map;
    bool ret = map.Compute(101, [](int key, TestObjectNew &value) {
        value = TestObjectNew(1010, "test_object_new");
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);
    TestObjectNew retrievedObj;
    map.Compute(101, [&retrievedObj](int key, TestObjectNew &value) {
        retrievedObj = value;
        return true;
    });
    EXPECT_EQ(retrievedObj.objectId, 1010);
    EXPECT_EQ(retrievedObj.objectName, "test_object_new");
}

/**
 * @tc.name: CustomObjectModifyTestNew
 * @tc.desc: Test modifying custom object in map
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, CustomObjectModifyTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<int, TestObjectNew> map;
    map.Compute(102, [](int key, TestObjectNew &value) {
        value = TestObjectNew(1020, "original_new");
        return true;
    });
    map.Compute(102, [](int key, TestObjectNew &value) {
        value.objectId = 1021;
        value.objectName = "modified_new";
        return true;
    });
    TestObjectNew retrievedObj;
    map.Compute(102, [&retrievedObj](int key, TestObjectNew &value) {
        retrievedObj = value;
        return true;
    });
    EXPECT_EQ(retrievedObj.objectId, 1021);
    EXPECT_EQ(retrievedObj.objectName, "modified_new");
}

/**
 * @tc.name: VectorValueTestNew
 * @tc.desc: Test ConcurrentStripedMap with vector as value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, VectorValueTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<int, std::vector<int>> map;
    bool ret = map.Compute(103, [](int key, std::vector<int> &value) {
        value = { 103, 203, 303, 403, 503 };
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);
    map.Compute(103, [](int key, std::vector<int> &value) {
        value.push_back(603);
        return true;
    });
    std::vector<int> retrievedVec;
    map.Compute(103, [&retrievedVec](int key, std::vector<int> &value) {
        retrievedVec = value;
        return true;
    });
    EXPECT_EQ(retrievedVec.size(), 6);
    EXPECT_EQ(retrievedVec[5], 603);
}

/**
 * @tc.name: PairValueTestNew
 * @tc.desc: Test ConcurrentStripedMap with pair as value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, PairValueTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::pair<int, std::string>> map;
    bool ret = map.Compute("pair_key_new", [](const std::string &key, std::pair<int, std::string> &value) {
        value = std::make_pair(420, "pair_value_new");
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);
    std::pair<int, std::string> retrievedPair;
    map.Compute("pair_key_new", [&retrievedPair](const std::string &key, std::pair<int, std::string> &value) {
        retrievedPair = value;
        return true;
    });
    EXPECT_EQ(retrievedPair.first, 420);
    EXPECT_EQ(retrievedPair.second, "pair_value_new");
}

/**
 * @tc.name: ConcurrentEraseIfTestNew
 * @tc.desc: Test concurrent EraseIf operations from different threads
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ConcurrentEraseIfTestNew, TestSize.Level0)
{
    const uint32_t mapSize = 50;
    const uint32_t numThreads = 2;
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < mapSize; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i * 20;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), mapSize);
    std::vector<std::thread> threads;
    std::atomic_uint32_t totalErased = 0;
    auto eraseAction = [&map, &totalErased](size_t i) mutable {
        map.EraseIf([i, &totalErased](const int32_t &key, int32_t &value) {
            if (key >= 10 * i && key <= 10 * i + 20) {
                totalErased++;
                return true;
            }
            return false;
        });
    };
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([&eraseAction, i]() {
            eraseAction(i);
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_LE(totalErased, mapSize);
    EXPECT_GE(map.Size(), mapSize - totalErased);
}

/**
 * @tc.name: ConcurrentComputeSameKeyTestNew
 * @tc.desc: Test concurrent Compute operations on the same key from different threads
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ConcurrentComputeSameKeyTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    const uint32_t numThreads = 2;
    const uint32_t opsPerThread = 50;
    std::atomic<uint32_t> operationCount = 0;
    std::vector<std::thread> threads;
    auto operation = [&map, &operationCount]() {
        for (uint32_t i = 0; i < opsPerThread; ++i) {
            map.Compute(100, [&operationCount](auto k, int32_t &value) {
                operationCount++;
                return true;
            });
        }
    };
    for (uint32_t t = 0; t < numThreads; ++t) {
        threads.emplace_back([&operation]() {
            operation();
        });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(operationCount.load(), numThreads * opsPerThread);
}

/**
 * @tc.name: ConcurrentComputeDifferentKeysTestNew
 * @tc.desc: Test concurrent Compute operations on different keys from multiple threads
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ConcurrentComputeDifferentKeysTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    std::atomic_uint32_t sharedValue = 0;
    std::promise<void> blocker;
    std::future<void> blockerFuture = blocker.get_future();
    std::promise<void> threadStarted;
    std::future<void> threadStartedFuture = threadStarted.get_future();
    std::thread blockingThread([&map, &blockerFuture, &threadStarted, &sharedValue]() {
        map.Compute(0, [&blockerFuture, &threadStarted, &sharedValue](auto k, int32_t &value) {
            threadStarted.set_value();
            blockerFuture.wait();
            value = ++sharedValue;
            return true;
        });
    });
    EXPECT_TRUE(threadStartedFuture.wait_for(std::chrono::milliseconds(50)) == std::future_status::ready);
    for (int i = 1; i < 5; ++i) {
        auto res = map.Compute(i, [&sharedValue](auto k, int32_t &value) {
            value = ++sharedValue;
            return true;
        });
        EXPECT_TRUE(res);
    }
    blocker.set_value();
    blockingThread.join();
    uint32_t finalValue = 0;
    map.ComputeIfPresent(0, [&finalValue](auto k, int32_t &value) {
        finalValue = value;
        return true;
    });
    EXPECT_EQ(sharedValue.load(), finalValue);
}

/**
 * @tc.name: MultipleComputeOperations
 * @tc.desc: Test multiple sequential Compute operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, MultipleComputeOperations, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 100; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i * 3;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 100);
    for (int i = 0; i < 100; ++i) {
        int32_t retrievedValue = 0;
        map.Compute(i, [&retrievedValue, i](auto k, int32_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, i * 3);
    }
}

/**
 * @tc.name: StringKeyMultipleOperations
 * @tc.desc: Test multiple operations with string keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, StringKeyMultipleOperations, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, int32_t> map;
    for (int i = 0; i < 50; ++i) {
        std::string key = "str_key_" + std::to_string(i);
        map.Compute(key, [i](const std::string &key, int32_t &value) {
            value = i * 7;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 50);
    for (int i = 0; i < 50; ++i) {
        std::string key = "str_key_" + std::to_string(i);
        int32_t retrievedValue = 0;
        map.Compute(key, [&retrievedValue, i](const std::string &k, int32_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, i * 7);
    }
}

/**
 * @tc.name: ComputeIfPresentMultipleKeys
 * @tc.desc: Test ComputeIfPresent with multiple keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentMultipleKeys, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 30; ++i) {
        map.Compute(i * 10, [i](auto key, int32_t &value) {
            value = i * 11;
            return true;
        });
    }
    for (int i = 0; i < 30; ++i) {
        bool ret = map.ComputeIfPresent(i * 10, [](auto key, int32_t &value) {
            value = value + 1;
            return true;
        });
        EXPECT_TRUE(ret);
    }
    for (int i = 0; i < 30; ++i) {
        int32_t retrievedValue = 0;
        map.Compute(i * 10, [&retrievedValue](auto k, int32_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, i * 11 + 1);
    }
}

/**
 * @tc.name: EraseIfMultipleConditions
 * @tc.desc: Test EraseIf with multiple conditions
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EraseIfMultipleConditions, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 60; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 60);
    size_t erasedCount = map.EraseIf([](const int32_t &key, int32_t &value) {
        return (key % 2 == 0) && (value < 30);
    });
    EXPECT_EQ(erasedCount, 15);
    EXPECT_EQ(map.Size(), 45);
}

/**
 * @tc.name: DoActionWithEmptyMap
 * @tc.desc: Test DoAction on empty map
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, DoActionWithEmptyMap, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.DoAction([]() {
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 0);
}

/**
 * @tc.name: CustomObjectVectorTest
 * @tc.desc: Test ConcurrentStripedMap with vector of custom objects
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, CustomObjectVectorTest, TestSize.Level0)
{
    ConcurrentStripedMap<int, std::vector<TestObjectNew>> map;
    bool ret = map.Compute(104, [](int key, std::vector<TestObjectNew> &value) {
        value = { TestObjectNew(1040, "a"), TestObjectNew(1041, "b"), TestObjectNew(1042, "c") };
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);
    map.Compute(104, [](int key, std::vector<TestObjectNew> &value) {
        value.push_back(TestObjectNew(1043, "d"));
        return true;
    });
    std::vector<TestObjectNew> retrievedVec;
    map.Compute(104, [&retrievedVec](int key, std::vector<TestObjectNew> &value) {
        retrievedVec = value;
        return true;
    });
    EXPECT_EQ(retrievedVec.size(), 4);
}

/**
 * @tc.name: LargeScaleComputeTest
 * @tc.desc: Test large scale Compute operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, LargeScaleComputeTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 500; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i * 4;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 500);
    for (int i = 0; i < 500; ++i) {
        int32_t retrievedValue = 0;
        map.Compute(i, [&retrievedValue, i](auto k, int32_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, i * 4);
    }
}

/**
 * @tc.name: StringKeyLargeScaleTest
 * @tc.desc: Test large scale operations with string keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, StringKeyLargeScaleTest, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    for (int i = 0; i < 200; ++i) {
        std::string key = "large_str_key_" + std::to_string(i);
        map.Compute(key, [i](const std::string &key, std::string &value) {
            value = "large_str_value_" + std::to_string(i);
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 200);
    for (int i = 0; i < 200; ++i) {
        std::string key = "large_str_key_" + std::to_string(i);
        std::string retrievedValue = "";
        map.Compute(key, [&retrievedValue, i](const std::string &k, std::string &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, "large_str_value_" + std::to_string(i));
    }
}

/**
 * @tc.name: ComputeWithNegativeKeys
 * @tc.desc: Test Compute with negative integer keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeWithNegativeKeys, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = -50; i < 50; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i * 5;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 100);
    for (int i = -50; i < 50; ++i) {
        int32_t retrievedValue = 0;
        map.Compute(i, [&retrievedValue, i](auto k, int32_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, i * 5);
    }
}

/**
 * @tc.name: ComputeIfPresentModifyAll
 * @tc.desc: Test ComputeIfPresent to modify all existing elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentModifyAll, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 40; ++i) {
        map.Compute(i * 2, [i](auto key, int32_t &value) {
            value = i * 6;
            return true;
        });
    }
    for (int i = 0; i < 40; ++i) {
        bool ret = map.ComputeIfPresent(i * 2, [](auto key, int32_t &value) {
            value = value * 2;
            return true;
        });
        EXPECT_TRUE(ret);
    }
    for (int i = 0; i < 40; ++i) {
        int32_t retrievedValue = 0;
        map.Compute(i * 2, [&retrievedValue](auto k, int32_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, i * 12);
    }
}

/**
 * @tc.name: EraseIfRemoveAllEven
 * @tc.desc: Test EraseIf to remove all even keyed elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EraseIfRemoveAllEven, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 80; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 80);
    size_t erasedCount = map.EraseIf([](const int32_t &key, int32_t &value) {
        return key % 2 == 0;
    });
    EXPECT_EQ(erasedCount, 40);
    EXPECT_EQ(map.Size(), 40);
}

/**
 * @tc.name: EraseIfRemoveAllOdd
 * @tc.desc: Test EraseIf to remove all odd valued elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EraseIfRemoveAllOdd, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 100; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 100);
    size_t erasedCount = map.EraseIf([](const int32_t &key, int32_t &value) {
        return value % 2 == 1;
    });
    EXPECT_EQ(erasedCount, 50);
    EXPECT_EQ(map.Size(), 50);
}

/**
 * @tc.name: ComputeWithZeroKey
 * @tc.desc: Test Compute with zero key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeWithZeroKey, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.Compute(0, [](auto key, int32_t &value) {
        value = 0;
        return true;
    });
    EXPECT_TRUE(ret);
    int32_t retrievedValue = 0;
    map.Compute(0, [&retrievedValue](auto k, int32_t &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, 0);
}

/**
 * @tc.name: ComputeIfPresentWithNonExistent
 * @tc.desc: Test ComputeIfPresent with mixture of existing and non-existing keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentWithNonExistent, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 20; ++i) {
        map.Compute(i * 3, [i](auto key, int32_t &value) {
            value = i * 8;
            return true;
        });
    }
    for (int i = 0; i < 60; ++i) {
        bool ret = map.ComputeIfPresent(i, [](auto key, int32_t &value) {
            value = value + 10;
            return true;
        });
        if (i % 3 == 0) {
            EXPECT_TRUE(ret);
        } else {
            EXPECT_FALSE(ret);
        }
    }
}

/**
 * @tc.name: EmptyStringKeyTest
 * @tc.desc: Test Compute with empty string key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EmptyStringKeyTest, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    bool ret = map.Compute("", [](const std::string &key, std::string &value) {
        value = "empty_key_value";
        return true;
    });
    EXPECT_TRUE(ret);
    std::string retrievedValue = "";
    map.Compute("", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, "empty_key_value");
}

/**
 * @tc.name: VeryLongStringKeyTest
 * @tc.desc: Test Compute with very long string key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, VeryLongStringKeyTest, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, int32_t> map;
    std::string longKey = "very_long_key_test_new_" + std::string(300, 'x');
    bool ret = map.Compute(longKey, [](const std::string &key, int32_t &value) {
        value = 9999;
        return true;
    });
    EXPECT_TRUE(ret);
    int32_t retrievedValue = 0;
    map.Compute(longKey, [&retrievedValue](const std::string &key, int32_t &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, 9999);
}

/**
 * @tc.name: Uint32KeyTest
 * @tc.desc: Test Compute with uint32_t keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, Uint32KeyTest, TestSize.Level0)
{
    ConcurrentStripedMap<uint32_t, uint32_t> map;
    for (uint32_t i = 0; i < 70; ++i) {
        map.Compute(i, [i](auto key, uint32_t &value) {
            value = i * 9;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 70);
    for (uint32_t i = 0; i < 70; ++i) {
        uint32_t retrievedValue = 0;
        map.Compute(i, [&retrievedValue, i](auto k, uint32_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, i * 9);
    }
}

/**
 * @tc.name: DoubleValueTest
 * @tc.desc: Test Compute with double values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, DoubleValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, double> map;
    for (int i = 0; i < 25; ++i) {
        map.Compute(i, [i](auto key, double &value) {
            value = i * 1.5;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 25);
    for (int i = 0; i < 25; ++i) {
        double retrievedValue = 0.0;
        map.Compute(i, [&retrievedValue, i](auto k, double &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_DOUBLE_EQ(retrievedValue, i * 1.5);
    }
}

/**
 * @tc.name: FloatValueTest
 * @tc.desc: Test Compute with float values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, FloatValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, float> map;
    for (int i = 0; i < 35; ++i) {
        map.Compute(i, [i](auto key, float &value) {
            value = i * 2.5f;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 35);
    for (int i = 0; i < 35; ++i) {
        float retrievedValue = 0.0f;
        map.Compute(i, [&retrievedValue, i](auto k, float &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_FLOAT_EQ(retrievedValue, i * 2.5f);
    }
}

/**
 * @tc.name: BooleanValueTest
 * @tc.desc: Test Compute with boolean values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, BooleanValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, bool> map;
    for (int i = 0; i < 45; ++i) {
        map.Compute(i, [i](auto key, bool &value) {
            value = (i % 2 == 0);
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 45);
    for (int i = 0; i < 45; ++i) {
        bool retrievedValue = false;
        map.Compute(i, [&retrievedValue, i](auto k, bool &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, (i % 2 == 0));
    }
}

/**
 * @tc.name: CharValueTest
 * @tc.desc: Test Compute with char values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, CharValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, char> map;
    for (int i = 0; i < 55; ++i) {
        map.Compute(i, [i](auto key, char &value) {
            value = 'A' + (i % 26);
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 55);
    for (int i = 0; i < 55; ++i) {
        char retrievedValue = '\0';
        map.Compute(i, [&retrievedValue, i](auto k, char &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, 'A' + (i % 26));
    }
}

/**
 * @tc.name: Int64ValueTest
 * @tc.desc: Test Compute with int64_t values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, Int64ValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int64_t> map;
    for (int i = 0; i < 65; ++i) {
        map.Compute(i, [i](auto key, int64_t &value) {
            value = static_cast<int64_t>(i) * 1000000000LL;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 65);
    for (int i = 0; i < 65; ++i) {
        int64_t retrievedValue = 0;
        map.Compute(i, [&retrievedValue, i](auto k, int64_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, static_cast<int64_t>(i) * 1000000000LL);
    }
}

/**
 * @tc.name: Uint64ValueTest
 * @tc.desc: Test Compute with uint64_t values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, Uint64ValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<uint32_t, uint64_t> map;
    for (uint32_t i = 0; i < 75; ++i) {
        map.Compute(i, [i](auto key, uint64_t &value) {
            value = static_cast<uint64_t>(i) * 2000000000ULL;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 75);
    for (uint32_t i = 0; i < 75; ++i) {
        uint64_t retrievedValue = 0;
        map.Compute(i, [&retrievedValue, i](auto k, uint64_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, static_cast<uint64_t>(i) * 2000000000ULL);
    }
}

/**
 * @tc.name: StringIntPairTest
 * @tc.desc: Test ConcurrentStripedMap with string-int pair values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, StringIntPairTest, TestSize.Level0)
{
    ConcurrentStripedMap<int, std::pair<std::string, int>> map;
    for (int i = 0; i < 30; ++i) {
        map.Compute(i, [i](int key, std::pair<std::string, int> &value) {
            value = std::make_pair("str_" + std::to_string(i), i * 10);
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 30);
    for (int i = 0; i < 30; ++i) {
        std::pair<std::string, int> retrievedPair;
        map.Compute(i, [&retrievedPair](int k, std::pair<std::string, int> &value) {
            retrievedPair = value;
            return false;
        });
        EXPECT_EQ(retrievedPair.first, "str_" + std::to_string(i));
        EXPECT_EQ(retrievedPair.second, i * 10);
    }
}

/**
 * @tc.name: ComputeIfPresentNoModify
 * @tc.desc: Test ComputeIfPresent without modifying values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentNoModify, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 50; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i * 12;
            return true;
        });
    }
    for (int i = 0; i < 50; ++i) {
        bool ret = map.ComputeIfPresent(i, [](auto key, int32_t &value) {
            return true;
        });
        EXPECT_TRUE(ret);
    }
    for (int i = 0; i < 50; ++i) {
        int32_t retrievedValue = 0;
        map.Compute(i, [&retrievedValue, i](auto k, int32_t &value) {
            retrievedValue = value;
            return false;
        });
        EXPECT_EQ(retrievedValue, i * 12);
    }
}

/**
 * @tc.name: EraseIfWithComplexCondition
 * @tc.desc: Test EraseIf with complex condition
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EraseIfWithComplexCondition, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 100; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i * i;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 100);
    size_t erasedCount = map.EraseIf([](const int32_t &key, int32_t &value) {
        return (key < 50 && value > 100) || (key >= 50 && value % 2 == 0);
    });
    EXPECT_EQ(map.Size(), 100 - erasedCount);
}

/**
 * @tc.name: DoActionMultipleTimes
 * @tc.desc: Test DoAction called multiple times
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, DoActionMultipleTimes, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 10; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
    }
    int count = 0;
    for (int i = 0; i < 5; ++i) {
        map.DoAction([&count]() {
            count++;
            return true;
        });
    }
    EXPECT_EQ(count, 5);
}

/**
 * @tc.name: ComputeOverwriteValue
 * @tc.desc: Test Compute to overwrite existing value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeOverwriteValue, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(100, [](auto key, int32_t &value) {
        value = 100;
        return true;
    });
    int32_t retrievedValue1 = 0;
    map.Compute(100, [&retrievedValue1](auto k, int32_t &value) {
        retrievedValue1 = value;
        return false;
    });
    EXPECT_EQ(retrievedValue1, 100);
    map.Compute(100, [](auto key, int32_t &value) {
        value = 200;
        return true;
    });
    int32_t retrievedValue2 = 0;
    map.Compute(100, [&retrievedValue2](auto k, int32_t &value) {
        retrievedValue2 = value;
        return false;
    });
    EXPECT_EQ(retrievedValue2, 200);
}

/**
 * @tc.name: StringKeyComputeOverwriteValue
 * @tc.desc: Test Compute with string key to overwrite existing value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, StringKeyComputeOverwriteValue, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    map.Compute("overwrite_key", [](const std::string &key, std::string &value) {
        value = "original_value";
        return true;
    });
    std::string retrievedValue1 = "";
    map.Compute("overwrite_key", [&retrievedValue1](const std::string &k, std::string &value) {
        retrievedValue1 = value;
        return false;
    });
    EXPECT_EQ(retrievedValue1, "original_value");
    map.Compute("overwrite_key", [](const std::string &key, std::string &value) {
        value = "new_value";
        return true;
    });
    std::string retrievedValue2 = "";
    map.Compute("overwrite_key", [&retrievedValue2](const std::string &k, std::string &value) {
        retrievedValue2 = value;
        return false;
    });
    EXPECT_EQ(retrievedValue2, "new_value");
}

/**
 * @tc.name: ComputeIfPresentReturnFalse
 * @tc.desc: Test ComputeIfPresent returning false
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentReturnFalse, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(123, [](auto key, int32_t &value) {
        value = 123;
        return true;
    });
    bool ret = map.ComputeIfPresent(123, [](auto key, int32_t &value) {
        return false;
    });
    EXPECT_FALSE(ret);
    EXPECT_EQ(map.Size(), 0);
    int32_t retrievedValue = 0;
    map.Compute(123, [&retrievedValue](auto k, int32_t &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, 0);
}

/**
 * @tc.name: EraseIfRemoveAll
 * @tc.desc: Test EraseIf to remove all elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EraseIfRemoveAll, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 40; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 40);
    size_t erasedCount = map.EraseIf([](const int32_t &key, int32_t &value) {
        return true;
    });
    EXPECT_EQ(erasedCount, 40);
    EXPECT_TRUE(map.Empty());
}

/**
 * @tc.name: EraseIfRemoveNone
 * @tc.desc: Test EraseIf to remove no elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EraseIfRemoveNone, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 50; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 50);
    size_t erasedCount = map.EraseIf([](const int32_t &key, int32_t &value) {
        return false;
    });
    EXPECT_EQ(erasedCount, 0);
    EXPECT_EQ(map.Size(), 50);
}

/**
 * @tc.name: ComputeWithDuplicateCalls
 * @tc.desc: Test Compute called multiple times with same key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeWithDuplicateCalls, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(999, [](auto key, int32_t &value) {
        value = 1;
        return true;
    });
    map.Compute(999, [](auto key, int32_t &value) {
        value = 2;
        return true;
    });
    map.Compute(999, [](auto key, int32_t &value) {
        value = 3;
        return true;
    });
    int32_t retrievedValue = 0;
    map.Compute(999, [&retrievedValue](auto k, int32_t &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, 3);
}

/**
 * @tc.name: ComputeIfPresentMultipleMaps
 * @tc.desc: Test ComputeIfPresent on multiple independent maps
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentMultipleMaps, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map1;
    ConcurrentStripedMap<std::string, int32_t> map2;
    for (int i = 0; i < 20; ++i) {
        map1.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
        std::string key = "key_" + std::to_string(i);
        map2.Compute(key, [i](const std::string &key, int32_t &value) {
            value = i * 2;
            return true;
        });
    }
    for (int i = 0; i < 20; ++i) {
        bool ret1 = map1.ComputeIfPresent(i, [](auto key, int32_t &value) {
            return true;
        });
        std::string key = "key_" + std::to_string(i);
        bool ret2 = map2.ComputeIfPresent(key, [](const std::string &key, int32_t &value) {
            return true;
        });
        EXPECT_TRUE(ret1);
        EXPECT_TRUE(ret2);
    }
}

/**
 * @tc.name: StringValueWithSpecialChars
 * @tc.desc: Test Compute with special characters in string values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, StringValueWithSpecialChars, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    map.Compute("special_key", [](const std::string &key, std::string &value) {
        value = "value_with_underscore-dash.dot";
        return true;
    });
    std::string retrievedValue = "";
    map.Compute("special_key", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, "value_with_underscore-dash.dot");
}

/**
 * @tc.name: EmptyStringValueTest
 * @tc.desc: Test Compute with empty string value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EmptyStringValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    map.Compute("empty_value_key", [](const std::string &key, std::string &value) {
        value = "";
        return true;
    });
    std::string retrievedValue = "not_empty";
    map.Compute("empty_value_key", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, "");
}

/**
 * @tc.name: LongStringValueTest
 * @tc.desc: Test Compute with long string value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, LongStringValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    std::string longValue = "long_value_" + std::string(200, 'a');
    map.Compute("long_value_key", [&longValue](const std::string &key, std::string &value) {
        value = longValue;
        return true;
    });
    std::string retrievedValue = "";
    map.Compute("long_value_key", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, longValue);
}

/**
 * @tc.name: ComputeWithMaxInt32Key
 * @tc.desc: Test Compute with maximum int32_t key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeWithMaxInt32Key, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    int32_t maxKey = INT32_MAX;
    map.Compute(maxKey, [maxKey](auto key, int32_t &value) {
        value = maxKey;
        return true;
    });
    int32_t retrievedValue = 0;
    map.Compute(maxKey, [&retrievedValue](auto k, int32_t &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, maxKey);
}

/**
 * @tc.name: ComputeWithMinInt32Key
 * @tc.desc: Test Compute with minimum int32_t key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeWithMinInt32Key, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    int32_t minKey = INT32_MIN;
    map.Compute(minKey, [minKey](auto key, int32_t &value) {
        value = minKey;
        return true;
    });
    int32_t retrievedValue = 0;
    map.Compute(minKey, [&retrievedValue](auto k, int32_t &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, minKey);
}

/**
 * @tc.name: MultipleEraseIfCalls
 * @tc.desc: Test multiple EraseIf calls
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, MultipleEraseIfCalls, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 120; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 120);
    size_t erased1 = map.EraseIf([](const int32_t &key, int32_t &value) {
        return key < 40;
    });
    EXPECT_EQ(erased1, 40);
    size_t erased2 = map.EraseIf([](const int32_t &key, int32_t &value) {
        return key >= 80 && key < 120;
    });
    EXPECT_EQ(erased2, 40);
    EXPECT_EQ(map.Size(), 40);
}

/**
 * @tc.name: ComputeIfPresentWithRemoval
 * @tc.desc: Test ComputeIfPresent followed by removal
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeIfPresentWithRemoval, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(500, [](auto key, int32_t &value) {
        value = 500;
        return true;
    });
    bool ret = map.ComputeIfPresent(500, [](auto key, int32_t &value) {
        return true;
    });
    EXPECT_TRUE(ret);
    map.EraseIf([](const int32_t &key, int32_t &value) {
        return key == 500;
    });
    EXPECT_EQ(map.Size(), 0);
    ret = map.ComputeIfPresent(500, [](auto key, int32_t &value) {
        return true;
    });
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ComputeAfterRemoval
 * @tc.desc: Test Compute after removal
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, ComputeAfterRemoval, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    map.Compute(600, [](auto key, int32_t &value) {
        value = 600;
        return true;
    });
    map.EraseIf([](const int32_t &key, int32_t &value) {
        return key == 600;
    });
    EXPECT_EQ(map.Size(), 0);
    bool ret = map.Compute(600, [](auto key, int32_t &value) {
        value = 700;
        return true;
    });
    EXPECT_TRUE(ret);
    int32_t retrievedValue = 0;
    map.Compute(600, [&retrievedValue](auto k, int32_t &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, 700);
}

/**
 * @tc.name: DoActionWithReturnValue
 * @tc.desc: Test DoAction with return value tracking
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, DoActionWithReturnValue, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    for (int i = 0; i < 10; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i;
            return true;
        });
    }
    int trueCount = 0;
    int falseCount = 0;
    for (int i = 0; i < 10; ++i) {
        map.DoAction([&trueCount, &falseCount, i]() {
            if (i % 2 == 0) {
                trueCount++;
                return true;
            } else {
                falseCount++;
                return false;
            }
        });
    }
    EXPECT_EQ(trueCount, 5);
    EXPECT_EQ(falseCount, 5);
}

/**
 * @tc.name: StringKeyWithValueUpdate
 * @tc.desc: Test updating values with string keys
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, StringKeyWithValueUpdate, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, int32_t> map;
    map.Compute("update_key", [](const std::string &key, int32_t &value) {
        value = 10;
        return true;
    });
    for (int i = 0; i < 10; ++i) {
        map.ComputeIfPresent("update_key", [](const std::string &key, int32_t &value) {
            value = value + 1;
            return true;
        });
    }
    int32_t retrievedValue = 0;
    map.Compute("update_key", [&retrievedValue](const std::string &key, int32_t &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, 20);
}

/**
 * @tc.name: VectorIntValueTest
 * @tc.desc: Test ConcurrentStripedMap with vector<int> values
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, VectorIntValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, std::vector<int>> map;
    for (int i = 0; i < 30; ++i) {
        map.Compute(i, [i](auto key, std::vector<int> &value) {
            value = { i, i * 2, i * 3 };
            return true;
        });
    }
    EXPECT_EQ(map.Size(), 30);
    for (int i = 0; i < 30; ++i) {
        std::vector<int> retrievedVec;
        map.Compute(i, [&retrievedVec, i](auto k, std::vector<int> &value) {
            retrievedVec = value;
            return false;
        });
        EXPECT_EQ(retrievedVec.size(), 3);
        EXPECT_EQ(retrievedVec[0], i);
        EXPECT_EQ(retrievedVec[1], i * 2);
        EXPECT_EQ(retrievedVec[2], i * 3);
    }
}

/**
 * @tc.name: EmptyStringValueTestNew
 * @tc.desc: Test with empty string key and value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, EmptyStringValueTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;
    bool ret = map.Compute("", [](const std::string &key, std::string &value) {
        value = "";
        return true;
    });
    EXPECT_TRUE(ret);
    std::string retrievedValue = "test";
    map.Compute("", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return false;
    });
    EXPECT_EQ(retrievedValue, "");
}

/**
 * @tc.name: FinalComprehensiveTestNew
 * @tc.desc: Comprehensive test with all operations
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTestNew, FinalComprehensiveTestNew, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    EXPECT_TRUE(map.Empty());
    EXPECT_EQ(map.Size(), 0);
    
    for (int i = 0; i < 100; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i * 10;
            return true;
        });
    }
    
    EXPECT_EQ(map.Size(), 100);
    EXPECT_FALSE(map.Empty());
    
    for (int i = 0; i < 100; i += 2) {
        bool ret = map.ComputeIfPresent(i, [](auto key, int32_t &value) {
            value = value * 2;
            return true;
        });
        EXPECT_TRUE(ret);
    }
    
    size_t erasedCount = map.EraseIf([](const int32_t &key, int32_t &value) {
        return key % 3 == 0;
    });
    EXPECT_EQ(erasedCount, 34);
    
    EXPECT_EQ(map.Size(), 66);
    
    map.EraseIf([](const int32_t &key, int32_t &value) {
        return true;
    });
    
    EXPECT_TRUE(map.Empty());
    EXPECT_EQ(map.Size(), 0);
}

} // namespace OHOS::Test
