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
#include "devicekvstoregetcount_fuzzer.h"

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
static std::shared_ptr<SingleKvStore> deviceKvStore_ = nullptr;
AppId appId = { "devicekvstorefuzzertest" };
StoreId storeId = { "fuzzer_device" };

void GetCountFuzz(FuzzedDataProvider &provider)
{
    InitKvstoreFuzzer fuzzer;
    fuzzer.SetUpTestCase(appId, storeId, KvStoreType::DEVICE_COLLABORATION, deviceKvStore_);
    int count;
    std::string prefix = provider.ConsumeRandomLengthString();
    DataQuery query;
    query.KeyPrefix(prefix);
    std::string keys = "test_";
    size_t sum = 10;
    for (size_t i = 0; i < sum; i++) {
        deviceKvStore_->Put(prefix + keys + std::to_string(i), keys + std::to_string(i));
    }
    deviceKvStore_->GetCount(query, count);
    for (size_t i = 0; i < sum; i++) {
        deviceKvStore_->Delete(prefix + keys + std::to_string(i));
    }
    fuzzer.TearDown(appId, storeId);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider provider(data, size);
    OHOS::GetCountFuzz(provider);
    return 0;
}