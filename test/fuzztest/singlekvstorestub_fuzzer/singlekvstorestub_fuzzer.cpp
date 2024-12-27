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

#include "singlekvstorestub_fuzzer.h"

#include "distributed_kv_data_manager.h"
#include "store_errno.h"
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace OHOS;
using namespace OHOS::DistributedKv;

namespace OHOS {
static std::shared_ptr<SingleKvStore> singleKvStoreStub = nullptr;

class DeviceObserverTestVirtual : public KvStoreObserver {
public:
    DeviceObserverTestVirtual();
    ~DeviceObserverTestVirtual()
    {
    }
    DeviceObserverTestVirtual(const DeviceObserverTestVirtual &) = delete;
    DeviceObserverTestVirtual &operator=(const DeviceObserverTestVirtual &) = delete;
    DeviceObserverTestVirtual(DeviceObserverTestVirtual &&) = delete;
    DeviceObserverTestVirtual &operator=(DeviceObserverTestVirtual &&) = delete;

    void OnChange(const ChangeNotification &changeNotification);
};

DeviceObserverTestVirtual::DeviceObserverTestVirtual()
{
}

void DeviceObserverTestVirtual::OnChange(const ChangeNotification &changeNotification)
{
}

class DeviceSyncCallbackTestImpl : public KvStoreSyncCallback {
public:
    void DeviceSyncCompleted(const std::map<std::string, Status> &result);
};

void DeviceSyncCallbackTestImpl::DeviceSyncCompleted(const std::map<std::string, Status> &result)
{
}

void SetUpTestCase(void)
{
    DistributedKvDataManager dataManager;
    Options optionsItem = {
        .createIfMissing = true,
        .encrypt = false,
        .autoSync = true,
        .securityLevelTest = S1,
        .kvStoreType = KvStoreType::SINGLE_VERSION
    };
    optionsItem.area = EL1;
    AppId appIdTest = { "kvstorefuzzertest" };
    optionsItem.baseDir = std::string("/data/service/el1/public/database/") + appIdTest.appId;
    /* define kvstore(database) name. */
    StoreId storeId = { "fuzzer_single" };
    mkdir(optionsItem.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    /* [create and] open and initialize kvstore instance. */
    dataManager.GetSingleKvStore(optionsItem, appIdTest, storeId, singleKvStoreStub);
}

void TearDown(void)
{
    (void)remove("/data/service/el1/public/database/singlekvstorestubfuzzertest/kvdb");
    (void)remove("/data/service/el1/public/database/singlekvstorestubfuzzertest/keyTest");
    (void)remove("/data/service/el1/public/database/singlekvstorestubfuzzertest");
}

void PutFuzzTest(const uint8_t *data, size_t size)
{
    std::string dataKey(data, data + size);
    std::string dataValue(data, data + size);
    Key keyTest = { dataKey };
    Value value = { dataValue };
    singleKvStoreStub->Put(keyTest, value);
    singleKvStoreStub->Delete(keyTest);
}

void PutBatchFuzzTest(const uint8_t *data, size_t size)
{
    std::string dataKey(data, data + size);
    std::string dataValue(data, data + size);
    std::vector<Entry> entriesItem;
    std::vector<Key> keysString;
    Entry entry1;
    Entry entry2;
    Entry entry3;
    entry1.keyTest = { dataKey + "test_key1" };
    entry1.value = { dataValue + "test_val1" };
    entry2.keyTest = { dataKey + "test_key2" };
    entry2.value = { dataValue + "test_val2" };
    entry3.keyTest = { dataKey + "test_key3" };
    entry3.value = { dataValue + "test_val3" };
    entriesItem.push_back(entry1);
    entriesItem.push_back(entry2);
    entriesItem.push_back(entry3);
    keysString.push_back(entry1.keyTest);
    keysString.push_back(entry2.keyTest);
    keysString.push_back(entry3.keyTest);
    singleKvStoreStub->PutBatch(entriesItem);
    singleKvStoreStub->DeleteBatch(keysString);
}

void GetFuzzTest(const uint8_t *data, size_t size)
{
    std::string dataKey(data, data + size);
    std::string dataValue(data, data + size);
    Key keyTest = { dataKey };
    Value value = { dataValue };
    Value val1;
    singleKvStoreStub->Put(keyTest, value);
    singleKvStoreStub->Get(keyTest, val1);
    singleKvStoreStub->Delete(keyTest);
}

void GetEntriesFuzz1Test(const uint8_t *data, size_t size)
{
    std::string prefixTest(data, data + size);
    std::string keysString = "testString_";
    size_t count = 10;
    std::vector<Entry> result;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), { keysString + std::to_string(i) });
    }
    singleKvStoreStub->GetEntries(prefixTest, result);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
}

