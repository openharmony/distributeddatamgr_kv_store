/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_NOTIFIER_CLIENT_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_NOTIFIER_CLIENT_H

#include <mutex>
#include "concurrent_map.h"
#include "kvdb_notifier_stub.h"
#include "kvstore_sync_callback.h"

namespace OHOS {
namespace DistributedKv {
class KVDBNotifierClient : public KVDBNotifierStub {
public:
    KVDBNotifierClient() = default;
    virtual ~KVDBNotifierClient();

    void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override;
    void SyncCompleted(uint64_t seqNum, ProgressDetail &&detail) override;

    void OnRemoteChange(const std::map<std::string, bool> &mask) override;

    void OnSwitchChange(const SwitchNotification &notification) override;

    void AddSyncCallback(const std::shared_ptr<KvStoreSyncCallback> callback, uint64_t sequenceId);

    void DeleteSyncCallback(uint64_t sequenceId);

    void AddSwitchCallback(const std::string &appId, std::shared_ptr<KvStoreObserver> observer);

    void DeleteSwitchCallback(const std::string &appId, std::shared_ptr<KvStoreObserver> observer);

    void AddCloudSyncCallback(uint64_t sequenceId, const AsyncDetail &async);
    void DeleteCloudSyncCallback(uint64_t sequenceId);

    bool IsChanged(const std::string &deviceId);

private:
    ConcurrentMap<uint64_t, std::shared_ptr<KvStoreSyncCallback>> syncCallbackInfo_;
    ConcurrentMap<std::string, bool> remotes_;
    ConcurrentMap<uintptr_t, std::shared_ptr<KvStoreObserver>> switchObservers_;
    ConcurrentMap<uint64_t, AsyncDetail> cloudSyncCallbacks_;
};
} // namespace DistributedKv
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_NOTIFIER_CLIENT_H
