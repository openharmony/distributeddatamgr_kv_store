/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "KVDBNotifierClient"

#include "kvdb_notifier_client.h"
#include <atomic>
#include <cinttypes>
#include <functional>
#include "dds_trace.h"
#include "dev_manager.h"
#include "log_print.h"
#include "store_util.h"

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::DistributedDataDfx;
KVDBNotifierClient::~KVDBNotifierClient()
{
    syncCallbackInfo_.Clear();
}

void KVDBNotifierClient::SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    auto finded = syncCallbackInfo_.Find(sequenceId);
    if (finded.first) {
        finded.second->SyncCompleted(results);
        finded.second->SyncCompleted(results, sequenceId);
        DeleteSyncCallback(sequenceId);
    }
}

void KVDBNotifierClient::SyncCompleted(uint64_t seqNum, ProgressDetail &&detail)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), TraceSwitch::BYTRACE_ON);
    cloudSyncCallbacks_.ComputeIfPresent(seqNum, [&detail](const auto &key, const AsyncDetail &callback) {
        auto finished = (detail.progress == SYNC_FINISH);
        ZLOGD("Sync complete, seqNum%{public}" PRIu64, key);
        if (callback != nullptr) {
            callback(std::move(detail));
        }
        return !finished;
    });
}

void KVDBNotifierClient::OnRemoteChange(const std::map<std::string, bool> &mask)
{
    ZLOGD("remote changed mask:%{public}zu", mask.size());
    for (const auto &[device, value] : mask) {
        auto clientUuid = DevManager::GetInstance().ToUUID(device);
        if (clientUuid.empty()) {
            continue;
        }
        remotes_.InsertOrAssign(clientUuid, value);
    }
}

void KVDBNotifierClient::OnSwitchChange(const SwitchNotification &notification)
{
    switchObservers_.ForEachCopies(
        [&notification](auto &, std::shared_ptr<KvStoreObserver> &observer) {
            observer->OnSwitchChange(notification);
            return false;
    });
}

bool KVDBNotifierClient::IsChanged(const std::string &deviceId)
{
    auto [exist, value] = remotes_.Find(deviceId);
    ZLOGD("exist:%{public}d, changed:%{public}d", exist, value);
    return exist ? value : true; // no exist, need to sync.
}

void KVDBNotifierClient::AddSyncCallback(
    const std::shared_ptr<KvStoreSyncCallback> callback, uint64_t sequenceId)
{
    if (callback == nullptr) {
        ZLOGE("callback is nullptr");
        return;
    }
    auto inserted = syncCallbackInfo_.Insert(sequenceId, callback);
    if (!inserted) {
        ZLOGE("The sequeuceId %{public}" PRIu64 "is repeat!", sequenceId);
    }
}

void KVDBNotifierClient::DeleteSyncCallback(uint64_t sequenceId)
{
    syncCallbackInfo_.Erase(sequenceId);
}

void KVDBNotifierClient::AddCloudSyncCallback(uint64_t sequenceId, const AsyncDetail &async)
{
    if (async == nullptr) {
        ZLOGE("Cloud sync callback is nullptr");
        return;
    }
    cloudSyncCallbacks_.Insert(sequenceId, async);
}

void KVDBNotifierClient::DeleteCloudSyncCallback(uint64_t sequenceId)
{
    cloudSyncCallbacks_.Erase(sequenceId);
}

void KVDBNotifierClient::AddSwitchCallback(const std::string &appId, std::shared_ptr<KvStoreObserver> observer)
{
    if (observer == nullptr) {
        return;
    }
    if (switchObservers_.Contains(uintptr_t(observer.get()))) {
        ZLOGI("duplicate observer");
        return;
    }
    switchObservers_.Insert(uintptr_t(observer.get()), observer);
}

void KVDBNotifierClient::DeleteSwitchCallback(const std::string &appId, std::shared_ptr<KvStoreObserver> observer)
{
    if (observer == nullptr) {
        return;
    }
    switchObservers_.Erase(uintptr_t(observer.get()));
}
}  // namespace DistributedKv
}  // namespace OHOS
