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

#include "runtime_context_impl.h"
#include "db_common.h"
#include "db_errno.h"
#include "db_dfx_adapter.h"
#include "log_print.h"
#include "communicator_aggregator.h"
#include "network_adapter.h"

namespace DistributedDB {
#ifdef RUNNING_ON_TESTCASE
static std::atomic_uint taskID = 0;
#endif

RuntimeContextImpl::RuntimeContextImpl()
    : adapter_(nullptr),
      communicatorAggregator_(nullptr),
      mainLoop_(nullptr),
      currentTimerId_(0),
      taskPool_(nullptr),
      taskPoolReportsTimerId_(0),
      timeTickMonitor_(nullptr),
      systemApiAdapter_(nullptr),
      lockStatusObserver_(nullptr),
      currentSessionId_(1),
      dbStatusAdapter_(nullptr),
      subscribeRecorder_(nullptr)
{
}

// Destruct the object.
RuntimeContextImpl::~RuntimeContextImpl()
{
    if (taskPoolReportsTimerId_ > 0) {
        RemoveTimer(taskPoolReportsTimerId_, true);
        taskPoolReportsTimerId_ = 0;
    }
    if (taskPool_ != nullptr) {
        taskPool_->Stop();
        taskPool_->Release(taskPool_);
        taskPool_ = nullptr;
    }
    if (mainLoop_ != nullptr) {
        mainLoop_->Stop();
        mainLoop_->KillAndDecObjRef(mainLoop_);
        mainLoop_ = nullptr;
    }
    SetCommunicatorAggregator(nullptr);
    (void)SetCommunicatorAdapter(nullptr);
    systemApiAdapter_ = nullptr;
    delete lockStatusObserver_;
    lockStatusObserver_ = nullptr;
    userChangeMonitor_ = nullptr;
    dbStatusAdapter_ = nullptr;
    subscribeRecorder_ = nullptr;
    SetThreadPool(nullptr);
}

// Set the label of this process.
void RuntimeContextImpl::SetProcessLabel(const std::string &label)
{
    std::lock_guard<std::mutex> labelLock(labelMutex_);
    processLabel_ = label;
}

std::string RuntimeContextImpl::GetProcessLabel() const
{
    std::lock_guard<std::mutex> labelLock(labelMutex_);
    return processLabel_;
}

int RuntimeContextImpl::SetCommunicatorAdapter(IAdapter *adapter)
{
    {
        std::lock_guard<std::mutex> autoLock(communicatorLock_);
        if (adapter_ != nullptr) {
            if (communicatorAggregator_ != nullptr) {
                return -E_NOT_SUPPORT;
            }
            delete adapter_;
        }
        adapter_ = adapter;
    }
    ICommunicatorAggregator *communicatorAggregator = nullptr;
    GetCommunicatorAggregator(communicatorAggregator);
    autoLaunch_.SetCommunicatorAggregator(communicatorAggregator);
    return E_OK;
}

int RuntimeContextImpl::GetCommunicatorAggregator(ICommunicatorAggregator *&outAggregator)
{
    outAggregator = nullptr;
    const std::shared_ptr<DBStatusAdapter> statusAdapter = GetDBStatusAdapter();
    std::lock_guard<std::mutex> lock(communicatorLock_);
    if (communicatorAggregator_ != nullptr) {
        outAggregator = communicatorAggregator_;
        return E_OK;
    }

    if (adapter_ == nullptr) {
        LOGE("Adapter has not set!");
        return -E_NOT_INIT;
    }

    communicatorAggregator_ = new (std::nothrow) CommunicatorAggregator;
    if (communicatorAggregator_ == nullptr) {
        LOGE("CommunicatorAggregator create failed, may be no available memory!");
        return -E_OUT_OF_MEMORY;
    }

    int errCode = communicatorAggregator_->Initialize(adapter_, statusAdapter);
    if (errCode != E_OK) {
        LOGE("CommunicatorAggregator init failed, err = %d!", errCode);
        RefObject::KillAndDecObjRef(communicatorAggregator_);
        communicatorAggregator_ = nullptr;
    }
    outAggregator = communicatorAggregator_;
    return errCode;
}

void RuntimeContextImpl::SetCommunicatorAggregator(ICommunicatorAggregator *inAggregator)
{
    std::lock_guard<std::mutex> autoLock(communicatorLock_);
    if (communicatorAggregator_ != nullptr) {
        autoLaunch_.SetCommunicatorAggregator(nullptr);
        communicatorAggregator_->Finalize();
        RefObject::KillAndDecObjRef(communicatorAggregator_);
    }
    communicatorAggregator_ = inAggregator;
    autoLaunch_.SetCommunicatorAggregator(communicatorAggregator_);
}

int RuntimeContextImpl::GetLocalIdentity(std::string &outTarget)
{
    std::lock_guard<std::mutex> autoLock(communicatorLock_);
    if (communicatorAggregator_ != nullptr) {
        return communicatorAggregator_->GetLocalIdentity(outTarget);
    }
    return -E_NOT_INIT;
}

// Add and start a timer.
int RuntimeContextImpl::SetTimer(int milliSeconds, const TimerAction &action,
    const TimerFinalizer &finalizer, TimerId &timerId)
{
    timerId = 0;
    if ((milliSeconds < 0) || !action) {
        return -E_INVALID_ARGS;
    }
    int errCode = SetTimerByThreadPool(milliSeconds, action, finalizer, true, timerId);
    if (errCode != -E_NOT_SUPPORT) {
        return errCode;
    }
    IEventLoop *loop = nullptr;
    errCode = PrepareLoop(loop);
    if (errCode != E_OK) {
        LOGE("SetTimer(), prepare loop failed.");
        return errCode;
    }

    IEvent *evTimer = IEvent::CreateEvent(milliSeconds, errCode);
    if (evTimer == nullptr) {
        loop->DecObjRef(loop);
        loop = nullptr;
        return errCode;
    }

    errCode = AllocTimerId(evTimer, timerId);
    if (errCode != E_OK) {
        evTimer->DecObjRef(evTimer);
        evTimer = nullptr;
        loop->DecObjRef(loop);
        loop = nullptr;
        return errCode;
    }

    evTimer->SetAction([this, timerId, action](EventsMask revents) -> int {
            int errCodeInner = action(timerId);
            if (errCodeInner != E_OK) {
                RemoveTimer(timerId, false);
            }
            return errCodeInner;
        },
        finalizer);

    errCode = loop->Add(evTimer);
    if (errCode != E_OK) {
        evTimer->IgnoreFinalizer();
        RemoveTimer(timerId, false);
        timerId = 0;
    }

    loop->DecObjRef(loop);
    loop = nullptr;
    return errCode;
}

// Modify the interval of the timer.
int RuntimeContextImpl::ModifyTimer(TimerId timerId, int milliSeconds)
{
    if (milliSeconds < 0) {
        return -E_INVALID_ARGS;
    }
    int errCode = ModifyTimerByThreadPool(timerId, milliSeconds);
    if (errCode != -E_NOT_SUPPORT) {
        return errCode;
    }
    std::lock_guard<std::mutex> autoLock(timersLock_);
    auto iter = timers_.find(timerId);
    if (iter == timers_.end()) {
        return -E_NO_SUCH_ENTRY;
    }

    IEvent *evTimer = iter->second;
    if (evTimer == nullptr) {
        return -E_INTERNAL_ERROR;
    }
    return evTimer->SetTimeout(milliSeconds);
}

// Remove the timer.
void RuntimeContextImpl::RemoveTimer(TimerId timerId, bool wait)
{
    RemoveTimerByThreadPool(timerId, wait);
    IEvent *evTimer = nullptr;
    {
        std::lock_guard<std::mutex> autoLock(timersLock_);
        auto iter = timers_.find(timerId);
        if (iter == timers_.end()) {
            return;
        }
        evTimer = iter->second;
        timers_.erase(iter);
    }

    if (evTimer != nullptr) {
        evTimer->Detach(wait);
        evTimer->DecObjRef(evTimer);
        evTimer = nullptr;
    }
}

// Task interfaces.
int RuntimeContextImpl::ScheduleTask(const TaskAction &task)
{
    if (ScheduleTaskByThreadPool(task) == E_OK) {
        return E_OK;
    }
    std::lock_guard<std::mutex> autoLock(taskLock_);
    int errCode = PrepareTaskPool();
    if (errCode != E_OK) {
        LOGE("Schedule task failed, fail to prepare task pool.");
        return errCode;
    }
#ifdef RUNNING_ON_TESTCASE
    auto id = taskID++;
    LOGI("Schedule task succeed, ID:%u", id);
    return taskPool_->Schedule([task, id] {
        LOGI("Execute task, ID:%u", id);
        task();
    });
#else
    return taskPool_->Schedule(task);
#endif
}

int RuntimeContextImpl::ScheduleQueuedTask(const std::string &queueTag,
    const TaskAction &task)
{
    if (ScheduleTaskByThreadPool(task) == E_OK) {
        return E_OK;
    }
    std::lock_guard<std::mutex> autoLock(taskLock_);
    int errCode = PrepareTaskPool();
    if (errCode != E_OK) {
        LOGE("Schedule queued task failed, fail to prepare task pool.");
        return errCode;
    }
#ifdef RUNNING_ON_TESTCASE
    auto id = taskID++;
    LOGI("Schedule queued task succeed, ID:%u", id);
    return taskPool_->Schedule(queueTag, [task, id] {
        LOGI("Execute queued task, ID:%u", id);
        task();
    });
#else
    return taskPool_->Schedule(queueTag, task);
#endif
}

void RuntimeContextImpl::ShrinkMemory(const std::string &description)
{
    std::lock_guard<std::mutex> autoLock(taskLock_);
    if (taskPool_ != nullptr) {
        taskPool_->ShrinkMemory(description);
    }
}

NotificationChain::Listener *RuntimeContextImpl::RegisterTimeChangedLister(const TimeChangedAction &action,
    const TimeFinalizeAction &finalize, int &errCode)
{
    std::lock_guard<std::mutex> autoLock(timeTickMonitorLock_);
    if (timeTickMonitor_ == nullptr) {
        timeTickMonitor_ = std::make_unique<TimeTickMonitor>();
        errCode = timeTickMonitor_->StartTimeTickMonitor();
        if (errCode != E_OK) {
            LOGE("TimeTickMonitor start failed!");
            timeTickMonitor_ = nullptr;
            return nullptr;
        }
        LOGD("[RuntimeContext] TimeTickMonitor start success");
    }
    LOGD("[RuntimeContext] call RegisterTimeChangedLister");
    return timeTickMonitor_->RegisterTimeChangedLister(action, finalize, errCode);
}

int RuntimeContextImpl::PrepareLoop(IEventLoop *&loop)
{
    std::lock_guard<std::mutex> autoLock(loopLock_);
    if (mainLoop_ != nullptr) {
        loop = mainLoop_;
        loop->IncObjRef(loop); // ref 1 returned to caller.
        return E_OK;
    }

    int errCode = E_OK;
    loop = IEventLoop::CreateEventLoop(errCode);
    if (loop == nullptr) {
        return errCode;
    }

    loop->IncObjRef(loop); // ref 1 owned by thread.
    std::thread loopThread([loop]() {
            loop->Run();
            loop->DecObjRef(loop); // ref 1 dropped by thread.
        });
    loopThread.detach();

    mainLoop_ = loop;
    loop->IncObjRef(loop); // ref 1 returned to caller.
    return E_OK;
}

int RuntimeContextImpl::PrepareTaskPool()
{
    if (taskPool_ != nullptr) {
        return E_OK;
    }

    int errCode = E_OK;
    TaskPool *taskPool = TaskPool::Create(MAX_TP_THREADS, MIN_TP_THREADS, errCode);
    if (taskPool == nullptr) {
        return errCode;
    }

    errCode = taskPool->Start();
    if (errCode != E_OK) {
        taskPool->Release(taskPool);
        return errCode;
    }

    taskPool_ = taskPool;
    return E_OK;
}

int RuntimeContextImpl::AllocTimerId(IEvent *evTimer, TimerId &timerId)
{
    std::lock_guard<std::mutex> autoLock(timersLock_);
    TimerId startId = currentTimerId_;
    while (++currentTimerId_ != startId) {
        if (currentTimerId_ == 0) {
            continue;
        }
        if (timers_.find(currentTimerId_) == timers_.end()) {
            timerId = currentTimerId_;
            timers_[timerId] = evTimer;
            return E_OK;
        }
    }
    return -E_OUT_OF_IDS;
}

int RuntimeContextImpl::SetPermissionCheckCallback(const PermissionCheckCallback &callback)
{
    std::unique_lock<std::shared_mutex> writeLock(permissionCheckCallbackMutex_);
    permissionCheckCallback_ = callback;
    LOGI("SetPermissionCheckCallback ok");
    return E_OK;
}

int RuntimeContextImpl::SetPermissionCheckCallback(const PermissionCheckCallbackV2 &callback)
{
    std::unique_lock<std::shared_mutex> writeLock(permissionCheckCallbackMutex_);
    permissionCheckCallbackV2_ = callback;
    LOGI("SetPermissionCheckCallback V2 ok");
    return E_OK;
}

int RuntimeContextImpl::SetPermissionCheckCallback(const PermissionCheckCallbackV3 &callback)
{
    std::unique_lock<std::shared_mutex> writeLock(permissionCheckCallbackMutex_);
    permissionCheckCallbackV3_ = callback;
    LOGI("SetPermissionCheckCallback V3 ok");
    return E_OK;
}

int RuntimeContextImpl::RunPermissionCheck(const PermissionCheckParam &param, uint8_t flag) const
{
    bool checkResult = false;
    std::shared_lock<std::shared_mutex> autoLock(permissionCheckCallbackMutex_);
    if (permissionCheckCallbackV3_) {
        checkResult = permissionCheckCallbackV3_(param, flag);
    } else if (permissionCheckCallbackV2_) {
        checkResult = permissionCheckCallbackV2_(param.userId, param.appId, param.storeId, param.deviceId, flag);
    } else if (permissionCheckCallback_) {
        checkResult = permissionCheckCallback_(param.userId, param.appId, param.storeId, flag);
    } else {
        return E_OK;
    }
    if (checkResult) {
        return E_OK;
    }
    return -E_NOT_PERMIT;
}

int RuntimeContextImpl::EnableKvStoreAutoLaunch(const KvDBProperties &properties, AutoLaunchNotifier notifier,
    const AutoLaunchOption &option)
{
    return autoLaunch_.EnableKvStoreAutoLaunch(properties, notifier, option);
}

int RuntimeContextImpl::DisableKvStoreAutoLaunch(const std::string &normalIdentifier,
    const std::string &dualTupleIdentifier, const std::string &userId)
{
    return autoLaunch_.DisableKvStoreAutoLaunch(normalIdentifier, dualTupleIdentifier, userId);
}

void RuntimeContextImpl::GetAutoLaunchSyncDevices(const std::string &identifier,
    std::vector<std::string> &devices) const
{
    return autoLaunch_.GetAutoLaunchSyncDevices(identifier, devices);
}

void RuntimeContextImpl::SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback, DBTypeInner type)
{
    autoLaunch_.SetAutoLaunchRequestCallback(callback, type);
}

