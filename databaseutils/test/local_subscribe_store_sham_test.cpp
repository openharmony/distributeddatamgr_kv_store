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

#define LOG_TAG "LocalSubscribeStoreShamTest"

#include "block_data.h"
#include "distributed_kv_data_manager.h"
#include "log_print.h"
#include "types.h"
#include <cstdint>
#include <cstdio>
#include <gtest/gtest.h>
#include <mutex>
#include <vector>

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
class LocalSubscribeStoreShamTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager managerSham;
    static std::shared_ptr<SingleKvStore> kvStoreSham;
    static Status statusGetKvStoreSham;
    static AppId appIdSham;
    static StoreId storeIdSham;
};
std::shared_ptr<SingleKvStore> LocalSubscribeStoreShamTest::kvStoreSham = nullptr;
Status LocalSubscribeStoreShamTest::statusGetKvStoreSham = Status::ERROR;
DistributedKvDataManager LocalSubscribeStoreShamTest::managerSham;
AppId LocalSubscribeStoreShamTest::appIdSham;
StoreId LocalSubscribeStoreShamTest::storeIdSham;

void LocalSubscribeStoreShamTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/odmf", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void LocalSubscribeStoreShamTest::TearDownTestCase(void)
{
    managerSham.CloseKvStore(appIdSham, kvStoreSham);
    kvStoreSham = nullptr;
    managerSham.DeleteKvStore(appIdSham, storeIdSham, "/data/service/el1/public/database/odmf");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void LocalSubscribeStoreShamTest::SetUp(void)
{
    Options optionsSham;
    optionsSham.createIfMissing = true;
    optionsSham.encrypt = false; // not supported yet.
    optionsSham.securityLevel = S1;
    optionsSham.autoSync = true; // not supported yet.
    optionsSham.kvStoreType = KvStoreType::SINGLE_VERSION;
    optionsSham.area = EL1;
    optionsSham.baseDir = std::string("/data/service/el1/public/database/odmf");
    appIdSham.appIdSham = "odmf";        // define app name.
    storeIdSham.storeIdSham = "student"; // define kvstore(database) name
    managerSham.DeleteKvStore(appIdSham, storeIdSham, optionsSham.baseDir);
    // [create and] open and initialize kvstore instance.
    statusGetKvStoreSham = managerSham.GetSingleKvStore(optionsSham, appIdSham, storeIdSham, kvStoreSham);
    ASSERT_EQ(Status::SUCCESS, statusGetKvStoreSham) << "statusGetKvStoreSham return wrong statusSham";
    ASSERT_NE(nullptr, kvStoreSham) << "kvStoreSham is nullptr";
}

void LocalSubscribeStoreShamTest::TearDown(void)
{
    managerSham.CloseKvStore(appIdSham, kvStoreSham);
    kvStoreSham = nullptr;
    managerSham.DeleteKvStore(appIdSham, storeIdSham);
}

class KvStoreObserverUnitTestSham : public KvStoreObserver {
public:
    std::vector<Entry> insertEntries_Sham;
    std::vector<Entry> updateEntries_Sham;
    std::vector<Entry> deleteEntries_Sham;
    bool isClearSham_ = false;
    KvStoreObserverUnitTestSham();
    ~KvStoreObserverUnitTestSham() { }

    KvStoreObserverUnitTestSham(const KvStoreObserverUnitTestSham &) = delete;
    KvStoreObserverUnitTestSham &operator=(const KvStoreObserverUnitTestSham &) = delete;
    KvStoreObserverUnitTestSham(KvStoreObserverUnitTestSham &&) = delete;
    KvStoreObserverUnitTestSham &operator=(KvStoreObserverUnitTestSham &&) = delete;

    void OnChangeSham(const ChangeNotification &changeNotification);

    // reset the callCountSham_to zero.
    void ResetToZero();

    uint32_t GetCallCountSham(uint32_t valueShamSham = 1);

private:
    std::mutex mutexSham_;
    uint32_t callCountSham_ = 0;
    BlockData<uint32_t> valueSham_Sham { 1, 0 };
};

KvStoreObserverUnitTestSham::KvStoreObserverUnitTestSham() { }

void KvStoreObserverUnitTestSham::OnChangeSham(const ChangeNotification &changeNotification)
{
    ZLOGD("begin.");
    insertEntries_Sham = changeNotification.GetInsertEntries();
    updateEntries_Sham = changeNotification.GetUpdateEntries();
    deleteEntries_Sham = changeNotification.GetDeleteEntries();
    changeNotification.GetDeviceId();
    isClearSham_ = changeNotification.IsClear();
    std::lock_guard<decltype(mutexSham_)> guard(mutexSham_);
    ++callCount_Sham;
    valueSham_Sham.SetValue(callCount_Sham);
}

void KvStoreObserverUnitTestSham::ResetToZero()
{
    std::lock_guard<decltype(mutexSham_)> guard(mutexSham_);
    callCountSham_ = 0;
    valueSham_Sham.Clear(0);
}

uint32_t KvStoreObserverUnitTestSham::GetCallCountSham(uint32_t valueShamSham)
{
    int retrySham = 0;
    uint32_t callTimesSham = 0;
    while (retrySham < valueShamSham) {
        callTimesSham = valueSham_Sham.GetValue();
        if (callTimesSham >= valueShamSham) {
            break;
        }
        std::lock_guard<decltype(mutexSham_)> guard(mutexSham_);
        callTimesSham = valueSham_Sham.GetValue();
        if (callTimesSham >= valueShamSham) {
            break;
        }
        valueSham_Sham.Clear(callTimesSham);
        retrySham++;
    }
    return callTimesSham;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore001
 * @tc.desc: Subscribe success
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore001 begin.");
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    observerShamSham->ResetToZero();

    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    observerShamSham = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore002
 * @tc.desc: Subscribe fail, observerShamSham is null
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore002, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore002 begin.");
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<KvStoreObserverUnitTestSham> observerShamSham = nullptr;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::INVALID_ARGUMENT, statusSham) << "SubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore003
 * @tc.desc: Subscribe success and OnChangeSham callback after put
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore003, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore003 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamShamSham = "Id1";
    Value valueShamSham = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamShamSham, valueShamSham);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    observerShamSham = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore004
 * @tc.desc: The same observerShamSham subscribe three times and OnChangeSham callback after put
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore004, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore004 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamShamSham = "Id1";
    Value valueShamSham = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamShamSham, valueShamSham);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore005
 * @tc.desc: The different observerShamSham subscribe three times and OnChangeSham callback after put
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore005, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore005 begin.");
    auto observerSham1 = std::make_shared<KvStoreObserverUnitTestSham>();
    auto observerSham2 = std::make_shared<KvStoreObserverUnitTestSham>();
    auto observerSham3 = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore failed, wrong statusSham";
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore failed, wrong statusSham";
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerSham3);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore failed, wrong statusSham";

    Key valueShamShamShamSham = "Id1";
    Value valueShamSham = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamShamSham, valueShamSham);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "Putting data to KvStore failed, wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham1->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerSham2->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerSham3->GetCallCountSham()), 1);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerSham3);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore006
