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
static constexpr const int MAX_FILE_NAME_LEN = 100;
static constexpr const int MIN_CIPHER_TYPE = static_cast<int>(CipherType::DEFAULT);
static constexpr const int MAX_CIPHER_TYPE = static_cast<int>(CipherType::AES_256_GCM) + 1;
std::vector<Entry> CreateEntries(FuzzedDataProvider &fdp, std::vector<Key>& keys)
{
    std::vector<Entry> entries;
    // key'length is less than 1024.
    auto count = fdp.ConsumeIntegralInRange<int>(0, size_t(MOD));
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = fdp.ConsumeBytes<uint8_t>(i);
        entry.value = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, MOD));
        keys.emplace_back(entry.key);
        entries.emplace_back(entry);
    }
    return entries;
}

KvStoreDelegateManager g_kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
KvStoreDelegate *PrepareKvStore(KvStoreConfig &config, KvStoreDelegate::Option &option)
{
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    g_kvManager.SetKvStoreConfig(config);
    KvStoreDelegate *kvDelegatePtr = nullptr;
    g_kvManager.GetKvStore("distributed_delegate_test", option,
        [&kvDelegatePtr](DBStatus status, KvStoreDelegate* kvDelegate) {
            if (status == DBStatus::OK) {
                kvDelegatePtr = kvDelegate;
            }
        });
    return kvDelegatePtr;
}

void EncryptOperation(FuzzedDataProvider &fdp, std::string &DirPath, KvStoreDelegate *kvDelegatePtr)
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
    int len = fdp.ConsumeIntegralInRange<int>(0, MAX_FILE_NAME_LEN);
    std::string fileName = fdp.ConsumeRandomLengthString(len);
    std::string multiExportFileName = DirPath + "/" + fileName + ".db";
    kvDelegatePtr->Export(multiExportFileName, passwd);
    kvDelegatePtr->Import(multiExportFileName, passwd);
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

CipherPassword GetPassWord(FuzzedDataProvider &fdp)
{
    CipherPassword passwd;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t *val = static_cast<uint8_t*>(new uint8_t[size]);
    fdp.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;
    return passwd;
}

void StoreManagerFuzzer(FuzzedDataProvider &fdp)
{
    std::string dataDir = fdp.ConsumeRandomLengthString();
    KvStoreConfig config = { dataDir };
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    g_kvManager.SetKvStoreConfig(config);
    KvStoreDelegate *kvDelegatePtr = nullptr;
    bool createIfNecessary = fdp.ConsumeBool();
    bool localOnly = fdp.ConsumeBool();
    bool isEncryptedDb = fdp.ConsumeBool();
    CipherType cipher = static_cast<CipherType>(fdp.ConsumeIntegralInRange<int>(MIN_CIPHER_TYPE, MAX_CIPHER_TYPE));
    CipherPassword passwd = GetPassWord(fdp);
    KvStoreDelegate::Option option = {createIfNecessary, localOnly, isEncryptedDb, cipher, passwd};
    const std::string storeId = fdp.ConsumeRandomLengthString();
    g_kvManager.GetKvStore(storeId, option,
        [&kvDelegatePtr](DBStatus status, KvStoreDelegate *kvDelegate) {
            if (status == DBStatus::OK) {
                kvDelegatePtr = kvDelegate;
            }
        });
    g_kvManager.CloseKvStore(kvDelegatePtr);
    g_kvManager.DeleteKvStore(storeId);
}

void MultiCombineFuzzer(FuzzedDataProvider &fdp, KvStoreDelegate::Option &option)
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
    kvDelegatePtr->PutBatch(CreateEntries(fdp, keys));
    Key keyPrefix = fdp.ConsumeBytes<uint8_t>(1);
    kvStoreSnapshotPtr->GetEntries(keyPrefix, [](DBStatus status, const std::vector<Entry> &entries) {
        (void) entries.size();
    });
    if (option.isEncryptedDb) {
        EncryptOperation(fdp, config.dataDir, kvDelegatePtr);
    }
    kvDelegatePtr->DeleteBatch(keys);
    kvDelegatePtr->Clear();
    CombineTest(fdp, kvDelegatePtr);
    kvDelegatePtr->UnRegisterObserver(observer);
    delete observer;
    observer = nullptr;
    kvDelegatePtr->ReleaseKvStoreSnapshot(kvStoreSnapshotPtr);
    g_kvManager.CloseKvStore(kvDelegatePtr);
    g_kvManager.DeleteKvStore("distributed_delegate_test");
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    CipherPassword passwd;
    KvStoreDelegate::Option option = {true, true, false, CipherType::DEFAULT, passwd};
    FuzzedDataProvider fdp(data, size);
    OHOS::StoreManagerFuzzer(fdp);
    OHOS::MultiCombineFuzzer(fdp, option);
    option = {true, false, false, CipherType::DEFAULT, passwd};
    OHOS::MultiCombineFuzzer(fdp, option);
    return 0;
}
