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
ProcessCommunicationImpl::ProcessCommunicationImpl(std::shared_ptr<EntryPoint> entryPoint)
    : entryPoint_(entryPoint)
{
    ZLOGI("constructor.");
}

ProcessCommunicationImpl::~ProcessCommunicationImpl()
{
    ZLOGI("destructor.");
}

DBStatus ProcessCommunicationImpl::Start(const std::string &processLabel)
{
    ZLOGI("init ProcessCommunication");
    Status errCode = entryPoint_->Start(processLabel);
    if (errCode != Status::SUCCESS) {
        ZLOGE("entryPoint Start Fail.");
        return DBStatus::DB_ERROR;
    }
    isCreateSessionServer_ = true;
    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::Stop()
{
    Status errCode = entryPoint_->Stop();
    if (errCode != Status::SUCCESS) {
        ZLOGE("entryPoint Stop Fail.");
        return DBStatus::DB_ERROR;
    }
    isCreateSessionServer_ = false;
    return DBStatus::OK;
}

DBStatus ProcessCommunicationImpl::RegOnDeviceChange(const OnDeviceChange &callback)
{
    auto devChangeCallback = [callback](const DeviceInfos &devInfos, bool isOnline) {
        DistributedDB::DeviceInfos devInfo = {
            devInfos.identifier
        };
        callback(devInfo, isOnline);
    };

    Status errCode = entryPoint_->RegOnDeviceChange(devChangeCallback);
    if (errCode != Status::SUCCESS) {
        ZLOGE("RegOnDeviceChange Fail.");
        return DBStatus::DB_ERROR;
    };

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
    
    Status errCode = entryPoint_->RegOnDataReceive(dataReciveCallback);
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
    Status errCode = entryPoint_->SendData(infos, data, length);
    if (errCode != Status::SUCCESS) {
        ZLOGE("SendData Fail.");
        return DBStatus::DB_ERROR;
    }

    return DBStatus::OK;
}

uint32_t ProcessCommunicationImpl::GetMtuSize()
{
    return DEFAULT_MTU_SIZE;
}

uint32_t ProcessCommunicationImpl::GetMtuSize(const DistributedDB::DeviceInfos &devInfo)
{
    DeviceInfos infos = {
        devInfo.identifier
    };
    return entryPoint_->GetMtuSize(infos);
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

bool ProcessCommunicationImpl::IsSameProcessLabelStartedOnPeerDevice(__attribute__((unused)) const DistributedDB::DeviceInfos &peerDevInfo)
{
    return isCreateSessionServer_;
}
} // namespace AppDistributedKv
} // namespace OHOS

