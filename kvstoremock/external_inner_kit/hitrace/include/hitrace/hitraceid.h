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

#ifndef HIVIEWDFX_HITRACE_ID_H
#define HIVIEWDFX_HITRACE_ID_H

#include "hitrace/hitracechainc.h"

#ifdef __cplusplus

namespace OHOS {
namespace HiviewDFX {
class HiTraceId final {
public:
    HiTraceId();
    HiTraceId(const HiTraceIdStruct& id);
    HiTraceId(const uint8_t* pIdArray, int len);
    ~HiTraceId() = default;

    // Judge whether the trace id is valid or not.
    bool IsValid() const;

    /**
     * @brief Judge whether the trace id has enabled a trace flag or not.
     * @param flag      the trace flag to check.
     * @return true means this trace flag has been enabled.
     */
    bool IsFlagEnabled(HiTraceFlag flag) const;

    /**
     * @brief Enable the designative trace flag for the trace id.
     * @param flag      the trace flag to check.
     */
    void EnableFlag(HiTraceFlag flag);

    // Get trace flag of the trace id.
    int GetFlags() const;

    /**
     * @brief Set trace flag for the trace id.
     * @param flag      the trace flag to set.
     */
    void SetFlags(int flags);

    // Get chain id of the trace id.
    uint64_t GetChainId() const;
 
    /**
     * @brief Set chain id for the trace id.
     * @param chainId      the chain id to set.
     */
    void SetChainId(uint64_t chainId);

    // Get span id of the trace id.
    uint64_t GetSpanId() const;

    /**
     * @brief Set span id for the trace id.
     * @param spanId      the span id to set.
     */
    void SetSpanId(uint64_t spanId);

    // Get parent span id of the trace id.
    uint64_t GetParentSpanId() const;

    /**
     * @brief Set parent span id for the trace id.
     * @param parentSpanId      the parent span id to set.
     */
    void SetParentSpanId(uint64_t parentSpanId);

    /**
     * @brief Serialize the trace id into bytes
     * @param pIdArray      bytes array which load the serialized trace id.
     * @param len           len of the array.
     */
    int ToBytes(uint8_t* pIdArray, int len) const;

private:
    HiTraceIdStruct id_;
    friend class HiTraceChain;
};
} // namespace HiviewDFX
} // namespace OHOS

#endif // __cplusplus

#endif // HIVIEWDFX_HITRACE_ID_H
