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
#include <list>
#include <map>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

#include "iremote_object.h"
#include "message_parcel.h"
namespace OHOS {
template<class T>
struct is_container : std::false_type {
};
template<class T>
struct is_container<std::vector<T>> : std::true_type {
};
template<class T>
struct is_container<std::list<T>> : std::true_type {
};
namespace ITypesUtil {
inline constexpr size_t MAX_COUNT = 100000;
inline constexpr size_t MAX_SIZE = 1 * 1024 * 1024 * 1024; //1G
static inline bool Marshal(MessageParcel &data)
{
    return true;
}

static inline bool Unmarshal(MessageParcel &data)
{
    return true;
}

static inline bool Marshalling(int16_t input, MessageParcel &data)
{
    return data.WriteInt16(input);
}

static inline bool Unmarshalling(int16_t &output, MessageParcel &data)
{
    return data.ReadInt16(output);
}

static inline bool Marshalling(uint16_t input, MessageParcel &data)
{
    return data.WriteUint16(input);
}

static inline bool Unmarshalling(uint16_t &output, MessageParcel &data)
{
    return data.ReadUint16(output);
}

static inline bool Marshalling(uint32_t input, MessageParcel &data)
{
    return data.WriteUint32(input);
}

static inline bool Unmarshalling(uint32_t &output, MessageParcel &data)
{
    return data.ReadUint32(output);
}

static inline bool Marshalling(int32_t input, MessageParcel &data)
{
    return data.WriteInt32(input);
}

static inline bool Unmarshalling(int32_t &output, MessageParcel &data)
{
    return data.ReadInt32(output);
}

static inline bool Marshalling(uint64_t input, MessageParcel &data)
{
    return data.WriteUint64(input);
}

static inline bool Unmarshalling(uint64_t &output, MessageParcel &data)
{
    return data.ReadUint64(output);
}

static inline bool Marshalling(int64_t input, MessageParcel &data)
{
    return data.WriteInt64(input);
}

static inline bool Unmarshalling(int64_t &output, MessageParcel &data)
{
    return data.ReadInt64(output);
}

static inline bool Marshalling(double input, MessageParcel &data)
{
    return data.WriteDouble(input);
}

static inline bool Unmarshalling(double &output, MessageParcel &data)
{
    return data.ReadDouble(output);
}

static inline bool Marshalling(bool input, MessageParcel &data)
{
    return data.WriteBool(input);
}

static inline bool Unmarshalling(bool &output, MessageParcel &data)
{
    return data.ReadBool(output);
}

static inline bool Marshalling(const std::monostate &input, MessageParcel &data)
{
    return true;
}

static inline bool Unmarshalling(std::monostate &output, MessageParcel &data)
{
    return true;
}

static inline bool Marshalling(const std::vector<float> &input, MessageParcel &data)
{
    return data.WriteFloatVector(input);
}

static inline bool Unmarshalling(std::vector<float> &output, MessageParcel &data)
{
    return data.ReadFloatVector(&output);
}

static inline bool Marshalling(const std::string &input, MessageParcel &data)
{
    return data.WriteString(input);
}

static inline bool Unmarshalling(std::string &output, MessageParcel &data)
{
    return data.ReadString(output);
}

static inline bool Marshalling(const std::u16string &input, MessageParcel &data)
{
    return data.WriteString16(input);
}

static inline bool Unmarshalling(std::u16string &output, MessageParcel &data)
{
    return data.ReadString16(output);
}

static inline bool Marshalling(const std::vector<uint8_t> &input, MessageParcel &data)
{
    return data.WriteUInt8Vector(input);
}

static inline bool Unmarshalling(std::vector<uint8_t> &output, MessageParcel &data)
{
    return data.ReadUInt8Vector(&output);
}

static inline bool Marshalling(const sptr<IRemoteObject> &input, MessageParcel &data)
{
    return data.WriteRemoteObject(input);
}

static inline bool Unmarshalling(sptr<IRemoteObject> &output, MessageParcel &data)
{
    output = data.ReadRemoteObject();
    return true;
}

static inline bool Marshalling(IRemoteObject *input, MessageParcel &data)
{
    return data.WriteRemoteObject(input);
}

template<typename _OutTp>
bool ReadVariant(uint32_t step, uint32_t index, const _OutTp &output, MessageParcel &data);
template<typename _OutTp, typename _First, typename... _Rest>
bool ReadVariant(uint32_t step, uint32_t index, const _OutTp &output, MessageParcel &data);

template<typename _InTp>
bool WriteVariant(uint32_t step, const _InTp &input, MessageParcel &data);
template<typename _InTp, typename _First, typename... _Rest>
bool WriteVariant(uint32_t step, const _InTp &input, MessageParcel &data);

template<typename... _Types>
bool Marshalling(const std::variant<_Types...> &input, MessageParcel &data);
template<typename... _Types>
bool Unmarshalling(std::variant<_Types...> &output, MessageParcel &data);

template<class K, class V>
bool Marshalling(const std::map<K, V> &result, MessageParcel &parcel);
template<class K, class V>
bool Unmarshalling(std::map<K, V> &val, MessageParcel &parcel);

template<class F, class S, class T>
bool Marshalling(const std::tuple<F, S, T> &result, MessageParcel &parcel);
template<class F, class S, class T>
bool Unmarshalling(std::tuple<F, S, T> &val, MessageParcel &parcel);

template<class F, class S>
bool Marshalling(const std::pair<F, S> &result, MessageParcel &parcel);
template<class F, class S>
bool Unmarshalling(std::pair<F, S> &val, MessageParcel &parcel);

template<class T>
bool Marshalling(const std::vector<T> &val, MessageParcel &parcel);
template<class T>
bool Unmarshalling(std::vector<T> &val, MessageParcel &parcel);

template<class T>
bool Marshalling(const std::list<T> &val, MessageParcel &parcel);
template<class T>
bool Unmarshalling(std::list<T> &val, MessageParcel &parcel);

template<typename T, typename std::enable_if<std::is_pointer<T>{}, int>::type = 0>
bool Marshalling(const T &input, MessageParcel &data) = delete;
template<typename T, typename std::enable_if<std::is_pointer<T>{}, int>::type = 0>
bool Unmarshalling(T &output, MessageParcel &data) = delete;

template<typename T>
bool Marshalling(const T &input, MessageParcel &data);
template<typename T>
bool Unmarshalling(T &output, MessageParcel &data);

template<class T, typename std::enable_if<is_container<T>{}, int>::type = 0>
bool MarshalToContainer(const T &val, MessageParcel &parcel);
template<class T, typename std::enable_if<is_container<T>{}, int>::type = 0>
bool UnmarshalFromContainer(T &val, MessageParcel &parcel);

template<typename T>
bool MarshalToBuffer(const T &input, int size, MessageParcel &data);
template<typename T>
bool MarshalToBuffer(const std::vector<T> &input, int size, MessageParcel &data);

template<typename T>
bool UnmarshalFromBuffer(MessageParcel &data, T &output);
template<typename T>
bool UnmarshalFromBuffer(MessageParcel &data, std::vector<T> &output);

template<typename T, typename... Types>
bool Marshal(MessageParcel &parcel, const T &first, const Types &...others);

template<typename T, typename... Types>
bool Unmarshal(MessageParcel &parcel, T &first, Types &...others);
} // namespace ITypesUtil

template<typename _OutTp>
bool ITypesUtil::ReadVariant(uint32_t step, uint32_t index, const _OutTp &output, MessageParcel &data)
{
    return false;
}

template<typename _OutTp, typename _First, typename... _Rest>
bool ITypesUtil::ReadVariant(uint32_t step, uint32_t index, const _OutTp &output, MessageParcel &data)
{
    if (step == index) {
        _First value{};
        auto success = ITypesUtil::Unmarshalling(value, data);
        output = value;
        return success;
    }
    return ITypesUtil::ReadVariant<_OutTp, _Rest...>(step + 1, index, output, data);
}

template<typename _InTp>
bool ITypesUtil::WriteVariant(uint32_t step, const _InTp &input, MessageParcel &data)
{
    return false;
}

template<typename _InTp, typename _First, typename... _Rest>
bool ITypesUtil::WriteVariant(uint32_t step, const _InTp &input, MessageParcel &data)
{
    if (step == input.index()) {
        return ITypesUtil::Marshalling(std::get<_First>(input), data);
    }
    return ITypesUtil::WriteVariant<_InTp, _Rest...>(step + 1, input, data);
}

template<typename... _Types>
bool ITypesUtil::Marshalling(const std::variant<_Types...> &input, MessageParcel &data)
{
    uint32_t index = static_cast<uint32_t>(input.index());
    if (!data.WriteUint32(index)) {
        return false;
    }

    return ITypesUtil::WriteVariant<decltype(input), _Types...>(0, input, data);
}

template<typename... _Types>
bool ITypesUtil::Unmarshalling(std::variant<_Types...> &output, MessageParcel &data)
{
    uint32_t index = data.ReadUint32();
    if (index >= sizeof...(_Types)) {
        return false;
    }

    return ITypesUtil::ReadVariant<decltype(output), _Types...>(0, index, output, data);
}

template<class K, class V>
bool ITypesUtil::Marshalling(const std::map<K, V> &result, MessageParcel &parcel)
{
    if (!parcel.WriteInt32(static_cast<int32_t>(result.size()))) {
        return false;
    }
    for (const auto &entry : result) {
        if ((!ITypesUtil::Marshalling(entry.first, parcel)) || (!ITypesUtil::Marshalling(entry.second, parcel))) {
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
    if ((static_cast<size_t>(size) > readAbleSize) || static_cast<size_t>(size) > val.max_size()) {
        return false;
    }

    for (int32_t i = 0; i < size; i++) {
        K key;
        if ((!ITypesUtil::Unmarshalling(key, parcel)) || (!ITypesUtil::Unmarshalling(val[key], parcel))) {
            return false;
        }
    }
    return true;
}

template<class F, class S, class T>
bool ITypesUtil::Marshalling(const std::tuple<F, S, T> &result, MessageParcel &parcel)
{
    if (!ITypesUtil::Marshalling(std::get<0>(result), parcel)) {
        return false;
    }
    if (!ITypesUtil::Marshalling(std::get<1>(result), parcel)) {
        return false;
    }
    if (!ITypesUtil::Marshalling(std::get<2>(result), parcel)) { // 2 is the last element in tuple
        return false;
    }
    return true;
}

template<class F, class S, class T>
bool ITypesUtil::Unmarshalling(std::tuple<F, S, T> &val, MessageParcel &parcel)
{
    F first;
    if (!ITypesUtil::Unmarshalling(first, parcel)) {
        return false;
    }
    S second;
    if (!ITypesUtil::Unmarshalling(second, parcel)) {
        return false;
    }
    T third;
    if (!ITypesUtil::Unmarshalling(third, parcel)) {
        return false;
    }
    val = { first, second, third };
    return true;
}

template<class F, class S>
bool ITypesUtil::Marshalling(const std::pair<F, S> &result, MessageParcel &parcel)
{
    if (!ITypesUtil::Marshalling(result.first, parcel)) {
        return false;
    }
    if (!ITypesUtil::Marshalling(result.second, parcel)) {
        return false;
    }
    return true;
}

template<class F, class S>
bool ITypesUtil::Unmarshalling(std::pair<F, S> &val, MessageParcel &parcel)
{
    F first;
    if (!ITypesUtil::Unmarshalling(first, parcel)) {
        return false;
    }
    S second;
    if (!ITypesUtil::Unmarshalling(second, parcel)) {
        return false;
    }
    val = { first, second };
    return true;
}

template<class T>
bool ITypesUtil::Marshalling(const std::vector<T> &val, MessageParcel &parcel)
{
    return ITypesUtil::MarshalToContainer(val, parcel);
}

template<class T>
bool ITypesUtil::Unmarshalling(std::vector<T> &val, MessageParcel &parcel)
{
    return ITypesUtil::UnmarshalFromContainer(val, parcel);
}

template<class T>
bool ITypesUtil::Marshalling(const std::list<T> &val, MessageParcel &parcel)
{
    return ITypesUtil::MarshalToContainer(val, parcel);
}

template<class T>
bool ITypesUtil::Unmarshalling(std::list<T> &val, MessageParcel &parcel)
{
    return ITypesUtil::UnmarshalFromContainer(val, parcel);
}

template<class T, typename std::enable_if<is_container<T>{}, int>::type>
bool ITypesUtil::MarshalToContainer(const T &val, MessageParcel &parcel)
{
    if (val.size() > INT_MAX) {
        return false;
    }

    if (!parcel.WriteInt32(static_cast<int32_t>(val.size()))) {
        return false;
    }

    for (auto &v : val) {
        if (!ITypesUtil::Marshalling(v, parcel)) {
            return false;
        }
    }
    return true;
}

template<class T, typename std::enable_if<is_container<T>{}, int>::type>
bool ITypesUtil::UnmarshalFromContainer(T &val, MessageParcel &parcel)
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

    val.clear();
    for (size_t i = 0; i < size; i++) {
        typename T::value_type value;
        if (!ITypesUtil::Unmarshalling(value, parcel)) {
            return false;
        }
        val.emplace_back(std::move(value));
    }
    return true;
}

template<typename T>
bool ITypesUtil::MarshalToBuffer(const T &input, int size, MessageParcel &data)
{
    if (size < 0 || static_cast<size_t>(size) > MAX_SIZE || !data.WriteInt32(size)) {
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
    if (size < 0 || static_cast<size_t>(size) > MAX_SIZE || input.size() > MAX_COUNT || !data.WriteInt32(size)) {
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
    if (size < 0 || static_cast<size_t>(size) > MAX_SIZE) {
        return false;
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
    if (size < 0 || static_cast<size_t>(size) > MAX_SIZE) {
        return false;
    }
    int count = data.ReadInt32();
    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(data.ReadRawData(size));
    if (count < 0 || static_cast<size_t>(count) > MAX_COUNT || buffer == nullptr) {
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
    if (!ITypesUtil::Marshalling(first, parcel)) {
        return false;
    }
    return ITypesUtil::Marshal(parcel, others...);
}

template<typename T, typename... Types>
bool ITypesUtil::Unmarshal(MessageParcel &parcel, T &first, Types &...others)
{
    if (!ITypesUtil::Unmarshalling(first, parcel)) {
        return false;
    }
    return ITypesUtil::Unmarshal(parcel, others...);
}
} // namespace OHOS
#endif