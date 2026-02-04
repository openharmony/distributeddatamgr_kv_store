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

#include "time_helper.h"

#include "platform_specific.h"

namespace DistributedDB {
std::mutex TimeHelper::systemTimeLock_;
Timestamp TimeHelper::lastSystemTimeUs_ = 0;
Timestamp TimeHelper::currentIncCount_ = 0;

Timestamp TimeHelper::GetSysCurrentTime()
{
    uint64_t curTime = 0;
    std::lock_guard<std::mutex> lock(systemTimeLock_);
    int errCode = OS::GetCurrentSysTimeInMicrosecond(curTime);
    if (errCode != E_OK) {
        return INVALID_TIMESTAMP;
    }

    // If GetSysCurrentTime in 1us, we need increase the currentIncCount_
    if (curTime == lastSystemTimeUs_) {
        // if the currentIncCount_ has been increased MAX_INC_COUNT, keep the currentIncCount_
        if (currentIncCount_ < MAX_INC_COUNT) {
            currentIncCount_++;
        }
    } else {
        lastSystemTimeUs_ = curTime;
        currentIncCount_ = 0;
    }
    return (curTime * TO_100_NS) + currentIncCount_;  // Currently Timestamp is uint64_t
}
}  // namespace DistributedDB
