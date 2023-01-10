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
#include <thread>
#include <unistd.h>
#include "device_manager.h"
#include "device_manager_callback.h"
#include "dm_device_info.h"
#include "kvdb_service_client.h"
#include "log_print.h"
#include "store_util.h"
#include "task_executor.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedHardware;
constexpr int32_t DM_OK = 0;
constexpr int32_t DM_ERROR = -1;
constexpr size_t DevManager::MAX_ID_LEN;
constexpr const char *PKG_NAME_EX = "_distributed_data";
class DMStateCallback : public DeviceStateCallback {
public:
    explicit DMStateCallback(DevManager &devManager) : devManager_(devManager){};
    void OnDeviceOnline(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceOffline(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceChanged(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceReady(const DmDeviceInfo &deviceInfo) override;

private:
    DevManager &devManager_;
};

void DMStateCallback::OnDeviceOnline(const DmDeviceInfo &deviceInfo)
{
    devManager_.Online(deviceInfo.networkId);
}

void DMStateCallback::OnDeviceOffline(const DmDeviceInfo &deviceInfo)
{
    devManager_.Offline(deviceInfo.networkId);
}

void DMStateCallback::OnDeviceChanged(const DmDeviceInfo &deviceInfo)
{
    devManager_.OnChanged(deviceInfo.networkId);
}

void DMStateCallback::OnDeviceReady(const DmDeviceInfo &deviceInfo)
{
    devManager_.OnReady(deviceInfo.networkId);
}

class DmDeathCallback : public DmInitCallback {
public:
    explicit DmDeathCallback(DevManager &devManager) : devManager_(devManager){};
    void OnRemoteDied() override;

private:
    DevManager &devManager_;
};

void DmDeathCallback::OnRemoteDied()
{
    ZLOGI("dm device manager died, init it again");
    devManager_.RegisterDevCallback();
}

DevManager::DevManager(const std::string &pkgName) : PKG_NAME(pkgName + PKG_NAME_EX)
{
    RegisterDevCallback();
}

int32_t DevManager::Init()
{
    auto &deviceManager = DeviceManager::GetInstance();
    auto deviceInitCallback = std::make_shared<DmDeathCallback>(*this);
    auto deviceCallback = std::make_shared<DMStateCallback>(*this);
    int32_t errNo = deviceManager.InitDeviceManager(PKG_NAME, deviceInitCallback);
    if (errNo != DM_OK) {
        return errNo;
    }
    errNo = deviceManager.RegisterDevStateCallback(PKG_NAME, "", deviceCallback);
    return errNo;
}

void DevManager::RegisterDevCallback()
{
    auto check = Retry();
    check();
}

std::function<void()> DevManager::Retry()
{
    return [this]() {
        int32_t errNo = DM_ERROR;
        errNo = Init();
        if (errNo == DM_OK) {
            return;
        }
        constexpr int32_t interval = 100;
        TaskExecutor::GetInstance().Execute(Retry(), interval);
    };
}

DevManager &DevManager::GetInstance()
{
    static DevManager instance(std::to_string(getpid()));
    return instance;
}

std::string DevManager::ToUUID(const std::string &networkId)
{
    return GetDvInfoFromBucket(networkId).deviceUuid;
}

std::string DevManager::ToNetworkId(const std::string &uuid)
{
    return GetDvInfoFromBucket(uuid).deviceId;
}

DeviceInfo DevManager::GetDvInfoFromBucket(const std::string &id)
{
    DeviceInfo dvInfo;
    if (!deviceInfos_.Get(id, dvInfo)) {
        UpdateBucket();
        deviceInfos_.Get(id, dvInfo);
    }
    if (dvInfo.deviceUuid.empty()) {
        ZLOGE("id:%{public}s", StoreUtil::Anonymous(id).c_str());
    }
    return dvInfo;
}

void DevManager::UpdateBucket()
{
    auto dvInfos = GetRemoteDevices();
    if (dvInfos.empty()) {
        ZLOGD("no remote device");
    }
    dvInfos.emplace_back(GetLocalDevice());
    for (const auto &info : dvInfos) {
        if (info.deviceId.empty() || info.deviceUuid.empty()) {
            continue;
        }
        deviceInfos_.Set(info.deviceId, info);
        deviceInfos_.Set(info.deviceUuid, info);
    }
}

const DeviceInfo &DevManager::GetLocalDevice()
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (!localInfo_.deviceUuid.empty()) {
        return localInfo_;
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        ZLOGE("service unavailable");
        return {};
    }
    auto status = service->GetLocalDevice(localInfo_);
    if (status != SUCCESS) {
        return {};
    }
    ZLOGI("[LocalDevice] id:%{public}s, name:%{public}s, type:%{public}s",
          StoreUtil::Anonymous(localInfo_.deviceUuid).c_str(),
          localInfo_.deviceName.c_str(), localInfo_.deviceType.c_str());
    return localInfo_;
}

std::vector<DeviceInfo> DevManager::GetRemoteDevices() const
{
    std::vector<DeviceInfo> dvInfos;
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        ZLOGE("service unavailable");
        return {};
    }
    auto status = service->GetRemoteDevices(dvInfos);
    if (status != SUCCESS || dvInfos.empty()) {
        ZLOGD("no remote device");
    }
    return dvInfos;
}

void DevManager::Online(const std::string &networkId)
{
    // do nothing
    ZLOGI("%{public}s observers:%{public}zu", StoreUtil::Anonymous(networkId).c_str(), observers_.Size());
}

void DevManager::Offline(const std::string &networkId)
{
    DeviceInfo dvInfo;
    if (deviceInfos_.Get(networkId, dvInfo)) {
        deviceInfos_.Delete(dvInfo.deviceId);
        deviceInfos_.Delete(dvInfo.deviceUuid);
    }
    ZLOGI("%{public}s observers:%{public}zu", StoreUtil::Anonymous(networkId).c_str(), observers_.Size());
    observers_.ForEach([&networkId](const auto &key, auto &value) {
        value->Offline(networkId);
        return false;
    });
}

void DevManager::OnChanged(const std::string &networkId)
{
    // do nothing
    ZLOGI("%{public}s observers:%{public}zu", StoreUtil::Anonymous(networkId).c_str(), observers_.Size());
}

void DevManager::OnReady(const std::string &networkId)
{
    ZLOGI("%{public}s observers:%{public}zu", StoreUtil::Anonymous(networkId).c_str(), observers_.Size());
    observers_.ForEach([&networkId](const auto &key, auto &value) {
        value->Online(networkId);
        return false;
    });
}

void DevManager::Register(DevManager::Observer *observer)
{
    observers_.Insert(observer, observer);
}

void DevManager::Unregister(DevManager::Observer *observer)
{
    observers_.Erase(observer);
}
} // namespace OHOS::DistributedKv
