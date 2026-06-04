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

#ifndef OHOS_ABILITY_RUNTIME_EXTENSION_H
#define OHOS_ABILITY_RUNTIME_EXTENSION_H

#include <string>

#include "ability_transaction_callback_info.h"
#include "napi_remote_object.h"
#include "iremote_object.h"
#include "session_info.h"
#include "wm/window.h"
#include "launch_param.h"
#include "ui_extension_window_command.h"
#include "want.h"

namespace OHOS {
namespace AppExecFwk {
struct AbilityInfo;
class OHOSApplication;
class AbilityHandler;
class AbilityLocalRecord;
class Configuration;
struct InsightIntentExecuteResult;
}
namespace AbilityRuntime {
using Want = OHOS::AAFwk::Want;
/**
 * @brief The Extension component to schedule task with no pages.
 */
class Extension : public std::enable_shared_from_this<Extension> {
public:
    Extension() = default;
    virtual ~Extension() = default;

    /**
     * @brief Get the ability handler for this extension.
     * Override to provide a custom handler (e.g., shared worker thread).
     * Default returns nullptr, meaning the caller should create handler by itself.
     *
     * @param abilityInfo The ability info for handler creation.
     * @return The shared ability handler, or nullptr to use default behavior.
     */
    virtual std::shared_ptr<AppExecFwk::AbilityHandler> GetAbilityHandler(
        const std::shared_ptr<AppExecFwk::AbilityInfo> &abilityInfo);

    /**
     * @brief Init the extension.
     *
     * @param record the extension record.
     * @param application the application info.
     * @param handler the extension handler.
     * @param token the remote token.
     */
    virtual void Init(const std::shared_ptr<AppExecFwk::AbilityLocalRecord> &record,
        const std::shared_ptr<AppExecFwk::OHOSApplication> &application,
        std::shared_ptr<AppExecFwk::AbilityHandler> &handler,
        const sptr<IRemoteObject> &token);

    /**
     * @brief Sets the first want object.
     *
     * @param Want information of other ability or extension.
     */
    void SetLaunchWant(const AAFwk::Want &want);

    /**
     * @brief Obtains the first want object.
     *
     * @return Returns the first want object.
     */
    std::shared_ptr<AAFwk::Want> GetLaunchWant();

    /**
     * @brief Sets the last want object.
     *
     * @param Want information of other ability or extension.
     */
    void SetLastRequestWant(const AAFwk::Want &want);

    /**
     * @brief Sets the callingInfo from IPC.
     *
     * @param CallingInfo information of caller.
     */
    void SetCallingInfo(const CallingInfo &callingInfo);

    /**
     * @return std::shared_ptr<CallingInfo> the pointer of callingInfo.
     */
    std::shared_ptr<CallingInfo> GetCallingInfo();

    /**
     * @brief Called when this extension is started. You must override this function if you want to perform some
     *        initialization operations during extension startup.
     *
     * This function can be called only once in the entire lifecycle of an extension.
     * @param Want Indicates the {@link Want} structure containing startup information about the extension.
     */
    virtual void OnStart(const AAFwk::Want &want);
    virtual void OnStart(const AAFwk::Want &want, sptr<AAFwk::SessionInfo> sessionInfo);

    /**
     * @brief Called when this Service extension is connected for the first time.
     *
     * You can override this function to implement your own processing logic.
     *
     * @param want Indicates the {@link Want} structure containing connection information about the Service extension.
     * @return Returns a pointer to the <b>sid</b> of the connected Service extension.
     */
    virtual sptr<IRemoteObject> OnConnect(const AAFwk::Want &want);

    /**
     * @brief Called when this Service extension is connected for the first time.
     *
     * You can override this function to implement your own processing logic.
     *
     * @param want Indicates the {@link Want} structure containing connection information about the Service extension.
     * @param callbackInfo Indicates the lifecycle transaction callback information
     * @param isAsyncCallback Indicates whether it is an asynchronous lifecycle callback
     * @return Returns a pointer to the <b>sid</b> of the connected Service extension.
     */
    virtual sptr<IRemoteObject> OnConnect(const AAFwk::Want &want,
        AppExecFwk::AbilityTransactionCallbackInfo<sptr<IRemoteObject>> *callbackInfo, bool &isAsyncCallback);

    /**
     * @brief Called when all abilities connected to this Service extension are disconnected.
     *
     * You can override this function to implement your own processing logic.
     *
     */
    virtual void OnDisconnect(const AAFwk::Want &want);

    /**
     * @brief Called when all abilities connected to this Service extension are disconnected.
     *
     * You can override this function to implement your own processing logic.
     * @param callbackInfo Indicates the lifecycle transaction callback information
     * @param isAsyncCallback Indicates whether it is an asynchronous lifecycle callback
     */
    virtual void OnDisconnect(const AAFwk::Want &want, AppExecFwk::AbilityTransactionCallbackInfo<> *callbackInfo,
        bool &isAsyncCallback);

