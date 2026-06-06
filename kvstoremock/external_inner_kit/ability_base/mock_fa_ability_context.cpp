/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "native/fa_ability_context.h"

namespace OHOS::AppExecFwk {

int AbilityContext::ABILITY_CONTEXT_DEFAULT_REQUEST_CODE = -1;

ErrCode AbilityContext::StartAbility(const AAFwk::Want &want, int requestCode)
{
    return 0;
}

ErrCode AbilityContext::StartAbility(const Want &want, int requestCode, const AbilityStartSetting &abilityStartSetting)
{
    return 0;
}

ErrCode AbilityContext::TerminateAbility()
{
    return 0;
}

std::string AbilityContext::GetCallingBundle()
{
    return "";
}

std::shared_ptr<ElementName> AbilityContext::GetElementName()
{
    return std::shared_ptr<ElementName>();
}

std::shared_ptr<ElementName> AbilityContext::GetCallingAbility()
{
    return std::shared_ptr<ElementName>();
}

bool AbilityContext::ConnectAbility(const Want &want, const sptr<AAFwk::IAbilityConnection> &conn)
{
    return false;
}

ErrCode AbilityContext::DisconnectAbility(const sptr<AAFwk::IAbilityConnection> &conn)
{
    return 0;
}

bool AbilityContext::StopAbility(const AAFwk::Want &want)
{
    return false;
}

std::shared_ptr<Global::Resource::ResourceManager> AbilityContext::GetResourceManager() const
{
    return nullptr;
}

int AbilityContext::VerifyPermission(const std::string &permission, int pid, int uid)
{
    return 0;
}

void AbilityContext::RequestPermissionsFromUser(std::vector<std::string> &permissions,
    std::vector<int> &permissionsState, PermissionRequestTask &&task)
{
}

void AbilityContext::SetCallingContext(const std::string &deviceId, const std::string &bundleName,
    const std::string &abilityName, const std::string &moduleName)
{
    callingDeviceId_ = deviceId;
    callingBundleName_ = bundleName;
    callingAbilityName_ = abilityName;
    callingModuleName_ = moduleName;
}

void AbilityContext::StartAbilities(const std::vector<AAFwk::Want> &wants)
{
}

sptr<IRemoteObject> AbilityContext::GetToken()
{
    return token_;
}

sptr<IRemoteObject> AbilityContext::GetSessionToken()
{
    return sessionToken_;
}

void AbilityContext::SetAbilityRecordId(int32_t abilityRecordId)
{
    abilityRecordId_ = abilityRecordId;
}

int32_t AbilityContext::GetAbilityRecordId() const
{
    return abilityRecordId_;
}

}  // namespace OHOS::AppExecFwk