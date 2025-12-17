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

#include "queryrange_fuzzer.h"
#include "fuzzer_data.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "get_query_info.h"

using namespace DistributedDB;
using namespace std;

namespace OHOS {
void FuzzIsNotNull(FuzzedDataProvider &provider)
{
    std::string rawString = provider.ConsumeRandomLengthString();
    Query query = Query::Select().IsNotNull(rawString);
}

void FuzzPrefixKey(FuzzedDataProvider &provider)
{
    // 1024 max size
    int maxSize = 1024;
    int len = provider.ConsumeIntegralInRange<int>(0, maxSize);
    Key key = provider.ConsumeBytes<uint8_t>(len);
    Query query = Query::Select().PrefixKey(key);
}

void FuzzSuggestIndex(FuzzedDataProvider &provider)
{
    std::string rawString = provider.ConsumeRandomLengthString();
    Query query = Query::Select().SuggestIndex(rawString);
}

void FuzzInKeys(FuzzedDataProvider &provider)
{
    std::set<Key> keys;
    int countMax = 1024;
    int count = provider.ConsumeIntegralInRange<int>(0, countMax);
    int byteLen = provider.ConsumeIntegralInRange<int>(0, countMax);
    for (int i = 0; i < count; i++) {
        Key tmpKey = provider.ConsumeBytes<uint8_t>(byteLen);
        keys.emplace(tmpKey);
    }
    Query query = Query::Select().InKeys(keys);
}

void FuzzRange(FuzzedDataProvider &provider)
{
    // 512 max size
    int maxSize = 512;
    int beginSize = provider.ConsumeIntegralInRange<int>(0, maxSize);
    int endSize = provider.ConsumeIntegralInRange<int>(0, maxSize);
    std::vector<uint8_t> valuesBegin = provider.ConsumeBytes<uint8_t>(beginSize);
    std::vector<uint8_t> valuesEnd = provider.ConsumeBytes<uint8_t>(endSize);
    Query query = Query::Select().Range(valuesBegin, valuesEnd);
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
    OHOS::FuzzIsNotNull(provider);
    OHOS::FuzzPrefixKey(provider);
    OHOS::FuzzSuggestIndex(provider);
    OHOS::FuzzInKeys(provider);
    OHOS::FuzzRange(provider);
    return 0;
}
