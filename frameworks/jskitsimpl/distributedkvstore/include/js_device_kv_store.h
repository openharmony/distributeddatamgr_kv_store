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
#ifndef OHOS_DEVICE_KV_STORE_H
#define OHOS_DEVICE_KV_STORE_H
#include "js_kv_manager.h"
#include "js_single_kv_store.h"
#include "napi_queue.h"

namespace OHOS::DistributedKVStore {
class JsDeviceKVStore : public JsSingleKVStore {
public:
    explicit JsDeviceKVStore(const std::string& storeId);
    ~JsDeviceKVStore() = default;

    static napi_value Constructor(napi_env env);

    static napi_value New(napi_env env, napi_callback_info info);

private:
    static napi_value Get(napi_env env, napi_callback_info info);
    static napi_value GetEntries(napi_env env, napi_callback_info info);
    static napi_value GetResultSet(napi_env env, napi_callback_info info);
    static napi_value CloseResultSet(napi_env env, napi_callback_info info);
    static napi_value GetResultSize(napi_env env, napi_callback_info info);
    static napi_value RemoveDeviceData(napi_env env, napi_callback_info info);
    static napi_value Sync(napi_env env, napi_callback_info info);
    static napi_value OnEvent(napi_env env, napi_callback_info info);
    static napi_value OffEvent(napi_env env, napi_callback_info info);
};
} // namespace OHOS::DistributedKVStore
#endif // OHOS_DEVICE_KV_STORE_H
