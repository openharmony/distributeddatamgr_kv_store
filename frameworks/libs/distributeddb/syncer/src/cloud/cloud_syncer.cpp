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
#include "cloud_syncer.h"

#include <cstdint>
#include <utility>
#include <unordered_map>

#include "cloud/asset_operation_utils.h"
#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "cloud/icloud_db.h"
#include "cloud_sync_tag_assets.h"
#include "cloud_sync_utils.h"
#include "db_errno.h"
#include "log_print.h"
#include "runtime_context.h"
#include "storage_proxy.h"
#include "store_types.h"
#include "strategy_factory.h"
#include "version.h"

namespace DistributedDB {
CloudSyncer::CloudSyncer(std::shared_ptr<StorageProxy> storageProxy, SingleVerConflictResolvePolicy policy)
    : lastTaskId_(INVALID_TASK_ID),
      storageProxy_(std::move(storageProxy)),
      queuedManualSyncLimit_(DBConstant::QUEUED_SYNC_LIMIT_DEFAULT),
      closed_(false),
      timerId_(0u),
      heartBeatCount_(0),
      failedHeartBeatCount_(0),
      policy_(policy)
{
    if (storageProxy_ != nullptr) {
        id_ = storageProxy_->GetIdentify();
    }
    InitCloudSyncStateMachine();
}

void CloudSyncer::InitCloudSyncStateMachine()
{
    cloudSyncStateMachine_.Initialize();
    cloudSyncStateMachine_.RegisterFunc(CloudSyncState::DO_DOWNLOAD, [this]() {
        return SyncMachineDoDownload();
    });
    cloudSyncStateMachine_.RegisterFunc(CloudSyncState::DO_UPLOAD, [this]() {
        return SyncMachineDoUpload();
    });
    cloudSyncStateMachine_.RegisterFunc(CloudSyncState::DO_FINISHED, [this]() {
        return SyncMachineDoFinished();
    });
}

int CloudSyncer::Sync(const std::vector<DeviceID> &devices, SyncMode mode,
    const std::vector<std::string> &tables, const SyncProcessCallback &callback, int64_t waitTime)
{
    CloudTaskInfo taskInfo;
    taskInfo.mode = mode;
    taskInfo.table = tables;
    taskInfo.callback = callback;
    taskInfo.timeout = waitTime;
    taskInfo.devices = devices;
    for (const auto &item: tables) {
        QuerySyncObject syncObject;
        syncObject.SetTableName(item);
        taskInfo.queryList.push_back(syncObject);
    }
    return Sync(taskInfo);
}

int CloudSyncer::Sync(const CloudTaskInfo &taskInfo)
{
    int errCode = CloudSyncUtils::CheckParamValid(taskInfo.devices, taskInfo.mode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (cloudDB_.IsNotExistCloudDB()) {
        LOGE("[CloudSyncer] Not set cloudDB!");
        return -E_CLOUD_ERROR;
    }
    if (closed_) {
        LOGE("[CloudSyncer] DB is closed!");
        return -E_DB_CLOSED;
    }
    CloudTaskInfo info = taskInfo;
    info.status = ProcessStatus::PREPARED;
    errCode = TryToAddSyncTask(std::move(info));
    if (errCode != E_OK) {
        return errCode;
    }
    if (taskInfo.priorityTask) {
        MarkCurrentTaskPausedIfNeed();
    }
    return TriggerSync();
}

void CloudSyncer::SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB)
{
    cloudDB_.SetCloudDB(cloudDB);
    LOGI("[CloudSyncer] SetCloudDB finish");
}

void CloudSyncer::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    storageProxy_->SetIAssetLoader(loader);
    cloudDB_.SetIAssetLoader(loader);
    LOGI("[CloudSyncer] SetIAssetLoader finish");
}

void CloudSyncer::Close()
{
    closed_ = true;
    CloudSyncer::TaskId currentTask;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentTask = currentContext_.currentTaskId;
    }
    // mark current task db_closed
    SetTaskFailed(currentTask, -E_DB_CLOSED);
    UnlockIfNeed();
    cloudDB_.Close();
    {
        LOGD("[CloudSyncer] begin wait current task finished");
        std::unique_lock<std::mutex> uniqueLock(dataLock_);
        contextCv_.wait(uniqueLock, [this]() {
            return currentContext_.currentTaskId == INVALID_TASK_ID;
        });
        LOGD("[CloudSyncer] current task has been finished");
    }

    // copy all task from queue
    std::vector<CloudTaskInfo> infoList;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        for (const auto &item: cloudTaskInfos_) {
            infoList.push_back(item.second);
        }
        taskQueue_.clear();
        priorityTaskQueue_.clear();
        cloudTaskInfos_.clear();
        resumeTaskInfos_.clear();
        currentContext_.notifier = nullptr;
    }
    for (auto &info: infoList) {
        LOGI("[CloudSyncer] finished taskId %" PRIu64 " with db closed.", info.taskId);
    }
    storageProxy_->Close();
}

int CloudSyncer::TriggerSync()
{
    if (closed_) {
        return -E_DB_CLOSED;
    }
    RefObject::IncObjRef(this);
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this]() {
        DoSyncIfNeed();
        RefObject::DecObjRef(this);
    });
    if (errCode != E_OK) {
        LOGW("[CloudSyncer] schedule sync task failed %d", errCode);
        RefObject::DecObjRef(this);
    }
    return errCode;
}

void CloudSyncer::SetProxyUser(const std::string &user)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    storageProxy_->SetUser(user);
    currentContext_.notifier->SetUser(user);
    currentContext_.currentUserIndex = currentContext_.currentUserIndex + 1;
    cloudDB_.SwitchCloudDB(user);
}

void CloudSyncer::DoSyncIfNeed()
{
    if (closed_) {
        return;
    }
    // do all sync task in this loop
    do {
        // get taskId from queue
        TaskId triggerTaskId = GetNextTaskId();
        if (triggerTaskId == INVALID_TASK_ID) {
            LOGD("[CloudSyncer] task queue empty");
            break;
        }
        // pop taskId in queue
        if (PrepareSync(triggerTaskId) != E_OK) {
            break;
        }
        // do sync logic
        std::vector<std::string> usersList;
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            usersList = cloudTaskInfos_[triggerTaskId].users;
            currentContext_.currentUserIndex = 0;
        }
        int errCode = E_OK;
        if (usersList.empty()) {
            SetProxyUser("");
            errCode = DoSync(triggerTaskId);
        } else {
            for (const auto &user : usersList) {
                SetProxyUser(user);
                errCode = DoSync(triggerTaskId);
            }
        }
        LOGD("[CloudSyncer] DoSync finished, errCode %d", errCode);
    } while (!closed_);
    LOGD("[CloudSyncer] DoSyncIfNeed finished, closed status %d", static_cast<int>(closed_));
}

int CloudSyncer::DoSync(TaskId taskId)
{
    std::lock_guard<std::mutex> lock(syncMutex_);
    CloudTaskInfo taskInfo;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        taskInfo = cloudTaskInfos_[taskId];
    }
    storageProxy_->SetCloudTaskConfig({ !taskInfo.priorityTask });
    bool needUpload = true;
    bool isNeedFirstDownload = false;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        needUpload = currentContext_.strategy->JudgeUpload();
        // 1. if the locker is already exist, directly reuse the lock, no need do the first download
        // 2. if the task(resume task) is already be tagged need upload data, no need do the first download
        isNeedFirstDownload = (currentContext_.locker == nullptr) && (!currentContext_.isNeedUpload);
    }
    int errCode = E_OK;
    bool isFirstDownload = true;
    if (isNeedFirstDownload) {
        // do first download
        errCode = DoDownloadInNeed(taskInfo, needUpload, isFirstDownload);
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            cloudTaskInfos_[currentContext_.currentTaskId].errCode = errCode;
        }
        if (errCode != E_OK) {
            SyncMachineDoFinished();
            return errCode;
        }
        bool isActuallyNeedUpload = false;  // whether the task actually has data to upload
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            isActuallyNeedUpload = currentContext_.isNeedUpload;
        }
        if (!isActuallyNeedUpload) {
            LOGI("[CloudSyncer] no table need upload!");
            SyncMachineDoFinished();
            return E_OK;
        }
        isFirstDownload = false;
    }

    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentContext_.isFirstDownload = isFirstDownload;
        currentContext_.isRealNeedUpload = needUpload;
    }
    errCode = DoSyncInner(taskInfo, needUpload, isFirstDownload);
    return errCode;
}

int CloudSyncer::PrepareAndUpload(const CloudTaskInfo &taskInfo, size_t index)
{
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentContext_.tableName = taskInfo.table[index];
    }
    int errCode = CheckTaskIdValid(taskInfo.taskId);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] task is invalid, abort sync");
        return errCode;
    }
    errCode = DoUpload(taskInfo.taskId, index == (taskInfo.table.size() - 1u), taskInfo.lockAction);
    if (errCode == -E_CLOUD_VERSION_CONFLICT) {
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            currentContext_.isDownloadFinished[taskInfo.table[index]] = false;
            LOGI("[CloudSyncer] upload version conflict, index:%zu", index);
        }
        return errCode;
    }
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] upload failed %d", errCode);
        return errCode;
    }
    return errCode;
}

