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
            startTime = 0;
            endTime = 0;
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
            int retry = 0;
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
    static std::shared_ptr<SingleKvStore> kvStore_;
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
    sleep(10);
}

/**
* @tc.name: SingleWrite
* @tc.desc: get the store id of the kv store
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
* @tc.name: MultiWrite
* @tc.desc: get the store id of the kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Yang Qing
*/
HWTEST_F(AutoSyncTimerTest, MultiWrite, TestSize.Level1)
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
            usleep(40);
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