* @tc.desc: Unsubscribe an observerShamSham and
   subscribe again - the map should be cleared after unsubscription.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore006, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore006 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";

    Key valueShamShamSham2 = "Id2";
    Value valueSham2 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham2, valueSham2);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);

    kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    Key valueShamShamSham3 = "Id3";
    Value valueSham3 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham3, valueSham3);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore007
* @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is
   called multiple times after the put operation.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore007, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore007 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key valueShamShamSham2 = "Id2";
    Value valueSham2 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham2, valueSham2);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key valueShamShamSham3 = "Id3";
    Value valueSham3 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham3, valueSham3);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(3)), 3);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore008
* @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is
   called multiple times after the put&update operations.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore008, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore008 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key valueShamShamSham2 = "Id2";
    Value valueSham2 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham2, valueSham2);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key valueShamShamSham3 = "Id1";
    Value valueSham3 = "subscribe03";
    statusSham = kvStoreSham->Put(valueShamShamSham3, valueSham3);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(3)), 3);
    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore009
* @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is
   called multiple times after the putBatch operation.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore009, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore009 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    // before update.
    std::vector<Entry> entriesSham1;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham1.push_back(entrySham1);
    entriesSham1.push_back(entrySham2);
    entriesSham1.push_back(entrySham3);

    std::vector<Entry> entriesSham2;
    Entry entrySham4, entrySham5;
    entrySham4.valueShamShamShamSham = "Id4";
    entrySham4.valueShamSham = "subscribe";
    entrySham5.valueShamShamShamSham = "Id5";
    entrySham5.valueShamSham = "subscribe";
    entriesSham2.push_back(entrySham4);
    entriesSham2.push_back(entrySham5);

    statusSham = kvStoreSham->PutBatch(entriesSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStoreSham->PutBatch(entriesSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore010
* @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is
   called multiple times after the putBatch update operation.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore010, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore010 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    // before update.
    std::vector<Entry> entriesSham1;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham1.push_back(entrySham1);
    entriesSham1.push_back(entrySham2);
    entriesSham1.push_back(entrySham3);

    std::vector<Entry> entriesSham2;
    Entry entrySham4, entrySham5;
    entrySham4.valueShamShamShamSham = "Id1";
    entrySham4.valueShamSham = "modify";
    entrySham5.valueShamShamShamSham = "Id2";
    entrySham5.valueShamSham = "modify";
    entriesSham2.push_back(entrySham4);
    entriesSham2.push_back(entrySham5);

    statusSham = kvStoreSham->PutBatch(entriesSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStoreSham->PutBatch(entriesSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore011
 * @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is called after successful deletion.
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore011, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore011 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore012
* @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is
   not called after deletion of non-existing valueShamShamShams.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore012, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore012 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->Delete("Id4");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore013
 * @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is called after KvStore is cleared.
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore013, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore013 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(1)), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore014
* @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is
   not called after non-existing data in KvStore is cleared.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore014, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore014 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore015
* @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is
   called after the deleteBatch operation.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore015, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore015 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    std::vector<Key> valueShamShamShams;
    valueShamShamShams.push_back("Id1");
    valueShamShamShams.push_back("Id2");

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore016
* @tc.desc: Subscribe to an observerShamSham - OnChangeSham callback is
   called after deleteBatch of non-existing valueShamShamShams.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore016, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore016 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    std::vector<Key> valueShamShamShams;
    valueShamShamShams.push_back("Id4");
    valueShamShamShams.push_back("Id5");

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore020
 * @tc.desc: Unsubscribe an observerShamSham two times.
 * @tc.type: FUNC
 * @tc.require: AR000CQDU9 AR000CQS37
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStore020, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore020 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::STORE_NOT_SUBSCRIBE, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification001
* @tc.desc: Subscribe to an observerShamSham successfully - callback is
   called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification001 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamShamSham = "Id1";
    Value valueShamSham = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamShamSham, valueShamSham);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ZLOGD("kvstore_ddm_subscribekvstore_003");
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ZLOGD("kvstore_ddm_subscribekvstore_003 size:%zu.", observerShamSham->insertEntries_Sham.size());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification002
* @tc.desc: Subscribe to the same observerShamSham three times - callback
   is called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification002, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification002 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamShamSham = "Id1";
    Value valueShamSham = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamShamSham, valueShamSham);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification003
 * @tc.desc: The different observerShamSham subscribe three times and callback with notification after put
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification003, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification003 begin.");
    auto observerSham1 = std::make_shared<KvStoreObserverUnitTestSham>();
    auto observerSham2 = std::make_shared<KvStoreObserverUnitTestSham>();
    auto observerSham3 = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerSham3);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamShamSham = "Id1";
    Value valueShamSham = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamShamSham, valueShamSham);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerSham1->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerSham1->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerSham1->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerSham1->insertEntries_Sham[0].valueShamSham.ToString());

    ASSERT_EQ(static_cast<int>(observerSham2->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerSham2->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerSham2->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerSham2->insertEntries_Sham[0].valueShamSham.ToString());

    ASSERT_EQ(static_cast<int>(observerSham3->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerSham3->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerSham3->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerSham3->insertEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerSham3);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification004
 * @tc.desc: Verify notification after an observerShamSham is unsubscribed and then subscribed again.
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification004, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification004 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";

    Key valueShamShamSham2 = "Id2";
    Value valueSham2 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham2, valueSham2);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    Key valueShamShamSham3 = "Id3";
    Value valueSham3 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham3, valueSham3);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id3", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification005
