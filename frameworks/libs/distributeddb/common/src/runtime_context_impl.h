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

#ifndef RUNTIME_CONTEXT_IMPL_H
#define RUNTIME_CONTEXT_IMPL_H

#include <map>
#include <mutex>
#include <shared_mutex>

#include "auto_launch.h"
#include "db_status_adapter.h"
#include "evloop/src/ievent.h"
#include "evloop/src/ievent_loop.h"
#include "icommunicator_aggregator.h"
#include "lock_status_observer.h"
#include "subscribe_recorder.h"
#include "task_pool.h"
#include "time_tick_monitor.h"
#include "user_change_monitor.h"

namespace DistributedDB {
class RuntimeContextImpl final : public RuntimeContext {
public:
    RuntimeContextImpl();
    ~RuntimeContextImpl() override;

    // Get/Set the label of this process.
    void SetProcessLabel(const std::string &label) override;
    std::string GetProcessLabel() const override;
    int SetCommunicatorAdapter(IAdapter *adapter) override;
    int GetCommunicatorAggregator(ICommunicatorAggregator *&outAggregator) override;
    void SetCommunicatorAggregator(ICommunicatorAggregator *inAggregator) override;
    int GetLocalIdentity(std::string &outTarget) override;
    // Add and start a timer.
    int SetTimer(int milliSeconds, const TimerAction &action,
        const TimerFinalizer &finalizer, TimerId &timerId) override;

    // Modify the interval of the timer.
    int ModifyTimer(TimerId timerId, int milliSeconds) override;

    // Remove the timer.
    void RemoveTimer(TimerId timerId, bool wait) override;

    // Task interfaces.
    int ScheduleTask(const TaskAction &task) override;
    int ScheduleQueuedTask(const std::string &queueTag, const TaskAction &task) override;

    // Shrink as much memory as possible.
    void ShrinkMemory(const std::string &description) override;

    // Register a time changed lister, it will be callback when local time changed.
    NotificationChain::Listener *RegisterTimeChangedLister(const TimeChangedAction &action,
        const TimeFinalizeAction &finalize, int &errCode) override;

    int SetPermissionCheckCallback(const PermissionCheckCallback &callback) override;

    int SetPermissionCheckCallback(const PermissionCheckCallbackV2 &callback) override;

    int SetPermissionCheckCallback(const PermissionCheckCallbackV3 &callback) override;

    int RunPermissionCheck(const PermissionCheckParam &param, uint8_t flag) const override;

    int EnableKvStoreAutoLaunch(const KvDBProperties &properties, AutoLaunchNotifier notifier,
        const AutoLaunchOption &option) override;

    int DisableKvStoreAutoLaunch(const std::string &normalIdentifier, const std::string &dualTupleIdentifier,
        const std::string &userId) override;

    void GetAutoLaunchSyncDevices(const std::string &identifier, std::vector<std::string> &devices) const override;

    void SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback, DBTypeInner type) override;

    NotificationChain::Listener *RegisterLockStatusLister(const LockStatusNotifier &action, int &errCode) override;

    bool IsAccessControlled() const override;

    int SetSecurityOption(const std::string &filePath, const SecurityOption &option) const override;

    int GetSecurityOption(const std::string &filePath, SecurityOption &option) const override;

    bool CheckDeviceSecurityAbility(const std::string &devId, const SecurityOption &option) const override;

    int SetProcessSystemApiAdapter(const std::shared_ptr<IProcessSystemApiAdapter> &adapter) override;

    bool IsProcessSystemApiAdapterValid() const override;

    bool IsCommunicatorAggregatorValid() const override;

    // Notify TIME_CHANGE_EVENT.
    void NotifyTimestampChanged(TimeOffset offset) const override;

    void SetStoreStatusNotifier(const StoreStatusNotifier &notifier) override;

    void NotifyDatabaseStatusChange(const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &deviceId, bool onlineStatus) override;

