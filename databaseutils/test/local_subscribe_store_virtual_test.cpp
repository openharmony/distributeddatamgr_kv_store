/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#define LOG_TAG "LocalSubscribeStoreVirtualTest"
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
class LocalSubscribeStoreVirtualTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager managerVirtual;
    static std::shared_ptr<SingleKvStore> kvStoreVirtual;
    static Status statusGetKvStoreVirtual;
    static AppId appIdVirtual;
    static StoreId storeIdVirtual;
};
std::shared_ptr<SingleKvStore> LocalSubscribeStoreVirtualTest::kvStoreVirtual = nullptr;
Status LocalSubscribeStoreVirtualTest::statusGetKvStoreVirtual = Status::ERROR;
DistributedKvDataManager LocalSubscribeStoreVirtualTest::managerVirtual;
AppId LocalSubscribeStoreVirtualTest::appIdVirtual;
StoreId LocalSubscribeStoreVirtualTest::storeIdVirtual;

void LocalSubscribeStoreVirtualTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/odmf", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void LocalSubscribeStoreVirtualTest::TearDownTestCase(void)
{
    managerVirtual.CloseKvStore(appIdVirtual, kvStoreVirtual);
    kvStoreVirtual = nullptr;
    managerVirtual.DeleteKvStore(appIdVirtual, storeIdVirtual, "/data/service/el1/public/database/odmf");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void LocalSubscribeStoreVirtualTest::SetUp(void)
{
    Options optionsVirtual;
    optionsVirtual.createIfMissing = true;
    optionsVirtual.encrypt = false;  // not supported yet.
    optionsVirtual.securityLevel = S1;
    optionsVirtual.autoSync = true;  // not supported yet.
    optionsVirtual.kvStoreType = KvStoreType::SINGLE_VERSION;
    optionsVirtual.area = EL1;
    optionsVirtual.baseDir = std::string("/data/service/el1/public/database/odmf");
    appIdVirtual.appIdVirtual = "odmf";         // define app name.
    storeIdVirtual.storeIdVirtual = "student";  // define kvstore(database) name
    managerVirtual.DeleteKvStore(appIdVirtual, storeIdVirtual, optionsVirtual.baseDir);
    // [create and] open and initialize kvstore instance.
    statusGetKvStoreVirtual =
        managerVirtual.GetSingleKvStore(optionsVirtual, appIdVirtual, storeIdVirtual, kvStoreVirtual);
    EXPECT_EQ(Status::SUCCESS, statusGetKvStoreVirtual) << "statusGetKvStoreVirtual return wrong statusVirtual";
    EXPECT_NE(nullptr, kvStoreVirtual) << "kvStoreVirtual is nullptr";
}

void LocalSubscribeStoreVirtualTest::TearDown(void)
{
    managerVirtual.CloseKvStore(appIdVirtual, kvStoreVirtual);
    kvStoreVirtual = nullptr;
    managerVirtual.DeleteKvStore(appIdVirtual, storeIdVirtual);
}

class KvStoreObserverUnitTestVirtual : public KvStoreObserver {
public:
    std::vector<Entry> insertEntries_Virtual;
    std::vector<Entry> updateEntries_Virtual;
    std::vector<Entry> deleteEntries_Virtual;
    bool isClearVirtual_ = false;
    KvStoreObserverUnitTestVirtual();
    ~KvStoreObserverUnitTestVirtual()
    {}

    KvStoreObserverUnitTestVirtual(const KvStoreObserverUnitTestVirtual &) = delete;
    KvStoreObserverUnitTestVirtual &operator=(const KvStoreObserverUnitTestVirtual &) = delete;
    KvStoreObserverUnitTestVirtual(KvStoreObserverUnitTestVirtual &&) = delete;
    KvStoreObserverUnitTestVirtual &operator=(KvStoreObserverUnitTestVirtual &&) = delete;

    void OnChangeVirtual(const ChangeNotification &changeNotification);

    // reset the callCountVirtual_to zero.
    void ResetToZero();

    uint32_t GetCallCountVirtual(uint32_t valueVirtualVirtual = 1);

private:
    std::mutex mutexVirtual_;
    uint32_t callCountVirtual_ = 0;
    BlockData<uint32_t> valueVirtual_Virtual{ 1, 0 };
};

KvStoreObserverUnitTestVirtual::KvStoreObserverUnitTestVirtual()
{
}

void KvStoreObserverUnitTestVirtual::OnChangeVirtual(const ChangeNotification &changeNotification)
{
    ZLOGD("begin.");
    insertEntries_Virtual = changeNotification.GetInsertEntries();
    updateEntries_Virtual = changeNotification.GetUpdateEntries();
    deleteEntries_Virtual = changeNotification.GetDeleteEntries();
    changeNotification.GetDeviceId();
    isClearVirtual_ = changeNotification.IsClear();
    std::lock_guard<decltype(mutexVirtual_)> guard(mutexVirtual_);
    ++callCount_Virtual;
    valueVirtual_Virtual.SetValue(callCount_Virtual);
}

void KvStoreObserverUnitTestVirtual::ResetToZero()
{
    std::lock_guard<decltype(mutexVirtual_)> guard(mutexVirtual_);
    callCountVirtual_ = 0;
    valueVirtual_Virtual.Clear(0);
}

uint32_t KvStoreObserverUnitTestVirtual::GetCallCountVirtual(uint32_t valueVirtualVirtual)
{
    int retryVirtual = 0;
    uint32_t callTimesVirtual = 0;
    while (retryVirtual < valueVirtualVirtual) {
        callTimesVirtual = valueVirtual_Virtual.GetValue();
        if (callTimesVirtual >= valueVirtualVirtual) {
            break;
        }
        std::lock_guard<decltype(mutexVirtual_)> guard(mutexVirtual_);
        callTimesVirtual = valueVirtual_Virtual.GetValue();
        if (callTimesVirtual >= valueVirtualVirtual) {
            break;
        }
        valueVirtual_Virtual.Clear(callTimesVirtual);
        retryVirtual++;
    }
    return callTimesVirtual;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore001
* @tc.desc: Subscribe success
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore001 begin.");
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    observerVirtualVirtual->ResetToZero();

    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual()), 0);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    observerVirtualVirtual = nullptr;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore002
