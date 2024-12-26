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

#define LOG_TAG "LocalKvStoreVirtualTest"
#include <cstdint>
#include <gtest/gtest.h>
#include <mutex>
#include <vector>
#include "block_data.h"
#include "distributed_kv_data_manager.h"
#include "log_print.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS;
class LocalKvStoreVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager manager_Virtual;
    static std::shared_ptr<SingleKvStore> kvStore_Virtual;
    static Status status_Virtual;
    static AppId appId_Virtual;
    static StoreId storeId_Virtual;
};
std::shared_ptr<SingleKvStore> LocalKvStoreVirtualTest::kvStore_Virtual = nullptr;
Status LocalKvStoreVirtualTest::status_Virtual = Status::ERROR;
DistributedKvDataManager LocalKvStoreVirtualTest::manager_Virtual;
AppId LocalKvStoreVirtualTest::appId_Virtual;
StoreId LocalKvStoreVirtualTest::storeId_Virtual;

void LocalKvStoreVirtualTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/dev_local_sub", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void LocalKvStoreVirtualTest::TearDownTestCase(void)
{
    manager_Virtual.CloseKvStore(appId_Virtual, kvStore_Virtual);
    kvStore_Virtual = nullptr;
    manager_Virtual.DeleteKvStore(appId_Virtual, storeId_Virtual, "/data/service/el1/public/database/dev_local_sub");
    (void)remove("/data/service/el1/public/database/dev_local_sub/kvdb");
    (void)remove("/data/service/el1/public/database/dev_local_sub");
}

void LocalKvStoreVirtualTest::SetUp(void)
{
    Options options;
    options.securityLevel = S1;
    options.baseDir = std::string("/data/service/el1/public/database/dev_local_sub");
    appId_Virtual.appId = "dev_local_sub"; // define app name.
    storeId_Virtual.storeId = "student";   // define kvstore(database) name
    manager_Virtual.DeleteKvStore(appId_Virtual, storeId_Virtual, options.baseDir);
    // [create and] open and initialize kvstore instance.
    status_Virtual = manager_Virtual.GetSingleKvStore(options, appId_Virtual, storeId_Virtual, kvStore_Virtual);
    EXPECT_EQ(Status::SUCCESS, status_Virtual) << "wrong statusVirtual";
    EXPECT_NE(nullptr, kvStore_Virtual) << "kvStore is nullptr";
}

void LocalKvStoreVirtualTest::TearDown(void)
{
    manager_Virtual.CloseKvStore(appId_Virtual, kvStore_Virtual);
    kvStore_Virtual = nullptr;
    manager_Virtual.DeleteKvStore(appId_Virtual, storeId_Virtual);
}

class DeviceObserverTest : public KvStoreObserver {
public:
    std::vector<Entry> insertEntries_;
    std::vector<Entry> updateEntries_;
    std::vector<Entry> deleteEntries_;
    std::string deviceId_;
    bool isClear_ = false;
    DeviceObserverTest();
    ~DeviceObserverTest() = default;

    void OnChange(const ChangeNotification &changeNotification);

    // reset the callCount_ to zero.
    void ResetToZero();

    uint32_t GetCallCount(uint32_t valueVirtual = 1);

private:
    std::mutex mutex_;
    uint32_t callCount_ = 0;
    BlockData<uint32_t> value_{ 1, 0 };
};

DeviceObserverTest::DeviceObserverTest()
{
}

void DeviceObserverTest::OnChange(const ChangeNotification &changeNotification)
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

void DeviceObserverTest::ResetToZero()
{
    std::lock_guard<decltype(mutex_)> guard(mutex_);
    callCount_ = 0;
    value_.Clear(0);
}

