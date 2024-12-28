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

#define LOG_TAG "SubscribeUnitTest"
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
class SubscribeUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager dataManager;
    static std::shared_ptr<SingleKvStore> store;
    static Status statusGetKvStore;
    static AppId appId;
    static StoreId storeId;
};
std::shared_ptr<SingleKvStore> SubscribeInterceptionUnitTest::store = nullptr;
Status SubscribeInterceptionUnitTest::statusGetKvStore = Status::ERROR;
DistributedKvDataManager SubscribeInterceptionUnitTest::dataManager;
AppId SubscribeInterceptionUnitTest::appId;
StoreId SubscribeInterceptionUnitTest::storeId;

void SubscribeInterceptionUnitTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/kvstore", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void SubscribeInterceptionUnitTest::TearDownTestCase(void)
{
    dataManager.CloseKvStore(appId, store);
    store = nullptr;
    dataManager.DeleteKvStore(appId, storeId, "/data/service/el1/public/database/kvstore");
    (void)remove("/data/service/el1/public/database/kvstore/kvdb");
    (void)remove("/data/service/el1/public/database/kvstore");
}

void SubscribeInterceptionUnitTest::SetUp(void)
{
    Options option;
    option.createIfMissing = true;
    option.encrypt = false;  // not supported yet.
    option.securityLevel = S1;
    option.autoSync = true;  // not supported yet.
    option.storeType = KvStoreType::SINGLE_VERSION;
    option.area = EL1;
    option.baseDir = std::string("/data/service/el1/public/database/kvstore");
    appId.appId = "kvstore";         // define app name.
    storeId.storeId = "Individuals";  // define kvstore(database) name
    dataManager.DeleteKvStore(appId, storeId, option.baseDir);
    // [create and] open and initialize kvstore instance.
    statusKvStore = dataManager.GetSingleKvStore(option, appId, storeId, store);
    EXPECT_EQ(Status::SUCCESS, statusKvStore) << "GetSingleKvStore return wrong.";
    EXPECT_NE(nullptr, store) << "store is null";
}

void SubscribeInterceptionUnitTest::TearDown(void)
{
    dataManager.CloseKvStore(appId, store);
    store = nullptr;
    dataManager.DeleteKvStore(appId, storeId);
}

class KvObserverUnitTest : public KvStoreObserver {
public:
    std::vector<Entry> insertVec_;
    std::vector<Entry> updateVec_;
    std::vector<Entry> deleteVec_;
    bool isCleared_ = false;
    KvObserverUnitTest();
    ~KvObserverUnitTest() {}
    KvObserverUnitTest(const KvObserverUnitTest &) = delete;
    KvObserverUnitTest &operator=(const KvObserverUnitTest &) = delete;
    KvObserverUnitTest(KvObserverUnitTest &&) = delete;
    KvObserverUnitTest &operator=(KvObserverUnitTest &&) = delete;
    void OnChange(const ChangeNotification &change);
    // reset the callNum_ to zero.
    void ResetToZero();
    uint32_t GetCallNum(uint32_t value = 1);
private:
    std::mutex dataMutex_;
    uint32_t callNum_ = 0;
    BlockData<uint32_t> value_{ 1, 0 };
};

KvObserverUnitTest::KvObserverUnitTest()
{
}

void KvObserverUnitTest::OnChange(const ChangeNotification &change)
{
    ZLOGD("begin.");
    insertVec_ = change.GetInsertEntries();
    updateVec_ = change.GetUpdateEntries();
    deleteVec_ = change.GetDeleteEntries();
    change.GetDeviceId();
    isCleared_ = change.IsClear();
    std::lock_guard<decltype(dataMutex_)> guard(dataMutex_);
    ++callNum_;
    value_.SetValue(callNum_);
}

void KvObserverUnitTest::ResetToZero()
{
    std::lock_guard<decltype(dataMutex_)> guard(dataMutex_);
    callNum_ = 0;
    value_.Clear(0);
}

uint32_t KvObserverUnitTest::GetCallNum(uint32_t value)
{
    int retry = 0;
    uint32_t callCount = 0;
    while (retry < value) {
        callCount = value_.GetValue();
        if (callCount >= value) {
            break;
        }
        std::lock_guard<decltype(dataMutex_)> guard(dataMutex_);
        callCount = value_.GetValue();
        if (callCount >= value) {
            break;
        }
        value_.Clear(callCount);
        retry++;
    }
    return callCount;
}

