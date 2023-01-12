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

#include "auto_sync_timer.h"

#include <gtest/gtest.h>

#include "block_data.h"
#include "kvdb_service_client.h"
#include "store_manager.h"

using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace std::chrono;
namespace OHOS::Test {
class AutoSyncTimerTest : public testing::Test {
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

        Status Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override
        {
            endTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
            values_[appId.appId].insert(storeId.storeId);
            {
                std::lock_guard<decltype(mutex_)> guard(mutex_);
                ++callCount;
                value_.SetValue(callCount);
            }
            return KVDBServiceClient::Sync(appId, storeId, syncInfo);
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

        void ResetToZero()
        {
            std::lock_guard<decltype(mutex_)> guard(mutex_);
            callCount = 0;
            value_.Clear(0);
        }

        uint64_t startTime = 0;
        uint64_t endTime = 0;
        uint32_t callCount = 0;
        std::map<std::string, std::set<std::string>> values_;
        BlockData<uint32_t> value_{ 1, 0 };
        std::mutex mutex_;
    };
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const std::map<std::string, Status> &results) override
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
};
BrokerDelegator<AutoSyncTimerTest::KVDBServiceMock> AutoSyncTimerTest::delegator_;
AutoSyncTimerTest::KVDBServiceMock *AutoSyncTimerTest::KVDBServiceMock::instance_ = nullptr;
void AutoSyncTimerTest::SetUpTestCase(void)
{
}

void AutoSyncTimerTest::TearDownTestCase(void)
{
}

void AutoSyncTimerTest::SetUp(void)
{
}

void AutoSyncTimerTest::TearDown(void)
{
    sleep(10); // make sure the case has executed completely
}

/**
* @tc.name: SingleWrite
* @tc.desc:  single write
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Yang Qing
*/
HWTEST_F(AutoSyncTimerTest, SingleWrite, TestSize.Level0)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->ResetToZero();
    instance->startTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    instance->endTime = 0;
    instance->values_.clear();
    AutoSyncTimer::GetInstance().DoAutoSync("ut_test", { { "ut_test_store" } });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    auto it = instance->values_.find("ut_test");
    ASSERT_NE(it, instance->values_.end());
    ASSERT_EQ(it->second.count("ut_test_store"), 1);
    ASSERT_LT(instance->endTime - instance->startTime, 100);
}

