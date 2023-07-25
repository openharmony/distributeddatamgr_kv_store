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

#ifndef DIFFERENTIAL_STRATEGY_H
#define DIFFERENTIAL_STRATEGY_H
#include "cloud_sync_strategy.h"

namespace DistributedDB {
class CloudMergeStrategy : public CloudSyncStrategy {
public:
    CloudMergeStrategy() = default;
    ~CloudMergeStrategy() override = default;

    OpType TagSyncDataStatus(bool existInLocal, LogInfo &localInfo, LogInfo &cloudInfo,
        std::set<Key> &deletePrimaryKeySet) override;

    bool JudgeUpdateCursor() override;

    bool JudgeUpload() override;
};
}
#endif // DIFFERENTIAL_STRATEGY_H
