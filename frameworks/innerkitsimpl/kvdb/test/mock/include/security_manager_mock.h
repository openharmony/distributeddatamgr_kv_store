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

#ifndef OHOS_DISTRIBUTED_DATA_SECURITY_MANAGER_MOCK_H
#define OHOS_DISTRIBUTED_DATA_SECURITY_MANAGER_MOCK_H

#include "security_manager.h"
#include <gmock/gmock.h>

namespace OHOS::DistributedKv {
class BSecurityManager {
public:
    virtual bool SaveDBPassword(const std::string &, const std::string &, const DistributedDB::CipherPassword &) = 0;
    virtual SecurityManager::DBPassword GetDBPassword(const std::string &, const std::string &, bool) = 0;
    BSecurityManager() = default;
    virtual ~BSecurityManager() = default;

public:
    static inline std::shared_ptr<BSecurityManager> securityManager = nullptr;
};

class SecurityManagerMock : public BSecurityManager {
public:
    MOCK_METHOD(
        bool, SaveDBPassword, (const std::string &, const std::string &, const DistributedDB::CipherPassword &));
    MOCK_METHOD(SecurityManager::DBPassword, GetDBPassword, (const std::string &, const std::string &, bool));
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_STORE_UTIL_MOCK_H