int CloudSyncer::DoUploadInNeed(const CloudTaskInfo &taskInfo, const bool needUpload)
{
    if (!needUpload) {
        return E_OK;
    }
    int errCode = storageProxy_->StartTransaction();
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] start transaction failed before doing upload.");
        return errCode;
    }
    for (size_t i = GetStartTableIndex(taskInfo.taskId, true); i < taskInfo.table.size(); ++i) {
        LOGD("[CloudSyncer] try upload table, index: %zu", i);
        errCode = PrepareAndUpload(taskInfo, i);
        if (errCode != E_OK) {
            break;
        }
    }
    if (errCode == -E_TASK_PAUSED) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        resumeTaskInfos_[taskInfo.taskId].upload = true;
    }
    if (errCode == E_OK || errCode == -E_TASK_PAUSED) {
        int commitErrorCode = storageProxy_->Commit();
        if (commitErrorCode != E_OK) {
            LOGE("[CloudSyncer] cannot commit transaction: %d.", commitErrorCode);
        }
    } else {
        int rollBackErrorCode = storageProxy_->Rollback();
        if (rollBackErrorCode != E_OK) {
            LOGE("[CloudSyncer] cannot roll back transaction: %d.", rollBackErrorCode);
        }
    }
    return errCode;
}

CloudSyncEvent CloudSyncer::SyncMachineDoDownload()
{
    CloudTaskInfo taskInfo;
    bool needUpload;
    bool isFirstDownload;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        taskInfo = cloudTaskInfos_[currentContext_.currentTaskId];
        needUpload = currentContext_.isRealNeedUpload;
        isFirstDownload = currentContext_.isFirstDownload;
    }
    int errCode = E_OK;
    if (IsLockInDownload()) {
        errCode = LockCloudIfNeed(taskInfo.taskId);
    }
    if (errCode != E_OK) {
        return SetCurrentTaskFailedInMachine(errCode);
    }
    errCode = DoDownloadInNeed(taskInfo, needUpload, isFirstDownload);
    if (errCode != E_OK) {
        return SetCurrentTaskFailedInMachine(errCode);
    }
    return CloudSyncEvent::DOWNLOAD_FINISHED_EVENT;
}

CloudSyncEvent CloudSyncer::SyncMachineDoUpload()
{
    CloudTaskInfo taskInfo;
    bool needUpload;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        taskInfo = cloudTaskInfos_[currentContext_.currentTaskId];
        needUpload = currentContext_.isRealNeedUpload;
    }
    int errCode = DoUploadInNeed(taskInfo, needUpload);
    if (errCode == -E_CLOUD_VERSION_CONFLICT) {
        return CloudSyncEvent::REPEAT_DOWNLOAD_EVENT;
    }
    if (errCode != E_OK) {
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            cloudTaskInfos_[currentContext_.currentTaskId].errCode = errCode;
        }
        return CloudSyncEvent::ERROR_EVENT;
    }

    return CloudSyncEvent::UPLOAD_FINISHED_EVENT;
}

CloudSyncEvent CloudSyncer::SyncMachineDoFinished()
{
    UnlockIfNeed();
    TaskId taskId;
    int errCode;
    int currentUserIndex;
    int userListSize;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        taskId = currentContext_.currentTaskId;
        errCode = cloudTaskInfos_[currentContext_.currentTaskId].errCode;
        cloudTaskInfos_[currentContext_.currentTaskId].errCode = E_OK;
        currentUserIndex = currentContext_.currentUserIndex;
        userListSize = cloudTaskInfos_[taskId].users.size();
    }
    if (currentUserIndex >= userListSize) {
        DoFinished(taskId, errCode);
    }
    return CloudSyncEvent::ALL_TASK_FINISHED_EVENT;
}

int CloudSyncer::DoSyncInner(const CloudTaskInfo &taskInfo, const bool needUpload, bool isFirstDownload)
{
    cloudSyncStateMachine_.SwitchStateAndStep(CloudSyncEvent::START_SYNC_EVENT);
    return E_OK;
}

void CloudSyncer::DoFinished(TaskId taskId, int errCode)
{
    storageProxy_->OnSyncFinish();
    if (errCode == -E_TASK_PAUSED) {
        LOGD("[CloudSyncer] taskId %" PRIu64 " was paused, it won't be finished now", taskId);
        {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            resumeTaskInfos_[taskId].context = std::move(currentContext_);
            currentContext_.locker = resumeTaskInfos_[taskId].context.locker;
            resumeTaskInfos_[taskId].context.locker = nullptr;
            ClearCurrentContextWithoutLock();
        }
        contextCv_.notify_one();
        return;
    }
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        taskQueue_.remove(taskId);
        priorityTaskQueue_.remove(taskId);
    }
    ClearContextAndNotify(taskId, errCode);
}

/**
 * UpdateChangedData will be used for Insert case, which we can only get rowid after we saved data in db.
*/
int CloudSyncer::UpdateChangedData(SyncParam &param, DownloadList &assetsDownloadList)
{
    if (param.withoutRowIdData.insertData.empty() && param.withoutRowIdData.updateData.empty()) {
        return E_OK;
    }
    int ret = E_OK;
    for (size_t j : param.withoutRowIdData.insertData) {
        VBucket &datum = param.downloadData.data[j];
        std::vector<Type> primaryValues;
        ret = CloudSyncUtils::GetCloudPkVals(datum, param.changedData.field,
            std::get<int64_t>(datum[CloudDbConstant::ROW_ID_FIELD_NAME]), primaryValues);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] updateChangedData cannot get primaryValues");
            return ret;
        }
        param.changedData.primaryData[ChangeType::OP_INSERT].push_back(primaryValues);
    }
    for (const auto &tuple : param.withoutRowIdData.assetInsertData) {
        size_t downloadIndex = std::get<0>(tuple);
        VBucket &datum = param.downloadData.data[downloadIndex];
        size_t insertIdx = std::get<1>(tuple);
        std::vector<Type> &pkVal = std::get<5>(assetsDownloadList[insertIdx]); // 5 means primary key list
        pkVal[0] = datum[CloudDbConstant::ROW_ID_FIELD_NAME];
    }
    for (const auto &tuple : param.withoutRowIdData.updateData) {
        size_t downloadIndex = std::get<0>(tuple);
        size_t updateIndex = std::get<1>(tuple);
        VBucket &datum = param.downloadData.data[downloadIndex];
        size_t size = param.changedData.primaryData[ChangeType::OP_UPDATE].size();
        if (updateIndex >= size) {
            LOGE("[CloudSyncer] updateIndex is invalid. index=%zu, size=%zu", updateIndex, size);
            return -E_INTERNAL_ERROR;
        }
        if (param.changedData.primaryData[ChangeType::OP_UPDATE][updateIndex].empty()) {
            LOGE("[CloudSyncer] primary key value list should not be empty.");
            return -E_INTERNAL_ERROR;
        }
        // no primary key or composite primary key, the first element is rowid
        param.changedData.primaryData[ChangeType::OP_UPDATE][updateIndex][0] =
            datum[CloudDbConstant::ROW_ID_FIELD_NAME];
    }
    return ret;
}

bool CloudSyncer::IsDataContainDuplicateAsset(const std::vector<Field> &assetFields, VBucket &data)
{
    for (const auto &assetField : assetFields) {
        if (assetField.type == TYPE_INDEX<Assets> && data[assetField.colName].index() == TYPE_INDEX<Assets>) {
            if (CloudStorageUtils::IsAssetsContainDuplicateAsset(std::get<Assets>(data[assetField.colName]))) {
                return true;
            }
        }
    }
    return false;
}

bool CloudSyncer::IsDataContainAssets()
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    bool hasTable = (currentContext_.assetFields.find(currentContext_.tableName) != currentContext_.assetFields.end());
    if (!hasTable) {
        LOGW("[CloudSyncer] failed to get assetFields, because tableName doesn't exist in currentContext, %d.",
            -E_INTERNAL_ERROR);
            return false;
    }
    if (currentContext_.assetFields[currentContext_.tableName].empty()) {
        LOGI("[CloudSyncer] Current table do not contain assets, thereby we needn't download assets");
        return false;
    }
    return true;
}

std::map<std::string, Assets> CloudSyncer::TagAssetsInSingleRecord(VBucket &coveredData, VBucket &beCoveredData,
    bool setNormalStatus, int &errCode)
{
    // Define a map to store the result
    std::map<std::string, Assets> res = {};
    std::vector<Field> assetFields;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        assetFields = currentContext_.assetFields[currentContext_.tableName];
    }
    // For every column contain asset or assets, assetFields are in context
    for (const Field &assetField : assetFields) {
        Assets assets = TagAssetsInSingleCol(coveredData, beCoveredData, assetField, setNormalStatus, errCode);
        if (!assets.empty()) {
            res[assetField.colName] = assets;
        }
        if (errCode != E_OK) {
            break;
        }
    }
    return res;
}

int CloudSyncer::FillCloudAssets(const std::string &tableName, VBucket &normalAssets,
    VBucket &failedAssets)
{
    int ret = E_OK;
    if (normalAssets.size() > 1) {
        ret = storageProxy_->FillCloudAssetForDownload(tableName, normalAssets, true);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Can not fill normal cloud assets for download");
            return ret;
        }
    }
    if (failedAssets.size() > 1) {
        ret = storageProxy_->FillCloudAssetForDownload(tableName, failedAssets, false);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Can not fill abnormal assets for download");
            return ret;
        }
    }
    return E_OK;
}

