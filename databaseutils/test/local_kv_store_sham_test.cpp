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
#define LOG_TAG "LocalKvStoreShamTest"

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
class LocalKvStoreShamTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager manager_Sham;
    static std::shared_ptr<SingleKvStore> kvStore_Sham;
    static Status status_Sham;
    static AppId appId_Sham;
    static StoreId storeId_Sham;
};
std::shared_ptr<SingleKvStore> LocalKvStoreShamTest::kvStore_Sham = nullptr;
Status LocalKvStoreShamTest::status_Sham = Status::ERROR;
DistributedKvDataManager LocalKvStoreShamTest::manager_Sham;
AppId LocalKvStoreShamTest::appId_Sham;
StoreId LocalKvStoreShamTest::storeId_Sham;

void LocalKvStoreShamTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/dev_local_sub", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void LocalKvStoreShamTest::TearDownTestCase(void)
{
    manager_Sham.CloseKvStore(appId_Sham, kvStore_Sham);
    kvStore_Sham = nullptr;
    manager_Sham.DeleteKvStore(appId_Sham, storeId_Sham, "/data/service/el1/public/database/dev_local_sub");
    (void)remove("/data/service/el1/public/database/dev_local_sub/kvdb");
    (void)remove("/data/service/el1/public/database/dev_local_sub");
}

void LocalKvStoreShamTest::SetUp(void)
{
    Options options;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/dev_local_sub");
    appId_Sham.appId = "dev_local_sub"; // define app name.
    storeId_Sham.storeId = "student";   // define kvstore(database) name
    manager_Sham.DeleteKvStore(appId_Sham, storeId_Sham, options.baseDir);
    // [create and] open and initialize kvstore instance.
    status_Sham = manager_Sham.GetSingleKvStore(options, appId_Sham, storeId_Sham, kvStore_Sham);
    ASSERT_EQ(Status::SUCCESS, status_Sham) << "wrong statusSham";
    ASSERT_NE(nullptr, kvStore_Sham) << "kvStore is nullptr";
}

void LocalKvStoreShamTest::TearDown(void)
{
    manager_Sham.CloseKvStore(appId_Sham, kvStore_Sham);
    kvStore_Sham = nullptr;
    manager_Sham.DeleteKvStore(appId_Sham, storeId_Sham);
}

class DeviceObserverShamTest : public KvStoreObserver {
public:
    std::vector<Entry> insertEntries_;
    std::vector<Entry> updateEntries_;
    std::vector<Entry> deleteEntries_;
    std::string deviceId_;
    bool isClear_ = false;
    DeviceObserverShamTest();
    ~DeviceObserverShamTest() = default;

    void OnChange(const ChangeNotification &changeNotification);

    // reset the callCount_ to zero.
    void ResetToZero();

    uint32_t GetCallCount(uint32_t valueSham = 1);

private:
    std::mutex mutex_;
    uint32_t callCount_ = 0;
    BlockData<uint32_t> value_ { 1, 0 };
};

DeviceObserverShamTest::DeviceObserverShamTest() { }

void DeviceObserverShamTest::OnChange(const ChangeNotification &changeNotification)
{
    ZLOGD("begin.");
    insertEntries_ = changeNotification.GetInsertEntries();
    updateEntries_ = changeNotification.GetUpdateEntries();
    deleteEntries_ = changeNotification.GetDeleteEntries();
    deviceId_ = changeNotification.GetDeviceId();
    isClear_ = changeNotification.IsClear();
    std::lock_guard<decltype(mutex_)> guard(mutex_);
    ++callCount_;
    value_.SetValue(callCount_);
}

void DeviceObserverShamTest::ResetToZero()
{
    std::lock_guard<decltype(mutex_)> guard(mutex_);
    callCount_ = 0;
    value_.Clear(0);
}

