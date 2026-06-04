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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_GET_LARGEST_ITEMS_CALLBACK_INTERFACE_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_GET_LARGEST_ITEMS_CALLBACK_INTERFACE_H

#include "iremote_broker.h"

namespace OHOS {
namespace AppExecFwk {
class IGetLargestItemsCallback : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.appexecfwk.GetLargestItemsCallback");

    /**
     * @brief Called when get largest items progress finished.
     * @param errCode Indicates the result of the get largest items progress.
     * @param largestItems Indicates the JSON string containing largest items with path and size.
     */
    virtual void OnGetLargestItemsFinished(ErrCode errCode, const std::string &largestItems) = 0;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_GET_LARGEST_ITEMS_CALLBACK_INTERFACE_H
