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

#ifndef MOCK_TIME_SYNC_H
#define MOCK_TIME_SYNC_H

#include <gmock/gmock.h>
#include "time_sync.h"

namespace DistributedDB {
class MockTimeSync : public TimeSync {
public:
    void ModifyTimer(int milliSeconds)
    {
        std::lock_guard<std::mutex> lock(timeDriverLock_);
        RuntimeContext::GetInstance()->ModifyTimer(driverTimerId_, milliSeconds);
    }
};
}
#endif // MOCK_TIME_SYNC_H
