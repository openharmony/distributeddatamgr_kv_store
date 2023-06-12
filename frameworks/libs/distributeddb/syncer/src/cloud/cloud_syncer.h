/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef CLOUD_SYNCER_H
#define CLOUD_SYNCER_H
#include <atomic>
#include <condition_variable>
#include <mutex>
#include "cloud_db_proxy.h"
#include "cloud/cloud_store_types.h"
#include "cloud/cloud_sync_strategy.h"
#include "data_transformer.h"
#include "db_common.h"
#include "cloud/icloud_db.h"
#include "ref_object.h"
#include "runtime_context.h"
#include "storage_proxy.h"
#include "store_observer.h"

namespace DistributedDB {
class CloudSyncer : public RefObject {
public:
    explicit CloudSyncer(std::shared_ptr<StorageProxy> storageProxy);
    ~CloudSyncer() override = default;
    DISABLE_COPY_ASSIGN_MOVE(CloudSyncer);

    int Sync(const std::vector<DeviceID> &devices, SyncMode mode, const std::vector<std::string> &tables,
        const SyncProcessCallback &callback, int64_t waitTime);

    int SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB);

    void Close();
protected:
    using TaskId = uint64_t;
    struct CloudTaskInfo {
        SyncMode mode = SyncMode::SYNC_MODE_PUSH_ONLY;
        ProcessStatus status = ProcessStatus::PREPARED;
        int errCode = 0;
        TaskId taskId = 0u;
        std::vector<std::string> table;
        SyncProcessCallback callback;
        int64_t timeout;
        std::vector<std::string> devices;
    };
    struct InnerProcessInfo {
        std::string tableName;
        ProcessStatus tableStatus = ProcessStatus::PREPARED;
        Info downLoadInfo;
        Info upLoadInfo;
    };
    class ProcessNotifier {
    public:
        void Init(const std::vector<std::string> &tableName, const std::vector<std::string> &devices);

        void UpdateProcess(const InnerProcessInfo &process);

        void NotifyProcess(const CloudTaskInfo &taskInfo, const InnerProcessInfo &process);
    
        std::vector<std::string> GetDevices() const;
    protected:
        std::mutex processMutex_;
        SyncProcess syncProcess_;
        std::vector<std::string> devices_;
    };
    struct TaskContext {
        TaskId currentTaskId = 0u;
        std::string tableName;
        std::shared_ptr<ProcessNotifier> notifier;
        std::shared_ptr<CloudSyncStrategy> strategy;
    };
    struct UploadParam {
        int64_t count = 0;
        TaskId taskId = 0u;
        LocalWaterMark localMark = 0u;
        bool lastTable = false;
    };

    int TriggerSync();

    void DoSyncIfNeed();

    int DoSync(TaskId taskId);

    int DoSyncInner(const CloudTaskInfo &taskInfo);

    void DoFinished(TaskId taskId, int errCode, const InnerProcessInfo &processInfo);

    virtual int DoDownload(TaskId taskId);

    int PreCheckUpload(TaskId &taskId, const TableName &tableName);

    int PreCheck(TaskId &taskId, const TableName &tableName);

    int DoBatchUpload(CloudSyncData &uploadData, UploadParam &uploadParam, InnerProcessInfo &innerProcessInfo);
    
    int CheckCloudSyncDataValid(CloudSyncData uploadData, const std::string &tableName, const int64_t &count,
        TaskId &taskId);

    bool CheckCloudSyncDataEmpty(CloudSyncData &uploadData);

    int GetWaterMarkInner(const std::vector<VBucket>& extend, LocalWaterMark &waterMark);

    int CalculateLocalWaterMark(CloudSyncData &uploadData, const int64_t &count, TaskId taskId,
        LocalWaterMark &waterMark);

    void ClearCloudSyncData(CloudSyncData &uploadData);

    int PreProcessBatchUpload(TaskId taskId, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo,
        LocalWaterMark &localMark);

    virtual int DoUpload(TaskId taskId, bool lastTable);

    int DoUploadInner(const std::string &tableName, UploadParam &uploadParam);

    int CheckDownloadDatum(VBucket &datum);

    int QueryCloudData(const std::string &tableName, DownloadData &downloadData);

    int CheckTaskIdValid(TaskId taskId);

    int GetCurrentTableName(std::string &tableName);

    int TryToAddSyncTask(CloudTaskInfo &&taskInfo);

    int CheckQueueSizeWithNoLock();

    int PrepareSync(TaskId taskId);

    int LockCloud(TaskId taskId);

    int UnlockCloud();

    int StartHeartBeatTimer(int period, TaskId taskId);

    void FinishHeartBeatTimer();

    void WaitAllHeartBeatTaskExit();

    void HeartBeat(TimerId timerId, TaskId taskId);

    void HeartBeatFailed(TaskId taskId);

    void SetTaskFailed(TaskId taskId, int errCode);
    
    int SaveData(const TableName &tablename, DownloadData &downloadData,
        Info &downloadInfo, CloudWaterMark &latestCloudWaterMark, ChangedData &changedData);

    int SaveChangedData(DownloadData &downloadData,
        int dataIndex, DataInfoWithLog &localLogInfo, LogInfo &cloudLogInfo, ChangedData &changedData);

    int SaveDataNotifyProcess(CloudSyncer::TaskId taskId, const TableName &tableName,
        DownloadData &downloadData, InnerProcessInfo &info, const std::vector<std::string> &pkColNames);

    void NotifyInBatchUpload(const UploadParam &uploadParam, const InnerProcessInfo &innerProcessInfo);

    int NotifyChangedData(ChangedData &&changedData);

    static int CheckParamValid(const std::vector<DeviceID> &devices, SyncMode mode);

    std::mutex queueLock_;
    std::atomic<TaskId> currentTaskId_;
    std::list<TaskId> taskQueue_;
    std::map<TaskId, CloudTaskInfo> cloudTaskInfos_;

    std::mutex contextLock_;
    TaskContext currentContext_;
    std::condition_variable contextCv_;

    CloudDBProxy cloudDB_;

    std::shared_ptr<StorageProxy> storageProxy_;
    std::atomic<int32_t> queuedManualSyncLimit_;

    std::atomic<bool> closed_;

    std::atomic<TimerId> timerId_;
    std::mutex heartbeatMutex_;
    std::condition_variable heartbeatCv_;
    int32_t heartBeatCount_;
    std::atomic<int32_t> failedHeartBeatCount_;
};
}
#endif // CLOUD_SYNCER_H