NotificationChain::Listener *RuntimeContextImpl::RegisterLockStatusLister(const LockStatusNotifier &action,
    int &errCode)
{
    std::lock(lockStatusLock_, systemApiAdapterLock_);
    std::lock_guard<std::mutex> lockStatusLock(lockStatusLock_, std::adopt_lock);
    std::lock_guard<std::recursive_mutex> systemApiAdapterLock(systemApiAdapterLock_, std::adopt_lock);
    if (lockStatusObserver_ == nullptr) {
        lockStatusObserver_ = new (std::nothrow) LockStatusObserver();
        if (lockStatusObserver_ == nullptr) {
            LOGE("lockStatusObserver_ is nullptr");
            errCode = -E_OUT_OF_MEMORY;
            return nullptr;
        }
    }

    if (!lockStatusObserver_->IsStarted()) {
        errCode = lockStatusObserver_->Start();
        if (errCode != E_OK) {
            LOGE("lockStatusObserver start failed, err = %d", errCode);
            delete lockStatusObserver_;
            lockStatusObserver_ = nullptr;
            return nullptr;
        }

        if (systemApiAdapter_ != nullptr) {
            auto callback = std::bind(&LockStatusObserver::OnStatusChange,
                lockStatusObserver_, std::placeholders::_1);
            errCode = systemApiAdapter_->RegOnAccessControlledEvent(callback);
            if (errCode != OK) {
                delete lockStatusObserver_;
                lockStatusObserver_ = nullptr;
                return nullptr;
            }
        }
    }

    NotificationChain::Listener *listener = lockStatusObserver_->RegisterLockStatusChangedLister(action, errCode);
    if ((listener == nullptr) || (errCode != E_OK)) {
        LOGE("Register lock status changed listener failed, err = %d", errCode);
        delete lockStatusObserver_;
        lockStatusObserver_ = nullptr;
        return nullptr;
    }
    return listener;
}

