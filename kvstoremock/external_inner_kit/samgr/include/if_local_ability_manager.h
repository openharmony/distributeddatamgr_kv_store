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

#ifndef IF_LOCAL_ABILITY_MANAGER_H
#define IF_LOCAL_ABILITY_MANAGER_H

#include <string>
#include <unordered_map>
#include "iremote_broker.h"
#include "iremote_object.h"
#include "iremote_stub.h"
#include "iremote_proxy.h"
#include "nlohmann/json.hpp"
#include "safwk_ipc_interface_code.h"

namespace OHOS {
enum {
    IPC_STAT_CMD_START = 0,
    IPC_STAT_CMD_STOP = 1,
    IPC_STAT_CMD_GET = 2,
    IPC_STAT_CMD_MAX = 3
};
enum {
    FFRT_STAT_CMD_START = 0,
    FFRT_STAT_CMD_STOP = 1,
    FFRT_STAT_CMD_GET = 2,
    FFRT_STAT_CMD_MAX = 3
};
class SystemAbilityExtensionPara {
public:
    SystemAbilityExtensionPara()
    {
        data_ = nullptr;
        reply_ = nullptr;
    };
    virtual ~SystemAbilityExtensionPara() {};

    MessageParcel *data_;
    MessageParcel *reply_;
    virtual bool InputParaSet(MessageParcel& data)
    {
        (void)data;
        return true;
    };
    virtual bool OutputParaGet(MessageParcel& reply)
    {
        (void)reply;
        return true;
    };
};
class ILocalAbilityManager : public IRemoteBroker {
public:
    virtual bool StartAbility(int32_t systemAbilityId, const std::string& eventStr) = 0;
    virtual bool StopAbility(int32_t systemAbilityId, const std::string& eventStr) = 0;
    virtual bool ActiveAbility(int32_t systemAbilityId,
        const nlohmann::json& activeReason) = 0;
    virtual bool IdleAbility(int32_t systemAbilityId,
        const nlohmann::json& idleReason, int32_t& delayTime) = 0;
    virtual bool SendStrategyToSA(int32_t type, int32_t systemAbilityId, int32_t level, std::string& action) = 0;
    virtual bool IpcStatCmdProc(int32_t fd, int32_t cmd) = 0;
    virtual bool FfrtStatCmdProc(int32_t fd, int32_t cmd) = 0;
    virtual bool FfrtDumperProc(std::string& result) = 0;
    virtual int32_t SystemAbilityExtProc(const std::string& extension, int32_t said,
        SystemAbilityExtensionPara* callback, bool isAsync = false) = 0;
    virtual int32_t ServiceControlCmd(int32_t fd, int32_t systemAbilityId,
        const std::vector<std::u16string>& args) = 0;
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.ILocalAbilityManager");
protected:
    static inline const std::u16string LOCAL_ABILITY_MANAGER_INTERFACE_TOKEN = u"ohos.localabilitymanager.accessToken";
};
}
#endif // !defined(IF_LOCAL_ABILITY_MANAGER_H)
