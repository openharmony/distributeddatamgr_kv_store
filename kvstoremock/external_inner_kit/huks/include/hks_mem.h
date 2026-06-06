/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef HKS_MEM_H
#define HKS_MEM_H

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>

#define HKS_NULL_POINTER nullptr
#else
#include <stdint.h>
#include <stdlib.h>

#define HKS_NULL_POINTER NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MALLOC_SIZE 0x800000 // 8M

void *HksMalloc(size_t size);
void HksFreeImpl(void *addr);
int32_t HksMemCmp(const void *ptr1, const void *ptr2, uint32_t size);

#define SELF_FREE_PTR(PTR, FREE_FUNC) \
{ \
    if ((PTR) != HKS_NULL_POINTER) { \
        FREE_FUNC(PTR); \
        (PTR) = HKS_NULL_POINTER; \
    } \
}

#define HKS_FREE(p) ({SELF_FREE_PTR(p, HksFreeImpl)})

#define HKS_FREE_PTR(p) SELF_FREE_PTR(p, HksFreeImpl)

#define HKS_FREE_BLOB(blob) do { \
    if ((blob).data != HKS_NULL_POINTER) { \
        HksFreeImpl((blob).data); \
        (blob).data = HKS_NULL_POINTER; \
    } \
    (blob).size = 0; \
} while (0)

#define HKS_MEMSET_FREE_PTR(ptr, size) \
{ \
    if ((ptr) != HKS_NULL_POINTER) { \
        (void)memset_s((ptr), (size), 0, (size)); \
        HksFreeImpl(ptr); \
        (ptr) = HKS_NULL_POINTER; \
    } \
}

#define HKS_MEMSET_FREE_BLOB(blob) do { \
    if ((blob).data != HKS_NULL_POINTER) { \
        (void)memset_s((blob).data, (blob).size, 0, (blob).size); \
        HksFreeImpl((blob).data); \
        (blob).data = HKS_NULL_POINTER; \
    } \
    (blob).size = 0; \
} while (0)

#define HKS_FREE_ENCAPSULATION_RESULT(encapResult) do { \
    if ((encapResult) != HKS_NULL_POINTER) { \
        HKS_MEMSET_FREE_BLOB((encapResult)->encapsulatedData); \
        HKS_MEMSET_FREE_BLOB((encapResult)->sharedSecret); \
    } \
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* HKS_MEM_H */