bool RuntimeContextImpl::IsAccessControlled() const
{
    std::lock_guard<std::recursive_mutex> autoLock(systemApiAdapterLock_);
    if (systemApiAdapter_ == nullptr) {
        return false;
    }
    return systemApiAdapter_->IsAccessControlled();
}

int RuntimeContextImpl::SetSecurityOption(const std::string &filePath, const SecurityOption &option) const
{
    std::lock_guard<std::recursive_mutex> autoLock(systemApiAdapterLock_);
    if (systemApiAdapter_ == nullptr || !OS::CheckPathExistence(filePath)) {
        LOGI("Adapter is not set, or path not existed, not support set security option!");
        return -E_NOT_SUPPORT;
    }

    if (option == SecurityOption()) {
        LOGD("SecurityOption is NOT_SET,Not need to set security option!");
        return E_OK;
    }

    std::string fileRealPath;
    int errCode = OS::GetRealPath(filePath, fileRealPath);
    if (errCode != E_OK) {
        LOGE("Get real path failed when set security option!");
        return errCode;
    }

    errCode = systemApiAdapter_->SetSecurityOption(fileRealPath, option);
    if (errCode != OK) {
        if (errCode == NOT_SUPPORT) {
            return -E_NOT_SUPPORT;
        }
        LOGE("SetSecurityOption failed, errCode = %d", errCode);
        return -E_SYSTEM_API_ADAPTER_CALL_FAILED;
    }
    return E_OK;
}

