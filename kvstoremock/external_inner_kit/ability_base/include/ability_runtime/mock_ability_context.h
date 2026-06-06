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

#ifndef OHOS_ABILITY_RUNTIME_MOCK_ABILITY_CONTEXT_H
#define OHOS_ABILITY_RUNTIME_MOCK_ABILITY_CONTEXT_H

#include "ability/native/ability_context.h"

namespace OHOS {
namespace AbilityRuntime {

class MockAbilityContext : public AbilityContext {
public:
    MockAbilityContext() = default;
    virtual ~MockAbilityContext() = default;

    ErrCode StartAbility(const AAFwk::Want &want, int requestCode) override;
    ErrCode StartAbilityWithAccount(const AAFwk::Want &want, int accountId, int requestCode) override;
    ErrCode StartAbility(const AAFwk::Want &want, const AAFwk::StartOptions &startOptions,
        int requestCode) override;
    ErrCode StartAbilityAsCaller(const AAFwk::Want &want, int requestCode) override;
    ErrCode StartAbilityAsCaller(const AAFwk::Want &want, const AAFwk::StartOptions &startOptions,
        int requestCode) override;
    ErrCode StartAbilityWithAccount(const AAFwk::Want &want, int accountId,
        const AAFwk::StartOptions &startOptions, int requestCode) override;
    ErrCode StartAbilityForResult(const AAFwk::Want &want, int requestCode, RuntimeTask &&task) override;
    ErrCode StartAbilityForResultWithAccount(const AAFwk::Want &want, int accountId, int requestCode,
        RuntimeTask &&task) override;
    ErrCode StartAbilityForResult(const AAFwk::Want &want, const AAFwk::StartOptions &startOptions,
        int requestCode, RuntimeTask &&task) override;
    ErrCode StartAbilityForResultWithAccount(const AAFwk::Want &want, int accountId,
        const AAFwk::StartOptions &startOptions, int requestCode, RuntimeTask &&task) override;
    ErrCode StartServiceExtensionAbility(const AAFwk::Want &want, int32_t accountId) override;
    ErrCode StartUIServiceExtensionAbility(const AAFwk::Want &want, int32_t accountId) override;
    ErrCode StopServiceExtensionAbility(const AAFwk::Want& want, int32_t accountId) override;
    ErrCode TerminateAbilityWithResult(const AAFwk::Want &want, int resultCode) override;
    ErrCode BackToCallerAbilityWithResult(const AAFwk::Want &want, int resultCode, int64_t requestCode) override;
    ErrCode RestoreWindowStage(void* env, void* contentStorage) override;
    ErrCode RestoreWindowStage(void *contentStorage) override;
    void OnAbilityResult(int requestCode, int resultCode, const AAFwk::Want &resultData) override;
    ErrCode RequestModalUIExtension(const AAFwk::Want& want) override;
    ErrCode OpenLink(const AAFwk::Want &want, int requestCode, bool hideFailureTipDialog) override;
    ErrCode OpenAtomicService(AAFwk::Want& want, const AAFwk::StartOptions &options, int requestCode,
        RuntimeTask &&task) override;
    ErrCode AddFreeInstallObserver(const sptr<AbilityRuntime::IFreeInstallObserver> &observer) override;
    ErrCode ConnectAbility(const AAFwk::Want &want,
        const sptr<AbilityConnectCallback> &connectCallback) override;
    ErrCode ConnectAbilityWithAccount(const AAFwk::Want &want, int accountId,
        const sptr<AbilityConnectCallback> &connectCallback) override;
    ErrCode ConnectUIServiceExtensionAbility(const AAFwk::Want& want,
        const sptr<AbilityConnectCallback>& connectCallback) override;
    void DisconnectAbility(const AAFwk::Want &want,
        const sptr<AbilityConnectCallback> &connectCallback, int32_t accountId) override;
    std::shared_ptr<AppExecFwk::AbilityInfo> GetAbilityInfo() const override;
    void MinimizeAbility(bool fromUser) override;
    ErrCode OnBackPressedCallBack(bool &needMoveToBackground) override;
    ErrCode MoveAbilityToBackground() override;
    ErrCode MoveUIAbilityToBackground() override;
    ErrCode TerminateSelf() override;
    ErrCode CloseAbility() override;
    std::unique_ptr<NativeReference>& GetContentStorage() override;
    void *GetEtsContentStorage() override;
    ErrCode StartAbilityByCall(const AAFwk::Want& want,
        const std::shared_ptr<CallerCallBack> &callback, int32_t accountId) override;
    ErrCode ReleaseCall(const std::shared_ptr<CallerCallBack> &callback) override;
    void ClearFailedCallConnection(const std::shared_ptr<CallerCallBack> &callback) override;
    std::shared_ptr<LocalCallContainer> GetLocalCallContainer() override;
    void SetConfiguration(const std::shared_ptr<AppExecFwk::Configuration> &config) override;
    void RegisterAbilityCallback(std::weak_ptr<AppExecFwk::IAbilityCallback> abilityCallback) override;
    void SetWeakSessionToken(const wptr<IRemoteObject>& sessionToken) override;
    void SetAbilityRecordId(int32_t abilityRecordId) override;
    int32_t GetAbilityRecordId() override;
    ErrCode RequestDialogService(void* env, AAFwk::Want &want, RequestDialogResultTask &&task) override;
    ErrCode RequestDialogService(AAFwk::Want &want, RequestDialogResultTask &&task) override;
    ErrCode ReportDrawnCompleted() override;
    ErrCode GetMissionId(int32_t &missionId) override;
    ErrCode SetMissionContinueState(const AAFwk::ContinueState &state) override;
    void RegisterAbilityLifecycleObserver(const std::shared_ptr<AppExecFwk::ILifecycleObserver> &observer) override;
    void UnregisterAbilityLifecycleObserver(const std::shared_ptr<AppExecFwk::ILifecycleObserver> &observer) override;
    void SetRestoreEnabled(bool enabled) override;
    bool GetRestoreEnabled() override;
    void RegisterAbilityConfigUpdateCallback(AbilityConfigUpdateCallback abilityConfigUpdateCallback) override;
    std::shared_ptr<AppExecFwk::Configuration> GetAbilityConfiguration() const override;
    void SetAbilityConfiguration(const AppExecFwk::Configuration &config) override;
    void SetAbilityColorMode(int32_t colorMode) override;
    void RegisterBindingObjectConfigUpdateCallback(BindingObjectConfigUpdateCallback callback) override;
    void NotifyBindingObjectConfigUpdate() override;
    void SetAbilityResourceManager(std::shared_ptr<Global::Resource::ResourceManager> abilityResourceMgr) override;
    bool GetHookOff() override;
    void SetHookOff(bool hookOff) override;
    ErrCode RevokeDelegator() override;
    ErrCode SetOnNewWantSkipScenarios(int32_t scenarios) override;
    ErrCode RestartAppWithWindow(const AAFwk::Want &want) override;
    std::shared_ptr<AAFwk::Want> GetWant() override;
    bool IsTerminating() override;
    bool IsHook() override;
    void SetHook(bool isHook) override;
    void SetTerminating(bool state) override;
    void InsertResultCallbackTask(int requestCode, RuntimeTask&& task) override;
    void RemoveResultCallbackTask(int requestCode) override;
    ErrCode AddCompletionHandler(const std::string &requestId, OnRequestResult onRequestSucc,
        OnRequestResult onRequestFail) override;
    void OnRequestSuccess(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message) override;
    void OnRequestFailure(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message, int32_t resultCode) override;
    void OnOpenLinkRequestSuccess(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message) override;
    void OnOpenLinkRequestFailure(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message) override;
    ErrCode AddCompletionHandlerForAtomicService(const std::string &requestId,
        OnAtomicRequestSuccess onRequestSucc, OnAtomicRequestFailure onRequestFail,
        const std::string &appId) override;
    ErrCode AddCompletionHandlerForOpenLink(const std::string &requestId, OnRequestResult onRequestSucc,
        OnRequestResult onRequestFail) override;
    ErrCode StartSelfUIAbilityInCurrentProcess(const AAFwk::Want &want, const std::string &specifiedFlag,
        const AAFwk::StartOptions &startOptions, bool hasOptions) override;
    ErrCode NotifyCancelGamePreLaunch() override;
    ErrCode NotifyCompleteGamePreLaunch() override;

private:
    std::unique_ptr<NativeReference> contentStorage_;
    void *etsContentStorage_ = nullptr;
    sptr<IRemoteObject> token_;
    std::shared_ptr<AppExecFwk::AbilityInfo> abilityInfo_;
    std::shared_ptr<AppExecFwk::Configuration> config_;
    std::shared_ptr<LocalCallContainer> localCallContainer_;
    int32_t abilityRecordId_ = 0;
    std::atomic<bool> isTerminating_ {false};
    bool isHook_ = false;
    bool hookOff_ = false;
};

}  // namespace AbilityRuntime
}  // namespace OHOS

#endif  // OHOS_ABILITY_RUNTIME_MOCK_ABILITY_CONTEXT_H