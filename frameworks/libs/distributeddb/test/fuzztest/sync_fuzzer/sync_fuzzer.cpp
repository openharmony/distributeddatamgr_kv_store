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

#include "sync_fuzzer.h"

#include "db_constant.h"
#include "db_common.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "virtual_communicator_aggregator.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"
#include "log_print.h"

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;
static constexpr const int MOD = 1024; // 1024 is mod
static constexpr const int HUNDRED = 100;
static constexpr const int TEN = 10;
/* Keep C++ file names the same as the class name. */
class SyncFuzzer {
};

VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
KvVirtualDevice *g_deviceB = nullptr;
KvStoreDelegateManager g_mgr("APP_ID", "USER_ID");
KvStoreNbDelegate* g_kvDelegatePtr = nullptr;
const std::string DEVICE_B = "deviceB";
const std::string STORE_ID = "kv_strore_sync_test";

int InitEnv()
{
    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    if (g_communicatorAggregator == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
    return E_OK;
}

void FinalizeEnv()
{
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void SetUpTestcase()
{
    KvStoreNbDelegate::Option option = {true, false, false};
    g_mgr.GetKvStore("distributed_nb_delegate_test", option,
        [] (DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                g_kvDelegatePtr = kvNbDelegate;
            }
        });
    g_deviceB = new (std::nothrow) KvVirtualDevice(DEVICE_B);
    if (g_deviceB == nullptr) {
        return;
    }
    VirtualSingleVerSyncDBInterface *syncInterfaceB = new (std::nothrow) VirtualSingleVerSyncDBInterface();
    if (syncInterfaceB == nullptr) {
        return;
    }
    g_deviceB->Initialize(g_communicatorAggregator, syncInterfaceB);
}

void TearDownTestCase()
{
    RuntimeContext::GetInstance()->StopTaskPool();
    if (g_kvDelegatePtr != nullptr) {
        g_mgr.CloseKvStore(g_kvDelegatePtr);
        g_kvDelegatePtr = nullptr;
        g_mgr.DeleteKvStore(STORE_ID);
    }
    if (g_deviceB != nullptr) {
        delete g_deviceB;
        g_deviceB = nullptr;
    }
}

std::vector<Entry> CreateEntries(FuzzedDataProvider &fdp, std::vector<Key> keys)
{
    std::vector<Entry> entries;
    // key'length is less than 1024.
    auto count = fdp.ConsumeIntegralInRange<int32_t>(0, MOD);
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = fdp.ConsumeBytes<uint8_t>(1);
        keys.push_back(entry.key);
        size_t size = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
        entry.value = fdp.ConsumeBytes<uint8_t>(size);
        entries.push_back(entry);
    }
    return entries;
}

void NormalSyncPush(FuzzedDataProvider &fdp, bool isWithQuery = false)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    std::vector<Key> keys;
    std::vector<Entry> tmp = CreateEntries(fdp, keys);
    g_kvDelegatePtr->PutBatch(tmp);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    if (isWithQuery) {
        int len = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
        Key tmpKey = fdp.ConsumeBytes<uint8_t>(len);
        Query query = Query::Select().PrefixKey(tmpKey);
        DistributedDBToolsTest::SyncTestWithQuery(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query);
    } else {
        DistributedDBToolsTest::SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    }
    TearDownTestCase();
}

void NormalSyncPull(FuzzedDataProvider &fdp, bool isWithQuery = false)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    std::vector<Key> keys;
    std::vector<Entry> tmp = CreateEntries(fdp, keys);
    g_kvDelegatePtr->PutBatch(tmp);
    int i = 0;
    for (auto &item : tmp) {
        g_deviceB->PutData(item.key, item.value, i++, 0);
    }
    int pushfinishedFlag = 0;
    g_kvDelegatePtr->SetRemotePushFinishedNotify(
        [&pushfinishedFlag](const RemotePushNotifyInfo &info) {
            pushfinishedFlag = 1;
    });

    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    if (isWithQuery) {
        int len = fdp.ConsumeIntegralInRange<size_t>(0, HUNDRED);
        Key tmpKey = fdp.ConsumeBytes<uint8_t>(len);
        Query query = Query::Select().PrefixKey(tmpKey);
        DistributedDBToolsTest::SyncTestWithQuery(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, query);
    } else {
        DistributedDBToolsTest::SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    }
    TearDownTestCase();
}

