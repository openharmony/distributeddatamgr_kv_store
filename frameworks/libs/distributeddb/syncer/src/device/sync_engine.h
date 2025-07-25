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

#ifndef SYNC_ENGINE_H
#define SYNC_ENGINE_H

#include <map>
#include <mutex>
#include <queue>

#include "communicator_proxy.h"
#include "device_manager.h"
#include "isync_engine.h"
#include "isync_task_context.h"
#include "remote_executor.h"
#include "subscribe_manager.h"
#include "task_pool.h"

namespace DistributedDB {

class SyncEngine : public ISyncEngine {
public:
    SyncEngine();
    ~SyncEngine() override;

    // Do some init things
    int Initialize(ISyncInterface *syncInterface, const std::shared_ptr<Metadata> &metadata,
        const InitCallbackParam &callbackParam) override;

    // Do some things, when db close.
    int Close() override;

    // Alloc and Add sync SyncTarget
    // return E_OK if operator success.
    int AddSyncOperation(SyncOperation *operation) override;

    // Clear the SyncTarget matched the syncId.
    void RemoveSyncOperation(int syncId) override;

#ifndef OMIT_MULTI_VER
    // notify other devices data has changed
    void BroadCastDataChanged() const override;
#endif

    // Get Online devices
    void GetOnlineDevices(std::vector<std::string> &devices) const override;

    // Register the device connect callback, this function must be called after Engine inited
    void StartCommunicator() override;

    // Get the queue cache memory size
    int GetQueueCacheSize() const;

    // Set the queue cache memory size
    void SetQueueCacheSize(int size);

    // Get the number of message which is discarded
    unsigned int GetDiscardMsgNum() const;

    // Set the number of message which is discarded
    void SetDiscardMsgNum(unsigned int num);

    // Get the maximum of executing message number
    unsigned int GetMaxExecNum() const;

    // Get the maximum of queue cache memory size
    int GetMaxQueueCacheSize() const;

    // Set the maximum of queue cache memory size
    void SetMaxQueueCacheSize(int value);

    std::string GetLabel() const override;

    bool GetSyncRetry() const;
    void SetSyncRetry(bool isRetry) override;

    // Set an equal identifier for this database, After this called, send msg to the target will use this identifier
    int SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets) override;

    void SetEqualIdentifier() override;

    void SetEqualIdentifierMap(const std::string &identifier, const std::vector<std::string> &targets) override;

    void OfflineHandleByDevice(const std::string &deviceId, ISyncInterface *storage);

    void ClearAllSyncTaskByDevice(const std::string &deviceId) override;

    void GetLocalSubscribeQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries);

    // subscribeQueries item is queryId
    void GetRemoteSubscribeQueryIds(const std::string &device, std::vector<std::string> &subscribeQueryIds);

    void GetRemoteSubscribeQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries);

    void PutUnfinishedSubQueries(const std::string &device, const std::vector<QuerySyncObject> &subscribeQueries);

    void GetAllUnFinishSubQueries(std::map<std::string, std::vector<QuerySyncObject>> &allSyncQueries);

    // used by SingleVerSyncer when db online
    int StartAutoSubscribeTimer(const ISyncInterface &syncInterface) override;

    // used by SingleVerSyncer when remote/local db closed
    void StopAutoSubscribeTimer() override;

    int SubscribeLimitCheck(const std::vector<std::string> &devices, QuerySyncObject &query) const override;

    bool IsEngineActive() const override;

    void SchemaChange() override;

    void Dump(int fd) override;

    int RemoteQuery(const std::string &device, const RemoteCondition &condition,
        uint64_t timeout, uint64_t connectionId, std::shared_ptr<ResultSet> &result) override;

    void NotifyConnectionClosed(uint64_t connectionId) override;

    void NotifyUserChange() override;

    void AbortMachineIfNeed(uint32_t syncId) override;

    void AddSubscribe(SyncGenericInterface *storage,
        const std::map<std::string, std::vector<QuerySyncObject>> &subscribeQuery) override;

    void TimeChange() override;

    int32_t GetResponseTaskCount() override;

    int32_t GetRemoteQueryTaskCount() override;
protected:
    // Create a context
    virtual ISyncTaskContext *CreateSyncTaskContext(const ISyncInterface &syncInterface) = 0;

    // Find SyncTaskContext from the map
    ISyncTaskContext *FindSyncTaskContext(const DeviceSyncTarget &target);
    std::vector<ISyncTaskContext *> GetSyncTaskContextAndInc(const std::string &deviceId);
    void GetQueryAutoSyncParam(const std::string &device, const QuerySyncObject &query, InternalSyncParma &outParam);
    void GetSubscribeSyncParam(const std::string &device, const QuerySyncObject &query, InternalSyncParma &outParam);

    void ClearSyncInterface();
    ISyncInterface *GetAndIncSyncInterface();
    void SetSyncInterface(ISyncInterface *syncInterface);

    ISyncTaskContext *GetSyncTaskContext(const DeviceSyncTarget &target, int &errCode);

    std::mutex storageMutex_;
    ISyncInterface *syncInterface_;
    // Used to store all send sync task infos (such as pull sync response, and push sync request)
    std::map<DeviceSyncTarget, ISyncTaskContext *> syncTaskContextMap_;
    std::mutex contextMapLock_;
    std::shared_ptr<SubscribeManager> subManager_;
    std::function<void(const InternalSyncParma &param)> queryAutoSyncCallback_;

