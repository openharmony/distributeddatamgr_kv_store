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
using AllocCommunicatorCallback = std::function<void(const std::string &userId)>;
using ReleaseCommunicatorCallback = std::function<void(const std::string &userId)>;
class VirtualCommunicatorAggregator : public ICommunicatorAggregator {
public:
    // Return 0 as success. Return negative as error
    int Initialize(IAdapter *inAdapter, const std::shared_ptr<DBStatusAdapter> &statusAdapter) override;

    void Finalize() override;

    // If not success, return nullptr and set outErrorNo
    ICommunicator *AllocCommunicator(uint64_t commLabel, int &outErrorNo, const std::string &userId = "") override;
    ICommunicator *AllocCommunicator(const LabelType &commLabel, int &outErrorNo,
        const std::string &userId = "") override;

    void ReleaseCommunicator(ICommunicator *inCommunicator, const std::string &userId = "") override;

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

    std::set<std::string> GetOnlineDevices();

    void DisableCommunicator();

    void EnableCommunicator();

    void RegBeforeDispatch(const std::function<void(const std::string &, const Message *)> &beforeDispatch);

    void SetLocalDeviceId(const std::string &deviceId);

    void MockGetLocalDeviceRes(int mockRes);

    void SetAllocCommunicatorCallback(AllocCommunicatorCallback allocCommunicatorCallback);

    void SetReleaseCommunicatorCallback(ReleaseCommunicatorCallback releaseCommunicatorCallback);

    void MockCommErrCode(int mockErrCode);

    void MockDirectEndFlag(bool isDirectEnd);

    void ClearOnlineLabel() override;

    void SetRemoteDeviceId(const std::string &dev);

    uint64_t GetAllSendMsgSize() const;

    ~VirtualCommunicatorAggregator() override = default;
    VirtualCommunicatorAggregator() = default;

private:
    void CallSendEnd(int errCode, const OnSendEnd &onEnd);
    void DelayTimeHandle(uint32_t messageId, const std::string &dstTarget);
    void DispatchMessageInner(const std::string &srcTarget, const std::string &dstTarget, const Message *inMsg,
        const OnSendEnd &onEnd);

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
    std::function<void(const std::string &target, const Message *inMsg)> beforeDispatch_;
    std::string userId_;

    uint32_t sendDelayTime_ = 0;
    uint32_t delayMessageId_ = INVALID_MESSAGE_ID;
    uint32_t delayTimes_ = 0; // ms
    uint32_t skipTimes_ = 0;
    std::set<std::string> delayDevices_;

    mutable std::mutex localDeviceIdMutex_;
    std::string localDeviceId_;
    int getLocalDeviceRet_ = E_OK;
    int commErrCodeMock_ = E_OK;
    bool isDirectEnd_ = true;

    AllocCommunicatorCallback allocCommunicatorCallback_;
    ReleaseCommunicatorCallback releaseCommunicatorCallback_;
};
} // namespace DistributedDB

#endif // VIRTUAL_ICOMMUNICATORAGGREGATOR_H