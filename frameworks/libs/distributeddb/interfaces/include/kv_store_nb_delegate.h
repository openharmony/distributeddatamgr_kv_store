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

#ifndef KV_STORE_NB_DELEGATE_H
#define KV_STORE_NB_DELEGATE_H

#include <functional>
#include <map>
#include <string>

#include "cloud/cloud_store_types.h"
#include "cloud/icloud_db.h"
#include "intercepted_data.h"
#include "iprocess_system_api_adapter.h"
#include "kv_store_nb_conflict_data.h"
#include "kv_store_observer.h"
#include "kv_store_result_set.h"
#include "query.h"
#include "store_types.h"

namespace DistributedDB {
using KvStoreNbPublishOnConflict = std::function<void (const Entry &local, const Entry *sync, bool isLocalLastest)>;
using KvStoreNbConflictNotifier = std::function<void (const KvStoreNbConflictData &data)>;

class KvStoreNbDelegate {
public:
    struct Option {
        bool createIfNecessary = true;
        bool isMemoryDb = false;
        bool isEncryptedDb = false;
        CipherType cipher = CipherType::DEFAULT;
        CipherPassword passwd;
        std::string schema = "";
        bool createDirByStoreIdOnly = false;
        SecurityOption secOption; // Add data security level parameter
        KvStoreObserver *observer = nullptr;
        Key key; // The key that needs to be subscribed on obsever, empty means full subscription
        unsigned int mode = 0; // obsever mode
        int conflictType = 0;
        KvStoreNbConflictNotifier notifier = nullptr;
        int conflictResolvePolicy = LAST_WIN;
        bool isNeedIntegrityCheck = false;
        bool isNeedRmCorruptedDb = false;
        bool isNeedCompressOnSync = false;
        uint8_t compressionRate = 100; // Valid in [1, 100].
        bool syncDualTupleMode = false; // communicator label use dualTuple hash or not
        bool localOnly = false; // active sync module
        std::string storageEngineType = SQLITE; // use gaussdb_rd as storage engine
        Rdconfig rdconfig;
    };

    struct DatabaseStatus {
        bool isRebuild = false;
    };

    DB_API virtual ~KvStoreNbDelegate() {}

    // Public zone interfaces
    // Get value from the public zone of this store according to the key.
    DB_API virtual DBStatus Get(const Key &key, Value &value) const = 0;

    // Get entries from the public zone of this store by key prefix.
    // If 'keyPrefix' is empty, It would return all the entries in the zone.
    DB_API virtual DBStatus GetEntries(const Key &keyPrefix, std::vector<Entry> &entries) const = 0;

    // Get entries from the public zone of this store by key prefix.
    // If 'keyPrefix' is empty, It would return all the entries in the zone.
    DB_API virtual DBStatus GetEntries(const Key &keyPrefix, KvStoreResultSet *&resultSet) const = 0;

    // Get entries from the public zone of this store by query.
    // If 'query' is empty, It would return all the entries in the zone.
    DB_API virtual DBStatus GetEntries(const Query &query, std::vector<Entry> &entries) const = 0;

    // Get entries from the public zone of this store by query.
    // If query is empty, It would return all the entries in the zone.
    DB_API virtual DBStatus GetEntries(const Query &query, KvStoreResultSet *&resultSet) const = 0;

    // Get count from the public zone of this store those meet conditions.
    // If query is empty, It would return all the entries count in the zone.
    DB_API virtual DBStatus GetCount(const Query &query, int &count) const = 0;

    // Close the result set returned by GetEntries().
    DB_API virtual DBStatus CloseResultSet(KvStoreResultSet *&resultSet) = 0;

    // Put one key-value entry into the public zone of this store.
    DB_API virtual DBStatus Put(const Key &key, const Value &value) = 0;

    // Put a batch of entries into the public zone of this store.
    DB_API virtual DBStatus PutBatch(const std::vector<Entry> &entries) = 0;

    // Delete a batch of entries from the public zone of this store.
    DB_API virtual DBStatus DeleteBatch(const std::vector<Key> &keys) = 0;

    // Delete one key-value entry from the public zone of this store according to the key.
    DB_API virtual DBStatus Delete(const Key &key) = 0;

    // Local zone interfaces
    // Get value from the local zone of this store according to the key.
    DB_API virtual DBStatus GetLocal(const Key &key, Value &value) const = 0;

