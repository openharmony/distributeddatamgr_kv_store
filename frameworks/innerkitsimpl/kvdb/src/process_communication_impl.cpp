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

// #include "device_manager_adapter.h"
// #include "softbus_adapter.h"
#include "process_communication_impl.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
using namespace DistributedDB;
ProcessCommunicationImpl::ProcessCommunicationImpl(std::shared_ptr<EntryPoint> entryPoint)
    : entryPoint_(entryPoint)
{
}

ProcessCommunicationImpl::~ProcessCommunicationImpl()
{
    ZLOGE("destructor.");
}

DBStatus ProcessCommunicationImpl::Start(const std::string &processLabel)
{
    ZLOGI("init ProcessCommunication");
    Status errCode = entryPoint_->Start("dpProcess");
    if (errCode != Status::SUCCESS) {
        ZLOGE("proComm Start Fail.");
        return DBStatus::DB_ERROR;
    }
    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::Stop()
{
    Status errCode = entryPoint_->Stop();
    if (errCode != Status::SUCCESS) {
        ZLOGE("proComm Stop Fail.");
        return DBStatus::DB_ERROR;
    }
    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::RegOnDeviceChange(const OnDeviceChange &callback)
{
    auto devChangeCb = [callback](const DeviceInfos &devInfos, bool isOnline) {
        DistributedDB::DeviceInfos devInfo = {
            devInfos.identifier
        };
        callback(devInfo, isOnline);
    };

    Status errCode = entryPoint_->RegOnDeviceChange(devChangeCb);
    if (errCode != Status::SUCCESS) {
        ZLOGE("proComm RegOnDeviceChange Fail.");
        return DBStatus::DB_ERROR;
    };

    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::RegOnDataReceive(const OnDataReceive &callback)
{
    auto dataReciveCb = [callback](const DeviceInfos &srcDevInfo, const uint8_t *data, uint32_t length) {
        DistributedDB::DeviceInfos devInfo = {
            srcDevInfo.identifier
        };
        callback(devInfo, data, length);
    };
    
    Status errCode = entryPoint_->RegOnDataReceive(dataReciveCb);
    if (errCode != Status::SUCCESS) {
        ZLOGE("proComm RegOnDataReceive Fail.");
        return DBStatus::DB_ERROR;
    }

    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::SendData(const DistributedDB::DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length)
{
    DeviceInfos infos = {
        dstDevInfo.identifier
    };
    Status errCode = entryPoint_->SendData(infos, data, length);
    if (errCode != Status::SUCCESS) {
        ZLOGE("proComm SendData Fail.");
        return DBStatus::DB_ERROR;
    }

    return DBStatus::OK;
}

uint32_t ProcessCommunicationImpl::GetMtuSize()
{
    return entryPoint_->GetMtuSize();
}

uint32_t ProcessCommunicationImpl::GetTimeout(const DistributedDB::DeviceInfos &devInfo)
{
    DeviceInfos infos = {
        devInfo.identifier
    };
    return entryPoint_->GetTimeout(infos);
}

DistributedDB::DeviceInfos ProcessCommunicationImpl::GetLocalDeviceInfos()
{
    auto devInfo = entryPoint_->GetLocalDeviceInfos();
    DistributedDB::DeviceInfos devInfos = {
        devInfo.identifier
    };
    return devInfos;
}

std::vector<DistributedDB::DeviceInfos> ProcessCommunicationImpl::GetRemoteOnlineDeviceInfosList()
{
    auto devInfos = entryPoint_->GetRemoteOnlineDeviceInfosList();
    if (devInfos.empty()) {
        return {};
    }
    int size = devInfos.size();
    std::vector<DistributedDB::DeviceInfos> infos(size, {""});
    for (int i = 0; i < size; ++i) {
        infos[i].identifier = devInfos[i].identifier;
    }
    return infos;
}

bool ProcessCommunicationImpl::IsSameProcessLabelStartedOnPeerDevice(const DistributedDB::DeviceInfos &peerDevInfo)
{
    DeviceInfos devInfo = {
        peerDevInfo.identifier
    };
    return entryPoint_->IsSameProcessLabelStartedOnPeerDevice(devInfo);
}

void ProcessCommunicationImpl::OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const
{
    entryPoint_->OnDeviceChanged(info, type);
}
} // namespace AppDistributedKv
} // namespace OHOS

