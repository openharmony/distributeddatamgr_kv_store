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
#include "fuzzer_init.h"

using namespace OHOS;
using namespace OHOS::DistributedKv;

namespace OHOS {
void InitKvstoreFuzzer::SetUpTestCase(AppId appId, StoreId storeId, KvStoreType type,
    std::shared_ptr<SingleKvStore> &singleKvStore)
{
    options_.createIfMissing = true;
    options_.encrypt = false;
    options_.autoSync = true;
    options_.securityLevel = S1;
    options_.kvStoreType = type;
    options_.area = EL1;
    options_.baseDir = std::string("/data/service/el1/public/database/") + appId.appId;
    mkdir(options_.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    /* [create and] open and initialize kvstore instance. */
    manager_.GetSingleKvStore(options_, appId, storeId, singleKvStore);
}

void InitKvstoreFuzzer::TearDown(AppId appId, StoreId storeId)
{
    manager_.CloseKvStore(appId, storeId);
    manager_.DeleteKvStore(appId, storeId, options_.baseDir);
    (void)remove("/data/service/el1/public/database/devicekvstorefuzzertest/key");
    (void)remove("/data/service/el1/public/database/devicekvstorefuzzertest/kvdb");
    (void)remove("/data/service/el1/public/database/devicekvstorefuzzertest");
}
} // namespace OHOS
