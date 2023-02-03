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
#define LOG_TAG "NapiQueue"
#include "napi_queue.h"

using namespace OHOS::DistributedKv;

namespace OHOS::DistributedKVStore {
ContextBase::~ContextBase()
{
    ZLOGD("no memory leak after callback or promise[resolved/rejected]");
    if (env != nullptr) {
        if (work != nullptr) {
            auto status = napi_delete_async_work(env, work);
            ZLOGD("status:%{public}d", status);
        }
        if (callbackRef != nullptr) {
            auto status = napi_delete_reference(env, callbackRef);
            ZLOGD("status:%{public}d", status);
        }
        if (selfRef != nullptr) {
            auto status = napi_delete_reference(env, selfRef);
            ZLOGD("status:%{public}d", status);
        }
        env = nullptr;
    }
}

void ContextBase::GetCbInfo(napi_env envi, napi_callback_info info, NapiCbInfoParser parse, bool sync)
{
    env = envi;
    size_t argc = ARGC_MAX;
    napi_value argv[ARGC_MAX] = { nullptr };
    status = napi_get_cb_info(env, info, &argc, argv, &self, nullptr);
    ASSERT_STATUS(this, "napi_get_cb_info failed!");
    ASSERT_ARGS(this, argc <= ARGC_MAX, "too many arguments!");
    ASSERT_ARGS(this, self != nullptr, "no JavaScript this argument!");
    if (!sync) {
        napi_create_reference(env, self, 1, &selfRef);
    }
    status = napi_unwrap(env, self, &native);
    ASSERT_STATUS(this, "self unwrap failed!");

    if (!sync && (argc > 0)) {
        // get the last arguments :: <callback>
        size_t index = argc - 1;
        napi_valuetype type = napi_undefined;
        napi_status tyst = napi_typeof(env, argv[index], &type);
        if ((tyst == napi_ok) && (type == napi_function)) {
            status = napi_create_reference(env, argv[index], 1, &callbackRef);
            ASSERT_STATUS(this, "ref callback failed!");
            argc = index;
            ZLOGD("async callback, no promise");
        } else {
            ZLOGD("no callback, async pormose");
        }
    }

    if (parse) {
        parse(argc, argv);
    } else {
        ASSERT_ARGS(this, argc == 0, "required no arguments!");
    }
}

napi_value NapiQueue::AsyncWork(napi_env env, std::shared_ptr<ContextBase> ctxt, const std::string& name,
    NapiAsyncExecute execute, NapiAsyncComplete complete)
{
    ZLOGD("name=%{public}s", name.c_str());

    napi_value promise = nullptr;
    if (ctxt->callbackRef == nullptr) {
        napi_create_promise(ctxt->env, &ctxt->deferred, &promise);
        ZLOGD("create deferred promise");
    } else {
        napi_get_undefined(ctxt->env, &promise);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(ctxt->env, name.c_str(), NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(
        ctxt->env, nullptr, resource,
        [](napi_env env, void* data) {
            ASSERT_VOID(data != nullptr, "napi_async_execute_callback nullptr");
            auto ctxt = reinterpret_cast<ContextBase*>(data);
            ZLOGD("napi_async_execute_callback ctxt->status=%{public}d", ctxt->status);
            if (ctxt->execute && ctxt->status == napi_ok) {
                ctxt->execute();
            }
        },
        [](napi_env env, napi_status status, void* data) {
            ASSERT_VOID(data != nullptr, "napi_async_complete_callback nullptr");
            auto ctxt = reinterpret_cast<ContextBase*>(data);
            ZLOGD("napi_async_complete_callback status=%{public}d, ctxt->status=%{public}d", status, ctxt->status);
            if ((status != napi_ok) && (ctxt->status == napi_ok)) {
                ctxt->status = status;
            }
            if ((ctxt->complete) && (status == napi_ok) && (ctxt->status == napi_ok)) {
                ctxt->complete(ctxt->output);
            }
            GenerateOutput(ctxt);
        },
        reinterpret_cast<void*>(ctxt.get()), &ctxt->work);
    ctxt->execute = std::move(execute);
    ctxt->complete = std::move(complete);
    ctxt->hold = ctxt; // save crossing-thread ctxt.
    napi_queue_async_work(ctxt->env, ctxt->work);
    return promise;
}

void NapiQueue::GenerateOutput(ContextBase* ctxt)
{
    napi_value result[RESULT_ALL] = { nullptr };
    if (ctxt->status == napi_ok) {
        napi_get_undefined(ctxt->env, &result[RESULT_ERROR]);
        if (ctxt->output == nullptr) {
            napi_get_undefined(ctxt->env, &ctxt->output);
        }
        result[RESULT_DATA] = ctxt->output;
    } else {
        napi_value message = nullptr;
        napi_value errorCode = nullptr;
        if (ctxt->jsCode != 0 && ctxt->jsCode != -1) {
            napi_create_string_utf8(ctxt->env, std::to_string(ctxt->jsCode).c_str(), NAPI_AUTO_LENGTH, &errorCode);
        }
        if (ctxt->jsCode == -1) {
            std::string jscode = "";
            napi_create_string_utf8(ctxt->env, jscode.c_str(), NAPI_AUTO_LENGTH, &errorCode);
        }
        napi_create_string_utf8(ctxt->env, ctxt->error.c_str(), NAPI_AUTO_LENGTH, &message);
        napi_create_error(ctxt->env, errorCode, message, &result[RESULT_ERROR]);
        napi_get_undefined(ctxt->env, &result[RESULT_DATA]);
    }
    if (ctxt->deferred != nullptr) {
        if (ctxt->status == napi_ok) {
            ZLOGD("deferred promise resolved");
            napi_resolve_deferred(ctxt->env, ctxt->deferred, result[RESULT_DATA]);
        } else {
            ZLOGD("deferred promise rejected");
            napi_reject_deferred(ctxt->env, ctxt->deferred, result[RESULT_ERROR]);
        }
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(ctxt->env, ctxt->callbackRef, &callback);
        napi_value callbackResult = nullptr;
        ZLOGD("call callback function");
        napi_call_function(ctxt->env, nullptr, callback, RESULT_ALL, result, &callbackResult);
    }
    ctxt->execute = nullptr;
    ctxt->complete = nullptr;
    ctxt->hold.reset(); // release ctxt.
}
} // namespace OHOS::DistributedKVStore
