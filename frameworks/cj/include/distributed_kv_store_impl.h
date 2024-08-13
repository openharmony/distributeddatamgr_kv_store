/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DISTRIBUTED_KV_STORE_IMPL_H
#define DISTRIBUTED_KV_STORE_IMPL_H

#include <string>
#include <mutex>
#include "distributed_kv_data_manager.h"
#include "kvstore_death_recipient.h"
#include "distributed_kv_store_log.h"
#include "ability_context_impl.h"
#include "kvstore_result_set.h"

namespace OHOS {
namespace DistributedKVStore {
using namespace OHOS::DistributedKv;
enum {
    /* exported cj SubscribeType  is (DistributedKv::SubscribeType-1) */
    SUBSCRIBE_LOCAL = 0,        /* i.e. SubscribeType::SUBSCRIBE_TYPE_LOCAL-1  */
    SUBSCRIBE_REMOTE = 1,       /* i.e. SubscribeType::SUBSCRIBE_TYPE_REMOTE-1 */
    SUBSCRIBE_LOCAL_REMOTE = 2, /* i.e. SubscribeType::SUBSCRIBE_TYPE_ALL-1   */
    SUBSCRIBE_COUNT = 3
};

struct ContextParam {
    std::string baseDir = "";
    std::string hapName = "";
    int32_t area = DistributedKv::Area::EL1;
};

struct CJFieldNode {
    bool nullable;
    char* defaultString;
    int32_t type;
};

struct CJSchema {
    CJFieldNode root;
    char** indexes;
    int32_t indexesSize;
    int32_t mode;
    int32_t skip;
};

struct CJOptions {
    bool createIfMissing;
    bool encrypt;
    bool backup;
    bool autoSync;
    int32_t kvStoreType;
    int32_t securityLevel;
    CJSchema schema;
};

struct CArrByte {
    u_int8_t* head;
    int64_t size;
};

struct CArrStr {
    char** head;
    int64_t size;
};

struct CArrNumber {
    int32_t* intList;
    float* floatList;
    double* doubleList;
    int64_t size;
    uint8_t tag;
};

struct ValueType {
    char* string;
    int32_t integer;
    float flo;
    CArrByte byteArray;
    bool boolean;
    double dou;
    uint8_t tag;
};

struct CStringNum {
    char** headChar;
    int32_t* headNum;
    int64_t size;
};

struct CEntry {
    char *key;
    ValueType value;
};

struct CArrEntry {
    CEntry* head;
    int64_t size;
};

struct CChangeNotification {
    CArrEntry insertEntries;
    CArrEntry updateEntries;
    CArrEntry deleteEntries;
    char* deviceId;
};

class CKvStoreResultSet : public OHOS::FFI::FFIData {
public:
    OHOS::FFI::RuntimeType* GetRuntimeType() override
    {
        return GetClassType();
    }

    explicit CKvStoreResultSet(std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet);

    std::shared_ptr<DistributedKv::KvStoreResultSet> GetKvStoreResultSet();

    int32_t GetCount();

    int32_t GetPosition();

    bool MoveToFirst();

    bool MoveToLast();

    bool MoveToNext();

    bool MoveToPrevious();

    bool Move(int32_t offset);

    bool MoveToPosition(int32_t position);

    bool IsFirst();

    bool IsLast();

    bool IsBeforeFirst();

    bool IsAfterLast();

    CEntry GetEntry();

private:
    std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet;
    friend class OHOS::FFI::RuntimeType;
    friend class OHOS::FFI::TypeBase;
    static OHOS::FFI::RuntimeType* GetClassType()
    {
        static OHOS::FFI::RuntimeType runtimeType =
            OHOS::FFI::RuntimeType::Create<OHOS::FFI::FFIData>("CKvStoreResultSet");
        return &runtimeType;
    }
};

class CJKVManager : public OHOS::FFI::FFIData {
public:
    OHOS::FFI::RuntimeType* GetRuntimeType() override
    {
        return GetClassType();
    }

    CJKVManager();
    CJKVManager(const char* boudleName, OHOS::AbilityRuntime::Context* context);

