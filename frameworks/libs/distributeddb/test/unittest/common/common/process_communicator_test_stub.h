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

#ifndef PROCESSCOMMUNICATOR_TEST_STUB_H_H
#define PROCESSCOMMUNICATOR_TEST_STUB_H_H

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

#include "iprocess_communicator.h"
#include "store_types.h"

namespace DistributedDB {
class ProcessCommunicatorTestStub : public IProcessCommunicator {
public:
    ProcessCommunicatorTestStub() {}
    ~ProcessCommunicatorTestStub() override {}

    DBStatus Start(const std::string &processLabel) override
    {
        return OK;
    }

    // The Stop should only be called after Start successfully
    DBStatus Stop() override
    {
        return OK;
    }

    DBStatus RegOnDeviceChange(const OnDeviceChange &callback) override
    {
        return OK;
    }
    DBStatus RegOnDataReceive(const OnDataReceive &callback) override
    {
        return OK;
    }

    DBStatus SendData(const DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length) override
    {
        if (isCommErr) {
            return COMM_FAILURE;
        }
        return OK;
    }

    uint32_t GetMtuSize() override
    {
        return 1 * 1024 * 1024; // 1MB
    }

    DeviceInfos GetLocalDeviceInfos() override
    {
        DeviceInfos info;
        info.identifier = "default";
        return info;
    }

    std::vector<DeviceInfos> GetRemoteOnlineDeviceInfosList() override
    {
        std::vector<DeviceInfos> info;
        return info;
    }

    bool IsSameProcessLabelStartedOnPeerDevice(const DeviceInfos &peerDevInfo) override
    {
        return true;
    }

    void SetCommErr(bool commErr)
    {
        isCommErr = commErr;
    }

    DBStatus GetDataUserInfo(DataUserInfo dataUserInfo, std::vector<UserInfo> &userInfos) override
    {
        userInfos = userInfos_;
        return OK;
    }

    void SetDataUserInfo(const std::vector<UserInfo> &userInfos)
    {
        userInfos_ = userInfos;
    }
private:
    bool isCommErr = false;
    std::vector<UserInfo> userInfos_;
};
} // namespace DistributedDB

#endif // PROCESSCOMMUNICATOR_TEST_STUB_H_H
