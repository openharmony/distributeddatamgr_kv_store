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
#include "device_manager.h"
#include "device_manager_callback.h"
#include "dm_device_info.h"
#include "kvdb_service_client.h"
#include "log_print.h"
#include "store_util.h"
#include "task_executor.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedHardware;
using DevInfo = OHOS::DistributedHardware::DmDeviceInfo;
constexpr int32_t DM_OK = 0;
constexpr int32_t DM_ERROR = -1;
constexpr size_t DevManager::MAX_ID_LEN;
constexpr const char *PKG_NAME_EX = "_distributed_data";
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
    return deviceManager.InitDeviceManager(PKG_NAME, deviceInitCallback);
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
        TaskExecutor::GetInstance().Schedule(std::chrono::milliseconds(interval), Retry());
    };
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

DevManager::DetailInfo DevManager::GetDevInfoFromBucket(const std::string &id)
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
        ZLOGD("no remote device");
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

const DevManager::DetailInfo &DevManager::GetLocalDevice()
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (!localInfo_.uuid.empty()) {
        return localInfo_;
    }
    DevInfo info;
    auto ret = DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, info);
    if (ret != DM_OK) {
        ZLOGE("get local device info fail");
        return invalidDetail_;
    }
    auto networkId = std::string(info.networkId);
    std::string uuid;
    DeviceManager::GetInstance().GetEncryptedUuidByNetworkId(PKG_NAME, networkId, uuid);
    if (uuid.empty() || networkId.empty()) {
        return invalidDetail_;
    }
    localInfo_.networkId = std::move(networkId);
    localInfo_.uuid = std::move(uuid);
    ZLOGI("[LocalDevice] uuid:%{public}s, networkId:%{public}s", StoreUtil::Anonymous(localInfo_.uuid).c_str(),
        StoreUtil::Anonymous(localInfo_.networkId).c_str());
    return localInfo_;
}

std::vector<DevManager::DetailInfo> DevManager::GetRemoteDevices()
{
    std::vector<DevInfo> dmInfos;
    auto ret = DeviceManager::GetInstance().GetTrustedDeviceList(PKG_NAME, "", dmInfos);
    if (ret != DM_OK) {
        ZLOGE("get trusted device:%{public}d", ret);
        return {};
    }
    if (dmInfos.empty()) {
        ZLOGD("no remote device");
        return {};
    }
    std::vector<DetailInfo> dtInfos;
    for (auto &device : dmInfos) {
        DetailInfo dtInfo;
        auto networkId = std::string(device.networkId);
        std::string uuid;
        DeviceManager::GetInstance().GetEncryptedUuidByNetworkId(PKG_NAME, networkId, uuid);
        dtInfo.networkId = std::move(device.networkId);
        dtInfo.uuid = std::move(uuid);
        dtInfos.push_back(dtInfo);
    }
    return dtInfos;
}
} // namespace OHOS::DistributedKv