void NormalSyncPushAndPull(FuzzedDataProvider &fdp, bool isWithQuery = false)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    std::vector<Key> keys;
    std::vector<Entry> tmp = CreateEntries(fdp, keys);
    g_kvDelegatePtr->PutBatch(tmp);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    if (isWithQuery) {
        Key tmpKey = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<int>(0, HUNDRED));
        Query query = Query::Select().PrefixKey(tmpKey);
        DistributedDBToolsTest::SyncTestWithQuery(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result, query);
    } else {
        DistributedDBToolsTest::SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    }
    TearDownTestCase();
}

void SubscribeOperation(FuzzedDataProvider &fdp)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    Query query2 = Query::Select().EqualTo("$.field_name1", 1).Limit(20, 0);
    g_kvDelegatePtr->SubscribeRemoteQuery(devices, nullptr, query2, true);
    std::set<Key> keys;
    int countMax = 3;
    int count = fdp.ConsumeIntegralInRange<int>(0, countMax);
    int byteLen = 1;
    for (int i = 0; i < count; i++) {
        Key tmpKey = fdp.ConsumeBytes<uint8_t>(byteLen);
        keys.insert(tmpKey);
    }
    Query query = Query::Select().InKeys(keys);
    g_kvDelegatePtr->SubscribeRemoteQuery(devices, nullptr, query, true);
    g_kvDelegatePtr->UnSubscribeRemoteQuery(devices, nullptr, query2, true);
    g_kvDelegatePtr->UnSubscribeRemoteQuery(devices, nullptr, query, true);
    TearDownTestCase();
}

void OtherOperation(FuzzedDataProvider &fdp)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    std::string tmpIdentifier = fdp.ConsumeRandomLengthString(TEN);
    std::vector<std::string> targets;
    int countMax = 4;
    int count = fdp.ConsumeIntegralInRange<int>(0, countMax);
    for (int i = 0; i < count; i++) {
        std::string tmpStr = fdp.ConsumeBytesAsString(1);
        targets.push_back(tmpStr);
    }
    g_kvDelegatePtr->SetEqualIdentifier(tmpIdentifier, targets);
    TearDownTestCase();
}

void PragmaOperation(FuzzedDataProvider &fdp)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    bool autoSync = fdp.ConsumeBool();
    PragmaData praData = static_cast<PragmaData>(&autoSync);
    g_kvDelegatePtr->Pragma(AUTO_SYNC, praData);

    PragmaDeviceIdentifier param1;
    std::string tmpStr = fdp.ConsumeRandomLengthString(fdp.ConsumeIntegralInRange<int>(0, HUNDRED));
    param1.deviceID = tmpStr;
    PragmaData input = static_cast<void *>(&param1);
    g_kvDelegatePtr->Pragma(GET_IDENTIFIER_OF_DEVICE, input);

    PragmaEntryDeviceIdentifier param2;
    param2.key = fdp.ConsumeBytes<uint8_t>(TEN);
    param2.origDevice = false;
    input = static_cast<void *>(&param2);
    g_kvDelegatePtr->Pragma(GET_DEVICE_IDENTIFIER_OF_ENTRY, input);

    int size2;
    input = static_cast<PragmaData>(&size2);
    g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_SIZE, input);

    int limit = fdp.ConsumeIntegral<int>();
    input = static_cast<PragmaData>(&limit);
    g_kvDelegatePtr->Pragma(SET_QUEUED_SYNC_LIMIT, input);

    int limit2 = 0;
    input = static_cast<PragmaData>(&limit2);
    g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_LIMIT, input);
    TearDownTestCase();
}

void FuzzSync(FuzzedDataProvider &fdp)
{
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    g_mgr.SetKvStoreConfig(config);
    InitEnv();
    NormalSyncPush(fdp);
    NormalSyncPull(fdp);
    NormalSyncPushAndPull(fdp);
    NormalSyncPush(fdp, true);
    NormalSyncPull(fdp, true);
    NormalSyncPushAndPull(fdp, true);
    SubscribeOperation(fdp);
    OtherOperation(fdp);
    PragmaOperation(fdp);
    FinalizeEnv();
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::FuzzSync(fdp);
    return 0;
}