int RuntimeContextImpl::GetSecurityOption(const std::string &filePath, SecurityOption &option) const
{
    std::lock_guard<std::recursive_mutex> autoLock(systemApiAdapterLock_);
    if (systemApiAdapter_ == nullptr) {
        LOGI("Get Security option, but not set system api adapter!");
        return -E_NOT_SUPPORT;
    }
    int errCode = systemApiAdapter_->GetSecurityOption(filePath, option);
    if (errCode != OK) {
        if (errCode == NOT_SUPPORT) {
            return -E_NOT_SUPPORT;
        }
        LOGE("GetSecurityOption failed, errCode = %d", errCode);
        return -E_SYSTEM_API_ADAPTER_CALL_FAILED;
    }

    LOGD("Get security option from system adapter [%d, %d]", option.securityLabel, option.securityFlag);
    // This interface may return success but failed to obtain the flag and modified it to -1
    if (option.securityFlag == INVALID_SEC_FLAG) {
        // Currently ignoring the failure to obtain flags -1 other than S3, modify the flag to the default value
        if (option.securityLabel == S3) {
            LOGE("GetSecurityOption failed, SecurityOption is invalid [3, -1]!");
            return -E_SYSTEM_API_ADAPTER_CALL_FAILED;
        }
        option.securityFlag = 0; // 0 is default value
    }
    return E_OK;
}

bool RuntimeContextImpl::CheckDeviceSecurityAbility(const std::string &devId, const SecurityOption &option) const
{
    std::shared_ptr<IProcessSystemApiAdapter> tempSystemApiAdapter = nullptr;
    {
        std::lock_guard<std::recursive_mutex> autoLock(systemApiAdapterLock_);
        if (systemApiAdapter_ == nullptr) {
            LOGI("[CheckDeviceSecurityAbility] security not set");
            return true;
        }
        tempSystemApiAdapter = systemApiAdapter_;
    }

    return tempSystemApiAdapter->CheckDeviceSecurityAbility(devId, option);
}

int RuntimeContextImpl::SetProcessSystemApiAdapter(const std::shared_ptr<IProcessSystemApiAdapter> &adapter)
{
    std::lock(lockStatusLock_, systemApiAdapterLock_);
    std::lock_guard<std::mutex> lockStatusLock(lockStatusLock_, std::adopt_lock);
    std::lock_guard<std::recursive_mutex> systemApiAdapterLock(systemApiAdapterLock_, std::adopt_lock);
    systemApiAdapter_ = adapter;
    if (systemApiAdapter_ != nullptr && lockStatusObserver_ != nullptr && lockStatusObserver_->IsStarted()) {
        auto callback = std::bind(&LockStatusObserver::OnStatusChange,
            lockStatusObserver_, std::placeholders::_1);
        int errCode = systemApiAdapter_->RegOnAccessControlledEvent(callback);
        if (errCode != OK) {
            LOGE("Register access controlled event failed while setting adapter, err = %d", errCode);
            delete lockStatusObserver_;
            lockStatusObserver_ = nullptr;
            return -E_SYSTEM_API_ADAPTER_CALL_FAILED;
        }
    }
    return E_OK;
}

bool RuntimeContextImpl::IsProcessSystemApiAdapterValid() const
{
    std::lock_guard<std::recursive_mutex> autoLock(systemApiAdapterLock_);
    return (systemApiAdapter_ != nullptr);
}

