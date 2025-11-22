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

#ifndef CLOUD_SYNC_STRATEGY_H
#define CLOUD_SYNC_STRATEGY_H

#include "cloud/icloud_conflict_handler.h"
#include "data_transformer.h"
#include "db_errno.h"
#include "db_types.h"
#include "icloud_sync_storage_interface.h"

namespace DistributedDB {
struct DataStatusInfo {
    bool isExistInLocal = false;
    bool isCloudWin = false;
    std::string table;
};
class CloudSyncStrategy {
public:
    CloudSyncStrategy();
    virtual ~CloudSyncStrategy() = default;

    void SetIsKvScene(bool isKvScene);

    void SetConflictResolvePolicy(SingleVerConflictResolvePolicy policy);

    SingleVerConflictResolvePolicy GetConflictResolvePolicy() const;

    virtual OpType TagSyncDataStatus(const DataStatusInfo &statusInfo,
        const LogInfo &localInfo, const VBucket &localData,
        const LogInfo &cloudInfo, VBucket &cloudData) const;

    virtual OpType TagSyncDataStatus(bool existInLocal, bool isCloudWin, const LogInfo &localInfo,
        const LogInfo &cloudInfo) const;

    virtual bool JudgeUpdateCursor() const;

    virtual bool JudgeUpload() const;

    virtual bool JudgeDownload() const;

    virtual bool JudgeLocker() const;

    bool JudgeKvScene() const;

    virtual bool JudgeQueryLocalData() const;

    virtual void SetCloudConflictHandler(const std::weak_ptr<ICloudConflictHandler> &handler);

    static bool IsDelete(const LogInfo &info);

    static bool IsLogNeedUpdate(const LogInfo &cloudInfo, const LogInfo &localInfo);

    OpType TagUpdateLocal(const LogInfo &cloudInfo, const LogInfo &localInfo) const;
protected:
    bool IsIgnoreUpdate(const LogInfo &localInfo) const;

    static bool IsSameRecord(const LogInfo &cloudInfo, const LogInfo &localInfo);

    SingleVerConflictResolvePolicy policy_;

    // isKvScene_ is used to distinguish between the KV and RDB in the following scenarios:
    // 1. Whether upload to the cloud after delete local data that does not have a gid.
    // 2. Whether the local data need update for different flag when the local time is larger.
    bool isKvScene_ = false;
};
}
#endif // CLOUD_SYNC_STRATEGY_H
