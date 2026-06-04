/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "distributed_file_daemon_manager.h"
#include "distributed_file_daemon_manager_impl.h"
#include "remote_file_share.h"
#include "asset/asset_recv_callback_stub.h"
#include "asset/asset_send_callback_stub.h"
#include "asset/asset_obj.h"
#include "dfs_device_info.h"

namespace OHOS {
namespace AppFileService {
namespace ModuleRemoteFileShare {
int32_t RemoteFileShare::GetDfsUrisDirFromLocal(const std::vector<std::string> &uriList, const int32_t &userId,
    std::unordered_map<std::string, HmdfsUriInfo> &uriToDfsUriMaps)
{
    (void)uriList;
    (void)userId;
    (void)uriToDfsUriMaps;
    return 0;
}
} // namespace ModuleRemoteFileShare
} // namespace AppFileService
namespace Storage {
namespace DistributedFile {

int32_t AssetRecvCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    (void)code;
    (void)data;
    (void)reply;
    (void)option;
    return 0;
}
AssetRecvCallbackStub::AssetRecvCallbackStub() {}
int32_t AssetSendCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    (void)code;
    (void)data;
    (void)reply;
    (void)option;
    return 0;
}
AssetSendCallbackStub::AssetSendCallbackStub() {}
DistributedFileDaemonManagerImpl &DistributedFileDaemonManagerImpl::GetInstance()
{
    static DistributedFileDaemonManagerImpl manager;
    return manager;
}
int32_t DistributedFileDaemonManagerImpl::RequestSendFile(const std::string &srcUri, const std::string &dstPath,
    const std::string &remoteDeviceId, const std::string &sessionName)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::OpenP2PConnectionEx(const std::string &networkId,
    sptr<IFileDfsListener> remoteReverseObj)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::CloseP2PConnectionEx(const std::string &networkId)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::PrepareSession(const std::string &srcUri, const std::string &dstUri,
    const std::string &srcDeviceId, const sptr<IRemoteObject> &listener, HmdfsInfo &info)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::CancelCopyTask(const std::string &sessionName)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::GetRemoteCopyInfo(const std::string &srcUri, bool &isFile, bool &isDir)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::PushAsset(int32_t userId, const sptr<AssetObj> &assetObj,
    const sptr<IAssetSendCallback> &sendCallback)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::CancelCopyTask(const std::string &srcUri, const std::string &dstUri)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::GetSize(const std::string &uri, bool isSrcUri, uint64_t &size)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::IsDirectory(const std::string &uri, bool isSrcUri, bool &isDirectory)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::Copy(const std::string &srcUri, const std::string &destUri,
    DistributedFileDaemonManager::ProcessCallback processCallback)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::Cancel(const std::string &srcUri, const std::string &destUri)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::Cancel()
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::GetDfsSwitchStatus(const std::string &networkId, int32_t &switchStatus)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::UpdateDfsSwitchStatus(int32_t switchStatus)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::GetConnectedDeviceList(std::vector<DfsDeviceInfo> &deviceList)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::RegisterFileDfsListener(const std::string &instanceId,
    const sptr<IFileDfsListener> &listener)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::UnregisterFileDfsListener(const std::string &instanceId)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::IsSameAccountDevice(const std::string &networkId, bool &isSameAccount)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::ConnectDfs(const std::string &networkId)
{
    return 0;
}
int32_t DistributedFileDaemonManagerImpl::DisconnectDfs(const std::string &networkId)
{
    return 0;
}

DistributedFileDaemonManager &DistributedFileDaemonManager::GetInstance()
{
    return DistributedFileDaemonManagerImpl::GetInstance();
}
bool AssetObj::Marshalling(Parcel &parcel) const
{
    return true;
}
bool AssetObj::ReadFromParcel(Parcel &parcel)
{
    return true;
}
AssetObj *AssetObj::Unmarshalling(Parcel &parcel)
{
    return nullptr;
}
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS