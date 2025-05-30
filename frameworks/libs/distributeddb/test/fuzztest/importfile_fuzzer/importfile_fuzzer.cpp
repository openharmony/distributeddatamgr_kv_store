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

#include "importfile_fuzzer.h"
#include <fstream>
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_test.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "platform_specific.h"
#include "process_communicator_test_stub.h"

using namespace DistributedDB;
using namespace DistributedDBTest;
using namespace std;

static KvStoreConfig g_config;

namespace OHOS {
static constexpr const int PASSWDLEN = 20; // 20 is passwdlen
void SingerVerImport(FuzzedDataProvider &provider, const std::string &importFile)
{
    static auto kvManager = KvStoreDelegateManager("APP_ID", "USER_ID");
    kvManager.SetKvStoreConfig(g_config);
    kvManager.SetProcessLabel("FUZZ", "DISTRIBUTEDDB");
    kvManager.SetProcessCommunicator(std::make_shared<ProcessCommunicatorTestStub>());
    CipherPassword passwd;
    size_t size = provider.ConsumeIntegralInRange<size_t>(0, PASSWDLEN);
    uint8_t* val = static_cast<uint8_t*>(new uint8_t[size]);
    provider.ConsumeData(val, size);
    passwd.SetValue(val, size);
    delete[] static_cast<uint8_t*>(val);
    val = nullptr;
    KvStoreNbDelegate::Option option = {true, false, true, CipherType::DEFAULT, passwd};

    KvStoreNbDelegate *kvNbDelegatePtr = nullptr;
    kvManager.GetKvStore("distributed_import_single", option,
        [&kvNbDelegatePtr](DBStatus status, KvStoreNbDelegate* kvNbDelegate) {
            if (status == DBStatus::OK) {
                kvNbDelegatePtr = kvNbDelegate;
            }
        });
    if (kvNbDelegatePtr == nullptr) {
        return;
    }

    kvNbDelegatePtr->Import(importFile, passwd);
    kvManager.CloseKvStore(kvNbDelegatePtr);
    kvManager.DeleteKvStore("distributed_import_single");
}

bool MakeImportFile(FuzzedDataProvider &provider, const std::string &realPath)
{
    std::ofstream ofs(realPath, std::ofstream::out);
    if (!ofs.is_open()) {
        LOGE("the file open failed");
        return false;
    }
    std::string rawString = provider.ConsumeRandomLengthString();
    ofs.write(rawString.c_str(), rawString.length());
    return true;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string dataDir;
    std::string testDataDir = provider.ConsumeRandomLengthString();
    DistributedDBToolsTest::TestDirInit(dataDir);
    g_config.dataDir = dataDir;
    std::string path = dataDir + "/fuzz" + testDataDir;
    std::string realPath;
    OS::GetRealPath(path, realPath);
    if (OHOS::MakeImportFile(provider, realPath)) {
        OHOS::SingerVerImport(provider, realPath);
    }

    DistributedDBToolsTest::RemoveTestDbFiles(dataDir);
    return 0;
}

