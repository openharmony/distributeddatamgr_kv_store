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
#define LOG_TAG "DevManager"
#include "dev_manager.h"
#include <unistd.h>
#include "kvdb_service_client.h"
#include "log_print.h"
#include "store_util.h"

namespace OHOS::DistributedKv {
constexpr size_t DevManager::MAX_ID_LEN;
constexpr const char *PKG_NAME_EX = "_distributed_data";

using Creator = std::shared_ptr<OHOS::DistributedKv::DeviceAdapter> (*)(const std::string &pkgName);

DevManager::DevManager(const std::string &pkgName) : PKG_NAME(pkgName + PKG_NAME_EX)
{
    (void)GetDelegate();
}

void* DevManager::GetHandle()
{
    std::lock_guard<std::mutex> lock(handleMutex_);
    if (handle_ == nullptr) {
        handle_ = dlopen("libdm_adapter.z.so", RTLD_LAZY);
        if (handle_ == nullptr) {
            ZLOGE("dlopen dm so failed errno is %{public}d", errno);
        }
    }
    return handle_;
}

std::shared_ptr<DeviceAdapter> DevManager::CreateDelegate()
{
    auto handle = GetHandle();
    if (handle == nullptr) {
        return nullptr;
    }
    auto creator = reinterpret_cast<Creator>(dlsym(handle, "CreateDeviceAdapterDelegate"));
    if (creator == nullptr) {
        ZLOGE("dlsym CreateDeviceAdapterDelegate failed, errno:%{public}d", errno);
        return nullptr;
    }
    return creator(PKG_NAME);
}

std::shared_ptr<DeviceAdapter> DevManager::GetDelegate()
{
    std::lock_guard<std::mutex> lock(delegateMutex_);
    if (deviceAdapter_ != nullptr) {
        return deviceAdapter_;
    }
    deviceAdapter_ = CreateDelegate();
    if (deviceAdapter_ == nullptr) {
        return nullptr;
    }
    return deviceAdapter_;
}

DevManager &DevManager::GetInstance()
{
    static DevManager instance(std::to_string(getpid()));
    return instance;
}

std::string DevManager::ToUUID(const std::string &networkId)
{
    return GetDevInfoFromBucket(networkId).uuid;
}

std::string DevManager::ToNetworkId(const std::string &uuid)
{
    return GetDevInfoFromBucket(uuid).networkId;
}

DetailInfo DevManager::GetDevInfoFromBucket(const std::string &id)
{
    DetailInfo detailInfo;
    if (!deviceInfos_.Get(id, detailInfo)) {
        UpdateBucket();
        deviceInfos_.Get(id, detailInfo);
    }
    if (detailInfo.uuid.empty()) {
        ZLOGE("id:%{public}s", StoreUtil::Anonymous(id).c_str());
    }
    return detailInfo;
}

void DevManager::UpdateBucket()
{
    auto detailInfos = GetRemoteDevices();
    if (detailInfos.empty()) {
        ZLOGD("No remote device");
    }
    detailInfos.emplace_back(GetLocalDevice());
    for (const auto &detailInfo : detailInfos) {
        if (detailInfo.uuid.empty() || detailInfo.networkId.empty()) {
            continue;
        }
        deviceInfos_.Set(detailInfo.uuid, detailInfo);
        deviceInfos_.Set(detailInfo.networkId, detailInfo);
    }
}

std::string DevManager::GetUnEncryptedUuid()
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (!UnEncryptedLocalInfo_.uuid.empty()) {
        return UnEncryptedLocalInfo_.uuid;
    }
    DetailInfo info;
    auto deviceAdapter = GetDelegate();
    if (deviceAdapter == nullptr) {
        return "";
    }
    if (!deviceAdapter->GetLocalDeviceInfo(info)) {
        return "";
    }
    auto networkId = std::string(info.networkId);
    if (networkId.empty()) {
        ZLOGE("This networkid is empty");
        return "";
    }
    std::string uuid = deviceAdapter->GetUuidByNetworkId(networkId);
    if (uuid.empty()) {
        ZLOGE("Get uuid by networkid fail");
        return "";
    }
    UnEncryptedLocalInfo_.uuid = std::move(uuid);
    ZLOGI("[GetUnEncryptedUuid] uuid:%{public}s, networkId:%{public}s",
        StoreUtil::Anonymous(UnEncryptedLocalInfo_.uuid).c_str(),
        StoreUtil::Anonymous(networkId).c_str());
    return UnEncryptedLocalInfo_.uuid;
}

const DetailInfo &DevManager::GetLocalDevice()
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (!localInfo_.uuid.empty()) {
        return localInfo_;
    }
    auto deviceAdapter = GetDelegate();
    if (deviceAdapter == nullptr) {
        return invalidDetail_;
    }
    DetailInfo info;
    if (!deviceAdapter->GetLocalDeviceInfo(info)) {
        return invalidDetail_;
    }
    auto networkId = std::string(info.networkId);
    std::string uuid = deviceAdapter->GetEncryptedUuidByNetworkId(networkId);
    if (uuid.empty() || networkId.empty()) {
        return invalidDetail_;
    }
    localInfo_.networkId = std::move(networkId);
    localInfo_.uuid = std::move(uuid);
    ZLOGI("[LocalDevice] uuid:%{public}s, networkId:%{public}s", StoreUtil::Anonymous(localInfo_.uuid).c_str(),
        StoreUtil::Anonymous(localInfo_.networkId).c_str());
    return localInfo_;
}

std::vector<DetailInfo> DevManager::GetRemoteDevices()
{
    auto deviceAdapter = GetDelegate();
    if (deviceAdapter == nullptr) {
        return {};
    }
    std::vector<DetailInfo> dtInfos = deviceAdapter->GetTrustedDeviceList();
    if (dtInfos.empty()) {
        ZLOGD("No remote device");
        return {};
    }
    for (auto &dtInfo : dtInfos) {
        auto networkId = std::string(dtInfo.networkId);
        std::string uuid = deviceAdapter->GetEncryptedUuidByNetworkId(networkId);
        if (uuid.empty()) {
            ZLOGE("Get uuid fail by networkId,networkId:%{public}s", StoreUtil::Anonymous(networkId).c_str());
            continue;
        }
        dtInfo.uuid = std::move(uuid);
        dtInfos.push_back(dtInfo);
    }
    return dtInfos;
}
} // namespace OHOS::DistributedKv
