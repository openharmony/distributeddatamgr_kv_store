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
#include "process_notifier.h"

#include "db_errno.h"
#include "kv_store_errno.h"
#include "runtime_context.h"
namespace DistributedDB {
ProcessNotifier::ProcessNotifier(ICloudSyncer *syncer)
    : syncer_(syncer)
{
    RefObject::IncObjRef(syncer_);
}

ProcessNotifier::~ProcessNotifier()
{
    RefObject::DecObjRef(syncer_);
}

void ProcessNotifier::Init(const std::vector<std::string> &tableName,
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

void ProcessNotifier::UpdateProcess(const ICloudSyncer::InnerProcessInfo &process)
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
    if (!user_.empty()) {
        multiSyncProcess_[user_] = syncProcess_;
    }
}

void ProcessNotifier::NotifyProcess(const ICloudSyncer::CloudTaskInfo &taskInfo,
    const ICloudSyncer::InnerProcessInfo &process, bool notifyWhenError)
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
        multiSyncProcess_[user_].errCode = TransferDBErrno(taskInfo.errCode);
        multiSyncProcess_[user_].process = taskInfo.status;
        if (user_.empty()) {
            for (const auto &device : devices_) {
                // make sure only one device
                currentProcess[device] = syncProcess_;
            }
        } else {
            currentProcess = multiSyncProcess_;
        }
    }
    SyncProcessCallback callback = taskInfo.callback;
    if (!callback) {
        LOGD("[ProcessNotifier] task hasn't callback");
        return;
    }
    ICloudSyncer *syncer = syncer_;
    if (syncer == nullptr) {
        LOGW("[ProcessNotifier] cancel notify because syncer is nullptr");
        return; // should not happen
    }
    RefObject::IncObjRef(syncer);
    auto id = syncer->GetIdentify();
    int errCode = RuntimeContext::GetInstance()->ScheduleQueuedTask(id, [callback, currentProcess, syncer]() {
        LOGD("[ProcessNotifier] begin notify process");
        if (syncer->IsClosed()) {
            LOGI("[ProcessNotifier] db has closed, process return");
            RefObject::DecObjRef(syncer);
            return;
        }
        callback(currentProcess);
        RefObject::DecObjRef(syncer);
        LOGD("[ProcessNotifier] notify process finish");
    });
    if (errCode != E_OK) {
        LOGW("[ProcessNotifier] schedule notify process failed %d", errCode);
    }
}

std::vector<std::string> ProcessNotifier::GetDevices() const
{
    return devices_;
}

uint32_t ProcessNotifier::GetUploadBatchIndex(const std::string &tableName) const
{
    std::lock_guard<std::mutex> autoLock(processMutex_);
    if (syncProcess_.tableProcess.find(tableName) == syncProcess_.tableProcess.end()) {
        return 0u;
    }
    return syncProcess_.tableProcess.at(tableName).upLoadInfo.batchIndex;
}

uint32_t ProcessNotifier::GetLastUploadSuccessCount(const std::string &tableName) const
{
    std::lock_guard<std::mutex> autoLock(processMutex_);
    if (syncProcess_.tableProcess.find(tableName) == syncProcess_.tableProcess.end()) {
        return 0u;
    }
    return syncProcess_.tableProcess.at(tableName).upLoadInfo.successCount;
}

void ProcessNotifier::GetDownloadInfoByTableName(ICloudSyncer::InnerProcessInfo &process)
{
    if (process.tableName.empty()) {
        return;
    }
    std::lock_guard<std::mutex> autoLock(processMutex_);
    if (syncProcess_.tableProcess.find(process.tableName) == syncProcess_.tableProcess.end()) {
        process.downLoadInfo = syncProcess_.tableProcess[process.tableName].downLoadInfo;
    }
}

void ProcessNotifier::SetUser(const std::string &user)
{
    user_ = user;
}
}