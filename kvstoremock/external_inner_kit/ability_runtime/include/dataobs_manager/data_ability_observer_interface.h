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
#ifndef OHOS_ABILITY_RUNTIME_DATA_ABILITY_OBSERVER_INTERFACE_H
#define OHOS_ABILITY_RUNTIME_DATA_ABILITY_OBSERVER_INTERFACE_H

#include <iremote_broker.h>
#include "dataobs_mgr_changeinfo.h"

namespace OHOS {
namespace AAFwk {
class IDataAbilityObserver : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.aafwk.DataAbilityObserver");

    enum {
        TRANS_HEAD,
        DATA_ABILITY_OBSERVER_CHANGE = TRANS_HEAD,
        DATA_ABILITY_OBSERVER_CHANGE_EXT,
        DATA_ABILITY_OBSERVER_CHANGE_PREFERENCES,
        TRANS_BUTT,
    };

    /**
     * @brief Called back to notify that the data being observed has changed.
     *
     * @param uri Indicates the path of the data to operate.
     */
    virtual void OnChange() = 0;

    /**
     * @brief Called back to notify that the data being observed has changed.
     *
     * @param changeInfo Indicates the info of the data to operate.
     */
    virtual void OnChangeExt(const ChangeInfo &changeInfo)
    {
        return;
    }

    /**
     * @brief Called back to notify that the data being observed has changed.
     *
     * @param key Indicates the key that has changed.
     */
    virtual void OnChangePreferences(const std::string &key)
    {
        return;
    }
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_DATA_ABILITY_OBSERVER_INTERFACE_H
