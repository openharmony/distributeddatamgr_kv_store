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

#include "cloud/cloud_db_proxy.h"
#include "cloud/cloud_store_types.h"
#include "cloud/cloud_sync_state_machine.h"
#include "cloud/icloud_db.h"
#include "cloud/icloud_syncer.h"
#include "cloud/process_notifier.h"
#include "cloud/process_recorder.h"
#include "cloud/strategy_proxy.h"
#include "cloud_locker.h"
#include "data_transformer.h"
#include "db_common.h"
#include "ref_object.h"
#include "runtime_context.h"
#include "storage_proxy.h"
#include "store_observer.h"

namespace DistributedDB {
using DownloadCommitList = std::vector<std::tuple<std::string, std::map<std::string, Assets>, bool>>;
const std::string CLOUD_PRIORITY_TASK_STRING = "priority";
const std::string CLOUD_COMMON_TASK_STRING = "common";
class CloudSyncer : public ICloudSyncer {
public:
    explicit CloudSyncer(std::shared_ptr<StorageProxy> storageProxy, bool isKvScene = false,
        SingleVerConflictResolvePolicy policy = SingleVerConflictResolvePolicy::DEFAULT_LAST_WIN);
    void InitCloudSyncStateMachine();
    ~CloudSyncer() override = default;
    DISABLE_COPY_ASSIGN_MOVE(CloudSyncer);

    int Sync(const std::vector<DeviceID> &devices, SyncMode mode, const std::vector<std::string> &tables,
        const SyncProcessCallback &callback, int64_t waitTime);

    int Sync(const CloudTaskInfo &taskInfo);

    void SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB);

    void SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader);

    int CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
        const RelationalSchemaObject &localSchema);

    int ClearCloudWatermark(const std::vector<std::string> &tableNameList);

    int StopSyncTask(const std::function<int(void)> &removeFunc, int errCode = -E_CLOUD_ERROR);

    int CleanWaterMarkInMemory(const std::set<std::string> &tableNameList);

    int32_t GetCloudSyncTaskCount();

    void Close();

    void StopAllTasks(int errCode = -E_USER_CHANGE);

    std::string GetIdentify() const override;

    bool IsClosed() const override;

    void GenerateCompensatedSync(CloudTaskInfo &taskInfo);

    int SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs);

    const std::map<std::string, std::shared_ptr<ICloudDb>> GetCloudDB() const;

    void CleanAllWaterMark();

    CloudSyncEvent SyncMachineDoDownload();

    CloudSyncEvent SyncMachineDoUpload();

    CloudSyncEvent SyncMachineDoFinished();

    void SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback);

    SyncProcess GetCloudTaskStatus(uint64_t taskId) const;

    int ClearCloudWatermark(std::function<int(void)> &clearFunc);

    void SetCloudConflictHandler(const std::shared_ptr<ICloudConflictHandler> &handler);
