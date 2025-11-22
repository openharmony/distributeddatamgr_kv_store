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

#include "cloud_custom_pull_strategy.h"

namespace DistributedDB {
OpType CloudCustomPullStrategy::TagSyncDataStatus(const DataStatusInfo &statusInfo, const LogInfo &localInfo,
    const VBucket &localData, const LogInfo &cloudInfo, VBucket &cloudData) const
{
    std::weak_ptr<ICloudConflictHandler> handler;
    {
        std::lock_guard<std::mutex> autoLock(handlerMutex_);
        handler = conflictHandler_;
    }
    auto conflictHandler = handler.lock();
    if (conflictHandler == nullptr) {
        LOGW("[CloudCustomPullStrategy] Tag data status without conflict handler");
        return OpType::NOT_HANDLE;
    }
    VBucket upsert;
    auto ret = conflictHandler->HandleConflict(statusInfo.table, localData, cloudData, upsert);
    for (const auto &item: upsert) {
        cloudData.insert_or_assign(item.first, item.second);
    }
    if (ret == ConflictRet::NOT_HANDLE && IsDelete(cloudInfo)) {
        return OpType::CLEAR_GID;
    }
    return ConvertToOpType(statusInfo.isExistInLocal, ret);
}

bool CloudCustomPullStrategy::JudgeUpdateCursor() const
{
    return true;
}

bool CloudCustomPullStrategy::JudgeUpload() const
{
    return false;
}

bool CloudCustomPullStrategy::JudgeLocker() const
{
    return false;
}

bool CloudCustomPullStrategy::JudgeQueryLocalData() const
{
    return true;
}

void CloudCustomPullStrategy::SetCloudConflictHandler(const std::weak_ptr<ICloudConflictHandler> &handler)
{
    std::lock_guard<std::mutex> autoLock(handlerMutex_);
    conflictHandler_ = handler;
}

OpType CloudCustomPullStrategy::ConvertToOpType(bool isLocalExist, ConflictRet conflictRet)
{
    switch (conflictRet) {
        case ConflictRet::UPSERT:
            return isLocalExist ? OpType::UPDATE : OpType::INSERT;
        case ConflictRet::DELETE:
            return OpType::DELETE;
        case ConflictRet::NOT_HANDLE:
            return OpType::NOT_HANDLE;
        default:
            LOGW("[CloudCustomPullStrategy] Unknown conflict ret %d", static_cast<int>(conflictRet));
            return OpType::NOT_HANDLE;
    }
}
} // DistributedDB