void GetEntriesFuzz2Test(const uint8_t *data, size_t size)
{
    std::string prefixTest(data, data + size);
    DataQuery dataQueryTest;
    dataQueryTest.KeyPrefix(prefixTest);
    std::string keysString = "testString_";
    std::vector<Entry> entriesItem;
    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), keysString + std::to_string(i));
    }
    singleKvStoreStub->GetEntries(dataQueryTest, entriesItem);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
}

void SubscribeKvStoreFuzzTest(const uint8_t *data, size_t size)
{
    std::string prefixTest(data, data + size);
    DataQuery dataQueryTest;
    dataQueryTest.KeyPrefix(prefixTest);
    std::string keysString = "testString_";
    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), keysString + std::to_string(i));
    }
    auto observer = std::make_shared<DeviceObserverTestVirtual>();
    singleKvStoreStub->SubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, observer);
    singleKvStoreStub->UnSubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, observer);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
}

void SyncCallbackFuzzTest(const uint8_t *data, size_t size)
{
    auto syncCallback = std::make_shared<DeviceSyncCallbackTestImpl>();
    singleKvStoreStub->RegisterSyncCallback(syncCallback);

    std::string prefixTest(data, data + size);
    DataQuery dataQueryTest;
    dataQueryTest.KeyPrefix(prefixTest);
    std::string keysString = "testString_";
    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), keysString + std::to_string(i));
    }

    std::map<std::string, Status> result;
    syncCallback->DeviceSyncCompleted(result);

    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
    singleKvStoreStub->UnRegisterSyncCallback();
}

void GetResultSetFuzz1Test(const uint8_t *data, size_t size)
{
    std::string prefixTest(data, data + size);
    std::string keysString = "testString_";
    int position = static_cast<int>(size);
    std::shared_ptr<KvStoreResultSet> kvResultSet;
    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), keysString + std::to_string(i));
    }
    auto status = singleKvStoreStub->GetResultSet(prefixTest, kvResultSet);
    if (status != Status::SUCCESS || kvResultSet == nullptr) {
        return;
    }
    kvResultSet->Move(position);
    kvResultSet->MoveToPosition(position);
    Entry entry;
    kvResultSet->GetEntry(entry);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
}

void GetResultSetFuzz2Test(const uint8_t *data, size_t size)
{
    std::string prefixTest(data, data + size);
    DataQuery dataQueryTest;
    dataQueryTest.KeyPrefix(prefixTest);
    std::string keysString = "testString_";
    std::shared_ptr<KvStoreResultSet> kvResultSet;
    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), keysString + std::to_string(i));
    }
    singleKvStoreStub->GetResultSet(dataQueryTest, kvResultSet);
    singleKvStoreStub->CloseResultSet(kvResultSet);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
}

void GetResultSetFuzz3Test(const uint8_t *data, size_t size)
{
    std::string prefixTest(data, data + size);
    DataQuery dataQueryTest;
    dataQueryTest.KeyPrefix(prefixTest);
    std::string keysString = "testString_";
    std::shared_ptr<KvStoreResultSet> kvResultSet;
    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), keysString + std::to_string(i));
    }
    singleKvStoreStub->GetResultSet(dataQueryTest, kvResultSet);
    auto status = singleKvStoreStub->GetResultSet(prefixTest, kvResultSet);
    if (status != Status::SUCCESS || kvResultSet == nullptr) {
        return;
    }
    int cnt = kvResultSet->GetCount();
    if (static_cast<int>(size) != cnt) {
        return;
    }
    kvResultSet->GetPosition();
    kvResultSet->IsBeforeFirst();
    kvResultSet->IsFirst();
    kvResultSet->IsBeforeFirst();
    kvResultSet->IsFirst();
    kvResultSet->MoveToPrevious();
    while (kvResultSet->MoveToNext()) {
        Entry entry;
        kvResultSet->GetEntry(entry);
    }
    Entry entry;
    kvResultSet->GetEntry(entry);
    kvResultSet->IsLast();
    kvResultSet->IsAfterLast();
    kvResultSet->MoveToNext();
    kvResultSet->IsLast();
    kvResultSet->IsAfterLast();
    kvResultSet->MoveToFirst();
    kvResultSet->GetEntry(entry);
    kvResultSet->MoveToLast();
    kvResultSet->IsAfterLast();
    kvResultSet->Move(1);
    kvResultSet->IsLast();
    kvResultSet->GetEntry(entry);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
}

