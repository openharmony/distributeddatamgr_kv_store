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

#ifndef ENTRY_POINT_H
#define ENTRY_POINT_H

#include <functional>
#include "types.h"

namespace OHOS {
namespace DistributedKv {
class API_EXPORT EntryPoint {
public:
    
    using OnDeviceChange = std::function<void(const DeviceInfos &devInfo, bool isOnline)>;
    using OnDataReceive = std::function<void(const DeviceInfos &srcDevInfo, const uint8_t *data, uint32_t length)>;
    using DeviceInfos = DistributedKv::DeviceInfos;
    using SecurityOption = DistributedKv::SecurityOption;

    /**
     * @brief Constructor.
     */
    API_EXPORT EntryPoint() = default;

    /**
     * @brief Destructor.
     */
    API_EXPORT virtual ~EntryPoint() {};

    /**
     * @brief Start the Process Communicator.
     * @param processLabel Identifies current process
     * @return Return SUCCESS for success, others for failure.    
     */
    API_EXPORT virtual Status Start(const std::string &processLabel) = 0;

    /**
     * @brief Start the Process Communicator.
     * @return Return SUCCESS for success, others for failure. 
     */
    API_EXPORT virtual Status Stop() = 0;

    /**
     * @brief Register the callback function for receiving data.
     * @param callback Callback to register device change.
     * @return Return SUCCESS for success, others for failure. 
     */
    API_EXPORT virtual Status RegOnDeviceChange(const OnDeviceChange &callback) = 0;

    /**
     * @brief Close all opened kvstores for this appId.
     * @param callback Callback to register data change.
     * @return Return SUCCESS for success, others for failure.
     */
    API_EXPORT virtual Status RegOnDataReceive(const OnDataReceive &callback) = 0;

    /**
     * @brief Used to send data to the softbus.
     * @param dstDevInfo Target device ID.
     * @param data Pointer to data to be send.
     * @param length Data length.
     * @return Return SUCCESS for success, others for failure.
     */
    API_EXPORT virtual Status SendData(const DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length) = 0;

    /**
     * @brief Obtains the size of maximum unit sent by the bottom layer.
     * @param devInfo Remote device ID.
     * @return Data size.
     */
    API_EXPORT virtual uint32_t GetMtuSize(const DeviceInfos &devInfo) = 0;
    
    /**
     * @brief Obtains the ID of the loacl device info.
     * @return loacl device info.
     */
    API_EXPORT virtual DeviceInfos GetLocalDeviceInfos() = 0;

    /**
     * @brief Obtains the IDs of all online devices.
     * @return All online devices info.
     */
    API_EXPORT virtual std::vector<DeviceInfos> GetRemoteOnlineDeviceInfosList() = 0;
    
    /**
     * @brief Determines whether the device has the capability of data of this level.
     * @param devId Target device ID.
     * @param option Security params.
     * @return Return true for success, false for failure.
     */
    API_EXPORT virtual bool CheckDeviceSecurityAbility(const std::string &devId, const SecurityOption &option) = 0;

    /**
     * @brief Verify sync permission.
     * @param param Params for sync permission.
     * @param flag The direction of sync.
     * @return Return true for success, false for failure.
     */
    API_EXPORT virtual bool SyncPermissionCheck(const PermissionCheckParam &param, uint8_t flag) = 0;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // ENTRY_POINT_H
