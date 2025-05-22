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

#include "fileoper_fuzzer.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "process_communicator_test_stub.h"

using namespace DistributedDB;
using namespace DistributedDBTest;

namespace OHOS {
static constexpr const int PASSWDLEN = 20; // 20 is passwdlen
static constexpr const int MOD = 1024;
static auto g_kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
std::vector<Entry> CreateEntries(FuzzedDataProvider &fdp)
{
    std::vector<Entry> entries;
    auto count = fdp.ConsumeIntegralInRange<size_t>(0, MOD);
    for (int i = 1; i < count; i++) {
        Entry entry;
        entry.key = fdp.ConsumeBytes<uint8_t>(i);
        entry.value = fdp.ConsumeBytes<uint8_t>(fdp.ConsumeIntegralInRange<size_t>(0, MOD));
        entries.push_back(entry);
    }
    return entries;
}

void SingerVerExportAndImport(FuzzedDataProvider &fdp, const std::string& testDir)
{
    KvStoreNbDelegate::Option nbOption = {true, false, true};
    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;

    g_kvManager.GetKvStore("distributed_file_oper_single", nbOption,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    if (kvNbDelegatePtr == nullptr) {
        return;
    }
    kvNbDelegatePtr->PutBatch(CreateEntries(fdp));

    std::string rawString = fdp.ConsumeRandomLengthString();
    std::string singleExportFileName = testDir + "/" + rawString;

    CipherPassword passwd;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t* val = static_cast<uint8_t*>(new uint8_t[size]);
    fdp.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;
    kvNbDelegatePtr->Export(singleExportFileName, passwd);
    kvNbDelegatePtr->Import(singleExportFileName, passwd);

    g_kvManager.CloseKvStore(kvNbDelegatePtr);
    g_kvManager.DeleteKvStore("distributed_file_oper_single");
}

void MultiVerExportAndImport(FuzzedDataProvider &fdp, const std::string& testDir)
{
    CipherPassword passwd;
    size_t size = fdp.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t* val = static_cast<uint8_t*>(new uint8_t[size]);
    fdp.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;
    KvStoreDelegate::Option option = {true, false, true, CipherType::DEFAULT, passwd};
    KvStoreDelegate *kvDelegatePtr = nullptr;
    g_kvManager.GetKvStore("distributed_file_oper_multi", option,
        [&kvDelegatePtr](DBStatus status, KvStoreDelegate* kvStoreDelegate) {
            if (status == DBStatus::OK) {
                kvDelegatePtr = kvStoreDelegate;
            }
        });
    if (kvDelegatePtr == nullptr) {
        return;
    }
    kvDelegatePtr->PutBatch(CreateEntries(fdp));
    std::string rawString = fdp.ConsumeRandomLengthString();

    std::string  multiExportFileName = testDir + "/" + rawString;
    kvDelegatePtr->Export(multiExportFileName, passwd);
    kvDelegatePtr->Import(multiExportFileName, passwd);
    g_kvManager.CloseKvStore(kvDelegatePtr);
    g_kvManager.DeleteKvStore("distributed_file_oper_multi");
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    KvStoreConfig config;
    DistributedDBToolsTest::TestDirInit(config.dataDir);
    OHOS::g_kvManager.SetKvStoreConfig(config);
    OHOS::g_kvManager.SetProcessLabel("FUZZ", "DISTRIBUTEDDB");
    OHOS::g_kvManager.SetProcessCommunicator(std::make_shared<ProcessCommunicatorTestStub>());
    FuzzedDataProvider provider(data, size);
    OHOS::SingerVerExportAndImport(provider, config.dataDir);
    OHOS::MultiVerExportAndImport(provider, config.dataDir);
    DistributedDBToolsTest::RemoveTestDbFiles(config.dataDir);
    return 0;
}

