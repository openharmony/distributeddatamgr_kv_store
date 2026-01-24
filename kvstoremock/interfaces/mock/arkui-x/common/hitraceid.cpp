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

#include "hitraceid.h"

#include <cstdint>

namespace OHOS {
namespace HiviewDFX {

HiTraceId::HiTraceId()
{
    id_.valid = HITRACE_ID_INVALID;
    id_.ver = 0;
    id_.chainId = 0;
    id_.flags = 0;
    id_.spanId = 0;
    id_.parentSpanId = 0;
}

HiTraceId::HiTraceId(const HiTraceIdStruct& id) : id_(id)
{}

HiTraceId::HiTraceId(const uint8_t* pIdArray, int len)
{
    id_ = HiTraceChainBytesToId(pIdArray, len);
}

bool HiTraceId::IsValid() const
{
    return false;
}

bool HiTraceId::IsFlagEnabled(HiTraceFlag flag) const
{
    return false;
}

void HiTraceId::EnableFlag(HiTraceFlag flag)
{
}

int HiTraceId::GetFlags() const
{
    return 0;
}

void HiTraceId::SetFlags(int flags)
{
}

uint64_t HiTraceId::GetChainId() const
{
    return 0;
}

void HiTraceId::SetChainId(uint64_t chainId)
{
}

uint64_t HiTraceId::GetSpanId() const
{
    return 0;
}

void HiTraceId::SetSpanId(uint64_t spanId)
{
}

uint64_t HiTraceId::GetParentSpanId() const
{
    return 0;
}

void HiTraceId::SetParentSpanId(uint64_t parentSpanId)
{
}

int HiTraceId::ToBytes(uint8_t* pIdArray, int len) const
{
    return 0;
}

} // namespace HiviewDFX
} // namespace OHOS
