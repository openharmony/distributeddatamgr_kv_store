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

#ifndef OHOS_ABILITY_RUNTIME_EXTENSION_CONTEXT_H
#define OHOS_ABILITY_RUNTIME_EXTENSION_CONTEXT_H

#include "ability_info.h"
#include "context/context_impl.h"
#include "start_options.h"
#include "open_link_options.h"

namespace OHOS {
namespace AbilityRuntime {
/**
 * @brief context supply for extension
 *
 */
class ExtensionContext : public ContextImpl {
public:
    ExtensionContext() = default;
    virtual ~ExtensionContext() = default;

    /**
     * @brief Obtains information about the current ability.
     * The returned information includes the class name, bundle name, and other information about the current ability.
     *
     * @return Returns the AbilityInfo object for the current ability.
     */
    std::shared_ptr<OHOS::AppExecFwk::AbilityInfo> GetAbilityInfo() const;

    /**
     * @brief Set AbilityInfo when init.
     *
     */
    void SetAbilityInfo(const std::shared_ptr<OHOS::AppExecFwk::AbilityInfo> &abilityInfo);

        /**
     * @brief Add CompletioHandler.
     *
     * @param requestId, the requestId.
     * @param onRequestSucc, the callback ot be called upon request success.
     * @param onRequestFail, the callback ot be called upon request failure.
     * @return ERR_OK on success, otherwise failure.
     */
    virtual ErrCode AddCompletionHandlerForAtomicService(const std::string &requestId,
        OnAtomicRequestSuccess onRequestSucc, OnAtomicRequestFailure onRequestFail, const std::string &appId);

    virtual ErrCode AddCompletionHandlerForOpenLink(const std::string &requestId,
        OnRequestResult onRequestSucc, OnRequestResult onRequestFail);

    /**
     * @brief Callback on request success.
     *
     * @param requestId, the requestId.
     * @param element, the want element of startAbility.
     * @param message, the message returned to the callback.
     */
    virtual void OnRequestSuccess(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message);

    /**
     * @brief Callback on request failure.
     *
     * @param requestId, the requestId.
     * @param element, the want element of startAbility.
     * @param message, the message returned to the callback.
     */
    virtual void OnRequestFailure(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message, int32_t resultCode = 0);

    virtual void OnOpenLinkRequestSuccess(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message);
    virtual void OnOpenLinkRequestFailure(const std::string &requestId, const AppExecFwk::ElementName &element,
        const std::string &message);

    using SelfType = ExtensionContext;
    static const size_t CONTEXT_TYPE_ID;

protected:
    bool IsContext(size_t contextTypeId) override
    {
        return contextTypeId == CONTEXT_TYPE_ID || Context::IsContext(contextTypeId);
    }

private:
    std::shared_ptr<OHOS::AppExecFwk::AbilityInfo> abilityInfo_;
};
}  // namespace AbilityRuntime
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_EXTENSION_CONTEXT_H
