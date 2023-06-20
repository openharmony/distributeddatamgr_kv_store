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

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_storage_utils.h"
#include "db_errno.h"
#include "cloud/icloud_db.h"
#include "kv_store_errno.h"
#include "log_print.h"
#include "platform_specific.h"
#include "runtime_context.h"
#include "strategy_factory.h"
#include "storage_proxy.h"
#include "store_types.h"

namespace DistributedDB {
namespace {
    const TaskId INVALID_TASK_ID = 0u;
    const int MAX_HEARTBEAT_FAILED_LIMIT = 2;
    const int HEARTBEAT_PERIOD = 3;
}
CloudSyncer::CloudSyncer(std::shared_ptr<StorageProxy> storageProxy)
    : currentTaskId_(INVALID_TASK_ID),
      storageProxy_(std::move(storageProxy)),
      queuedManualSyncLimit_(DBConstant::QUEUED_SYNC_LIMIT_DEFAULT),
      closed_(false),
      timerId_(0u),
      heartBeatCount_(0),
      failedHeartBeatCount_(0)
{
}

int CloudSyncer::Sync(const std::vector<DeviceID> &devices, SyncMode mode,
    const std::vector<std::string> &tables, const SyncProcessCallback &callback, int64_t waitTime)
{
    int errCode = CheckParamValid(devices, mode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (cloudDB_.IsNotExistCloudDB()) {
        return -E_CLOUD_ERROR;
    }
    if (closed_) {
        return -E_DB_CLOSED;
    }
    CloudTaskInfo taskInfo;
    taskInfo.mode = mode;
    taskInfo.table = tables;
    taskInfo.callback = callback;
    taskInfo.timeout = waitTime;
    taskInfo.devices = devices;
    errCode = TryToAddSyncTask(std::move(taskInfo));
    if (errCode != E_OK) {
        return errCode;
    }
    return TriggerSync();
}

int CloudSyncer::SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDB)
{
    cloudDB_.SetCloudDB(cloudDB);
    LOGI("[CloudSyncer] SetCloudDB finish");
    return E_OK;
}

int CloudSyncer::SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader)
{
    cloudDB_.SetIAssetLoader(loader);
    LOGI("[CloudSyncer] SetIAssetLoader finish");
    return E_OK;
}

void CloudSyncer::Close()
{
    closed_ = true;
    cloudDB_.Close();
    {
        LOGD("[CloudSyncer] begin wait current task finished");
        std::unique_lock<std::mutex> uniqueLock(contextLock_);
        contextCv_.wait(uniqueLock, [this]() {
            return currentContext_.currentTaskId == INVALID_TASK_ID;
        });
        LOGD("[CloudSyncer] current task has been finished");
    }

    // copy all task from queue
    std::vector<CloudTaskInfo> infoList;
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        for (const auto &item: cloudTaskInfos_) {
            infoList.push_back(item.second);
        }
        taskQueue_.clear();
        cloudTaskInfos_.clear();
    }
    // notify all DB_CLOSED
    for (auto &info: infoList) {
        info.status = ProcessStatus::FINISHED;
        info.errCode = -E_DB_CLOSED;
        ProcessNotifier notifier;
        notifier.Init(info.table, info.devices);
        notifier.NotifyProcess(info, {}, true);
        LOGI("[CloudSyncer] finished taskId %" PRIu64 " errCode %d", info.taskId, info.errCode);
    }
    storageProxy_->Close();
}

void CloudSyncer::ProcessNotifier::Init(const std::vector<std::string> &tableName,
    const std::vector<std::string> &devices)
{
    std::lock_guard<std::mutex> autoLock(processMutex_);
    syncProcess_.errCode = OK;
    syncProcess_.process = ProcessStatus::PROCESSING;
    for (const auto &table: tableName) {
        TableProcessInfo tableInfo = {
            .process = ProcessStatus::PREPARED
        };
        syncProcess_.tableProcess[table] = tableInfo;
    }
    devices_ = devices;
}

void CloudSyncer::ProcessNotifier::UpdateProcess(const CloudSyncer::InnerProcessInfo &process)
{
    if (process.tableName.empty()) {
        return;
    }
    std::lock_guard<std::mutex> autoLock(processMutex_);
    syncProcess_.tableProcess[process.tableName].process = process.tableStatus;
    if (process.downLoadInfo.batchIndex != 0u) {
        LOGD("[ProcessNotifier] update download process index: %" PRIu32, process.downLoadInfo.batchIndex);
        syncProcess_.tableProcess[process.tableName].downLoadInfo.batchIndex = process.downLoadInfo.batchIndex;
        syncProcess_.tableProcess[process.tableName].downLoadInfo.total = process.downLoadInfo.total;
        syncProcess_.tableProcess[process.tableName].downLoadInfo.failCount = process.downLoadInfo.failCount;
        syncProcess_.tableProcess[process.tableName].downLoadInfo.successCount = process.downLoadInfo.successCount;
    }
    if (process.upLoadInfo.batchIndex != 0u) {
        LOGD("[ProcessNotifier] update upload process index: %" PRIu32, process.upLoadInfo.batchIndex);
        syncProcess_.tableProcess[process.tableName].upLoadInfo.batchIndex = process.upLoadInfo.batchIndex;
        syncProcess_.tableProcess[process.tableName].upLoadInfo.total = process.upLoadInfo.total;
        syncProcess_.tableProcess[process.tableName].upLoadInfo.failCount = process.upLoadInfo.failCount;
        syncProcess_.tableProcess[process.tableName].upLoadInfo.successCount = process.upLoadInfo.successCount;
    }
}

void CloudSyncer::ProcessNotifier::NotifyProcess(const CloudTaskInfo &taskInfo, const InnerProcessInfo &process,
    bool notifyWhenError)
{
    UpdateProcess(process);
    std::map<std::string, SyncProcess> currentProcess;
    {
        std::lock_guard<std::mutex> autoLock(processMutex_);
        if (!notifyWhenError && taskInfo.errCode != E_OK) {
            LOGD("[ProcessNotifier] task has error, do not notify now");
            return;
        }
        syncProcess_.errCode = TransferDBErrno(taskInfo.errCode);
        syncProcess_.process = taskInfo.status;
        for (const auto &device : devices_) {
            // make sure only one device
            currentProcess[device] = syncProcess_;
        }
    }
    SyncProcessCallback callback = taskInfo.callback;
    if (!callback) {
        return;
    }
    int errCode = RuntimeContext::GetInstance()->ScheduleTask([callback, currentProcess]() {
        callback(currentProcess);
        LOGD("[ProcessNotifier] notify process finish");
    });
    if (errCode != E_OK) {
        LOGW("[ProcessNotifier] schedule notify process failed %d", errCode);
    }
}

std::vector<std::string> CloudSyncer::ProcessNotifier::GetDevices() const
{
    return devices_;
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

void CloudSyncer::DoSyncIfNeed()
{
    if (closed_) {
        return;
    }
    std::lock_guard<std::mutex> lock(syncMutex_);
    // get taskId from queue
    TaskId triggerTaskId;
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        if (taskQueue_.empty()) {
            return;
        }
        triggerTaskId = taskQueue_.front();
    }
    // pop taskId in queue
    if (PrepareSync(triggerTaskId) != E_OK) {
        return;
    }
    // do sync logic
    int errCode = DoSync(triggerTaskId);
    // finished after sync
    DoFinished(triggerTaskId, errCode, {});
    // do next task async
    (void)TriggerSync();
}

int CloudSyncer::DoSync(TaskId taskId)
{
    CloudTaskInfo taskInfo;
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        taskInfo = cloudTaskInfos_[taskId];
    }
    int errCode = LockCloud(taskId);
    if (errCode != E_OK) {
        return errCode;
    }
    bool needUpload = true;
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        needUpload = currentContext_.strategy->JudgeUpload();
    }
    errCode = DoSyncInner(taskInfo, needUpload);
    int unlockCode = UnlockCloud();
    if (errCode == E_OK) {
        errCode = unlockCode;
    }
    return errCode;
}

