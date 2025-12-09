/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "singlekvstoreresultset_fuzzer.h"

#include <string>
#include <sys/stat.h>
#include <vector>

#include "distributed_kv_data_manager.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer_init.h"
#include "store_errno.h"

using namespace OHOS;
using namespace OHOS::DistributedKv;

namespace OHOS {
static std::shared_ptr<SingleKvStore> singleKvStore_ = nullptr;
AppId appId = { "singlekvstorefuzzertest" };
StoreId storeId = { "fuzzer_single" };

void GetResultSetFuzz(FuzzedDataProvider &provider)
{
    InitKvstoreFuzzer fuzzer;
    fuzzer.SetUpTestCase(appId, storeId, KvStoreType::SINGLE_VERSION, singleKvStore_);
    std::string prefix = provider.ConsumeRandomLengthString();
    DataQuery dataQuery;
    dataQuery.KeyPrefix(prefix);
    std::string keys = "test_";
    std::shared_ptr<KvStoreResultSet> resultSet;
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(prefix + keys + std::to_string(i), keys + std::to_string(i));
    }
    singleKvStore_->GetResultSet(dataQuery, resultSet);
    auto status = singleKvStore_->GetResultSet(prefix, resultSet);
    if (status != Status::SUCCESS || resultSet == nullptr) {
        return;
    }
    int cnt = resultSet->GetCount();
    if (cnt != sum) {
        return;
    }
    resultSet->GetPosition();
    resultSet->IsBeforeFirst();
    resultSet->IsFirst();
    resultSet->MoveToPrevious();
    resultSet->IsBeforeFirst();
    resultSet->IsFirst();
    while (resultSet->MoveToNext()) {
        Entry entry;
        resultSet->GetEntry(entry);
    }
    Entry entry;
    resultSet->GetEntry(entry);
    resultSet->IsLast();
    resultSet->IsAfterLast();
    resultSet->MoveToNext();
    resultSet->IsLast();
    resultSet->IsAfterLast();
    resultSet->Move(1);
    resultSet->IsLast();
    resultSet->IsAfterLast();
    resultSet->MoveToFirst();
    resultSet->GetEntry(entry);
    resultSet->MoveToLast();
    resultSet->GetEntry(entry);
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(prefix + keys + std::to_string(i));
    }
    fuzzer.TearDown(appId, storeId);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider provider(data, size);
    OHOS::GetResultSetFuzz(provider);
    return 0;
}