uint32_t DeviceObserverTest::GetCallCount(uint32_t valueVirtual)
{
    int retry = 0;
    uint32_t callTimes = 0;
    while (retry < valueVirtual) {
        callTimes = value_.GetValue();
        if (callTimes >= valueVirtual) {
            break;
        }
        std::lock_guard<decltype(mutex_)> guard(mutex_);
        callTimes = value_.GetValue();
        if (callTimes >= valueVirtual) {
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
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore001 begin.");
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 0);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    observerVirtual = nullptr;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore002
* @tc.desc: Subscribe fail, observerVirtual is null
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore002, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore002 begin.");
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<DeviceObserverTest> observerVirtual = nullptr;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::INVALID_ARGUMENT, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore003
* @tc.desc: Subscribe success and OnChange callback after put
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore003, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore003 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key keyVirtual = "Id1";
    Value valueVirtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(keyVirtual, valueVirtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    observerVirtual = nullptr;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore004
* @tc.desc: The same observerVirtual subscribe three times and OnChange callback after put
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore004 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key keyVirtual = "Id1";
    Value valueVirtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(keyVirtual, valueVirtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore005
* @tc.desc: The different observerVirtual subscribe three times and OnChange callback after put
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore005 begin.");
    auto observer1 = std::make_shared<DeviceObserverTest>();
    auto observer2 = std::make_shared<DeviceObserverTest>();
    auto observer3 = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observer1);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore failed, wrong statusVirtual";
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observer2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore failed, wrong statusVirtual";
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observer3);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore failed, wrong statusVirtual";

    Key keyVirtual = "Id1";
    Value valueVirtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(keyVirtual, valueVirtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "Putting data to KvStore failed, wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer3->GetCallCount()), 1);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observer1);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observer2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observer3);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore006
* @tc.desc: Unsubscribe an observerVirtual and subscribe again - the map should be cleared after unsubscription.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore006 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key key1Virtual = "Id1";
    Value value1Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key1Virtual, value1Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";

    Key key2Virtual = "Id2";
    Value value2Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key2Virtual, value2Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);

    kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    Key key3Virtual = "Id3";
    Value value3Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key3Virtual, value3Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(2)), 2);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore007
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is called multiple times after the put operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore007 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key key1Virtual = "Id1";
    Value value1Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key1Virtual, value1Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key key2Virtual = "Id2";
    Value value2Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key2Virtual, value2Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key key3Virtual = "Id3";
    Value value3Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key3Virtual, value3Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(3)), 3);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore008
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is
    called multiple times after the put&update operations.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore008 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key key1Virtual = "Id1";
    Value value1Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key1Virtual, value1Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key key2Virtual = "Id2";
    Value value2Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key2Virtual, value2Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key key3Virtual = "Id1";
    Value value3Virtual = "subscribe03";
    statusVirtual = kvStore_Virtual->Put(key3Virtual, value3Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(3)), 3);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore009
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is called multiple times after the putBatch operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore009 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    // before update.
    std::vector<Entry> entries1Virtual;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "Id1";
    entryVirtual1.valueVirtual = "subscribe";
    entryVirtual2.keyVirtual = "Id2";
    entryVirtual2.valueVirtual = "subscribe";
    entryVirtual3.keyVirtual = "Id3";
    entryVirtual3.valueVirtual = "subscribe";
    entries1Virtual.push_back(entryVirtual1);
    entries1Virtual.push_back(entryVirtual2);
    entries1Virtual.push_back(entryVirtual3);

    std::vector<Entry> entries2;
    Entry entryVirtual4, entryVirtual5;
    entryVirtual4.keyVirtual = "Id4";
    entryVirtual4.valueVirtual = "subscribe";
    entryVirtual5.keyVirtual = "Id5";
    entryVirtual5.valueVirtual = "subscribe";
    entries2.push_back(entryVirtual4);
    entries2.push_back(entryVirtual5);

    statusVirtual = kvStore_Virtual->PutBatch(entries1Virtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->PutBatch(entries2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(2)), 2);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore010
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is
    called multiple times after the putBatch update operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore010 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    // before update.
    std::vector<Entry> entries1Virtual;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "Id1";
    entryVirtual1.valueVirtual = "subscribe";
    entryVirtual2.keyVirtual = "Id2";
    entryVirtual2.valueVirtual = "subscribe";
    entryVirtual3.keyVirtual = "Id3";
    entryVirtual3.valueVirtual = "subscribe";
    entries1Virtual.push_back(entryVirtual1);
    entries1Virtual.push_back(entryVirtual2);
    entries1Virtual.push_back(entryVirtual3);

    std::vector<Entry> entries2;
    Entry entryVirtual4, entryVirtual5;
    entryVirtual4.keyVirtual = "Id1";
    entryVirtual4.valueVirtual = "modify";
    entryVirtual5.keyVirtual = "Id2";
    entryVirtual5.valueVirtual = "modify";
    entries2.push_back(entryVirtual4);
    entries2.push_back(entryVirtual5);

    statusVirtual = kvStore_Virtual->PutBatch(entries1Virtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->PutBatch(entries2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(2)), 2);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore011
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is called after successful deletion.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore011 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "Id1";
    entryVirtual1.valueVirtual = "subscribe";
    entryVirtual2.keyVirtual = "Id2";
    entryVirtual2.valueVirtual = "subscribe";
    entryVirtual3.keyVirtual = "Id3";
    entryVirtual3.valueVirtual = "subscribe";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    Status statusVirtual = kvStore_Virtual->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->Delete("Id1");
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore Delete data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore012
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is not called after deletion of non-existing keys.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore012 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "Id1";
    entryVirtual1.valueVirtual = "subscribe";
    entryVirtual2.keyVirtual = "Id2";
    entryVirtual2.valueVirtual = "subscribe";
    entryVirtual3.keyVirtual = "Id3";
    entryVirtual3.valueVirtual = "subscribe";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    Status statusVirtual = kvStore_Virtual->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->Delete("Id4");
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore Delete data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 0);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore013
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is called after KvStore is cleared.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore013 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "Id1";
    entryVirtual1.valueVirtual = "subscribe";
    entryVirtual2.keyVirtual = "Id2";
    entryVirtual2.valueVirtual = "subscribe";
    entryVirtual3.keyVirtual = "Id3";
    entryVirtual3.valueVirtual = "subscribe";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    Status statusVirtual = kvStore_Virtual->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 0);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore014
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is
    not called after non-existing data in KvStore is cleared.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore014 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 0);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore015
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is called after the deleteBatch operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore015 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "Id1";
    entryVirtual1.valueVirtual = "subscribe";
    entryVirtual2.keyVirtual = "Id2";
    entryVirtual2.valueVirtual = "subscribe";
    entryVirtual3.keyVirtual = "Id3";
    entryVirtual3.valueVirtual = "subscribe";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status statusVirtual = kvStore_Virtual->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    statusVirtual = kvStore_Virtual->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore DeleteBatch data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore016
