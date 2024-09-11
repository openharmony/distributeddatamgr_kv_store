/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DEVICE_MANAGER_IMPL_H
#define OHOS_DEVICE_MANAGER_IMPL_H

#include "device_manager.h"
#include <map>
#include <mutex>

namespace OHOS {
namespace DistributedHardware {
class DeviceManagerImpl : public DeviceManager {
public:
    static DeviceManagerImpl &GetInstance();

public:

    virtual int32_t InitDeviceManager(const std::string &pkgName,
                                      std::shared_ptr<DmInitCallback> dmInitCallback) override;

    virtual int32_t UnInitDeviceManager(const std::string &pkgName) override;

    virtual int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
                                         std::vector<DmDeviceInfo> &deviceList) override;

    virtual int32_t GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
        bool isRefresh, std::vector<DmDeviceInfo> &deviceList) override;

    virtual int32_t GetAvailableDeviceList(const std::string &pkgName,
    	std::vector<DmDeviceBasicInfo> &deviceList) override;

    virtual int32_t GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &info) override;

    virtual int32_t GetDeviceInfo(const std::string &pkgName, const std::string networkId,
                                  DmDeviceInfo &deviceInfo) override;

    virtual int32_t RegisterDevStateCallback(const std::string &pkgName, const std::string &extra,
                                             std::shared_ptr<DeviceStateCallback> callback) override;

    virtual int32_t RegisterDevStatusCallback(const std::string &pkgName, const std::string &extra,
                                                std::shared_ptr<DeviceStatusCallback> callback) override;

    virtual int32_t UnRegisterDevStateCallback(const std::string &pkgName) override;

    virtual int32_t UnRegisterDevStatusCallback(const std::string &pkgName) override;

    virtual int32_t StartDeviceDiscovery(const std::string &pkgName, const DmSubscribeInfo &subscribeInfo,
                                         const std::string &extra,
                                         std::shared_ptr<DiscoveryCallback> callback) override;

    virtual int32_t StartDeviceDiscovery(const std::string &pkgName, uint64_t tokenId,
                                         const std::string &filterOptions,
                                         std::shared_ptr<DiscoveryCallback> callback) override;

    virtual int32_t StopDeviceDiscovery(const std::string &pkgName, uint16_t subscribeId) override;

    virtual int32_t StopDeviceDiscovery(uint64_t tokenId, const std::string &pkgName) override;

    virtual int32_t PublishDeviceDiscovery(const std::string &pkgName, const DmPublishInfo &publishInfo,
        std::shared_ptr<PublishCallback> callback) override;

    virtual int32_t UnPublishDeviceDiscovery(const std::string &pkgName, int32_t publishId) override;

    virtual int32_t AuthenticateDevice(const std::string &pkgName, int32_t authType, const DmDeviceInfo &deviceInfo,
                                       const std::string &extra,
                                       std::shared_ptr<AuthenticateCallback> callback) override;

    virtual int32_t UnAuthenticateDevice(const std::string &pkgName, const DmDeviceInfo &deviceInfo) override;

    virtual int32_t VerifyAuthentication(const std::string &pkgName, const std::string &authPara,
                                         std::shared_ptr<VerifyAuthCallback> callback) override;

    virtual int32_t RegisterDeviceManagerFaCallback(const std::string &pkgName,
                                                    std::shared_ptr<DeviceManagerUiCallback> callback) override;

    virtual int32_t UnRegisterDeviceManagerFaCallback(const std::string &pkgName) override;

    virtual int32_t GetFaParam(const std::string &pkgName, DmAuthParam &dmFaParam) override;

    virtual int32_t SetUserOperation(const std::string &pkgName, int32_t action, const std::string &params) override;

    virtual int32_t GetUdidByNetworkId(const std::string &pkgName, const std::string &netWorkId,
                                       std::string &udid) override;

    virtual int32_t GetUuidByNetworkId(const std::string &pkgName, const std::string &netWorkId,
                                       std::string &uuid) override;

    virtual int32_t RegisterDevStateCallback(const std::string &pkgName, const std::string &extra) override;

    virtual int32_t UnRegisterDevStateCallback(const std::string &pkgName, const std::string &extra) override;

    virtual int32_t RequestCredential(const std::string &pkgName, const std::string &reqJsonStr,
        std::string &returnJsonStr) override;

    virtual int32_t ImportCredential(const std::string &pkgName, const std::string &credentialInfo) override;

    virtual int32_t RegisterPinHolderCallback(const std::string &pkgName,
        std::shared_ptr<PinHolderCallback> callback) override;

    virtual int32_t CreatePinHolder(const std::string &pkgName, const PeerTargetId &targetId,
        DmPinType pinType, const std::string &payload) override;

    virtual int32_t DestroyPinHolder(const std::string &pkgName, const PeerTargetId &targetId,
        DmPinType pinType, const std::string &payload) override;

    virtual int32_t DeleteCredential(const std::string &pkgName, const std::string &deleteInfo) override;

    virtual int32_t RegisterCredentialCallback(const std::string &pkgName,
        std::shared_ptr<CredentialCallback> callback) override;

    virtual int32_t UnRegisterCredentialCallback(const std::string &pkgName) override;

    virtual int32_t NotifyEvent(const std::string &pkgName, const int32_t eventId, const std::string &event) override;

    virtual int32_t RequestCredential(const std::string &pkgName, std::string &returnJsonStr) override;

    virtual int32_t CheckCredential(const std::string &pkgName, const std::string &reqJsonStr,
                                    std::string &returnJsonStr) override;

    virtual int32_t ImportCredential(const std::string &pkgName, const std::string &reqJsonStr,
                                     std::string &returnJsonStr) override;

    virtual int32_t DeleteCredential(const std::string &pkgName, const std::string &reqJsonStr,
                                     std::string &returnJsonStr) override;

    virtual int32_t GetEncryptedUuidByNetworkId(const std::string &pkgName, const std::string &networkId,
        std::string &uuid) override;

    virtual int32_t GenerateEncryptedUuid(const std::string &pkgName, const std::string &uuid,
        const std::string &appId, std::string &encryptedUuid) override;

    virtual int32_t CheckAPIAccessPermission() override;
    virtual int32_t CheckNewAPIAccessPermission() override;

    int32_t OnDmServiceDied();
    int32_t RegisterUiStateCallback(const std::string &pkgName);
    int32_t UnRegisterUiStateCallback(const std::string &pkgName);

    virtual int32_t GetLocalDeviceNetWorkId(const std::string &pkgName, std::string &networkId) override;
    virtual int32_t GetLocalDeviceId(const std::string &pkgName, std::string &networkId) override;
    virtual int32_t GetLocalDeviceType(const std::string &pkgName, int32_t &deviceType) override;
    virtual int32_t GetLocalDeviceName(const std::string &pkgName, std::string &deviceName) override;
    virtual int32_t GetDeviceName(const std::string &pkgName, const std::string &networkId,
        std::string &deviceName) override;
    virtual int32_t GetDeviceType(const std::string &pkgName,
        const std::string &networkId, int32_t &deviceType) override;
    virtual int32_t BindDevice(const std::string &pkgName, int32_t bindType, const std::string &deviceId,
        const std::string &extra, std::shared_ptr<AuthenticateCallback> callback) override;
    virtual int32_t UnBindDevice(const std::string &pkgName, const std::string &deviceId) override;
    virtual int32_t GetNetworkTypeByNetworkId(const std::string &pkgName, const std::string &netWorkId,
                                       int32_t &netWorkType) override;
    virtual int32_t ImportAuthCode(const std::string &pkgName, const std::string &authCode) override;
    virtual int32_t ExportAuthCode(std::string &authCode) override;

    // The following interfaces are provided since OpenHarmony 4.1 Version.
    virtual int32_t StartDiscovering(const std::string &pkgName, std::map<std::string, std::string> &discoverParam,
        const std::map<std::string, std::string> &filterOptions, std::shared_ptr<DiscoveryCallback> callback) override;

    virtual int32_t StopDiscovering(const std::string &pkgName,
        std::map<std::string, std::string> &discoverParam) override;

    virtual int32_t RegisterDiscoveryCallback(const std::string &pkgName,
        std::map<std::string, std::string> &discoverParam, const std::map<std::string, std::string> &filterOptions,
        std::shared_ptr<DiscoveryCallback> callback) override;

    virtual int32_t UnRegisterDiscoveryCallback(const std::string &pkgName) override;

    virtual int32_t StartAdvertising(const std::string &pkgName, std::map<std::string, std::string> &advertiseParam,
        std::shared_ptr<PublishCallback> callback) override;

    virtual int32_t StopAdvertising(const std::string &pkgName,
        std::map<std::string, std::string> &advertiseParam) override;

    virtual int32_t BindTarget(const std::string &pkgName, const PeerTargetId &targetId,
        std::map<std::string, std::string> &bindParam, std::shared_ptr<BindTargetCallback> callback) override;

    virtual int32_t UnbindTarget(const std::string &pkgName, const PeerTargetId &targetId,
        std::map<std::string, std::string> &unbindParam, std::shared_ptr<UnbindTargetCallback> callback) override;

    virtual int32_t GetTrustedDeviceList(const std::string &pkgName,
        const std::map<std::string, std::string> &filterOptions, bool isRefresh,
        std::vector<DmDeviceInfo> &deviceList) override;

    virtual int32_t RegisterDevStateCallback(const std::string &pkgName,
        const std::map<std::string, std::string> &extraParam,
        std::shared_ptr<DeviceStateCallback> callback) override;

    virtual int32_t CheckAccessToTarget(uint64_t tokenId, const std::string &targetId) override;

    virtual int32_t DpAclAdd(const int64_t accessControlId, const std::string &udid, const int32_t bindType) override;

    virtual int32_t GetDeviceSecurityLevel(const std::string &pkgName, const std::string &networkId,
                                           int32_t &securityLevel) override;

    bool IsSameAccount(const std::string &netWorkId) override;
    bool CheckAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee) override;
    bool CheckIsSameAccount(const DmAccessCaller &caller, const DmAccessCallee &callee) override;
    virtual int32_t GetErrCode(int32_t errCode) override;
    virtual int32_t ShiftLNNGear(const std::string &pkgName) override;
    virtual int32_t RegDevTrustChangeCallback(const std::string &pkgName,
        std::shared_ptr<DevTrustChangeCallback> callback) override;

    virtual int32_t SetDnPolicy(const std::string &pkgName, std::map<std::string, std::string> &policy) override;