* @tc.desc: Subscribe fail, observerVirtualVirtual is null
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore002, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore002 begin.");
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<KvStoreObserverUnitTestVirtual> observerVirtualVirtual = nullptr;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::INVALID_ARGUMENT, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore003
* @tc.desc: Subscribe success and OnChangeVirtual callback after put
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore003, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore003 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key valueVirtualVirtualVirtualVirtual = "Id1";
    Value valueVirtualVirtual = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtualVirtual, valueVirtualVirtual);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual()), 1);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    observerVirtualVirtual = nullptr;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore004
* @tc.desc: The same observerVirtualVirtual subscribe three times and OnChangeVirtual callback after put
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore004, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore004 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key valueVirtualVirtualVirtualVirtual = "Id1";
    Value valueVirtualVirtual = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtualVirtual, valueVirtualVirtual);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual()), 1);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore005
* @tc.desc: The different observerVirtualVirtual subscribe three times and OnChangeVirtual callback after put
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore005, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore005 begin.");
    auto observerVirtual1 = std::make_shared<KvStoreObserverUnitTestVirtual>();
    auto observerVirtual2 = std::make_shared<KvStoreObserverUnitTestVirtual>();
    auto observerVirtual3 = std::make_shared<KvStoreObserverUnitTestVirtual>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual1);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore failed, wrong statusVirtual";
    statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore failed, wrong statusVirtual";
    statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtual3);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore failed, wrong statusVirtual";

    Key valueVirtualVirtualVirtualVirtual = "Id1";
    Value valueVirtualVirtual = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtualVirtual, valueVirtualVirtual);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "Putting data to KvStore failed, wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtual1->GetCallCountVirtual()), 1);
    EXPECT_EQ(static_cast<int>(observerVirtual2->GetCallCountVirtual()), 1);
    EXPECT_EQ(static_cast<int>(observerVirtual3->GetCallCountVirtual()), 1);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual1);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtual3);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore006
* @tc.desc: Unsubscribe an observerVirtualVirtual and
    subscribe again - the map should be cleared after unsubscription.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore006, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore006 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key valueVirtualVirtualVirtual1 = "Id1";
    Value valueVirtual1 = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual1, valueVirtual1);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual()), 1);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";

    Key valueVirtualVirtualVirtual2 = "Id2";
    Value valueVirtual2 = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual2, valueVirtual2);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual()), 1);

    kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual()), 1);
    Key valueVirtualVirtualVirtual3 = "Id3";
    Value valueVirtual3 = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual3, valueVirtual3);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual(2)), 2);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore007
