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

#ifndef HIVIEWDFX_HITRACECHAIN_CPP_H
#define HIVIEWDFX_HITRACECHAIN_CPP_H

#include "hitrace/hitraceid.h"

#ifdef __cplusplus

#include <string>

namespace OHOS {
namespace HiviewDFX {
class HiTraceChain final {
public:
    /**
     * @brief Start tracing a process impl.
     * @param name      name or description for current trace.
     * @param flags     trace flags to be set for current trace.
     * @return trace id.
     */
    static HiTraceId Begin(const std::string& name, int flags);

    /**
     * @brief Start tracing a process impl.
     * @param name      name or description for current trace.
     * @param flags     trace flags to be set for current trace.
     * @param domain    hilog domain.
     * @return trace id.
     */
    static HiTraceId Begin(const std::string& name, int flags, unsigned int domain);

    /**
     * @brief Stop process tracing and clear trace id of current thread
     *     if the given trace id is valid, otherwise do nothing.
     * @param id        the trace id to end.
     */
    static void End(const HiTraceId& id);

    /**
     * @brief Stop process tracing and clear trace id of current thread
     *     if the given trace id is valid, otherwise do nothing.
     * @param id        the trace id to end.
     * @param domain    hilog domain.
     */
    static void End(const HiTraceId& id, unsigned int domain);

    // Get trace id of current thread, and return a invalid trace id if no
    // trace id belong to current thread.
    static HiTraceId GetId();

    // Get trace id address of current thread.
    static HiTraceId* GetIdAddress();

    /**
     * @brief Set id as trace id of current thread. Do nothing if id is invalid.
     * @param id      the trace id to set.
     */
    static void SetId(const HiTraceId& id);

    // Clear trace id of current thread and set it invalid.
    static void ClearId();

    // Create a new span id according to the trace id of current thread.
    static HiTraceId CreateSpan();

    /**
     * @brief tracepoint hitracechain information.
     * @param type      type of tracepoint.
     * @param id        the trace id of current thread.
     * @param fmt       format of trace information.
     */
    static void Tracepoint(HiTraceTracepointType type, const HiTraceId& id, const char* fmt, ...)
        __attribute__((__format__(os_log, 3, 4)));

    /**
     * @brief tracepoint hitracechain information.
     * @param mode      mode of communication.
     * @param type      type of tracepoint.
     * @param id        the trace id of current thread.
     * @param fmt       format of trace information.
     */
    static void Tracepoint(HiTraceCommunicationMode mode, HiTraceTracepointType type, const HiTraceId& id,
        const char* fmt, ...) __attribute__((__format__(os_log, 4, 5)));

    /**
     * @brief tracepoint hitracechain information.
     * @param mode      mode of communication.
     * @param type      type of tracepoint.
     * @param id        the trace id of current thread.
     * @param domain    hilog domain.
     * @param fmt       format of trace information.
     */
    static void Tracepoint(HiTraceCommunicationMode mode, HiTraceTracepointType type, const HiTraceId& id,
        unsigned int domain, const char* fmt, ...) __attribute__((__format__(os_log, 5, 6)));

    /**
     * @brief set the target id and return the old id.
     * @param id the trace id of target id.
     */
    static HiTraceId SaveAndSet(const HiTraceId& id);

    /**
     * @brief restore the current thread id.
     */
    static void Restore(const HiTraceId& id);
private:
    HiTraceChain() = default;
    ~HiTraceChain() = default;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // __cplusplus

#endif // HIVIEWDFX_HITRACECHAIN_CPP_H
