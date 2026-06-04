/**
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
#ifndef TOKEN_SETPROC_H
#define TOKEN_SETPROC_H
#include <stdint.h>
#include "setproc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t GetSelfTokenID(void);

int SetSelfTokenID(uint64_t tokenID);

uint64_t GetFirstCallerTokenID(void);

int SetFirstCallerTokenID(uint64_t tokenID);

#ifdef __cplusplus
}
#endif

#endif