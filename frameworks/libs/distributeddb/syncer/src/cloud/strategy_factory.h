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

#ifndef STRATEGY_FACTORY_H
#define STRATEGY_FACTORY_H
#include "cloud_sync_strategy.h"
#include "db_common.h"
#include "store_types.h"
namespace DistributedDB {
class StrategyFactory {
public:
    static std::shared_ptr<CloudSyncStrategy> BuildSyncStrategy(SyncMode mode,
        SingleVerConflictResolvePolicy policy = SingleVerConflictResolvePolicy::DEFAULT_LAST_WIN);
};
}

#endif // STRATEGY_FACTORY_H
