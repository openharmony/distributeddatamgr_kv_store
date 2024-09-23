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

#ifndef OHOS_DEVICE_MANAGER_IMPL_H
#define OHOS_DEVICE_MANAGER_IMPL_H

#include "device_manager_mock.h"
#include <map>
#include <mutex>

namespace OHOS {
namespace DistributedHardware {
class DeviceManagerImpl : public DeviceManager {
public:
    static DeviceManagerImpl &GetInstance();

public:

    virtual int32_t InitDeviceManager(const std::string &pkgName,
                                      std::shared_ptr<DmInitCallback> dmInitCallback) override;

    virtual int32_t UnInitDeviceManager(const std::string &pkgName) override;

    virtual int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
                                         std::vector<DmDeviceInfo> &deviceList) override;

    virtual int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
        bool isRefresh, std::vector<DmDeviceInfo> &deviceList) override;

    virtual int32_t GetAvailableDeviceList(const std::string &pkgName,
    	std::vector<DmDeviceBasicInfo> &deviceList) override;

    virtual int32_t GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &info) override;

    virtual int32_t RegisterDevStateCallback(const std::string &pkgName, const std::string &extra,
                                             std::shared_ptr<DeviceStateCallback> callback) override;

    virtual int32_t RegisterDevStatusCallback(const std::string &pkgName, const std::string &extra,
                                                std::shared_ptr<DeviceStatusCallback> callback) override;

    virtual int32_t GetUuidByNetworkId(const std::string &pkgName, const std::string &netWorkId,
                                       std::string &uuid) override;

    virtual int32_t GetEncryptedUuidByNetworkId(const std::string &pkgName, const std::string &networkId,
        std::string &uuid) override;

private:
    DeviceManagerImpl() = default;
    ~DeviceManagerImpl() = default;
    DeviceManagerImpl(const DeviceManagerImpl &) = delete;
    DeviceManagerImpl &operator=(const DeviceManagerImpl &) = delete;
    DeviceManagerImpl(DeviceManagerImpl &&) = delete;
    DeviceManagerImpl &operator=(DeviceManagerImpl &&) = delete;
};
} // namespace DistributedHardware
} // namespace OHOS
#endif // OHOS_DEVICE_MANAGER_IMPL_H
