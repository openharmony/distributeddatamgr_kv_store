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
#include "end_point.h"

namespace OHOS {
namespace DistributedKv {

class ProcessCommunicationImpl : public DistributedDB::IProcessCommunicator {
public:
    using DBStatus = DistributedDB::DBStatus;
    using OnDeviceChange = DistributedDB::OnDeviceChange;
    using OnDataReceive = DistributedDB::OnDataReceive;
    
    API_EXPORT explicit ProcessCommunicationImpl(std::shared_ptr<Endpoint> endpoint);
    API_EXPORT ~ProcessCommunicationImpl() override;

    DBStatus Start(const std::string &processLabel) override;
    DBStatus Stop() override;

    DBStatus RegOnDeviceChange(const OnDeviceChange &callback) override;
    DBStatus RegOnDataReceive(const OnDataReceive &callback) override;

    DBStatus SendData(const DistributedDB::DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length) override;
    uint32_t GetMtuSize() override;
    uint32_t GetMtuSize(const DistributedDB::DeviceInfos &devInfo) override;
    DistributedDB::DeviceInfos GetLocalDeviceInfos() override;
    std::vector<DistributedDB::DeviceInfos> GetRemoteOnlineDeviceInfosList() override;
    bool IsSameProcessLabelStartedOnPeerDevice(const DistributedDB::DeviceInfos &peerDevInfo) override;
    std::shared_ptr<DistributedDB::ExtendHeaderHandle> GetExtendHeaderHandle(
        const DistributedDB::ExtendInfo &paramInfo) override;
private:
    
    std::shared_ptr<Endpoint> endpoint_;
    bool isCreateSessionServer_ = false;
};
}  // namespace AppDistributedKv
}  // namespace OHOS
#endif // PROCESS_COMMUNICATION_IMPL_H