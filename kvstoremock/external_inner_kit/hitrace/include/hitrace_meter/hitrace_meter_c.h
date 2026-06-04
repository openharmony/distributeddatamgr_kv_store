/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef HITRACE_METER_H
#define HITRACE_METER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HiTraceOutputLevel {
    HITRACE_LEVEL_DEBUG = 0,
    HITRACE_LEVEL_INFO = 1,
    HITRACE_LEVEL_CRITICAL = 2,
    HITRACE_LEVEL_COMMERCIAL = 3,
    HITRACE_LEVEL_MAX = HITRACE_LEVEL_COMMERCIAL,
} HiTraceOutputLevel;

typedef void (*TraceEventListener)(bool traceStatus);

void HiTraceStartTrace(uint64_t tag, const char* name);
void HiTraceFinishTrace(uint64_t tag);
void HiTraceStartAsyncTrace(uint64_t tag, const char* name, int32_t taskId);
void HiTraceFinishAsyncTrace(uint64_t tag, const char* name, int32_t taskId);
void HiTraceCountTrace(uint64_t tag, const char* name, int64_t count);

void HiTraceStartTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, const char* customArgs);
void HiTraceFinishTraceEx(HiTraceOutputLevel level, uint64_t tag);
void HiTraceStartAsyncTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, int32_t taskId,
    const char* customCategory, const char* customArgs);
void HiTraceFinishAsyncTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, int32_t taskId);
void HiTraceCountTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, int64_t count);
bool HiTraceIsTagEnabled(uint64_t tag);

int32_t HiTraceRegisterTraceListener(TraceEventListener callback);
int32_t HiTraceUnregisterTraceListener(int32_t index);

#ifdef __cplusplus
}
#endif
#endif /* HITRACE_METER_H */
