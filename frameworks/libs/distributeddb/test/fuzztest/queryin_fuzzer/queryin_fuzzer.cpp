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

#include "queryin_fuzzer.h"
#include "fuzzer_data.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "get_query_info.h"

using namespace DistributedDB;
using namespace std;

namespace {
    constexpr const char *TEST_FIELD_NAME = "$.test";
}

namespace OHOS {
void FuzzIn(FuzzedDataProvider &provider)
{
    std::string rawString = provider.ConsumeRandomLengthString();
    std::vector<std::string> values;
    // 512 max size
    int maxSize = 512;
    int size = provider.ConsumeIntegralInRange<int>(0, maxSize);
    for (int i = 0; i < size; i++) {
        values.emplace_back(rawString);
    }
    Query query = Query::Select().In(TEST_FIELD_NAME, values);
}

void FuzzNotIn(FuzzedDataProvider &provider)
{
    std::string rawString = provider.ConsumeRandomLengthString();
    std::vector<std::string> values;
    // 512 max size
    int maxSize = 512;
    int size = provider.ConsumeIntegralInRange<int>(0, maxSize);
    for (int i = 0; i < size; i++) {
        values.emplace_back(rawString);
    }
    Query query = Query::Select().NotIn(TEST_FIELD_NAME, values);
}

void FuzzIsNull(FuzzedDataProvider &provider)
{
    std::string rawString = provider.ConsumeRandomLengthString();
    Query query = Query::Select().IsNull(rawString);
}

void FuzzAssetsOnly(FuzzedDataProvider &provider)
{
    const int lenMod = 30;  // 30 is mod for string vector size
    std::string tableName = provider.ConsumeRandomLengthString(provider.ConsumeIntegralInRange<size_t>(0, lenMod));
    std::string fieldName = provider.ConsumeRandomLengthString(provider.ConsumeIntegralInRange<size_t>(0, lenMod));
    std::map<std::string, std::set<std::string>> assets;
    std::set<std::string> set;
    // 512 max size
    size_t maxSize = 512;
    size_t size = provider.ConsumeIntegralInRange<size_t>(0, maxSize);
    for (size_t i = 1; i <= size; i++) {
        set.emplace(provider.ConsumeRandomLengthString(provider.ConsumeIntegralInRange<size_t>(0, i)));
    }
    assets[provider.ConsumeRandomLengthString(provider.ConsumeIntegralInRange<size_t>(0, lenMod))] = set;
    Query query = Query::Select().From(tableName)
                      .BeginGroup()
                      .EqualTo(fieldName, provider.ConsumeIntegral<int>())
                      .And()
                      .Or()
                      .AssetsOnly(assets)
                      .EndGroup();
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // u32 4 bytes
    if (size < 4) {
        return 0;
    }
    FuzzedDataProvider provider(data, size);
    // Run your code on data
    OHOS::FuzzIn(provider);
    OHOS::FuzzNotIn(provider);
    OHOS::FuzzIsNull(provider);
    OHOS::FuzzAssetsOnly(provider);
    return 0;
}
