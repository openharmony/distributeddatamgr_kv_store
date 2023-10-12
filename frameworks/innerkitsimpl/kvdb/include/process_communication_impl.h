/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef PROCESS_COMMUNICATION_IMPL_H
#define PROCESS_COMMUNICATION_IMPL_H

#include "iprocess_communicator.h"
#include "types.h"
#include "entry_point.h"

namespace OHOS {
namespace DistributedKv {
class AppDeviceChangeListener {
public:
    API_EXPORT virtual ~AppDeviceChangeListener() {};
    API_EXPORT virtual void OnDeviceChanged(const CommDeviceInfo &info, const DeviceChangeType &type) const = 0;
};

class API_EXPORT ProcessCommunicationImpl : public DistributedDB::IProcessCommunicator, public AppDeviceChangeListener {
public:
    using DeviceInfo = DistributedKv::CommDeviceInfo;
    using DBStatus = DistributedDB::DBStatus;
    using OnDeviceChange = DistributedDB::OnDeviceChange;
    using OnDataReceive = DistributedDB::OnDataReceive;
    
    API_EXPORT explicit ProcessCommunicationImpl(std::shared_ptr<EntryPoint> entryPoint);
    API_EXPORT ~ProcessCommunicationImpl() override;

    DBStatus Start(const std::string &processLabel) override;
    DBStatus Stop() override;

    DBStatus RegOnDeviceChange(const OnDeviceChange &callback) override;
    DBStatus RegOnDataReceive(const OnDataReceive &callback) override;

    DBStatus SendData(const DistributedDB::DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length) override;
    uint32_t GetMtuSize() override;
    uint32_t GetTimeout(const DistributedDB::DeviceInfos &devInfo) override;
    DistributedDB::DeviceInfos GetLocalDeviceInfos() override;
    std::vector<DistributedDB::DeviceInfos> GetRemoteOnlineDeviceInfosList() override;
    bool IsSameProcessLabelStartedOnPeerDevice(const DistributedDB::DeviceInfos &peerDevInfo) override;
    void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const override;
private:
    
    static constexpr uint32_t MTU_SIZE = 4194304; // the max transmission unit size(4M - 80B)
    std::shared_ptr<EntryPoint> entryPoint_;
};
}  // namespace AppDistributedKv
}  // namespace OHOS
#endif // PROCESS_COMMUNICATION_IMPL_H