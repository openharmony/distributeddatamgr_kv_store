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
#define LOG_TAG "JS_DeviceKVStore"
#include "js_device_kv_store.h"
#include <iomanip>
#include "js_kv_store_resultset.h"
#include "js_query.h"
#include "js_util.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"
#include "distributed_kv_data_manager.h"

using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
namespace OHOS::DistributedKVStore {
constexpr int DEVICEID_WIDTH = 4;
static std::string GetDeviceKey(const std::string& deviceId, const std::string& key)
{
    std::ostringstream oss;
    if (!deviceId.empty()) {
        oss << std::setfill('0') << std::setw(DEVICEID_WIDTH) << deviceId.length() << deviceId;
    }
    oss << key;
    return oss.str();
}

JsDeviceKVStore::JsDeviceKVStore(const std::string& storeId)
    : JsSingleKVStore(storeId)
{
}

napi_value JsDeviceKVStore::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("put", JsSingleKVStore::Put),
        DECLARE_NAPI_FUNCTION("delete", JsSingleKVStore::Delete),
        DECLARE_NAPI_FUNCTION("putBatch", JsSingleKVStore::PutBatch),
        DECLARE_NAPI_FUNCTION("deleteBatch", JsSingleKVStore::DeleteBatch),
        DECLARE_NAPI_FUNCTION("startTransaction", JsSingleKVStore::StartTransaction),
        DECLARE_NAPI_FUNCTION("commit", JsSingleKVStore::Commit),
        DECLARE_NAPI_FUNCTION("rollback", JsSingleKVStore::Rollback),
        DECLARE_NAPI_FUNCTION("enableSync", JsSingleKVStore::EnableSync),
        DECLARE_NAPI_FUNCTION("setSyncRange", JsSingleKVStore::SetSyncRange),
        /* JsDeviceKVStore externs JsSingleKVStore */
        DECLARE_NAPI_FUNCTION("get", JsDeviceKVStore::Get),
        DECLARE_NAPI_FUNCTION("getEntries", JsDeviceKVStore::GetEntries),
        DECLARE_NAPI_FUNCTION("getResultSet", JsDeviceKVStore::GetResultSet),
        DECLARE_NAPI_FUNCTION("getResultSize", JsDeviceKVStore::GetResultSize),
        DECLARE_NAPI_FUNCTION("closeResultSet", JsSingleKVStore::CloseResultSet),
        DECLARE_NAPI_FUNCTION("removeDeviceData", JsSingleKVStore::RemoveDeviceData),
        DECLARE_NAPI_FUNCTION("sync", JsSingleKVStore::Sync),
        DECLARE_NAPI_FUNCTION("on", JsSingleKVStore::OnEvent), /* same to JsSingleKVStore */
        DECLARE_NAPI_FUNCTION("off", JsSingleKVStore::OffEvent) /* same to JsSingleKVStore */
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);

    return JSUtil::DefineClass(env, "DeviceKVStore", properties, count, JsDeviceKVStore::New);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      get(deviceId:string, key:string, callback:AsyncCallback<boolean|string|number|Uint8Array>):void;
 * [Promise]
 *      get(deviceId:string, key:string):Promise<boolean|string|number|Uint8Array>;
 */
