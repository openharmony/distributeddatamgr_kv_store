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
#define LOG_TAG "JS_SingleKVStore"
#include "js_single_kv_store.h"
#include "js_util.h"
#include "js_kv_store_resultset.h"
#include "datashare_predicates.h"
#include "js_query.h"
#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"
#include "kv_utils.h"

using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
namespace OHOS::DistributedKVStore {

std::map<std::string, JsSingleKVStore::Exec> JsSingleKVStore::onEventHandlers_ = {
    { "dataChange", JsSingleKVStore::OnDataChange },
    { "syncComplete", JsSingleKVStore::OnSyncComplete }
};

std::map<std::string, JsSingleKVStore::Exec> JsSingleKVStore::offEventHandlers_ = {
    { "dataChange", JsSingleKVStore::OffDataChange },
    { "syncComplete", JsSingleKVStore::OffSyncComplete }
};

std::map<napi_valuetype, std::string> JsSingleKVStore::valueTypeToString_ = {
    { napi_string, std::string("string") },
    { napi_number, std::string("integer") },
    { napi_object, std::string("bytearray") },
    { napi_boolean, std::string("bollean") },
};

static bool ValidSubscribeType(uint8_t type)
{
    return (SUBSCRIBE_LOCAL <= type) && (type <= SUBSCRIBE_LOCAL_REMOTE);
}

static SubscribeType ToSubscribeType(uint8_t type)
{
    return static_cast<SubscribeType>(type + 1);
}

JsSingleKVStore::JsSingleKVStore(const std::string& storeId)
    : storeId_(storeId)
{
}

void JsSingleKVStore::SetKvStorePtr(std::shared_ptr<SingleKvStore> kvStore)
{
    kvStore_ = kvStore;
}

std::shared_ptr<SingleKvStore> JsSingleKVStore::GetKvStorePtr()
{
    return kvStore_;
}

void JsSingleKVStore::SetContextParam(std::shared_ptr<ContextParam> param)
{
    param_ = param;
}

void JsSingleKVStore::SetUvQueue(std::shared_ptr<UvQueue> uvQueue)
{
    uvQueue_ = uvQueue;
}

bool JsSingleKVStore::IsSchemaStore() const
{
    return isSchemaStore_;
}

void JsSingleKVStore::SetSchemaInfo(bool isSchemaStore)
{
    isSchemaStore_ = isSchemaStore;
}

JsSingleKVStore::~JsSingleKVStore()
{
    ZLOGD("no memory leak for JsSingleKVStore");
    if (kvStore_ == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lck(listMutex_);
    for (uint8_t type = SUBSCRIBE_LOCAL; type < SUBSCRIBE_COUNT; type++) {
        for (auto& observer : dataObserver_[type]) {
            auto subscribeType = ToSubscribeType(type);
            kvStore_->UnSubscribeKvStore(subscribeType, observer);
            observer->Clear();
        }
        dataObserver_[type].clear();
    }

    kvStore_->UnRegisterSyncCallback();
    for (auto &syncObserver : syncObservers_) {
        syncObserver->Clear();
    }
    syncObservers_.clear();
}

napi_value JsSingleKVStore::Constructor(napi_env env)
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
        DECLARE_NAPI_FUNCTION("backup", JsSingleKVStore::Backup),
        DECLARE_NAPI_FUNCTION("restore", JsSingleKVStore::Restore),
        DECLARE_NAPI_FUNCTION("deleteBackup", JsSingleKVStore::DeleteBackup),

        DECLARE_NAPI_FUNCTION("get", JsSingleKVStore::Get),
        DECLARE_NAPI_FUNCTION("getEntries", JsSingleKVStore::GetEntries),
        DECLARE_NAPI_FUNCTION("getResultSet", JsSingleKVStore::GetResultSet),
        DECLARE_NAPI_FUNCTION("closeResultSet", JsSingleKVStore::CloseResultSet),
        DECLARE_NAPI_FUNCTION("getResultSize", JsSingleKVStore::GetResultSize),
        DECLARE_NAPI_FUNCTION("removeDeviceData", JsSingleKVStore::RemoveDeviceData),
        DECLARE_NAPI_FUNCTION("sync", JsSingleKVStore::Sync),
        DECLARE_NAPI_FUNCTION("setSyncParam", JsSingleKVStore::SetSyncParam),
        DECLARE_NAPI_FUNCTION("getSecurityLevel", JsSingleKVStore::GetSecurityLevel),
        DECLARE_NAPI_FUNCTION("on", JsSingleKVStore::OnEvent), /* same to JsDeviceKVStore */
        DECLARE_NAPI_FUNCTION("off", JsSingleKVStore::OffEvent) /* same to JsDeviceKVStore */
    };
    size_t count = sizeof(properties) / sizeof(properties[0]);
    return JSUtil::DefineClass(env, "SingleKVStore", properties, count, JsSingleKVStore::New);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      put(key:string, value:Uint8Array | string | boolean | number, callback: AsyncCallback<void>):void;
 * [Promise]
 *      put(key:string, value:Uint8Array | string | boolean | number):Promise<void>;
 */
napi_value JsSingleKVStore::Put(napi_env env, napi_callback_info info)
{
    struct PutContext : public ContextBase {
        std::string key;
        JSUtil::KvStoreVariant value;
    };
    auto ctxt = std::make_shared<PutContext>();
    ctxt->GetCbInfo(env, info, [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <key> <value>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 2, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->key);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, "The type of key must be string.");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->value);
        if (ctxt->status != napi_ok) {
            ctxt->isThrowError = true;
            napi_valuetype ntype = napi_undefined;
            napi_typeof(env, argv[0], &ntype);
            auto type = valueTypeToString_.find(ntype);
            ThrowNapiError(env, Status::INVALID_ARGUMENT, "The type of value must be " + type->second);
            return;
        }
    });
    ASSERT_NULL(!ctxt->isThrowError, "Put exit");

    auto execute = [ctxt]() {
        DistributedKv::Key key(ctxt->key);
        bool isSchemaStore = reinterpret_cast<JsSingleKVStore *>(ctxt->native)->IsSchemaStore();
        auto &kvStore = reinterpret_cast<JsSingleKVStore *>(ctxt->native)->kvStore_;
        DistributedKv::Value value = isSchemaStore ? DistributedKv::Blob(std::get<std::string>(ctxt->value))
                                                   : JSUtil::VariantValue2Blob(ctxt->value);
        Status status = kvStore->Put(key, value);
        ZLOGD("kvStore->Put return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      delete(key: string, callback: AsyncCallback<void>): void;
 * [Promise]
 *      delete(key: string): Promise<void>;
 */
napi_value JsSingleKVStore::Delete(napi_env env, napi_callback_info info)
{
    struct DeleteContext : public ContextBase {
        std::string key;
        std::vector<DistributedKv::Blob> keys;
        napi_valuetype type;
    };
    auto ctxt = std::make_shared<DeleteContext>();
    ctxt->GetCbInfo(env, info, [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <key> || <predicates>
        ASSERT_BUSINESS_ERR(ctxt, argc == 1, Status::INVALID_ARGUMENT, "The number of parameter is incorrect.");
        ctxt->type = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &(ctxt->type));
        if (ctxt->type == napi_string) {
            ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->key);
            ZLOGD("kvStore->Delete %{public}.6s  status:%{public}d", ctxt->key.c_str(), ctxt->status);
            ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
                "The type of key must be string.");
        } else if (ctxt->type == napi_object) {
            ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->keys);
            ZLOGD("kvStore->Delete status:%{public}d", ctxt->status);
            ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
                "The parameters predicates is incorrect.");
        }
    });
    ASSERT_NULL(!ctxt->isThrowError, "Delete exit");

    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), [ctxt]() {
        Status status = Status::INVALID_ARGUMENT;
        if (ctxt->type == napi_string) {
            OHOS::DistributedKv::Key key(ctxt->key);
            auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
            status = kvStore->Delete(key);
            ZLOGD("kvStore->Delete %{public}.6s status:%{public}d", ctxt->key.c_str(), status);
        } else if (ctxt->type == napi_object) {
            auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
            status = kvStore->DeleteBatch(ctxt->keys);
            ZLOGD("kvStore->DeleteBatch status:%{public}d", status);
        }
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    });
}

