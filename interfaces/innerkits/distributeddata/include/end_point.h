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

#ifndef END_POINT_H
#define END_POINT_H

#include <tuple>
#include <map>
#include <mutex>
#include <functional>
#include "types.h"

namespace OHOS {
namespace DistributedKv {
struct StoreBriefInfo {
    std::string userId;
    std::string appId;
    std::string storeId;
    std::string deviceId;
    int32_t instanceId = 0;
    std::map<std::string, std::string> extraConditions;
};

class API_EXPORT Endpoint {
public:

    using RecvHandler = std::function<void(const std::string &identifier, const uint8_t *data, uint32_t length)>;

    /**
     * @brief Constructor.
     */
    API_EXPORT Endpoint() = default;

    /**
     * @brief Destructor.
     */
    API_EXPORT virtual ~Endpoint() {};

    /**
     * @brief Start the Process Communicator.
     * @param processLabel Identifies current process.
     * @return Return SUCCESS for success, others for failure.
     */
    virtual Status Start() = 0;

    /**
     * @brief Start the Process Communicator.
     * @return Return SUCCESS for success, others for failure.
     */
    virtual Status Stop() = 0;

    /**
     * @brief Close all opened kvstores for this appId.
     * @param callback Callback to register data change.
     * @return Return SUCCESS for success, others for failure.
     */
    virtual Status RegOnDataReceive(const RecvHandler &callback) = 0;

    /**
     * @brief Used to send data to the softbus.
     * @param dstDevInfo Target device ID.
     * @param data Pointer to data to be send.
     * @param length Data length.
     * @return Return SUCCESS for success, others for failure.
     */
    virtual Status SendData(const std::string &dtsIdentifier, const uint8_t *data, uint32_t length) = 0;

    /**
     * @brief Obtains the size of maximum unit sent by the bottom layer.
     * @param devInfo Remote device ID.
     * @return Data size.
     */
    virtual uint32_t GetMtuSize(const std::string &identifier) = 0;
    
    /**
     * @brief Obtains the ID of the loacl device info.
     * @return loacl device info.
     */
    virtual std::string GetLocalDeviceInfos() = 0;
    
    /**
     * @brief Determines whether the device has the capability of data of this level.
     * @param devId Target device ID.
     * @param option Security params.
     * @return Return true for success, false for failure.
     */
    virtual bool IsSaferThanDevice(int securityLevel, const std::string &devId) = 0;

    /**
     * @brief Verify sync permission.
     * @param param Params for sync permission.
     * @param flag The direction of sync.
     * @return Return true for success, false for failure.
     */
    virtual bool HasDataSyncPermission(const StoreBriefInfo &param, uint8_t flag) = 0;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // END_POINT_H
