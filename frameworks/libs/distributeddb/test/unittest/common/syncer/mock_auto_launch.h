/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef MOCK_AUTO_LAUNCH_H
#define MOCK_AUTO_LAUNCH_H

#include <gmock/gmock.h>
#include "auto_launch.h"
#include "concurrent_adapter.h"
#include "res_finalizer.h"

namespace DistributedDB {
class MockAutoLaunch : public AutoLaunch {
public:
    void SetAutoLaunchItem(const std::string &identify, const std::string &userId, const AutoLaunchItem &item)
    {
        ConcurrentAdapter::AdapterAutoLock(extLock_);
        ResFinalizer finalizer([this]() { ConcurrentAdapter::AdapterAutoUnLock(extLock_); });
        extItemMap_[identify][userId] = item;
    }

    void CallExtConnectionLifeCycleCallbackTask(const std::string &identifier, const std::string &userId)
    {
        AutoLaunch::ExtConnectionLifeCycleCallbackTask(identifier, userId);
    }

    void SetWhiteListItem(const std::string &id, const std::string &userId, const AutoLaunchItem &item)
    {
        autoLaunchItemMap_[id][userId] = item;
    }

    void ClearWhiteList()
    {
        autoLaunchItemMap_.clear();
    }

    std::string CallGetAutoLaunchItemUid(const std::string &identifier, const std::string &originalUserId, bool &ext)
    {
        return AutoLaunch::GetAutoLaunchItemUid(identifier, originalUserId, ext);
    }

    MOCK_METHOD1(TryCloseConnection, void(AutoLaunchItem &));
};
} // namespace DistributedDB
#endif  // #define MOCK_AUTO_LAUNCH_H