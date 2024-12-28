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
#define LOG_TAG "LocalKvStoreCounterfeitUnitTest"

#include "block_data.h"
#include "distributed_kv_data_manager.h"
#include "log_print.h"
#include "types.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <mutex>
#include <vector>

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
class LocalKvStoreCounterfeitUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager counterfeitTestManger_;
    static std::shared_ptr<SingleKvStore> counterfeitTestStore_;
    static Status counterfeitTestStatus_;
    static AppId counterfeitTestAppId_;
    static StoreId counterfeitTestStoreId_;
};
std::shared_ptr<SingleKvStore> LocalKvStoreCounterfeitUnitTest::counterfeitTestStore_ = nullptr;
Status LocalKvStoreCounterfeitUnitTest::counterfeitTestStatus_ = Status::ERROR;
DistributedKvDataManager LocalKvStoreCounterfeitUnitTest::counterfeitTestManger_;
AppId LocalKvStoreCounterfeitUnitTest::counterfeitTestAppId_;
StoreId LocalKvStoreCounterfeitUnitTest::counterfeitTestStoreId_;

void LocalKvStoreCounterfeitUnitTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/local", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void LocalKvStoreCounterfeitUnitTest::TearDownTestCase(void)
{
    counterfeitTestManger_.CloseKvStore(counterfeitTestAppId_, counterfeitTestStore_);
    counterfeitTestStore_ = nullptr;
    counterfeitTestManger_.DeleteKvStore(counterfeitTestAppId_,
        counterfeitTestStoreId_, "/data/service/el1/public/database/local");
    (void)remove("/data/service/el1/public/database/local/kvdb");
    (void)remove("/data/service/el1/public/database/local");
}

void LocalKvStoreCounterfeitUnitTest::SetUp(void)
{
    Options counterfeitOptions;
    counterfeitOptions.securityLevel = S1;
    counterfeitOptions.baseDir = std::string("/data/service/el1/public/database/local");
    counterfeitTestAppId_.appId = "local"; // define app name.
    counterfeitTestStoreId_.storeId = "MAN";   // define kvstore(database) name
    counterfeitTestManger_.DeleteKvStore(counterfeitTestAppId_, counterfeitTestStoreId_, counterfeitOptions.baseDir);
    // [create and] open and initialize kvstore instance.
    counterfeitTestStatus_ = counterfeitTestManger_.GetSingleKvStore(counterfeitOptions,
        counterfeitTestAppId_, counterfeitTestStoreId_, counterfeitTestStore_);
    ASSERT_EQ(Status::SUCCESS, counterfeitTestStatus_) << "wrong status";
    ASSERT_NE(nullptr, counterfeitTestStore_) << "kvStore is nullptr";
}

void LocalKvStoreCounterfeitUnitTest::TearDown(void)
{
    counterfeitTestManger_.CloseKvStore(counterfeitTestAppId_, counterfeitTestStore_);
    counterfeitTestStore_ = nullptr;
    counterfeitTestManger_.DeleteKvStore(counterfeitTestAppId_, counterfeitTestStoreId_);
}

class DeviceObserverCounterfeitUnitTest : public KvStoreObserver {
public:
    std::vector<Entry> insertCounterfeitEntries_;
    std::vector<Entry> updateCounterfeitEntries_;
    std::vector<Entry> deleteCounterfeitEntries_;
    std::string counterfeitDeviceId_;
    bool isCounterfeitClear_ = false;
    DeviceObserverCounterfeitUnitTest();
    ~DeviceObserverCounterfeitUnitTest() = default;

    void OnChange(const ChangeNotification &changeNotification);

    // reset the counterfeitCallCount_ to zero.
    void ResetToZero();

    uint32_t GetCallCount(uint32_t counterfeitTestValue = 1);

private:
    std::mutex counterfeitMutex_;
    uint32_t counterfeitCallCount_ = 0;
    BlockData<uint32_t> counterfeitValue_ { 1, 0 };
};

DeviceObserverCounterfeitUnitTest::DeviceObserverCounterfeitUnitTest() { }

void DeviceObserverCounterfeitUnitTest::OnChange(const ChangeNotification &changeNotification)
{
    ZLOGD("begin.");
    insertCounterfeitEntries_ = changeNotification.GetInsertEntries();
    updateCounterfeitEntries_ = changeNotification.GetUpdateEntries();
    deleteCounterfeitEntries_ = changeNotification.GetDeleteEntries();
    counterfeitDeviceId_ = changeNotification.GetDeviceId();
    isCounterfeitClear_ = changeNotification.IsClear();
    std::lock_guard<decltype(counterfeitMutex_)> guard(counterfeitMutex_);
    ++counterfeitCallCount_;
    counterfeitValue_.SetValue(counterfeitCallCount_);
}

