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

#include "concurrent_adapter.h"

namespace DistributedDB {
int ConcurrentAdapter::ScheduleTask(const TaskAction &action, Dependence inDeps, Dependence outDeps)
{
#ifdef USE_FFRT
    if (inDeps == nullptr && outDeps == nullptr) {
        ffrt::submit(action);
    } else if (inDeps == nullptr) {
        ffrt::submit(action, {}, { outDeps });
    } else if (outDeps == nullptr) {
        ffrt::submit(action, { inDeps }, {});
    } else {
        ffrt::submit(action, { inDeps }, { outDeps });
    }
    return E_OK;
#else
    return RuntimeContext::GetInstance()->ScheduleTask(action);
#endif
}

TaskHandle ConcurrentAdapter::ScheduleTaskH(const TaskAction &action, Dependence inDeps, Dependence outDeps)
{
#ifdef USE_FFRT
    if (inDeps == nullptr && outDeps == nullptr) {
        return ffrt::submit_h(action);
    } else if (inDeps == nullptr) {
        return ffrt::submit_h(action, {}, { outDeps });
    } else if (outDeps == nullptr) {
        return ffrt::submit_h(action, { inDeps }, {});
    } else {
        return ffrt::submit_h(action, { inDeps }, { outDeps });
    }
#else
    (void)action();
    return 0;
#endif
}
}