/**
* @tc.name: KvStoreSubscribeInterception001
* @tc.desc: Subscribe success
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception001, TestSize.Level1)
{
    ZLOGI("KvStoreSubscribeInterception001 begin.");
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    kvObserver->ResetToZero();

    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
    kvObserver = nullptr;
}

/**
* @tc.name: KvStoreSubscribeInterception002
* @tc.desc: Subscribe fail, kvObserver is null
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception002, TestSize.Level1)
{
    ZLOGI("KvStoreSubscribeInterception002 begin.");
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<KvObserverUnitTest> kvObserver = nullptr;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status) << "SubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception003
* @tc.desc: Subscribe success and OnChange callback after put
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception003, TestSize.Level1)
{
    ZLOGI("KvStoreSubscribeInterception003 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey = "testKey1";
    Value value = "subscribeValue";
    status = store->Put(testKey, value);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
    kvObserver = nullptr;
}

/**
* @tc.name: KvStoreSubscribeInterception004
* @tc.desc: The same kvObserver subscribeValue three times and OnChange callback after put
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception004, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception004 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, status) << "SubscribeKvStore return wrong.";
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, status) << "SubscribeKvStore return wrong.";

    Key testKey = "testKey1";
    Value value = "subscribeValue";
    status = store->Put(testKey, value);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception005
* @tc.desc: The different kvObserver subscribeValue three times and OnChange callback after put
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception005, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception005 begin.");
    auto kvObserver1 = std::make_shared<KvObserverUnitTest>();
    auto kvObserver2 = std::make_shared<KvObserverUnitTest>();
    auto kvObserver3 = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver1);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore failed, wrong.";
    status = store->SubscribeKvStore(subType, kvObserver2);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore failed, wrong.";
    status = store->SubscribeKvStore(subType, kvObserver3);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore failed, wrong.";

    Key testKey = "testKey1";
    Value value = "subscribeValue";
    status = store->Put(testKey, value);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "Putting data to KvStore failed, wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver1->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver2->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver3->GetCallNum()), 1);

    status = store->UnSubscribeKvStore(subType, kvObserver1);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
    status = store->UnSubscribeKvStore(subType, kvObserver2);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
    status = store->UnSubscribeKvStore(subType, kvObserver3);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception006
* @tc.desc: UnsubscribeValue an kvObserver and subscribeValue again - the map should be cleared after unsubscription.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception006, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception006 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";
    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";

    Key testKey2 = "testKey2";
    Value value2 = "subscribeValue";
    status = store->Put(testKey2, value2);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);

    store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    Key testKey3 = "testKey3";
    Value value3 = "subscribeValue";
    status = store->Put(testKey3, value3);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception007
* @tc.desc: Subscribe to an kvObserver - OnChange callback is called multiple times after the put operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception007, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception007 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";
    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";

    Key testKey2 = "testKey2";
    Value value2 = "subscribeValue";
    status = store->Put(testKey2, value2);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";

    Key testKey3 = "testKey3";
    Value value3 = "subscribeValue";
    status = store->Put(testKey3, value3);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(3)), 3);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception008
* @tc.desc: Subscribe to an kvObserver - OnChange callback is called multiple times after the put&update operations.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception008, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception008 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";
    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";

    Key testKey2 = "testKey2";
    Value value2 = "subscribeValue";
    status = store->Put(testKey2, value2);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";

    Key testKey3 = "testKey1";
    Value value3 = "subscribeValue03";
    status = store->Put(testKey3, value3);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(3)), 3);
    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception009
* @tc.desc: Subscribe to an kvObserver - OnChange callback is called multiple times after the putBatch operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception009, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception009 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    // before update.
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Entry> entryVec1;
    Entry entry_4, entry_5;
    entry_4.testKey = "Id4";
    entry_4.value = "subscribeValue";
    entry_5.testKey = "Id5";
    entry_5.value = "subscribeValue";
    entryVec1.push_back(entry_4);
    entryVec1.push_back(entry_5);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    status = store->PutBatch(entryVec1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception010
* @tc.desc: Subscribe to an kvObserver - OnChange callback is called multiple times after the putBatch update operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception010, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception010 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    // before update.
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Entry> entryVec1;
    Entry entry_4, entry_5;
    entry_4.testKey = "testKey1";
    entry_4.value = "modify";
    entry_5.testKey = "testKey2";
    entry_5.value = "modify";
    entryVec1.push_back(entry_4);
    entryVec1.push_back(entry_5);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    status = store->PutBatch(entryVec1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception011
* @tc.desc: Subscribe to an kvObserver - OnChange callback is called after successful deletion.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception011, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception011 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->Delete("testKey1");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception012
* @tc.desc: Subscribe to an kvObserver - OnChange callback is not called after deletion of non-existing testKeyVec.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception012, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception012 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->Delete("Id4");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception013
* @tc.desc: Subscribe to an kvObserver - OnChange callback is called after KvStore is cleared.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception013, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception013 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(1)), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception014
* @tc.desc: Subscribe to an kvObserver - OnChange callback is not called after non-existing data in KvStore is cleared.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception014, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception014 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception015
* @tc.desc: Subscribe to an kvObserver - OnChange callback is called after the deleteBatch operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception015, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception015 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Key> testKeyVec;
    testKeyVec.push_back("testKey1");
    testKeyVec.push_back("testKey2");

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception016
* @tc.desc: Subscribe to an kvObserver - OnChange callback is called after deleteBatch of non-existing testKeyVec.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception016, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception016 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Key> testKeyVec;
    testKeyVec.push_back("Id4");
    testKeyVec.push_back("Id5");

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterception020
* @tc.desc: UnsubscribeValue an kvObserver two times.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterception020, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterception020 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::STORE_NOT_SUBSCRIBE, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification001
* @tc.desc: Subscribe to an kvObserver successfully - callback is called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification001, TestSize.Level1)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification001 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey = "testKey1";
    Value value = "subscribeValue";
    status = store->Put(testKey, value);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    ZLOGD("kvstore_ddm_subscribeValuekvstore_003");
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    ZLOGD("kvstore_ddm_subscribeValuekvstore_003 size:%zu.", kvObserver->insertVec_.size());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification002
* @tc.desc: Subscribe to the same kvObserver three times - callback is called with
            a notification after the put operation.
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification002, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification002 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, status) << "SubscribeKvStore return wrong.";
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, status) << "SubscribeKvStore return wrong.";

    Key testKey = "testKey1";
    Value value = "subscribeValue";
    status = store->Put(testKey, value);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification003
* @tc.desc: The different kvObserver subscribeValue three times and callback with notification after put
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification003, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification003 begin.");
    auto kvObserver1 = std::make_shared<KvObserverUnitTest>();
    auto kvObserver2 = std::make_shared<KvObserverUnitTest>();
    auto kvObserver3 = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver1);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->SubscribeKvStore(subType, kvObserver2);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->SubscribeKvStore(subType, kvObserver3);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey = "testKey1";
    Value value = "subscribeValue";
    status = store->Put(testKey, value);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver1->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver1->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver1->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver1->insertVec_[0].value.ToString());

    EXPECT_EQ(static_cast<int>(kvObserver2->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver2->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver2->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver2->insertVec_[0].value.ToString());

    EXPECT_EQ(static_cast<int>(kvObserver3->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver3->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver3->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver3->insertVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver1);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
    status = store->UnSubscribeKvStore(subType, kvObserver2);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
    status = store->UnSubscribeKvStore(subType, kvObserver3);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification004
* @tc.desc: Verify notification after an kvObserver is unsubscribeValued and then subscribeValued again.
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification004, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification004 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";
    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";

    Key testKey2 = "testKey2";
    Value value2 = "subscribeValue";
    status = store->Put(testKey2, value2);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    Key testKey3 = "testKey3";
    Value value3 = "subscribeValue";
    status = store->Put(testKey3, value3);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey3", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification005
* @tc.desc: Subscribe to an kvObserver, callback with notification many times after put the different data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification005, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification005 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";
    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    Key testKey2 = "testKey2";
    Value value2 = "subscribeValue";
    status = store->Put(testKey2, value2);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey2", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    Key testKey3 = "testKey3";
    Value value3 = "subscribeValue";
    status = store->Put(testKey3, value3);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(3)), 3);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey3", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification006
* @tc.desc: Subscribe to an kvObserver, callback with notification many times after put the same data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification006, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification006 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";
    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    Key testKey2 = "testKey1";
    Value value2 = "subscribeValue";
    status = store->Put(testKey2, value2);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->updateVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->updateVec_[0].value.ToString());

    Key testKey3 = "testKey1";
    Value value3 = "subscribeValue";
    status = store->Put(testKey3, value3);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(3)), 3);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->updateVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->updateVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification007
* @tc.desc: Subscribe to an kvObserver, callback with notification many times after put&update
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification007, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification007 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";
    Status status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";

    Key testKey2 = "testKey2";
    Value value2 = "subscribeValue";
    status = store->Put(testKey2, value2);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey3 = "testKey1";
    Value value3 = "subscribeValue03";
    status = store->Put(testKey3, value3);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";

    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->updateVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue03", kvObserver->updateVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification008
* @tc.desc: Subscribe to an kvObserver, callback with notification one times after putbatch&update
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification008, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification008 begin.");
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;

    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    entryVec.clear();
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue_modify";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue_modify";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 2);
    EXPECT_EQ("testKey1", kvObserver->updateVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue_modify", kvObserver->updateVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->updateVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue_modify", kvObserver->updateVec_[1].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification009
* @tc.desc: Subscribe to an kvObserver, callback with notification one times after putbatch all different data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification009, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification009 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;

    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 3);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->insertVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[1].value.ToString());
    EXPECT_EQ("testKey3", kvObserver->insertVec_[2].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[2].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification010
* @tc.desc: Subscribe to an kvObserver, callback with notification one times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification010, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification010 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;

    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey1";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey2";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 2);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->insertVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[1].value.ToString());
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification011
* @tc.desc: Subscribe to an kvObserver, callback with notification one times after putbatch all same data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification011, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification011 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;

    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey1";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey1";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification012
* @tc.desc: Subscribe to an kvObserver, callback with notification many times after putbatch all different data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification012, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification012 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;

    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Entry> entryVec1;
    Entry entry_4, entry_5;
    entry_4.testKey = "Id4";
    entry_4.value = "subscribeValue";
    entry_5.testKey = "Id5";
    entry_5.value = "subscribeValue";
    entryVec1.push_back(entry_4);
    entryVec1.push_back(entry_5);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 3);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->insertVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[1].value.ToString());
    EXPECT_EQ("testKey3", kvObserver->insertVec_[2].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[2].value.ToString());

    status = store->PutBatch(entryVec1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 2);
    EXPECT_EQ("Id4", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ("Id5", kvObserver->insertVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[1].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification013
* @tc.desc: Subscribe to an kvObserver,
            callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification013, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification013 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;

    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Entry> entryVec1;
    Entry entry_4, entry_5;
    entry_4.testKey = "testKey1";
    entry_4.value = "subscribeValue";
    entry_5.testKey = "Id4";
    entry_5.value = "subscribeValue";
    entryVec1.push_back(entry_4);
    entryVec1.push_back(entry_5);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 3);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->insertVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[1].value.ToString());
    EXPECT_EQ("testKey3", kvObserver->insertVec_[2].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[2].value.ToString());

    status = store->PutBatch(entryVec1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->updateVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->updateVec_[0].value.ToString());
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("Id4", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification014
* @tc.desc: Subscribe to an kvObserver, callback with notification many times after putbatch all same data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification014, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification014 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;

    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Entry> entryVec1;
    Entry entry_4, entry_5;
    entry_4.testKey = "testKey1";
    entry_4.value = "subscribeValue";
    entry_5.testKey = "testKey2";
    entry_5.value = "subscribeValue";
    entryVec1.push_back(entry_4);
    entryVec1.push_back(entry_5);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 3);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->insertVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[1].value.ToString());
    EXPECT_EQ("testKey3", kvObserver->insertVec_[2].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[2].value.ToString());

    status = store->PutBatch(entryVec1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 2);
    EXPECT_EQ("testKey1", kvObserver->updateVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->updateVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->updateVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->updateVec_[1].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification015
* @tc.desc: Subscribe to an kvObserver, callback with notification many times after putbatch complex data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification015, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification015 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;

    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey1";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Entry> entryVec1;
    Entry entry_4, entry_5;
    entry_4.testKey = "testKey1";
    entry_4.value = "subscribeValue";
    entry_5.testKey = "testKey2";
    entry_5.value = "subscribeValue";
    entryVec1.push_back(entry_4);
    entryVec1.push_back(entry_5);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 2);
    EXPECT_EQ("testKey1", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ("testKey3", kvObserver->insertVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[1].value.ToString());

    status = store->PutBatch(entryVec1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 2);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->updateVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->updateVec_[0].value.ToString());
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 1);
    EXPECT_EQ("testKey2", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->insertVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification016
* @tc.desc: Pressure test subscribeValue, callback with notification many times after putbatch
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification016, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification016 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    const int ENTRIES_MAX_LEN = 100;
    std::vector<Entry> entryVec;
    for (int i = 0; i < ENTRIES_MAX_LEN; i++) {
        Entry entry;
        entry.testKey = std::to_string(i);
        entry.value = "subscribeValue";
        entryVec.push_back(entry);
    }

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 100);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification017
* @tc.desc: Subscribe to an kvObserver, callback with notification after delete success
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification017, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification017 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->Delete("testKey1");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->deleteVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->deleteVec_[0].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification018
* @tc.desc: Subscribe to an kvObserver, not callback after delete which testKey not exist
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification018, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification018 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->Delete("Id4");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification019
* @tc.desc: Subscribe to an kvObserver, delete the same data many times and only first delete callback with notification
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification019, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification019 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";
    status = store->Delete("testKey1");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 1);
    EXPECT_EQ("testKey1", kvObserver->deleteVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->deleteVec_[0].value.ToString());

    status = store->Delete("testKey1");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 1); // not callback so not clear

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification020
* @tc.desc: Subscribe to an kvObserver, callback with notification after deleteBatch
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification020, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification020 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Key> testKeyVec;
    testKeyVec.push_back("testKey1");
    testKeyVec.push_back("testKey2");

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 2);
    EXPECT_EQ("testKey1", kvObserver->deleteVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->deleteVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->deleteVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->deleteVec_[1].value.ToString());

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification021
* @tc.desc: Subscribe to an kvObserver, not callback after deleteBatch which all testKeyVec not exist
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification021, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification021 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Key> testKeyVec;
    testKeyVec.push_back("Id4");
    testKeyVec.push_back("Id5");

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification022
* @tc.desc: Subscribe to an kvObserver, deletebatch the same data many times and only first deletebatch callback with
* notification
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification022, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification022 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey1";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey2";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "testKey3";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Key> testKeyVec;
    testKeyVec.push_back("testKey1");
    testKeyVec.push_back("testKey2");

    Status status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 2);
    EXPECT_EQ("testKey1", kvObserver->deleteVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->deleteVec_[0].value.ToString());
    EXPECT_EQ("testKey2", kvObserver->deleteVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->deleteVec_[1].value.ToString());

    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(2)), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 2); // not callback so not clear

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification023
* @tc.desc: Subscribe to an kvObserver, include Clear Put PutBatch Delete DeleteBatch
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification023, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification023 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey2";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey3";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "Id4";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Key> testKeyVec;
    testKeyVec.push_back("testKey2");
    testKeyVec.push_back("testKey3");

    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    status = store->Delete(testKey1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore delete data return wrong.";
    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum(4)), 4);
    // every callback will clear vector
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 2);
    EXPECT_EQ("testKey2", kvObserver->deleteVec_[0].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->deleteVec_[0].value.ToString());
    EXPECT_EQ("testKey3", kvObserver->deleteVec_[1].testKey.ToString());
    EXPECT_EQ("subscribeValue", kvObserver->deleteVec_[1].value.ToString());
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification024
* @tc.desc: Subscribe to an kvObserver[use transaction], include Clear Put PutBatch Delete DeleteBatch
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification024, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification024 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey2";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey3";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "Id4";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Key> testKeyVec;
    testKeyVec.push_back("testKey2");
    testKeyVec.push_back("testKey3");

    status = store->StartTransaction();
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore startTransaction return wrong.";
    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    status = store->Delete(testKey1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore delete data return wrong.";
    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    status = store->Commit();
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Commit return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification025
* @tc.desc: Subscribe to an kvObserver[use transaction], include Clear Put PutBatch Delete DeleteBatch
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author:
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification025, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification025 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    Key testKey1 = "testKey1";
    Value value1 = "subscribeValue";

    std::vector<Entry> entryVec;
    Entry entry_1, entry_2, entry_3;
    entry_1.testKey = "testKey2";
    entry_1.value = "subscribeValue";
    entry_2.testKey = "testKey3";
    entry_2.value = "subscribeValue";
    entry_3.testKey = "Id4";
    entry_3.value = "subscribeValue";
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);

    std::vector<Key> testKeyVec;
    testKeyVec.push_back("testKey2");
    testKeyVec.push_back("testKey3");

    status = store->StartTransaction();
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore startTransaction return wrong.";
    status = store->Put(testKey1, value1);  // insert or update testKey-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong.";
    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";
    status = store->Delete(testKey1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore delete data return wrong.";
    status = store->DeleteBatch(testKeyVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong.";
    status = store->Rollback();
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Commit return wrong.";
    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->updateVec_.size()), 0);
    EXPECT_EQ(static_cast<int>(kvObserver->deleteVec_.size()), 0);

    status = store->UnSubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong.";
    kvObserver = nullptr;
}

/**
* @tc.name: KvStoreSubscribeInterceptionNotification026
* @tc.desc: Subscribe to an kvObserver[use transaction], include bigData PutBatch  update  insert delete
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: dukaizhan
*/
HWTEST_F(SubscribeInterceptionUnitTest, KvStoreSubscribeInterceptionNotification026, TestSize.Level2)
{
    ZLOGI("KvStoreSubscribeInterceptionNotification026 begin.");
    auto kvObserver = std::make_shared<KvObserverUnitTest>();
    SubscribeType subType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = store->SubscribeKvStore(subType, kvObserver);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong.";

    std::vector<Entry> entryVec;
    Entry entry_0, entry_1, entry_2, entry_3, entry_4, entry_5, entry6, entry7;

    int maxSize = 2 * 1024 * 1024; // max value size is 2M.
    std::vector<uint8_t> val(maxSize);
    for (int i = 0; i < maxSize; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value value = val;

    int maxSize2 = 1000 * 1024; // max value size is 1000k.
    std::vector<uint8_t> val2(maxSize2);
    for (int i = 0; i < maxSize2; i++) {
        val2[i] = static_cast<uint8_t>(i);
    }
    Value value2 = val2;

    entry_0.testKey = "SingleKvStoreDdmPutBatch006_0";
    entry_0.value = "beijing";
    entry_1.testKey = "SingleKvStoreDdmPutBatch006_1";
    entry_1.value = value;
    entry_2.testKey = "SingleKvStoreDdmPutBatch006_2";
    entry_2.value = value;
    entry_3.testKey = "SingleKvStoreDdmPutBatch006_3";
    entry_3.value = "ZuiHouBuZhiTianZaiShui";
    entry_4.testKey = "SingleKvStoreDdmPutBatch006_4";
    entry_4.value = value;

    entryVec.push_back(entry_0);
    entryVec.push_back(entry_1);
    entryVec.push_back(entry_2);
    entryVec.push_back(entry_3);
    entryVec.push_back(entry_4);

    status = store->PutBatch(entryVec);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong.";

    EXPECT_EQ(static_cast<int>(kvObserver->GetCallNum()), 1);
    EXPECT_EQ(static_cast<int>(kvObserver->insertVec_.size()), 5);
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_0", kvObserver->insertVec_[0].testKey.ToString());
    EXPECT_EQ("beijing", kvObserver->insertVec_[0].value.ToString());
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_1", kvObserver->insertVec_[1].testKey.ToString());
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_2", kvObserver->insertVec_[2].testKey.ToString());
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_3", kvObserver->insertVec_[3].testKey.ToString());
    EXPECT_EQ("ZuiHouBuZhiTianZaiShui", kvObserver->insertVec_[3].value.ToString());
}