int CloudSyncer::HandleDownloadResult(const DownloadItem &downloadItem, const std::string &tableName,
    DownloadCommitList &commitList, uint32_t &successCount)
{
    successCount = 0;
    int errCode = storageProxy_->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] start transaction Failed before handle download.");
        return errCode;
    }
    errCode = CommitDownloadAssets(downloadItem, tableName, commitList, successCount);
    if (errCode != E_OK) {
        successCount = 0;
        int ret = E_OK;
        if (errCode == -E_REMOVE_ASSETS_FAILED) {
            // remove assets failed no effect to asset status, just commit
            ret = storageProxy_->Commit();
        } else {
            ret = storageProxy_->Rollback();
        }
        LOGE("[CloudSyncer] commit download assets failed %d commit/rollback ret %d", errCode, ret);
        return errCode;
    }
    errCode = storageProxy_->Commit();
    if (errCode != E_OK) {
        successCount = 0;
        LOGE("[CloudSyncer] commit failed %d", errCode);
    }
    return errCode;
}

int CloudSyncer::CloudDbDownloadAssets(TaskId taskId, InnerProcessInfo &info, const DownloadList &downloadList,
    const std::set<Key> &dupHashKeySet, ChangedData &changedAssets)
{
    int downloadStatus = E_OK;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        downloadStatus = resumeTaskInfos_[taskId].downloadStatus;
        resumeTaskInfos_[taskId].downloadStatus = E_OK;
    }
    int errorCode = E_OK;
    DownloadCommitList commitList;
    for (size_t i = GetDownloadAssetIndex(taskId); i < downloadList.size(); i++) {
        errorCode = CheckTaskIdValid(taskId);
        if (errorCode != E_OK) {
            std::lock_guard<std::mutex> autoLock(dataLock_);
            resumeTaskInfos_[taskId].lastDownloadIndex = i;
            resumeTaskInfos_[taskId].downloadStatus = downloadStatus;
            break;
        }
        DownloadItem downloadItem;
        GetDownloadItem(downloadList, i, downloadItem);
        errorCode = DownloadOneAssetRecord(dupHashKeySet, downloadList, downloadItem, info, changedAssets);
        if (errorCode == -E_NOT_SET) {
            info.downLoadInfo.failCount += (downloadList.size() - i);
            info.downLoadInfo.successCount -= (downloadList.size() - i);
            return errorCode;
        }
        if (downloadItem.strategy == OpType::DELETE) {
            downloadItem.assets = {};
            downloadItem.gid = "";
        }
        // Process result of each asset
        commitList.push_back(std::make_tuple(downloadItem.gid, std::move(downloadItem.assets), errorCode == E_OK));
        downloadStatus = downloadStatus == E_OK ? errorCode : downloadStatus;
        int ret = CommitDownloadResult(downloadItem, info, commitList, errorCode);
        if (ret != E_OK && ret != -E_REMOVE_ASSETS_FAILED) {
            return ret;
        }
        downloadStatus = downloadStatus == E_OK ? ret : downloadStatus;
    }
    LOGD("Download status is %d", downloadStatus);
    return errorCode == E_OK ? downloadStatus : errorCode;
}

int CloudSyncer::DownloadAssets(InnerProcessInfo &info, const std::vector<std::string> &pKColNames,
    const std::set<Key> &dupHashKeySet, ChangedData &changedAssets)
{
    if (!IsDataContainAssets()) {
        return E_OK;
    }
    // update changed data info
    if (!CloudSyncUtils::IsChangeDataEmpty(changedAssets)) {
        // changedData.primaryData should have no member inside
        return -E_INVALID_ARGS;
    }
    changedAssets.tableName = info.tableName;
    changedAssets.type = ChangedDataType::ASSET;
    changedAssets.field = pKColNames;

    // Get AssetDownloadList
    DownloadList changeList;
    TaskId taskId;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        changeList = currentContext_.assetDownloadList;
        taskId = currentContext_.currentTaskId;
    }
    // Download data (include deleting) will handle return Code in this situation
    int ret = CloudDbDownloadAssets(taskId, info, changeList, dupHashKeySet, changedAssets);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Can not download assets or can not handle download result %d", ret);
    }
    return ret;
}

std::map<std::string, Assets> CloudSyncer::GetAssetsFromVBucket(VBucket &data)
{
    std::map<std::string, Assets> assets;
    std::vector<Field> fields;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        fields = currentContext_.assetFields[currentContext_.tableName];
    }
    for (const auto &field : fields) {
        if (data.find(field.colName) != data.end()) {
            if (field.type == TYPE_INDEX<Asset> && data[field.colName].index() == TYPE_INDEX<Asset>) {
                assets[field.colName] = { std::get<Asset>(data[field.colName]) };
            } else if (field.type == TYPE_INDEX<Assets> && data[field.colName].index() == TYPE_INDEX<Assets>) {
                assets[field.colName] = std::get<Assets>(data[field.colName]);
            } else {
                Assets emptyAssets;
                assets[field.colName] = emptyAssets;
            }
        }
    }
    return assets;
}

int CloudSyncer::TagStatus(bool isExist, SyncParam &param, size_t idx, DataInfo &dataInfo, VBucket &localAssetInfo)
{
    OpType strategyOpResult = OpType::NOT_HANDLE;
    int errCode = TagStatusByStrategy(isExist, param, dataInfo, strategyOpResult);
    if (errCode != E_OK) {
        return errCode;
    }
    param.downloadData.opType[idx] = strategyOpResult;
    if (!IsDataContainAssets()) {
        return E_OK;
    }
    Key hashKey;
    if (isExist) {
        hashKey = dataInfo.localInfo.logInfo.hashKey;
    }
    return TagDownloadAssets(hashKey, idx, param, dataInfo, localAssetInfo);
}

int CloudSyncer::TagDownloadAssets(const Key &hashKey, size_t idx, SyncParam &param, const DataInfo &dataInfo,
    VBucket &localAssetInfo)
{
    int ret = E_OK;
    OpType strategy = param.downloadData.opType[idx];
    switch (strategy) {
        case OpType::INSERT:
        case OpType::UPDATE:
        case OpType::DELETE:
            ret = HandleTagAssets(hashKey, dataInfo, idx, param, localAssetInfo);
            break;
        case OpType::NOT_HANDLE:
        case OpType::ONLY_UPDATE_GID:
        case OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO: { // means upload need this data
            (void)TagAssetsInSingleRecord(localAssetInfo, param.downloadData.data[idx], true, ret);
            for (const auto &[col, value]: localAssetInfo) {
                param.downloadData.data[idx].insert_or_assign(col, value);
            }
            break;
        }
        default:
            break;
    }
    return ret;
}

int CloudSyncer::HandleTagAssets(const Key &hashKey, const DataInfo &dataInfo, size_t idx, SyncParam &param,
    VBucket &localAssetInfo)
{
    Type prefix;
    std::vector<Type> pkVals;
    OpType strategy = param.downloadData.opType[idx];
    bool isDelStrategy = (strategy == OpType::DELETE);
    int ret = CloudSyncUtils::GetCloudPkVals(isDelStrategy ? dataInfo.localInfo.primaryKeys :
        param.downloadData.data[idx], param.pkColNames, dataInfo.localInfo.logInfo.dataKey, pkVals);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] HandleTagAssets cannot get primary key value list. %d", ret);
        return ret;
    }
    prefix = param.isSinglePrimaryKey ? pkVals[0] : prefix;
    if (param.isSinglePrimaryKey && prefix.index() == TYPE_INDEX<Nil>) {
        LOGE("[CloudSyncer] Invalid primary key type in TagStatus, it's Nil.");
        return -E_INTERNAL_ERROR;
    }
    AssetOperationUtils::FilterDeleteAsset(param.downloadData.data[idx]);
    std::map<std::string, Assets> assetsMap = TagAssetsInSingleRecord(param.downloadData.data[idx], localAssetInfo,
        false, ret);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] TagAssetsInSingleRecord report ERROR in download data");
        return ret;
    }

    if (!param.isSinglePrimaryKey && strategy == OpType::INSERT) {
        param.withoutRowIdData.assetInsertData.push_back(std::make_tuple(idx, param.assetsDownloadList.size()));
    }
    param.assetsDownloadList.push_back(
        std::make_tuple(dataInfo.cloudLogInfo.cloudGid, prefix, strategy, assetsMap, hashKey,
        pkVals, dataInfo.cloudLogInfo.timestamp));
    return ret;
}

int CloudSyncer::SaveDatum(SyncParam &param, size_t idx, std::vector<std::pair<Key, size_t>> &deletedList,
    std::map<std::string, LogInfo> &localLogInfoCache)
{
    int ret = PreHandleData(param.downloadData.data[idx], param.pkColNames);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid download data:%d", ret);
        return ret;
    }
    CloudSyncUtils::ModifyCloudDataTime(param.downloadData.data[idx]);
    DataInfo dataInfo;
    VBucket localAssetInfo;
    bool isExist = true;
    ret = GetLocalInfo(idx, param, dataInfo.localInfo, localLogInfoCache, localAssetInfo);
    if (ret == -E_NOT_FOUND) {
        isExist = false;
    } else if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot get info by primary key or gid: %d.", ret);
        return ret;
    }
    // Get cloudLogInfo from cloud data
    dataInfo.cloudLogInfo = CloudSyncUtils::GetCloudLogInfo(param.downloadData.data[idx]);
    // Tag datum to get opType
    ret = TagStatus(isExist, param, idx, dataInfo, localAssetInfo);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot tag status: %d.", ret);
        return ret;
    }
    CloudSyncUtils::UpdateLocalCache(param.downloadData.opType[idx], dataInfo.cloudLogInfo, dataInfo.localInfo.logInfo,
        localLogInfoCache);
    ret = CloudSyncUtils::SaveChangedData(param, idx, dataInfo, deletedList);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot save changed data: %d.", ret);
    }
    return ret;
}

