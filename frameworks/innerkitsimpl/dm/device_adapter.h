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
#ifndef DEVICE_ADAPTER_H
#define DEVICE_ADAPTER_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS::DistributedKv {
struct DetailInfo {
    std::string uuid;
    std::string networkId;
    std::string deviceName;
    std::string deviceType;
};

class DeviceAdapter {
public:
    virtual ~DeviceAdapter();
    virtual void Init() = 0;
    virtual bool GetLocalDeviceInfo(DetailInfo &detailInfo) = 0;
    virtual std::string GetUuidByNetworkId(const std::string &networkId) = 0;
    virtual std::string GetEncryptedUuidByNetworkId(const std::string &networkId) = 0;
    virtual std::vector<DetailInfo> GetTrustedDeviceList() = 0;
};
} // namespace OHOS::DistributedKv
#endif // DEVICE_ADAPTER_H