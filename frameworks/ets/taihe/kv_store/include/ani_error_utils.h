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
#ifndef OHOS_ANI_ERROR_UTILS_H
#define OHOS_ANI_ERROR_UTILS_H
#include <string>
#include <optional>
#include "store_errno.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKVStore {
using Status = OHOS::DistributedKv::Status;

struct JsErrorCode {
    int32_t status;
    int32_t jsCode;
    const char *message;
};

const std::optional<JsErrorCode> GetJsErrorCode(int32_t errorCode);

void ThrowError(const char* message);
void ThrowError(int32_t code, const char* message);
void ThrowAniError(int32_t errCode, const std::string &errMessage, bool isNewApi = false);

#define ANI_ASSERT(assertion, message, retVal)                           \
    do {                                                                 \
        if (!(assertion)) {                                              \
            ThrowError("assertion (" #assertion ") failed: " message);   \
            return retVal;                                               \
        }                                                                \
    } while (0)

} // namespace DistributedKVStore
}  // namespace OHOS
#endif  // OHOS_ANI_ERROR_UTILS_H