private:

    // Init DeviceManager set callback and remoteExecutor
    int InitInnerSource(const std::function<void(std::string)> &onRemoteDataChanged,
        const std::function<void(std::string)> &offlineChanged, ISyncInterface *syncInterface);

    // Init Comunicator, register callbacks
    int InitComunicator(const ISyncInterface *syncInterface);

    // Add the sync task info to the map.
    int AddSyncOperForContext(const std::string &deviceId, SyncOperation *operation);

    // Sync Request CallbackTask run at a sub thread.
    void MessageReciveCallbackTask(ISyncTaskContext *context, const ICommunicator *communicator, Message *inMsg);

    void RemoteDataChangedTask(ISyncTaskContext *context, const ICommunicator *communicator, Message *inMsg);

    void ScheduleTaskOut(ISyncTaskContext *context, const ICommunicator *communicator);

    // wrapper of MessageReciveCallbackTask
    void MessageReciveCallback(const std::string &targetDev, Message *inMsg);

    // Sync Request Callback
    int MessageReciveCallbackInner(const std::string &targetDev, Message *inMsg);

    // Exec the given SyncTarget. and callback onComplete.
    int ExecSyncTask(ISyncTaskContext *context);

    // Anti-DOS attack
    void PutMsgIntoQueue(const std::string &targetDev, Message *inMsg, int msgSize);

    // Get message size
    int GetMsgSize(const Message *inMsg) const;

    // Do not run MessageReceiveCallbackTask until msgQueue is empty
    int DealMsgUtilQueueEmpty();

    // Handle message in order.
    int ScheduleDealMsg(ISyncTaskContext *context, Message *inMsg);

    ISyncTaskContext *GetContextForMsg(const DeviceSyncTarget &target, int &errCode);

    ICommunicator *AllocCommunicator(const std::string &identifier, int &errCode, std::string userId = "");

    void UnRegCommunicatorsCallback();

    void ReleaseCommunicators();

    bool IsSkipCalculateLen(const Message *inMsg);

    void ClearInnerResource();

    void IncExecTaskCount();

    void DecExecTaskCount();

    RemoteExecutor *GetAndIncRemoteExector();

    void SetRemoteExector(RemoteExecutor *executor);

    bool CheckDeviceIdValid(const std::string &checkDeviceId, const std::string &localDeviceId);

    int GetLocalDeviceId(std::string &deviceId);

    void WaitingExecTaskExist();

    int HandleRemoteExecutorMsg(const std::string &targetDev, Message *inMsg);

    void AddQuerySubscribe(SyncGenericInterface *storage, const std::string &device, const QuerySyncObject &query);

    std::string GetUserId();

    std::string GetUserId(const ISyncInterface *syncInterface);

    uint32_t GetTimeout(const std::string &dev);

    std::string GetTargetUserId(const std::string &dev);
    ICommunicator *communicator_;
    DeviceManager *deviceManager_;
    std::function<void(const std::string &)> onRemoteDataChanged_;
    std::function<void(const std::string &)> offlineChanged_;
    std::shared_ptr<Metadata> metadata_;
    std::deque<Message *> msgQueue_;
    volatile uint32_t execTaskCount_;
    std::string label_;
    volatile bool isSyncRetry_;
    std::mutex communicatorProxyLock_;
    CommunicatorProxy *communicatorProxy_;
    std::mutex equalCommunicatorsLock_;
    std::map<std::string, ICommunicator *> equalCommunicators_;

    static int queueCacheSize_;
    static int maxQueueCacheSize_;
    static unsigned int discardMsgNum_;
    static const unsigned int MAX_EXEC_NUM = 7; // Set the maximum of threads as 6 < 7
    static constexpr int DEFAULT_CACHE_SIZE = 160 * 1024 * 1024; // Initial the default cache size of queue as 160MB
    static std::mutex queueLock_;
    std::atomic<bool> isActive_;

    // key: device value: equalIdentifier
    std::map<std::string, std::string> equalIdentifierMap_;
    std::mutex execTaskCountLock_;
    std::condition_variable execTaskCv_;

    std::mutex remoteExecutorLock_;
    RemoteExecutor *remoteExecutor_;
};
} // namespace DistributedDB

#endif // SYNC_ENGINE_H