void DeviceObserverCounterfeitUnitTest::ResetToZero()
{
    std::lock_guard<decltype(counterfeitMutex_)> guard(counterfeitMutex_);
    counterfeitCallCount_ = 0;
    counterfeitValue_.Clear(0);
}

uint32_t DeviceObserverCounterfeitUnitTest::GetCallCount(uint32_t counterfeitTestValue)
{
    int retryTimes = 0;
    uint32_t callCount = 0;
    while (retryTimes < counterfeitTestValue) {
        callCount = counterfeitValue_.GetValue();
        if (callCount >= counterfeitTestValue) {
            break;
        }
        std::lock_guard<decltype(counterfeitMutex_)> guard(counterfeitMutex_);
        callCount = counterfeitValue_.GetValue();
        if (callCount >= counterfeitTestValue) {
            break;
        }
        counterfeitValue_.Clear(callCount);
        retryTimes++;
    }
    return callCount;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore001
 * @tc.desc: Subscribe success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore001 begin.");
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
    counterfeitObserver = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore002
 * @tc.desc: Subscribe fail, counterfeitObserver is null
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore002, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore002 begin.");
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<DeviceObserverCounterfeitUnitTest> counterfeitObserver = nullptr;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::INVALID_ARGUMENT, counterfeitStatus) << "SubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore003
 * @tc.desc: Subscribe success and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore003, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore003 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey = "Id1";
    Value counterfeitTestValue = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey, counterfeitTestValue);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
    counterfeitObserver = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore004
 * @tc.desc: The same counterfeitObserver subscribe three times and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore004 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey = "Id1";
    Value counterfeitTestValue = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey, counterfeitTestValue);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore005
 * @tc.desc: The different counterfeitObserver subscribe three times and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore005 begin.");
    auto observer1 = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    auto observer2 = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    auto observer3 = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, observer1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore failed, wrong";
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, observer2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore failed, wrong";
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, observer3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore failed, wrong";

    Key counterfeitTestKey = "Id1";
    Value counterfeitTestValue = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey, counterfeitTestValue);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "Putting data to KvStore failed, wrong";
    ASSERT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer3->GetCallCount()), 1);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, observer1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, observer2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, observer3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore006
 * @tc.desc: Unsubscribe an counterfeitObserver and subscribe again - the map should be cleared after unsubscription.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore006 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";

    Key counterfeitTestKey2 = "Id2";
    Value counterfeitTestValue2 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey2, counterfeitTestValue2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);

    counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    Key counterfeitTestKey3 = "Id3";
    Value counterfeitTestValue3 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey3, counterfeitTestValue3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore007
 * @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is called multiple times after the put operation.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore007 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";

    Key counterfeitTestKey2 = "Id2";
    Value counterfeitTestValue2 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey2, counterfeitTestValue2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";

    Key counterfeitTestKey3 = "Id3";
    Value counterfeitTestValue3 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey3, counterfeitTestValue3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(3)), 3);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore008
* @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is
  called multiple times after the put&update operations.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore008 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";

    Key counterfeitTestKey2 = "Id2";
    Value counterfeitTestValue2 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey2, counterfeitTestValue2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";

    Key counterfeitTestKey3 = "Id1";
    Value counterfeitTestValue3 = "subscribe03";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey3, counterfeitTestValue3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(3)), 3);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore009
 * @tc.desc: Subscribe to an counterfeitObserver - OnChange
 *           callback is called multiple times after the putBatch operation.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore009 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    // before update.
    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id4";
    counterfeitTestEnty4.counterfeitTestValue = "subscribe";
    counterfeitTestEnty5.counterfeitTestKey = "Id5";
    counterfeitTestEnty5.counterfeitTestValue = "subscribe";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore010
* @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is
  called multiple times after the putBatch update operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore010 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    // before update.
    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id1";
    counterfeitTestEnty4.counterfeitTestValue = "modify";
    counterfeitTestEnty5.counterfeitTestKey = "Id2";
    counterfeitTestEnty5.counterfeitTestValue = "modify";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore011
 * @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is called after successful deletion.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore011 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore012
 * @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is not called after deletion of non-existing keys.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore012 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->Delete("Id4");
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore013
 * @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is called after KvStore is cleared.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore013 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore014
