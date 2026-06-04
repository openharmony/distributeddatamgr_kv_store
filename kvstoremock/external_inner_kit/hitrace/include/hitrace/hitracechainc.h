/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef HIVIEWDFX_HITRACECHAIN_C_H
#define HIVIEWDFX_HITRACECHAIN_C_H

#include <endian.h>
#include <stdarg.h>
#include <stdint.h>

#include "securec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HiTraceIdValid {
    HITRACE_ID_INVALID = 0,
    HITRACE_ID_VALID = 1,
} HiTraceIdValid;

typedef enum HiTraceVersion {
    HITRACE_VER_1 = 0,
} HiTraceVersion;

typedef enum HiTraceFlag {
    // MIN: valid.
    HITRACE_FLAG_MIN = 0,
    // DEFAULT: default value.
    HITRACE_FLAG_DEFAULT = 0,
    // trace sync and async call. default: trace sync call only.
    HITRACE_FLAG_INCLUDE_ASYNC = 1 << 0,
    // do not create child span. default: create child span.
    HITRACE_FLAG_DONOT_CREATE_SPAN = 1 << 1,
    // output tracepoint info in span. default: do not output tracepoint info.
    HITRACE_FLAG_TP_INFO = 1 << 2,
    // do not output begin and end info. default: output begin and end info.
    HITRACE_FLAG_NO_BE_INFO = 1 << 3,
    // do not add id to log. default: add id to log.
    HITRACE_FLAG_DONOT_ENABLE_LOG = 1 << 4,
    // the trace is triggered by fault.
    HITRACE_FLAG_FAULT_TRIGGER = 1 << 5,
    // output device-to-device tracepoint info in span only. default: do not output device-to-device tracepoint info.
    HITRACE_FLAG_D2D_TP_INFO = 1 << 6,
    // MAX: valid.
    HITRACE_FLAG_MAX = (1 << 7) - 1,
} HiTraceFlag;

// HiTrace tracepoint type
typedef enum HiTraceTracepointType {
    HITRACE_TP_MIN = 0,    // MIN: valid
    HITRACE_TP_CS = 0,      // client send
    HITRACE_TP_CR = 1,      // client receive
    HITRACE_TP_SS = 2,      // server send
    HITRACE_TP_SR = 3,      // server receive
    HITRACE_TP_GENERAL = 4, // general info
    HITRACE_TP_MAX = 4,     // MAX: valid
} HiTraceTracepointType;

// HiTrace communication mode
typedef enum HiTraceCommunicationMode {
    HITRACE_CM_MIN = 0,      // MIN: valid
    HITRACE_CM_DEFAULT = 0, // unspecified communication mode
    HITRACE_CM_THREAD = 1,  // thread-to-thread communication mode
    HITRACE_CM_PROCESS = 2, // process-to-process communication mode
    HITRACE_CM_DEVICE = 3,  // device-to-device communication mode
    HITRACE_CM_MAX = 3,     // MAX: valid
} HiTraceCommunicationMode;

typedef struct HiTraceIdStruct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint64_t valid : 1;
    uint64_t ver : 3;
    uint64_t chainId : 60;

    uint64_t flags : 12;
    uint64_t spanId : 26;
    uint64_t parentSpanId : 26;
#elif __BYTE_ORDER == __BIG_ENDIAN
    uint64_t chainId : 60;
    uint64_t ver : 3;
    uint64_t valid : 1;

    uint64_t parentSpanId : 26;
    uint64_t spanId : 26;
    uint64_t flags : 12;
#else
#error "ERROR: No BIG_LITTLE_ENDIAN defines."
#endif
} HiTraceIdStruct;

#define HITRACE_ID_LEN sizeof(HiTraceIdStruct)

HiTraceIdStruct HiTraceChainBegin(const char* name, int flags);
HiTraceIdStruct HiTraceChainBeginWithDomain(const char* name, int flags, unsigned int domain);
void HiTraceChainEnd(const HiTraceIdStruct* pId);
void HiTraceChainEndWithDomain(const HiTraceIdStruct* pId, unsigned int domain);
HiTraceIdStruct HiTraceChainGetId(void);
HiTraceIdStruct* HiTraceChainGetIdAddress(void);
void HiTraceChainSetId(const HiTraceIdStruct* pId);
void HiTraceChainClearId(void);
HiTraceIdStruct HiTraceChainCreateSpan(void);
HiTraceIdStruct HiTraceChainSaveAndSetId(const HiTraceIdStruct* pId);
void HiTraceChainRestoreId(const HiTraceIdStruct* oldId);
void HiTraceChainTracepoint(HiTraceTracepointType type, const HiTraceIdStruct* pId, const char* fmt, ...)
    __attribute__((__format__(os_log, 3, 4)));
void HiTraceChainTracepointWithArgs(HiTraceTracepointType type, const HiTraceIdStruct* pId, const char* fmt,
    va_list args);
void HiTraceChainTracepointEx(HiTraceCommunicationMode mode, HiTraceTracepointType type, const HiTraceIdStruct* pId,
    const char* fmt, ...) __attribute__((__format__(os_log, 4, 5)));