int CloudSyncer::SaveData(SyncParam &param)
{
    if (!CloudSyncUtils::IsChangeDataEmpty(param.changedData)) {
        LOGE("[CloudSyncer] changedData.primaryData should have no member inside.");
        return -E_INVALID_ARGS;
    }
    // Update download batch Info
    param.info.downLoadInfo.batchIndex += 1;
    param.info.downLoadInfo.total += param.downloadData.data.size();
    int ret = E_OK;
    DownloadList assetsDownloadList;
    param.assetsDownloadList = assetsDownloadList;
    param.deletePrimaryKeySet.clear();
    param.dupHashKeySet.clear();
    CloudSyncUtils::ClearWithoutData(param);
    std::vector<std::pair<Key, size_t>> deletedList;
    // use for record local delete status
    std::map<std::string, LogInfo> localLogInfoCache;
    for (size_t i = 0; i < param.downloadData.data.size(); i++) {
        ret = SaveDatum(param, i, deletedList, localLogInfoCache);
        if (ret != E_OK) {
            param.info.downLoadInfo.failCount += param.downloadData.data.size();
            LOGE("[CloudSyncer] Cannot save datum due to error code %d", ret);
            return ret;
        }
    }
    // Save assetsMap into current context
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentContext_.assetDownloadList = param.assetsDownloadList;
    }
    // save the data to the database by batch, downloadData will return rowid when insert data.
    ret = storageProxy_->PutCloudSyncData(param.tableName, param.downloadData);
    if (ret != E_OK) {
        param.info.downLoadInfo.failCount += param.downloadData.data.size();
        LOGE("[CloudSyncer] Cannot save the data to database with error code: %d.", ret);
        return ret;
    }
    ret = UpdateChangedData(param, currentContext_.assetDownloadList);
    if (ret != E_OK) {
        param.info.downLoadInfo.failCount += param.downloadData.data.size();
        LOGE("[CloudSyncer] Cannot update changed data: %d.", ret);
        return ret;
    }
    // Update downloadInfo
    param.info.downLoadInfo.successCount += param.downloadData.data.size();
    // Get latest cloudWaterMark
    VBucket &lastData = param.downloadData.data.back();
    param.cloudWaterMark = std::get<std::string>(lastData[CloudDbConstant::CURSOR_FIELD]);
    return UpdateFlagForSavedRecord(param);
}

int CloudSyncer::PreCheck(CloudSyncer::TaskId &taskId, const TableName &tableName)
{
    // Check Input and Context Validity
    int ret = CheckTaskIdValid(taskId);
    if (ret != E_OK) {
        return ret;
    }
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
            LOGE("[CloudSyncer] Cloud Task Info does not exist taskId: , %" PRIu64 ".", taskId);
            return -E_INVALID_ARGS;
        }
    }
    if (currentContext_.strategy == nullptr) {
        LOGE("[CloudSyncer] Strategy has not been initialized");
        return -E_INVALID_ARGS;
    }
    ret = storageProxy_->CheckSchema(tableName);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] A schema error occurred on the table to be synced, %d", ret);
        return ret;
    }
    return E_OK;
}

bool CloudSyncer::NeedNotifyChangedData(const ChangedData &changedData)
{
    TaskId taskId;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        taskId = currentContext_.currentTaskId;
    }
    if (IsModeForcePush(taskId)) {
        return false;
    }
    // when there have no data been changed, it don't need fill back
    if (changedData.primaryData[OP_INSERT].empty() && changedData.primaryData[OP_UPDATE].empty() &&
        changedData.primaryData[OP_DELETE].empty()) {
            return false;
        }
    return true;
}

int CloudSyncer::NotifyChangedData(ChangedData &&changedData)
{
    if (!NeedNotifyChangedData(changedData)) {
        return E_OK;
    }
    std::string deviceName;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        std::vector<std::string> devices = currentContext_.notifier->GetDevices();
        if (devices.empty()) {
            LOGE("[CloudSyncer] CurrentContext do not contain device info");
            return -E_CLOUD_ERROR;
        }
        // We use first device name as the target of NotifyChangedData
        deviceName = devices[0];
    }
    int ret = storageProxy_->NotifyChangedData(deviceName, std::move(changedData));
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot notify changed data while downloading, %d.", ret);
    }
    return ret;
}

void CloudSyncer::NotifyInDownload(CloudSyncer::TaskId taskId, SyncParam &param, bool isFirstDownload)
{
    if (!isFirstDownload && param.downloadData.data.empty()) {
        // if the second download and there is no download data, do not notify
        return;
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (currentContext_.strategy->JudgeUpload()) {
        currentContext_.notifier->NotifyProcess(cloudTaskInfos_[taskId], param.info);
    } else {
        if (param.isLastBatch) {
            param.info.tableStatus = ProcessStatus::FINISHED;
        }
        if (cloudTaskInfos_[taskId].table.back() == param.tableName && param.isLastBatch) {
            currentContext_.notifier->UpdateProcess(param.info);
        } else {
            currentContext_.notifier->NotifyProcess(cloudTaskInfos_[taskId], param.info);
        }
    }
}

int CloudSyncer::SaveDataInTransaction(CloudSyncer::TaskId taskId, SyncParam &param)
{
    int ret = storageProxy_->StartTransaction(TransactType::IMMEDIATE);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot start a transaction: %d.", ret);
        return ret;
    }
    if (!IsModeForcePush(taskId)) {
        param.changedData.tableName = param.info.tableName;
        param.changedData.field = param.pkColNames;
        param.changedData.type = ChangedDataType::DATA;
    }
    ret = SaveData(param);
    param.insertPk.clear();
    if (ret != E_OK) {
        LOGE("[CloudSyncer] cannot save data: %d.", ret);
        int rollBackErrorCode = storageProxy_->Rollback();
        if (rollBackErrorCode != E_OK) {
            LOGE("[CloudSyncer] cannot roll back transaction: %d.", rollBackErrorCode);
        } else {
            LOGI("[CloudSyncer] Roll back transaction success: %d.", ret);
        }
        return ret;
    }
    ret = storageProxy_->Commit();
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot commit a transaction: %d.", ret);
    }
    return ret;
}

int CloudSyncer::DoDownloadAssets(bool skipSave, SyncParam &param)
{
    // Begin dowloading assets
    ChangedData changedAssets;
    int ret = DownloadAssets(param.info, param.pkColNames, param.dupHashKeySet, changedAssets);
    bool isSharedTable = false;
    int errCode = storageProxy_->IsSharedTable(param.tableName, isSharedTable);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] HandleTagAssets cannot judge the table is a shared table. %d", errCode);
        return errCode;
    }
    if (!isSharedTable) {
        (void)NotifyChangedData(std::move(changedAssets));
    }
    if (ret == -E_TASK_PAUSED) {
        LOGD("[CloudSyncer] current task was paused, abort dowload asset");
        std::lock_guard<std::mutex> autoLock(dataLock_);
        resumeTaskInfos_[currentContext_.currentTaskId].skipQuery = true;
        return ret;
    } else if (skipSave) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        resumeTaskInfos_[currentContext_.currentTaskId].skipQuery = false;
    }
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot notify downloadAssets due to error %d", ret);
    }
    return ret;
}

int CloudSyncer::SaveDataNotifyProcess(CloudSyncer::TaskId taskId, SyncParam &param)
{
    ChangedData changedData;
    bool skipSave = false;
    {
        bool currentTableResume = IsCurrentTableResume(taskId, false);
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (currentTableResume && resumeTaskInfos_[currentContext_.currentTaskId].skipQuery) {
            skipSave = true;
        }
    }
    int ret;
    if (!skipSave) {
        param.changedData = changedData;
        param.downloadData.opType.resize(param.downloadData.data.size());
        param.downloadData.existDataKey.resize(param.downloadData.data.size());
        param.downloadData.existDataHashKey.resize(param.downloadData.data.size());
        ret = SaveDataInTransaction(taskId, param);
        if (ret != E_OK) {
            return ret;
        }
        // call OnChange to notify changedData object first time (without Assets)
        ret = NotifyChangedData(std::move(param.changedData));
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Cannot notify changed data due to error %d", ret);
            return ret;
        }
    }
    ret = DoDownloadAssets(skipSave, param);
    if (ret != E_OK) {
        return ret;
    }
    UpdateCloudWaterMark(taskId, param);
    return E_OK;
}

void CloudSyncer::NotifyInBatchUpload(const UploadParam &uploadParam, const InnerProcessInfo &innerProcessInfo,
    bool lastBatch)
{
    CloudTaskInfo taskInfo;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        taskInfo = cloudTaskInfos_[uploadParam.taskId];
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (uploadParam.lastTable && lastBatch) {
        currentContext_.notifier->UpdateProcess(innerProcessInfo);
    } else {
        currentContext_.notifier->NotifyProcess(taskInfo, innerProcessInfo);
    }
}