int CloudSyncer::DoSyncInner(const CloudTaskInfo &taskInfo, const bool &needUpload)
{
    int errCode = E_OK;
    for (size_t i = 0; i < taskInfo.table.size(); ++i) {
        LOGD("[CloudSyncer] try download table, index: %zu", i + 1);
        errCode = CheckTaskIdValid(taskInfo.taskId);
        if (errCode != E_OK) {
            LOGE("[CloudSyncer] task is invalid, abort sync");
            return errCode;
        }
        {
            std::lock_guard<std::mutex> autoLock(contextLock_);
            currentContext_.tableName = taskInfo.table[i];
        }
        errCode = DoDownload(taskInfo.taskId);
        if (errCode != E_OK) {
            LOGE("[CloudSyncer] download failed %d", errCode);
            return errCode;
        }
    }

    if (needUpload) {
        errCode = storageProxy_->StartTransaction();
        if (errCode != E_OK) {
            LOGE("[CloudSyncer] start transaction Failed before doing upload.");
            return errCode;
        }
        for (size_t i = 0; i < taskInfo.table.size(); ++i) {
            LOGD("[CloudSyncer] try upload table, index: %zu", i + 1);
            errCode = CheckTaskIdValid(taskInfo.taskId);
            if (errCode != E_OK) {
                LOGE("[CloudSyncer] task is invalid, abort sync");
                break;
            }
            {
                std::lock_guard<std::mutex> autoLock(contextLock_);
                currentContext_.tableName = taskInfo.table[i];
            }
            errCode = DoUpload(taskInfo.taskId, i == (taskInfo.table.size() - 1u));
            if (errCode != E_OK) {
                LOGE("[CloudSyncer] upload failed %d", errCode);
                break;
            }
        }
        if (errCode == E_OK) {
            storageProxy_->Commit();
        } else {
            storageProxy_->Rollback();
        }
    }
    return errCode;
}

void CloudSyncer::DoFinished(TaskId taskId, int errCode, const InnerProcessInfo &processInfo)
{
    std::shared_ptr<ProcessNotifier> notifier = nullptr;
    {
        // check current task is running or not
        std::lock_guard<std::mutex> autoLock(contextLock_);
        if (currentContext_.currentTaskId != taskId) { // should not happen
            LOGW("[CloudSyncer] taskId %" PRIu64 " not exist in context!", taskId);
            return;
        }
        currentContext_.currentTaskId = INVALID_TASK_ID;
        notifier = currentContext_.notifier;
        currentContext_.notifier = nullptr;
        currentContext_.strategy = nullptr;
        currentContext_.tableName.clear();
        currentContext_.assetDownloadList.completeDownloadList.clear();
        currentContext_.assetDownloadList.downloadList.clear();
        currentContext_.assetFields.clear();
        currentContext_.assetsInfo.clear();
        currentContext_.cloudWaterMarks.clear();
    }
    CloudTaskInfo info;
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) { // should not happen
            LOGW("[CloudSyncer] taskId %" PRIu64 " has been finished!", taskId);
            contextCv_.notify_one();
            return;
        }
        info = std::move(cloudTaskInfos_[taskId]);
        cloudTaskInfos_.erase(taskId);
    }
    contextCv_.notify_one();
    if (info.errCode == E_OK) {
        info.errCode = errCode;
    }
    LOGI("[CloudSyncer] finished taskId %" PRIu64 " errCode %d", taskId, info.errCode);
    info.status = ProcessStatus::FINISHED;
    if (notifier != nullptr) {
        notifier->NotifyProcess(info, processInfo, true);
    }
}

static int GetCloudPkVals(VBucket &datum, const std::vector<std::string> &pkColNames, int64_t dataKey,
    std::vector<Type> &cloudPkVals)
{
    if (!cloudPkVals.empty()) {
        LOGE("Output paramater should be empty");
        return -E_INVALID_ARGS;
    }
    if (pkColNames.size() == 1 && pkColNames[0] == CloudDbConstant::ROW_ID_FIELD_NAME) {
        // if data don't have primary key, then use rowID as value
        cloudPkVals.emplace_back(std::move(dataKey));
        return E_OK;
    }
    for (const auto &pkColName : pkColNames) {
        auto iter = datum.find(pkColName);
        if (iter == datum.end()) {
            LOGE("Cloud data do not contain expected primary field value");
            return -E_CLOUD_ERROR;
        }
        cloudPkVals.push_back(datum[pkColName]);
    }
    return E_OK;
}

static int SaveChangedtData(VBucket &datum, ChangedData &changedData, DataInfoWithLog &localInfo, ChangeType type)
{
    int ret = E_OK;
    std::vector<Type> cloudPkVals;
    if (type == ChangeType::OP_DELETE) {
        ret = GetCloudPkVals(localInfo.primaryKeys, changedData.field, localInfo.logInfo.dataKey, cloudPkVals);
    } else {
        ret = GetCloudPkVals(datum, changedData.field, localInfo.logInfo.dataKey, cloudPkVals);
    }
    if (ret != E_OK) {
        return ret;
    }
    changedData.primaryData[type].emplace_back(std::move(cloudPkVals));
    return E_OK;
}

static bool shouldSaveData(const LogInfo &localLogInfo, const LogInfo &cloudLogInfo)
{
    // if timeStamp, write timestamp, cloudGid are all the same,
    // we thought that the datum is mostly be the same between cloud and local
    // However, there are still slightly possibility that it may be created from different device,
    // So, during the strategy policy [i.e. TagSyncDataStatus], the datum was tagged as UPDATE
    // But we won't notify the datum
    if (localLogInfo.timestamp == cloudLogInfo.timestamp &&
        localLogInfo.wTimestamp == cloudLogInfo.wTimestamp &&
        localLogInfo.cloudGid == cloudLogInfo.cloudGid) {
            return false;
        }
    return true;
}

int CloudSyncer::SaveChangedData(DownloadData &downloadData, int dataIndex, DataInfoWithLog &localInfo,
    LogInfo &cloudLogInfo, ChangedData &changedData, std::vector<size_t> &InsertDataNoPrimaryKeys)
{
    // For no primary key situation,
    if (downloadData.opType[dataIndex] == OpType::INSERT && changedData.field.size() == 1 &&
        changedData.field[0] == CloudDbConstant::ROW_ID_FIELD_NAME) {
        InsertDataNoPrimaryKeys.push_back(dataIndex);
        return E_OK;
    }
    switch (downloadData.opType[dataIndex]) {
        case OpType::INSERT:
            return SaveChangedtData(
                downloadData.data[dataIndex], changedData, localInfo, ChangeType::OP_INSERT);
        case OpType::UPDATE:
            if (shouldSaveData(localInfo.logInfo, cloudLogInfo)) {
                return SaveChangedtData(
                    downloadData.data[dataIndex], changedData, localInfo, ChangeType::OP_UPDATE);
            }
            break;
        case OpType::DELETE:
            return SaveChangedtData(
                downloadData.data[dataIndex], changedData, localInfo, ChangeType::OP_DELETE);
        default:
            break;
    }
    return E_OK;
}

static bool IsChngDataEmpty(ChangedData &changedData)
{
    return changedData.primaryData[ChangeType::OP_INSERT].empty() ||
           changedData.primaryData[ChangeType::OP_UPDATE].empty() ||
           changedData.primaryData[ChangeType::OP_DELETE].empty();
}

static LogInfo GetCloudLogInfo(VBucket &datum)
{
    LogInfo cloudLogInfo = { 0 };
    cloudLogInfo.timestamp = (Timestamp)std::get<int64_t>(datum[CloudDbConstant::MODIFY_FIELD]);
    cloudLogInfo.wTimestamp = (Timestamp)std::get<int64_t>(datum[CloudDbConstant::CREATE_FIELD]);
    cloudLogInfo.flag = (std::get<bool>(datum[CloudDbConstant::DELETE_FIELD])) ? 1u : 0u;
    cloudLogInfo.cloudGid = std::get<std::string>(datum[CloudDbConstant::GID_FIELD]);
    return cloudLogInfo;
}

static void UpdateChangedData(
    DownloadData &downloadData, const std::vector<size_t> &InsertDataNoPrimaryKeys, ChangedData &changedData)
{
    if (InsertDataNoPrimaryKeys.empty()) {
        return;
    }
    for (size_t j : InsertDataNoPrimaryKeys) {
        VBucket &datum = downloadData.data[j];
        changedData.primaryData[ChangeType::OP_INSERT].push_back({datum[CloudDbConstant::ROW_ID_FIELD_NAME]});
    }
}

static void TagAsset(AssetOpType flag, AssetStatus status, Asset &asset, Assets &res)
{
    asset.flag = static_cast<uint32_t>(flag);
    asset.status = static_cast<uint32_t>(status);
    Timestamp timestamp;
    OS::GetCurrentSysTimeInMicrosecond(timestamp);
    asset.timestamp = timestamp / CloudDbConstant::TEN_THOUSAND;
    res.push_back(asset);
}

static void TagAssets(AssetOpType flag, AssetStatus status, Assets &assets, Assets &res)
{
    for (Asset &asset : assets) {
        TagAsset(flag, status, asset, res);
    }
}

template<typename T>
static bool IsDataContainField(const std::string &assetFieldName, VBucket &data)
{
    auto assetIter = data.find(assetFieldName);
    if (assetIter == data.end()) {
        return false;
    }
    if (assetIter->second.index() == TYPE_INDEX<Assets>) {
        if (std::get<Assets>(assetIter->second).empty()) {
            return false;
        }
    }
    if (assetIter->second.index() != TYPE_INDEX<T>) {
        return false;
    }
    return true;
}