void HiTraceChainTracepointExWithDomain(HiTraceCommunicationMode mode, HiTraceTracepointType type,
    const HiTraceIdStruct* pId, unsigned int domain, const char* fmt, ...) __attribute__((__format__(os_log, 5, 6)));
void HiTraceChainTracepointExWithArgs(HiTraceCommunicationMode mode, HiTraceTracepointType type,
    const HiTraceIdStruct* pId, const char* fmt, va_list args);
void HiTraceChainTracepointExWithArgsDomain(HiTraceCommunicationMode mode, HiTraceTracepointType type,
    const HiTraceIdStruct* pId, unsigned int domain, const char* fmt, va_list args);

static inline void HiTraceChainInitId(HiTraceIdStruct* pId)
{
    pId->valid = HITRACE_ID_INVALID;
    pId->ver = 0;
    pId->chainId = 0;
    pId->flags = 0;
    pId->spanId = 0;
    pId->parentSpanId = 0;
}

static inline int HiTraceChainIsValid(const HiTraceIdStruct* pId)
{
    return (pId) && (pId->valid == HITRACE_ID_VALID);
}

static inline int HiTraceChainIsFlagEnabled(const HiTraceIdStruct* pId, HiTraceFlag flag)
{
    return HiTraceChainIsValid(pId) && ((pId->flags & (uint64_t)flag) != 0);
}

static inline void HiTraceChainEnableFlag(HiTraceIdStruct* pId, HiTraceFlag flag)
{
    if (HiTraceChainIsValid(pId)) {
        pId->flags |= (uint64_t)flag;
    }
    return;
}

static inline int HiTraceChainGetFlags(const HiTraceIdStruct* pId)
{
    if (!HiTraceChainIsValid(pId)) {
        return 0;
    }
    return pId->flags;
}

static inline void HiTraceChainSetFlags(HiTraceIdStruct* pId, int flags)
{
    if (HiTraceChainIsValid(pId) && (flags >= HITRACE_FLAG_MIN) && (flags < HITRACE_FLAG_MAX)) {
        pId->flags = (uint64_t)flags;
    }
    return;
}

static inline uint64_t HiTraceChainGetChainId(const HiTraceIdStruct* pId)
{
    if (!HiTraceChainIsValid(pId)) {
        return 0;
    }
    return pId->chainId;
}

static inline void HiTraceChainSetChainId(HiTraceIdStruct* pId, uint64_t chainId)
{
    if (!pId || chainId == 0) {
        return;
    }

    if (!HiTraceChainIsValid(pId)) {
        pId->valid = HITRACE_ID_VALID;
        pId->ver = HITRACE_VER_1;
        pId->flags = pId->spanId = pId->parentSpanId = 0;
    }
    pId->chainId = chainId;
}

static inline uint64_t HiTraceChainGetSpanId(const HiTraceIdStruct* pId)
{
    if (!HiTraceChainIsValid(pId)) {
        return 0;
    }
    return pId->spanId;
}

static inline void HiTraceChainSetSpanId(HiTraceIdStruct* pId, uint64_t spanId)
{
    if (HiTraceChainIsValid(pId)) {
        pId->spanId = spanId;
    }
    return;
}

static inline uint64_t HiTraceChainGetParentSpanId(const HiTraceIdStruct* pId)
{
    if (!HiTraceChainIsValid(pId)) {
        return 0;
    }
    return pId->parentSpanId;
}

static inline void HiTraceChainSetParentSpanId(HiTraceIdStruct* pId, uint64_t parentSpanId)
{
    if (HiTraceChainIsValid(pId)) {
        pId->parentSpanId = parentSpanId;
    }
    return;
}

static inline int HiTraceChainIdToBytes(const HiTraceIdStruct* pId, uint8_t* pIdArray, int len)
{
    if (!HiTraceChainIsValid(pId) || (len < (int)HITRACE_ID_LEN)) {
        return 0;
    }

    *((uint64_t*)pIdArray) = htobe64(*((uint64_t*)pId));
    *((uint64_t*)pIdArray + 1) = htobe64(*((uint64_t*)pId + 1));
    return sizeof(HiTraceIdStruct);
}

static inline HiTraceIdStruct HiTraceChainBytesToId(const uint8_t* pIdArray, int len)
{
    HiTraceIdStruct id = {0, 0, 0, 0, 0, 0};
    HiTraceChainInitId(&id);

    if ((!pIdArray) || (len != (int)HITRACE_ID_LEN)) {
        return id;
    }

    uint64_t tmp1 = 0;
    uint64_t tmp2 = 0;
    if (memcpy_s(&tmp1, sizeof(tmp1), pIdArray, sizeof(uint64_t)) != EOK ||
        memcpy_s(&tmp2, sizeof(tmp2), pIdArray + sizeof(uint64_t), sizeof(uint64_t)) != EOK) {
        return id;
    }
    *((uint64_t*)(&id)) = be64toh(tmp1);
    *((uint64_t*)(&id) + 1) = be64toh(tmp2);
    return id;
}

#ifdef __cplusplus
}
#endif

#endif // HIVIEWDFX_HITRACECHAIN_C_H
