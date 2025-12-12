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
#define LOG_TAG "AniKvStoreUtils"
#include "ani_kvstore_utils.h"
#include "ani_utils.h"
#include <algorithm>
#include <endian.h>

#include "log_print.h"

using namespace ::ohos::data::distributedkvstore;
using namespace OHOS::DistributedKVStore;

namespace ani_kvstoreutils {

static constexpr int MAX_STORE_ID_LEN = 128;
static constexpr uint8_t UNVALID_SUBSCRIBE_TYPE = 255;

bool IsValidStoreId(std::string const& storeId)
{
    if (storeId.empty() || storeId.size() > MAX_STORE_ID_LEN) {
        return false;
    }
    auto iter = std::find_if_not(storeId.begin(), storeId.end(),
        [](char c) { return (std::isdigit(c) || std::isalpha(c) || c == '_'); });
    return (iter == storeId.end());
}

bool IsStoreTypeSupported(DistributedKv::Options const& options)
{
    return (options.kvStoreType == DistributedKv::KvStoreType::DEVICE_COLLABORATION)
        || (options.kvStoreType == DistributedKv::KvStoreType::SINGLE_VERSION);
}

bool TaiheSecurityLevelToNative(int32_t level, int32_t &out)
{
    if (level == JS_SECURITY_LEVEL_S1) {
        out = OHOS::DistributedKv::SecurityLevel::S1;
    } else if (level == JS_SECURITY_LEVEL_S2) {
        out = OHOS::DistributedKv::SecurityLevel::S2;
    } else if (level == JS_SECURITY_LEVEL_S3) {
        out = OHOS::DistributedKv::SecurityLevel::S3;
    } else if (level == JS_SECURITY_LEVEL_S4) {
        out = OHOS::DistributedKv::SecurityLevel::S4;
    } else {
        ZLOGE("TaiheSecurityLevelToNative failed, in level = %{public}d", level);
        return false;
    }
    return true;
}

bool SecurityLevelToTaihe(int32_t level, int32_t &out)
{
    if (level == OHOS::DistributedKv::SecurityLevel::S1) {
        out = JS_SECURITY_LEVEL_S1;
    } else if (level == OHOS::DistributedKv::SecurityLevel::S2) {
        out = JS_SECURITY_LEVEL_S2;
    } else if (level == OHOS::DistributedKv::SecurityLevel::S3) {
        out = JS_SECURITY_LEVEL_S3;
    } else if (level == OHOS::DistributedKv::SecurityLevel::S4) {
        out = JS_SECURITY_LEVEL_S4;
    } else {
        ZLOGE("SecurityLevelToTaihe failed, in level = %{public}d", level);
        return false;
    }
    return true;
}

void TaiheValueUnionToNativeVariant(::ohos::data::distributedkvstore::ValueUnion const& value,
    ValueVariant &resultObj)
{
    auto tag = value.get_tag();
    switch (tag) {
        case ValueUnion::tag_t::F64: {
            resultObj = value.get_F64_ref();
        }
            break;
        case ValueUnion::tag_t::INT64: {
            resultObj = value.get_INT64_ref();
        }
            break;
        case ValueUnion::tag_t::STRING: {
            resultObj = std::string(value.get_STRING_ref());
        }
            break;
        case ValueUnion::tag_t::BOOL: {
            resultObj = value.get_BOOL_ref();
        }
            break;
        case ValueUnion::tag_t::UINT8Array: {
            auto &tmp = value.get_UINT8Array_ref();
            std::vector<uint8_t> stdvector(tmp.data(), tmp.data() + tmp.size());
            resultObj = stdvector;
        }
            break;
        default:
            ZLOGE("TaiheValueUnionToNativeVariant unmatched tag");
            break;
    }
}

void TaiheValueToVariant(::ohos::data::distributedkvstore::Value const& value,
    ValueVariant &resultObj)
{
    ::ohos::data::distributedkvstore::ValueUnion valueUnion = value.value;
    auto tag = valueUnion.get_tag();
    switch (tag) {
        case ValueUnion::tag_t::F64: {
            resultObj = valueUnion.get_F64_ref();
        }
            break;
        case ValueUnion::tag_t::INT64: {
            resultObj = valueUnion.get_INT64_ref();
        }
            break;
        case ValueUnion::tag_t::STRING: {
            resultObj = std::string(valueUnion.get_STRING_ref());
        }
            break;
        case ValueUnion::tag_t::BOOL: {
            resultObj = valueUnion.get_BOOL_ref();
        }
            break;
        case ValueUnion::tag_t::UINT8Array: {
            auto &tmp = valueUnion.get_UINT8Array_ref();
            std::vector<uint8_t> stdvector(tmp.data(), tmp.data() + tmp.size());
            resultObj = stdvector;
        }
            break;
        default:
            ZLOGE("TaiheValueToVariant unmatched tag");
            break;
    }
}

void TaiheDataShareValueToVariant(::ohos::data::distributedkvstore::DataShareValueUnion const& value,
    DataShareValueVariant &resultObj)
{
    auto tag = value.get_tag();
    switch (tag) {
        case DataShareValueUnion::tag_t::INT64: {
            resultObj = value.get_INT64_ref();
        }
            break;
        case DataShareValueUnion::tag_t::F64: {
            resultObj = value.get_F64_ref();
        }
            break;
        case DataShareValueUnion::tag_t::STRING: {
            resultObj = std::string(value.get_STRING_ref());
        }
            break;
        case DataShareValueUnion::tag_t::BOOL: {
            resultObj = value.get_BOOL_ref();
        }
            break;
        case DataShareValueUnion::tag_t::UINT8Array: {
            auto &tmp = value.get_UINT8Array_ref();
            std::vector<uint8_t> stdvector(tmp.data(), tmp.data() + tmp.size());
            resultObj = stdvector;
        }
            break;
        default:
            ZLOGE("TaiheDataShareValueToVariant unmatched tag");
            break;
    }
}

DistributedKv::SubscribeType SubscribeTypeToNative(::ohos::data::distributedkvstore::SubscribeType type)
{
    switch (type.get_key()) {
        case ::ohos::data::distributedkvstore::SubscribeType::key_t::SUBSCRIBE_TYPE_LOCAL:
            return DistributedKv::SubscribeType::SUBSCRIBE_TYPE_LOCAL;
        case ::ohos::data::distributedkvstore::SubscribeType::key_t::SUBSCRIBE_TYPE_REMOTE:
            return DistributedKv::SubscribeType::SUBSCRIBE_TYPE_REMOTE;
        case ::ohos::data::distributedkvstore::SubscribeType::key_t::SUBSCRIBE_TYPE_ALL:
            return DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL;
        default:
            return static_cast<DistributedKv::SubscribeType>(UNVALID_SUBSCRIBE_TYPE);
    }
}

DistributedKv::SubscribeType SubscribeTypeToNative(uint8_t type)
{
    switch (type) {
        case 0:  // 0 means SUBSCRIBE_TYPE_LOCAL
            return DistributedKv::SubscribeType::SUBSCRIBE_TYPE_LOCAL;
        case 1:  // 1 means SUBSCRIBE_TYPE_REMOTE
            return DistributedKv::SubscribeType::SUBSCRIBE_TYPE_REMOTE;
        case 2:  // 2 means SUBSCRIBE_TYPE_ALL
            return DistributedKv::SubscribeType::SUBSCRIBE_TYPE_ALL;
        default:
            return DistributedKv::SubscribeType::SUBSCRIBE_TYPE_LOCAL;
    }
}

::ohos::data::distributedkvstore::ValueUnion Blob2TaiheValue(DistributedKv::Blob const& blob, uint8_t &resultType)
{
    auto& data = blob.Data();
    if (data.size() < 1) {
        ZLOGE("Blob have no data!");
        return ::ohos::data::distributedkvstore::ValueUnion::make_STRING(std::string(""));
    }
    resultType = data[0];
    std::vector<uint8_t> real(data.begin() + 1, data.end());
    ZLOGD("Blob::type %{public}d size=%{public}d", static_cast<int>(data[0]), static_cast<int>(real.size()));
    if (data[0] == ani_kvstoreutils::INTEGER) {
        resultType = ani_kvstoreutils::LONG;
        uint32_t tmp4int = be32toh(*reinterpret_cast<uint32_t*>(&(real[0])));
        int32_t tmp = *reinterpret_cast<int32_t*>(&tmp4int);
        return ::ohos::data::distributedkvstore::ValueUnion::make_INT64(tmp);
    } else if (data[0] == ani_kvstoreutils::FLOAT) {
        resultType = ani_kvstoreutils::DOUBLE;
        uint32_t tmp4flt = be32toh(*reinterpret_cast<uint32_t*>(&(real[0])));
        float tmp = *reinterpret_cast<float*>((void*)(&tmp4flt));
        return ::ohos::data::distributedkvstore::ValueUnion::make_F64(tmp);
    } else if (data[0] == ani_kvstoreutils::BYTE_ARRAY) {
        auto tmp = std::vector<uint8_t>(real.begin(), real.end());
        return ::ohos::data::distributedkvstore::ValueUnion::make_UINT8Array(tmp);
    } else if (data[0] == ani_kvstoreutils::BOOLEAN) {
        return ::ohos::data::distributedkvstore::ValueUnion::make_BOOL(real[0]);
    } else if (data[0] == ani_kvstoreutils::DOUBLE) {
        uint64_t tmp4dbl = be64toh(*reinterpret_cast<uint64_t*>(&(real[0])));
        double tmp = *reinterpret_cast<double*>((void*)(&tmp4dbl));
        return ::ohos::data::distributedkvstore::ValueUnion::make_F64(tmp);
    } else if (data[0] == ani_kvstoreutils::STRING) {
        auto tmp = std::string(real.begin(), real.end());
        return ::ohos::data::distributedkvstore::ValueUnion::make_STRING(tmp);
    } else if (data[0] == ani_kvstoreutils::LONG) {
        uint64_t tmp8int = be64toh(*reinterpret_cast<uint64_t*>(&(real[0])));
        int64_t tmp = *reinterpret_cast<int64_t*>(&tmp8int);
        return ::ohos::data::distributedkvstore::ValueUnion::make_INT64(tmp);
    } else {
        // for schema-db, if (data[0] == JSUtil::STRING), no beginning byte!
        resultType = ani_kvstoreutils::STRING;
        auto tmp = std::string(data.begin(), data.end());
        return ::ohos::data::distributedkvstore::ValueUnion::make_STRING(tmp);
    }
}

DistributedKv::Blob VariantValue2Blob(ValueVariant const& value)
{
    std::vector<uint8_t> data;
    auto strValue = std::get_if<std::string>(&value);
    if (strValue != nullptr) {
        data.push_back(ani_kvstoreutils::STRING);
        data.insert(data.end(), (*strValue).begin(), (*strValue).end());
    }
    auto u8ArrayValue = std::get_if<std::vector<uint8_t>>(&value);
    if (u8ArrayValue != nullptr) {
        data.push_back(ani_kvstoreutils::BYTE_ARRAY);
        data.insert(data.end(), (*u8ArrayValue).begin(), (*u8ArrayValue).end());
    }
    auto boolValue = std::get_if<bool>(&value);
    if (boolValue != nullptr) {
        data.push_back(ani_kvstoreutils::BOOLEAN);
        data.push_back(static_cast<uint8_t>(*boolValue));
    }
    uint8_t *res = nullptr;
    auto intValue = std::get_if<int32_t>(&value);
    if (intValue != nullptr) {
        int32_t tmp = *intValue; // copy value, and make it available in stack space.
        uint32_t tmp32 = htobe32(*reinterpret_cast<uint32_t*>(&tmp));
        res = reinterpret_cast<uint8_t*>(&tmp32);
        data.push_back(ani_kvstoreutils::INTEGER);
        data.insert(data.end(), res, res + sizeof(int32_t) / sizeof(uint8_t));
    }
    auto int64Value = std::get_if<int64_t>(&value);
    if (int64Value != nullptr) {
        int64_t tmp = *int64Value; // copy value, and make it available in stack space.
        uint64_t tmp64 = htobe64(*reinterpret_cast<uint64_t*>(&tmp));
        res = reinterpret_cast<uint8_t*>(&tmp64);
        data.push_back(ani_kvstoreutils::LONG);
        data.insert(data.end(), res, res + sizeof(int64_t) / sizeof(uint8_t));
    }
    auto fltValue = std::get_if<float>(&value);
    if (fltValue != nullptr) {
        float tmp = *fltValue; // copy value, and make it available in stack space.
        uint32_t tmp32 = htobe32(*reinterpret_cast<uint32_t*>(&tmp));
        res = reinterpret_cast<uint8_t*>(&tmp32);
        data.push_back(ani_kvstoreutils::FLOAT);
        data.insert(data.end(), res, res + sizeof(float) / sizeof(uint8_t));
    }
    auto dblValue = std::get_if<double>(&value);
    if (dblValue != nullptr) {
        double tmp = *dblValue; // copy value, and make it available in stack space.
        uint64_t tmp64 = htobe64(*reinterpret_cast<uint64_t*>(&tmp));
        res = reinterpret_cast<uint8_t*>(&tmp64);
        data.push_back(ani_kvstoreutils::DOUBLE);
        data.insert(data.end(), res, res + sizeof(double) / sizeof(uint8_t));
    }
    return DistributedKv::Blob(data);
}

std::vector<std::string> StringArrayToNative(::taihe::array_view<::taihe::string> const& para)
{
    std::vector<std::string> result(para.size());
    std::transform(para.begin(), para.end(), result.begin(), [](::taihe::string c) {
        return std::string(c);
    });
    return result;
}

bool EntryArrayToNative(::taihe::array_view<::ohos::data::distributedkvstore::Entry> const& taiheEntries,
    std::vector<DistributedKv::Entry> &out, bool hasSchema)
{
    for (auto iter = taiheEntries.begin(); iter != taiheEntries.end(); iter++) {
        ::ohos::data::distributedkvstore::Entry &item = *iter;
        DistributedKv::Entry kvEntry;
        kvEntry.key = std::string(item.key);
        ani_kvstoreutils::ValueVariant variant;
        TaiheValueToVariant(item.value, variant);
        if (hasSchema) {
            if (!std::holds_alternative<std::string>(variant)) {
                return false;
            }
            kvEntry.value = std::get<std::string>(variant);
        } else {
            kvEntry.value = VariantValue2Blob(variant);
        }
        out.push_back(kvEntry);
    }
    return true;
}

::ohos::data::distributedkvstore::Entry GetEmptyTaiheEntry()
{
    auto taiheStringValueType = ohos::data::distributedkvstore::ValueType::from_value(0);
    ::ohos::data::distributedkvstore::Entry taiEntryEmpty = { std::string(),
        { taiheStringValueType, ::ohos::data::distributedkvstore::ValueUnion::make_STRING(std::string()) }
    };
    return taiEntryEmpty;
}

::ohos::data::distributedkvstore::Entry KvEntryToTaihe(DistributedKv::Entry const& kventry, bool hasSchema)
{
    if (kventry.value.Size() <= 0) {
        auto taiEntryEmpty = GetEmptyTaiheEntry();
        ZLOGW("KvEntryToTaihe ret empty");
        return taiEntryEmpty;
    }
    if (hasSchema) {
        auto strValueType = ohos::data::distributedkvstore::ValueType::from_value(0);
        ::ohos::data::distributedkvstore::Entry taiEntry = { kventry.key.ToString(),
            {strValueType, ::ohos::data::distributedkvstore::ValueUnion::make_STRING(kventry.value.ToString()) }
        };
        return taiEntry;
    } else {
        uint8_t resultType = kventry.value[0];
        auto taiheTemp = Blob2TaiheValue(kventry.value, resultType);
        auto taiheValueType = NativeTypeToTaihe(resultType);
        ::ohos::data::distributedkvstore::Entry taiEntry = { kventry.key.ToString(),
            {taiheValueType, taiheTemp }
        };
        return taiEntry;
    }
}

::taihe::array<::ohos::data::distributedkvstore::Entry> KvEntryArrayToTaihe(
    std::vector<DistributedKv::Entry> const& kventries, bool hasSchema)
{
    std::vector<::ohos::data::distributedkvstore::Entry> taiheEntries;
    std::for_each(kventries.begin(), kventries.end(), [ hasSchema, &taiheEntries ](DistributedKv::Entry c) {
        ::ohos::data::distributedkvstore::Entry taiheItem = ani_kvstoreutils::KvEntryToTaihe(c, hasSchema);
        taiheEntries.push_back(taiheItem);
    });
    return ::taihe::array<::ohos::data::distributedkvstore::Entry>(::taihe::copy_data_t{},
        taiheEntries.data(), taiheEntries.size());
}

::ohos::data::distributedkvstore::ChangeNotification KvChangeNotificationToTaihe(
    DistributedKv::ChangeNotification const& kvNotification, bool hasSchema)
{
    ::ohos::data::distributedkvstore::ChangeNotification result = {{}, {}, {}, ""};
    result.insertEntries = KvEntryArrayToTaihe(kvNotification.GetInsertEntries(), hasSchema);
    result.updateEntries = KvEntryArrayToTaihe(kvNotification.GetUpdateEntries(), hasSchema);
    result.deleteEntries = KvEntryArrayToTaihe(kvNotification.GetDeleteEntries(), hasSchema);
    result.deviceId = kvNotification.GetDeviceId();
    return result;
}

::taihe::map<::taihe::string, int32_t> KvStatusMapToTaiheMap(
    std::map<std::string, DistributedKv::Status> const& mapInfo)
{
    ::taihe::map<::taihe::string, int32_t> result;
    for (auto &[key, value] : mapInfo) {
        ::taihe::string taihekey(key);
        result.emplace(taihekey, value);
    }
    return result;
}

::taihe::array<uintptr_t> KvStatusMapToTaiheArray(ani_env* env,
    std::map<std::string, DistributedKv::Status> const& mapInfo)
{
    if (env == nullptr) {
        return {};
    }
    std::vector<uintptr_t> stdArray;
    for (auto &[key, value] : mapInfo) {
        ani_ref strRef = ani_utils::AniStringUtils::ToAni(env, key);
        ani_object valueObj = {};
        if (ANI_OK != ani_utils::AniCreateInt(env, value, valueObj)) {
            break;
        }
        ani_tuple_value tupleItem {};
        if (!ani_utils::AniCreateTuple(env, strRef, valueObj, tupleItem)) {
            break;
        }
        stdArray.push_back(reinterpret_cast<uintptr_t>(tupleItem));
    }
    return ::taihe::array<uintptr_t>(::taihe::copy_data_t{}, stdArray.data(), stdArray.size());
}

int32_t TaiheValueTypeToNative(int32_t taiheType)
{
    switch (taiheType) {
        case (int32_t)::ohos::data::distributedkvstore::ValueType::key_t::STRING:
            return ani_kvstoreutils::STRING;
            break;
        case (int32_t)::ohos::data::distributedkvstore::ValueType::key_t::BYTE_ARRAY:
            return ani_kvstoreutils::BYTE_ARRAY;
            break;
        case (int32_t)::ohos::data::distributedkvstore::ValueType::key_t::BOOLEAN:
            return ani_kvstoreutils::BOOLEAN;
            break;
        case (int32_t)::ohos::data::distributedkvstore::ValueType::key_t::DOUBLE:
            return ani_kvstoreutils::DOUBLE;
            break;
        case (int32_t)::ohos::data::distributedkvstore::ValueType::key_t::LONG:
            return ani_kvstoreutils::LONG;
            break;
        default: {
            ZLOGE("TaiheValueTypeToNative, unexpected taiheType type %{public}d", taiheType);
            return ani_kvstoreutils::DOUBLE;
        }
            break;
    }
}

::ohos::data::distributedkvstore::ValueType NativeTypeToTaihe(int32_t nativeType)
{
    ::ohos::data::distributedkvstore::ValueType::key_t key = ::ohos::data::distributedkvstore::ValueType::key_t::DOUBLE;
    switch (nativeType) {
        case ani_kvstoreutils::STRING:
            key = ::ohos::data::distributedkvstore::ValueType::key_t::STRING;
            break;
        case ani_kvstoreutils::BYTE_ARRAY:
            key = ::ohos::data::distributedkvstore::ValueType::key_t::BYTE_ARRAY;
            break;
        case ani_kvstoreutils::BOOLEAN:
            key = ::ohos::data::distributedkvstore::ValueType::key_t::BOOLEAN;
            break;
        case ani_kvstoreutils::DOUBLE:
            key = ::ohos::data::distributedkvstore::ValueType::key_t::DOUBLE;
            break;
        case ani_kvstoreutils::LONG:
            key = ::ohos::data::distributedkvstore::ValueType::key_t::LONG;
            break;
        default: {
            ZLOGE("NativeTypeToTaihe, unexpected nativeType type %{public}d", nativeType);
            key = ::ohos::data::distributedkvstore::ValueType::key_t::DOUBLE;
        }
            break;
    }
    return ::ohos::data::distributedkvstore::ValueType(key);
}

} // namespace ani_kvstoreutils