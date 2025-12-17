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

#include "queryorder_fuzzer.h"
#include "fuzzer_data.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "get_query_info.h"

using namespace DistributedDB;
using namespace std;

namespace {
    constexpr const char *TEST_FIELD_NAME = "$.test";
}

namespace OHOS {
void FuzzOrderBy(FuzzedDataProvider &provider)
{
    std::string rawString = provider.ConsumeRandomLengthString();
    bool isAsc = provider.ConsumeBool();
    (void)Query::Select().OrderBy(rawString, isAsc);
    (void)Query::Select().GreaterThanOrEqualTo(rawString, true);
    (void)Query::Select().GreaterThanOrEqualTo(rawString, false);
}

void FuzzLimit(FuzzedDataProvider &provider)
{
    int size1 = provider.ConsumeIntegral<int>();
    int size2 = provider.ConsumeIntegral<int>();
    Query query = Query::Select().Limit(size1, size2);
}

void FuzzLike(FuzzedDataProvider &provider)
{
    std::string rawString = provider.ConsumeRandomLengthString();
    Query query = Query::Select().Like(rawString, rawString);
}

void FuzzNotLike(FuzzedDataProvider &provider)
{
    std::string rawString = provider.ConsumeRandomLengthString();
    Query query = Query::Select().NotLike(TEST_FIELD_NAME, rawString);
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
    OHOS::FuzzOrderBy(provider);
    OHOS::FuzzLimit(provider);
    OHOS::FuzzLike(provider);
    OHOS::FuzzNotLike(provider);
    return 0;
}