    uint64_t GetKVStore(const char* cStoreId, const CJOptions cjOptions, int32_t& errCode);
    int32_t CloseKVStore(const char* appId, const char* storeId);
    int32_t DeleteKVStore(const char* appId, const char* storeId);
    CArrStr GetAllKVStoreId(const char* appId, int32_t& errCode);
    int32_t OnDistributedDataServiceDie(void (*callbackId)());
    int32_t OffDistributedDataServiceDie(void (*callbackId)());
    int32_t OffAllDistributedDataServiceDie();

private:
    class DeathRecipient : public DistributedKv::KvStoreDeathRecipient {
    public:
        DeathRecipient(void (*callbackId)(), const std::function<void()>& callbackRef);
        virtual ~DeathRecipient() = default;
        void OnRemoteDied() override;
        void (*m_callbackId)();
        std::function<void()> m_callbackRef;
    };
    DistributedKv::DistributedKvDataManager kvDataManager_ {};
    std::mutex deathMutex_ {};
    std::list<std::shared_ptr<DeathRecipient>> deathRecipient_;
    std::string bundleName_ {};
    std::shared_ptr<ContextParam> param_;
    friend class OHOS::FFI::RuntimeType;
    friend class OHOS::FFI::TypeBase;
    static OHOS::FFI::RuntimeType* GetClassType()
    {
        static OHOS::FFI::RuntimeType runtimeType = OHOS::FFI::RuntimeType::Create<OHOS::FFI::FFIData>("CJKVManager");
        return &runtimeType;
    }
};
class CQuery : public OHOS::FFI::FFIData {
public:
    OHOS::FFI::RuntimeType* GetRuntimeType() override
    {
        return GetClassType();
    }

    CQuery() {};

    const DistributedKv::DataQuery& GetDataQuery() const;

    void Reset();

    void EqualTo(const std::string &field, ValueType &value);

    void NotEqualTo(const std::string &field, ValueType &value);

    void GreaterThan(const std::string &field, ValueType &value);

    void LessThan(const std::string &field, ValueType &value);

    void GreaterThanOrEqualTo(const std::string &field, ValueType &value);

    void LessThanOrEqualTo(const std::string &field, ValueType &value);

    void IsNull(const std::string &field);

    void InNumber(const std::string &field, const CArrNumber &valueList);

    void InString(const std::string &field, const CArrStr &valueList);

    void NotInNumber(const std::string &field, const CArrNumber &valueList);

    void NotInString(const std::string &field, const CArrStr &valueList);

    void Like(const std::string &field, const std::string &value);

    void Unlike(const std::string &field, const std::string &value);

    void And();

    void Or();

    void OrderByAsc(const std::string &field);

    void OrderByDesc(const std::string &field);

    void Limit(int32_t total, int32_t offset);

    void IsNotNull(const std::string &field);

    void BeginGroup();

    void EndGroup();

    void PrefixKey(const std::string &prefix);

    void SetSuggestIndex(const std::string &index);

    void DeviceId(const std::string &deviceId);

    const std::string GetSqlLike();

private:
    DistributedKv::DataQuery query_;
    friend class OHOS::FFI::RuntimeType;
    friend class OHOS::FFI::TypeBase;
    static OHOS::FFI::RuntimeType* GetClassType()
    {
        static OHOS::FFI::RuntimeType runtimeType = OHOS::FFI::RuntimeType::Create<OHOS::FFI::FFIData>("CQuery");
        return &runtimeType;
    }
};

class CJSingleKVStore : public OHOS::FFI::FFIData {
public:
    OHOS::FFI::RuntimeType* GetRuntimeType() override
    {
        return GetClassType();
    }

    explicit CJSingleKVStore(const std::string& storeId);

