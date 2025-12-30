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

#define LOG_TAG "ConcurrentStripedMap"

#include "concurrent_striped_map.h"

#include <future>
#include <thread>

#include "gtest/gtest.h"
using namespace testing::ext;
namespace OHOS::Test {
class ConcurrentStripedMapTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}

    static void TearDownTestCase(void) {}

protected:
    void SetUp() {}

    void TearDown() {}
};

/**
* @tc.name: ElementAddedWhenLambdaReturnsTrue
* @tc.desc: Test the bool Compute() function with a lambda that always returns true,
* verifying that the element is added to the map and can be retrieved with correct value
* @tc.type: FUNC
* @tc.require: Verify the Compute method behavior when lambda returns true for new key insertion
*/
HWTEST_F(ConcurrentStripedMapTest, ElementAddedUsingCompute, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.Compute(1, [](auto key, int32_t &value) {
        value = 1;
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);
    int32_t value = 0;
    map.Compute(1, [&value](auto k, int32_t &v) {
        value = v;
        return false;
    });
    EXPECT_EQ(value, 1);
}

/**
 * @tc.name: ComputeNullActionTest
 * @tc.desc: Test Compute method with null action
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, ComputeNullActionTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.Compute(1, nullptr);
    EXPECT_FALSE(ret);
    EXPECT_EQ(map.Size(), 0);
}

/**
 * @tc.name: ComputeRemoveElementTest
 * @tc.desc: Test Compute method that removes an element
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, ComputeRemoveElementTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    // Add an element
    map.Compute(1, [](auto key, int32_t &value) {
        value = 200;
        return true;
    });
    EXPECT_EQ(map.Size(), 1);

    // Remove the element
    bool ret = map.Compute(1, [](auto key, int32_t &value) {
        return false; // Remove element
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 0);
}

/**
 * @tc.name: ComputeIfPresentNullActionTest
 * @tc.desc: Test ComputeIfPresent method with null action
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, ComputeIfPresentNullActionTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.ComputeIfPresent(1, nullptr);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: ComputeIfPresentNonExistentKeyTest
 * @tc.desc: Test ComputeIfPresent method with non-existent key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, ComputeIfPresentNonExistentKeyTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    bool ret = map.ComputeIfPresent(1, [](auto key, int32_t &value) {
        value = 300;
        return true;
    });
    EXPECT_FALSE(ret);
    EXPECT_TRUE(map.Empty());
}

/**
 * @tc.name: ComputeIfPresentExistingKeyTest
 * @tc.desc: Test ComputeIfPresent method with existing key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, ComputeIfPresentExistingKeyTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    // Add an element first
    map.Compute(1, [](auto key, int32_t &value) {
        value = 400;
        return true;
    });

    // Now use ComputeIfPresent
    bool ret = map.ComputeIfPresent(1, [](auto key, int32_t &value) {
        EXPECT_EQ(value, 400);
        value = 500; // Modify the value
        return true; // Keep element
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);

    // Verify the value was modified
    int32_t retrievedValue = 0;
    map.Compute(1, [&retrievedValue](auto key, int32_t &value) {
        retrievedValue = value;
        return true;
    });
    EXPECT_EQ(retrievedValue, 500);
}

/**
 * @tc.name: EraseIfNullActionTest
 * @tc.desc: Test EraseIf method with null action
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, EraseIfNullActionTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    // Add some elements
    map.Compute(1, [](auto key, int32_t &value) {
        value = 10;
        return true;
    });
    map.Compute(2, [](auto key, int32_t &value) {
        value = 20;
        return true;
    });
    EXPECT_EQ(map.Size(), 2);

    size_t count = map.EraseIf(nullptr);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(map.Size(), 2); // No elements should be erased
}

/**
 * @tc.name: EraseIfWithConditionTest
 * @tc.desc: Test EraseIf method with a condition that removes some elements
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, EraseIfWithConditionTest, TestSize.Level0)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    // Add elements with different values
    map.Compute(1, [](auto key, int32_t &value) {
        value = 10;
        return true;
    });
    map.Compute(2, [](auto key, int32_t &value) {
        value = 20;
        return true;
    });
    map.Compute(3, [](auto key, int32_t &value) {
        value = 5;
        return true;
    });
    EXPECT_EQ(map.Size(), 3);

    // Remove elements with value < 15
    size_t count = map.EraseIf([](const int32_t &key, int32_t &value) {
        return value < 15; // Remove if value is less than 15
    });

    EXPECT_EQ(count, 2);      // Should remove keys 1 and 3
    EXPECT_EQ(map.Size(), 1); // Only key 2 should remain

    // Verify remaining element
    int32_t remainingValue = 0;
    bool exists = false;
    map.ComputeIfPresent(2, [&remainingValue, &exists](auto key, int32_t &value) {
        remainingValue = value;
        exists = true;
        return true;
    });
    EXPECT_TRUE(exists);
    EXPECT_EQ(remainingValue, 20);

    // Verify removed elements are gone
    exists = false;
    map.ComputeIfPresent(1, [&exists](auto key, int32_t &value) {
        exists = true;
        return true;
    });
    EXPECT_FALSE(exists);
}

/**
 * @tc.name: DoActionNullActionTest
 * @tc.desc: Test DoAction method with null action、 action return false、 action return true
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, DoActionNullActionTest, TestSize.Level0)
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
 * @tc.name: ConcurrentStripedMapStringKeyTest
 * @tc.desc: Test ConcurrentStripedMap with string key and value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, StringKeyAndValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;

    // Add string key-value pair
    bool ret = map.Compute("key1", [](const std::string &key, std::string &value) {
        value = "value1";
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);

    // Retrieve and verify value
    std::string retrievedValue = "";
    map.Compute("key1", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return true;
    });
    EXPECT_EQ(retrievedValue, "value1");
}

/**
 * @tc.name: ConcurrentStripedMapStringKeyComputeIfPresentTest
 * @tc.desc: Test ComputeIfPresent with string key
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, StringKeyComputeIfPresentTest, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::string> map;

    // Add element first
    map.Compute("test_key", [](const std::string &key, std::string &value) {
        value = "test_value";
        return true;
    });

    // Use ComputeIfPresent to modify existing value
    bool ret = map.ComputeIfPresent("test_key", [](const std::string &key, std::string &value) {
        value = "modified_value";
        return true;
    });
    EXPECT_TRUE(ret);

    // Verify the modified value
    std::string retrievedValue = "";
    map.Compute("test_key", [&retrievedValue](const std::string &key, std::string &value) {
        retrievedValue = value;
        return true;
    });
    EXPECT_EQ(retrievedValue, "modified_value");
}

struct TestObject {
    int id;
    std::string name;

    TestObject() : id(0), name("") {}
    TestObject(int i, const std::string &n) : id(i), name(n) {}

    bool operator==(const TestObject &other) const
    {
        return id == other.id && name == other.name;
    }
};

/**
 * @tc.name: ConcurrentStripedMapCustomObjectTest
 * @tc.desc: Test ConcurrentStripedMap with custom object as value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, CustomObjectTest, TestSize.Level0)
{
    ConcurrentStripedMap<int, TestObject> map;

    // Add custom object
    bool ret = map.Compute(1, [](int key, TestObject &value) {
        value = TestObject(100, "test_object");
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);

    // Retrieve and verify custom object
    TestObject retrievedObj;
    map.Compute(1, [&retrievedObj](int key, TestObject &value) {
        retrievedObj = value;
        return true;
    });

    EXPECT_EQ(retrievedObj.id, 100);
    EXPECT_EQ(retrievedObj.name, "test_object");
}

/**
 * @tc.name: ConcurrentStripedMapCustomObjectModifyTest
 * @tc.desc: Test modifying custom object in map
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, CustomObjectModifyTest, TestSize.Level0)
{
    ConcurrentStripedMap<int, TestObject> map;

    // Add initial object
    map.Compute(1, [](int key, TestObject &value) {
        value = TestObject(1, "original");
        return true;
    });

    // Modify the object
    map.Compute(1, [](int key, TestObject &value) {
        value.id = 2;
        value.name = "modified";
        return true;
    });

    // Verify modifications
    TestObject retrievedObj;
    map.Compute(1, [&retrievedObj](int key, TestObject &value) {
        retrievedObj = value;
        return true;
    });

    EXPECT_EQ(retrievedObj.id, 2);
    EXPECT_EQ(retrievedObj.name, "modified");
}

/**
 * @tc.name: ConcurrentStripedMapVectorValueTest
 * @tc.desc: Test ConcurrentStripedMap with vector as value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, VectorValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<int, std::vector<int>> map;

    // Add vector value
    bool ret = map.Compute(1, [](int key, std::vector<int> &value) {
        value = { 1, 2, 3, 4, 5 };
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);

    // Modify the vector
    map.Compute(1, [](int key, std::vector<int> &value) {
        value.push_back(6);
        return true;
    });

    // Retrieve and verify vector
    std::vector<int> retrievedVec;
    map.Compute(1, [&retrievedVec](int key, std::vector<int> &value) {
        retrievedVec = value;
        return true;
    });

    EXPECT_EQ(retrievedVec.size(), 6);
    EXPECT_EQ(retrievedVec[5], 6);
}

/**
 * @tc.name: ConcurrentStripedMapPairValueTest
 * @tc.desc: Test ConcurrentStripedMap with pair as value
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, PairValueTest, TestSize.Level0)
{
    ConcurrentStripedMap<std::string, std::pair<int, std::string>> map;

    // Add pair value
    bool ret = map.Compute("pair_key", [](const std::string &key, std::pair<int, std::string> &value) {
        value = std::make_pair(42, "pair_value");
        return true;
    });
    EXPECT_TRUE(ret);
    EXPECT_EQ(map.Size(), 1);

    // Retrieve and verify pair
    std::pair<int, std::string> retrievedPair;
    map.Compute("pair_key", [&retrievedPair](const std::string &key, std::pair<int, std::string> &value) {
        retrievedPair = value;
        return true;
    });

    EXPECT_EQ(retrievedPair.first, 42);
    EXPECT_EQ(retrievedPair.second, "pair_value");
}

/**
 * @tc.name: ConcurrentEraseIfTest
 * @tc.desc: Test concurrent EraseIf operations from different threads with overlapping conditions
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, ConcurrentEraseIfTest, TestSize.Level0)
{
    const uint32_t mapSize = 100;
    const uint32_t numThreads = 4;
    ConcurrentStripedMap<int32_t, int32_t> map;
    // Add multiple elements
    for (int i = 0; i < mapSize; ++i) {
        map.Compute(i, [i](auto key, int32_t &value) {
            value = i * 10; // Store value as key*10
            return true;
        });
    }
    EXPECT_EQ(map.Size(), mapSize);

    // Create multiple threads that will erase different ranges with overlapping conditions
    std::vector<std::thread> threads;
    std::atomic_uint32_t totalErased = 0;
    auto eraseAction = [&map, &totalErased](size_t i) mutable {
        map.EraseIf([i, &totalErased](const int32_t &key, int32_t &value) {
            if (key >= 20 * i && key <= 20 * i + 40) {
                totalErased++;
                return true;
            }
            return false;
        });
    };
    for (size_t i = 0; i < numThreads; ++i) {
        // Thread i: Remove elements with keys in range [20 * i, 20 * i + 40]
        threads.emplace_back([&eraseAction, i]() {
            eraseAction(i);
        });
    }

    // Wait for all threads to complete
    for (auto &thread : threads) {
        thread.join();
    }

    // Check that the remaining size + total erased equals original size
    EXPECT_EQ(totalErased, mapSize);
    EXPECT_EQ(map.Size(), 0);
}

/**
 * @tc.name: ConcurrentComputeSameKeyTest
 * @tc.desc: Test concurrent Compute operations on the same key from different threads
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, ConcurrentComputeSameKeyTest, TestSize.Level2)
{
    ConcurrentStripedMap<int32_t, int32_t> map;
    const uint32_t numThreads = 4;
    const uint32_t opsPerThread = 100;
    std::atomic<uint32_t> operationCount = 0;
    std::vector<std::thread> threads;
    auto operation = [&map, &operationCount]() {
        for (uint32_t i = 0; i < opsPerThread; ++i) {
            map.Compute(1, [&operationCount](auto k, int32_t &value) {
                operationCount++;
                return true;
            });
        }
    };
    // Create multiple threads that will modify the same key concurrently
    for (uint32_t t = 0; t < numThreads; ++t) {
        threads.emplace_back([&operation]() {
            operation();
        });
    }

    // Wait for all threads to complete
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_EQ(operationCount.load(), numThreads * opsPerThread);
}

/**
 * @tc.name: ConcurrentComputeDifferentKeysTest
 * @tc.desc: Test concurrent Compute operations on different keys from multiple threads
 * @tc.type: FUNC
 */
HWTEST_F(ConcurrentStripedMapTest, ConcurrentComputeDifferentKeysTest, TestSize.Level0)
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
    EXPECT_TRUE(threadStartedFuture.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready);
    for (int i = 1; i < 10; ++i) {
        auto res = map.Compute(i, [&sharedValue](auto k, int32_t &value) {
            value = ++sharedValue;
            return true;
        });
        EXPECT_TRUE(res);
    }
    blocker.set_value(); // Unblock the thread
    blockingThread.join();
    uint32_t finalValue = 0;
    map.ComputeIfPresent(0, [&finalValue](auto k, int32_t &value) {
        finalValue = value;
        return true;
    });
    EXPECT_EQ(sharedValue.load(), finalValue);
}
} // namespace OHOS::Test