/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "dev_manager.h"

namespace OHOS::DistributedKv {
static constexpr const char *PKG_NAME_EX = "kv_store";
DevManager::DevManager(const std::string &pkgName) : PKG_NAME(pkgName)
{
    RegisterDevCallback();
}

int32_t DevManager::Init()
{
    return -1;
}

void DevManager::RegisterDevCallback()
{
}

std::function<void()> DevManager::Retry()
{
    return nullptr;
}

DevManager &DevManager::GetInstance()
{
    static DevManager instance(PKG_NAME_EX);
    return instance;
}

std::string DevManager::ToUUID(const std::string &networkId)
{
    (void)networkId;
    return "";
}

std::string DevManager::ToNetworkId(const std::string &uuid)
{
    (void)uuid;
    return "";
}

DevManager::DetailInfo DevManager::GetDevInfoFromBucket(const std::string &id)
{
    (void)id;
    DetailInfo detailInfo;
    return detailInfo;
}

void DevManager::UpdateBucket()
{
}

std::string DevManager::GetUnEncryptedUuid()
{
    return "";
}

const DevManager::DetailInfo &DevManager::GetLocalDevice()
{
    return localInfo_;
}

std::vector<DevManager::DetailInfo> DevManager::GetRemoteDevices()
{
    return {};
}
} // namespace OHOS::DistributedKv