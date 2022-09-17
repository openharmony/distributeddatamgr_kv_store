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
#include "log_print.h"
#include "device_manager_callback.h"
#include "dm_device_info.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedHardware;
constexpr int32_t DM_OK = 0;
constexpr int32_t DM_ERROR = -1;
constexpr size_t DevManager::MAX_ID_LEN;

DevManager::DevManager()
{
    RegisterDevCallback();
}

int32_t DevManager::Init()
{
    return 0;
}

void DevManager::RegisterDevCallback()
{
    int32_t errNo = Init();
    if (errNo != DM_OK) {
        ZLOGE("register device failed, try again");
    }
    std::thread th = std::thread([this]() {
        constexpr int RETRY_TIMES = 300;
        int i = 0;
        int32_t errNo = DM_ERROR;
        while (i++ < RETRY_TIMES) {
            errNo = Init();
            if (errNo == DM_OK) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        ZLOGI("reg device exit now: %{public}d times, errNo: %{public}d", i, errNo);
    });
    th.detach();
}

DevManager &DevManager::GetInstance()
{
    static DevManager instance;
    return instance;
}

std::string DevManager::ToUUID(const std::string &networkId) const
{
    return "123456";
}

std::string DevManager::ToNetworkId(const std::string &uuid) const
{
    DetailInfo deviceInfo;
    if (deviceInfos_.Get(uuid, deviceInfo)) {
        return deviceInfo.networkId;
    }
    auto infos = GetRemoteDevices();
    for (auto &info : infos) {
        if (info.uuid == uuid) {
            deviceInfos_.Set(info.uuid, info);
            deviceInfos_.Set(info.networkId, info);
            return info.networkId;
        }
    }

    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    return (localInfo_.uuid == uuid) ? localInfo_.networkId : "";
}

const DevManager::DetailInfo &DevManager::GetLocalDevice()
{
    return localInfo_;
}

std::vector<DevManager::DetailInfo> DevManager::GetRemoteDevices() const
{
    std::vector<DetailInfo> devices;
    return devices;
}

void DevManager::Online(const std::string &networkId)
{
    // do nothing
}

void DevManager::Offline(const std::string &networkId)
{
    DetailInfo deviceInfo;
    if (deviceInfos_.Get(networkId, deviceInfo)) {
        deviceInfos_.Delete(networkId);
        deviceInfos_.Delete(deviceInfo.uuid);
    }
}

void DevManager::OnChanged(const std::string &networkId)
{
    // do nothing
}
} // namespace OHOS::DistributedKv
