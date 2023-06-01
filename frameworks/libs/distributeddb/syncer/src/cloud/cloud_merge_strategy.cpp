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

OpType CloudMergeStrategy::TagSyncDataStatus(bool existInLocal, LogInfo &localInfo, LogInfo &cloudInfo)
{
    if (!existInLocal) {
        if ((cloudInfo.flag & 0x1) == 1) {
            return OpType::NOT_HANDLE;
        }
        return OpType::INSERT;
    }
    OpType type = OpType::NOT_HANDLE;
    if (localInfo.timestamp > cloudInfo.timestamp) {
        if (localInfo.cloudGid.empty()) {
            type = OpType::ONLY_UPDATE_GID;
        }
        return type;
    }
    if ((cloudInfo.flag & 0x1) == 1) {
        type = OpType::DELETE;
    } else {
        type = OpType::UPDATE;
    }
    return type;
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