void RuntimeContextImpl::NotifyTimestampChanged(TimeOffset offset) const
{
    std::lock_guard<std::mutex> autoLock(timeTickMonitorLock_);
    if (timeTickMonitor_ == nullptr) {
        LOGD("NotifyTimestampChanged fail, timeTickMonitor_ is null.");
        return;
    }
    timeTickMonitor_->NotifyTimeChange(offset);
}

bool RuntimeContextImpl::IsCommunicatorAggregatorValid() const
{
    std::lock_guard<std::mutex> autoLock(communicatorLock_);
    if (communicatorAggregator_ == nullptr && adapter_ == nullptr) {
        return false;
    }
    return true;
}

void RuntimeContextImpl::SetStoreStatusNotifier(const StoreStatusNotifier &notifier)
{
    std::unique_lock<std::shared_mutex> writeLock(databaseStatusCallbackMutex_);
    databaseStatusNotifyCallback_ = notifier;
    LOGI("SetStoreStatusNotifier ok");
}

void RuntimeContextImpl::NotifyDatabaseStatusChange(const std::string &userId, const std::string &appId,
    const std::string &storeId, const std::string &deviceId, bool onlineStatus)
{
    ScheduleTask([this, userId, appId, storeId, deviceId, onlineStatus] {
        std::shared_lock<std::shared_mutex> autoLock(databaseStatusCallbackMutex_);
        if (databaseStatusNotifyCallback_) {
            LOGI("start notify database status:%d", onlineStatus);
            databaseStatusNotifyCallback_(userId, appId, storeId, deviceId, onlineStatus);
        }
    });
}

int RuntimeContextImpl::SetSyncActivationCheckCallback(const SyncActivationCheckCallback &callback)
{
    std::unique_lock<std::shared_mutex> writeLock(syncActivationCheckCallbackMutex_);
    syncActivationCheckCallback_ = callback;
    LOGI("SetSyncActivationCheckCallback ok");
    return E_OK;
}

int RuntimeContextImpl::SetSyncActivationCheckCallback(const SyncActivationCheckCallbackV2 &callback)
{
    std::unique_lock<std::shared_mutex> writeLock(syncActivationCheckCallbackMutex_);
    syncActivationCheckCallbackV2_ = callback;
    LOGI("SetSyncActivationCheckCallbackV2 ok");
    return E_OK;
}

bool RuntimeContextImpl::IsSyncerNeedActive(const DBProperties &properties) const
{
    ActivationCheckParam param = {
        properties.GetStringProp(DBProperties::USER_ID, ""),
        properties.GetStringProp(DBProperties::APP_ID, ""),
        properties.GetStringProp(DBProperties::STORE_ID, ""),
        properties.GetIntProp(DBProperties::INSTANCE_ID, 0)
    };
    std::shared_lock<std::shared_mutex> autoLock(syncActivationCheckCallbackMutex_);
    if (syncActivationCheckCallbackV2_) {
        return syncActivationCheckCallbackV2_(param);
    } else if (syncActivationCheckCallback_) {
        return syncActivationCheckCallback_(param.userId, param.appId, param.storeId);
    }
    return true;
}

NotificationChain::Listener *RuntimeContextImpl::RegisterUserChangedListener(const UserChangedAction &action,
    EventType event)
{
    int errCode;
    std::lock_guard<std::mutex> autoLock(userChangeMonitorLock_);
    if (userChangeMonitor_ == nullptr) {
        userChangeMonitor_ = std::make_unique<UserChangeMonitor>();
        errCode = userChangeMonitor_->Start();
        if (errCode != E_OK) {
            LOGE("UserChangeMonitor start failed!");
            userChangeMonitor_ = nullptr;
            return nullptr;
        }
    }
    NotificationChain::Listener *listener = userChangeMonitor_->RegisterUserChangedListener(action, event, errCode);
    if ((listener == nullptr) || (errCode != E_OK)) {
        LOGE("Register user status changed listener failed, err = %d", errCode);
        return nullptr;
    }
    return listener;
}

int RuntimeContextImpl::NotifyUserChanged() const
{
    {
        std::lock_guard<std::mutex> autoLock(userChangeMonitorLock_);
        if (userChangeMonitor_ == nullptr) {
            LOGD("userChangeMonitor is null, all db is in normal sync mode");
            return E_OK;
        }
    }
    userChangeMonitor_->NotifyUserChanged();
    return E_OK;
}

uint32_t RuntimeContextImpl::GenerateSessionId()
{
    uint32_t sessionId = currentSessionId_++;
    if (sessionId == 0) {
        sessionId = currentSessionId_++;
    }
    return sessionId;
}

void RuntimeContextImpl::DumpCommonInfo(int fd)
{
    autoLaunch_.Dump(fd);
}

void RuntimeContextImpl::CloseAutoLaunchConnection(DBTypeInner type, const DBProperties &properties)
{
    autoLaunch_.CloseConnection(type, properties);
}

int RuntimeContextImpl::SetPermissionConditionCallback(const PermissionConditionCallback &callback)
{
    std::unique_lock<std::shared_mutex> autoLock(permissionConditionLock_);
    permissionConditionCallback_ = callback;
    return E_OK;
}

std::map<std::string, std::string> RuntimeContextImpl::GetPermissionCheckParam(const DBProperties &properties)
{
    PermissionConditionParam param = {
        properties.GetStringProp(DBProperties::USER_ID, ""),
        properties.GetStringProp(DBProperties::APP_ID, ""),
        properties.GetStringProp(DBProperties::STORE_ID, ""),
        properties.GetIntProp(DBProperties::INSTANCE_ID, 0)
    };
    std::shared_lock<std::shared_mutex> autoLock(permissionConditionLock_);
    if (permissionConditionCallback_ == nullptr) {
        return {};
    }
    return permissionConditionCallback_(param);
}