private:
    DeviceManagerImpl() = default;
    ~DeviceManagerImpl() = default;
    DeviceManagerImpl(const DeviceManagerImpl &) = delete;
    DeviceManagerImpl &operator=(const DeviceManagerImpl &) = delete;
    DeviceManagerImpl(DeviceManagerImpl &&) = delete;
    DeviceManagerImpl &operator=(DeviceManagerImpl &&) = delete;

    uint16_t AddDiscoveryCallback(const std::string &pkgName, std::shared_ptr<DiscoveryCallback> callback);
    uint16_t RemoveDiscoveryCallback(const std::string &pkgName);
    int32_t AddPublishCallback(const std::string &pkgName);
    int32_t RemovePublishCallback(const std::string &pkgName);
    int32_t CheckApiPermission(int32_t permissionLevel);
    void ConvertDeviceInfoToDeviceBasicInfo(const DmDeviceInfo &info, DmDeviceBasicInfo &deviceBasicInfo);

private:
    std::mutex subscribIdLock;
    std::map<uint64_t, uint16_t> subscribIdMap_;

    std::mutex subMapLock;
    std::map<std::string, uint16_t> pkgName2SubIdMap_;

    std::mutex pubMapLock;
    std::map<std::string, uint16_t> pkgName2PubIdMap_;
};
} // namespace DistributedHardware
} // namespace OHOS
#endif // OHOS_DEVICE_MANAGER_IMPL_H
