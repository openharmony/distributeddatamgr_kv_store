/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_FILE_DAEMON_MANAGER_IMPL_H
#define DISTRIBUTED_FILE_DAEMON_MANAGER_IMPL_H

#include "asset/asset_obj.h"
#include "asset/i_asset_recv_callback.h"
#include "asset/i_asset_send_callback.h"
#ifdef DFS_ENABLE_DISTRIBUTED_ABILITY
#include "dm_device_info.h"
#endif
#include "ipc/distributed_file_daemon_manager.h"
#include "ipc/i_file_dfs_listener.h"
#include "nocopyable.h"
#include "refbase.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
constexpr int32_t MAX_CONCURRENT_NUM = 3;
class DistributedFileDaemonManagerImpl final : public DistributedFileDaemonManager, public NoCopyable {
public:
    static DistributedFileDaemonManagerImpl &GetInstance();

    int32_t ConnectDfs(const std::string &networkId) override;
    int32_t DisconnectDfs(const std::string &networkId) override;
    int32_t OpenP2PConnectionEx(const std::string &networkId, sptr<IFileDfsListener> remoteReverseObj) override;
    int32_t CloseP2PConnectionEx(const std::string &networkId) override;
    int32_t PrepareSession(const std::string &srcUri,
                           const std::string &dstUri,
                           const std::string &srcDeviceId,
                           const sptr<IRemoteObject> &listener,
                           HmdfsInfo &info) override;
    int32_t CancelCopyTask(const std::string &sessionName) override;
    int32_t CancelCopyTask(const std::string &srcUri, const std::string &dstUri) override;
    int32_t RequestSendFile(const std::string &srcUri,
                            const std::string &dstPath,
                            const std::string &remoteDeviceId,
                            const std::string &sessionName);
    int32_t GetRemoteCopyInfo(const std::string &srcUri, bool &isFile, bool &isDir);

    int32_t PushAsset(int32_t userId,
                      const sptr<AssetObj> &assetObj,
                      const sptr<IAssetSendCallback> &sendCallback) override;
    int32_t RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) override;
    int32_t UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) override;

    int32_t GetSize(const std::string &uri, bool isSrcUri, uint64_t &size) override;
    int32_t IsDirectory(const std::string &uri, bool isSrcUri, bool &isDirectory) override;
    int32_t Copy(const std::string &srcUri, const std::string &destUri, ProcessCallback processCallback) override;
    int32_t Cancel(const std::string &srcUri, const std::string &destUri) override;
    int32_t Cancel() override;
    int32_t GetDfsSwitchStatus(const std::string &networkId, int32_t &switchStatus) override;
    int32_t UpdateDfsSwitchStatus(int32_t switchStatus) override;
    int32_t GetConnectedDeviceList(std::vector<DfsDeviceInfo> &deviceList) override;
    int32_t RegisterFileDfsListener(const std::string &instanceId, const sptr<IFileDfsListener> &listener) override;
    int32_t UnregisterFileDfsListener(const std::string &instanceId) override;
    int32_t IsSameAccountDevice(const std::string &networkId, bool &isSameAccount) override;
private:
    DistributedFileDaemonManagerImpl() = default;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS

#endif // DISTRIBUTED_FILE_DAEMON_MANAGER_IMPL_H
