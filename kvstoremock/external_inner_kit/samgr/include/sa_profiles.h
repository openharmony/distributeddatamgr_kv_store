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

#ifndef SAMGR_INTERFACE_INNERKITS_COMMOM_INCLUDE_SAPROFILE_H
#define SAMGR_INTERFACE_INNERKITS_COMMOM_INCLUDE_SAPROFILE_H

#include <map>
#include <string>
#include <vector>
#include <list>

namespace OHOS {
using DlHandle = void*;

enum {
    INTERFACE_CALL = 0,
    DEVICE_ONLINE,
    SETTING_SWITCH,
    PARAM,
    COMMON_EVENT,
    TIMED_EVENT,
    UNREF_EVENT,
};

enum {
    // sa load key stage
    SA_LOAD_OPENSO = 1,
    SA_LOAD_ON_START,

    // sa unload key stage
    SA_UNLOAD_ON_STOP = 10,
};

enum {
    START_ON_DEMAND = 1,
    STOP_ON_DEMAND,
};

enum {
    START = 1,
    KILL,
    FREEZE,
};

enum {
    HIGH_PRIORITY = 1,
    MEDIUM_PRIORITY = 2,
    LOW_PRIORITY = 3,
};

enum {
    IMMEDIATELY = 0,
    LOW_MEMORY,
};

struct OnDemandCondition {
    int32_t eventId;
    std::string name;
    std::string value;
    std::map<std::string, std::string> extraMessages;
};

struct OnDemandEvent {
    int32_t eventId;
    std::string name;
    std::string value;
    int64_t extraDataId = -1;
    bool persistence = false;
    std::vector<OnDemandCondition> conditions;
    bool enableOnce = false;
    uint32_t loadPriority = LOW_PRIORITY;
    std::map<std::string, std::string> extraMessages;

    bool operator==(const OnDemandEvent& event) const
    {
        return this->eventId == event.eventId && this->name == event.name && this->value == event.value;
    }

    std::string ToString() const
    {
        return this->name + "_" + this->value;
    }
};

struct SaControlInfo {
    int32_t ondemandId;
    int32_t saId;
    bool enableOnce = false;
    uint32_t loadPriority = LOW_PRIORITY;
    bool cacheCommonEvent = false;
};

struct StartOnDemand {
    bool allowUpdate = false;
    std::vector<OnDemandEvent> onDemandEvents;
};

struct StopOnDemand {
    bool allowUpdate = false;
    bool unrefUnload = false;
    int32_t delayTime = 20000;
    int32_t unusedTimeout = -1;
    std::vector<OnDemandEvent> onDemandEvents;
};

struct SaProfile {
    std::u16string process;
    int32_t saId = 0;
    std::string libPath;
    std::vector<int32_t> dependSa;
    int32_t dependTimeout = 0;
    bool runOnCreate = false;
    bool moduleUpdate = false;
    bool autoRestart = false;
    bool distributed = false;
    int32_t dumpLevel = 0;
    // default OTHER_START
    uint32_t bootPhase = 3;
    StartOnDemand startOnDemand;
    StopOnDemand stopOnDemand;
    DlHandle handle = nullptr;
    int32_t recycleStrategy = IMMEDIATELY;
    std::list<std::string> extension;
    bool cacheCommonEvent = false;
};

struct CommonSaProfile {
    std::u16string process;
    int32_t saId = 0;
    bool moduleUpdate = false;
    bool distributed = false;
    bool cacheCommonEvent = false;
    bool startAllowUpdate = false;
    bool stopAllowUpdate = false;
    int32_t recycleStrategy = IMMEDIATELY;
    std::list<std::string> extension;
};

struct CollMgrSaProfile {
    int32_t saId = 0;
    std::vector<OnDemandEvent> startOnDemandEvents;
    std::vector<OnDemandEvent> stopOnDemandEvents;
    bool cacheCommonEvent = false;
};
}
#endif // SAMGR_INTERFACE_INNERKITS_COMMOM_INCLUDE_SAPROFILE_H
