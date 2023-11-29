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

namespace DistributedDB {

OpType CloudMergeStrategy::TagSyncDataStatus(bool existInLocal, const LogInfo &localInfo, const LogInfo &cloudInfo)
{
    bool isCloudDelete = IsDelete(cloudInfo);
    bool isLocalDelete = IsDelete(localInfo);
    if (!existInLocal) {
        // when cloud data is deleted, we think it is different data
        if (isCloudDelete) {
            return OpType::NOT_HANDLE;
        }
        return OpType::INSERT;
    }
    OpType type = OpType::NOT_HANDLE;
    if (localInfo.timestamp > cloudInfo.timestamp) {
        if (localInfo.cloudGid.empty()) {
            type = isCloudDelete ? OpType::NOT_HANDLE : (isLocalDelete ? OpType::INSERT : OpType::ONLY_UPDATE_GID);
        } else {
            type = isCloudDelete ? OpType::CLEAR_GID : type;
        }
        return type;
    }
    if (isCloudDelete) {
        return isLocalDelete ? OpType::UPDATE_TIMESTAMP : OpType::DELETE;
    }
    if (isLocalDelete) {
        return OpType::INSERT;
    }
    // avoid local data insert to cloud success but return failed
    // we just fill back gid here
    if ((localInfo.timestamp == cloudInfo.timestamp) &&
        (localInfo.wTimestamp == cloudInfo.wTimestamp) && localInfo.cloudGid.empty()) {
        return OpType::ONLY_UPDATE_GID;
    }
    return OpType::UPDATE;
}

bool CloudMergeStrategy::JudgeUpdateCursor()
{
    return true;
}

bool CloudMergeStrategy::JudgeUpload()
{
    return true;
}
}