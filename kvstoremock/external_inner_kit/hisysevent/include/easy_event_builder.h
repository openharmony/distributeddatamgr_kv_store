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

#ifndef HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_BUILDER_H
#define HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_BUILDER_H

#include <stddef.h>
#include <stdint.h>

#include "easy_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DOMAIN_ARRAY_LEN  (MAX_DOMAIN_LENGTH + 1)
#define NAME_ARRAY_LEN    (MAX_EVENT_NAME_LENGTH + 1)

#pragma pack(1)
struct HiSysEventEasyHeader {
    char domain[DOMAIN_ARRAY_LEN];
    char name[NAME_ARRAY_LEN];
    uint64_t timestamp;
    uint8_t timeZone;
    uint32_t uid;
    uint32_t pid;
    uint32_t tid;
    uint64_t id;
    uint8_t type : 2;
    uint8_t isTraceOpened : 1;
};
#pragma pack()

/**
 * @brief Append event header to event
 *
 * @param eventBuffer  allocated memory of event
 * @param bufferLen    max length of the event buffer
 * @param offset       position to append header
 * @param header       event header
 * @return 0 means success, others means failure
 */
int AppendHeader(uint8_t* eventBuffer, const size_t bufferLen, size_t* offset,
    const struct HiSysEventEasyHeader* header);

/**
 * @brief Append customized param with string type to event
 *
 * @param eventBuffer  allocated memory of event
 * @param bufferLen    max length of the event buffer
 * @param offset       position to append string value
 * @param key          param key
 * @param val          string value
 * @return 0 means success, others means failure
 */
int AppendStringParam(uint8_t* eventBuffer, const size_t bufferLen, size_t* offset, const char* key, const char* val);

#ifdef __cplusplus
}
#endif
#endif // HISYSEVENT_INTERFACES_NATIVE_INNERKITS_EASY_BUILDER_H