void RuntimeContextImpl::StopTaskPool()
{
    std::lock_guard<std::mutex> autoLock(taskLock_);
    if (taskPool_ != nullptr) {
        taskPool_->Stop();
        TaskPool::Release(taskPool_);
        taskPool_ = nullptr;
    }
}

void RuntimeContextImpl::StopTimeTickMonitorIfNeed()
{
    if (IsCommunicatorAggregatorValid()) {
        return;
    }
    // release monitor in client
    std::lock_guard<std::mutex> autoLock(timeTickMonitorLock_);
    if (timeTickMonitor_ == nullptr) {
        return;
    }
    if (timeTickMonitor_->EmptyListener()) {
        LOGD("[RuntimeContext] TimeTickMonitor exist because no listener");
        timeTickMonitor_ = nullptr;
    }
    LOGD("[RuntimeContext] TimeTickMonitor can not stop because listener is not empty");
}

void RuntimeContextImpl::SetDBInfoHandle(const std::shared_ptr<DBInfoHandle> &handle)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter != nullptr) {
        dbStatusAdapter->SetDBInfoHandle(handle);
    }
    std::shared_ptr<SubscribeRecorder> subscribeRecorder = GetSubscribeRecorder();
    if (subscribeRecorder != nullptr) {
        subscribeRecorder->RemoveAllSubscribe();
    }
}

void RuntimeContextImpl::NotifyDBInfos(const DeviceInfos &devInfos, const std::vector<DBInfo> &dbInfos)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter != nullptr) {
        dbStatusAdapter->NotifyDBInfos(devInfos, dbInfos);
    }
}

std::shared_ptr<DBStatusAdapter> RuntimeContextImpl::GetDBStatusAdapter()
{
    std::lock_guard<std::mutex> autoLock(statusAdapterMutex_);
    if (dbStatusAdapter_ == nullptr) {
        dbStatusAdapter_ = std::make_unique<DBStatusAdapter>();
    }
    if (dbStatusAdapter_ == nullptr) {
        LOGE("[RuntimeContextImpl] DbStatusAdapter create failed!");
    }
    return dbStatusAdapter_;
}

void RuntimeContextImpl::RecordRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId,
    const QuerySyncObject &query)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter != nullptr && dbStatusAdapter->IsSendLabelExchange()) {
        return;
    }
    std::shared_ptr<SubscribeRecorder> subscribeRecorder = GetSubscribeRecorder();
    if (subscribeRecorder != nullptr) {
        subscribeRecorder->RecordSubscribe(dbInfo, deviceId, query);
    }
}

void RuntimeContextImpl::RemoveRemoteSubscribe(const DeviceID &deviceId)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter != nullptr && dbStatusAdapter->IsSendLabelExchange()) {
        return;
    }
    std::shared_ptr<SubscribeRecorder> subscribeRecorder = GetSubscribeRecorder();
    if (subscribeRecorder != nullptr) {
        subscribeRecorder->RemoveRemoteSubscribe(deviceId);
    }
}

void RuntimeContextImpl::RemoveRemoteSubscribe(const DBInfo &dbInfo)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter != nullptr && dbStatusAdapter->IsSendLabelExchange()) {
        return;
    }
    std::shared_ptr<SubscribeRecorder> subscribeRecorder = GetSubscribeRecorder();
    if (subscribeRecorder != nullptr) {
        subscribeRecorder->RemoveRemoteSubscribe(dbInfo);
    }
}

void RuntimeContextImpl::RemoveRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter != nullptr && dbStatusAdapter->IsSendLabelExchange()) {
        return;
    }
    std::shared_ptr<SubscribeRecorder> subscribeRecorder = GetSubscribeRecorder();
    if (subscribeRecorder != nullptr) {
        subscribeRecorder->RemoveRemoteSubscribe(dbInfo, deviceId);
    }
}

void RuntimeContextImpl::RemoveRemoteSubscribe(const DBInfo &dbInfo, const DeviceID &deviceId,
    const QuerySyncObject &query)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter != nullptr && dbStatusAdapter->IsSendLabelExchange()) {
        return;
    }
    std::shared_ptr<SubscribeRecorder> subscribeRecorder = GetSubscribeRecorder();
    if (subscribeRecorder != nullptr) {
        subscribeRecorder->RemoveRemoteSubscribe(dbInfo, deviceId, query);
    }
}

void RuntimeContextImpl::GetSubscribeQuery(const DBInfo &dbInfo,
    std::map<std::string, std::vector<QuerySyncObject>> &subscribeQuery)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter != nullptr && dbStatusAdapter->IsSendLabelExchange()) {
        return;
    }
    std::shared_ptr<SubscribeRecorder> subscribeRecorder = GetSubscribeRecorder();
    if (subscribeRecorder != nullptr) {
        subscribeRecorder->GetSubscribeQuery(dbInfo, subscribeQuery);
    }
}

std::shared_ptr<SubscribeRecorder> RuntimeContextImpl::GetSubscribeRecorder()
{
    std::lock_guard<std::mutex> autoLock(subscribeRecorderMutex_);
    if (subscribeRecorder_ == nullptr) {
        subscribeRecorder_ = std::make_unique<SubscribeRecorder>();
    }
    if (subscribeRecorder_ == nullptr) {
        LOGE("[RuntimeContextImpl] SubscribeRecorder create failed!");
    }
    return subscribeRecorder_;
}

bool RuntimeContextImpl::IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
    const std::string &devInfo)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter == nullptr) {
        return true;
    }
    return dbStatusAdapter->IsNeedAutoSync(userId, appId, storeId, devInfo);
}

