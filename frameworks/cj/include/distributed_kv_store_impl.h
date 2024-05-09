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

struct ValueType {
    char* string;
    int32_t integer;
    float flo;
    CArrByte byteArray;
    bool boolean;
    double dou;
    uint8_t tag;
};

struct CEntry {
    char *key;
    ValueType value;
};

struct CArrEntry {
    CEntry* head;
    int64_t size;
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
private:
    DistributedKv::DistributedKvDataManager kvDataManager_ {};
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
    ValueType Get(const std::string &key, int32_t& errCode);
    int32_t Backup(const std::string &file);
    int32_t Restore(const std::string &file);
    int32_t StartTransaction();
    int32_t Commit();
    int32_t Rollback();
    int32_t EnableSync(bool enabled);
    int32_t SetSyncParam(uint32_t defaultAllowedDelayMs);

private:
    /* private non-static members */
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

class CKvStoreResultSet : public OHOS::FFI::FFIData {
public:
    OHOS::FFI::RuntimeType* GetRuntimeType() override
    {
        return GetClassType();
    }

    explicit CKvStoreResultSet(std::shared_ptr<DistributedKv::KvStoreResultSet> kvResultSet);

    std::shared_ptr<DistributedKv::KvStoreResultSet> GetKvStoreResultSet();

    int32_t GetCount();

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
}
} // namespace OHOS::DistributedKVStore

#endif