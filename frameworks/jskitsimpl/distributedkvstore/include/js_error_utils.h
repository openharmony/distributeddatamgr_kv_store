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

#ifndef OHOS_ERROR_UTILS_H
#define OHOS_ERROR_UTILS_H
#include <map>
#include <string>
#include <optional>
#include "store_errno.h"
#include "js_native_api.h"
#include "napi/native_common.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKVStore {
using Status = OHOS::DistributedKv::Status;

struct JsErrorCode {
    int32_t jsCode;
    std::string message;
};

const std::optional<JsErrorCode> GetJsErrorCode(int32_t errorCode);
Status GenerateNapiError(Status status, int32_t &errCode, std::string &errMessage);
void ThrowNapiError(napi_env env, int32_t errCode, std::string errMessage, bool isParamsCheck = true);
napi_value GenerateErrorMsg(napi_env env, JsErrorCode jsInfo);

#define ASSERT_ERR(env, assertion, errorcode, message)                                       \
    do {                                                                                     \
        if (!(assertion)) {                                                                  \
            ThrowNapiError(env, errorcode, message);                                         \
            return nullptr;                                                                  \
        }                                                                                    \
    } while (0)

#define ASSERT_BUSINESS_ERR(ctxt, assertion, errorcode, message)                             \
    do {                                                                                     \
        if (!(assertion)) {                                                                  \
            (ctxt)->isThrowError = true;                                                     \
            ThrowNapiError((ctxt)->env, errorcode, message);                                 \
            return;                                                                          \
        }                                                                                    \
    } while (0)

} // namespace DistributedKVStore
}  // namespace OHOS
#endif  // OHOS_ERROR_UTILS_H
