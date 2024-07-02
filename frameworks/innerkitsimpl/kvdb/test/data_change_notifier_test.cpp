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

#include "data_change_notifier.h"

#include <gtest/gtest.h>

#include "block_data.h"
#include "kvdb_service_client.h"
#include "store_manager.h"

using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace std::chrono;
namespace OHOS::Test {
class DataChangeNotifierTest : public testing::Test {
public:
    class KVDBServiceMock : public KVDBServiceClient {
    private:
        static KVDBServiceMock *instance_;

    public:
        static KVDBServiceMock *GetInstance()
        {
            KVDBServiceClient::GetInstance();
            return instance_;
        }
        explicit KVDBServiceMock(const sptr<IRemoteObject> &object) : KVDBServiceClient(object)
        {
            instance_ = this;
        }
        virtual ~KVDBServiceMock()
        {
            instance_ = nullptr;
        }

        Status NotifyDataChange(const AppId &appId, const StoreId &storeId, uint64_t delay) override
        {
            endTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
            values_[appId.appId].insert(storeId.storeId);
            {
                std::lock_guard<decltype(mutex_)> guard(mutex_);
                ++callCount;
                value_.SetValue(callCount);
            }
            return KVDBServiceClient::NotifyDataChange(appId, storeId, delay);
        }

        uint32_t GetCallCount(uint32_t value)
        {
            uint32_t retry = 0;
            uint32_t callTimes = 0;
            while (retry < value) {
                callTimes = value_.GetValue();
                if (callTimes >= value) {
                    break;
                }
                std::lock_guard<decltype(mutex_)> guard(mutex_);
                callTimes = value_.GetValue();
                if (callTimes >= value) {
                    break;
                }
                value_.Clear(callTimes);
                retry++;
            }
            return callTimes;
        }

        void Reset()
        {
            std::lock_guard<decltype(mutex_)> guard(mutex_);
            callCount = 0;
            value_.Clear(0);
            startTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
            endTime = 0;
            values_.clear();
        }

        uint64_t startTime = 0;
        uint64_t endTime = 0;
        uint32_t callCount = 0;
        std::map<std::string, std::set<std::string>> values_;
        BlockData<uint32_t> value_{ 5, 0 };
        std::mutex mutex_;
    };
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const std::map<std::string, Status> &results) override
        {
            ASSERT_TRUE(true);
        }
        void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override
        {
            ASSERT_TRUE(true);
        }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static BrokerDelegator<KVDBServiceMock> delegator_;
    static constexpr int SLEEP_TIME = 10;
};
BrokerDelegator<DataChangeNotifierTest::KVDBServiceMock> DataChangeNotifierTest::delegator_;
DataChangeNotifierTest::KVDBServiceMock *DataChangeNotifierTest::KVDBServiceMock::instance_ = nullptr;
void DataChangeNotifierTest::SetUpTestCase(void)
{
}

void DataChangeNotifierTest::TearDownTestCase(void)
{
}

void DataChangeNotifierTest::SetUp(void)
{
}

void DataChangeNotifierTest::TearDown(void)
{
    sleep(SLEEP_TIME); // make sure the case has executed completely
}

/**
 * @tc.name: SingleWriteAtOnce
 * @tc.desc: single write, and notify change at once
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangjiang
 */
HWTEST_F(DataChangeNotifierTest, SingleWriteAtOnce, TestSize.Level0)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->Reset();
    DataChangeNotifier::GetInstance().DoNotifyChange("ut_test", { { "ut_test_store" } });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    auto it = instance->values_.find("ut_test");
    ASSERT_NE(it, instance->values_.end());
    ASSERT_EQ(it->second.count("ut_test_store"), 1);
    ASSERT_LT(instance->endTime - instance->startTime, 50);
}

/**
 * @tc.name: SingleWriteOverFiveKvStoreAtOnce
 * @tc.desc: single write over five kvstore, and notify change at once
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangjiang
 */
