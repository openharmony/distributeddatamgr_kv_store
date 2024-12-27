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
#include "distributedkvdataservice_fuzzer.h"

#include "distributed_kv_data_manager.h"
#include "kvstore_death_recipient.h"
#include "kvstore_observer.h"
#include "types.h"
#include <vector>
#include <sys/stat.h>

using namespace OHOS;
using namespace OHOS::DistributedKv;

class DistributedKvDataServiceFuzzer {
    /* Keep C++ file names the same as the class name */
};

namespace OHOS {

static std::shared_ptr<SingleKvStore> singleKvStore = nullptr;
static distributedkvdataservice service;
static AppId appIdTest;
static StoreId storeIdTest;
static Options createTest;
static Options noCreate;
static UserId userId;

class MyDeathRecipient : public KvStoreDeathRecipient {
public:
    MyDeathRecipient() { }
    virtual ~MyDeathRecipient() { }
    void OnRemoteDied() override { }
};

class SwitchDataObserver : public KvStoreObserver {
public:
    void OnSwitchChange(const SwitchNotification &notification) override
    {
        blockData_.SetValue(notification);
    }

    SwitchNotification Get()
    {
        return blockData_.GetValue();
    }

private:
    BlockData<SwitchNotification> blockData_ { 1, SwitchNotification() };
};

void SetUpTestCase(void)
{
    userId.userId = "account";
    appIdTest.appId = "distributedkvdataservicefuzzertest";
    createTest.createIfMissing = true;
    createTest.encrypt = false;
    createTest.autoSync = true;
    createTest.kvStoreType = SINGLE_VERSION;
    createTest.area = EL1;
    createTest.baseDir = std::string("/data/service/el1/public/database/") + appIdTest.appId;
    mkdir(createTest.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    service.CloseAllKvStore(appIdTest);
    service.DeleteAllKvStore(appIdTest, createTest.baseDir);
}

void TearDown(void)
{
    service.CloseAllKvStore(appIdTest);
    service.DeleteAllKvStore(appIdTest, createTest.baseDir);
    (void)remove("/data/service/el1/public/database/distributedkvdataservicefuzzertest/key");
    (void)remove("/data/service/el1/public/database/distributedkvdataservicefuzzertest/kvdb");
    (void)remove("/data/service/el1/public/database/distributedkvdataservicefuzzertest");
}

void GetKvStoreFuzzTest(const uint8_t *data, size_t size)
{
    StoreId storeId;
    storeId.storeId = std::string(data, data + size);
    std::shared_ptr<SingleKvStore> notExistKvStore;
    service.GetSingleKvStore(createTest, appIdTest, storeId, notExistKvStore);
    std::shared_ptr<SingleKvStore> existKvStore;
    service.GetSingleKvStore(noCreate, appIdTest, storeId, existKvStore);
    service.CloseKvStore(appIdTest, storeId);
    service.DeleteKvStore(appIdTest, storeId);
}

void GetAllKvStoreFuzzTest(const uint8_t *data, size_t size)
{
    std::vector<StoreId> storeIds;
    service.GetAllKvStoreId(appIdTest, storeIds);

    std::shared_ptr<SingleKvStore> KvStore;
    std::string storeIdBase(data, data + size);
    int sum = 10;

    for (int i = 0; i < sum; i++) {
        StoreId storeId;
        storeId.storeId = storeIdBase + "_" + std::to_string(i);
        service.GetSingleKvStore(createTest, appIdTest, storeId, KvStore);
    }
    service.GetAllKvStoreId(appIdTest, storeIds);
    service.CloseAllKvStore(appIdTest);

    service.GetAllKvStoreId(appIdTest, storeIds);
}

void CloseKvStoreFuzzTest(const uint8_t *data, size_t size)
{
    StoreId storeId;
    storeId.storeId = std::string(data, data + size);
    service.CloseKvStore(appIdTest, storeId);
    std::shared_ptr<SingleKvStore> kvStoreImpl;
    service.GetSingleKvStore(createTest, appIdTest, storeId, kvStoreImpl);
    service.CloseKvStore(appIdTest, storeId);
    service.CloseKvStore(appIdTest, storeId);
}

void DeleteKvStoreFuzzTest(const uint8_t *data, size_t size)
{
    StoreId storeId;
    storeId.storeId = std::string(data, data + size);
    service.DeleteKvStore(appIdTest, storeId, createTest.baseDir);

    std::shared_ptr<SingleKvStore> kvStoreImpl;
    service.GetSingleKvStore(createTest, appIdTest, storeId, kvStoreImpl);
    service.CloseKvStore(appIdTest, storeId);
    service.DeleteKvStore(appIdTest, storeId, createTest.baseDir);

    service.CloseKvStore(appIdTest, storeId);
}

void DeleteAllKvStoreFuzz1Test(const uint8_t *data, size_t size)
{
    std::vector<StoreId> storeIds;
    service.GetAllKvStoreId(appIdTest, storeIds);

    service.DeleteAllKvStore(appIdTest, createTest.baseDir);
    std::shared_ptr<SingleKvStore> KvStore;
    std::string storeIdBase(data, data + size);

    int sum = 10;
    for (int i = 0; i < sum; i++) {
        StoreId storeId;
        storeId.storeId = storeIdBase + "_" + std::to_string(i);
        service.GetSingleKvStore(createTest, appIdTest, storeId, KvStore);

        service.CloseKvStore(appIdTest, storeId);
    }
    service.DeleteAllKvStore(appIdTest, createTest.baseDir);
}

void DeleteAllKvStoreFuzz2Test(const uint8_t *data, size_t size)
{
    std::vector<StoreId> storeIds;
    service.GetAllKvStoreId(appIdTest, storeIds);

    std::shared_ptr<SingleKvStore> KvStore;
    std::string storeIdBase(data, data + size);
    service.GetSingleKvStore(create, appIdTest, storeIdTest, KvStore);
    service.CloseKvStore(appIdTest, storeIdTest);
    int sum = 10;

    for (int i = 0; i < sum; i++) {
        StoreId storeId;
        storeId.storeId = storeIdBase + "_" + std::to_string(i);
        service.GetSingleKvStore(create, appIdTest, storeId, KvStore);
    }
    service.DeleteAllKvStore(appIdTest, createTest.baseDir);
}

void DeleteAllKvStoreFuzz3Test(const uint8_t *data, size_t size)
{
    std::vector<StoreId> storeIds;
    service.GetAllKvStoreId(appIdTest, storeIds);

    std::shared_ptr<SingleKvStore> KvStore;
    std::string storeIdBase(data, data + size);
    int sum = 10;
    for (int i = 0; i < sum; i++) {
        StoreId storeId;
        storeId.storeId = storeIdBase + "_" + std::to_string(i);
        service.GetSingleKvStore(create, appIdTest, storeId, KvStore);
    }
    service.DeleteAllKvStore(appIdTest, createTest.baseDir);
}

void RegisterKvStoreServiceDeathRecipientFuzzTest()
{
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipientImpl = std::make_shared<MyDeathRecipient>();
    service.RegisterKvStoreServiceDeathRecipient(kvStoreDeathRecipientImpl);
    kvStoreDeathRecipientImpl->OnRemoteDied();
}

void UnRegisterKvStoreServiceDeathRecipientFuzzTest()
{
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipientImpl = std::make_shared<MyDeathRecipient>();

    service.UnRegisterKvStoreServiceDeathRecipient(kvStoreDeathRecipientImpl);
    kvStoreDeathRecipientImpl->OnRemoteDied();
}

void PutSwitchFuzzTest(const uint8_t *data, size_t size)
{
    std::string appIds(data, data + size);
    uint32_t input = static_cast<uint32_t>(size);
    SwitchData switchData11;
    switchData11.value = input;
    switchData11.length = input & 0xFFFF;

    service.PutSwitch({ appIds }, switchData11);
    service.PutSwitch({ "distributed_device_profile_service" }, switchData11);
}

void GetSwitchFuzzTest(const uint8_t *data, size_t size)
{
    std::string appIds(data, data + size);
    std::string networkId(data, data + size);
    
    service.GetSwitch({ appIds }, networkId);
    service.GetSwitch({ "distributed_device_profile_service" }, networkId);
}

void SubscribeSwitchDataFuzzTest(const uint8_t *data, size_t size)
{
    std::string appIds(data, data + size);
    std::shared_ptr<SwitchDataObserver> dataObserver = std::make_shared<SwitchDataObserver>();

    service.SubscribeSwitchData({ appIds }, dataObserver);
}

void UnsubscribeSwitchDataFuzzTest(const uint8_t *data, size_t size)
{
    std::string appIds(data, data + size);
    std::shared_ptr<SwitchDataObserver> dataObserver = std::make_shared<SwitchDataObserver>();

    service.SubscribeSwitchData({ appIds }, dataObserver);
    service.UnsubscribeSwitchData({ appIds }, dataObserver);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SetUpTestCase();
    OHOS::GetKvStoreFuzzTest(data, size);
    OHOS::GetAllKvStoreFuzzTest(data, size);
    OHOS::CloseKvStoreFuzzTest(data, size);
    OHOS::DeleteKvStoreFuzzTest(data, size);
    OHOS::DeleteAllKvStoreFuzz1Test(data, size);
    OHOS::DeleteAllKvStoreFuzz2Test(data, size);
    OHOS::DeleteAllKvStoreFuzz3Test(data, size);
    OHOS::RegisterKvStoreServiceDeathRecipientFuzzTest();
    OHOS::UnRegisterKvStoreServiceDeathRecipientFuzzTest();
    OHOS::PutSwitchFuzzTest(data, size);
    OHOS::GetSwitchFuzzTest(data, size);
    OHOS::SubscribeSwitchDataFuzzTest(data, size);
    OHOS::UnsubscribeSwitchDataFuzzTest(data, size);
    OHOS::TearDown();
    return 0;
}