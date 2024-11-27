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
#include "include/security_manager_mock.h"

namespace OHOS::DistributedKv {
SecurityManager::SecurityManager() { }

SecurityManager::~SecurityManager() { }

bool SecurityManager::SaveDBPassword(
    const std::string &name, const std::string &path, const DistributedDB::CipherPassword &key)
{
    if (BSecurityManager::securityManager == nullptr) {
        return false;
    }
    return BSecurityManager::securityManager->SaveDBPassword(name, path, key);
}

SecurityManager &SecurityManager::GetInstance()
{
    static SecurityManager instance;
    return instance;
}

SecurityManager::DBPassword SecurityManager::GetDBPassword(
    const std::string &name, const std::string &path, bool needCreate)
{
    if (BSecurityManager::securityManager == nullptr) {
        SecurityManager::DBPassword dbPassword;
        return dbPassword;
    }
    return BSecurityManager::securityManager->GetDBPassword(name, path, needCreate);
}
} // namespace OHOS::DistributedKv