    /**
     * @brief Called back when Service is started.
     * This method can be called only by Service. You can use the StartAbility(ohos.aafwk.content.Want) method to start
     * Service. Then the system calls back the current method to use the transferred want parameter to execute its own
     * logic.
     *
     * @param want Indicates the want of Service to start.
     * @param restart Indicates the startup mode. The value true indicates that Service is restarted after being
     * destroyed, and the value false indicates a normal startup.
     * @param startId Indicates the number of times the Service extension has been started. The startId is
     * incremented by 1 every time the extension is started. For example, if the extension has been started
     * for six times, the value of startId is 6.
     */
    virtual void OnCommand(const AAFwk::Want &want, bool restart, int startId);

    virtual void OnCommandWindow(const AAFwk::Want &want, const sptr<AAFwk::SessionInfo> &sessionInfo,
        AAFwk::WindowCommand winCmd);
    /**
     * @brief Called when this extension enters the <b>STATE_STOP</b> state.
     *
     * The extension in the <b>STATE_STOP</b> is being destroyed.
     * You can override this function to implement your own processing logic.
     */
    virtual void OnStop();
    /**
     * @brief Called when this extension enters the <b>STATE_STOP</b> state.
     *
     * The ability in the <b>STATE_STOP</b> is being destroyed.
     * You can override this function to implement your own processing logic.
     *
     * @param callbackInfo Indicates the lifecycle transaction callback information
     * @param isAsyncCallback Indicates whether it is an asynchronous lifecycle callback
     */
    virtual void OnStop(AppExecFwk::AbilityTransactionCallbackInfo<> *callbackInfo, bool &isAsyncCallback);

    /**
     * @brief Called when the system configuration is updated.
     *
     * @param configuration Indicates the updated configuration information.
     */
    
    /**
     * @brief The callback of OnStop.
     */
    virtual void OnStopCallBack();
    
    virtual void OnConfigurationUpdated(const AppExecFwk::Configuration& configuration);

    /**
     * @brief Notify current memory level.
     *
     * @param level Current memory level.
     */
    virtual void OnMemoryLevel(int level);

    /**
     * @brief Called when this extension enters the <b>STATE_FOREGROUND</b> state.
     *
     *
     * The extension in the <b>STATE_FOREGROUND</b> state is visible.
     * You can override this function to implement your own processing logic.
     */
    virtual void OnForeground(const Want &want, sptr<AAFwk::SessionInfo> sessionInfo);

    /**
     * @brief Called when this extension enters the <b>STATE_BACKGROUND</b> state.
     *
     *
     * The extension in the <b>STATE_BACKGROUND</b> state is invisible.
     * You can override this function to implement your own processing logic.
     */
    virtual void OnBackground();

    /**
     * @brief Called when extension need dump info.
     *
     * @param params The params from service.
     * @param info The dump info to show.
     */
    virtual void Dump(const std::vector<std::string> &params, std::vector<std::string> &info);

    void SetExtensionWindowLifeCycleListener(const sptr<Rosen::IWindowLifeCycle> &listener);

    /**
     * @brief Set the launch param.
     *
     * @param launchParam The launch param.
     */
    void SetLaunchParam(const AAFwk::LaunchParam &launchParam);

    /**
     * @brief Get the launch param.
     *
     * @return Launch param information.
     */
    const AAFwk::LaunchParam &GetLaunchParam() const;

    /**
     * @brief Called when startAbilityForResult(ohos.aafwk.content.Want,int) is called to start an extension ability
     * and the result is returned.
     * @param requestCode Indicates the request code returned after the ability is started. You can define the request
     * code to identify the results returned by abilities. The value ranges from 0 to 65535.
     * @param resultCode Indicates the result code returned after the ability is started. You can define the result
     * code to identify an error.
     * @param want Indicates the data returned after the ability is started. You can define the data returned. The
     * value can be null.
     */
    virtual void OnAbilityResult(int requestCode, int resultCode, const Want &want);

    virtual void OnCommandWindowDone(const sptr<AAFwk::SessionInfo> &sessionInfo, AAFwk::WindowCommand winCmd);

    virtual void OnInsightIntentExecuteDone(const sptr<AAFwk::SessionInfo> &sessionInfo,
        const AppExecFwk::InsightIntentExecuteResult &result);

    virtual bool HandleInsightIntent(const AAFwk::Want &want);

    virtual bool OnInsightIntentExecuteDone(uint64_t intentId, const AppExecFwk::InsightIntentExecuteResult &result);

    virtual void OnExtensionAbilityRequestFailure(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message, int32_t resultCode = 0);

    virtual void OnExtensionAbilityRequestSuccess(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message);

    virtual bool HandleExecuteSkill(const AAFwk::Want &want);

    std::shared_ptr<AppExecFwk::AbilityInfo> abilityInfo_ = nullptr;
protected:
    std::shared_ptr<AppExecFwk::AbilityHandler> handler_ = nullptr;

    //  window scene
    sptr<Rosen::IWindowLifeCycle> extensionWindowLifeCycleListener_ = nullptr;
private:
    std::shared_ptr<AppExecFwk::OHOSApplication> application_ = nullptr;
    std::shared_ptr<AAFwk::Want> launchWant_ = nullptr;
    std::shared_ptr<AAFwk::Want> lastRequestWant_ = nullptr;
    AAFwk::LaunchParam launchParam_;
    std::shared_ptr<CallingInfo> callingInfo_ = nullptr;
};
}  // namespace AbilityRuntime
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_EXTENSION_H
