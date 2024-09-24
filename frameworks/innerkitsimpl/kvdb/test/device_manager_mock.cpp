/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "device_manager_mock.h"
#include "dm_constants.h"

namespace OHOS {
namespace DistributedHardware {
DeviceManager &DeviceManager::GetInstance()
{
    static DeviceManager instance;
    return instance;
}

int32_t DeviceManager::InitDeviceManager(const std::string &pkgName,
    std::shared_ptr<DmInitCallback> dmInitCallback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManager::UnInitDeviceManager(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManager::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    std::vector<DmDeviceInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManager::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    bool isRefresh, std::vector<DmDeviceInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManager::GetAvailableDeviceList(const std::string &pkgName,
        std::vector<DmDeviceBasicInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManager::GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &deviceInfo)
{
    if (pkgName == "demo_distributed_data") {
        deviceInfo.networkId[0] = 'a';
        return DM_OK;
    }
    if (pkgName == "test_distributed_data") {
        return DM_OK;
    }
    return ERR_DM_IPC_SEND_REQUEST_FAILED;
}

int32_t DeviceManager::RegisterDevStateCallback(const std::string &pkgName, const std::string &extra,
        std::shared_ptr<DeviceStateCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManager::RegisterDevStatusCallback(const std::string &pkgName, const std::string &extra,
        std::shared_ptr<DeviceStatusCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManager::GetUuidByNetworkId(const std::string &pkgName, const std::string &netWorkId,
    std::string &uuid)
{
    uuid = "";
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManager::GetEncryptedUuidByNetworkId(const std::string &pkgName, const std::string &networkId,
    std::string &uuid)
{
    return ERR_DM_INPUT_PARA_INVALID;
}
} // namespace DistributedHardware
} // namespace OHOS
