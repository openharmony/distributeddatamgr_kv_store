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

#include "distributed_kv_store_ffi.h"
#include "distributed_kv_store_impl.h"
#include "distributed_kv_store_utils.h"

using namespace OHOS::FFI;
using namespace OHOS::DistributedKVStore;

namespace OHOS {
namespace DistributedKVStore {
extern "C" {
int64_t FfiOHOSDistributedKVStoreCreateKVManager(const char* boudleName, OHOS::AbilityRuntime::Context* context)
{
    if (context == nullptr) {
        return -1;
    }
    auto nativeCJKVManager = FFIData::Create<CJKVManager>(boudleName, context);
    if (nativeCJKVManager == nullptr) {
        return -1;
    }
    return nativeCJKVManager->GetID();
}

int64_t FfiOHOSDistributedKVStoreGetKVStore(int64_t id, const char* storeId, CJOptions options, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJKVManager>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return -1;
    }
    return instance->GetKVStore(storeId, options, *errCode);
}

int32_t FfiOHOSDistributedKVStoreCloseKVStore(int64_t id, const char* appId, const char* storeId)
{
    auto instance = FFIData::GetData<CJKVManager>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->CloseKVStore(appId, storeId);
}

int32_t FfiOHOSDistributedKVStoreDeleteKVStore(int64_t id, const char* appId, const char* storeId)
{
    auto instance = FFIData::GetData<CJKVManager>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->DeleteKVStore(appId, storeId);
}

CArrStr FfiOHOSDistributedKVStoreGetAllKVStoreId(int64_t id, const char* appId, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJKVManager>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return CArrStr{};
    }
    return instance->GetAllKVStoreId(appId, *errCode);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStorePut(int64_t id, const char* key, ValueType value)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->Put(key, value);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStorePutBatch(int64_t id, const CArrEntry cArrEntry)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->PutBatch(cArrEntry);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreDelete(int64_t id, const char* key)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->Delete(key);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreDeleteBatch(int64_t id, const CArrStr cArrStr)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->DeleteBatch(cArrStr);
}

ValueType FfiOHOSDistributedKVStoreSingleKVStoreGet(int64_t id, const char* key, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return ValueType{};
    }
    return instance->Get(key, *errCode);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreBackup(int64_t id, const char* file)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->Backup(file);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreRestore(int64_t id, const char* file)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->Restore(file);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreStartTransaction(int64_t id)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->StartTransaction();
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreCommit(int64_t id)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->Commit();
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreRollback(int64_t id)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->Rollback();
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreEnableSync(int64_t id, bool enabled)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->EnableSync(enabled);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreSetSyncParam(int64_t id, uint32_t defaultAllowedDelayMs)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->SetSyncParam(defaultAllowedDelayMs);
}


int64_t FfiOHOSDistributedKVStoreQueryConstructor()
{
    auto nativeCJQuery = FFIData::Create<CQuery>();
    if (nativeCJQuery == nullptr) {
        return -1;
    }
    return nativeCJQuery->GetID();
}

void FfiOHOSDistributedKVStoreQueryReset(int64_t id)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->Reset();
}

void FfiOHOSDistributedKVStoreQueryEqualTo(int64_t id, const char* field, ValueType value)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->EqualTo(field, value);
}

ValueType FfiOHOSDistributedKVStoreDeviceKVStoreGet(int64_t id, const char* deviceId, const char* key, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJDeviceKVStore>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return ValueType{};
    }
    return instance->Get(deviceId, key, *errCode);
}

CArrEntry FfiOHOSDistributedKVStoreDeviceKVStoreGetEntries(int64_t id, const char* deviceId, const char* keyPrefix,
    int32_t* errCode)
{
    auto instance = FFIData::GetData<CJDeviceKVStore>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return CArrEntry{};
    }
    return instance->GetEntries(deviceId, keyPrefix, *errCode);
}

CArrEntry FfiOHOSDistributedKVStoreDeviceKVStoreGetEntriesQuery(int64_t id, const char* deviceId, int64_t queryId,
    int32_t* errCode)
{
    auto instance = FFIData::GetData<CJDeviceKVStore>(id);
    auto query = FFIData::GetData<CQuery>(queryId);
    if (instance == nullptr || query == nullptr) {
        *errCode = -1;
        return CArrEntry{};
    }
    return instance->GetEntries(deviceId, query, *errCode);
}

int64_t FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSet(int64_t id, const char* deviceId, const char* keyPrefix,
    int32_t* errCode)
{
    auto instance = FFIData::GetData<CJDeviceKVStore>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return -1;
    }
    return instance->GetResultSet(deviceId, keyPrefix, *errCode);
}

int64_t FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSetQuery(int64_t id, const char* deviceId, int64_t queryId,
    int32_t* errCode)
{
    auto instance = FFIData::GetData<CJDeviceKVStore>(id);
    auto query = FFIData::GetData<CQuery>(queryId);
    if (instance == nullptr || query == nullptr) {
        *errCode = -1;
        return -1;
    }
    return instance->GetResultSetQuery(deviceId, query, *errCode);
}

int32_t FfiOHOSDistributedKVStoreDeviceKVStoreGetResultSize(int64_t id, const char* deviceId, int64_t queryId,
    int32_t* errCode)
{
    auto instance = FFIData::GetData<CJDeviceKVStore>(id);
    auto query = FFIData::GetData<CQuery>(queryId);
    if (instance == nullptr || query == nullptr) {
        *errCode = -1;
        return -1;
    }
    return instance->GetResultSize(deviceId, query, *errCode);
}

int32_t FfiOHOSDistributedKVStoreKVStoreResultSetGetCount(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->GetCount();
}
}
}
}