uint32_t DeviceObserverShamTest::GetCallCount(uint32_t valueSham)
{
    int retry = 0;
    uint32_t callTimes = 0;
    while (retry < valueSham) {
        callTimes = value_.GetValue();
        if (callTimes >= valueSham) {
            break;
        }
        std::lock_guard<decltype(mutex_)> guard(mutex_);
        callTimes = value_.GetValue();
        if (callTimes >= valueSham) {
            break;
        }
        value_.Clear(callTimes);
        retry++;
    }
    return callTimes;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore001
 * @tc.desc: Subscribe success
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore001 begin.");
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    observerSham = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore002
 * @tc.desc: Subscribe fail, observerSham is null
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore002, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore002 begin.");
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<DeviceObserverShamTest> observerSham = nullptr;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::INVALID_ARGUMENT, statusSham) << "SubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore003
 * @tc.desc: Subscribe success and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore003, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore003 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key keySham = "Id1";
    Value valueSham = "subscribe";
    statusSham = kvStore_Sham->Put(keySham, valueSham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    observerSham = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore004
 * @tc.desc: The same observerSham subscribe three times and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore004 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key keySham = "Id1";
    Value valueSham = "subscribe";
    statusSham = kvStore_Sham->Put(keySham, valueSham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore005
 * @tc.desc: The different observerSham subscribe three times and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore005 begin.");
    auto observer1 = std::make_shared<DeviceObserverShamTest>();
    auto observer2 = std::make_shared<DeviceObserverShamTest>();
    auto observer3 = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observer1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore failed, wrong statusSham";
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observer2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore failed, wrong statusSham";
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observer3);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore failed, wrong statusSham";

    Key keySham = "Id1";
    Value valueSham = "subscribe";
    statusSham = kvStore_Sham->Put(keySham, valueSham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "Putting data to KvStore failed, wrong statusSham";
    ASSERT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer3->GetCallCount()), 1);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observer1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observer2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observer3);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore006
 * @tc.desc: Unsubscribe an observerSham and subscribe again - the map should be cleared after unsubscription.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore006 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key1Sham, value1Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";

    Key key2Sham = "Id2";
    Value value2Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key2Sham, value2Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);

    kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    Key key3Sham = "Id3";
    Value value3Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key3Sham, value3Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore007
 * @tc.desc: Subscribe to an observerSham - OnChange callback is called multiple times after the put operation.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore007 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key1Sham, value1Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key key2Sham = "Id2";
    Value value2Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key2Sham, value2Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key key3Sham = "Id3";
    Value value3Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key3Sham, value3Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(3)), 3);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore008
