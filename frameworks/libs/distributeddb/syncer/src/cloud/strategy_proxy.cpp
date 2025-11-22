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
#include "cloud/strategy_proxy.h"

#include "cloud/cloud_sync_utils.h"
#include "cloud/strategy/strategy_factory.h"
namespace DistributedDB {
StrategyProxy::StrategyProxy(const StrategyProxy &other)
{
    CopyStrategy(other);
}

StrategyProxy &StrategyProxy::operator=(const StrategyProxy &other)
{
    if (&other == this) {
        return *this;
    }
    CopyStrategy(other);
    return *this;
}

void StrategyProxy::CopyStrategy(const StrategyProxy &other)
{
    std::scoped_lock<std::mutex, std::mutex> scopedLock(strategyMutex_, other.strategyMutex_);
    strategy_ = other.strategy_;
}

void StrategyProxy::UpdateStrategy(SyncMode mode, bool isKvScene, SingleVerConflictResolvePolicy policy,
    const std::weak_ptr<ICloudConflictHandler> &handler)
{
    auto strategy = StrategyFactory::BuildSyncStrategy(mode, isKvScene, policy);
    std::lock_guard<std::mutex> autoLock(strategyMutex_);
    strategy_ = strategy;
    strategy_->SetCloudConflictHandler(handler);
}

void StrategyProxy::ResetStrategy()
{
    std::lock_guard<std::mutex> autoLock(strategyMutex_);
    strategy_ = nullptr;
}

void StrategyProxy::SetCloudConflictHandler(const std::shared_ptr<ICloudConflictHandler> &handler)
{
    std::lock_guard<std::mutex> autoLock(strategyMutex_);
    handler_ = handler;
}

bool StrategyProxy::JudgeUpload() const
{
    std::lock_guard<std::mutex> autoLock(strategyMutex_);
    if (strategy_ == nullptr) {
        LOGW("[StrategyProxy] Judge upload without strategy");
        return false;
    }
    return strategy_->JudgeUpload();
}

bool StrategyProxy::JudgeUpdateCursor() const
{
    std::lock_guard<std::mutex> autoLock(strategyMutex_);
    if (strategy_ == nullptr) {
        LOGW("[StrategyProxy] Judge update cursor without strategy");
        return false;
    }
    return strategy_->JudgeUpdateCursor();
}

bool StrategyProxy::JudgeDownload() const
{
    std::lock_guard<std::mutex> autoLock(strategyMutex_);
    if (strategy_ == nullptr) {
        LOGW("[StrategyProxy] Judge download without strategy");
        return false;
    }
    return strategy_->JudgeDownload();
}

bool StrategyProxy::JudgeLocker() const
{
    std::lock_guard<std::mutex> autoLock(strategyMutex_);
    if (strategy_ == nullptr) {
        LOGW("[StrategyProxy] Judge locker without strategy");
        return false;
    }
    return strategy_->JudgeLocker();
}

bool StrategyProxy::JudgeQueryLocalData() const
{
    std::lock_guard<std::mutex> autoLock(strategyMutex_);
    if (strategy_ == nullptr) {
        LOGW("[StrategyProxy] Judge query local without strategy");
        return false;
    }
    return strategy_->JudgeQueryLocalData();
}

std::pair<int, OpType> StrategyProxy::TagStatus(bool isExist, size_t idx, const std::shared_ptr<StorageProxy> &storage,
    ICloudSyncer::SyncParam &param, ICloudSyncer::DataInfo &dataInfo) const
{
    // Get cloudLogInfo from cloud data
    dataInfo.cloudLogInfo = CloudSyncUtils::GetCloudLogInfo(param.downloadData.data[idx]);
    auto res = TagStatusByStrategy(isExist, idx, storage, param, dataInfo);
    param.downloadData.opType[idx] = res.second;
    return res;
}

std::pair<int, OpType> StrategyProxy::TagStatusByStrategy(bool isExist, size_t idx,
    const std::shared_ptr<StorageProxy> &storage,
    ICloudSyncer::SyncParam &param, ICloudSyncer::DataInfo &dataInfo) const
{
    std::pair<int, OpType> res = {E_OK, OpType::NOT_HANDLE};
    auto &[errCode, strategyOpResult] = res;
    // ignore same record with local generate data
    if (dataInfo.localInfo.logInfo.device.empty() &&
        !CloudSyncUtils::NeedSaveData(dataInfo.localInfo.logInfo, dataInfo.cloudLogInfo)) {
        // not handle same data
        return res;
    }
    std::shared_ptr<CloudSyncStrategy> strategy;
    {
        std::lock_guard<std::mutex> autoLock(strategyMutex_);
        if (strategy_ == nullptr) {
            LOGW("[StrategyProxy] Tag data status without strategy");
            errCode = -E_INTERNAL_ERROR;
            return res;
        }
        strategy = strategy_;
    }
    bool isCloudWin = storage->IsTagCloudUpdateLocal(dataInfo.localInfo.logInfo,
        dataInfo.cloudLogInfo, strategy->GetConflictResolvePolicy());
    strategyOpResult = strategy->TagSyncDataStatus({isExist, isCloudWin, param.tableName},
        dataInfo.localInfo.logInfo, dataInfo.localInfo.localData, dataInfo.cloudLogInfo, param.downloadData.data[idx]);
    if (strategyOpResult == OpType::DELETE) {
        param.deletePrimaryKeySet.insert(dataInfo.localInfo.logInfo.hashKey);
    }
    return res;
}
}