// AssetOpType and AssetStatus will be tagged, assets to be changed will be returned
static Assets TagAsset(const std::string &assetFieldName, VBucket &coveredData, VBucket &beCoveredData)
{
    Assets res = {};
    bool beCoveredHasAsset = IsDataContainField<Asset>(assetFieldName, beCoveredData) ||
        IsDataContainField<Assets>(assetFieldName, beCoveredData);
    bool coveredHasAsset = IsDataContainField<Asset>(assetFieldName, coveredData);
    if (!beCoveredHasAsset) {
        if (!coveredHasAsset) {
            LOGD("Both data do not contain certain asset field");
            return res;
        }
        LOGD("coveredData will be regarded as insert");
        TagAsset(AssetOpType::INSERT, AssetStatus::DOWNLOADING, std::get<Asset>(coveredData[assetFieldName]), res);
        return res;
    }
    if (!coveredHasAsset) {
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, std::get<Asset>(beCoveredData[assetFieldName]), res);
        return res;
    }
    Asset &covered = std::get<Asset>(coveredData[assetFieldName]);
    Assets beCoveredDataInAssets;
    Asset beCovered;
    int ret = CloudStorageUtils::GetValueFromVBucket(assetFieldName, beCoveredData, beCoveredDataInAssets);
    if (ret != E_OK) {
        // This indicates that asset in cloudData is stored as Asset
        beCovered = std::get<Asset>(beCoveredData[assetFieldName]);
    } else {
        // This indicates that asset in cloudData is stored as ASSETS
        // first element in assets will be the target asset
        beCovered = beCoveredDataInAssets[0];
    }
    if (covered.name != beCovered.name) {
        TagAsset(AssetOpType::INSERT, AssetStatus::DOWNLOADING, covered, res);
        TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, beCovered, res);
        return res;
    }
    if (covered.hash != beCovered.hash) {
        TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, covered, res);
    }
    return res;
}

static std::unordered_map<std::string, size_t> GenAssetsMap(Assets &assets)
{
    std::unordered_map<std::string, size_t> assetsMap;
    for (size_t i = 0; i < assets.size(); i++) {
        assetsMap[assets[i].name] = i;
    }
    return assetsMap;
}

// AssetOpType and AssetStatus will be tagged, assets to be changed will be returned
// use VBucket rather than Type because we need to check whether it is empty
static Assets TagAssets(const std::string &assetFieldName, VBucket &coveredData, VBucket &beCoveredData,
    bool WriteToCoveredData)
{
    Assets res = {};
    bool beCoveredHasAssets = IsDataContainField<Assets>(assetFieldName, beCoveredData);
    bool coveredHasAssets = IsDataContainField<Assets>(assetFieldName, coveredData);
    if (!beCoveredHasAssets) {
        if (!coveredHasAssets) {
            return res;
        }
        // all the element in assets will be set to INSERT
        TagAssets(AssetOpType::INSERT, AssetStatus::DOWNLOADING, std::get<Assets>(coveredData[assetFieldName]), res);
        return res;
    }
    if (!coveredHasAssets) {
        // all the element in assets will be set to DELETE
        TagAssets(AssetOpType::DELETE, AssetStatus::DOWNLOADING, std::get<Assets>(beCoveredData[assetFieldName]), res);
        if (WriteToCoveredData) {
            LOGD("Write assets to be deleted from beCoveredData to CoveredData");
            coveredData[assetFieldName] = res;
        }
        return res;
    }
    Assets &covered = std::get<Assets>(coveredData[assetFieldName]);
    Assets &beCovered = std::get<Assets>(beCoveredData[assetFieldName]);
    std::unordered_map<std::string, size_t> CoveredAssetsMap = GenAssetsMap(covered);
    for (Asset &beCoveredAsset : beCovered) {
        auto it = CoveredAssetsMap.find(beCoveredAsset.name);
        if (it == CoveredAssetsMap.end()) {
            TagAsset(AssetOpType::DELETE, AssetStatus::DOWNLOADING, beCoveredAsset, res);
            if (WriteToCoveredData) {
                std::get<Assets>(coveredData[assetFieldName]).push_back(beCoveredAsset);
            }
            continue;
        }
        Asset &coveredAsset = covered[it->second];
        if (beCoveredAsset.hash != coveredAsset.hash) {
            TagAsset(AssetOpType::UPDATE, AssetStatus::DOWNLOADING, coveredAsset, res);
        }
        // Erase element which has been handled, remaining element will be set to Insert
        CoveredAssetsMap.erase(it);
        // flag in Asset is defaultly set to NoChange, so we just continue
        continue;
    }
    for (auto &noHandledAssetKvPair : CoveredAssetsMap) {
        TagAsset(AssetOpType::INSERT, AssetStatus::DOWNLOADING, covered[noHandledAssetKvPair.second], res);
    }
    return res;
}

bool CloudSyncer::ShouldProcessAssets()
{
    std::lock_guard<std::mutex> autoLock(contextLock_);
    if (currentContext_.assetFields[currentContext_.tableName].empty()) {
        LOGI("Current table do not contain assets, thereby we needn't download assets");
        return false;
    }
    return true;
}

std::map<std::string, Assets> CloudSyncer::TagAssetsInSingleRecord(VBucket &CoveredData, VBucket &BeCoveredData,
    bool WriteToCoveredData)
{
    // Define a map to store the result
    std::map<std::string, Assets> res = {};
    std::vector<Field> assetFields;
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        assetFields = currentContext_.assetFields[currentContext_.tableName];
    }
    // For every column contain asset or assets, assetFields are in context
    for (const Field &assetField : assetFields) {
        res[assetField.colName] = TagAssetsInSingleCol(CoveredData, BeCoveredData, assetField, WriteToCoveredData);
    }
    return res;
}

Assets CloudSyncer::TagAssetsInSingleCol(
    VBucket &CoveredData, VBucket &BeCoveredData, const Field &assetField, bool WriteToCoveredData)
{
    // Define a list to store the tagged result
    Assets assets = {};
    switch (assetField.type) {
        case TYPE_INDEX<Assets>: {
            assets = TagAssets(assetField.colName, CoveredData, BeCoveredData, WriteToCoveredData);
            break;
        }
        case TYPE_INDEX<Asset>: {
            assets = TagAsset(assetField.colName, CoveredData, BeCoveredData);
            break;
        }
        default:
            LOGW("Meet an unexpected type %d", assetField.type);
            break;
    }
    return assets;
}

int CloudSyncer::FillCloudAssets(
    const std::string &tableName, VBucket &normalAssets, VBucket &failedAssets)
{
    int ret = E_OK;
    if (!normalAssets.empty() && normalAssets.size() > 1) {
        ret = storageProxy_->FillCloudAssetForDownload(tableName, normalAssets, true);
        if (ret != E_OK) {
            return ret;
        }
    }
    if (!failedAssets.empty() && failedAssets.size() > 1) {
        ret = storageProxy_->FillCloudAssetForDownload(tableName, failedAssets, false);
        if (ret != E_OK) {
            return ret;
        }
    }
    return E_OK;
}

int CloudSyncer::HandleDownloadResult(const std::string &tableName, std::string gid,
    std::map<std::string, Assets> &DownloadResult, bool setAllNormal)
{
    VBucket normalAssets;
    VBucket failedAssets;
    normalAssets[CloudDbConstant::GID_FIELD] = gid;
    failedAssets[CloudDbConstant::GID_FIELD] = gid;
    for (auto &assetKvPair : DownloadResult) {
        Assets &assets = assetKvPair.second;
        Assets normalAssetsList = {};
        Assets failedAssetsList = {};
        for (Asset &asset : assets) {
            // if success, write every attribute of asset into database
            if (static_cast<AssetStatus>(asset.status) == AssetStatus::NORMAL || setAllNormal) {
                normalAssetsList.push_back(asset);
                continue;
            }
            // if fail, partially write cloud asset attribute into database
            if (static_cast<AssetStatus>(asset.status) == AssetStatus::ABNORMAL) {
                failedAssetsList.push_back(asset);
            }
            if (static_cast<AssetStatus>(asset.status) == AssetStatus::DOWNLOADING) {
                LOGW("Asset status should not be DOWNLOADING after download operation");
            }
        }
        if (!normalAssetsList.empty()) {
            normalAssets[assetKvPair.first] = normalAssetsList;
        }
        if (!failedAssetsList.empty()) {
            failedAssets[assetKvPair.first] = failedAssetsList;
        }
    }
    return FillCloudAssets(tableName, normalAssets, failedAssets);
}

static ChangeType OpTypeToChangeType(OpType strategy)
{
    switch (strategy) {
        case OpType::INSERT:
            return OP_INSERT;
        case OpType::DELETE:
            return OP_DELETE;
        case OpType::UPDATE:
            return OP_UPDATE;
        default:
            return OP_BUTT;
    }
}