* @tc.desc: Subscribe to an observerVirtualVirtual - OnChangeVirtual callback is
    called multiple times after the put operation.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore007, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore007 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key valueVirtualVirtualVirtual1 = "Id1";
    Value valueVirtual1 = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual1, valueVirtual1);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key valueVirtualVirtualVirtual2 = "Id2";
    Value valueVirtual2 = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual2, valueVirtual2);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key valueVirtualVirtualVirtual3 = "Id3";
    Value valueVirtual3 = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual3, valueVirtual3);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual(3)), 3);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore008
* @tc.desc: Subscribe to an observerVirtualVirtual - OnChangeVirtual callback is
    called multiple times after the put&update operations.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore008, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore008 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    Key valueVirtualVirtualVirtual1 = "Id1";
    Value valueVirtual1 = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual1, valueVirtual1);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key valueVirtualVirtualVirtual2 = "Id2";
    Value valueVirtual2 = "subscribe";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual2, valueVirtual2);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";

    Key valueVirtualVirtualVirtual3 = "Id1";
    Value valueVirtual3 = "subscribe03";
    statusVirtual = kvStoreVirtual->Put(valueVirtualVirtualVirtual3, valueVirtual3);
    // insert or update valueVirtualVirtualVirtualVirtual-valueVirtualVirtual
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore put data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual(3)), 3);
    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore009
* @tc.desc: Subscribe to an observerVirtualVirtual - OnChangeVirtual callback is
    called multiple times after the putBatch operation.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore009, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore009 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    // before update.
    std::vector<Entry> entriesVirtual1;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.valueVirtualVirtualVirtualVirtual = "Id1";
    entryVirtual1.valueVirtualVirtual = "subscribe";
    entryVirtual2.valueVirtualVirtualVirtualVirtual = "Id2";
    entryVirtual2.valueVirtualVirtual = "subscribe";
    entryVirtual3.valueVirtualVirtualVirtualVirtual = "Id3";
    entryVirtual3.valueVirtualVirtual = "subscribe";
    entriesVirtual1.push_back(entryVirtual1);
    entriesVirtual1.push_back(entryVirtual2);
    entriesVirtual1.push_back(entryVirtual3);

    std::vector<Entry> entriesVirtual2;
    Entry entryVirtual4, entryVirtual5;
    entryVirtual4.valueVirtualVirtualVirtualVirtual = "Id4";
    entryVirtual4.valueVirtualVirtual = "subscribe";
    entryVirtual5.valueVirtualVirtualVirtualVirtual = "Id5";
    entryVirtual5.valueVirtualVirtual = "subscribe";
    entriesVirtual2.push_back(entryVirtual4);
    entriesVirtual2.push_back(entryVirtual5);

    statusVirtual = kvStoreVirtual->PutBatch(entriesVirtual1);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    statusVirtual = kvStoreVirtual->PutBatch(entriesVirtual2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual(2)), 2);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore010
* @tc.desc: Subscribe to an observerVirtualVirtual - OnChangeVirtual callback is
    called multiple times after the putBatch update operation.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore010, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore010 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";

    // before update.
    std::vector<Entry> entriesVirtual1;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.valueVirtualVirtualVirtualVirtual = "Id1";
    entryVirtual1.valueVirtualVirtual = "subscribe";
    entryVirtual2.valueVirtualVirtualVirtualVirtual = "Id2";
    entryVirtual2.valueVirtualVirtual = "subscribe";
    entryVirtual3.valueVirtualVirtualVirtualVirtual = "Id3";
    entryVirtual3.valueVirtualVirtual = "subscribe";
    entriesVirtual1.push_back(entryVirtual1);
    entriesVirtual1.push_back(entryVirtual2);
    entriesVirtual1.push_back(entryVirtual3);

    std::vector<Entry> entriesVirtual2;
    Entry entryVirtual4, entryVirtual5;
    entryVirtual4.valueVirtualVirtualVirtualVirtual = "Id1";
    entryVirtual4.valueVirtualVirtual = "modify";
    entryVirtual5.valueVirtualVirtualVirtualVirtual = "Id2";
    entryVirtual5.valueVirtualVirtual = "modify";
    entriesVirtual2.push_back(entryVirtual4);
    entriesVirtual2.push_back(entryVirtual5);

    statusVirtual = kvStoreVirtual->PutBatch(entriesVirtual1);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    statusVirtual = kvStoreVirtual->PutBatch(entriesVirtual2);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual(2)), 2);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore011
