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
#define LOG_TAG "JS_DeviceKVStoreV9"
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
namespace OHOS::DistributedData {
constexpr int DEVICEID_WIDTH = 4;
static std::string GetDeviceKey(const std::string& deviceId, const std::string& key)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(DEVICEID_WIDTH) << deviceId.length();
    oss << deviceId << key;
    return oss.str();
}

JsDeviceKVStoreV9::JsDeviceKVStoreV9(const std::string& storeId)
    : JsKVStoreV9(storeId)
{
}

napi_value JsDeviceKVStoreV9::Constructor(napi_env env)
{
    const napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("put", JsKVStoreV9::Put),
        DECLARE_NAPI_FUNCTION("delete", JsKVStoreV9::Delete),
        DECLARE_NAPI_FUNCTION("putBatch", JsKVStoreV9::PutBatch),
        DECLARE_NAPI_FUNCTION("deleteBatch", JsKVStoreV9::DeleteBatch),
        DECLARE_NAPI_FUNCTION("startTransaction", JsKVStoreV9::StartTransaction),
        DECLARE_NAPI_FUNCTION("commit", JsKVStoreV9::Commit),
        DECLARE_NAPI_FUNCTION("rollback", JsKVStoreV9::Rollback),
        DECLARE_NAPI_FUNCTION("enableSync", JsKVStoreV9::EnableSync),
        DECLARE_NAPI_FUNCTION("setSyncRange", JsKVStoreV9::SetSyncRange),
        /* JsDeviceKVStoreV9 externs JsKVStore */
        DECLARE_NAPI_FUNCTION("get", JsDeviceKVStoreV9::Get),
        DECLARE_NAPI_FUNCTION("getEntries", JsDeviceKVStoreV9::GetEntries),
        DECLARE_NAPI_FUNCTION("getResultSet", JsDeviceKVStoreV9::GetResultSet),
        DECLARE_NAPI_FUNCTION("closeResultSet", JsDeviceKVStoreV9::CloseResultSet),
        DECLARE_NAPI_FUNCTION("getResultSize", JsDeviceKVStoreV9::GetResultSize),
        DECLARE_NAPI_FUNCTION("removeDeviceData", JsDeviceKVStoreV9::RemoveDeviceData),
        DECLARE_NAPI_FUNCTION("sync", JsDeviceKVStoreV9::Sync),
        DECLARE_NAPI_FUNCTION("on", JsKVStoreV9::OnEvent), /* same to JsSingleKVStore */
        DECLARE_NAPI_FUNCTION("off", JsKVStoreV9::OffEvent) /* same to JsSingleKVStore */
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);

    return JSUtil::DefineClass(env, "DeviceKVStoreV9", properties, count, JsDeviceKVStoreV9::New);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      get(deviceId:string, key:string, callback:AsyncCallback<boolean|string|number|Uint8Array>):void;
 * [Promise]
 *      get(deviceId:string, key:string):Promise<boolean|string|number|Uint8Array>;
 */
