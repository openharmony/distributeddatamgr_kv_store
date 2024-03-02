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

#ifndef SINGLE_VER_SYNC_TASK_CONTEXT_H
#define SINGLE_VER_SYNC_TASK_CONTEXT_H

#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

#include "db_ability.h"
#include "query_sync_object.h"
#include "schema_negotiate.h"
#include "single_ver_kvdb_sync_interface.h"
#include "single_ver_sync_target.h"
#include "subscribe_manager.h"
#include "sync_target.h"
#include "sync_task_context.h"
#include "time_helper.h"


namespace DistributedDB {
class SingleVerSyncTaskContext : public SyncTaskContext {
public:
    explicit SingleVerSyncTaskContext();

    DISABLE_COPY_ASSIGN_MOVE(SingleVerSyncTaskContext);

    // Init SingleVerSyncTaskContext
    int Initialize(const std::string &deviceId, ISyncInterface *syncInterface,
        const std::shared_ptr<Metadata> &metadata, ICommunicator *communicator) override;

    // Add a sync task target with the operation to the queue
    int AddSyncOperation(SyncOperation *operation) override;

    bool IsCurrentSyncTaskCanBeSkipped() const override;

    // Set the end watermark of this task
    void SetEndMark(WaterMark endMark);

    // Get the end watermark of this task
    WaterMark GetEndMark() const;

    void GetContinueToken(ContinueToken &outToken) const;

    void SetContinueToken(ContinueToken token);

    void ReleaseContinueToken();

    int PopResponseTarget(SingleVerSyncTarget &target);

    int GetRspTargetQueueSize() const;

    // responseSessionId used for mark the pull response task
    void SetResponseSessionId(uint32_t responseSessionId);

    // responseSessionId used for mark the pull response task
    uint32_t GetResponseSessionId() const;

    void Clear() override;

    void Abort(int status) override;

    void ClearAllSyncTask() override;

    // If set true, remote stale data will be clear when remote db rebuilt.
    void EnableClearRemoteStaleData(bool enable);

    // Check if need to clear remote device stale data in syncing, when the remote db rebuilt.
    bool IsNeedClearRemoteStaleData() const;

    // start a timer to ResetWatchDog when sync data one (key,value) size bigger than mtu
    bool StartFeedDogForSync(uint32_t time, SyncDirectionFlag flag);

    // stop timer to ResetWatchDog when sync data one (key,value) size bigger than mtu
    void StopFeedDogForSync(SyncDirectionFlag flag);

    // is receive waterMark err
    bool IsReceiveWaterMarkErr() const;

    // set receive waterMark err
    void SetReceiveWaterMarkErr(bool isErr);

    void SetRemoteSeccurityOption(SecurityOption secOption);

    SecurityOption GetRemoteSeccurityOption() const;

    void SetReceivcPermitCheck(bool isChecked);

    bool GetReceivcPermitCheck() const;

    void SetSendPermitCheck(bool isChecked);

    bool GetSendPermitCheck() const;
    // pair<bool, bool>: first:SyncStrategy.permitSync, second: isSchemaSync_
    virtual std::pair<bool, bool> GetSchemaSyncStatus(QuerySyncObject &querySyncObject) const = 0;

    bool IsSkipTimeoutError(int errCode) const;

    bool FindResponseSyncTarget(uint32_t responseSessionId) const;

    // For query sync
    void SetQuery(const QuerySyncObject &query);
    QuerySyncObject GetQuery() const;
    void SetQuerySync(bool isQuerySync);
    bool IsQuerySync() const;
    std::set<CompressAlgorithm> GetRemoteCompressAlgo() const;
    std::string GetRemoteCompressAlgoStr() const;
    void SetDbAbility(DbAbility &remoteDbAbility) override;
    CompressAlgorithm ChooseCompressAlgo() const;
    bool IsNotSupportAbility(const AbilityItem &abilityItem) const;

    void SetSubscribeManager(std::shared_ptr<SubscribeManager> &subManager);
    std::shared_ptr<SubscribeManager> GetSubscribeManager() const;

    void SaveLastPushTaskExecStatus(int finalStatus) override;
    void ResetLastPushTaskStatus() override;

    virtual std::string GetQuerySyncId() const = 0;
    virtual std::string GetDeleteSyncId() const = 0;

    void SetCommNormal(bool isCommNormal);

    void StartFeedDogForGetData(uint32_t sessionId);
    void StopFeedDogForGetData();
protected:
    ~SingleVerSyncTaskContext() override;
    void CopyTargetData(const ISyncTarget *target, const TaskParam &taskParam) override;

    // For querySync
    mutable std::mutex queryMutex_;
    QuerySyncObject query_;
    bool isQuerySync_ = false;
    
    // for merge sync task
    volatile int lastFullSyncTaskStatus_ = SyncOperation::Status::OP_WAITING;
private:
    int GetCorrectedSendWaterMarkForCurrentTask(const SyncOperation *operation, uint64_t &waterMark) const;

    bool IsCurrentSyncTaskCanBeSkippedInner(const SyncOperation *operation) const;

    DECLARE_OBJECT_TAG(SingleVerSyncTaskContext);

    ContinueToken token_;
    WaterMark endMark_;
    volatile uint32_t responseSessionId_ = 0;

    bool needClearRemoteStaleData_;
    mutable std::mutex securityOptionMutex_;
    SecurityOption remoteSecOption_ = {0, 0}; // remote targe can handle secOption data or not.
    volatile bool isReceivcPermitChecked_ = false;
    volatile bool isSendPermitChecked_ = false;

    // is receive waterMark err, peerWaterMark bigger than remote localWaterMark
    volatile bool isReceiveWaterMarkErr_ = false;

    // For db ability
    mutable std::mutex remoteDbAbilityLock_;
    DbAbility remoteDbAbility_;

    // For subscribe manager
    std::shared_ptr<SubscribeManager> subManager_;

    mutable std::mutex queryTaskStatusMutex_;
    // <queryId, lastExcuStatus>
    std::unordered_map<std::string, int> lastQuerySyncTaskStatusMap_;
};
} // namespace DistributedDB

#endif // SYNC_TASK_CONTEXT_H