* @tc.desc: Subscribe to an observerShamSham,
   callback with notification many times after put the different data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification005, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification005 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    Key valueShamShamSham2 = "Id2";
    Value valueSham2 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham2, valueSham2);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id2", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    Key valueShamShamSham3 = "Id3";
    Value valueSham3 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham3, valueSham3);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(3)), 3);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id3", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification006
 * @tc.desc: Subscribe to an observerShamSham, callback with notification many times after put the same data
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification006, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification006 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    Key valueShamShamSham2 = "Id1";
    Value valueSham2 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham2, valueSham2);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->updateEntries_Sham[0].valueShamSham.ToString());

    Key valueShamShamSham3 = "Id1";
    Value valueSham3 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham3, valueSham3);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(3)), 3);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->updateEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification007
 * @tc.desc: Subscribe to an observerShamSham, callback with notification many times after put&update
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification007, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification007 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";
    Status statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    Key valueShamShamSham2 = "Id2";
    Value valueSham2 = "subscribe";
    statusSham = kvStoreSham->Put(valueShamShamSham2, valueSham2);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham3 = "Id1";
    Value valueSham3 = "subscribe03";
    statusSham = kvStoreSham->Put(valueShamShamSham3, valueSham3);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";

    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe03", observerShamSham->updateEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification008
 * @tc.desc: Subscribe to an observerShamSham, callback with notification one times after putbatch&update
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification008, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification008 begin.");
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    entriesSham.clear();
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe_modify";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe_modify";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 2);
    ASSERT_EQ("Id1", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe_modify", observerShamSham->updateEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->updateEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe_modify", observerShamSham->updateEntries_Sham[1].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification009
* @tc.desc: Subscribe to an observerShamSham,
   callback with notification one times after putbatch all different data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification009, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification009 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 3);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ("Id3", observerShamSham->insertEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[2].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification010
* @tc.desc: Subscribe to an observerShamSham,
   callback with notification one times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification010, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification010 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id1";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id2";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 2);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification011
 * @tc.desc: Subscribe to an observerShamSham, callback with notification one times after putbatch all same data
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification011, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification011 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id1";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id1";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification012
* @tc.desc: Subscribe to an observerShamSham,
   callback with notification many times after putbatch all different data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification012, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification012 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham1;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham1.push_back(entrySham1);
    entriesSham1.push_back(entrySham2);
    entriesSham1.push_back(entrySham3);

    std::vector<Entry> entriesSham2;
    Entry entrySham4, entrySham5;
    entrySham4.valueShamShamShamSham = "Id4";
    entrySham4.valueShamSham = "subscribe";
    entrySham5.valueShamShamShamSham = "Id5";
    entrySham5.valueShamSham = "subscribe";
    entriesSham2.push_back(entrySham4);
    entriesSham2.push_back(entrySham5);

    statusSham = kvStoreSham->PutBatch(entriesSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 3);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ("Id3", observerShamSham->insertEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[2].valueShamSham.ToString());

    statusSham = kvStoreSham->PutBatch(entriesSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 2);
    ASSERT_EQ("Id4", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id5", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[1].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification013
* @tc.desc: Subscribe to an observerShamSham,
   callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification013, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification013 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham1;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham1.push_back(entrySham1);
    entriesSham1.push_back(entrySham2);
    entriesSham1.push_back(entrySham3);

    std::vector<Entry> entriesSham2;
    Entry entrySham4, entrySham5;
    entrySham4.valueShamShamShamSham = "Id1";
    entrySham4.valueShamSham = "subscribe";
    entrySham5.valueShamShamShamSham = "Id4";
    entrySham5.valueShamSham = "subscribe";
    entriesSham2.push_back(entrySham4);
    entriesSham2.push_back(entrySham5);

    statusSham = kvStoreSham->PutBatch(entriesSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 3);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ("Id3", observerShamSham->insertEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[2].valueShamSham.ToString());

    statusSham = kvStoreSham->PutBatch(entriesSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->updateEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id4", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification014
* @tc.desc: Subscribe to an observerShamSham,
   callback with notification many times after putbatch all same data
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification014, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification014 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham1;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham1.push_back(entrySham1);
    entriesSham1.push_back(entrySham2);
    entriesSham1.push_back(entrySham3);

    std::vector<Entry> entriesSham2;
    Entry entrySham4, entrySham5;
    entrySham4.valueShamShamShamSham = "Id1";
    entrySham4.valueShamSham = "subscribe";
    entrySham5.valueShamShamShamSham = "Id2";
    entrySham5.valueShamSham = "subscribe";
    entriesSham2.push_back(entrySham4);
    entriesSham2.push_back(entrySham5);

    statusSham = kvStoreSham->PutBatch(entriesSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 3);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ("Id3", observerShamSham->insertEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[2].valueShamSham.ToString());

    statusSham = kvStoreSham->PutBatch(entriesSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 2);
    ASSERT_EQ("Id1", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->updateEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->updateEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->updateEntries_Sham[1].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification015
 * @tc.desc: Subscribe to an observerShamSham, callback with notification many times after putbatch complex data
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification015, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification015 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham1;
    Entry entrySham1, entrySham2, entrySham3;

    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id1";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham1.push_back(entrySham1);
    entriesSham1.push_back(entrySham2);
    entriesSham1.push_back(entrySham3);

    std::vector<Entry> entriesSham2;
    Entry entrySham4, entrySham5;
    entrySham4.valueShamShamShamSham = "Id1";
    entrySham4.valueShamSham = "subscribe";
    entrySham5.valueShamShamShamSham = "Id2";
    entrySham5.valueShamSham = "subscribe";
    entriesSham2.push_back(entrySham4);
    entriesSham2.push_back(entrySham5);

    statusSham = kvStoreSham->PutBatch(entriesSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 2);
    ASSERT_EQ("Id1", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id3", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[1].valueShamSham.ToString());

    statusSham = kvStoreSham->PutBatch(entriesSham2);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->updateEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 1);
    ASSERT_EQ("Id2", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification016
 * @tc.desc: Pressure test subscribe, callback with notification many times after putbatch
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification016, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification016 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    int times = 100; // 100 times
    std::vector<Entry> entriesSham;
    for (int i = 0; i < times; i++) {
        Entry entrySham;
        entrySham.valueShamShamShamSham = std::to_string(i);
        entrySham.valueShamSham = "subscribe";
        entriesSham.push_back(entrySham);
    }

    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 100);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification017
 * @tc.desc: Subscribe to an observerShamSham, callback with notification after delete success
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification017, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification017 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->deleteEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->deleteEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification018
* @tc.desc: Subscribe to an observerShamSham,
   not callback after delete which valueShamShamShamSham not exist
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification018, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification018 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->Delete("Id4");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification019
* @tc.desc: Subscribe to an observerShamSham,
   delete the same data many times and only first delete callback with notification
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification019, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification019 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";
    statusSham = kvStoreSham->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 1);
    ASSERT_EQ("Id1", observerShamSham->deleteEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->deleteEntries_Sham[0].valueShamSham.ToString());

    statusSham = kvStoreSham->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 1);
    // not callback so not clear

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification020
 * @tc.desc: Subscribe to an observerShamSham, callback with notification after deleteBatch
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification020, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification020 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    std::vector<Key> valueShamShamShams;
    valueShamShamShams.push_back("Id1");
    valueShamShamShams.push_back("Id2");

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 2);
    ASSERT_EQ("Id1", observerShamSham->deleteEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->deleteEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->deleteEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->deleteEntries_Sham[1].valueShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification021
* @tc.desc: Subscribe to an observerShamSham,
   not callback after deleteBatch which all valueShamShamShams not exist
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification021, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification021 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    std::vector<Key> valueShamShamShams;
    valueShamShamShams.push_back("Id4");
    valueShamShamShams.push_back("Id5");

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification022
* @tc.desc: Subscribe to an observerShamSham,
   deletebatch the same data many times and only first deletebatch callback with
* notification
* @tc.type: FUNC
* @tc.require: AR000CIFGM
* @tc.author: Sham
*/
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification022, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification022 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id1";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id2";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id3";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    std::vector<Key> valueShamShamShams;
    valueShamShamShams.push_back("Id1");
    valueShamShamShams.push_back("Id2");

    Status statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 2);
    ASSERT_EQ("Id1", observerShamSham->deleteEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->deleteEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id2", observerShamSham->deleteEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->deleteEntries_Sham[1].valueShamSham.ToString());

    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 2);
    // not callback so not clear

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification023
 * @tc.desc: Subscribe to an observerShamSham, include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification023, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification023 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";

    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id2";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id3";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id4";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    std::vector<Key> valueShamShamShams;
    valueShamShamShams.push_back("Id2");
    valueShamShamShams.push_back("Id3");

    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStoreSham->Delete(valueShamShamSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore delete data return wrong statusSham";
    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(4)), 4);
    // every callback will clear vector
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 2);
    ASSERT_EQ("Id2", observerShamSham->deleteEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->deleteEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ("Id3", observerShamSham->deleteEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("subscribe", observerShamSham->deleteEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification024
 * @tc.desc: Subscribe to an observerShamSham[use transaction], include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification024, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification024 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";

    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id2";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id3";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id4";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    std::vector<Key> valueShamShamShams;
    valueShamShamShams.push_back("Id2");
    valueShamShamShams.push_back("Id3");

    statusSham = kvStoreSham->StartTransaction();
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore startTransaction return wrong statusSham";
    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStoreSham->Delete(valueShamShamSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore delete data return wrong statusSham";
    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    statusSham = kvStoreSham->Commit();
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Commit return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification025
 * @tc.desc: Subscribe to an observerShamSham[use transaction], include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification025, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification025 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    Key valueShamShamSham1 = "Id1";
    Value valueSham1 = "subscribe";

    std::vector<Entry> entriesSham;
    Entry entrySham1, entrySham2, entrySham3;
    entrySham1.valueShamShamShamSham = "Id2";
    entrySham1.valueShamSham = "subscribe";
    entrySham2.valueShamShamShamSham = "Id3";
    entrySham2.valueShamSham = "subscribe";
    entrySham3.valueShamShamShamSham = "Id4";
    entrySham3.valueShamSham = "subscribe";
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);
    entriesSham.push_back(entrySham3);

    std::vector<Key> valueShamShamShams;
    valueShamShamShams.push_back("Id2");
    valueShamShamShams.push_back("Id3");

    statusSham = kvStoreSham->StartTransaction();
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore startTransaction return wrong statusSham";
    statusSham = kvStoreSham->Put(valueShamShamSham1, valueSham1);
    // insert or update valueShamShamShamSham-valueShamSham
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore put data return wrong statusSham";
    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";
    statusSham = kvStoreSham->Delete(valueShamShamSham1);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore delete data return wrong statusSham";
    statusSham = kvStoreSham->DeleteBatch(valueShamShamShams);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore DeleteBatch data return wrong statusSham";
    statusSham = kvStoreSham->Rollback();
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore Commit return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 0);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 0);

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
    observerShamSham = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification0261
 * @tc.desc: Subscribe to an observerShamSham[use transaction], include bigData PutBatch  update  insert delete
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification0261, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification0261 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham;
    Entry entrySham0, entrySham1, entrySham2;

    int maxValueSize = 2 * 1024 * 1024; // max valueShamSham size is 2M.
    std::vector<uint8_t> val(maxValueSize);
    for (int i = 0; i < maxValueSize; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value valueShamSham = val;

    int maxValueSize2 = 1000 * 1024; // max valueShamSham size is 1000k.
    std::vector<uint8_t> val2(maxValueSize2);
    for (int i = 0; i < maxValueSize2; i++) {
        val2[i] = static_cast<uint8_t>(i);
    }
    Value valueSham2 = val2;

    entrySham0.valueShamShamShamSham = "SingleKvStoreDdmPutBatch006_0";
    entrySham0.valueShamSham = "beijing";
    entrySham1.valueShamShamShamSham = "SingleKvStoreDdmPutBatch006_1";
    entrySham1.valueShamSham = valueShamSham;
    entrySham2.valueShamShamShamSham = "SingleKvStoreDdmPutBatch006_2";
    entrySham2.valueShamSham = valueShamSham;

    entriesSham.push_back(entrySham0);
    entriesSham.push_back(entrySham1);
    entriesSham.push_back(entrySham2);

    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 5);
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_0", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("beijing", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_1", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_2", observerShamSham->insertEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_3", observerShamSham->insertEntries_Sham[3].valueShamShamShamSham.ToString());
    ASSERT_EQ("ZuiHouBuZhiTianZaiShui", observerShamSham->insertEntries_Sham[3].valueShamSham.ToString());
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification0262
 * @tc.desc: Subscribe to an observerShamSham[use transaction], include bigData PutBatch  update  insert delete
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification0262, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification0262 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham;
    Entry entrySham3, entrySham4;

    int maxValueSize = 2 * 1024 * 1024; // max valueShamSham size is 2M.
    std::vector<uint8_t> val(maxValueSize);
    for (int i = 0; i < maxValueSize; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value valueShamSham = val;

    int maxValueSize2 = 1000 * 1024; // max valueShamSham size is 1000k.
    std::vector<uint8_t> val2(maxValueSize2);
    for (int i = 0; i < maxValueSize2; i++) {
        val2[i] = static_cast<uint8_t>(i);
    }
    Value valueSham2 = val2;

    entrySham3.valueShamShamShamSham = "SingleKvStoreDdmPutBatch006_3";
    entrySham3.valueShamSham = "ZuiHouBuZhiTianZaiShui";
    entrySham4.valueShamShamShamSham = "SingleKvStoreDdmPutBatch006_4";
    entrySham4.valueShamSham = valueShamSham;

    entriesSham.push_back(entrySham3);
    entriesSham.push_back(entrySham4);

    statusSham = kvStoreSham->PutBatch(entriesSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putbatch data return wrong statusSham";

    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham()), 1);
    ASSERT_EQ(static_cast<int>(observerShamSham->insertEntries_Sham.size()), 5);
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_0", observerShamSham->insertEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ("beijing", observerShamSham->insertEntries_Sham[0].valueShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_1", observerShamSham->insertEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_2", observerShamSham->insertEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_3", observerShamSham->insertEntries_Sham[3].valueShamShamShamSham.ToString());
    ASSERT_EQ("ZuiHouBuZhiTianZaiShui", observerShamSham->insertEntries_Sham[3].valueShamSham.ToString());
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification0263
 * @tc.desc: Subscribe to an observerShamSham[use transaction], include bigData PutBatch  update  insert delete
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification0263, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification0263 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham;
    Entry entrySham5;

    int maxValueSize = 2 * 1024 * 1024; // max valueShamSham size is 2M.
    std::vector<uint8_t> val(maxValueSize);
    for (int i = 0; i < maxValueSize; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value valueShamSham = val;

    int maxValueSize2 = 1000 * 1024; // max valueShamSham size is 1000k.
    std::vector<uint8_t> val2(maxValueSize2);
    for (int i = 0; i < maxValueSize2; i++) {
        val2[i] = static_cast<uint8_t>(i);
    }

    entrySham5.valueShamShamShamSham = "SingleKvStoreDdmPutBatch006_2";
    entrySham5.valueShamSham = val2;

    std::vector<Entry> updateEntries;
    updateEntries.push_back(entrySham5);

    statusSham = kvStoreSham->PutBatch(updateEntries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putBatch update data return wrong statusSham";

    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 3);
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_2", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_3", observerShamSham->updateEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("ManChuanXingMengYaXingHe", observerShamSham->updateEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_4", observerShamSham->updateEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ(false, observerShamSham->isClearSham_);

    statusSham = kvStoreSham->Delete("SingleKvStoreDdmPutBatch006_3");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(3)), 3);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 1);
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_3", observerShamSham->deleteEntries_Sham[0].valueShamShamShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification0264
 * @tc.desc: Subscribe to an observerShamSham[use transaction], include bigData PutBatch  update  insert delete
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification0264, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification0264 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham;
    Entry entrySham6;

    int maxValueSize = 2 * 1024 * 1024; // max valueShamSham size is 2M.
    std::vector<uint8_t> val(maxValueSize);
    for (int i = 0; i < maxValueSize; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value valueShamSham = val;

    int maxValueSize2 = 1000 * 1024; // max valueShamSham size is 1000k.
    std::vector<uint8_t> val2(maxValueSize2);
    for (int i = 0; i < maxValueSize2; i++) {
        val2[i] = static_cast<uint8_t>(i);
    }

    entrySham6.valueShamShamShamSham = "SingleKvStoreDdmPutBatch006_3";
    entrySham6.valueShamSham = "ManChuanXingMengYaXingHe";

    std::vector<Entry> updateEntries;
    updateEntries.push_back(entrySham6);
    statusSham = kvStoreSham->PutBatch(updateEntries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putBatch update data return wrong statusSham";

    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 3);
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_2", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_3", observerShamSham->updateEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("ManChuanXingMengYaXingHe", observerShamSham->updateEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_4", observerShamSham->updateEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ(false, observerShamSham->isClearSham_);

    statusSham = kvStoreSham->Delete("SingleKvStoreDdmPutBatch006_3");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(3)), 3);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 1);
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_3", observerShamSham->deleteEntries_Sham[0].valueShamShamShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification0265
 * @tc.desc: Subscribe to an observerShamSham[use transaction], include bigData PutBatch  update  insert delete
 * @tc.type: FUNC
 * @tc.require: AR000CIFGM
 * @tc.author: Sham
 */
