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
#include "cloud/cloud_merge_strategy.h"
#include "cloud/cloud_storage_utils.h"

namespace DistributedDB {

OpType CloudMergeStrategy::TagSyncDataStatus(bool existInLocal, const LogInfo &localInfo, const LogInfo &cloudInfo)
{
    if (CloudStorageUtils::IsDataLocked(localInfo.status)) {
        return OpType::LOCKED_NOT_HANDLE;
    }
    bool isCloudDelete = IsDelete(cloudInfo);
    bool isLocalDelete = IsDelete(localInfo);
    if (!existInLocal) {
        // when cloud data is deleted, we think it is different data
        if (isCloudDelete) {
            return OpType::NOT_HANDLE;
        }
        return OpType::INSERT;
    }
    if (localInfo.timestamp > cloudInfo.timestamp) {
        return TagLocallyNewer(localInfo, cloudInfo, isCloudDelete, isLocalDelete);
    }
    if (isCloudDelete) {
        return isLocalDelete ? OpType::UPDATE_TIMESTAMP : OpType::DELETE;
    }
    if (isLocalDelete) {
        return OpType::INSERT;
    }
    // avoid local data insert to cloud success but return failed
    // we just fill back gid here
    bool isTimeSame = (localInfo.timestamp == cloudInfo.timestamp) && (localInfo.wTimestamp == cloudInfo.wTimestamp);
    if (isTimeSame && (localInfo.cloudGid.empty() || IsLogNeedUpdate(cloudInfo, localInfo))) {
        return OpType::ONLY_UPDATE_GID;
    }
    return TagUpdateLocal(cloudInfo, localInfo);
}

bool CloudMergeStrategy::JudgeUpdateCursor()
{
    return true;
}

bool CloudMergeStrategy::JudgeUpload()
{
    return true;
}

OpType CloudMergeStrategy::TagLocallyNewer(const LogInfo &localInfo, const LogInfo &cloudInfo,
    bool isCloudDelete, bool isLocalDelete)
{
    if (localInfo.cloudGid.empty()) {
        return isCloudDelete ? OpType::NOT_HANDLE : (isLocalDelete ? OpType::INSERT : OpType::ONLY_UPDATE_GID);
    }
    if (isCloudDelete) {
        return OpType::CLEAR_GID;
    }
    if (IsLogNeedUpdate(cloudInfo, localInfo)) {
        return OpType::ONLY_UPDATE_GID;
    }
    return OpType::NOT_HANDLE;
}
}