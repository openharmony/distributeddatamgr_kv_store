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

#ifndef OHOS_ABILITY_RUNTIME_ABILITY_LOCAL_RECORD_H
#define OHOS_ABILITY_RUNTIME_ABILITY_LOCAL_RECORD_H

#include <string>

#include "iremote_object.h"
#include "ability_info.h"
#include "refbase.h"
#include "want.h"

namespace OHOS {
namespace AppExecFwk {
class AbilityThread;
class AbilityLocalRecord {
public:
    /**
     *
     * default constructor
     *
     */
    AbilityLocalRecord(const std::shared_ptr<AbilityInfo> &info, const sptr<IRemoteObject> &token,
        const std::shared_ptr<AAFwk::Want> &want, int32_t abilityRecordId);

    /**
     *
     * @default Destructor
     *
     */
    virtual ~AbilityLocalRecord();

    /**
     * @description: Get an AbilityInfo in an ability.
     *
     * @return Returns a pointer to abilityinfo.
     */
    const std::shared_ptr<AbilityInfo> &GetAbilityInfo();

    /**
     * @description: Gets the identity of the ability
     * @return return the identity of the ability.
     */
    const sptr<IRemoteObject> &GetToken();

    int32_t GetAbilityRecordId() const;

    /**
     * @description: Obtains the information based on ability thread.
     * @return return AbilityThread Pointer
     */
    const sptr<AbilityThread> &GetAbilityThread();

    /**
     * @description: Set an AbilityThread in an ability.
     * @param abilityThread AbilityThread object
     * @return None.
     */
    void SetAbilityThread(const sptr<AbilityThread> &abilityThread);

    void SetWant(const std::shared_ptr<AAFwk::Want> &want);

    const std::shared_ptr<AAFwk::Want> &GetWant();
    
    bool IsHook() const;

    void SetSkipAbilityStageLifecycle(bool skipAbilityStageLifecycle);

    bool IsSkipAbilityStageLifecycle() const;
private:
    std::shared_ptr<AbilityInfo> abilityInfo_ = nullptr;
    sptr<IRemoteObject> token_ = nullptr;
    std::shared_ptr<AAFwk::Want> want_ = nullptr;
    int32_t abilityRecordId_ = 0;
    sptr<AbilityThread> abilityThread_;
    bool skipAbilityStageLifecycle_ = false;
};
} // namespace AppExecFwk
} // namespace OHOS
#endif // OHOS_ABILITY_RUNTIME_ABILITY_LOCAL_RECORD_H
