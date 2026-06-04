/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_RUNTIME_FREE_INSTALL_OBSERVER_INTERFACE_H
#define OHOS_ABILITY_RUNTIME_FREE_INSTALL_OBSERVER_INTERFACE_H

#include <string>
#include "iremote_broker.h"

namespace OHOS {
namespace AbilityRuntime {

class IFreeInstallObserver : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.aafwk.IFreeInstallObserver");

    virtual void OnInstallFinished(const std::string &bundleName, const std::string &abilityName,
        const std::string &startTime, int32_t resultCode) = 0;

    virtual void OnInstallFinishedByUrl(const std::string &startTime, const std::string &url,
        int32_t resultCode) = 0;

    enum {
        ON_INSTALL_FINISHED = 1,
        ON_INSTALL_FINISHED_BY_URL = 2,
    };
};

}
}

#endif