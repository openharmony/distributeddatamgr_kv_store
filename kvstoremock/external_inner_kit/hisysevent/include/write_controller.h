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

#ifndef WRITE_CONTROLLER_H
#define WRITE_CONTROLLER_H

#include <list>
#include <mutex>
#include <sys/time.h>
#include <unordered_map>

namespace OHOS {
namespace HiviewDFX {
static constexpr uint64_t INVALID_TIME_STAMP = 0;
static constexpr size_t HISYSEVENT_DEFAULT_PERIOD = 5;
static constexpr size_t HISYSEVENT_DEFAULT_THRESHOLD = 100;

using ControlParam = struct {
    size_t period;
    size_t threshold;
};

using CallerInfo = struct {
    const char* func;
    int64_t line;
    uint64_t timeStamp;
};

class EventWroteLruCache;

class WriteController {
public:
    static uint64_t GetCurrentTimeMills();
    static uint64_t CheckLimitWritingEvent(const ControlParam& param, const char* domain, const char* eventName,
        const CallerInfo& callerInfo);
    static uint64_t CheckLimitWritingEvent(const ControlParam& param, const char* domain, const char* eventName,
        const char* func, int64_t line);

private:
    static EventWroteLruCache eventWroteLruCache_;
};
} // HiviewDFX
} // OHOS

#endif // WRITE_CONTROLLER_H
