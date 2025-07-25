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

#ifndef KV_STORE_NB_DELEGATE_IMPL_H
#define KV_STORE_NB_DELEGATE_IMPL_H

#include <functional>
#include <map>
#include <mutex>
#include <string>

#include "db_types.h"
#include "store_types.h"
#include "ikvdb_connection.h"
#include "kv_store_nb_conflict_data.h"
#include "kv_store_nb_delegate.h"

namespace DistributedDB {
class KvStoreNbDelegateImpl final : public KvStoreNbDelegate {
public:
    KvStoreNbDelegateImpl(IKvDBConnection *conn, const std::string &storeId);
    ~KvStoreNbDelegateImpl() override;

    DISABLE_COPY_ASSIGN_MOVE(KvStoreNbDelegateImpl);

    // Public zone interfaces
    DBStatus Get(const Key &key, Value &value) const override;

    DBStatus GetEntries(const Key &keyPrefix, std::vector<Entry> &entries) const override;

    DBStatus GetEntries(const Key &keyPrefix, KvStoreResultSet *&resultSet) const override;

    DBStatus GetEntries(const Query &query, std::vector<Entry> &entries) const override;

    DBStatus GetEntries(const Query &query, KvStoreResultSet *&resultSet) const override;

    DBStatus GetCount(const Query &query, int &count) const override;

    DBStatus CloseResultSet(KvStoreResultSet *&resultSet) override;

    DBStatus Put(const Key &key, const Value &value) override;

    DBStatus PutBatch(const std::vector<Entry> &entries) override;

    DBStatus DeleteBatch(const std::vector<Key> &keys) override;

    DBStatus Delete(const Key &key) override;

    // Local zone interfaces
    DBStatus GetLocal(const Key &key, Value &value) const override;

    DBStatus GetLocalEntries(const Key &keyPrefix, std::vector<Entry> &entries) const override;

    DBStatus PutLocal(const Key &key, const Value &value) override;

    DBStatus DeleteLocal(const Key &key) override;

    DBStatus PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
        const KvStoreNbPublishOnConflict &onConflict) override;

    DBStatus UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp) override;

    // Observer interfaces
    DBStatus RegisterObserver(const Key &key, unsigned int mode, KvStoreObserver *observer) override;

    DBStatus UnRegisterObserver(const KvStoreObserver *observer) override;

    DBStatus RemoveDeviceData(const std::string &device) override;

    // Other interfaces
    std::string GetStoreId() const override;

    // Sync function interface, if wait set true, this function will be blocked until sync finished
    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        bool wait) override;

    // Special pragma interface, see PragmaCmd and PragmaData,
    DBStatus Pragma(PragmaCmd cmd, PragmaData &paramData) override;

    // Set the conflict notifier for getting the specified type conflict data.
    DBStatus SetConflictNotifier(int conflictType, const KvStoreNbConflictNotifier &notifier) override;

    // Rekey the database.
    DBStatus Rekey(const CipherPassword &password) override;

    // Empty passwords represent non-encrypted files.
    // Export existing database files to a specified database file in the specified directory.
    DBStatus Export(const std::string &filePath, const CipherPassword &passwd, bool force) override;

    // Import the existing database files to the specified database file in the specified directory.
    DBStatus Import(const std::string &filePath, const CipherPassword &passwd,
        bool isNeedIntegrityCheck = false) override;

    // Start a transaction
    DBStatus StartTransaction() override;

    // Commit a transaction
    DBStatus Commit() override;

    // Rollback a transaction
    DBStatus Rollback() override;

    DBStatus PutLocalBatch(const std::vector<Entry> &entries) override;

    DBStatus DeleteLocalBatch(const std::vector<Key> &keys) override;

    // Get the SecurityOption of this kvStore.
    DBStatus GetSecurityOption(SecurityOption &option) const override;

    DBStatus SetRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier) override;

    void SetReleaseFlag(bool flag);

    DBStatus Close();

    // Sync function interface, if wait set true, this function will be blocked until sync finished.
    // Param query used to filter the records to be synchronized.
    // Now just support push mode and query by prefixKey.
    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait) override;

    // Sync with devices, provides sync count information
    DBStatus Sync(const DeviceSyncOption &option, const DeviceSyncProcessCallback &onProcess) override;

    // Cancel sync by syncId
    DBStatus CancelSync(uint32_t syncId) override;

    DBStatus CheckIntegrity() const override;

    // Set an equal identifier for this database, After this called, send msg to the target will use this identifier
    DBStatus SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets) override;

    DBStatus SetPushDataInterceptor(const PushDataInterceptor &interceptor) override;

    // Register a subscriber query on peer devices. The data in the peer device meets the subscriber query condition
    // will automatically push to the local device when it's changed.
    DBStatus SubscribeRemoteQuery(const std::vector<std::string> &devices,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait) override;

    // Unregister a subscriber query on peer devices.
    DBStatus UnSubscribeRemoteQuery(const std::vector<std::string> &devices,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait) override;

    DBStatus RemoveDeviceData() override;

    DBStatus GetKeys(const Key &keyPrefix, std::vector<Key> &keys) const override;

    size_t GetSyncDataSize(const std::string &device) const override;

    // update all key in sync_data which is not deleted data
    DBStatus UpdateKey(const UpdateKeyCallback &callback) override;

    std::pair<DBStatus, WatermarkInfo> GetWatermarkInfo(const std::string &device) override;

    DBStatus RemoveDeviceData(const std::string &device, ClearMode mode) override;

    DBStatus RemoveDeviceData(const std::string &device, const std::string &user, ClearMode mode) override;

    int32_t GetTaskCount() override;

    DBStatus SetReceiveDataInterceptor(const DataInterceptor &interceptor) override;

    DBStatus GetDeviceEntries(const std::string &device, std::vector<Entry> &entries) const override;

    DatabaseStatus GetDatabaseStatus() const override;

