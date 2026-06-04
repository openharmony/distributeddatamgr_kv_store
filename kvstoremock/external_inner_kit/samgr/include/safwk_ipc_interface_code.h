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

#ifndef SAFWK_IPC_INTERFACE_CODE_H
#define SAFWK_IPC_INTERFACE_CODE_H

/* SAFWK */
namespace OHOS {
enum SafwkInterfaceCode : uint32_t {
    START_ABILITY_TRANSACTION = 1,
    STOP_ABILITY_TRANSACTION = 2,
    ACTIVE_ABILITY_TRANSACTION = 3,
    IDLE_ABILITY_TRANSACTION = 4,
    SEND_STRATEGY_TO_SA_TRANSACTION = 5,
    IPC_STAT_CMD_TRANSACTION = 6,
    FFRT_DUMPER_TRANSACTION = 7,
    SYSTEM_ABILITY_EXT_TRANSACTION = 8,
    SERVICE_CONTROL_CMD_TRANSACTION = 9,
    FFRT_STAT_CMD_TRANSACTION = 10,
};
}
#endif // !defined(SAFWK_IPC_INTERFACE_CODE_H)
