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
#ifndef RD_SINGLE_VER_NATURAL_STORE_H
#define RD_SINGLE_VER_NATURAL_STORE_H
#include <atomic>
#include <mutex>

#include "db_common.h"
#include "grd_kv_api.h"
#include "kvdb_utils.h"
#include "rd_single_ver_storage_engine.h"
#include "rd_single_ver_storage_executor.h"
#include "single_ver_natural_store.h"
#include "storage_engine_manager.h"

namespace DistributedDB {
class RdSingleVerNaturalStore : public SingleVerNaturalStore {
public:
    RdSingleVerNaturalStore();
    ~RdSingleVerNaturalStore() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(RdSingleVerNaturalStore);

    // Open the database
    int Open(const KvDBProperties &kvDBProp) override;

    // Invoked automatically when connection count is zero
    void Close() override;

    // Create a connection object.
    GenericKvDBConnection *NewConnection(int &errCode) override;

    // Get interface type of this kvdb.
    int GetInterfaceType() const override;

    // Get the interface ref-count, in order to access asynchronously.
    void IncRefCount() override;

    // Drop the interface ref-count.
    void DecRefCount() override;

    // Get the identifier of this kvdb.
    std::vector<uint8_t> GetIdentifier() const override;

    // Get interface for syncer.
    IKvDBSyncInterface *GetSyncInterface() override;

    int GetMetaData(const Key &key, Value &value) const override;

    int PutMetaData(const Key &key, const Value &value, bool isInTransaction) override;

    // Delete multiple meta data records in a transaction.
    int DeleteMetaData(const std::vector<Key> &keys) override;

    // Delete multiple meta data records with key prefix in a transaction.
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const override;

    int GetAllMetaKeys(std::vector<Key> &keys) const override;

    void GetMaxTimestamp(Timestamp &stamp) const override;

    void SetMaxTimestamp(Timestamp timestamp);

    int Rekey(const CipherPassword &passwd) override;

    int Export(const std::string &filePath, const CipherPassword &passwd) override;

    int Import(const std::string &filePath, const CipherPassword &passwd) override;

    RdSingleVerStorageExecutor *GetHandle(bool isWrite, int &errCode,
        OperatePerm perm = OperatePerm::NORMAL_PERM) const;

    void ReleaseHandle(RdSingleVerStorageExecutor *&handle) const;

    int TransObserverTypeToRegisterFunctionType(int observerType, RegisterFuncType &type) const override;

    SchemaObject GetSchemaInfo() const override;

    bool CheckCompatible(const std::string &schema, uint8_t type) const override;

    Timestamp GetCurrentTimestamp();

    SchemaObject GetSchemaObject() const;

    const SchemaObject &GetSchemaObjectConstRef() const;

    const KvDBProperties &GetDbProperties() const override;

    int GetKvDBSize(const KvDBProperties &properties, uint64_t &size) const override;

    KvDBProperties &GetDbPropertyForUpdate();

    int InitDatabaseContext(const KvDBProperties &kvDBProp);

    int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier);

    int SetAutoLifeCycleTime(uint32_t time);

    bool IsDataMigrating() const override;

    int TriggerToMigrateData() const;

    int CheckReadDataControlled() const;

    bool IsCacheDBMode() const;

    bool IsExtendedCacheDBMode() const;

    void IncreaseCacheRecordVersion() const;

    uint64_t GetCacheRecordVersion() const;

    uint64_t GetAndIncreaseCacheRecordVersion() const;

    int CheckIntegrity() const override;

    int GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const override;

    void SetDataInterceptor(const PushDataInterceptor &interceptor) override;

    int SetMaxLogSize(uint64_t limit);

    uint64_t GetMaxLogSize() const;

    void Dump(int fd) override;

    void WakeUpSyncer() override;

    void CommitNotify(int notifyEvent, KvDBCommitNotifyFilterAbleData *data) override;

private:

    int GetAndInitStorageEngine(const KvDBProperties &kvDBProp);

    int RegisterNotification();

    void ReleaseResources();

    void InitDataBaseOption(const KvDBProperties &kvDBProp, OpenDbProperties &option);
    mutable std::shared_mutex engineMutex_;
    RdSingleVerStorageEngine *storageEngine_;
    bool notificationEventsRegistered_;
};
} // namespace DistributedDB
#endif // RD_SINGLE_VER_NATURAL_STORE_H
