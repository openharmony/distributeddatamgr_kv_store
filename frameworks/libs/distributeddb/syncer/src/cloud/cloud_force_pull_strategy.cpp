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
#include "cloud_force_pull_strategy.h"
#include "cloud/cloud_storage_utils.h"

namespace DistributedDB {

OpType CloudForcePullStrategy::TagSyncDataStatus(bool existInLocal, const LogInfo &localInfo, const LogInfo &cloudInfo)
{
    if (CloudStorageUtils::IsDataLocked(localInfo.status)) {
        return OpType::LOCKED_NOT_HANDLE;
    }
    if (existInLocal) {
        if (!IsDelete(localInfo) && IsDelete(cloudInfo)) {
            return OpType::DELETE;
        } else if (IsDelete(cloudInfo)) {
            return OpType::UPDATE_TIMESTAMP;
        }
        if (IsDelete(localInfo)) {
            return OpType::INSERT;
        }
        return TagUpdateLocal(cloudInfo, localInfo);
    }
    bool gidEmpty = localInfo.cloudGid.empty();
    if (IsDelete(cloudInfo)) {
        return gidEmpty ? OpType::NOT_HANDLE : OpType::DELETE;
    }
    if (gidEmpty) {
        return OpType::INSERT;
    }
    return TagUpdateLocal(cloudInfo, localInfo);
}

bool CloudForcePullStrategy::JudgeUpdateCursor()
{
    return true;
}

bool CloudForcePullStrategy::JudgeUpload()
{
    return false;
}
}