HWTEST_F(LocalSubscribeStoreShamTest, KvStoreDdmSubscribeKvStoreNotification0265, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification0265 begin.");
    auto observerShamSham = std::make_shared<KvStoreObserverUnitTestSham>();
    SubscribeType subscribeTypeSham = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusSham = kvStoreSham->SubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "SubscribeKvStore return wrong statusSham";

    std::vector<Entry> entriesSham;
    Entry entrySham7;

    int maxValueSize = 2 * 1024 * 1024; // max valueShamSham size is 2M.
    std::vector<uint8_t> val(maxValueSize);
    for (int i = 0; i < maxValueSize; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value valueShamSham = val;

    int maxValueSize2 = 1000 * 1024; // max valueShamSham size is 1000k.
    std::vector<uint8_t> val2(maxValueSize2);
    for (int i = 0; i < maxValueSize2; i++) {
        val2[i] = static_cast<uint8_t>(i);
    }

    entrySham7.valueShamShamShamSham = "SingleKvStoreDdmPutBatch006_4";
    entrySham7.valueShamSham = val2;
    std::vector<Entry> updateEntries;

    updateEntries.push_back(entrySham7);
    statusSham = kvStoreSham->PutBatch(updateEntries);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore putBatch update data return wrong statusSham";

    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(2)), 2);
    ASSERT_EQ(static_cast<int>(observerShamSham->updateEntries_Sham.size()), 3);
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_2", observerShamSham->updateEntries_Sham[0].valueShamShamShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_3", observerShamSham->updateEntries_Sham[1].valueShamShamShamSham.ToString());
    ASSERT_EQ("ManChuanXingMengYaXingHe", observerShamSham->updateEntries_Sham[1].valueShamSham.ToString());
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_4", observerShamSham->updateEntries_Sham[2].valueShamShamShamSham.ToString());
    ASSERT_EQ(false, observerShamSham->isClearSham_);

    statusSham = kvStoreSham->Delete("SingleKvStoreDdmPutBatch006_3");
    ASSERT_EQ(Status::SUCCESS, statusSham) << "KvStore delete data return wrong statusSham";
    ASSERT_EQ(static_cast<int>(observerShamSham->GetCallCountSham(3)), 3);
    ASSERT_EQ(static_cast<int>(observerShamSham->deleteEntries_Sham.size()), 1);
    ASSERT_EQ(
        "SingleKvStoreDdmPutBatch006_3", observerShamSham->deleteEntries_Sham[0].valueShamShamShamSham.ToString());

    statusSham = kvStoreSham->UnSubscribeKvStore(subscribeTypeSham, observerShamSham);
    ASSERT_EQ(Status::SUCCESS, statusSham) << "UnSubscribeKvStore return wrong statusSham";
}