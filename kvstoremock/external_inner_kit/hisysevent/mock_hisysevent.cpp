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
#include "hisysevent_c.h"
#include "hisysevent.h"

int HiSysEvent_Write(const char* func, int64_t line, const char *domain, const char *name,
    HiSysEventEventType type, const HiSysEventParam params[], size_t size)
{
    return 0;
}

namespace OHOS {
namespace HiviewDFX {
void HiSysEvent::AppendHexData(EventBase &eventBase, const std::string &key, uint64_t value) {}

int HiSysEvent::CheckArraySize(unsigned long size) { return 0; }

int HiSysEvent::CheckKey(const std::string &key) { return 0; }

int HiSysEvent::CheckValue(const std::string &value) { return 0; }

void HiSysEvent::InnerWrite(EventBase &eventBase) {}

bool HiSysEvent::IsError(EventBase &eventBase) { return false; }

bool HiSysEvent::IsErrorAndUpdate(int retCode, EventBase &eventBase) { return false; }

bool HiSysEvent::IsWarnAndUpdate(int retCode, EventBase &eventBase) { return false; }

void HiSysEvent::SendSysEvent(EventBase &eventBase) {}

void HiSysEvent::WritebaseInfo(EventBase &eventBase) {}

bool HiSysEvent::UpdateAndCheckKeyNumIsOver(HiSysEvent::EventBase &eventBase) { return false; }

int HiSysEvent::ExplainThenReturnRetCode(const int retCode) { return retCode; }

uint64_t WriteController::GetCurrentTimeMills() { return 0; }

uint64_t WriteController::CheckLimitWritingEvent(const ControlParam& param, const char *domain,
    const char *eventName, const char *func, int64_t line) { return 0; }
}
}