void GetCountFuzz1Test(const uint8_t *data, size_t size)
{
    int count;
    std::string prefixTest(data, data + size);
    DataQuery query11;
    query11.KeyPrefix(prefixTest);
    std::string keysString = "testString_";
    size_t count = 10;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), keysString + std::to_string(i));
    }
    singleKvStoreStub->GetCount(query11, count);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
}

void GetCountFuzz2Test(const uint8_t *data, size_t size)
{
    int count;
    size_t count = 10;
    std::vector<std::string> keysString;
    std::string prefixTest(data, data + size);
    for (size_t i = 0; i < count; i++) {
        keysString.push_back(prefixTest);
    }
    DataQuery query11;
    query11.InKeys(keysString);
    std::string dataKey = "testString_";
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + dataKey + std::to_string(i), dataKey + std::to_string(i));
    }
    singleKvStoreStub->GetCount(query11, count);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + dataKey + std::to_string(i));
    }
}

void RemoveDeviceDataFuzzTest(const uint8_t *data, size_t size)
{
    size_t count = 10;
    std::string deviceId01(data, data + size);
    std::vector<Entry> input;
    auto cmp = [](const Key &entry, const Key &sentry) { return entry.Data() < sentry.Data(); };
    std::map<Key, Value, decltype(cmp)> dictionary(cmp);
    for (size_t i = 0; i < count; ++i) {
        Entry entry;
        entry.keyTest = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        dictionary[entry.keyTest] = entry.value;
        input.push_back(entry);
    }
    singleKvStoreStub->PutBatch(input);
    singleKvStoreStub->RemoveDeviceData(deviceId01);
    singleKvStoreStub->RemoveDeviceData("");

    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(std::to_string(i).append("_k"));
    }
}

void GetSecurityLevelFuzzTest(const uint8_t *data, size_t size)
{
    size_t count = 10;
    std::vector<std::string> keysString;
    std::string prefixTest(data, data + size);
    for (size_t i = 0; i < count; i++) {
        keysString.push_back(prefixTest);
    }
    std::string dataKey = "testString_";
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + dataKey + std::to_string(i), dataKey + std::to_string(i));
    }
    SecurityLevel securityLevelTest;
    singleKvStoreStub->GetSecurityLevel(securityLevelTest);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + dataKey + std::to_string(i));
    }
}

void SyncFuzz1Test(const uint8_t *data, size_t size)
{
    size_t count = 10;
    std::string dataKey = "testString_";
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(dataKey + std::to_string(i), dataKey + std::to_string(i));
    }
    std::string deviceId01(data, data + size);
    std::vector<std::string> deviceIdItem = { deviceId01 };
    uint32_t allowedDelay = 200;
    singleKvStoreStub->Sync(deviceIdItem, SyncMode::PUSH, allowedDelay);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(dataKey + std::to_string(i));
    }
}

void SyncFuzz2Test(const uint8_t *data, size_t size)
{
    size_t count = 10;
    std::string dataKey = "testString_";
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(dataKey + std::to_string(i), dataKey + std::to_string(i));
    }
    std::string deviceId01(data, data + size);
    std::vector<std::string> deviceIdItem = { deviceId01 };
    DataQuery dataQueryTest;
    dataQueryTest.KeyPrefix("name");
    singleKvStoreStub->Sync(deviceIdItem, SyncMode::PULL, dataQueryTest, nullptr);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(dataKey + std::to_string(i));
    }
}

void SyncParamFuzzTest(const uint8_t *data, size_t size)
{
    size_t count = 10;
    std::vector<std::string> keysString;
    std::string prefixTest(data, data + size);
    for (size_t i = 0; i < count; i++) {
        keysString.push_back(prefixTest);
    }
    std::string dataKey = "testString_";
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + dataKey + std::to_string(i), dataKey + std::to_string(i));
    }

    KvSyncParam syncParam { 500 };
    singleKvStoreStub->SetSyncParam(syncParam);

    KvSyncParam syncParam;
    singleKvStoreStub->GetSyncParam(syncParam);

    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + dataKey + std::to_string(i));
    }
}

void SetCapabilityRangeFuzzTest(const uint8_t *data, size_t size)
{
    std::string label(data, data + size);
    std::vector<std::string> local = { label + "_local1", label + "_local2" };
    std::vector<std::string> remote = { label + "_remote1", label + "_remote2" };
    singleKvStoreStub->SetCapabilityRange(local, remote);
}

