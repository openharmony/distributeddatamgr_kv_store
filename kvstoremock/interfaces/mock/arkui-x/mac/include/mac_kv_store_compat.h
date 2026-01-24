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

#ifndef DISTRIBUTED_KVSTORE_MOCK_KV_STORE_IOS_COMPAT_H
#define DISTRIBUTED_KVSTORE_MOCK_KV_STORE_IOS_COMPAT_H

#include <pthread.h>

#ifdef __cplusplus
#include "mac_glibc.h"
#if defined(pthread_setname_np)
#undef pthread_setname_np
#endif
#define pthread_setname_np OHOS::pthread_setname_np
#endif

#if defined(__APPLE__)
#ifndef st_atim
#define st_atim st_atimespec
#endif
#ifndef st_mtim
#define st_mtim st_mtimespec
#endif
#ifndef st_ctim
#define st_ctim st_ctimespec
#endif
#endif

#ifndef htobe64
#define htobe64(x) __builtin_bswap64(x)
#endif
#ifndef be64toh
#define be64toh(x) __builtin_bswap64(x)
#endif
#ifndef htobe32
#define htobe32(x) __builtin_bswap32(x)
#endif
#ifndef be32toh
#define be32toh(x) __builtin_bswap32(x)
#endif
#endif // DISTRIBUTED_KVSTORE_MOCK_KV_STORE_IOS_COMPAT_H
