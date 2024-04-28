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

#include "virtual_cloud_syncer.h"

namespace DistributedDB {
VirtualCloudSyncer::VirtualCloudSyncer(std::shared_ptr<StorageProxy> storageProxy)
    : CloudSyncer(storageProxy)
{
}

int VirtualCloudSyncer::DoDownload(CloudSyncer::TaskId taskId, bool isFirstDownload)
{
    if (!doDownload_) {
        LOGI("[VirtualCloudSyncer] download just return ok");
        return E_OK;
    }
    if (downloadFunc_) {
        return downloadFunc_();
    }
    return CloudSyncer::DoDownload(taskId, isFirstDownload);
}

int VirtualCloudSyncer::DoDownloadInNeed(const CloudTaskInfo &taskInfo, const bool needUpload, bool isFirstDownload)
{
    if (!doDownload_) {
        LOGI("[VirtualCloudSyncer] download just return ok");
        return E_OK;
    }
    if (downloadInNeedFunc_) {
        return downloadInNeedFunc_();
    }
    return CloudSyncer::DoDownloadInNeed(taskInfo, needUpload, isFirstDownload);
}

int VirtualCloudSyncer::DoUpload(CloudSyncer::TaskId taskId, bool lastTable, LockAction lockAction)
{
    if (!doUpload_) {
        LOGI("[VirtualCloudSyncer] upload just return ok");
        return E_OK;
    }
    if (uploadFunc_) {
        return uploadFunc_();
    }
    return CloudSyncer::DoUpload(taskId, lastTable, lockAction);
}

void VirtualCloudSyncer::SetSyncAction(bool doDownload, bool doUpload)
{
    doDownload_ = doDownload;
    doUpload_ = doUpload;
}

void VirtualCloudSyncer::SetDownloadFunc(const std::function<int()> &function)
{
    downloadFunc_ = function;
}

void VirtualCloudSyncer::SetDownloadInNeedFunc(const std::function<int()> &function)
{
    downloadInNeedFunc_ = function;
}

void VirtualCloudSyncer::SetTaskNeedUpload()
{
    currentContext_.isNeedUpload = true;
}

void VirtualCloudSyncer::SetUploadFunc(const std::function<int()> &function)
{
    uploadFunc_ = function;
}

void VirtualCloudSyncer::Notify(bool notifyIfError)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    CloudTaskInfo taskInfo = cloudTaskInfos_[currentContext_.currentTaskId];
    currentContext_.notifier->NotifyProcess(taskInfo, {}, notifyIfError);
}

size_t VirtualCloudSyncer::GetQueueCount()
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    return taskQueue_.size();
}

void VirtualCloudSyncer::SetCurrentTaskInfo(const SyncProcessCallback &callback,
    CloudSyncer::TaskId taskId)
{
    std::lock_guard<std::mutex> autoLock(dataLock_);
    currentContext_.currentTaskId = taskId;
    currentContext_.notifier = std::make_shared<ProcessNotifier>(this);
    CloudTaskInfo taskInfo;
    taskInfo.callback = callback;
    cloudTaskInfos_[taskId] = taskInfo;
}

int VirtualCloudSyncer::CallTagStatusByStrategy(bool isExist, const DataInfoWithLog &localInfo,
    const LogInfo &cloudInfo, OpType &strategyOpResult)
{
    SyncParam param;
    DataInfo dataInfo;
    dataInfo.localInfo = localInfo;
    dataInfo.cloudLogInfo = cloudInfo;
    return CloudSyncer::TagStatusByStrategy(isExist, param, dataInfo, strategyOpResult);
}
}