* @tc.desc: Subscribe to an observerVirtual - OnChange callback is called after deleteBatch of non-existing keys.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore016 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    std::vector<Entry> entries;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.keyVirtual = "Id1";
    entryVirtual1.valueVirtual = "subscribe";
    entryVirtual2.keyVirtual = "Id2";
    entryVirtual2.valueVirtual = "subscribe";
    entryVirtual3.keyVirtual = "Id3";
    entryVirtual3.valueVirtual = "subscribe";
    entries.push_back(entryVirtual1);
    entries.push_back(entryVirtual2);
    entries.push_back(entryVirtual3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status statusVirtual = kvStore_Virtual->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    statusVirtual = kvStore_Virtual->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore DeleteBatch data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 0);

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore020
* @tc.desc: Unsubscribe an observerVirtual two times.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStore020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore020 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::STORE_NOT_SUBSCRIBE, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification001
* @tc.desc: Subscribe to an observerVirtual successfully - callback is
    called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStoreNotification001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification001 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key keyVirtual = "Id1";
    Value valueVirtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(keyVirtual, valueVirtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    ZLOGD("kvstore_ddm_subscribekvstore_003");
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());
    ZLOGD("kvstore_ddm_subscribekvstore_003 size:%zu.", observerVirtual->insertEntries_.size());

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification002
* @tc.desc: Subscribe to the same observerVirtual three times - callback is
    called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStoreNotification002, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification002 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key keyVirtual = "Id1";
    Value valueVirtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(keyVirtual, valueVirtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification003
* @tc.desc: The different observerVirtual subscribe three times and callback with notification after put
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStoreNotification003, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification003 begin.");
    auto observer1 = std::make_shared<DeviceObserverTest>();
    auto observer2 = std::make_shared<DeviceObserverTest>();
    auto observer3 = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observer1);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observer2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observer3);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key keyVirtual = "Id1";
    Value valueVirtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(keyVirtual, valueVirtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer1->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer1->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observer1->insertEntries_[0].valueVirtual.ToString());

    EXPECT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer2->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer2->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observer2->insertEntries_[0].valueVirtual.ToString());

    EXPECT_EQ(static_cast<int>(observer3->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer3->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer3->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observer3->insertEntries_[0].valueVirtual.ToString());

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observer1);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observer2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observer3);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification004
* @tc.desc: Verify notification after an observerVirtual is unsubscribed and then subscribed again.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStoreNotification004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification004 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key key1Virtual = "Id1";
    Value value1Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key1Virtual, value1Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";

    Key key2Virtual = "Id2";
    Value value2Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key2Virtual, value2Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());

    kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    Key key3Virtual = "Id3";
    Value value3Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key3Virtual, value3Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(2)), 2);
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id3", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification005
* @tc.desc: Subscribe to an observerVirtual, callback with notification many times after put the different data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStoreNotification005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification005 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key key1Virtual = "Id1";
    Value value1Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key1Virtual, value1Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());

    Key key2Virtual = "Id2";
    Value value2Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key2Virtual, value2Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(2)), 2);
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id2", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());

    Key key3Virtual = "Id3";
    Value value3Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key3Virtual, value3Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(3)), 3);
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id3", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification006
* @tc.desc: Subscribe to an observerVirtual, callback with notification many times after put the same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStoreNotification006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification006 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key key1Virtual = "Id1";
    Value value1Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key1Virtual, value1Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observerVirtual->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->insertEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->insertEntries_[0].valueVirtual.ToString());

    Key key2Virtual = "Id1";
    Value value2Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key2Virtual, value2Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(2)), 2);
    EXPECT_EQ(static_cast<int>(observerVirtual->updateEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->updateEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->updateEntries_[0].valueVirtual.ToString());

    Key key3Virtual = "Id1";
    Value value3Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key3Virtual, value3Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount(3)), 3);
    EXPECT_EQ(static_cast<int>(observerVirtual->updateEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->updateEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe", observerVirtual->updateEntries_[0].valueVirtual.ToString());

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification007
* @tc.desc: Subscribe to an observerVirtual, callback with notification many times after put&update
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: sql
*/
HWTEST_F(LocalKvStoreVirtualTest, KvStoreDdmSubscribeKvStoreNotification007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification007 begin.");
    auto observerVirtual = std::make_shared<DeviceObserverTest>();
    Key key1Virtual = "Id1";
    Value value1Virtual = "subscribe";
    Status statusVirtual = kvStore_Virtual->Put(key1Virtual, value1Virtual);
    // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key key2Virtual = "Id2";
    Value value2Virtual = "subscribe";
    statusVirtual = kvStore_Virtual->Put(key2Virtual, value2Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStore_Virtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key key3Virtual = "Id1";
    Value value3Virtual = "subscribe03";
    statusVirtual = kvStore_Virtual->Put(key3Virtual, value3Virtual); // insert or update keyVirtual-valueVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observerVirtual->updateEntries_.size()), 1);
    EXPECT_EQ("Id1", observerVirtual->updateEntries_[0].keyVirtual.ToString());
    EXPECT_EQ("subscribe03", observerVirtual->updateEntries_[0].valueVirtual.ToString());

    statusVirtual = kvStore_Virtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}