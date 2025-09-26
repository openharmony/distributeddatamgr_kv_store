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
#include "devicekvstoresyncparam_fuzzer.h"

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

void SyncParamFuzz(FuzzedDataProvider &provider)
{
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

    KvSyncParam syncParam { 500 };
    deviceKvStore_->SetSyncParam(syncParam);

    KvSyncParam syncParamRet;
    deviceKvStore_->GetSyncParam(syncParamRet);

    for (size_t i = 0; i < sum; i++) {
        deviceKvStore_->Delete(prefix + skey + std::to_string(i));
    }
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider provider(data, size);
    OHOS::SetUpTestCase();
    OHOS::SyncParamFuzz(provider);
    OHOS::TearDown();
    return 0;
}