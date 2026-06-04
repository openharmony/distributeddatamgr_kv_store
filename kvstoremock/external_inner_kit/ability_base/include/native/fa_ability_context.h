/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_FA_ABILITY_CONTEXT_H
#define OHOS_ABILITY_RUNTIME_FA_ABILITY_CONTEXT_H

#include <map>
#include <memory>
#include <string>

#include "context_container.h"

namespace OHOS {
namespace AppExecFwk {

class AbilityContext : public ContextContainer {
public:
    AbilityContext() = default;
    virtual ~AbilityContext() = default;

    ErrCode StartAbility(const AAFwk::Want &want, int requestCode) override;
    ErrCode StartAbility(const Want &want, int requestCode, const AbilityStartSetting &abilityStartSetting) override;
    ErrCode TerminateAbility() override;
    std::string GetCallingBundle() override;
    std::shared_ptr<ElementName> GetElementName();
    std::shared_ptr<ElementName> GetCallingAbility();
    bool ConnectAbility(const Want &want, const sptr<AAFwk::IAbilityConnection> &conn) override;
    ErrCode DisconnectAbility(const sptr<AAFwk::IAbilityConnection> &conn) override;
    bool StopAbility(const AAFwk::Want &want) override;
    std::shared_ptr<Global::Resource::ResourceManager> GetResourceManager() const override;
    int VerifyPermission(const std::string &permission, int pid, int uid) override;
    void RequestPermissionsFromUser(std::vector<std::string> &permissions, std::vector<int> &permissionsState,
        PermissionRequestTask &&task) override;
    void SetCallingContext(const std::string &deviceId, const std::string &bundleName,
        const std::string &abilityName, const std::string &moduleName = "");
    void StartAbilities(const std::vector<AAFwk::Want> &wants) override;
    sptr<IRemoteObject> GetToken() override;

    void SetAbilityRecordId(int32_t abilityRecordId);
    int32_t GetAbilityRecordId() const;

    static int ABILITY_CONTEXT_DEFAULT_REQUEST_CODE;

protected:
    sptr<IRemoteObject> GetSessionToken();

    int resultCode_ = -1;
    int32_t abilityRecordId_ = 0;
    sptr<IRemoteObject> token_;
    sptr<IRemoteObject> sessionToken_;
    sptr<IRemoteObject> renderSession_;
    std::string callingDeviceId_;
    std::string callingBundleName_;
    std::string callingAbilityName_;
    std::string callingModuleName_;
    std::map<sptr<AAFwk::IAbilityConnection>, sptr<IRemoteObject>> abilityConnectionMap_;
    std::mutex sessionTokenMutex_;
    AAFwk::Want resultWant_;
};

}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // OHOS_ABILITY_RUNTIME_FA_ABILITY_CONTEXT_H