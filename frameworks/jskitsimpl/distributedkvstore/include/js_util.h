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
#ifndef OHOS_JS_UTIL_H
#define OHOS_JS_UTIL_H
#include <cstdint>
#include <map>
#include <variant>
#include "data_query.h"
#include "js_kv_manager.h"
#include "change_notification.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "datashare_abs_predicates.h"
#include "datashare_values_bucket.h"
#include "js_error_utils.h"

namespace OHOS::DistributedKVStore {
class JSUtil final {
public:
    enum {
        /* Blob's first byte is the blob's data ValueType */
        STRING = 0,
        INTEGER = 1,
        FLOAT = 2,
        BYTE_ARRAY = 3,
        BOOLEAN = 4,
        DOUBLE = 5,
        INVALID = 255
    };
	
    enum JsApiType {
        NORMAL = 0,
        DATASHARE = 1
    };
	
    struct StatusMsg {
        napi_status status = napi_ok;
        JsApiType jsApiType = NORMAL;
        StatusMsg(napi_status status) : status(status) {}
        StatusMsg(napi_status status, JsApiType jsApiType) : status(status), jsApiType(jsApiType) {}
        operator napi_status()
        {
            return status;
        }
    };

    struct JsFeatureSpace {
        const char* spaceName;
        const char* nameBase64;
        bool isComponent;
    };
	
    using JsSchema = class JsSchema;
    using Blob = OHOS::DistributedKv::Blob;
    using ChangeNotification = OHOS::DistributedKv::ChangeNotification;
    using Options = OHOS::DistributedKv::Options;
    using Entry = OHOS::DistributedKv::Entry;
    using StoreId = OHOS::DistributedKv::StoreId;
    using Status = OHOS::DistributedKv::Status;
    using DataQuery = OHOS::DistributedKv::DataQuery;
    using ValuesBucket = OHOS::DataShare::DataShareValuesBucket;
    using ValueObject = OHOS::DataShare::DataShareValueObject;
    /* for kvStore Put/Get : boolean|string|number|Uint8Array */
    using KvStoreVariant = std::variant<std::string, int32_t, float, std::vector<uint8_t>, bool, double>;
    using Descriptor = std::function<std::vector<napi_property_descriptor>()>;
	
    static KvStoreVariant Blob2VariantValue(const Blob& blob);
    static Blob VariantValue2Blob(const KvStoreVariant& value);

    /* for query value related : number|string|boolean */
    using QueryVariant = std::variant<std::string, bool, double>;

    static bool IsSystemApi(JsApiType jsApiType);

    static StatusMsg GetValue(napi_env env, napi_value in, napi_value& out);
    static StatusMsg SetValue(napi_env env, napi_value in, napi_value& out);
    /* napi_value <-> bool */
    static StatusMsg GetValue(napi_env env, napi_value in, bool& out);
    static StatusMsg SetValue(napi_env env, const bool& in, napi_value& out);

    /* napi_value <-> int32_t */
    static StatusMsg GetValue(napi_env env, napi_value in, int32_t& out);
    static StatusMsg SetValue(napi_env env, const int32_t& in, napi_value& out);

    /* napi_value <-> uint32_t */
    static StatusMsg GetValue(napi_env env, napi_value in, uint32_t& out);
    static StatusMsg SetValue(napi_env env, const uint32_t& in, napi_value& out);

    /* napi_value <-> int64_t */
    static StatusMsg GetValue(napi_env env, napi_value in, int64_t& out);
    static StatusMsg SetValue(napi_env env, const int64_t& in, napi_value& out);

    /* napi_value <-> double */
    static StatusMsg GetValue(napi_env env, napi_value in, double& out);
    static StatusMsg SetValue(napi_env env, const double& in, napi_value& out);

    /* napi_value <-> std::string */
    static StatusMsg GetValue(napi_env env, napi_value in, std::string& out);
    static StatusMsg SetValue(napi_env env, const std::string& in, napi_value& out);

    /* napi_value <-> KvStoreVariant */
    static StatusMsg GetValue(napi_env env, napi_value in, KvStoreVariant& out);
    static StatusMsg SetValue(napi_env env, const KvStoreVariant& in, napi_value& out);

    /* napi_value <-> QueryVariant */
    static StatusMsg GetValue(napi_env env, napi_value in, QueryVariant& out);
    static StatusMsg SetValue(napi_env env, const QueryVariant& in, napi_value& out);

