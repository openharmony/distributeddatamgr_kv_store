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

namespace OHOS::DistributedData {
ContextBase::~ContextBase()
{
    ZLOGD("no memory leak after callback or promise[resolved/rejected]");
    if (env != nullptr) {
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
    CHECK_STATUS_RETURN_VOID(this, "napi_get_cb_info failed!");
    CHECK_ARGS_RETURN_VOID(this, argc <= ARGC_MAX, "too many arguments!");
    CHECK_ARGS_RETURN_VOID(this, self != nullptr, "no JavaScript this argument!");
    if (!sync) {
        napi_create_reference(env, self, 1, &selfRef);
    }
    status = napi_unwrap(env, self, &native);
    CHECK_STATUS_RETURN_VOID(this, "self unwrap failed!");

    if (!sync && (argc > 0)) {
        // get the last arguments :: <callback>
        size_t index = argc - 1;
        napi_valuetype type = napi_undefined;
        napi_status tyst = napi_typeof(env, argv[index], &type);
        if ((tyst == napi_ok) && (type == napi_function)) {
            status = napi_create_reference(env, argv[index], 1, &callbackRef);
            CHECK_STATUS_RETURN_VOID(this, "ref callback failed!");
            argc = index;
            ZLOGD("async callback, no promise");
        } else {
            ZLOGD("no callback, async pormose");
        }
    }

    if (parse) {
        parse(argc, argv);
    } else {
        CHECK_ARGS_RETURN_VOID(this, argc == 0, "required no arguments!");
    }
}

napi_value NapiQueue::AsyncWork(napi_env env, std::shared_ptr<ContextBase> ctxt, const std::string& name,
    NapiAsyncExecute execute, NapiAsyncComplete complete)
{
    ZLOGD("name=%{public}s", name.c_str());
    AsyncContext *aCtx = new AsyncContext;
    aCtx->env = env;
    aCtx->ctx = std::move(ctxt);
    aCtx->execute = std::move(execute);
    aCtx->complete = std::move(complete);
    napi_value promise = nullptr;
    if (aCtx->ctx->callbackRef == nullptr) {
        napi_create_promise(env, &aCtx->deferred, &promise);
        ZLOGD("create deferred promise");
    } else {
        napi_get_undefined(env, &promise);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void* data) {
            CHECK_RETURN_VOID(data != nullptr, "napi_async_execute_callback nullptr");
            auto actx = reinterpret_cast<AsyncContext*>(data);
            ZLOGD("napi_async_execute_callback ctxt->status=%{public}d", actx->ctx->status);
            if (actx->execute && actx->ctx->status == napi_ok) {
                actx->execute();
            }
        },
        [](napi_env env, napi_status status, void* data) {
            CHECK_RETURN_VOID(data != nullptr, "napi_async_complete_callback nullptr");
            auto actx = reinterpret_cast<AsyncContext*>(data);
            ZLOGD("napi_async_complete_callback status=%{public}d, ctxt->status=%{public}d", status, actx->ctx->status);
            if ((status != napi_ok) && (actx->ctx->status == napi_ok)) {
                actx->ctx->status = status;
            }
            napi_value output = nullptr;
            if ((actx->complete) && (status == napi_ok) && (actx->ctx->status == napi_ok)) {
                actx->complete(output);
            }
            GenerateOutput(*actx, output);
            delete actx;
        },
        reinterpret_cast<void*>(aCtx), &aCtx->work);
    auto status = napi_queue_async_work_with_qos(env, aCtx->work, napi_qos_user_initiated);
    if (status != napi_ok) {
        napi_get_undefined(env, &promise);
        delete aCtx;
    }
    return promise;
}

void NapiQueue::GenerateOutput(AsyncContext &ctx, napi_value output)
{
    napi_value result[RESULT_ALL] = { nullptr };
    if (ctx.ctx->status == napi_ok) {
        napi_get_undefined(ctx.env, &result[RESULT_ERROR]);
        if (output == nullptr) {
            napi_get_undefined(ctx.env, &output);
        }
        result[RESULT_DATA] = output;
    } else {
        napi_value message = nullptr;
        napi_create_string_utf8(ctx.env, ctx.ctx->error.c_str(), NAPI_AUTO_LENGTH, &message);
        napi_create_error(ctx.env, nullptr, message, &result[RESULT_ERROR]);
        napi_get_undefined(ctx.env, &result[RESULT_DATA]);
    }
    if (ctx.deferred != nullptr) {
        if (ctx.ctx->status == napi_ok) {
            ZLOGD("deferred promise resolved");
            napi_resolve_deferred(ctx.env, ctx.deferred, result[RESULT_DATA]);
        } else {
            ZLOGD("deferred promise rejected");
            napi_reject_deferred(ctx.env, ctx.deferred, result[RESULT_ERROR]);
        }
    } else {
        napi_value callback = nullptr;
        napi_get_reference_value(ctx.env, ctx.ctx->callbackRef, &callback);
        napi_value callbackResult = nullptr;
        ZLOGD("call callback function");
        napi_call_function(ctx.env, nullptr, callback, RESULT_ALL, result, &callbackResult);
    }
}
} // namespace OHOS::DistributedData