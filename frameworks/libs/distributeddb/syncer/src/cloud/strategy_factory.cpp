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

#include "cloud/cloud_force_pull_strategy.h"
#include "cloud/cloud_force_push_strategy.h"
#include "cloud/cloud_merge_strategy.h"
#include "strategy_factory.h"

namespace DistributedDB {
std::shared_ptr<CloudSyncStrategy> StrategyFactory::BuildSyncStrategy(SyncMode mode,
    SingleVerConflictResolvePolicy policy)
{
    std::shared_ptr<CloudSyncStrategy> strategy;
    switch (mode) {
        case SyncMode::SYNC_MODE_CLOUD_MERGE:
            strategy = std::make_shared<CloudMergeStrategy>();
            break;
        case SyncMode::SYNC_MODE_CLOUD_FORCE_PULL:
            strategy = std::make_shared<CloudForcePullStrategy>();
            break;
        case SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH:
            strategy = std::make_shared<CloudForcePushStrategy>();
            break;
        default:
            LOGW("[StrategyFactory] Not support mode %d", static_cast<int>(mode));
            strategy = std::make_shared<CloudSyncStrategy>();
    }
    strategy->SetConflictResolvePolicy(policy);
    return strategy;
}
}
