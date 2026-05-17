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
#define LOG_TAG "DeviceAdapter"

#include "device_adapter.h"

#include "device_manager.h"
#include "device_manager_callback.h"
#include "dm_device_info.h"
#include "log_print.h"
#include "visibility.h"

using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedHardware;
using DevInfo = OHOS::DistributedHardware::DmDeviceInfo;

API_EXPORT std::shared_ptr<OHOS::DistributedKv::DeviceAdapter> Create(const std::string &pkgName) asm(
     "CreateDeviceAdapterDelegate");

namespace OHOS::DistributedKv {
DeviceAdapter::~DeviceAdapter()
{}

class DmDeathCallback : public DmInitCallback {
public:
    explicit DmDeathCallback(DeviceAdapter &deviceAdapter) : deviceAdapter_(deviceAdapter){};
    void OnRemoteDied() override;

private:
    DeviceAdapter &deviceAdapter_;
};

void DmDeathCallback::OnRemoteDied()
{
    ZLOGI("Dm device manager died, init it again");
    deviceAdapter_.Init();
}

constexpr int32_t DM_OK = 0;

class DeviceAdapterImpl : public DeviceAdapter {
public:
    explicit DeviceAdapterImpl(const std::string &pkgName);
    ~DeviceAdapterImpl() override = default;
    void Init() override;
    bool GetLocalDeviceInfo(DetailInfo &detailInfo) override;
    std::string GetUuidByNetworkId(const std::string &networkId) override;
    std::string GetEncryptedUuidByNetworkId(const std::string &networkId) override;
    std::vector<DetailInfo> GetTrustedDeviceList() override;

private:
    friend class DmDeathCallback;
    const std::string PKG_NAME;
};

DeviceAdapterImpl::DeviceAdapterImpl(const std::string &pkgName) : PKG_NAME(pkgName)
{
    Init();
}

void DeviceAdapterImpl::Init()
{
    auto &deviceManager = DeviceManager::GetInstance();
    auto deviceInitCallback = std::make_shared<DmDeathCallback>(*this);
    deviceManager.InitDeviceManager(PKG_NAME, deviceInitCallback);
}

bool DeviceAdapterImpl::GetLocalDeviceInfo(DetailInfo &detailInfo)
{
    DevInfo info;
    auto ret = DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, info);
    if (ret != DM_OK) {
        ZLOGE("Get local device info fail");
        return false;
    }
    detailInfo.networkId = info.networkId;
    return true;
}

std::string DeviceAdapterImpl::GetUuidByNetworkId(const std::string &networkId)
{
    std::string uuid;
    DeviceManager::GetInstance().GetUuidByNetworkId(PKG_NAME, networkId, uuid);
    return uuid;
}

std::string DeviceAdapterImpl::GetEncryptedUuidByNetworkId(const std::string &networkId)
{
    std::string uuid;
    DeviceManager::GetInstance().GetEncryptedUuidByNetworkId(PKG_NAME, networkId, uuid);
    return uuid;
}

std::vector<DetailInfo> DeviceAdapterImpl::GetTrustedDeviceList()
{
    std::vector<DevInfo> dmInfos;
    auto ret = DeviceManager::GetInstance().GetTrustedDeviceList(PKG_NAME, "", dmInfos);
    if (ret != DM_OK) {
        ZLOGE("Get trusted device:%{public}d", ret);
        return {};
    }
    if (dmInfos.empty()) {
        ZLOGD("No remote device");
        return {};
    }
    std::vector<DetailInfo> dtInfos;
    for (auto &device : dmInfos) {
        DetailInfo dtInfo;
        dtInfo.networkId = std::move(device.networkId);
        dtInfos.push_back(dtInfo);
    }
    return dtInfos;
}
} // namespace OHOS::DistributedKv

std::shared_ptr<OHOS::DistributedKv::DeviceAdapter> Create(const std::string &pkgName)
{
    return std::make_shared<OHOS::DistributedKv::DeviceAdapterImpl>(pkgName);
}
