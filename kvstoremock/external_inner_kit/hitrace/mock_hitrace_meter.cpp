/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "hitrace/trace.h"
#include "hitrace_meter/hitrace_meter.h"
#ifdef __cplusplus
extern "C" {
#endif
void StartTrace(uint64_t label, const std::string& value, float limit) {};
void StartTraceEx(HiTraceOutputLevel level, uint64_t tag, const char* name, const char* customArgs) {};
void MiddleTrace(uint64_t label, const std::string& beforeValue, const std::string& afterValue) {};
void FinishTrace(uint64_t label) {};
void FinishTraceEx(HiTraceOutputLevel level, uint64_t tag) {};
void StartAsyncTrace(uint64_t label, const std::string& name, int32_t taskId) {};
void FinishAsyncTrace(uint64_t label, const std::string& name, int32_t taskId) {};
void UpdateTraceLabel() {}
#ifdef __cplusplus
}
#endif
namespace OHOS::HiviewDFX {
HiTraceId::HiTraceId()
{}

HiTraceId::HiTraceId(const HiTraceIdStruct &id)
{
}
HiTraceId::HiTraceId(const uint8_t *pIdArray, int len)
{
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
int HiTraceId::ToBytes(uint8_t *pIdArray, int len) const
{
    return 0;
}
HiTraceId HiTraceChain::Begin(const std::string& name, int flags)
{
    return {};
}
void HiTraceChain::End(const HiTraceId &id)
{
}
HiTraceId HiTraceChain::GetId()
{
    return HiTraceId();
}
void HiTraceChain::SetId(const HiTraceId &id)
{
}
void HiTraceChain::ClearId()
{
}
HiTraceId HiTraceChain::CreateSpan()
{
    return HiTraceId();
}
}