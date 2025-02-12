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
#define LOG_TAG "UvQueue"

#include "log_print.h"
#include "napi_queue.h"
#include "uv_queue.h"

namespace OHOS::DistributedKVStore {
UvQueue::UvQueue(napi_env env)
    : env_(env)
{
}

UvQueue::~UvQueue()
{
    ZLOGD("No memory leak for queue-callback");
    env_ = nullptr;
}

void UvQueue::AsyncCall(NapiCallbackGetter getter, NapiArgsGenerator genArgs)
{
    if (!getter) {
        ZLOGE("This callback is nullptr");
        return;
    }
    auto env = env_;
    auto task = [env, getter, genArgs]() {
        napi_handle_scope scope = nullptr;
        napi_open_handle_scope(env, &scope);
        if (scope == nullptr) {
            return;
        }
        napi_value method = getter(env);
        if (method == nullptr) {
            ZLOGE("The callback is invalid, maybe is cleared!");
            napi_close_handle_scope(env, scope);
            return;
        }
        int argc = 0;
        napi_value argv[ARGC_MAX] = { nullptr };
        if (genArgs) {
            argc = ARGC_MAX;
            genArgs(env, argc, argv);
        }
        napi_value global = nullptr;
        napi_status status = napi_get_global(env, &global);
        if (status != napi_ok) {
            ZLOGE("Get napi global failed. status: %{public}d.", status);
            napi_close_handle_scope(env, scope);
            return;
        }
        napi_value result;
        status = napi_call_function(env, global, method, argc, argv, &result);
        if (status != napi_ok) {
            ZLOGE("Notify data change failed. status:%{public}d.", status);
        }
        napi_close_handle_scope(env, scope);
    };
    napi_status status = napi_send_event(env_, task, napi_eprio_immediate);
    if (status != napi_ok) {
        ZLOGE("Failed to napi_send_event. status:%{public}d", status);
    }
}

napi_env UvQueue::GetEnv()
{
    return env_;
}
} // namespace OHOS::DistributedKVStore