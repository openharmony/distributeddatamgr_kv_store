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
#include "mock_iclouddb.h"

namespace DistributedDB {

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
        Close();
    }
    void SetTimeOut(TaskId taskId, int64_t timeout)
    {
        this->cloudTaskInfos_[taskId].timeout = timeout;
    }

    void InitCloudSyncerForSync()
    {
        this->closed_ = false;
        this->cloudTaskInfos_[this->currentTaskId_].callback = [this](
            const std::map<std::string, SyncProcess> &process) {
            if (process.size() == 1u) {
                process_[this->currentTaskId_] = process.begin()->second;
            } else {
                SyncProcess tmpProcess;
                process_[this->currentTaskId_] = tmpProcess;
            }
        };
    }

    int CallDoSyncInner(const CloudTaskInfo &taskInfo, const bool &needUpload)
    {
        return DoSyncInner(taskInfo, needUpload);
    }

    SyncProcessCallback getCallback(TaskId taskId)
    {
        return cloudTaskInfos_[taskId].callback;
    }

    TaskId getCurrentTaskId()
    {
        return currentContext_.currentTaskId;
    }
    
    int CallDoUpload(TaskId taskId, bool lastTable = false)
    {
        storageProxy_->StartTransaction();
        int ret = CloudSyncer::DoUpload(taskId, lastTable);
        storageProxy_->Commit();
        return ret;
    }

    int CallDoDownload(TaskId taskId)
    {
        return CloudSyncer::DoDownload(taskId);
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
        this->ClearCloudSyncData(uploadData);
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

    void CallDoFinished(TaskId taskId, int errCode, const InnerProcessInfo &processInfo)
    {
        DoFinished(taskId,  errCode,  processInfo);
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
        VBucket tmp = {std::pair<std::string, int64_t>(CloudDbConstant::MODIFY_FIELD, 1),
                       std::pair<std::string, int64_t>(CloudDbConstant::CREATE_FIELD, 1)};
        uploadData.insData.record = std::vector<VBucket>(size, tmp);
        uploadData.insData.extend = std::vector<VBucket>(size, tmp);
        uploadData.updData.record = std::vector<VBucket>(size, tmp);
        uploadData.updData.extend = std::vector<VBucket>(size, tmp);
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
        VBucket &CoveredData, VBucket &BeCoveredData, bool WriteToCoveredData = false)
    {
        return TagAssetsInSingleRecord(CoveredData, BeCoveredData, WriteToCoveredData);
    }

    void SetCloudWaterMarks(const TableName &tableName, const CloudWaterMark &mark)
    {
        currentContext_.tableName = tableName;
        currentContext_.cloudWaterMarks[tableName] = mark;
    }

    CloudTaskInfo taskInfo_;
private:
    std::map<TaskId, SyncProcess> process_;
};

}
#endif // #define CLOUDSYNCER_TEST_H