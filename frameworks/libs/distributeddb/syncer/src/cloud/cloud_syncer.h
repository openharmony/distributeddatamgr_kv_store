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

#include "cloud_db_proxy.h"
#include "cloud/cloud_store_types.h"
#include "cloud/cloud_sync_strategy.h"
#include "cloud/icloud_syncer.h"
#include "cloud/process_notifier.h"
#include "data_transformer.h"
#include "db_common.h"
#include "cloud/icloud_db.h"
#include "ref_object.h"
#include "runtime_context.h"
#include "storage_proxy.h"
#include "store_observer.h"

namespace DistributedDB {
using DownloadList = std::vector<std::tuple<std::string, Type, OpType, std::map<std::string, Assets>, Key,
    std::vector<Type>>>;
using DownloadCommitList = std::vector<std::tuple<std::string, std::map<std::string, Assets>, bool>>;
class CloudSyncer : public ICloudSyncer {
public:
    explicit CloudSyncer(std::shared_ptr<StorageProxy> storageProxy);
    ~CloudSyncer() override = default;
    DISABLE_COPY_ASSIGN_MOVE(CloudSyncer);

    int Sync(const std::vector<DeviceID> &devices, SyncMode mode, const std::vector<std::string> &tables,
        const SyncProcessCallback &callback, int64_t waitTime);

    void SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB);

    void SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader);

    int CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
        const RelationalSchemaObject &localSchema);

    int32_t GetCloudSyncTaskCount();

    void Close();

    void IncSyncCallbackTaskCount() override;

    void DecSyncCallbackTaskCount() override;

    std::string GetIdentify() const override;