* @tc.desc: Subscribe to an observerVirtualVirtual - OnChangeVirtual callback is called after successful deletion.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore011, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore011 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    std::vector<Entry> entriesVirtual;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.valueVirtualVirtualVirtualVirtual = "Id1";
    entryVirtual1.valueVirtualVirtual = "subscribe";
    entryVirtual2.valueVirtualVirtualVirtualVirtual = "Id2";
    entryVirtual2.valueVirtualVirtual = "subscribe";
    entryVirtual3.valueVirtualVirtualVirtualVirtual = "Id3";
    entryVirtual3.valueVirtualVirtual = "subscribe";
    entriesVirtual.push_back(entryVirtual1);
    entriesVirtual.push_back(entryVirtual2);
    entriesVirtual.push_back(entryVirtual3);

    Status statusVirtual = kvStoreVirtual->PutBatch(entriesVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStoreVirtual->Delete("Id1");
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore Delete data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual()), 1);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore012
* @tc.desc: Subscribe to an observerVirtualVirtual - OnChangeVirtual callback is
    not called after deletion of non-existing valueVirtualVirtualVirtuals.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore012, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore012 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    std::vector<Entry> entriesVirtual;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.valueVirtualVirtualVirtualVirtual = "Id1";
    entryVirtual1.valueVirtualVirtual = "subscribe";
    entryVirtual2.valueVirtualVirtualVirtualVirtual = "Id2";
    entryVirtual2.valueVirtualVirtual = "subscribe";
    entryVirtual3.valueVirtualVirtualVirtualVirtual = "Id3";
    entryVirtual3.valueVirtualVirtual = "subscribe";
    entriesVirtual.push_back(entryVirtual1);
    entriesVirtual.push_back(entryVirtual2);
    entriesVirtual.push_back(entryVirtual3);

    Status statusVirtual = kvStoreVirtual->PutBatch(entriesVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    statusVirtual = kvStoreVirtual->Delete("Id4");
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore Delete data return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual()), 0);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore013
* @tc.desc: Subscribe to an observerVirtualVirtual - OnChangeVirtual callback is called after KvStore is cleared.
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: Virtual
*/
HWTEST_F(LocalSubscribeStoreVirtualTest, KvStoreDdmSubscribeKvStore013, TestSize.Level0)
{
    ZLOGI("KvStoreDdmSubscribeKvStore013 begin.");
    auto observerVirtualVirtual = std::make_shared<KvStoreObserverUnitTestVirtual>();
    std::vector<Entry> entriesVirtual;
    Entry entryVirtual1, entryVirtual2, entryVirtual3;
    entryVirtual1.valueVirtualVirtualVirtualVirtual = "Id1";
    entryVirtual1.valueVirtualVirtual = "subscribe";
    entryVirtual2.valueVirtualVirtualVirtualVirtual = "Id2";
    entryVirtual2.valueVirtualVirtual = "subscribe";
    entryVirtual3.valueVirtualVirtualVirtualVirtual = "Id3";
    entryVirtual3.valueVirtualVirtual = "subscribe";
    entriesVirtual.push_back(entryVirtual1);
    entriesVirtual.push_back(entryVirtual2);
    entriesVirtual.push_back(entryVirtual3);

    Status statusVirtual = kvStoreVirtual->PutBatch(entriesVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "KvStore putbatch data return wrong statusVirtual";

    SubscribeType subscribeTypeVirtual = SubscribeType::SUBSCRIBE_TYPE_ALL;
    statusVirtual = kvStoreVirtual->SubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "SubscribeKvStore return wrong statusVirtual";
    EXPECT_EQ(static_cast<int>(observerVirtualVirtual->GetCallCountVirtual(1)), 0);

    statusVirtual = kvStoreVirtual->UnSubscribeKvStore(subscribeTypeVirtual, observerVirtualVirtual);
    EXPECT_EQ(Status::SUCCESS, statusVirtual) << "UnSubscribeKvStore return wrong statusVirtual";
}