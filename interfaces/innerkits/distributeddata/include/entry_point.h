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
    
    using DeviceInfo = DistributedKv::CommDeviceInfo;
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
     * @brief 
     * @param processLabel    
     * @return 
     *         
     *         
    */
    API_EXPORT virtual Status Start(const std::string &processLabel) = 0;

    /**
     * @brief
     * @return
    */
    API_EXPORT virtual Status Stop() = 0;

    /**
     * @brief
     * @warning
     * @param callback
     * @return
    */
    API_EXPORT virtual Status RegOnDeviceChange(const OnDeviceChange &callback) = 0;

    /**
     * @brief Close all opened kvstores for this appId.
     * @param callback
     * @return
    */
    API_EXPORT virtual Status RegOnDataReceive(const OnDataReceive &callback) = 0;

    /**
     * @brief
     * @param dstDevInfo
     * @param data
     * @param length
     * @return
    */
    API_EXPORT virtual Status SendData(const DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length) = 0;

    /**
     * @brief Delete all kvstore for this appId.
     *
     * @param peerDevInfo
     * @return
    */
    API_EXPORT virtual bool IsSameProcessLabelStartedOnPeerDevice(const DeviceInfos &peerDevInfo) = 0;

    /**
     * @brief Observer the kvstore service death.
     * @return
    */
    API_EXPORT virtual uint32_t GetMtuSize() = 0;

    /**
     * @brief
     * @param DeviceInfos 
     * @return
    */
    API_EXPORT virtual uint32_t GetTimeout(const DeviceInfos &devInfo) = 0;
    
    /**
     * @brief
     * @return
    */
    API_EXPORT virtual DeviceInfos GetLocalDeviceInfos() = 0;

    /**
     * @brief
     * @return
    */
    API_EXPORT virtual std::vector<DeviceInfos> GetRemoteOnlineDeviceInfosList() = 0;
    
    /**
     * @brief
     * @param DeviceInfo
     * @param DeviceChangeType
    */
    API_EXPORT virtual void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const = 0;

    /**
     * @param devId
     * @param SecurityOption
     * @return
    */
    API_EXPORT virtual bool CheckDeviceSecurityAbility(const std::string &devId, const SecurityOption &option) = 0;

    /**
     * @brief 
     * @param PermissionCheckParam
     * @param flag
    */
    API_EXPORT virtual bool SyncPermissionCheck(const PermissionCheckParam &param, uint8_t flag) = 0;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // ENTRY_POINT_H
