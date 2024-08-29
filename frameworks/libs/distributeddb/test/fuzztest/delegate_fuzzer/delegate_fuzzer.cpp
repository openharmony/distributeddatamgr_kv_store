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

#include "delegate_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

namespace OHOS {
static constexpr const int MOD = 1024; // 1024 is mod
static constexpr const int PASSWDLEN = 20; // 20 is passwdlen
std::vector<Entry> CreateEntries(const uint8_t *data, size_t size, std::vector<Key>& keys)
{
    std::vector<Entry> entries;
    // key'length is less than 1024.
    auto count = static_cast<int>(std::min(size, size_t(MOD)));
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = std::vector<uint8_t>(data, data + 1);
        entry.value = std::vector<uint8_t>(data, data + size);
        keys.push_back(entry.key);
        entries.push_back(entry);
    }
    return entries;
}

KvStoreDelegateManager g_kvManger = KvStoreDelegateManager("APP_ID", "USER_ID");
KvStoreDelegate *PrepareKvStore(KvStoreConfig &config, KvStoreDelegate::Option &option)
{
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    g_kvManger.SetKvStoreConfig(config);
    KvStoreDelegate *kvDelegatePtr = nullptr;
    g_kvManger.GetKvStore("distributed_delegate_test", option,
        [&kvDelegatePtr](DBStatus status, KvStoreDelegate* kvDelegate) {
            if (status == DBStatus::OK) {
                kvDelegatePtr = kvDelegate;
            }
        });
    return kvDelegatePtr;
}

void EncryptOperation(const uint8_t *data, size_t size, KvStoreDelegate *kvDelegatePtr)
{
    if (kvDelegatePtr == nullptr) {
        return;
    }
    CipherPassword passwd;
    int len = static_cast<int>(std::min(size, size_t(PASSWDLEN)));
    passwd.SetValue(data, len);
    kvDelegatePtr->Rekey(passwd);
    bool autoSync = (size == 0) ? true : data[0];
    PragmaData praData = static_cast<PragmaData>(&autoSync);
    kvDelegatePtr->Pragma(AUTO_SYNC, praData);
}

void CombineTest(const uint8_t *data, KvStoreDelegate *kvDelegatePtr)
{
    if (kvDelegatePtr == nullptr) {
        return;
    }
    kvDelegatePtr->GetStoreId();
    kvDelegatePtr->Rollback();
    kvDelegatePtr->Commit();
    auto type = static_cast<ResolutionPolicyType>(data[0]);
    kvDelegatePtr->SetConflictResolutionPolicy(type, nullptr);
}

void MultiCombineFuzzer(const uint8_t *data, size_t size, KvStoreDelegate::Option &option)
{
    KvStoreConfig config;
    KvStoreDelegate *kvDelegatePtr = PrepareKvStore(config, option);
    KvStoreObserverTest *observer = new (std::nothrow) KvStoreObserverTest;
    if ((kvDelegatePtr == nullptr) || (observer == nullptr)) {
        delete observer;
        observer = nullptr;
        return;
    }

    kvDelegatePtr->RegisterObserver(observer);
    Key key = std::vector<uint8_t>(data, data + (size % 1024)); /* 1024 is max */
    Value value = std::vector<uint8_t>(data, data + size);
    kvDelegatePtr->Put(key, value);
    kvDelegatePtr->StartTransaction();
    KvStoreSnapshotDelegate *kvStoreSnapshotPtr = nullptr;
    kvDelegatePtr->GetKvStoreSnapshot(nullptr,
        [&kvStoreSnapshotPtr](DBStatus status, KvStoreSnapshotDelegate* kvStoreSnapshot) {
            kvStoreSnapshotPtr = std::move(kvStoreSnapshot);
        });
    if (kvStoreSnapshotPtr == nullptr) {
        kvDelegatePtr->UnRegisterObserver(observer);
        delete observer;
        return;
    }
    auto valueCallback = [&value] (DBStatus status, const Value &getValue) {
        value = getValue;
    };

    kvStoreSnapshotPtr->Get(key, valueCallback);
    kvDelegatePtr->Delete(key);
    kvStoreSnapshotPtr->Get(key, valueCallback);
    std::vector<Key> keys;
    kvDelegatePtr->PutBatch(CreateEntries(data, size, keys));
    Key keyPrefix = std::vector<uint8_t>(data, data + 1);
    kvStoreSnapshotPtr->GetEntries(keyPrefix, [](DBStatus status, const std::vector<Entry> &entries) {
        (void) entries.size();
    });
    if (option.isEncryptedDb) {
        EncryptOperation(data, size, kvDelegatePtr);
    }
    kvDelegatePtr->DeleteBatch(keys);
    kvDelegatePtr->Clear();
    CombineTest(data, kvDelegatePtr);
    kvDelegatePtr->UnRegisterObserver(observer);
    delete observer;
    observer = nullptr;
    kvDelegatePtr->ReleaseKvStoreSnapshot(kvStoreSnapshotPtr);
    g_kvManger.CloseKvStore(kvDelegatePtr);
    g_kvManger.DeleteKvStore("distributed_delegate_test");
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    CipherPassword passwd;
    KvStoreDelegate::Option option = {true, true, false, CipherType::DEFAULT, passwd};
    OHOS::MultiCombineFuzzer(data, size, option);
    option = {true, false, false, CipherType::DEFAULT, passwd};
    OHOS::MultiCombineFuzzer(data, size, option);
    return 0;
}