int CloudSyncer::CloudDbDownloadAssets(InnerProcessInfo &info, DownloadList &downloadList, bool willHandleResult,
    ChangedData &changedAssets)
{
    for (auto &assetsDownloadInfo : downloadList) {
        std::string gid = std::get<0>(assetsDownloadInfo);
        Type primaryKey = std::get<1>(assetsDownloadInfo);
        OpType strategy = std::get<2>(assetsDownloadInfo);
        std::map<std::string, Assets> assets = std::get<3>(assetsDownloadInfo);
        // Download data (include deleting)
        int errorCode = cloudDB_.Download(info.tableName, gid, primaryKey, assets);
        if (!willHandleResult || assets.empty()) {
            continue;
        }
        // Process result of each asset
        if (errorCode != E_OK) {
            // if not OK, update process info and handle download result seperately
            info.downLoadInfo.failCount++;
            info.downLoadInfo.successCount--;
            int ret = HandleDownloadResult(info.tableName, gid, assets, false);
            if (ret != E_OK) {
                return ret;
            }
            continue;
        }
        // if success, update ChangedData && Write every attribute of asset into database
        // All status will be set to NORMAL(0)
        changedAssets.primaryData[OpTypeToChangeType(strategy)].push_back({primaryKey});
        int ret = HandleDownloadResult(info.tableName, gid, assets, true);
        if (ret != E_OK) {
            return ret;
        }
    }
    return E_OK;
}

int CloudSyncer::DownloadNotifyAssets(InnerProcessInfo &info, std::vector<std::string> &pKColNames,
    ChangedData &changedAssets)
{
    if (!ShouldProcessAssets()) {
        return E_OK;
    }
    // update changed data info
    if (!IsChngDataEmpty(changedAssets)) {
        // changedData.primaryData should have no member inside
        return -E_INVALID_ARGS;
    }
    changedAssets.tableName = info.tableName;
    changedAssets.type = ChangedDataType::ASSET;
    changedAssets.field = pKColNames;
    // Get AssetDownloadList
    DownloadList downloadList;
    DownloadList completeDeletedList;
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        downloadList = currentContext_.assetDownloadList.downloadList;
        completeDeletedList = currentContext_.assetDownloadList.completeDownloadList;
    }
    // Download data (include deleting) will handle return Code in this situation
    int ret = CloudDbDownloadAssets(info, downloadList, true, changedAssets);
    if (ret != E_OK) {
        return ret;
    }
    // Download data (include deleting), won't handle return Code in this situation
    ret = CloudDbDownloadAssets(info, completeDeletedList, false, changedAssets);
    if (ret != E_OK) {
        return ret;
    }
    ret = NotifyChangedData(std::move(changedAssets));
    if (ret != E_OK) {
        LOGE("Cannot notify changed data due to error %d", ret);
        return ret;
    }
    return E_OK;
}

std::map<std::string, Assets> CloudSyncer::GetAssetsFromVBucket(VBucket &data)
{
    std::map<std::string, Assets> assets;
    std::vector<Field> fields;
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        fields = currentContext_.assetFields[currentContext_.tableName];
    }
    for (Field &field : fields) {
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

static bool IsCompositeKey(std::vector<std::string> &pKColNames)
{
    if (pKColNames.empty()) {
        return false;
    }
    if (pKColNames.size() > 1) {
        return true;
    }
    return false;
}

static bool IsNoPrimaryKey(std::vector<std::string> &pKColNames)
{
    if (pKColNames.empty()) {
        return true;
    }
    if (pKColNames.size() == 1 && pKColNames[0] == CloudDbConstant::ROW_ID_FIELD_NAME) {
        return true;
    }
    return false;
}

static bool IsSinglePrimaryKey(std::vector<std::string> &pKColNames)
{
    if (IsCompositeKey(pKColNames) || IsNoPrimaryKey(pKColNames)) {
        return false;
    }
    return true;
}

void CloudSyncer::TagStatus(bool isExist, const TableName &tableName, size_t idx, DownloadData &downloadData,
    std::vector<std::string> &pKColNames, DataInfoWithLog &localInfo, LogInfo &cloudLogInfo, VBucket &localAssetInfo,
    AssetDownloadList &assetsDownloadList)
{
    OpType strategy =
        currentContext_.strategy->TagSyncDataStatus(isExist, localInfo.logInfo, cloudLogInfo);
    downloadData.opType[idx] = strategy;
    if (!ShouldProcessAssets()) {
        return;
    }
    std::map<std::string, Assets> assetsMap;
    Type prefix;
    std::vector<Type> pKVals;
    int ret = E_OK;
    if (IsSinglePrimaryKey(pKColNames)) {
        if (strategy == OpType::DELETE) {
            ret = GetCloudPkVals(localInfo.primaryKeys, pKColNames, localInfo.logInfo.dataKey, pKVals);
        } else {
            ret = GetCloudPkVals(downloadData.data[idx], pKColNames, localInfo.logInfo.dataKey, pKVals);
        }
        prefix = pKVals[0];
    }
    switch (strategy)
    {
        case OpType::INSERT:
        case OpType::UPDATE:
            assetsMap = TagAssetsInSingleRecord(downloadData.data[idx], localAssetInfo, false);
            assetsDownloadList.downloadList.push_back(
                std::make_tuple(cloudLogInfo.cloudGid, prefix, strategy, assetsMap));
            break;
        case OpType::DELETE:
            assetsMap = TagAssetsInSingleRecord(downloadData.data[idx], localAssetInfo, false);
            assetsDownloadList.completeDownloadList.push_back(
                std::make_tuple(cloudLogInfo.cloudGid, prefix, strategy, assetsMap));
            break;
        case OpType::NOT_HANDLE:
        case OpType::ONLY_UPDATE_GID:
        case OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO: {// means upload need this data
            // Save the asset info into context
            assetsMap = GetAssetsFromVBucket(downloadData.data[idx]);
            if (!assetsMap.empty()) {
                {
                    std::lock_guard<std::mutex> autoLock(contextLock_);
                    if (currentContext_.assetsInfo.find(tableName) == currentContext_.assetsInfo.end()) {
                        currentContext_.assetsInfo[tableName] = {};
                    }
                    currentContext_.assetsInfo[tableName][cloudLogInfo.cloudGid] = assetsMap;
                }
            }
            break;
        }
        default:
            break;
    }
    return;
}

int CloudSyncer::SaveDatum(const std::string &tableName, size_t idx, DownloadData &downloadData,
    AssetDownloadList &assetsDownloadList, ChangedData &changedData, std::vector<size_t> &InsertDataNoPrimaryKeys)
{
    int ret = CheckDownloadDatum(downloadData.data[idx]);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid download data:%d", ret);
        return ret;
    }
    ModifyCloudDataTime(downloadData.data[idx]);
    DataInfoWithLog localInfo;
    VBucket localAssetInfo;
    bool isExist = true;
    ret = storageProxy_->GetInfoByPrimaryKeyOrGid(tableName, downloadData.data[idx], localInfo, localAssetInfo);
    if (ret == -E_NOT_FOUND) {
        isExist = false;
    } else if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot get cloud water level from cloud meta data: %d.", ret);
        return ret;
    }
    // Get cloudLogInfo from cloud data
    LogInfo cloudLogInfo = GetCloudLogInfo(downloadData.data[idx]);
    // Tag datum to get opType
    TagStatus(isExist, tableName, idx, downloadData, changedData.field, localInfo, cloudLogInfo, localAssetInfo,
        assetsDownloadList);
    ret = SaveChangedData(downloadData, idx, localInfo, cloudLogInfo, changedData, InsertDataNoPrimaryKeys);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot save changed data: %d.", ret);
        return ret;
    }
    return E_OK;
}

int CloudSyncer::SaveData(const TableName &tableName, DownloadData &downloadData,
    Info &downloadInfo, CloudWaterMark &latestCloudWaterMark, ChangedData &changedData)
{
    if (!IsChngDataEmpty(changedData)) {
        // changedData.primaryData should have no member inside
        return -E_INVALID_ARGS;
    }
    // Update download btach Info
    downloadInfo.batchIndex += 1;
    downloadInfo.total += downloadData.data.size();
    int ret = E_OK;
    std::vector<size_t> InsertDataNoPrimaryKeys;
    AssetDownloadList assetsDownloadList;
    for (size_t i = 0; i < downloadData.data.size(); i++) {
        ret = SaveDatum(tableName, i, downloadData, assetsDownloadList, changedData, InsertDataNoPrimaryKeys);
        if (ret != E_OK) {
            LOGE("Cannot save datum due to error code %d", ret);
            return ret;
        }
    }
    // Save assetsMap into current context
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        currentContext_.assetDownloadList = assetsDownloadList;
    }
    // save the data to the database by batch
    ret = storageProxy_->PutCloudSyncData(tableName, downloadData);
    if (ret != E_OK) {
        downloadInfo.failCount += downloadData.data.size();
        LOGE("[CloudSyncer] Cannot save the data to databse with error code: %d.", ret);
        return ret;
    }
    UpdateChangedData(downloadData, InsertDataNoPrimaryKeys, changedData);
    // Update downloadInfo
    downloadInfo.successCount += downloadData.data.size();
    // Get latest cloudWaterMark
    VBucket &lastData = downloadData.data.back();
    latestCloudWaterMark = std::get<std::string>(lastData[CloudDbConstant::CURSOR_FIELD]);
    return E_OK;
}

