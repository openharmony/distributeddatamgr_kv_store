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

#include "include/dev_manager_mock.h"

namespace OHOS::DistributedKv {

constexpr const char *PKG_NAME_EX = "_distributed_data";

DevManager::DevManager(const std::string &pkgName) : PKG_NAME(pkgName + PKG_NAME_EX) { }

DevManager &DevManager::GetInstance()
{
    static DevManager instance(std::to_string(getpid()));
    return instance;
}

const DevManager::DetailInfo &DevManager::GetLocalDevice()
{
    if (BDevManager::devManager == nullptr) {
        return invalidDetail_;
    }
    return BDevManager::devManager->GetLocalDevice();
}

std::string DevManager::ToUUID(const std::string &networkId)
{
    if (BDevManager::devManager == nullptr) {
        return "";
    }
    return BDevManager::devManager->ToUUID(networkId);
}

std::string DevManager::ToNetworkId(const std::string &uuid)
{
    if (BDevManager::devManager == nullptr) {
        return "";
    }
    return BDevManager::devManager->ToNetworkId(uuid);
}

std::string DevManager::GetUnEncryptedUuid()
{
    if (BDevManager::devManager == nullptr) {
        return "";
    }
    return BDevManager::devManager->GetUnEncryptedUuid();
}
} // namespace OHOS::DistributedKv