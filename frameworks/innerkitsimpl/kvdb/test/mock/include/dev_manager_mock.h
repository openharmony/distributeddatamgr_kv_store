/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_DEV_MANAGER_MOCK_H
#define OHOS_DISTRIBUTED_DATA_DEV_MANAGER_MOCK_H

#include "dev_manager.h"
#include <gmock/gmock.h>

namespace OHOS::DistributedKv {
class BDevManager {
public:
    virtual std::string ToUUID(const std::string &) = 0;
    virtual const DevManager::DetailInfo &GetLocalDevice() = 0;
    virtual std::string ToNetworkId(const std::string &) = 0;
    virtual std::string GetUnEncryptedUuid() = 0;
    BDevManager() = default;
    virtual ~BDevManager() = default;

public:
    static inline std::shared_ptr<BDevManager> devManager = nullptr;
};

class DevManagerMock : public BDevManager {
public:
    MOCK_METHOD(std::string, ToUUID, (const std::string &));
    MOCK_METHOD(const DevManager::DetailInfo &, GetLocalDevice, ());
    MOCK_METHOD(std::string, ToNetworkId, (const std::string &));
    MOCK_METHOD(std::string, GetUnEncryptedUuid, ());
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_DEV_MANAGER_MOCK_H