* @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is
  not called after non-existing data in KvStore is cleared.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore014 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore015
 * @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is called after the deleteBatch operation.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore015 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    counterfeitStatus = counterfeitTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore016
 * @tc.desc: Subscribe to an counterfeitObserver - OnChange callback is called after deleteBatch of non-existing keys.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore016 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    counterfeitStatus = counterfeitTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore020
 * @tc.desc: Unsubscribe an counterfeitObserver two times.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStore020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore020 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::STORE_NOT_SUBSCRIBE, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification001
* @tc.desc: Subscribe to an counterfeitObserver successfully - callback is
  called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification001 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey = "Id1";
    Value counterfeitTestValue = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey, counterfeitTestValue);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ZLOGD("kvstore_ddm_subscribekvstore_003");
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ZLOGD("kvstore_ddm_subscribekvstore_003 size:%zu.", counterfeitObserver->insertCounterfeitEntries_.size());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification002
* @tc.desc: Subscribe to the same counterfeitObserver three times - callback is
  called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification002, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification002 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey = "Id1";
    Value counterfeitTestValue = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey, counterfeitTestValue);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification003
 * @tc.desc: The different counterfeitObserver subscribe three times and callback with notification after put
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification003, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification003 begin.");
    auto observer1 = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    auto observer2 = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    auto observer3 = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, observer1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, observer2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, observer3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey = "Id1";
    Value counterfeitTestValue = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey, counterfeitTestValue);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer1->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", observer1->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", observer1->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    ASSERT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer2->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", observer2->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", observer2->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    ASSERT_EQ(static_cast<int>(observer3->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer3->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", observer3->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", observer3->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, observer1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, observer2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, observer3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification004
 * @tc.desc: Verify notification after an counterfeitObserver is unsubscribed and then subscribed again.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification004 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";

    Key counterfeitTestKey2 = "Id2";
    Value counterfeitTestValue2 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey2, counterfeitTestValue2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    Key counterfeitTestKey3 = "Id3";
    Value counterfeitTestValue3 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey3, counterfeitTestValue3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id3", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification005
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification many times after put the different data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification005 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    Key counterfeitTestKey2 = "Id2";
    Value counterfeitTestValue2 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey2, counterfeitTestValue2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id2", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    Key counterfeitTestKey3 = "Id3";
    Value counterfeitTestValue3 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey3, counterfeitTestValue3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(3)), 3);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id3", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification006
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification many times after put the same data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification006 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    Key counterfeitTestKey2 = "Id1";
    Value counterfeitTestValue2 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey2, counterfeitTestValue2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestValue.ToString());

    Key counterfeitTestKey3 = "Id1";
    Value counterfeitTestValue3 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey3, counterfeitTestValue3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(3)), 3);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification007
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification many times after put&update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification007 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";
    Status counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
   
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";

    Key counterfeitTestKey2 = "Id2";
    Value counterfeitTestValue2 = "subscribe";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey2, counterfeitTestValue2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey3 = "Id1";
    Value counterfeitTestValue3 = "subscribe03";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey3, counterfeitTestValue3);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe03", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification008
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification one times after putbatch&update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification008 begin.");
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitTestEntries.clear();
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe_modify";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe_modify";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 2);
    ASSERT_EQ("Id1", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe_modify", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->updateCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe_modify", counterfeitObserver->updateCounterfeitEntries_[1].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification009
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification one times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification009 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 3);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestValue.ToString());
    ASSERT_EQ("Id3", counterfeitObserver->insertCounterfeitEntries_[2].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[2].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification010
* @tc.desc: Subscribe to an counterfeitObserver,
  callback with notification one times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification010 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id1";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id2";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 2);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestValue.ToString());
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification011
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification one times after putbatch all same data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification011 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id1";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id1";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification012
 * @tc.desc: Subscribe to an observer, callback with notification many times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification012 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id4";
    counterfeitTestEnty4.counterfeitTestValue = "subscribe";
    counterfeitTestEnty5.counterfeitTestKey = "Id5";
    counterfeitTestEnty5.counterfeitTestValue = "subscribe";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 3);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestValue.ToString());
    ASSERT_EQ("Id3", counterfeitObserver->insertCounterfeitEntries_[2].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[2].counterfeitTestValue.ToString());
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification012b
 * @tc.desc: Subscribe to an observer, callback with notification many times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification012b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification012b begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id4";
    counterfeitTestEnty4.counterfeitTestValue = "subscribe";
    counterfeitTestEnty5.counterfeitTestKey = "Id5";
    counterfeitTestEnty5.counterfeitTestValue = "subscribe";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 2);
    ASSERT_EQ("Id4", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id5", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}
/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification013
* @tc.desc: Subscribe to an counterfeitObserver,
  callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification013 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id1";
    counterfeitTestEnty4.counterfeitTestValue = "subscribe";
    counterfeitTestEnty5.counterfeitTestKey = "Id4";
    counterfeitTestEnty5.counterfeitTestValue = "subscribe";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 3);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestValue.ToString());
    ASSERT_EQ("Id3", counterfeitObserver->insertCounterfeitEntries_[2].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[2].counterfeitTestValue.ToString());
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification013b
* @tc.desc: Subscribe to an counterfeitObserver,
  callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification013b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification013b begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id1";
    counterfeitTestEnty4.counterfeitTestValue = "subscribe";
    counterfeitTestEnty5.counterfeitTestKey = "Id4";
    counterfeitTestEnty5.counterfeitTestValue = "subscribe";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);
    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id4", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}
