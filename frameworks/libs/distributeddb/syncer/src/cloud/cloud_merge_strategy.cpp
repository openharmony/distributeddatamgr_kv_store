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

OpType CloudMergeStrategy::TagSyncDataStatus(bool existInLocal, bool isCloudWin, const LogInfo &localInfo,
    const LogInfo &cloudInfo)
{
    bool isCloudDelete = IsDelete(cloudInfo);
    bool isLocalDelete = IsDelete(localInfo);
    if (isCloudWin) {
        return TagCloudUpdateLocal(localInfo, cloudInfo, isCloudDelete, isLocalDelete);
    }
    if (CloudStorageUtils::IsDataLocked(localInfo.status)) {
        return OpType::LOCKED_NOT_HANDLE;
    }
    if (!existInLocal) {
        return TagLocalNotExist(isCloudDelete);
    }
    if (IsIgnoreUpdate(localInfo)) {
        return OpType::NOT_HANDLE;
    }
    if (JudgeLocallyNewer(localInfo, cloudInfo)) {
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
    if (isTimeSame && (localInfo.cloudGid.empty() || cloudInfo.sharingResource != localInfo.sharingResource)) {
        return OpType::ONLY_UPDATE_GID;
    }
    if (!localInfo.isNeedUpdateAsset && IsSameRecord(cloudInfo, localInfo)) {
        return OpType::NOT_HANDLE;
    }
    return TagLoginUserAndUpdate(localInfo, cloudInfo);
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
        return isCloudDelete ? OpType::NOT_HANDLE
                : ((isLocalDelete && !JudgeKvScene()) ? OpType::INSERT : OpType::ONLY_UPDATE_GID);
    }
    if (isCloudDelete) {
        return OpType::CLEAR_GID;
    }
    if (IsLogNeedUpdate(cloudInfo, localInfo)) {
        return OpType::ONLY_UPDATE_GID;
    }
    return OpType::NOT_HANDLE;
}

OpType CloudMergeStrategy::TagLoginUserAndUpdate(const LogInfo &localInfo, const LogInfo &cloudInfo)
{
    if (JudgeKvScene() && (localInfo.flag & static_cast<uint64_t>(LogInfoFlag::FLAG_LOCAL)) == 0 &&
        (localInfo.cloud_flag & static_cast<uint64_t>(LogInfoFlag::FLAG_LOGIN_USER)) == 0
        && localInfo.timestamp > cloudInfo.timestamp) {
        return OpType::NOT_HANDLE;
    }
    return TagUpdateLocal(cloudInfo, localInfo);
}

OpType CloudMergeStrategy::TagCloudUpdateLocal(const LogInfo &localInfo, const LogInfo &cloudInfo,
    bool isCloudDelete, bool isLocalDelete)
{
    if (isCloudDelete) {
        return isLocalDelete ? OpType::UPDATE_TIMESTAMP : OpType::DELETE;
    }
    if (isLocalDelete) {
        return OpType::INSERT;
    }
    return TagUpdateLocal(cloudInfo, localInfo);
}

OpType CloudMergeStrategy::TagLocalNotExist(bool isCloudDelete)
{
    // when cloud data is deleted, we think it is different data
    if (isCloudDelete) {
        return OpType::NOT_HANDLE;
    }
    return OpType::INSERT;
}

bool CloudMergeStrategy::JudgeLocallyNewer(const LogInfo &localInfo, const LogInfo &cloudInfo)
{
    return (localInfo.timestamp > cloudInfo.timestamp) &&
        ((JudgeKvScene() && (localInfo.flag & static_cast<uint64_t>(LogInfoFlag::FLAG_CLOUD_WRITE)) == 0) ||
        ((!JudgeKvScene()) && (localInfo.flag & static_cast<uint64_t>(LogInfoFlag::FLAG_LOCAL)) != 0));
}
}