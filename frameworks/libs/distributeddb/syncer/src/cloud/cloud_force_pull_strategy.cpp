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
#ifdef MANNUAL_SYNC_AND_CLEAN_CLOUD_DATA
#include "cloud_force_pull_strategy.h"
#include "db_common.h"
#include "db_errno.h"

namespace DistributedDB {

OpType CloudForcePullStrategy::TagSyncDataStatus(bool existInLocal, LogInfo &localInfo, LogInfo &cloudInfo)
{
    bool gidEmpty = localInfo.cloudGid.empty();
    if (existInLocal) {
        if (!IsDelete(localInfo) && IsDelete(cloudInfo)) {
            return OpType::DELETE;
        } else {
            // When gid doestn't exist in local, it will fill gid on local also.
            return OpType::UPDATE;
        }
    } else {
        if (IsDelete(cloudInfo)) {
            return gidEmpty ? OpType::NOT_HANDLE : OpType::DELETE;
        } else {
            return gidEmpty ? OpType::INSERT : OpType::UPDATE;
        }
    }
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
#endif // MANNUAL_SYNC_AND_CLEAN_CLOUD_DATA