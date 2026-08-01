/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define LOG_TAG "NapiDataShareAdapter"
#include "js_kv_store_resultset.h"
#include "js_util.h"
#include "js_proxy.h"
#include "kv_utils.h"
#include "log_print.h"
#include "datashare_values_bucket.h"

using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
namespace OHOS::DistributedKVStore {

static JSUtil::StatusMsg GetValueValueObject(
    napi_env env, napi_value jsValue, DataShare::DataShareValueObject::Type &valueObject)
{
    napi_valuetype type = napi_undefined;
    napi_typeof(env, jsValue, &type);
    if (type == napi_string) {
        std::string value;
        JSUtil::GetValue(env, jsValue, value);
        valueObject = value;
    } else if (type == napi_number) {
        double value = 0.0;
        napi_get_value_double(env, jsValue, &value);
        valueObject = value;
    } else if (type == napi_boolean) {
        bool value = false;
        napi_get_value_bool(env, jsValue, &value);
        valueObject = value;
    } else if (type == napi_object) {
        std::vector<uint8_t> value;
        JSUtil::GetValue(env, jsValue, value);
        valueObject = std::move(value);
    }
    return napi_ok;
}

JSUtil::StatusMsg JSUtil::GetValue(napi_env env, napi_value jsValue, DataShare::DataShareValuesBucket &valuesBucket)
{
    napi_value keys = 0;
    napi_get_property_names(env, jsValue, &keys);
    uint32_t arrLen = 0;
    JSUtil::StatusMsg statusMsg = napi_get_array_length(env, keys, &arrLen);
    if (statusMsg.status != napi_ok) {
        return statusMsg;
    }
    for (size_t i = 0; i < arrLen; ++i) {
        napi_value jsKey = 0;
        statusMsg.status = napi_get_element(env, keys, i, &jsKey);
        ASSERT((statusMsg.status == napi_ok), "no element", statusMsg);
        std::string key;
        JSUtil::GetValue(env, jsKey, key);
        napi_value valueJs = 0;
        napi_get_property(env, jsValue, jsKey, &valueJs);
        GetValueValueObject(env, valueJs, valuesBucket.valuesMap[key]);
    }
    return napi_ok;
}

std::shared_ptr<ResultSetBridge> JsKVStoreResultSet::CreateBridge(std::shared_ptr<KvStoreResultSet> instance)
{
    return KvUtils::ToResultSetBridge(instance);
}

JSUtil::StatusMsg JSUtil::GetValue(napi_env env, napi_value in, std::vector<Blob> &out)
{
    ZLOGD("napi_value -> std::GetValue Blob");
    out.clear();
    napi_valuetype type = napi_undefined;
    JSUtil::StatusMsg statusMsg = napi_typeof(env, in, &type);
    ASSERT((statusMsg.status == napi_ok) && (type == napi_object), "invalid type", napi_invalid_arg);
    JSProxy::JSProxy<DataShare::DataShareAbsPredicates> *jsProxy = nullptr;
    napi_unwrap(env, in, reinterpret_cast<void **>(&jsProxy));
    ASSERT((jsProxy != nullptr && jsProxy->GetInstance() != nullptr), "invalid type", napi_invalid_arg);
    std::vector<OHOS::DistributedKv::Key> keys;
    statusMsg.status = napi_invalid_arg;
    Status status = OHOS::DistributedKv::KvUtils::GetKeys(*(jsProxy->GetInstance()), keys);
    if (status == Status::SUCCESS) {
        ZLOGD("napi_value —> GetValue Blob ok");
        out = keys;
        statusMsg.status = napi_ok;
        statusMsg.jsApiType = DATASHARE;
    }
    return statusMsg;
}

JSUtil::StatusMsg JSUtil::GetValue(napi_env env, napi_value in, DataQuery &query)
{
    ZLOGD("napi_value -> std::GetValue DataQuery");
    napi_valuetype type = napi_undefined;
    napi_status nstatus = napi_typeof(env, in, &type);
    ASSERT((nstatus == napi_ok) && (type == napi_object), "invalid type", napi_invalid_arg);
    JSProxy::JSProxy<DataShare::DataShareAbsPredicates> *jsProxy = nullptr;
    napi_unwrap(env, in, reinterpret_cast<void **>(&jsProxy));
    ASSERT((jsProxy != nullptr && jsProxy->GetInstance() != nullptr), "invalid type", napi_invalid_arg);
    Status status = OHOS::DistributedKv::KvUtils::ToQuery(*(jsProxy->GetInstance()), query);
    if (status != Status::SUCCESS) {
        ZLOGD("napi_value -> GetValue DataQuery failed ");
    }
    return nstatus;
}

JSUtil::StatusMsg GetEntry(
    napi_env env, napi_value item, OHOS::DistributedKv::Entry &entry, bool hasSchema)
{
    OHOS::DataShare::DataShareValuesBucket values;
    JSUtil::StatusMsg statusMsg = JSUtil::GetValue(env, item, values);
    if (statusMsg.status != napi_ok) {
        return statusMsg;
    }
    entry = OHOS::DistributedKv::KvUtils::ToEntry(values);
    entry.key = std::vector<uint8_t>(entry.key.Data().begin(), entry.key.Data().end());
    if (hasSchema) {
        entry.value = std::vector<uint8_t>(entry.value.Data().begin() + 1, entry.value.Data().end());
    }
    return statusMsg;
}

} // namespace OHOS::DistributedKVStore