napi_value JsDeviceKVStoreV9::Get(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::get()");
    struct GetContext : public ContextBase {
        std::string deviceId;
        std::string key;
        JSUtil::KvStoreVariant value;
    };
    auto ctxt = std::make_shared<GetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // number 2 means: required 2 arguments, <deviceId> + <key>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");  
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceId);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of deviceId must be string.");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->key);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The type of key must be string.");
    };
    ctxt->GetCbInfo(env, info, input);
    CHECK_IF_RETURN("DeviceGetV9 exits", ctxt->isThrowError);

    auto execute = [ctxt]() {
        std::string deviceKey = GetDeviceKey(ctxt->deviceId, ctxt->key);
        OHOS::DistributedKv::Key key(deviceKey);
        OHOS::DistributedKv::Value value;
        auto& kvStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->GetNative();
        CHECK_STATUS_RETURN_VOID(ctxt, "kvStore->result() failed!");
        bool isSchemaStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->IsSchemaStore();
        Status status = kvStore->Get(key, value);
        ZLOGD("kvStore->Get return %{public}d", status);
        ctxt->value = isSchemaStore ? value.ToString() : JSUtil::Blob2VariantValue(value);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, ctxt->value, result);
        CHECK_STATUS_RETURN_VOID(ctxt, "output failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

enum class ArgsType : uint8_t {
    /* input arguments' combination type */
    DEVICEID_KEYPREFIX = 0,
    DEVICEID_QUERY,
    QUERY,
    UNKNOWN = 255
};
struct VariantArgs {
    /* input arguments' combinations */
    std::string deviceId;
    std::string keyPrefix;
    JsQuery* query;
    ArgsType type = ArgsType::UNKNOWN;
    DataQuery dataQuery;
    std::string errMsg;
};

static napi_status GetVariantArgs(napi_env env, size_t argc, napi_value* argv, VariantArgs& va)
{
    if (argc == 0) {
        va.errMsg = "The number of parameters is incorrect.";
        return napi_invalid_arg;
    }
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, argv[0], &type);
    if (!(type == napi_string || type == napi_object)) {
        va.errMsg = "The type of parameters keyPrefix/query is incorrect.";
        return napi_invalid_arg;
    }
    if (type == napi_string) {
        // number 2 means: required 2 arguments, <deviceId> <keyPrefix/query>
        if (argc < 2) {
            va.errMsg = "The number of parameters is incorrect.";
            return napi_invalid_arg;
        }
        status = JSUtil::GetValue(env, argv[0], va.deviceId);
        if (va.deviceId.empty()) {
            va.errMsg = "The type of parameters deviceId is incorrect.";
            return napi_invalid_arg;
        }
        status = napi_typeof(env, argv[1], &type);
        if (!(type == napi_string || type == napi_object)) {
            va.errMsg = "The type of parameters keyPrefix/query is incorrect.";
            return napi_invalid_arg;
        }
        if (type == napi_string) {
            status = JSUtil::GetValue(env, argv[1], va.keyPrefix);
            if (va.keyPrefix.empty()) {
                va.errMsg = "The type of parameters keyPrefix is incorrect.";
                return napi_invalid_arg;
            }
            va.type = ArgsType::DEVICEID_KEYPREFIX;
        } else if (type == napi_object) {
            bool result = false;
            status = napi_instanceof(env, argv[1], JsQuery::Constructor(env), &result);
            if ((status == napi_ok) && (result != false)) {
                status = JSUtil::Unwrap(env, argv[1], reinterpret_cast<void**>(&va.query), JsQuery::Constructor(env));
                if (va.query == nullptr) {
                    va.errMsg = "The parameters query is incorrect.";
                    return napi_invalid_arg;
                }
                va.type = ArgsType::DEVICEID_QUERY;
            } else {
                status = JSUtil::GetValue(env, argv[1], va.dataQuery);
                ZLOGD("kvStoreDataShare->GetResultSet return %{public}d", status);
            }
        }
    } else if (type == napi_object) {
        // number 1 means: required 1 arguments, <query>
        if (argc < 1) {
            va.errMsg = "The number of parameters is incorrect.";
            return napi_invalid_arg;
        }
        bool result = false;
        status = napi_instanceof(env, argv[0], JsQuery::Constructor(env), &result);
        if ((status == napi_ok) && (result != false)) {
            status = JSUtil::Unwrap(env, argv[0], reinterpret_cast<void**>(&va.query), JsQuery::Constructor(env));
            if (va.query == nullptr) {
                va.errMsg = "The parameters query is incorrect.";
                return napi_invalid_arg;
            }
            va.type = ArgsType::QUERY;
        } else {
            DeviceInfo info;
            DistributedKvDataManager manager;
            Status daviceStatus = manager.GetLocalDevice(info);
            if (daviceStatus != Status::SUCCESS) {
                ZLOGD("GetLocalDevice return %{public}d", daviceStatus);
            }
            va.deviceId = info.deviceId;
            status = JSUtil::GetValue(env, argv[0], va.dataQuery);
            ZLOGD("GetResultSet arguement[0] return %{public}d", status);
        }
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
napi_value JsDeviceKVStoreV9::GetEntries(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::GetEntries()");
    struct GetEntriesContext : public ContextBase {
        VariantArgs va;
        std::vector<Entry> entries;
    };
    auto ctxt = std::make_shared<GetEntriesContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status != napi_invalid_arg, PARAM_ERROR, ctxt->va.errMsg);
    };
    ctxt->GetCbInfo(env, info, input);
    CHECK_IF_RETURN("GetEntriesV9 exit", ctxt->isThrowError);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->GetNative();
        Status status = Status::INVALID_ARGUMENT;
        if (ctxt->va.type == ArgsType::DEVICEID_KEYPREFIX) {
            std::string deviceKey = GetDeviceKey(ctxt->va.deviceId, ctxt->va.keyPrefix);
            OHOS::DistributedKv::Key keyPrefix(deviceKey);
            status = kvStore->GetEntries(keyPrefix, ctxt->entries);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else if (ctxt->va.type == ArgsType::DEVICEID_QUERY) {
            auto query = ctxt->va.query->GetNative();
            query.DeviceId(ctxt->va.deviceId);
            status = kvStore->GetEntries(query, ctxt->entries);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else if (ctxt->va.type == ArgsType::QUERY) {
            auto query = ctxt->va.query->GetNative();
            status = kvStore->GetEntries(query, ctxt->entries);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        }
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        auto isSchemaStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->IsSchemaStore();
        ctxt->status = JSUtil::SetValue(env, ctxt->entries, result, isSchemaStore);
        CHECK_STATUS_RETURN_VOID(ctxt, "output failed!");
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
napi_value JsDeviceKVStoreV9::GetResultSet(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::GetResultSet()");
    struct GetResultSetContext : public ContextBase {
        VariantArgs va;
        JsKVStoreResultSet* resultSet = nullptr;
        napi_ref ref = nullptr;
    };
    auto ctxt = std::make_shared<GetResultSetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status != napi_invalid_arg, PARAM_ERROR, ctxt->va.errMsg);
        ctxt->ref = JSUtil::NewWithRef(env, 0, nullptr, reinterpret_cast<void**>(&ctxt->resultSet),
            JsKVStoreResultSet::Constructor(env));
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->resultSet != nullptr, PARAM_ERROR, "KVStoreResultSet::New failed!");
    };
    ctxt->GetCbInfo(env, info, input);
    CHECK_IF_RETURN("GetEntriesV9 exit", ctxt->isThrowError);

    auto execute = [ctxt]() {
        std::shared_ptr<KvStoreResultSet> kvResultSet;
        auto& kvStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->GetNative();
        Status status = Status::INVALID_ARGUMENT;
        if (ctxt->va.type == ArgsType::DEVICEID_KEYPREFIX) {
            std::string deviceKey = GetDeviceKey(ctxt->va.deviceId, ctxt->va.keyPrefix);
            OHOS::DistributedKv::Key keyPrefix(deviceKey);
            status = kvStore->GetResultSet(keyPrefix, kvResultSet);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else if (ctxt->va.type == ArgsType::DEVICEID_QUERY) {
            auto query = ctxt->va.query->GetNative();
            query.DeviceId(ctxt->va.deviceId);
            status = kvStore->GetResultSet(query, kvResultSet);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else if (ctxt->va.type == ArgsType::QUERY) {
            auto query = ctxt->va.query->GetNative();
            status = kvStore->GetResultSet(query, kvResultSet);
            ZLOGD("kvStore->GetEntries() return %{public}d", status);
        } else {
            ctxt->va.dataQuery.DeviceId(ctxt->va.deviceId);
            ZLOGD("ArgsType::DEVICEID_PREDICATES ToQuery return %{public}d", status);
            status = kvStore->GetResultSet(ctxt->va.dataQuery, kvResultSet);
            ZLOGD("ArgsType::DEVICEID_PREDICATES GetResultSetWithQuery return %{public}d", status);
        };
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
        ctxt->resultSet->SetNative(kvResultSet);
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = napi_get_reference_value(env, ctxt->ref, &result);
        napi_delete_reference(env, ctxt->ref);
        CHECK_STATUS_RETURN_VOID(ctxt, "output KvResultSet failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  closeResultSet(resultSet:KVStoreResultSet, callback: AsyncCallback<void>):void
 *  closeResultSet(resultSet:KVStoreResultSet):Promise<void>
 */
napi_value JsDeviceKVStoreV9::CloseResultSet(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::CloseResultSet()");
    struct CloseResultSetContext : public ContextBase {
        JsKVStoreResultSet* resultSet = nullptr;
    };
    auto ctxt = std::make_shared<CloseResultSetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <resultSet>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect.");  
        napi_valuetype type = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &type);
        CHECK_THROW_BUSINESS_ERR(ctxt, type == napi_object, PARAM_ERROR, "The type of parameters resultSet is incorrect."); 
        ctxt->status = JSUtil::Unwrap(env, argv[0], reinterpret_cast<void**>(&ctxt->resultSet),
            JsKVStoreResultSet::Constructor(env));
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->resultSet != nullptr, PARAM_ERROR, "The type of parameters resultSet is incorrect."); 
    };
    ctxt->GetCbInfo(env, info, input);
    CHECK_IF_RETURN("CloseResultSetV9 exit", ctxt->isThrowError);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->GetNative();
        Status status = kvStore->CloseResultSet(ctxt->resultSet->GetNative());
        ZLOGD("kvStore->CloseResultSet return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 *  getResultSize(query:Query, callback: AsyncCallback<number>):void
 *  getResultSize(query:Query):Promise<number>
 *
 *  getResultSize(deviceId:string, query:Query, callback: AsyncCallback<number>):void
 *  getResultSize(deviceId:string, query:Query):Promise<number>
 */
napi_value JsDeviceKVStoreV9::GetResultSize(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::GetResultSize()");
    struct ResultSizeContext : public ContextBase {
        VariantArgs va;
        int resultSize = 0;
    };
    auto ctxt = std::make_shared<ResultSizeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status != napi_invalid_arg, PARAM_ERROR, ctxt->va.errMsg);
        CHECK_THROW_BUSINESS_ERR(ctxt, (ctxt->va.type == ArgsType::DEVICEID_QUERY) || (ctxt->va.type == ArgsType::QUERY),
            PARAM_ERROR, "The type of parameters ArgsType is incorrect.");
    };

    ctxt->GetCbInfo(env, info, input);
    CHECK_IF_RETURN("GetResultSizeV9 exit", ctxt->isThrowError);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->GetNative();
        auto query = ctxt->va.query->GetNative();
        if (ctxt->va.type == ArgsType::DEVICEID_QUERY) {
            query.DeviceId(ctxt->va.deviceId);
        }
        Status status = kvStore->GetCount(query, ctxt->resultSize);
        ZLOGD("kvStore->GetCount() return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, static_cast<int32_t>(ctxt->resultSize), result);
        CHECK_STATUS_RETURN_VOID(ctxt, "output resultSize failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  removeDeviceData(deviceId:string, callback: AsyncCallback<void>):void
 *  removeDeviceData(deviceId:string):Promise<void>
 */
napi_value JsDeviceKVStoreV9::RemoveDeviceData(napi_env env, napi_callback_info info)
{
    ZLOGD("DeviceKVStore::RemoveDeviceData()");
    struct RemoveDeviceContext : public ContextBase {
        std::string deviceId;
    };
    auto ctxt = std::make_shared<RemoveDeviceContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <deviceId>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 1, PARAM_ERROR, "The number of parameters is incorrect."); 
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceId);
        CHECK_THROW_BUSINESS_ERR(ctxt, (!ctxt->deviceId.empty()) && (ctxt->status == napi_ok), PARAM_ERROR, "The parameters deviceId is incorrect.");
    };
    ctxt->GetCbInfo(env, info, input);
    CHECK_IF_RETURN("CloseResultSetV9 exit", ctxt->isThrowError);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->GetNative();
        Status status = kvStore->RemoveDeviceData(ctxt->deviceId);
        ZLOGD("kvStore->RemoveDeviceData return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 *  sync(deviceIdList:string[], mode:SyncMode, allowedDelayMs?:number):void
 */
napi_value JsDeviceKVStoreV9::Sync(napi_env env, napi_callback_info info)
{
    struct SyncContext : public ContextBase {
        std::vector<std::string> deviceIdList;
        uint32_t mode = 0;
        uint32_t allowedDelayMs = 0;
        JsQuery* query = nullptr;
        napi_valuetype type = napi_undefined;
    };
    auto ctxt = std::make_shared<SyncContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 3 arguments :: <deviceIdList> <mode> [allowedDelayMs]
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceIdList);
        CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The parameters deviceIdList is incorrect.");  
        napi_typeof(env, argv[1], &ctxt->type);
        if (ctxt->type == napi_object) {
            ctxt->status = JSUtil::Unwrap(env,
                argv[1], reinterpret_cast<void**>(&ctxt->query), JsQuery::Constructor(env));
            CHECK_THROW_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, PARAM_ERROR, "The parameters mode is incorrect.");
            ctxt->status = JSUtil::GetValue(env, argv[2], ctxt->mode);
        }
        if (ctxt->type == napi_number) {
            ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->mode);
            if (argc == 3) {
                ctxt->status = JSUtil::GetValue(env, argv[2], ctxt->allowedDelayMs);
            }
        }
        CHECK_THROW_BUSINESS_ERR(ctxt, (ctxt->mode <= uint32_t(SyncMode::PUSH_PULL)) && (ctxt->status == napi_ok), PARAM_ERROR,
            "The parameters mode is incorrect.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("DeviceSyncV9 exit", ctxt->isThrowError);

    auto& kvStore = reinterpret_cast<JsDeviceKVStoreV9*>(ctxt->native)->GetNative();
    Status status = Status::INVALID_ARGUMENT;
    if (ctxt->type == napi_object) {
        auto query = ctxt->query->GetNative();
        status = kvStore->Sync(ctxt->deviceIdList, static_cast<SyncMode>(ctxt->mode), query, nullptr);
    }
    if (ctxt->type == napi_number) {
        status = kvStore->Sync(ctxt->deviceIdList, static_cast<SyncMode>(ctxt->mode), ctxt->allowedDelayMs);
    }
    ZLOGD("kvStore->Sync return %{public}d!", status);
    ThrowNapiError(env, status, "", false);
    return nullptr;
}

napi_value JsDeviceKVStoreV9::New(napi_env env, napi_callback_info info)
{
    ZLOGD("Constructor single kv store!");
    std::string storeId;
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt, &storeId](size_t argc, napi_value* argv) {
        // required 2 arguments :: <storeId> <options>
        CHECK_THROW_BUSINESS_ERR(ctxt, argc >= 2, PARAM_ERROR, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], storeId);
        CHECK_THROW_BUSINESS_ERR(ctxt, (ctxt->status == napi_ok) && !storeId.empty(), PARAM_ERROR,
            "The type of storeId must be string.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    CHECK_IF_RETURN("DeviceSyncV9 exit", ctxt->isThrowError);

    JsDeviceKVStoreV9* kvStore = new (std::nothrow) JsDeviceKVStoreV9(storeId);
    if (kvStore == nullptr) {
        ThrowNapiError(env, PARAM_ERROR, "no memory for kvStore.");
        return nullptr;
    }

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGD("deviceKvStore finalize.");
        auto* kvStore = reinterpret_cast<JsDeviceKVStoreV9*>(data);
        CHECK_RETURN_VOID(kvStore != nullptr, "finalize null!");
        delete kvStore;
    };
    NAPI_CALL(env, napi_wrap(env, ctxt->self, kvStore, finalize, nullptr, nullptr));
    return ctxt->self;
}
} // namespace OHOS::DistributedData