int CloudSyncer::DoDownload(CloudSyncer::TaskId taskId, bool isFirstDownload)
{
    SyncParam param;
    int errCode = GetSyncParamForDownload(taskId, param);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] get sync param for download failed %d", errCode);
        return errCode;
    }
    (void)storageProxy_->CreateTempSyncTrigger(param.tableName);
    errCode = DoDownloadInner(taskId, param, isFirstDownload);
    (void)storageProxy_->ClearAllTempSyncTrigger();
    if (errCode == -E_TASK_PAUSED) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        resumeTaskInfos_[taskId].syncParam = std::move(param);
    }
    return errCode;
}

int CloudSyncer::DoDownloadInner(CloudSyncer::TaskId taskId, SyncParam &param, bool isFirstDownload)
{
    // Query data by batch until reaching end and not more data need to be download
    int ret = PreCheck(taskId, param.info.tableName);
    if (ret != E_OK) {
        return ret;
    }
    do {
        ret = DownloadOneBatch(taskId, param, isFirstDownload);
        if (ret != E_OK) {
            return ret;
        }
    } while (!param.isLastBatch);
    return E_OK;
}

void CloudSyncer::NotifyInEmptyDownload(CloudSyncer::TaskId taskId, InnerProcessInfo &info)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (currentContext_.strategy->JudgeUpload()) {
        currentContext_.notifier->NotifyProcess(cloudTaskInfos_[taskId], info);
    } else {
        info.tableStatus = FINISHED;
        if (cloudTaskInfos_[taskId].table.back() == info.tableName) {
            currentContext_.notifier->UpdateProcess(info);
        } else {
            currentContext_.notifier->NotifyProcess(cloudTaskInfos_[taskId], info);
        }
    }
}

int CloudSyncer::PreCheckUpload(CloudSyncer::TaskId &taskId, const TableName &tableName, Timestamp &localMark)
{
    int ret = PreCheck(taskId, tableName);
    if (ret != E_OK) {
        return ret;
    }
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
            LOGE("[CloudSyncer] Cloud Task Info does not exist taskId: %" PRIu64 ".", taskId);
            return -E_INVALID_ARGS;
        }
        if ((cloudTaskInfos_[taskId].mode < SYNC_MODE_CLOUD_MERGE) ||
            (cloudTaskInfos_[taskId].mode > SYNC_MODE_CLOUD_FORCE_PUSH)) {
            LOGE("[CloudSyncer] Upload failed, invalid sync mode: %d.",
                static_cast<int>(cloudTaskInfos_[taskId].mode));
            return -E_INVALID_ARGS;
        }
    }

    return ret;
}

int CloudSyncer::SaveUploadData(Info &insertInfo, Info &updateInfo, Info &deleteInfo, CloudSyncData &uploadData,
    InnerProcessInfo &innerProcessInfo)
{
    int errCode = E_OK;
    if (!uploadData.delData.record.empty() && !uploadData.delData.extend.empty()) {
        errCode = BatchDelete(deleteInfo, uploadData, innerProcessInfo);
        if (errCode != E_OK) {
            return errCode;
        }
    }

    if (!uploadData.updData.record.empty() && !uploadData.updData.extend.empty()) {
        errCode = BatchUpdate(updateInfo, uploadData, innerProcessInfo);
        if (errCode != E_OK) {
            return errCode;
        }
    }

    if (!uploadData.insData.record.empty() && !uploadData.insData.extend.empty()) {
        errCode = BatchInsert(insertInfo, uploadData, innerProcessInfo);
        if (errCode != E_OK) {
            return errCode;
        }
    }

    if (!uploadData.lockData.rowid.empty()) {
        errCode = storageProxy_->FillCloudLogAndAsset(OpType::LOCKED_NOT_HANDLE, uploadData);
    }
    return errCode;
}

int CloudSyncer::DoBatchUpload(CloudSyncData &uploadData, UploadParam &uploadParam, InnerProcessInfo &innerProcessInfo)
{
    int errCode = storageProxy_->FillCloudLogAndAsset(OpType::SET_UPLOADING, uploadData);
    if (errCode != E_OK) {
        return errCode;
    }
    Info insertInfo;
    Info updateInfo;
    Info deleteInfo;
    errCode = SaveUploadData(insertInfo, updateInfo, deleteInfo, uploadData, innerProcessInfo);
    if (errCode != E_OK) {
        return errCode;
    }
    bool lastBatch = innerProcessInfo.upLoadInfo.successCount == innerProcessInfo.upLoadInfo.total;
    if (lastBatch) {
        innerProcessInfo.tableStatus = ProcessStatus::FINISHED;
    }
    // After each batch upload successed, call NotifyProcess
    NotifyInBatchUpload(uploadParam, innerProcessInfo, lastBatch);

    // if batch upload successed, update local water mark
    // The cloud water mark cannot be updated here, because the cloud api doesn't return cursor here.
    errCode = PutWaterMarkAfterBatchUpload(uploadData.tableName, uploadParam);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Failed to set local water mark when doing upload, %d.", errCode);
    }
    return errCode;
}

int CloudSyncer::PutWaterMarkAfterBatchUpload(const std::string &tableName, UploadParam &uploadParam)
{
    int errCode = E_OK;
    // if we use local cover cloud strategy, it won't update local water mark also.
    if (IsModeForcePush(uploadParam.taskId) || (IsPriorityTask(uploadParam.taskId) &&
        !IsQueryListEmpty(uploadParam.taskId))) {
        return E_OK;
    }
    errCode = storageProxy_->PutWaterMarkByMode(tableName, uploadParam.localMark, uploadParam.mode);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Cannot set local water mark while Uploading, %d.", errCode);
    }
    return errCode;
}

int CloudSyncer::DoUpload(CloudSyncer::TaskId taskId, bool lastTable, LockAction lockAction)
{
    std::string tableName;
    int ret = GetCurrentTableName(tableName);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid table name for syncing: %d", ret);
        return ret;
    }

    Timestamp localMark = 0u;
    ret = PreCheckUpload(taskId, tableName, localMark);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Doing upload sync pre check failed, %d.", ret);
        return ret;
    }
    ReloadWaterMarkIfNeed(taskId, localMark);
    storageProxy_->OnUploadStart();

    int64_t count = 0;
    ret = storageProxy_->GetUploadCount(GetQuerySyncObject(tableName), IsModeForcePush(taskId),
        IsCompensatedTask(taskId), IsNeedGetLocalWater(taskId), count);
    LOGI("get upload count:%zu", count);
    if (ret != E_OK) {
        // GetUploadCount will return E_OK when upload count is zero.
        LOGE("[CloudSyncer] Failed to get Upload Data Count, %d.", ret);
        return ret;
    }
    if (count == 0) {
        UpdateProcessInfoWithoutUpload(taskId, tableName, !lastTable);
        return E_OK;
    }
    UploadParam param;
    param.count = count;
    param.lastTable = lastTable;
    param.taskId = taskId;
    param.lockAction = lockAction;
    return DoUploadInner(tableName, param);
}

int CloudSyncer::PreProcessBatchUpload(UploadParam &uploadParam, const InnerProcessInfo &innerProcessInfo,
    CloudSyncData &uploadData)
{
    // Precheck and calculate local water mark which would be updated if batch upload successed.
    int ret = CheckTaskIdValid(uploadParam.taskId);
    if (ret != E_OK) {
        return ret;
    }
    ret = CloudSyncUtils::CheckCloudSyncDataValid(uploadData, innerProcessInfo.tableName,
        innerProcessInfo.upLoadInfo.total, uploadParam.taskId);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid Cloud Sync Data of Upload, %d.", ret);
        return ret;
    }
    ret = TagUploadAssets(uploadData);
    if (ret != E_OK) {
        LOGE("TagUploadAssets report ERROR, cannot tag uploadAssets");
        return ret;
    }
    CloudSyncUtils::UpdateAssetsFlag(uploadData);
    // get local water mark to be updated in future.
    ret = CloudSyncUtils::UpdateExtendTime(uploadData, innerProcessInfo.upLoadInfo.total,
        uploadParam.taskId, uploadParam.localMark);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Failed to get new local water mark in Cloud Sync Data, %d.", ret);
    }
    return ret;
}

int CloudSyncer::SaveCloudWaterMark(const TableName &tableName, const TaskId taskId)
{
    std::string cloudWaterMark;
    bool isUpdateCloudCursor = true;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (currentContext_.cloudWaterMarks.find(tableName) == currentContext_.cloudWaterMarks.end()) {
            LOGD("[CloudSyncer] Not found water mark just return");
            return E_OK;
        }
        cloudWaterMark = currentContext_.cloudWaterMarks[tableName];
        isUpdateCloudCursor = currentContext_.strategy->JudgeUpdateCursor();
    }
    isUpdateCloudCursor = isUpdateCloudCursor && !(IsPriorityTask(taskId) && !IsQueryListEmpty(taskId));
    if (isUpdateCloudCursor) {
        int errCode = storageProxy_->SetCloudWaterMark(tableName, cloudWaterMark);
        if (errCode != E_OK) {
            LOGE("[CloudSyncer] Cannot set cloud water mark, %d.", errCode);
        }
        return errCode;
    }
    return E_OK;
}

