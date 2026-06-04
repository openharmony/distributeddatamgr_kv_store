/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_EXTENSION_MANAGER_PROXY_H
#define OHOS_ABILITY_RUNTIME_EXTENSION_MANAGER_PROXY_H

#include "extension_manager_interface.h"
#include "iremote_proxy.h"

namespace OHOS {
class MessageParcel;
namespace AAFwk {
enum class AbilityManagerInterfaceCode;
/**
 * @class ExtensionManagerProxy
 * ExtensionManagerProxy.
 */
class ExtensionManagerProxy : public IRemoteProxy<IExtensionManager> {
public:
    explicit ExtensionManagerProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IExtensionManager>(impl) {}
    virtual ~ExtensionManagerProxy() = default;

    /**
     * ConnectAbility, connect session with service ability.
     *
     * @param want, Special want for service type's ability.
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @param callerToken, caller ability token.
     * @param userId, Designation User ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int ConnectAbilityCommon(const Want &want, sptr<IRemoteObject> connect,
        const sptr<IRemoteObject> &callerToken, AppExecFwk::ExtensionAbilityType extensionType,
        int32_t userId = DEFAULT_INVALID_USER_ID, bool isQueryExtensionOnly = true) override;

    /**
     * Disconnect session with extension ability.
     *
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int DisconnectAbility(const sptr<IRemoteObject> &connect) override;

    /**
     * @brief Get the extension running information.
     *
     * @param upperLimit The maximum limit of information wish to get.
     * @param info Extension running information.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int GetExtensionRunningInfos(int upperLimit, std::vector<ExtensionRunningInfo> &info) override;

    /**
     * Transfer resultCode & want to ability manager service.
     *
     * @param callerToken caller ability token.
     * @param requestCode the resultCode of the ability to start.
     * @param want Indicates the ability to start.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int32_t TransferAbilityResultForExtension(const sptr<IRemoteObject> &callerToken, int32_t resultCode,
        const Want &want) override;
private:
    bool WriteInterfaceToken(MessageParcel &data);
    ErrCode SendRequest(AbilityManagerInterfaceCode code, MessageParcel &data, MessageParcel &reply,
        MessageOption &option);
    template <typename T>
    int GetParcelableInfos(MessageParcel &reply, std::vector<T> &parcelableInfos);
private:
    static inline BrokerDelegator<ExtensionManagerProxy> delegator_;
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_EXTENSION_MANAGER_PROXY_H
