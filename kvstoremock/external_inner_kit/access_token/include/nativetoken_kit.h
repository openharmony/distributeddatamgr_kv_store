/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNER_KITS_NATIVE_TOKEN_H
#define INTERFACES_INNER_KITS_NATIVE_TOKEN_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TokenInfoParams {
    int32_t dcapsNum;
    int32_t permsNum;
    int32_t aclsNum;
    const char **dcaps;
    const char **perms;
    const char **acls;
    const char *processName;
    const char *aplStr;
} NativeTokenInfoParams;

extern uint64_t GetAccessTokenId(NativeTokenInfoParams *tokenInfo);

#ifdef __cplusplus
}
#endif

#endif // INTERFACES_INNER_KITS_NATIVE_TOKEN_H