/**
* @tc.name: MultiWriteBelowDelayTime
* @tc.desc: write every 40 milliseconds
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Yang Qing
*/
HWTEST_F(AutoSyncTimerTest, MultiWriteBelowDelayTime, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->ResetToZero();
    instance->startTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    instance->endTime = 0;
    instance->values_.clear();
    std::atomic_bool finished = false;
    std::thread thread([&finished] {
        while (!finished.load()) {
            AutoSyncTimer::GetInstance().DoAutoSync("ut_test", { { "ut_test_store" } });
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    ASSERT_GE(instance->endTime - instance->startTime, 200);
    ASSERT_LT(instance->endTime - instance->startTime, 250);
    finished.store(true);
    thread.join();
    auto it = instance->values_.find("ut_test");
    ASSERT_NE(it, instance->values_.end());
    ASSERT_EQ(it->second.count("ut_test_store"), 1);
}

/**
* @tc.name: MultiWriteOverDelayTime
* @tc.desc: write every 150 milliseconds
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Yang Qing
 */
HWTEST_F(AutoSyncTimerTest, MultiWriteOverDelayTime, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->ResetToZero();
    instance->startTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    instance->endTime = 0;
    instance->values_.clear();
    std::atomic_bool finished = false;
    std::thread thread([&finished] {
        while (!finished.load()) {
            AutoSyncTimer::GetInstance().DoAutoSync("ut_test", { { "ut_test_store" } });
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    ASSERT_LT(instance->endTime - instance->startTime, 100);
    finished.store(true);
    thread.join();
    auto it = instance->values_.find("ut_test");
    ASSERT_NE(it, instance->values_.end());
    ASSERT_EQ(it->second.count("ut_test_store"), 1);
}

/**
* @tc.name: MultiWriteOverForceTime
* @tc.desc: write every 400 milliseconds
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Yang Qing
 */
HWTEST_F(AutoSyncTimerTest, MultiWriteOverForceTime, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->ResetToZero();
    instance->startTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    instance->endTime = 0;
    instance->values_.clear();
    std::atomic_bool finished = false;
    std::thread thread([&finished] {
        while (!finished.load()) {
            AutoSyncTimer::GetInstance().DoAutoSync("ut_test", { { "ut_test_store" } });
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
    });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    ASSERT_LT(instance->endTime - instance->startTime, 100);
    finished.store(true);
    thread.join();
    auto it = instance->values_.find("ut_test");
    ASSERT_NE(it, instance->values_.end());
    ASSERT_EQ(it->second.count("ut_test_store"), 1);
}

/**
* @tc.name: SingleWriteOvertenKVStores
* @tc.desc: single write over ten kv stores
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Yang Qing
*/
HWTEST_F(AutoSyncTimerTest, SingleWriteOvertenKVStores, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->ResetToZero();
    instance->startTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    instance->endTime = 0;
    instance->values_.clear();
    AutoSyncTimer::GetInstance().DoAutoSync("ut_test", {
                                                           { "ut_test_store0" },
                                                           { "ut_test_store1" },
                                                           { "ut_test_store2" },
                                                           { "ut_test_store3" },
                                                           { "ut_test_store4" },
                                                           { "ut_test_store5" },
                                                           { "ut_test_store6" },
                                                           { "ut_test_store7" },
                                                           { "ut_test_store8" },
                                                           { "ut_test_store9" },
                                                           { "ut_test_store10" },
                                                       });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    ASSERT_LT(instance->endTime - instance->startTime, 100);
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(11)), 11);
    auto it = instance->values_.find("ut_test");
    ASSERT_NE(it, instance->values_.end());
    ASSERT_EQ(it->second.count("ut_test_store0"), 1);
    ASSERT_EQ(it->second.count("ut_test_store1"), 1);
    ASSERT_EQ(it->second.count("ut_test_store2"), 1);
    ASSERT_EQ(it->second.count("ut_test_store3"), 1);
    ASSERT_EQ(it->second.count("ut_test_store4"), 1);
    ASSERT_EQ(it->second.count("ut_test_store5"), 1);
    ASSERT_EQ(it->second.count("ut_test_store6"), 1);
    ASSERT_EQ(it->second.count("ut_test_store7"), 1);
    ASSERT_EQ(it->second.count("ut_test_store8"), 1);
    ASSERT_EQ(it->second.count("ut_test_store9"), 1);
    ASSERT_EQ(it->second.count("ut_test_store10"), 1);
}

/**
* @tc.name: MultiWriteOvertenKVStores
* @tc.desc: mulity wirte
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: YangQing
*/
HWTEST_F(AutoSyncTimerTest, MultiWriteOvertenKVStores, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->ResetToZero();
    instance->startTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    instance->endTime = 0;
    instance->values_.clear();
    std::atomic_bool finished = false;
    std::thread thread([&finished] {
        while (!finished.load()) {
            AutoSyncTimer::GetInstance().DoAutoSync("ut_test", {
                                                                   { "ut_test_store0" },
                                                                   { "ut_test_store1" },
                                                                   { "ut_test_store2" },
                                                                   { "ut_test_store3" },
                                                                   { "ut_test_store4" },
                                                                   { "ut_test_store5" },
                                                                   { "ut_test_store6" },
                                                                   { "ut_test_store7" },
                                                                   { "ut_test_store8" },
                                                                   { "ut_test_store9" },
                                                                   { "ut_test_store10" },
                                                               });
            usleep(40);
        }
    });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    ASSERT_GE(instance->endTime - instance->startTime, 200);
    ASSERT_LT(instance->endTime - instance->startTime, 250);
    finished.store(true);
    thread.join();
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(11)), 11);
    auto it = instance->values_.find("ut_test");
    ASSERT_EQ(it->second.count("ut_test_store0"), 1);
    ASSERT_EQ(it->second.count("ut_test_store1"), 1);
    ASSERT_EQ(it->second.count("ut_test_store2"), 1);
    ASSERT_EQ(it->second.count("ut_test_store3"), 1);
    ASSERT_EQ(it->second.count("ut_test_store4"), 1);
    ASSERT_EQ(it->second.count("ut_test_store5"), 1);
    ASSERT_EQ(it->second.count("ut_test_store6"), 1);
    ASSERT_EQ(it->second.count("ut_test_store7"), 1);
    ASSERT_EQ(it->second.count("ut_test_store8"), 1);
    ASSERT_EQ(it->second.count("ut_test_store9"), 1);
    ASSERT_EQ(it->second.count("ut_test_store10"), 1);
}

/**
* @tc.name: DoubleWrite
* @tc.desc: double wirte
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: YangQing
 */
HWTEST_F(AutoSyncTimerTest, DoubleWrite, TestSize.Level1)
{
    auto *instance = KVDBServiceMock::GetInstance();
    ASSERT_NE(instance, nullptr);
    instance->ResetToZero();
    instance->startTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    instance->endTime = 0;
    instance->values_.clear();
    AutoSyncTimer::GetInstance().DoAutoSync("ut_test", {
                                                           { "ut_test_store0" },
                                                           { "ut_test_store1" },
                                                           { "ut_test_store2" },
                                                           { "ut_test_store3" },
                                                           { "ut_test_store4" },
                                                           { "ut_test_store5" },
                                                           { "ut_test_store6" },
                                                           { "ut_test_store7" },
                                                           { "ut_test_store8" },
                                                           { "ut_test_store9" },
                                                           { "ut_test_store10" },
                                                       });
    AutoSyncTimer::GetInstance().DoAutoSync("ut_test", {
                                                           { "ut_test_store-0" },
                                                           { "ut_test_store-1" },
                                                           { "ut_test_store-2" },
                                                           { "ut_test_store-3" },
                                                           { "ut_test_store-4" },
                                                           { "ut_test_store-5" },
                                                           { "ut_test_store-6" },
                                                           { "ut_test_store-7" },
                                                           { "ut_test_store-8" },
                                                       });
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(1)), 1);
    ASSERT_LT(instance->endTime - instance->startTime, 100);
    EXPECT_EQ(static_cast<int>(instance->GetCallCount(20)), 20);
    auto it = instance->values_.find("ut_test");
    ASSERT_EQ(it->second.count("ut_test_store0"), 1);
    ASSERT_EQ(it->second.count("ut_test_store1"), 1);
    ASSERT_EQ(it->second.count("ut_test_store2"), 1);
    ASSERT_EQ(it->second.count("ut_test_store3"), 1);
    ASSERT_EQ(it->second.count("ut_test_store4"), 1);
    ASSERT_EQ(it->second.count("ut_test_store5"), 1);
    ASSERT_EQ(it->second.count("ut_test_store6"), 1);
    ASSERT_EQ(it->second.count("ut_test_store7"), 1);
    ASSERT_EQ(it->second.count("ut_test_store8"), 1);
    ASSERT_EQ(it->second.count("ut_test_store9"), 1);
    ASSERT_EQ(it->second.count("ut_test_store10"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-0"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-1"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-2"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-3"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-4"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-5"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-6"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-7"), 1);
    ASSERT_EQ(it->second.count("ut_test_store-8"), 1);
}
} // namespace OHOS::Test