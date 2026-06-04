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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_USER_MGR_INTERFACE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_USER_MGR_INTERFACE_H

#include "appexecfwk_errors.h"
#include "iremote_broker.h"
#include <optional>

namespace OHOS {
namespace AppExecFwk {
class IBundleUserMgr : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.BundleUserMgr");

    /**
     * @brief Create new user.
     * @param userId Indicates the userId.
     * @param disallowList Pass in the provisioned disallowList.
     */
    virtual ErrCode CreateNewUser(int32_t userId, const std::vector<std::string> &disallowList = {},
        const std::optional<std::vector<std::string>> &allowList = std::nullopt)
    {
        return ERR_APPEXECFWK_SERVICE_INTERNAL_ERROR;
    }
    /**
     * @brief Remove user.
     * @param userId Indicates the userId.
     */
    virtual ErrCode RemoveUser(int32_t userId)
    {
        return ERR_APPEXECFWK_SERVICE_INTERNAL_ERROR;
    }
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLEMGR_BUNDLE_USER_MGR_INTERFACE_H