napi_value JsDeviceKVStore::Get(napi_env env, napi_callback_info info)
{
    struct GetContext : public ContextBase {
        std::string deviceId;
        std::string key;
        JSUtil::KvStoreVariant value;
    };
    auto ctxt = std::make_shared<GetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // number 2 means: required 2 arguments, <deviceId> + <key>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        if (argc > 1) {
            ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceId);
            ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
                "The parameter deviceId is incorrect.");
        }
        int32_t pos = (argc == 1) ? 0 : 1;
        ctxt->status = JSUtil::GetValue(env, argv[pos], ctxt->key);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, "The type of key must be string.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "DeviceGet exits");

    auto execute = [ctxt]() {
        std::string deviceKey = GetDeviceKey(ctxt->deviceId, ctxt->key);
        OHOS::DistributedKv::Key key(deviceKey);
        OHOS::DistributedKv::Value value;
        auto kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetKvStorePtr();
        ASSERT_STATUS(ctxt, "kvStore->result() failed!");
        bool isSchemaStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->IsSchemaStore();
        Status status = kvStore->Get(key, value);
        ZLOGD("kvStore->Get return %{public}d", status);
        ctxt->value = isSchemaStore ? value.ToString() : JSUtil::Blob2VariantValue(value);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, ctxt->value, result);
        ASSERT_STATUS(ctxt, "output failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

struct VariantArgs {
    /* input arguments' combinations */
    DataQuery dataQuery;
    std::string errMsg = "";
};

static napi_status GetVariantArgs(napi_env env, size_t argc, napi_value* argv, VariantArgs& va)
{
    int32_t pos = (argc == 1) ? 0 : 1;
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, argv[pos], &type);
    if (status != napi_ok||(type != napi_string && type != napi_object)) {
        va.errMsg = "The type of parameters keyPrefix/query is incorrect.";
        return status != napi_ok?status:napi_invalid_arg;
    }
    if (type == napi_string) {
        std::string keyPrefix;
        status = JSUtil::GetValue(env, argv[pos], keyPrefix);
        if (keyPrefix.empty()) {
            va.errMsg = "The type of parameters keyPrefix is incorrect.";
            return napi_invalid_arg;
        }
        va.dataQuery.KeyPrefix(keyPrefix);
    } else {
        bool result = false;
        status = napi_instanceof(env, argv[pos], JsQuery::Constructor(env), &result);
        if ((status == napi_ok) && (result != false)) {
            JsQuery *jsQuery = nullptr;
            status = JSUtil::Unwrap(env, argv[pos], reinterpret_cast<void **>(&jsQuery), JsQuery::Constructor(env));
            if (jsQuery == nullptr) {
                va.errMsg = "The parameters query is incorrect.";
                return napi_invalid_arg;
            }
            va.dataQuery = jsQuery->GetDataQuery();
        } else {
            status = JSUtil::GetValue(env, argv[pos], va.dataQuery);
            ZLOGD("kvStoreDataShare->GetResultSet return %{public}d", status);
        }
    }
    std::string deviceId;
    if (argc > 1) {
        JSUtil::GetValue(env, argv[0], deviceId);
        va.dataQuery.DeviceId(deviceId);
    }
    return status;
};

/*
 * [JS API Prototype]
 *  getEntries(deviceId:string, keyPrefix:string, callback:AsyncCallback<Entry[]>):void
 *  getEntries(deviceId:string, keyPrefix:string):Promise<Entry[]>
 *
 *  getEntries(query:Query, callback:AsyncCallback<Entry[]>):void
 *  getEntries(query:Query) : Promise<Entry[]>
 *
 *  getEntries(deviceId:string, query:Query):callback:AsyncCallback<Entry[]>):void
 *  getEntries(deviceId:string, query:Query):Promise<Entry[]>
 */
