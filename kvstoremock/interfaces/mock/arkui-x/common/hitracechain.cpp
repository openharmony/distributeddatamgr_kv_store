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

#include "hitrace/hitracechain.h"

namespace OHOS {
namespace HiviewDFX {

HiTraceId HiTraceChain::Begin(const std::string& name, int flags)
{
    return HiTraceId();
}

HiTraceId HiTraceChain::Begin(const std::string& name, int flags, unsigned int domain)
{
    return HiTraceId();
}

void HiTraceChain::End(const HiTraceId& id)
{
}

void HiTraceChain::End(const HiTraceId& id, unsigned int domain)
{
}

HiTraceId HiTraceChain::GetId()
{
    return HiTraceId();
}

HiTraceId* HiTraceChain::GetIdAddress()
{
    return nullptr;
}

void HiTraceChain::SetId(const HiTraceId& id)
{
}

void HiTraceChain::ClearId()
{
}

HiTraceId HiTraceChain::CreateSpan()
{
    return HiTraceId();
}

void HiTraceChain::Tracepoint(HiTraceTracepointType type, const HiTraceId& id, const char* fmt, ...)
{
}

void HiTraceChain::Tracepoint(HiTraceCommunicationMode mode, HiTraceTracepointType type, const HiTraceId& id,
    const char* fmt, ...)
{
}

void HiTraceChain::Tracepoint(HiTraceCommunicationMode mode, HiTraceTracepointType type, const HiTraceId& id,
    unsigned int domain, const char* fmt, ...)
{
}

HiTraceId HiTraceChain::SaveAndSet(const HiTraceId& id)
{
    return HiTraceId();
}

void HiTraceChain::Restore(const HiTraceId &id)
{
}
} // namespace HiviewDFX
} // namespace OHOS