#ifdef USE_DISTRIBUTEDDB_CLOUD
    DBStatus Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess) override;

    DBStatus SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs) override;

    DBStatus SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema) override;

    void SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback) override;

    DBStatus SetCloudSyncConfig(const CloudSyncConfig &config) override;

    std::pair<DBStatus, std::map<std::string, std::string>> GetCloudVersion(const std::string &device) override;

    DBStatus ClearMetaData(ClearKvMetaDataOption option) override;
#endif

    DBStatus OperateDataStatus(uint32_t dataOperator) override;

    void SetHandle(void *handle);

private:
    DBStatus GetInner(const IOption &option, const Key &key, Value &value) const;
    DBStatus PutInner(const IOption &option, const Key &key, const Value &value);
    DBStatus DeleteInner(const IOption &option, const Key &key);
    DBStatus GetEntriesInner(const IOption &option, const Key &keyPrefix, std::vector<Entry> &entries) const;

    void OnSyncComplete(const std::map<std::string, int> &statuses,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete) const;
    
    void OnDeviceSyncProcess(const std::map<std::string, DeviceSyncProcess> &processMap,
        const DeviceSyncProcessCallback &onProcess) const;

    DBStatus RegisterDeviceObserver(const Key &key, unsigned int mode, KvStoreObserver *observer);

    DBStatus RegisterCloudObserver(const Key &key, unsigned int mode, KvStoreObserver *observer);

    DBStatus UnRegisterDeviceObserver(const KvStoreObserver *observer);

    DBStatus UnRegisterCloudObserver(const KvStoreObserver *observer);

    DBStatus CheckDeviceObserver(const Key &key, unsigned int mode, KvStoreObserver *observer);

    DBStatus CheckCloudObserver(KvStoreObserver *observer);

    IKvDBConnection *conn_;
    std::string storeId_;
    bool releaseFlag_;
#ifndef _WIN32
    mutable std::mutex libMutex_;
    void *dlHandle_ = nullptr;
#endif
    std::mutex observerMapLock_;
    std::map<const KvStoreObserver *, const KvDBObserverHandle *> observerMap_;
    std::map<const KvStoreObserver *, unsigned int> cloudObserverMap_;
};
} // namespace DistributedDB

#endif // KV_STORE_NB_DELEGATE_IMPL_H
