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

#ifndef CONCURRENT_ADAPTER_H
#define CONCURRENT_ADAPTER_H

#include "runtime_context.h"
#ifdef USE_FFRT
#include "ffrt.h"
#endif

namespace DistributedDB {
#ifdef USE_FFRT
#define ADAPTER_AUTO_LOCK(n, m)
#define ADAPTER_WAIT(x) ffrt::wait({x})
#else
#define ADAPTER_AUTO_LOCK(n, m) std::lock_guard<decltype(m)> n(m)
#define ADAPTER_WAIT(x) (void)(x)
#endif
using TaskHandle = void *;
using Dependence = void *;
class ConcurrentAdapter {
public:
    static int ScheduleTask(const TaskAction &action, Dependence inDeps = nullptr,
        Dependence outDeps = nullptr);
    static TaskHandle ScheduleTaskH(const TaskAction &action, Dependence inDeps = nullptr,
        Dependence outDeps = nullptr);
};
}

#endif // CONCURRENT_ADAPTER_H
