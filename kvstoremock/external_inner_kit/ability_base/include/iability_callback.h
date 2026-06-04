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

#ifndef OHOS_ABILITY_RUNTIME_IABILITY_CALLBACK_H
#define OHOS_ABILITY_RUNTIME_IABILITY_CALLBACK_H

#include <string>
#include <memory>

namespace OHOS {
namespace AppExecFwk {
class IAbilityCallback {
public:
    IAbilityCallback() = default;
    virtual ~IAbilityCallback() = default;

    virtual int GetCurrentWindowMode() = 0;
    virtual ErrCode SetMissionLabel(const std::string &label) = 0;
    virtual ErrCode SetMissionIcon(const std::shared_ptr<void> &icon) = 0;
    virtual bool OnBackPress() { return false; }
    virtual void GetWindowRect(int32_t &left, int32_t &top, int32_t &width, int32_t &height) = 0;
    virtual void* GetUIContent() = 0;
    virtual void EraseUIExtension(int32_t sessionId) = 0;
    virtual void RegisterAbilityLifecycleObserver(const std::shared_ptr<void> &observer) = 0;
    virtual void UnregisterAbilityLifecycleObserver(const std::shared_ptr<void> &observer) = 0;
    virtual std::shared_ptr<void> GetWant() = 0;
    virtual void SetContinueState(int32_t state) {}
    virtual void NotifyWindowDestroy() {}
};
}
}

#endif