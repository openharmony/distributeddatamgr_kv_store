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

#ifndef IADAPTER_H
#define IADAPTER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include "communicator_type_define.h"
#include "iprocess_communicator.h"

namespace DistributedDB {
struct DataUserInfoProc {
    const uint8_t *data = nullptr;
    uint32_t length = 0;
    std::shared_ptr<IProcessCommunicator> processCommunicator = nullptr;
};
struct ReceiveBytesInfo {
    const uint8_t *bytes = nullptr;
    const std::string srcTarget;
    uint32_t length = 0;
    uint32_t headLength = 0;
};

// SendableCallback only notify when status changed from unsendable to sendable
using BytesReceiveCallback = std::function<void(const ReceiveBytesInfo &receiveBytesInfo,
    const DataUserInfoProc &userInfoProc)>;
using TargetChangeCallback = std::function<void(const std::string &target, bool isConnect)>;
using SendableCallback = std::function<void(const std::string &target, int deviceCommErrCode)>;

class IAdapter {
public:
    // Register all callback before call StartAdapter.
    // Return 0 as success. Return negative as error
    // The StartAdapter should only be called by its user not owner
    virtual int StartAdapter() = 0;

    // The StopAdapter may be called by its user in precondition of StartAdapter success
    // The StopAdapter should only be called by its user not owner
    virtual void StopAdapter() = 0;

    // Should returns the multiples of 8
    virtual uint32_t GetMtuSize() = 0;
    virtual uint32_t GetMtuSize(const std::string &target) = 0;

    // Should returns timeout in range [5s, 60s]
    virtual uint32_t GetTimeout() = 0;
    virtual uint32_t GetTimeout(const std::string &target) = 0;

    // Get local target name for identify self
    virtual int GetLocalIdentity(std::string &outTarget) = 0;

    // Not assume bytes to be heap memory. Not assume SendBytes to be not blocking
    // Return 0 as success. Return negative as error
    virtual int SendBytes(const std::string &dstTarget, const uint8_t *bytes, uint32_t length,
        uint32_t totalLength) = 0;
    virtual int SendBytes(const DeviceInfos &deviceInfos, const uint8_t *bytes, uint32_t length,
        uint32_t totalLength) = 0;

    // Pass nullptr as inHandle to do unReg if need (inDecRef also nullptr)
    // Return 0 as success. Return negative as error
    virtual int RegBytesReceiveCallback(const BytesReceiveCallback &onReceive, const Finalizer &inOper) = 0;

    // Pass nullptr as inHandle to do unReg if need (inDecRef also nullptr)
    // Return 0 as success. Return negative as error
    virtual int RegTargetChangeCallback(const TargetChangeCallback &onChange, const Finalizer &inOper) = 0;

    // Pass nullptr as inHandle to do unReg if need (inDecRef also nullptr)
    // Return 0 as success. Return negative as error
    virtual int RegSendableCallback(const SendableCallback &onSendable, const Finalizer &inOper) = 0;

    virtual bool IsDeviceOnline(const std::string &device) = 0;

    virtual std::shared_ptr<ExtendHeaderHandle> GetExtendHeaderHandle(const ExtendInfo &paramInfo) = 0;

    virtual ~IAdapter() {};
};
} // namespace DistributedDB

#endif // IADAPTER_H