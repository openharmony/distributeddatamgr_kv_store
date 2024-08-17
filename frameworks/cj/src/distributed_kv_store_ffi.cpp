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

int32_t FfiOHOSDistributedKVStoreOnDistributedDataServiceDie(int64_t id, void (*callbackId)())
{
    auto instance = FFIData::GetData<CJKVManager>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->OnDistributedDataServiceDie(callbackId);
}

int32_t FfiOHOSDistributedKVStoreOffDistributedDataServiceDie(int64_t id, void (*callbackId)())
{
    auto instance = FFIData::GetData<CJKVManager>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->OffDistributedDataServiceDie(callbackId);
}

int32_t FfiOHOSDistributedKVStoreOffAllDistributedDataServiceDie(int64_t id)
{
    auto instance = FFIData::GetData<CJKVManager>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->OffAllDistributedDataServiceDie();
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

int32_t FfiOHOSDistributedKVStoreSingleKVStoreRemoveDeviceData(int64_t id, const char* deviceId)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->RemoveDeviceData(deviceId);
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

CArrEntry FfiOHOSDistributedKVStoreSingleKVStoreGetEntriesByQuery(int64_t id, int64_t queryId, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    auto query = FFIData::GetData<CQuery>(queryId);
    if (instance == nullptr || query == nullptr) {
        *errCode = -1;
        return CArrEntry{};
    }
    return instance->GetEntries(query, *errCode);
}

CArrEntry FfiOHOSDistributedKVStoreSingleKVStoreGetEntriesByString(int64_t id, const char* prefix, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return CArrEntry{};
    }
    return instance->GetEntries(prefix, *errCode);
}

int64_t FfiOHOSDistributedKVStoreSingleKVStoreGetResultSetByString(int64_t id, const char* keyPrefix, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return 0;
    }
    return instance->GetResultSetByString(keyPrefix, *errCode);
}

int64_t FfiOHOSDistributedKVStoreSingleKVStoreGetResultSetByQuery(int64_t id, int64_t queryId, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    auto query = FFIData::GetData<CQuery>(queryId);
    if (instance == nullptr || query == nullptr) {
        *errCode = -1;
        return 0;
    }
    return instance->GetResultSetByQuery(query, *errCode);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreCloseResultSet(int64_t id, int64_t resultSetId)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    auto resultSet = FFIData::GetData<CKvStoreResultSet>(resultSetId);
    if (instance == nullptr || resultSet == nullptr) {
        return -1;
    }
    return instance->CloseResultSet(resultSet);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreGetResultSize(int64_t id, int64_t queryId, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    auto query = FFIData::GetData<CQuery>(queryId);
    if (instance == nullptr || query == nullptr) {
        *errCode = -1;
        return 0;
    }
    return instance->GetResultSize(query, *errCode);
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

CStringNum FfiOHOSDistributedKVStoreSingleKVStoreDeleteBackup(int64_t id,
    const CArrStr cArrStr, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return CStringNum{};
    }
    return instance->DeleteBackup(cArrStr, *errCode);
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

int32_t FfiOHOSDistributedKVStoreSingleKVStoreSetSyncRange(int64_t id, const CArrStr localLabels,
    const CArrStr remoteSupportLabels)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->SetSyncRange(localLabels, remoteSupportLabels);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreSetSyncParam(int64_t id, uint32_t defaultAllowedDelayMs)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->SetSyncParam(defaultAllowedDelayMs);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreSync(int64_t id, const CArrStr deviceIds, uint8_t mode, uint32_t delayMs)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->Sync(deviceIds, mode, delayMs);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreSyncByQuery(int64_t id, const CArrStr deviceIds, int64_t queryId,
    uint8_t mode, uint32_t delayMs)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    auto query = FFIData::GetData<CQuery>(queryId);
    if (instance == nullptr || query == nullptr) {
        return -1;
    }
    return instance->SyncByQuery(deviceIds, query, mode, delayMs);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreOnDataChange(int64_t id, uint8_t type,
    void (*callbackId)(const CChangeNotification valueRef))
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->OnDataChange(type, callbackId);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreOffDataChange(int64_t id,
    void (*callbackId)(const CChangeNotification valueRef))
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->OffDataChange(callbackId);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreOffAllDataChange(int64_t id)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->OffAllDataChange();
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreOnSyncComplete(int64_t id, void (*callbackId)(const CStringNum valueRef))
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->OnSyncComplete(callbackId);
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreOffAllSyncComplete(int64_t id)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->OffAllSyncComplete();
}

int32_t FfiOHOSDistributedKVStoreSingleKVStoreGetSecurityLevel(int64_t id, int32_t* errCode)
{
    auto instance = FFIData::GetData<CJSingleKVStore>(id);
    if (instance == nullptr) {
        *errCode = -1;
        return 0;
    }
    return instance->GetSecurityLevel(*errCode);
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

void FfiOHOSDistributedKVStoreQueryNotEqualTo(int64_t id, const char* field, ValueType value)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->NotEqualTo(field, value);
}

void FfiOHOSDistributedKVStoreQueryGreaterThan(int64_t id, const char* field, ValueType value)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->GreaterThan(field, value);
}

