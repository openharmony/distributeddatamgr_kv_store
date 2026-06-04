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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_EVENT_CALLBACK_PROXY_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_EVENT_CALLBACK_PROXY_H

#include "bundle_event_callback_interface.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace AppExecFwk {
class BundleEventCallbackProxy : public IRemoteProxy<IBundleEventCallback> {
public:
    explicit BundleEventCallbackProxy(const sptr<IRemoteObject> &object);
    virtual ~BundleEventCallbackProxy() override;

    virtual void OnReceiveEvent(const EventFwk::CommonEventData eventData) override;

private:
    static inline BrokerDelegator<BundleEventCallbackProxy> delegator_;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_CORE_INCLUDE_BUNDLE_EVENT_CALLBACK_PROXY_H
