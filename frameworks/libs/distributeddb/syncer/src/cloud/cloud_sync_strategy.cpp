/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "cloud/cloud_sync_strategy.h"

namespace DistributedDB {
CloudSyncStrategy::CloudSyncStrategy() : policy_(SingleVerConflictResolvePolicy::DEFAULT_LAST_WIN)
{
}

void CloudSyncStrategy::SetConflictResolvePolicy(SingleVerConflictResolvePolicy policy)
{
    policy_ = policy;
}

OpType CloudSyncStrategy::TagSyncDataStatus([[gnu::unused]] bool existInLocal,
    [[gnu::unused]] const LogInfo &localInfo, [[gnu::unused]] const LogInfo &cloudInfo)
{
    return OpType::NOT_HANDLE;
}

bool CloudSyncStrategy::JudgeUpdateCursor()
{
    return false;
}

bool CloudSyncStrategy::JudgeUpload()
{
    return false;
}

bool CloudSyncStrategy::IsDelete(const LogInfo &info)
{
    return (info.flag & static_cast<uint32_t>(LogInfoFlag::FLAG_DELETE)) ==
        static_cast<uint32_t>(LogInfoFlag::FLAG_DELETE);
}

bool CloudSyncStrategy::IsLogNeedUpdate(const LogInfo &cloudInfo, const LogInfo &localInfo)
{
    return (cloudInfo.sharingResource != localInfo.sharingResource) || (cloudInfo.version != localInfo.version);
}

bool CloudSyncStrategy::IsSameVersion(const LogInfo &cloudInfo, const LogInfo &localInfo)
{
    if (cloudInfo.version.empty() || localInfo.version.empty()) {
        return false;
    }
    return (cloudInfo.version == localInfo.version);
}

bool CloudSyncStrategy::IsIgnoreUpdate(const LogInfo &localInfo)
{
    if (policy_ == SingleVerConflictResolvePolicy::DEFAULT_LAST_WIN) {
        return false;
    }
    if (localInfo.originDev.empty() && localInfo.device.empty()) {
        LOGW("[CloudSyncStrategy] %.3s was ignored override", localInfo.cloudGid.c_str());
        return true;
    }
    return false;
}

OpType CloudSyncStrategy::TagUpdateLocal(const LogInfo &cloudInfo, const LogInfo &localInfo)
{
    return IsIgnoreUpdate(localInfo) ? OpType::NOT_HANDLE : OpType::UPDATE;
}
}