/*
 * [JS API Prototype]
 * [Callback]
 *      on(event:'syncComplete',syncCallback: Callback<Array<[string, number]>>):void;
 *      on(event:'dataChange', subType: SubscribeType, observer: Callback<ChangeNotification>): void;
 */
napi_value JsSingleKVStore::OnEvent(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <event> [...] <callback>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 2, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        std::string event;
        ctxt->status = JSUtil::GetValue(env, argv[0], event);
        ZLOGI("subscribe to event:%{public}s", event.c_str());
        auto handle = onEventHandlers_.find(event);
        ASSERT_BUSINESS_ERR(ctxt, handle != onEventHandlers_.end(), Status::INVALID_ARGUMENT,
            "The type of parameters event is incorrect.");
        // shift 1 argument, for JsSingleKVStore::Exec.
        handle->second(env, argc - 1, &argv[1], ctxt);
    };
    ctxt->GetCbInfoSync(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "OnEvent exit");
    if (ctxt->status != napi_ok) {
        ThrowNapiError(env, Status::INVALID_ARGUMENT, "");
    }
    return nullptr;
}

/*
 * [JS API Prototype]
 * [Callback]
 *      off(event:'syncComplete',syncCallback: Callback<Array<[string, number]>>):void;
 *      off(event:'dataChange', subType: SubscribeType, observer: Callback<ChangeNotification>): void;
 */
