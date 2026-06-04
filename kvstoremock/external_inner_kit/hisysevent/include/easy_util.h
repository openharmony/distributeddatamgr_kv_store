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

#ifndef HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_UTIL_H
#define HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_UTIL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize memory with designated length
 *
 * @param data      pointer to the memory which need to be initilized
 * @param dataLen   byte count of memory which need to be initilized
 */
int MemoryInit(uint8_t* data, const size_t dataLen);

/**
 * @brief Copy memory with designated length
 *
 * @param dest     pointer to the dest memory
 * @param destLen  length of the dest memory
 * @param src      pointer to the src memory
 * @param srcLen   length of the dest memory
 */
int MemoryCopy(uint8_t* dest, size_t destLen, uint8_t* src, const size_t srcLen);

#ifdef __cplusplus
}
#endif
#endif // HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_UTIL_H
