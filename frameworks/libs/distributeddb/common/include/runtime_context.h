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

#ifndef RUNTIME_CONTEXT_H
#define RUNTIME_CONTEXT_H

#include <cstdint>
#include <functional>
#include <string>

#include "auto_launch.h"
#include "auto_launch_export.h"
#include "cloud/assets_download_manager.h"
#include "cloud/icloud_data_translate.h"
#include "db_info_handle.h"
#include "icommunicator_aggregator.h"
#include "iprocess_system_api_adapter.h"
#include "ithread_pool.h"
#include "kv_store_observer.h"
#include "kvdb_properties.h"
#include "macro_utils.h"
#include "notification_chain.h"
#include "query_sync_object.h"
#include "types_export.h"

namespace DistributedDB {
using TimerId = uint64_t;
using TimerAction = std::function<int(TimerId timerId)>;
using TimerFinalizer = std::function<void(void)>;
using TaskAction = std::function<void(void)>;
using TimeChangedAction = std::function<void(void *)>;
using TimeFinalizeAction = std::function<void(void)>;
using LockStatusNotifier = std::function<void(void *isLocked)>;
using UserChangedAction = std::function<void(void *)>;

class RuntimeContext {
public:
    DISABLE_COPY_ASSIGN_MOVE(RuntimeContext);

    // Global setting interfaces.
    virtual void SetProcessLabel(const std::string &label) = 0;
    virtual std::string GetProcessLabel() const = 0;

    // If the pre adapter is not nullptr, set new adapter will release the pre adapter,
    // must be called after SetCommunicatorAggregator
    virtual int SetCommunicatorAdapter(IAdapter *adapter) = 0;
    virtual int GetCommunicatorAggregator(ICommunicatorAggregator *&outAggregator) = 0;
    virtual void SetCommunicatorAggregator(ICommunicatorAggregator *inAggregator) = 0;
    virtual int GetLocalIdentity(std::string &outTarget) = 0;

    // Timer interfaces.
    virtual int SetTimer(int milliSeconds, const TimerAction &action,
        const TimerFinalizer &finalizer, TimerId &timerId) = 0;
    virtual int ModifyTimer(TimerId timerId, int milliSeconds) = 0;
    virtual void RemoveTimer(TimerId timerId, bool wait = false) = 0;

    // Task interfaces.
    virtual int ScheduleTask(const TaskAction &task) = 0;
    virtual int ScheduleQueuedTask(const std::string &queueTag,
        const TaskAction &task) = 0;

    // Shrink as much memory as possible.
    virtual void ShrinkMemory(const std::string &description) = 0;

    // Register a time changed lister, it will be callback when local time changed.
    virtual NotificationChain::Listener *RegisterTimeChangedLister(const TimeChangedAction &action,
        const TimeFinalizeAction &finalize, int &errCode) = 0;

    // Get the global context object(singleton), never return nullptr.
    static RuntimeContext *GetInstance();

    virtual int SetPermissionCheckCallback(const PermissionCheckCallback &callback) = 0;

    virtual int SetPermissionCheckCallback(const PermissionCheckCallbackV2 &callback) = 0;

    virtual int SetPermissionCheckCallback(const PermissionCheckCallbackV3 &callback) = 0;

    virtual int SetPermissionCheckCallback(const PermissionCheckCallbackV4 &callback) = 0;

    virtual int RunPermissionCheck(const PermissionCheckParam &param, uint8_t flag) const = 0;

    virtual int EnableKvStoreAutoLaunch(const KvDBProperties &properties, AutoLaunchNotifier notifier,
        const AutoLaunchOption &option) = 0;

    virtual int DisableKvStoreAutoLaunch(const std::string &normalIdentifier, const std::string &dualTupleIdentifier,
        const std::string &userId) = 0;

    virtual void GetAutoLaunchSyncDevices(const std::string &identifier, std::vector<std::string> &devices) const = 0;

    virtual void SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback, DBTypeInner type) = 0;

    virtual NotificationChain::Listener *RegisterLockStatusLister(const LockStatusNotifier &action, int &errCode) = 0;

    virtual bool IsAccessControlled() const = 0;

    virtual int SetSecurityOption(const std::string &filePath, const SecurityOption &option) const = 0;

    virtual int GetSecurityOption(const std::string &filePath, SecurityOption &option) const = 0;

    virtual bool CheckDeviceSecurityAbility(const std::string &devId, const SecurityOption &option) const = 0;

    virtual int SetProcessSystemApiAdapter(const std::shared_ptr<IProcessSystemApiAdapter> &adapter) = 0;

    virtual bool IsProcessSystemApiAdapterValid() const = 0;

    virtual bool IsCommunicatorAggregatorValid() const = 0;

