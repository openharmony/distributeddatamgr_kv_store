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

#include "fuzzer/FuzzedDataProvider.h"
#include "rekey_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

namespace OHOS {
static auto g_kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
static constexpr const int MOD = 1024; // 1024 is mod

static constexpr const int PASSWDLEN = 20;
std::vector<Entry> CreateEntries(FuzzedDataProvider &provider)
{
    std::vector<Entry> entries;

    auto count =provider.ConsumeIntegralInRange<int32_t>(0, MOD);
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = provider.ConsumeBytes<uint8_t>(i);
        entry.value = provider.ConsumeBytes<uint8_t>(provider.ConsumeIntegral<int>());
        entries.push_back(entry);
    }
    return entries;
}

void SingerVerReKey(FuzzedDataProvider &provider)
{
    CipherPassword passwd;
    // div 2 -> half
    size_t size = provider.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t* val = static_cast<uint8_t*>(new uint8_t[size]);
    provider.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;

    KvStoreNbDelegate::Option nbOption = {true, false, true, CipherType::DEFAULT, passwd};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

    g_kvManager.GetKvStore("distributed_nb_rekey_test", nbOption,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate * kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });

    if (kvNbDelegatePtr != nullptr) {
        kvNbDelegatePtr->PutBatch(CreateEntries(provider));
        size_t size2 = provider.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
        uint8_t* val2 = static_cast<uint8_t*>(new uint8_t[size2]);
        provider.ConsumeData(val2, size2);
        passwd.SetValue(val2, size2);
        delete[] static_cast<uint8_t*>(val2);
        val2 = nullptr;
        kvNbDelegatePtr->Rekey(passwd);
        g_kvManager.CloseKvStore(kvNbDelegatePtr);
    }
}

void MultiVerVerReKey(FuzzedDataProvider &fdp)
{
    CipherPassword passwd;
    // div 2 -> half
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t* val = static_cast<uint8_t*>(new uint8_t[size]);
    fdp.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;

    KvStoreNbDelegate::Option nbOption = {true, false, true, CipherType::DEFAULT, passwd};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

    g_kvManager.GetKvStore("distributed_rekey_test", nbOption,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate * kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });

    if (kvNbDelegatePtr != nullptr) {
        kvNbDelegatePtr->PutBatch(CreateEntries(fdp));
        CipherPassword passwdTwo;
        size_t size2 = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
        uint8_t* val2 = static_cast<uint8_t*>(new uint8_t[size2]);
        fdp.ConsumeData(val2, size2);
        passwdTwo.SetValue(val2, size2);
        delete[] static_cast<uint8_t*>(val2);
        val2 = nullptr;
        kvNbDelegatePtr->Rekey(passwdTwo);
        g_kvManager.CloseKvStore(kvNbDelegatePtr);
    }
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    OHOS::g_kvManager.SetKvStoreConfig(config);
    FuzzedDataProvider provider(data, size);
    OHOS::SingerVerReKey(provider);
    OHOS::MultiVerVerReKey(provider);
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
    return 0;
}

