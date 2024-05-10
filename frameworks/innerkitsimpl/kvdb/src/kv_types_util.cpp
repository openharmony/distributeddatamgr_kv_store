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
#define LOG_TAG "ITypesUtil::KvTypesUtil"
#include "kv_types_util.h"
#include "log_print.h"
namespace OHOS::ITypesUtil {
using namespace DistributedKv;
template<>
bool Marshalling(const Blob &blob, MessageParcel &data)
{
    return data.WriteUInt8Vector(blob.Data());
}

template<>
bool Unmarshalling(Blob &output, MessageParcel &data)
{
    std::vector<uint8_t> blob;
    bool result = data.ReadUInt8Vector(&blob);
    output = blob;
    return result;
}

template<>
bool Marshalling(const AppId &input, MessageParcel &data)
{
    return ITypesUtil::Marshalling(input.appId, data);
}

template<>
bool Unmarshalling(AppId &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshalling(output.appId, data);
}

template<>
bool Marshalling(const StoreId &input, MessageParcel &data)
{
    return ITypesUtil::Marshalling(input.storeId, data);
}

template<>
bool Unmarshalling(StoreId &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshalling(output.storeId, data);
}

template<>
bool Marshalling(const Entry &entry, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, entry.key, entry.value);
}

template<>
bool Unmarshalling(Entry &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.key, output.value);
}

template<>
bool Marshalling(const DeviceInfo &entry, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, entry.deviceId, entry.deviceName, entry.deviceType);
}

template<>
bool Unmarshalling(DeviceInfo &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.deviceId, output.deviceName, output.deviceType);
}

template<>
bool Marshalling(const ChangeNotification &notification, MessageParcel &parcel)
{
    return ITypesUtil::Marshal(parcel, notification.GetInsertEntries(), notification.GetUpdateEntries(),
        notification.GetDeleteEntries(), notification.GetDeviceId(), notification.IsClear());
}

template<>
bool Unmarshalling(ChangeNotification &output, MessageParcel &parcel)
{
    std::vector<Entry> inserts;
    std::vector<Entry> updates;
    std::vector<Entry> deletes;
    std::string deviceId;
    bool isClear = false;
    if (!ITypesUtil::Unmarshal(parcel, inserts, updates, deletes, deviceId, isClear)) {
        return false;
    }
    output = ChangeNotification(std::move(inserts), std::move(updates), std::move(deletes), deviceId, isClear);
    return true;
}

template<>
bool Marshalling(const Options &input, MessageParcel &data)
{
    if (!ITypesUtil::Marshal(data, input.schema, input.hapName, input.policies)) {
        ZLOGE("write policies failed");
        return false;
    }

    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(sizeof(input));
    Options *target = reinterpret_cast<Options *>(buffer.get());
    target->createIfMissing = input.createIfMissing;
    target->encrypt = input.encrypt;
    target->persistent = input.persistent;
    target->backup = input.backup;
    target->autoSync = input.autoSync;
    target->syncable = input.syncable;
    target->securityLevel = input.securityLevel;
    target->area = input.area;
    target->kvStoreType = input.kvStoreType;
    target->isNeedCompress = input.isNeedCompress;
    target->dataType = input.dataType;
    target->isPublic = input.isPublic;
    return data.WriteRawData(buffer.get(), sizeof(input));
}

template<>
bool Unmarshalling(Options &output, MessageParcel &data)
{
    if (!ITypesUtil::Unmarshal(data, output.schema, output.hapName, output.policies)) {
        ZLOGE("read policies failed");
        return false;
    }

    const Options *source = reinterpret_cast<const Options *>(data.ReadRawData(sizeof(output)));
    if (source == nullptr) {
        return false;
    }
    output.createIfMissing = source->createIfMissing;
    output.encrypt = source->encrypt;
    output.persistent = source->persistent;
    output.backup = source->backup;
    output.autoSync = source->autoSync;
    output.securityLevel = source->securityLevel;
    output.area = source->area;
    output.kvStoreType = source->kvStoreType;
    output.syncable = source->syncable;
    output.isNeedCompress = source->isNeedCompress;
    output.dataType = source->dataType;
    output.isPublic = source->isPublic;
    return true;
}

template<>
bool Marshalling(const SyncPolicy &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.type, input.value);
}

template<>
bool Unmarshalling(SyncPolicy &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.type, output.value);
}

template<>
bool Marshalling(const SwitchData &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.value, input.length);
}

template<>
bool Unmarshalling(SwitchData &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.value, output.length);
}

template<>
bool Marshalling(const Status &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, static_cast<int32_t>(input));
}

template<>
bool Unmarshalling(Status &output, MessageParcel &data)
{
    int32_t status;
    if (!ITypesUtil::Unmarshal(data, status)) {
        return false;
    }
    output = static_cast<Status>(status);
    return true;
}

template<>
bool Marshalling(const Notification &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(
        data, input.data.value, input.data.length, input.deviceId, static_cast<int32_t>(input.state));
}

template<>
bool Unmarshalling(Notification &output, MessageParcel &data)
{
    int32_t state;
    if (!ITypesUtil::Unmarshal(data, output.data.value, output.data.length, output.deviceId, state)) {
        return false;
    }
    output.state = static_cast<SwitchState>(state);
    return true;
}

template<>
bool Marshalling(const ProgressDetail &input, MessageParcel &data)
{
    return Marshal(data, input.progress, input.code, input.details);
}
template<>
bool Unmarshalling(ProgressDetail &output, MessageParcel &data)
{
    return Unmarshal(data, output.progress, output.code, output.details);
}
template<>
bool Marshalling(const TableDetail &input, MessageParcel &data)
{
    return Marshal(data, input.upload, input.download);
}
template<>
bool Unmarshalling(TableDetail &output, MessageParcel &data)
{
    return Unmarshal(data, output.upload, output.download);
}
template<>
bool Marshalling(const Statistic &input, MessageParcel &data)
{
    return Marshal(data, input.total, input.success, input.failed, input.untreated);
}
template<>
bool Unmarshalling(Statistic &output, MessageParcel &data)
{
    return Unmarshal(data, output.total, output.success, output.failed, output.untreated);
}

int64_t GetTotalSize(const std::vector<Entry> &entries)
{
    int64_t bufferSize = 1;
    for (const auto &item : entries) {
        if (item.key.Size() > Entry::MAX_KEY_LENGTH || item.value.Size() > Entry::MAX_VALUE_LENGTH) {
            return -bufferSize;
        }
        bufferSize += item.key.RawSize() + item.value.RawSize();
    }
    return bufferSize - 1;
}

int64_t GetTotalSize(const std::vector<Key> &entries)
{
    int64_t bufferSize = 1;
    for (const auto &item : entries) {
        if (item.Size() > Entry::MAX_KEY_LENGTH) {
            return -bufferSize;
        }
        bufferSize += item.RawSize();
    }
    return bufferSize - 1;
}
}