void CloudSyncer::SetUploadDataFlag(const TaskId taskId, CloudSyncData& uploadData)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    uploadData.isCloudForcePushStrategy = (cloudTaskInfos_[taskId].mode == SYNC_MODE_CLOUD_FORCE_PUSH);
    uploadData.isCompensatedTask = cloudTaskInfos_[taskId].compensatedTask;
}

bool CloudSyncer::IsModeForcePush(const TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return cloudTaskInfos_[taskId].mode == SYNC_MODE_CLOUD_FORCE_PUSH;
}

bool CloudSyncer::IsModeForcePull(const TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return cloudTaskInfos_[taskId].mode == SYNC_MODE_CLOUD_FORCE_PULL;
}

bool CloudSyncer::IsPriorityTask(TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return cloudTaskInfos_[taskId].priorityTask;
}

int CloudSyncer::DoUploadByMode(const std::string &tableName, UploadParam &uploadParam, InnerProcessInfo &info)
{
    ContinueToken continueStmtToken = nullptr;
    CloudSyncData uploadData(tableName, uploadParam.mode);
    SetUploadDataFlag(uploadParam.taskId, uploadData);
    auto [err, localWater] = GetLocalWater(tableName, uploadParam);
    if (err != E_OK) {
        return err;
    }
    int ret = storageProxy_->GetCloudData(GetQuerySyncObject(tableName), localWater, continueStmtToken, uploadData);
    if ((ret != E_OK) && (ret != -E_UNFINISHED)) {
        LOGE("[CloudSyncer] Failed to get cloud data when upload, %d.", ret);
        return ret;
    }
    uploadParam.count -= uploadData.ignoredCount;
    info.upLoadInfo.total -= uploadData.ignoredCount;
    ret = HandleBatchUpload(uploadParam, info, uploadData, continueStmtToken);
    if (ret != -E_TASK_PAUSED) {
        // reset watermark to zero when task no paused
        RecordWaterMark(uploadParam.taskId, 0u);
    }
    if (continueStmtToken != nullptr) {
        storageProxy_->ReleaseContinueToken(continueStmtToken);
    }
    return ret;
}

int CloudSyncer::PreHandleData(VBucket &datum, const std::vector<std::string> &pkColNames)
{
    // type index of field in fields, true means mandatory filed
    static std::vector<std::tuple<std::string, int32_t, bool>> fieldAndIndex = {
        std::make_tuple(CloudDbConstant::GID_FIELD, TYPE_INDEX<std::string>, true),
        std::make_tuple(CloudDbConstant::CREATE_FIELD, TYPE_INDEX<int64_t>, true),
        std::make_tuple(CloudDbConstant::MODIFY_FIELD, TYPE_INDEX<int64_t>, true),
        std::make_tuple(CloudDbConstant::DELETE_FIELD, TYPE_INDEX<bool>, true),
        std::make_tuple(CloudDbConstant::CURSOR_FIELD, TYPE_INDEX<std::string>, true),
        std::make_tuple(CloudDbConstant::SHARING_RESOURCE_FIELD, TYPE_INDEX<std::string>, false)
    };

    for (const auto &fieldIndex : fieldAndIndex) {
        if (datum.find(std::get<0>(fieldIndex)) == datum.end()) {
            if (!std::get<2>(fieldIndex)) { // 2 is index of mandatory flag
                continue;
            }
            LOGE("[CloudSyncer] Cloud data do not contain expected field: %s.", std::get<0>(fieldIndex).c_str());
            return -E_CLOUD_ERROR;
        }
        if (datum[std::get<0>(fieldIndex)].index() != static_cast<size_t>(std::get<1>(fieldIndex))) {
            LOGE("[CloudSyncer] Cloud data's field: %s, doesn't has expected type.", std::get<0>(fieldIndex).c_str());
            return -E_CLOUD_ERROR;
        }
    }

    if (std::get<bool>(datum[CloudDbConstant::DELETE_FIELD])) {
        CloudSyncUtils::RemoveDataExceptExtendInfo(datum, pkColNames);
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (IsDataContainDuplicateAsset(currentContext_.assetFields[currentContext_.tableName], datum)) {
        LOGE("[CloudSyncer] Cloud data contain duplicate asset");
        return -E_CLOUD_ERROR;
    }
    return E_OK;
}

int CloudSyncer::QueryCloudData(TaskId taskId, const std::string &tableName, std::string &cloudWaterMark,
    DownloadData &downloadData)
{
    VBucket extend;
    int ret = FillDownloadExtend(taskId, tableName, cloudWaterMark, extend);
    if (ret != E_OK) {
        return ret;
    }
    ret = cloudDB_.Query(tableName, extend, downloadData.data);
    if (ret == -E_QUERY_END) {
        LOGD("[CloudSyncer] Download data from cloud database success and no more data need to be downloaded");
        return -E_QUERY_END;
    }
    if (ret == E_OK && downloadData.data.empty()) {
        if (extend[CloudDbConstant::CURSOR_FIELD].index() != TYPE_INDEX<std::string>) {
            LOGE("[CloudSyncer] cursor type is not valid=%d", extend[CloudDbConstant::CURSOR_FIELD].index());
            return -E_CLOUD_ERROR;
        }
        cloudWaterMark = std::get<std::string>(extend[CloudDbConstant::CURSOR_FIELD]);
        LOGD("[CloudSyncer] Download data is empty, try to use other cursor=%s", cloudWaterMark.c_str());
        return ret;
    }
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Download data from cloud database unsuccess %d", ret);
    }
    return ret;
}

int CloudSyncer::CheckTaskIdValid(TaskId taskId)
{
    if (closed_) {
        LOGE("[CloudSyncer] DB is closed.");
        return -E_DB_CLOSED;
    }
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
        LOGE("[CloudSyncer] not found task.");
        return -E_INVALID_ARGS;
    }
    if (cloudTaskInfos_[taskId].pause) {
        LOGW("[CloudSyncer] check task %" PRIu64 " was paused!", taskId);
        return -E_TASK_PAUSED;
    }
    if (cloudTaskInfos_[taskId].errCode != E_OK) {
        return cloudTaskInfos_[taskId].errCode;
    }
    return currentContext_.currentTaskId == taskId ? E_OK : -E_INVALID_ARGS;
}

int CloudSyncer::GetCurrentTableName(std::string &tableName)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (currentContext_.tableName.empty()) {
        return -E_BUSY;
    }
    tableName = currentContext_.tableName;
    return E_OK;
}

int CloudSyncer::CheckQueueSizeWithNoLock(bool priorityTask)
{
    int32_t limit = queuedManualSyncLimit_;
    if (!priorityTask && taskQueue_.size() >= static_cast<size_t>(limit)) {
        LOGW("[CloudSyncer] too much sync task");
        return -E_BUSY;
    } else if (priorityTask && priorityTaskQueue_.size() >= static_cast<size_t>(limit)) {
        LOGW("[CloudSyncer] too much priority sync task");
        return -E_BUSY;
    }
    return E_OK;
}

int CloudSyncer::PrepareSync(TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (closed_ || cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
        LOGW("[CloudSyncer] Abort sync because syncer is closed");
        return -E_DB_CLOSED;
    }
    if (closed_ || currentContext_.currentTaskId != INVALID_TASK_ID) {
        LOGW("[CloudSyncer] Abort sync because syncer is closed or another task is running");
        return -E_DB_CLOSED;
    }
    currentContext_.currentTaskId = taskId;
    cloudTaskInfos_[taskId].resume = cloudTaskInfos_[taskId].pause;
    cloudTaskInfos_[taskId].pause = false;
    cloudTaskInfos_[taskId].status = ProcessStatus::PROCESSING;
    if (cloudTaskInfos_[taskId].resume) {
        auto tempLocker = currentContext_.locker;
        currentContext_ = resumeTaskInfos_[taskId].context;
        currentContext_.locker = tempLocker;
    } else {
        currentContext_.notifier = std::make_shared<ProcessNotifier>(this);
        currentContext_.strategy = StrategyFactory::BuildSyncStrategy(cloudTaskInfos_[taskId].mode, policy_);
        currentContext_.notifier->Init(cloudTaskInfos_[taskId].table, cloudTaskInfos_[taskId].devices);
    }
    LOGI("[CloudSyncer] exec taskId %" PRIu64, taskId);
    return E_OK;
}

int CloudSyncer::LockCloud(TaskId taskId)
{
    int period;
    {
        auto res = cloudDB_.Lock();
        if (res.first != E_OK) {
            return res.first;
        }
        period = static_cast<int>(res.second) / HEARTBEAT_PERIOD;
    }
    int errCode = StartHeartBeatTimer(period, taskId);
    if (errCode != E_OK) {
        UnlockCloud();
    }
    return errCode;
}

int CloudSyncer::UnlockCloud()
{
    FinishHeartBeatTimer();
    int errCode = cloudDB_.UnLock();
    WaitAllHeartBeatTaskExit();
    return errCode;
}

int CloudSyncer::StartHeartBeatTimer(int period, TaskId taskId)
{
    if (timerId_ != 0u) {
        LOGW("[CloudSyncer] HeartBeat timer has been start!");
        return E_OK;
    }
    TimerId timerId = 0;
    int errCode = RuntimeContext::GetInstance()->SetTimer(period, [this, taskId](TimerId timerId) {
        HeartBeat(timerId, taskId);
        return E_OK;
    }, nullptr, timerId);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] HeartBeat timer start failed %d", errCode);
        return errCode;
    }
    timerId_ = timerId;
    return E_OK;
}

