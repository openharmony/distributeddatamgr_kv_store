/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_TASK_PRIORITY_H
#define OHOS_ABILITY_RUNTIME_TASK_PRIORITY_H

namespace OHOS {
namespace AppExecFwk {
enum TaskPriority {
    /**
     * Indicates the highest task priority. Tasks with this priority are executed before other tasks.
     *
     * @see #DEFAULT
     * @see #LOW
     * @since 1
     */
    HIGH,
    /**
     * Indicates the default task priority. Tasks with this priority are executed after those with the {@link #HIGH}
     * priority are complete.
     *
     * @see #HIGH
     * @see #LOW
     * @since 1
     */
    DEFAULT,
    /**
     * Indicates the low task priority. Tasks with this priority are executed after those with the {@link #HIGH} or
     * {@link #DEFAULT} priority are complete.
     *
     * @see #HIGH
     * @see #DEFAULT
     * @since 1
     */
    LOW
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif
