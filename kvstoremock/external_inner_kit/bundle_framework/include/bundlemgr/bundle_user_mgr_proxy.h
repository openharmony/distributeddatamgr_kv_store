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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_USER_MGR_PROXY_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_USER_MGR_PROXY_H

#include "iremote_proxy.h"

#include "bundle_framework_core_ipc_interface_code.h"
#include "bundle_user_mgr_interface.h"

namespace OHOS {
namespace AppExecFwk {
class BundleUserMgrProxy : public IRemoteProxy<IBundleUserMgr> {
public:
    explicit BundleUserMgrProxy(const sptr<IRemoteObject> &object);
    virtual ~BundleUserMgrProxy() override;

    /**
     * @brief Create new user.
     * @param userId Indicates the userId.
     * @param disallowList Pass in the provisioned disallowList.
     */
    ErrCode CreateNewUser(int32_t userId, const std::vector<std::string> &disallowList = {},
        const std::optional<std::vector<std::string>> &allowList = std::nullopt) override;
    /**
     * @brief Remove user.
     * @param userId Indicates the userId.
     */
    ErrCode RemoveUser(int32_t userId) override;

private:
    bool SendTransactCmd(BundleUserMgrInterfaceCode code, MessageParcel &data, MessageParcel &reply);
    ErrCode WriteStrListToData(MessageParcel &data, const std::vector<std::string> &list, size_t maxListSize);
    static inline BrokerDelegator<BundleUserMgrProxy> delegator_;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_USER_MGR_PROXY_H
