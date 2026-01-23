/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DISTRIBUTED_KVSTORE_MOCK_HKS_TYPE_H
#define DISTRIBUTED_KVSTORE_MOCK_HKS_TYPE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum HksTagType {
    HKS_TAG_TYPE_UINT = 2 << 28,
    HKS_TAG_TYPE_BYTES = 5 << 28,
};

enum HksErrorCode {
    HKS_SUCCESS = 0,
    HKS_FAILURE = -1,
    HKS_ERROR_INVALID_ARGUMENT = -3,

    HKS_ERROR_NOT_EXIST = -13,
};

enum HksKeyAlg {
    HKS_ALG_AES = 20,
};

enum HksKeyPurpose {
    HKS_KEY_PURPOSE_ENCRYPT = 1,
    HKS_KEY_PURPOSE_DECRYPT = 2,
};

enum HksKeyPadding {
    HKS_PADDING_NONE = 0,
};

enum HksCipherMode {
    HKS_MODE_GCM = 32,
};

enum HksAuthStorageLevel {
    HKS_AUTH_STORAGE_LEVEL_DE = 0,
};

enum HksKeySize {
    HKS_AES_KEY_SIZE_256 = 256,
};

struct HksBlob {
    uint32_t size;
    uint8_t *data;
};

struct HksParam {
    uint32_t tag;
    union {
        bool boolParam;
        int32_t int32Param;
        uint32_t uint32Param;
        uint64_t uint64Param;
        struct HksBlob blob;
    };
};

enum HksTag {
    HKS_TAG_ALGORITHM = HKS_TAG_TYPE_UINT | 1,
    HKS_TAG_PURPOSE = HKS_TAG_TYPE_UINT | 2,
    HKS_TAG_KEY_SIZE = HKS_TAG_TYPE_UINT | 3,
    HKS_TAG_DIGEST = HKS_TAG_TYPE_UINT | 4,
    HKS_TAG_PADDING = HKS_TAG_TYPE_UINT | 5,
    HKS_TAG_BLOCK_MODE = HKS_TAG_TYPE_UINT | 6,
    HKS_TAG_ASSOCIATED_DATA = HKS_TAG_TYPE_BYTES | 8,
    HKS_TAG_NONCE = HKS_TAG_TYPE_BYTES | 9,

    HKS_TAG_AUTH_STORAGE_LEVEL = HKS_TAG_TYPE_UINT | 316,

    HKS_TAG_AE_TAG = HKS_TAG_TYPE_BYTES | 10009,
};

struct HksParamSet;

#ifdef __cplusplus
}
#endif
#endif // DISTRIBUTED_KVSTORE_MOCK_HKS_TYPE_H