int CloudSyncer::PreCheck(CloudSyncer::TaskId &taskId, const TableName &tableName)
{
    // Check Input and Context Validity
    int ret = CheckTaskIdValid(taskId);
    if (ret != E_OK) {
        return ret;
    }
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
            LOGE("[CloudSyncer] Cloud Task Info does not exist taskId: , %" PRIu64 ".", taskId);
            return -E_INVALID_ARGS;
        }
    }
    if (currentContext_.strategy == nullptr) {
        LOGE("Strategy has not been initialized");
        return -E_INVALID_ARGS;
    }
    ret = storageProxy_->CheckSchema(tableName);
    if (ret != E_OK) {
        LOGE("A schema error occurred on the table to be synced, %d", ret);
        return ret;
    }
    return E_OK;
}

bool CloudSyncer::NeedNotifyChangedData(ChangedData &changedData)
{
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        if (IsModeForcePush(currentContext_.currentTaskId)) {
            return false;
        }
    }
    // when there have no data been changed, it needn't notified
    if (changedData.primaryData[OP_INSERT].empty() &&
        changedData.primaryData[OP_UPDATE].empty() &&
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
        std::lock_guard<std::mutex> autoLock(contextLock_);
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
        return ret;
    }
    return ret;
}

int CloudSyncer::SaveDataInTransaction(CloudSyncer::TaskId taskId,  DownloadData &downloadData,
    std::vector<std::string> &pkColNames, InnerProcessInfo &info, CloudWaterMark &newCloudWaterMark,
    ChangedData &changedData)
{
    int ret = storageProxy_->StartTransaction(TransactType::IMMEDIATE);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot start a transaction: %d.", ret);
        return ret;
    }
    if (!IsModeForcePush(taskId)) {
        changedData.tableName = info.tableName;
        changedData.field = pkColNames;
        changedData.type = ChangedDataType::DATA;
    }
    ret = SaveData(info.tableName, downloadData, info.downLoadInfo, newCloudWaterMark, changedData);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] cannot save data: %d.", ret);
        {
            std::lock_guard<std::mutex> autoLock(contextLock_);
            currentContext_.notifier->UpdateProcess(info);
        }
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
        return ret;
    }
    return E_OK;
}

int CloudSyncer::SaveDataNotifyProcess(CloudSyncer::TaskId taskId, DownloadData &downloadData,
        std::vector<std::string> &pkColNames, InnerProcessInfo &info, CloudWaterMark &newCloudWaterMark)
{
    ChangedData changedData;
    int ret = SaveDataInTransaction(taskId, downloadData, pkColNames, info, newCloudWaterMark, changedData);
    if (ret != E_OK) {
        return ret;
    }
    // call OnChange to notify changedData object first time (without Assets)
    ret = NotifyChangedData(std::move(changedData));
    if (ret != E_OK) {
        LOGE("Cannot notify changed data due to error %d", ret);
        return ret;
    }
    // Begin dowloading assets
    ChangedData changedAssets;
    ret = DownloadNotifyAssets(info, pkColNames, changedAssets);
    if (ret != E_OK) {
        LOGE("Someting wrong happened during assets downloading due to error %d", ret);
        return ret;
    }
    bool isUpdateCloudCursor = true;
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        currentContext_.notifier->NotifyProcess(cloudTaskInfos_[taskId], info);
        isUpdateCloudCursor = currentContext_.strategy->JudgeUpdateCursor();
    }
    // use the cursor of the last datum in data set to update cloud water mark
    if (isUpdateCloudCursor) {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        currentContext_.cloudWaterMarks[info.tableName] = newCloudWaterMark;
    }
    return E_OK;
}

void CloudSyncer::NotifyInBatchUpload(const UploadParam &uploadParam, const InnerProcessInfo &innerProcessInfo,
    bool lastBatch)
{
    CloudTaskInfo taskInfo;
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        taskInfo = cloudTaskInfos_[uploadParam.taskId];
    }
    std::lock_guard<std::mutex> autoLock(contextLock_);
    if (uploadParam.lastTable && lastBatch) {
        currentContext_.notifier->UpdateProcess(innerProcessInfo);
    } else {
        currentContext_.notifier->NotifyProcess(taskInfo, innerProcessInfo);
    }
}

int CloudSyncer::DoDownload(CloudSyncer::TaskId taskId)
{
    TableName tableName;
    int ret = GetCurrentTableName(tableName);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid table name for syncing: %d", ret);
        return ret;
    }
    InnerProcessInfo info;
    info.tableName = tableName;

    std::vector<std::string> colNames;
    std::vector<Field> assetFields;
    ret = storageProxy_->GetPrimaryColNamesWithAssetsFields(tableName, colNames, assetFields);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot get primary column names: %d", ret);
        return ret;
    }
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        currentContext_.assetFields[currentContext_.tableName] = assetFields;
    }

    CloudWaterMark cloudWaterMark;
    ret = storageProxy_->GetCloudWaterMark(tableName, cloudWaterMark);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Cannot get cloud water level from cloud meta data: %d.", ret);
        return ret;
    }
    return DoDownloadInner(taskId, colNames, info, cloudWaterMark);
}


int CloudSyncer::DoDownloadInner(CloudSyncer::TaskId taskId, std::vector<std::string> &colNames,
    InnerProcessInfo &info, CloudWaterMark &cloudWaterMark)
{
    // Query data by batch until reaching end and not more data need to be download
    bool queryEnd = false;
    uint32_t retryCnt = 0;
    int ret = E_OK;
    while (!queryEnd) {
        ret = PreCheck(taskId, info.tableName);
        if (ret != E_OK) {
            return ret;
        }
        // Get cloud data after cloud water mark
        info.tableStatus = ProcessStatus::PROCESSING;
        DownloadData downloadData;
        ret = QueryCloudData(info.tableName, cloudWaterMark, downloadData);
        if (ret == -E_QUERY_END) {
            // Won't break here since downloadData may not be null
            queryEnd = true;
        } else if (ret != E_OK) {
            return ret;
        }
        if (downloadData.data.empty()) {
            if (ret == E_OK && retryCnt >= CloudDbConstant::MAX_DOWNLOAD_RETRY_TIME) {
                LOGE("Cloud Db send empty data but didn't return QUERY_END for too much time");
                return -E_CLOUD_ERROR;
            }
            if (ret == E_OK && retryCnt < CloudDbConstant::MAX_DOWNLOAD_RETRY_TIME) {
                LOGW("Cloud Db return E_OK but send empty data, it should return QUERY_END, retry");
                retryCnt++;
                continue;
            }
            NotifyInEmptyDownload(taskId, info);
            break;
        }
        // Save data in transaction, update cloud water mark, notify process and changed data
        ret = SaveDataNotifyProcess(taskId, downloadData, colNames, info, cloudWaterMark);
        if (ret != E_OK) {
            return ret;
        }
        retryCnt = 0;
    }
    return E_OK;
}

void CloudSyncer::NotifyInEmptyDownload(CloudSyncer::TaskId taskId, InnerProcessInfo &info)
{
    std::lock_guard<std::mutex> autoLock(contextLock_);
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

int CloudSyncer::PreCheckUpload(CloudSyncer::TaskId &taskId, const TableName &tableName, LocalWaterMark &localMark)
{
    int ret = PreCheck(taskId, tableName);
    if (ret != E_OK) {
        return ret;
    }
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
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

    if (!IsModeForcePush(taskId)) {
        ret = storageProxy_->GetLocalWaterMark(tableName, localMark);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Failed to get local water mark when upload, %d.", ret);
        }
    }
    return ret;
}

bool CloudSyncer::CheckCloudSyncDataEmpty(CloudSyncData &uploadData)
{
    return uploadData.insData.extend.empty() && uploadData.insData.record.empty() &&
           uploadData.updData.extend.empty() && uploadData.updData.record.empty() &&
           uploadData.delData.extend.empty() && uploadData.delData.record.empty();
}

