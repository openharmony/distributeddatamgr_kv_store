/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef CONFLICT_HANDLER_H
#define CONFLICT_HANDLER_H

#include "cloud/icloud_conflict_handler.h"
#include "cloud/icloud_syncer.h"
#include "cloud/strategy/cloud_sync_strategy.h"
#include "storage_proxy.h"

namespace DistributedDB {
class StrategyProxy {
public:
    StrategyProxy() = default;
    ~StrategyProxy() = default;

    StrategyProxy(const StrategyProxy &other);
    StrategyProxy &operator=(const StrategyProxy &other);

    bool JudgeUpload() const;
    bool JudgeUpdateCursor() const;
    bool JudgeDownload() const;
    bool JudgeLocker() const;
    bool JudgeQueryLocalData() const;
    std::pair<int, OpType> TagStatus(bool isExist, size_t idx, const std::shared_ptr<StorageProxy> &storage,
        ICloudSyncer::SyncParam &param, ICloudSyncer::DataInfo &dataInfo) const;
    std::pair<int, OpType> TagStatusByStrategy(bool isExist, size_t idx, const std::shared_ptr<StorageProxy> &storage,
        ICloudSyncer::SyncParam &param, ICloudSyncer::DataInfo &dataInfo) const;
    void UpdateStrategy(SyncMode mode, bool isKvScene, SingleVerConflictResolvePolicy policy,
        const std::weak_ptr<ICloudConflictHandler> &handler);
    void ResetStrategy();
    void SetCloudConflictHandler(const std::shared_ptr<ICloudConflictHandler> &handler);
private:
    void CopyStrategy(const StrategyProxy &other);
    mutable std::mutex strategyMutex_;
    std::shared_ptr<CloudSyncStrategy> strategy_;
    std::shared_ptr<ICloudConflictHandler> handler_;
};
}
#endif // CONFLICT_HANDLER_H
