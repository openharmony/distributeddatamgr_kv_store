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

#include "include/kvdb_notifier_client_mock.h"

namespace OHOS::DistributedKv {

KVDBNotifierClient::~KVDBNotifierClient()
{
}

bool KVDBNotifierClient::IsChanged(const std::string &deviceId, DataType dataType)
{
    if (BKVDBNotifierClient::kVDBNotifierClient == nullptr) {
        return false;
    }
    return BKVDBNotifierClient::kVDBNotifierClient->IsChanged(deviceId, dataType);
}

void KVDBNotifierClient::AddSyncCallback(const std::shared_ptr<KvStoreSyncCallback> callback, uint64_t sequenceId)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->AddSyncCallback(callback, sequenceId);
    }
}

void KVDBNotifierClient::DeleteSyncCallback(uint64_t sequenceId)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->DeleteSyncCallback(sequenceId);
    }
}

void KVDBNotifierClient::AddCloudSyncCallback(uint64_t sequenceId, const AsyncDetail &async)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->AddCloudSyncCallback(sequenceId, async);
    }
}

void KVDBNotifierClient::DeleteCloudSyncCallback(uint64_t sequenceId)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->DeleteCloudSyncCallback(sequenceId);
    }
}

void KVDBNotifierClient::AddSwitchCallback(const std::string &appId, std::shared_ptr<KvStoreObserver> observer)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->AddSwitchCallback(appId, observer);
    }
}

void KVDBNotifierClient::DeleteSwitchCallback(const std::string &appId, std::shared_ptr<KvStoreObserver> observer)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->DeleteSwitchCallback(appId, observer);
    }
}

void KVDBNotifierClient::SyncCompleted(uint64_t seqNum, ProgressDetail &&detail)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->SyncCompleted(seqNum, std::move(detail));
    }
}

void KVDBNotifierClient::SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->SyncCompleted(results, sequenceId);
    }
}

void KVDBNotifierClient::OnRemoteChange(const std::map<std::string, bool> &mask, int32_t dataType)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->OnRemoteChange(mask, dataType);
    }
}

void KVDBNotifierClient::OnSwitchChange(const SwitchNotification &notification)
{
    if (BKVDBNotifierClient::kVDBNotifierClient != nullptr) {
        return BKVDBNotifierClient::kVDBNotifierClient->OnSwitchChange(notification);
    }
}
} // namespace OHOS::DistributedKv