int CloudSyncer::DoBatchUpload(CloudSyncData &uploadData, UploadParam &uploadParam, InnerProcessInfo &innerProcessInfo)
{
    int errCode = E_OK;
    Info insertInfo;
    Info updateInfo;
    Info deleteInfo;

    if (!uploadData.delData.record.empty() && !uploadData.delData.extend.empty()) {
        errCode = cloudDB_.BatchDelete(uploadData.tableName, uploadData.delData.record,
            uploadData.delData.extend, deleteInfo);
        if (errCode != E_OK) {
            return errCode;
        }
        innerProcessInfo.upLoadInfo.successCount += deleteInfo.successCount;
    }

    if (!uploadData.insData.record.empty() && !uploadData.insData.extend.empty()) {
        errCode = cloudDB_.BatchInsert(uploadData.tableName, uploadData.insData.record,
            uploadData.insData.extend, insertInfo);
        if (errCode != E_OK) {
            return errCode;
        }
        // we need to fill back gid after insert data to cloud.
        int ret = storageProxy_->FillCloudGid(uploadData);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Failed to fill back gid when doing upload, %d.", ret);
            return ret;
        }
        innerProcessInfo.upLoadInfo.successCount += insertInfo.successCount;
    }

    if (!uploadData.updData.record.empty() && !uploadData.updData.extend.empty()) {
        errCode = cloudDB_.BatchUpdate(uploadData.tableName, uploadData.updData.record,
            uploadData.updData.extend, updateInfo);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = storageProxy_->FillCloudAssetForUpload(uploadData);
        if (errCode != E_OK) {
            LOGE("Cannot fill cloud asset during upload procedure");
            return errCode;
        }
        innerProcessInfo.upLoadInfo.successCount += updateInfo.successCount;
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
    if (IsModeForcePush(uploadParam.taskId)) {
        return E_OK;
    }
    errCode = storageProxy_->PutLocalWaterMark(tableName, uploadParam.localMark);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Cannot set local water mark while Uploading, %d.", errCode);
        return errCode;
    }
    CloudWaterMark cloudWaterMark;
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        auto it = currentContext_.cloudWaterMarks.find(tableName);
        if (it == currentContext_.cloudWaterMarks.end()) {
            LOGD("[CloudSyncer] Not found water mark just return");
            return E_OK;
        }
        cloudWaterMark = currentContext_.cloudWaterMarks[tableName];
    }
    errCode = storageProxy_->PutCloudWaterMark(tableName, cloudWaterMark);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] Cannot set cloud water mark while Uploading, %d.", errCode);
    }
    return errCode;
}

int CloudSyncer::DoUpload(CloudSyncer::TaskId taskId, bool lastTable)
{
    std::string tableName;
    int ret = GetCurrentTableName(tableName);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid table name for syncing: %d", ret);
        return ret;
    }

    LocalWaterMark localMark = 0u;
    ret = PreCheckUpload(taskId, tableName, localMark);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Doing upload sync pre check failed, %d.", ret);
        return ret;
    }

    int64_t count = 0;
    ret = storageProxy_->GetUploadCount(tableName, localMark, IsModeForcePush(taskId), count);
    if (ret != E_OK) {
        // GetUploadCount will return E_OK when upload count is zero.
        LOGE("[CloudSyncer] Failed to get Upload Data Count, %d.", ret);
        return ret;
    }
    if (count == 0) {
        LOGI("[CloudSyncer] There is no need to doing upload, as the upload data count is zero.");
        InnerProcessInfo innerProcessInfo;
        innerProcessInfo.tableName = tableName;
        innerProcessInfo.upLoadInfo.total = 0;  // count is zero
        innerProcessInfo.tableStatus = ProcessStatus::FINISHED;
        {
            std::lock_guard<std::mutex> autoLock(contextLock_);
            if (lastTable) {
                currentContext_.notifier->UpdateProcess(innerProcessInfo);
            } else {
                currentContext_.notifier->NotifyProcess(cloudTaskInfos_[taskId], innerProcessInfo);
            }
        }
        return E_OK;
    }
    UploadParam param;
    param.count = count;
    param.localMark = localMark;
    param.lastTable = lastTable;
    param.taskId = taskId;
    return DoUploadInner(tableName, param);
}

static AssetOpType StatusToFlag(AssetStatus status) {
    switch (status)
    {
        case AssetStatus::INSERT:
            return AssetOpType::INSERT;
        case AssetStatus::DELETE:
            return AssetOpType::DELETE;
        case AssetStatus::UPDATE:
            return AssetOpType::UPDATE;
        case AssetStatus::NORMAL:
            return AssetOpType::NO_CHANGE;
        default:
            LOGW("Unexpected Situation and won't be handled, Caller should ensure that current situation won't occur");
            return AssetOpType::NO_CHANGE;
    }
}

static void StatusToFlagForAsset(Asset &asset)
{
    asset.flag = static_cast<uint32_t>(StatusToFlag(static_cast<AssetStatus>(asset.status)));
}

static void StatusToFlagForAssets(Assets &assets)
{
    for (Asset &asset : assets) {
        StatusToFlagForAsset(asset);
    }
}

static void StatusToFlagForAssetsInRecord(const std::vector<Field> &fields, VBucket &record)
{
    for (const Field &field : fields) {
        if (field.type == TYPE_INDEX<Assets>) {
            StatusToFlagForAssets(std::get<Assets>(record[field.colName]));
        }
        if (field.type == TYPE_INDEX<Asset>) {
            StatusToFlagForAsset(std::get<Asset>(record[field.colName]));
        }
    }
}

void CloudSyncer::TagUploadAssets(CloudSyncData &uploadData)
{
    if (!ShouldProcessAssets()) {
        return;
    }
    std::map<std::string, std::map<std::string, Assets>> cloudAssets;
    {
        std::lock_guard<std::mutex> autoLock(contextLock_);
        cloudAssets = currentContext_.assetsInfo[currentContext_.tableName];
    }
    // for delete scenario, assets should not appear in the records. Thereby we needn't tag the assests.
    // for insert scenario, gid does not exist. Thereby, we needn't compare with cloud asset get in download procedure
    for (size_t i = 0; i < uploadData.insData.extend.size(); i++) {
        VBucket cloudAsset; // cloudAsset must be empty
        (void)TagAssetsInSingleRecord(uploadData.insData.record[i], cloudAsset, true);
    }
    // for update scenario, assets shoulb be compared with asset get in download procedure.
    for (size_t i = 0; i < uploadData.updData.extend.size(); i++) {
        VBucket cloudAsset;
        // gid must exist in UPDATE scenario, cause we have re-fill gid during download procedure
        // But we need to check for safety
        auto gidIter = uploadData.updData.extend[i].find(CloudDbConstant::GID_FIELD);
        if (gidIter == uploadData.updData.extend[i].end()) {
            LOGE("Datum to be upload must contain gid");
            return;
        }
        // update data must contain gid, however, we could only pull data after water mark
        // Therefore, we need to check whether we contain the data
        std::string &gid = std::get<std::string>(gidIter->second);
        if (cloudAssets.find(gid) == cloudAssets.end()) {
            // In this case, we directly upload data without compartion and tagging
            std::vector<Field> assetFields;
            {
                std::lock_guard<std::mutex> autoLock(contextLock_);
                assetFields = currentContext_.assetFields[currentContext_.tableName];
            }
            StatusToFlagForAssetsInRecord(assetFields, uploadData.updData.record[i]);
            continue;
        }
        for (auto &it : cloudAssets[gid]) {
            cloudAsset[it.first] = it.second;
        }
        (void)TagAssetsInSingleRecord(uploadData.updData.record[i], cloudAsset, true);
    }
}

int CloudSyncer::PreProcessBatchUpload(TaskId taskId, CloudSyncData &uploadData, InnerProcessInfo &innerProcessInfo,
    LocalWaterMark &localMark)
{
    // Precheck and calculate local water mark which would be updated if batch upload successed.
    int ret = CheckTaskIdValid(taskId);
    if (ret != E_OK) {
        return ret;
    }
    ret = CheckCloudSyncDataValid(uploadData, innerProcessInfo.tableName, innerProcessInfo.upLoadInfo.total, taskId);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid Cloud Sync Data of Upload, %d.", ret);
        return ret;
    }
    TagUploadAssets(uploadData);
    // get local water mark to be updated in future.
    if (!IsModeForcePush(taskId)) {
        ret = UpdateExtendTime(uploadData, innerProcessInfo.upLoadInfo.total, taskId, localMark);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Failed to get new local water mark in Cloud Sync Data, %d.", ret);
        }
    }
    return ret;
}

void CloudSyncer::SetUploadDataFlag(const TaskId taskId, CloudSyncData& uploadData)
{
    std::lock_guard<std::mutex> autoLock(queueLock_);
    uploadData.isCloudForcePushStrategy = (cloudTaskInfos_[taskId].mode == SYNC_MODE_CLOUD_FORCE_PUSH);
}

bool CloudSyncer::IsModeForcePush(const TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(queueLock_);
    return cloudTaskInfos_[taskId].mode == SYNC_MODE_CLOUD_FORCE_PUSH;
}

