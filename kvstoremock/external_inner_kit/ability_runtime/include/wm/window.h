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

#ifndef OHOS_ROSEN_WINDOW_H
#define OHOS_ROSEN_WINDOW_H

#include <refbase.h>

namespace OHOS {
namespace Rosen {

class IWindowLifeCycle : virtual public RefBase {
public:
    virtual void AfterForeground() {}
    virtual void AfterBackground() {}
    virtual void AfterFocused() {}
    virtual void AfterUnfocused() {}
    virtual void ForegroundFailed(int32_t ret) {}
    virtual void BackgroundFailed(int32_t ret) {}
    virtual void AfterActive() {}
    virtual void AfterInactive() {}
    virtual void AfterResumed() {}
    virtual void AfterPaused() {}
    virtual void AfterDestroyed() {}
    virtual void AfterDidForeground() {}
    virtual void AfterDidBackground() {}

    void SetIsWindowSceneListener(bool isWindowSceneListener)
    {
        isWindowSceneListener_ = isWindowSceneListener;
    }

    bool IsWindowSceneListener()
    {
        return isWindowSceneListener_;
    }
protected:
    bool isWindowSceneListener_ = false;
};

} // namespace Rosen
} // namespace OHOS
#endif // OHOS_ROSEN_WINDOW_H