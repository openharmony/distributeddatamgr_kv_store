/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef CLOUD_SYNC_STATE_MACHINE_H
#define CLOUD_SYNC_STATE_MACHINE_H

#include "cloud/cloud_store_types.h"

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace DistributedDB {
using EventToState = std::map<uint8_t, uint8_t>;
using StateMappingHandler = std::function<uint8_t(void)>;

struct CloudStateSwitchTable {
    uint32_t version = 0;
    std::map<uint8_t, EventToState> switchTable;
};

class CloudSyncStateMachine {
public:
    CloudSyncStateMachine() = default;

    ~CloudSyncStateMachine() = default;

    // Init the CloudSyncStateMachine
    int Initialize();

    void SwitchStateAndStep(uint8_t event);

    void RegisterFunc(CloudSyncState state, const std::function<uint8_t(void)> &function);

protected:
    // Step the CloudSyncStateMachine
    void SyncStep();

private:
    // Used to init sync state machine switchbables
    static void InitCloudStateSwitchTables();

    // To generate the statemachine switchtable with the given version
    static void InitCloudStateSwitchTable(const std::vector<std::vector<uint8_t>> &switchTable);

    int SwitchMachineState(uint8_t event);

    static std::mutex stateSwitchTableLock_;
    static bool isStateSwitchTableInited_;
    static std::vector<CloudStateSwitchTable> stateSwitchTables_;
    std::map<uint8_t, StateMappingHandler> stateMapping_;
    CloudSyncState currentState_ = CloudSyncState::IDLE;
};
} // namespace DistributedDB

#endif // CLOUD_SYNC_STATE_MACHINE_H
