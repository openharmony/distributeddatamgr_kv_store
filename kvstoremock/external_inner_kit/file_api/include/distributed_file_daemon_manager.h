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

#ifndef DISTRIBUTED_FILE_DAEMON_MANAGER_H
#define DISTRIBUTED_FILE_DAEMON_MANAGER_H

#include <functional>
#include <memory>

#include "asset/asset_obj.h"
#include "asset/i_asset_recv_callback.h"
#include "asset/i_asset_send_callback.h"
#include "dfs_device_info.h"
#ifdef DFS_ENABLE_DISTRIBUTED_ABILITY
#include "dm_device_info.h"
#endif
#include "hmdfs_info.h"
#include "i_file_dfs_listener.h"
#include "i_file_trans_listener.h"

namespace OHOS {
namespace Storage {
namespace DistributedFile {
class DistributedFileDaemonManager {
public:
    using ProcessCallback = std::function<void (uint64_t processSize, uint64_t totalSize)>;
public:
    static DistributedFileDaemonManager &GetInstance();

    virtual int32_t ConnectDfs(const std::string &networkId) = 0;
    virtual int32_t DisconnectDfs(const std::string &networkId) = 0;
    virtual int32_t OpenP2PConnectionEx(const std::string &networkId, sptr<IFileDfsListener> remoteReverseObj) = 0;
    virtual int32_t CloseP2PConnectionEx(const std::string &networkId) = 0;
    virtual int32_t PrepareSession(const std::string &srcUri,
                                   const std::string &dstUri,
                                   const std::string &srcDeviceId,
                                   const sptr<IRemoteObject> &listener,
                                   HmdfsInfo &info) = 0;
    virtual int32_t CancelCopyTask(const std::string &sessionName) = 0;
    virtual int32_t CancelCopyTask(const std::string &srcUri, const std::string &dstUri) = 0;

    virtual int32_t PushAsset(int32_t userId,
                              const sptr<AssetObj> &assetObj,
                              const sptr<IAssetSendCallback> &sendCallback) = 0;
    virtual int32_t RegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) = 0;
    virtual int32_t UnRegisterAssetCallback(const sptr<IAssetRecvCallback> &recvCallback) =0;

    virtual int32_t GetSize(const std::string &uri, bool isSrcUri, uint64_t &size) = 0;
    virtual int32_t IsDirectory(const std::string &uri, bool isSrcUri, bool &isDirectory) = 0;
    virtual int32_t Copy(const std::string &srcUri, const std::string &destUri, ProcessCallback processCallback) = 0;
    virtual int32_t Cancel(const std::string &srcUri, const std::string &destUri) = 0;
    virtual int32_t Cancel() = 0;

    // distributed filemanager&picker
    virtual int32_t GetDfsSwitchStatus(const std::string &networkId, int32_t &switchStatus) = 0;
    virtual int32_t UpdateDfsSwitchStatus(int32_t switchStatus) = 0;
    virtual int32_t GetConnectedDeviceList(std::vector<DfsDeviceInfo> &deviceList) = 0;
    virtual int32_t RegisterFileDfsListener(const std::string &instanceId, const sptr<IFileDfsListener> &listener) = 0;
    virtual int32_t UnregisterFileDfsListener(const std::string &instanceId) = 0;
    virtual int32_t IsSameAccountDevice(const std::string &networkId, bool &isSameAccount) = 0;
};
} // namespace DistributedFile
} // namespace Storage
} // namespace OHOS

#endif // DISTRIBUTED_FILE_DAEMON_MANAGER_H