int CloudSyncer::DoUploadInner(const std::string &tableName, UploadParam &uploadParam)
{
    ContinueToken continueStmtToken = nullptr;
    CloudSyncData uploadData(tableName);
    SetUploadDataFlag(uploadParam.taskId, uploadData);
    bool getDataUnfinished = false;

    int ret = storageProxy_->GetCloudData(tableName, uploadParam.localMark, continueStmtToken, uploadData);
    if ((ret != E_OK) && (ret != -E_UNFINISHED)) {
        LOGE("[CloudSyncer] Failed to get cloud data when upload, %d.", ret);
        return ret;
    }

    InnerProcessInfo info;
    info.tableName = tableName;
    info.tableStatus = ProcessStatus::PROCESSING;
    info.upLoadInfo.total = uploadParam.count;
    uint32_t batchIndex = 0;

    while (!CheckCloudSyncDataEmpty(uploadData)) {
        getDataUnfinished = (ret == -E_UNFINISHED);
        ret = PreProcessBatchUpload(uploadParam.taskId, uploadData, info, uploadParam.localMark);
        if (ret != E_OK) {
            goto RELEASE_EXIT;
        }
        info.upLoadInfo.batchIndex = ++batchIndex;

        ret = DoBatchUpload(uploadData, uploadParam, info);
        if (ret != E_OK) {
            LOGE("[CloudSyncer] Failed to do upload, %d", ret);
            info.upLoadInfo.failCount = info.upLoadInfo.total - info.upLoadInfo.successCount;
            {
                std::lock_guard<std::mutex> autoLock(contextLock_);
                currentContext_.notifier->UpdateProcess(info);
            }
            goto RELEASE_EXIT;
        }

        ClearCloudSyncData(uploadData);

        if (continueStmtToken == nullptr) {
            break;
        }
        SetUploadDataFlag(uploadParam.taskId, uploadData);

        ret = storageProxy_->GetCloudDataNext(continueStmtToken, uploadData);
        if ((ret != E_OK) && (ret != -E_UNFINISHED)) {
            LOGE("[CloudSyncer] Failed to get cloud data next when doing upload, %d.", ret);
            goto RELEASE_EXIT;
        }
    }

RELEASE_EXIT:
    if (getDataUnfinished) {
        storageProxy_->ReleaseContinueToken(continueStmtToken);
    }
    return ret;
}

int CloudSyncer::CheckDownloadDatum(VBucket &datum)
{
    if (datum.find(CloudDbConstant::GID_FIELD) == datum.end() ||
        datum.find(CloudDbConstant::CREATE_FIELD) == datum.end() ||
        datum.find(CloudDbConstant::MODIFY_FIELD) == datum.end() ||
        datum.find(CloudDbConstant::DELETE_FIELD) == datum.end() ||
        datum.find(CloudDbConstant::CURSOR_FIELD) == datum.end()) {
            LOGE("Cloud data do not contain expected field");
            return -E_CLOUD_ERROR;
        }
    if (datum[CloudDbConstant::GID_FIELD].index() != TYPE_INDEX<std::string> ||
        datum[CloudDbConstant::CREATE_FIELD].index() != TYPE_INDEX<int64_t> ||
        datum[CloudDbConstant::MODIFY_FIELD].index() != TYPE_INDEX<int64_t> ||
        datum[CloudDbConstant::DELETE_FIELD].index() != TYPE_INDEX<bool> ||
        datum[CloudDbConstant::CURSOR_FIELD].index() != TYPE_INDEX<std::string>) {
            LOGE("Cloud data do not contain expected type");
            return -E_CLOUD_ERROR;
        }
    return E_OK;
}

int CloudSyncer::QueryCloudData(const std::string &tableName, const CloudWaterMark &cloudWaterMark,
    DownloadData &downloadData)
{
    VBucket extend = {
        {CloudDbConstant::CURSOR_FIELD, cloudWaterMark}
    };
    int ret = cloudDB_.Query(tableName, extend, downloadData.data);
    downloadData.opType.resize(downloadData.data.size());
    if (ret == -E_QUERY_END) {
        LOGI("[CloudSyncer] Download data from cloud database success and no more data need to be downloaded");
        return -E_QUERY_END;
    }
    if (ret == E_OK) {
        LOGI("[CloudSyncer] Download data from cloud database success but still has data to download");
        return E_OK;
    }
    LOGE("[CloudSyncer] Download data from cloud database unsuccess %d", ret);
    return -E_CLOUD_ERROR;
}

int CloudSyncer::CheckParamValid(const std::vector<DeviceID> &devices, SyncMode mode)
{
    if (devices.empty()) {
        LOGE("[CloudSyncer] devices is empty");
        return -E_INVALID_ARGS;
    }
    if (devices.size() > 1) {
        LOGE("[CloudSyncer] too much devices");
        return -E_INVALID_ARGS;
    }
    for (const auto &dev: devices) {
        if (dev.empty() || dev.size() > DBConstant::MAX_DEV_LENGTH) {
            LOGE("[CloudSyncer] invalid device, size %zu", dev.size());
            return -E_INVALID_ARGS;
        }
    }
    if (mode >= SyncMode::SYNC_MODE_PUSH_ONLY && mode < SyncMode::SYNC_MODE_CLOUD_MERGE) {
        LOGE("[CloudSyncer] not support mode %d", static_cast<int>(mode));
        return -E_NOT_SUPPORT;
    }
    if (mode < SyncMode::SYNC_MODE_PUSH_ONLY || mode > SyncMode::SYNC_MODE_CLOUD_FORCE_PULL) {
        LOGE("[CloudSyncer] invalid mode %d", static_cast<int>(mode));
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

int CloudSyncer::CheckTaskIdValid(TaskId taskId)
{
    if (closed_) {
        LOGE("[CloudSyncer] DB is closed.");
        return -E_DB_CLOSED;
    }
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
            LOGE("[CloudSyncer] not found task.");
            return -E_INVALID_ARGS;
        }
        if (cloudTaskInfos_[taskId].errCode != E_OK) {
            return cloudTaskInfos_[taskId].errCode;
        }
    }
    std::lock_guard<std::mutex> autoLock(contextLock_);
    return currentContext_.currentTaskId == taskId ? E_OK : -E_INVALID_ARGS;
}

int CloudSyncer::GetCurrentTableName(std::string &tableName)
{
    std::lock_guard<std::mutex> autoLock(contextLock_);
    if (currentContext_.tableName.empty()) {
        return -E_BUSY;
    }
    tableName = currentContext_.tableName;
    return E_OK;
}

int CloudSyncer::TryToAddSyncTask(CloudTaskInfo &&taskInfo)
{
    if (closed_) {
        LOGW("[CloudSyncer] syncer is closed, should not sync now");
        return -E_DB_CLOSED;
    }
    std::lock_guard<std::mutex> autoLock(queueLock_);
    int errCode = CheckQueueSizeWithNoLock();
    if (errCode != E_OK) {
        return errCode;
    }
    do {
        currentTaskId_++;
    } while (currentTaskId_ == 0);
    taskInfo.taskId = currentTaskId_;
    taskQueue_.push_back(currentTaskId_);
    cloudTaskInfos_[currentTaskId_] = taskInfo;
    LOGI("[CloudSyncer] Add task ok, taskId %" PRIu64, cloudTaskInfos_[currentTaskId_].taskId);
    return E_OK;
}

int CloudSyncer::CheckQueueSizeWithNoLock()
{
    int32_t limit = queuedManualSyncLimit_;
    if (taskQueue_.size() >= static_cast<size_t>(limit)) {
        LOGW("[CloudSyncer] too much sync task");
        return -E_BUSY;
    }
    return E_OK;
}