void FfiOHOSDistributedKVStoreQueryLessThan(int64_t id, const char* field, ValueType value)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->LessThan(field, value);
}

void FfiOHOSDistributedKVStoreQueryGreaterThanOrEqualTo(int64_t id, const char* field, ValueType value)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->GreaterThanOrEqualTo(field, value);
}

void FfiOHOSDistributedKVStoreQueryLessThanOrEqualTo(int64_t id, const char* field, ValueType value)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->LessThanOrEqualTo(field, value);
}

void FfiOHOSDistributedKVStoreQueryIsNull(int64_t id, const char* field)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->IsNull(field);
}

void FfiOHOSDistributedKVStoreQueryInNumber(int64_t id, const char* field, const CArrNumber valueList)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->InNumber(field, valueList);
}

void FfiOHOSDistributedKVStoreQueryInString(int64_t id, const char* field, const CArrStr valueList)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->InString(field, valueList);
}

void FfiOHOSDistributedKVStoreQueryNotInNumber(int64_t id, const char* field, const CArrNumber valueList)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->NotInNumber(field, valueList);
}

void FfiOHOSDistributedKVStoreQueryNotInString(int64_t id, const char* field, const CArrStr valueList)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->NotInString(field, valueList);
}

void FfiOHOSDistributedKVStoreQueryLike(int64_t id, const char* field, const char* value)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->Like(field, value);
}

void FfiOHOSDistributedKVStoreQueryUnlike(int64_t id, const char* field, const char* value)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->Unlike(field, value);
}

void FfiOHOSDistributedKVStoreQueryAnd(int64_t id)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->And();
}

void FfiOHOSDistributedKVStoreQueryOr(int64_t id)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->Or();
}

void FfiOHOSDistributedKVStoreQueryOrderByAsc(int64_t id, const char* field)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->OrderByAsc(field);
}

void FfiOHOSDistributedKVStoreQueryOrderByDesc(int64_t id, const char* field)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->OrderByDesc(field);
}

void FfiOHOSDistributedKVStoreQueryLimit(int64_t id, int32_t total, int32_t offset)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->Limit(total, offset);
}

void FfiOHOSDistributedKVStoreQueryIsNotNull(int64_t id, const char* field)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->IsNotNull(field);
}

void FfiOHOSDistributedKVStoreQueryBeginGroup(int64_t id)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->BeginGroup();
}

void FfiOHOSDistributedKVStoreQueryEndGroup(int64_t id)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->EndGroup();
}

void FfiOHOSDistributedKVStoreQueryPrefixKey(int64_t id, const char* prefix)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->PrefixKey(prefix);
}

void FfiOHOSDistributedKVStoreQuerySetSuggestIndex(int64_t id, const char* index)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->SetSuggestIndex(index);
}

void FfiOHOSDistributedKVStoreQueryDeviceId(int64_t id, const char* deviceId)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return;
    }
    return instance->DeviceId(deviceId);
}

const char* FfiOHOSDistributedKVStoreQueryGetSqlLike(int64_t id)
{
    auto instance = FFIData::GetData<CQuery>(id);
    if (instance == nullptr) {
        return nullptr;
    }
    return MallocCString(instance->GetSqlLike());
}

ValueType FfiOHOSDistributedKVStoreDeviceKVStoreGet(int64_t id, const char* deviceId,
    const char* key, int32_t* errCode)
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

int32_t FfiOHOSDistributedKVStoreKVStoreResultSetGetPosition(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return -1;
    }
    return instance->GetPosition();
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToFirst(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->MoveToFirst();
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToLast(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->MoveToLast();
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToNext(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->MoveToNext();
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToPrevious(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->MoveToPrevious();
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetMove(int64_t id, int32_t offset)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->Move(offset);
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetMoveToPosition(int64_t id, int32_t position)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->MoveToPosition(position);
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetIsFirst(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->IsFirst();
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetIsLast(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->IsLast();
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetIsBeforeFirst(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->IsBeforeFirst();
}

bool FfiOHOSDistributedKVStoreKVStoreResultSetIsAfterLast(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return false;
    }
    return instance->IsAfterLast();
}

CEntry FfiOHOSDistributedKVStoreKVStoreResultSetGetEntry(int64_t id)
{
    auto instance = FFIData::GetData<OHOS::DistributedKVStore::CKvStoreResultSet>(id);
    if (instance == nullptr) {
        return CEntry{};
    }
    return instance->GetEntry();
}
}
}
}
