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
#ifndef MOCK_PROCESS_COMMUNICATOR_H
#define MOCK_PROCESS_COMMUNICATOR_H
#include <gmock/gmock.h>
#include "iprocess_communicator.h"

namespace DistributedDB {
class MockProcessCommunicator : public IProcessCommunicator {
public:
    MOCK_METHOD1(Start, DBStatus(const std::string &));
    MOCK_METHOD0(Stop, DBStatus());
    MOCK_METHOD1(RegOnDeviceChange, DBStatus(const OnDeviceChange &));
    MOCK_METHOD1(RegOnDataReceive, DBStatus(const OnDataReceive &));
    MOCK_METHOD3(SendData, DBStatus(const DeviceInfos &, const uint8_t *, uint32_t));
    MOCK_METHOD0(GetMtuSize, uint32_t());
    MOCK_METHOD0(GetTimeout, uint32_t());
    MOCK_METHOD0(GetLocalDeviceInfos, DeviceInfos());
    MOCK_METHOD0(GetRemoteOnlineDeviceInfosList, std::vector<DeviceInfos>());
    MOCK_METHOD1(IsSameProcessLabelStartedOnPeerDevice, bool(const DeviceInfos &));
    MOCK_METHOD4(CheckAndGetDataHeadInfo, DBStatus(const uint8_t *, uint32_t, uint32_t &,
        std::vector<std::string> &));
};
}
#endif // MOCK_PROCESS_COMMUNICATOR_H
