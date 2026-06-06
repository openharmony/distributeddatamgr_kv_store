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

#ifndef OHOS_ABILITY_RUNTIME_EXTENSION_MANAGER_INTERFACE_H
#define OHOS_ABILITY_RUNTIME_EXTENSION_MANAGER_INTERFACE_H

#include <iremote_broker.h>
#include <vector>

namespace OHOS {
namespace AppExecFwk {
enum class ExtensionAbilityType;
}
namespace AAFwk {
struct ExtensionRunningInfo;
namespace {
constexpr int DEFAULT_INVALID_USER_ID = -1;
}
class Want;
/**
 * @class IExtensionManager
 * IExtensionManager interface is used to access ability manager services.
 */
class IExtensionManager : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.aafwk.ExtensionManager")

    /**
     * Connect ability common method.
     *
     * @param want, special want for service type's ability.
     * @param connect, callback used to notify caller the result of connecting or disconnecting.
     * @param callerToken, caller ability token.
     * @param extensionType, type of the extension.
     * @param userId, the service user ID.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int ConnectAbilityCommon(const Want &want, sptr<IRemoteObject> connect,
        const sptr<IRemoteObject> &callerToken, AppExecFwk::ExtensionAbilityType extensionType,
        int32_t userId = DEFAULT_INVALID_USER_ID, bool isQuerryExtensionOnly = true)
    {
        return 0;
    }

    /**
     * Disconnect session with extension ability.
     *
     * @param connect, Callback used to notify caller the result of connecting or disconnecting.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int DisconnectAbility(const sptr<IRemoteObject> &connect) = 0;

    /**
     * @brief Get the extension running information.
     *
     * @param upperLimit The maximum limit of information wish to get.
     * @param info Extension running information.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int GetExtensionRunningInfos(int upperLimit, std::vector<ExtensionRunningInfo> &info) = 0;

    /**
     * Transfer resultCode & want to ability manager service.
     *
     * @param resultCode, the resultCode of the ability to terminate.
     * @param resultWant, the Want of the ability to return.
     * @return Returns ERR_OK on success, others on failure.
     */
    virtual int32_t TransferAbilityResultForExtension(const sptr<IRemoteObject> &callerToken, int32_t resultCode,
        const Want &want)
    {
        return 0;
    }
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_EXTENSION_MANAGER_INTERFACE_H
