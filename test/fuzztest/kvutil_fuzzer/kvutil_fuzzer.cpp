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

#include "kvutil_fuzzer.h"

#include <string>
#include <sys/stat.h>
#include <vector>
#include "distributed_kv_data_manager.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "store_errno.h"
#include "kv_utils.h"

using namespace OHOS;
using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
namespace OHOS {
using ValueType = std::variant<std::monostate, int64_t, double, std::string, bool, std::vector<uint8_t>>;
static std::shared_ptr<SingleKvStore> singleKvStore_ = nullptr;
static constexpr const char *KEY = "key";
static constexpr const char *VALUE = "value";
static constexpr const char *PREFIX = "test_";
enum DataType {
    STRING = 1,
    INT64,
    BOOL,
    DOUBLE,
    VECTOR_INT64,
};

void SetUpTestCase(void)
{
    DistributedKvDataManager manager;
    Options options = {
        .createIfMissing = true,
        .encrypt = false,
        .autoSync = true,
        .securityLevel = S1,
        .kvStoreType = KvStoreType::SINGLE_VERSION
    };
    options.area = EL1;
    AppId appId = { "kvstorefuzzertest" };
    options.baseDir = std::string("/data/service/el1/public/database/") + appId.appId;
    /* define kvstore(database) name. */
    StoreId storeId = { "fuzzer_single" };
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    /* [create and] open and initialize kvstore instance. */
    manager.GetSingleKvStore(options, appId, storeId, singleKvStore_);
}

void TearDown(void)
{
    (void)remove("/data/service/el1/public/database/kvutilfuzzertest/key");
    (void)remove("/data/service/el1/public/database/kvutilfuzzertest/kvdb");
    (void)remove("/data/service/el1/public/database/kvutilfuzzertest");
}

void ToResultSetBridgeFuzz(FuzzedDataProvider &provider)
{
    std::string skey;
    std::string svalue;
    Entry entry;
    std::vector<Entry> entries;
    std::vector<Key> keys;
    int sum = provider.ConsumeIntegralInRange<size_t>(0, 10);
    for (size_t i = 0; i < sum; i++) {
        skey = provider.ConsumeRandomLengthString();
        svalue = provider.ConsumeRandomLengthString();
        entry.key = {PREFIX + skey};
        entry.value = {PREFIX + svalue};
        entries.push_back(entry);
        keys.push_back(entry.key);
    }
    singleKvStore_->PutBatch(entries);
    std::shared_ptr<KvStoreResultSet> resultSet = nullptr;
    DataSharePredicates predicates;
    predicates.KeyPrefix("test");
    DataQuery query;
    auto status = KvUtils::ToQuery(predicates, query);
    if (status == Status::SUCCESS) {
        status = singleKvStore_->GetResultSet(query, resultSet);
    }
    KvUtils::ToResultSetBridge(resultSet);
    singleKvStore_->DeleteBatch(keys);
}

void ToEntryFuzz(FuzzedDataProvider &provider)
{
    DataShareValuesBucket bucket {};
    if (provider.ConsumeBool()) {
        bucket.Put(KEY, provider.ConsumeRandomLengthString());
        bucket.Put(VALUE, provider.ConsumeRandomLengthString());
    }
    auto entry = KvUtils::ToEntry(bucket);
}

ValueType CreateData(int32_t type, FuzzedDataProvider &provider)
{
    ValueType data;
    switch (type) {
        case DataType::STRING:
            data = provider.ConsumeRandomLengthString();
            break;
        case DataType::INT64:
            data = provider.ConsumeIntegral<int64_t>();
            break;
        case DataType::BOOL:
            data = provider.ConsumeBool();
            break;
        case DataType::DOUBLE:
            data = provider.ConsumeFloatingPoint<double>();
            break;
        case DataType::VECTOR_INT64:
            data = provider.ConsumeRemainingBytes<uint8_t>();
            break;
        default:
            data = {};
            break;
    }
    return data;
}

void ToEntriesFuzz(FuzzedDataProvider &provider)
{
    std::vector<DataShareValuesBucket> buckets;
    const uint8_t bucketCount = provider.ConsumeIntegralInRange<uint8_t>(1, 10);
    DataShareValuesBucket bucket;
    ValueType key;
    ValueType value;
    for (int i = 1; i < bucketCount; ++i) {
        auto keyType = provider.ConsumeIntegralInRange(1, 5);
        auto valueType = provider.ConsumeIntegralInRange(1, 5);
        key = CreateData(keyType, provider);
        value = CreateData(valueType, provider);
        bucket.Put(KEY, key);
        bucket.Put(VALUE, value);
        buckets.push_back(std::move(bucket));
    }
    auto entry = KvUtils::ToEntries(buckets);
}

void GetKeysFuzz(FuzzedDataProvider &provider)
{
    std::vector<std::string> keys;
    keys.push_back(provider.ConsumeRandomLengthString());
    keys.push_back(provider.ConsumeRandomLengthString());
    keys.push_back(provider.ConsumeRandomLengthString());
    DataSharePredicates predicates;
    predicates.InKeys(keys);
    std::vector<Key> kvKeys;
    KvUtils::GetKeys(predicates, kvKeys);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider provider(data, size);
    OHOS::SetUpTestCase();
    OHOS::ToResultSetBridgeFuzz(provider);
    OHOS::ToEntryFuzz(provider);
    OHOS::ToEntriesFuzz(provider);
    OHOS::GetKeysFuzz(provider);
    OHOS::TearDown();
    return 0;
}