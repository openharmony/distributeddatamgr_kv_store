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
#define LOG_TAG "EntryPoint"
#include "js_const_properties.h"
#include "js_field_node.h"
#include "js_kv_manager.h"
#include "js_query.h"
#include "js_schema.h"
#include "js_util.h"
#include "log_print.h"

using namespace OHOS::DistributedKVStore;

static napi_value Init(napi_env env, napi_value exports)
{
    const napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createKVManager", JsKVManager::CreateKVManager)
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    napi_set_named_property(env, exports, "FieldNode", JsFieldNode::Constructor(env));
    napi_set_named_property(env, exports, "Schema", JsSchema::Constructor(env));
    napi_set_named_property(env, exports, "Query", JsQuery::Constructor(env));
    InitConstProperties(env, exports);

    return exports;
}

static __attribute__((constructor)) void RegisterModule()
{
    static napi_module module = { .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = Init,
        .nm_modname = "data.distributedKVStore",
        .nm_priv = ((void*)0),
        .reserved = { 0 } };
    napi_module_register(&module);
    ZLOGI("Module register data.distributedKvstore");
}
