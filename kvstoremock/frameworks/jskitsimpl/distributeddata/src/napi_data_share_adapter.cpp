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
#include "kv_utils.h"
#include "log_print.h"

using namespace OHOS::DistributedKv;
using namespace OHOS::DataShare;
namespace OHOS::DistributedData {
std::shared_ptr<ResultSetBridge> JsKVStoreResultSet::CreateBridge(std::shared_ptr<KvStoreResultSet> instance)
{
    return KvUtils::ToResultSetBridge(instance);
}

struct PredicatesProxy {
    std::shared_ptr<DataShareAbsPredicates> predicates_;
};

napi_status JSUtil::GetValue(napi_env env, napi_value in, DataQuery &query)
{
    ZLOGD("napi_value -> std::GetValue DataQuery");
    napi_valuetype type = napi_undefined;
    napi_status nstatus = napi_typeof(env, in, &type);
    CHECK_RETURN((nstatus == napi_ok) && (type == napi_object), "invalid type", napi_invalid_arg);
    PredicatesProxy *predicates = nullptr;
    napi_unwrap(env, in, reinterpret_cast<void **>(&predicates));
    CHECK_RETURN((predicates != nullptr), "invalid type", napi_invalid_arg);
    Status status = KvUtils::ToQuery(*(predicates->predicates_), query);
    if (status != Status::SUCCESS) {
        ZLOGD("napi_value -> GetValue DataQuery failed ");
    }
    return nstatus;
}

} // namespace OHOS::DistributedData