* @tc.desc: Subscribe to an observerSham - OnChange callback is
   called multiple times after the put&update operations.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore008 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key1Sham, value1Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key key2Sham = "Id2";
    Value value2Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key2Sham, value2Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key key3Sham = "Id1";
    Value value3Sham = "subscribe03";
    statusSham = kvStore_Sham->Put(key3Sham, value3Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(3)), 3);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore009
 * @tc.desc: Subscribe to an observerSham - OnChange callback is called multiple times after the putBatch operation.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore009 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    // before update.
    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id4";
    entrySham4.valueSham = "subscribe";
    entrySham5.keySham = "Id5";
    entrySham5.valueSham = "subscribe";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);

    statusSham = kvStore_Sham->PutBatch(entries1Sham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStore_Sham->PutBatch(entries2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore010
* @tc.desc: Subscribe to an observerSham - OnChange callback is
   called multiple times after the putBatch update operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore010 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    // before update.
    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id1";
    entrySham4.valueSham = "modify";
    entrySham5.keySham = "Id2";
    entrySham5.valueSham = "modify";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);

    statusSham = kvStore_Sham->PutBatch(entries1Sham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStore_Sham->PutBatch(entries2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore011
 * @tc.desc: Subscribe to an observerSham - OnChange callback is called after successful deletion.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore011 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore012
 * @tc.desc: Subscribe to an observerSham - OnChange callback is not called after deletion of non-existing keys.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore012 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->Delete("Id4");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore013
 * @tc.desc: Subscribe to an observerSham - OnChange callback is called after KvStore is cleared.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore013 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore014
* @tc.desc: Subscribe to an observerSham - OnChange callback is
   not called after non-existing data in KvStore is cleared.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore014 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore015
 * @tc.desc: Subscribe to an observerSham - OnChange callback is called after the deleteBatch operation.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore015 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStore_Sham->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore016
 * @tc.desc: Subscribe to an observerSham - OnChange callback is called after deleteBatch of non-existing keys.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore016 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStore_Sham->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore020
 * @tc.desc: Unsubscribe an observerSham two times.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStore020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore020 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::STORE_NOT_SUBSCRIBE, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification001
* @tc.desc: Subscribe to an observerSham successfully - callback is
   called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification001 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key keySham = "Id1";
    Value valueSham = "subscribe";
    statusSham = kvStore_Sham->Put(keySham, valueSham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ZLOGD("kvstore_ddm_subscribekvstore_003");
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ZLOGD("kvstore_ddm_subscribekvstore_003 size:%zu.", observerSham->insertEntries_.size());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification002
* @tc.desc: Subscribe to the same observerSham three times - callback is
   called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification002, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification002 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key keySham = "Id1";
    Value valueSham = "subscribe";
    statusSham = kvStore_Sham->Put(keySham, valueSham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification003
 * @tc.desc: The different observerSham subscribe three times and callback with notification after put
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification003, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification003 begin.");
    auto observer1 = std::make_shared<DeviceObserverShamTest>();
    auto observer2 = std::make_shared<DeviceObserverShamTest>();
    auto observer3 = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observer1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observer2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observer3);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key keySham = "Id1";
    Value valueSham = "subscribe";
    statusSham = kvStore_Sham->Put(keySham, valueSham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer1->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observer1->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observer1->insertEntries_[0].valueSham.ToString());

    ASSERT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer2->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observer2->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observer2->insertEntries_[0].valueSham.ToString());

    ASSERT_EQ(static_cast<int>(observer3->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer3->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observer3->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observer3->insertEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observer1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observer2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observer3);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification004
 * @tc.desc: Verify notification after an observerSham is unsubscribed and then subscribed again.
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification004 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key1Sham, value1Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";

    Key key2Sham = "Id2";
    Value value2Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key2Sham, value2Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    Key key3Sham = "Id3";
    Value value3Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key3Sham, value3Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id3", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification005
 * @tc.desc: Subscribe to an observerSham, callback with notification many times after put the different data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification005 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key1Sham, value1Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    Key key2Sham = "Id2";
    Value value2Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key2Sham, value2Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id2", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    Key key3Sham = "Id3";
    Value value3Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key3Sham, value3Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(3)), 3);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id3", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification006
 * @tc.desc: Subscribe to an observerSham, callback with notification many times after put the same data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification006 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key1Sham, value1Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    Key key2Sham = "Id1";
    Value value2Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key2Sham, value2Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->updateEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->updateEntries_[0].valueSham.ToString());

    Key key3Sham = "Id1";
    Value value3Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key3Sham, value3Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(3)), 3);
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->updateEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->updateEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification007
 * @tc.desc: Subscribe to an observerSham, callback with notification many times after put&update
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification007 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";
    Status statusSham = kvStore_Sham->Put(key1Sham, value1Sham);
    // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key key2Sham = "Id2";
    Value value2Sham = "subscribe";
    statusSham = kvStore_Sham->Put(key2Sham, value2Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key3Sham = "Id1";
    Value value3Sham = "subscribe03";
    statusSham = kvStore_Sham->Put(key3Sham, value3Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->updateEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe03", observerSham->updateEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification008
 * @tc.desc: Subscribe to an observerSham, callback with notification one times after putbatch&update
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification008 begin.");
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    entries.clear();
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe_modify";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe_modify";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 2);
    ASSERT_EQ("Id1", observerSham->updateEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe_modify", observerSham->updateEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->updateEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe_modify", observerSham->updateEntries_[1].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification009
 * @tc.desc: Subscribe to an observerSham, callback with notification one times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification009 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 3);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->insertEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[1].valueSham.ToString());
    ASSERT_EQ("Id3", observerSham->insertEntries_[2].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[2].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification010
* @tc.desc: Subscribe to an observerSham,
   callback with notification one times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification010 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id1";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id2";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 2);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->insertEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[1].valueSham.ToString());
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification011
 * @tc.desc: Subscribe to an observerSham, callback with notification one times after putbatch all same data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification011 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id1";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id1";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification012
 * @tc.desc: Subscribe to an observerSham, callback with notification many times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification012 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id4";
    entrySham4.valueSham = "subscribe";
    entrySham5.keySham = "Id5";
    entrySham5.valueSham = "subscribe";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);

    statusSham = kvStore_Sham->PutBatch(entries1Sham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 3);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->insertEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[1].valueSham.ToString());
    ASSERT_EQ("Id3", observerSham->insertEntries_[2].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[2].valueSham.ToString());
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification012b
 * @tc.desc: Subscribe to an observerSham, callback with notification many times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification012b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification012b begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id4";
    entrySham4.valueSham = "subscribe";
    entrySham5.keySham = "Id5";
    entrySham5.valueSham = "subscribe";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);

    statusSham = kvStore_Sham->PutBatch(entries2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 2);
    ASSERT_EQ("Id4", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id5", observerSham->insertEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[1].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}
/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification013
* @tc.desc: Subscribe to an observerSham,
   callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification013 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id1";
    entrySham4.valueSham = "subscribe";
    entrySham5.keySham = "Id4";
    entrySham5.valueSham = "subscribe";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);

    statusSham = kvStore_Sham->PutBatch(entries1Sham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 3);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->insertEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[1].valueSham.ToString());
    ASSERT_EQ("Id3", observerSham->insertEntries_[2].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[2].valueSham.ToString());
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification013b
* @tc.desc: Subscribe to an observerSham,
   callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification013b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification013b begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id1";
    entrySham4.valueSham = "subscribe";
    entrySham5.keySham = "Id4";
    entrySham5.valueSham = "subscribe";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);
    statusSham = kvStore_Sham->PutBatch(entries2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->updateEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->updateEntries_[0].valueSham.ToString());
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id4", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}
/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification014
 * @tc.desc: Subscribe to an observerSham, callback with notification many times after putbatch all same data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification014 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id1";
    entrySham4.valueSham = "subscribe";
    entrySham5.keySham = "Id2";
    entrySham5.valueSham = "subscribe";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);

    statusSham = kvStore_Sham->PutBatch(entries1Sham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 3);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->insertEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[1].valueSham.ToString());
    ASSERT_EQ("Id3", observerSham->insertEntries_[2].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[2].valueSham.ToString());

    statusSham = kvStore_Sham->PutBatch(entries2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 2);
    ASSERT_EQ("Id1", observerSham->updateEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->updateEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->updateEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->updateEntries_[1].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification015
 * @tc.desc: Subscribe to an observerSham, callback with notification many times after putbatch complex data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification015 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id1";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id1";
    entrySham4.valueSham = "subscribe";
    entrySham5.keySham = "Id2";
    entrySham5.valueSham = "subscribe";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);

    statusSham = kvStore_Sham->PutBatch(entries1Sham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 2);
    ASSERT_EQ("Id1", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id3", observerSham->insertEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[1].valueSham.ToString());
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification015b
 * @tc.desc: Subscribe to an observerSham, callback with notification many times after putbatch complex data
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification015b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification015b begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entries1Sham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id1";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries1Sham.push_back(entrySham1);
    entries1Sham.push_back(entrySham2);
    entries1Sham.push_back(entrySham3);

    std::vector<Entry> entries2;
    Entry entrySham4, entrySham5;
    entrySham4.keySham = "Id1";
    entrySham4.valueSham = "subscribe";
    entrySham5.keySham = "Id2";
    entrySham5.valueSham = "subscribe";
    entries2.push_back(entrySham4);
    entries2.push_back(entrySham5);
    statusSham = kvStore_Sham->PutBatch(entries2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->updateEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->updateEntries_[0].valueSham.ToString());
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 1);
    ASSERT_EQ("Id2", observerSham->insertEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->insertEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}