HWTEST_F(DataChangeNotifierTest, SingleWriteOverFiveKvStoreAtOnce, TestSize.Level0)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->Reset();
    std::set<StoreId> storeIds;
    const int NUM = 6;
    for (int i = 0; i < NUM; i++) {
        storeIds.insert({ "ut_test_store_" + std::to_string(i) });
    }
    DataChangeNotifier::GetInstance().DoNotifyChange("ut_test", storeIds);
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(NUM)), NUM);
    auto it = instance->values_.find("ut_test");
    ASSERT_NE(it, instance->values_.end());
    ASSERT_EQ(it->second, std::set<std::string>(storeIds.begin(), storeIds.end()));
    ASSERT_LT(instance->endTime - instance->startTime, 50);
}

/**
 * @tc.name: MultiWrite
 * @tc.desc: write every 10 milliseconds reached 5 times
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangjiang
 */
HWTEST_F(DataChangeNotifierTest, MultiWrite, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->Reset();
    std::thread thread([] {
        int times = 0;
        while (times < 5) {
            DataChangeNotifier::GetInstance().DoNotifyChange("ut_test", { { "ut_test_store" } });
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            times++;
        }
    });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    ASSERT_LT(instance->endTime - instance->startTime, 50);
    auto it = instance->values_.find("ut_test");
    ASSERT_NE(it, instance->values_.end());
    ASSERT_EQ(it->second.count("ut_test_store"), 1);
    thread.join();
}

/**
 * @tc.name: MultiWriteOverFiveKVStores
 * @tc.desc: write every 100 milliseconds reached 5 times
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zuojiangjiang
 */
HWTEST_F(DataChangeNotifierTest, MultiWriteOverFiveKVStores, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->Reset();
    std::set<StoreId> storeIds;
    const int NUM = 6;
    for (int i = 0; i < NUM; i++) {
        storeIds.insert({ "ut_test_store_" + std::to_string(i) });
    }
    std::thread thread([storeIds] {
        int times = 0;
        while (times < 5) {
            DataChangeNotifier::GetInstance().DoNotifyChange("ut_test", storeIds);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            times++;
        }
    });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(NUM)), NUM);
    ASSERT_GE(instance->endTime - instance->startTime, 1400);
    auto it = instance->values_.find("ut_test");
    ASSERT_EQ(it->second, std::set<std::string>(storeIds.begin(), storeIds.end()));
    thread.join();
}

/**
* @tc.name: DoubleWrite
* @tc.desc: double wirte
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DataChangeNotifierTest, DoubleWrite, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->Reset();
    const int NUM = 6;
    for (int i = 0; i < NUM; i++) {
        DataChangeNotifier::GetInstance().DoNotifyChange("ut_test", { { "ut_test_store" } });
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(NUM)), NUM);
    ASSERT_GE(instance->endTime - instance->startTime, 500 * NUM);
    auto it = instance->values_.find("ut_test");
    ASSERT_EQ(it->second.count("ut_test_store"), 1);
}

/**
 * @tc.name: MultiWriteWithIncreasedStores
 * @tc.desc: Write multi times within the interval of increasing the number of databases
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: ht
 */
HWTEST_F(DataChangeNotifierTest, MultiWriteWithIncreasedStores, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->Reset();
    std::set<StoreId> storeIds;
    const int NUM = 6;
    for (int i = 0; i < NUM; i++) {
        storeIds.insert({ "ut_test_store_" + std::to_string(i) });
    }
    std::thread thread([storeIds] {
        int times = 0;
        while (times < 5) {
            auto end = ++storeIds.begin();
            DataChangeNotifier::GetInstance().DoNotifyChange("ut_test", { storeIds.begin(), end });
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            times++;
        }
    });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(NUM)), NUM);
    ASSERT_LE(instance->endTime - instance->startTime, 100);
    auto it = instance->values_.find("ut_test");
    ASSERT_EQ(it->second, std::set<std::string>(storeIds.begin(), storeIds.end()));
    thread.join();
}
} // namespace OHOS::Test