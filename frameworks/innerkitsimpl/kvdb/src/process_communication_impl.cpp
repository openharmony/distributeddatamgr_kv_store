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

#define LOG_TAG "ProcessCommunicationImpl"

#include "process_communication_impl.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
using namespace DistributedDB;
ProcessCommunicationImpl::ProcessCommunicationImpl(std::shared_ptr<Endpoint> endpoint)
    : endpoint_(endpoint)
{
}

ProcessCommunicationImpl::~ProcessCommunicationImpl()
{
}

DBStatus ProcessCommunicationImpl::Start(const std::string &processLabel)
{
    Status errCode = endpoint_->Start();
    if (errCode != Status::SUCCESS) {
        ZLOGE("endpoint Start Fail: %{public}d", errCode);
        return DBStatus::DB_ERROR;
    }
    isCreateSessionServer_ = true;
    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::Stop()
{
    Status errCode = endpoint_->Stop();
    if (errCode != Status::SUCCESS) {
        ZLOGE("endpoint Stop Fail: %{public}d", errCode);
        return DBStatus::DB_ERROR;
    }
    isCreateSessionServer_ = false;
    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::RegOnDeviceChange(const OnDeviceChange &callback)
{
    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::RegOnDataReceive(const OnDataReceive &callback)
{
    auto dataReciveCallback = [callback](const DeviceInfos &srcDevInfo, const uint8_t *data, uint32_t length) {
        DistributedDB::DeviceInfos devInfo = {
            srcDevInfo.identifier
        };
        callback(devInfo, data, length);
    };
    
    Status errCode = endpoint_->RegOnDataReceive(dataReciveCallback);
    if (errCode != Status::SUCCESS) {
        ZLOGE("RegOnDataReceive Fail.");
        return DBStatus::DB_ERROR;
    }

    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::SendData(const DistributedDB::DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length)
{
    DeviceInfos infos = {
        dstDevInfo.identifier
    };
    Status errCode = endpoint_->SendData(infos, data, length);
    if (errCode != Status::SUCCESS) {
        ZLOGE("SendData Fail.");
        return DBStatus::DB_ERROR;
    }

    return DBStatus::OK;
}

uint32_t ProcessCommunicationImpl::GetMtuSize()
{
    return endpoint_->GetMtuSize({""});
}

uint32_t ProcessCommunicationImpl::GetMtuSize(const DistributedDB::DeviceInfos &devInfo)
{
    DeviceInfos infos = {
        devInfo.identifier
    };
    return endpoint_->GetMtuSize(infos);
}

DistributedDB::DeviceInfos ProcessCommunicationImpl::GetLocalDeviceInfos()
{
    auto devInfo = endpoint_->GetLocalDeviceInfos();
    DistributedDB::DeviceInfos devInfos = {
        devInfo.identifier
    };
    return devInfos;
}

std::vector<DistributedDB::DeviceInfos> ProcessCommunicationImpl::GetRemoteOnlineDeviceInfosList()
{
    return {};
}

bool ProcessCommunicationImpl::IsSameProcessLabelStartedOnPeerDevice(__attribute__((unused)) const DistributedDB::DeviceInfos &peerDevInfo)
{
    return isCreateSessionServer_;
}
} // namespace AppDistributedKv
} // namespace OHOS