    // Notify TIME_CHANGE_EVENT.
    virtual void NotifyTimestampChanged(TimeOffset offset) const = 0;

    virtual void SetStoreStatusNotifier(const StoreStatusNotifier &notifier) = 0;

    virtual void SetStoreStatusNotifier(const StoreStatusNotifierV2 &notifier) = 0;

    virtual void NotifyDatabaseStatusChange(const StoreStatusNotifierParam &param, bool onlineStatus) = 0;

    virtual int SetSyncActivationCheckCallback(const SyncActivationCheckCallback &callback) = 0;

    virtual int SetSyncActivationCheckCallback(const SyncActivationCheckCallbackV2 &callback) = 0;

    virtual bool IsSyncerNeedActive(const DBProperties &properties) const = 0;

    virtual NotificationChain::Listener *RegisterUserChangedListener(const UserChangedAction &action,
        EventType event) = 0;

    virtual int NotifyUserChanged() const = 0;

    // Generate global sessionId in current process
    virtual uint32_t GenerateSessionId() = 0;

    virtual void DumpCommonInfo(int fd) = 0;

    virtual void CloseAutoLaunchConnection(DBTypeInner type, const DBProperties &properties) = 0;

    virtual int SetPermissionConditionCallback(const PermissionConditionCallback &callback) = 0;

    virtual std::map<std::string, std::string> GetPermissionCheckParam(const DBProperties &properties) = 0;

    virtual void StopTaskPool() = 0;

    virtual void StopTimeTickMonitorIfNeed() = 0;

    virtual void SetDBInfoHandle(const std::shared_ptr<DBInfoHandle> &handle) = 0;

    virtual void NotifyDBInfos(const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos) = 0;

    virtual void RecordRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId,
        const QuerySyncObject &query) = 0;

    virtual void RemoveRemoteSubscribe(const DeviceID &deviceId) = 0;

    virtual void RemoveRemoteSubscribe(const DBInfo &dbInfo) = 0;

    virtual void RemoveRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId) = 0;

    virtual void RemoveRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId,
        const QuerySyncObject &query) = 0;

    virtual void GetSubscribeQuery(const DBInfo &dbInfo,
        std::map<std::string, std::vector<QuerySyncObject>> &subscribeQuery) = 0;

    virtual bool IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &devInfo) = 0;

    virtual void SetRemoteOptimizeCommunication(const std::string &dev, bool optimize) = 0;

    virtual void SetTranslateToDeviceIdCallback(const TranslateToDeviceIdCallback &callback) = 0;

    virtual int TranslateDeviceId(const std::string &deviceId,
        const StoreInfo &info, std::string &newDeviceId) = 0;

    virtual bool ExistTranslateDevIdCallback() const = 0;

    virtual void SetThreadPool(const std::shared_ptr<IThreadPool> &threadPool) = 0;

    virtual std::shared_ptr<IThreadPool> GetThreadPool() const = 0;

    virtual void SetCloudTranslate(const std::shared_ptr<ICloudDataTranslate> &dataTranslate) = 0;
    virtual int AssetToBlob(const Asset &asset, std::vector<uint8_t> &blob) = 0;
    virtual int AssetsToBlob(const Assets &assets, std::vector<uint8_t> &blob) = 0;
    virtual int BlobToAsset(const std::vector<uint8_t> &blob, Asset &asset) = 0;
    virtual int BlobToAssets(const std::vector<uint8_t> &blob, Assets &assets) = 0;

    virtual std::pair<int, DeviceTimeInfo> GetDeviceTimeInfo(const std::string &device) const = 0;
    virtual void SetDeviceTimeInfo(const std::string &device, const DeviceTimeInfo &deviceTimeInfo) = 0;
    virtual void ClearDeviceTimeInfo(const std::string &device) = 0;
    virtual void ClearAllDeviceTimeInfo() = 0;
    virtual void RecordAllTimeChange() = 0;
    virtual void ResetDBTimeChangeStatus(const std::vector<uint8_t> &dbId) = 0;
    virtual bool CheckDBTimeChange(const std::vector<uint8_t> &dbId) = 0;
    virtual bool IsTimeTickMonitorValid() const = 0;
    virtual bool IsTimeChanged() const = 0;
    virtual void SetTimeChanged(bool timeChange) = 0;

    virtual bool IsBatchDownloadAssets() const = 0;
    virtual void SetBatchDownloadAssets(bool isBatchDownload) = 0;

    virtual std::shared_ptr<AssetsDownloadManager> GetAssetsDownloadManager() = 0;

    virtual void ClearOnlineLabel() = 0;
protected:
    RuntimeContext() = default;
    virtual ~RuntimeContext() {}
};
} // namespace DistributedDB

#endif // RUNTIME_CONTEXT_H
