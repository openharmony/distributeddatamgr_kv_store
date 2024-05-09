/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef DISTRIBUTED_KV_STORE_UTILS_H
#define DISTRIBUTED_KV_STORE_UTILS_H

#include <cstdint>
#include <memory>
#include <string>

namespace OHOS {
namespace DistributedKVStore {
    char* MallocCString(const std::string& origin);
    enum CJErrorCode {
        CJ_ERROR_PERMISSION_DENIED = 202,

        CJ_ERROR_INVALID_ARGUMENT = 401,

        CJ_ERROR_OVER_MAX_LIMITS = 15100001,

        CJ_ERROR_STORE_META_CHANGED = 15100002,

        CJ_ERROR_CRYPT_ERROR = 15100003,

        CJ_ERROR_NOT_FOUND = 15100004,

        CJ_ERROR_ALREADY_CLOSED = 15100005,
    };

    enum TypeSymbol {
        /* Blob's first byte is the blob's data ValueType */
        STRING = 0,
        INTEGER = 1,
        FLOAT = 2,
        BYTE_ARRAY = 3,
        BOOLEAN = 4,
        DOUBLE = 5,
        INVALID = 255
    };
}
}
#endif