/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification014
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification many times after putbatch all same data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification014 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id1";
    counterfeitTestEnty4.counterfeitTestValue = "subscribe";
    counterfeitTestEnty5.counterfeitTestKey = "Id2";
    counterfeitTestEnty5.counterfeitTestValue = "subscribe";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 3);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestValue.ToString());
    ASSERT_EQ("Id3", counterfeitObserver->insertCounterfeitEntries_[2].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[2].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 2);
    ASSERT_EQ("Id1", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->updateCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->updateCounterfeitEntries_[1].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification015
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification many times after putbatch complex data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification015 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id1";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id1";
    counterfeitTestEnty4.counterfeitTestValue = "subscribe";
    counterfeitTestEnty5.counterfeitTestKey = "Id2";
    counterfeitTestEnty5.counterfeitTestValue = "subscribe";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 2);
    ASSERT_EQ("Id1", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id3", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[1].counterfeitTestValue.ToString());
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification015b
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification many times after putbatch complex data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification015b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification015b begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> counterfeitTestEntries1;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;

    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id1";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries1.push_back(counterfeitTestEnty1);
    counterfeitTestEntries1.push_back(counterfeitTestEnty2);
    counterfeitTestEntries1.push_back(counterfeitTestEnty3);

    std::vector<Entry> counterfeitTestEntries2;
    Entry counterfeitTestEnty4, counterfeitTestEnty5;
    counterfeitTestEnty4.counterfeitTestKey = "Id1";
    counterfeitTestEnty4.counterfeitTestValue = "subscribe";
    counterfeitTestEnty5.counterfeitTestKey = "Id2";
    counterfeitTestEnty5.counterfeitTestValue = "subscribe";
    counterfeitTestEntries2.push_back(counterfeitTestEnty4);
    counterfeitTestEntries2.push_back(counterfeitTestEnty5);
    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries2);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->updateCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id2", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->insertCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}
/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification016
 * @tc.desc: Pressure test subscribe, callback with notification many times after putbatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification016 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    int times = 100; // 100 times
    std::vector<Entry> counterfeitTestEntries;
    for (int i = 0; i < times; i++) {
        Entry counterfeitTestEnty;
        counterfeitTestEnty.counterfeitTestKey = std::to_string(i);
        counterfeitTestEnty.counterfeitTestValue = "subscribe";
        counterfeitTestEntries.push_back(counterfeitTestEnty);
    }

    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 100);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification017
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification after delete success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification017, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification017 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification018
 * @tc.desc: Subscribe to an counterfeitObserver, not callback after delete which counterfeitTestKey not exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification018, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification018 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->Delete("Id4");
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 0);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification019
* @tc.desc: Subscribe to an counterfeitObserver,
  delete the same data many times and only first delete callback with notification
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification019, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification019 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";
    counterfeitStatus = counterfeitTestStore_->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 1);
    ASSERT_EQ("Id1", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 1);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification020
 * @tc.desc: Subscribe to an counterfeitObserver, callback with notification after deleteBatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification020 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    counterfeitStatus = counterfeitTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 2);
    ASSERT_EQ("Id1", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->deleteCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->deleteCounterfeitEntries_[1].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification021
 * @tc.desc: Subscribe to an counterfeitObserver, not callback after deleteBatch which all keys not exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification021, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification021 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    counterfeitStatus = counterfeitTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 0);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification022