void RuntimeContextImpl::SetRemoteOptimizeCommunication(const std::string &dev, bool optimize)
{
    std::shared_ptr<DBStatusAdapter> dbStatusAdapter = GetDBStatusAdapter();
    if (dbStatusAdapter == nullptr) {
        return;
    }
    dbStatusAdapter->SetRemoteOptimizeCommunication(dev, optimize);
}

void RuntimeContextImpl::SetTranslateToDeviceIdCallback(const TranslateToDeviceIdCallback &callback)
{
    std::lock_guard<std::mutex> autoLock(translateToDeviceIdLock_);
    translateToDeviceIdCallback_ = callback;
    deviceIdCache_.clear();
}

int RuntimeContextImpl::TranslateDeviceId(const std::string &deviceId,
    const StoreInfo &info, std::string &newDeviceId)
{
    const std::string id = DBCommon::GenerateIdentifierId(info.storeId, info.appId, info.userId);
    std::lock_guard<std::mutex> autoLock(translateToDeviceIdLock_);
    if (translateToDeviceIdCallback_ == nullptr) {
        return -E_NOT_SUPPORT;
    }
    if (deviceIdCache_.find(deviceId) == deviceIdCache_.end() ||
        deviceIdCache_[deviceId].find(id) == deviceIdCache_[deviceId].end()) {
        deviceIdCache_[deviceId][id] = translateToDeviceIdCallback_(deviceId, info);
    }
    newDeviceId = deviceIdCache_[deviceId][id];
    return E_OK;
}

bool RuntimeContextImpl::ExistTranslateDevIdCallback() const
{
    std::lock_guard<std::mutex> autoLock(translateToDeviceIdLock_);
    return translateToDeviceIdCallback_ != nullptr;
}

void RuntimeContextImpl::SetThreadPool(const std::shared_ptr<IThreadPool> &threadPool)
{
    std::unique_lock<std::shared_mutex> writeLock(threadPoolLock_);
    threadPool_ = threadPool;
    LOGD("[RuntimeContext] Set thread pool finished");
}

std::shared_ptr<IThreadPool> RuntimeContextImpl::GetThreadPool() const
{
    std::shared_lock<std::shared_mutex> readLock(threadPoolLock_);
    return threadPool_;
}

int RuntimeContextImpl::ScheduleTaskByThreadPool(const DistributedDB::TaskAction &task) const
{
    std::shared_ptr<IThreadPool> threadPool = GetThreadPool();
    if (threadPool == nullptr) {
        return -E_NOT_SUPPORT;
    }
    (void)threadPool->Execute(task);
    return E_OK;
}

int RuntimeContextImpl::SetTimerByThreadPool(int milliSeconds, const TimerAction &action,
    const TimerFinalizer &finalizer, bool allocTimerId, TimerId &timerId)
{
    std::shared_ptr<IThreadPool> threadPool = GetThreadPool();
    if (threadPool == nullptr) {
        return -E_NOT_SUPPORT;
    }
    int errCode = E_OK;
    if (allocTimerId) {
        errCode = AllocTimerId(nullptr, timerId);
    } else {
        std::lock_guard<std::mutex> autoLock(timerTaskLock_);
        if (taskIds_.find(timerId) == taskIds_.end()) {
            LOGD("[SetTimerByThreadPool] Timer has been remove");
            return -E_NO_SUCH_ENTRY;
        }
    }
    if (errCode != E_OK) {
        return errCode;
    }
    std::lock_guard<std::mutex> autoLock(timerTaskLock_);
    if (!allocTimerId && taskIds_.find(timerId) == taskIds_.end()) {
        LOGD("[SetTimerByThreadPool] Timer has been remove");
        return -E_NO_SUCH_ENTRY;
    }
    timerFinalizers_[timerId] = finalizer;
    Duration duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::milliseconds(milliSeconds));
    TaskId taskId = threadPool->Execute([milliSeconds, action, timerId, this]() {
        ThreadPoolTimerAction(milliSeconds, action, timerId);
    }, duration);
    taskIds_[timerId] = taskId;
    return E_OK;
}

int RuntimeContextImpl::ModifyTimerByThreadPool(TimerId timerId, int milliSeconds)
{
    std::shared_ptr<IThreadPool> threadPool = GetThreadPool();
    if (threadPool == nullptr) {
        return -E_NOT_SUPPORT;
    }
    TaskId taskId;
    {
        std::lock_guard<std::mutex> autoLock(timerTaskLock_);
        if (taskIds_.find(timerId) == taskIds_.end()) {
            return -E_NO_SUCH_ENTRY;
        }
        taskId = taskIds_[timerId];
    }
    Duration duration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::milliseconds(milliSeconds));
    TaskId ret = threadPool->Reset(taskId, duration);
    if (ret != taskId) {
        return -E_NO_SUCH_ENTRY;
    }
    return E_OK;
}

void RuntimeContextImpl::RemoveTimerByThreadPool(TimerId timerId, bool wait)
{
    std::shared_ptr<IThreadPool> threadPool = GetThreadPool();
    if (threadPool == nullptr) {
        return;
    }
    TaskId taskId;
    {
        std::lock_guard<std::mutex> autoLock(timerTaskLock_);
        if (taskIds_.find(timerId) == taskIds_.end()) {
            return;
        }
        taskId = taskIds_[timerId];
        taskIds_.erase(timerId);
    }
    bool removeBeforeExecute = threadPool->Remove(taskId, wait);
    TimerFinalizer timerFinalizer = nullptr;
    if (removeBeforeExecute) {
        std::lock_guard<std::mutex> autoLock(timerTaskLock_);
        timerFinalizer = timerFinalizers_[timerId];
        timerFinalizers_.erase(timerId);
    }
    if (timerFinalizer) {
        timerFinalizer();
    }
}