void SetCapabilityEnabledFuzzTest(const uint8_t *data, size_t size)
{
    size_t count = 10;
    std::vector<std::string> keysString;
    std::string prefixTest(data, data + size);
    for (size_t i = 0; i < count; i++) {
        keysString.push_back(prefixTest);
    }
    std::string dataKey = "testString_";
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + dataKey + std::to_string(i), dataKey + std::to_string(i));
    }

    singleKvStoreStub->SetCapabilityEnable(true);
    singleKvStoreStub->SetCapabilityEnable(false);

    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + dataKey + std::to_string(i));
    }
}

void SubscribeWithQueryFuzzTest(const uint8_t *data, size_t size)
{
    std::string deviceId01(data, data + size);
    std::vector<std::string> deviceIdItem = { deviceId01 + "_1", deviceId01 + "_2" };
    DataQuery dataQueryTest;
    dataQueryTest.KeyPrefix("name");
    singleKvStoreStub->SubscribeWithQuery(deviceIdItem, dataQueryTest);
    singleKvStoreStub->UnsubscribeWithQuery(deviceIdItem, dataQueryTest);
}

void UnSubscribeWithQueryFuzzTest(const uint8_t *data, size_t size)
{
    std::string deviceId01(data, data + size);
    std::vector<std::string> deviceIdItem = { deviceId01 + "_1", deviceId01 + "_2" };
    DataQuery dataQueryTest;
    dataQueryTest.KeyPrefix("name");
    singleKvStoreStub->UnsubscribeWithQuery(deviceIdItem, dataQueryTest);
}

void AsyncGetFuzzTest(const uint8_t *data, size_t size)
{
    std::string strKey(data, data + size);
    std::string strValue(data, data + size);
    Key keyTest = { strKey };
    Value value = { strValue };
    singleKvStoreStub->Put(keyTest, value);
    Value out;
    std::function<void(Status, Value &&)> call = [](Status status, Value &&value) {};
    std::string networkIdTest(data, data + size);
    singleKvStoreStub->Get(keyTest, networkIdTest, call);
    singleKvStoreStub->Delete(keyTest);
}

void AsyncGetEntriesFuzzTest(const uint8_t *data, size_t size)
{
    std::string prefixTest(data, data + size);
    std::string keysString = "testString_";
    size_t count = 10;
    std::vector<Entry> result;
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Put(prefixTest + keysString + std::to_string(i), { keysString + std::to_string(i) });
    }
    std::function<void(Status, std::vector<Entry> &&)> call = [](Status status, std::vector<Entry> &&entry) {};
    std::string networkIdTest(data, data + size);
    singleKvStoreStub->GetEntries(prefixTest, networkIdTest, call);
    for (size_t i = 0; i < count; i++) {
        singleKvStoreStub->Delete(prefixTest + keysString + std::to_string(i));
    }
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SetUpTestCase();
    OHOS::PutFuzzTest(data, size);
    OHOS::PutBatchFuzzTest(data, size);
    OHOS::GetFuzzTest(data, size);
    OHOS::GetEntriesFuzz1Test(data, size);
    OHOS::GetEntriesFuzz2Test(data, size);
    OHOS::GetResultSetFuzz1Test(data, size);
    OHOS::GetResultSetFuzz2Test(data, size);
    OHOS::GetResultSetFuzz3Test(data, size);
    OHOS::GetCountFuzz1Test(data, size);
    OHOS::GetCountFuzz2Test(data, size);
    OHOS::SyncFuzz1Test(data, size);
    OHOS::SyncFuzz2Test(data, size);
    OHOS::SubscribeKvStoreFuzzTest(data, size);
    OHOS::RemoveDeviceDataFuzzTest(data, size);
    OHOS::GetSecurityLevelFuzzTest(data, size);
    OHOS::SyncCallbackFuzzTest(data, size);
    OHOS::SyncParamFuzzTest(data, size);
    OHOS::SetCapabilityEnabledFuzzTest(data, size);
    OHOS::SetCapabilityRangeFuzzTest(data, size);
    OHOS::SubscribeWithQueryFuzzTest(data, size);
    OHOS::UnSubscribeWithQueryFuzzTest(data, size);
    OHOS::AsyncGetFuzzTest(data, size);
    OHOS::AsyncGetEntriesFuzzTest(data, size);
    OHOS::TearDown();
    return 0;
}