int CloudSyncer::PrepareSync(TaskId taskId)
{
    std::vector<std::string> tableNames;
    std::vector<std::string> devices;
    SyncMode mode;
    {
        std::lock_guard<std::mutex> autoLock(queueLock_);
        if (closed_ || cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
            LOGW("[CloudSyncer] Abort sync because syncer is closed");
            return -E_DB_CLOSED;
        }
        tableNames = cloudTaskInfos_[taskId].table;
        mode = cloudTaskInfos_[taskId].mode;
        devices = cloudTaskInfos_[taskId].devices;
    }
    {
        // check current task is running or not
        std::lock_guard<std::mutex> autoLock(contextLock_);
        if (closed_ || currentContext_.currentTaskId != INVALID_TASK_ID) {
            LOGW("[CloudSyncer] Abort sync because syncer is closed or another task is running");
            return -E_DB_CLOSED;
        }
        currentContext_.currentTaskId = taskId;
        currentContext_.notifier = std::make_shared<ProcessNotifier>();
        currentContext_.strategy = StrategyFactory::BuildSyncStrategy(mode);
        currentContext_.notifier->Init(tableNames, devices);
        LOGI("[CloudSyncer] exec taskId %" PRIu64, taskId);
    }
    // remove task id from queue
    std::lock_guard<std::mutex> autoLock(queueLock_);
    if (!taskQueue_.empty()) {
        taskQueue_.pop_front();
    }
    cloudTaskInfos_[taskId].status = ProcessStatus::PROCESSING;
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
            LOGD("[CloudSyncer] Trigger heartbeat");
            if (cloudDB_.HeartBeat() != E_OK) {
                HeartBeatFailed(taskId);
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

void CloudSyncer::HeartBeatFailed(TaskId taskId)
{
    failedHeartBeatCount_++;
    if (failedHeartBeatCount_ < MAX_HEARTBEAT_FAILED_LIMIT) {
        return;
    }
    LOGW("[CloudSyncer] heartbeat failed too much times!");
    FinishHeartBeatTimer();
    SetTaskFailed(taskId, -E_CLOUD_ERROR);
}

void CloudSyncer::SetTaskFailed(TaskId taskId, int errCode)
{
    std::lock_guard<std::mutex> autoLock(queueLock_);
    if (cloudTaskInfos_.find(taskId) == cloudTaskInfos_.end()) {
        return;
    }
    cloudTaskInfos_[taskId].errCode = errCode;
}

int CloudSyncer::CheckCloudSyncDataValid(CloudSyncData uploadData, const std::string &tableName,
    const int64_t &count, TaskId &taskId)
{
    size_t insRecordLen = uploadData.insData.record.size();
    size_t insExtendLen = uploadData.insData.extend.size();
    size_t updRecordLen = uploadData.updData.record.size();
    size_t updExtendLen = uploadData.updData.extend.size();
    size_t delRecordLen = uploadData.delData.record.size();
    size_t delExtendLen = uploadData.delData.extend.size();

    bool syncDataValid = (uploadData.tableName == tableName) &&
        ((insRecordLen > 0 && insExtendLen > 0 && insRecordLen == insExtendLen) ||
        (updRecordLen > 0 && updExtendLen > 0 && updRecordLen == updExtendLen) ||
        (delRecordLen > 0 && delExtendLen > 0 && delRecordLen == delExtendLen));
    if (!syncDataValid) {
        LOGE("upload data is empty but upload count is not zero or upload table name"
            " is not the same as table name of sync data.");
        return -E_INTERNAL_ERROR;
    }
    int64_t syncDataCount = static_cast<int64_t>(insRecordLen) + static_cast<int64_t>(updRecordLen) +
        static_cast<int64_t>(delRecordLen);
    if (syncDataCount > count) {
        LOGE("Size of a batch of sync data is greater than upload data size.");
        return -E_INTERNAL_ERROR;
    }

    return E_OK;
}

int CloudSyncer::GetWaterMarkAndUpdateTime(std::vector<VBucket>& extend, LocalWaterMark &waterMark)
{
    for (auto &extendData: extend) {
        if (extendData.empty() || extendData.find(CloudDbConstant::MODIFY_FIELD) == extendData.end()) {
            LOGE("[CloudSyncer] VBucket is empty or MODIFY_FIELD doesn't exist.");
            return -E_INTERNAL_ERROR;
        }
        if (TYPE_INDEX<int64_t> != extendData.at(CloudDbConstant::MODIFY_FIELD).index()) {
            LOGE("[CloudSyncer] VBucket's MODIFY_FIELD doestn't fit int64_t.");
            return -E_INTERNAL_ERROR;
        }
        if (extendData.empty() || extendData.find(CloudDbConstant::CREATE_FIELD) == extendData.end()) {
            LOGE("[CloudSyncer] VBucket is empty or MODIFY_FIELD doesn't exist.");
            return -E_INTERNAL_ERROR;
        }
        if (TYPE_INDEX<int64_t> != extendData.at(CloudDbConstant::CREATE_FIELD).index()) {
            LOGE("[CloudSyncer] VBucket's MODIFY_FIELD doestn't fit int64_t.");
            return -E_INTERNAL_ERROR;
        }
        waterMark = std::max(int64_t(waterMark), std::get<int64_t>(extendData.at(CloudDbConstant::MODIFY_FIELD)));
        int64_t modifyTime =
            std::get<int64_t>(extendData.at(CloudDbConstant::MODIFY_FIELD)) / CloudDbConstant::TEN_THOUSAND;
        int64_t createTime =
            std::get<int64_t>(extendData.at(CloudDbConstant::CREATE_FIELD)) / CloudDbConstant::TEN_THOUSAND;
        extendData.insert_or_assign(CloudDbConstant::MODIFY_FIELD, modifyTime);
        extendData.insert_or_assign(CloudDbConstant::CREATE_FIELD, createTime);
    }
    return E_OK;
}

// After doing a batch upload, we need to use CloudSyncData's maximum timestamp to update the water mark;
int CloudSyncer::UpdateExtendTime(CloudSyncData &uploadData, const int64_t &count,
    TaskId taskId, LocalWaterMark &waterMark)
{
    int ret = E_OK;
    ret = CheckCloudSyncDataValid(uploadData, uploadData.tableName, count, taskId);
    if (ret != E_OK) {
        LOGE("[CloudSyncer] Invalid Sync Data when get local water mark.");
        return ret;
    }
    if (!uploadData.insData.extend.empty()) {
        if (uploadData.insData.record.size() != uploadData.insData.extend.size()) {
            LOGE("[CloudSyncer] Inconsistent size of inserted data.");
            return -E_INTERNAL_ERROR;
        }
        ret = GetWaterMarkAndUpdateTime(uploadData.insData.extend, waterMark);
        if (ret != E_OK) {
            return ret;
        }
    }

    if (!uploadData.updData.extend.empty()) {
        if (uploadData.updData.record.size() != uploadData.updData.extend.size()) {
            LOGE("[CloudSyncer] Inconsistent size of updated data, %d.", -E_INTERNAL_ERROR);
            return -E_INTERNAL_ERROR;
        }
        ret = GetWaterMarkAndUpdateTime(uploadData.updData.extend, waterMark);
        if (ret != E_OK) {
            return ret;
        }
    }

    if (!uploadData.delData.extend.empty()) {
        if (uploadData.delData.record.size() != uploadData.delData.extend.size()) {
            LOGE("[CloudSyncer] Inconsistent size of deleted data, %d.", -E_INTERNAL_ERROR);
            return -E_INTERNAL_ERROR;
        }
        ret = GetWaterMarkAndUpdateTime(uploadData.delData.extend, waterMark);
        if (ret != E_OK) {
            return ret;
        }
    }
    return E_OK;
}

void CloudSyncer::ClearCloudSyncData(CloudSyncData &uploadData)
{
    std::vector<VBucket>().swap(uploadData.insData.record);
    std::vector<VBucket>().swap(uploadData.insData.extend);
    std::vector<int64_t>().swap(uploadData.insData.rowid);
    std::vector<VBucket>().swap(uploadData.updData.record);
    std::vector<VBucket>().swap(uploadData.updData.extend);
    std::vector<VBucket>().swap(uploadData.delData.record);
    std::vector<VBucket>().swap(uploadData.delData.extend);
}

int CloudSyncer::CleanCloudData(ClearMode mode, const std::vector<std::string> &tableNameList)
{
    std::lock_guard<std::mutex> lock(syncMutex_);
    std::string emptyString;
    int index = 1;
    for (const auto &tableName: tableNameList) {
        LOGD("[CloudSyncer] Start clean cloud water mark. table index: %d.", index);
        int ret = storageProxy_->PutCloudWaterMark(tableName, emptyString);
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
    errCode = storageProxy_->CleanCloudData(mode, tableNameList, assets);
    if (errCode != E_OK) {
        LOGE("[CloudSyncer] failed to clean cloud data, %d.", errCode);
        storageProxy_->Rollback();
        return errCode;
    }

    errCode = cloudDB_.RemoveLocalAssets(assets);
    if (errCode != E_OK) {
        LOGE("[Storage Executor] failed to remove local assets, %d.", errCode);
        storageProxy_->Rollback();
        return errCode;
    }
    storageProxy_->Commit();

    return errCode;
}


void CloudSyncer::ModifyCloudDataTime(VBucket &data)
{
    // data already check field modify_field and create_field
    int64_t modifyTime = std::get<int64_t>(data[CloudDbConstant::MODIFY_FIELD]) * CloudDbConstant::TEN_THOUSAND;
    int64_t createTime = std::get<int64_t>(data[CloudDbConstant::CREATE_FIELD]) * CloudDbConstant::TEN_THOUSAND;
    data[CloudDbConstant::MODIFY_FIELD] = modifyTime;
    data[CloudDbConstant::CREATE_FIELD] = createTime;
}
} // namespace DistributedDB