void CloudSyncer::FinishHeartBeatTimer()
{
    if (timerId_ == 0u) {
        return;
    }
    RuntimeContext::GetInstance()->RemoveTimer(timerId_, true);
    timerId_ = 0u;
    LOGD("[CloudSyncer] Finish heartbeat timer ok");
}

void CloudSyncer::WaitAllHeartBeatTaskExit()
{
    std::unique_lock<std::mutex> uniqueLock(heartbeatMutex_);
    if (heartBeatCount_ <= 0) {
        return;
    }
    LOGD("[CloudSyncer] Begin wait all heartbeat task exit");
    heartbeatCv_.wait(uniqueLock, [this]() {
        return heartBeatCount_ <= 0;
    });
    LOGD("[CloudSyncer] End wait all heartbeat task exit");
}

void CloudSyncer::HeartBeat(TimerId timerId, TaskId taskId)
{
    if (timerId_ != timerId) {
        return;
    }
    {
        std::lock_guard<std::mutex> autoLock(heartbeatMutex_);
        heartBeatCount_++;
    }
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([this, taskId]() {
        if (heartBeatCount_ >= HEARTBEAT_PERIOD) {
            // heartbeat block twice should finish task now
            SetTaskFailed(taskId, -E_CLOUD_ERROR);
        } else {
            int ret = cloudDB_.HeartBeat();
            if (ret != E_OK) {
                HeartBeatFailed(taskId, ret);
            } else {
                failedHeartBeatCount_ = 0;
            }
        }
        {
            std::lock_guard<std::mutex> autoLock(heartbeatMutex_);
            heartBeatCount_--;
        }
        heartbeatCv_.notify_all();
    });
    if (errCode != E_OK) {
        LOGW("[CloudSyncer] schedule heartbeat task failed %d", errCode);
        {
            std::lock_guard<std::mutex> autoLock(heartbeatMutex_);
            heartBeatCount_--;
        }
        heartbeatCv_.notify_all();
    }
}

void CloudSyncer::HeartBeatFailed(TaskId taskId, int errCode)
{
    failedHeartBeatCount_++;
    if (failedHeartBeatCount_ < MAX_HEARTBEAT_FAILED_LIMIT) {
        return;
    }
    LOGW("[CloudSyncer] heartbeat failed too much times!");
    FinishHeartBeatTimer();
    SetTaskFailed(taskId, errCode);
}

void CloudSyncer::SetTaskFailed(TaskId taskId, int errCode)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
        return;
    }
    if (cloudTaskInfos_[taskId].errCode != E_OK) {
        return;
    }
    cloudTaskInfos_[taskId].errCode = errCode;
}

int32_t CloudSyncer::GetCloudSyncTaskCount()
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return taskQueue_.size();
}

int CloudSyncer::CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList,
    const RelationalSchemaObject &localSchema)
{
    std::lock_guard<std::mutex> lock(syncMutex_);
    int index = 1;
    for (const auto &tableName: tableNameList) {
        LOGD("[CloudSyncer] Start clean cloud water mark. table index: %d.", index);
        int ret = storageProxy_->CleanWaterMark(tableName);
        if (ret != E_OK) {
        LOGE("[CloudSyncer] failed to put cloud water mark after clean cloud data, %d.", ret);
            return ret;
        }
        index++;
    }
    int errCode = storageProxy_->StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] failed to start Transaction before clean cloud data, %d", errCode);
        return errCode;
    }

    std::vector<Asset> assets;
    errCode = storageProxy_->CleanCloudData(mode, tableNameList, localSchema, assets);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] failed to clean cloud data, %d.", errCode);
        storageProxy_->Rollback();
        return errCode;
    }

    if (!assets.empty() && mode == FLAG_AND_DATA) {
        errCode = cloudDB_.RemoveLocalAssets(assets);
        if (errCode != E_OK) {
            LOGE("[Storage Executor] failed to remove local assets, %d.", errCode);
            storageProxy_->Rollback();
            return errCode;
        }
    }

    storageProxy_->Commit();
    return errCode;
}

int CloudSyncer::CleanWaterMarkInMemory(const std::set<std::string> &tableNameList)
{
    std::lock_guard<std::mutex> lock(syncMutex_);
    for (const auto &tableName: tableNameList) {
        int ret = storageProxy_->CleanWaterMarkInMemory(tableName);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] failed to clean cloud water mark in memory, %d.", ret);
            return ret;
        }
    }
    return E_OK;
}

void CloudSyncer::UpdateCloudWaterMark(TaskId taskId, const SyncParam &param)
{
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentContext_.cloudWaterMarks[param.info.tableName] = param.cloudWaterMark;
    }
}

int CloudSyncer::CommitDownloadResult(const DownloadItem &downloadItem, InnerProcessInfo &info,
    DownloadCommitList &commitList, int errCode)
{
    if (commitList.empty()) {
        return E_OK;
    }
    uint32_t successCount = 0u;
    int ret = HandleDownloadResult(downloadItem, info.tableName, commitList, successCount);
    if (errCode == E_OK) {
        info.downLoadInfo.failCount += (commitList.size() - successCount);
        info.downLoadInfo.successCount -= (commitList.size() - successCount);
    }
    if (ret != E_OK) {
        LOGE("Commit download result failed.%d", ret);
    }
    commitList.clear();
    return ret;
}

std::string CloudSyncer::GetIdentify() const
{
    return id_;
}

int CloudSyncer::TagStatusByStrategy(bool isExist, SyncParam &param, DataInfo &dataInfo, OpType &strategyOpResult)
{
    strategyOpResult = OpType::NOT_HANDLE;
    // ignore same record with local generate data
    if (dataInfo.localInfo.logInfo.device.empty() &&
        !CloudSyncUtils::NeedSaveData(dataInfo.localInfo.logInfo, dataInfo.cloudLogInfo)) {
        // not handle same data
        return E_OK;
    }
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (!currentContext_.strategy) {
            LOGE("[CloudSyncer] strategy has not been set when tag status, %d.", -E_INTERNAL_ERROR);
            return -E_INTERNAL_ERROR;
        }
        strategyOpResult = currentContext_.strategy->TagSyncDataStatus(isExist, dataInfo.localInfo.logInfo,
            dataInfo.cloudLogInfo);
    }
    if (strategyOpResult == OpType::DELETE) {
        param.deletePrimaryKeySet.insert(dataInfo.localInfo.logInfo.hashKey);
    }
    return E_OK;
}

TaskId CloudSyncer::GetNextTaskId()
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (!priorityTaskQueue_.empty()) {
        return priorityTaskQueue_.front();
    }
    if (!taskQueue_.empty()) {
        return taskQueue_.front();
    }
    return INVALID_TASK_ID;
}

void CloudSyncer::MarkCurrentTaskPausedIfNeed()
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (currentContext_.currentTaskId == INVALID_TASK_ID) {
        return;
    }
    if (cloudTaskInfos_.find(currentContext_.currentTaskId) == cloudTaskInfos_.end()) {
        return;
    }
    if (!cloudTaskInfos_[currentContext_.currentTaskId].priorityTask) {
        cloudTaskInfos_[currentContext_.currentTaskId].pause = true;
        LOGD("[CloudSyncer] Mark taskId %" PRIu64 " paused success", currentContext_.currentTaskId);
    }
}

void CloudSyncer::SetCurrentTaskFailedWithoutLock(int errCode)
{
    if (currentContext_.currentTaskId == INVALID_TASK_ID) {
        return;
    }
    cloudTaskInfos_[currentContext_.currentTaskId].errCode = errCode;
}

int CloudSyncer::LockCloudIfNeed(TaskId taskId)
{
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (currentContext_.locker != nullptr) {
            LOGD("[CloudSyncer] lock exist");
            return E_OK;
        }
    }
    std::shared_ptr<CloudLocker> locker = nullptr;
    int errCode = CloudLocker::BuildCloudLock([taskId, this]() {
        return LockCloud(taskId);
    }, [this]() {
        int unlockCode = UnlockCloud();
        if (unlockCode != E_OK) {
            SetCurrentTaskFailedWithoutLock(unlockCode);
        }
    }, locker);
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentContext_.locker = locker;
    }
    return errCode;
}

void CloudSyncer::UnlockIfNeed()
{
    std::shared_ptr<CloudLocker> cacheLocker;
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (currentContext_.locker == nullptr) {
            LOGW("[CloudSyncer] locker is nullptr when unlock it"); // should not happen
        }
        cacheLocker = currentContext_.locker;
        currentContext_.locker = nullptr;
    }
    // unlock without mutex
    cacheLocker = nullptr;
}

void CloudSyncer::ClearCurrentContextWithoutLock()
{
    currentContext_.currentTaskId = INVALID_TASK_ID;
    currentContext_.notifier = nullptr;
    currentContext_.strategy = nullptr;
    currentContext_.tableName.clear();
    currentContext_.assetDownloadList.clear();
    currentContext_.assetFields.clear();
    currentContext_.assetsInfo.clear();
    currentContext_.cloudWaterMarks.clear();
    currentContext_.isNeedUpload = false;
    currentContext_.isDownloadFinished.clear();
    currentContext_.currentState = CloudSyncState::IDLE;
    currentContext_.currentUserIndex = 0;
}

