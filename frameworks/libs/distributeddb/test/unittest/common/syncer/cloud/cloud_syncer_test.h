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
#ifndef CLOUDSYNCER_TEST_H
#define CLOUDSYNCER_TEST_H

#include "cloud_merge_strategy.h"
#include "cloud_syncer.h"
#include "cloud_syncer_test.h"
#include "cloud_sync_utils.h"
#include "mock_iclouddb.h"

namespace DistributedDB {

const Asset ASSET_COPY = { .version = 1,
    .name = "Phone",
    .assetId = "0",
    .subpath = "/local/sync",
    .uri = "/local/sync",
    .modifyTime = "123456",
    .createTime = "",
    .size = "256",
    .hash = "ASE" };

class TestStorageProxy : public StorageProxy {
public:
    explicit TestStorageProxy(ICloudSyncStorageInterface *iCloud) : StorageProxy(iCloud)
    {
        Init();
    }
};

class TestCloudSyncer : public CloudSyncer {
public:
    explicit TestCloudSyncer(std::shared_ptr<DistributedDB::TestStorageProxy> storageProxy) : CloudSyncer(storageProxy)
    {
    }
    ~TestCloudSyncer() override = default;
    DISABLE_COPY_ASSIGN_MOVE(TestCloudSyncer);

    void InitCloudSyncer(TaskId taskId, SyncMode mode)
    {
        cloudTaskInfos_.insert(std::pair<TaskId, CloudTaskInfo>{taskId, CloudTaskInfo()});
        cloudTaskInfos_[taskId].mode = mode;
        cloudTaskInfos_[taskId].taskId = taskId;
        currentContext_.currentTaskId = taskId;
        currentContext_.tableName = "TestTable" + std::to_string(taskId);
        currentContext_.notifier = std::make_shared<ProcessNotifier>(this);
        currentContext_.notifier->Init({currentContext_.tableName}, { "cloud" });
        currentContext_.strategy = std::make_shared<CloudMergeStrategy>();
        closed_ = false;
        cloudTaskInfos_[taskId].callback = [this, taskId](const std::map<std::string, SyncProcess> &process) {
            if (process.size() >= 1u) {
                process_[taskId] = process.begin()->second;
            } else {
                SyncProcess tmpProcess;
                process_[taskId] = tmpProcess;
            }
        };
    }

    void SetCurrentCloudTaskInfos(std::vector<std::string> tables, const SyncProcessCallback &onProcess)
    {
        cloudTaskInfos_[currentContext_.currentTaskId].table = tables;
        cloudTaskInfos_[currentContext_.currentTaskId].callback = onProcess;
    }

    CloudTaskInfo GetCurrentCloudTaskInfos()
    {
        return cloudTaskInfos_[currentContext_.currentTaskId];
    }

    int CreateCloudTaskInfoAndCallTryToAddSync(SyncMode mode, const std::vector<std::string> &tables,
        const SyncProcessCallback &onProcess, int64_t waitTime)
    {
        CloudTaskInfo taskInfo;
        taskInfo.mode = mode;
        taskInfo.table = tables;
        taskInfo.callback = onProcess;
        taskInfo.timeout = waitTime;
        return TryToAddSyncTask(std::move(taskInfo));
    }

    void CallClose()
    {
        currentContext_.currentTaskId = 0u;
        currentContext_.strategy = nullptr;
        currentContext_.notifier = nullptr;
        Close();
    }
    void SetTimeOut(TaskId taskId, int64_t timeout)
    {
        this->cloudTaskInfos_[taskId].timeout = timeout;
    }

    void InitCloudSyncerForSync()
    {
        this->closed_ = false;
        this->cloudTaskInfos_[this->lastTaskId_].callback = [this](
            const std::map<std::string, SyncProcess> &process) {
            if (process.size() == 1u) {
                process_[this->lastTaskId_] = process.begin()->second;
            } else {
                SyncProcess tmpProcess;
                process_[this->lastTaskId_] = tmpProcess;
            }
        };
    }

    int CallDoSyncInner(const CloudTaskInfo &taskInfo, const bool &needUpload)
    {
        return DoSyncInner(taskInfo, needUpload, true);
    }

    SyncProcessCallback getCallback(TaskId taskId)
    {
        return cloudTaskInfos_[taskId].callback;
    }

    TaskId getCurrentTaskId()
    {
        return currentContext_.currentTaskId;
    }

    int CallDoUpload(TaskId taskId, bool lastTable = false, LockAction lockAction = LockAction::INSERT)
    {
        storageProxy_->StartTransaction();
        int ret = CloudSyncer::DoUpload(taskId, lastTable, lockAction);
        storageProxy_->Commit();
        return ret;
    }

    int CallDoDownload(TaskId taskId)
    {
        return CloudSyncer::DoDownload(taskId, true);
    }

    std::string GetCurrentContextTableName()
    {
        return this->currentContext_.tableName;
    }

    void SetCurrentContextTableName(std::string name)
    {
        this->currentContext_.tableName = name;
    }

    void CallClearCloudSyncData(CloudSyncData& uploadData)
    {
        CloudSyncUtils::ClearCloudSyncData(uploadData);
    }

    int32_t GetUploadSuccessCount(TaskId taskId)
    {
        return this->process_[taskId].tableProcess[this->GetCurrentContextTableName()].upLoadInfo.successCount;
    }

    int32_t GetUploadFailCount(TaskId taskId)
    {
        return this->process_[taskId].tableProcess[this->GetCurrentContextTableName()].upLoadInfo.failCount;
    }

    void SetMockICloudDB(MockICloudDB *icloudDB)
    {
        this->cloudDB_.SetCloudDB(std::shared_ptr<MockICloudDB>(icloudDB));
    }

