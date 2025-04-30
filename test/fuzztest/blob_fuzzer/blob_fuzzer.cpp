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

#include "blob_fuzzer.h"

#include <cstdint>
#include <vector>
#include "fuzzer/FuzzedDataProvider.h"
#include "securec.h"
#include "types.h"
#include "blob.h"

using namespace OHOS::DistributedKv;
namespace OHOS {
void BlobSelfOption(const Blob &blob)
{
    blob.Empty();
    blob.Size();
    blob.Data();
    blob.ToString();
    blob.RawSize();
}

void BlobEachOtherOption(const Blob &blob1, const Blob &blob2)
{
    blob1.Compare(blob2);
    Blob blobOut;
    blob1.Compare(blobOut);
    blob1.StartsWith(blob2);
}

void BlobOption(const Blob &blob)
{
    BlobSelfOption(blob);
    Blob blobTmp(blob);
    BlobEachOtherOption(blob, blobTmp);

    Blob blobPrefix = { "fuzz" };
    blobTmp = blobPrefix.ToString() + blob.ToString();
    if (blobPrefix[0] == blobTmp[0] && (!(blobPrefix == blobTmp))) {
        BlobEachOtherOption(blobTmp, blobPrefix);
    }
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fuzzer(data, size);
    size_t blobSize = fuzzer.ConsumeIntegralInRange<size_t>(1, 50);
    std::vector<uint8_t> fuzzVec = fuzzer.ConsumeBytes<uint8_t>(blobSize);

    std::string str1 = fuzzer.ConsumeRandomLengthString();
    Blob blob1(str1.c_str());
    std::string str2 = fuzzer.ConsumeRandomLengthString();
    blob1 = str2.c_str();
    Blob blob2(fuzzer.ConsumeRandomLengthString());
    blob2 = fuzzer.ConsumeRandomLengthString();
    Blob blob3(fuzzVec);
    std::string str3 = fuzzer.ConsumeRandomLengthString();
    Blob blob4(str3.c_str(), str3.length() + 1);
    Blob blob5(blob4);
    Blob blob6(std::move(blob5));
    Blob blob7 = blob6;
    blob7 = Blob(blob6);
    int count = 10;
    auto buffer = std::make_unique<uint8_t[]>(count);
    uint8_t *writePtr = buffer.get();
    Blob blob8(fuzzer.ConsumeRandomLengthString());
    blob8.WriteToBuffer(writePtr, count);
    OHOS::BlobOption(blob8);

    return 0;
}
