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
#include "devicekvstorecapenabled_fuzzer.h"

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

void SetCapabilityEnabledFuzz(FuzzedDataProvider &provider)
{
    InitKvstoreFuzzer fuzzer;
    fuzzer.SetUpTestCase(appId, storeId, KvStoreType::DEVICE_COLLABORATION, deviceKvStore_);
    size_t sum = 10;
    std::vector<std::string> keys;
    std::string prefix = provider.ConsumeRandomLengthString();
    for (size_t i = 0; i < sum; i++) {
        keys.push_back(prefix);
    }
    std::string skey = "test_";
    for (size_t i = 0; i < sum; i++) {
        deviceKvStore_->Put(prefix + skey + std::to_string(i), skey + std::to_string(i));
    }

    deviceKvStore_->SetCapabilityEnabled(true);
    deviceKvStore_->SetCapabilityEnabled(false);

    for (size_t i = 0; i < sum; i++) {
        deviceKvStore_->Delete(prefix + skey + std::to_string(i));
    }
    fuzzer.TearDown(appId, storeId);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider provider(data, size);
    OHOS::SetCapabilityEnabledFuzz(provider);
    return 0;
}