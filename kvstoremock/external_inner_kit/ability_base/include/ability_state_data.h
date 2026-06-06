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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_STATE_DATA_H
#define OHOS_ABILITY_RUNTIME_ABILITY_STATE_DATA_H

#include <string>
#include <sys/types.h>
#include "parcel.h"
#include "iremote_object.h"

namespace OHOS {
namespace AppExecFwk {

struct AbilityStateData : public Parcelable {
    bool isFocused = false;
    bool isAtomicService = false;
    int32_t uid = 0;
    int32_t callerUid = -1;
    int32_t callerPid = 0;
    int32_t abilityRecordId = 0;
    int32_t appCloneIndex = -1;
    int32_t extensionAbilityType = -1;
    int32_t processType = 0;
    int32_t abilityType = 0;
    int32_t abilityState = 0;
    pid_t pid = 0;
    sptr<IRemoteObject> token;
    std::string moduleName;
    std::string bundleName;
    std::string abilityName;
    std::string callerBundleName;
    std::string callerAbilityName;
    bool isInnerNotify = false;
    int32_t preloadMode = -1;
    
    bool ReadFromParcel(Parcel &parcel) { return true; }
    bool Marshalling(Parcel &parcel) const override { return true; }
    bool MarshallingOne(Parcel &parcel) const { return true; }
    static AbilityStateData* Unmarshalling(Parcel &parcel) { return new AbilityStateData(); }
};

}
}

#endif