    int SetSyncActivationCheckCallback(const SyncActivationCheckCallback &callback) override;

    int SetSyncActivationCheckCallback(const SyncActivationCheckCallbackV2 &callback) override;

    bool IsSyncerNeedActive(const DBProperties &properties) const override;

    // Register a user changed lister, it will be callback when user change.
    NotificationChain::Listener *RegisterUserChangedListener(const UserChangedAction &action,
        EventType event) override;
    // Notify TIME_CHANGE_EVENT.
    int NotifyUserChanged() const override;

    uint32_t GenerateSessionId() override;

    void DumpCommonInfo(int fd) override;

    void CloseAutoLaunchConnection(DBTypeInner type, const DBProperties &properties) override;

    int SetPermissionConditionCallback(const PermissionConditionCallback &callback) override;

    std::map<std::string, std::string> GetPermissionCheckParam(const DBProperties &properties) override;

    void StopTaskPool() override;

    void StopTimeTickMonitorIfNeed() override;

    void SetDBInfoHandle(const std::shared_ptr<DBInfoHandle> &handle) override;

    void NotifyDBInfos(const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos) override;

    void RecordRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId, const QuerySyncObject &query) override;

    void RemoveRemoteSubscribe(const DeviceID &deviceId) override;

    void RemoveRemoteSubscribe(const DBInfo &dbInfo) override;

    void RemoveRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId) override;

    void RemoveRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId, const QuerySyncObject &query) override;

    void GetSubscribeQuery(const DBInfo &dbInfo,
        std::map<std::string, std::vector<QuerySyncObject>> &subscribeQuery) override;

    bool IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &devInfo) override;

    void SetRemoteOptimizeCommunication(const std::string &dev, bool optimize) override;

    void SetTranslateToDeviceIdCallback(const TranslateToDeviceIdCallback &callback) override;

    int TranslateDeviceId(const std::string &deviceId,
        const StoreInfo &info, std::string &newDeviceId) override;

    bool ExistTranslateDevIdCallback() const override;

    void SetThreadPool(const std::shared_ptr<IThreadPool> &threadPool) override;

    std::shared_ptr<IThreadPool> GetThreadPool() const override;

    void SetCloudTranslate(const std::shared_ptr<ICloudDataTranslate> &dataTranslate) override;
    int AssetToBlob(const Asset &asset, std::vector<uint8_t> &blob) override;
    int AssetsToBlob(const Assets &assets, std::vector<uint8_t> &blob) override;
    int BlobToAsset(const std::vector<uint8_t> &blob, Asset &asset) override;
    int BlobToAssets(const std::vector<uint8_t> &blob, Assets &assets) override;

    std::pair<int, DeviceTimeInfo> GetDeviceTimeInfo(const std::string &device) const override;
    void SetDeviceTimeInfo(const std::string &device, const DeviceTimeInfo &deviceTimeInfo) override;
    void ClearDeviceTimeInfo(const std::string &device) override;
    void ClearAllDeviceTimeInfo() override;
    void RecordAllTimeChange() override;
    void ResetDBTimeChangeStatus(const std::vector<uint8_t> &dbId) override;
    bool CheckDBTimeChange(const std::vector<uint8_t> &dbId) override;
    bool IsTimeTickMonitorValid() const override;
