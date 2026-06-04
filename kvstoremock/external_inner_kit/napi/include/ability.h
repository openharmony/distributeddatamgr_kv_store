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

#ifndef FOUNDATION_APPEXECFWK_OHOS_ABILITY_H
#define FOUNDATION_APPEXECFWK_OHOS_ABILITY_H

#include <memory>
#include <string>

namespace OHOS {
namespace AbilityRuntime {
class Context;
class AbilityContext;
class Runtime;
}  // namespace AbilityRuntime

namespace AppExecFwk {
class Ability {
public:
    static Ability* Create(const std::unique_ptr<AbilityRuntime::Runtime>& runtime);
    
    Ability() = default;
    virtual ~Ability() = default;
    
    inline std::shared_ptr<AbilityRuntime::AbilityContext> GetAbilityContext()
    {
        return abilityContext_;
    }
    
    virtual void OnStart();
    virtual void OnStop();
    virtual void OnActive();
    virtual void OnInactive();
    virtual void OnForeground();
    virtual void OnBackground();
    virtual void OnConfigurationUpdated();
    
protected:
    std::shared_ptr<AbilityRuntime::AbilityContext> abilityContext_;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif /* FOUNDATION_APPEXECFWK_OHOS_ABILITY_H */