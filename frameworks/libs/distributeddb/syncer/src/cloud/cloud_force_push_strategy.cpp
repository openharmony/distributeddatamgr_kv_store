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
#include "cloud_force_push_strategy.h"

namespace DistributedDB {
const std::string cloud_device_name = "cloud";
OpType CloudForcePushStrategy::TagSyncDataStatus(bool existInLocal, LogInfo &localInfo, LogInfo &cloudInfo,
    std::set<Key> &deletePrimaryKeySet)
{
    (void)deletePrimaryKeySet;
    bool isCloudDelete = IsDelete(cloudInfo);
    if (!existInLocal) {
        return OpType::NOT_HANDLE;
    }

    if (localInfo.cloudGid.empty()) {
        // when cloud data is deleted, we think it is different data
        return isCloudDelete ? OpType::NOT_HANDLE : OpType::ONLY_UPDATE_GID;
    }

    if (isCloudDelete) {
        return OpType::CLEAR_GID;
    }
    if (localInfo.device == cloud_device_name && localInfo.timestamp == cloudInfo.timestamp) {
        return OpType::SET_CLOUD_FORCE_PUSH_FLAG_ONE;
    }
    if (localInfo.device == cloud_device_name && localInfo.timestamp != cloudInfo.timestamp) {
        return OpType::SET_CLOUD_FORCE_PUSH_FLAG_ZERO;
    }
    return OpType::NOT_HANDLE;
}

bool CloudForcePushStrategy::JudgeUpdateCursor()
{
    return false;
}

bool CloudForcePushStrategy::JudgeUpload()
{
    return true;
}
}