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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_INNER_TASK_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_INNER_TASK_H

namespace OHOS {
using TaskId = uint64_t;
using Duration = std::chrono::steady_clock::duration;
using Task = std::function<void()>;
using Time = std::chrono::steady_clock::time_point;

static constexpr TaskId INVALID_TASKID = static_cast<uint64_t>(0l);
static constexpr Duration INVALID_INTERVAL = std::chrono::milliseconds(0);
static constexpr uint64_t UNLIMITED_TIMES = std::numeric_limits<uint64_t>::max();

class InnerTask {
public:
    Duration interval = INVALID_INTERVAL;
    uint64_t times = UNLIMITED_TIMES;
    TaskId taskId = INVALID_TASKID;
    Time startTime;
    Task exec;
    Duration execTime;
    bool Valid()
    {
        if (exec) {
            return true;
        }
        return false;
    }
    TaskId GetId() {
        return taskId;
    }
};
} // namespace OHOS
#endif //OHOS_DISTRIBUTED_DATA_FRAMEWORKS_COMMON_EXECUTOR_POOL_INNER_TASK_H
