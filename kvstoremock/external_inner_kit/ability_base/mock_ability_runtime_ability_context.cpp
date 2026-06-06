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

#include "ability_runtime/mock_ability_context.h"

namespace OHOS::AbilityRuntime {

const size_t AbilityContext::CONTEXT_TYPE_ID = 0;

ErrCode MockAbilityContext::StartAbility(const AAFwk::Want &want, int requestCode)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbilityWithAccount(const AAFwk::Want &want, int accountId, int requestCode)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbility(const AAFwk::Want &want, const AAFwk::StartOptions &startOptions,
    int requestCode)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbilityAsCaller(const AAFwk::Want &want, int requestCode)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbilityAsCaller(const AAFwk::Want &want,
    const AAFwk::StartOptions &startOptions, int requestCode)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbilityWithAccount(const AAFwk::Want &want, int accountId,
    const AAFwk::StartOptions &startOptions, int requestCode)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbilityForResult(const AAFwk::Want &want, int requestCode, RuntimeTask &&task)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbilityForResultWithAccount(const AAFwk::Want &want, int accountId,
    int requestCode, RuntimeTask &&task)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbilityForResult(const AAFwk::Want &want,
    const AAFwk::StartOptions &startOptions, int requestCode, RuntimeTask &&task)
{
    return 0;
}

ErrCode MockAbilityContext::StartAbilityForResultWithAccount(const AAFwk::Want &want, int accountId,
    const AAFwk::StartOptions &startOptions, int requestCode, RuntimeTask &&task)
{
    return 0;
}

ErrCode MockAbilityContext::StartServiceExtensionAbility(const AAFwk::Want &want, int32_t accountId)
{
    return 0;
}

ErrCode MockAbilityContext::StartUIServiceExtensionAbility(const AAFwk::Want &want, int32_t accountId)
{
    return 0;
}

ErrCode MockAbilityContext::StopServiceExtensionAbility(const AAFwk::Want& want, int32_t accountId)
{
    return 0;
}

ErrCode MockAbilityContext::TerminateAbilityWithResult(const AAFwk::Want &want, int resultCode)
{
    return 0;
}

ErrCode MockAbilityContext::BackToCallerAbilityWithResult(const AAFwk::Want &want, int resultCode, int64_t requestCode)
{
    return 0;
}

ErrCode MockAbilityContext::RestoreWindowStage(void* env, void* contentStorage)
{
    return 0;
}

ErrCode MockAbilityContext::RestoreWindowStage(void *contentStorage)
{
    return 0;
}

void MockAbilityContext::OnAbilityResult(int requestCode, int resultCode, const AAFwk::Want &resultData)
{
}

ErrCode MockAbilityContext::RequestModalUIExtension(const AAFwk::Want& want)
{
    return 0;
}

ErrCode MockAbilityContext::OpenLink(const AAFwk::Want &want, int requestCode, bool hideFailureTipDialog)
{
    return 0;
}

ErrCode MockAbilityContext::OpenAtomicService(AAFwk::Want& want, const AAFwk::StartOptions &options,
    int requestCode, RuntimeTask &&task)
{
    return 0;
}

ErrCode MockAbilityContext::AddFreeInstallObserver(const sptr<AbilityRuntime::IFreeInstallObserver> &observer)
{
    return 0;
}

ErrCode MockAbilityContext::ConnectAbility(const AAFwk::Want &want, const sptr<AbilityConnectCallback> &connectCallback)
{
    return 0;
}

ErrCode MockAbilityContext::ConnectAbilityWithAccount(const AAFwk::Want &want, int accountId,
    const sptr<AbilityConnectCallback> &connectCallback)
{
    return 0;
}

ErrCode MockAbilityContext::ConnectUIServiceExtensionAbility(const AAFwk::Want& want,
    const sptr<AbilityConnectCallback>& connectCallback)
{
    return 0;
}

void MockAbilityContext::DisconnectAbility(const AAFwk::Want &want,
    const sptr<AbilityConnectCallback> &connectCallback, int32_t accountId)
{
}

std::shared_ptr<AppExecFwk::AbilityInfo> MockAbilityContext::GetAbilityInfo() const
{
    return abilityInfo_;
}

void MockAbilityContext::MinimizeAbility(bool fromUser)
{
}

ErrCode MockAbilityContext::OnBackPressedCallBack(bool &needMoveToBackground)
{
    needMoveToBackground = false;
    return 0;
}

ErrCode MockAbilityContext::MoveAbilityToBackground()
{
    return 0;
}

ErrCode MockAbilityContext::MoveUIAbilityToBackground()
{
    return 0;
}

ErrCode MockAbilityContext::TerminateSelf()
{
    return 0;
}

ErrCode MockAbilityContext::CloseAbility()
{
    return 0;
}

std::unique_ptr<NativeReference>& MockAbilityContext::GetContentStorage()
{
    return contentStorage_;
}

void *MockAbilityContext::GetEtsContentStorage()
{
    return etsContentStorage_;
}

ErrCode MockAbilityContext::StartAbilityByCall(const AAFwk::Want& want,
    const std::shared_ptr<CallerCallBack> &callback, int32_t accountId)
{
    return 0;
}

ErrCode MockAbilityContext::ReleaseCall(const std::shared_ptr<CallerCallBack> &callback)
{
    return 0;
}

void MockAbilityContext::ClearFailedCallConnection(const std::shared_ptr<CallerCallBack> &callback)
{
}

std::shared_ptr<LocalCallContainer> MockAbilityContext::GetLocalCallContainer()
{
    return localCallContainer_;
}

void MockAbilityContext::SetConfiguration(const std::shared_ptr<AppExecFwk::Configuration> &config)
{
    config_ = config;
}

void MockAbilityContext::RegisterAbilityCallback(std::weak_ptr<AppExecFwk::IAbilityCallback> abilityCallback)
{
}

void MockAbilityContext::SetWeakSessionToken(const wptr<IRemoteObject>& sessionToken)
{
}

void MockAbilityContext::SetAbilityRecordId(int32_t abilityRecordId)
{
    abilityRecordId_ = abilityRecordId;
}

int32_t MockAbilityContext::GetAbilityRecordId()
{
    return abilityRecordId_;
}

ErrCode MockAbilityContext::RequestDialogService(void* env, AAFwk::Want &want, RequestDialogResultTask &&task)
{
    return 0;
}

ErrCode MockAbilityContext::RequestDialogService(AAFwk::Want &want, RequestDialogResultTask &&task)
{
    return 0;
}

ErrCode MockAbilityContext::ReportDrawnCompleted()
{
    return 0;
}

ErrCode MockAbilityContext::GetMissionId(int32_t &missionId)
{
    missionId = 0;
    return 0;
}

ErrCode MockAbilityContext::SetMissionContinueState(const AAFwk::ContinueState &state)
{
    return 0;
}

void MockAbilityContext::RegisterAbilityLifecycleObserver(
    const std::shared_ptr<AppExecFwk::ILifecycleObserver> &observer)
{
}

void MockAbilityContext::UnregisterAbilityLifecycleObserver(
    const std::shared_ptr<AppExecFwk::ILifecycleObserver> &observer)
{
}

void MockAbilityContext::SetRestoreEnabled(bool enabled)
{
}

bool MockAbilityContext::GetRestoreEnabled()
{
    return false;
}

void MockAbilityContext::RegisterAbilityConfigUpdateCallback(AbilityConfigUpdateCallback abilityConfigUpdateCallback)
{
}

std::shared_ptr<AppExecFwk::Configuration> MockAbilityContext::GetAbilityConfiguration() const
{
    return config_;
}

void MockAbilityContext::SetAbilityConfiguration(const AppExecFwk::Configuration &config)
{
}

void MockAbilityContext::SetAbilityColorMode(int32_t colorMode)
{
}

void MockAbilityContext::RegisterBindingObjectConfigUpdateCallback(BindingObjectConfigUpdateCallback callback)
{
}

void MockAbilityContext::NotifyBindingObjectConfigUpdate()
{
}

void MockAbilityContext::SetAbilityResourceManager(
    std::shared_ptr<Global::Resource::ResourceManager> abilityResourceMgr)
{
}

bool MockAbilityContext::GetHookOff()
{
    return hookOff_;
}

void MockAbilityContext::SetHookOff(bool hookOff)
{
    hookOff_ = hookOff;
}

ErrCode MockAbilityContext::RevokeDelegator()
{
    return 0;
}

ErrCode MockAbilityContext::SetOnNewWantSkipScenarios(int32_t scenarios)
{
    return 0;
}

ErrCode MockAbilityContext::RestartAppWithWindow(const AAFwk::Want &want)
{
    return 0;
}

std::shared_ptr<AAFwk::Want> MockAbilityContext::GetWant()
{
    return nullptr;
}

bool MockAbilityContext::IsTerminating()
{
    return isTerminating_.load();
}

bool MockAbilityContext::IsHook()
{
    return isHook_;
}

void MockAbilityContext::SetHook(bool isHook)
{
    isHook_ = isHook;
}

void MockAbilityContext::SetTerminating(bool state)
{
    isTerminating_.store(state);
}

void MockAbilityContext::InsertResultCallbackTask(int requestCode, RuntimeTask&& task)
{
}

void MockAbilityContext::RemoveResultCallbackTask(int requestCode)
{
}

ErrCode MockAbilityContext::AddCompletionHandler(const std::string &requestId,
    OnRequestResult onRequestSucc, OnRequestResult onRequestFail)
{
    return 0;
}

void MockAbilityContext::OnRequestSuccess(const std::string &requestId,
    const AppExecFwk::ElementName &element, const std::string &message)
{
}

void MockAbilityContext::OnRequestFailure(const std::string &requestId,
    const AppExecFwk::ElementName &element, const std::string &message, int32_t resultCode)
{
}

void MockAbilityContext::OnOpenLinkRequestSuccess(const std::string &requestId,
    const AppExecFwk::ElementName &element, const std::string &message)
{
}

void MockAbilityContext::OnOpenLinkRequestFailure(const std::string &requestId,
    const AppExecFwk::ElementName &element, const std::string &message)
{
}

ErrCode MockAbilityContext::AddCompletionHandlerForAtomicService(const std::string &requestId,
    OnAtomicRequestSuccess onRequestSucc, OnAtomicRequestFailure onRequestFail, const std::string &appId)
{
    return 0;
}

ErrCode MockAbilityContext::AddCompletionHandlerForOpenLink(const std::string &requestId,
    OnRequestResult onRequestSucc, OnRequestResult onRequestFail)
{
    return 0;
}

ErrCode MockAbilityContext::StartSelfUIAbilityInCurrentProcess(const AAFwk::Want &want,
    const std::string &specifiedFlag, const AAFwk::StartOptions &startOptions, bool hasOptions)
{
    return 0;
}

ErrCode MockAbilityContext::NotifyCancelGamePreLaunch()
{
    return 0;
}

ErrCode MockAbilityContext::NotifyCompleteGamePreLaunch()
{
    return 0;
}

}  // namespace OHOS::AbilityRuntime