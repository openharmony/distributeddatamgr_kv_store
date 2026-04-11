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

#include "singlekvstorerekey_fuzzer.h"

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
constexpr int32_t REKEY_BASIC_TEST = 0;
constexpr int32_t REKEY_MULTIPLE_TIMES_TEST = 1;
constexpr int32_t REKEY_BATCH_DATA_TEST = 2;
constexpr int32_t REKEY_TRANSACTION_TEST = 3;
constexpr int32_t REKEY_UNENCRYPTED_TEST = 4;
constexpr int32_t REKEY_TIMES_COUNT = 3;
constexpr int32_t MAX_TEST_CASE = 5;

static std::shared_ptr<SingleKvStore> singleKvStore_ = nullptr;
AppId appId = { "singlekvstorerekeyfuzzertest" };
StoreId storeId = { "fuzzer_rekey_single" };

static void SetUpEncryptedKvStore(AppId appId, StoreId storeId, std::shared_ptr<SingleKvStore> &singleKvStore)
{
    Options options;
    options.createIfMissing = true;
    options.encrypt = true;
    options.autoSync = true;
    options.securityLevel = S2;
    options.kvStoreType = KvStoreType::SINGLE_VERSION;
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/") + appId.appId;
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    DistributedKvDataManager manager;
    manager.GetSingleKvStore(options, appId, storeId, singleKvStore);
}

static void TearDownEncryptedKvStore(AppId appId, StoreId storeId, const std::string &baseDir)
{
    DistributedKvDataManager manager;
    manager.CloseKvStore(appId, storeId);
    manager.DeleteKvStore(appId, storeId, baseDir);
    (void)remove((baseDir + "/key").c_str());
    (void)remove((baseDir + "/kvdb").c_str());
    (void)remove(baseDir.c_str());
}

void RekeyFuzz(FuzzedDataProvider &provider)
{
    std::string baseDir = std::string("/data/service/el1/public/database/") + appId.appId;

    SetUpEncryptedKvStore(appId, storeId, singleKvStore_);
    if (singleKvStore_ == nullptr) {
        return;
    }

    size_t sum = 10;
    std::string prefix = provider.ConsumeRandomLengthString();
    std::string skey = "test_";
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Put(prefix + skey + std::to_string(i), skey + std::to_string(i));
    }
    singleKvStore_->Rekey();
    for (size_t i = 0; i < sum; i++) {
        singleKvStore_->Delete(prefix + skey + std::to_string(i));
    }

    TearDownEncryptedKvStore(appId, storeId, baseDir);
    singleKvStore_ = nullptr;
}

void RekeyMultipleTimesFuzz(FuzzedDataProvider &provider)
{
    std::string baseDir = std::string("/data/service/el1/public/database/") + appId.appId;

    SetUpEncryptedKvStore(appId, storeId, singleKvStore_);
    if (singleKvStore_ == nullptr) {
        return;
    }

    std::string key = provider.ConsumeRandomLengthString();
    std::string value = provider.ConsumeRandomLengthString();

    singleKvStore_->Put(key, value);

    for (int i = 0; i < REKEY_TIMES_COUNT; i++) {
        singleKvStore_->Rekey();
    }

    singleKvStore_->Delete(key);
    TearDownEncryptedKvStore(appId, storeId, baseDir);
    singleKvStore_ = nullptr;
}

void RekeyWithBatchDataFuzz(FuzzedDataProvider &provider)
{
    std::string baseDir = std::string("/data/service/el1/public/database/") + appId.appId;

    SetUpEncryptedKvStore(appId, storeId, singleKvStore_);
    if (singleKvStore_ == nullptr) {
        return;
    }

    size_t batchSize = provider.ConsumeIntegralInRange<size_t>(1, 50);
    std::vector<Entry> entries;
    std::string prefix = provider.ConsumeRandomLengthString();

    for (size_t i = 0; i < batchSize; i++) {
        Entry entry;
        entry.key = prefix + "_key_" + std::to_string(i);
        entry.value = prefix + "_value_" + std::to_string(i);
        entries.push_back(entry);
    }

    singleKvStore_->PutBatch(entries);

    singleKvStore_->Rekey();

    std::vector<Key> keys;
    for (size_t i = 0; i < batchSize; i++) {
        keys.push_back(prefix + "_key_" + std::to_string(i));
    }
    singleKvStore_->DeleteBatch(keys);

    TearDownEncryptedKvStore(appId, storeId, baseDir);
    singleKvStore_ = nullptr;
}

void RekeyAfterTransactionFuzz(FuzzedDataProvider &provider)
{
    std::string baseDir = std::string("/data/service/el1/public/database/") + appId.appId;

    SetUpEncryptedKvStore(appId, storeId, singleKvStore_);
    if (singleKvStore_ == nullptr) {
        return;
    }

    singleKvStore_->StartTransaction();

    std::string key1 = provider.ConsumeRandomLengthString() + "_trans_1";
    std::string key2 = provider.ConsumeRandomLengthString() + "_trans_2";
    singleKvStore_->Put(key1, "value1");
    singleKvStore_->Put(key2, "value2");

    singleKvStore_->Commit();

    singleKvStore_->Rekey();

    singleKvStore_->Delete(key1);
    singleKvStore_->Delete(key2);

    TearDownEncryptedKvStore(appId, storeId, baseDir);
    singleKvStore_ = nullptr;
}

void RekeyUnencryptedFuzz(FuzzedDataProvider &provider)
{
    InitKvstoreFuzzer fuzzer;
    fuzzer.SetUpTestCase(appId, storeId, KvStoreType::SINGLE_VERSION, singleKvStore_);
    if (singleKvStore_ == nullptr) {
        return;
    }

    std::string key = provider.ConsumeRandomLengthString();
    std::string value = provider.ConsumeRandomLengthString();
    singleKvStore_->Put(key, value);

    singleKvStore_->Rekey();

    singleKvStore_->Delete(key);
    fuzzer.TearDown(appId, storeId);
    singleKvStore_ = nullptr;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    uint32_t testCase = provider.ConsumeIntegralInRange<uint32_t>(0, MAX_TEST_CASE);
    switch (testCase) {
        case REKEY_BASIC_TEST:
            OHOS::RekeyFuzz(provider);
            break;
        case REKEY_MULTIPLE_TIMES_TEST:
            OHOS::RekeyMultipleTimesFuzz(provider);
            break;
        case REKEY_BATCH_DATA_TEST:
            OHOS::RekeyWithBatchDataFuzz(provider);
            break;
        case REKEY_TRANSACTION_TEST:
            OHOS::RekeyAfterTransactionFuzz(provider);
            break;
        case REKEY_UNENCRYPTED_TEST:
            OHOS::RekeyUnencryptedFuzz(provider);
            break;
        default:
            OHOS::RekeyFuzz(provider);
            break;
    }
    return 0;
}
