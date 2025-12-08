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
#include "devicekvstoresyncdelay_fuzzer.h"

#include <string>
#include <sys/stat.h>
#include <vector>

#include "distributed_kv_data_manager.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "store_errno.h"

using namespace OHOS;
using namespace OHOS::DistributedKv;

namespace OHOS {
static std::shared_ptr<SingleKvStore> deviceKvStore_ = nullptr;
DistributedKvDataManager manager;
AppId appId = { "devicekvstorefuzzertest" };
StoreId storeId = { "fuzzer_device" };
Options options = {
    .createIfMissing = true,
    .encrypt = false,
    .autoSync = true,
    .securityLevel = S1,
    .kvStoreType = KvStoreType::DEVICE_COLLABORATION
};
void SetUpTestCase(void)
{
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/") + appId.appId;
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    /* [create and] open and initialize kvstore instance. */
    manager.GetSingleKvStore(options, appId, storeId, deviceKvStore_);
}

void TearDown(void)
{
    manager.CloseKvStore(appId, storeId);
    manager.DeleteKvStore(appId, storeId, options.baseDir);
    (void)remove("/data/service/el1/public/database/devicekvstorefuzzertest/key");
    (void)remove("/data/service/el1/public/database/devicekvstorefuzzertest/kvdb");
    (void)remove("/data/service/el1/public/database/devicekvstorefuzzertest");
}

void SyncDelayFuzz(FuzzedDataProvider &provider)
{
    size_t sum = 10;
    std::string skey = "test_";
    for (size_t i = 0; i < sum; i++) {
        deviceKvStore_->Put(skey + std::to_string(i), skey + std::to_string(i));
    }
    std::string deviceId = provider.ConsumeRandomLengthString();
    std::vector<std::string> deviceIds = { deviceId };
    uint32_t allowedDelayMs = 200;
    deviceKvStore_->Sync(deviceIds, SyncMode::PUSH, allowedDelayMs);
    for (size_t i = 0; i < sum; i++) {
        deviceKvStore_->Delete(skey + std::to_string(i));
    }
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider provider(data, size);
    OHOS::SetUpTestCase();
    OHOS::SyncDelayFuzz(provider);
    OHOS::TearDown();
    return 0;
}