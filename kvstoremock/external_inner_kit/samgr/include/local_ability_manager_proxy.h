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

#ifndef LOCAL_ABILITY_MANAGER_PROXY_H
#define LOCAL_ABILITY_MANAGER_PROXY_H

#include <string>
#include "hilog/log.h"
#include "if_local_ability_manager.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "refbase.h"

namespace OHOS {
class LocalAbilityManagerProxy : public IRemoteProxy<ILocalAbilityManager> {
public:
    explicit LocalAbilityManagerProxy(const sptr<IRemoteObject>& impl)
        : IRemoteProxy<ILocalAbilityManager>(impl) {}
    ~LocalAbilityManagerProxy() = default;

    bool StartAbility(int32_t systemAbilityId, const std::string& eventStr);
    bool StopAbility(int32_t systemAbilityId, const std::string& eventStr);
    bool ActiveAbility(int32_t systemAbilityId,
        const nlohmann::json& activeReason);
    bool IdleAbility(int32_t systemAbilityId,
        const nlohmann::json& idleReason, int32_t& delayTime);
    bool SendStrategyToSA(int32_t type, int32_t systemAbilityId, int32_t level, std::string& action);
    bool IpcStatCmdProc(int32_t fd, int32_t cmd);
    bool FfrtStatCmdProc(int32_t fd, int32_t cmd);
    bool FfrtDumperProc(std::string& ffrtDumperInfo);
    int32_t SystemAbilityExtProc(const std::string& extension, int32_t said,
        SystemAbilityExtensionPara* callback, bool isAsync = false);
    int32_t ServiceControlCmd(int32_t fd, int32_t systemAbilityId, const std::vector<std::u16string>& args);
private:
    static inline BrokerDelegator<LocalAbilityManagerProxy> delegator_;
    bool PrepareData(MessageParcel& data, int32_t said, const std::string& extension);
};
}
#endif // !defined(LOCAL_ABILITY_MANAGER_PROXY_H)
