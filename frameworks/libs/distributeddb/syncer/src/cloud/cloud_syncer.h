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
#include <utility>

#include "cloud/cloud_store_types.h"
#include "cloud/cloud_sync_strategy.h"
#include "cloud/icloud_db.h"
#include "cloud/icloud_syncer.h"
#include "cloud/process_notifier.h"
#include "cloud_db_proxy.h"
#include "cloud_locker.h"
#include "data_transformer.h"
#include "db_common.h"
#include "ref_object.h"
#include "runtime_context.h"
#include "storage_proxy.h"
#include "store_observer.h"

namespace DistributedDB {
using DownloadCommitList = std::vector<std::tuple<std::string, std::map<std::string, Assets>, bool>>;
class CloudSyncer : public ICloudSyncer {
public:
    explicit CloudSyncer(std::shared_ptr<StorageProxy> storageProxy);
    ~CloudSyncer() override = default;
    DISABLE_COPY_ASSIGN_MOVE(CloudSyncer);

    int Sync(const std::vector<DeviceID> &devices, SyncMode mode, const std::vector<std::string> &tables,
        const SyncProcessCallback &callback, int64_t waitTime);

    int Sync(const CloudTaskInfo &taskInfo);

    void SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB);

    void SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader);

    int CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
        const RelationalSchemaObject &localSchema);

    int CleanWaterMarkInMemory(const std::set<std::string> &tableNameList);

    int32_t GetCloudSyncTaskCount();

    void Close();

    void IncSyncCallbackTaskCount() override;

    void DecSyncCallbackTaskCount() override;

    std::string GetIdentify() const override;
