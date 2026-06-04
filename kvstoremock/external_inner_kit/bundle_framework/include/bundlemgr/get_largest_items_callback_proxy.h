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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_GET_LARGEST_ITEMS_CALLBACK_PROXY_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_GET_LARGEST_ITEMS_CALLBACK_PROXY_H

#include "iremote_proxy.h"

#include "get_largest_items_callback_interface.h"

namespace OHOS {
namespace AppExecFwk {
class GetLargestItemsCallbackProxy : public IRemoteProxy<IGetLargestItemsCallback> {
public:
    explicit GetLargestItemsCallbackProxy(const sptr<IRemoteObject> &object);
    virtual ~GetLargestItemsCallbackProxy() override;
    /**
     * @brief Called when get largest items progress finished through the proxy object.
     * @param errCode Indicates the result of the get largest items progress.
     * @param largestItems Indicates the JSON string containing largest items with path and size.
     */
    virtual void OnGetLargestItemsFinished(ErrCode errCode, const std::string &largestItems) override;

private:
    static inline BrokerDelegator<GetLargestItemsCallbackProxy> delegator_;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_GET_LARGEST_ITEMS_CALLBACK_PROXY_H
