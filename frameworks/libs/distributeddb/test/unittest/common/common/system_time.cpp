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

#include "system_time.h"

#include <atomic>
#include <sys/time.h>

#include "log_print.h"
#include "platform_specific.h"

namespace {
    const uint64_t MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS = 1000000;
    std::atomic<int64_t> g_timeOffset(0);
}

namespace DistributedDB {
namespace OS {

#ifdef DB_DEBUG_ENV
int GetCurrentSysTimeInMicrosecond(uint64_t &outTime)
{
    struct timeval rawTime;
    int errCode = gettimeofday(&rawTime, nullptr);
    if (errCode < 0) {
        LOGE("[GetSysTime] Fail:%d.", errCode);
        return errCode;
    }
    outTime = static_cast<uint64_t>(rawTime.tv_sec) * MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS +
        static_cast<uint64_t>(rawTime.tv_usec);
    outTime = outTime + g_timeOffset.load();
    return 0;
}
#endif // DB_DEBUG_ENV

void SetOffsetBySecond(int64_t inSecond)
{
    int64_t microSecond = static_cast<int64_t>(inSecond) * MULTIPLES_BETWEEN_SECONDS_AND_MICROSECONDS; // to ms
    g_timeOffset.store(microSecond);
    LOGD("[SetTimeOffset] offset : %llds", inSecond);
}
}
}
