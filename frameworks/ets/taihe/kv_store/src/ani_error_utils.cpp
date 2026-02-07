/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "AniErrorUtils"
#include "taihe/runtime.hpp"
#include "ani_error_utils.h"

#include <algorithm>

namespace OHOS::DistributedKVStore {
using JsErrorCode = OHOS::DistributedKVStore::JsErrorCode;

static constexpr JsErrorCode JS_ERROR_CODE_MSGS[] = {
    { Status::INVALID_ARGUMENT, 401, "Parameter error: Parameters verification failed." },
    { Status::STORE_NOT_OPEN, 0, "" },
    { Status::STORE_ALREADY_SUBSCRIBE, 0, "" },
    { Status::STORE_NOT_SUBSCRIBE, 0, "" },
    { Status::NOT_FOUND, 15100004, "Not found." },
    { Status::STORE_META_CHANGED, 15100002, "Open existed database with changed options." },
    { Status::PERMISSION_DENIED, 202, "Permission denied" },
    { Status::CRYPT_ERROR, 15100003, "Database corrupted." },
    { Status::OVER_MAX_LIMITS, 15100001, "Over max limits." },
    { Status::ALREADY_CLOSED, 15100005, "Database or result set already closed." },
    { Status::DATA_CORRUPTED, 15100003, "Database corrupted" },
    { Status::WAL_OVER_LIMITS, 14800047, "the WAL file size exceeds the default limit."}
};

static constexpr JsErrorCode JS_NEW_API_ERROR_CODE_MSGS[] = {
    { Status::INVALID_ARGUMENT, 15100000, "Parameter error: Parameters verification failed." },
    { Status::STORE_NOT_OPEN, 0, "" },
    { Status::STORE_ALREADY_SUBSCRIBE, 0, "" },
    { Status::STORE_NOT_SUBSCRIBE, 0, "" },
    { Status::NOT_FOUND, 15100004, "Not found." },
    { Status::STORE_META_CHANGED, 15100002, "Open existed database with changed options." },
    { Status::PERMISSION_DENIED, 202, "Permission denied" },
    { Status::CRYPT_ERROR, 15100003, "Database corrupted." },
    { Status::OVER_MAX_LIMITS, 15100001, "Over max limits." },
    { Status::ALREADY_CLOSED, 15100005, "Database or result set already closed." },
    { Status::DATA_CORRUPTED, 15100003, "Database corrupted" },
    { Status::WAL_OVER_LIMITS, 14800047, "the WAL file size exceeds the default limit."}
};

const std::optional<JsErrorCode> GetJsErrorCode(int32_t errorCode)
{
    auto jsErrorCode = JsErrorCode{ errorCode, -1, "" };
    auto iter = std::lower_bound(JS_ERROR_CODE_MSGS,
        JS_ERROR_CODE_MSGS + sizeof(JS_ERROR_CODE_MSGS) / sizeof(JS_ERROR_CODE_MSGS[0]), jsErrorCode,
        [](const JsErrorCode &jsErrorCode1, const JsErrorCode &jsErrorCode2) {
            return jsErrorCode1.status < jsErrorCode2.status;
        });
    if (iter < JS_ERROR_CODE_MSGS + sizeof(JS_ERROR_CODE_MSGS) / sizeof(JS_ERROR_CODE_MSGS[0]) &&
        iter->status == errorCode) {
        return *iter;
    }
    return std::nullopt;
}

const std::optional<JsErrorCode> GetJsNewApiErrorCode(int32_t errorCode)
{
    auto jsErrorCode = JsErrorCode{ errorCode, -1, "" };
    auto iter = std::lower_bound(JS_NEW_API_ERROR_CODE_MSGS,
        JS_NEW_API_ERROR_CODE_MSGS + sizeof(JS_NEW_API_ERROR_CODE_MSGS) / sizeof(JS_NEW_API_ERROR_CODE_MSGS[0]),
        jsErrorCode,
        [](const JsErrorCode &jsErrorCode1, const JsErrorCode &jsErrorCode2) {
            return jsErrorCode1.status < jsErrorCode2.status;
        });
    if (iter < JS_NEW_API_ERROR_CODE_MSGS +
        sizeof(JS_NEW_API_ERROR_CODE_MSGS) / sizeof(JS_NEW_API_ERROR_CODE_MSGS[0]) &&
        iter->status == errorCode) {
        return *iter;
    }
    return std::nullopt;
}

void ThrowError(const char* message)
{
    if (message == nullptr) {
        return;
    }
    std::string errMsg(message);
    taihe::set_business_error(-1, errMsg);
}

void ThrowError(int32_t code, const char* message)
{
    if (message == nullptr) {
        return;
    }
    std::string errMsg(message);
    taihe::set_business_error(code, errMsg);
}

void ThrowAniError(int32_t status, const std::string &errMessage, bool isNewApi)
{
    ZLOGW("ThrowAniError status %{public}d, offset %{public}d, message: %{public}s",
        status, status - DistributedKv::Status::ERROR, errMessage.c_str());
    if (status == DistributedKv::Status::SUCCESS) {
        return;
    }
    auto errorMsg = isNewApi ? GetJsNewApiErrorCode(status) : GetJsErrorCode(status);
    JsErrorCode aniError;
    if (errorMsg.has_value()) {
        aniError = errorMsg.value();
    } else {
        aniError.jsCode = -1;
        aniError.message = "";
    }

    std::string message(aniError.message);
    if (status == DistributedKv::Status::INVALID_ARGUMENT) {
        message += errMessage;
    }

    if (aniError.jsCode == -1) {
        ThrowError(message.c_str());
        return;
    }
    ThrowError(aniError.jsCode, message.c_str());
}
} // namespace OHOS::DistributedKVStore