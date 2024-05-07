/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SINGLE_STORE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SINGLE_STORE_IMPL_H
#include <functional>
#include <shared_mutex>

#include "concurrent_map.h"
#include "convertor.h"
#include "dev_manager.h"
#include "kv_store_nb_delegate.h"
#include "kvdb_notifier_client.h"
#include "kvdb_service.h"
#include "kvstore_death_recipient.h"
#include "observer_bridge.h"
#include "single_kvstore.h"
#include "sync_observer.h"
#include "task_executor.h"

namespace OHOS::DistributedKv {
class SingleStoreImpl : public SingleKvStore,
                        public KvStoreDeathRecipient,
                        public KvStoreSyncCallback,
                        public std::enable_shared_from_this<SingleStoreImpl> {
public:
    using Observer = KvStoreObserver;
    using SyncCallback = KvStoreSyncCallback;
    using ResultSet = KvStoreResultSet;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using DBEntry = DistributedDB::Entry;
    using DBKey = DistributedDB::Key;
    using DBValue = DistributedDB::Value;
    using DBQuery = DistributedDB::Query;
    using SyncInfo = KVDBService::SyncInfo;
    using Convert = std::function<Key(const DBKey &key, std::string &deviceId)>;
    using TimePoint = std::chrono::steady_clock::time_point;
    SingleStoreImpl(std::shared_ptr<DBStore> dbStore, const AppId &appId, const Options &options, const Convertor &cvt);
    ~SingleStoreImpl();
    StoreId GetStoreId() const override;
    Status Put(const Key &key, const Value &value) override;
    Status PutBatch(const std::vector<Entry> &entries) override;
    Status Delete(const Key &key) override;
    Status DeleteBatch(const std::vector<Key> &keys) override;
    Status StartTransaction() override;
    Status Commit() override;
    Status Rollback() override;
    Status SubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer) override;
    Status UnSubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer) override;
    Status Get(const Key &key, Value &value) override;
    void Get(const Key &key, const std::string &networkId,
        const std::function<void(Status, Value &&)> &onResult) override;
    void GetEntries(const Key &prefix, const std::string &networkId,
        const std::function<void(Status, std::vector<Entry> &&)> &onResult) override;
    Status GetEntries(const Key &prefix, std::vector<Entry> &entries) const override;
    Status GetEntries(const DataQuery &query, std::vector<Entry> &entries) const override;
    Status GetResultSet(const Key &prefix, std::shared_ptr<ResultSet> &resultSet) const override;
    Status GetResultSet(const DataQuery &query, std::shared_ptr<ResultSet> &resultSet) const override;
    Status CloseResultSet(std::shared_ptr<ResultSet> &resultSet) override;
    Status GetCount(const DataQuery &query, int &count) const override;
    Status GetSecurityLevel(SecurityLevel &secLevel) const override;
    Status RemoveDeviceData(const std::string &device) override;
    Status Backup(const std::string &file, const std::string &baseDir) override;
    Status Restore(const std::string &file, const std::string &baseDir) override;
    Status DeleteBackup(const std::vector<std::string> &files, const std::string &baseDir,
        std::map<std::string, DistributedKv::Status> &status) override;
    void OnRemoteDied() override;
    void SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId) override;
    void SyncCompleted(const std::map<std::string, Status> &results) override;

    // normal function
    int32_t Close(bool isForce = false);
    int32_t AddRef();
    Status SetIdentifier(const std::string &accountId, const std::string &appId,
        const std::string &storeId, const std::vector<std::string> &tagretDev) override;
    // IPC interface
    Status Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t delay) override;
    Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
        std::shared_ptr<SyncCallback> syncCallback, uint32_t delay) override;
    Status RegisterSyncCallback(std::shared_ptr<SyncCallback> callback) override;
    Status UnRegisterSyncCallback() override;
    Status SetSyncParam(const KvSyncParam &syncParam) override;
    Status GetSyncParam(KvSyncParam &syncParam) override;
    Status SetCapabilityEnabled(bool enabled) const override;
    Status SetCapabilityRange(const std::vector<std::string> &local,
        const std::vector<std::string> &remote) const override;
    Status SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) override;
    Status UnsubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) override;
    Status CloudSync(const AsyncDetail &async) override;
protected:
    std::shared_ptr<ObserverBridge> PutIn(uint32_t &realType, std::shared_ptr<Observer> observer);
    std::shared_ptr<ObserverBridge> TakeOut(uint32_t &realType, std::shared_ptr<Observer> observer);

private:
    using Time = ExecutorPool::Time;
    using Duration = ExecutorPool::Duration;
    struct AsyncFunc {
        Key key;
        std::function<void(Status, Value &&)> toGet;
        std::function<void(Status, std::vector<Entry> &&)> toGetEntries;
    };

    static constexpr size_t MAX_VALUE_LENGTH = 4 * 1024 * 1024;
    static constexpr size_t MAX_OBSERVER_SIZE = 8;
    static constexpr Duration SYNC_DURATION = std::chrono::seconds(60);
    static constexpr int32_t INTERVAL = 500; // ms
    static constexpr uint64_t INVALID_SEQ_ID = 0;
    Status GetResultSet(const DBQuery &query, std::shared_ptr<ResultSet> &resultSet) const;
    Status GetEntries(const DBQuery &query, std::vector<Entry> &entries) const;
    Status RetryWithCheckPoint(std::function<DistributedDB::DBStatus()> lambda);
    std::function<void(ObserverBridge *)> BridgeReleaser();
    Status DoSync(const SyncInfo &syncInfo, std::shared_ptr<SyncCallback> observer);
    Status DoSyncExt(const SyncInfo &syncInfo, std::shared_ptr<SyncCallback> observer);
    Status DoClientSync(const SyncInfo &syncInfo, std::shared_ptr<SyncCallback> observer);
    Status SyncExt(const std::string &networkId, uint64_t sequenceId);
    bool IsRemoteChanged(const std::string &deviceId);
    void DoNotifyChange();
    void Register();

    bool autoSync_ = false;
    bool isClientSync_ = false;
    mutable std::shared_mutex rwMutex_;
    const Convertor &convertor_;
    std::string appId_;
    std::string storeId_;
    int32_t ref_ = 1;
    int32_t dataType_ = DataType::TYPE_DYNAMICAL;
    uint32_t roleType_ = 0;
    uint64_t taskId_ = 0;
    std::shared_ptr<DBStore> dbStore_ = nullptr;
    std::shared_ptr<SyncObserver> syncObserver_ = nullptr;
    ConcurrentMap<uintptr_t, std::pair<uint32_t, std::shared_ptr<ObserverBridge>>> observers_;
    ConcurrentMap<uint64_t, AsyncFunc> asyncFuncs_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SINGLE_STORE_IMPL_H
