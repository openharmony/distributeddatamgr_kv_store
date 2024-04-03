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

#ifndef PROCESS_NOTIFIER_H
#define PROCESS_NOTIFIER_H
#include "icloud_syncer.h"
namespace DistributedDB {
class ProcessNotifier {
public:
    explicit ProcessNotifier(ICloudSyncer *syncer);
    ~ProcessNotifier();

    void Init(const std::vector<std::string> &tableName, const std::vector<std::string> &devices);

    void UpdateProcess(const ICloudSyncer::InnerProcessInfo &process);

    void NotifyProcess(const ICloudSyncer::CloudTaskInfo &taskInfo, const ICloudSyncer::InnerProcessInfo &process,
        bool notifyWhenError = false);

    std::vector<std::string> GetDevices() const;

    uint32_t GetUploadBatchIndex(const std::string &tableName) const;

    uint32_t GetLastUploadSuccessCount(const std::string &tableName) const;

    void GetDownloadInfoByTableName(ICloudSyncer::InnerProcessInfo &process);

    void SetUser(const std::string &user);
protected:
    mutable std::mutex processMutex_;
    SyncProcess syncProcess_;
    std::map<std::string, SyncProcess> multiSyncProcess_;
    std::vector<std::string> devices_;
    ICloudSyncer *syncer_;
    std::string user_;
};
}
#endif // PROCESS_NOTIFIER_H
