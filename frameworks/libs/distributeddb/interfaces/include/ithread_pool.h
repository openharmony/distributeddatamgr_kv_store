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

#ifndef ITHREAD_POOL_H
#define ITHREAD_POOL_H

#include <thread>
#include "store_types.h"

namespace DistributedDB {
using Task = std::function<void()>;
using TaskId = uint64_t;
using Duration = std::chrono::steady_clock::duration;
class IThreadPool {
public:
    IThreadPool() = default;
    virtual ~IThreadPool() = default;

    virtual TaskId Execute(const Task &task) = 0;

    virtual TaskId Execute(const Task &task, Duration delay) = 0;

    virtual TaskId Schedule(const Task &task, Duration interval) = 0;

    virtual TaskId Schedule(const Task &task, Duration delay, Duration interval) = 0;

    virtual TaskId Schedule(const Task &task, Duration delay, Duration interval, uint64_t times) = 0;

    virtual bool Remove(const TaskId &taskId, bool wait) = 0;

    virtual TaskId Reset(const TaskId &taskId, Duration interval) = 0;
};
}
#endif // ITHREAD_POOL_H