void CloudSyncer::ClearContextAndNotify(TaskId taskId, int errCode)
{
    std::shared_ptr<ProcessNotifier> notifier = nullptr;
    CloudTaskInfo info;
    {
        // clear current context
        std::lock_guard<std::mutex> autoLock(dataLock_);
        notifier = currentContext_.notifier;
        ClearCurrentContextWithoutLock();
        if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) { // should not happen
            LOGW("[CloudSyncer] taskId %" PRIu64 " has been finished!", taskId);
            contextCv_.notify_one();
            return;
        }
        info = std::move(cloudTaskInfos_[taskId]);
        cloudTaskInfos_.erase(taskId);
        resumeTaskInfos_.erase(taskId);
    }
    contextCv_.notify_one();
    if (info.errCode == E_OK) {
        info.errCode = errCode;
    }
    LOGI("[CloudSyncer] finished taskId %" PRIu64 " errCode %d", taskId, info.errCode);
    info.status = ProcessStatus::FINISHED;
    if (notifier != nullptr) {
        notifier->NotifyProcess(info, {}, true);
    }
    // generate compensated sync
    if (!info.priorityTask) {
        CloudTaskInfo taskInfo = CloudSyncUtils::InitCompensatedSyncTaskInfo();
        GenerateCompensatedSync(taskInfo);
    }
}

int CloudSyncer::DownloadOneBatch(TaskId taskId, SyncParam &param, bool isFirstDownload)
{
    int ret = CheckTaskIdValid(taskId);
    if (ret != E_OK) {
        return ret;
    }
    bool abort = false;
    ret = DownloadDataFromCloud(taskId, param, abort, isFirstDownload);
    if (abort) {
        return ret;
    }
    // Save data in transaction, update cloud water mark, notify process and changed data
    ret = SaveDataNotifyProcess(taskId, param);
    if (ret == -E_TASK_PAUSED) {
        return ret;
    }
    if (ret != E_OK) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        param.info.tableStatus = ProcessStatus::FINISHED;
        currentContext_.notifier->UpdateProcess(param.info);
        return ret;
    }
    (void)NotifyInDownload(taskId, param, isFirstDownload);
    return SaveCloudWaterMark(param.tableName, taskId);
}

int CloudSyncer::DownloadOneAssetRecord(const std::set<Key> &dupHashKeySet, const DownloadList &downloadList,
    DownloadItem &downloadItem, InnerProcessInfo &info, ChangedData &changedAssets)
{
    CloudStorageUtils::EraseNoChangeAsset(downloadItem.assets);
    if (downloadItem.assets.empty()) { // Download data (include deleting)
        return E_OK;
    }
    bool isSharedTable = false;
    int errorCode = storageProxy_->IsSharedTable(info.tableName, isSharedTable);
    if (errorCode != E_OK) {
        LOGE("[CloudSyncer] DownloadOneAssetRecord cannot judge the table is a shared table. %d", errorCode);
        return errorCode;
    }
    if (!isSharedTable) {
        errorCode = DownloadAssetsOneByOne(info, downloadItem, downloadItem.assets);
        if (errorCode == -E_NOT_SET) {
            return -E_NOT_SET;
        }
    } else {
        // share table will not download asset, need to reset the status
        for (auto &entry: downloadItem.assets) {
            for (auto &asset: entry.second) {
                asset.status = AssetStatus::NORMAL;
            }
        }
    }
    if (errorCode != E_OK) {
        info.downLoadInfo.failCount += 1;
        info.downLoadInfo.successCount -= 1;
    }
    if (dupHashKeySet.find(downloadItem.hashKey) == dupHashKeySet.end()) {
        changedAssets.primaryData[CloudSyncUtils::OpTypeToChangeType(downloadItem.strategy)].push_back(
            downloadItem.primaryKeyValList);
    } else if (downloadItem.strategy == OpType::INSERT) {
        changedAssets.primaryData[ChangeType::OP_UPDATE].push_back(downloadItem.primaryKeyValList);
    }
    // If the assets are DELETE, needn't fill back cloud assets.
    if (downloadItem.strategy == OpType::DELETE) {
        return E_OK;
    }
    return errorCode;
}

int CloudSyncer::GetSyncParamForDownload(TaskId taskId, SyncParam &param)
{
    if (IsCurrentTableResume(taskId, false)) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (resumeTaskInfos_[taskId].syncParam.tableName == currentContext_.tableName) {
            param = resumeTaskInfos_[taskId].syncParam;
            resumeTaskInfos_[taskId].syncParam = {};
            LOGD("[CloudSyncer] Get sync param from cache");
            return E_OK;
        }
    }
    int ret = GetCurrentTableName(param.tableName);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid table name for syncing: %d", ret);
        return ret;
    }
    param.info.tableName = param.tableName;
    std::vector<Field> assetFields;
    // only no primary key and composite primary key contains rowid.
    ret = storageProxy_->GetPrimaryColNamesWithAssetsFields(param.tableName, param.pkColNames, assetFields);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot get primary column names: %d", ret);
        return ret;
    }
    {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        currentContext_.assetFields[currentContext_.tableName] = assetFields;
    }
    param.isSinglePrimaryKey = CloudSyncUtils::IsSinglePrimaryKey(param.pkColNames);
    if (!IsModeForcePull(taskId) && !(IsPriorityTask(taskId) && !IsQueryListEmpty(taskId))) {
        ret = storageProxy_->GetCloudWaterMark(param.tableName, param.cloudWaterMark);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Cannot get cloud water level from cloud meta data: %d.", ret);
        }
        ReloadCloudWaterMarkIfNeed(param.tableName, param.cloudWaterMark);
    }
    currentContext_.notifier->GetDownloadInfoByTableName(param.info);
    return ret;
}

bool CloudSyncer::IsCurrentTableResume(TaskId taskId, bool upload)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (!cloudTaskInfos_[taskId].resume) {
        return false;
    }
    if (currentContext_.tableName != resumeTaskInfos_[taskId].context.tableName) {
        return false;
    }
    return upload == resumeTaskInfos_[taskId].upload;
}

int CloudSyncer::DownloadDataFromCloud(TaskId taskId, SyncParam &param, bool &abort,
    bool isFirstDownload)
{
    if (IsCurrentTableResume(taskId, false)) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        if (resumeTaskInfos_[taskId].skipQuery) {
            LOGD("[CloudSyncer] skip query");
            return E_OK;
        }
    }
    // Get cloud data after cloud water mark
    param.info.tableStatus = ProcessStatus::PROCESSING;
    param.downloadData = {};
    int ret = QueryCloudData(taskId, param.info.tableName, param.cloudWaterMark, param.downloadData);
    if (ret == -E_QUERY_END) {
        // Won't break here since downloadData may not be null
        param.isLastBatch = true;
    } else if (ret != E_OK) {
        std::lock_guard<std::mutex> autoLock(dataLock_);
        param.info.tableStatus = ProcessStatus::FINISHED;
        currentContext_.notifier->UpdateProcess(param.info);
        abort = true;
        return ret;
    }
    if (param.downloadData.data.empty()) {
        if (ret == E_OK || isFirstDownload) {
            LOGD("[CloudSyncer] try to query cloud data use increment water mark");
            UpdateCloudWaterMark(taskId, param);
        }
        if (isFirstDownload) {
            NotifyInEmptyDownload(taskId, param.info);
        }
        abort = true;
    }
    return E_OK;
}

size_t CloudSyncer::GetDownloadAssetIndex(TaskId taskId)
{
    size_t index = 0u;
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (resumeTaskInfos_[taskId].lastDownloadIndex != 0u) {
        index = resumeTaskInfos_[taskId].lastDownloadIndex;
        resumeTaskInfos_[taskId].lastDownloadIndex = 0u;
    }
    return index;
}

size_t CloudSyncer::GetStartTableIndex(TaskId taskId, bool upload)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    if (!cloudTaskInfos_[taskId].resume) {
        return 0u;
    }
    if (upload != resumeTaskInfos_[taskId].upload) {
        return upload ? 0u : cloudTaskInfos_[taskId].table.size();
    }
    for (size_t i = 0; i < cloudTaskInfos_[taskId].table.size(); ++i) {
        if (resumeTaskInfos_[taskId].context.tableName == cloudTaskInfos_[taskId].table[i]) {
            return i;
        }
    }
    return 0u;
}

uint32_t CloudSyncer::GetCurrentTableUploadBatchIndex()
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return currentContext_.notifier->GetUploadBatchIndex(currentContext_.tableName);
}

void CloudSyncer::RecordWaterMark(TaskId taskId, Timestamp waterMark)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    resumeTaskInfos_[taskId].lastLocalWatermark = waterMark;
}

Timestamp CloudSyncer::GetResumeWaterMark(TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return resumeTaskInfos_[taskId].lastLocalWatermark;
}

CloudSyncer::InnerProcessInfo CloudSyncer::GetInnerProcessInfo(const std::string &tableName, UploadParam &uploadParam)
{
    InnerProcessInfo info;
    info.tableName = tableName;
    info.tableStatus = ProcessStatus::PROCESSING;
    ReloadUploadInfoIfNeed(uploadParam.taskId, uploadParam, info);
    return info;
}

void CloudSyncer::SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback)
{
    cloudDB_.SetGenCloudVersionCallback(callback);
}
} // namespace DistributedDB