    void SetMockICloudDB(std::shared_ptr<MockICloudDB> &icloudDB)
    {
        this->cloudDB_.SetCloudDB(icloudDB);
    }

    CloudTaskInfo SetAndGetCloudTaskInfo(SyncMode mode, std::vector<std::string> table,
        SyncProcessCallback callback, int64_t timeout)
    {
        CloudTaskInfo taskInfo;
        taskInfo.mode = mode;
        taskInfo.table = table;
        taskInfo.callback = callback;
        taskInfo.timeout = timeout;
        return taskInfo;
    }

    void initFullCloudSyncData(CloudSyncData &uploadData, int size)
    {
        VBucket tmp = { std::pair<std::string, int64_t>(CloudDbConstant::MODIFY_FIELD, 1),
                        std::pair<std::string, int64_t>(CloudDbConstant::CREATE_FIELD, 1),
                        std::pair<std::string, Asset>(CloudDbConstant::ASSET, ASSET_COPY) };
        VBucket asset = { std::pair<std::string, Asset>(CloudDbConstant::ASSET, ASSET_COPY) };
        uploadData.insData.record = std::vector<VBucket>(size, tmp);
        uploadData.insData.extend = std::vector<VBucket>(size, tmp);
        uploadData.insData.assets = std::vector<VBucket>(size, asset);
        uploadData.updData.record = std::vector<VBucket>(size, tmp);
        uploadData.updData.extend = std::vector<VBucket>(size, tmp);
        uploadData.updData.assets = std::vector<VBucket>(size, asset);
        uploadData.delData.record = std::vector<VBucket>(size, tmp);
        uploadData.delData.extend = std::vector<VBucket>(size, tmp);
    }

    int CallTryToAddSyncTask(CloudTaskInfo &&taskInfo)
    {
        return TryToAddSyncTask(std::move(taskInfo));
    }

    void PopTaskQueue()
    {
        taskQueue_.pop_back();
    }

    int CallPrepareSync(TaskId taskId)
    {
        return PrepareSync(taskId);
    }

    void CallNotify()
    {
        auto info = cloudTaskInfos_[currentContext_.currentTaskId];
        currentContext_.notifier->NotifyProcess(info, {});
    }

    void SetAssetFields(const TableName &tableName, const std::vector<Field> &assetFields)
    {
        currentContext_.tableName = tableName;
        currentContext_.assetFields[currentContext_.tableName] = assetFields;
    }

    std::map<std::string, Assets> TestTagAssetsInSingleRecord(
        VBucket &coveredData, VBucket &beCoveredData, bool setNormalStatus = false)
    {
        int ret = E_OK;
        return TagAssetsInSingleRecord(coveredData, beCoveredData, setNormalStatus, ret);
    }

    bool TestIsDataContainDuplicateAsset(std::vector<Field> &assetFields, VBucket &data)
    {
        return IsDataContainDuplicateAsset(assetFields, data);
    }

    void SetCloudWaterMarks(const TableName &tableName, const std::string &mark)
    {
        currentContext_.tableName = tableName;
        currentContext_.cloudWaterMarks[tableName] = mark;
    }

    int CallDownloadAssets()
    {
        InnerProcessInfo info;
        std::vector<std::string> pKColNames;
        std::set<Key> dupHashKeySet;
        ChangedData changedAssets;
        return CloudSyncer::DownloadAssets(info, pKColNames, dupHashKeySet, changedAssets);
    }

    void SetCurrentContext(TaskId taskId)
    {
        currentContext_.currentTaskId = taskId;
    }

    void SetLastTaskId(TaskId taskId)
    {
        lastTaskId_ = taskId;
    }

    void SetCurrentTaskPause()
    {
        cloudTaskInfos_[currentContext_.currentTaskId].pause = true;
    }

    void SetAssetDownloadList(int downloadCount)
    {
        for (int i = 0; i < downloadCount; ++i) {
            currentContext_.assetDownloadList.push_back({});
        }
    }

    void SetQuerySyncObject(TaskId taskId, const QuerySyncObject &query)
    {
        std::vector<QuerySyncObject> queryList;
        queryList.push_back(query);
        cloudTaskInfos_[taskId].queryList = queryList;
    }

    QuerySyncObject CallGetQuerySyncObject(const std::string &tableName)
    {
        return CloudSyncer::GetQuerySyncObject(tableName);
    }

    void CallReloadWaterMarkIfNeed(TaskId taskId, WaterMark &waterMark)
    {
        CloudSyncer::ReloadWaterMarkIfNeed(taskId, waterMark);
    }

    void CallRecordWaterMark(TaskId taskId, Timestamp waterMark)
    {
        CloudSyncer::RecordWaterMark(taskId, waterMark);
    }

    void SetResumeSyncParam(TaskId taskId, const SyncParam &syncParam)
    {
        resumeTaskInfos_[taskId].syncParam = syncParam;
        resumeTaskInfos_[taskId].context.tableName = syncParam.tableName;
    }

    void ClearResumeTaskInfo(TaskId taskId)
    {
        resumeTaskInfos_.erase(taskId);
    }

    void SetTaskResume(TaskId taskId, bool resume)
    {
        cloudTaskInfos_[taskId].resume = resume;
    }

    int CallGetSyncParamForDownload(TaskId taskId, SyncParam &param)
    {
        return CloudSyncer::GetSyncParamForDownload(taskId, param);
    }

    bool IsResumeTaskUpload(TaskId taskId)
    {
        return resumeTaskInfos_[taskId].upload;
    }
    CloudTaskInfo taskInfo_;
private:
    std::map<TaskId, SyncProcess> process_;
};

}
#endif // #define CLOUDSYNCER_TEST_H