* @tc.desc: Subscribe to an counterfeitObserver,
  deletebatch the same data many times and only first deletebatch callback with
* notification
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification022, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification022 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id1";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id2";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id3";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";

    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    counterfeitStatus = counterfeitTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 2);
    ASSERT_EQ("Id1", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id2", counterfeitObserver->deleteCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->deleteCounterfeitEntries_[1].counterfeitTestValue.ToString());

    counterfeitStatus = counterfeitTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(2)), 1);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 2);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification023
 * @tc.desc: Subscribe to an counterfeitObserver, include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification023, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification023 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";

    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id2";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id3";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id4";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    counterfeitStatus = counterfeitTestStore_->Delete(counterfeitTestKey1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore delete data return wrong";
    counterfeitStatus = counterfeitTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount(4)), 4);
    // every callback will clear vector
    ASSERT_EQ(static_cast<int>(counterfeitObserver->deleteCounterfeitEntries_.size()), 2);
    ASSERT_EQ("Id2", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->deleteCounterfeitEntries_[0].counterfeitTestValue.ToString());
    ASSERT_EQ("Id3", counterfeitObserver->deleteCounterfeitEntries_[1].counterfeitTestKey.ToString());
    ASSERT_EQ("subscribe", counterfeitObserver->deleteCounterfeitEntries_[1].counterfeitTestValue.ToString());
    ASSERT_EQ(static_cast<int>(counterfeitObserver->updateCounterfeitEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(counterfeitObserver->insertCounterfeitEntries_.size()), 0);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification024
 * @tc.desc: Subscribe to an counterfeitObserver[use transaction], include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreCounterfeitUnitTest, KvStoreDdmSubscribeKvStoreNotification024, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification024 begin.");
    auto counterfeitObserver = std::make_shared<DeviceObserverCounterfeitUnitTest>();
    SubscribeType counterfeitSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status counterfeitStatus = counterfeitTestStore_->SubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "SubscribeKvStore return wrong";

    Key counterfeitTestKey1 = "Id1";
    Value counterfeitTestValue1 = "subscribe";

    std::vector<Entry> counterfeitTestEntries;
    Entry counterfeitTestEnty1, counterfeitTestEnty2, counterfeitTestEnty3;
    counterfeitTestEnty1.counterfeitTestKey = "Id2";
    counterfeitTestEnty1.counterfeitTestValue = "subscribe";
    counterfeitTestEnty2.counterfeitTestKey = "Id3";
    counterfeitTestEnty2.counterfeitTestValue = "subscribe";
    counterfeitTestEnty3.counterfeitTestKey = "Id4";
    counterfeitTestEnty3.counterfeitTestValue = "subscribe";
    counterfeitTestEntries.push_back(counterfeitTestEnty1);
    counterfeitTestEntries.push_back(counterfeitTestEnty2);
    counterfeitTestEntries.push_back(counterfeitTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    counterfeitStatus = counterfeitTestStore_->StartTransaction();
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore startTransaction return wrong";
    counterfeitStatus = counterfeitTestStore_->Put(counterfeitTestKey1, counterfeitTestValue1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore put data return wrong";
    counterfeitStatus = counterfeitTestStore_->PutBatch(counterfeitTestEntries);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore putbatch data return wrong";
    counterfeitStatus = counterfeitTestStore_->Delete(counterfeitTestKey1);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore delete data return wrong";
    counterfeitStatus = counterfeitTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore DeleteBatch data return wrong";
    counterfeitStatus = counterfeitTestStore_->Commit();
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "KvStore Commit return wrong";
    ASSERT_EQ(static_cast<int>(counterfeitObserver->GetCallCount()), 1);

    counterfeitStatus = counterfeitTestStore_->UnSubscribeKvStore(counterfeitSubscribeType, counterfeitObserver);
    ASSERT_EQ(Status::SUCCESS, counterfeitStatus) << "UnSubscribeKvStore return wrong";
}