void RuntimeContextImpl::ThreadPoolTimerAction(int milliSeconds, const TimerAction &action, TimerId timerId)
{
    TimerFinalizer timerFinalizer = nullptr;
    bool timerExist = true;
    {
        std::lock_guard<std::mutex> autoLock(timerTaskLock_);
        if (timerFinalizers_.find(timerId) == timerFinalizers_.end()) {
            LOGD("[ThreadPoolTimerAction] Timer has been finalize");
            return;
        }
        timerFinalizer = timerFinalizers_[timerId];
        timerFinalizers_.erase(timerId);
        if (taskIds_.find(timerId) == taskIds_.end()) {
            LOGD("[ThreadPoolTimerAction] Timer has been removed");
            timerExist = false;
        }
    }
    if (timerExist && action(timerId) == E_OK) {
        // schedule task again
        int errCode = SetTimerByThreadPool(milliSeconds, action, timerFinalizer, false, timerId);
        if (errCode == E_OK) {
            return;
        }
        LOGW("[RuntimeContext] create timer failed %d", errCode);
    }
    if (timerFinalizer) {
        timerFinalizer();
    }
    {
        std::lock_guard<std::mutex> autoLock(timerTaskLock_);
        taskIds_.erase(timerId);
    }
    std::lock_guard<std::mutex> autoLock(timersLock_);
    timers_.erase(timerId);
}

void RuntimeContextImpl::SetCloudTranslate(const std::shared_ptr<ICloudDataTranslate> &dataTranslate)
{
    std::unique_lock<std::shared_mutex> autoLock(dataTranslateLock_);
    dataTranslate_ = dataTranslate;
}

int RuntimeContextImpl::AssetToBlob(const Asset &asset, std::vector<uint8_t> &blob)
{
    std::shared_lock<std::shared_mutex> autoLock(dataTranslateLock_);
    if (dataTranslate_ == nullptr) {
        return -E_NOT_INIT;
    }
    blob = dataTranslate_->AssetToBlob(asset);
    return E_OK;
}

int RuntimeContextImpl::AssetsToBlob(const Assets &assets, std::vector<uint8_t> &blob)
{
    std::shared_lock<std::shared_mutex> autoLock(dataTranslateLock_);
    if (dataTranslate_ == nullptr) {
        return -E_NOT_INIT;
    }
    blob = dataTranslate_->AssetsToBlob(assets);
    return E_OK;
}

int RuntimeContextImpl::BlobToAsset(const std::vector<uint8_t> &blob, Asset &asset)
{
    std::shared_lock<std::shared_mutex> autoLock(dataTranslateLock_);
    if (dataTranslate_ == nullptr) {
        return -E_NOT_INIT;
    }
    asset = dataTranslate_->BlobToAsset(blob);
    return E_OK;
}

int RuntimeContextImpl::BlobToAssets(const std::vector<uint8_t> &blob, Assets &assets)
{
    std::shared_lock<std::shared_mutex> autoLock(dataTranslateLock_);
    if (dataTranslate_ == nullptr) {
        return -E_NOT_INIT;
    }
    assets = dataTranslate_->BlobToAssets(blob);
    return E_OK;
}

std::pair<int, DeviceTimeInfo> RuntimeContextImpl::GetDeviceTimeInfo(const std::string &device) const
{
    std::pair<int, DeviceTimeInfo> res;
    auto &[errCode, info] = res;
    std::lock_guard<std::mutex> autoLock(deviceTimeInfoLock_);
    if (deviceTimeInfos_.find(device) == deviceTimeInfos_.end()) {
        errCode = -E_NOT_FOUND;
    } else {
        info = deviceTimeInfos_.at(device);
        errCode = E_OK;
    }
    return res;
}

void RuntimeContextImpl::SetDeviceTimeInfo(const std::string &device, const DeviceTimeInfo &deviceTimeInfo)
{
    std::lock_guard<std::mutex> autoLock(deviceTimeInfoLock_);
    deviceTimeInfos_[device] = deviceTimeInfo;
}

void RuntimeContextImpl::ClearDeviceTimeInfo(const std::string &device)
{
    std::lock_guard<std::mutex> autoLock(deviceTimeInfoLock_);
    deviceTimeInfos_.erase(device);
}

void RuntimeContextImpl::ClearAllDeviceTimeInfo()
{
    std::lock_guard<std::mutex> autoLock(deviceTimeInfoLock_);
    deviceTimeInfos_.clear();
}

void RuntimeContextImpl::RecordAllTimeChange()
{
    std::lock_guard<std::mutex> autoLock(deviceTimeInfoLock_);
    for (auto &item : dbTimeChange_) {
        item.second = true;
    }
}

void RuntimeContextImpl::ResetDBTimeChangeStatus(const std::vector<uint8_t> &dbId)
{
    std::lock_guard<std::mutex> autoLock(deviceTimeInfoLock_);
    dbTimeChange_[dbId] = false;
}

bool RuntimeContextImpl::CheckDBTimeChange(const std::vector<uint8_t> &dbId)
{
    std::lock_guard<std::mutex> autoLock(deviceTimeInfoLock_);
    return dbTimeChange_[dbId];
}

bool RuntimeContextImpl::IsTimeTickMonitorValid() const
{
    std::lock_guard<std::mutex> autoLock(timeTickMonitorLock_);
    return timeTickMonitor_ != nullptr;
}
} // namespace DistributedDB
