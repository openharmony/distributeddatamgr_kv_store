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
#include "fuzzer/FuzzedDataProvider.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

namespace OHOS {
static constexpr const int HUNDRED = 100;
static constexpr const int MOD = 1024; // 1024 is mod
static constexpr const int PASSWDLEN = 20; // 20 is passwdlen
std::vector<Entry> CreateEntries(FuzzedDataProvider &fdp, std::vector<Key>& keys)
{
    std::vector<Entry> entries;
    // key'length is less than 1024.
    auto count =  fdp.ConsumeIntegralInRange<int>(0, size_t(MOD));
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = fdp.ConsumeBytes<uint8_t>(i);
        entry.value = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, MOD));
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

void EncryptOperation(FuzzedDataProvider &fdp, KvStoreDelegate *kvDelegatePtr)
{
    if (kvDelegatePtr == nullptr) {
        return;
    }
    CipherPassword passwd;
    size_t data_size = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t* val = static_cast<uint8_t*>(new uint8_t[data_size]);
    fdp.ConsumeData(val, data_size);
    passwd.SetValue(val, data_size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;
    kvDelegatePtr->Rekey(passwd);
    bool autoSync = fdp.ConsumeBool();
    PragmaData praData = static_cast<PragmaData>(&autoSync);
    kvDelegatePtr->Pragma(AUTO_SYNC, praData);
}

void CombineTest(FuzzedDataProvider &fdp, KvStoreDelegate *kvDelegatePtr)
{
    if (kvDelegatePtr == nullptr) {
        return;
    }
    kvDelegatePtr->GetStoreId();
    kvDelegatePtr->Rollback();
    kvDelegatePtr->Commit();
    auto type = static_cast<ResolutionPolicyType>(fdp.ConsumeIntegral<int>());
    kvDelegatePtr->SetConflictResolutionPolicy(type, nullptr);
}

void MultiCombineFuzzer(FuzzedDataProvider &fdp, KvStoreDelegate::Option &option)
{
    KvStoreConfig config;
    KvStoreDelegate *kvDelegatePtr = PrepareKvStore(config, option);
    std::shared_ptr<KvStoreObserverTest> observer = std::make_shared<KvStoreObserverTest>();
    if ((kvDelegatePtr == nullptr) || (observer == nullptr)) {
        observer = nullptr;
        return;
    }

    kvDelegatePtr->RegisterObserver(observer);
    Key key = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, MOD));
    Value value = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, HUNDRED));
    kvDelegatePtr->Put(key, value);
    kvDelegatePtr->StartTransaction();
    KvStoreSnapshotDelegate *kvStoreSnapshotPtr = nullptr;
    kvDelegatePtr->GetKvStoreSnapshot(nullptr,
        [&kvStoreSnapshotPtr](DBStatus status, KvStoreSnapshotDelegate* kvStoreSnapshot) {
            kvStoreSnapshotPtr = std::move(kvStoreSnapshot);
        });
    if (kvStoreSnapshotPtr == nullptr) {
        kvDelegatePtr->UnRegisterObserver(observer);
        return;
    }
    auto valueCallback = [&value] (DBStatus status, const Value &getValue) {
        value = getValue;
    };

    kvStoreSnapshotPtr->Get(key, valueCallback);
    kvDelegatePtr->Delete(key);
    kvStoreSnapshotPtr->Get(key, valueCallback);
    std::vector<Key> keys;
    kvDelegatePtr->PutBatch(CreateEntries(fdp, keys));
    Key keyPrefix = fdp.ConsumeBytes<uint8_t>(1);
    kvStoreSnapshotPtr->GetEntries(keyPrefix, [](DBStatus status, const std::vector<Entry> &entries) {
        (void) entries.size();
    });
    if (option.isEncryptedDb) {
        EncryptOperation(fdp, kvDelegatePtr);
    }
    kvDelegatePtr->DeleteBatch(keys);
    kvDelegatePtr->Clear();
    CombineTest(fdp, kvDelegatePtr);
    kvDelegatePtr->UnRegisterObserver(observer);
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
    FuzzedDataProvider fdp(data, size);
    OHOS::MultiCombineFuzzer(fdp, option);
    option = {true, false, false, CipherType::DEFAULT, passwd};
    OHOS::MultiCombineFuzzer(fdp, option);
    return 0;
}