/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification016
 * @tc.desc: Pressure test subscribe, callback with notification many times after putbatch
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification016 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    int times = 100; // 100 times
    std::vector<Entry> entries;
    for (int i = 0; i < times; i++) {
        Entry entrySham;
        entrySham.keySham = std::to_string(i);
        entrySham.valueSham = "subscribe";
        entries.push_back(entrySham);
    }

    statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 100);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification017
 * @tc.desc: Subscribe to an observerSham, callback with notification after delete success
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification017, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification017 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->deleteEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->deleteEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification018
 * @tc.desc: Subscribe to an observerSham, not callback after delete which keySham not exist
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification018, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification018 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->Delete("Id4");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 0);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification019
* @tc.desc: Subscribe to an observerSham,
   delete the same data many times and only first delete callback with notification
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification019, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification019 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStore_Sham->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 1);
    ASSERT_EQ("Id1", observerSham->deleteEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->deleteEntries_[0].valueSham.ToString());

    statusSham = kvStore_Sham->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 1);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 1); // not callback so not clear

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification020
 * @tc.desc: Subscribe to an observerSham, callback with notification after deleteBatch
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification020 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStore_Sham->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 2);
    ASSERT_EQ("Id1", observerSham->deleteEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->deleteEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->deleteEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->deleteEntries_[1].valueSham.ToString());

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification021
 * @tc.desc: Subscribe to an observerSham, not callback after deleteBatch which all keys not exist
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification021, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification021 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStore_Sham->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 0);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification022
* @tc.desc: Subscribe to an observerSham,
   deletebatch the same data many times and only first deletebatch callback with
