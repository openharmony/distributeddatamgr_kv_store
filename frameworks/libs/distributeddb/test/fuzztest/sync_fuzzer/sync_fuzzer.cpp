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
#include "virtual_communicator_aggregator.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"
#include "log_print.h"

class KvStoreSyncFuzzer {
    /* Keep C++ file names the same as the class name. */
};

namespace OHOS {
using namespace DistributedDB;
using namespace DistributedDBTest;

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

std::vector<Entry> CreateEntries(const uint8_t* data, size_t size, std::vector<Key> keys)
{
    std::vector<Entry> entries;
    // key'length is less than 1024.
    auto count = static_cast<int>(std::min(size, size_t(1024)));
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = std::vector<uint8_t>(data, data + i);
        keys.push_back(entry.key);
        entry.value = std::vector<uint8_t>(data, data + size);
        entries.push_back(entry);
    }
    return entries;
}

void NormalSyncPush(const uint8_t* data, size_t size, bool isWithQuery = false)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    std::vector<Key> keys;
    std::vector<Entry> tmp = CreateEntries(data, size, keys);
    g_kvDelegatePtr->PutBatch(tmp);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    if (isWithQuery) {
        int len = std::min(size, size_t(100));
        Key tmpKey = std::vector<uint8_t>(data, data + len);
        Query query = Query::Select().PrefixKey(tmpKey);
        DistributedDBToolsTest::SyncTestWithQuery(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result, query);
    } else {
        DistributedDBToolsTest::SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_ONLY, result);
    }
    TearDownTestCase();
}

void NormalSyncPull(const uint8_t* data, size_t size, bool isWithQuery = false)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    std::vector<Key> keys;
    std::vector<Entry> tmp = CreateEntries(data, size, keys);
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
        int len = std::min(size, size_t(100));
        Key tmpKey = std::vector<uint8_t>(data, data + len);
        Query query = Query::Select().PrefixKey(tmpKey);
        DistributedDBToolsTest::SyncTestWithQuery(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result, query);
    } else {
        DistributedDBToolsTest::SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PULL_ONLY, result);
    }
    TearDownTestCase();
}

void NormalSyncPushAndPull(const uint8_t* data, size_t size, bool isWithQuery = false)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    std::vector<Key> keys;
    std::vector<Entry> tmp = CreateEntries(data, size, keys);
    g_kvDelegatePtr->PutBatch(tmp);
    std::vector<std::string> devices;
    devices.push_back(g_deviceB->GetDeviceId());
    std::map<std::string, DBStatus> result;
    if (isWithQuery) {
        int len = std::min(size, size_t(100));
        Key tmpKey = std::vector<uint8_t>(data, data + len);
        Query query = Query::Select().PrefixKey(tmpKey);
        DistributedDBToolsTest::SyncTestWithQuery(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result, query);
    } else {
        DistributedDBToolsTest::SyncTest(g_kvDelegatePtr, devices, SYNC_MODE_PUSH_PULL, result);
    }
    TearDownTestCase();
}

void SubscribeOperation(const uint8_t* data, size_t size)
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
    int count = std::min(size, size_t(3));
    for (int i = 0; i < count; i++) {
        Key tmpKey = std::vector<uint8_t>(data + i, data + i + 1);
        keys.insert(tmpKey);
    }
    Query query = Query::Select().InKeys(keys);
    g_kvDelegatePtr->SubscribeRemoteQuery(devices, nullptr, query, true);
    g_kvDelegatePtr->UnSubscribeRemoteQuery(devices, nullptr, query2, true);
    g_kvDelegatePtr->UnSubscribeRemoteQuery(devices, nullptr, query, true);
    TearDownTestCase();
}

void OtherOperation(const uint8_t* data, size_t size)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    int len = std::min(size, size_t(10));
    std::string tmpIdentifier(data, data + len);
    std::vector<std::string> targets;
    int j = 0;
    int count = std::min(size, size_t(4));
    for (int i = 0; i < count; i++) {
        std::string tmpStr(data + j, data + j + 1);
        j = j + 10; // target size is 10
        targets.push_back(tmpStr);
        if ((1 + j) >= static_cast<int>(size)) {
            break;
        }
    }
    g_kvDelegatePtr->SetEqualIdentifier(tmpIdentifier, targets);
    TearDownTestCase();
}

void PragmaOperation(const uint8_t* data, size_t size)
{
    SetUpTestcase();
    if (g_kvDelegatePtr == nullptr) {
        return;
    }
    bool autoSync = (size == 0) ? true : data[0];
    PragmaData praData = static_cast<PragmaData>(&autoSync);
    g_kvDelegatePtr->Pragma(AUTO_SYNC, praData);

    PragmaDeviceIdentifier param1;
    int len = std::min(size, size_t(100));
    std::string tmpStr(data, data + len);
    param1.deviceID = tmpStr;
    PragmaData input = static_cast<void *>(&param1);
    g_kvDelegatePtr->Pragma(GET_IDENTIFIER_OF_DEVICE, input);

    PragmaEntryDeviceIdentifier param2;
    len = std::min(size, size_t(10)); // use min 10
    param2.key.assign(data, data + len);
    param2.origDevice = false;
    input = static_cast<void *>(&param2);
    g_kvDelegatePtr->Pragma(GET_DEVICE_IDENTIFIER_OF_ENTRY, input);

    int size2;
    input = static_cast<PragmaData>(&size2);
    g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_SIZE, input);

    int limit = 1; // init 1
    if (size > sizeof(int)) {
        auto *r = reinterpret_cast<const int *>(data);
        limit = *r;
    }
    input = static_cast<PragmaData>(&limit);
    g_kvDelegatePtr->Pragma(SET_QUEUED_SYNC_LIMIT, input);

    int limit2 = 0;
    input = static_cast<PragmaData>(&limit2);
    g_kvDelegatePtr->Pragma(GET_QUEUED_SYNC_LIMIT, input);
    TearDownTestCase();
}

void FuzzSync(const uint8_t* data, size_t size)
{
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    g_mgr.SetKvStoreConfig(config);
    InitEnv();
    NormalSyncPush(data, size);
    NormalSyncPull(data, size);
    NormalSyncPushAndPull(data, size);
    NormalSyncPush(data, size, true);
    NormalSyncPull(data, size, true);
    NormalSyncPushAndPull(data, size, true);
    SubscribeOperation(data, size);
    OtherOperation(data, size);
    PragmaOperation(data, size);
    FinalizeEnv();
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::FuzzSync(data, size);
    return 0;
}