napi_value JsSingleKVStore::OffEvent(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <event> [callback]
        ASSERT_BUSINESS_ERR(ctxt, argc != 0, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        std::string event;
        ctxt->status = JSUtil::GetValue(env, argv[0], event);
        ZLOGI("unsubscribe to event:%{public}s", event.c_str());
        auto handle = offEventHandlers_.find(event);
        ASSERT_BUSINESS_ERR(ctxt, handle != offEventHandlers_.end(), Status::INVALID_ARGUMENT,
            "The type of parameters event is incorrect.");
        // shift 1 argument, for JsSingleKVStore::Exec.
        handle->second(env, argc - 1, &argv[1], ctxt);
    };
    ctxt->GetCbInfoSync(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "OffEvent exit");
    if (ctxt->status != napi_ok) {
        ThrowNapiError(env, Status::INVALID_ARGUMENT, "");
    }
    return nullptr;
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      putBatch(entries: Entry[], callback: AsyncCallback<void>):void;
 * [Promise]
 *      putBatch(entries: Entry[]):Promise<void>;
 */
napi_value JsSingleKVStore::PutBatch(napi_env env, napi_callback_info info)
{
    struct PutBatchContext : public ContextBase {
        std::vector<Entry> entries;
    };
    auto ctxt = std::make_shared<PutBatchContext>();
    ctxt->GetCbInfo(env, info, [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <entries>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        auto isSchemaStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->IsSchemaStore();
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->entries, isSchemaStore);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The type of entries is incorrect.");
    });
    ASSERT_NULL(!ctxt->isThrowError, "PutBatch exit");

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->PutBatch(ctxt->entries);
        ZLOGD("kvStore->DeleteBatch return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      deleteBatch(keys: string[], callback: AsyncCallback<void>):void;
 * [Promise]
 *      deleteBatch(keys: string[]):Promise<void>;
 */
napi_value JsSingleKVStore::DeleteBatch(napi_env env, napi_callback_info info)
{
    struct DeleteBatchContext : public ContextBase {
        std::vector<std::string> keys;
    };
    auto ctxt = std::make_shared<DeleteBatchContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <keys>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->keys);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, "The type of keys is incorrect.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "DeleteBatch exit");
    
    auto execute = [ctxt]() {
        std::vector<DistributedKv::Key> keys;
        for (auto it : ctxt->keys) {
            DistributedKv::Key key(it);
            keys.push_back(key);
        }
        auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->DeleteBatch(keys);
        ZLOGD("kvStore->DeleteBatch return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      startTransaction(callback: AsyncCallback<void>):void;
 * [Promise]
 *      startTransaction() : Promise<void>;
 */
napi_value JsSingleKVStore::StartTransaction(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfo(env, info);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->StartTransaction();
        ZLOGD("kvStore->StartTransaction return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      commit(callback: AsyncCallback<void>):void;
 * [Promise]
 *      commit() : Promise<void>;
 */
napi_value JsSingleKVStore::Commit(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfo(env, info);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->Commit();
        ZLOGD("kvStore->Commit return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      rollback(callback: AsyncCallback<void>):void;
 * [Promise]
 *      rollback() : Promise<void>;
 */
napi_value JsSingleKVStore::Rollback(napi_env env, napi_callback_info info)
{
    auto ctxt = std::make_shared<ContextBase>();
    ctxt->GetCbInfo(env, info);

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->Rollback();
        ZLOGD("kvStore->Commit return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      enableSync(enabled:boolean, callback: AsyncCallback<void>):void;
 * [Promise]
 *      enableSync(enabled:boolean) : Promise<void>;
 */
napi_value JsSingleKVStore::EnableSync(napi_env env, napi_callback_info info)
{
    struct EnableSyncContext : public ContextBase {
        bool enable = false;
    };
    auto ctxt = std::make_shared<EnableSyncContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <enable>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = napi_get_value_bool(env, argv[0], &ctxt->enable);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The parameters of enable is incorrect.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "EnableSync exit");
    
    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->SetCapabilityEnabled(ctxt->enable);
        ZLOGD("kvStore->SetCapabilityEnabled return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      setSyncRange(localLabels:string[], remoteSupportLabels:string[], callback: AsyncCallback<void>):void;
 * [Promise]
 *      setSyncRange(localLabels:string[], remoteSupportLabels:string[]) : Promise<void>;
 */
napi_value JsSingleKVStore::SetSyncRange(napi_env env, napi_callback_info info)
{
    struct SyncRangeContext : public ContextBase {
        std::vector<std::string> localLabels;
        std::vector<std::string> remoteSupportLabels;
    };
    auto ctxt = std::make_shared<SyncRangeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 2 arguments :: <localLabels> <remoteSupportLabels>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 2, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->localLabels);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The type of parameter localLabels is string array.");
        ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->remoteSupportLabels);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The type of parameter remoteSupportLabels is string array.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "SetSyncRange exit");

    auto execute = [ctxt]() {
        auto& kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->kvStore_;
        Status status = kvStore->SetCapabilityRange(ctxt->localLabels, ctxt->remoteSupportLabels);
        ZLOGD("kvStore->SetCapabilityRange return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      backup(file:string, callback: AsyncCallback<void>):void;
 * [Promise]
 *      backup(file:string): Promise<void>;
 */
napi_value JsSingleKVStore::Backup(napi_env env, napi_callback_info info)
{
    struct BackupContext : public ContextBase {
        std::string file;
    };
    auto ctxt = std::make_shared<BackupContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <file>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->file);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The type of parameter file is string.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "Backup exit");
    
    auto execute = [ctxt]() {
        auto jsKvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native);
        Status status = jsKvStore->kvStore_->Backup(ctxt->file, jsKvStore->param_->baseDir);
        ZLOGD("kvStore->Backup return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      restore(file:string, callback: AsyncCallback<void>):void;
 * [Promise]
 *      restore(file:string): Promise<void>;
 */
napi_value JsSingleKVStore::Restore(napi_env env, napi_callback_info info)
{
    struct RestoreContext : public ContextBase {
        std::string file;
    };
    auto ctxt = std::make_shared<RestoreContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <file>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->file);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The type of parameter file is string.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "Restore exit");
    
    auto execute = [ctxt]() {
        auto jsKvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native);
        Status status = jsKvStore->kvStore_->Restore(ctxt->file, jsKvStore->param_->baseDir);
        ZLOGD("kvStore->Restore return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      deleteBackup(files:Array<string>, callback: AsyncCallback<Array<[string, number]>>):void;
 * [Promise]
 *      deleteBackup(files:Array<string>): Promise<Array<[string, number]>>;
 */
napi_value JsSingleKVStore::DeleteBackup(napi_env env, napi_callback_info info)
{
    struct DeleteBackupContext : public ContextBase {
        std::vector<std::string> files;
        std::map<std::string, DistributedKv::Status> results;
    };
    auto ctxt = std::make_shared<DeleteBackupContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <files>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->files);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The type of parameter file is string.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "DeleteBackup exit");
    
    auto execute = [ctxt]() {
        auto jsKvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native);
        Status status = jsKvStore->kvStore_->DeleteBackup(ctxt->files,
            jsKvStore->param_->baseDir, ctxt->results);
        ZLOGD("kvStore->DeleteBackup return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, ctxt->results, result);
        ASSERT_STATUS(ctxt, "output failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype] JsSingleKVStore::OnDataChange is private non-static.
 * [Callback]
 *      on(event:'dataChange', subType: SubscribeType, observer: Callback<ChangeNotification>): void;
 */
void JsSingleKVStore::OnDataChange(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt)
{
    // required 2 arguments :: <SubscribeType> <observer>
    ASSERT_BUSINESS_ERR(ctxt, argc >= 2, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");

    int32_t type = SUBSCRIBE_COUNT;
    ctxt->status = napi_get_value_int32(env, argv[0], &type);
    ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, "");
    ASSERT_BUSINESS_ERR(ctxt, ValidSubscribeType(type), Status::INVALID_ARGUMENT,
        "The type of parameter event is incorrect.");

    napi_valuetype valueType = napi_undefined;
    ctxt->status = napi_typeof(env, argv[1], &valueType);
    ASSERT_BUSINESS_ERR(ctxt, (ctxt->status == napi_ok) && (valueType == napi_function), Status::INVALID_ARGUMENT,
        "The type of parameter Callback is incorrect.");
 
    ZLOGI("subscribe data change type %{public}d", type);
    auto proxy = reinterpret_cast<JsSingleKVStore*>(ctxt->native);
    std::lock_guard<std::mutex> lck(proxy->listMutex_);
    for (auto& it : proxy->dataObserver_[type]) {
        if (JSUtil::Equals(env, argv[1], it->GetCallback())) {
            ZLOGI("function is already subscribe type");
            return;
        }
    }

    Status status = proxy->Subscribe(type,
                                     std::make_shared<DataObserver>(proxy->uvQueue_, argv[1], proxy->IsSchemaStore()));
    ThrowNapiError(env, status, "", false);
}

/*
 * [JS API Prototype] JsSingleKVStore::OffDataChange is private non-static.
 * [Callback]
 *      on(event:'dataChange', subType: SubscribeType, observer: Callback<ChangeNotification>): void;
 * [NOTES!!!]  no SubscribeType while off...
 *      off(event:'dataChange', observer: Callback<ChangeNotification>): void;
 */
void JsSingleKVStore::OffDataChange(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt)
{
    // required 1 arguments :: [callback]
    ASSERT_BUSINESS_ERR(ctxt, argc <= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
    // have 1 arguments :: have the callback
    if (argc == 1) {
        napi_valuetype valueType = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &valueType);
        ASSERT_BUSINESS_ERR(ctxt, (ctxt->status == napi_ok) && (valueType == napi_function), Status::INVALID_ARGUMENT,
            "The type of parameter Callback is incorrect.");
    }

    ZLOGI("unsubscribe dataChange, %{public}s specified observer.", (argc == 0) ? "without": "with");

    auto proxy = reinterpret_cast<JsSingleKVStore*>(ctxt->native);
    bool found = false;
    Status status = Status::SUCCESS;
    auto traverseType = [argc, argv, proxy, env, &found, &status](uint8_t type, auto& observers) {
        auto it = observers.begin();
        while (it != observers.end()) {
            if ((argc == 1) && !JSUtil::Equals(env, argv[0], (*it)->GetCallback())) {
                ++it;
                continue; // specified observer and not current iterator
            }
            found = true;
            status = proxy->UnSubscribe(type, *it);
            if (status != Status::SUCCESS) {
                break; // stop on fail.
            }
            it = observers.erase(it);
        }
    };

    std::lock_guard<std::mutex> lck(proxy->listMutex_);
    for (uint8_t type = SUBSCRIBE_LOCAL; type < SUBSCRIBE_COUNT; type++) {
        traverseType(type, proxy->dataObserver_[type]);
        if (status != Status::SUCCESS) {
            break; // stop on fail.
        }
    }
    ASSERT_BUSINESS_ERR(ctxt, found || (argc == 0), Status::INVALID_ARGUMENT, "not Subscribed!");
    ThrowNapiError(env, status, "", false);
}

/*
 * [JS API Prototype] JsSingleKVStore::OnSyncComplete is private non-static.
 * [Callback]
 *      on(event:'syncComplete',syncCallback: Callback<Array<[string, number]>>):void;
 */
void JsSingleKVStore::OnSyncComplete(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt)
{
    // required 1 arguments :: <callback>
    ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
    napi_valuetype valueType = napi_undefined;
    ctxt->status = napi_typeof(env, argv[0], &valueType);
    ASSERT_BUSINESS_ERR(ctxt, (ctxt->status == napi_ok) && (valueType == napi_function), Status::INVALID_ARGUMENT,
        "The type of parameter Callback is incorrect.");

    auto proxy = reinterpret_cast<JsSingleKVStore*>(ctxt->native);
    ctxt->status = proxy->RegisterSyncCallback(std::make_shared<SyncObserver>(proxy->uvQueue_, argv[0]));
    ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, "RegisterSyncCallback failed!");
}

/*
 * [JS API Prototype] JsSingleKVStore::OffSyncComplete is private non-static.
 * [Callback]
 *      off(event:'syncComplete',syncCallback: Callback<Array<[string, number]>>):void;
 */
void JsSingleKVStore::OffSyncComplete(napi_env env, size_t argc, napi_value* argv, std::shared_ptr<ContextBase> ctxt)
{
    // required 1 arguments :: [callback]
    auto proxy = reinterpret_cast<JsSingleKVStore*>(ctxt->native);
    // have 1 arguments :: have the callback
    if (argc == 1) {
        napi_valuetype valueType = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &valueType);
        ASSERT_BUSINESS_ERR(ctxt, (ctxt->status == napi_ok) && (valueType == napi_function), Status::INVALID_ARGUMENT,
            "The type of parameter Callback is incorrect.");
        std::lock_guard<std::mutex> lck(proxy->listMutex_);
        auto it = proxy->syncObservers_.begin();
        while (it != proxy->syncObservers_.end()) {
            if (JSUtil::Equals(env, argv[0], (*it)->GetCallback())) {
                (*it)->Clear();
                proxy->syncObservers_.erase(it);
                break;
            }
        }
        ctxt->status = napi_ok;
    }
    ZLOGI("unsubscribe syncComplete, %{public}s specified observer.", (argc == 0) ? "without": "with");
    if (argc == 0 || proxy->syncObservers_.empty()) {
        ctxt->status = proxy->UnRegisterSyncCallback();
    }
    ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, "UnRegisterSyncCallback failed!");
}

/*
 * [Internal private non-static]
 */
napi_status JsSingleKVStore::RegisterSyncCallback(std::shared_ptr<SyncObserver> callback)
{
    Status status = kvStore_->RegisterSyncCallback(callback);
    if (status != Status::SUCCESS) {
        callback->Clear();
        return napi_generic_failure;
    }
    std::lock_guard<std::mutex> lck(listMutex_);
    syncObservers_.push_back(callback);
    return napi_ok;
}

napi_status JsSingleKVStore::UnRegisterSyncCallback()
{
    Status status = kvStore_->UnRegisterSyncCallback();
    if (status != Status::SUCCESS) {
        return napi_generic_failure;
    }
    std::lock_guard<std::mutex> lck(listMutex_);
    for (auto &syncObserver : syncObservers_) {
        syncObserver->Clear();
    }
    syncObservers_.clear();
    return napi_ok;
}

Status JsSingleKVStore::Subscribe(uint8_t type, std::shared_ptr<DataObserver> observer)
{
    auto subscribeType = ToSubscribeType(type);
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    ZLOGD("kvStore_->SubscribeKvStore(%{public}d) return %{public}d", type, status);
    if (status != Status::SUCCESS) {
        observer->Clear();
        return status;
    }
    dataObserver_[type].push_back(observer);
    return status;
}

Status JsSingleKVStore::UnSubscribe(uint8_t type, std::shared_ptr<DataObserver> observer)
{
    auto subscribeType = ToSubscribeType(type);
    Status status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    ZLOGD("kvStore_->UnSubscribeKvStore(%{public}d) return %{public}d", type, status);
    if (status == Status::SUCCESS) {
        observer->Clear();
        return status;
    }
    return status;
}

void JsSingleKVStore::DataObserver::OnChange(const ChangeNotification& notification)
{
    ZLOGD("data change insert:%{public}zu, update:%{public}zu, delete:%{public}zu",
        notification.GetInsertEntries().size(), notification.GetUpdateEntries().size(),
        notification.GetDeleteEntries().size());
    KvStoreObserver::OnChange(notification);

    auto args = [notification, isSchema = isSchema_](napi_env env, int& argc, napi_value* argv) {
        // generate 1 arguments for callback function.
        argc = 1;
        JSUtil::SetValue(env, notification, argv[0], isSchema);
    };
    AsyncCall(args);
}

void JsSingleKVStore::SyncObserver::SyncCompleted(const std::map<std::string, DistributedKv::Status>& results)
{
    auto args = [results](napi_env env, int& argc, napi_value* argv) {
        // generate 1 arguments for callback function.
        argc = 1;
        JSUtil::SetValue(env, results, argv[0]);
    };
    AsyncCall(args);
}

/*
 * [JS API Prototype]
 * [AsyncCallback]
 *      get(key:string, callback:AsyncCallback<boolean|string|number|Uint8Array>):void;
 * [Promise]
 *      get(key:string):Promise<boolean|string|number|Uint8Array>;
 */
napi_value JsSingleKVStore::Get(napi_env env, napi_callback_info info)
{
    struct GetContext : public ContextBase {
        std::string key;
        JSUtil::KvStoreVariant value;
    };
    auto ctxt = std::make_shared<GetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <key>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->key);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, "The type of key must be string.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "Get exit");

    ZLOGD("key=%{public}.8s", ctxt->key.c_str());
    auto execute = [env, ctxt]() {
        OHOS::DistributedKv::Key key(ctxt->key);
        OHOS::DistributedKv::Value value;
        auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
        bool isSchemaStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->IsSchemaStore();
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
    DataQuery dataQuery;
    std::string errMsg = "";
};

static napi_status GetVariantArgs(napi_env env, size_t argc, napi_value* argv, VariantArgs& va)
{
    // required 1 arguments :: <keyPrefix/query>
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, argv[0], &type);
    if (status != napi_ok || (type != napi_string && type != napi_object)) {
        va.errMsg = "The type of parameters keyPrefix/query is incorrect.";
        return status != napi_ok ? status : napi_invalid_arg;
    }
    if (type == napi_string) {
        std::string keyPrefix;
        JSUtil::GetValue(env, argv[0], keyPrefix);
        if (keyPrefix.empty()) {
            va.errMsg = "The type of parameters keyPrefix is incorrect.";
            return napi_invalid_arg;
        }
        va.dataQuery.KeyPrefix(keyPrefix);
    } else if (type == napi_object) {
        bool result = false;
        status = napi_instanceof(env, argv[0], JsQuery::Constructor(env), &result);
        if ((status == napi_ok) && (result != false)) {
            JsQuery *jsQuery = nullptr;
            status = JSUtil::Unwrap(env, argv[0], reinterpret_cast<void **>(&jsQuery), JsQuery::Constructor(env));
            if (jsQuery == nullptr) {
                va.errMsg = "The parameters query is incorrect.";
                return napi_invalid_arg;
            }
            va.dataQuery = jsQuery->GetDataQuery();
        } else {
            status = JSUtil::GetValue(env, argv[0], va.dataQuery);
            ZLOGD("kvStoreDataShare->GetResultSet return %{public}d", status);
        }
    }
    return status;
};

/*
 * [JS API Prototype]
 *  getEntries(keyPrefix:string, callback:AsyncCallback<Entry[]>):void
 *  getEntries(keyPrefix:string):Promise<Entry[]>
 *
 *  getEntries(query:Query, callback:AsyncCallback<Entry[]>):void
 *  getEntries(query:Query) : Promise<Entry[]>
 */
napi_value JsSingleKVStore::GetEntries(napi_env env, napi_callback_info info)
{
    struct GetEntriesContext : public ContextBase {
        VariantArgs va;
        std::vector<Entry> entries;
    };
    auto ctxt = std::make_shared<GetEntriesContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <keyPrefix/query>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, ctxt->va.errMsg);
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "GetEntries exit");

    auto execute = [ctxt]() {
        auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
        Status status = kvStore->GetEntries(ctxt->va.dataQuery, ctxt->entries);
        ZLOGD("kvStore->GetEntries() return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        auto isSchemaStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->IsSchemaStore();
        ctxt->status = JSUtil::SetValue(env, ctxt->entries, result, isSchemaStore);
        ASSERT_STATUS(ctxt, "output failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  getResultSet(keyPrefix:string, callback:AsyncCallback<KvStoreResultSet>):void
 *  getResultSet(keyPrefix:string):Promise<KvStoreResultSet>
 *
 *  getResultSet(query:Query, callback:AsyncCallback<KvStoreResultSet>):void
 *  getResultSet(query:Query):Promise<KvStoreResultSet>
 */
napi_value JsSingleKVStore::GetResultSet(napi_env env, napi_callback_info info)
{
    struct GetResultSetContext : public ContextBase {
        VariantArgs va;
        JsKVStoreResultSet* resultSet = nullptr;
        napi_ref ref = nullptr;
    };
    auto ctxt = std::make_shared<GetResultSetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <keyPrefix/query>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = GetVariantArgs(env, argc, argv, ctxt->va);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, ctxt->va.errMsg);
        ctxt->ref = JSUtil::NewWithRef(env, 0, nullptr, reinterpret_cast<void**>(&ctxt->resultSet),
            JsKVStoreResultSet::Constructor(env));
        ASSERT_BUSINESS_ERR(ctxt, ctxt->resultSet != nullptr, Status::INVALID_ARGUMENT,
            "KVStoreResultSet::New failed!");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "GetResultSet exit");

    auto execute = [ctxt]() {
        std::shared_ptr<KvStoreResultSet> kvResultSet;
        auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
        Status status = kvStore->GetResultSet(ctxt->va.dataQuery, kvResultSet);
        ZLOGD("kvStore->GetResultSet() return %{public}d", status);

        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
        ctxt->resultSet->SetKvStoreResultSetPtr(kvResultSet);
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = napi_get_reference_value(env, ctxt->ref, &result);
        napi_delete_reference(env, ctxt->ref);
        ASSERT_STATUS(ctxt, "output kvResultSet failed");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

/*
 * [JS API Prototype]
 *  closeResultSet(resultSet:KVStoreResultSet, callback: AsyncCallback<void>):void
 *  closeResultSet(resultSet:KVStoreResultSet):Promise<void>
 */
napi_value JsSingleKVStore::CloseResultSet(napi_env env, napi_callback_info info)
{
    struct CloseResultSetContext : public ContextBase {
        JsKVStoreResultSet* resultSet = nullptr;
    };
    auto ctxt = std::make_shared<CloseResultSetContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <resultSet>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        napi_valuetype type = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &type);
        ASSERT_BUSINESS_ERR(ctxt, type == napi_object, Status::INVALID_ARGUMENT,
            "The type of parameters resultSet is incorrect.");
        ctxt->status = JSUtil::Unwrap(env, argv[0], reinterpret_cast<void**>(&ctxt->resultSet),
            JsKVStoreResultSet::Constructor(env));
        ASSERT_BUSINESS_ERR(ctxt, ctxt->resultSet != nullptr, Status::INVALID_ARGUMENT,
            "The parameters resultSet is incorrect.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "CloseResultSet exit");

    auto execute = [ctxt]() {
        auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
        auto resultSet = ctxt->resultSet->GetKvStoreResultSetPtr();
        Status status = kvStore->CloseResultSet(resultSet);
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
 */
napi_value JsSingleKVStore::GetResultSize(napi_env env, napi_callback_info info)
{
    struct ResultSizeContext : public ContextBase {
        JsQuery* query = nullptr;
        int resultSize = 0;
    };
    auto ctxt = std::make_shared<ResultSizeContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <query>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        napi_valuetype type = napi_undefined;
        ctxt->status = napi_typeof(env, argv[0], &type);
        ASSERT_BUSINESS_ERR(ctxt, type == napi_object, Status::INVALID_ARGUMENT,
            "The type of parameters query is incorrect.");
        ctxt->status = JSUtil::Unwrap(env, argv[0], reinterpret_cast<void**>(&ctxt->query), JsQuery::Constructor(env));
        ASSERT_BUSINESS_ERR(ctxt, ctxt->query != nullptr, Status::INVALID_ARGUMENT,
            "The parameters query is incorrect.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "GetResultSize exit");

    auto execute = [ctxt]() {
        auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
        auto query = ctxt->query->GetDataQuery();
        Status status = kvStore->GetCount(query, ctxt->resultSize);
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

/*
 * [JS API Prototype]
 *  removeDeviceData(deviceId:string, callback: AsyncCallback<void>):void
 *  removeDeviceData(deviceId:string):Promise<void>
 */
napi_value JsSingleKVStore::RemoveDeviceData(napi_env env, napi_callback_info info)
{
    struct RemoveDeviceContext : public ContextBase {
        std::string deviceId;
    };
    auto ctxt = std::make_shared<RemoveDeviceContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <deviceId>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceId);
        ASSERT_BUSINESS_ERR(ctxt, (!ctxt->deviceId.empty()) && (ctxt->status == napi_ok), Status::INVALID_ARGUMENT,
            "The parameters deviceId is incorrect.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "RemoveDeviceData exit");

    auto execute = [ctxt]() {
        auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
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
napi_value JsSingleKVStore::Sync(napi_env env, napi_callback_info info)
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
        ASSERT_BUSINESS_ERR(ctxt, argc >= 2, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->deviceIdList);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The deviceIdList parameters is incorrect.");
        napi_typeof(env, argv[1], &ctxt->type);
        if (ctxt->type == napi_object) {
            ctxt->status = JSUtil::Unwrap(env,
                argv[1], reinterpret_cast<void**>(&ctxt->query), JsQuery::Constructor(env));
            ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
                "The parameters mode is incorrect.");
            ctxt->status = JSUtil::GetValue(env, argv[2], ctxt->mode);
        }
        if (ctxt->type == napi_number) {
            ctxt->status = JSUtil::GetValue(env, argv[1], ctxt->mode);
            if (argc == 3) {
                ctxt->status = JSUtil::GetValue(env, argv[2], ctxt->allowedDelayMs);
            }
        }
        ASSERT_BUSINESS_ERR(ctxt, (ctxt->mode <= uint32_t(SyncMode::PUSH_PULL)) && (ctxt->status == napi_ok),
            Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
    };
    ctxt->GetCbInfoSync(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "Sync exit");

    ZLOGD("sync deviceIdList.size=%{public}d, mode:%{public}u, allowedDelayMs:%{public}u",
        (int)ctxt->deviceIdList.size(), ctxt->mode, ctxt->allowedDelayMs);

    auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
    Status status = Status::INVALID_ARGUMENT;
    if (ctxt->type == napi_object) {
        auto query = ctxt->query->GetDataQuery();
        status = kvStore->Sync(ctxt->deviceIdList, static_cast<SyncMode>(ctxt->mode), query, nullptr);
    }
    if (ctxt->type == napi_number) {
        status = kvStore->Sync(ctxt->deviceIdList, static_cast<SyncMode>(ctxt->mode), ctxt->allowedDelayMs);
    }
    ZLOGD("kvStore->Sync return %{public}d!", status);
    ThrowNapiError(env, status, "", false);
    return nullptr;
}

/*
 * [JS API Prototype]
 *  setSyncParam(defaultAllowedDelayMs:number, callback: AsyncCallback<number>):void
 *  setSyncParam(defaultAllowedDelayMs:number):Promise<void>
 */
napi_value JsSingleKVStore::SetSyncParam(napi_env env, napi_callback_info info)
{
    struct SyncParamContext : public ContextBase {
        uint32_t allowedDelayMs;
    };
    auto ctxt = std::make_shared<SyncParamContext>();
    auto input = [env, ctxt](size_t argc, napi_value* argv) {
        // required 1 arguments :: <allowedDelayMs>
        ASSERT_BUSINESS_ERR(ctxt, argc >= 1, Status::INVALID_ARGUMENT, "The number of parameters is incorrect.");
        ctxt->status = JSUtil::GetValue(env, argv[0], ctxt->allowedDelayMs);
        ASSERT_BUSINESS_ERR(ctxt, ctxt->status == napi_ok, Status::INVALID_ARGUMENT,
            "The parameters allowedDelayMs is incorrect.");
    };
    ctxt->GetCbInfo(env, info, input);
    ASSERT_NULL(!ctxt->isThrowError, "SetSyncParam exit");

    auto execute = [ctxt]() {
        auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
        KvSyncParam syncParam { ctxt->allowedDelayMs };
        Status status = kvStore->SetSyncParam(syncParam);
        ZLOGD("kvStore->SetSyncParam return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute);
}

/*
 * [JS API Prototype]
 *  getSecurityLevel(callback: AsyncCallback<SecurityLevel>):void
 *  getSecurityLevel():Promise<SecurityLevel>
 */
napi_value JsSingleKVStore::GetSecurityLevel(napi_env env, napi_callback_info info)
{
    struct SecurityLevelContext : public ContextBase {
        SecurityLevel securityLevel;
    };
    auto ctxt = std::make_shared<SecurityLevelContext>();
    ctxt->GetCbInfo(env, info);

    auto execute = [ctxt]() {
        auto kvStore = reinterpret_cast<JsSingleKVStore*>(ctxt->native)->GetKvStorePtr();
        Status status = kvStore->GetSecurityLevel(ctxt->securityLevel);
        ZLOGD("kvStore->GetSecurityLevel return %{public}d", status);
        ctxt->status = (GenerateNapiError(status, ctxt->jsCode, ctxt->error) == Status::SUCCESS) ?
            napi_ok : napi_generic_failure;
    };
    auto output = [env, ctxt](napi_value& result) {
        ctxt->status = JSUtil::SetValue(env, static_cast<uint8_t>(ctxt->securityLevel), result);
        ASSERT_STATUS(ctxt, "output failed!");
    };
    return NapiQueue::AsyncWork(env, ctxt, std::string(__FUNCTION__), execute, output);
}

napi_value JsSingleKVStore::New(napi_env env, napi_callback_info info)
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
    ASSERT_NULL(!ctxt->isThrowError, "SingleKVStore new exit");
    ASSERT_ERR(env, ctxt->status == napi_ok, Status::INVALID_ARGUMENT, "no memory for kvStore");

    JsSingleKVStore* kvStore = new (std::nothrow) JsSingleKVStore(storeId);
    ASSERT_ERR(env, kvStore != nullptr, Status::INVALID_ARGUMENT, "no memory for kvStore");

    auto finalize = [](napi_env env, void* data, void* hint) {
        ZLOGI("singleKVStore finalize.");
        auto* kvStore = reinterpret_cast<JsSingleKVStore*>(data);
        ASSERT_VOID(kvStore != nullptr, "finalize null!");
        delete kvStore;
    };
    ASSERT_CALL(env, napi_wrap(env, ctxt->self, kvStore, finalize, nullptr, nullptr), kvStore);
    return ctxt->self;
}
} // namespace OHOS::DistributedKVStore