    std::shared_ptr<DistributedKv::SingleKvStore> GetKvStorePtr();
    void SetKvStorePtr(std::shared_ptr<DistributedKv::SingleKvStore> kvStore);
    void SetContextParam(std::shared_ptr<ContextParam> param);
    int32_t Put(const std::string &key, const ValueType &value);
    int32_t PutBatch(const CArrEntry &cArrEntry);
    int32_t Delete(const std::string &key);
    int32_t DeleteBatch(const CArrStr &cArrStr);
    int32_t RemoveDeviceData(const std::string &deviceId);
    ValueType Get(const std::string &key, int32_t& errCode);
    CArrEntry GetEntries(OHOS::sptr<CQuery> query, int32_t& errCode);
    CArrEntry GetEntries(const std::string &keyPrefix, int32_t& errCode);
    int64_t GetResultSetByString(const std::string &keyPrefix, int32_t& errCode);
    int64_t GetResultSetByQuery(OHOS::sptr<CQuery> query, int32_t& errCode);
    int32_t CloseResultSet(OHOS::sptr<CKvStoreResultSet> resultSet);
    int32_t GetResultSize(OHOS::sptr<CQuery> query, int32_t& errCode);
    int32_t Backup(const std::string &file);
    int32_t Restore(const std::string &file);
    CStringNum DeleteBackup(const CArrStr &cArrStr, int32_t& errCode);
    int32_t StartTransaction();
    int32_t Commit();
    int32_t Rollback();
    int32_t EnableSync(bool enabled);
    int32_t SetSyncRange(const CArrStr &localLabels, const CArrStr &remoteSupportLabels);
    int32_t SetSyncParam(uint32_t defaultAllowedDelayMs);
    int32_t Sync(const CArrStr deviceIds, uint8_t mode, uint32_t delayMs);
    int32_t SyncByQuery(const CArrStr deviceIds, OHOS::sptr<CQuery> query, uint8_t mode, uint32_t delayMs);
    int32_t OnDataChange(uint8_t type, void (*callbackId)(const CChangeNotification valueRef));
    int32_t OffDataChange(void (*callbackId)(const CChangeNotification valueRef));
    int32_t OffAllDataChange();
    int32_t OnSyncComplete(void (*callbackId)(const CStringNum valueRef));
    int32_t OffAllSyncComplete();
    int32_t GetSecurityLevel(int32_t& errCode);

private:
    class DataObserver : public DistributedKv::KvStoreObserver {
    public:
        DataObserver(void (*callbackId)(const CChangeNotification),
            const std::function<void(DistributedKv::ChangeNotification)>& callbackRef);
        virtual ~DataObserver() = default;
        void OnChange(const DistributedKv::ChangeNotification& notification) override;
        void (*m_callbackId)(const CChangeNotification);
        std::function<void(DistributedKv::ChangeNotification)> m_callbackRef;
    };
    class SyncObserver : public DistributedKv::KvStoreSyncCallback {
    public:
        SyncObserver(void (*callbackId)(const CStringNum),
            const std::function<void(std::map<std::string, DistributedKv::Status>)>& callbackRef);
        virtual ~SyncObserver() = default;
        void SyncCompleted(const std::map<std::string, DistributedKv::Status>& results) override;
        void (*m_callbackId)(const CStringNum);
        std::function<void(std::map<std::string, DistributedKv::Status>)> m_callbackRef;
    };
    std::shared_ptr<DistributedKv::SingleKvStore> kvStore_ = nullptr;
    std::string storeId_;
    std::shared_ptr<ContextParam> param_ = nullptr;
    friend class OHOS::FFI::RuntimeType;
    friend class OHOS::FFI::TypeBase;
    static OHOS::FFI::RuntimeType* GetClassType()
    {
        static OHOS::FFI::RuntimeType runtimeType =
            OHOS::FFI::RuntimeType::Create<OHOS::FFI::FFIData>("CJSingleKVStore");
        return &runtimeType;
    }
    std::mutex listMutex_ {};
    std::list<std::shared_ptr<DataObserver>> dataObserver_[SUBSCRIBE_COUNT];
    std::list<std::shared_ptr<SyncObserver>> syncObservers_;
};
class CJDeviceKVStore : public CJSingleKVStore {
public:
    OHOS::FFI::RuntimeType* GetRuntimeType() override
    {
        return GetClassType();
    }

    explicit CJDeviceKVStore(const std::string& storeId);

    ValueType Get(const std::string &deviceId, const std::string &key, int32_t& errCode);

    CArrEntry GetEntriesByDataQuery(DistributedKVStore::DataQuery dataQuery, int32_t& errCode);

    CArrEntry GetEntries(const std::string &deviceId, const std::string &keyPrefix, int32_t& errCode);

    CArrEntry GetEntries(const std::string &deviceId, OHOS::sptr<CQuery> query, int32_t& errCode);

    int64_t GetResultSet(const std::string &deviceId, const std::string &keyPrefix, int32_t& errCode);

    int64_t GetResultSetQuery(const std::string &deviceId, OHOS::sptr<CQuery> query, int32_t& errCode);

    int32_t GetResultSize(const std::string &deviceId, OHOS::sptr<CQuery> query, int32_t& errCode);

private:
    friend class OHOS::FFI::RuntimeType;
    friend class OHOS::FFI::TypeBase;
    static OHOS::FFI::RuntimeType* GetClassType()
    {
        static OHOS::FFI::RuntimeType runtimeType =
            OHOS::FFI::RuntimeType::Create<OHOS::FFI::FFIData>("CJDeviceKVStore");
        return &runtimeType;
    }
};
}
} // namespace OHOS::DistributedKVStore

#endif