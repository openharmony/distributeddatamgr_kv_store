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

#ifndef VIRTUAL_ICOMMUNICATORAGGREGATOR_H
#define VIRTUAL_ICOMMUNICATORAGGREGATOR_H

#include <cstdint>
#include <set>

#include "icommunicator_aggregator.h"
#include "virtual_communicator.h"

namespace DistributedDB {
class ICommunicator;  // Forward Declaration

class VirtualCommunicatorAggregator : public ICommunicatorAggregator {
public:
    // Return 0 as success. Return negative as error
    int Initialize(IAdapter *inAdapter) override;

    void Finalize() override;

    // If not success, return nullptr and set outErrorNo
    ICommunicator *AllocCommunicator(uint64_t commLabel, int &outErrorNo) override;
    ICommunicator *AllocCommunicator(const LabelType &commLabel, int &outErrorNo) override;

    void ReleaseCommunicator(ICommunicator *inCommunicator) override;

    int RegCommunicatorLackCallback(const CommunicatorLackCallback &onCommLack, const Finalizer &inOper) override;
    int RegOnConnectCallback(const OnConnectCallback &onConnect, const Finalizer &inOper) override;
    void RunCommunicatorLackCallback(const LabelType &commLabel);
    void RunOnConnectCallback(const std::string &target, bool isConnect);

    int GetLocalIdentity(std::string &outTarget) const override;

    // online a virtual device to the VirtualCommunicator, should call in main thread
    void OnlineDevice(const std::string &deviceId) const;

    // offline a virtual device to the VirtualCommunicator, should call in main thread
    void OfflineDevice(const std::string &deviceId) const;

    void DispatchMessage(const std::string &srcTarget, const std::string &dstTarget, const Message *inMsg,
        const OnSendEnd &onEnd);

    // If not success, return nullptr and set outErrorNo
    ICommunicator *AllocCommunicator(const std::string &deviceId, int &outErrorNo);

    ICommunicator *GetCommunicator(const std::string &deviceId) const;

    void Disable();

    void Enable();

    void SetBlockValue(bool value);

    bool GetBlockValue() const;

    void RegOnDispatch(const std::function<void(const std::string &target, Message *inMsg)> &onDispatch);

    void SetCurrentUserId(const std::string &userId);

    void SetTimeout(const std::string &deviceId, uint32_t timeout);

    void SetDropMessageTypeByDevice(const std::string &deviceId, MessageId msgid, uint32_t dropTimes = 1);

    void SetDeviceMtuSize(const std::string &deviceId, uint32_t mtuSize);

    void SetSendDelayInfo(uint32_t sendDelayTime, uint32_t delayMessageId, uint32_t delayTimes, uint32_t skipTimes,
        std::set<std::string> &delayDevices);
    void ResetSendDelayInfo();

    ~VirtualCommunicatorAggregator() {};
    VirtualCommunicatorAggregator() {};

private:
    void CallSendEnd(int errCode, const OnSendEnd &onEnd);
    void DelayTimeHandle(uint32_t messageId, const std::string &dstTarget);

    mutable std::mutex communicatorsLock_;
    std::map<std::string, VirtualCommunicator *> communicators_;
    std::string remoteDeviceId_ = "real_device";
    std::mutex blockLock_;
    std::condition_variable conditionVar_;
    bool isEnable_ = true;
    bool isBlock_ = false;
    CommunicatorLackCallback onCommLack_;
    OnConnectCallback onConnect_;
    std::function<void(const std::string &target, Message *inMsg)> onDispatch_;
    std::string userId_;

    uint32_t sendDelayTime_ = 0;
    uint32_t delayMessageId_ = INVALID_MESSAGE_ID;
    uint32_t delayTimes_ = 0; // ms
    uint32_t skipTimes_ = 0;
    std::set<std::string> delayDevices_;
};
} // namespace DistributedDB

#endif // VIRTUAL_ICOMMUNICATORAGGREGATOR_H