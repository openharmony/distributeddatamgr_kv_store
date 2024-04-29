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
#include "cloud_sync_state_machine.h"

#include <cmath>
#include <climits>
#include <algorithm>

#include "db_errno.h"
#include "log_print.h"
#include "runtime_context.h"

namespace DistributedDB {
using Event = CloudSyncEvent;
using State = CloudSyncState;
namespace {
    // used for state switch table
    constexpr int CURRENT_STATE_INDEX = 0;
    constexpr int EVENT_INDEX = 1;
    constexpr int OUTPUT_STATE_INDEX = 2;

    const std::vector<std::vector<uint8_t>> STATE_SWITCH_TABLE = {
        // In IDEL state
        {State::IDLE, Event::START_SYNC_EVENT, State::DO_DOWNLOAD},
        {State::IDLE, Event::ERROR_EVENT, State::DO_FINISHED},

        // In DO_DOWNLOAD state
        {State::DO_DOWNLOAD, Event::DOWNLOAD_FINISHED_EVENT, State::DO_UPLOAD},
        {State::DO_DOWNLOAD, Event::ERROR_EVENT, State::DO_FINISHED},

        // In DO_UPLOAD state
        {State::DO_UPLOAD, Event::UPLOAD_FINISHED_EVENT, State::DO_FINISHED},
        {State::DO_UPLOAD, Event::ERROR_EVENT, State::DO_FINISHED},
        {State::DO_UPLOAD, Event::REPEAT_DOWNLOAD_EVENT, State::DO_DOWNLOAD},

        // In DO_FINISHED state
        {State::DO_FINISHED, Event::ALL_TASK_FINISHED_EVENT, State::IDLE},
    };
}

std::mutex CloudSyncStateMachine::stateSwitchTableLock_;
std::vector<CloudStateSwitchTable> CloudSyncStateMachine::stateSwitchTables_;
bool CloudSyncStateMachine::isStateSwitchTableInited_ = false;

int CloudSyncStateMachine::Initialize()
{
    InitCloudStateSwitchTables();
    return E_OK;
}

void CloudSyncStateMachine::SyncStep()
{
    Event event = Event::ERROR_EVENT;
    do {
        auto iter = stateMapping_.find(currentState_);
        if (iter != stateMapping_.end()) {
            event = static_cast<Event>(iter->second());
        } else {
            LOGE("[CloudSyncStateMachine][SyncStep] can not find state=%d", currentState_);
            break;
        }
    } while (SwitchMachineState(event) == E_OK && currentState_ != State::IDLE);
}

void CloudSyncStateMachine::InitCloudStateSwitchTables()
{
    if (isStateSwitchTableInited_) {
        return;
    }

    std::lock_guard<std::mutex> lock(stateSwitchTableLock_);
    if (isStateSwitchTableInited_) {
        return;
    }

    InitCloudStateSwitchTable(STATE_SWITCH_TABLE);
    isStateSwitchTableInited_ = true;
}

void CloudSyncStateMachine::InitCloudStateSwitchTable(
    const std::vector<std::vector<uint8_t>> &switchTable)
{
    CloudStateSwitchTable table;
    for (const auto &stateSwitch : switchTable) {
        if (stateSwitch.size() <= OUTPUT_STATE_INDEX) {
            LOGE("[StateMachine][InitSwitchTable] stateSwitch size err,size=%zu", stateSwitch.size());
            return;
        }
        if (table.switchTable.count(stateSwitch[CURRENT_STATE_INDEX]) == 0) {
            EventToState eventToState; // new EventToState
            eventToState[stateSwitch[EVENT_INDEX]] = stateSwitch[OUTPUT_STATE_INDEX];
            table.switchTable[stateSwitch[CURRENT_STATE_INDEX]] = eventToState;
        } else { // key stateSwitch[CURRENT_STATE_INDEX] already has EventToState
            EventToState &eventToState = table.switchTable[stateSwitch[CURRENT_STATE_INDEX]];
            eventToState[stateSwitch[EVENT_INDEX]] = stateSwitch[OUTPUT_STATE_INDEX];
        }
    }
    stateSwitchTables_.push_back(table);
}

void CloudSyncStateMachine::RegisterFunc(State state, const std::function<uint8_t(void)> &function)
{
    stateMapping_[state] = function;
};

int CloudSyncStateMachine::SwitchMachineState(uint8_t event)
{
    auto tableIter = std::find_if(stateSwitchTables_.begin(), stateSwitchTables_.end(),
        [](const CloudStateSwitchTable &table) {
            return table.version <= 0;
        });
    if (tableIter == stateSwitchTables_.end()) {
        LOGE("[CloudSyncStateMachine][SwitchState] Can't find a compatible state switch table.");
        return -E_NOT_FOUND;
    }

    const std::map<uint8_t, EventToState> &table = (*tableIter).switchTable;
    auto eventToStateIter = table.find(currentState_);
    if (eventToStateIter == table.end()) {
        LOGE("[CloudSyncStateMachine][SwitchState] Can't find EventToState with currentSate %u",
            currentState_);
        return E_OK;
    }

    const EventToState &eventToState = eventToStateIter->second;
    auto stateIter = eventToState.find(event);
    if (stateIter == eventToState.end()) {
        LOGD("[CloudSyncStateMachine][SwitchState] Can't find event %u int currentSate %u ignore",
            event, currentState_);
        return -E_NOT_FOUND;
    }

    currentState_ = static_cast<CloudSyncState>(stateIter->second);
    return E_OK;
}

void CloudSyncStateMachine::SwitchStateAndStep(uint8_t event)
{
    if (SwitchMachineState(event) == E_OK) {
        SyncStep();
    }
}

} // namespace DistributedDB
