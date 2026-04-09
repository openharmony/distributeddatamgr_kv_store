/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef CLOUD_CUSTOM_PULL_STRATEGY_H
#define CLOUD_CUSTOM_PULL_STRATEGY_H

#include "cloud_sync_strategy.h"

namespace DistributedDB {
class CloudCustomPullStrategy : public CloudSyncStrategy {
public:
    OpType TagSyncDataStatus(const DataStatusInfo &statusInfo, const LogInfo &localInfo, const VBucket &localData,
        const LogInfo &cloudInfo, VBucket &cloudData) const override;

    bool JudgeUpdateCursor() const override;

    bool JudgeUpload() const override;

    bool JudgeLocker() const override;

    bool JudgeQueryLocalData() const override;

    void SetCloudConflictHandler(const std::weak_ptr<ICloudConflictHandler> &handler) override;
private:
    static OpType ConvertToOpType(bool isLocalExist, ConflictRet conflictRet);
    mutable std::mutex handlerMutex_;
    std::weak_ptr<ICloudConflictHandler> conflictHandler_;
};
} // DistributedDB
#endif // CLOUD_CUSTOM_PULL_STRATEGY_H