protected:
    struct DataInfo {
        DataInfoWithLog localInfo;
        LogInfo cloudLogInfo;
    };
    struct WithoutRowIdData {
        std::vector<size_t> insertData = {};
        std::vector<std::tuple<size_t, size_t>> updateData = {};
        std::vector<std::tuple<size_t, size_t>> assetInsertData = {};
    };
    struct SyncParam {
        DownloadData downloadData;
        ChangedData changedData;
        InnerProcessInfo info;
        DownloadList assetsDownloadList;
        std::string cloudWaterMark;
        std::vector<std::string> pkColNames;
        std::set<Key> deletePrimaryKeySet;
        std::set<Key> dupHashKeySet;
        std::string tableName;
        bool isSinglePrimaryKey;
        bool isLastBatch = false;
        WithoutRowIdData withoutRowIdData;
    };
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

    int TriggerSync();

    void DoSyncIfNeed();

    int DoSync(TaskId taskId);

    int DoSyncInner(const CloudTaskInfo &taskInfo, const bool needUpload);

    int DoUploadInNeed(const CloudTaskInfo &taskInfo, const bool needUpload);

    void DoFinished(TaskId taskId, int errCode, const InnerProcessInfo &processInfo);

    virtual int DoDownload(CloudSyncer::TaskId taskId);

    int DoDownloadInner(CloudSyncer::TaskId taskId, SyncParam &param);

    void NotifyInEmptyDownload(CloudSyncer::TaskId taskId, InnerProcessInfo &info);

    int PreCheckUpload(TaskId &taskId, const TableName &tableName, Timestamp &localMark);

    int PreCheck(TaskId &taskId, const TableName &tableName);

    int DoBatchUpload(CloudSyncData &uploadData, UploadParam &uploadParam, InnerProcessInfo &innerProcessInfo);

    int CheckCloudSyncDataValid(CloudSyncData uploadData, const std::string &tableName, const int64_t &count,
        TaskId &taskId);

    static bool CheckCloudSyncDataEmpty(const CloudSyncData &uploadData);

    int GetWaterMarkAndUpdateTime(std::vector<VBucket>& extend, Timestamp &waterMark);

    int UpdateExtendTime(CloudSyncData &uploadData, const int64_t &count, TaskId taskId,
        Timestamp &waterMark);

    void ClearCloudSyncData(CloudSyncData &uploadData);

    int PreProcessBatchUpload(TaskId taskId, const InnerProcessInfo &innerProcessInfo,
        CloudSyncData &uploadData, Timestamp &localMark);

    int PutWaterMarkAfterBatchUpload(const std::string &tableName, UploadParam &uploadParam);

    virtual int DoUpload(CloudSyncer::TaskId taskId, bool lastTable);

    void SetUploadDataFlag(const TaskId taskId, CloudSyncData& uploadData);

    bool IsModeForcePush(const TaskId taskId);

    bool IsModeForcePull(const TaskId taskId);

    int DoUploadInner(const std::string &tableName, UploadParam &uploadParam);

    int PreHandleData(VBucket &datum, const std::vector<std::string> &pkColNames);

    int QueryCloudData(const std::string &tableName, std::string &cloudWaterMark, DownloadData &downloadData);

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

    void HeartBeatFailed(TaskId taskId, int errCode);

    void SetTaskFailed(TaskId taskId, int errCode);

    int SaveDatum(SyncParam &param, size_t idx, std::vector<std::pair<Key, size_t>> &deletedList,
        std::map<std::string, LogInfo> &localLogInfoCache);

    int SaveData(SyncParam &param);

    void NotifyInDownload(CloudSyncer::TaskId taskId, SyncParam &param);

    int SaveDataInTransaction(CloudSyncer::TaskId taskId,  SyncParam &param);

    int FindDeletedListIndex(const std::vector<std::pair<Key, size_t>> &deletedList, const Key &hashKey,
        size_t &delIdx);

    int SaveChangedData(SyncParam &param, size_t dataIndex, const DataInfo &dataInfo,
        std::vector<std::pair<Key, size_t>> &deletedList);

    int SaveDataNotifyProcess(CloudSyncer::TaskId taskId, SyncParam &param);

    void NotifyInBatchUpload(const UploadParam &uploadParam, const InnerProcessInfo &innerProcessInfo, bool lastBatch);

    bool NeedNotifyChangedData(const ChangedData &changedData);

    int NotifyChangedData(ChangedData &&changedData);

    std::map<std::string, Assets> GetAssetsFromVBucket(VBucket &data);

    std::map<std::string, Assets> TagAssetsInSingleRecord(VBucket &coveredData, VBucket &beCoveredData,
        bool setNormalStatus, int &errCode);

    int TagStatus(bool isExist, SyncParam &param, size_t idx, DataInfo &dataInfo, VBucket &localAssetInfo);

    int HandleTagAssets(const Key &hashKey, size_t idx, SyncParam &param, DataInfo &dataInfo, VBucket &localAssetInfo);

    int TagDownloadAssets(const Key &hashKey, size_t idx, SyncParam &param, DataInfo &dataInfo,
        VBucket &localAssetInfo);

    int TagUploadAssets(CloudSyncData &uploadData);

    int FillCloudAssets(const std::string &tableName, VBucket &normalAssets, VBucket &failedAssets);

    int HandleDownloadResult(const std::string &tableName, DownloadCommitList &commitList, uint32_t &successCount);

    int FillDownloadResult(const std::string &tableName, DownloadCommitList &commitList, uint32_t &successCount);

    int DownloadAssets(InnerProcessInfo &info, const std::vector<std::string> &pKColNames,
        const std::set<Key> &dupHashKeySet, ChangedData &changedAssets);

    int CloudDbDownloadAssets(InnerProcessInfo &info, const DownloadList &downloadList,
        const std::set<Key> &dupHashKeySet, ChangedData &changedAssets);

    void GetDownloadItem(const DownloadList &downloadList, size_t i, DownloadItem &downloadItem);

    bool IsDataContainAssets();

    void ModifyCloudDataTime(VBucket &data);

    int SaveCloudWaterMark(const TableName &tableName);

    bool IsDataContainDuplicateAsset(const std::vector<Field> &assetFields, VBucket &data);

    int UpdateChangedData(SyncParam &param, DownloadList &assetsDownloadList);

    void WaitAllSyncCallbackTaskFinish();

    void UpdateCloudWaterMark(const SyncParam &param);

    int TagStatusByStrategy(bool isExist, SyncParam &param, DataInfo &dataInfo, OpType &strategyOpResult);

    int CommitDownloadResult(InnerProcessInfo &info, DownloadCommitList &commitList);

    static int CheckParamValid(const std::vector<DeviceID> &devices, SyncMode mode);

    void ClearWithoutData(SyncParam &param);

    void ModifyFieldNameToLower(VBucket &data);

    int GetLocalInfo(const std::string &tableName, const VBucket &cloudData, DataInfoWithLog &logInfo,
        std::map<std::string, LogInfo> &localLogInfoCache, VBucket &localAssetInfo);

    void UpdateLocalCache(OpType opType, const LogInfo &cloudInfo,
        const LogInfo &localInfo, std::map<std::string, LogInfo> &localLogInfoCache);

    std::mutex queueLock_;
    TaskId currentTaskId_;
    std::list<TaskId> taskQueue_;
    std::map<TaskId, CloudTaskInfo> cloudTaskInfos_;

    std::mutex contextLock_;
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
};
}
#endif // CLOUD_SYNCER_H