    // Get key-value entries from the local zone of this store by key prefix.
    // If keyPrefix is empty, It would return all the entries in the local zone.
    DB_API virtual DBStatus GetLocalEntries(const Key &keyPrefix, std::vector<Entry> &entries) const = 0;

    // Put one key-value entry into the local zone of this store.
    DB_API virtual DBStatus PutLocal(const Key &key, const Value &value) = 0;

    // Delete one key-value entry from the local zone of this store according to the key.
    DB_API virtual DBStatus DeleteLocal(const Key &key) = 0;

    // Migrating(local zone <-> public zone) interfaces
    // Publish a local key-value entry.
    // Migrate the entry from the local zone to public zone.
    DB_API virtual DBStatus PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
        const KvStoreNbPublishOnConflict &onConflict) = 0;

    // Unpublish a public key-value entry.
    // Migrate the entry from the public zone to local zone.
    DB_API virtual DBStatus UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp) = 0;

    // Observer interfaces
    // Register one observer which concerns the key and the changed data mode.
    // If key is empty, observer would get all the changed data of the mode.
    // There are three mode: native changes of nb syncable kv store,
    //                       synced data changes from remote devices,
    //                       local changes of local kv store.
    DB_API virtual DBStatus RegisterObserver(const Key &key, unsigned int mode, KvStoreObserver *observer) = 0;

    // UnRegister the registered observer.
    DB_API virtual DBStatus UnRegisterObserver(const KvStoreObserver *observer) = 0;

    // Remove the device data synced from remote.
    DB_API virtual DBStatus RemoveDeviceData(const std::string &device) = 0;

    // Other interfaces
    DB_API virtual std::string GetStoreId() const = 0;

    // Sync function interface, if wait set true, this function will be blocked until sync finished
    DB_API virtual DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        bool wait = false) = 0;

    // Special pragma interface, see PragmaCmd and PragmaData,
    DB_API virtual DBStatus Pragma(PragmaCmd cmd, PragmaData &paramData) = 0;

    // Set the conflict notifier for getting the specified type conflict data.
    DB_API virtual DBStatus SetConflictNotifier(int conflictType,
        const KvStoreNbConflictNotifier &notifier) = 0;

    // Used to rekey the database.
    // Warning rekey may reopen database file, file handle may lose while locked
    DB_API virtual DBStatus Rekey(const CipherPassword &password) = 0;

    // Empty passwords represent non-encrypted files.
    // Export existing database files to a specified database file in the specified directory.
    DB_API virtual DBStatus Export(const std::string &filePath, const CipherPassword &passwd, bool force = false) = 0;

    // Import the existing database files to the specified database file in the specified directory.
    // Warning Import may reopen database file in locked state
    DB_API virtual DBStatus Import(const std::string &filePath, const CipherPassword &passwd,
        bool isNeedIntegrityCheck = false) = 0;

    // Start a transaction
    DB_API virtual DBStatus StartTransaction() = 0;

    // Commit a transaction
    DB_API virtual DBStatus Commit() = 0;

    // Rollback a transaction
    DB_API virtual DBStatus Rollback() = 0;

    // Put a batch of entries into the local zone of this store.
    DB_API virtual DBStatus PutLocalBatch(const std::vector<Entry> &entries) = 0;

    // Delete a batch of entries from the local zone of this store according to the keys.
    DB_API virtual DBStatus DeleteLocalBatch(const std::vector<Key> &keys) = 0;

    // Get the SecurityOption of this kvStore.
    DB_API virtual DBStatus GetSecurityOption(SecurityOption &option) const = 0;

    // Set a notify callback, it will be called when remote push or push_pull finished.
    // If Repeat set, subject to the last time.
    // If set nullptr, means unregister the notify.
    DB_API virtual DBStatus SetRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier) = 0;

    // Sync function interface, if wait set true, this function will be blocked until sync finished.
    // Param query used to filter the records to be synchronized.
    // Now just support push mode and query by prefixKey.
    // If Query.limit is used, its query cache will not be recorded, In the same way below,
    // the synchronization will still take the full amount.
    DB_API virtual DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait) = 0;

    // Sync with device, provides sync count information
    DB_API virtual DBStatus Sync(const DeviceSyncOption &option, const DeviceSyncProcessCallback &onProcess)
    {
        return OK;
    };

    // Cancel sync by syncId
    DB_API virtual DBStatus CancelSync(uint32_t syncId)
    {
        return OK;
    };

    // Check the integrity of this kvStore.
    DB_API virtual DBStatus CheckIntegrity() const = 0;

    // Set an equal identifier for this database, After this called, send msg to the target will use this identifier
    DB_API virtual DBStatus SetEqualIdentifier(const std::string &identifier,
        const std::vector<std::string> &targets) = 0;

    // This API is not recommended. Before using this API, you need to understand the API usage rules.
    // Set pushdatainterceptor. The interceptor works when send data.
    DB_API virtual DBStatus SetPushDataInterceptor(const PushDataInterceptor &interceptor) = 0;

    // Register a subscriber query on peer devices. The data in the peer device meets the subscriber query condition
    // will automatically push to the local device when it's changed.
    DB_API virtual DBStatus SubscribeRemoteQuery(const std::vector<std::string> &devices,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait) = 0;

    // Unregister a subscriber query on peer devices.
    DB_API virtual DBStatus UnSubscribeRemoteQuery(const std::vector<std::string> &devices,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait) = 0;

    // Remove all other device data synced from other remote devices.
    DB_API virtual DBStatus RemoveDeviceData() = 0;

    // Get keys from the public zone of this store by key prefix.
    // If 'keyPrefix' is empty, It would return all the keys in the zone.
    DB_API virtual DBStatus GetKeys(const Key &keyPrefix, std::vector<Key> &keys) const = 0;

    // calculate full sync sync data size after Serialize;
    // return 1M while sync data size is larger than 1M, otherwise return actualy size
    DB_API virtual size_t GetSyncDataSize(const std::string &device) const = 0;

    // update all key in sync_data which is not deleted data
    DB_API virtual DBStatus UpdateKey(const UpdateKeyCallback &callback) = 0;

    // get full watermark by device which is not contain query watermark
    // this api get watermark from cache which reload in get kv store
    DB_API virtual std::pair<DBStatus, WatermarkInfo> GetWatermarkInfo(const std::string &device) = 0;

    // sync with cloud
    DB_API virtual DBStatus Sync([[gnu::unused]] const CloudSyncOption &option,
        [[gnu::unused]] const SyncProcessCallback &onProcess)
    {
        return OK;
    }

    // set cloud db with user
    DB_API virtual DBStatus SetCloudDB(
        [[gnu::unused]] const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs)
    {
        return OK;
    }

    // set cloud schema
    DB_API virtual DBStatus SetCloudDbSchema([[gnu::unused]] const std::map<std::string, DataBaseSchema> &schema)
    {
        return OK;
    }

    // remove device data for cloud
    DB_API virtual DBStatus RemoveDeviceData(const std::string &device, ClearMode mode) = 0;

    // remove device data for cloud and user
    DB_API virtual DBStatus RemoveDeviceData(const std::string &device, const std::string &user, ClearMode mode) = 0;

    // get all sync task count
    DB_API virtual int32_t GetTaskCount() = 0;

    // set generate cloud version callback
    DB_API virtual void SetGenCloudVersionCallback([[gnu::unused]] const GenerateCloudVersionCallback &callback)
    {
        return;
    }

    // get cloud version by device
    DB_API virtual std::pair<DBStatus, std::map<std::string, std::string>> GetCloudVersion(
        [[gnu::unused]] const std::string &device)
    {
        return {};
    }

    // This API is not recommended. Before using this API, you need to understand the API usage rules.
    // The interceptor works when receive data.
    DB_API virtual DBStatus SetReceiveDataInterceptor(const DataInterceptor &interceptor) = 0;

    // set the config for cloud sync task
    DB_API virtual DBStatus SetCloudSyncConfig([[gnu::unused]] const CloudSyncConfig &config)
    {
        return OK;
    }

    // Get Entries by the device(uuid) in sync_data.
    // If device is empty, it would return all the entries which was written by local device.
    DB_API virtual DBStatus GetDeviceEntries(const std::string &device, std::vector<Entry> &entries) const = 0;

    DB_API virtual DatabaseStatus GetDatabaseStatus() const
    {
        return {};
    }

    DB_API virtual DBStatus OperateDataStatus([[gnu::unused]] uint32_t dataOperator)
    {
        return OK;
    }

    DB_API virtual DBStatus ClearMetaData([[gnu::unused]] ClearKvMetaDataOption option)
    {
        return OK;
    }
};
} // namespace DistributedDB

#endif // KV_STORE_NB_DELEGATE_H
