/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ACTIVE_BOOTTIME_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ACTIVE_BOOTTIME_H

#if __linux__
#include <sys/time.h>
#endif
#include <chrono>

namespace OHOS {
class Boottime final {
public:
    static constexpr int64_t SECONDS_TO_NANO = 1000000000;

    using NanoSec = std::chrono::nanoseconds;
    using TimePoint = std::chrono::steady_clock::time_point;

    static TimePoint Now()
    {
        #if __linux__
        struct timespec tv {};
        if (clock_gettime(CLOCK_BOOTTIME, &tv) < 0) {
            return TimePoint(NanoSec(0));
        }
        auto time = NanoSec(tv.tv_sec * SECONDS_TO_NANO + tv.tv_nsec);
        return TimePoint(time);
        #else
        return std::chrono::steady_clock::now();
        #endif
    }
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ACTIVE_BOOTTIME_H