private:
    static constexpr int MAX_TP_THREADS = 10;  // max threads of the task pool.
    static constexpr int MIN_TP_THREADS = 1;   // min threads of the task pool.
    static constexpr int TASK_POOL_REPORTS_INTERVAL = 10000;   // task pool reports its state every 10 seconds.

    int PrepareLoop(IEventLoop *&loop);
    int PrepareTaskPool();
    int AllocTimerId(IEvent *evTimer, TimerId &timerId);
    std::shared_ptr<DBStatusAdapter> GetDBStatusAdapter();
    std::shared_ptr<SubscribeRecorder> GetSubscribeRecorder();

    int ScheduleTaskByThreadPool(const TaskAction &task) const __attribute__((no_sanitize("cfi")));

    int SetTimerByThreadPool(int milliSeconds, const TimerAction &action,
        const TimerFinalizer &finalizer, bool allocTimerId, TimerId &timerId) __attribute__((no_sanitize("cfi")));

    int ModifyTimerByThreadPool(TimerId timerId, int milliSeconds);

    void RemoveTimerByThreadPool(TimerId timerId, bool wait) __attribute__((no_sanitize("cfi")));

    void ThreadPoolTimerAction(int milliSeconds, const TimerAction &action, TimerId timerId);

    // Context fields
    mutable std::mutex labelMutex_;
    std::string processLabel_;

    // Communicator
    mutable std::mutex communicatorLock_;
    IAdapter *adapter_;
    ICommunicatorAggregator *communicatorAggregator_;

    // Loop and timer
    mutable std::mutex loopLock_;
    IEventLoop *mainLoop_;
    std::mutex timersLock_;
    TimerId currentTimerId_;
    std::map<TimerId, IEvent *> timers_;

    // Task pool
    std::mutex taskLock_;
    TaskPool *taskPool_;
    TimerId taskPoolReportsTimerId_;

    // TimeTick
    mutable std::mutex timeTickMonitorLock_;
    std::unique_ptr<TimeTickMonitor> timeTickMonitor_;

    mutable std::shared_mutex permissionCheckCallbackMutex_ {};
    PermissionCheckCallback permissionCheckCallback_;
    PermissionCheckCallbackV2 permissionCheckCallbackV2_;
    PermissionCheckCallbackV3 permissionCheckCallbackV3_;

    AutoLaunch autoLaunch_;

    // System api
    mutable std::recursive_mutex systemApiAdapterLock_;
    std::shared_ptr<IProcessSystemApiAdapter> systemApiAdapter_;
    mutable std::mutex lockStatusLock_; // Mutex for lockStatusObserver_.
    LockStatusObserver *lockStatusObserver_;

    mutable std::shared_mutex databaseStatusCallbackMutex_ {};
    StoreStatusNotifier databaseStatusNotifyCallback_;

    mutable std::shared_mutex syncActivationCheckCallbackMutex_ {};
    SyncActivationCheckCallback syncActivationCheckCallback_;
    SyncActivationCheckCallbackV2 syncActivationCheckCallbackV2_;

    mutable std::mutex userChangeMonitorLock_;
    std::unique_ptr<UserChangeMonitor> userChangeMonitor_;

    std::atomic<uint32_t> currentSessionId_;

    // Get map from this callback, use for run permission check in remote device
    mutable std::shared_mutex permissionConditionLock_;
    PermissionConditionCallback permissionConditionCallback_;

    mutable std::mutex statusAdapterMutex_;
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter_;

    mutable std::mutex subscribeRecorderMutex_;
    std::shared_ptr<SubscribeRecorder> subscribeRecorder_;

    mutable std::mutex translateToDeviceIdLock_;
    TranslateToDeviceIdCallback translateToDeviceIdCallback_;
    std::map<std::string, std::map<std::string, std::string>> deviceIdCache_; // cache <uuid, <appId, newDeviceId>>

    mutable std::shared_mutex threadPoolLock_;
    std::shared_ptr<IThreadPool> threadPool_;

    mutable std::mutex timerTaskLock_;
    std::map<TimerId, TaskId> taskIds_;
    std::map<TimerId, TimerFinalizer> timerFinalizers_;

    mutable std::shared_mutex dataTranslateLock_;
    std::shared_ptr<ICloudDataTranslate> dataTranslate_;

    mutable std::mutex deviceTimeInfoLock_;
    std::map<std::string, DeviceTimeInfo> deviceTimeInfos_;
    std::map<std::vector<uint8_t>, bool> dbTimeChange_;
};
} // namespace DistributedDB

#endif // RUNTIME_CONTEXT_IMPL_H