    /* napi_value <-> std::vector<std::string> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<std::string>& out);
    static StatusMsg SetValue(napi_env env, const std::vector<std::string>& in, napi_value& out);

    /* napi_value <-> std::vector<uint8_t> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<uint8_t>& out);
    static StatusMsg SetValue(napi_env env, const std::vector<uint8_t>& in, napi_value& out);

    /* napi_value <-> std::vector<int32_t> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<int32_t>& out);
    static StatusMsg SetValue(napi_env env, const std::vector<int32_t>& in, napi_value& out);

    /* napi_value <-> std::vector<uint32_t> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<uint32_t>& out);
    static StatusMsg SetValue(napi_env env, const std::vector<uint32_t>& in, napi_value& out);

    /* napi_value <-> std::vector<int64_t> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<int64_t>& out);
    static StatusMsg SetValue(napi_env env, const std::vector<int64_t>& in, napi_value& out);

    /* napi_value <-> std::vector<double> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<double>& out);
    static StatusMsg SetValue(napi_env env, const std::vector<double>& in, napi_value& out);

    /* napi_value <-> ChangeNotification */
    static StatusMsg GetValue(napi_env env, napi_value in, ChangeNotification& out, bool hasSchema);
    static StatusMsg SetValue(napi_env env, const ChangeNotification& in, napi_value& out, bool hasSchema);

    /* napi_value <-> Options */
    static StatusMsg GetValue(napi_env env, napi_value in, Options& out);
    static StatusMsg SetValue(napi_env env, const Options& in, napi_value& out);

    /* napi_value <-> Entry */
    static StatusMsg GetValue(napi_env env, napi_value in, Entry& out, bool hasSchema);
    static StatusMsg SetValue(napi_env env, const Entry& in, napi_value& out, bool hasSchema);

    /* napi_value <-> Options */
    static StatusMsg GetValue(napi_env env, napi_value in, std::list<Entry>& out, bool hasSchema);
    static StatusMsg SetValue(napi_env env, const std::list<Entry>& in, napi_value& out, bool hasSchema);

    /* napi_value <-> std::vector<Entry> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<Entry>& out, bool hasSchema);
    static StatusMsg SetValue(napi_env env, const std::vector<Entry>& in, napi_value& out, bool hasSchema);

    /* napi_value <-> std::vector<StoreId> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<StoreId>& out);
    static StatusMsg SetValue(napi_env env, const std::vector<StoreId>& in, napi_value& out);

    /* napi_value <-> std::map<std::string, Status> */
    static StatusMsg GetValue(napi_env env, napi_value in, std::map<std::string, Status>& out);
    static StatusMsg SetValue(napi_env env, const std::map<std::string, Status>& in, napi_value& out);
    
    static StatusMsg GetValue(napi_env env, napi_value in, JsSchema*& out);

    static StatusMsg GetValue(napi_env env, napi_value in, std::vector<Blob> &out);
    static StatusMsg GetValue(napi_env env, napi_value in, DataQuery &out);

    static StatusMsg GetValue(napi_env env, napi_value jsValue, ValueObject::Type &value);
    static StatusMsg GetValue(napi_env env, napi_value jsValue, ValuesBucket &valuesBucket);

    static StatusMsg GetValue(napi_env env, napi_value in, ContextParam &param);

    static StatusMsg GetCurrentAbilityParam(napi_env env, ContextParam &param);
    /* napi_get_named_property wrapper */
    template <typename T>
    static inline napi_status GetNamedProperty(
        napi_env env, napi_value in, const std::string& prop, T& value, bool optional = false)
    {
        auto [status, jsValue] = GetInnerValue(env, in, prop, optional);
        return (jsValue == nullptr) ? StatusMsg(status) : GetValue(env, jsValue, value);
    };

    static inline bool IsValid(const std::string& storeId)
    {
        if (storeId.empty() || storeId.size() > MAX_STORE_ID_LEN) {
            return false;
        }
        auto iter = std::find_if_not(storeId.begin(), storeId.end(),
            [](char c) { return (std::isdigit(c) || std::isalpha(c) || c == '_'); });
        return (iter == storeId.end());
    }

    static const std::optional<JsFeatureSpace> GetJsFeatureSpace(const std::string &name);
    /* napi_define_class  wrapper */
    static napi_value DefineClass(napi_env env, const std::string &spaceName, const std::string &className,
        const Descriptor &descriptor, napi_callback ctor);
    static napi_value GetClass(napi_env env, const std::string &spaceName, const std::string &className);
    /* napi_new_instance  wrapper */
    static napi_ref NewWithRef(napi_env env, size_t argc, napi_value* argv, void** out, napi_value constructor);

    /* napi_unwrap with napi_instanceof */
    static napi_status Unwrap(napi_env env, napi_value in, void** out, napi_value constructor);

    static bool Equals(napi_env env, napi_value value, napi_ref copy);

    static bool IsSystemApp();

    static bool IsNull(napi_env env, napi_value value);

private:
    enum {
        /* std::map<key, value> to js::tuple<key, value> */
        TUPLE_KEY = 0,
        TUPLE_VALUE,
        TUPLE_SIZE
    };
    static napi_status GetLevel(int32_t level, int32_t &out);
    static std::pair<napi_status, napi_value> GetInnerValue(
        napi_env env, napi_value in, const std::string& prop, bool optional);
    static constexpr int MAX_STORE_ID_LEN = 128;
};
} // namespace OHOS::DistributedKVStore
#endif // OHOS_JS_UTIL_H
