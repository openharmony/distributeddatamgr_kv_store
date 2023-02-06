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

#ifndef DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ITYPES_UTIL_H
#define DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ITYPES_UTIL_H
#include <climits>
#include <memory>

#include "change_notification.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "kvdb_service.h"
#include "message_parcel.h"
#include "rdb_types.h"
#include "types.h"

namespace OHOS {
namespace DistributedKv {
class ITypesUtil final {
public:
    API_EXPORT static bool Marshal(MessageParcel &data);
    API_EXPORT static bool Unmarshal(MessageParcel &data);

    API_EXPORT static bool Marshalling(uint32_t input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(uint32_t &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(int32_t input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(int32_t &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(uint64_t input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(uint64_t &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const std::monostate &input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(std::monostate &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const std::string &input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(std::string &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const std::vector<uint8_t> &input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(std::vector<uint8_t> &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const Blob &blob, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(Blob &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const Entry &entry, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(Entry &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const DeviceInfo &entry, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(DeviceInfo &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const ChangeNotification &notification, MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(ChangeNotification &output, MessageParcel &parcel);

    API_EXPORT static bool Marshalling(const DistributedRdb::RdbSyncerParam &param, MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(DistributedRdb::RdbSyncerParam &param, MessageParcel &parcel);

    API_EXPORT static bool Marshalling(const DistributedRdb::SyncOption &option, MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(DistributedRdb::SyncOption &option, MessageParcel &parcel);

    API_EXPORT static bool Marshalling(const DistributedRdb::RdbPredicates &predicates, MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(DistributedRdb::RdbPredicates &predicates, MessageParcel &parcel);

    API_EXPORT static bool Marshalling(const Options &input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(Options &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const sptr<IRemoteObject> &input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(sptr<IRemoteObject> &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(IRemoteObject* input, MessageParcel &data);

    API_EXPORT static bool Marshalling(const SyncPolicy &input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(SyncPolicy &output, MessageParcel &data);

    API_EXPORT static bool Marshalling(const KVDBService::DevBrief &input, MessageParcel &data);
    API_EXPORT static bool Unmarshalling(KVDBService::DevBrief &output, MessageParcel &data);

    API_EXPORT static bool Unmarshalling(DataShare::DataSharePredicates &predicates, MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(DataShare::DataShareValuesBucket &valuesBucket, MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(DataShare::OperationItem &operationItem, MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(DataShare::DataSharePredicatesObject &predicatesObject, MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(DataShare::DataSharePredicatesObjects &predicatesObject,
        MessageParcel &parcel);
    API_EXPORT static bool Unmarshalling(DataShare::DataShareValueObject &valueObject, MessageParcel &parcel);

    API_EXPORT static int64_t GetTotalSize(const std::vector<Entry> &entries);
    API_EXPORT static int64_t GetTotalSize(const std::vector<Key> &entries);

    template<typename ..._Types>
    static bool Marshalling(const std::variant<_Types...> &input, MessageParcel &data)
    {
        uint32_t index = static_cast<uint32_t>(input.index());
        if (!data.WriteUint32(index)) {
            return false;
        }

        return WriteVariant<decltype(input), _Types...>(0, input, data);
    }

    template<typename ..._Types>
    static bool Unmarshalling(std::variant<_Types...> &output, MessageParcel &data)
    {
        uint32_t index = data.ReadUint32();
        if (index >= sizeof ...(_Types)) {
            return false;
        }

        return ReadVariant<decltype(output), _Types...>(0, index, output, data);
    }

    template<class T>
    static bool Marshalling(const std::vector<T> &val, MessageParcel &parcel);
    template<class T>
    static bool Unmarshalling(std::vector<T> &val, MessageParcel &parcel);

    template<class K, class V>
    static bool Marshalling(const std::map<K, V> &val, MessageParcel &parcel);
    template<class K, class V>
    static bool Unmarshalling(std::map<K, V> &val, MessageParcel &parcel);

    template<typename T, typename... Types>
    static bool Marshal(MessageParcel &parcel, const T &first, const Types &...others);
    template<typename T, typename... Types>
    static bool Unmarshal(MessageParcel &parcel, T &first, Types &...others);

    template<typename T>
    static bool MarshalToBuffer(const T &input, int size, MessageParcel &data);

    template<typename T>
    static bool MarshalToBuffer(const std::vector<T> &input, int size, MessageParcel &data);

    template<typename T>
    static bool UnmarshalFromBuffer(MessageParcel &data, T &output);
    template<typename T>
    static bool UnmarshalFromBuffer(MessageParcel &data, std::vector<T> &output);

private:
    static bool Marshalling(bool input, MessageParcel &data) = delete;
    static bool Unmarshalling(bool &output, MessageParcel &data) = delete;
    template<typename _OutTp>
    static bool ReadVariant(uint32_t step, uint32_t index, const _OutTp &output, MessageParcel &data)
    {
        return false;
    }

    template<typename _OutTp, typename _First, typename ..._Rest>
    static bool ReadVariant(uint32_t step, uint32_t index, const _OutTp &output, MessageParcel &data)
    {
        if (step == index) {
            _First value{};
            auto success = ITypesUtil::Unmarshalling(value, data);
            output = value;
            return success;
        }
        return ReadVariant<_OutTp, _Rest...>(step + 1, index, output, data);
    }

    template<typename _InTp>
    static bool WriteVariant(uint32_t step, const _InTp &input, MessageParcel &data)
    {
        return false;
    }

    template<typename _InTp, typename _First, typename ..._Rest>
    static bool WriteVariant(uint32_t step, const _InTp &input, MessageParcel &data)
    {
        if (step == input.index()) {
            return ITypesUtil::Marshalling(std::get<_First>(input), data);
        }
        return WriteVariant<_InTp, _Rest...>(step + 1, input, data);
    }
};

template<class T>
bool ITypesUtil::Marshalling(const std::vector<T> &val, MessageParcel &parcel)
{
    if (val.size() > INT_MAX) {
        return false;
    }

    if (!parcel.WriteInt32(static_cast<int32_t>(val.size()))) {
        return false;
    }

    for (auto &v : val) {
        if (!Marshalling(v, parcel)) {
            return false;
        }
    }
    return true;
}

template<class T>
bool ITypesUtil::Unmarshalling(std::vector<T> &val, MessageParcel &parcel)
{
    int32_t len = parcel.ReadInt32();
    if (len < 0) {
        return false;
    }

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    if ((size > readAbleSize) || (size > val.max_size())) {
        return false;
    }

    val.resize(size);
    if (val.size() < size) {
        return false;
    }

    for (auto &v : val) {
        if (!Unmarshalling(v, parcel)) {
            return false;
        }
    }

    return true;
}

template<typename T>
bool ITypesUtil::MarshalToBuffer(const T &input, int size, MessageParcel &data)
{
    if (!data.WriteInt32(size)) {
        return false;
    }
    if (size == 0) {
        return true;
    }
    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(size);
    if (buffer == nullptr) {
        return false;
    }

    int leftSize = size;
    uint8_t *cursor = buffer.get();
    if (!input.WriteToBuffer(cursor, leftSize)) {
        return false;
    }
    return data.WriteRawData(buffer.get(), size);
}

template<typename T>
bool ITypesUtil::MarshalToBuffer(const std::vector<T> &input, int size, MessageParcel &data)
{
    if (!data.WriteInt32(size)) {
        return false;
    }
    if (size == 0) {
        return true;
    }
    if (!data.WriteInt32(input.size())) {
        return false;
    }

    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(size);
    if (buffer == nullptr) {
        return false;
    }

    uint8_t *cursor = buffer.get();
    int32_t left = size;
    for (const auto &entry : input) {
        if (!entry.WriteToBuffer(cursor, left)) {
            return false;
        }
    }
    return data.WriteRawData(buffer.get(), size);
}

template<typename T>
bool ITypesUtil::UnmarshalFromBuffer(MessageParcel &data, T &output)
{
    int32_t size = data.ReadInt32();
    if (size == 0) {
        return true;
    }
    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(data.ReadRawData(size));
    if (buffer == nullptr) {
        return false;
    }
    return output.ReadFromBuffer(buffer, size);
}

template<typename T>
bool ITypesUtil::UnmarshalFromBuffer(MessageParcel &data, std::vector<T> &output)
{
    int size = data.ReadInt32();
    if (size == 0) {
        return true;
    }

    int count = data.ReadInt32();
    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(data.ReadRawData(size));
    if (count < 0 || buffer == nullptr) {
        return false;
    }

    output.resize(count);
    for (auto &entry : output) {
        if (!entry.ReadFromBuffer(buffer, size)) {
            output.clear();
            return false;
        }
    }
    return true;
}

template<typename T, typename... Types>
bool ITypesUtil::Marshal(MessageParcel &parcel, const T &first, const Types &...others)
{
    if (!Marshalling(first, parcel)) {
        return false;
    }
    return Marshal(parcel, others...);
}

template<typename T, typename... Types>
bool ITypesUtil::Unmarshal(MessageParcel &parcel, T &first, Types &...others)
{
    if (!Unmarshalling(first, parcel)) {
        return false;
    }
    return Unmarshal(parcel, others...);
}

template<class K, class V>
bool ITypesUtil::Marshalling(const std::map<K, V> &result, MessageParcel &parcel)
{
    if (!parcel.WriteInt32(static_cast<int32_t>(result.size()))) {
        return false;
    }
    for (const auto &entry : result) {
        if (!Marshalling(entry.first, parcel)) {
            return false;
        }
        if (!Marshalling(entry.second, parcel)) {
            return false;
        }
    }
    return true;
}

template<class K, class V>
bool ITypesUtil::Unmarshalling(std::map<K, V> &val, MessageParcel &parcel)
{
    int32_t size = 0;
    if (!parcel.ReadInt32(size)) {
        return false;
    }
    if (size < 0) {
        return false;
    }

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t len = static_cast<size_t>(size);
    if ((len > readAbleSize) || len > val.max_size()) {
        return false;
    }

    for (int32_t i = 0; i < size; i++) {
        K key;
        if (!Unmarshalling(key, parcel)) {
            return false;
        }
        V value;
        if (!Unmarshalling(value, parcel)) {
            return false;
        }
        val.insert({ key, value });
    }
    return true;
}
} // namespace DistributedKv
} // namespace OHOS
#endif
