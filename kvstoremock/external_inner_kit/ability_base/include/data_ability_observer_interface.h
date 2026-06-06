/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

namespace OHOS {
namespace AAFwk {
struct ChangeInfo {
    enum ChangeType : uint32_t {
        INSERT = 0,
        DELETE,
        UPDATE,
        OTHER,
        INVAILD,
    };
    ChangeType changeType_ = INVAILD;
};

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

    virtual void OnChange() = 0;

    virtual void OnChangeExt(const ChangeInfo &changeInfo)
    {
        return;
    }

    virtual void OnChangePreferences(const std::string &key)
    {
        return;
    }
};
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_RUNTIME_DATA_ABILITY_OBSERVER_INTERFACE_H