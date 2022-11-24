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
#define LOG_TAG "JS_ERROR_UTILS"
#include "js_error_utils.h"

namespace OHOS::DistributedKVStore {
using JsErrorCode = OHOS::DistributedKVStore::JsErrorCode;

static const std::map<int32_t, JsErrorCode> jsErrCodeMsgMap {
    {Status::INVALID_ARGUMENT,               {401, "Parameter error."}},
    {Status::OVER_MAX_SUBSCRIBE_LIMITS,      {15100001, "Over max subscribe limits."}},
    {Status::STORE_META_CHANGED,             {15100002, "Open existed database with changed options."}},
    {Status::CRYPT_ERROR,                    {15100003, "Database corrupted."}},
    {Status::NOT_FOUND,                      {15100004, "Not found."}},
    {Status::ALREADY_CLOSED,                 {15100005, "Database or result set already closed."}},
    {Status::STORE_ALREADY_SUBSCRIBE,        {0, ""}},
    {Status::STORE_NOT_OPEN,                 {0, ""}},
    {Status::STORE_NOT_SUBSCRIBE,            {0, ""}},
};

const std::optional<JsErrorCode> GetJsErrorCode(int32_t errorCode)
{
    auto iter = jsErrCodeMsgMap.find(errorCode);
    if (iter != jsErrCodeMsgMap.end()) {
        return iter->second;
    }
    return std::nullopt;
}

Status GenerateNapiError(Status status, int32_t &errCode, std::string &errMessage)
{
    auto errormsg = GetJsErrorCode(status);
    if (errormsg.has_value()) {
        auto napiError = errormsg.value();
        errCode = napiError.jsCode;
        errMessage = napiError.message;
    } else {
        // unmatched status return unified error code
        errCode = -1;
        errMessage = "";
    }
    ZLOGD("GenerateNapiError errCode is %{public}d", errCode);
    if (errCode == 0) {
        return Status::SUCCESS;
    }
    return status;
}

void ThrowNapiError(napi_env env, int32_t status, std::string errMessage, bool isParamsCheck)
{
    ZLOGD("ThrowNapiError message: %{public}s", errMessage.c_str());
    if (status == Status::SUCCESS) {
        return;
    }
    auto errormsg = GetJsErrorCode(status);
    JsErrorCode napiError;
    if (errormsg.has_value()) {
        napiError = errormsg.value();
    } else {
        napiError.jsCode = -1;
        napiError.message = "";
    }

    if (isParamsCheck) {
        napiError.message += errMessage;
        napiError.jsCode = 401;
    }

    std::string jsCode;
    if (napiError.jsCode == -1) {
        jsCode = "";
    } else {
        jsCode = std::to_string(napiError.jsCode);
    }
    napi_throw_error(env, jsCode.c_str(), napiError.message.c_str());
}
}  // namespace OHOS::DistributedKVStore