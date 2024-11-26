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

#ifndef OHOS_DISTRIBUTED_DATA_KVDB_NOTIFIER_CLIENT_MOCK_H
#define OHOS_DISTRIBUTED_DATA_KVDB_NOTIFIER_CLIENT_MOCK_H

#include <gmock/gmock.h>
#include "kvdb_notifier_client.h"

namespace OHOS::DistributedKv {
class BKVDBNotifierClient {
public:
    virtual bool IsChanged(const std::string &, DataType) = 0;
    virtual void AddSyncCallback(const std::shared_ptr<KvStoreSyncCallback>, uint64_t) = 0;
    virtual void DeleteSyncCallback(uint64_t) = 0;
    virtual void AddCloudSyncCallback(uint64_t, const AsyncDetail &) = 0;
    virtual void DeleteCloudSyncCallback(uint64_t) = 0;
    virtual void AddSwitchCallback(const std::string &, std::shared_ptr<KvStoreObserver>) = 0;
    virtual void DeleteSwitchCallback(const std::string &, std::shared_ptr<KvStoreObserver>) = 0;
    virtual void SyncCompleted(uint64_t, ProgressDetail &&) = 0;
    virtual void SyncCompleted(const std::map<std::string, Status> &, uint64_t) = 0;
    virtual void OnRemoteChange(const std::map<std::string, bool> &, int32_t) = 0;
    virtual void OnSwitchChange(const SwitchNotification &) = 0;
    BKVDBNotifierClient() = default;
    virtual ~BKVDBNotifierClient() = default;
public:
    static inline std::shared_ptr<BKVDBNotifierClient> kVDBNotifierClient = nullptr;
};

class KVDBNotifierClientMock : public BKVDBNotifierClient {
public:
    MOCK_METHOD(bool, IsChanged, (const std::string&, DataType dataType));
    MOCK_METHOD(void, AddSyncCallback, (const std::shared_ptr<KvStoreSyncCallback>, uint64_t));
    MOCK_METHOD(void, DeleteSyncCallback, (uint64_t));
    MOCK_METHOD(void, AddCloudSyncCallback, (uint64_t, const AsyncDetail&));
    MOCK_METHOD(void, DeleteCloudSyncCallback, (uint64_t));
    MOCK_METHOD(void, AddSwitchCallback, (const std::string&, std::shared_ptr<KvStoreObserver>));
    MOCK_METHOD(void, DeleteSwitchCallback, (const std::string&, std::shared_ptr<KvStoreObserver>));
    MOCK_METHOD(void, SyncCompleted, (uint64_t, ProgressDetail &&));
    MOCK_METHOD(void, SyncCompleted, ((const std::map<std::string, Status>&), uint64_t));
    MOCK_METHOD(void, OnRemoteChange, ((const std::map<std::string, bool>&), int32_t));
    MOCK_METHOD(void, OnSwitchChange, (const SwitchNotification &));
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_KVDB_NOTIFIER_CLIENT_MOCK_H