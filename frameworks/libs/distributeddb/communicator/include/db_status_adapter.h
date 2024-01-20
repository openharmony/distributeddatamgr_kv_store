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

#ifndef DB_STATUS_ADAPTER_H
#define DB_STATUS_ADAPTER_H

#include "db_info_handle.h"
#include <mutex>
#include "macro_utils.h"

namespace DistributedDB {
using RemoteDBChangeCallback = std::function<void(const std::string &devInfo, const std::vector<DBInfo> &dbInfos)>;
using LocalDBChangeCallback = std::function<void(void)>;
using RemoteSupportChangeCallback = std::function<void(const std::string &devInfo)>;
class DBStatusAdapter {
public:
    DBStatusAdapter();
    ~DBStatusAdapter() = default;
    DISABLE_COPY_ASSIGN_MOVE(DBStatusAdapter);

    void SetDBInfoHandle(const std::shared_ptr<DBInfoHandle> &dbInfoHandle);
    bool IsSupport(const std::string &devInfo);
    int GetLocalDBInfos(std::vector<DBInfo> &dbInfos);
    void SetDBStatusChangeCallback(const RemoteDBChangeCallback &remote, const LocalDBChangeCallback &local,
        const RemoteSupportChangeCallback &supportCallback);
    void NotifyDBInfos(const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos);
    void TargetOffline(const std::string &device);
    bool IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &devInfo);
    void SetRemoteOptimizeCommunication(const std::string &dev, bool optimize);
    bool IsSendLabelExchange();
private:
    std::shared_ptr<DBInfoHandle> GetDBInfoHandle() const;
    bool LoadIntoCache(bool isLocal, const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos);
    void ClearAllCache();
    void NotifyRemoteOffline();

    static int GetLocalDeviceId(std::string &deviceId);
    static bool IsLocalDeviceId(const std::string &deviceId);
    static bool MergeDBInfos(const std::vector<DBInfo> &srcDbInfos, std::vector<DBInfo> &dstDbInfos);
    mutable std::mutex handleMutex_;
    std::shared_ptr<DBInfoHandle> dbInfoHandle_ = nullptr;

    mutable std::mutex callbackMutex_;
    RemoteDBChangeCallback remoteCallback_;
    LocalDBChangeCallback localCallback_;
    RemoteSupportChangeCallback supportCallback_;

    mutable std::mutex localInfoMutex_;
    std::vector<DBInfo> localDBInfos_;

    mutable std::mutex remoteInfoMutex_;
    std::map<std::string, std::vector<DBInfo>> remoteDBInfos_; // key: device uuid, value: all db which has notified

    mutable std::mutex supportMutex_;
    bool localSendLabelExchange_;
    bool cacheLocalSendLabelExchange_;
    std::map<std::string, bool> remoteOptimizeInfo_; // key: device uuid, value: is support notified by user
};
}
#endif // DB_STATUS_ADAPTER_H