protected:
    struct TaskContext {
        TaskId currentTaskId = 0u;
        bool isNeedUpload = false;  // whether the current task need to do upload
        bool isRealNeedUpload = false;
        bool isFirstDownload = false;
        int currentUserIndex = 0;
        int repeatCount = 0;
        std::string tableName;
        std::shared_ptr<ProcessNotifier> notifier;
        std::shared_ptr<ProcessRecorder> processRecorder;
        std::map<TableName, std::vector<Field>> assetFields;
        // store GID and assets, using in upload procedure
        std::map<TableName, std::map<std::string, std::map<std::string, Assets>>> assetsInfo;
        // struct: <currentUserIndex, <tableName, waterMark>>
        std::map<int, std::map<TableName, std::string>> cloudWaterMarks;
        std::shared_ptr<CloudLocker> locker;
        // should be cleared after each Download
        DownloadList assetDownloadList;
    };
    struct UploadParam {
        int64_t count = 0;
        TaskId taskId = 0u;
        Timestamp localMark = 0u;
        bool lastTable = false;
        CloudWaterType mode = CloudWaterType::DELETE;
        LockAction lockAction = LockAction::INSERT;
    };
    struct DownloadItem {
        std::string gid;
        Type prefix;
        OpType strategy;
        std::map<std::string, Assets> assets;
        Key hashKey;
        std::vector<Type> primaryKeyValList;
        Timestamp timestamp;
        bool recordConflict = false;
    };
    struct ResumeTaskInfo {
        TaskContext context;
        SyncParam syncParam;
        bool upload = false; // task pause when upload
        size_t lastDownloadIndex = 0u;
        Timestamp lastLocalWatermark = 0u;
        int downloadStatus = E_OK;
    };
    struct DownloadItems {
        DownloadItem downloadItem;
        std::map<std::string, Assets> assetsToRemove;
        std::map<std::string, Assets> assetsToDownload;
        std::map<std::string, std::vector<uint32_t>> flags;
    };

    int TriggerSync();

    void DoSyncIfNeed();

    int DoSync(TaskId taskId);

    std::pair<int, bool> DoFirstDownload(TaskId taskId, const CloudTaskInfo &taskInfo, bool needUpload,
        bool &isFirstDownload);

    int PrepareAndUpload(const CloudTaskInfo &taskInfo, size_t index);

    int DoSyncInner(const CloudTaskInfo &taskInfo);

    int DoUploadInNeed(const CloudTaskInfo &taskInfo, const bool needUpload);

    void DoNotifyInNeed(const CloudSyncer::TaskId &taskId, const std::vector<std::string> &needNotifyTables,
        const bool isFirstDownload);

    int GetUploadCountByTable(const CloudSyncer::TaskId &taskId, int64_t &count);

    void UpdateProcessInfoWithoutUpload(CloudSyncer::TaskId taskId, const std::string &tableName, bool needNotify);

    virtual int DoDownloadInNeed(const CloudTaskInfo &taskInfo, bool needUpload, bool isFirstDownload);

    void SetNeedUpload(bool isNeedUpload);

    void DoFinished(TaskId taskId, int errCode);

    virtual int DoDownload(CloudSyncer::TaskId taskId, bool isFirstDownload);

    int DoDownloadInner(CloudSyncer::TaskId taskId, SyncParam &param, bool isFirstDownload);

    void NotifyInEmptyDownload(CloudSyncer::TaskId taskId, InnerProcessInfo &info);

    int PreCheckUpload(TaskId &taskId, const TableName &tableName, Timestamp &localMark);

    int PreCheck(TaskId &taskId, const TableName &tableName);

    int SaveUploadData(Info &insertInfo, Info &updateInfo, Info &deleteInfo, CloudSyncData &uploadData,
        InnerProcessInfo &innerProcessInfo);

    int DoBatchUpload(CloudSyncData &uploadData, UploadParam &uploadParam, InnerProcessInfo &innerProcessInfo);

    int PreProcessBatchUpload(UploadParam &uploadParam, const InnerProcessInfo &innerProcessInfo,
        CloudSyncData &uploadData);

    int PutWaterMarkAfterBatchUpload(const std::string &tableName, UploadParam &uploadParam);

    virtual int DoUpload(CloudSyncer::TaskId taskId, bool lastTable, LockAction lockAction);

    void SetUploadDataFlag(const TaskId taskId, CloudSyncData& uploadData);

    bool IsModeForcePush(TaskId taskId);

    bool IsModeForcePull(const TaskId taskId);

    bool IsPriorityTask(TaskId taskId);

    bool IsCompensatedTask(TaskId taskId);

    int DoUploadInner(const std::string &tableName, UploadParam &uploadParam);

    int DoUploadByMode(const std::string &tableName, UploadParam &uploadParam, InnerProcessInfo &info);

    int PreHandleData(VBucket &datum, const std::vector<std::string> &pkColNames);

    int QueryCloudData(TaskId taskId, const std::string &tableName, std::string &cloudWaterMark,
        DownloadData &downloadData);

    size_t GetCurrentCommonTaskNum();

    int CheckTaskIdValid(TaskId taskId);

    int GetCurrentTableName(std::string &tableName);

    int TryToAddSyncTask(CloudTaskInfo &&taskInfo);

    int CheckQueueSizeWithNoLock(bool priorityTask);

    int PrepareSync(TaskId taskId);

    int LockCloud(TaskId taskId);

    int UnlockCloud();

    int StartHeartBeatTimer(int period, TaskId taskId);

    void FinishHeartBeatTimer();

    void HeartBeat(TimerId timerId, TaskId taskId);

    void HeartBeatFailed(TaskId taskId, int errCode);

    void SetTaskFailed(TaskId taskId, int errCode);

    int SaveDatum(SyncParam &param, size_t idx, std::vector<std::pair<Key, size_t>> &deletedList,
        std::map<std::string, LogInfo> &localLogInfoCache, std::vector<VBucket> &localInfo);

    int SaveData(CloudSyncer::TaskId taskId, SyncParam &param);

    void NotifyInDownload(CloudSyncer::TaskId taskId, SyncParam &param, bool isFirstDownload);

    int SaveDataInTransaction(CloudSyncer::TaskId taskId,  SyncParam &param);

    int DoDownloadAssets(SyncParam &param);

    int SaveDataNotifyProcess(CloudSyncer::TaskId taskId, SyncParam &param, bool downloadAssetOnly);

    void NotifyInBatchUpload(const UploadParam &uploadParam, const InnerProcessInfo &innerProcessInfo, bool lastBatch);

    bool NeedNotifyChangedData(const ChangedData &changedData);

    int NotifyChangedDataInCurrentTask(ChangedData &&changedData);

    std::map<std::string, Assets> TagAssetsInSingleRecord(VBucket &coveredData, VBucket &beCoveredData,
        bool setNormalStatus, bool isForcePullAseets, int &errCode);

    int TagStatus(bool isExist, SyncParam &param, size_t idx, DataInfo &dataInfo, VBucket &localAssetInfo);

    int HandleTagAssets(const Key &hashKey, const DataInfo &dataInfo, size_t idx, SyncParam &param,
        VBucket &localAssetInfo);

    int TagDownloadAssets(const Key &hashKey, size_t idx, SyncParam &param, const DataInfo &dataInfo,
        VBucket &localAssetInfo);

    int TagDownloadAssetsForAssetOnly(
        const Key &hashKey, size_t idx, SyncParam &param, const DataInfo &dataInfo, VBucket &localAssetInfo);

    void TagUploadAssets(CloudSyncData &uploadData);

    int FillCloudAssets(InnerProcessInfo &info, VBucket &normalAssets, VBucket &failedAssets);

    int HandleDownloadResult(const DownloadItem &downloadItem, InnerProcessInfo &info,
        DownloadCommitList &commitList, uint32_t &successCount);

    int HandleDownloadResultForAsyncDownload(const DownloadItem &downloadItem, InnerProcessInfo &info,
    DownloadCommitList &commitList, uint32_t &successCount);

    int FillDownloadExtend(TaskId taskId, const std::string &tableName, const std::string &cloudWaterMark,
        VBucket &extend);

    int GetCloudGid(TaskId taskId, const std::string &tableName, QuerySyncObject &obj);

    int GetCloudGid(
        TaskId taskId, const std::string &tableName, QuerySyncObject &obj, std::vector<std::string> &cloudGid);

    int DownloadAssets(InnerProcessInfo &info, const std::vector<std::string> &pKColNames,
        const std::set<Key> &dupHashKeySet, ChangedData &changedAssets);

    int CloudDbDownloadAssets(TaskId taskId, InnerProcessInfo &info, const DownloadList &downloadList,
        const std::set<Key> &dupHashKeySet, ChangedData &changedAssets);

    void GetDownloadItem(const DownloadList &downloadList, size_t i, DownloadItem &downloadItem);

    bool IsDataContainAssets();

    int SaveCloudWaterMark(const TableName &tableName, const TaskId taskId);

    bool IsDataContainDuplicateAsset(const std::vector<Field> &assetFields, VBucket &data);

    int UpdateChangedData(SyncParam &param, DownloadList &assetsDownloadList);

    void UpdateCloudWaterMark(TaskId taskId, const SyncParam &param);

    int CommitDownloadResult(const DownloadItem &downloadItem, InnerProcessInfo &info,
        DownloadCommitList &commitList, int errCode);

    int GetLocalInfo(size_t index, SyncParam &param, DataInfoWithLog &logInfo,
        std::map<std::string, LogInfo> &localLogInfoCache, VBucket &localAssetInfo);

    TaskId GetNextTaskId();

    void MarkCurrentTaskPausedIfNeed(const CloudTaskInfo &taskInfo);

    void SetCurrentTaskFailedWithoutLock(int errCode);

    int LockCloudIfNeed(TaskId taskId);

    void UnlockIfNeed();

    void ClearCurrentContextWithoutLock();

    void ClearContextAndNotify(TaskId taskId, int errCode);

    int DownloadOneBatch(TaskId taskId, SyncParam &param, bool isFirstDownload);

    int DownloadOneAssetRecord(const std::set<Key> &dupHashKeySet, const DownloadList &downloadList,
        DownloadItem &downloadItem, InnerProcessInfo &info, ChangedData &changedAssets);

    int GetSyncParamForDownload(TaskId taskId, SyncParam &param);

    bool IsCurrentTaskResume(TaskId taskId);

    bool IsCurrentTableResume(TaskId taskId, bool upload);

    int DownloadDataFromCloud(TaskId taskId, SyncParam &param, bool isFirstDownload);

    size_t GetDownloadAssetIndex(TaskId taskId);

    uint32_t GetCurrentTableUploadBatchIndex();

    void ResetCurrentTableUploadBatchIndex();

    void RecordWaterMark(TaskId taskId, Timestamp waterMark);

    Timestamp GetResumeWaterMark(TaskId taskId);

    void ReloadWaterMarkIfNeed(TaskId taskId, WaterMark &waterMark);

    void ReloadCloudWaterMarkIfNeed(const std::string &tableName, std::string &cloudWaterMark);

    void ReloadUploadInfoIfNeed(const UploadParam &param, InnerProcessInfo &info);

    void GetLastUploadInfo(const std::string &tableName, Info &lastUploadInfo, UploadRetryInfo &retryInfo);

    QuerySyncObject GetQuerySyncObject(const std::string &tableName);

    InnerProcessInfo GetInnerProcessInfo(const std::string &tableName, UploadParam &uploadParam);

    void NotifyUploadFailed(int errCode, InnerProcessInfo &info);

    void UpdateProcessWhenUploadFailed(InnerProcessInfo &info);

    int BatchInsert(Info &insertInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo);

    int BatchUpdate(Info &updateInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo);

    int BatchInsertOrUpdate(Info &uploadInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo,
        bool isInsert);

    int BackFillAfterBatchUpload(CloudSyncData &uploadData, bool isInsert, int batchUploadResult);

    int BatchDelete(Info &deleteInfo, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo);

    int DownloadAssetsOneByOne(const InnerProcessInfo &info, DownloadItem &downloadItem,
        std::map<std::string, Assets> &downloadAssets);

    std::pair<int, uint32_t> GetDBAssets(bool isSharedTable, const InnerProcessInfo &info,
        const DownloadItem &downloadItem, VBucket &dbAssets);

    std::map<std::string, Assets>& BackFillAssetsAfterDownload(int downloadCode, int deleteCode,
        std::map<std::string, std::vector<uint32_t>> &tmpFlags, std::map<std::string, Assets> &tmpAssetsToDownload,
        std::map<std::string, Assets> &tmpAssetsToDelete);

    int IsNeedSkipDownload(bool isSharedTable, int &errCode, const InnerProcessInfo &info,
        const DownloadItem &downloadItem, VBucket &dbAssets);

    bool CheckDownloadOrDeleteCode(int &errCode, int downloadCode, int deleteCode, DownloadItem &downloadItem);

    int DownloadAssetsOneByOneInner(bool isSharedTable, const InnerProcessInfo &info, DownloadItem &downloadItem,
        std::map<std::string, Assets> &downloadAssets);

    int CommitDownloadAssets(const DownloadItem &downloadItem, InnerProcessInfo &info,
        DownloadCommitList &commitList, uint32_t &successCount);

    int CommitDownloadAssetsForAsyncDownload(const DownloadItem &downloadItem, InnerProcessInfo &info,
        DownloadCommitList &commitList, uint32_t &successCount);

    void SeparateNormalAndFailAssets(const std::map<std::string, Assets> &assetsMap, VBucket &normalAssets,
        VBucket &failedAssets);

    void ChkIgnoredProcess(InnerProcessInfo &info, const CloudSyncData &uploadData, UploadParam &uploadParam);

    int SaveCursorIfNeed(const std::string &tableName);

    int PrepareAndDownload(const std::string &table, const CloudTaskInfo &taskInfo, bool isFirstDownload);

    int UpdateFlagForSavedRecord(const SyncParam &param);

    bool IsNeedGetLocalWater(TaskId taskId);

    bool IsNeedProcessCloudCursor(TaskId taskId);

    void SetProxyUser(const std::string &user);

    void MergeTaskInfo(const std::shared_ptr<DataBaseSchema> &cloudSchema, TaskId taskId);

    void RemoveTaskFromQueue(int32_t priorityLevel, TaskId taskId);

    std::pair<bool, TaskId> TryMergeTask(const std::shared_ptr<DataBaseSchema> &cloudSchema, TaskId tryTaskId);

    bool IsTaskCanMerge(const CloudTaskInfo &taskInfo);

    bool IsTasksCanMerge(TaskId taskId, TaskId tryMergeTaskId);

    bool MergeTaskTablesIfConsistent(TaskId sourceId, TaskId targetId);

    void AdjustTableBasedOnSchema(const std::shared_ptr<DataBaseSchema> &cloudSchema, CloudTaskInfo &taskInfo);

    std::pair<TaskId, TaskId> SwapTwoTaskAndCopyTable(TaskId source, TaskId target);

    bool IsQueryListEmpty(TaskId taskId);

    std::pair<int, Timestamp> GetLocalWater(const std::string &tableName, UploadParam &uploadParam);

    int HandleBatchUpload(UploadParam &uploadParam, InnerProcessInfo &info, CloudSyncData &uploadData,
        ContinueToken &continueStmtToken, std::vector<ReviseModTimeInfo> &revisedData);

    bool IsNeedLock(const UploadParam &param);

    int UploadVersionRecordIfNeed(const UploadParam &uploadParam);

    std::vector<CloudTaskInfo> CopyAndClearTaskInfos(const std::optional<TaskId> taskId = {});

    void WaitCurTaskFinished();

    bool IsLockInDownload();

    CloudSyncEvent SetCurrentTaskFailedInMachine(int errCode);

    CloudSyncEvent SyncMachineDoRepeatCheck();

    void MarkDownloadFinishIfNeed(const std::string &downloadTable, bool isFinish = true);

    bool IsTableFinishInUpload(const std::string &table);

    void MarkUploadFinishIfNeed(const std::string &table);

    int GenerateTaskIdIfNeed(CloudTaskInfo &taskInfo);

    void ProcessVersionConflictInfo(InnerProcessInfo &innerProcessInfo, uint32_t retryCount);

    std::string GetStoreIdByTask(TaskId taskId);

    int CloudDbBatchDownloadAssets(TaskId taskId, const DownloadList &downloadList, const std::set<Key> &dupHashKeySet,
        InnerProcessInfo &info, ChangedData &changedAssets);

    void FillDownloadItem(const std::set<Key> &dupHashKeySet, const DownloadList &downloadList,
        const InnerProcessInfo &info, bool isSharedTable, DownloadItems &record);

    using DownloadItemRecords = std::vector<DownloadItems>;
    using RemoveAssetsRecords = std::vector<IAssetLoader::AssetRecord>;
    using DownloadAssetsRecords = std::vector<IAssetLoader::AssetRecord>;
    using DownloadAssetDetail = std::tuple<DownloadItemRecords, RemoveAssetsRecords, DownloadAssetsRecords>;
    DownloadAssetDetail GetDownloadRecords(const DownloadList &downloadList, const std::set<Key> &dupHashKeySet,
        bool isSharedTable, bool isAsyncDownloadAssets, const InnerProcessInfo &info);

    int BatchDownloadAndCommitRes(const DownloadList &downloadList, const std::set<Key> &dupHashKeySet,
        InnerProcessInfo &info, ChangedData &changedAssets,
        std::tuple<DownloadItemRecords, RemoveAssetsRecords, DownloadAssetsRecords, bool> &downloadDetail);

    static void StatisticDownloadRes(const IAssetLoader::AssetRecord &downloadRecord,
        const IAssetLoader::AssetRecord &removeRecord, InnerProcessInfo &info, DownloadItem &downloadItem);

    static void AddNotifyDataFromDownloadAssets(const std::set<Key> &dupHashKeySet, DownloadItem &downloadItem,
        ChangedData &changedAssets);

    void CheckDataAfterDownload(const std::string &tableName);

    bool IsAsyncDownloadAssets(TaskId taskId);

    void TriggerAsyncDownloadAssetsInTaskIfNeed(bool isFirstDownload);

    void TriggerAsyncDownloadAssetsIfNeed();

    void BackgroundDownloadAssetsTask();

    void CancelDownloadListener();

    void DoBackgroundDownloadAssets();

    void CancelBackgroundDownloadAssetsTaskIfNeed();

    void CancelBackgroundDownloadAssetsTask(bool cancelDownload = true);

    int BackgroundDownloadAssetsByTable(const std::string &table, std::map<std::string, int64_t> &downloadBeginTime);

    int CheckCloudQueryAssetsOnlyIfNeed(TaskId taskId, SyncParam &param);

    int CheckLocalQueryAssetsOnlyIfNeed(VBucket &localAssetInfo, SyncParam &param, DataInfoWithLog &logInfo);

    int PutCloudSyncDataOrUpdateStatusForAssetOnly(SyncParam &param, std::vector<VBucket> &localInfo);

    bool IsCurrentAsyncDownloadTask();

    int GetCloudGidAndFillExtend(TaskId taskId, const std::string &tableName, QuerySyncObject &obj, VBucket &extend);

    int QueryCloudGidForAssetsOnly(
        TaskId taskId, SyncParam &param, int64_t groupIdx, std::vector<std::string> &cloudGid);

    int GetEmptyGidAssetsMapFromDownloadData(
        const std::vector<VBucket> &data, std::map<std::string, AssetsMap> &gidAssetsMap);

    int SetAssetsMapAndEraseDataForAssetsOnly(TaskId taskId, SyncParam &param, std::vector<VBucket> &downloadData,
        std::map<std::string, AssetsMap> &gidAssetsMap);

    void NotifyChangedDataWithDefaultDev(ChangedData &&changedData);

    bool IsAlreadyHaveCompensatedSyncTask();

    bool TryToInitQueryAndUserListForCompensatedSync(TaskId triggerTaskId);

    int FillCloudAssetsForOneRecord(const std::string &gid, const std::map<std::string, Assets> &assetsMap,
        InnerProcessInfo &info, bool setAllNormal, bool &isExistAssetDownloadFail);

    int UpdateRecordFlagForOneRecord(const std::string &gid, const DownloadItem &downloadItem, InnerProcessInfo &info,
        bool isExistAssetDownloadFail);

    static void ModifyDownLoadInfoCount(const int errorCode, InnerProcessInfo &info);

    void ChangeProcessStatusAndNotifyIfNeed(UploadParam &uploadParam, InnerProcessInfo &info);

    void ExecuteAsyncDownloadAssets(TaskId taskId);

    bool IsCloudForcePush(TaskId taskId);

    TaskId GetCurrentTaskId();

    int32_t GetHeatbeatCount(TaskId taskId);

    void RemoveHeatbeatData(TaskId taskId);

    void ExecuteHeartBeatTask(TaskId taskId);

    int WaitAsyncGenLogTaskFinished(TaskId triggerTaskId);

    void RetainCurrentTaskInfo(TaskId taskId);

    void SetCurrentTmpError(int errCode);

    mutable std::mutex dataLock_;
    TaskId lastTaskId_;
    std::multimap<int, TaskId, std::greater<int>> taskQueue_;
    std::map<TaskId, CloudTaskInfo> cloudTaskInfos_;
    std::map<TaskId, ResumeTaskInfo> resumeTaskInfos_;

    TaskContext currentContext_;
    std::condition_variable contextCv_;
    std::mutex syncMutex_;  // Clean Cloud Data and Sync are mutually exclusive

    CloudDBProxy cloudDB_;
    StrategyProxy strategyProxy_;

    std::shared_ptr<StorageProxy> storageProxy_;
    std::atomic<int32_t> queuedManualSyncLimit_;

    std::atomic<bool> closed_;
    std::atomic<bool> hasKvRemoveTask;
    std::atomic<TimerId> timerId_;
    std::mutex heartbeatMutex_;
    std::condition_variable heartbeatCv_;
    std::map<TaskId, int32_t> heartbeatCount_;
    std::map<TaskId, int32_t> failedHeartbeatCount_;

    std::string id_;

    // isKvScene_ is used to distinguish between the KV and RDB in the following scenarios:
    // 1. Whether upload to the cloud after delete local data that does not have a gid.
    // 2. Whether the local data need update for different flag when the local time is larger.
    bool isKvScene_;
    std::atomic<SingleVerConflictResolvePolicy> policy_;
    std::condition_variable asyncTaskCv_;
    TaskId asyncTaskId_;
    std::atomic<bool> cancelAsyncTask_;
    std::atomic<int> scheduleTaskCount_;
    std::mutex listenerMutex_;
    NotificationChain::Listener *waitDownloadListener_;

    static constexpr const TaskId INVALID_TASK_ID = 0u;
    static constexpr const int MAX_HEARTBEAT_FAILED_LIMIT = 2;
    static constexpr const int HEARTBEAT_PERIOD = 3;

    CloudSyncStateMachine cloudSyncStateMachine_;
};
}
#endif // CLOUD_SYNCER_H