* notification
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification022, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification022 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id1";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id2";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id3";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStore_Sham->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 2);
    ASSERT_EQ("Id1", observerSham->deleteEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->deleteEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id2", observerSham->deleteEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->deleteEntries_[1].valueSham.ToString());

    statusSham = kvStore_Sham->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(2)), 1);
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 2); // not callback so not clear

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification023
 * @tc.desc: Subscribe to an observerSham, include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification023, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification023 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";

    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id2";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id3";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id4";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    statusSham = kvStore_Sham->Put(key1Sham, value1Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStore_Sham->Delete(key1Sham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore delete data return wrong statusSham";
    statusSham = kvStore_Sham->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount(4)), 4);
    // every callback will clear vector
    ASSERT_EQ(static_cast<int>(observerSham->deleteEntries_.size()), 2);
    ASSERT_EQ("Id2", observerSham->deleteEntries_[0].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->deleteEntries_[0].valueSham.ToString());
    ASSERT_EQ("Id3", observerSham->deleteEntries_[1].keySham.ToString());
    ASSERT_EQ("subscribe", observerSham->deleteEntries_[1].valueSham.ToString());
    ASSERT_EQ(static_cast<int>(observerSham->updateEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(observerSham->insertEntries_.size()), 0);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification024
 * @tc.desc: Subscribe to an observerSham[use transaction], include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require: I5GG0N
 * @tc.author: sql
 */
HWTEST_F(LocalKvStoreShamTest, KvStoreDdmSubscribeKvStoreNotification024, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification024 begin.");
    auto observerSham = std::make_shared<DeviceObserverShamTest>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStore_Sham->SubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key key1Sham = "Id1";
    Value value1Sham = "subscribe";

    std::vector<Entry> entries;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.keySham = "Id2";
    entrySham1.valueSham = "subscribe";
    entrySham2.keySham = "Id3";
    entrySham2.valueSham = "subscribe";
    entrySham3.keySham = "Id4";
    entrySham3.valueSham = "subscribe";
    entries.push_back(entrySham1);
    entries.push_back(entrySham2);
    entries.push_back(entrySham3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    statusSham = kvStore_Sham->StartTransaction();
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore startTransaction return wrong statusSham";
    statusSham = kvStore_Sham->Put(key1Sham, value1Sham); // insert or update keySham-valueSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    statusSham = kvStore_Sham->PutBatch(entries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStore_Sham->Delete(key1Sham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore delete data return wrong statusSham";
    statusSham = kvStore_Sham->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    statusSham = kvStore_Sham->Commit();
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Commit return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham->GetCallCount()), 1);

    statusSham = kvStore_Sham->UnSubscribeKvStore(subscribeTypeSham, observerSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}