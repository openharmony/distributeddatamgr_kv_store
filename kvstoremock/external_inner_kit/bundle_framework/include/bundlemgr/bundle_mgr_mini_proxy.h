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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_MGR_MINI_PROXY_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_MGR_MINI_PROXY_H

#include <string>

#include "bundle_framework_core_ipc_interface_code.h"
#include "bundle_mgr_interface.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace AppExecFwk {

class BundleMgrMiniProxy : public IRemoteProxy<IBundleMgr> {
public:
    explicit BundleMgrMiniProxy(const sptr<IRemoteObject> &impl);
    virtual ~BundleMgrMiniProxy() override;

    /**
     * @brief Obtains the formal name associated with the given UID.
     * @param uid Indicates the uid.
     * @param name Indicates the obtained formal name.
     * @return Returns ERR_OK if execute success; returns errCode otherwise.
     */
    virtual ErrCode GetNameForUid(const int uid, std::string &name) override;

    /**
     * @brief Obtains the application ID based on the given bundle name and user ID.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @return Returns the application ID if successfully obtained; returns empty string otherwise.
     */
    virtual std::string GetAppIdByBundleName(const std::string &bundleName, const int userId) override;

    /**
     * @brief Obtains the application UID based on the given bundle name and user ID through the proxy object.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @return Returns the uid if successfully obtained; returns -1 otherwise.
     */

    virtual int GetUidByBundleName(const std::string &bundleName, const int userId) override;

    /**
     * @brief Obtains the application UID based on the given bundle name and user ID through the proxy object.
     * @param bundleName Indicates the bundle name of the application.
     * @param userId Indicates the user ID.
     * @param userId Indicates the app Index.
     * @return Returns the uid if successfully obtained; returns -1 otherwise.
     */
    virtual int32_t GetUidByBundleName(const std::string &bundleName, const int32_t userId, int32_t appIndex) override;

private:
    static inline BrokerDelegator<BundleMgrMiniProxy> delegator_;

    /**
     * @brief Send a command message from the proxy object.
     * @param code Indicates the message code to be sent.
     * @param data Indicates the objects to be sent.
     * @param reply Indicates the reply to be sent;
     * @return Returns true if message send successfully; returns false otherwise.
     */
    bool SendTransactCmd(BundleMgrInterfaceCode code, MessageParcel &data, MessageParcel &reply);

    /**
     * @brief Send a command message from the proxy object and  printf log.
     * @param code Indicates the message code to be sent.
     * @param data Indicates the objects to be sent.
     * @param reply Indicates the reply to be sent;
     * @return Returns true if message send successfully; returns false otherwise.
     */
    bool SendTransactCmdWithLog(BundleMgrInterfaceCode code, MessageParcel &data, MessageParcel &reply);
};

}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_MGR_MINI_PROXY_H
