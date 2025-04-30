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

#include "typesutil_fuzzer.h"

#include <cstdint>
#include <variant>
#include <vector>

#include "change_notification.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "itypes_util.h"
#include "kv_types_util.h"
#include "types.h"

using namespace OHOS::DistributedKv;
namespace OHOS {
void ClientDevFuzz(FuzzedDataProvider &provider)
{
    DeviceInfo clientDev;
    clientDev.deviceId = provider.ConsumeRandomLengthString();
    clientDev.deviceName = provider.ConsumeRandomLengthString();
    clientDev.deviceType = provider.ConsumeRandomLengthString();
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, clientDev);
    DeviceInfo serverDev;
    ITypesUtil::Unmarshal(parcel, serverDev);
}

void EntryFuzz(FuzzedDataProvider &provider)
{
    Entry entryIn;
    entryIn.key = provider.ConsumeRandomLengthString();
    entryIn.value = provider.ConsumeRandomLengthString();
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, entryIn);
    Entry entryOut;
    ITypesUtil::Unmarshal(parcel, entryOut);
}

void BlobFuzz(FuzzedDataProvider &provider)
{
    Blob blobIn = provider.ConsumeRandomLengthString();
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, blobIn);
    Blob blobOut;
    ITypesUtil::Unmarshal(parcel, blobOut);
}

void VecFuzz(FuzzedDataProvider &provider)
{
    size_t blobSize = provider.ConsumeIntegralInRange<size_t>(1, 50);
    std::vector<uint8_t> vec = provider.ConsumeBytes<uint8_t>(blobSize);
    std::vector<uint8_t> vecIn(vec);
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, vecIn);
    std::vector<uint8_t> vecOut;
    ITypesUtil::Unmarshal(parcel, vecOut);
}

void OptionsFuzz(FuzzedDataProvider &provider)
{
    Options optionsIn = {
        .createIfMissing = true,
        .encrypt = false,
        .autoSync = true,
        .kvStoreType = KvStoreType::SINGLE_VERSION
    };
    optionsIn.area = EL1;
    optionsIn.baseDir = provider.ConsumeRandomLengthString();
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, optionsIn);
    Options optionsOut;
    ITypesUtil::Unmarshal(parcel, optionsOut);
}

void SyncPolicyFuzz(FuzzedDataProvider &provider)
{
    uint32_t base1 = provider.ConsumeIntegral<uint32_t>();
    uint32_t base2 = provider.ConsumeIntegral<uint32_t>();
    SyncPolicy syncPolicyIn { base1, base2 };
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, syncPolicyIn);
    SyncPolicy syncPolicyOut;
    ITypesUtil::Unmarshal(parcel, syncPolicyOut);
}

void ChangeNotificationFuzz(FuzzedDataProvider &provider)
{
    Entry insert;
    Entry update;
    Entry del;
    insert.key = provider.ConsumeRandomLengthString();
    update.key = provider.ConsumeRandomLengthString();
    del.key = provider.ConsumeRandomLengthString();
    insert.value = provider.ConsumeRandomLengthString();
    update.value = provider.ConsumeRandomLengthString();
    del.value = provider.ConsumeRandomLengthString();
    std::vector<Entry> inserts;
    std::vector<Entry> updates;
    std::vector<Entry> deleteds;
    inserts.push_back(insert);
    updates.push_back(update);
    deleteds.push_back(del);

    bool boolBase = provider.ConsumeBool();
    std::string strBase = provider.ConsumeRandomLengthString();
    ChangeNotification changeIn(std::move(inserts), std::move(updates), std::move(deleteds), strBase, boolBase);
    MessageParcel parcel;
    ITypesUtil::Marshal(parcel, changeIn);
    std::vector<Entry> empty;
    ChangeNotification changeOut(std::move(empty), {}, {}, "", !boolBase);
    ITypesUtil::Unmarshal(parcel, changeOut);
}

void IntFuzz(FuzzedDataProvider &provider)
{
    size_t valBase = provider.ConsumeIntegral<size_t>();
    MessageParcel parcel;
    int32_t int32In = static_cast<int32_t>(valBase);
    ITypesUtil::Marshal(parcel, int32In);
    int32_t int32Out;
    ITypesUtil::Unmarshal(parcel, int32Out);

    uint32_t uint32In = static_cast<uint32_t>(valBase);
    ITypesUtil::Marshal(parcel, uint32In);
    uint32_t uint32Out;
    ITypesUtil::Unmarshal(parcel, uint32Out);

    uint64_t uint64In = static_cast<uint64_t>(valBase);
    ITypesUtil::Marshal(parcel, uint64In);
    uint64_t uint64Out;
    ITypesUtil::Unmarshal(parcel, uint64Out);
}

void StringFuzz(FuzzedDataProvider &provider)
{
    MessageParcel parcel;
    std::string strIn = provider.ConsumeRandomLengthString();
    ITypesUtil::Marshal(parcel, strIn);
    std::string strOut;
    ITypesUtil::Unmarshal(parcel, strOut);
}

void GetTotalSizeFuzz(FuzzedDataProvider &provider)
{
    Entry entry;
    entry.key = provider.ConsumeRandomLengthString();
    entry.value = provider.ConsumeRandomLengthString();
    size_t size1 = provider.ConsumeIntegralInRange<size_t>(1, 10);
    std::vector<Entry> VecEntryIn(size1, entry);
    std::string strBase = provider.ConsumeRandomLengthString();
    size_t size2 = provider.ConsumeIntegralInRange<size_t>(1, 10);
    std::vector<Key> VecKeyIn(size2, Key { strBase });
    ITypesUtil::GetTotalSize(VecEntryIn);
    ITypesUtil::GetTotalSize(VecKeyIn);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider provider(data, size);
    OHOS::ClientDevFuzz(provider);
    OHOS::EntryFuzz(provider);
    OHOS::BlobFuzz(provider);
    OHOS::VecFuzz(provider);
    OHOS::OptionsFuzz(provider);
    OHOS::SyncPolicyFuzz(provider);
    OHOS::ChangeNotificationFuzz(provider);
    OHOS::IntFuzz(provider);
    OHOS::StringFuzz(provider);
    OHOS::GetTotalSizeFuzz(provider);
    return 0;
}
