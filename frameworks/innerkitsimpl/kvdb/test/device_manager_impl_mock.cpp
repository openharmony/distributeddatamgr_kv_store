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

#include "device_manager_impl_mock.h"
#include "dm_constants.h"
namespace OHOS {
namespace DistributedHardware {
DeviceManagerImpl &DeviceManagerImpl::GetInstance()
{
    static DeviceManagerImpl instance;
    return instance;
}

int32_t DeviceManagerImpl::InitDeviceManager(const std::string &pkgName,
    std::shared_ptr<DmInitCallback> dmInitCallback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnInitDeviceManager(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    std::vector<DmDeviceInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetTrustedDeviceList(const std::string &pkgName, const std::string &extra,
    bool isRefresh, std::vector<DmDeviceInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetAvailableDeviceList(const std::string &pkgName,
    std::vector<DmDeviceBasicInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &deviceInfo)
{
    if (pkgName == "demo_distributed_data")
    {
        deviceInfo.networkId[0] = 'a';
        return DM_OK;
    }
    if (pkgName == "test_distributed_data")
    {
        return DM_OK;
    }
    return ERR_DM_IPC_SEND_REQUEST_FAILED;
}

int32_t DeviceManagerImpl::GetDeviceInfo(const std::string &pkgName, const std::string networkId,
    DmDeviceInfo &deviceInfo)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RegisterDevStateCallback(const std::string &pkgName, const std::string &extra,
    std::shared_ptr<DeviceStateCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RegisterDevStatusCallback(const std::string &pkgName, const std::string &extra,
    std::shared_ptr<DeviceStatusCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnRegisterDevStateCallback(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnRegisterDevStatusCallback(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::StartDeviceDiscovery(const std::string &pkgName, const DmSubscribeInfo &subscribeInfo,
    const std::string &extra, std::shared_ptr<DiscoveryCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::StartDeviceDiscovery(const std::string &pkgName, uint64_t tokenId,
    const std::string &filterOptions, std::shared_ptr<DiscoveryCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::StopDeviceDiscovery(const std::string &pkgName, uint16_t subscribeId)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::StopDeviceDiscovery(uint64_t tokenId, const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::PublishDeviceDiscovery(const std::string &pkgName, const DmPublishInfo &publishInfo,
    std::shared_ptr<PublishCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnPublishDeviceDiscovery(const std::string &pkgName, int32_t publishId)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::AuthenticateDevice(const std::string &pkgName, int32_t authType,
    const DmDeviceInfo &deviceInfo, const std::string &extra, std::shared_ptr<AuthenticateCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnAuthenticateDevice(const std::string &pkgName, const DmDeviceInfo &deviceInfo)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::VerifyAuthentication(const std::string &pkgName, const std::string &authPara,
    std::shared_ptr<VerifyAuthCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RegisterDeviceManagerFaCallback(const std::string &pkgName,
    std::shared_ptr<DeviceManagerUiCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnRegisterDeviceManagerFaCallback(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetFaParam(const std::string &pkgName, DmAuthParam &faParam)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::SetUserOperation(const std::string &pkgName, int32_t action, const std::string &params)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetUdidByNetworkId(const std::string &pkgName, const std::string &netWorkId,
    std::string &udid)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetUuidByNetworkId(const std::string &pkgName, const std::string &netWorkId,
    std::string &uuid)
{
    uuid = "";
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RegisterDevStateCallback(const std::string &pkgName, const std::string &extra)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnRegisterDevStateCallback(const std::string &pkgName, const std::string &extra)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RequestCredential(const std::string &pkgName, const std::string &reqJsonStr,
    std::string &returnJsonStr)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::ImportCredential(const std::string &pkgName, const std::string &credentialInfo)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::DeleteCredential(const std::string &pkgName, const std::string &deleteInfo)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RegisterCredentialCallback(const std::string &pkgName,
    std::shared_ptr<CredentialCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnRegisterCredentialCallback(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::NotifyEvent(const std::string &pkgName, const int32_t eventId, const std::string &event)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetEncryptedUuidByNetworkId(const std::string &pkgName, const std::string &networkId,
    std::string &uuid)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GenerateEncryptedUuid(const std::string &pkgName, const std::string &uuid,
    const std::string &appId, std::string &encryptedUuid)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::CheckAPIAccessPermission()
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetLocalDeviceNetWorkId(const std::string &pkgName, std::string &networkId)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetLocalDeviceId(const std::string &pkgName, std::string &networkId)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetLocalDeviceName(const std::string &pkgName, std::string &deviceName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetLocalDeviceType(const std::string &pkgName, int32_t &deviceType)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetDeviceName(const std::string &pkgName, const std::string &networkId,
    std::string &deviceName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetDeviceType(const std::string &pkgName, const std::string &networkId, int32_t &deviceType)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::BindDevice(const std::string &pkgName, int32_t bindType, const std::string &deviceId,
    const std::string &extra, std::shared_ptr<AuthenticateCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnBindDevice(const std::string &pkgName, const std::string &deviceId)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::CheckNewAPIAccessPermission()
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetNetworkTypeByNetworkId(const std::string &pkgName,
    const std::string &netWorkId, int32_t &netWorkType)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::ImportAuthCode(const std::string &pkgName, const std::string &authCode)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::ExportAuthCode(std::string &authCode)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::StartDiscovering(const std::string &pkgName,
    std::map<std::string, std::string> &discoverParam, const std::map<std::string, std::string> &filterOptions,
    std::shared_ptr<DistributedHardware::DiscoveryCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::StopDiscovering(const std::string &pkgName,
    std::map<std::string, std::string> &discoverParam)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RegisterDiscoveryCallback(const std::string &pkgName,
    std::map<std::string, std::string> &discoverParam, const std::map<std::string, std::string> &filterOptions,
    std::shared_ptr<DiscoveryCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnRegisterDiscoveryCallback(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::StartAdvertising(const std::string &pkgName,
    std::map<std::string, std::string> &advertiseParam, std::shared_ptr<PublishCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::StopAdvertising(const std::string &pkgName,
    std::map<std::string, std::string> &advertiseParam)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::BindTarget(const std::string &pkgName, const PeerTargetId &targetId,
    std::map<std::string, std::string> &bindParam, std::shared_ptr<BindTargetCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::UnbindTarget(const std::string &pkgName, const PeerTargetId &targetId,
    std::map<std::string, std::string> &unbindParam, std::shared_ptr<UnbindTargetCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetTrustedDeviceList(const std::string &pkgName,
    const std::map<std::string, std::string> &filterOptions, bool isRefresh,
    std::vector<DmDeviceInfo> &deviceList)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RegisterDevStateCallback(const std::string &pkgName,
    const std::map<std::string, std::string> &extraParam, std::shared_ptr<DeviceStateCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::CheckAccessToTarget(uint64_t tokenId, const std::string &targetId)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RegisterPinHolderCallback(const std::string &pkgName,
    std::shared_ptr<PinHolderCallback> callback)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::CreatePinHolder(const std::string &pkgName, const PeerTargetId &targetId,
    DmPinType pinType, const std::string &payload)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::DestroyPinHolder(const std::string &pkgName, const PeerTargetId &targetId,
    DmPinType pinType, const std::string &payload)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::RequestCredential(const std::string &pkgName, std::string &returnJsonStr)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::CheckCredential(const std::string &pkgName, const std::string &reqJsonStr,
    std::string &returnJsonStr)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::ImportCredential(const std::string &pkgName, const std::string &reqJsonStr,
    std::string &returnJsonStr)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::DeleteCredential(const std::string &pkgName, const std::string &reqJsonStr,
    std::string &returnJsonStr)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::DpAclAdd(const int64_t accessControlId, const std::string &udid, const int32_t bindType)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::GetDeviceSecurityLevel(const std::string &pkgName, const std::string &networkId,
    int32_t &securityLevel)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

bool DeviceManagerImpl::IsSameAccount(const std::string &netWorkId)
{
    return true;
}

bool DeviceManagerImpl::CheckAccessControl(const DmAccessCaller &caller, const DmAccessCallee &callee)
{
    return true;
}

bool DeviceManagerImpl::CheckIsSameAccount(const DmAccessCaller &caller, const DmAccessCallee &callee)
{
    return true;
}

int32_t DeviceManagerImpl::GetErrCode(int32_t errCode)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

int32_t DeviceManagerImpl::ShiftLNNGear(const std::string &pkgName)
{
    return ERR_DM_INPUT_PARA_INVALID;
}

} // namespace DistributedHardware
} // namespace OHOS