protected:
    struct TaskContext {
        TaskId currentTaskId = 0u;
        std::string tableName;
        std::shared_ptr<ProcessNotifier> notifier;
        std::shared_ptr<CloudSyncStrategy> strategy;
        std::map<TableName, std::vector<Field>> assetFields;
        // should be cleared after each Download
        DownloadList assetDownloadList;
        // store GID and assets, using in upload procedure
        std::map<TableName, std::map<std::string, std::map<std::string, Assets>>> assetsInfo;
        std::map<TableName, std::string> cloudWaterMarks;
        std::shared_ptr<CloudLocker> locker;
    };
    struct UploadParam {
        int64_t count = 0;
        TaskId taskId = 0u;
        Timestamp localMark = 0u;
        bool lastTable = false;
    };
    struct DownloadItem {
        std::string gid;
        Type prefix;
        OpType strategy;
        std::map<std::string, Assets> assets;
        Key hashKey;
        std::vector<Type> primaryKeyValList;
    };
    struct ResumeTaskInfo {
        TaskContext context;
        SyncParam syncParam;
        bool upload = false; // task pause when upload
        bool skipQuery = false; // task should skip query now
        size_t lastDownloadIndex = 0u;
        Timestamp lastLocalWatermark = 0u;
        int downloadStatus = E_OK;
    };

    int TriggerSync();

    void DoSyncIfNeed();

    int DoSync(TaskId taskId);

    int DoSyncInner(const CloudTaskInfo &taskInfo, const bool needUpload);

    int DoUploadInNeed(const CloudTaskInfo &taskInfo, const bool needUpload);

    void DoFinished(TaskId taskId, int errCode);

    virtual int DoDownload(CloudSyncer::TaskId taskId);

    int DoDownloadInner(CloudSyncer::TaskId taskId, SyncParam &param);

    void NotifyInEmptyDownload(CloudSyncer::TaskId taskId, InnerProcessInfo &info);

    int PreCheckUpload(TaskId &taskId, const TableName &tableName, Timestamp &localMark);

    int PreCheck(TaskId &taskId, const TableName &tableName);

    int SaveUploadData(Info &insertInfo, Info &updateInfo, Info &deleteInfo, CloudSyncData &uploadData,
        InnerProcessInfo &innerProcessInfo);

    int DoBatchUpload(CloudSyncData &uploadData, UploadParam &uploadParam, InnerProcessInfo &innerProcessInfo);

    int PreProcessBatchUpload(TaskId taskId, const InnerProcessInfo &innerProcessInfo,
        CloudSyncData &uploadData, Timestamp &localMark);

    int PutWaterMarkAfterBatchUpload(const std::string &tableName, UploadParam &uploadParam);

    virtual int DoUpload(CloudSyncer::TaskId taskId, bool lastTable);

    void SetUploadDataFlag(const TaskId taskId, CloudSyncData& uploadData);

    bool IsModeForcePush(const TaskId taskId);

    bool IsModeForcePull(const TaskId taskId);

    bool IsPriorityTask(TaskId taskId);

    int DoUploadInner(const std::string &tableName, UploadParam &uploadParam);

    int PreHandleData(VBucket &datum, const std::vector<std::string> &pkColNames);

    int QueryCloudData(TaskId taskId, const std::string &tableName, std::string &cloudWaterMark,
        DownloadData &downloadData);

    int CheckTaskIdValid(TaskId taskId);

    int GetCurrentTableName(std::string &tableName);

    int TryToAddSyncTask(CloudTaskInfo &&taskInfo);

    int CheckQueueSizeWithNoLock(bool priorityTask);

    int PrepareSync(TaskId taskId);

    int LockCloud(TaskId taskId);

    int UnlockCloud();

    int StartHeartBeatTimer(int period, TaskId taskId);

    void FinishHeartBeatTimer();

    void WaitAllHeartBeatTaskExit();

    void HeartBeat(TimerId timerId, TaskId taskId);

    void HeartBeatFailed(TaskId taskId, int errCode);

    void SetTaskFailed(TaskId taskId, int errCode);

    int SaveDatum(SyncParam &param, size_t idx, std::vector<std::pair<Key, size_t>> &deletedList,
        std::map<std::string, LogInfo> &localLogInfoCache);

    int SaveData(SyncParam &param);

    void NotifyInDownload(CloudSyncer::TaskId taskId, SyncParam &param);

    int SaveDataInTransaction(CloudSyncer::TaskId taskId,  SyncParam &param);

    int SaveChangedData(SyncParam &param, size_t dataIndex, const DataInfo &dataInfo,
        std::vector<std::pair<Key, size_t>> &deletedList);

    int DoDownloadAssets(bool skipSave, SyncParam &param);

    int SaveDataNotifyProcess(CloudSyncer::TaskId taskId, SyncParam &param);

    void NotifyInBatchUpload(const UploadParam &uploadParam, const InnerProcessInfo &innerProcessInfo, bool lastBatch);

    bool NeedNotifyChangedData(const ChangedData &changedData);

    int NotifyChangedData(ChangedData &&changedData);

    std::map<std::string, Assets> GetAssetsFromVBucket(VBucket &data);

    std::map<std::string, Assets> TagAssetsInSingleRecord(VBucket &coveredData, VBucket &beCoveredData,
        bool setNormalStatus, int &errCode);

    int TagStatus(bool isExist, SyncParam &param, size_t idx, DataInfo &dataInfo, VBucket &localAssetInfo);

    int HandleTagAssets(const Key &hashKey, const DataInfo &dataInfo, size_t idx, SyncParam &param,
        VBucket &localAssetInfo);

    int TagDownloadAssets(const Key &hashKey, size_t idx, SyncParam &param, DataInfo &dataInfo,
        VBucket &localAssetInfo);

    int TagUploadAssets(CloudSyncData &uploadData);

    int FillCloudAssets(const std::string &tableName, VBucket &normalAssets, VBucket &failedAssets);

    int HandleDownloadResult(const std::string &tableName, DownloadCommitList &commitList, uint32_t &successCount);

    int FillDownloadExtend(TaskId taskId, const std::string &tableName, const std::string &cloudWaterMark,
        VBucket &extend);

    int GetCloudGid(TaskId taskId, const std::string &tableName, QuerySyncObject &obj);

    int DownloadAssets(InnerProcessInfo &info, const std::vector<std::string> &pKColNames,
        const std::set<Key> &dupHashKeySet, ChangedData &changedAssets);

    int CloudDbDownloadAssets(TaskId taskId, InnerProcessInfo &info, const DownloadList &downloadList,
        const std::set<Key> &dupHashKeySet, ChangedData &changedAssets);

    void GetDownloadItem(const DownloadList &downloadList, size_t i, DownloadItem &downloadItem);

    bool IsDataContainAssets();

    int SaveCloudWaterMark(const TableName &tableName);

    bool IsDataContainDuplicateAsset(const std::vector<Field> &assetFields, VBucket &data);

    int UpdateChangedData(SyncParam &param, DownloadList &assetsDownloadList);

    void WaitAllSyncCallbackTaskFinish();

    void UpdateCloudWaterMark(TaskId taskId, const SyncParam &param);

    int TagStatusByStrategy(bool isExist, SyncParam &param, DataInfo &dataInfo, OpType &strategyOpResult);

    int CommitDownloadResult(InnerProcessInfo &info, DownloadCommitList &commitList);

    void ClearWithoutData(SyncParam &param);

    void ModifyFieldNameToLower(VBucket &data);

    int GetLocalInfo(size_t index, SyncParam &param, DataInfoWithLog &logInfo,
        std::map<std::string, LogInfo> &localLogInfoCache, VBucket &localAssetInfo);

    TaskId GetNextTaskId();

    void MarkCurrentTaskPausedIfNeed();

    void SetCurrentTaskFailedWithoutLock(int errCode);

    int LockCloudIfNeed(TaskId taskId);

    void UnlockIfNeed();

    void ClearCurrentContextWithoutLock();

    void ClearContextAndNotify(TaskId taskId, int errCode);

    int DownloadOneBatch(TaskId taskId, SyncParam &param);

    int DownloadOneAssetRecord(const std::set<Key> &dupHashKeySet, const DownloadList &downloadList,
        DownloadItem &downloadItem, InnerProcessInfo &info, ChangedData &changedAssets);

    int GetSyncParamForDownload(TaskId taskId, SyncParam &param);

    bool IsCurrentTableResume(TaskId taskId, bool upload);

    int DownloadDataFromCloud(TaskId taskId, SyncParam &param, bool &abort);

    size_t GetDownloadAssetIndex(TaskId taskId);

    size_t GetStartTableIndex(TaskId taskId, bool upload);

    uint32_t GetCurrentTableUploadBatchIndex();

    void RecordWaterMark(TaskId taskId, Timestamp waterMark);

    Timestamp GetResumeWaterMark(TaskId taskId);

    void ReloadWaterMarkIfNeed(TaskId taskId, WaterMark &waterMark);

    void ReloadUploadInfoIfNeed(TaskId taskId, const UploadParam &param, InnerProcessInfo &info);

    uint32_t GetLastUploadSuccessCount(const std::string &tableName);

    QuerySyncObject GetQuerySyncObject(const std::string &tableName);

    InnerProcessInfo GetInnerProcessInfo(const std::string &tableName, UploadParam &uploadParam);

    void NotifyUploadFailed(int errCode, InnerProcessInfo &info);

    int BatchInsert(Info &insertInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo);

    int BatchUpdate(Info &updateInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo);

    std::mutex dataLock_;
    TaskId lastTaskId_;
    std::list<TaskId> taskQueue_;
    std::list<TaskId> priorityTaskQueue_;
    std::map<TaskId, CloudTaskInfo> cloudTaskInfos_;
    std::map<TaskId, ResumeTaskInfo> resumeTaskInfos_;

    TaskContext currentContext_;
    std::condition_variable contextCv_;
    std::mutex syncMutex_;  // Clean Cloud Data and Sync are mutually exclusive

    CloudDBProxy cloudDB_;

    std::shared_ptr<StorageProxy> storageProxy_;
    std::atomic<int32_t> queuedManualSyncLimit_;

    std::atomic<bool> closed_;

    std::atomic<TimerId> timerId_;
    std::mutex heartbeatMutex_;
    std::condition_variable heartbeatCv_;
    int32_t heartBeatCount_;
    std::atomic<int32_t> failedHeartBeatCount_;

    std::mutex syncCallbackMutex_;
    std::condition_variable syncCallbackCv_;
    int32_t syncCallbackCount_;

    std::string id_;

    static constexpr const TaskId INVALID_TASK_ID = 0u;
    static constexpr const int MAX_HEARTBEAT_FAILED_LIMIT = 2;
    static constexpr const int HEARTBEAT_PERIOD = 3;
    static constexpr const int MAX_DOWNLOAD_COMMIT_LIMIT = 1;
};
}
#endif // CLOUD_SYNCER_H
