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

#include "device_manager_impl_mock.h"
#include "dm_constants.h"
namespace OHOS {
namespace DistributedHardware {
DeviceManagerImpl &DeviceManagerImpl::GetInstance()
{
    static DeviceManagerImpl instance;
    return instance;
}

int32_t DeviceManagerImpl::InitDeviceManager(const std::string &pkgName,
    std::shared_ptr<DmInitCallback> dmInitCallback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnInitDeviceManager(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    std::vector<DmDeviceInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    bool isRefresh, std::vector<DmDeviceInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &deviceInfo)
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

int32_t DeviceManagerImpl::GetUuidByNetworkId(const std::string &pkgName, const std::string &netWorkId,
    std::string &uuid)
{
    uuid = "";
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetEncryptedUuidByNetworkId(const std::string &pkgName, const std::string &networkId,
    std::string &uuid)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

} // namespace DistributedHardware
} // namespace OHOS