napi_value JsDeviceKVStore::GetEntries(napi_env env, napi_callback_info info)
{
    struct GetEntriesContext : public ContextBase {
        VariantArgs va;
        std::vector<Entry> entries;
    };
    auto ctxt = std::make_shared<GetEntriesContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, ctxt->va.errMsg);
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "GetEntries exit");

    auto execute = [ctxt]() {
        auto kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetKvStorePtr();
        Status status = kvStore->GetEntries(ctxt->va.dataQuery, ctxt->entries);
        ZLOGD("kvStore->GetEntries() return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        auto isSchemaStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->IsSchemaStore();
        ctxt->status = JSUtil::SetValue(env, ctxt->entries, result, isSchemaStore);
        ASSERT_STATUS(ctxt, "output failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  getResultSet(deviceId:string, keyPrefix:string, callback:AsyncCallback<KvStoreResultSet>):void
 *  getResultSet(deviceId:string, keyPrefix:string):Promise<KvStoreResultSet>
 *
 *  getResultSet(query:Query, callback:AsyncCallback<KvStoreResultSet>):void
 *  getResultSet(query:Query):Promise<KvStoreResultSet>
 *
 *  getResultSet(deviceId:string, query:Query, callback:AsyncCallback<KvStoreResultSet>):void
 *  getResultSet(deviceId:string, query:Query):Promise<KvStoreResultSet>
 */
napi_value JsDeviceKVStore::GetResultSet(napi_env env, napi_callback_info info)
{
    struct GetResultSetContext : public ContextBase {
        VariantArgs va;
        JsKVStoreResultSet* resultSet = nullptr;
        napi_ref ref = nullptr;
    };
    auto ctxt = std::make_shared<GetResultSetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status != napi_invalid_arg, Status::INVALID_ARGUMENT, ctxt->va.errMsg);
        ctxt->ref = JSUtil::NewWithRef(env, 0, nullptr, reinterpret_cast<void **>(&ctxt->resultSet),
            JsKVStoreResultSet::Constructor(env));
        ASSERT_BUSINESS_ERR(ctxt, ctxt->resultSet != nullptr, Status::INVALID_ARGUMENT,
            "KVStoreResultSet::New failed!");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "GetResultSet exit");

    auto execute = [ctxt]() {
        std::shared_ptr<KvStoreResultSet> kvResultSet;
        auto kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetKvStorePtr();
        Status status = kvStore->GetResultSet(ctxt->va.dataQuery, kvResultSet);
        ZLOGD("kvStore->GetResultSet() return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
        ctxt->resultSet->SetKvStoreResultSetPtr(kvResultSet);
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = napi_get_reference_value(env, ctxt->ref, &result);
        napi_delete_reference(env, ctxt->ref);
        ASSERT_STATUS(ctxt, "output KvResultSet failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  getResultSize(query:Query, callback: AsyncCallback<number>):void
 *  getResultSize(query:Query):Promise<number>
 *
 *  getResultSize(deviceId:string, query:Query, callback: AsyncCallback<number>):void
 *  getResultSize(deviceId:string, query:Query):Promise<number>
 */
napi_value JsDeviceKVStore::GetResultSize(napi_env env, napi_callback_info info)
{
    struct ResultSizeContext : public ContextBase {
        VariantArgs va;
        int resultSize = 0;
    };
    auto ctxt = std::make_shared<ResultSizeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status != napi_invalid_arg, Status::INVALID_ARGUMENT, ctxt->va.errMsg);
    };

    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "GetResultSize exit");

    auto execute = [ctxt]() {
        auto kvStore = reinterpret_cast<JsDeviceKVStore*>(ctxt->native)->GetKvStorePtr();
        Status status = kvStore->GetCount(ctxt->va.dataQuery, ctxt->resultSize);
        ZLOGD("kvStore->GetCount() return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, static_cast<int32_t>(ctxt->resultSize), result);
        ASSERT_STATUS(ctxt, "output resultSize failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

napi_value JsDeviceKVStore::New(napi_env env, napi_callback_info info)
{
    ZLOGD("Constructor single kv store!");
    std::string storeId;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &storeId](size_t argc, napi_value* argv) {
        // required 2 arguments :: <storeId> <options>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 2, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], storeId);
        ASSERT_BUSINESS_ERR(ctxt, (ctxt->status == napi_ok) && !storeId.empty(), Status::INVALID_ARGUMENT,
            "The type of storeId must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "New JsDeviceKVStore exit");

    JsDeviceKVStore* kvStore = new (std::nothrow) JsDeviceKVStore(storeId);
    ASSERT_ERR(env, kvStore != nullptr, Status::INVALID_ARGUMENT, "no memory for kvStore");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGI("deviceKvStore finalize.");
        auto* kvStore = reinterpret_cast<JsDeviceKVStore*>(data);
        ASSERT_VOID(kvStore != nullptr, "finalize null!");
        delete kvStore;
    };
    ASSERT_CALL(env, napi_wrap(env, ctxt->self, kvStore, finalize, nullptr, nullptr), kvStore);
    return ctxt->self;
}
} // namespace OHOS::DistributedKVStore
